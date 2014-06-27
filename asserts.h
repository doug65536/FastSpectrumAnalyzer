#ifndef ASSERTS_H
#define ASSERTS_H

#include <iostream>
#include <cstdarg>
#include <utility>

#ifdef _WIN32
#include <intrin.h>
#define AssertBreakForce(expr) ((expr) ? 1 : ((void)__debugbreak(), 0))
#else
#include <signal.h>
#define AssertBreakForce(expr) ((expr) ? 1 : ((void)raise_with_expr(#expr, SIGTRAP), 0))
#endif

#ifndef NDEBUG
#define AssertBreak(expr) AssertBreakForce(expr)
#define AssertOnly(e) e
#else
#define AssertBreak(expr) ((void)0)
#define AssertOnly(e)
#endif

#define AssertBreakMessage(message) AssertBreak(!message)

static inline void DebugMessageOut()
{
}

template<typename T1>
void DebugMessageOut(T1&& arg)
{
    std::cerr << std::forward<T1>(arg);
}

template<typename T1, typename ...T>
void DebugMessageOut(T1&& arg, T&& ...args)
{
    std::cerr << std::forward<T1>(arg);
    DebugMessageOut(std::forward<T>(args)...);
}

template<typename ...T>
void DebugMessage(T&& ...args)
{
    DebugMessageOut(std::forward<T>(args)...);
    if (sizeof...(args) > 0)
        std::cerr << std::endl;
}

static inline void raise_with_expr(char const* expr, int trap)
{
    std::cerr << "Assertion failed: " << expr << std::endl;
    raise(trap);
}

static inline void DebugMessagev_C(const char *msg, va_list ap)
{
    vfprintf(stderr, msg, ap);
}

static inline void DebugMessage_C(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    DebugMessagev_C(msg, ap);
    va_end(ap);
}

#ifndef _countof
#define _countof(x) ((sizeof(x))/sizeof(*(x)))
#endif

class AssertCompare
{
public:
    template<typename T1, typename T2>
    static void cmp(T1 a, T2 b, char const* oper, char const*file, int line);
};

template<typename T1, typename T2>
void AssertCompare::cmp(T1 a, T2 b, char const* oper, char const*file, int line)
{
    std::cout << file << "(" << line << "): Assertion failed (" << a << ") " << oper << " (" << b << ")" << std::endl;
    raise(SIGTRAP);
}

#define ASSERTS_FORCE
#if !defined(NDEBUG) || defined(ASSERTS_FORCE)
#define AssertBreakNotEqual(a, b) ((a) != (b) ? (void)0 : AssertCompare::cmp((a), (b), "!=", __FILE__, __LINE__))
#define AssertBreakEqual(a, b) ((a) == (b) ? (void)0 : AssertCompare::cmp((a), (b), "==", __FILE__, __LINE__))
#define AssertBreakGreater(a, b) ((a) > (b) ? (void)0 : AssertCompare::cmp((a), (b), ">", __FILE__, __LINE__))
#define AssertBreakLess(a, b) ((a) < (b) ? (void)0 : AssertCompare::cmp((a), (b), "<", __FILE__, __LINE__))
#define AssertBreakGEqual(a, b) ((a) >= (b) ? (void)0 : AssertCompare::cmp((a), (b), ">=", __FILE__, __LINE__))
#define AssertBreakLEqual(a, b) ((a) <= (b) ? (void)0 : AssertCompare::cmp((a), (b), "<=", __FILE__, __LINE__))
#else
#define AssertBreakNotEqual(a, b) ((void)0)
#define AssertBreakEqual(a, b) ((void)0)
#define AssertBreakGreater(a, b) ((void)0)
#define AssertBreakLess(a, b) ((void)0)
#define AssertBreakGEqual(a, b) ((void)0)
#define AssertBreakLEqual(a, b) ((void)0)
#endif

#endif // ASSERTS_H
