#pragma once

#include "core/sloth_defines.h"

namespace sloth {

    // Simple bump (linear) memory arena. Memory is reserved once up front and
    // handed out via Push() calls that just advance an offset; individual
    // allocations are never freed - the whole arena is reset or torn down at
    // once. No destructors are run for pushed objects.
    class Arena {
      public:
        Arena() = default;
        ~Arena();

        SL_NON_COPYABLE( Arena );
        SL_NON_MOVABLE( Arena );

        void    Init( usize size );
        void    Shutdown();

        void *  Push( usize size, usize alignment = alignof( std::max_align_t ) );
        void *  PushZero( usize size, usize alignment = alignof( std::max_align_t ) );

        template <typename T>
        T *     PushArray( usize count ) { return static_cast<T*>( Push( sizeof( T ) * count, alignof( T ) ) );  }

        template <typename T>
        T *     PushArrayZero( usize count ) { return static_cast<T*>( PushZero( sizeof( T ) * count, alignof( T ) ) ); }

        template <typename T>
        T *     PushStruct() { return PushArray<T>( 1 ); }

        void    Reset();

        usize   GetUsed() const { return offset; }
        usize   GetCapacity() const { return capacity; }

      private:
        u8 *    base = nullptr;
        usize   capacity = 0;
        usize   offset = 0;
    };

} // namespace sloth
