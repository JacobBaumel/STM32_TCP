# STM32_TCP

This repository contains an example implementation of the LWIP TCP/IP stack with FreeRTOS using STM32CubeMX, for the STM32H753ZI Nucleo. 

## Extra Info

The default cubemx setup will actually work just fine with caches disabled. The more difficult part of ethernet on STM is when you enable caches and need to begin worrying about cache coherency. There are 4 primary variables that need to be placed in non-cacheable regions:
 - The LWIP receive pool
 - The ethernet DMA TX/RX descriptors
 - The LWIP RAM

The LWIP receive pool is a user-defined sized array for storing received data from the ethernet DMA. It is located in `LWIP/Target/ethernetif.c`, and is defined with the line `LWIP_MEMPOOL_DECLARE`. It is sized approximately as 1568 (sizeof one ethernet frame) * ETH_RX_BUFFER_CNT (currently 10, configed in MX), for a total of ~16KB. This is placed in RAM via the linkerscript at 0x30000000. 

The DMA descriptors also need to be placed in a noncachable region. These are each 24 bytes, and there are 4 of each. Thus, the TX and RX buffers can be placed at 0x30003F00 and 0x30003E00, just after the receive pool. There is a parameter for this in MX, however because we are using GCC this does not actually take any effect. You must manually declare a section in the linkerscript at the appropriate location.

Finally, we must also place the LWIP RAM. This is a parameter in MX (LWIP -> Key Options -> LWIP_RAM_HEAP_POINTER). This does also properly take effect, since instead of defining an array at that location, LWIP simply assigns a pointer that value (see `Middlewares/Third_Party/LwIP/src/core/mem.c` line 524). However, to make sure the compiler does not place other data in the LWIP RAM (since there is nothing currently telling it to reserve that region), this project also allocates a uint8_t array at the appropriate location (using the linkerscript) to block anything else from being placed there. 

All of these regions need to be noncacheable. This can be achieved with either a single 64K region starting at 0x30000000, or one 32K at 0x30000000 and one 16K at 0x30004000 for tighter bounds. The region needs to be set as:
 - TEX 1
 - ALL ACCESS PERMITTED
 - No instruction access
 - Shareability Enabled
 - Noncacheable
 - Nonbufferable

These should be the only modifications required (linkerscript + MPU).