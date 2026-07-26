#include "tower_entity.h"
#include "tower_building.h"

namespace tower {

    Entity MakeEntity( EntityType type, glm::vec3 position ) {
        Entity entity = {}; // Clear to zero
        entity.type = type;
        entity.position = position;
        entity.rotation = glm::quat( 1, 0, 0, 0 );
        entity.scale = 1.0f;
        entity.rigidBody = RigidBody();
        entity.character = CharacterHandle();

        switch ( type ) {
            case ENTITY_TYPE_INVALID: {
                SL_ASSERT_MSG( false, "Making entity of invalid type" );
            } break;
            case ENTITY_TYPE_PROP: {
                entity.rigidBodyData.createRigidBody = true;
                entity.prop = PropData();
            } break;
            case ENTITY_TYPE_PLAYER: {
                entity.rigidBodyData.createRigidBody = true;
                entity.rigidBodyData.shape = RigidBodyShape::PlayerCapsule;
                entity.rigidBodyData.height = 1.8f;
                entity.rigidBodyData.radius = 0.35f;
                entity.rigidBodyData.motionType = BodyMotionType::Dynamic;
                entity.player = PlayerData();
            } break;
            case ENTITY_TYPE_REMOTE_PLAYER: {
                // No rigid body/character - purely a visual proxy, positioned
                // directly from network snapshots (see TowerGame::UpdateNetworking).
                entity.remotePlayer = RemotePlayerData();
            } break;
            case ENTITY_TYPE_BUILDING_FLOOR: {
                entity.rigidBodyData.createRigidBody = true;
                entity.rigidBodyData.shape = RigidBodyShape::Box;
                entity.rigidBodyData.halfExtents = { BuildingFloorSize * 0.5f, BuildingFloorThickness * 0.5f, BuildingFloorSize * 0.5f };
                entity.rigidBodyData.motionType = BodyMotionType::Static;
                entity.buildingFloor = BuildingFloorData();
            } break;
            case ENTITY_TYPE_BUILDING_WALL: {
                entity.rigidBodyData.createRigidBody = true;
                entity.rigidBodyData.shape = RigidBodyShape::Box;
                entity.rigidBodyData.halfExtents = { BuildingWallThickness * 0.5f, BuildingWallHeight * 0.5f, BuildingFloorSize * 0.5f };
                entity.rigidBodyData.motionType = BodyMotionType::Static;
                entity.buildingWall = BuildingWallData();
            } break;
        }

        return entity;
    }

    const char * ToString( EntityType type ) {
        switch ( type ) {
            case ENTITY_TYPE_INVALID:
                return "Invalid";
            case ENTITY_TYPE_PROP:
                return "Prop";
            case ENTITY_TYPE_PLAYER:
                return "Player";
            case ENTITY_TYPE_REMOTE_PLAYER:
                return "RemotePlayer";
            case ENTITY_TYPE_BUILDING_FLOOR:
                return "BuildingFloor";
            case ENTITY_TYPE_BUILDING_WALL:
                return "BuildingWall";
        }

        return "Unknown";
    }

} // namespace tower
