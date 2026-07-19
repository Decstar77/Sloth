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
        const i64 cap = InvetoryGetItemCapacity( type );
        for ( InventoryItem & item : inventory.items ) {
            if ( item.type == type && item.amount < cap ) {
                // Surely we can simplify this
                item.amount += amount;
                if ( item.amount > cap ) {
                    amount = item.amount - cap; // The amount remaining
                    item.amount = cap;
                } else {
                    amount = 0;
                }
                
                if ( amount == 0 ) {
                    return true;
                }
            }
        }

        // Edge case here, what if the amount is more than the full cap of a invenotry block ?
        
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

    i64 InvetoryRemoveItem( Inventory & inventory, InventoryItemType type ) {
        i64 amount = 0;
        FixedList<u32, INVENTORY_CAPACITY> removals;
        const u32 count = inventory.items.GetCount();
        for ( u32 i = 0; i < count; i++ ) {
            if ( inventory.items[i].type == type ) {
                amount += inventory.items[i].amount;
                removals.Add( i );
            }
        }

        for ( u32 i = 0; i < removals.GetCount(); i++ ) {
            inventory.items.RemoveAt( removals[i] );
        }

        return amount;
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
            case ENTITY_TYPE_SHOP: {
                entity.rigidBodyData.createRigidBody = true;
                entity.shop = Shop();
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