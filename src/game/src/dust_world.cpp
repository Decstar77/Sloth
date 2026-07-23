#include "dust_world.h"

namespace dust {

    void DustWorld::Init( sloth::PhysicsWorld * newPhysicsWorld ) {
        physicsWorld = newPhysicsWorld;

        factionNeutral.type     = FACTION_TYPE_NEUTRAL;
        factionRemnant.type     = FACTION_TYPE_REMNANT;
        factionRustborn.type    = FACTION_TYPE_RUSTBORN;
        factionZenith.type      = FACTION_TYPE_ZENITH;
        factionPlayer.type      = FACTION_TYPE_PLAYER;
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
        if ( id == INVALID_ENTITY_ID || id.index < 0 || id.index >= static_cast<i32>( entities.size() ) ) {
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

    Entity * DustWorld::QueryClosestOreNode( glm::vec3 pos, OreNodeType type ) {
        f32 maxDist = FLT_MAX;
        Entity * found = nullptr;
        for ( Entity & entity : entities ) {
            if ( entity.type == ENTITY_TYPE_ORE_NODE ) {
                if ( entity.oreNode.type == type ) {
                    f32 dist = glm::distance( entity.position, pos );
                    if ( dist < maxDist ) {
                        maxDist = dist;
                        found = &entity;
                    }
                }
            }
        }

        return found;
    }

    Entity * DustWorld::QueryClosestShop( glm::vec3 pos, FactionType faction ) {
        f32 maxDist = FLT_MAX;
        Entity * found = nullptr;
        for ( Entity & entity : entities ) {
            if ( entity.type == ENTITY_TYPE_BUILDING && entity.building.type == BUILDING_TYPE_SHOP && entity.faction == faction ) {
                f32 dist = glm::distance( entity.position, pos );
                if ( dist < maxDist ) {
                    maxDist = dist;
                    found = &entity;
                }
            }
        }

        return found;
    }

    void DustWorld::ActionIdle( Entity * entity ) {
        SL_ASSERT( entity );

        entity->action = {};
        entity->action.type = ENTITY_ACTION_TYPE_IDLE;
    }

    void DustWorld::ActionTravelTo( Entity * entity, EntityId target ) {
        SL_ASSERT( entity );
        if ( entity->playerControlled == true ) {
            return;
        }

        entity->action = {};
        entity->action.type = ENTITY_ACTION_TYPE_TRAVELING;
        entity->action.targetId = target;
    }

    void DustWorld::ActionMineOre( Entity * entity, EntityId oreNodeId ) {
        SL_ASSERT( entity );

        entity->action = {};
        entity->action.type = ENTITY_ACTION_TYPE_MINING_ORE;
        entity->action.targetId = oreNodeId;
    }

    void DustWorld::ActionSellOre( Entity * entity, EntityId shopId ) {
        SL_ASSERT( entity );

        entity->action = {};
        entity->action.type = ENTITY_ACTION_TYPE_SELL_ORE;
        entity->action.targetId = shopId;
    }

    void DustWorld::ActionPlayerControl( Entity * entity, EntityId target ) {
        SL_ASSERT( entity );

        entity->action = {};
        entity->action.type = ENTITY_ACTION_TYPE_PLAYER_CONTROL;
        entity->action.targetId = target;
    }

    bool DustWorld::ShopSellItem( Entity * shop, Entity * seller, i32 sellingItemIndex ) {
        SL_ASSERT( shop );
        SL_ASSERT( seller );

        if ( shop->type != ENTITY_TYPE_BUILDING || shop->building.type != BUILDING_TYPE_SHOP ) {
            return false;
        }

        if ( sellingItemIndex < 0 || static_cast<u32>( sellingItemIndex ) >= seller->inventory.items.GetCount() ) {
            return false;
        }

        const InventoryItem item = seller->inventory.items[sellingItemIndex];
        if ( InvetoryAddItem( shop->inventory, item.type, static_cast<i32>( item.amount ) ) == false ) {
            return false;
        }

        const i64 amountRemoved = InvetoryRemoveItemByIndex( seller->inventory, sellingItemIndex );
        const i64 HackyItemPrice = 2;
        const i64 finalPrice = HackyItemPrice * amountRemoved;

        Faction & faction = GetFaction( seller->faction );

        // Important to up both the local entity credits and faction credits to keep things balanced / correctly audited
        seller->credits += finalPrice;
        faction.credits += finalPrice;

        return true;
    }

    bool DustWorld::ShopBuyItem( Entity * shop, Entity * buyer, i32 shopItemIndex ) {
        SL_ASSERT( shop );
        SL_ASSERT( buyer );

        if ( shop->type != ENTITY_TYPE_BUILDING || shop->building.type != BUILDING_TYPE_SHOP ) {
            return false;
        }

        if ( shopItemIndex < 0 || static_cast<u32>( shopItemIndex ) >= shop->inventory.items.GetCount() ) {
            return false;
        }

        const InventoryItem item = shop->inventory.items[shopItemIndex];

        const i64 HackyItemPrice = 2;
        const i64 finalPrice = HackyItemPrice * item.amount;

        Faction & faction = GetFaction( buyer->faction );
        if ( faction.credits < finalPrice ) {
            return false;
        }

        if ( InvetoryAddItem( buyer->inventory, item.type, static_cast<i32>( item.amount ) ) == false ) {
            return false;
        }

        InvetoryRemoveItemByIndex( shop->inventory, shopItemIndex );

        // Important to up both the local entity credits and faction credits to keep things balanced / correctly audited
        buyer->credits -= finalPrice;
        faction.credits -= finalPrice;

        return true;
    }

    bool DustWorld::RefineryPurchaseItem( Entity * buyer, EntityId refineryId, InventoryItemType itemType ) {
        SL_ASSERT( buyer );

        Entity * refinery = GetEntity( refineryId );
        if ( refinery == nullptr || refinery->type != ENTITY_TYPE_BUILDING || refinery->building.type != BUILDING_TYPE_REFINERY ) {
            return false;
        }

        const Price price = RefineryPriceForItem( itemType );

        Faction & buyerFaction = GetFaction( buyer->faction );
        if ( buyerFaction.credits < price.credits ) {
            return false;
        }

        constexpr InventoryItemType oreTypes[] = {
            INVENTORY_ITEM_TYPE_ORE_IRON,
            INVENTORY_ITEM_TYPE_ORE_COPPER,
            INVENTORY_ITEM_TYPE_ORE_SULPHUR,
            INVENTORY_ITEM_TYPE_ORE_ALUMINUM,
            INVENTORY_ITEM_TYPE_ORE_CRUDE_OIL,
            INVENTORY_ITEM_TYPE_ORE_WATER,
            INVENTORY_ITEM_TYPE_ORE_SILICON,
        };
        const i64 oreCosts[] = {
            price.oreIron, price.oreCopper, price.oreSulphur,
            price.oreAluminum, price.oreCrudeOil, price.oreWater, price.oreSilicon,
        };

        for ( u32 i = 0; i < SL_ARRAY_COUNT( oreTypes ); i++ ) {
            if ( InventoryGetTotalAmount( buyer->inventory, oreTypes[i] ) < oreCosts[i] ) {
                return false;
            }
        }

        // Check inventory space before touching any resources so a full
        // inventory fails cleanly instead of charging without delivering.
        if ( InvetoryAddItem( buyer->inventory, itemType, 1 ) == false ) {
            return false;
        }

        for ( u32 i = 0; i < SL_ARRAY_COUNT( oreTypes ); i++ ) {
            if ( oreCosts[i] > 0 ) {
                InventoryRemoveAmount( buyer->inventory, oreTypes[i], oreCosts[i] );
            }
        }

        buyerFaction.credits -= price.credits;
        refinery->building.credits += price.credits;

        return true;
    }

    void DustWorld::ApplySpawn( const PendingSpawn & spawn ) {
        Entity & entity = entities[spawn.id.index];

        // Copy over data
        entity = spawn.entity;

        Faction & faction = GetFaction( entity.faction );
        faction.entities.push_back( entity.id );

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

    Faction & DustWorld::GetFaction( FactionType faction ) {
        switch ( faction ) {
            case FACTION_TYPE_NEUTRAL: {
                return factionNeutral;
            } break;
            case FACTION_TYPE_REMNANT: {
                return factionRemnant;
            } break;
            case FACTION_TYPE_RUSTBORN: {
                return factionRustborn;
            } break;
            case FACTION_TYPE_ZENITH: {
                return factionZenith;
            } break;
            case FACTION_TYPE_PLAYER: {
                return factionPlayer;
            } break;
            default: {
                SL_ASSERT_MSG( false, "INVALID FACTION" );
            } break;
        } 

        return factionNeutral;
    }

    void DustWorld::Update( f32 deltaTime ) {
        // TODO: LOD simulation
        for ( Entity & entity : entities ) {

            // Entity thinking
            if ( entity.type == ENTITY_TYPE_VEHICLE && entity.playerControlled == false ) {
                if ( entity.action.type == ENTITY_ACTION_TYPE_IDLE ) {
                    if ( entity.inventory.items.IsEmpty() ) {
                        Entity * oreNode = QueryClosestOreNode( entity.position, ORE_NODE_TYPE_IRON );
                        if ( oreNode != nullptr ) {
                            if ( glm::distance( entity.position, oreNode->position ) <= 10 ) {
                                ActionMineOre( &entity, oreNode->id );
                            } else {
                                ActionTravelTo( &entity, oreNode->id );
                            }
                        }
                    }
                    else {
                        Entity * shop = QueryClosestShop( entity.position, entity.faction );
                        if ( shop != nullptr ) {
                            if ( glm::distance( entity.position, shop->position ) <= 10 ) {
                                ActionSellOre( &entity, shop->id );
                            }
                            else {
                                ActionTravelTo( &entity, shop->id );
                            }
                        }
                    }
                }
            }

            // Entity actions
            if ( entity.action.targetId != INVALID_ENTITY_ID ) {
                Entity * targetEntity = GetEntity( entity.action.targetId );
                if ( targetEntity != nullptr ) {
                    switch ( entity.action.type ) {
                        //=================================
                        case ENTITY_ACTION_TYPE_TRAVELING: {
                            const f32 ArrivalCirlce = 15.0f; // Hack
                            if ( glm::distance( entity.position, targetEntity->position ) <= ArrivalCirlce ) {
                                ActionIdle( &entity );
                                break;
                            }

                            DriveVehicleToward( *physicsWorld, entity, targetEntity->position, deltaTime );
                        } break;
                        //=================================
                        case ENTITY_ACTION_TYPE_MINING_ORE: {
                            SL_ASSERT( targetEntity->type == ENTITY_TYPE_ORE_NODE );
                            if ( glm::distance( entity.position, targetEntity->position ) >= 15.0f ) {
                                ActionTravelTo( &entity, targetEntity->id );
                                break;
                            }

                            InventoryItemType itemType = OreNodeTypeToItemType( targetEntity->oreNode.type );
                            entity.action.progress += deltaTime;

                            if ( entity.action.progress >= 0.5f ) {
                                bool result = InvetoryAddItem( entity.inventory, itemType, 1 );
                                entity.action.progress = 0.0f;
                                if ( result == false ) {
                                    ActionIdle( &entity );
                                }
                            }
                        } break;
                        //=================================
                        case ENTITY_ACTION_TYPE_SELL_ORE: {
                            SL_ASSERT( targetEntity->type == ENTITY_TYPE_BUILDING && targetEntity->building.type == BUILDING_TYPE_SHOP );
                            if ( glm::distance( entity.position, targetEntity->position ) >= 15.0f ) {
                                ActionTravelTo( &entity, targetEntity->id );
                                break;
                            }

                            const i32 inventoryCount = static_cast<i32>( entity.inventory.items.GetCount() );
                            for ( i32 itemIndex = 0; itemIndex < inventoryCount; itemIndex++ ) {
                                if ( ItemIsRawMaterial( entity.inventory.items[itemIndex].type ) == true ) {
                                    ShopSellItem( targetEntity, &entity, itemIndex );
                                }
                            }

                            ActionIdle( &entity );
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
