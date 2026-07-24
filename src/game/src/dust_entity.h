#pragma once

#include <core/sloth_defines.h>
#include <core/sloth_list.h>
#include <dust_faction.h>
#include <physics/sloth_physics_world.h>
#include <renderer/sloth_render_model.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace sloth;

namespace dust {

    struct Entity;
    enum EntityType {
        ENTITY_TYPE_INVALID = 0,
        ENTITY_TYPE_PROP,
        ENTITY_TYPE_VEHICLE,
        ENTITY_TYPE_ORE_NODE,
        ENTITY_TYPE_BUILDING,
    };

    struct EntityId {
        i32 index      = -1;
        i32 generation = 0;
    };

    constexpr EntityId INVALID_ENTITY_ID = {};

    inline bool operator==(const EntityId& a, const EntityId& b) { return a.index == b.index && a.generation == b.generation; }
    inline bool operator!=(const EntityId& a, const EntityId& b) { return !(a == b); }

    enum InventoryItemType {
        INVENTORY_ITEM_TYPE_NONE = 0,

        INVENTORY_ITEM_TYPE_RAW_MATERIAL_BEGIN,
        INVENTORY_ITEM_TYPE_ORE_IRON,
        INVENTORY_ITEM_TYPE_ORE_COPPER,
        INVENTORY_ITEM_TYPE_ORE_SULPHUR,
        INVENTORY_ITEM_TYPE_ORE_ALUMINUM,
        INVENTORY_ITEM_TYPE_ORE_CRUDE_OIL,
        INVENTORY_ITEM_TYPE_ORE_WATER,
        INVENTORY_ITEM_TYPE_ORE_SILICON,
        INVENTORY_ITEM_TYPE_RAW_MATERIAL_END,

        // Refinery
        INVENTORY_ITEM_TYPE_STEEL_INGOT,
        INVENTORY_ITEM_TYPE_COPPER_WIRE,
        INVENTORY_ITEM_TYPE_ALUMINUM_PLATE,
        INVENTORY_ITEM_TYPE_PETROL,
        INVENTORY_ITEM_TYPE_LUBRICANT,
        INVENTORY_ITEM_TYPE_GLASS,

        // Chemical Plant
        INVENTORY_ITEM_TYPE_SULPHURIC_ACID,
        INVENTORY_ITEM_TYPE_GUNPOWDER,
        INVENTORY_ITEM_TYPE_RUBBER,
        INVENTORY_ITEM_TYPE_PLASTIC,
        INVENTORY_ITEM_TYPE_SILICON_WAFER,
        INVENTORY_ITEM_TYPE_PURIFIED_WATER,

        // Vechicle Armour Parts
        INVENTORY_ITEM_TYPE_ARMOUR_BEGIN,
        INVENTORY_ITEM_TYPE_ARMOUR_WOOD_PLANKS,
        INVENTORY_ITEM_TYPE_ARMOUR_STEEL_PLATING,
        INVENTORY_ITEM_TYPE_ARMOUR_END,

        // Vechicle Power Parts
        INVENTORY_ITEM_TYPE_POWER_BEGIN,
        INVENTORY_ITEM_TYPE_POWER_FUEL_TANK,
        INVENTORY_ITEM_TYPE_POWER_BATTERY,
        INVENTORY_ITEM_TYPE_POWER_END,

        // Vechicle Engine Parts
        INVENTORY_ITEM_TYPE_ENGINE_BEGIN,
        INVENTORY_ITEM_TYPE_ENGINE_PETROL,
        INVENTORY_ITEM_TYPE_ENGINE_ELECTRIC,
        INVENTORY_ITEM_TYPE_ENGINE_END,

        // Vechicle Tire Parts
        INVENTORY_ITEM_TYPE_TIRE_BEGIN,
        INVENTORY_ITEM_TYPE_TIRE_SHRUB,
        INVENTORY_ITEM_TYPE_TIRE_RUBBER,
        INVENTORY_ITEM_TYPE_TIRE_END,

        INVENTORY_ITEM_TYPE_TURRET_BEGIN,
        INVENTORY_ITEM_TYPE_TURRET_MINING_LASER,
        INVENTORY_ITEM_TYPE_TURRET_BULLET_GUN,
        INVENTORY_ITEM_TYPE_TURRET_END,
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

    struct VehicleChassisDefinition {
        VehicleChassisType chassisType;
        union {
            struct {
                InventoryItemType engineSlot;
                InventoryItemType tireSlot;
                InventoryItemType turretSlot;
                InventoryItemType powerSlot;
                InventoryItemType generalSlot1;
                InventoryItemType generalSlot2;
            } buggy;
            struct {
                InventoryItemType engineSlot;
                InventoryItemType tireSlot;
                InventoryItemType turretSlot;
                InventoryItemType powerSlot;
                InventoryItemType generalSlot1;
                InventoryItemType generalSlot2;
                InventoryItemType generalSlot3;
                InventoryItemType generalSlot4;
            } truck;
        };
    };

    // Arcade-style vehicle: a single dynamic box-shaped chassis body, driven
    // by forces/torque applied directly to it (no wheel physics/constraints
    // yet). Wheels are purely visual, positioned/spun/steered from this data
    // by the renderer; they don't have their own rigid bodies.
    struct VehicleData {
        // Vehicle makeup
        VehicleChassisDefinition definition;

        // Phyiscs data
        glm::vec3   chassisHalfExtents = { 0.9f, 0.35f, 1.7f };
        glm::vec3   wheelOffsets[4] = { {  0.9f, 0.10f,  1.3f }, { -0.9f, 0.10f,  1.3f }, {  0.9f, 0.10f, -1.3f }, { -0.9f, 0.10f, -1.3f } };
        f32         wheelRadius = 0.45f;
        f32         wheelWidth = 0.3f;
        f32         enginePower = 18000.0f;    // N, forward/back drive force
        f32         turnTorque = 18000.0f;     // steering yaw torque
        f32         maxYawRateRadians = 2.2f;  // rad/s, clamps the turn once spun up
        f32         maxSpeed = 10.0f;          // m/s, engine cuts out past this
        f32         gripStrength = 6.0f;       // 1/s, how hard sideways slide is cancelled
        f32         maxSteerAngleDegrees = 30.0f;
        f32         steerAngleDegrees = 0.0f;
        f32         wheelSpinRadians = 0.0f;
    };

    void DriveVehicle( PhysicsWorld & physicsWorld, Entity & entity, f32 throttle, f32 steer, f32 deltaTime );
    void DriveVehicleToward( PhysicsWorld & physicsWorld, Entity & entity, glm::vec3 targetPosition, f32 deltaTime );

    enum OreNodeType {
        ORE_NODE_TYPE_IRON,
        ORE_NODE_TYPE_COPPER,
        ORE_NODE_TYPE_SULPHUR,
        ORE_NODE_TYPE_ALUMINUM,
        ORE_NODE_TYPE_CRUDE_OIL,
        ORE_NODE_TYPE_WATER,
        ORE_NODE_TYPE_SILICON,
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

    i64                     InvetoryGetItemCapacity( InventoryItemType type );
    bool                    InvetoryAddItem( Inventory & inventory, InventoryItemType type, i32 amount );
    InventoryItem *         InvetoryFindItem( Inventory & inventory, InventoryItemType type );
    const InventoryItem *   InvetoryFindItem( const Inventory & inventory, InventoryItemType type );
    i64                     InvetoryRemoveItemByIndex( Inventory & inventory, i32 index );
    i64                     InvetoryRemoveItemByType( Inventory & inventory, InventoryItemType type );
    i64                     InventoryGetTotalAmount( const Inventory & inventory, InventoryItemType type );
    bool                    InventoryRemoveAmount( Inventory & inventory, InventoryItemType type, i64 amount );

    bool                    ItemIsRawMaterial( InventoryItemType type );

    InventoryItemType       OreNodeTypeToItemType( OreNodeType type );
    const char *            ToString( InventoryItemType type );
    const char *            ToShortCode( InventoryItemType type );
    const char *            ToString( VehicleChassisType type );

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

    enum BuildingType {
        BUILDING_TYPE_SHOP = 0,
        BUILDING_TYPE_REFINERY,
        BUILDING_TYPE_CHEMICALPLANT,
    };

    struct Building {
        BuildingType type;
        i64 credits;
    };

    struct Price {
        i64 credits;
        i64 oreIron;
        i64 oreCopper;
        i64 oreSulphur;
        i64 oreAluminum;
        i64 oreCrudeOil;
        i64 oreWater;
        i64 oreSilicon;
    };

    Price RefineryPriceForItem( InventoryItemType item );
    //Price ChemicalPlanePriceForItem( InventoryItemType item );

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
        bool            playerControlled = false;
        EntityAction    action;
        
        // Faction
        FactionType faction;

        // Local credits for this entity
        i64 credits;

        // Inventory
        Inventory inventory;

        union {
            VehicleData vehicle;
            OreNode     oreNode;
            Building    building;
        };
    };

    Entity          MakeEntity( EntityType type, FactionType faction, glm::vec3 position );
    const char *    ToString( EntityType type );

}
