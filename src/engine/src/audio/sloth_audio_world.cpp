#include "sloth_audio_world.h"

#include <miniaudio.h>

namespace sloth {

    // ------------------------------------------------------------------------
    // AudioWorld::Impl
    // ------------------------------------------------------------------------

    struct AudioWorld::Impl {
        // Heap-allocated so ma_sound's internal self-references (it embeds
        // node-graph nodes that point back into itself once initialized)
        // stay valid even as the pool vector grows.
        struct Slot {
            ma_sound sound {};
            bool active = false;
            bool looping = false;
            u32 generation = 0;
        };

        ma_engine engine {};
        std::vector<std::unique_ptr<Slot>> pool;
        std::vector<u32> freeList;

        Slot * TryGetSlot( SoundHandle handle ) {
            if ( !handle.IsValid() || handle.index >= pool.size() ) {
                return nullptr;
            }

            Slot * slot = pool[handle.index].get();
            if ( !slot->active || slot->generation != handle.generation ) {
                return nullptr;
            }

            return slot;
        }

        SoundHandle AcquireSlot( u32 & outIndex ) {
            u32 index;
            if ( !freeList.empty() ) {
                index = freeList.back();
                freeList.pop_back();
            } else {
                pool.push_back( std::make_unique<Slot>() );
                index = static_cast<u32>( pool.size() - 1 );
            }

            outIndex = index;
            Slot * slot = pool[index].get();
            slot->active = true;
            slot->looping = false;
            return SoundHandle { index, slot->generation };
        }

        void ReleaseSlot( u32 index ) {
            Slot * slot = pool[index].get();
            ma_sound_uninit( &slot->sound );
            slot->active = false;
            slot->generation++;
            freeList.push_back( index );
        }

        SoundHandle PlaySoundInternal( const char * filePath, const SoundDesc & desc, bool spatialized, const glm::vec3 & position ) {
            u32 index;
            SoundHandle handle = AcquireSlot( index );
            Slot * slot = pool[index].get();

            ma_result result = ma_sound_init_from_file( &engine, filePath, 0, nullptr, nullptr, &slot->sound );
            if ( result != MA_SUCCESS ) {
                SL_LOG_ERROR( "AudioWorld: failed to load sound '%s' (%d)", filePath, static_cast<int>( result ) );
                slot->active = false;
                slot->generation++;
                freeList.push_back( index );
                return SoundHandle {};
            }

            slot->looping = desc.loop;

            ma_sound_set_looping( &slot->sound, desc.loop ? MA_TRUE : MA_FALSE );
            ma_sound_set_volume( &slot->sound, desc.volume );
            ma_sound_set_pitch( &slot->sound, desc.pitch );
            ma_sound_set_spatialization_enabled( &slot->sound, spatialized ? MA_TRUE : MA_FALSE );
            if ( spatialized ) {
                ma_sound_set_position( &slot->sound, position.x, position.y, position.z );
            }

            ma_sound_start( &slot->sound );

            return handle;
        }
    };

    // ------------------------------------------------------------------------
    // AudioWorld
    // ------------------------------------------------------------------------

    AudioWorld::AudioWorld()
        : impl( std::make_unique<Impl>() ) {
        ma_result result = ma_engine_init( nullptr, &impl->engine );
        SL_ASSERT_MSG( result == MA_SUCCESS, "AudioWorld: ma_engine_init failed (%d)", static_cast<int>( result ) );
    }

    AudioWorld::~AudioWorld() {
        for ( usize i = 0; i < impl->pool.size(); ++i ) {
            if ( impl->pool[i]->active ) {
                ma_sound_uninit( &impl->pool[i]->sound );
            }
        }
        impl->pool.clear();

        ma_engine_uninit( &impl->engine );
    }

    void AudioWorld::Update() {
        for ( u32 i = 0; i < static_cast<u32>( impl->pool.size() ); ++i ) {
            Impl::Slot * slot = impl->pool[i].get();
            if ( slot->active && !slot->looping && ma_sound_at_end( &slot->sound ) ) {
                impl->ReleaseSlot( i );
            }
        }
    }

    SoundHandle AudioWorld::PlaySound2D( const char * filePath, const SoundDesc & desc ) {
        return impl->PlaySoundInternal( filePath, desc, false, glm::vec3( 0.0f ) );
    }

    SoundHandle AudioWorld::PlaySound3D( const char * filePath, const glm::vec3 & position, const SoundDesc & desc ) {
        return impl->PlaySoundInternal( filePath, desc, true, position );
    }

    void AudioWorld::Stop( SoundHandle handle ) {
        Impl::Slot * slot = impl->TryGetSlot( handle );
        if ( slot == nullptr ) {
            return;
        }

        impl->ReleaseSlot( handle.index );
    }

    bool AudioWorld::IsPlaying( SoundHandle handle ) const {
        Impl::Slot * slot = impl->TryGetSlot( handle );
        return slot != nullptr && ma_sound_is_playing( &slot->sound ) == MA_TRUE;
    }

    void AudioWorld::SetPosition( SoundHandle handle, const glm::vec3 & position ) {
        Impl::Slot * slot = impl->TryGetSlot( handle );
        if ( slot == nullptr ) {
            return;
        }

        ma_sound_set_position( &slot->sound, position.x, position.y, position.z );
    }

    void AudioWorld::SetVolume( SoundHandle handle, f32 volume ) {
        Impl::Slot * slot = impl->TryGetSlot( handle );
        if ( slot == nullptr ) {
            return;
        }

        ma_sound_set_volume( &slot->sound, volume );
    }

    void AudioWorld::SetPitch( SoundHandle handle, f32 pitch ) {
        Impl::Slot * slot = impl->TryGetSlot( handle );
        if ( slot == nullptr ) {
            return;
        }

        ma_sound_set_pitch( &slot->sound, pitch );
    }

    void AudioWorld::SetListenerTransform( const glm::vec3 & position, const glm::vec3 & forward, const glm::vec3 & up ) {
        ma_engine_listener_set_position( &impl->engine, 0, position.x, position.y, position.z );
        ma_engine_listener_set_direction( &impl->engine, 0, forward.x, forward.y, forward.z );
        ma_engine_listener_set_world_up( &impl->engine, 0, up.x, up.y, up.z );
    }

    void AudioWorld::SetMasterVolume( f32 volume ) {
        ma_engine_set_volume( &impl->engine, volume );
    }

} // namespace sloth
