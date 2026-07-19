#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace sloth {

    struct SoundHandle {
        static constexpr u32 InvalidIndex = 0xFFFFFFFF;

        u32 index = InvalidIndex;
        u32 generation = 0;

        bool IsValid() const { return index != InvalidIndex; }
    };

    struct SoundDesc {
        f32 volume = 1.0f;
        f32 pitch = 1.0f;
        bool loop = false;
    };

    // Owns a miniaudio engine (device + mixing graph + resource manager).
    // All miniaudio types are hidden behind Impl (like Window hides
    // GLFWwindow, PhysicsWorld hides Jolt) so nothing outside this .cpp
    // needs miniaudio's header.
    //
    // Sounds are decoded/streamed via miniaudio's built-in resource
    // manager, which transparently shares decoded data across sounds
    // loaded from the same file path - callers don't need to manage a
    // separate sound bank.
    //
    // Threading: mixing happens on miniaudio's own audio thread; every
    // method on this class is main-thread-only, matching the rest of the
    // engine's single-threaded-per-frame model.
    class AudioWorld {
      public:
        AudioWorld();
        ~AudioWorld();

        SL_NON_COPYABLE( AudioWorld );
        SL_NON_MOVABLE( AudioWorld );

        // Reclaims pool slots belonging to one-shot sounds that have
        // finished playing. Call once per frame. Looping sounds are only
        // ever reclaimed by an explicit Stop().
        void Update();

        SoundHandle PlaySound2D( const char * filePath, const SoundDesc & desc = SoundDesc() );
        SoundHandle PlaySound3D( const char * filePath, const glm::vec3 & position, const SoundDesc & desc = SoundDesc() );
        
        bool IsPlaying( SoundHandle handle ) const;
        void SetPosition( SoundHandle handle, const glm::vec3 & position );
        void SetVolume( SoundHandle handle, f32 volume );
        void SetPitch( SoundHandle handle, f32 pitch );
        void Stop( SoundHandle handle );

        void SetListenerTransform( const glm::vec3 & position, const glm::vec3 & forward, const glm::vec3 & up );

        void SetMasterVolume( f32 volume );

      private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };

} // namespace sloth
