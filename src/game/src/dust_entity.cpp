#include "dust_entity.h"

namespace dust {
    i64 InvetoryGetItemCapacity( InventoryItemType type ) {
        switch ( type ) {
            case INVENTORY_ITEM_TYPE_ORE_IRON:
                return 10;
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

    const char * ToShortCode( InventoryItemType type ) {
        switch ( type ) {
            case INVENTORY_ITEM_TYPE_ORE_IRON:
                return "Fe";
            case INVENTORY_ITEM_TYPE_ORE_COPPER:
                return "Cu";
            case INVENTORY_ITEM_TYPE_ORE_COAL:
                return "C";
            case INVENTORY_ITEM_TYPE_ORE_SULPHUR:
                return "S";
            case INVENTORY_ITEM_TYPE_ORE_ALUMINUM:
                return "Al";
            case INVENTORY_ITEM_TYPE_ORE_CHROME:
                return "Cr";
        }

        return "?";
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

        const i64 slotCount = static_cast<i64>( inventory.xSize ) * static_cast<i64>( inventory.ySize );
        while ( remaining > 0 ) {
            if ( inventory.items.IsFull() || static_cast<i64>( inventory.items.GetCount() ) >= slotCount ) {
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
                entity.inventory.xSize = 2;
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

    void DriveVehicle( PhysicsWorld & physicsWorld, Entity & entity, f32 throttle, f32 steer, f32 deltaTime ) {
        if ( entity.type != ENTITY_TYPE_VEHICLE || !entity.rigidBody.IsValid() ) {
            return;
        }

        VehicleData & vehicle = entity.vehicle;

        glm::vec3 forward = entity.rotation * glm::vec3( 0.0f, 0.0f, 1.0f );
        glm::vec3 right = entity.rotation * glm::vec3( 1.0f, 0.0f, 0.0f );
        glm::vec3 up = entity.rotation * glm::vec3( 0.0f, 1.0f, 0.0f );

        glm::vec3 velocity = physicsWorld.GetLinearVelocity( entity.rigidBody );
        f32 forwardSpeed = glm::dot( velocity, forward );

        if ( throttle != 0.0f && glm::abs( forwardSpeed ) < vehicle.maxSpeed ) {
            physicsWorld.AddForce( entity.rigidBody, forward * throttle * vehicle.enginePower );
        }

        // Steering torque; flips sign in reverse like a real car backing up.
        // Not scaled down at low speed: the chassis is a flat box resting
        // directly on the ground (no wheels), so its own yaw friction
        // already resists spinning in place — a speed-scaled torque on top
        // of that was enough to fully cancel out at low speed and left
        // steering doing nothing. Turn rate is capped below instead.
        if ( steer != 0.0f ) {
            f32 reverseSign = forwardSpeed < 0.0f ? -1.0f : 1.0f;
            physicsWorld.AddTorque( entity.rigidBody, up * steer * reverseSign * vehicle.turnTorque );
        }

        // Clamp yaw spin rate so the steering torque above (sized to
        // overcome ground friction) doesn't turn the vehicle into a
        // spinning top once it gets going.
        {
            glm::vec3 angularVelocity = physicsWorld.GetAngularVelocity( entity.rigidBody );
            f32 yawRate = glm::dot( angularVelocity, up );
            f32 clampedYawRate = glm::clamp( yawRate, -vehicle.maxYawRateRadians, vehicle.maxYawRateRadians );
            if ( clampedYawRate != yawRate ) {
                physicsWorld.SetAngularVelocity( entity.rigidBody, angularVelocity + up * ( clampedYawRate - yawRate ) );
            }
        }

        // Arcade tire grip: cancel most sideways velocity each frame so the
        // vehicle corners instead of sliding around like a hockey puck.
        // Stands in for real wheel friction until there's an actual wheel
        // model.
        f32 lateralSpeed = glm::dot( velocity, right );
        f32 gripFactor = glm::clamp( vehicle.gripStrength * deltaTime, 0.0f, 1.0f );
        physicsWorld.SetLinearVelocity( entity.rigidBody, velocity - right * lateralSpeed * gripFactor );

        // Visual-only wheel state, consumed by DustGame::DrawVehicle().
        vehicle.steerAngleDegrees = glm::mix( vehicle.steerAngleDegrees, steer * vehicle.maxSteerAngleDegrees, glm::clamp( 8.0f * deltaTime, 0.0f, 1.0f ) );
        vehicle.wheelSpinRadians += ( forwardSpeed / glm::max( vehicle.wheelRadius, 0.01f ) ) * deltaTime;
    }

    void DriveVehicleToward( PhysicsWorld & physicsWorld, Entity & entity, glm::vec3 targetPosition, f32 deltaTime ) {
        // Distance at which throttle starts ramping down instead of driving
        // in at full speed and overshooting/orbiting the target.
        constexpr f32 slowRadius = 15.0f;
        // Full steering lock is used once misaligned by this much or more;
        // scales down smoothly below it so the vehicle doesn't twitch once
        // it's basically already facing the target.
        constexpr f32 fullSteerAngle = 30.0f * 3.14159265f / 180.0f;

        glm::vec3 toTarget = targetPosition - entity.position;
        toTarget.y = 0.0f;
        f32 distance = glm::length( toTarget );
        if ( distance < 0.01f ) {
            return;
        }
        glm::vec3 toTargetDir = toTarget / distance;

        glm::vec3 forward = entity.rotation * glm::vec3( 0.0f, 0.0f, 1.0f );
        forward.y = 0.0f;
        f32 forwardLength = glm::length( forward );
        if ( forwardLength < 0.01f ) {
            return;
        }
        forward /= forwardLength;

        glm::vec3 up = entity.rotation * glm::vec3( 0.0f, 1.0f, 0.0f );

        // Signed angle from `forward` to `toTargetDir` about `up`, in the
        // same sign convention DriveVehicle's steering torque uses (positive
        // steer rotates `forward` toward `up × forward`) - so a positive
        // angle here always means "steer positive to close it", regardless
        // of which way the vehicle happens to be facing.
        f32 angle = glm::atan( glm::dot( up, glm::cross( forward, toTargetDir ) ), glm::dot( forward, toTargetDir ) );

        f32 steer = glm::clamp( angle / fullSteerAngle, -1.0f, 1.0f );
        f32 throttle = glm::clamp( distance / slowRadius, 0.25f, 1.0f );

        DriveVehicle( physicsWorld, entity, throttle, steer, deltaTime );
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