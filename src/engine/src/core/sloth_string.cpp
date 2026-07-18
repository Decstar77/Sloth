#include "sloth_string.h"

#include "core/sloth_arena.h"

#include <cstring>

namespace sloth
{
    ArenaString::ArenaString(Arena* arenaPtr)
        : arena(arenaPtr)
    {
        SL_ASSERT_MSG(arena != nullptr, "ArenaString requires a valid arena");
    }

    ArenaString::ArenaString(Arena* arenaPtr, const char* str)
        : arena(arenaPtr)
    {
        SL_ASSERT_MSG(arena != nullptr, "ArenaString requires a valid arena");
        Assign(str);
    }

    ArenaString::ArenaString(Arena* arenaPtr, StringView view)
        : arena(arenaPtr)
    {
        SL_ASSERT_MSG(arena != nullptr, "ArenaString requires a valid arena");
        Assign(view);
    }

    ArenaString::ArenaString(ArenaString&& other) noexcept
        : arena(other.arena), buffer(other.buffer), length(other.length), capacity(other.capacity)
    {
        other.arena = nullptr;
        other.buffer = nullptr;
        other.length = 0;
        other.capacity = 0;
    }

    ArenaString& ArenaString::operator=(ArenaString&& other) noexcept
    {
        if (this != &other)
        {
            arena = other.arena;
            buffer = other.buffer;
            length = other.length;
            capacity = other.capacity;

            other.arena = nullptr;
            other.buffer = nullptr;
            other.length = 0;
            other.capacity = 0;
        }
        return *this;
    }

    void ArenaString::SetLength(usize newLength)
    {
        SL_ASSERT_MSG(newLength <= capacity, "ArenaString overflow (capacity=%zu, requested=%zu)", capacity, newLength);
        length = newLength <= capacity ? newLength : capacity;
        if (buffer != nullptr)
        {
            buffer[length] = '\0';
        }
    }

    bool ArenaString::EnsureCapacity(usize required)
    {
        if (required <= capacity)
        {
            return true;
        }

        SL_ASSERT_MSG(arena != nullptr, "ArenaString has no arena to grow from");
        if (arena == nullptr)
        {
            return false;
        }

        usize newCapacity = capacity == 0 ? 16 : capacity;
        while (newCapacity < required)
        {
            newCapacity *= 2;
        }

        // The old block (if any) is left dead in the arena - see the class
        // comment in sloth_string.h.
        char* newBuffer = arena->PushArray<char>(newCapacity + 1);
        if (newBuffer == nullptr)
        {
            return false;
        }

        if (length > 0 && buffer != nullptr)
        {
            std::memcpy(newBuffer, buffer, length);
        }
        newBuffer[length] = '\0';

        buffer = newBuffer;
        capacity = newCapacity;
        return true;
    }

} // namespace sloth
