---
name: cubemx-freertos-stm32l476disco
description: FreeRTOS firmware workflow for the ST STM32L476G-DISCO / 32L476GDISCOVERY (STM32L476VG, Cortex-M4F) with classic STM32CubeMX 6.18 in headless script mode, the STM32CubeL4 HAL, FreeRTOS V10.3.1 from the firmware package and CMake - check the pinned tools, create the .ioc from the CubeMX board configuration plus the USART2 console on the ST-LINK virtual COM port, the HAL timebase on TIM6 and FreeRTOS (heap, stack-overflow check), generate the CMake project headlessly with an RTOS gate, write layered code whose execution layer runs FreeRTOS tasks (board -> func -> app_main/app_rtos), build with a manifest, discover the ST-LINK/V2-1, flash after asking, run the UART tests (tasks report, joystick, LD4/LD5), read the debug status. Use when asked to create, build, flash or test FreeRTOS firmware (tasks, two tasks, RTOS) for the STM32L476G-DISCO (บอร์ด 32L476GDISCOVERY กับ FreeRTOS), or how to use this skill (ขอวิธีใช้, help). Bare-metal apps on this board use cubemx-hal-stm32l476disco.
license: MIT
compatibility: Claude Code or another agent that runs bash; Windows 10/11 + Git Bash, Python 3 + pyserial; needs STM32CubeMX 6.18 + STM32CubeL4 + STM32Cube bundles (GCC, CMake, Ninja, STM32CubeProgrammer); uses this repo's lib/ and boards/ (install the whole plugin or clone the repo)
---

# STM32L476G-DISCO firmware workflow (STM32CubeMX + STM32CubeL4 HAL + CMake) - FreeRTOS

Gated stages, the same contract as every skill in this repo (`docs/workflow.md`). Do them in
order; do not start a stage until the previous gate passed. **Windows + Git Bash only**
(STM32CubeMX for Windows, STM32Cube bundles, `cygpath`, `taskkill`). Every app runs in **FreeRTOS
tasks** (section "FreeRTOS execution model"). Scripts, board profile and board layer come from the
bare-metal skill
[cubemx-hal-stm32l476disco](https://github.com/vsupacha/automated-firmware-skills/blob/main/skills/cubemx-hal-stm32l476disco/SKILL.md)
(automated-firmware-skills); templates and `app_rtos.c/.h` from `cubemx2-freertos-stm32c562nucleo`
in this repository.

```
1 tools   ─► 2 project     ─► 2e code  ─► 3 build ─► 4 connect        ─► 5 flash ─► 6 test    ─► 7 debug ─► 16 clean
check_tools  new_app / regen  src/board   build.sh   discover (ST-LINK)  flash.sh   serial_test  debug.sh   clean.sh
             open_ide         src/func
                              app_main.c
```

Stage numbers are the shared ones of `docs/workflow.md`; the section headings below use the same numbers.
Stage 4 (`discover.sh`, read-only) is needed before flash and test, not before create or build.

**Two paths after Stage 2** (`docs/workflow.md`): the developer may code in VS Code at any
time - the **IDE path** (`new_app.sh` already opened the app; `open_ide.sh <app>` reopens it; they
build, flash and debug with the vendor extension's buttons) - or let you run the stages - the
**script path** (build, layer checks, flash, tests behind their gates). Ask which one when it is
not clear. On the IDE path, stop after stage 2d and run scripts only when asked ("build it", "test
it"); never overwrite files the developer is editing. Both paths share the app folder and build
outputs, so switching is always possible: on the script path re-run the gates from Stage 3.

## Main path (script path)

<!-- main-path -->
The whole automated flow, in order, from the repository root. Board id `stm32l476g-disco`; `<app>` is the
folder `new_app.sh` prints (in this PC's apps workspace). Other templates: replace `blink`.

```bash
S=skills/cubemx-freertos-stm32l476disco/scripts
bash $S/check_tools.sh stm32l476g-disco                         # 1 tools:   line with missing/bad=0
bash $S/new_app.sh stm32l476g-disco my-app "" blink --no-open   # 2 create:  "Created <app>" (prints the app folder)
bash $S/build.sh <app>                                          # 3 build:   BUILD: PASS
bash $S/discover.sh stm32l476g-disco                            # 4 connect: IDENTITY: PASS (read-only, needs the board)
bash $S/flash.sh <app>                                          # 5 flash:   stops with ACTION: APPROVE; after the developer's yes add --yes -> FLASH: PASS
python $S/serial_test.py auto <app>/tests/blink.json <app>/logs/test.log --board stm32l476g-disco   # 6 test:    RESULT: PASS
```

A stage passed only when its gate line is printed. Exit code 10 with an `ACTION:` line means: stop,
relay it to the developer, and re-run the same command after they acted - never add `--yes` on
your own. The sections below explain these stages; IDE handoff, debug, unit tests, tool updates
and clean are optional.

## Milestones, stage numbers and developer actions

This skill implements the shared contract in `docs/workflow.md` (repo root):

| Milestone | Shared stage | In this file |
| --- | --- | --- |
| M1 bring-up | 1 setup, 2 create, 2d IDE, 3 build | Stage 1, Stage 2 steps 2-4 (+ `regen.sh`, `open_ide.sh`), Stage 3 |
| M1 bring-up (board) | 4 connect, 5 flash, 6 test, 7 debug | Stage 4 (discover), Stage 5, Stage 6, Stage 7 |
| M2 board support | 8 board drivers *(planned)*, 9 layer check, 10 host test *(optional)* | Stage 2e (rules, shared `lib/func`, `check_layers.py`, `host_test.sh`) |
| M3 execution, M4 components, M5 application | 11 execution + trace, 12 components, 13 profile *(planned)* | - |
| - | 14 export *(planned)*, 15 port *(planned)* | Extending |
| - | 16 clean | Stage 16 |

**Exit codes:** 0 gate passed, 1 failed, 2 warnings, **10 = developer action needed**. On exit 10
the script's last line is `ACTION: <TYPE> <what to do>` (SETUP, CONNECT, APPROVE, CHOOSE...): tell
the user exactly that, wait for their OK, then re-run the same command. Never work around an
ACTION yourself - installing STM32CubeMX, firmware packages, bundles or drivers, plugging hardware
and approving flashes or deletions are the developer's.

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
<repo>/skills/cubemx-freertos-stm32l476disco/        this skill (SKILL.md, scripts/, reference/, templates/)
<repo>/lib/func/                              shared C logic layer, copied into every app's src/func
<repo>/lib/fwtest.py                          serial test engine (serial_test.py wraps it)
<repo>/boards/<id>/README.md                  board hardware (tool-independent)
<repo>/boards/<id>/cubemx-freertos-stm32l476disco/   board profile: board.env, README (verification log)
<repo>/apps/                                  apps workspace only when this PC has none chosen
```

Workspace `$L476_WS` (set it to override): this PC's apps workspace, chosen once with `lib/fw_workspace.sh` and saved in `~/.fw-skills.env`
(for example `C:/fw-ws`); `<repo>/apps` is used only when none is chosen. In every later command use
the app path that `new_app.sh` prints (`<workspace>/<app>`).
Board profiles:
`<project>/boards/<id>/cubemx-freertos-stm32l476disco/` first, then `<repo>/boards` (override
`L476_BOARDS_DIR`). STM32CubeMX: installed for all users (`C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeMX`)
or per user (`%LOCALAPPDATA%\Programs\STM32CubeMX`) - both are searched (override `CUBEMX_HOME`);
its firmware repository comes from CubeMX's own settings. STM32Cube bundles: `%LOCALAPPDATA%\stm32cube`
(override `STM32CUBE_ROOT`).

## What an app looks like

```
apps/<app>/
  <app>.ioc             STM32CubeMX configuration - the source of truth (commit it)
  src/                  our layered code: board/, func/ (copy of lib/func), app_main.c, app_rtos.c/.h (commit it)
  tests/<spec>.json     test contract, the same specs as the other skills (commit it)
  l476-app.env            record: board, template, where the .ioc came from, CubeMX version
  <app>.code-workspace  VS Code workspace: mx/, src/, tests/ as folders (open_ide.sh; git-ignored)
  mx/                   generated CMake project (CubeMX-owned + overlay; git-ignored)
  .mx/ logs/            CubeMX scripts, build/flash/test logs (git-ignored)
```

**Where the .ioc comes from:** `new_app.sh` runs `MX_START` (`loadboard STM32L476G-DISCO nomode`:
the CubeMX board configuration - labelled pins, joystick, LEDs, SWD, the MSI 4 MHz clock, without the
board's LCD/audio/MEMS/QSPI) and the profile's `MX_CONFIG` (`set mode USART2 Asynchronous` + `set pin PD5
USART2_TX` / `PD6 USART2_RX`: the ST-LINK virtual COM port, 115200 8N1, plus the FreeRTOS lines of the
next section). Change the hardware with the STM32CubeMX GUI on `<app>.ioc`, then `regen.sh` (it refuses a
project without FreeRTOS or the TIM timebase).

## FreeRTOS execution model

What `new_app.sh` puts into every `.ioc` (the profile's `MX_CONFIG`, classic STM32CubeMX CLI):

| Setting | CLI command | Why |
| --- | --- | --- |
| HAL timebase TIM6 | `set mode SYS TIM6` | FreeRTOS uses SysTick for its 1 ms tick |
| FreeRTOS | `set mode FREERTOS CMSIS_V2` | from STM32Cube_FW_L4 (kernel V10.3.1, port GCC/ARM_CM4F, heap_4); the apps use the native FreeRTOS API, not the CMSIS-RTOS2 calls |
| heap | `set ip parameters FREERTOS configTOTAL_HEAP_SIZE 16384` | three printf tasks of 512 words + kernel tasks |
| stack overflow check | `set ip parameters FREERTOS configCHECK_FOR_STACK_OVERFLOW 2` | the generated `freertos.c` hook is empty - `overlay.py` makes it stop (interrupts off, endless loop) |
| high-water marks | `set ip parameters FREERTOS INCLUDE_uxTaskGetStackHighWaterMark 1` | free stack per task in the `tasks` report |

`get mode all` lists no middleware, but these `set` commands answer OK. `new_app.sh` and `regen.sh`
run `rtos_gate` (scripts/env.sh) on the generated project - FreeRTOSConfig.h present, HAL timebase
on a TIM (`stm32l4xx_hal_timebase_tim.c`), heap >= `RTOS_HEAP_SIZE`, stack check 2 - and print
`RTOS: FreeRTOS V10.3.1, HAL timebase TIM6, heap 16384 B, stack check 2` before `REGEN: PASS`.

**Kernel start:** `app_main()` (USER CODE 2 in the generated `main.c`) creates the tasks with
`app_rtos_task()` and calls `app_rtos_run()` = `vTaskStartScheduler()`; it never returns, so the
generated `osKernelInitialize()` / `defaultTask` after it are never reached. Do not call CMSIS-RTOS2
(`os...`) functions - the wrapper is not initialised. The console RX interrupt calls no RTOS API.

## Board type vs bench instance

- **Board type** (`boards/<id>/`): `README.md` = hardware; `cubemx-freertos-stm32l476disco/board.env` =
  pinned CubeMX, firmware package and bundle versions, `MX_START`, `MX_CONFIG`, `BOARD_DEFINE`,
  `BTN_LABELS`, expected ST-LINK board / device name / device ID.
- **Bench instance** (`<workspace>/.bench/<id>.env`, written by `discover.sh`): `PROBE_SERIAL`
  (ST-LINK) and `CONSOLE_PORT` (its virtual COM port). Override with `L476_PROBE_SERIAL` /
  `L476_CONSOLE`. Never write them into `boards/`.

| id | board | status |
| --- | --- | --- |
| `stm32l476g-disco` | ST STM32L476G-DISCO (STM32L476VGT6, ST-LINK/V2-1 with VCP) | see `boards/stm32l476g-disco/cubemx-freertos-stm32l476disco/README.md` |

## Hardware quick reference

| Board | Button | LED | Console |
| --- | --- | --- | --- |
| STM32L476G-DISCO | 5-way **joystick** as BTN1..BTN5: center PA0, left PA1, right PA2, up PA3, down PA5, pull-down, **pressed = high**; black B1 = RESET | **LD4** red PB2 (LED1), **LD5** green PE8 (LED2), active high | USART2 PD5/PD6 → ST-LINK/V2-1 VCP, 115200; **no RX FIFO** - console RX by interrupt |

## Stage 1 - Tools and folder paths

```bash
bash "$SKILL/scripts/check_tools.sh" <board-id> [--ws <workspace>]
```
Checks the workspace path (no spaces/non-ASCII, ≤100 characters), STM32CubeMX (pinned version, its
bundled java), the CubeMX repository and the pinned `STM32Cube_FW_L4` package (+ the board's BSP),
GNU Tools for STM32 / CMake / Ninja / STM32CubeProgrammer bundles, python + pyserial (board stages) and the
IDE (VS Code + STM32CubeIDE for VS Code, warning only). Every tool row says **GLOBAL** (installed
for all users) or **LOCAL** (per user). Read-only. **Gate:** `missing/bad=0`.

**Tool updates (stage 1b):** `bash "$SKILL/scripts/update_tools.sh" <board-id>` compares every pin of the
board profile (CubeMX, firmware package, STM32Cube bundles, `IDE_EXT_VERSION`) with this PC. Read-only.
**Gate:** `UPDATE: NONE`, or `UPDATE: AVAILABLE n` + exit 10 `ACTION: APPROVE` - ask the developer;
only after a yes run it again with `--apply --yes` (re-pins the profile, logs "not re-qualified"),
then re-qualify: regen + build, flash (asks first) and the tests. Run it when check_tools shows
an `IDE: extension` WARN or a tool update is announced.

## Stage 2 - Create the project (and stage 4: discover the board)

1. **Discover (stage 4 connect, read-only):** `bash "$SKILL/scripts/discover.sh" <board-id> [--serial <sn>]`
   - lists ST-LINKs, hot-plug connects (no reset, no write) and checks the board name the ST-LINK
     reports (`STM32L476G-DK`), the device name and the device ID (0x415); saves the bench file.
   - **Gate:** `IDENTITY: PASS`. Several ST-LINKs: `--serial` (it never guesses). No board yet?
     Skip to step 2.
2. **Create:** `bash "$SKILL/scripts/new_app.sh" <board-id> <app> [<workspace>|""] [template] [--no-open]`
   - Example with a template and no IDE window (the empty `""` keeps this PC's workspace):
     `bash "$SKILL/scripts/new_app.sh" <board-id> my-app "" blink --no-open`
   - **Workspace (once per PC):** exit 10 `ACTION: CHOOSE` = no apps workspace chosen for this PC
     and `apps/` is in a synced folder or too long. Ask the developer (outside the synced folder,
     e.g. `C:/fw-ws` - recommended - or keep `apps/`), run `bash <repo>/lib/fw_workspace.sh set <dir>`,
     re-run `new_app.sh`; then use the app path it prints (`<workspace>/<app>`) in the later stages
     (docs/workflow.md "Apps workspace").
   - copies `lib/func` + `templates/_common/src` + the template, then one headless CubeMX run
     (`MX_START`, `MX_CONFIG`, save `<app>.ioc`, generate `mx/`), then `overlay.py`. 30 s - 2 min.
   - **Gate:** `REGEN: PASS` and `Created ...` (any `KO`, lost file or missing `exit` fails it).
3. **After any .ioc change:** `bash "$SKILL/scripts/regen.sh" <app>` - regenerates `mx/` (USER CODE
   sections survive) and re-applies the overlay. **Gate:** `REGEN: PASS`.
4. **Open in the IDE (stage 2d, automatic):** at the end, `new_app.sh` runs
   `bash "$SKILL/scripts/open_ide.sh" <app>` (`--no-open`: no window). It opens
   `<app>.code-workspace` - never the app folder - in VS Code; `new_app`/`regen` have pre-written
   the STM32CubeIDE for VS Code setup of `mx/` from the board profile (`.settings/ide.store.json`,
   `.settings/bundles.store.json` with the pinned bundle versions, `.vscode/settings.json` for
   cube-cmake, `.vscode/launch.json` = the extension's default ST-LINK launch), so the extension
   configures and indexes the project without a prompt (seen 2026-10-07). The developer can then
   build (CMake preset, same `mx/build/Debug` as build.sh), flash + debug (F5) and edit `src/`.
   **Gate:** `IDE: READY <workspace>` + `IDE: OPENED` (`IDE: not opened` with `--no-open`); exit 10 `ACTION: SETUP` = VS Code or the
   extension is missing (new_app.sh only reports it). See docs/workflow.md "IDE handoff".

## Stage 2e - Code in layers

Read `reference/layering.md` first. Rules:
- `src/board/` - **only** place for GPIO ports/pins, HAL/CMSIS calls, `#if defined(BOARD_xxx)`.
  `board_init()` configures LD4/LD5 and the joystick itself; the console uses the generated `huart2`
  (blocking TX through `_write`, RX by the USART2 RXNE interrupt into a ring buffer - `USART2_IRQHandler`
  lives in `board.c`, so keep the USART2 interrupt **disabled** in the `.ioc` NVIC settings).
- `src/func/` - services (console, led, button) copied from `<repo>/lib/func` - shared unchanged
  with the other C skills; a reusable new service goes into `lib/func` (board.h only).
- `src/app_main.c` + `src/app_rtos.c/.h` - the execution layer: tasks, priorities, notifications;
  `app_main()` is called from `USER CODE BEGIN 2` and starts the scheduler (FreeRTOS headers only here
  and in `board/`; `check_layers.py` FAILs them in `func/`).
- Never edit `mx/` outside USER CODE sections. Peripherals: enable them in `<app>.ioc`, regenerate,
  then wrap the generated handle in `board/`.
- Console protocol: `READY`, `INFO app=.. board=<id>`, `OK ...`, `ERR ...`, `EVT ...` over the VCP.
- Templates (all as tasks, each with the console `tasks` command = free stack per task + heap):
  `hello-world` (prints every 1 s), `blink` (LED1 every 0.5 s), `push-to-light` (console `led`/`btn`
  commands, joystick center toggles LD4), `blink-rate` (buttonTask samples the joystick center every
  100 ms -> task notification -> ledTask steps LD4 1 s / 0.5 s / 0.2 s; rules in `func/blink_rate.c`).
  Tests: `tests/<spec>.json` (named after the template).
- Console lines stay whole: `board.c` collects each task's `printf()` pieces in a per-task line slot
  and sends the full line under a mutex (without it, lines of two tasks mixed on the board:
  `TASK name=BBLINK led1=1`). `board_delay_ms()` = `vTaskDelay()` once the scheduler runs.

**Check the layers (M2 - gated, stops with `ACTION: SETUP` until enabled):**
`python <repo>/lib/check_layers.py <app-dir>` → `LAYERS: PASS`, and
`bash <repo>/lib/host_test.sh <app-dir>` → `HOST: PASS` (the app's `func/` on the PC, test stub of the board).

## Stage 3 - Build

```bash
bash "$SKILL/scripts/build.sh" <app> [--clean] [--allow-warnings]
```
`cmake --preset Debug` + `cmake --build --preset Debug` in `mx/` with the pinned bundles on PATH.
**Gate:** `BUILD: PASS` = exit 0, `mx/build/Debug/mx.elf` present, **0 warnings in `src/`** (exit
2 = fix them). Writes `mx.hex` and `mx/build/Debug/manifest.txt` (CubeMX + firmware package + tool
versions, SYSCLK, `.ioc` sha256, ELF/HEX sha256, size) **only on PASS** - flash.sh requires it.

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
Gates: ELF matches the PASS manifest → bench ST-LINK connected → hot-plug identity (ST-LINK board
`STM32L476G-DK`, device name `STM32L4x1/STM32L475xx/STM32L476xx...`, device ID 0x415; stops on signs of read-out
protection) → `-c port=SWD sn=<sn> mode=UR reset=HWrst -d <elf> -v -rst`. **Gate:** `FLASH: PASS`
(`Download verified successfully`).

## Stage 6 - Test (after flashing)

Spec format (`tests/*.json` keys, JSON escaping, what is not measured): `docs/workflow.md`
"Test specs" in the repo root.

```bash
python "$SKILL/scripts/serial_test.py" auto <app>/tests/<spec>.json <app>/logs/test-<ts>.log --board <id> [--interactive | --only-interactive]
```
`auto` = the VCP of the bench ST-LINK. Syncs with `info` until `READY`, checks `board=<id>`, runs
the steps; steps for a second LED/button are skipped (`leds=1 btns=1`). Interactive steps (press
joystick center / left, look at LD4) only with `--interactive`, after telling the user exactly what to do - the
button press must come within 30 s. **Gate:** `RESULT: PASS`. `led?` reads the GPIO output
register back.

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
made from push-to-light: the `led 1 on` command reaches `cmd_led`, its arguments, the HAL tick,
no fault). Keys: header of `<repo>/lib/fwdebug.py`. **The core is halted while GDB is attached -
tell the developer first**; the engine always detaches, so the firmware runs on (after
push-to-light.json LED1 stays on). An app's own scenarios go into `<app>/scenarios/`.
**Gate:** `DEBUG: PASS (n/m steps, log <app>/logs/debug-<ts>.log)`.
Halting changes timing: keep timing checks in Stage 6 tests, not in scenarios. Under FreeRTOS only
`debug.sh status` was run on this board (2026-11-12); a `debug.sh run` scenario is not verified yet.

With several ST-LINKs plugged in, `discover.sh` picks the one whose on-board ST-LINK reports
this board (`EXPECTED_STLINK_BOARD`); otherwise it asks (`ACTION: CHOOSE`, `board=` column).

## Stage 16 - Clean

```bash
bash "$SKILL/scripts/clean.sh" [--apps] [<workspace>]          # dry run
bash "$SKILL/scripts/clean.sh" [--apps] [<workspace>] --yes    # delete (ask the user first)
```
Default: `mx/build` and `.mx` only. `--apps`: whole apps (`l476-app.env`) incl. the user's `.ioc` and
`src/`, plus `.bench` - only when explicitly asked. `mx/` itself is kept (regen.sh).

## Extending

- **Another STM32L4 board in the CubeMX board database:** copy `boards/_template/`, fill `README.md`
  + `cubemx-freertos-stm32l476disco/board.env` (`MX_START` with its CubeMX board name, `MX_CONFIG` for its
  console UART, expected ST-LINK board / device name / ID from `discover.sh`), add its `#elif`
  I/O table in `templates/_common/src/board/board.c` (and its console UART + IRQ if not USART2) and
  the `BOARD_NAME` mapping in `board.h`. A different board may later get a skill of its own name.
- **Tool/package update:** change the versions in the profile, re-run every template through
  create → build → flash → test, compare manifests, log it.

## Reporting (always)

Report levels separately: **source matches → generated → built → flashed → booted (READY) →
tested (PASS n/m) → observed by a human**. Quote ELF sha256, CubeMX/package/tool versions, log
paths. Add a dated line to `boards/<id>/cubemx-freertos-stm32l476disco/README.md` when a board fact is
newly verified.

## Safety rules

- Identity gates before every write; never flash an ELF without a PASS manifest (except `--elf`
  chosen by the user) or a board whose ST-LINK board / device name / ID don't match.
- Never change option bytes, read-out protection (RDP) or security settings; never mass-erase or
  upgrade the ST-LINK firmware without asking.
- Ask before every flash and before `clean.sh --yes`. Say before `debug.sh run` that it halts the
  core while attached; never reset, reflash or detach-and-reset a board whose fault state is
  being kept (Stage 7 exit 1) unless the developer agrees.
- CubeMX runs headless through its own java on scripts the skill writes; the scripts end that java
  process tree themselves - never kill other STM32CubeMX / STM32CubeIDE processes.
