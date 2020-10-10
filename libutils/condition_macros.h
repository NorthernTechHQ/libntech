#ifndef __CONDITION_MACROS_H__
#define __CONDITION_MACROS_H__

#include <assert.h>
#include <stdbool.h>

// This file contains macros to use for error checking, assertions,
// abort, return, etc. Each macro should have a comment about when to use it.
// The normal assert() macro should only be used to catch programmer mistakes,
// things which should never happen, even for weird file/network inputs.
// For example, a function which doesn't accept NULL pointer arguments,
// should assert that the parameter is not NULL.

// Used when you want to assert a precondtion (catch programmer mistake)
// but also want to handle the error if it ever happens in a release build.
// Useful if you are unsure if you need to handle the error, for example
// if you are adding assertions to older code. Also useful in cases where
// you know you want both, for example when network or file input can trigger
// the condition
#ifndef assert_or_return
#define assert_or_return(expr, val) { \
    assert(expr);                     \
    if (!(expr))                      \
    {                                 \
        return val;                   \
    }                                 \
}
#endif

// Similar to assert_or_return, except you put it inside the if
// body which handles the error in release builds
#ifndef debug_abort_if_reached
#define debug_abort_if_reached() { \
    assert(false);                 \
}
#endif

// libntech static assert (compile time check):
// has nt_ prefix to not be confused with static_assert
// Why not just static_assert?
// - It is already defined on some platforms (recent RHEL and Ubuntu),
//   but in incompatible ways. On RHEL second arg (message) is required,
//   on Ubuntu it is not.
// Why not redefine static_assert (undef + define)?
// - Hard to know which static_assert you are using. Include ordering becomes
//   very important. Error messages are confusing if you end up using the wrong
//   static_assert. Error message would only appear on some platforms, not others.

// #ifdef _Static_assert
// // Note: The message here is not really necessary,
// //       most compilers will include this information anyway.
// #define nt_static_assert(x) _Static_assert(x, __FILE__ ":" TO_STRING(__LINE__) ": (" #x ") -> false")
// #else
#define nt_static_assert(x) {                               \
    switch (0) {                                            \
    case 0: /* Cause duplicate case if next is 0 as well */ \
        break;                                              \
    case x: /* Error if 0 or if non-const expression */     \
        break;                                              \
    }                                                       \
}
// #endif

#endif
