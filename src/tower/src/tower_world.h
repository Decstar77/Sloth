#pragma once

#include "tower_entity.h"
#include <physics/sloth_physics_world.h>

#include <vector>

namespace tower {

    class TowerWorld {
    public:
        void                        Init( sloth::PhysicsWorld * physicsWorld );

        EntityId                    SpawnEntity( Entity entity );
        void                        DestroyEntity( EntityId id );

        Entity *                    GetEntity( EntityId id );
        const Entity *              GetEntity( EntityId id ) const;
        EntityId                    FindEntityByRigidBody( sloth::RigidBody body ) const;

        const std::vector<Entity> & GetEntities() const { return entities; }

        void                        SyncPhysicsTransforms();
        void                        FlushPendingChanges();

    private:
        struct PendingSpawn {
            EntityId   id;
            Entity     entity;
        };

        i32         AllocateSlot();
        void        ApplySpawn( const PendingSpawn & spawn );
        void        ApplyDestroy( EntityId id );

    private:
        sloth::PhysicsWorld * physicsWorld = nullptr;

        std::vector<Entity>         entities;
        std::vector<i32>            freeSlots;

        std::vector<PendingSpawn>   pendingSpawns;
        std::vector<EntityId>       pendingDestroys;
    };

}
