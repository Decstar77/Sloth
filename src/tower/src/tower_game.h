#pragma once

#include <audio/sloth_audio_world.h>
#include <core/sloth_defines.h>
#include <core/sloth_string.h>
#include <font/sloth_font.h>
#include <network/sloth_network.h>
#include <physics/sloth_physics_world.h>
#include <renderer/sloth_glyph_cache.h>
#include <renderer/sloth_gui_renderer.h>
#include <renderer/sloth_shader.h>
#include <renderer/sloth_static_mesh.h>
#include <renderer/sloth_text_renderer.h>

#include "tower_building.h"
#include "tower_camera.h"
#include "tower_protocol.h"
#include "tower_world.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace tower {

    class TowerGame {
    public:
        void                    Init();
        void                    Shutdown();

        void                    Update( f32 deltaTime );
        void                    Render();

        TowerCamera &           GetCamera() { return camera; }
        sloth::PhysicsWorld &   GetPhysicsWorld() { return physicsWorld; }
        TowerWorld &            GetWorld() { return world; }

        sloth::NetConnectionState GetServerConnectionState() const { return network.GetConnectionState( serverConnection ); }

    private:
        void                    UpdatePlayerMovement( f32 deltaTime );
        void                    UpdateShooting( f32 deltaTime );
        void                    UpdateNetworking();
        void                    ApplyWorldSnapshot( const u8 * data, usize size );
        void                    HandlePlayerHealth( const PlayerHealthMessage & message );
        void                    HandlePlayerRespawn( const PlayerRespawnMessage & message );
        void                    UpdateFootsteps( f32 deltaTime, bool grounded, const glm::vec3 & desiredMovement );
        void                    UpdateBuilding();
        void                    RenderPlayerHands();
        void                    RenderHud();

    private:
        TowerWorld                          world;
        TowerCamera                         camera;
        EntityId                            playerId;

        // Rendering stuff, need a better place to put all this
        std::unique_ptr<sloth::Shader>      shader;
        std::unique_ptr<sloth::StaticMesh>  floorMesh;
        std::unique_ptr<sloth::StaticMesh>  boxMesh;
        std::unique_ptr<sloth::StaticMesh>  capsuleMesh; // No capsule primitive in Geometry yet - a cylinder stands in for other players' capsules.
        std::unique_ptr<sloth::StaticMesh>  handMesh;    // Small placeholder box drawn in front of every player (including the local one) so facing/aim is visible.
        std::unique_ptr<sloth::StaticMesh>  buildingFloorMesh;
        std::unique_ptr<sloth::StaticMesh>  buildingWallMesh;

        // HUD - crosshair (GuiRenderer, no font needed) and health text
        // (TextRenderer/Font/GlyphCache). No GuiContext/GuiFrame here since
        // there are no widgets (buttons/panels) yet, just static drawing.
        std::unique_ptr<sloth::Font>        font;
        sloth::GlyphCache                   glyphCache;
        sloth::TextRenderer                 textRenderer;
        sloth::GuiRenderer                  guiRenderer;

        sloth::PhysicsWorld                 physicsWorld;
        sloth::AudioWorld                   audioWorld;
        f32                                  footstepDistance = 0.0f; // Meters walked since the last footstep - see UpdateFootsteps.

        sloth::NetworkSystem                network;
        sloth::NetConnection                serverConnection;

        // See tower_protocol.h. Movement is client-authoritative: each
        // client reports its own transform every frame and mirrors everyone
        // else's via ENTITY_TYPE_REMOTE_PLAYER entities driven straight from
        // the server's snapshots (no physics/prediction on remote players).
        u32                                  localPlayerId = 0;
        bool                                 hasLocalPlayerId = false;
        std::unordered_map<u32, EntityId>    remotePlayers;

        // Combat: hitscan is resolved server-side (see TowerServer::
        // HandlePlayerShot) - firing just reports a ray and waits for a
        // PlayerHealth/PlayerRespawn reply. recoilTimer only drives the
        // local hands-box kickback, it's not gameplay state.
        f32                                  localHealth = 100.0f;
        f32                                  recoilTimer = 0.0f;
    };

}
