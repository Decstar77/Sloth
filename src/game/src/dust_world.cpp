#include "dust_world.h"

namespace dust {

    void DustWorld::Init(sloth::PhysicsWorld* newPhysicsWorld)
    {
        physicsWorld = newPhysicsWorld;
    }

    i32 DustWorld::AllocateSlot()
    {
        if (!freeSlots.empty())
        {
            i32 slot = freeSlots.back();
            freeSlots.pop_back();
            return slot;
        }

        entities.push_back(Entity{});
        return static_cast<i32>(entities.size()) - 1;
    }

    EntityId DustWorld::SpawnEntity(Entity entity)
    {
        SL_ASSERT_MSG(physicsWorld, "DustWorld::Init() must be called before spawning entities");

        i32 slot = AllocateSlot();
        entity.id = { slot, entities[slot].id.generation };

        pendingSpawns.push_back({ entity.id, entity });
        return entity.id;
    }

    void DustWorld::DestroyEntity(EntityId id)
    {
        if (id.index < 0 || id.index >= static_cast<i32>(entities.size()))
        {
            return;
        }

        pendingDestroys.push_back(id);
    }

    Entity* DustWorld::GetEntity(EntityId id)
    {
        if (id.index < 0 || id.index >= static_cast<i32>(entities.size()))
        {
            return nullptr;
        }

        Entity& entity = entities[id.index];
        if (entity.type == ENTITY_TYPE_INVALID || entity.id != id)
        {
            return nullptr;
        }

        return &entity;
    }

    const Entity* DustWorld::GetEntity(EntityId id) const
    {
        return const_cast<DustWorld*>(this)->GetEntity(id);
    }

    void DustWorld::ApplySpawn(const PendingSpawn& spawn)
    {
        Entity& entity = entities[spawn.id.index];

        // Copy over data
        entity = spawn.entity;

        if (spawn.entity.type == ENTITY_TYPE_PROP)
        {
            sloth::RigidBodyDesc bodyDesc;
            bodyDesc.Position    = spawn.entity.position;
            bodyDesc.Rotation    = spawn.entity.rotation;
            bodyDesc.MotionType  = spawn.entity.prop.motionType;
            bodyDesc.Friction    = spawn.entity.prop.friction;
            bodyDesc.Restitution = spawn.entity.prop.restitution;

            entity.rigidBody = (spawn.entity.prop.propShape == PropShape::Box)
                ? physicsWorld->CreateBoxBody(spawn.entity.prop.halfExtents, bodyDesc)
                : physicsWorld->CreateSphereBody(spawn.entity.prop.radius, bodyDesc);
        }
    }

    void DustWorld::ApplyDestroy(EntityId id)
    {
        if (id.index < 0 || id.index >= static_cast<i32>(entities.size()))
        {
            return;
        }

        // Cancel a same-frame pending spawn rather than destroying a live entity.
        for (auto it = pendingSpawns.begin(); it != pendingSpawns.end(); ++it)
        {
            if (it->id == id)
            {
                pendingSpawns.erase(it);
                freeSlots.push_back(id.index);
                return;
            }
        }

        Entity& entity = entities[id.index];
        if (entity.type == ENTITY_TYPE_INVALID || entity.id != id)
        {
            return; // Stale id: already destroyed, or never existed.
        }

        if (entity.type == ENTITY_TYPE_PROP)
        {
            physicsWorld->DestroyBody(entity.rigidBody);
        }

        i32 nextGeneration = entity.id.generation + 1;
        entity = Entity{};
        entity.id = { id.index, nextGeneration };

        freeSlots.push_back(id.index);
    }

    void DustWorld::SyncPhysicsTransforms()
    {
        for (Entity& entity : entities)
        {
            if (entity.rigidBody.IsValid() == false) 
            {
                continue;
            }

            entity.position = physicsWorld->GetPosition(entity.rigidBody);
            entity.rotation = physicsWorld->GetRotation(entity.rigidBody);
        }
    }

    void DustWorld::FlushPendingChanges()
    {
        for (EntityId id : pendingDestroys)
        {
            ApplyDestroy(id);
        }
        pendingDestroys.clear();

        for (const PendingSpawn& spawn : pendingSpawns)
        {
            ApplySpawn(spawn);
        }
        pendingSpawns.clear();
    }

}
