# STM32CubeMX2 command line (headless) - what the scripts use

CLI: `node <stm32cube>/bundles/stm32cubemx-application/<ver>/dist/bin.js <command>` with the node
bundled under `.../<ver>/bundles/node/<node-ver>/node/bin/node.exe`. Every command talks to a
backend over TCP (`--port`) and answers JSON `{"status": "success" | "error", ...}`. Each call
also prints a pack-licence notice line - drop it before parsing.

## Backend

```bash
node bin.js start --no-detached --port <p> --log-level warn [<project.ioc2>]
node bin.js ide-project status --port <p> --project-path <ioc2>      # readiness check
```
- No stop command. The backend is `stm32cubemx2.exe` (Electron) under the node launcher; stop it
  by killing the launcher's process tree (`taskkill //PID <winpid> //T //F`; the Windows PID of a
  Git Bash background job is `/proc/<pid>/winpid`). `env.sh` `mx_start` / `mx_stop` do this and
  pick a free port in 51240-51299, so a user's own STM32CubeMX2 session is never touched.
- Started without a project it answers `project create-from-board`; with a project it answers
  everything below.

## Commands used

| Step | Command |
| --- | --- |
| create | `project create-from-board --cpn <BOARD> --pack-version <v> --project-location <dir> --project-name <app>` → `<dir>/<app>.ioc2` (offline with local packs, < 1 s) |
| configure | `peripherals enable --project-path <ioc2> --peripheral USART2 --software-project <app> --mode Async`, then `project save` |
| inspect | `peripherals list`, `peripherals view --peripheral X` (modes), `pinout assigned-pins`, `project get-sw-project-list`, `hw-platform board-info` |
| generate | `ide-project generate --project-path <ioc2> --software-project <sw> --build-target .debug_GCC+<BOARD> --destination <dir> --format CMake --source include-packs-from-local --force --remove-board --diff overwrite` |

Facts found while porting (2026-10-07, MX 1.1.1):
- The board-default NUCLEO-C562RE project assigns PA2/PA3 to USART2, PA5 (LD1), PC13 (B1) and the
  SWD pins, but **leaves USART2 disabled**; enabling it needs mode `Async` (`Asynchronous` fails).
- `generate` stores the destination **as an absolute path** (`"outputPath"`) in the `.ioc2` it
  generates from → generate from a copy (`regen.sh`).
- The software project is named after the `.ioc2` file name; the ELF after the software project.
- No USART interrupt handler is generated for the default project, and no GPIO init for LD1/B1
  (the HAL GPIO module is compiled in, `USE_HAL_GPIO_MODULE 1`).
- The generated `CMakeLists.txt` has empty `target_*` blocks with `# Add additional ...` markers;
  `main.c` has `You can start your application code here` before `while (1) {}`. `overlay.py`
  relies on exactly these markers.
- `utilities/syscalls/syscalls.c` defines `_write` / `_read` weak - a strong `_write` in the app
  redirects `printf`.

## STM32CubeMX2 1.2.0 + FreeRTOS (2026-11-12)

Commands used by `env.sh` `mx_rtos_setup` (project open in the backend):

| Step | Command |
| --- | --- |
| add FreeRTOS | `composer add-component -p <ioc2> -s <sw> -c '<MX_RTOS_COMPONENT>'`, `composer resolve-dependencies`, `composer list-project-fixes` (must be `{}`), `composer get-rtos` (name FreeRTOS) |
| HAL tick timer | `peripherals enable --peripheral TIM6` (no mode) |
| HAL timebase | `sw-config set-parameter-value -r HALCore --parameter-path common/timebase_feature/hal_timebase_selection -v TIM`, then `.../timer_selection -v TIM6` (bare name; `{"selection":"TIM6"}` is refused) |
| kernel settings | `sw-config set-parameter-value -r FreeRTOS --parameter-path core/basic/kernel/fpu_mode -v true` (+ heap, stack check); list with `sw-config list-parameters -r FreeRTOS`, values with `get-parameter-possible-values` |
| install a pack | `pack-manager install --pack-ids STMicroelectronics.freertos.2.2.0` (network; the developer's step) |

Facts:
- `middleware get-available-middleware` reports a middleware `incompatible` while its pack is only
  metadata (`installed: false`), and `enable-middleware` then fails with "No compatible middleware"
  although its help says it downloads the pack. After `pack-manager install` it was `compatible` -
  **only in that backend session**: every later session reported FreeRTOS `incompatible` again
  (installed: true) and `enable-middleware` failed. `composer add-component` with the CMSIS id
  works in every session. (So "metadata only - incompatible" in a pack list says little about a
  board family; the WB55 note of 2026-10-27 may have the same cause - not re-checked.)
- HAL timebase choices (HAL2 C5 pack `stm32c5xx_hal_core_parameters.json`): SysTick, RTC Alarm,
  RTC Wakeup, Timer, Custom. "Timer" lists only peripherals of type `tim` that are **enabled**;
  LPTIM1 has type `lptim` and never appears. "Timer" without a timer selected still generates, but
  with broken aliases (`#define timebase_init mx__init`) that fail only at build time.
- The FreeRTOS config defaults `fpu_mode` to false (`configENABLE_FPU 0`) while the generated
  CMake builds with `-mfpu=fpv5-sp-d16 -mfloat-abi=hard` - tasks would share FPU registers unsaved.
- With `check_for_stack_overflow` 1 or 2, `generated/middleware/mx_freertos_app.c` defines
  `vApplicationStackOverflowHook` (endless loop); a second definition in `src/` fails to link.
- The generated `main.c` with FreeRTOS is unchanged: no `vTaskStartScheduler()` call -
  `app_main()` starts the scheduler. `mx_freertos_app.c` holds an empty `app_synctasks_init()`
  that nothing calls.
- `ide-project generate --diff overwrite` leaves unchanged files alone (old mtimes), so a file
  time does not tell whether `mx/` is current: `regen.sh` records the `.ioc2` sha256
  (`APP_IOC2_SHA` in `cubemx-app.env`) and `build.sh` compares it.
- MX 1.2.0 bundles node 24.22.1+st.1; create + configure + generate of a FreeRTOS app ~3.5 min.

## Generated tree (`mx/`)

`CMakeLists.txt`, `CMakePresets.json`, `cmake/` (toolchain, flags `-O0 -g3 -Wall`, files,
components), `main.c/h`, `generated/hal/` (`mx_system.c`, `mx_usart2.c`, `mx_rcc.c`,
`stm32c5xx_hal_conf.h`...), `stm32c5xx_dfp/`, `stm32c5xx_drivers/` (HAL2 + LL), `arch/cmsis`,
`utilities/syscalls`, `user_modifiable/` (startup, linker script). About 30 MB before building.
