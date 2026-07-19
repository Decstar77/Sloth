#include <core/sloth_engine.h>
#include <font/sloth_font.h>
#include <gui/sloth_gui_context.h>
#include <gui/sloth_gui_widgets.h>
#include <audio/sloth_audio_world.h>
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

    AudioWorld audioWorld;

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
    bool inventoryOpen = false;

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

        audioWorld.Update();

        guiContext.NewFrame( engine.GetInput() );

        game.Update( deltaTime );
        game.Render();

        if ( engine.GetInput().IsKeyPressed( Key::T ) ) {
            audioWorld.PlaySound2D( "../../assets/sounds/button_click.wav" );
        }

        if ( engine.GetInput().IsKeyPressed( Key::I ) ) {
            inventoryOpen = !inventoryOpen;
        }

        glm::mat4 screenProjection = MakeScreenProjection( static_cast<f32>( window.GetWidth() ), static_cast<f32>( window.GetHeight() ) );

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
        //    if ( Button( guiContext, guiRenderer, textRenderer, font, glyphCache, "DemoButton", buttonMin, buttonMax, screenProjection ) ) {
        //        ++clickCount;
        //    }

        //    LargeString buttonLabel;
        //    buttonLabel.Format( "Clicked: %d", clickCount );
        //    Label( textRenderer, font, glyphCache, buttonLabel.View(), { buttonMin.x, buttonMax.y + 24.0f }, 16.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, screenProjection );

        //    Checkbox( guiContext, guiRenderer, textRenderer, font, glyphCache, "Demo checkbox", { 32.0f, 400.0f }, demoCheckboxValue, screenProjection );
        //}

        if ( font.IsLoaded() ) {
            textRenderer.DrawText( font, glyphCache, "Hello, Sloth! i am engine clicked", { 32.0f, 64.0f }, 28.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, screenProjection );
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
                dust::INVENTORY_ITEM_TYPE_ORE_COAL,
                dust::INVENTORY_ITEM_TYPE_ORE_SULPHUR,
                dust::INVENTORY_ITEM_TYPE_ORE_ALUMINUM,
                dust::INVENTORY_ITEM_TYPE_ORE_CHROME,
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

        // Inventory grid panel, toggled with 'I'. One button per slot,
        // laid out from Inventory::xSize/ySize; slots beyond the current
        // item count are drawn empty.
        if ( inventoryOpen && font.IsLoaded() ) {
            const dust::Entity * player = game.GetPlayer();
            if ( player != nullptr ) {
                const dust::Inventory & inventory = player->inventory;

                i32 gridCols = inventory.xSize > 0 ? inventory.xSize : 1;
                i32 gridRows = inventory.ySize > 0 ? inventory.ySize : 1;

                constexpr f32 slotSize = 64.0f;
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
                glm::vec2 defaultPos {
                    ( static_cast<f32>( window.GetWidth() ) - panelSize.x ) * 0.5f,
                    ( static_cast<f32>( window.GetHeight() ) - panelSize.y ) * 0.5f,
                };

                PanelResult panel = BeginPanel( guiContext, guiRenderer, textRenderer, font, glyphCache, "Inventory##InvPanel", defaultPos, panelSize, screenProjection );

                glm::vec2 gridOrigin = panel.contentMin;

                i32 slotCount = gridCols * gridRows;
                i32 itemCount = static_cast<i32>( inventory.items.GetCount() );
                for ( i32 slot = 0; slot < slotCount; ++slot ) {
                    i32 col = slot % gridCols;
                    i32 row = slot / gridCols;

                    glm::vec2 slotMin = gridOrigin + glm::vec2( static_cast<f32>( col ) * ( slotSize + slotGap ), static_cast<f32>( row ) * ( slotSize + slotGap ) );
                    glm::vec2 slotMax = slotMin + glm::vec2( slotSize, slotSize );

                    LargeString slotLabel;
                    if ( slot < itemCount ) {
                        const dust::InventoryItem & item = inventory.items[slot];
                        slotLabel.Format( "%s (%d)##InvSlot%d", dust::ToShortCode( item.type ), item.amount, slot );
                    } else {
                        slotLabel.Format( "##InvSlot%d", slot );
                    }

                    Button( guiContext, guiRenderer, textRenderer, font, glyphCache, slotLabel.View(), slotMin, slotMax, screenProjection );
                }

                EndPanel( guiContext, guiRenderer, screenProjection );
            }
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

        guiContext.EndFrame();
        engine.EndFrame();
        window.OnUpdate();
    }

    game.Shutdown();
    engine.Shutdown();

    return 0;
}
