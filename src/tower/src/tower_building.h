#pragma once

#include <core/sloth_defines.h>
#include <renderer/sloth_render_model.h>

#include "tower_entity.h"
#include "tower_world.h"

using namespace sloth;

namespace tower {

    // Rust-style building, kept deliberately minimal for now: a floor tile
    // can be placed anywhere at an arbitrary yaw (not grid-locked to a
    // world grid), and each of its 4 sides is a wall socket that can hold
    // exactly one wall. No durability/damage, no other piece types (stairs,
    // doors, roofs, ...), and - importantly - this is all client-local for
    // now: placed pieces are not yet replicated to other clients or the
    // server, unlike players. See TowerGame::UpdateBuilding for the
    // placement controls/raycasting that drives these.
    constexpr f32 BuildingFloorSize = 4.0f;       // Square tile, X/Z extent.
    constexpr f32 BuildingFloorThickness = 0.3f;
    constexpr f32 BuildingWallHeight = 3.0f;
    constexpr f32 BuildingWallThickness = 0.2f;

    // Offset from a floor tile's center to the center of the wall socket on
    // `side`, and the wall's rotation relative to the floor's own rotation -
    // both still in the floor's local space, so callers rotate/translate by
    // the floor's actual world transform on top of these. The wall mesh/
    // body is always built as (BuildingWallThickness x BuildingWallHeight x
    // BuildingFloorSize) - GetWallLocalRotation is what reorients that
    // shape to span the X sides instead of the Z sides it's built for.
    glm::vec3   GetWallLocalOffset( BuildingSide side );
    glm::quat   GetWallLocalRotation( BuildingSide side );

    // Spawns a new floor tile (deferred - see TowerWorld::SpawnEntity) at an
    // arbitrary world position/rotation.
    EntityId    PlaceFloor( TowerWorld & world, const glm::vec3 & position, const glm::quat & rotation, const RenderModel & renderModel );

    // Attempts to attach a wall to one side of an existing floor tile.
    // Fails (returns INVALID_ENTITY_ID, spawns nothing) if that floor
    // doesn't exist or that side is already occupied.
    EntityId    PlaceWall( TowerWorld & world, EntityId floorId, BuildingSide side, const RenderModel & renderModel );

} // namespace tower
