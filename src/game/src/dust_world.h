#pragma once

#include "dust_entity.h"
#include "dust_faction.h"
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
        void                        Init(sloth::PhysicsWorld* physicsWorld);

        EntityId                    SpawnEntity(Entity entity);
        void                        DestroyEntity(EntityId id);

        Entity*                     GetEntity(EntityId id);
        const Entity*               GetEntity(EntityId id) const;
        EntityId                    FindEntityByRigidBody(sloth::RigidBody body) const;
        
        const std::vector<Entity>&  GetEntities() const { return entities; }

        void                        Update( f32 deltaTime );
        void                        SyncPhysicsTransforms();
        void                        FlushPendingChanges();

    public:
        i64 playerCredits = 0; // @TEMP

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

        // Coarse simulation buckets, populated by future gameplay/LOD code.
        std::vector<EntityId> simHot;
        std::vector<EntityId> simWarm;
        std::vector<EntityId> simCold;
    };

}
