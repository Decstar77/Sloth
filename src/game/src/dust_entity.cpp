#include "dust_entity.h"


namespace dust {
    Entity MakeEntity(EntityType type, glm::vec3 position)
    {
        Entity entity = {}; // Clear to zero
        entity.type = type;
        entity.position = position;
        entity.rotation = glm::quat(1, 0, 0, 0);
        entity.scale = 1.0;


        switch (type) {
        case ENTITY_TYPE_INVALID: {
            SL_ASSERT_MSG(false, "Making entity of invalid type");
        } break;
        case ENTITY_TYPE_PROP: {
            // Init prop data
            entity.prop = PropData();
        } break;
        }

        return entity;
    }
}