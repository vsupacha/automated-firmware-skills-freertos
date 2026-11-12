#!/usr/bin/env python3
"""Hook the layered app (<app>/src) into the STM32CubeMX-generated project (<app>/mx).

Usage: python overlay.py <mx-dir> <BOARD_DEFINE>

Idempotent (re-run after every regeneration). Edits only USER CODE sections of mx/Src/main.c and
the user sections of mx/CMakeLists.txt (a file CubeMX generates once):
  main.c         #include "board.h" + "app_main.h"; app_main() after the MX_..._Init() calls
                 (USER CODE BEGIN 2)
  CMakeLists.txt every ../src/**/*.c, include paths src, src/board, src/func, -D<BOARD_DEFINE>,
                 -Wall -Wextra for those sources only
Exit 0 = done, 1 = a marker was not found (a new CubeMX changed the generated files - update this
script, never edit mx/ by hand).
"""
import re
import sys
from pathlib import Path

TAG = "fw-skill overlay"


def fail(msg):
    print(f"OVERLAY: FAIL - {msg}")
    sys.exit(1)


def insert_after(text, marker, block, what):
    if block.strip() in text:
        return text
    if marker not in text:
        fail(f"marker '{marker}' not found ({what})")
    return text.replace(marker, marker + "\n" + block, 1)


def edit(path, fn):
    raw = path.read_bytes().decode("utf-8")
    nl = "\r\n" if "\r\n" in raw else "\n"
    path.write_bytes(fn(raw.replace("\r\n", "\n")).replace("\n", nl).encode("utf-8"))


def main(argv):
    if len(argv) != 2:
        print(__doc__.strip())
        return 1
    mx, define = Path(argv[0]), argv[1]
    main_c, cml = mx / "Src" / "main.c", mx / "CMakeLists.txt"
    for f in (main_c, cml):
        if not f.is_file():
            fail(f"{f} missing - generation did not produce the project")

    def main_hooks(s):
        s = insert_after(s, "/* USER CODE BEGIN Includes */",
                         f'#include "board.h"     /* {TAG} */\n#include "app_main.h"  /* {TAG} */', "main.c includes")
        return insert_after(s, "/* USER CODE BEGIN 2 */",
                            f"  app_main();           /* {TAG}: layered app, does not return */", "main.c 2")

    def cmake_hooks(s):
        glob = (f"# {TAG}: the layered app sources (<app>/src), warnings on for them only\n"
                "file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../src/*.c)\n"
                "set_source_files_properties(${APP_SOURCES} PROPERTIES COMPILE_OPTIONS \"-Wall;-Wextra\")")
        if glob not in s:
            m = re.search(r"^# Add sources to executable", s, re.M)
            if not m:
                fail("'# Add sources to executable' not found (CMakeLists.txt)")
            s = s[:m.start()] + glob + "\n\n" + s[m.start():]
        s = insert_after(s, "    # Add user defined symbols", f"    {define}  # {TAG}", "CMakeLists symbols")
        s = insert_after(s, "    # Add user defined include paths",
                         f"    ${{CMAKE_CURRENT_SOURCE_DIR}}/../src  # {TAG}\n"
                         "    ${CMAKE_CURRENT_SOURCE_DIR}/../src/board\n"
                         "    ${CMAKE_CURRENT_SOURCE_DIR}/../src/func", "CMakeLists includes")
        return insert_after(s, "    # Add user sources here", f"    ${{APP_SOURCES}}  # {TAG}", "CMakeLists sources")

    def rtos_hooks(s):
        # STM32CubeMX generates vApplicationStackOverflowHook() with an empty body (USER CODE 4):
        # a task whose stack overflowed would run on - stop instead (interrupts off, endless loop)
        marker = "called if a stack overflow is detected. */"
        block = (f"   (void)xTask;            /* {TAG}: stop - the stack of task pcTaskName overflowed */\n"
                 "   (void)pcTaskName;\n"
                 "   taskDISABLE_INTERRUPTS();\n"
                 "   for (;;)\n"
                 "   {\n"
                 "   }")
        return insert_after(s, marker, block, "freertos.c stack overflow hook")

    edit(main_c, main_hooks)
    edit(cml, cmake_hooks)
    rtos_c = mx / "Src" / "freertos.c"
    if not rtos_c.is_file():
        fail(f"{rtos_c} missing - the .ioc has no FreeRTOS (MX_CONFIG: set mode FREERTOS CMSIS_V2)")
    edit(rtos_c, rtos_hooks)
    print("OVERLAY: done (main.c hooks, CMakeLists.txt sources/includes/define, freertos.c overflow hook)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
