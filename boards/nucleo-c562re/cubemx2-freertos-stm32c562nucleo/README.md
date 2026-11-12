# NUCLEO-C562RE - STM32CubeMX2 + FreeRTOS profile (cubemx2-freertos-stm32c562nucleo)

Board hardware (tool-independent): [../README.md](../README.md). This file: toolchain facts, tool
quirks and the verification log of the `cubemx2-freertos-stm32c562nucleo` skill. Machine-readable
values: `board.env`. The bare-metal profile of the same board:
[cubemx2-hal2-stm32c562nucleo](https://github.com/vsupacha/automated-firmware-skills/blob/main/boards/nucleo-c562re/cubemx2-hal2-stm32c562nucleo/README.md) in automated-firmware-skills (same bench
file, ST-LINK identity and console).

## Toolchain

| Item | Value |
| --- | --- |
| STM32CubeMX2 | 1.2.0, headless CLI (`node <mx>/dist/bin.js`, bundled node 24.22.1+st.1) |
| Board project | `mx project create-from-board --cpn NUCLEO-C562RE --pack-version 2.1.0`, `peripherals enable USART2 --mode Async` |
| FreeRTOS | pack `freertos` 2.2.0 (kernel V11.3.1, port GCC/ARM_CM33_NTZ, Heap_4), added with `composer add-component` |
| HAL timebase | TIM6 (FreeRTOS owns SysTick; 1000 Hz kernel tick) |
| Kernel settings | `fpu_mode` true, heap 16384 B, stack overflow check 2 (`MX_SWCONFIG`) |
| Packs | stm32c5xx_dfp 2.1.0, stm32c5xx_hal_drivers 2.1.0 (HAL2), templates_no_os_stm32c5 2.1.0, nucleo-c562re_hw-board 2.1.0, freertos 2.2.0 |
| Build | CMake 4.4.0+st.1 + Ninja 1.13.2+st.1 + GNU Tools for STM32 14.3.1+st.2, preset `debug_GCC_NUCLEO-C562RE`, `-mfpu=fpv5-sp-d16 -mfloat-abi=hard` |
| Flash | STM32CubeProgrammer CLI 2.23.0 (same as the bare-metal profile) |

## Tool quirks

- MX 1.2.0 rates FreeRTOS "incompatible" in `middleware get-available-middleware` except in the
  backend session that installed its pack, so `enable-middleware` is unreliable; the skill uses
  `composer add-component` with the CMSIS id (`MX_RTOS_COMPONENT`).
- HAL2's HAL timebase "Timer" lists only enabled TIM instances (no LPTIM): TIM6 is enabled first.
- FreeRTOS `fpu_mode` defaults to false although the build is hard-float - the skill sets it.
- The generated `mx_freertos_app.c` defines `vApplicationStackOverflowHook` when the stack check is
  on - not in `src/`.

## Verification log

- 2026-11-12 start point: clean-room app `rtos-clean-test` (MX 1.2.0, push-to-light + FreeRTOS
  enabled by the clean-room agent, HAL timebase moved to TIM6 by the CLI at the owner's request):
  REGEN PASS, build PASS 0 warnings (superloop code, scheduler not started). The clean-room run was
  stopped by the owner before its report (FreeRTOS configured by hand instead).
- 2026-11-12 skill `cubemx2-freertos-stm32c562nucleo` created from it (MX 1.2.0, packs as above,
  GCC 14.3.1+st.2, Windows 10 + Git Bash). `check_tools` 0 bad (freertos pack present).
  `blink-rate` (app `rt-rate`): create + FreeRTOS + TIM6 + kernel settings + generate 210 s,
  `RTOS: FreeRTOS V11.3.1, HAL timebase TIM6`, generated `configENABLE_FPU 1`, heap 16384, stack
  check 2; BUILD PASS 0 warnings (text 38,720 B, bss 19,504 B), ELF `4480ab08...`.
- 2026-11-12 `blink-rate` on the board (owner approved each flash): discover IDENTITY PASS (ST-LINK
  V3J16M9, NUCLEO-C562RE, STM32C5x, 0x44E). First image FLASH PASS, `BLINK` lines on the console and `debug.sh status`
  said running / fault none, but the test never synced: `info` arrived as `i` - the console task
  polled the 1-byte USART receive flag every 10 ms. Fix in `board.c`: RX by interrupt
  (`USART2_IRQHandler`, priority 6) into a 128-byte ring. Second image ELF `0b85fb19...` (text
  39,104 B): FLASH PASS, **RESULT: PASS 12/12** (3 interactive B1 steps skipped - not run):
  info `rtos=FreeRTOS V11.3.1`; `tasks`: ledTask prio 2 stack_free 379 words, buttonTask prio 3
  474, console prio 1 350 (of 512 each), heap free 8272 of 16384, 5 kernel tasks; `step` x3 ->
  `PERIOD ms=500 / 200 / 1000`, BLINK lines between (intervals not measured by the runner).
- 2026-11-12 `hello-world` (app `rt-hw`, ELF `b3a27adb...`) and `blink` (app `rt-bl`, ELF
  `de0a6fe5...`): create 3-4 min each, BUILD PASS 0 warnings with the RX fix. Not flashed.
- 2026-11-12 `push-to-light` (app `rt-pt`, ELF `10eaffa6...`, text 40,400 B): BUILD PASS 0
  warnings. Not flashed. `lib/check_layers.py` (M2 gate bypassed for this one run): `rt-rate` and
  `rt-pt` LAYERS PASS; a `FreeRTOS.h` include planted in `func/` FAILs as intended.
- 2026-11-12 `blink-rate` interactive steps (`--only-interactive`, image `0b85fb19...`): **RESULT: FAIL
  2/5** - sync + board id PASS, the three B1 steps got no `EVT btn1=pressed` / `PERIOD` in 3 x 30 s;
  89 `BLINK` lines (1 s interval) = firmware alive. Not known whether B1 was pressed in the window
  (the prompts are not shown live); the 100 ms sampled B1 path is untested. Next: owner presses B1
  while a read-only check watches (or `btn?`-style diagnostic, needs a flash), then re-run.
