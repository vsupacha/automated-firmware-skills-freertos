# Stage 2e - Layered code (FreeRTOS execution model)

```
src/app_main.c      execution: app_main() creates the tasks, the command table, the policies
src/app_rtos.c/.h   execution helpers: task registry, console task, "tasks" command, run
  │ uses
src/func/*.c/.h     logic: console (line editor + dispatch), led (1-based, read-back), button
  │ uses             (debounce, events) - identical to the bare-metal skill's func/ (lib/func);
  │                  template logic too (blink-rate: func/blink_rate.c). Never RTOS calls.
src/board/board.*   board adaptation: GPIO ports/pins, HAL2 + LL calls, BOARD_xxx #ifdefs,
  │ uses             console UART (TX via HAL under a mutex, RX interrupt -> ring), _write() for
  │                  printf, board_delay_ms() = vTaskDelay() once the scheduler runs
mx/ (generated)     mx_system_init(): clocks, USART2 + pins, TIM6 HAL tick, HAL init;
                    middleware/freertos (kernel 11.3.1, port ARM_CM33_NTZ, Heap_4),
                    generated/middleware/FreeRTOSConfig.h + mx_freertos_app.c (hooks)
```

Call chain: generated `main()` → `mx_system_init()` → `app_main()` (inserted by `regen.sh`) →
`app_rtos_task()` ... → `app_rtos_run()` = `vTaskStartScheduler()` (never returns).

Rules
- Dependency direction is strictly downward; `func/` and the execution layer never include HAL/LL
  headers. RTOS kernel headers (`FreeRTOS.h`, `task.h`, `queue.h`, `semphr.h` ...) are the
  execution layer's: allowed in `app_main.c` / `app_rtos.*` (and `board/`), **never in `func/`**
  (`lib/check_layers.py` fails it). A `func/` service stays init/poll - a task calls `*_poll()` on
  its own period - so the same service runs under the bare-metal skill.
- A board difference goes in `board/` (an `#elif` user-I/O table) or in the board's `.ioc2`
  configuration - never `#ifdef` in `func/`.
- Hardware owned by CubeMX (clocks, peripherals, their pins, the HAL timebase, FreeRTOS settings)
  is configured in the `.ioc2` and used through the generated `mx_<periph>_..._gethandle()`;
  simple user I/O may be configured at run time in `board.c` (documented there).
- `src/**/*.c` is picked up automatically (CMake glob, re-configured on new files); folders under
  `src/` are include paths only for `src/`, `src/board`, `src/func` - include others relatively.
- Everything in `src/` is compiled with `-Wall -Wextra`; the build gate allows 0 warnings there.

## Tasks

- Create every task with `app_rtos_task(fn, name, stack_words, prio, arg)` - it is listed by the
  `tasks` command and stops the board with `FATAL cannot create task` when the heap is too small.
- Stack sizes are in **words**. A task that calls `printf()` needs `APP_RTOS_STACK_PRINTF` (512).
  Check the margin on the board: `tasks` → `stack_free=<words>` (the specs want >= 50).
- Every task loop must block - `vTaskDelayUntil()` for periodic work (no drift), a notification /
  queue wait for events. A loop that only polls starves every lower priority task, the console too.
- Events between tasks: `xTaskNotifyGive()` / `ulTaskNotifyTake()` for a counted "something
  happened" (blink-rate), a queue when data travels with it.
- Printing: one `printf()` per line from any task (`board/` locks the UART per write); build a
  line in a buffer first when it has several parts (push-to-light `print_states()`). Never print
  from an interrupt handler.
- Time: `board_millis()` (HAL tick, TIM6) for time stamps; FreeRTOS ticks (1 ms) for delays.

## Console protocol (test contract)

| Line | Meaning |
| --- | --- |
| `READY` | boot finished, accepting commands (printed after `INFO ...`) |
| `INFO app=<name> v=<ver> board=<id> ... rtos=FreeRTOS V<x>` | identity - the test checks `board=<id>` |
| `OK ...` / `ERR <reason>` | command result |
| `EVT ...` | asynchronous event (`EVT btn1=pressed name=B1 pin=PC13 led1=1`) |
| `<TAG> k=v ...` | state report (`LED led1=0`, `BTN btn1=0`, `PERIOD ms=500`) |
| `TASK name=<n> state=<s> prio=<p> stack_free=<words>` | one per task, answer to `tasks` |
| `HEAP free=<b> min=<b> total=<b> tasks=<n>` | last line of `tasks` (`tasks` = all kernel tasks incl. idle and timer) |

## Adding a peripheral (recipe)

1. Enable it in `<app>.ioc2` (STM32CubeMX2 GUI, or `mx peripherals enable` / `mx pinout ...`).
   Its interrupt priority must be numerically >= 5 (`library_max_syscall_interrupt_priority`) if
   its handler calls a `...FromISR()` FreeRTOS function.
2. `regen.sh <app>` - the generated `mx_<periph>.c/.h` appear under `mx/generated/hal/`.
3. Wrap it in `board/` (`board_xxx()` using the generated handle getter).
4. Add a `func/xxx.c/.h` service if it is reusable (init/poll, no RTOS); give it a task or call
   its poll from an existing task in `app_main.c`; register console commands there.
5. Add test steps to `tests/<spec>.json` (named after the template); build, flash, test.
