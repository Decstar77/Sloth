#pragma once

#include "core/sloth_defines.h"

#include <new>
#include <utility>

namespace sloth {

    // Fixed-capacity, inline-storage list (no heap allocation). `Size` is the
    // maximum number of elements; `Count` tracks how many are currently live.
    // Element constructors/destructors are respected via placement-new /
    // explicit destructor calls, so T does not need to be trivial. Bounds are
    // checked with SL_ASSERT_MSG (stripped in Dist builds, like everywhere
    // else in the engine).
    template <typename T, u32 Size>
    class FixedList {
        static_assert( Size > 0, "FixedList size must be greater than zero" );

      public:
        using Iterator = T *;
        using ConstIterator = const T *;

        FixedList() = default;

        FixedList( const FixedList & other ) { CopyFrom( other ); }

        FixedList & operator=( const FixedList & other ) {
            if ( this != &other ) {
                Clear();
                CopyFrom( other );
            }
            return *this;
        }

        FixedList( FixedList && other ) noexcept { MoveFrom( other ); }

        FixedList & operator=( FixedList && other ) noexcept {
            if ( this != &other ) {
                Clear();
                MoveFrom( other );
            }
            return *this;
        }

        ~FixedList() { Clear(); }

        // --- Capacity -----------------------------------------------------

        u32  GetCount() const { return count; }
        u32  GetCapacity() const { return Size; }
        bool IsEmpty() const { return count == 0; }
        bool IsFull() const { return count == Size; }

        // --- Element access -------------------------------------------------

        T & operator[]( u32 index ) {
            SL_ASSERT_MSG( index < count, "FixedList index out of range (%u >= %u)", index, count );
            return Data()[index];
        }

        const T & operator[]( u32 index ) const {
            SL_ASSERT_MSG( index < count, "FixedList index out of range (%u >= %u)", index, count );
            return Data()[index];
        }

        T & Front() {
            SL_ASSERT_MSG( count > 0, "FixedList::Front() called on empty list" );
            return Data()[0];
        }

        const T & Front() const {
            SL_ASSERT_MSG( count > 0, "FixedList::Front() called on empty list" );
            return Data()[0];
        }

        T & Back() {
            SL_ASSERT_MSG( count > 0, "FixedList::Back() called on empty list" );
            return Data()[count - 1];
        }

        const T & Back() const {
            SL_ASSERT_MSG( count > 0, "FixedList::Back() called on empty list" );
            return Data()[count - 1];
        }

        T *       GetData() { return Data(); }
        const T * GetData() const { return Data(); }

        // --- Mutation -------------------------------------------------------

        T & Add( const T & value ) {
            SL_ASSERT_MSG( count < Size, "FixedList<%u> overflow on Add", Size );
            T * slot = &Data()[count];
            new ( slot ) T( value );
            ++count;
            return *slot;
        }

        T & Add( T && value ) {
            SL_ASSERT_MSG( count < Size, "FixedList<%u> overflow on Add", Size );
            T * slot = &Data()[count];
            new ( slot ) T( std::move( value ) );
            ++count;
            return *slot;
        }

        template <typename... Args>
        T & Emplace( Args &&... args ) {
            SL_ASSERT_MSG( count < Size, "FixedList<%u> overflow on Emplace", Size );
            T * slot = &Data()[count];
            new ( slot ) T( std::forward<Args>( args )... );
            ++count;
            return *slot;
        }

        // Removes the element at `index`, shifting subsequent elements down
        // to preserve order. O(n).
        void RemoveAt( u32 index ) {
            SL_ASSERT_MSG( index < count, "FixedList::RemoveAt index out of range (%u >= %u)", index, count );
            T * data = Data();
            for ( u32 i = index; i + 1 < count; ++i ) {
                data[i] = std::move( data[i + 1] );
            }
            data[count - 1].~T();
            --count;
        }

        // Removes the element at `index` by moving the last element into its
        // slot. O(1), does not preserve order.
        void RemoveAtSwap( u32 index ) {
            SL_ASSERT_MSG( index < count, "FixedList::RemoveAtSwap index out of range (%u >= %u)", index, count );
            T * data = Data();
            if ( index != count - 1 ) {
                data[index] = std::move( data[count - 1] );
            }
            data[count - 1].~T();
            --count;
        }

        // Removes the first element equal to `value` (order-preserving).
        // Returns true if an element was found and removed.
        bool Remove( const T & value ) {
            i64 index = IndexOf( value );
            if ( index < 0 ) {
                return false;
            }
            RemoveAt( static_cast<u32>( index ) );
            return true;
        }

        // Same as Remove(), but uses RemoveAtSwap() internally (does not
        // preserve order).
        bool RemoveSwap( const T & value ) {
            i64 index = IndexOf( value );
            if ( index < 0 ) {
                return false;
            }
            RemoveAtSwap( static_cast<u32>( index ) );
            return true;
        }

        void PopBack() {
            SL_ASSERT_MSG( count > 0, "FixedList::PopBack called on empty list" );
            Data()[count - 1].~T();
            --count;
        }

        void Clear() {
            T * data = Data();
            for ( u32 i = 0; i < count; ++i ) {
                data[i].~T();
            }
            count = 0;
        }

        // Resizes to `newCount`. Growing default-constructs new elements;
        // shrinking destroys the truncated ones.
        void Resize( u32 newCount ) {
            SL_ASSERT_MSG( newCount <= Size, "FixedList<%u> overflow on Resize (requested=%u)", Size, newCount );
            T * data = Data();
            if ( newCount > count ) {
                for ( u32 i = count; i < newCount; ++i ) {
                    new ( &data[i] ) T();
                }
            } else {
                for ( u32 i = newCount; i < count; ++i ) {
                    data[i].~T();
                }
            }
            count = newCount;
        }

        // --- Search ---------------------------------------------------------

        i64 IndexOf( const T & value ) const {
            const T * data = Data();
            for ( u32 i = 0; i < count; ++i ) {
                if ( data[i] == value ) {
                    return static_cast<i64>( i );
                }
            }
            return -1;
        }

        bool Contains( const T & value ) const { return IndexOf( value ) >= 0; }

        T * Find( const T & value ) {
            i64 index = IndexOf( value );
            return index >= 0 ? &Data()[index] : nullptr;
        }

        const T * Find( const T & value ) const {
            i64 index = IndexOf( value );
            return index >= 0 ? &Data()[index] : nullptr;
        }

        // --- Iteration --------------------------------------------------------

        Iterator      begin() { return Data(); }
        Iterator      end() { return Data() + count; }
        ConstIterator begin() const { return Data(); }
        ConstIterator end() const { return Data() + count; }

      private:
        T * Data() { return reinterpret_cast<T *>( storage ); }
        const T * Data() const { return reinterpret_cast<const T *>( storage ); }

        void CopyFrom( const FixedList & other ) {
            const T * src = other.Data();
            T * dst = Data();
            for ( u32 i = 0; i < other.count; ++i ) {
                new ( &dst[i] ) T( src[i] );
            }
            count = other.count;
        }

        void MoveFrom( FixedList & other ) noexcept {
            T * src = other.Data();
            T * dst = Data();
            for ( u32 i = 0; i < other.count; ++i ) {
                new ( &dst[i] ) T( std::move( src[i] ) );
            }
            count = other.count;
            other.Clear();
        }

        alignas( T ) u8 storage[sizeof( T ) * Size];
        u32 count = 0;
    };

    template <typename T, u32 Size>
    bool operator==( const FixedList<T, Size> & lhs, const FixedList<T, Size> & rhs ) {
        if ( lhs.GetCount() != rhs.GetCount() ) {
            return false;
        }
        for ( u32 i = 0; i < lhs.GetCount(); ++i ) {
            if ( !( lhs[i] == rhs[i] ) ) {
                return false;
            }
        }
        return true;
    }

    template <typename T, u32 Size>
    bool operator!=( const FixedList<T, Size> & lhs, const FixedList<T, Size> & rhs ) {
        return !( lhs == rhs );
    }

} // namespace sloth
