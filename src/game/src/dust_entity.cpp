#include "dust_entity.h"

namespace dust {
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
        }

        return "Unknown";
    }
} // namespace dust