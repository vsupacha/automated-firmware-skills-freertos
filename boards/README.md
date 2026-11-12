# Supported boards

Each board folder has a README with the tools to install, the commands of its skill(s), the bring-up
demos, the hardware sheet (pins, console, quirks) and one profile per skill (`<skill>/board.env` +
a dated verification log). Machine-readable: [index.json](index.json).

## Boards and on-board I/O

| Board | MCU | Skill | Console (UART) | LED1 | BTN1 |
| --- | --- | --- | --- | --- | --- |
| [ST NUCLEO-C562RE](nucleo-c562re/README.md) | ST STM32C5 (STM32C562RET6) | `cubemx2-freertos-stm32c562nucleo` (FreeRTOS; bare metal: [automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills/blob/main/boards/nucleo-c562re/README.md)) | ST-LINK virtual COM port | LD1 (green) | B1 (USER) |
| [ST STM32F3DISCOVERY](stm32f3-discovery/README.md) | ST STM32F3 (STM32F303VCT6) | `cubemx-freertos-stm32f303disco` (FreeRTOS; bare metal: [automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills/blob/main/boards/stm32f3-discovery/README.md)) | SWO (no virtual COM port) | LD3 (red) | B1 (blue) |

## Bring-up demos

The same three templates in every skill, checked with the same test specs (`tests/*.json`):

| Demo | Proves | Needs |
| --- | --- | --- |
| `hello-world` | toolchain, clock, console: prints "hello, world" every 1 s | a console (UART, USB CDC, SWO) |
| `blink` | GPIO output: LED1 toggles every 0.5 s, reported as `BLINK led1=0\|1` | one LED |
| `push-to-light` | GPIO input + console commands: a button press toggles LED1 (`EVT` lines), console commands `led <n> on|off|toggle` + `led?`/`btn?` | one button + one LED |

<!-- board-status:begin -->
| Board | Skill | hello-world | blink | push-to-light |
| --- | --- | --- | --- | --- |
| [ST NUCLEO-C562RE](nucleo-c562re/README.md) | `cubemx2-freertos-stm32c562nucleo` | built | built | built |
| [ST STM32F3DISCOVERY](stm32f3-discovery/README.md) | `cubemx-freertos-stm32f303disco` | built | built | built |
<!-- board-status:end -->

Levels: unverified < built < flashed < tested (automatic test PASS) < interactive (a person pressed
a button) < observed (a person saw the LED). \* = verified under the earlier template name
`uart-btn-led` (renamed 2026-10-09, same code). The UNO R4 WiFi has no user button: its
push-to-light runs the console LED commands only.

## Add a board

Copy [_template](_template/README.md) to `boards/<id>/`, fill the hardware sheet and the skill
profile (`<skill>/board.env`), add the board to the skill's board layer, then run the demos. Your own
board in your own project: put the profile in `<project>/boards/<id>/<skill>/` - the skills look there
first.
