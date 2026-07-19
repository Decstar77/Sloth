#pragma once

#include <core/sloth_defines.h>
#include <core/sloth_list.h>
#include <physics/sloth_physics_world.h>
#include <renderer/sloth_render_model.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace sloth;

namespace dust {

    enum EntityType {
        ENTITY_TYPE_INVALID = 0,
        ENTITY_TYPE_PROP,
        ENTITY_TYPE_VEHICLE,
        ENTITY_TYPE_ORE_NODE,
        ENTITY_TYPE_SHOP,
    };

    struct EntityId {
        i32 index      = -1;
        i32 generation = 0;
    };

    constexpr EntityId INVALID_ENTITY_ID = {};

    inline bool operator==(const EntityId& a, const EntityId& b) { return a.index == b.index && a.generation == b.generation; }
    inline bool operator!=(const EntityId& a, const EntityId& b) { return !(a == b); }

    enum class RigidBodyShape {
        Box,
        Sphere,
    };

    struct RigidBodySpawnData {
        bool                    createRigidBody = false;
        RigidBodyShape          shape = RigidBodyShape::Box;
        glm::vec3               halfExtents = { 0.5f, 0.5f, 0.5f };
        f32                     radius = 0.5f;
        BodyMotionType          motionType = BodyMotionType::Dynamic;
        f32                     friction = 0.5f;
        f32                     restitution = 0.0f;
    };

    enum VehicleChassisType {
        VEHICLE_CHASSIS_TYPE_BUGGY,
        VEHICLE_CHASSIS_TYPE_TRUCK,
        VEHICLE_CHASSIS_TYPE_APC,
        VEHICLE_CHASSIS_TYPE_TANK,
        VEHICLE_CHASSIS_TYPE_CRAWLER, // Aircraft carrier but on wheels
    };

    // Arcade-style vehicle: a single dynamic box-shaped chassis body, driven
    // by forces/torque applied directly to it (no wheel physics/constraints
    // yet). Wheels are purely visual, positioned/spun/steered from this data
    // by the renderer; they don't have their own rigid bodies.
    struct VehicleData {
        glm::vec3   chassisHalfExtents = { 0.9f, 0.35f, 1.7f };

        f32         wheelRadius = 0.45f;
        f32         wheelWidth = 0.3f;

        glm::vec3   wheelOffsets[4] = {
            {  0.9f, 0.10f,  1.3f },
            { -0.9f, 0.10f,  1.3f },
            {  0.9f, 0.10f, -1.3f },
            { -0.9f, 0.10f, -1.3f },
        };

        f32         enginePower = 18000.0f;    // N, forward/back drive force
        f32         turnTorque = 18000.0f;     // steering yaw torque
        f32         maxYawRateRadians = 2.2f;  // rad/s, clamps the turn once spun up
        f32         maxSpeed = 30.0f;          // m/s, engine cuts out past this
        f32         gripStrength = 6.0f;       // 1/s, how hard sideways slide is cancelled
        f32         maxSteerAngleDegrees = 30.0f;

        bool        playerControlled = false;

        f32         steerAngleDegrees = 0.0f;
        f32         wheelSpinRadians = 0.0f;
    };

    enum OreNodeType {
        ORE_NODE_TYPE_IRON,
        ORE_NODE_TYPE_COPPER,
        ORE_NODE_TYPE_COAL,
        ORE_NODE_TYPE_SULPHUR,
        ORE_NODE_TYPE_ALUMINUM,
        ORE_NODE_TYPE_CHROME,
    };

    enum DieselType {
        DIESEL_TYPE_PETROLEUM,
        DIESEL_TYPE_BIODIESEL,
        DIESEL_TYPE_COAL_LIQUID,
    };

    struct OreNode {
        OreNodeType type = ORE_NODE_TYPE_IRON;
        i64         amount = 100;
    };

    enum InventoryItemType {
        INVENTORY_ITEM_TYPE_ORE_IRON,
        INVENTORY_ITEM_TYPE_ORE_COPPER,
        INVENTORY_ITEM_TYPE_ORE_COAL,
        INVENTORY_ITEM_TYPE_ORE_SULPHUR,
        INVENTORY_ITEM_TYPE_ORE_ALUMINUM,
        INVENTORY_ITEM_TYPE_ORE_CHROME,
    };

    struct InventoryItem {
        InventoryItemType type;
        i64 amount;
        i32 flatIndex;
    };

    constexpr u32 INVENTORY_CAPACITY = 64;

    struct Inventory {
        i32 xSize;
        i32 ySize;
        FixedList<InventoryItem, INVENTORY_CAPACITY> items;
    };

    i64                     InvetoryGetItemCapacity( InventoryItemType type );
    bool                    InvetoryAddItem( Inventory & inventory, InventoryItemType type, i32 amount );
    InventoryItem *         InvetoryFindItem( Inventory & inventory, InventoryItemType type );
    const InventoryItem *   InvetoryFindItem( const Inventory & inventory, InventoryItemType type );
    i64                     InvetoryRemoveItem( Inventory & inventory, InventoryItemType type );

    InventoryItemType       OreNodeTypeToItemType( OreNodeType type );
    const char *            ToString( InventoryItemType type );
    const char *            ToShortCode( InventoryItemType type );

    enum EntityActionType {
        ENTITY_ACTION_TYPE_IDLE = 0,
        ENTITY_ACTION_TYPE_PLAYER_CONTROL,
        ENTITY_ACTION_TYPE_TRAVELING,
        ENTITY_ACTION_TYPE_MINING_ORE,
        ENTITY_ACTION_TYPE_SELL_ORE,
    };

    struct EntityAction {
        EntityActionType    type;
        EntityId            targetId;

        f32                 progress;
    };

    struct Shop {
        i64 credits;
    };

    struct Entity {
        // Entity
        EntityType  type;
        EntityId    id;

        // Transform
        glm::vec3   position;
        glm::quat   rotation;
        f32         scale;

        // Rendering
        RenderModel        renderModel;

        // Physics
        RigidBody          rigidBody;
        RigidBodySpawnData rigidBodyData;

        // Actions
        EntityAction action;

        // Inventory
        Inventory inventory;

        union {
            VehicleData vehicle;
            OreNode     oreNode;
            Shop        shop;
        };
    };

    Entity MakeEntity(EntityType type, glm::vec3 position);

    const char* ToString(EntityType type);
}
