#pragma once

#include "core/sloth_defines.h"

#include <new>
#include <type_traits>
#include <utility>

namespace sloth {

    // Shared storage + read-only surface for the fixed-capacity list types
    // below: inline byte storage for `Size` elements of `T`, a live `count`,
    // and every operation that doesn't care whether elements are
    // constructed via placement-new (FixedList) or just sit there as plain
    // bytes (FixedPodList). Mutation (Add/Remove/Clear/...) is left to the
    // derived classes since that's exactly where the two flavors differ.
    //
    // Deliberately has no user-declared constructors/destructor and no
    // member initializers, so it stays a trivial type regardless of T -
    // FixedPodList relies on this to remain trivial itself (see below).
    template <typename T, u32 Size>
    class ListBase {
        static_assert( Size > 0, "List size must be greater than zero" );

      public:
        using Iterator = T *;
        using ConstIterator = const T *;

        // --- Capacity -----------------------------------------------------

        u32  GetCount() const { return count; }
        u32  GetCapacity() const { return Size; }
        bool IsEmpty() const { return count == 0; }
        bool IsFull() const { return count == Size; }

        // --- Element access -------------------------------------------------

        T & operator[]( u32 index ) {
            SL_ASSERT_MSG( index < count, "List index out of range (%u >= %u)", index, count );
            return Data()[index];
        }

        const T & operator[]( u32 index ) const {
            SL_ASSERT_MSG( index < count, "List index out of range (%u >= %u)", index, count );
            return Data()[index];
        }

        T & Front() {
            SL_ASSERT_MSG( count > 0, "List::Front() called on empty list" );
            return Data()[0];
        }

        const T & Front() const {
            SL_ASSERT_MSG( count > 0, "List::Front() called on empty list" );
            return Data()[0];
        }

        T & Back() {
            SL_ASSERT_MSG( count > 0, "List::Back() called on empty list" );
            return Data()[count - 1];
        }

        const T & Back() const {
            SL_ASSERT_MSG( count > 0, "List::Back() called on empty list" );
            return Data()[count - 1];
        }

        T *       GetData() { return Data(); }
        const T * GetData() const { return Data(); }

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

      protected:
        T * Data() { return reinterpret_cast<T *>( storage ); }
        const T * Data() const { return reinterpret_cast<const T *>( storage ); }

        alignas( T ) u8 storage[sizeof( T ) * Size];
        u32 count; // No default member initializer on purpose - see FixedPodList.
    };

    // Fixed-capacity, inline-storage list (no heap allocation). `Size` is the
    // maximum number of elements; `Count` tracks how many are currently live.
    // Element constructors/destructors are respected via placement-new /
    // explicit destructor calls, so T does not need to be trivial. Bounds are
    // checked with SL_ASSERT_MSG (stripped in Dist builds, like everywhere
    // else in the engine).
    //
    // Not usable as a union member or inside a POD struct: the constructors/
    // destructor below make FixedList non-trivial, which deletes the
    // containing union's corresponding special member functions. Use
    // FixedPodList for that.
    template <typename T, u32 Size>
    class FixedList : public ListBase<T, Size> {
      public:
        FixedList() { this->count = 0; }

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

        // --- Mutation -------------------------------------------------------

        T & Add( const T & value ) {
            SL_ASSERT_MSG( this->count < Size, "FixedList<%u> overflow on Add", Size );
            T * slot = &this->Data()[this->count];
            new ( slot ) T( value );
            ++this->count;
            return *slot;
        }

        T & Add( T && value ) {
            SL_ASSERT_MSG( this->count < Size, "FixedList<%u> overflow on Add", Size );
            T * slot = &this->Data()[this->count];
            new ( slot ) T( std::move( value ) );
            ++this->count;
            return *slot;
        }

        template <typename... Args>
        T & Emplace( Args &&... args ) {
            SL_ASSERT_MSG( this->count < Size, "FixedList<%u> overflow on Emplace", Size );
            T * slot = &this->Data()[this->count];
            new ( slot ) T( std::forward<Args>( args )... );
            ++this->count;
            return *slot;
        }

        // Removes the element at `index`, shifting subsequent elements down
        // to preserve order. O(n).
        void RemoveAt( u32 index ) {
            SL_ASSERT_MSG( index < this->count, "FixedList::RemoveAt index out of range (%u >= %u)", index, this->count );
            T * data = this->Data();
            for ( u32 i = index; i + 1 < this->count; ++i ) {
                data[i] = std::move( data[i + 1] );
            }
            data[this->count - 1].~T();
            --this->count;
        }

        // Removes the element at `index` by moving the last element into its
        // slot. O(1), does not preserve order.
        void RemoveAtSwap( u32 index ) {
            SL_ASSERT_MSG( index < this->count, "FixedList::RemoveAtSwap index out of range (%u >= %u)", index, this->count );
            T * data = this->Data();
            if ( index != this->count - 1 ) {
                data[index] = std::move( data[this->count - 1] );
            }
            data[this->count - 1].~T();
            --this->count;
        }

        // Removes the first element equal to `value` (order-preserving).
        // Returns true if an element was found and removed.
        bool Remove( const T & value ) {
            i64 index = this->IndexOf( value );
            if ( index < 0 ) {
                return false;
            }
            RemoveAt( static_cast<u32>( index ) );
            return true;
        }

        // Same as Remove(), but uses RemoveAtSwap() internally (does not
        // preserve order).
        bool RemoveSwap( const T & value ) {
            i64 index = this->IndexOf( value );
            if ( index < 0 ) {
                return false;
            }
            RemoveAtSwap( static_cast<u32>( index ) );
            return true;
        }

        void PopBack() {
            SL_ASSERT_MSG( this->count > 0, "FixedList::PopBack called on empty list" );
            this->Data()[this->count - 1].~T();
            --this->count;
        }

        void Clear() {
            T * data = this->Data();
            for ( u32 i = 0; i < this->count; ++i ) {
                data[i].~T();
            }
            this->count = 0;
        }

        // Resizes to `newCount`. Growing default-constructs new elements;
        // shrinking destroys the truncated ones.
        void Resize( u32 newCount ) {
            SL_ASSERT_MSG( newCount <= Size, "FixedList<%u> overflow on Resize (requested=%u)", Size, newCount );
            T * data = this->Data();
            if ( newCount > this->count ) {
                for ( u32 i = this->count; i < newCount; ++i ) {
                    new ( &data[i] ) T();
                }
            } else {
                for ( u32 i = newCount; i < this->count; ++i ) {
                    data[i].~T();
                }
            }
            this->count = newCount;
        }

      private:
        void CopyFrom( const FixedList & other ) {
            const T * src = other.Data();
            T * dst = this->Data();
            for ( u32 i = 0; i < other.count; ++i ) {
                new ( &dst[i] ) T( src[i] );
            }
            this->count = other.count;
        }

        void MoveFrom( FixedList & other ) noexcept {
            T * src = other.Data();
            T * dst = this->Data();
            for ( u32 i = 0; i < other.count; ++i ) {
                new ( &dst[i] ) T( std::move( src[i] ) );
            }
            this->count = other.count;
            other.Clear();
        }
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

    // Fixed-capacity, inline-storage list for trivial T - no constructors, no
    // destructor, no placement-new, no member initializers; elements are
    // moved around with plain `=`. Left fully trivial on purpose so it:
    //   - can be used as a union member without deleting the union's default
    //     ctor/copy/dtor (a FixedList member would delete all of those),
    //   - can sit inside a plain-old-data struct that gets memcpy'd, zeroed
    //     with `= {}`, or serialized as raw bytes.
    // Trade-off: `count` (and any slots beyond it on a freshly-declared
    // instance) start out uninitialized, exactly like every other member of
    // a POD struct - zero the whole containing struct/union yourself
    // (`= {}` or memset) if you need a clean starting state.
    template <typename T, u32 Size>
    struct FixedPodList : public ListBase<T, Size> {
        static_assert( std::is_trivial<T>::value, "FixedPodList<T> requires a trivial T - use FixedList for non-trivial types" );

        // --- Mutation -------------------------------------------------------

        T & Add( const T & value ) {
            SL_ASSERT_MSG( this->count < Size, "FixedPodList<%u> overflow on Add", Size );
            T & slot = this->Data()[this->count];
            slot = value;
            ++this->count;
            return slot;
        }

        // Removes the element at `index`, shifting subsequent elements down
        // to preserve order. O(n).
        void RemoveAt( u32 index ) {
            SL_ASSERT_MSG( index < this->count, "FixedPodList::RemoveAt index out of range (%u >= %u)", index, this->count );
            T * data = this->Data();
            for ( u32 i = index; i + 1 < this->count; ++i ) {
                data[i] = data[i + 1];
            }
            --this->count;
        }

        // Removes the element at `index` by moving the last element into its
        // slot. O(1), does not preserve order.
        void RemoveAtSwap( u32 index ) {
            SL_ASSERT_MSG( index < this->count, "FixedPodList::RemoveAtSwap index out of range (%u >= %u)", index, this->count );
            T * data = this->Data();
            if ( index != this->count - 1 ) {
                data[index] = data[this->count - 1];
            }
            --this->count;
        }

        // Removes the first element equal to `value` (order-preserving).
        // Returns true if an element was found and removed.
        bool Remove( const T & value ) {
            i64 index = this->IndexOf( value );
            if ( index < 0 ) {
                return false;
            }
            RemoveAt( static_cast<u32>( index ) );
            return true;
        }

        // Same as Remove(), but uses RemoveAtSwap() internally (does not
        // preserve order).
        bool RemoveSwap( const T & value ) {
            i64 index = this->IndexOf( value );
            if ( index < 0 ) {
                return false;
            }
            RemoveAtSwap( static_cast<u32>( index ) );
            return true;
        }

        void PopBack() {
            SL_ASSERT_MSG( this->count > 0, "FixedPodList::PopBack called on empty list" );
            --this->count;
        }

        void Clear() { this->count = 0; }

        // Resizes to `newCount`. Growing leaves the new slots uninitialized
        // (POD semantics - no constructor to call); shrinking just truncates.
        void Resize( u32 newCount ) {
            SL_ASSERT_MSG( newCount <= Size, "FixedPodList<%u> overflow on Resize (requested=%u)", Size, newCount );
            this->count = newCount;
        }
    };

    template <typename T, u32 Size>
    bool operator==( const FixedPodList<T, Size> & lhs, const FixedPodList<T, Size> & rhs ) {
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
    bool operator!=( const FixedPodList<T, Size> & lhs, const FixedPodList<T, Size> & rhs ) {
        return !( lhs == rhs );
    }

} // namespace sloth
