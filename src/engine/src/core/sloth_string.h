#pragma once

#include "core/sloth_defines.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace sloth {
    class Arena;

    // Shared "not found" / "to the end" marker used by StringView and every
    // BaseString-derived type.
    inline constexpr usize StringNpos = static_cast<usize>( -1 );

    namespace detail {
        inline bool IsSpace( char c ) {
            return std::isspace( static_cast<unsigned char>( c ) ) != 0;
        }

        // Naive substring search - fine for the short paths/names/identifiers
        // engine code typically deals with.
        inline usize FindSubstring( const char * haystack, usize haystackLen, const char * needle, usize needleLen, usize startPos ) {
            if ( needleLen == 0 ) {
                return startPos <= haystackLen ? startPos : StringNpos;
            }

            if ( startPos > haystackLen || needleLen > haystackLen - startPos ) {
                return StringNpos;
            }

            usize last = haystackLen - needleLen;
            for ( usize i = startPos; i <= last; ++i ) {
                if ( std::memcmp( haystack + i, needle, needleLen ) == 0 ) {
                    return i;
                }
            }

            return StringNpos;
        }

        // Finds the last occurrence of any character in `chars` (a small
        // null-terminated set, e.g. "/\\"). Used for path splitting.
        inline usize FindLastOf( const char * data, usize length, const char * chars ) {
            for ( usize i = length; i > 0; --i ) {
                char c = data[i - 1];
                for ( const char * p = chars; *p != '\0'; ++p ) {
                    if ( c == *p ) {
                        return i - 1;
                    }
                }
            }

            return StringNpos;
        }

    } // namespace detail

    // A non-owning view over a run of characters. Not guaranteed to be
    // null-terminated - a StringView produced from a prefix slice (e.g.
    // BaseString::GetDirectory()) points into the middle of another buffer,
    // so always use Data()/Length() together rather than treating Data() as
    // a C string.
    class StringView {
      public:
        static constexpr usize npos = StringNpos;

        constexpr StringView() = default;
        constexpr StringView( const char * str, usize len ) : data( str ), length( len ) {}
        StringView( const char * str ) : data( str ), length( str != nullptr ? std::strlen( str ) : 0 ) {}

        const char * Data() const { return data; }
        usize Length() const { return length; }
        bool IsEmpty() const { return length == 0; }

        char operator[]( usize index ) const {
            SL_ASSERT_MSG( index < length, "StringView index out of range (%zu >= %zu)", index, length );
            return data[index];
        }

        bool Equals( StringView other ) const {
            return length == other.length && ( length == 0 || std::memcmp( data, other.data, length ) == 0 );
        }

        bool Contains( char c ) const { return IndexOf( c ) != npos; }
        bool Contains( StringView needle ) const { return IndexOf( needle ) != npos; }

        usize IndexOf( char c, usize startPos = 0 ) const {
            for ( usize i = startPos; i < length; ++i ) {
                if ( data[i] == c ) {
                    return i;
                }
            }
            return npos;
        }

        usize IndexOf( StringView needle, usize startPos = 0 ) const {
            return detail::FindSubstring( data, length, needle.data, needle.length, startPos );
        }

        bool StartsWith( StringView prefix ) const {
            return prefix.length <= length && std::memcmp( data, prefix.data, prefix.length ) == 0;
        }

        bool EndsWith( StringView suffix ) const {
            return suffix.length <= length && std::memcmp( data + ( length - suffix.length ), suffix.data, suffix.length ) == 0;
        }

        StringView Substring( usize start, usize len = npos ) const {
            if ( start >= length ) {
                return StringView();
            }

            usize maxLen = length - start;
            usize actualLen = ( len == npos || len > maxLen ) ? maxLen : len;
            return StringView( data + start, actualLen );
        }

        StringView TrimStart() const {
            usize start = 0;
            while ( start < length && detail::IsSpace( data[start] ) ) {
                ++start;
            }
            return StringView( data + start, length - start );
        }

        StringView TrimEnd() const {
            usize end = length;
            while ( end > 0 && detail::IsSpace( data[end - 1] ) ) {
                --end;
            }
            return StringView( data, end );
        }

        StringView Trim() const { return TrimStart().TrimEnd(); }

      private:
        const char * data = nullptr;
        usize length = 0;
    };

    inline bool operator==( StringView lhs, StringView rhs ) { return lhs.Equals( rhs ); }
    inline bool operator!=( StringView lhs, StringView rhs ) { return !lhs.Equals( rhs ); }

    // CRTP base implementing every string algorithm in terms of a small
    // protocol that each derived storage type provides:
    //
    //   char* Data(); const char* Data() const;   raw buffer, always valid and null-terminated
    //   usize Length() const;                     current length, excluding the null terminator
    //   usize Capacity() const;                    max length the buffer can currently hold
    //   void SetLength(usize newLength);           adjusts length and re-terminates
    //   bool EnsureCapacity(usize required);        true if `required` now fits (may grow); false if it had to clamp
    //
    // This gives FixedString and ArenaString a large, uniform, Python/C#-like
    // API (Contains, Split, RemoveExtension, ...) for free.
    template <typename Derived>
    class BaseString {
      public:
        static constexpr usize npos = StringNpos;

        const char * CStr() const { return Self()->Data(); }
        char * Data() { return Self()->Data(); }
        const char * Data() const { return Self()->Data(); }
        usize Length() const { return Self()->Length(); }
        usize Capacity() const { return Self()->Capacity(); }
        bool IsEmpty() const { return Length() == 0; }
        bool Reserve( usize newCapacity ) { return Self()->EnsureCapacity( newCapacity ); }
        void Clear() { Self()->SetLength( 0 ); }

        char & operator[]( usize index ) {
            SL_ASSERT_MSG( index < Length(), "String index out of range (%zu >= %zu)", index, Length() );
            return Self()->Data()[index];
        }

        char operator[]( usize index ) const {
            SL_ASSERT_MSG( index < Length(), "String index out of range (%zu >= %zu)", index, Length() );
            return Self()->Data()[index];
        }

        // --- Assignment / building -----------------------------------------

        Derived & Assign( const char * str, usize len ) {
            usize copyLen = len;
            if ( !Self()->EnsureCapacity( len ) ) {
                usize capacity = Self()->Capacity();
                copyLen = len < capacity ? len : capacity;
                SL_LOG_WARN( "String truncated on Assign (requested=%zu, capacity=%zu)", len, capacity );
            }

            if ( copyLen > 0 ) {
                std::memmove( Self()->Data(), str, copyLen );
            }
            Self()->SetLength( copyLen );
            return *Self();
        }

        Derived & Assign( const char * str ) { return Assign( str, str != nullptr ? std::strlen( str ) : 0 ); }
        Derived & Assign( StringView view ) { return Assign( view.Data(), view.Length() ); }

        template <typename Other>
        Derived & Assign( const BaseString<Other> & other ) { return Assign( other.CStr(), other.Length() ); }

        Derived & Append( const char * str, usize len ) {
            usize curLen = Length();
            usize desiredLen = curLen + len;
            usize copyLen = len;

            if ( !Self()->EnsureCapacity( desiredLen ) ) {
                usize capacity = Self()->Capacity();
                copyLen = capacity > curLen ? capacity - curLen : 0;
                SL_LOG_WARN( "String truncated on Append (requested=%zu, capacity=%zu)", desiredLen, capacity );
            }

            if ( copyLen > 0 ) {
                std::memmove( Self()->Data() + curLen, str, copyLen );
            }
            Self()->SetLength( curLen + copyLen );
            return *Self();
        }

        Derived & Append( const char * str ) { return Append( str, str != nullptr ? std::strlen( str ) : 0 ); }
        Derived & Append( char c ) { return Append( &c, 1 ); }
        Derived & Append( StringView view ) { return Append( view.Data(), view.Length() ); }

        template <typename Other>
        Derived & Append( const BaseString<Other> & other ) { return Append( other.CStr(), other.Length() ); }

        Derived & operator+=( const char * str ) { return Append( str ); }
        Derived & operator+=( char c ) { return Append( c ); }
        Derived & operator+=( StringView view ) { return Append( view ); }

        // printf-style formatting. Grows to fit (ArenaString) or truncates to
        // capacity (FixedString), same as Assign/Append.
        Derived & Format( const char * fmt, ... ) {
            va_list args;
            va_start( args, fmt );
            va_list argsCopy;
            va_copy( argsCopy, args );

            int required = std::vsnprintf( nullptr, 0, fmt, args );
            va_end( args );

            if ( required < 0 ) {
                va_end( argsCopy );
                return *Self();
            }

            usize requiredLen = static_cast<usize>( required );
            bool fits = Self()->EnsureCapacity( requiredLen );
            usize writeLen = fits ? requiredLen : Self()->Capacity();

            if ( writeLen > 0 ) {
                // vsnprintf's size parameter includes room for the null
                // terminator, hence writeLen + 1.
                std::vsnprintf( Self()->Data(), writeLen + 1, fmt, argsCopy );
            }
            va_end( argsCopy );

            Self()->SetLength( writeLen );
            return *Self();
        }

        // --- Comparison ------------------------------------------------------

        bool Equals( const char * other ) const {
            if ( other == nullptr ) {
                return false;
            }
            usize otherLen = std::strlen( other );
            return otherLen == Length() && std::memcmp( Self()->Data(), other, otherLen ) == 0;
        }

        bool Equals( StringView other ) const {
            return other.Length() == Length() && ( Length() == 0 || std::memcmp( Self()->Data(), other.Data(), Length() ) == 0 );
        }

        bool EqualsIgnoreCase( const char * other ) const {
            if ( other == nullptr ) {
                return false;
            }

            usize otherLen = std::strlen( other );
            if ( otherLen != Length() ) {
                return false;
            }

            const char * data = Self()->Data();
            for ( usize i = 0; i < otherLen; ++i ) {
                if ( std::tolower( static_cast<unsigned char>( data[i] ) ) != std::tolower( static_cast<unsigned char>( other[i] ) ) ) {
                    return false;
                }
            }
            return true;
        }

        i32 Compare( const char * other ) const { return std::strcmp( Self()->Data(), other != nullptr ? other : "" ); }

        // --- Search ----------------------------------------------------------

        bool Contains( const char * needle ) const { return IndexOf( needle ) != npos; }
        bool Contains( char c ) const { return IndexOf( c ) != npos; }

        usize IndexOf( const char * needle, usize startPos = 0 ) const {
            if ( needle == nullptr ) {
                return npos;
            }
            return detail::FindSubstring( Self()->Data(), Length(), needle, std::strlen( needle ), startPos );
        }

        usize IndexOf( char c, usize startPos = 0 ) const {
            const char * data = Self()->Data();
            usize len = Length();
            for ( usize i = startPos; i < len; ++i ) {
                if ( data[i] == c ) {
                    return i;
                }
            }
            return npos;
        }

        usize LastIndexOf( char c ) const {
            const char * data = Self()->Data();
            for ( usize i = Length(); i > 0; --i ) {
                if ( data[i - 1] == c ) {
                    return i - 1;
                }
            }
            return npos;
        }

        usize LastIndexOf( const char * needle ) const {
            if ( needle == nullptr ) {
                return npos;
            }

            usize needleLen = std::strlen( needle );
            usize len = Length();
            if ( needleLen == 0 || needleLen > len ) {
                return npos;
            }

            const char * data = Self()->Data();
            for ( usize i = len - needleLen + 1; i > 0; --i ) {
                if ( std::memcmp( data + i - 1, needle, needleLen ) == 0 ) {
                    return i - 1;
                }
            }
            return npos;
        }

        bool StartsWith( const char * prefix ) const {
            if ( prefix == nullptr ) {
                return false;
            }
            usize prefixLen = std::strlen( prefix );
            return prefixLen <= Length() && std::memcmp( Self()->Data(), prefix, prefixLen ) == 0;
        }

        bool EndsWith( const char * suffix ) const {
            if ( suffix == nullptr ) {
                return false;
            }
            usize suffixLen = std::strlen( suffix );
            usize len = Length();
            return suffixLen <= len && std::memcmp( Self()->Data() + ( len - suffixLen ), suffix, suffixLen ) == 0;
        }

        // --- Case / whitespace -------------------------------------------------

        Derived & ToUpper() {
            char * data = Self()->Data();
            usize len = Length();
            for ( usize i = 0; i < len; ++i ) {
                data[i] = static_cast<char>( std::toupper( static_cast<unsigned char>( data[i] ) ) );
            }
            return *Self();
        }

        Derived & ToLower() {
            char * data = Self()->Data();
            usize len = Length();
            for ( usize i = 0; i < len; ++i ) {
                data[i] = static_cast<char>( std::tolower( static_cast<unsigned char>( data[i] ) ) );
            }
            return *Self();
        }

        Derived & TrimStart() {
            char * data = Self()->Data();
            usize len = Length();
            usize start = 0;
            while ( start < len && detail::IsSpace( data[start] ) ) {
                ++start;
            }

            if ( start > 0 ) {
                std::memmove( data, data + start, len - start );
                Self()->SetLength( len - start );
            }
            return *Self();
        }

        Derived & TrimEnd() {
            const char * data = Self()->Data();
            usize len = Length();
            usize end = len;
            while ( end > 0 && detail::IsSpace( data[end - 1] ) ) {
                --end;
            }

            if ( end != len ) {
                Self()->SetLength( end );
            }
            return *Self();
        }

        Derived & Trim() {
            TrimEnd();
            TrimStart();
            return *Self();
        }

        // --- Replace ------------------------------------------------------

        Derived & Replace( char oldChar, char newChar ) {
            char * data = Self()->Data();
            usize len = Length();
            for ( usize i = 0; i < len; ++i ) {
                if ( data[i] == oldChar ) {
                    data[i] = newChar;
                }
            }
            return *Self();
        }

        Derived & Replace( const char * oldStr, const char * newStr ) {
            if ( oldStr == nullptr || *oldStr == '\0' ) {
                return *Self();
            }

            usize oldLen = std::strlen( oldStr );
            usize newLen = newStr != nullptr ? std::strlen( newStr ) : 0;

            constexpr usize kScratchSize = 4096;
            char scratch[kScratchSize];

            usize srcLen = Length();
            SL_ASSERT_MSG( srcLen < kScratchSize, "Replace: string too large for scratch buffer (%zu bytes)", srcLen );
            usize copyLen = srcLen < kScratchSize - 1 ? srcLen : kScratchSize - 1;
            std::memcpy( scratch, Self()->Data(), copyLen );

            Self()->SetLength( 0 );

            usize pos = 0;
            while ( pos < copyLen ) {
                usize found = detail::FindSubstring( scratch + pos, copyLen - pos, oldStr, oldLen, 0 );
                if ( found == npos ) {
                    Append( scratch + pos, copyLen - pos );
                    break;
                }

                if ( found > 0 ) {
                    Append( scratch + pos, found );
                }
                if ( newLen > 0 ) {
                    Append( newStr, newLen );
                }
                pos += found + oldLen;
            }

            return *Self();
        }

        // --- Views / substrings ---------------------------------------------

        StringView View() const { return StringView( Self()->Data(), Length() ); }

        StringView Substring( usize start, usize len = npos ) const {
            usize length = Length();
            if ( start >= length ) {
                return StringView();
            }

            usize maxLen = length - start;
            usize actualLen = ( len == npos || len > maxLen ) ? maxLen : len;
            return StringView( Self()->Data() + start, actualLen );
        }

        // Splits on `delimiter`, writing up to `maxParts` views into
        // `outParts`. Returns the number of parts written.
        usize Split( char delimiter, StringView * outParts, usize maxParts ) const {
            const char * data = Self()->Data();
            usize len = Length();
            usize count = 0;
            usize partStart = 0;

            for ( usize i = 0; i <= len && count < maxParts; ++i ) {
                if ( i == len || data[i] == delimiter ) {
                    outParts[count++] = StringView( data + partStart, i - partStart );
                    partStart = i + 1;
                }
            }

            return count;
        }

        // --- Paths ------------------------------------------------------------

        StringView GetDirectory() const {
            const char * data = Self()->Data();
            usize len = Length();
            usize slash = detail::FindLastOf( data, len, "/\\" );
            if ( slash == npos ) {
                return StringView();
            }
            return StringView( data, slash );
        }

        StringView GetFileName() const {
            const char * data = Self()->Data();
            usize len = Length();
            usize slash = detail::FindLastOf( data, len, "/\\" );
            usize start = slash == npos ? 0 : slash + 1;
            return StringView( data + start, len - start );
        }

        StringView GetExtension() const {
            StringView fileName = GetFileName();
            usize dot = detail::FindLastOf( fileName.Data(), fileName.Length(), "." );
            if ( dot == npos || dot == 0 ) {
                return StringView();
            }
            return StringView( fileName.Data() + dot + 1, fileName.Length() - dot - 1 );
        }

        StringView GetFileNameWithoutExtension() const {
            StringView fileName = GetFileName();
            usize dot = detail::FindLastOf( fileName.Data(), fileName.Length(), "." );
            if ( dot == npos || dot == 0 ) {
                return fileName;
            }
            return StringView( fileName.Data(), dot );
        }

        // Truncates the string in place, dropping the extension (if any).
        Derived & RemoveExtension() {
            const char * data = Self()->Data();
            usize len = Length();
            usize slash = detail::FindLastOf( data, len, "/\\" );
            usize nameStart = slash == npos ? 0 : slash + 1;
            usize dot = detail::FindLastOf( data + nameStart, len - nameStart, "." );
            if ( dot != npos && dot != 0 ) {
                Self()->SetLength( nameStart + dot );
            }
            return *Self();
        }

        // Keeps only the file name portion, dropping any leading directory.
        Derived & RemoveFilePath() {
            StringView fileName = GetFileName();
            if ( fileName.Data() != Self()->Data() ) {
                usize len = fileName.Length();
                if ( len > 0 ) {
                    std::memmove( Self()->Data(), fileName.Data(), len );
                }
                Self()->SetLength( len );
            }
            return *Self();
        }

        Derived & ReplaceExtension( const char * newExtension ) {
            RemoveExtension();
            if ( newExtension != nullptr && *newExtension != '\0' ) {
                Append( '.' );
                Append( newExtension );
            }
            return *Self();
        }

        // --- Numeric conversion ------------------------------------------------

        i32 ToInt( i32 defaultValue = 0 ) const {
            if ( IsEmpty() ) {
                return defaultValue;
            }
            char * end = nullptr;
            long value = std::strtol( Self()->Data(), &end, 10 );
            return end == Self()->Data() ? defaultValue : static_cast<i32>( value );
        }

        f32 ToFloat( f32 defaultValue = 0.0f ) const {
            if ( IsEmpty() ) {
                return defaultValue;
            }
            char * end = nullptr;
            float value = std::strtof( Self()->Data(), &end );
            return end == Self()->Data() ? defaultValue : value;
        }

        // FNV-1a. Suitable for hash-map keys, not for cryptographic use.
        u32 GetHash() const {
            const char * data = Self()->Data();
            usize len = Length();
            u32 hash = 2166136261u;
            for ( usize i = 0; i < len; ++i ) {
                hash ^= static_cast<u8>( data[i] );
                hash *= 16777619u;
            }
            return hash;
        }

      protected:
        ~BaseString() = default;

      private:
        Derived * Self() { return static_cast<Derived *>( this ); }
        const Derived * Self() const { return static_cast<const Derived *>( this ); }
    };

    template <typename Derived>
    bool operator==( const BaseString<Derived> & lhs, const char * rhs ) { return lhs.Equals( rhs ); }

    template <typename Derived>
    bool operator==( const char * lhs, const BaseString<Derived> & rhs ) { return rhs.Equals( lhs ); }

    template <typename Derived>
    bool operator!=( const BaseString<Derived> & lhs, const char * rhs ) { return !lhs.Equals( rhs ); }

    template <typename Derived>
    bool operator!=( const char * lhs, const BaseString<Derived> & rhs ) { return !rhs.Equals( lhs ); }

    template <typename LDerived, typename RDerived>
    bool operator==( const BaseString<LDerived> & lhs, const BaseString<RDerived> & rhs ) { return lhs.Equals( rhs.CStr() ); }

    template <typename LDerived, typename RDerived>
    bool operator!=( const BaseString<LDerived> & lhs, const BaseString<RDerived> & rhs ) { return !lhs.Equals( rhs.CStr() ); }

    // Fixed-capacity, inline-storage string. `Size` includes the null
    // terminator, so FixedString<64> can hold up to 63 characters.
    template <usize Size>
    class FixedString : public BaseString<FixedString<Size>> {
        static_assert( Size > 1, "FixedString size must be large enough to hold at least a null terminator" );

      public:
        FixedString() { buffer[0] = '\0'; }
        FixedString( const char * str ) { this->Assign( str ); }
        FixedString( StringView view ) { this->Assign( view ); }
        FixedString( const FixedString & other ) { this->Assign( other.CStr(), other.Length() ); }

        template <usize OtherSize>
        FixedString( const FixedString<OtherSize> & other ) { this->Assign( other.CStr(), other.Length() ); }

        FixedString & operator=( const char * str ) {
            this->Assign( str );
            return *this;
        }
        FixedString & operator=( StringView view ) {
            this->Assign( view );
            return *this;
        }
        FixedString & operator=( const FixedString & other ) {
            this->Assign( other.CStr(), other.Length() );
            return *this;
        }

        char * Data() { return buffer; }
        const char * Data() const { return buffer; }
        usize Length() const { return length; }
        usize Capacity() const { return kCapacity; }

        void SetLength( usize newLength ) {
            SL_ASSERT_MSG( newLength <= kCapacity, "FixedString<%zu> overflow (requested length=%zu)", Size, newLength );
            length = newLength <= kCapacity ? newLength : kCapacity;
            buffer[length] = '\0';
        }

        bool EnsureCapacity( usize required ) const { return required <= kCapacity; }

      private:
        static constexpr usize kCapacity = Size - 1;

        char buffer[Size];
        usize length = 0;
    };

    using SmallString = FixedString<64>;
    using LargeString = FixedString<256>;

    // Growable string allocated out of an Arena. Growth reallocates a bigger
    // block from the arena and copies the old contents over - the previous
    // block is simply left dead, since arenas only support bulk Reset(),
    // not freeing individual allocations. Fine for typical build-up-then-use
    // patterns (e.g. assembling a path once per frame out of the frame arena).
    class ArenaString : public BaseString<ArenaString> {
      public:
        explicit ArenaString( Arena * arena );
        ArenaString( Arena * arena, const char * str );
        ArenaString( Arena * arena, StringView view );

        SL_NON_COPYABLE( ArenaString );

        ArenaString( ArenaString && other ) noexcept;
        ArenaString & operator=( ArenaString && other ) noexcept;

        // May be null until the string has been given content; prefer CStr()
        // for read-only access, which always returns a valid C string.
        char * Data() { return buffer; }
        const char * Data() const { return buffer != nullptr ? buffer : ""; }
        usize Length() const { return length; }
        usize Capacity() const { return capacity; }

        void SetLength( usize newLength );
        bool EnsureCapacity( usize required );

      private:
        Arena * arena = nullptr;
        char * buffer = nullptr;
        usize length = 0;
        usize capacity = 0;
    };

} // namespace sloth
