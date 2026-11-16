# automated-firmware-skills-freertos

Agent skills that take a **FreeRTOS** firmware project for a board from "tools installed?" to
"tested on the board", with a gate at every stage. Split on 2026-11-12 from
[automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills), which keeps the
bare-metal (superloop) skills, because an RTOS changes how an app is built and checked: the RTOS
middleware is part of the generated project, the HAL tick moves off SysTick, the app runs in tasks,
and the tests also read task and stack status. Written for Claude Code; the scripts are plain
bash/Python, so a developer can run every stage by hand.

## Concepts

- **Skill = toolchain + framework + board** (`skills/<toolchain>-freertos-<board>/`): `SKILL.md`,
  scripts, references, app templates.
- **The RTOS comes from the vendor tool.** `new_app.sh` creates the board project with the board's
  configurator (the STM32CubeMX2 CLI on the C5, a classic STM32CubeMX script on the F3 and L476), adds the
  FreeRTOS middleware, moves the HAL timebase off SysTick (TIM6) and sets heap and stack-overflow checking
  (on the C5 also FPU context save); `regen.sh` refuses a generated project without FreeRTOS (`RTOS:` line).
- **Same layers as bare metal.** `src/board/` (RTOS-aware: `vTaskDelay`, console mutex, RX by
  interrupt), `src/func/` (portable services - no RTOS headers, `lib/check_layers.py` fails them
  there), and the execution layer (`app_main.c`, `app_rtos.c`) that owns the tasks.
- **Gated stages, the developer decides.** Each script ends with a gate line (`BUILD: PASS`,
  `IDENTITY: PASS`, `FLASH: PASS`, `RESULT: PASS`); steps only a person can do (installing a pack,
  plugging a board, **approving a flash**) stop with exit code 10 and an `ACTION: ...` line.
- **Honest results.** built ≠ flashed ≠ tested ≠ observed - every board keeps a dated log.

## Requirements

| What | Needed for | Notes |
| --- | --- | --- |
| Windows 10/11 + Git for Windows (Git Bash) | every script | bash scripts |
| Python 3 + `pyserial` | tests, debug | tested with Python 3.12 |
| STM32CubeMX2 1.2.0 + ST's FreeRTOS pack 2.2.0 (C5), or classic STM32CubeMX 6.18.1 + STM32Cube_FW_F3 1.11.6 (F3) / STM32Cube_FW_L4 1.18.2 (L476); the STM32Cube bundles (GCC, CMake, Ninja, STM32CubeProgrammer) | setup onward | `check_tools.sh <board>` lists what is missing and the install command |
| The board and its USB cable | connect, flash, test | build stages need no board |

## Skills and boards

| Skill | Framework | MCU | Board | How to use |
|---|---|---|---|---|
| [cubemx2-freertos-stm32c562nucleo](skills/cubemx2-freertos-stm32c562nucleo/SKILL.md) | STM32CubeMX2 1.2.0 + HAL2 + FreeRTOS 11.3.1 (ST pack 2.2.0) + CMake | ST STM32C5 | [NUCLEO-C562RE](boards/nucleo-c562re/README.md) | [guide](skills/cubemx2-freertos-stm32c562nucleo/README.md) · [ไทย](skills/cubemx2-freertos-stm32c562nucleo/README_TH.md) |
| [cubemx-freertos-stm32f303disco](skills/cubemx-freertos-stm32f303disco/SKILL.md) | STM32CubeMX 6.18 + STM32CubeF3 HAL + FreeRTOS V10.0.1 + CMake | ST STM32F3 | [STM32F3DISCOVERY](boards/stm32f3-discovery/README.md) | [guide](skills/cubemx-freertos-stm32f303disco/README.md) · [ไทย](skills/cubemx-freertos-stm32f303disco/README_TH.md) |
| [cubemx-freertos-stm32l476disco](skills/cubemx-freertos-stm32l476disco/SKILL.md) | STM32CubeMX 6.18 + STM32CubeL4 HAL + FreeRTOS V10.3.1 + CMake | ST STM32L4 | [STM32L476G-DISCO](boards/stm32l476g-disco/README.md) | [guide](skills/cubemx-freertos-stm32l476disco/README.md) · [ไทย](skills/cubemx-freertos-stm32l476disco/README_TH.md) |

The same boards' bare-metal skills: `cubemx2-hal2-stm32c562nucleo`, `cubemx-hal-stm32f303disco` and `cubemx-hal-stm32l476disco` in
[automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills).

Tool skills (plugin `firmware-tools`, installed with `firmware-freertos`):

| Skill | What it does |
|---|---|
| [cleanroom-test](skills/cleanroom-test/SKILL.md) | a fresh agent follows only the docs on a test app; its findings become doc and script fixes (prompt `rtos-two-tasks.md`, first report 2026-11-12) |

Board status and demo levels: [boards/README.md](boards/README.md). Stages, gates, exit codes and
the test-spec format: [docs/workflow.md](docs/workflow.md) (copy of the original repo's contract).

## Install

```
/plugin marketplace add vsupacha/automated-firmware-skills-freertos
/plugin install firmware-freertos@automated-firmware-skills-freertos
```

## Layout

```
skills/<skill>/          SKILL.md, scripts/, templates/, scenarios/, reference/
boards/<id>/             hardware sheet + per-skill profile (board.env) and verification log
boards/_template/        profile templates for a new board
lib/                     shared shell/Python helpers (common.sh, fwtest.py, fwdebug.py, fwpaths.py,
                         fw_workspace.sh, check_layers.py) and the portable func/ services
docs/                    workflow.md (stages, gates), board-api.md (layers, board.h)
tools/                   validate_skills.py, board_status.py
milestones.env           active milestones (board stages need M1)
```

## Related repositories

Five repositories work together; each one is usable on its own.

| Repository | Scope |
|---|---|
| [automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills) | bare-metal MCU skills (STM32Cube, PlatformIO, ModusToolbox, MCUXpresso) and the support tool skills: firmware-test, cleanroom-test, hw-troubleshooting, sensor-debug, web-wizard |
| **automated-firmware-skills-freertos** (this repository) | FreeRTOS skills (STM32 C5, F3, L476) |
| [automated-firmware-skills-rtthread](https://github.com/vsupacha/automated-firmware-skills-rtthread) | RT-Thread skills with Kconfig recipes (Renesas RA8 Vision Board and Titan Board, PSOC Edge Edgi-Talk) |
| [automated-firmware-skills-emblinux](https://github.com/vsupacha/automated-firmware-skills-emblinux) | embedded-Linux boards (Arduino UNO Q: App Lab Bricks + Zephyr sketch) |
| [firmware-dev-life-cycle](https://github.com/vsupacha/firmware-dev-life-cycle) | local web UI and phase contracts: pick a board, run the stages of any repository above (Auto page) or create an app and open its IDE (IDE page), with gates, logs and artifacts on screen |

To use several, clone them side by side in one folder; `firmware-dev-life-cycle` then finds the skill
repositories there (its `providers/*.json`). They share conventions - stage numbers, gate lines, exit code
10 with an `ACTION:` line for a developer step, one per-PC apps workspace in `~/.fw-skills.env` - but no
code: `lib/` is a copy in each repository.

Support tool skills that are not in this repository: `hw-troubleshooting` and `sensor-debug` work with any
board whose firmware prints console lines - use them from a clone of automated-firmware-skills.
`web-wizard` reads that repository's own boards and does not cover the boards here.

## Contributing

Run `python tools/validate_skills.py` before a commit (PASS required). See CONTRIBUTING.md.

## License

MIT (LICENSE). Vendor code generated into apps keeps its own licences (FreeRTOS MIT, STM32Cube HAL
BSD-3-Clause).
