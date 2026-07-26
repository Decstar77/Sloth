#include "tower_building.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

namespace tower {

    glm::vec3 GetWallLocalOffset( BuildingSide side ) {
        f32 halfFloor = BuildingFloorSize * 0.5f;
        f32 wallCenterY = BuildingFloorThickness * 0.5f + BuildingWallHeight * 0.5f; // Base of the wall rests on the floor's top surface.

        switch ( side ) {
            case BuildingSide::PosX: return { halfFloor, wallCenterY, 0.0f };
            case BuildingSide::NegX: return { -halfFloor, wallCenterY, 0.0f };
            case BuildingSide::PosZ: return { 0.0f, wallCenterY, halfFloor };
            case BuildingSide::NegZ: return { 0.0f, wallCenterY, -halfFloor };
        }

        SL_ASSERT_MSG( false, "Unknown BuildingSide" );
        return glm::vec3( 0.0f );
    }

    glm::quat GetWallLocalRotation( BuildingSide side ) {
        switch ( side ) {
            case BuildingSide::PosX:
            case BuildingSide::NegX:
                return glm::quat( 1.0f, 0.0f, 0.0f, 0.0f ); // Identity - the wall shape is already built spanning Z.
            case BuildingSide::PosZ:
            case BuildingSide::NegZ:
                return glm::angleAxis( glm::half_pi<f32>(), glm::vec3( 0.0f, 1.0f, 0.0f ) ); // Spin 90 degrees to span X instead.
        }

        SL_ASSERT_MSG( false, "Unknown BuildingSide" );
        return glm::quat( 1.0f, 0.0f, 0.0f, 0.0f );
    }

    EntityId PlaceFloor( TowerWorld & world, const glm::vec3 & position, const glm::quat & rotation, const RenderModel & renderModel ) {
        Entity entity = MakeEntity( ENTITY_TYPE_BUILDING_FLOOR, position );
        entity.rotation = rotation;
        entity.renderModel = renderModel;
        return world.SpawnEntity( entity );
    }

    EntityId PlaceWall( TowerWorld & world, EntityId floorId, BuildingSide side, const RenderModel & renderModel ) {
        Entity * floor = world.GetEntity( floorId );
        if ( floor == nullptr || floor->type != ENTITY_TYPE_BUILDING_FLOOR ) {
            return INVALID_ENTITY_ID;
        }

        u8 sideIndex = static_cast<u8>( side );
        if ( floor->buildingFloor.wallOccupied[sideIndex] ) {
            return INVALID_ENTITY_ID;
        }

        Entity wall = MakeEntity( ENTITY_TYPE_BUILDING_WALL, floor->position + floor->rotation * GetWallLocalOffset( side ) );
        wall.rotation = floor->rotation * GetWallLocalRotation( side );
        wall.renderModel = renderModel;
        wall.buildingWall.floorId = floorId;
        wall.buildingWall.side = sideIndex;

        // Marked immediately rather than waiting for the (deferred) spawn to
        // apply - floor is a live pointer into the world's entity table
        // already, and this prevents placing two walls on the same socket
        // in the same frame before FlushPendingChanges runs.
        floor->buildingFloor.wallOccupied[sideIndex] = true;

        return world.SpawnEntity( wall );
    }

} // namespace tower
