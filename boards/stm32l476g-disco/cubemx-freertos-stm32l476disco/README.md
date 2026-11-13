# STM32L476G-DISCO - cubemx-freertos-stm32l476disco profile

Board profile of the `cubemx-freertos-stm32l476disco` skill (FreeRTOS execution model): `board.env` (pinned STM32CubeMX,
firmware package and bundles, `MX_START`, `MX_CONFIG` incl. FreeRTOS, identity gate). Hardware:
[../README.md](../README.md). The bare-metal profile of the same board:
[cubemx-hal-stm32l476disco](https://github.com/vsupacha/automated-firmware-skills/blob/main/boards/stm32l476g-disco/cubemx-hal-stm32l476disco/README.md) in
automated-firmware-skills (same bench file and ST-LINK identity).

| Item | Value |
| --- | --- |
| STM32CubeMX | 6.18.1 (classic), headless script mode through its bundled java |
| Firmware package | STM32Cube_FW_L4 V1.18.2 (HAL, CMSIS, BSP STM32L476G-Discovery, FreeRTOS V10.3.1 + CMSIS-RTOS v2 wrapper) |
| Starting `.ioc` | `loadboard STM32L476G-DISCO nomode` (pins, joystick, LEDs, SWD; MSI 4 MHz) |
| `MX_CONFIG` | `set mode USART2 Asynchronous`, `set pin PD5 USART2_TX`, `set pin PD6 USART2_RX` (ST-LINK VCP), `set mode SYS TIM6` (HAL timebase), `set mode FREERTOS CMSIS_V2`, `set ip parameters FREERTOS configTOTAL_HEAP_SIZE 16384`, `... configCHECK_FOR_STACK_OVERFLOW 2`, `... INCLUDE_uxTaskGetStackHighWaterMark 1` |
| Generated RTOS files | `mx/Inc/FreeRTOSConfig.h`, `mx/Src/freertos.c` (stack overflow hook: the overlay makes it stop), `mx/Src/stm32l4xx_hal_timebase_tim.c`, port `GCC/ARM_CM4F`, heap_4 |
| Kernel start | `app_main()` (USER CODE 2) creates the tasks and calls `vTaskStartScheduler()`; the generated `osKernelInitialize()` / default task after it are never reached |
| Build | CMake 4.4.0+st.1 + Ninja 1.13.2+st.1 + GNU Tools for STM32 14.3.1+st.2, preset `Debug` |
| Console | USART2 on the ST-LINK/V2-1 virtual COM port (`huart2`), RX by interrupt; TX: whole lines per task (line slots + mutex in `_write()`) |

## Tool quirks

- `get mode all` lists no middleware, but `set mode FREERTOS CMSIS_V2` answers OK and
  `set ip parameters FREERTOS <config> <value>` sets kernel values (read back in the generated
  `FreeRTOSConfig.h`). CubeMX logs a harmless "Missing modes for component ... FreeRTOS" warning.
- The generated `vApplicationStackOverflowHook()` has an empty body (USER CODE 4); the overlay fills
  it with "interrupts off, endless loop".

## Verification log

- 2026-11-12 skill `cubemx-freertos-stm32l476disco` created (owner's names) from `cubemx-hal-stm32l476disco` (scripts, board layer) +
  `cubemx2-freertos-stm32c562nucleo` (templates, `func/blink_rate`) + `cubemx-freertos-stm32f303disco`
  (`app_rtos.c/.h`, `rtos_gate`, overlay hook). Home PC.
  - check_tools `missing/bad=0`. Create with `RTOS: FreeRTOS V10.3.1, HAL timebase TIM6, heap 16384 B,
    stack check 2`; BUILD PASS 0 warnings: `blink-rate` (app `rr-l476`), `hello-world` (`rh-l476`, ELF
    `1899e075...`), `blink` (`rb-l476`, `be9f4a4c...`), `push-to-light` (`rp-l476`, `5023dd1c...`).
  - `blink-rate` flashed with the owner's OK (ELF `eb1f1541...`): `FLASH: PASS`; serial test
    **RESULT: FAIL (11/12)** - the `tasks` report lost a line: `TASK name=BBLINK led1=1` (the ledTask's
    BLINK line landed inside the buttonTask line - stdout is unbuffered, the mutex covered single
    `_write()` pieces only). Fix: per-task line slots in `board.c` (whole lines under the mutex).
  - Fixed `blink-rate` (ELF `e46fbac7...`) flashed with the owner's OK: `FLASH: PASS`; serial test
    **RESULT: PASS (12/12, 3 interactive skipped)** on two runs. `tasks`: ledTask 381, buttonTask 476,
    console 353 words free; `HEAP free=9808 min=9808 total=16384 tasks=5`. The interactive joystick
    steps were not run. `debug.sh status` before the fix: running, fault none.
  - The line-slot fix was ported to `cubemx-freertos-stm32f303disco` and `cubemx2-freertos-stm32c562nucleo`
    (built, not re-run on those boards).
