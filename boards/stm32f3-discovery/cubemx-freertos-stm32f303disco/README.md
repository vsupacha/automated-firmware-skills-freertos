# STM32F3DISCOVERY - cubemx-freertos-stm32f303disco profile

Board profile of the `cubemx-freertos-stm32f303disco` skill (FreeRTOS execution model): `board.env` (pinned STM32CubeMX,
firmware package and bundles, `MX_START`, `MX_CONFIG` incl. FreeRTOS, identity gate). Hardware:
[../README.md](../README.md). The bare-metal profile of the same board:
[cubemx-hal-stm32f303disco](https://github.com/vsupacha/automated-firmware-skills/blob/main/boards/stm32f3-discovery/cubemx-hal-stm32f303disco/README.md) in
automated-firmware-skills (same bench file and ST-LINK identity).

| Item | Value |
| --- | --- |
| STM32CubeMX | 6.18.1 (classic), headless script mode through its bundled java |
| Firmware package | STM32Cube_FW_F3 V1.11.6 (HAL, CMSIS, BSP STM32F3-Discovery, FreeRTOS V10.0.1 + CMSIS-RTOS v2 wrapper) |
| Starting `.ioc` | `loadboard STM32F3DISCOVERY nomode` (pins, LD3-LD10, B1, SWD, clocks 48 MHz) |
| `MX_CONFIG` | `set mode SYS Trace_Asynchronous_SW` (SWO), `set mode SYS TIM6` (HAL timebase), `set mode FREERTOS CMSIS_V2`, `set ip parameters FREERTOS configTOTAL_HEAP_SIZE 16384`, `... configCHECK_FOR_STACK_OVERFLOW 2`, `... INCLUDE_uxTaskGetStackHighWaterMark 1` |
| Generated RTOS files | `mx/Inc/FreeRTOSConfig.h`, `mx/Src/freertos.c` (stack overflow hook: the overlay makes it stop), `mx/Src/stm32f3xx_hal_timebase_tim.c`, port `GCC/ARM_CM4F`, heap_4 |
| Kernel start | `app_main()` (USER CODE 2) creates the tasks and calls `vTaskStartScheduler()`; the generated `osKernelInitialize()` / default task after it are never reached |
| Build | CMake 4.4.0+st.1 + Ninja 1.13.2+st.1 + GNU Tools for STM32 14.3.1+st.2, preset `Debug` |
| Console | SWO output only (ITM port 0, mutex in `_write()`), no console input |

## Tool quirks

- `get mode all` lists no middleware, but `set mode FREERTOS CMSIS_V2` (or `CMSIS_V1`) answers OK
  and `set ip parameters FREERTOS <config> <value>` sets kernel values (read back in the generated
  `FreeRTOSConfig.h`).
- The generated `vApplicationStackOverflowHook()` has an empty body (USER CODE 4) - a task would run
  on after an overflow; the overlay fills it with "interrupts off, endless loop".

## Verification log

- 2026-11-12 skill `cubemx-freertos-stm32f303disco` created (owner's names) from `cubemx-hal-stm32f303disco` (scripts, board
  layer) + `cubemx2-freertos-stm32c562nucleo` (templates, `app_rtos.c/.h`, `func/blink_rate`). Home PC.
  - check_tools `missing/bad=0`. Create (~1 min each) with `RTOS: FreeRTOS V10.0.1, HAL timebase
    TIM6, heap 16384 B, stack check 2`; BUILD PASS 0 warnings: `blink-rate` (app `rr-f3`, ELF
    `057ed571...`), `hello-world` (`rh-f3`, `61000b90...`), `blink` (`rb-f3`, `2d3a95dc...`),
    `push-to-light` (`rp-f3`, `9b6df093...`).
  - `blink-rate` flashed with the owner's OK (identity `0x422`): `FLASH: PASS`. The owner saw LD3
    blink once a second and step to 0.5 s / 0.2 s / 1 s with each B1 press - both tasks and the
    task notification work: **observed**. No SWO reader, so no automatic test.
