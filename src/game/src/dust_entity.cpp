#include "dust_entity.h"

namespace dust {
    i64 InvetoryGetItemCapacity( InventoryItemType type ) {
        switch ( type ) {
            case INVENTORY_ITEM_TYPE_ORE_IRON:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_COPPER:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_COAL:
                return 100;
        }

        SL_ASSERT( false );
        return 0;
    }

    bool InvetoryAddItem( Inventory & inventory, InventoryItemType type, i32 amount ) {
        for ( InventoryItem & item : inventory.items ) {
            if ( item.type == type ) {
                i64 cap = InvetoryGetItemCapacity( type );
                item.amount = glm::clamp<i64>( item.amount + amount, 0, cap );
                return true;
            }
        }

        if ( inventory.items.IsFull() ) {
            return false;
        }

        InventoryItem & item = inventory.items.Emplace();
        item.type = type;
        item.amount = amount;
        item.flatIndex = inventory.items.GetCount();

        return true;
    }

    InventoryItem * InvetoryFindItem( Inventory & inventory, InventoryItemType type ) {
        for ( InventoryItem & item : inventory.items ) {
            if ( item.type == type ) {
                return &item;
            }
        }

        return nullptr;
    }

    const InventoryItem * InvetoryFindItem( const Inventory & inventory, InventoryItemType type ) {
        for ( const InventoryItem & item : inventory.items ) {
            if ( item.type == type ) {
                return &item;
            }
        }

        return nullptr;
    }

    Entity MakeEntity( EntityType type, glm::vec3 position ) {
        Entity entity = {}; // Clear to zero
        entity.type = type;
        entity.position = position;
        entity.rotation = glm::quat( 1, 0, 0, 0 );
        entity.scale = 1.0;
        entity.rigidBody = RigidBody();

        switch ( type ) {
            case ENTITY_TYPE_INVALID: {
                SL_ASSERT_MSG( false, "Making entity of invalid type" );
            } break;
            case ENTITY_TYPE_PROP: {
                entity.rigidBodyData.createRigidBody = true;
            } break;
            case ENTITY_TYPE_VEHICLE: {
                entity.rigidBodyData.createRigidBody = true;
                entity.vehicle = VehicleData();
                entity.inventory.xSize = 1;
                entity.inventory.ySize = 1;
            } break;
            case ENTITY_TYPE_ORE_NODE: {
                entity.rigidBodyData.createRigidBody = true;
                entity.oreNode = OreNode();
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
            case ENTITY_TYPE_VEHICLE:
                return "Vehicle";
            case ENTITY_TYPE_ORE_NODE:
                return "Ore Node";
            case ENTITY_TYPE_SHOP:
                return "Shop";
        }

        return "Unknown";
    }
} // namespace dust