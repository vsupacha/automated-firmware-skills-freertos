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
- **The RTOS comes from the vendor tool.** `new_app.sh` creates the board project with STM32CubeMX2,
  adds ST's FreeRTOS middleware with the CLI, moves the HAL timebase to TIM6, sets FPU context save,
  heap and stack-overflow checking, and reads every value back; `regen.sh` refuses a generated
  project without FreeRTOS (`RTOS:` line).
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
| STM32CubeMX2 1.2.0 + ST's FreeRTOS pack 2.2.0 (C5), or classic STM32CubeMX 6.18.1 + STM32Cube_FW_F3 1.11.6 (F3); the STM32Cube bundles (GCC, CMake, Ninja, STM32CubeProgrammer) | setup onward | `check_tools.sh <board>` lists what is missing and the install command |
| The board and its USB cable | connect, flash, test | build stages need no board |

## Skills and boards

| Skill | Framework | MCU | Board | How to use |
|---|---|---|---|---|
| [cubemx2-freertos-stm32c562nucleo](skills/cubemx2-freertos-stm32c562nucleo/SKILL.md) | STM32CubeMX2 1.2.0 + HAL2 + FreeRTOS 11.3.1 (ST pack 2.2.0) + CMake | ST STM32C5 | [NUCLEO-C562RE](boards/nucleo-c562re/README.md) | [guide](skills/cubemx2-freertos-stm32c562nucleo/README.md) · [ไทย](skills/cubemx2-freertos-stm32c562nucleo/README_TH.md) |
| [cubemx-freertos-stm32f303disco](skills/cubemx-freertos-stm32f303disco/SKILL.md) | STM32CubeMX 6.18 + STM32CubeF3 HAL + FreeRTOS V10.0.1 + CMake | ST STM32F3 | [STM32F3DISCOVERY](boards/stm32f3-discovery/README.md) | [guide](skills/cubemx-freertos-stm32f303disco/README.md) · [ไทย](skills/cubemx-freertos-stm32f303disco/README_TH.md) |

The same boards' bare-metal skills: `cubemx2-hal2-stm32c562nucleo` and `cubemx-hal-stm32f303disco` in
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

## Contributing

Run `python tools/validate_skills.py` before a commit (PASS required). See CONTRIBUTING.md.

## License

MIT (LICENSE). Vendor code generated into apps keeps its own licences (FreeRTOS MIT, STM32Cube HAL
BSD-3-Clause).
