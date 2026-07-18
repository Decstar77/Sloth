#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

// ------------------------------------------------------------------------
// Fixed-width integer types
// ------------------------------------------------------------------------

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using usize = size_t;
using isize = ptrdiff_t;

using b8  = i8;
using b32 = i32;

// ------------------------------------------------------------------------
// Platform detection
// ------------------------------------------------------------------------
//
// SLOTH_PLATFORM_WINDOWS is defined by premake (see premake5.lua). The
// others are inferred here for completeness even though only Windows is
// currently built.

#if defined(SLOTH_PLATFORM_WINDOWS)
    #define SL_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define SL_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define SL_PLATFORM_MACOS 1
#else
    #error "Sloth: unsupported platform"
#endif

// ------------------------------------------------------------------------
// Build configuration
// ------------------------------------------------------------------------
//
// SLOTH_DEBUG / SLOTH_RELEASE / SLOTH_DIST are defined by premake per
// configuration (see premake5.lua).

#if defined(SLOTH_DEBUG)
    #define SL_BUILD_DEBUG 1
#elif defined(SLOTH_RELEASE)
    #define SL_BUILD_RELEASE 1
#elif defined(SLOTH_DIST)
    #define SL_BUILD_DIST 1
#endif

// ------------------------------------------------------------------------
// Compiler helpers
// ------------------------------------------------------------------------

#if defined(_MSC_VER)
    #define SL_DEBUGBREAK() __debugbreak()
    #define SL_INLINE __forceinline
    #define SL_NOINLINE __declspec(noinline)
#elif defined(__clang__) || defined(__GNUC__)
    #define SL_DEBUGBREAK() __builtin_trap()
    #define SL_INLINE inline __attribute__((always_inline))
    #define SL_NOINLINE __attribute__((noinline))
#else
    #define SL_DEBUGBREAK()
    #define SL_INLINE inline
    #define SL_NOINLINE
#endif

#define SL_UNUSED(x) (void)(x)

#define SL_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

#define SL_BIT(x) (1u << (x))

#define SL_CONCAT_INNER(a, b) a##b
#define SL_CONCAT(a, b) SL_CONCAT_INNER(a, b)

// ------------------------------------------------------------------------
// Non-copyable / non-movable helpers
// ------------------------------------------------------------------------

#define SL_NON_COPYABLE(TypeName)          \
    TypeName(const TypeName&) = delete;    \
    TypeName& operator=(const TypeName&) = delete

#define SL_NON_MOVABLE(TypeName)           \
    TypeName(TypeName&&) = delete;         \
    TypeName& operator=(TypeName&&) = delete

// ------------------------------------------------------------------------
// Logging
// ------------------------------------------------------------------------
//
// Minimal stdio-based logging until a proper logging system exists.
// Errors and warnings go to stderr, everything else to stdout.

#define SL_LOG_TRACE(fmt, ...) std::fprintf(stdout, "[TRACE] " fmt "\n", ##__VA_ARGS__)
#define SL_LOG_INFO(fmt, ...)  std::fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__)
#define SL_LOG_WARN(fmt, ...)  std::fprintf(stderr, "[WARN]  " fmt "\n", ##__VA_ARGS__)
#define SL_LOG_ERROR(fmt, ...) std::fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define SL_LOG_FATAL(fmt, ...) std::fprintf(stderr, "[FATAL] " fmt "\n", ##__VA_ARGS__)

// ------------------------------------------------------------------------
// Asserts
// ------------------------------------------------------------------------
//
// SL_ASSERT is stripped in dist builds. SL_VERIFY always evaluates its
// expression (safe to use for calls with side effects) but only logs /
// breaks in non-dist builds.

#if !defined(SL_BUILD_DIST)
    #define SL_ENABLE_ASSERTS 1
#endif

#if defined(SL_ENABLE_ASSERTS)
    #define SL_ASSERT_MSG(expr, fmt, ...)                                            \
        do                                                                           \
        {                                                                            \
            if (!(expr))                                                            \
            {                                                                        \
                SL_LOG_FATAL("Assertion failed: %s (%s:%d) " fmt, #expr, __FILE__,   \
                              __LINE__, ##__VA_ARGS__);                              \
                SL_DEBUGBREAK();                                                     \
            }                                                                        \
        } while (0)

    #define SL_ASSERT(expr) SL_ASSERT_MSG(expr, "")

    #define SL_VERIFY_MSG(expr, fmt, ...) SL_ASSERT_MSG(expr, fmt, ##__VA_ARGS__)
    #define SL_VERIFY(expr) SL_ASSERT_MSG(expr, "")
#else
    #define SL_ASSERT_MSG(expr, fmt, ...)
    #define SL_ASSERT(expr)

    #define SL_VERIFY_MSG(expr, fmt, ...) (void)(expr)
    #define SL_VERIFY(expr) (void)(expr)
#endif

#define SL_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
