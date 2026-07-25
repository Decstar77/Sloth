#pragma once

#include <core/sloth_defines.h>
#include <core/sloth_list.h>

#include <dust_faction.h>
#include <dust_item.h>
#include <dust_vehicle.h>

#include <physics/sloth_physics_world.h>
#include <renderer/sloth_render_model.h>

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

    i32                     InvetoryGetItemCapacity( InventoryItemType type );
    bool                    InvetoryAddItem( Inventory & inventory, InventoryItemType type, i32 amount );
    InventoryItem *         InvetoryFindItem( Inventory & inventory, InventoryItemType type );
    const InventoryItem *   InvetoryFindItem( const Inventory & inventory, InventoryItemType type );
    i64                     InvetoryRemoveItemByIndex( Inventory & inventory, i32 index );
    i64                     InvetoryRemoveItemByType( Inventory & inventory, InventoryItemType type );
    i64                     InventoryGetTotalAmount( const Inventory & inventory, InventoryItemType type );
    bool                    InventoryRemoveAmount( Inventory & inventory, InventoryItemType type, i32 amount );

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
        BUILDING_TYPE_CHEMICAL_PLANT,
        BUILDING_TYPE_FACTORY,
        BUILDING_TYPE_WORKSHOP,
    };

    struct Building {
        BuildingType type;
    };

    struct Price {
        i64 credits;
        i32 oreIron;
        i32 oreCopper;
        i32 oreSulphur;
        i32 oreAluminum;
        i32 oreCrudeOil;
        i32 oreWater;
        i32 oreSilicon;

        i32 steelIngot;
        i32 copperWire;
        i32 aluminumPlate;
        i32 petrol;
        i32 lubricant;
        i32 glass;
        i32 sulphuricAcid;
        i32 gunPowder;
        i32 rubber;
        i32 plastic;
        i32 siliconWafer;
        i32 purifiedWater;
    };

    Price BuildingRefineryPriceForItem( InventoryItemType item );
    Price BuildingFactoryPriceForItem( InventoryItemType item );
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
