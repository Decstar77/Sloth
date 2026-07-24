#include <core/sloth_engine.h>
#include <font/sloth_font.h>
#include <gui/sloth_gui_context.h>
#include <gui/sloth_gui_frame.h>
#include <gui/sloth_gui_widgets.h>
#include <renderer/sloth_glyph_cache.h>
#include <renderer/sloth_gui_renderer.h>
#include <renderer/sloth_text_renderer.h>
#include <renderer/sloth_texture.h>

#include <dust_game.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace sloth;

int main() {
    WindowProps props;
    props.Title = "Sloth Engine";
    props.Width = 1280;
    props.Height = 720;

    Engine & engine = Engine::Get();
    engine.Init( props );

    Window & window = engine.GetWindow();

    Font font( &engine.GetPermanentArena() );
    font.Load( "../../assets/fonts/roboto/Roboto-Regular.ttf" );
    GlyphCache glyphCache;
    TextRenderer textRenderer;
    GuiRenderer guiRenderer;

    // Small in-memory checkerboard, exercising Texture's raw-pixel
    // constructor (no image file needed) - a stand-in for a real icon.
    constexpr i32 checkerSize = 16;
    u8 checkerPixels[checkerSize * checkerSize * 4];
    for ( i32 y = 0; y < checkerSize; ++y ) {
        for ( i32 x = 0; x < checkerSize; ++x ) {
            bool isLight = ( ( x / 4 ) + ( y / 4 ) ) % 2 == 0;
            u8 * pixel = &checkerPixels[( y * checkerSize + x ) * 4];
            pixel[0] = isLight ? 255 : 40;
            pixel[1] = isLight ? 210 : 40;
            pixel[2] = isLight ? 90 : 60;
            pixel[3] = 255;
        }
    }
    Texture checkerTexture( checkerPixels, checkerSize, checkerSize, TextureFilter::Nearest );

    GuiContext guiContext;
    bool demoCheckboxValue = false;

    dust::DustGame game;
    game.Init();

    f64 lastFrameTime = glfwGetTime();

    while ( !window.ShouldClose() ) {
        if ( engine.GetInput().IsKeyPressed( Key::Escape ) == true ) {
            window.SetShouldClose( true );
        }

        f64 currentFrameTime = glfwGetTime();
        f32 deltaTime = static_cast<f32>( currentFrameTime - lastFrameTime );
        lastFrameTime = currentFrameTime;

        glm::mat4 screenProjection = MakeScreenProjection( static_cast<f32>( window.GetWidth() ), static_cast<f32>( window.GetHeight() ) );
        GuiFrame guiFrame = BeginGuiFrame( guiContext, guiRenderer, textRenderer, font, glyphCache, engine.GetInput(), screenProjection );

        game.UpdateAndRender( deltaTime, guiFrame );

        //guiRenderer.DrawRect( { 32.0f, 96.0f }, { 320.0f, 280.0f }, { 0.15f, 0.15f, 0.18f, 0.9f }, 12.0f );
        //guiRenderer.DrawRect( { 48.0f, 112.0f }, { 140.0f, 144.0f }, { 0.2f, 0.55f, 0.9f, 1.0f }, 6.0f );
        //guiRenderer.DrawRect( { 48.0f, 160.0f }, { 140.0f, 192.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, 6.0f, 2.0f, { 0.9f, 0.9f, 0.9f, 1.0f } );
        //guiRenderer.DrawCircle( { 190.0f, 176.0f }, 32.0f, { 0.9f, 0.35f, 0.2f, 1.0f } );
        //guiRenderer.DrawCircle( { 270.0f, 176.0f }, 32.0f, { 0.15f, 0.15f, 0.18f, 1.0f }, 3.0f, { 0.4f, 0.9f, 0.5f, 1.0f } );
        //guiRenderer.DrawImage( { 48.0f, 208.0f }, { 112.0f, 272.0f }, checkerTexture );
        //guiRenderer.DrawImage( { 128.0f, 208.0f }, { 192.0f, 272.0f }, checkerTexture, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, 32.0f );

        // Widget wrapper demo - Button/Checkbox/Label built on top of
        // GuiContext's hot/active/click plumbing, replacing the hand-rolled
        // version that used to live here.
        //{
        //    glm::vec2 buttonMin { 32.0f, 320.0f };
        //    glm::vec2 buttonMax { 232.0f, 368.0f };
        //    i32 & clickCount = guiContext.GetStorage().GetOrAddInt( guiContext.GetId( "DemoButton" ) );
        //    if ( Button( guiFrame, "DemoButton", buttonMin, buttonMax ) ) {
        //        ++clickCount;
        //    }

        //    LargeString buttonLabel;
        //    buttonLabel.Format( "Clicked: %d", clickCount );
        //    Label( guiFrame, buttonLabel.View(), { buttonMin.x, buttonMax.y + 24.0f }, 16.0f, { 1.0f, 1.0f, 1.0f, 1.0f } );

        //    Checkbox( guiFrame, "Demo checkbox", { 32.0f, 400.0f }, demoCheckboxValue );
        //}

        // Faction entity counts, top-left corner.
        if ( font.IsLoaded() ) {
            constexpr dust::FactionType factionTypes[] = {
                dust::FACTION_TYPE_NEUTRAL,
                dust::FACTION_TYPE_REMNANT,
                dust::FACTION_TYPE_RUSTBORN,
                dust::FACTION_TYPE_ZENITH,
                dust::FACTION_TYPE_PLAYER
            };

            i32 factionCounts[SL_ARRAY_COUNT( factionTypes )] = {};
            for ( const dust::Entity & entity : game.GetWorld().GetEntities() ) {
                if ( entity.type == dust::ENTITY_TYPE_INVALID ) {
                    continue;
                }
                factionCounts[entity.faction]++;
            }

            LargeString factionLabel;
            f32 factionLabelY = 32.0f;
            for ( dust::FactionType factionType : factionTypes ) {
                factionLabel.Format( "%s: %d", dust::ToString( factionType ), factionCounts[factionType] );
                glm::vec2 factionLabelPos { 16.0f, factionLabelY };
                textRenderer.DrawText( font, glyphCache, factionLabel.View(), factionLabelPos, 22.0f, { 1.0f, 0.9f, 0.4f, 1.0f }, screenProjection );
                factionLabelY += 26.0f;
            }
        }

        // World-space nameplates above each ore node, identifying its resource.
        if ( font.IsLoaded() ) {
            const sloth::Camera & camera = game.GetCamera().GetCamera();
            constexpr f32 labelHeightAboveNode = 3.0f;

            for ( const dust::Entity & entity : game.GetWorld().GetEntities() ) {
                if ( entity.type != dust::ENTITY_TYPE_ORE_NODE ) {
                    continue;
                }

                dust::InventoryItemType itemType = dust::OreNodeTypeToItemType( entity.oreNode.type );
                LargeString label;
                label.Assign( dust::ToString( itemType ) );

                glm::vec3 worldPos = entity.position + glm::vec3( 0.0f, labelHeightAboveNode, 0.0f );
                textRenderer.DrawTextWorld( font, glyphCache, label.View(), worldPos, camera,
                    static_cast<f32>( window.GetWidth() ), static_cast<f32>( window.GetHeight() ), 20.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, screenProjection );
            }
        }

        // Player's current raycast target, top-right corner.
        if ( font.IsLoaded() ) {
            const dust::Entity * target = game.GetPlayerTarget();

            LargeString targetLabel;
            if ( target ) {
                targetLabel.Format( "Target: %s", dust::ToString( target->type ) );
            } else {
                targetLabel.Assign( "Target: none" );
            }

            constexpr f32 rightMargin = 300.0f;
            glm::vec2 targetLabelPos { static_cast<f32>( window.GetWidth() ) - rightMargin, 32.0f };
            textRenderer.DrawText( font, glyphCache, targetLabel.View(), targetLabelPos, 22.0f, { 1.0f, 0.9f, 0.4f, 1.0f }, screenProjection );


            const dust::Entity * player = game.GetPlayer();

            constexpr dust::InventoryItemType oreItemTypes[] = {
                dust::INVENTORY_ITEM_TYPE_ORE_IRON,
                dust::INVENTORY_ITEM_TYPE_ORE_COPPER,
                dust::INVENTORY_ITEM_TYPE_ORE_SULPHUR,
                dust::INVENTORY_ITEM_TYPE_ORE_ALUMINUM,
                dust::INVENTORY_ITEM_TYPE_ORE_CRUDE_OIL,
                dust::INVENTORY_ITEM_TYPE_ORE_WATER,
                dust::INVENTORY_ITEM_TYPE_ORE_SILICON,
            };

            f32 oreLabelY = 64.0f;
            for ( dust::InventoryItemType oreItemType : oreItemTypes ) {
                const dust::InventoryItem * item = dust::InvetoryFindItem( player->inventory, oreItemType );
                if ( item != nullptr ) {
                    targetLabel.Format( "%s %d", dust::ToString( oreItemType ), item->amount );
                    targetLabelPos = { static_cast<f32>( window.GetWidth() ) - rightMargin, oreLabelY };
                    textRenderer.DrawText( font, glyphCache, targetLabel.View(), targetLabelPos, 22.0f, { 1.0f, 0.9f, 0.4f, 1.0f }, screenProjection );
                    oreLabelY += 26.0f;
                }
            }

            targetLabel.Format( "Credits %d", game.GetPlayerCredits() );
            targetLabelPos = { static_cast<f32>( window.GetWidth() ) - rightMargin / 2, 32.0f };
            textRenderer.DrawText( font, glyphCache, targetLabel.View(), targetLabelPos, 22.0f, { 1.0f, 0.9f, 0.4f, 1.0f }, screenProjection );
        }

        // Clip-rect / scissor demo: a small "viewport" panel showing a list
        // of rows that overflow both above and below it. Rows outside the
        // viewport should be cut off cleanly at its edge instead of
        // spilling onto the rest of the screen, and the mouse shouldn't be
        // able to hover a row's clipped-off portion.
        //{
        //    glm::vec2 viewportMin { 360.0f, 96.0f };
        //    glm::vec2 viewportMax { 560.0f, 280.0f };

        //    guiRenderer.DrawRect( viewportMin, viewportMax, { 0.1f, 0.1f, 0.13f, 1.0f }, 8.0f );

        //    guiContext.PushClipRect( viewportMin, viewportMax );
        //    guiRenderer.PushClipRect( viewportMin, viewportMax );

        //    constexpr f32 rowHeight = 36.0f;
        //    constexpr f32 scrollOffset = 40.0f; // Pretend the list has been scrolled down a bit.
        //    for ( i32 row = 0; row < 8; ++row ) {
        //        f32 y = viewportMin.y + 8.0f + static_cast<f32>( row ) * rowHeight - scrollOffset;
        //        glm::vec2 rowMin { viewportMin.x + 8.0f, y };
        //        glm::vec2 rowMax { viewportMax.x - 8.0f, y + rowHeight - 6.0f };

        //        bool hovered = GuiContext::IsPointInRect( guiContext.GetMousePos(), rowMin, rowMax ) && guiContext.IsPointVisible( guiContext.GetMousePos() );
        //        glm::vec4 rowColor = hovered ? glm::vec4 { 0.35f, 0.35f, 0.42f, 1.0f } : glm::vec4 { 0.22f, 0.22f, 0.28f, 1.0f };
        //        guiRenderer.DrawRect( rowMin, rowMax, rowColor, 4.0f );
        //    }

        //    guiRenderer.PopClipRect();
        //    guiContext.PopClipRect();

        //    guiRenderer.Flush( screenProjection );
        //}

        EndGuiFrame( guiFrame );
        engine.EndFrame();
        window.OnUpdate();
    }

    game.Shutdown();
    engine.Shutdown();

    return 0;
}
