#!/usr/bin/env bash
# Command menu of the cubemx2-freertos-stm32c562nucleo skill: what you can ask Claude, with short examples,
# the script behind each request, and the boards/templates installed right now.
# Usage: help.sh [--en]      (Thai by default)

. "$(dirname "$0")/env.sh"
LANG_EN=0; [ "$1" = "--en" ] && LANG_EN=1

boards() {
  for id in $(known_boards); do
    f="$(board_profile_dir "$id")/board.env"
    name="$(sed -n 's/^BOARD_NAME="\{0,1\}\([^"#]*\)"\{0,1\}.*/\1/p' "$f" | head -1)"
    src=""; case "$f" in "$BOARDS_DIR"/*) ;; *) src="  [project boards/]";; esac
    printf '    %-14s %s%s\n' "$id" "$name" "$src"
  done
}
templates() {
  for d in "$SKILL_DIR"/templates/*/; do
    t="$(basename "$d")"; [ "${t#_}" = "$t" ] || continue
    printf '    %-14s %s\n' "$t" "$(head -1 "$d/description.txt" 2>/dev/null | tr -d '\r')"
  done
}

if [ $LANG_EN = 1 ]; then cat <<'EOF'
skill cubemx2-freertos-stm32c562nucleo (STM32C5 / NUCLEO-C562RE, FreeRTOS + STM32CubeMX2 + CMake) - ask Claude in your own words:
  (<app> in the later stages = the app folder new_app.sh prints, e.g. apps/my-app or C:/fw-ws/my-app)
 Set up / check
  "check my tools"                          -> check_tools.sh <board>
  "find my board"                           -> discover.sh <board>   (read-only, saves ST-LINK serial + COM port)
 Create and build (no board needed)
  "create a FreeRTOS hello-world app"       -> new_app.sh <board> <app> "" hello-world   (FreeRTOS + HAL tick on TIM6 in the .ioc2)
  "two tasks: B1 changes the blink rate"    -> new_app.sh <board> <app> "" blink-rate
  "start from my own .ioc2"                 -> new_app.sh <board> <app> "" <template> --ioc2 <file>  (must hold FreeRTOS + a TIM timebase)
  "I changed the .ioc2 - regenerate"        -> regen.sh <app>
  "add a task"                              -> app_rtos_task() in src/app_main.c (tasks only there; logic in src/func, pins in src/board)
  "open it in VS Code"                      -> open_ide.sh <app>  (VS Code + STM32CubeIDE extension; new_app does it)
     then choose: code + build/flash/debug yourself in VS Code (IDE path), or let Claude run build/flash/test (script path) - switch any time
  "build it"                                -> build.sh <app>
 Run on hardware (Claude asks before every flash)
  "flash it"                                -> flash.sh <app> [--yes]   (ST-LINK, verify after write)
  "test it" / "test with button presses"    -> serial_test.py auto <spec> <log> --board <b> [--interactive]  (checks tasks, stack, heap too)
  "is the board OK?" (no halt)              -> debug.sh status <board>
 Housekeeping
  "clean the build outputs"                 -> clean.sh [--apps]  (dry run, then --yes; --apps deletes apps)
  "add a new board" / "add a template"      -> boards/_template/, SKILL.md "Extending"
EOF
else cat <<'EOF'
skill cubemx2-freertos-stm32c562nucleo (STM32C5 / NUCLEO-C562RE, FreeRTOS + STM32CubeMX2 + CMake) - สั่ง Claude ได้ด้วยภาษาของคุณเอง ตัวอย่าง:
  (<app> ในขั้นหลัง new_app.sh = โฟลเดอร์แอปที่ new_app.sh พิมพ์ออกมา เช่น apps/my-app หรือ C:/fw-ws/my-app)
 เตรียม / ตรวจสอบ
  "ตรวจเครื่องมือให้หน่อย"                       -> check_tools.sh <board>
  "หาบอร์ดที่ต่ออยู่"                             -> discover.sh <board>   (อ่านอย่างเดียว บันทึก ST-LINK serial + COM port)
 สร้างและ build (ไม่ต้องมีบอร์ด)
  "สร้างแอป FreeRTOS hello-world"              -> new_app.sh <board> <app> "" hello-world   (.ioc2 มี FreeRTOS + HAL tick ที่ TIM6)
  "สองงาน: กด B1 เปลี่ยนความเร็วไฟกะพริบ"       -> new_app.sh <board> <app> "" blink-rate
  "เริ่มจากไฟล์ .ioc2 ของฉัน"                     -> new_app.sh <board> <app> "" <template> --ioc2 <file>  (ต้องมี FreeRTOS + timebase เป็น TIM)
  "แก้ .ioc2 แล้ว generate ใหม่"                 -> regen.sh <app>
  "เพิ่ม task"                                   -> app_rtos_task() ใน src/app_main.c (task อยู่ที่นี่เท่านั้น; logic ใน src/func, ขาใน src/board)
  "เปิดใน VS Code"                              -> open_ide.sh <app>  (VS Code + STM32CubeIDE extension; new_app เปิดให้เอง)
     จากนั้นเลือกได้: เขียนโค้ด + build/แฟลช/debug เองใน VS Code (ทาง IDE) หรือให้ Claude รัน build/flash/test (ทางสคริปต์) - สลับได้ทุกเมื่อ
  "build ให้หน่อย"                              -> build.sh <app>
 ใช้กับบอร์ดจริง (Claude จะถามก่อน flash ทุกครั้ง)
  "แฟลชเลย"                                    -> flash.sh <app> [--yes]   (ผ่าน ST-LINK, verify หลังเขียน)
  "ทดสอบ" / "ทดสอบแบบกดปุ่ม"                    -> serial_test.py auto <spec> <log> --board <b> [--interactive]  (ตรวจ task, stack, heap ด้วย)
  "บอร์ดยังปกติไหม" (ไม่หยุด CPU)                -> debug.sh status <board>
 งานอื่น
  "clean ให้หน่อย"                              -> clean.sh [--apps]  (ลบผล build; --apps ลบแอปด้วย, แสดงรายการก่อน)
  "เพิ่มบอร์ดใหม่" / "เพิ่ม template"             -> boards/_template/, SKILL.md หัวข้อ Extending
EOF
fi
if [ $LANG_EN = 1 ]; then echo " Active milestones: $(fw_active_milestones)"
else echo " milestone ที่เปิดใช้: $(fw_active_milestones)"; fi
[ $LANG_EN = 1 ] && echo " Boards installed:" || echo " บอร์ดที่มี:"
boards
[ $LANG_EN = 1 ] && echo " App templates:" || echo " template แอป:"
templates
[ $LANG_EN = 1 ] && echo " Apps workspace: $CUBE_WS" || echo " โฟลเดอร์แอป: $CUBE_WS"
