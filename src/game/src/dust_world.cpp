#include "dust_world.h"

namespace dust {

    void DustWorld::Init( sloth::PhysicsWorld * newPhysicsWorld ) {
        physicsWorld = newPhysicsWorld;
    }

    i32 DustWorld::AllocateSlot() {
        if ( !freeSlots.empty() ) {
            i32 slot = freeSlots.back();
            freeSlots.pop_back();
            return slot;
        }

        entities.push_back( Entity {} );
        return static_cast<i32>( entities.size() ) - 1;
    }

    EntityId DustWorld::SpawnEntity( Entity entity ) {
        SL_ASSERT_MSG( physicsWorld, "DustWorld::Init() must be called before spawning entities" );

        i32 slot = AllocateSlot();
        entity.id = { slot, entities[slot].id.generation };

        pendingSpawns.push_back( { entity.id, entity } );
        return entity.id;
    }

    void DustWorld::DestroyEntity( EntityId id ) {
        if ( id.index < 0 || id.index >= static_cast<i32>( entities.size() ) ) {
            return;
        }

        pendingDestroys.push_back( id );
    }

    Entity * DustWorld::GetEntity( EntityId id ) {
        if ( id.index < 0 || id.index >= static_cast<i32>( entities.size() ) ) {
            return nullptr;
        }

        Entity & entity = entities[id.index];
        if ( entity.type == ENTITY_TYPE_INVALID || entity.id != id ) {
            return nullptr;
        }

        return &entity;
    }

    const Entity * DustWorld::GetEntity( EntityId id ) const {
        return const_cast<DustWorld *>( this )->GetEntity( id );
    }

    EntityId DustWorld::FindEntityByRigidBody( sloth::RigidBody body ) const {
        if ( !body.IsValid() ) {
            return INVALID_ENTITY_ID;
        }

        for ( const Entity & entity : entities ) {
            if ( entity.type != ENTITY_TYPE_INVALID && entity.rigidBody.Id == body.Id ) {
                return entity.id;
            }
        }

        return INVALID_ENTITY_ID;
    }

    void DustWorld::ApplySpawn( const PendingSpawn & spawn ) {
        Entity & entity = entities[spawn.id.index];

        // Copy over data
        entity = spawn.entity;

        if ( entity.rigidBodyData.createRigidBody == true ) {
            sloth::RigidBodyDesc bodyDesc;
            bodyDesc.position = entity.position;
            bodyDesc.rotation = entity.rotation;
            bodyDesc.motionType = entity.rigidBodyData.motionType;
            bodyDesc.friction = entity.rigidBodyData.friction;
            bodyDesc.restitution = entity.rigidBodyData.restitution;

            entity.rigidBody = ( entity.rigidBodyData.shape == RigidBodyShape::Box )
                                   ? physicsWorld->CreateBoxBody( entity.rigidBodyData.halfExtents, bodyDesc )
                                   : physicsWorld->CreateSphereBody( entity.rigidBodyData.radius, bodyDesc );
        }
    }

    void DustWorld::ApplyDestroy( EntityId id ) {
        if ( id.index < 0 || id.index >= static_cast<i32>( entities.size() ) ) {
            return;
        }

        // Cancel a same-frame pending spawn rather than destroying a live entity.
        for ( auto it = pendingSpawns.begin(); it != pendingSpawns.end(); ++it ) {
            if ( it->id == id ) {
                pendingSpawns.erase( it );
                freeSlots.push_back( id.index );
                return;
            }
        }

        Entity & entity = entities[id.index];
        if ( entity.type == ENTITY_TYPE_INVALID || entity.id != id ) {
            return; // Stale id: already destroyed, or never existed.
        }

        if ( entity.rigidBody.IsValid() ) {
            physicsWorld->DestroyBody( entity.rigidBody );
        }

        i32 nextGeneration = entity.id.generation + 1;
        entity = Entity {};
        entity.id = { id.index, nextGeneration };

        freeSlots.push_back( id.index );
    }

    void DustWorld::Update( f32 deltaTime ) {
        // TODO: LOD simulation

        for ( Entity & entity : entities ) {
            if ( entity.action.targetId != INVALID_ENTITY_ID ) {
                Entity * targetEntity = GetEntity( entity.action.targetId );
                if ( targetEntity != nullptr ) {
                    switch ( entity.action.type ) {
                        case ENTITY_ACTION_TYPE_MINING_ORE: {
                            if ( glm::distance( entity.position, targetEntity->position ) >= 10 ) {
                                break;
                            }

                            InventoryItemType itemType = INVENTORY_ITEM_TYPE_ORE_IRON;
                            if ( targetEntity->oreNode.type == ORE_NODE_TYPE_IRON ) {
                                entity.action.progress += deltaTime;
                                itemType = INVENTORY_ITEM_TYPE_ORE_IRON;
                            }

                            if ( entity.action.progress >= 0.5f ) {
                                bool result = InvetoryAddItem( entity.inventory, itemType, 1 );
                                entity.action.progress = result  == true ? 0.0f : 0.99f;
                            }
                        } break;
                    }
                }
            }
        }
    }

    void DustWorld::SyncPhysicsTransforms() {
        for ( Entity & entity : entities ) {
            if ( entity.rigidBody.IsValid() == false ) {
                continue;
            }

            entity.position = physicsWorld->GetPosition( entity.rigidBody );
            entity.rotation = physicsWorld->GetRotation( entity.rigidBody );
        }
    }

    void DustWorld::FlushPendingChanges() {
        for ( EntityId id : pendingDestroys ) {
            ApplyDestroy( id );
        }
        pendingDestroys.clear();

        for ( const PendingSpawn & spawn : pendingSpawns ) {
            ApplySpawn( spawn );
        }
        pendingSpawns.clear();
    }

} // namespace dust
