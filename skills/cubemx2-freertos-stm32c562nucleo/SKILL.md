---
name: cubemx2-freertos-stm32c562nucleo
description: FreeRTOS firmware workflow for STM32C5 boards (NUCLEO-C562RE) with the STM32CubeMX2 CLI on Windows - check the pinned STM32Cube bundles and packs (incl. ST's FreeRTOS pack), create the .ioc2 from the board with FreeRTOS middleware and the HAL tick on TIM6, generate the CMake project headlessly, write layered code (board -> func -> tasks), build with the pinned GCC/CMake/Ninja, flash with STM32CubeProgrammer behind ST-LINK identity gates, run UART tests incl. a task/stack/heap report, regenerate after .ioc2 changes, clean. Use when asked for FreeRTOS / RTOS tasks on STM32C5 / NUCLEO-C562RE with STM32CubeMX2 (create, build, flash (แฟลช), test), to start from a FreeRTOS .ioc2, or for this skill's help (ขอวิธีใช้, มีคำสั่งอะไรบ้าง). Bare-metal (superloop) apps on the same board use cubemx2-hal2-stm32c562nucleo. Not for classic STM32CubeMX (.ioc) projects or other STM32 families.
license: MIT
compatibility: Claude Code or another agent that runs bash; Windows 10/11 + Git Bash, Python 3 + pyserial; needs STM32CubeMX2 + STM32Cube for VS Code bundles (GCC, CMake, Ninja, STM32CubeProgrammer); uses this repo's lib/ and boards/ (install the whole plugin or clone the repo)
---

# STM32C5 firmware workflow with FreeRTOS (STM32CubeMX2)

The FreeRTOS execution model of the NUCLEO-C562RE, kept apart from the bare-metal skill
`cubemx2-hal2-stm32c562nucleo` (own scripts, templates and board profile; same board id, bench file
and console contract, so the same test specs pass under both). Every app is created with ST's
FreeRTOS middleware from STM32CubeMX2 and the HAL tick on TIM6 (FreeRTOS owns SysTick); its code
runs in tasks. Start point: the clean-room app `rtos-clean-test` (2026-11-12, STM32CubeMX2 1.2.0).

Six gated stages plus a clean step, the same contract as every skill in this repo
(`docs/workflow.md`). Do them in order; do not start a stage until the previous gate passed.
**Windows + Git Bash only** (STM32Cube bundles for Windows, `taskkill`, `cygpath`).

```
1 tools ─► 2 discover + project ─► 3 code ─► 4 build ─► 5 flash ─► 6 test ─► 7 clean
check_tools  discover (ST-LINK)    src/board  build.sh   flash.sh   serial_test  clean.sh
             new_app / regen       src/func
                                   app_main.c
```

**Two paths after Stage 2** (`docs/workflow.md`): the developer may code in VS Code at any
time - the **IDE path** (`new_app.sh` already opened the app; `open_ide.sh <app>` reopens it; they
build, flash and debug with the vendor extension's buttons) - or let you run the stages - the
**script path** (build, layer checks, flash, tests behind their gates). Ask which one when it is
not clear. On the IDE path, stop after stage 2d and run scripts only when asked ("build it", "test
it"); never overwrite files the developer is editing. Both paths share the app folder and build
outputs, so switching is always possible: on the script path re-run the gates from Stage 3.

## Milestones, stage numbers and developer actions

This skill implements the shared contract in `docs/workflow.md` (repo root):

| Milestone | Shared stage | In this file |
| --- | --- | --- |
| M1 bring-up | 1 setup, 2 create, 2d IDE, 3 build | Stage 1, Stage 2 step 2 (+ `regen.sh`), Stage 3 |
| M1 bring-up (board) | 4 connect, 5 flash, 6 test, 7 debug | Stage 4 (discover), Stage 5, Stage 6, Stage 7 |
| M2 board support | 8 board drivers *(planned)*, 9 layer check, 10 host test *(optional)* | Stage 2e (rules, shared `lib/func`, `check_layers.py`, `host_test.sh`) |
| M3 execution model | 11 execution (FreeRTOS tasks: this skill), trace *(planned)* | "FreeRTOS execution model", Stage 2e, `tasks` in Stage 6 |
| M4 components, M5 application | 12 components, 13 profile *(planned)* | - |
| - | 14 export *(planned)*, 15 port *(planned)* | Extending |
| - | 16 clean | Stage 16 |

**Exit codes:** 0 gate passed, 1 failed, 2 warnings, **10 = developer action needed**. On exit 10
the script's last line is `ACTION: <TYPE> <what to do>` (SETUP, CONNECT, POWER_CYCLE, JUMPER,
APPROVE, CHOOSE): tell the user exactly that, wait for their OK, then re-run the same command
(with `--yes` / `--serial` when the action says so). Never work around an ACTION yourself -
installing tools, accepting licences, changing drivers/system settings, plugging hardware and
approving flashes or deletions are the developer's.

**Release scope:** `<repo>/milestones.env` lists the active milestones (this release: **M1**).
M1 includes the board stages: discover, flash and serial tests run when the developer asks for
them, and **every flash needs the developer's yes first** (`flash.sh --yes` only after they said
yes in chat, or `FLASH_POLICY=auto` that they wrote into their own bench file). Scripts of an
inactive milestone (e.g. M2 `check_layers.py`) stop with `ACTION: SETUP ... not active`; never
edit `milestones.env` or set `FW_ACTIVE_MILESTONES` yourself - relay the ACTION like any other.

## Help menu (answer this first when asked how to use the skill)

When the user asks for help, usage or the command list ("ขอวิธีใช้หน่อย", "มีคำสั่งอะไรบ้าง", "help"),
run `bash "$SKILL/scripts/help.sh"` (Thai) or `help.sh --en`, show the output as-is, then offer the
next step (usually Stage 1). Do not run other stages.

`$SKILL` below = the folder containing this SKILL.md. Always quote: `bash "$SKILL/scripts/x.sh"`.
Repo layout this skill needs:

```
<repo>/skills/cubemx2-freertos-stm32c562nucleo/        this skill (SKILL.md, scripts/, reference/, templates/)
<repo>/boards/<id>/README.md          board hardware (tool-independent)
<repo>/boards/<id>/cubemx2-freertos-stm32c562nucleo/    this skill's board profile: board.env, README (verification log)
<repo>/apps/                          apps workspace when working inside the repo
```

Workspace `$CUBE_WS`: `<repo>/apps` inside the repo checkout, otherwise `./apps`. Board profiles:
`<project>/boards/<id>/cubemx2-freertos-stm32c562nucleo/` first, then `<repo>/boards` (override `CUBE_BOARDS_DIR`).
STM32Cube bundles + packs: `%LOCALAPPDATA%\stm32cube` (override `STM32CUBE_ROOT`).

## What an app looks like

```
apps/<app>/
  <app>.ioc2        STM32CubeMX2 hardware configuration - the source of truth (commit it)
  src/              our layered code: board/, func/, app_main.c (commit it)
  tests/<spec>.json test contract (commit it)
  cubemx-app.env    record: board, template, how the .ioc2 was made, software project, preset
  mx/               generated CMake project (CubeMX-owned, rewritten by regen.sh; git-ignored)
  .mx/              working copy of the .ioc2 + backend logs (git-ignored)
  logs/             build/flash/test logs (git-ignored)
  <app>.code-workspace  VS Code workspace (mx/, src/, tests/) - open_ide.sh makes it (git-ignored)
```
`src/` holds `app_main.c` (the tasks of this app), `app_rtos.c/.h` (shared task helpers),
`board/` and `func/`.

**Is a .ioc2 file needed?** Yes - every app has one, but nobody has to supply it: `new_app.sh`
creates it from the board (`mx project create-from-board`, pinned board pack), applies the
profile's `MX_CONFIG` (e.g. USART2 Async) and the FreeRTOS execution model (next section). To start
from a user's own hardware configuration, pass `--ioc2 <file>` - it must already hold FreeRTOS and
a TIM HAL timebase (`regen.sh` stops otherwise). Change hardware with the STM32CubeMX2 GUI or CLI on
`<app>.ioc2`, then `regen.sh`.

## FreeRTOS execution model

What `new_app.sh` puts into every `.ioc2` (profile keys in `board.env`, applied by
`mx_rtos_setup` in `scripts/env.sh` with the STM32CubeMX2 CLI, each value read back):

| Setting | Value | Why |
| --- | --- | --- |
| FreeRTOS middleware | pack `freertos` 2.2.0 (kernel 11.3.1, Heap_4), `MX_RTOS_COMPONENT` | added with `composer add-component` + `resolve-dependencies`; `middleware enable-middleware` rates FreeRTOS "incompatible" in every session except the one that installed the pack (MX 1.2.0) |
| HAL timebase | Timer, TIM6 (`MX_TIMEBASE_TIM`) | FreeRTOS uses SysTick for its 1 ms tick. HAL2 offers only TIM instances **already enabled** - LPTIM is not offered at all - so TIM6 is enabled first |
| `fpu_mode` | true (`configENABLE_FPU 1`) | the build is `-mfloat-abi=hard`; with CubeMX2's default `false` the kernel does not save FPU registers per task |
| heap | 16384 bytes (`configTOTAL_HEAP_SIZE`) | default 8192 is too small for three printf tasks of 512 words |
| stack overflow check | 2 | the generated `mx_freertos_app.c` defines `vApplicationStackOverflowHook` (stops in a loop) - do not define it in `src/` |

`regen.sh` checks the generated code for all three essentials (FreeRTOSConfig.h present,
`configENABLE_FPU 1`, `timebase_init` = `mx_tim<N>_init`) and prints `RTOS: FreeRTOS V<x>, HAL
timebase TIM<N>` before `REGEN: PASS`. In the GUI: Middleware > FreeRTOS; HAL Core > HAL timebase =
Timer + Timer selection (enable the TIM in the peripherals view first); FreeRTOS > kernel > FPU.

## Board type vs bench instance

- **Board type** (`boards/<id>/`): `README.md` = hardware; `cubemx2-freertos-stm32c562nucleo/board.env` = board CPN,
  board pack version, `MX_CONFIG`, pinned bundle versions, packs, `BOARD_DEFINE`, `BTN_LABELS`,
  expected ST-LINK board / device name / device ID.
- **Bench instance** (`<workspace>/.bench/<id>.env`, written by `discover.sh`): `PROBE_SERIAL`
  (ST-LINK) and `CONSOLE_PORT` (its virtual COM port - the VCP's USB serial is the probe serial).
  Override with `CUBE_PROBE_SERIAL` / `CUBE_CONSOLE`. Never write them into `boards/`.

| id | board | status |
| --- | --- | --- |
| `nucleo-c562re` | ST NUCLEO-C562RE (STM32C562RET6, STLINK-V3EC) | see `boards/nucleo-c562re/cubemx2-freertos-stm32c562nucleo/README.md` |

## Hardware quick reference

| Board | Button | LED | Console |
| --- | --- | --- | --- |
| NUCLEO-C562RE | **B1** (USER, blue) PC13, pull-down, **pressed = high**; B2 = RESET | **LD1** green PA5, active high | USART2 PA2/PA3 → ST-LINK VCP, 115200 |

## Stage 1 - Tools and folder paths

```bash
bash "$SKILL/scripts/check_tools.sh" <board-id> [--ws <workspace>]
```
Checks the workspace path (no spaces/non-ASCII, ≤100 characters), every **pinned** bundle
(STM32CubeMX2 + its node, GNU Tools for STM32, CMake, Ninja, STM32CubeProgrammer CLI), every
pinned pack, python + pyserial, and lists ST-LINKs with their VCP. Read-only.
**Gate:** `missing/bad=0`. A missing bundle/pack: tell the user to install it with STM32CubeMX2 /
the STM32Cube bundle manager - don't install it yourself; never silently use another version.

## Stage 2 - Create the project (and stage 4: discover the board)

1. **Discover (read-only):** `bash "$SKILL/scripts/discover.sh" <board-id> [--serial <sn>]`
   - lists ST-LINKs, hot-plug connects (no reset, no write) and checks the board name the ST-LINK
     reports, the device name and the device ID; saves the bench file.
   - **Gate:** `IDENTITY: PASS`. Several ST-LINKs: `--serial` (it never guesses).
2. **Create the app:** `bash "$SKILL/scripts/new_app.sh" <board-id> <app> [<workspace>] [template] [--ioc2 <file>]`
   - **Workspace (once per PC):** exit 10 `ACTION: CHOOSE` = no apps workspace chosen for this PC
     and `apps/` is in a synced folder or too long. Ask the developer (outside the synced folder,
     e.g. `C:/fw-ws` - recommended - or keep `apps/`), run `bash <repo>/lib/fw_workspace.sh set <dir>`,
     re-run `new_app.sh`; then use the app path it prints (`<workspace>/<app>`) in the later stages
     (docs/workflow.md "Apps workspace").
   - starts a headless STM32CubeMX2 backend (own free port, stopped afterwards), creates
     `<app>.ioc2` from the board + `MX_CONFIG` + the FreeRTOS settings above, copies the template,
     then runs `regen.sh` (about 3.5 min with MX 1.2.0).
   - **Gate:** `RTOS: FreeRTOS ...`, `REGEN: PASS` and `Created ...`. "add FreeRTOS ... failed" =
     the FreeRTOS pack is missing: Stage 1 names it.
3. **After any .ioc2 change:** `bash "$SKILL/scripts/regen.sh" <app>` - generates `mx/` from a
   working copy (the app's `.ioc2` stays free of absolute paths), then re-applies the hook:
   `main.c` calls `app_main()`, `CMakeLists.txt` compiles `../src/**/*.c` with `-DBOARD_<ID>` and
   `-Wextra`. **Gate:** `REGEN: PASS`. "overlay failed" = the generated markers changed (new
   STM32CubeMX2) - update `scripts/overlay.py`, never hand-edit `mx/`.
4. **Open in the IDE (stage 2d, automatic):** at the end, `new_app.sh` runs
   `bash "$SKILL/scripts/open_ide.sh" <app>` (`--no-open` on new_app/open_ide: no window). It opens
   `<app>.code-workspace` (`mx/`, `src/`, `tests/` as folders - never the app folder) in VS Code;
   `regen.sh` pre-writes the STM32CubeIDE for VS Code setup of `mx/` from the board profile
   (`.settings/ide.store.json`, `.settings/bundles.store.json` = the pinned bundles,
   `.vscode/settings.json` for cube-cmake, `.vscode/launch.json` = the extension's default ST-LINK
   launch). The developer can then build (CMake preset, same `mx/build/<preset>` as build.sh),
   flash + debug (F5) and edit `src/` in the IDE; hardware changes go through STM32CubeMX2 on the
   `.ioc2` + `regen.sh`. **Gate:** `IDE: READY <workspace>` + `IDE: OPENED` (`IDE: not opened` with `--no-open`); exit 10
   `ACTION: SETUP` = VS Code or the extension is missing (new_app.sh only reports it). See
   docs/workflow.md "IDE handoff".

## Stage 2e - Code in layers

Read `reference/layering.md` first. Rules:
- `src/board/` - **only** place for GPIO ports/pins, HAL2/LL calls, `#if defined(BOARD_xxx)`.
  User LEDs/buttons are configured at run time here (HAL GPIO); clocks, USART2 and its pins come
  from the generated `mx_system_init()`.
- `src/func/` - services (console, led, button) copied from `<repo>/lib/func` - shared unchanged
  with the modus-pdl-edgitalk skill; a reusable new service goes into `lib/func` (board.h only).
- `src/app_main.c` - execution layer: `app_main()` initialises the board and the services,
  creates the tasks (`app_rtos_task()`), starts the console task (`app_rtos_console_start()`) and
  the scheduler (`app_rtos_run()`, never returns). Tasks, queues, notifications and delays
  (`FreeRTOS.h`, `task.h` ...) belong here only - `func/` stays init/poll and never includes RTOS
  headers (`check_layers.py` fails it), so the same `func/` runs under the bare-metal skill.
- `src/app_rtos.c/.h` (from `templates/_common`) - task registry, console task (polls the UART
  every 10 ms), the `tasks` console command (`TASK name=.. state=.. prio=.. stack_free=<words>`,
  then `HEAP free=.. min=.. total=.. tasks=..`), stack size constants (in **words**).
- `src/board/` here is RTOS-aware: `board_delay_ms()` = `vTaskDelay()` once the scheduler runs,
  `board_millis()` = the HAL tick (TIM6), console RX by interrupt, console writes hold a mutex - build each report line
  first and print it with ONE `printf` so lines of two tasks never mix.
- Never edit `mx/` (rewritten by `regen.sh`). Peripherals: enable them in the `.ioc2`, regenerate,
  then wrap the generated `mx_<periph>_..._gethandle()` in `board/`.
- Console protocol: `OK ...`, `ERR ...`, `EVT ...`, `<TAG> k=v ...`, `INFO ... board=<id> ...
  rtos=FreeRTOS V<x>` then `READY`. Console RX runs by interrupt into a 128-byte ring in `board/`
  (`USART2_IRQHandler`, priority 6); the console task empties it every 10 ms. Polling the receive
  flag from a task - as the bare-metal skill does in its loop - loses all but the first byte of a
  line (found on the board 2026-11-12: `info` arrived as `i`).
- Priorities: higher = more urgent; console 1, worker tasks 2-3. A task that never blocks starves
  the console - every task loop must block (`vTaskDelayUntil`, `ulTaskNotifyTake`, a queue).
- Templates: `hello-world` (helloTask), `blink` (ledTask), `push-to-light` (buttonTask polling
  `button_poll()` every 1 ms), `blink-rate` (buttonTask samples B1 every 100 ms and sends a toggle
  event - a task notification - to ledTask, which blinks LD1 at 1 s -> 0.5 s -> 0.2 s -> 1 s; its
  rules are in `func/blink_rate.c`; console `step` = the same event, `period?`). Tests:
  `tests/<spec>.json` (named after the template; the bring-up specs are the bare-metal ones plus a
  `tasks` step).

**Check the layers (M2 - gated, stops with `ACTION: SETUP` until enabled):**
`python <repo>/lib/check_layers.py <app-dir>` → `LAYERS: PASS` (exit 2 = warnings to fix), and
`bash <repo>/lib/host_test.sh <app-dir>` → `HOST: PASS` (the app's `func/` unit-tested on the PC
against a test stub of the board, no hardware). Fix a violation in the layer the message names.

## Stage 3 - Build

```bash
bash "$SKILL/scripts/build.sh" <app> [--clean] [--allow-warnings]
```
`cmake --preset debug_GCC_<CPN>` + `cmake --build` with the pinned tools on PATH.
**Gate:** `BUILD: PASS` = exit 0, ELF present, **0 warnings in `src/`** (exit 2 = fix them).
Writes `<sw>.hex` and `mx/build/<preset>/manifest.txt` (tool versions, packs as linked, `.ioc2`
sha256, ELF/HEX sha256, size) **only on PASS** - flash.sh requires it.

## Stage 5 - Flash

**Ask the user before every flash** (board, app, ELF sha256 prefix): it overwrites the board's
firmware. Then:
```bash
bash "$SKILL/scripts/flash.sh" <app> --yes [--elf <known-good.elf>]
```
`FLASH_POLICY=auto` in this board's bench file replaces `--yes` for a dedicated lab board - only
the developer writes it, never you. discover, flash and test hold a per-board lock: `board '<id>'
is in use` means another run has the board - wait; delete the lock only after the user confirms
that run is gone.
Gates: ELF matches the PASS manifest → bench ST-LINK connected → hot-plug identity (ST-LINK board,
device name, device ID; stops on signs of read-out protection) → `-c port=SWD sn=<sn> mode=UR
reset=HWrst -d <elf> -v -rst`. **Gate:** `FLASH: PASS` (`Download verified successfully`).

## Stage 6 - Test (after flashing)

Spec format (`tests/*.json` keys, JSON escaping, what is not measured): `docs/workflow.md`
"Test specs" in the repo root.

```bash
python "$SKILL/scripts/serial_test.py" auto <app>/tests/<spec>.json <app>/logs/test-<ts>.log --board <id> [--interactive | --only-interactive]
```
`auto` = the VCP of the bench ST-LINK. Syncs with `info` until `READY`, checks `board=<id>`, runs
the steps. Interactive steps (press B1, look at LD1) only with `--interactive`, after telling the
user exactly what to do. **Gate:** `RESULT: PASS`. `led?` reads the GPIO output register back.
Every spec checks `tasks`: each task exists and has >= 50 free stack words, heap total 16384.
Report the `TASK`/`HEAP` lines with the result (stack margin per task).

## Stage 7 - Debug through the ST-LINK

Read-only status - hot-plug connect, **no reset, no halt** (the firmware keeps running):
```bash
bash "$SKILL/scripts/debug.sh" status <board-id>
```
Prints CPUID, DHCSR (running / halted / sleeping / lockup) and the fault registers decoded (CFSR,
HFSR; MMFAR / BFAR only when CFSR marks them valid); raw output in `<apps>/.bench/<board>.debug-status.log`.
The connect sets "debug in low power mode" in the MCU's debug unit (a debug setting, not program
memory; it is cleared by a power cycle). Which app the board runs: its console `info` line (Stage 6
or sensor-debug `send info`); whether it is exactly this ELF: only `debug.sh run` (it halts). **Gate:** `DEBUG: STATUS <state> fault=<none|...>`; exit 1 = a fault or
lockup is latched - keep the board as it is (no reset, no flash) and report the decoded bits.

Scripted breakpoint / watch / step session:
```bash
bash "$SKILL/scripts/debug.sh" run <app> <scenario.json> [--port N]
```
ST-LINK_gdbserver **attaches** (no reset, no programming) and GDB first reads the app's loadable
segments back from the MCU: a board that runs another image stops with "flash the app first" (that
flash needs the developer's yes). Then the scenario's steps run - `break` (+ a console line sent
while running), `print`, `step`, `watch` (a hardware watchpoint), `read` (memory or a fault
register) - each with an expectation. Ready-made: `$SKILL/scenarios/push-to-light.json` (for an app
made from push-to-light: the `led 1 on` command reaches `cmd_led` in the console task, its
arguments, the HAL tick, no fault - **not yet run under FreeRTOS**; the HAL tick is TIM6 here). Keys: header of `<repo>/lib/fwdebug.py`. **The core is halted while GDB is attached -
tell the developer first**; the engine always detaches, so the firmware runs on (after
push-to-light.json LED1 stays on). An app's own scenarios go into `<app>/scenarios/`.
**Gate:** `DEBUG: PASS (n/m steps, log <app>/logs/debug-<ts>.log)`.
The STM32C5's core sits on access port 1 (`DEBUG_APID=1` in `board.env`; AP 0 gives "Target
unknown error 32").
Halting changes timing: keep timing checks in Stage 6 tests, not in scenarios.

With several ST-LINKs plugged in, `discover.sh` picks the one whose on-board ST-LINK reports
this board (`EXPECTED_STLINK_BOARD`); otherwise it asks (`ACTION: CHOOSE`, `board=` column).

## Stage 16 - Clean

```bash
bash "$SKILL/scripts/clean.sh" [--apps] [<workspace>]          # dry run
bash "$SKILL/scripts/clean.sh" [--apps] [<workspace>] --yes    # delete (ask the user first)
```
Default: `mx/build` and `.mx` only. `--apps`: whole apps (`cubemx-app.env`) incl. the user's
`.ioc2` and `src/`, plus `.bench` - only when explicitly asked. `mx/` itself is kept (regen.sh).

## Extending

- **Another STM32C5 board:** `mx finder` / the board pack name, then copy `boards/_template/`,
  fill `README.md` + `cubemx2-freertos-stm32c562nucleo/board.env` (CPN, pack version, `MX_CONFIG` for its VCP UART,
  a free basic timer for `MX_TIMEBASE_TIM`,
  device name/ID from `discover.sh`, `EXPECTED_STLINK_BOARD`), add its `#elif` user-I/O table in
  `templates/_common/src/board/board.c` and the `BOARD_NAME` mapping in `board.h`.
- **Tool/pack update:** change the versions in the profile (FreeRTOS: `MX_MIDDLEWARE`,
  `MX_RTOS_COMPONENT` and `PACKS` together), re-run every template through create → build → flash
  → test, compare manifests, log it. Never float to "whatever is installed".
- **Another RTOS setting:** add `<resource>|<path>|<value>` to `MX_SWCONFIG` (paths:
  `mx sw-config list-parameters -r FreeRTOS`); never edit `mx/generated/middleware/FreeRTOSConfig.h`.

## Reporting (always)

Report levels separately: **source matches → generated → built → flashed → booted (READY) →
tested (PASS n/m) → observed by a human**. Quote ELF sha256, tool/pack versions, log paths. Add a
dated line to `boards/<id>/cubemx2-freertos-stm32c562nucleo/README.md` when a board fact is newly verified.

## Safety rules

- Identity gates before every write; never flash an ELF without a PASS manifest (except `--elf`
  chosen by the user) or a board whose ST-LINK board / device name / ID don't match.
- Never change option bytes, read-out protection (RDP), product state or security settings;
  never mass-erase or upgrade ST-LINK firmware without asking.
- Ask before every flash and before `clean.sh --yes`. Say before `debug.sh run` that it halts the
  core while attached; never reset, reflash or detach-and-reset a board whose fault state is
  being kept (Stage 7 exit 1) unless the developer agrees.
- The headless backend is started and killed by the scripts on their own port; never kill other
  STM32CubeMX2 / STM32CubeIDE processes.
