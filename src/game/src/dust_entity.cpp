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
            case INVENTORY_ITEM_TYPE_ORE_SULPHUR:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_ALUMINUM:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_CHROME:
                return 100;
        }

        SL_ASSERT( false );
        return 0;
    }

    InventoryItemType OreNodeTypeToItemType( OreNodeType type ) {
        switch ( type ) {
            case ORE_NODE_TYPE_IRON:
                return INVENTORY_ITEM_TYPE_ORE_IRON;
            case ORE_NODE_TYPE_COPPER:
                return INVENTORY_ITEM_TYPE_ORE_COPPER;
            case ORE_NODE_TYPE_COAL:
                return INVENTORY_ITEM_TYPE_ORE_COAL;
            case ORE_NODE_TYPE_SULPHUR:
                return INVENTORY_ITEM_TYPE_ORE_SULPHUR;
            case ORE_NODE_TYPE_ALUMINUM:
                return INVENTORY_ITEM_TYPE_ORE_ALUMINUM;
            case ORE_NODE_TYPE_CHROME:
                return INVENTORY_ITEM_TYPE_ORE_CHROME;
        }

        SL_ASSERT( false );
        return INVENTORY_ITEM_TYPE_ORE_IRON;
    }

    const char * ToString( InventoryItemType type ) {
        switch ( type ) {
            case INVENTORY_ITEM_TYPE_ORE_IRON:
                return "Iron";
            case INVENTORY_ITEM_TYPE_ORE_COPPER:
                return "Copper";
            case INVENTORY_ITEM_TYPE_ORE_COAL:
                return "Coal";
            case INVENTORY_ITEM_TYPE_ORE_SULPHUR:
                return "Sulphur";
            case INVENTORY_ITEM_TYPE_ORE_ALUMINUM:
                return "Aluminum";
            case INVENTORY_ITEM_TYPE_ORE_CHROME:
                return "Chrome";
        }

        return "Unknown";
    }

    bool InvetoryAddItem( Inventory & inventory, InventoryItemType type, i32 amount ) {
        SL_ASSERT_MSG( amount >= 0, "InvetoryAddItem called with negative amount" );

        const i64 cap = InvetoryGetItemCapacity( type );
        i64 remaining = amount;

        // Top up any existing partial stacks of this type first.
        for ( InventoryItem & item : inventory.items ) {
            if ( remaining == 0 ) {
                break;
            }

            if ( item.type != type ) {
                continue;
            }

            const i64 space = cap - item.amount;
            if ( space <= 0 ) {
                continue;
            }

            const i64 add = space < remaining ? space : remaining;
            item.amount += add;
            remaining -= add;
        }

        // Spill any leftover into new stacks, splitting across as many as
        // needed so no single stack ever exceeds its capacity.
        while ( remaining > 0 ) {
            if ( inventory.items.IsFull() ) {
                return false;
            }

            const i64 add = cap < remaining ? cap : remaining;
            InventoryItem & item = inventory.items.Emplace();
            item.type = type;
            item.amount = add;
            item.flatIndex = inventory.items.GetCount() - 1;

            remaining -= add;
        }

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

        // Remove back-to-front so earlier indices in `removals` stay valid
        for ( u32 i = removals.GetCount(); i > 0; i-- ) {
            inventory.items.RemoveAt( removals[i - 1] );
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