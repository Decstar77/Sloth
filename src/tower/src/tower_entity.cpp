#include "tower_entity.h"

namespace tower {

    Entity MakeEntity( EntityType type, glm::vec3 position ) {
        Entity entity = {}; // Clear to zero
        entity.type = type;
        entity.position = position;
        entity.rotation = glm::quat( 1, 0, 0, 0 );
        entity.scale = 1.0f;
        entity.rigidBody = RigidBody();

        switch ( type ) {
            case ENTITY_TYPE_INVALID: {
                SL_ASSERT_MSG( false, "Making entity of invalid type" );
            } break;
            case ENTITY_TYPE_PROP: {
                entity.rigidBodyData.createRigidBody = true;
                entity.prop = PropData();
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
        }

        return "Unknown";
    }

} // namespace tower
