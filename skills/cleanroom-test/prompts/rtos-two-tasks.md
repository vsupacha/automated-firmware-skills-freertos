You are a new firmware developer on this repo (D:/Dropbox/workspace/Firmware_skills/automated-firmware-skills,
Windows, Git Bash + Python available, STM32CubeMX2 1.2.0 and the STM32Cube bundles installed, ST's FreeRTOS
pack 2.2.0 already in the STM32Cube pack folder). You know nothing about this repo beyond what its docs say.
Your goal is to test whether the documented workflow for the NUCLEO-C562RE skill works with STM32CubeMX2
1.2.0 when a developer adds an RTOS from the CubeMX middleware.

Task: build a FreeRTOS app for the NUCLEO-C562RE:
- create the app with the skill (start from whichever template the docs suggest fits best);
- add FreeRTOS as STM32CubeMX2 middleware (in the app's .ioc2, the way the docs say hardware/middleware
  changes are made; the STM32CubeMX2 command line is fine - use its --help where the docs stop);
- two tasks:
  - buttonTask: reads the USER button (B1) every 100 ms; on each press it sends a "toggle" event to ledTask;
  - ledTask: blinks LED LD1; its half-period starts at 1 s and each event steps it 1 s -> 0.5 s -> 0.2 s ->
    back to 1 s;
  - print a console line on every period change so a test can see it (e.g. "PERIOD ms=500");
- put the code in the layers the docs define (logic vs board vs app), and keep generated code untouched;
- IDE handoff with --no-open, then build until BUILD: PASS with 0 warnings in your code.

Rules:
- Use ONLY what the docs tell you: start from README.md in the repo root, then boards/README.md and the
  skill files it points to (skills/cubemx2-hal2-stm32c562nucleo/SKILL.md and what it references, docs/).
  Do not search the conversation history or other sessions, and do NOT read CLAUDE.local.md (private
  notes a new developer does not have).
- Choices made in advance: board nucleo-c562re, skill cubemx2-hal2-stm32c562nucleo, app name
  rtos-clean-test, apps workspace = this PC's chosen one (D:/fw-ws, already set).
- STOP after the build: do NOT flash, do NOT run discover/serial tests/debug (no board access at all).
  Also write the test spec you would use for stage 6 (tests/<name>.json in the app) - it will be run later.
- Do NOT modify anything under skills/, boards/, lib/, docs/, tools/, milestones.env or the README files,
  and do not change git or tool settings. Work only in D:/fw-ws/rtos-clean-test and your own scratch
  files. Install nothing (no packs, no bundles): on "ACTION: SETUP" or a missing pack stop that stage
  and report it. If the docs are wrong or a script fails, work around it if you can (inside your app),
  and record it.
- Do not commit anything.

Report back (concise, as your final message):
1. PASS/FAIL per stage: tools check, create, add FreeRTOS, regen, code change (layers), IDE handoff,
   build, test spec written.
2. The gate lines you got (missing/bad=, Created, REGEN:, IDE: READY, BUILD: with warnings/errors) and
   the ELF sha256 from the manifest.
3. Every point where the docs were missing, unclear, misleading or wrong, or a command failed - with
   the exact command/output and what you had to guess. This is the most important part. Include what
   the generator changed in main.c / CMakeLists.txt when FreeRTOS was added and whether the skill's
   hook into src/ still worked.
4. Which files you wrote in which layer and why, and where you put the FreeRTOS task code.
5. Time spent per stage roughly.
