# automated-firmware-skills-freertos (ภาษาไทย)

Agent skills สำหรับ firmware ที่ใช้ **FreeRTOS** พาโปรเจกต์ของบอร์ดตั้งแต่ "ติดตั้งเครื่องมือครบหรือยัง" จนถึง
"ทดสอบบนบอร์ดแล้ว" โดยมี gate ทุกขั้น แยกออกมาจาก
[automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills) เมื่อ 2026-11-12
repo เดิมเก็บเฉพาะ skill แบบ bare metal (superloop) ส่วน repo นี้เป็นของ RTOS เพราะ middleware ของ RTOS
เป็นส่วนหนึ่งของโปรเจกต์ที่ generate, HAL tick ย้ายออกจาก SysTick, แอปรันเป็น task และการทดสอบอ่านสถานะ
task กับ stack ด้วย

เอกสารหลักเป็นภาษาอังกฤษ: [README.md](README.md)

| บอร์ด | คู่มือ |
| --- | --- |
| ST NUCLEO-C562RE + FreeRTOS | [skills/cubemx2-freertos-stm32c562nucleo/README_TH.md](skills/cubemx2-freertos-stm32c562nucleo/README_TH.md) |

ติดตั้ง plugin:

```
/plugin marketplace add vsupacha/automated-firmware-skills-freertos
/plugin install firmware-freertos@automated-firmware-skills-freertos
```

skill แบบ bare metal ของบอร์ดเดียวกัน (`cubemx2-hal2-stm32c562nucleo`) อยู่ใน
[automated-firmware-skills](https://github.com/vsupacha/automated-firmware-skills)
