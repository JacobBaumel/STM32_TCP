# STM32_TCP

This repository contains an example implementation of the LWIP TCP/IP stack with FreeRTOS using STM32CubeMX, for the
STM32H753ZI Nucleo.

## Extra Info

There are two primary things that need to be changed to create a working setup. The first is required, the second is
required only for release builds.

### DMA Accessible Placements

To create a working project, various things must be placed in DMA accessible (and noncacheable, if using caches)
regions. There are 4 primary variables that need to be placed manually:

- The LWIP receive pool
- The ethernet DMA TX/RX descriptors
- The LWIP RAM

The LWIP receive pool is a user-defined sized array for storing received data from the ethernet DMA. It is located in
`LWIP/Target/ethernetif.c`, and is defined with the line `LWIP_MEMPOOL_DECLARE`. It is sized approximately as 1568 (
sizeof one ethernet frame) * ETH_RX_BUFFER_CNT (currently 10, configed in MX), for a total of ~16KB. This is placed in
RAM via the linkerscript at 0x30000000.

The DMA descriptors also need to be placed in a noncachable region. These are each 24 bytes, and there are 4 of each.
Thus, the TX and RX buffers can be placed at 0x30003F00 and 0x30003E00, just after the receive pool. There is a
parameter for this in MX, however because we are using GCC this does not actually take any effect. You must manually
declare a section in the linkerscript at the appropriate location.

Finally, we must also place the LWIP RAM. This is a parameter in MX (LWIP -> Key Options -> LWIP_RAM_HEAP_POINTER). This
does also properly take effect, since instead of defining an array at that location, LWIP simply assigns a pointer that
value (see `Middlewares/Third_Party/LwIP/src/core/mem.c` line 524). However, to make sure the compiler does not place
other data in the LWIP RAM (since there is nothing currently telling it to reserve that region), this project also
allocates a uint8_t array at the appropriate location (using the linkerscript) to block anything else from being placed
there.

If using caches, all of these regions need to also be marked noncacheable in the MPU. This can be achieved with either a
single 64K region starting at 0x30000000, or one 32K at 0x30000000 and one 16K at 0x30004000 for tighter bounds. The
region needs to be set as:

- TEX 1
- ALL ACCESS PERMITTED
- No instruction access
- Shareability Enabled
- Noncacheable
- Nonbufferable

These should be the only modifications required (linkerscript + MPU).

### Release Builds

In order for release-optimized builds to work, we need to replace the default memcpy used within LWIP with a different
version that will not be intrinsic optimized out by the compiler. "Intrinsics" are a particular class of code
optimizations applied in the most extreme settings, where common functions are replaced with more efficient assembly
versions that skip things like stack setup/teardown. This applies for the function `memcpy`, which copies a given number
of bytes from one memory location to another. For small numbers of bytes to be copied, GCC will actually replace the
call to memcpy with a few lines of assembly that perform the copy instead.

However, whatever intrinsic version GCC uses for some reason breaks whatever LWIP is doing, and this only applies in
release mode because when producing debug builds we disable intrinsic optimizations. To get around this, we have a
special, nonintrinsic memcpy function in`Core/Src/nonintrinsic_memcpy.c` which is pretty much just the normal memcpy,
but under a different name (its a simplified version of the newlib memcpy implementation). Then, we can also define in
`LWIP/Target/lwipopts.h` the `SMEMCPY` macro to override what function LWIP uses for memcpy, and force it to use our own
custom version. This prevents the intrinsic optimization, and prevents hardfaults in release builds. 