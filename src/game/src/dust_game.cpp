#include "dust_game.h"

#include <core/sloth_engine.h>
#include <font/sloth_font.h>
#include <gui/sloth_gui_widgets.h>
#include <gui/sloth_gui_context.h>
#include <renderer/sloth_debug_renderer.h>
#include <renderer/sloth_geometry.h>

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace sloth;

namespace dust {

    static const char * VertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aColor;

        uniform mat4 uViewProjection;
        uniform mat4 uModel;

        out vec3 vColor;

        void main()
        {
            vColor = aColor;
            gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
        }
    )";

    static const char * FragmentShaderSource = R"(
        #version 450 core
        in vec3 vColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    static std::unique_ptr<StaticMesh> UploadMesh( const MeshData & data ) {
        return std::make_unique<StaticMesh>( data.vertices.data(), static_cast<u32>( data.vertices.size() ), data.indices.data(), static_cast<u32>( data.indices.size() ) );
    }

    void DustGame::Init() {
        glEnable( GL_DEPTH_TEST );

        world.Init( &physicsWorld );

        shader = std::make_unique<Shader>( VertexShaderSource, FragmentShaderSource );

        // Static floor.
        {
            glm::vec3 halfExtents( 70.0f, 0.5f, 70.0f );
            floorMesh = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, { 0.5f, 0.5f, 0.55f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_PROP, FACTION_TYPE_NEUTRAL, { 0.0f, -0.5f, 0.0f } );
            entity.renderModel = { shader.get(), floorMesh.get() };
            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = halfExtents;
            entity.rigidBodyData.motionType = BodyMotionType::Static;
            world.SpawnEntity( entity );
        }

        // Player dune buggy.
        {
            VehicleData vehicleDefaults;
            glm::vec3 chassisHalfExtents = vehicleDefaults.chassisHalfExtents;

            buggyChassisMesh = UploadMesh( Geometry::CreateBox( chassisHalfExtents.x * 2.0f, chassisHalfExtents.y * 2.0f, chassisHalfExtents.z * 2.0f, { 0.90f, 0.85f, 0.20f } ) );
            buggyWheelMesh = UploadMesh( Geometry::CreateCylinder( vehicleDefaults.wheelRadius, vehicleDefaults.wheelWidth, 12, { 0.05f, 0.05f, 0.05f } ) );

            Entity entity = MakeEntity( ENTITY_TYPE_VEHICLE, FACTION_TYPE_PLAYER, { 0.0f, 3.0f, 55.0f } );
            entity.playerControlled = true;
            entity.renderModel = { shader.get(), buggyChassisMesh.get() };

            entity.rigidBodyData.shape = RigidBodyShape::Box;
            entity.rigidBodyData.halfExtents = chassisHalfExtents;
            entity.rigidBodyData.restitution = 0.1f;
            entity.rigidBodyData.motionType = BodyMotionType::Dynamic;
            entity.rigidBodyData.friction = 0.05f; // // Low, not zero: the chassis is a flat box directly touching the ground (no wheel model yet)
            entity.rigidBodyData.restitution = 0.05f;

            playerVehicleId = world.SpawnEntity( entity );
        }

        // Ore nodes, one of each type, clustered in the middle of the map.
        {
            struct OreNodeSpawn {
                OreNodeType type;
                glm::vec3   color;
                glm::vec3   position;
            };

            const OreNodeSpawn spawns[] = {
                { ORE_NODE_TYPE_IRON,      { 0.70f, 0.70f, 0.35f }, { -12, 0, -16 } },
                { ORE_NODE_TYPE_COPPER,    { 0.80f, 0.45f, 0.20f }, {   0, 0, -16 } },
                { ORE_NODE_TYPE_SULPHUR,   { 0.90f, 0.85f, 0.20f }, {  12, 0, -16 } },
                { ORE_NODE_TYPE_ALUMINUM,  { 0.75f, 0.78f, 0.80f }, { -12, 0,  -8 } },
                { ORE_NODE_TYPE_CRUDE_OIL, { 0.10f, 0.08f, 0.05f }, {   0, 0,  -8 } },
                { ORE_NODE_TYPE_WATER,     { 0.20f, 0.45f, 0.85f }, {  12, 0,  -8 } },
                { ORE_NODE_TYPE_SILICON,   { 0.60f, 0.60f, 0.65f }, {   0, 0,   8 } },
            };

            glm::vec3 halfExtents( 4.0f, 2.3f, 4.0f );
            for ( usize i = 0; i < sizeof( spawns ) / sizeof( spawns[0] ); i++ ) {
                const OreNodeSpawn & spawn = spawns[i];
                oreNodeMeshes[i] = UploadMesh( Geometry::CreateBox( halfExtents.x * 2.0f, halfExtents.y * 2.0f, halfExtents.z * 2.0f, spawn.color ) );

                Entity entity = MakeEntity( ENTITY_TYPE_ORE_NODE, FACTION_TYPE_NEUTRAL, spawn.position );
                entity.renderModel = { shader.get(), oreNodeMeshes[i].get() };
                entity.oreNode.type = spawn.type;
                entity.oreNode.amount = 2000;

                entity.rigidBodyData.shape = RigidBodyShape::Box;
                entity.rigidBodyData.halfExtents = halfExtents;
                entity.rigidBodyData.motionType = BodyMotionType::Static;

                world.SpawnEntity( entity );
            }
        }

        // Factions: each gets a shop and 2 AI dune buggies that mine the
        // shared ore cluster in the middle of the map and sell back home.
        {
            struct FactionSpawn {
                FactionType type;
                glm::vec3   color;
                glm::vec3   shopPosition;
                glm::vec3   aiPositions[2];
            };

            const FactionSpawn factionSpawns[] = {
                { FACTION_TYPE_REMNANT,  { 0.85f, 0.50f, 0.15f }, {   0.0f, 0.0f, -50.0f }, { { -6.0f, 3.0f, -45.0f }, {  6.0f, 3.0f, -45.0f } } },
                { FACTION_TYPE_RUSTBORN, { 0.65f, 0.25f, 0.15f }, { -45.0f, 0.0f,  30.0f }, { { -50.0f, 3.0f, 24.0f }, { -40.0f, 3.0f, 24.0f } } },
                { FACTION_TYPE_ZENITH,   { 0.20f, 0.60f, 0.85f }, {  45.0f, 0.0f,  30.0f }, { {  50.0f, 3.0f, 24.0f }, {  40.0f, 3.0f, 24.0f } } },
            };

            VehicleData vehicleDefaults;
            glm::vec3 chassisHalfExtents = vehicleDefaults.chassisHalfExtents;
            glm::vec3 shopHalfExtents( 3.0f, 2.3f, 3.0f );

            for ( usize i = 0; i < sizeof( factionSpawns ) / sizeof( factionSpawns[0] ); i++ ) {
                const FactionSpawn & fs = factionSpawns[i];

                factionChassisMeshes[i] = UploadMesh( Geometry::CreateBox( chassisHalfExtents.x * 2.0f, chassisHalfExtents.y * 2.0f, chassisHalfExtents.z * 2.0f, fs.color ) );
                factionShopMeshes[i] = UploadMesh( Geometry::CreateBox( shopHalfExtents.x * 2.0f, shopHalfExtents.y * 2.0f, shopHalfExtents.z * 2.0f, fs.color ) );

                // Shop
                {
                    Entity entity = MakeEntity( ENTITY_TYPE_BUILDING, fs.type, fs.shopPosition );
                    entity.renderModel = { shader.get(), factionShopMeshes[i].get() };
                    entity.building.type = BUILDING_TYPE_SHOP;
                    entity.building.credits = 1000;

                    entity.rigidBodyData.shape = RigidBodyShape::Box;
                    entity.rigidBodyData.halfExtents = shopHalfExtents;
                    entity.rigidBodyData.motionType = BodyMotionType::Static;

                    world.SpawnEntity( entity );
                }

                
                // Refinery
                {
                    Entity entity = MakeEntity( ENTITY_TYPE_BUILDING, fs.type, fs.shopPosition + glm::vec3(15, 0, 0));
                    entity.renderModel = { shader.get(), factionShopMeshes[i].get() };
                    entity.building.type = BUILDING_TYPE_REFINERY;
                    entity.building.credits = 1000;

                    entity.rigidBodyData.shape = RigidBodyShape::Box;
                    entity.rigidBodyData.halfExtents = shopHalfExtents;
                    entity.rigidBodyData.motionType = BodyMotionType::Static;

                    world.SpawnEntity( entity );
                }

                // AI dune buggies
                for ( const glm::vec3 & aiPosition : fs.aiPositions ) {
                    Entity entity = MakeEntity( ENTITY_TYPE_VEHICLE, fs.type, aiPosition );
                    entity.renderModel = { shader.get(), factionChassisMeshes[i].get() };

                    entity.rigidBodyData.shape = RigidBodyShape::Box;
                    entity.rigidBodyData.halfExtents = chassisHalfExtents;
                    entity.rigidBodyData.restitution = 0.1f;
                    entity.rigidBodyData.motionType = BodyMotionType::Dynamic;
                    entity.rigidBodyData.friction = 0.05f; // Low, not zero: the chassis is a flat box directly touching the ground (no wheel model yet)
                    entity.rigidBodyData.restitution = 0.05f;

                    world.SpawnEntity( entity );
                }
            }
        }

        camera.SetFocusPoint( { 0.0f, 0.0f, 0.0f } );
        camera.SetDistance( 30.0f );
    }

    void DustGame::Shutdown() {
        floorMesh.reset();
        sphereMesh.reset();
        boxMesh.reset();
        buggyChassisMesh.reset();
        buggyWheelMesh.reset();
        for ( auto & mesh : oreNodeMeshes ) {
            mesh.reset();
        }
        for ( auto & mesh : factionChassisMeshes ) {
            mesh.reset();
        }
        for ( auto & mesh : factionShopMeshes ) {
            mesh.reset();
        }
        shader.reset();
    }

    void DustGame::UpdateAndRender( f32 deltaTime, sloth::GuiFrame & guiFrame ) {
        camera.Update( deltaTime );

        PlayerUpdateVehicleControl( deltaTime );
        world.Update( deltaTime );
        physicsWorld.Update( deltaTime );
        world.SyncPhysicsTransforms();

        // Camera follows the player vehicle: overrides the manual WASD pan
        // camera.Update() just computed, since those keys now drive the
        // buggy instead.
        if ( Entity * vehicle = world.GetEntity( playerVehicleId ) ) {
            camera.SetFocusPoint( vehicle->position );
        }

        // Entity spawn/destroy requests buffered this frame are applied once
        // here, at the end of the frame's update.
        world.FlushPendingChanges();


        Render();
        RenderUI( guiFrame );

        // Must run after RenderUI(): panels/buttons only claim
        // guiFrame.ctx's hot/active id when they're actually submitted, which
        // happens inside RenderUI(). Checking WantsMouseInput() any earlier
        // in the frame would still see the id cleared by ctx.NewFrame(),
        // making it always false and letting world-picking click straight
        // through open panels.
        PlayerUpdateTargeting( guiFrame );
    }

    void DustGame::PlayerUpdateVehicleControl( f32 deltaTime ) {
        Entity * entity = world.GetEntity( playerVehicleId );
        if ( !entity || entity->type != ENTITY_TYPE_VEHICLE || !entity->rigidBody.IsValid() ) {
            return;
        }

        VehicleData & vehicle = entity->vehicle;
        if ( entity->playerControlled == false ) {
            return;
        }

        Input & input = Engine::Get().GetInput();

        bool hadInput = false;
        f32 throttle = 0.0f;
        if ( input.IsKeyDown( Key::W ) ) {
            throttle += 1.0f;
            hadInput = true;
        }
        if ( input.IsKeyDown( Key::S ) ) {
            throttle -= 1.0f;
            hadInput = true;
        }

        f32 steer = 0.0f;
        if ( input.IsKeyDown( Key::A ) ) {
            steer += 1.0f;
            hadInput = true;
        }
        if ( input.IsKeyDown( Key::D ) ) {
            steer -= 1.0f;
            hadInput = true;
        }

        if ( hadInput == true ) {
            world.ActionPlayerControl( entity );
        }

        DriveVehicle( physicsWorld, *entity, throttle, steer, deltaTime );
    }

    void DustGame::PlayerUpdateTargeting( sloth::GuiFrame & guiFrame ) {
        Input & input = Engine::Get().GetInput();
        if ( !input.IsMouseButtonPressed( MouseButton::Left ) ) {
            return;
        }

        if ( guiFrame.ctx.WantsMouseInput() == true ) {
            return;
        }

        Entity * player = world.GetEntity( playerVehicleId );
        if ( player == nullptr ) {
            return;
        }

        Window & window = Engine::Get().GetWindow();

        glm::vec3 rayOrigin, rayDirection;
        camera.GetCamera().ScreenPointToRay(
            static_cast<f32>( input.GetMouseX() ), static_cast<f32>( input.GetMouseY() ),
            static_cast<f32>( window.GetWidth() ), static_cast<f32>( window.GetHeight() ),
            rayOrigin, rayDirection );

        constexpr f32 maxRayDistance = 500.0f;
        RayCastHit hit;
        if ( !physicsWorld.Raycast( rayOrigin, rayDirection, maxRayDistance, hit ) ) {
            return;
        }

        EntityId hitId = world.FindEntityByRigidBody( hit.body );
        if ( hitId != INVALID_ENTITY_ID ) {
            Entity * targetEntity = world.GetEntity( hitId );
            SL_ASSERT( targetEntity );
            if ( targetEntity != nullptr ) {
                switch ( targetEntity->type ) {
                    case ENTITY_TYPE_PROP: { world.ActionIdle( player ); } break;
                    case ENTITY_TYPE_VEHICLE: { player->action.targetId = hitId; } break;
                    case ENTITY_TYPE_ORE_NODE: { world.ActionMineOre( player, hitId ); } break;
                    case ENTITY_TYPE_BUILDING: { player->action.targetId = hitId; } break;
                }
            }
        }
    }

    void DustGame::RenderUI( GuiFrame & guiFrame ) {
        if ( !guiFrame.font.IsLoaded() ) {
            return;
        }
        
        Entity * player = world.GetEntity( playerVehicleId );
        if ( player == nullptr ) {
            return;
        }

        if ( Engine::Get().GetInput().IsKeyPressed( Key::I ) ) {
            inventoryOpen = !inventoryOpen;
        }

        // Inventory grid panel, toggled with 'I'. One button per slot,
        // laid out from Inventory::xSize/ySize; slots beyond the current
        // item count are drawn empty.
        if ( inventoryOpen ) {
            const Inventory & inventory = player->inventory;

            i32 gridCols = inventory.xSize > 0 ? inventory.xSize : 1;
            i32 gridRows = inventory.ySize > 0 ? inventory.ySize : 1;

            constexpr f32 slotSize = 48.0f;
            constexpr f32 slotGap = 8.0f;

            f32 gridWidth = static_cast<f32>( gridCols ) * slotSize + static_cast<f32>( gridCols - 1 ) * slotGap;
            f32 gridHeight = static_cast<f32>( gridRows ) * slotSize + static_cast<f32>( gridRows - 1 ) * slotGap;

            // Panel spans the grid plus BeginPanel's own content padding
            // on all sides and its title bar up top; centred as the
            // default first-open position, then draggable from there.
            constexpr f32 panelPadding = 12.0f; // Matches BeginPanel's PanelContentPadding.
            constexpr f32 titleBarHeight = 28.0f; // Matches BeginPanel's PanelTitleBarHeight.
            glm::vec2 panelSize {
                gridWidth + panelPadding * 2.0f,
                gridHeight + panelPadding * 2.0f + titleBarHeight,
            };

            Window & window = Engine::Get().GetWindow();
            glm::vec2 defaultPos {
                ( static_cast<f32>( window.GetWidth() ) - panelSize.x ) * 0.5f + 600,
                ( static_cast<f32>( window.GetHeight() ) - panelSize.y ) * 0.5f,
            };

            PanelResult invPanel = BeginPanel( guiFrame, "Inventory##InvPanel", defaultPos, panelSize );

            glm::vec2 gridOrigin = invPanel.contentMin;

            i32 slotCount = gridCols * gridRows;
            i32 slotItemCount = static_cast<i32>( inventory.items.GetCount() );
            for ( i32 slot = 0; slot < slotCount; ++slot ) {
                i32 col = slot % gridCols;
                i32 row = slot / gridCols;

                glm::vec2 slotMin = gridOrigin + glm::vec2( static_cast<f32>( col ) * ( slotSize + slotGap ), static_cast<f32>( row ) * ( slotSize + slotGap ) );
                glm::vec2 slotMax = slotMin + glm::vec2( slotSize, slotSize );

                LargeString slotLabel;
                if ( slot < slotItemCount ) {
                    const InventoryItem & item = inventory.items[slot];
                    slotLabel.Format( "%s (%d)##InvSlot%d", ToShortCode( item.type ), item.amount, slot );
                } else {
                    slotLabel.Format( "##InvSlot%d", slot );
                }

                Button( guiFrame, slotLabel.View(), slotMin, slotMax );
            }

            EndPanel( guiFrame );
        }

        Entity * target = world.GetEntity( player->action.targetId );
        if ( target != nullptr && target->type == ENTITY_TYPE_BUILDING  ) {
            const f32 InteractionDist = 15;
            f32 dist = glm::distance( player->position, target->position );
            if ( dist <= InteractionDist && target->building.type == BUILDING_TYPE_REFINERY ) {
                constexpr InventoryItemType refineryItems[] = {
                    INVENTORY_ITEM_TYPE_STEEL_INGOT,
                    INVENTORY_ITEM_TYPE_COPPER_WIRE,
                    INVENTORY_ITEM_TYPE_ALUMINUM_PLATE,
                    INVENTORY_ITEM_TYPE_PETROL,
                    INVENTORY_ITEM_TYPE_LUBRICANT,
                    INVENTORY_ITEM_TYPE_GLASS,
                };

                constexpr f32 rowWidth = 260.0f;
                constexpr f32 rowHeight = 36.0f;
                constexpr f32 rowGap = 8.0f;
                constexpr f32 panelPadding = 12.0f;   // Matches BeginPanel's PanelContentPadding.
                constexpr f32 titleBarHeight = 28.0f; // Matches BeginPanel's PanelTitleBarHeight.

                constexpr i32 rowCount = static_cast<i32>( SL_ARRAY_COUNT( refineryItems ) ) + 1; // +1 for the Close button.
                glm::vec2 panelSize {
                    rowWidth + panelPadding * 2.0f,
                    static_cast<f32>( rowCount ) * ( rowHeight + rowGap ) - rowGap + panelPadding * 2.0f + titleBarHeight,
                };

                Window & window = Engine::Get().GetWindow();
                glm::vec2 defaultPos {
                    ( static_cast<f32>( window.GetWidth() ) - panelSize.x ) * 0.5f - 500,
                    ( static_cast<f32>( window.GetHeight() ) - panelSize.y ) * 0.5f,
                };

                PanelResult panel = BeginPanel( guiFrame, "Refinery##RefineryPanel", defaultPos, panelSize );

                glm::vec2 cursor = panel.contentMin;
                for ( InventoryItemType itemType : refineryItems ) {
                    const Price price = RefineryPriceForItem( itemType );

                    LargeString rowLabel;
                    rowLabel.Format( "%s - %dcr##RefineryBuy%d", ToString( itemType ), price.credits, static_cast<i32>( itemType ) );

                    glm::vec2 rowMin = cursor;
                    glm::vec2 rowMax = rowMin + glm::vec2( rowWidth, rowHeight );
                    if ( Button( guiFrame, rowLabel.View(), rowMin, rowMax ) ) {
                        world.RefineryPurchaseItem( player, target->id, itemType );
                    }

                    cursor.y += rowHeight + rowGap;
                }

                EndPanel( guiFrame );
            } else if ( dist <= InteractionDist && target->building.type == BUILDING_TYPE_SHOP ) {
                constexpr f32 rowWidth = 260.0f;
                constexpr f32 rowHeight = 36.0f;
                constexpr f32 rowGap = 8.0f;
                constexpr f32 panelPadding = 12.0f;   // Matches BeginPanel's PanelContentPadding.
                constexpr f32 titleBarHeight = 28.0f; // Matches BeginPanel's PanelTitleBarHeight.

                const i32 itemCount = static_cast<i32>( player->inventory.items.GetCount() );
                const i32 rowCount = itemCount > 0 ? itemCount : 1; // At least one row, for the "empty" message.
                glm::vec2 panelSize {
                    rowWidth + panelPadding * 2.0f,
                    static_cast<f32>( rowCount ) * ( rowHeight + rowGap ) - rowGap + panelPadding * 2.0f + titleBarHeight,
                };

                Window & window = Engine::Get().GetWindow();
                glm::vec2 defaultPos {
                    ( static_cast<f32>( window.GetWidth() ) - panelSize.x ) * 0.5f - 500,
                    ( static_cast<f32>( window.GetHeight() ) - panelSize.y ) * 0.5f,
                };

                PanelResult panel = BeginPanel( guiFrame, "Shop##ShopPanel", defaultPos, panelSize );

                glm::vec2 cursor = panel.contentMin;
                if ( itemCount == 0 ) {
                    Label( guiFrame, "Inventory empty", cursor + glm::vec2( 0.0f, rowHeight * 0.5f ), 16.0f, { 1.0f, 1.0f, 1.0f, 1.0f } );
                } else {
                    // Selling mutates the player's inventory, which would invalidate the remaining indices/items this loop is iterating over
                    bool sold = false;
                    for ( i32 i = 0; i < itemCount && !sold; i++ ) {
                        const InventoryItem & item = player->inventory.items[i];

                        LargeString rowLabel;
                        rowLabel.Format( "%s x%d##ShopSell%d", ToString( item.type ), static_cast<i32>( item.amount ), i );

                        glm::vec2 rowMin = cursor;
                        glm::vec2 rowMax = rowMin + glm::vec2( rowWidth, rowHeight );
                        if ( Button( guiFrame, rowLabel.View(), rowMin, rowMax ) ) {
                            sold = world.ShopSellItem( target, player, i );
                        }

                        cursor.y += rowHeight + rowGap;
                    }
                }

                EndPanel( guiFrame );
            }
        }
    }

    const Entity * DustGame::GetPlayer() const {
        const Entity * player = world.GetEntity( playerVehicleId );
        if ( !player ) {
            return nullptr;
        }

        return player;
    }

    const Entity * DustGame::GetPlayerTarget() const {
        const Entity * player = world.GetEntity( playerVehicleId );
        if ( !player ) {
            return nullptr;
        }

        return world.GetEntity( player->action.targetId );
    }

    i64 DustGame::GetPlayerCredits() const {
        return world.GetPlayerCredits();
    }

    void DustGame::Render() {
        glClearColor( 0.10f, 0.30f, 0.80f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        glm::mat4 viewProjection = camera.GetCamera().GetViewProjectionMatrix();

        for ( const Entity & entity : world.GetEntities() ) {
            if ( !entity.renderModel.shader || !entity.renderModel.mesh ) {
                continue;
            }

            if ( entity.type == ENTITY_TYPE_VEHICLE ) {
                DrawVehicle( entity, viewProjection );
            } else {
                // Generic draw
                glm::mat4 model = glm::translate( glm::mat4( 1.0f ), entity.position ) * glm::mat4_cast( entity.rotation );
                entity.renderModel.shader->SetMat4( "uViewProjection", viewProjection );
                entity.renderModel.shader->SetMat4( "uModel", model );
                entity.renderModel.mesh->Draw();
            }

            if ( entity.action.targetId != INVALID_ENTITY_ID ) {
                if ( const Entity * target = world.GetEntity( entity.action.targetId ) ) {
                    DebugRenderer::Get().DrawLine( entity.position, target->position, { 1.0f, 0.1f, 0.1f } );
                }
            }
        }

        DebugRenderer::Get().Render( viewProjection );
    }

    void DustGame::DrawVehicle( const Entity & entity, const glm::mat4 & viewProjection ) {
        if ( !buggyWheelMesh ) {
            return;
        }

        glm::mat4 chassisModel = glm::translate( glm::mat4( 1.0f ), entity.position ) * glm::mat4_cast( entity.rotation );

        entity.renderModel.shader->SetMat4( "uViewProjection", viewProjection );
        entity.renderModel.shader->SetMat4( "uModel", chassisModel );
        entity.renderModel.mesh->Draw();

        const VehicleData & vehicle = entity.vehicle;
        for ( i32 i = 0; i < 4; ++i ) {
            bool isFrontWheel = i < 2;
            f32 steerRadians = isFrontWheel ? glm::radians( vehicle.steerAngleDegrees ) : 0.0f;

            // Order (applied right-to-left): align the cylinder's default
            // Y-axis to the wheel's roll axis (X), spin it around that axis,
            // steer front wheels about the chassis' up axis, then place it.
            glm::mat4 wheelLocal = glm::translate( glm::mat4( 1.0f ), vehicle.wheelOffsets[i] ) * glm::rotate( glm::mat4( 1.0f ), steerRadians, glm::vec3( 0.0f, 1.0f, 0.0f ) ) * glm::rotate( glm::mat4( 1.0f ), vehicle.wheelSpinRadians, glm::vec3( 1.0f, 0.0f, 0.0f ) ) * glm::rotate( glm::mat4( 1.0f ), glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );

            entity.renderModel.shader->SetMat4( "uModel", chassisModel * wheelLocal );
            buggyWheelMesh->Draw();
        }
    }

} // namespace dust

