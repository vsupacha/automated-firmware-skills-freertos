# Changelog

All notable changes to this repository. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versions follow [Semantic Versioning](https://semver.org/) and match `version` in
`.claude-plugin/marketplace.json`. Verification results per board are in each board profile's
verification log, not here.

## [Unreleased]

### Added
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
