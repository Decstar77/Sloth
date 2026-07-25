#include "dust_entity.h"

namespace dust {
    InventoryItemType OreNodeTypeToItemType( OreNodeType type ) {
        switch ( type ) {
            case ORE_NODE_TYPE_IRON:
                return INVENTORY_ITEM_TYPE_ORE_IRON;
            case ORE_NODE_TYPE_COPPER:
                return INVENTORY_ITEM_TYPE_ORE_COPPER;
            case ORE_NODE_TYPE_SULPHUR:
                return INVENTORY_ITEM_TYPE_ORE_SULPHUR;
            case ORE_NODE_TYPE_ALUMINUM:
                return INVENTORY_ITEM_TYPE_ORE_ALUMINUM;
            case ORE_NODE_TYPE_CRUDE_OIL:
                return INVENTORY_ITEM_TYPE_ORE_CRUDE_OIL;
            case ORE_NODE_TYPE_WATER:
                return INVENTORY_ITEM_TYPE_ORE_WATER;
            case ORE_NODE_TYPE_SILICON:
                return INVENTORY_ITEM_TYPE_ORE_SILICON;
        }

        SL_ASSERT( false );
        return INVENTORY_ITEM_TYPE_ORE_IRON;
    }

    i32 InvetoryGetItemCapacity( InventoryItemType type ) {
        switch ( type ) {
            case INVENTORY_ITEM_TYPE_ORE_IRON:
                return 10;
            case INVENTORY_ITEM_TYPE_ORE_COPPER:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_SULPHUR:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_ALUMINUM:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_CRUDE_OIL:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_WATER:
                return 100;
            case INVENTORY_ITEM_TYPE_ORE_SILICON:
                return 100;

            case INVENTORY_ITEM_TYPE_STEEL_INGOT:
                return 100;
            case INVENTORY_ITEM_TYPE_COPPER_WIRE:
                return 100;
            case INVENTORY_ITEM_TYPE_ALUMINUM_PLATE:
                return 100;
            case INVENTORY_ITEM_TYPE_PETROL:
                return 100;
            case INVENTORY_ITEM_TYPE_LUBRICANT:
                return 100;
            case INVENTORY_ITEM_TYPE_GLASS:
                return 100;

            case INVENTORY_ITEM_TYPE_SULPHURIC_ACID:
                return 100;
            case INVENTORY_ITEM_TYPE_GUNPOWDER:
                return 100;
            case INVENTORY_ITEM_TYPE_RUBBER:
                return 100;
            case INVENTORY_ITEM_TYPE_PLASTIC:
                return 100;
            case INVENTORY_ITEM_TYPE_SILICON_WAFER:
                return 100;
            case INVENTORY_ITEM_TYPE_PURIFIED_WATER:
                return 100;
        }

        SL_ASSERT( false );
        return 0;
    }

    const char * ToString( InventoryItemType type ) {
        switch ( type ) {
            case INVENTORY_ITEM_TYPE_ORE_IRON:
                return "Iron";
            case INVENTORY_ITEM_TYPE_ORE_COPPER:
                return "Copper";
            case INVENTORY_ITEM_TYPE_ORE_SULPHUR:
                return "Sulphur";
            case INVENTORY_ITEM_TYPE_ORE_ALUMINUM:
                return "Aluminum";
            case INVENTORY_ITEM_TYPE_ORE_CRUDE_OIL:
                return "Crude Oil";
            case INVENTORY_ITEM_TYPE_ORE_WATER:
                return "Water";
            case INVENTORY_ITEM_TYPE_ORE_SILICON:
                return "Silicon";

            case INVENTORY_ITEM_TYPE_STEEL_INGOT:
                return "Steel Ingot";
            case INVENTORY_ITEM_TYPE_COPPER_WIRE:
                return "Copper Wire";
            case INVENTORY_ITEM_TYPE_ALUMINUM_PLATE:
                return "Aluminum Plate";
            case INVENTORY_ITEM_TYPE_PETROL:
                return "Petrol";
            case INVENTORY_ITEM_TYPE_LUBRICANT:
                return "Lubricant";
            case INVENTORY_ITEM_TYPE_GLASS:
                return "Glass";

            case INVENTORY_ITEM_TYPE_SULPHURIC_ACID:
                return "Sulphuric Acid";
            case INVENTORY_ITEM_TYPE_GUNPOWDER:
                return "Gunpowder";
            case INVENTORY_ITEM_TYPE_RUBBER:
                return "Rubber";
            case INVENTORY_ITEM_TYPE_PLASTIC:
                return "Plastic";
            case INVENTORY_ITEM_TYPE_SILICON_WAFER:
                return "Silicon Wafer";
            case INVENTORY_ITEM_TYPE_PURIFIED_WATER:
                return "Purified Water";

            case INVENTORY_ITEM_TYPE_ARMOUR_WOOD_PLANKS:
                return "Wood Plank Armour";
            case INVENTORY_ITEM_TYPE_ARMOUR_STEEL_PLATING:
                return "Steel Plate Armour";

            case INVENTORY_ITEM_TYPE_POWER_FUEL_TANK:
                return "Fuel Tank";
            case INVENTORY_ITEM_TYPE_POWER_BATTERY:
                return "Battery";

            case INVENTORY_ITEM_TYPE_ENGINE_PETROL:
                return "Petrol Engine";
            case INVENTORY_ITEM_TYPE_ENGINE_ELECTRIC:
                return "Electric Engine";

            case INVENTORY_ITEM_TYPE_TIRE_SHRUB:
                return "Shrub Tires";
            case INVENTORY_ITEM_TYPE_TIRE_RUBBER:
                return "Rubber Tires";

            case INVENTORY_ITEM_TYPE_TURRET_MINING_LASER:
                return "Mining Laser";
            case INVENTORY_ITEM_TYPE_TURRET_BULLET_GUN:
                return "Bullet Gun";

            default:
                break;
        }

        return "Unknown";
    }

    const char * ToShortCode( InventoryItemType type ) {
        switch ( type ) {
            case INVENTORY_ITEM_TYPE_ORE_IRON:
                return "Fe";
            case INVENTORY_ITEM_TYPE_ORE_COPPER:
                return "Cu";
            case INVENTORY_ITEM_TYPE_ORE_SULPHUR:
                return "S";
            case INVENTORY_ITEM_TYPE_ORE_ALUMINUM:
                return "Al";
            case INVENTORY_ITEM_TYPE_ORE_CRUDE_OIL:
                return "Oil";
            case INVENTORY_ITEM_TYPE_ORE_WATER:
                return "H2O";
            case INVENTORY_ITEM_TYPE_ORE_SILICON:
                return "Si";

            case INVENTORY_ITEM_TYPE_STEEL_INGOT:
                return "Stl";
            case INVENTORY_ITEM_TYPE_COPPER_WIRE:
                return "CuW";
            case INVENTORY_ITEM_TYPE_ALUMINUM_PLATE:
                return "AlP";
            case INVENTORY_ITEM_TYPE_PETROL:
                return "Pet";
            case INVENTORY_ITEM_TYPE_LUBRICANT:
                return "Lub";
            case INVENTORY_ITEM_TYPE_GLASS:
                return "Gls";

            case INVENTORY_ITEM_TYPE_SULPHURIC_ACID:
                return "H2SO4";
            case INVENTORY_ITEM_TYPE_GUNPOWDER:
                return "Gun";
            case INVENTORY_ITEM_TYPE_RUBBER:
                return "Rub";
            case INVENTORY_ITEM_TYPE_PLASTIC:
                return "Pla";
            case INVENTORY_ITEM_TYPE_SILICON_WAFER:
                return "SiW";
            case INVENTORY_ITEM_TYPE_PURIFIED_WATER:
                return "pH2O";

            case INVENTORY_ITEM_TYPE_ARMOUR_WOOD_PLANKS:
                return "WdArm";
            case INVENTORY_ITEM_TYPE_ARMOUR_STEEL_PLATING:
                return "StArm";

            case INVENTORY_ITEM_TYPE_POWER_FUEL_TANK:
                return "Fuel";
            case INVENTORY_ITEM_TYPE_POWER_BATTERY:
                return "Batt";

            case INVENTORY_ITEM_TYPE_ENGINE_PETROL:
                return "PEng";
            case INVENTORY_ITEM_TYPE_ENGINE_ELECTRIC:
                return "EEng";

            case INVENTORY_ITEM_TYPE_TIRE_SHRUB:
                return "STire";
            case INVENTORY_ITEM_TYPE_TIRE_RUBBER:
                return "RTire";

            case INVENTORY_ITEM_TYPE_TURRET_MINING_LASER:
                return "MLsr";
            case INVENTORY_ITEM_TYPE_TURRET_BULLET_GUN:
                return "MGun";

            default:
                break;
        }

        return "?";
    }

    bool InvetoryAddItem( Inventory & inventory, InventoryItemType type, i32 amount ) {
        SL_ASSERT_MSG( amount >= 0, "InvetoryAddItem called with negative amount" );

        const i32 cap = InvetoryGetItemCapacity( type );
        i32 remaining = amount;

        // Top up any existing partial stacks of this type first.
        for ( InventoryItem & item : inventory.items ) {
            if ( remaining == 0 ) {
                break;
            }

            if ( item.type != type ) {
                continue;
            }

            const i32 space = cap - item.amount;
            if ( space <= 0 ) {
                continue;
            }

            const i32 add = space < remaining ? space : remaining;
            item.amount += add;
            remaining -= add;
        }

        const i32 slotCount = static_cast<i32>( inventory.xSize ) * static_cast<i32>( inventory.ySize );
        while ( remaining > 0 ) {
            if ( inventory.items.IsFull() || static_cast<i32>( inventory.items.GetCount() ) >= slotCount ) {
                return false;
            }

            const i32 add = cap < remaining ? cap : remaining;
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

    i64 InvetoryRemoveItemByIndex( Inventory & inventory, i32 index ) {
        const i64 amount = inventory.items[index].amount;
        inventory.items.RemoveAt( index );
        return amount;
    }

    i64 InvetoryRemoveItemByType( Inventory & inventory, InventoryItemType type ) {
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

    i64 InventoryGetTotalAmount( const Inventory & inventory, InventoryItemType type ) {
        i64 total = 0;
        for ( const InventoryItem & item : inventory.items ) {
            if ( item.type == type ) {
                total += item.amount;
            }
        }

        return total;
    }

    bool InventoryRemoveAmount( Inventory & inventory, InventoryItemType type, i32 amount ) {
        if ( InventoryGetTotalAmount( inventory, type ) < amount ) {
            return false;
        }

        i32 remaining = amount;
        FixedList<u32, INVENTORY_CAPACITY> emptied;
        const u32 count = inventory.items.GetCount();
        for ( u32 i = 0; i < count && remaining > 0; i++ ) {
            InventoryItem & item = inventory.items[i];
            if ( item.type != type ) {
                continue;
            }

            const i32 take = item.amount < remaining ? item.amount : remaining;
            item.amount -= take;
            remaining -= take;

            if ( item.amount == 0 ) {
                emptied.Add( i );
            }
        }

        // Remove back-to-front so earlier indices in `emptied` stay valid.
        for ( u32 i = emptied.GetCount(); i > 0; i-- ) {
            inventory.items.RemoveAt( emptied[i - 1] );
        }

        return true;
    }

    bool ItemIsRawMaterial( InventoryItemType type ) {
        return type > INVENTORY_ITEM_TYPE_RAW_MATERIAL_BEGIN && type < INVENTORY_ITEM_TYPE_RAW_MATERIAL_END;
    }

    Price BuildingRefineryPriceForItem( InventoryItemType item ) {
        Price price = {};
        switch ( item ) {
            case INVENTORY_ITEM_TYPE_STEEL_INGOT: {
                price.credits = 10;
                price.oreIron = 25;
            } break;
            case INVENTORY_ITEM_TYPE_COPPER_WIRE: {
                price.credits = 10;
                price.oreCopper = 25;
            } break;
            case INVENTORY_ITEM_TYPE_ALUMINUM_PLATE: {
                price.credits = 10;
                price.oreAluminum = 25;
            } break;
            case INVENTORY_ITEM_TYPE_PETROL: {
                price.credits = 10;
                price.oreCrudeOil = 25;
            } break;
            case INVENTORY_ITEM_TYPE_LUBRICANT: {
                price.credits = 10;
                price.oreCrudeOil = 25;
            } break;
            case INVENTORY_ITEM_TYPE_GLASS: {
                price.credits = 10;
                price.oreSilicon = 25;
            } break;
            default:
                SL_ASSERT_MSG( false, "Unkown item for refinery" );
                break;
        }
        return price;
    }

    Price BuildingFactoryPriceForItem( InventoryItemType item ) {
        Price price = {};
        switch ( item ) {
            case INVENTORY_ITEM_TYPE_VEHICLE_CHASSIS_BUGGY: {
                price.credits = 10;
            } break;
            case INVENTORY_ITEM_TYPE_ARMOUR_WOOD_PLANKS: {
                price.credits = 10;
            } break;
            case INVENTORY_ITEM_TYPE_POWER_FUEL_TANK: {
                price.credits = 10;
            } break;
            case INVENTORY_ITEM_TYPE_ENGINE_PETROL: {
                price.credits = 10;
            } break;
            case INVENTORY_ITEM_TYPE_TIRE_SHRUB: {
                price.credits = 10;
            } break;
            case INVENTORY_ITEM_TYPE_TURRET_MINING_LASER: {
                price.credits = 10;
            } break;
        }
        return price;
    }

    Entity MakeEntity( EntityType type, FactionType faction, glm::vec3 position ) {
        Entity entity = {}; // Clear to zero
        entity.type = type;
        entity.faction = faction;
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
                entity.vehicle = VehicleData();
                entity.inventory.xSize = 5;
                entity.inventory.ySize = 3;

                entity.vehicle.definition.chassisType        = VEHICLE_CHASSIS_TYPE_BUGGY;
                entity.vehicle.definition.buggy.engineSlot   = INVENTORY_ITEM_TYPE_ENGINE_PETROL;
                entity.vehicle.definition.buggy.powerSlot    = INVENTORY_ITEM_TYPE_POWER_FUEL_TANK;
                entity.vehicle.definition.buggy.tireSlot     = INVENTORY_ITEM_TYPE_TIRE_SHRUB;
                entity.vehicle.definition.buggy.turretSlot   = INVENTORY_ITEM_TYPE_TURRET_MINING_LASER;
                entity.vehicle.definition.buggy.generalSlot1 = INVENTORY_ITEM_TYPE_NONE;
                entity.vehicle.definition.buggy.generalSlot2 = INVENTORY_ITEM_TYPE_NONE;

            } break;
            case ENTITY_TYPE_ORE_NODE: {
                entity.rigidBodyData.createRigidBody = true;
                entity.oreNode = OreNode();
            } break;
            case ENTITY_TYPE_BUILDING: {
                entity.rigidBodyData.createRigidBody = true;
                entity.building = {};
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
            case ENTITY_TYPE_BUILDING:
                return "Shop";
        }

        return "Unknown";
    }
} // namespace dust