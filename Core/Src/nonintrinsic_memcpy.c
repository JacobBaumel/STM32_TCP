#include <stddef.h>

/*
 * LWIP relies on memcpy for a variety of things, and it specifically needs a non-intrinsic version of memcpy
 * in order to function properly. However, in some of its usages of LWIP, it copies a fairly small number of
 * bytes (<= 4), in which case GCC replaces the memcpy call with an intrinsic version for optimization, but
 * only in release builds (this optimization is not taken in debug builds). You can prove this by running a
 * release build without this nonintrinsic version (uncomment SMEMCPY macro in lwipopts), and see how the
 * processor hardfaults. Then, you can try another relase build with -fno-builtins and observe it now works.
 *
 * To get around this, we create a nearly identical memcpy that the compiler won't optimize out, and use this
 * for LWIP's memcpy implementation. This is probably just as fast as the default implementation anyways, at
 * least as compared to newlib's implementation.
 */
void* nonintrinsic_memcpy(void* dest, const void* src, size_t bytes) {
    char* cdest = dest;
    const char* csrc = src;
    while(bytes--) *cdest++ = *csrc++;
    return dest;
}
