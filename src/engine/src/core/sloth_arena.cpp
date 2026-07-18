#include "sloth_arena.h"

#include <cstring>
#include <new>

namespace sloth {
    Arena::~Arena() {
        Shutdown();
    }

    void Arena::Init( usize size ) {
        SL_ASSERT_MSG( base == nullptr, "Arena has already been initialized" );

        base = static_cast<u8*>( ::operator new( size, std::align_val_t { alignof( std::max_align_t ) } ) );
        capacity = size;
        offset = 0;
    }

    void Arena::Shutdown() {
        if ( base != nullptr ) {
            ::operator delete( base, std::align_val_t { alignof( std::max_align_t ) } );
            base = nullptr;
        }

        capacity = 0;
        offset = 0;
    }

    void* Arena::Push( usize size, usize alignment ) {
        SL_ASSERT_MSG( base != nullptr, "Arena has not been initialized" );

        usize current = reinterpret_cast<usize>( base ) + offset;
        usize aligned = ( current + ( alignment - 1 ) ) & ~( alignment - 1 );
        usize alignedOffset = aligned - reinterpret_cast<usize>( base );
        usize newOffset = alignedOffset + size;

        SL_ASSERT_MSG( newOffset <= capacity, "Arena out of memory (capacity=%zu, requested=%zu)", capacity, newOffset );
        if ( newOffset > capacity ) {
            return nullptr;
        }

        offset = newOffset;
        return base + alignedOffset;
    }

    void* Arena::PushZero( usize size, usize alignment ) {
        void* ptr = Push( size, alignment );
        if ( ptr != nullptr ) {
            std::memset( ptr, 0, size );
        }

        return ptr;
    }

    void Arena::Reset() {
        offset = 0;
    }

} // namespace sloth
