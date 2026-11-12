# lib - code shared by every skill

Copies from [automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills)
(split 2026-11-12; no shared code between the repositories - a fix there is copied here by hand).

| Path | Used by | What |
| --- | --- | --- |
| `common.sh` | every `skills/*/scripts/env.sh` | board profile lookup (project before repo), path safety, `need_user` (`ACTION:` + exit 10), `.gitignore` merging, bench lock, flash approval |
| `fwtest.py` | every `skills/*/scripts/serial_test.py` (thin wrapper) | Stage 6 engine: data-driven serial tests from `tests/*.json` |
| `fwdebug.py` | `debug.sh` | Stage 7: fault decode and the GDB/MI scenario runner (break / watch / step / print) |
| `fw_workspace.sh` | the developer / agent, once per PC (`new_app.sh` asks with `ACTION: CHOOSE`) | this PC's apps workspace: `show`, `set <dir>`, `unset` (`~/.fw-skills.env`, never in git; shared with the other firmware-skills repositories) |
| `fwpaths.py` | `fwtest.py` | the same apps-workspace and `.bench/` rules as `common.sh`, for Python |
| `func/` | `new_app.sh` copies it into each app | logic layer in portable C: console, led, button, gesture - depends only on `board.h` (never on RTOS headers) |
| `check_layers.py` | stage 9 (M2), any skill's app | include / vendor-call / `#if BOARD_*` rules per layer; RTOS kernel headers allowed in the execution layer and board/, FAIL in func/: `LAYERS: PASS` |
| `host_test.sh`, `host/` | stage 10 (M2, optional) | builds `func/` with the fake board `host/board.h` and runs `host/test_func.c` on the PC: `HOST: PASS` |

`func/` is the portable middle layer (the `board.h` contract is docs/board-api.md): it may include
only `board.h` and the C library. Tasks, queues, notifications and delays belong in the execution
layer, so the same services run under bare metal and under FreeRTOS.
