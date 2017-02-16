#ifndef _COMPILER_H
#define _COMPILER_H

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#elif __MINGW32__
#define NORETURN __attribute__((noreturn))
#elif __clang__
#define NORETURN __attribute__((noreturn))
#elif _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#error "Unsupported compiler"
#endif

#ifdef __GNUC__
#define ALIGN_CACHELINE __attribute__((aligned(CACHE_LINE)))
#else
#error "Unsupported compiler"
#endif

#endif
