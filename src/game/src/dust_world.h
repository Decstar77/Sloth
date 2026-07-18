#pragma once

#include "dust_entity.h"

#include <physics/sloth_physics_world.h>

#include <vector>

namespace dust {

    // Owns the entity table plus the physics world entities are backed by.
    //
    // Spawn/Destroy are deferred: calling them only buffers a request and
    // hands back the EntityId the entity will have once applied. The actual
    // table/physics mutation happens in FlushPendingChanges(), which must be
    // called once per frame (from a single thread) after gameplay code is
    // done submitting requests for that frame. This keeps the entity table
    // stable for the rest of the frame (safe to read/iterate from multiple
    // threads later) and means a freshly spawned entity only becomes visible
    // to GetEntity() on the frame after it was requested.
    class DustWorld {
    public:
        void            Init(sloth::PhysicsWorld* physicsWorld);

        EntityId        SpawnEntity(Entity entity);
        void            DestroyEntity(EntityId id);

        Entity*         GetEntity(EntityId id);
        const Entity*   GetEntity(EntityId id) const;

        // All live entity slots (may include freed/invalid slots pending
        // reuse; check Entity::type != ENTITY_TYPE_INVALID before use).
        // Excludes entities whose spawn is still pending FlushPendingChanges().
        const std::vector<Entity>& GetEntities() const { return entities; }

        // Copies each physics-backed entity's position/rotation from its
        // physics body, so Entity::position/rotation stay the single source
        // of truth for "where is this entity" regardless of entity type.
        // Call once per frame after PhysicsWorld::Update().
        void            SyncPhysicsTransforms();

        void            FlushPendingChanges();

    private:
        struct PendingSpawn {
            EntityId   id;
            Entity     entity;
        };

        i32         AllocateSlot();
        void        ApplySpawn(const PendingSpawn& spawn);
        void        ApplyDestroy(EntityId id);

    private:
        sloth::PhysicsWorld* physicsWorld = nullptr;

        std::vector<Entity> entities;
        std::vector<i32>    freeSlots;

        std::vector<PendingSpawn> pendingSpawns;
        std::vector<EntityId>     pendingDestroys;

    public:
        // Coarse simulation buckets, populated by future gameplay/LOD code.
        std::vector<EntityId> simHot;
        std::vector<EntityId> simWarm;
        std::vector<EntityId> simCold;
    };

}
