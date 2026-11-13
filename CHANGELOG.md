# Changelog

All notable changes to this repository. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versions follow [Semantic Versioning](https://semver.org/) and match `version` in
`.claude-plugin/marketplace.json`. Verification results per board are in each board profile's
verification log, not here.

## [Unreleased]

### Fixed
- 2026-11-12: FreeRTOS board layers (L476, F3, C5): console lines of two tasks could mix
  (`TASK name=BBLINK led1=1` on the L476) - stdout is unbuffered and the mutex covered single
  `_write()` pieces. `board.c` now collects each task's output in a per-task line slot and sends
  whole lines under the mutex. Re-tested on the L476 only; F3 and C5 built, not re-run on the board.

### Added
- 2026-11-12: board skill `cubemx-freertos-stm32l476disco` (ST STM32L476G-DISCO / 32L476GDISCOVERY, STM32L476VG): classic
  STM32CubeMX 6.18.1 + STM32CubeL4 1.18.2 with FreeRTOS V10.3.1 from the firmware package, the same
  FreeRTOS `MX_CONFIG` as the F3 skill plus USART2 on the ST-LINK virtual COM port. Scripts and board
  layer from the bare-metal skill `cubemx-hal-stm32l476disco` ([automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills/blob/main/skills/cubemx-hal-stm32l476disco/SKILL.md)).
  All four templates build with 0 warnings; blink-rate tested on the board (12/12, tasks/heap report).
- 2026-11-12: board skill `cubemx-freertos-stm32f303disco` (ST STM32F3DISCOVERY, STM32F303VC): classic STM32CubeMX
  6.18.1 + STM32CubeF3 1.11.6 with FreeRTOS V10.0.1 from the firmware package. `MX_CONFIG` adds the
  HAL timebase TIM6, FreeRTOS (CMSIS-RTOS v2 wrapper; the apps use the native API), a 16 KB heap,
  stack-overflow check 2 and high-water marks; `rtos_gate` checks the generated project; the overlay
  makes the empty generated stack-overflow hook stop. Scripts and board layer from the bare-metal
  skill `cubemx-hal-stm32f303disco` ([automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills/blob/main/skills/cubemx-hal-stm32f303disco/SKILL.md)),
  templates and `app_rtos.c/.h` from `cubemx2-freertos-stm32c562nucleo`. Console SWO only. All four
  templates build with 0 warnings; blink-rate observed on the board (LD3 steps 1 s / 0.5 s / 0.2 s with B1).
- 2026-11-12: repository split from [automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills)
  (bare-metal skills stay there). Copies, no shared code: `lib/` (common.sh, fwtest.py, fwdebug.py,
  fwpaths.py, fw_workspace.sh, check_layers.py, host_test.sh + host/, func/), `tools/`
  (validate_skills.py, board_status.py), `docs/workflow.md` and `docs/board-api.md`, the
  NUCLEO-C562RE hardware sheet, and the tool skill `cleanroom-test` with the FreeRTOS prompt and report.
- Board skill `cubemx2-freertos-stm32c562nucleo` - the FreeRTOS execution model of the
  NUCLEO-C562RE (own scripts, templates and profile `boards/nucleo-c562re/cubemx2-freertos-stm32c562nucleo/`;
  same board id, bench file, identity and console contract as the bare-metal skill). Start point:
  the clean-room app `rtos-clean-test`. `new_app.sh` adds ST's FreeRTOS middleware (pack 2.2.0,
  kernel 11.3.1) with `composer add-component`, moves the HAL tick to TIM6, turns on FPU context
  save, a 16 KB heap and stack-overflow check 2 (`env.sh` `mx_rtos_setup`, every value read back);
  `regen.sh` refuses a generated project without FreeRTOS, FPU support or a TIM timebase (`RTOS:`
  line). RTOS-aware board layer (`vTaskDelay`, console mutex, RX by interrupt), execution helpers
  `app_rtos.c/.h` (task registry, console task, `tasks` command with free stack and heap), the three
  bring-up templates as tasks with the bare-metal test specs plus a `tasks` step, and the template
  `blink-rate` (buttonTask samples B1 every 100 ms -> task notification -> ledTask steps 1 s / 0.5 s
  / 0.2 s; rules in `func/blink_rate.c`). blink-rate automatic steps tested on the board (12/12).
- `lib/check_layers.py`: RTOS kernel headers (`FreeRTOS.h`, `task.h`, `queue.h` ...) are allowed in
  the execution layer and board/, and fail in `func/`.
