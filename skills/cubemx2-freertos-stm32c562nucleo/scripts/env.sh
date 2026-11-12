# Common environment for cubemx2-freertos-stm32c562nucleo scripts. Source it: . "$(dirname "$0")/env.sh"
# Windows + Git Bash (STM32Cube bundles for Windows: .exe tools, taskkill, cygpath).
# Run scripts from Git Bash or Claude's Bash tool - in PowerShell, "bash" may start WSL instead.
# Shared helpers (board lookup, paths, need_user/exit 10, .gitignore): <repo>/lib/common.sh

SKILL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SKILL_NAME="$(basename "$SKILL_DIR")"           # cubemx2-freertos-stm32c562nucleo: profile subfolder in boards/<id>/
IDE_EXT=stmicroelectronics.stm32-vscode-extension   # stage 2d: VS Code extension that opens, builds, flashes and debugs the app
IDE_NAME="STM32CubeIDE for Visual Studio Code"
REPO_ROOT="$(cd "$SKILL_DIR/../.." && pwd)"
# shellcheck disable=SC1091
. "$REPO_ROOT/lib/common.sh" || { echo "ERROR: $REPO_ROOT/lib/common.sh missing - keep the repo layout" >&2; exit 1; }
PATH_EXAMPLE="C:/stm32-ws"

# Where apps go (CUBE_WS): the per-PC workspace (FW_WS / ~/.fw-skills.env, lib/fw_workspace.sh), else
# <repo>/apps inside the repo checkout, else ./apps.
# Bench files: this PC's ST-LINK serial + VCP COM port.
CUBE_WS="${CUBE_WS:-$(fw_default_ws)}"
BENCH_DIR="${CUBE_BENCH_DIR:-$CUBE_WS/.bench}"
fw_init_boards "$CUBE_WS" "$CUBE_BOARDS_DIR"

# STM32Cube root (bundles/ + packs/) - installed by STM32CubeMX2 / the STM32Cube VS Code extension
if [ -z "$STM32CUBE_ROOT" ] && [ -n "$LOCALAPPDATA" ]; then STM32CUBE_ROOT="$(cygpath -u "$LOCALAPPDATA")/stm32cube"; fi
PYTHON="${PYTHON:-$(command -v python || command -v python3)}"

# set_tools: tool paths from the pinned versions of the loaded board profile
set_tools() {
  local B="$STM32CUBE_ROOT/bundles"
  MX_DIR="$B/stm32cubemx-application/$MX_VERSION"
  MX_NODE="$MX_DIR/bundles/node/$NODE_VERSION/node/bin/node.exe"
  MX_CLI="$MX_DIR/dist/bin.js"
  GCC_BIN="$B/gnu-tools-for-stm32/$GCC_VERSION/bin"
  CMAKE_BIN="$B/cmake/$CMAKE_VERSION/bin"
  NINJA_BIN="$B/ninja/$NINJA_VERSION/bin"
  PROGRAMMER="$B/programmer/$PROGRAMMER_VERSION/bin/STM32_Programmer_CLI.exe"
  PRESET="${BUILD_TYPE:-debug}_GCC_${MX_BOARD_CPN}"
  BUILD_TARGET=".${BUILD_TYPE:-debug}_GCC+${MX_BOARD_CPN}"
  export CMSIS_PACK_ROOT; CMSIS_PACK_ROOT="$(cygpath -w "$STM32CUBE_ROOT/packs")"
}

# load_board <id>: board profile + bench file, env overrides, pinned tool paths
load_board() {
  fw_load_profile "$1"
  [ -n "$CUBE_PROBE_SERIAL" ] && PROBE_SERIAL="$CUBE_PROBE_SERIAL"
  [ -n "$CUBE_CONSOLE" ] && CONSOLE_PORT="$CUBE_CONSOLE"
  set_tools
  return 0
}

# ---- STM32CubeMX2 headless backend ---------------------------------------------------------
# The CLI talks to a backend on a TCP port. mx_start launches one (optionally with a project),
# mx_stop kills its whole process tree (the backend has no stop command).
MX_PORT="${MX_PORT:-}"
mx() {   # mx <command...>: run the CLI against our backend, drop the licence notice line
  "$MX_NODE" "$MX_CLI" "$@" --port "$MX_PORT" 2>&1 | tr -d '\r' | grep -v "^By executing these commands"
}
mx_ok() { grep -q '"status": *"success"'; }
free_port() {
  local p
  for p in $(seq 51240 51299); do
    netstat -ano 2>/dev/null | grep -qE "[:.]$p[[:space:]].*LISTEN" || { echo "$p"; return 0; }
  done
  return 1
}
# mx_start <log> [ioc2]: start the backend, wait until it answers (needs the project for status)
mx_start() {
  local log="$1" ioc2="$2" i
  [ -f "$MX_CLI" ] && [ -x "$MX_NODE" ] || die "STM32CubeMX2 $MX_VERSION not found under $MX_DIR - run check_tools.sh"
  MX_PORT="$(free_port)" || die "no free TCP port for the STM32CubeMX2 backend"
  if [ -n "$ioc2" ]; then
    "$MX_NODE" "$MX_CLI" start --no-detached --port "$MX_PORT" --log-level warn "$(winpath "$ioc2")" >"$log" 2>&1 &
  else
    "$MX_NODE" "$MX_CLI" start --no-detached --port "$MX_PORT" --log-level warn >"$log" 2>&1 &
  fi
  MX_LAUNCHER=$!
  MX_WINPID="$(cat /proc/$MX_LAUNCHER/winpid 2>/dev/null)"
  trap mx_stop EXIT
  for i in $(seq 1 90); do
    if [ -n "$ioc2" ]; then
      mx ide-project status --project-path "$(winpath "$ioc2")" | mx_ok && return 0
    else
      netstat -ano 2>/dev/null | grep -qE "[:.]$MX_PORT[[:space:]].*LISTEN" && { sleep 3; return 0; }
    fi
    kill -0 "$MX_LAUNCHER" 2>/dev/null || die "STM32CubeMX2 backend exited during start (log $log)"
    sleep 1
  done
  die "STM32CubeMX2 backend not ready after 90 s (log $log)"
}
# mx_set <ioc2> <sw> <resource> <path> <value> <log>: sw-config set + read back (the CLI answers
# "success" even with a warning, so the value is checked, not the status)
mx_set() {
  local i; i="$(winpath "$1")"
  mx sw-config set-parameter-value -p "$i" -s "$2" -r "$3" --parameter-path "$4" -v "$5" >>"$6"
  mx sw-config get-parameter-value -p "$i" -s "$2" -r "$3" --parameter-path "$4" | tee -a "$6" \
    | grep -qE "\"data\": *\"?$5\"?[[:space:]]*$" || { echo "ERROR: $3 $4 is not $5 after setting it (log $6)" >&2; return 1; }
}
# mx_rtos_setup <ioc2> <sw-project> <log>: the FreeRTOS execution model, from the board profile
#   MX_RTOS_COMPONENT  CMSIS component id of the FreeRTOS config (pack MX_MIDDLEWARE): added with
#                   `composer add-component` + `resolve-dependencies` (pulls FreeRTOS Core). Not
#                   `middleware enable-middleware`: MX 1.2.0 rates FreeRTOS "incompatible" in every
#                   backend session except the one that installed the pack. The pack must be
#                   installed (check_tools: PACKS)
#   MX_TIMEBASE_TIM the HAL tick moves to this basic timer: FreeRTOS owns SysTick. HAL2 offers only
#                   TIM instances that are already enabled (no LPTIM), so enable it first
#   MX_SWCONFIG     <resource>|<path>|<value> kernel settings (FPU context save, heap, stack check)
mx_rtos_setup() {
  local ioc2="$1" sw="$2" log="$3" i out item r p v
  i="$(winpath "$ioc2")"
  if ! mx composer get-rtos -p "$i" -s "$sw" | grep -q '"name": *"FreeRTOS"'; then
    out="$(mx composer add-component -p "$i" -s "$sw" -c "$MX_RTOS_COMPONENT")"
    echo "$out" >>"$log"
    echo "$out" | mx_ok || { echo "ERROR: add FreeRTOS ($MX_RTOS_COMPONENT) failed: $(echo "$out" | tail -3)" >&2
      echo "  is the pack installed? check_tools.sh <board> (ACTION: SETUP names the command)" >&2; return 1; }
  fi
  out="$(mx composer resolve-dependencies -p "$i" -s "$sw")"; echo "$out" >>"$log"
  echo "$out" | mx_ok || { echo "ERROR: resolve-dependencies: $out" >&2; return 1; }
  out="$(mx composer list-project-fixes -p "$i" -s "$sw")"; echo "$out" >>"$log"
  echo "$out" | tr -d ' \n' | grep -q '"data":{}' || { echo "ERROR: open component fixes after adding FreeRTOS: $out" >&2; return 1; }
  mx composer get-rtos -p "$i" -s "$sw" | grep -q '"name": *"FreeRTOS"' || { echo "ERROR: FreeRTOS not in the project after add-component" >&2; return 1; }
  out="$(mx peripherals enable -p "$i" --peripheral "$MX_TIMEBASE_TIM" --software-project "$sw")"
  echo "$out" >>"$log"
  echo "$out" | mx_ok || echo "$out" | grep -q "already enabled" || { echo "ERROR: enable $MX_TIMEBASE_TIM: $out" >&2; return 1; }
  mx_set "$ioc2" "$sw" HALCore common/timebase_feature/hal_timebase_selection TIM "$log" || return 1
  mx_set "$ioc2" "$sw" HALCore common/timebase_feature/timer_selection "$MX_TIMEBASE_TIM" "$log" || return 1
  for item in $MX_SWCONFIG; do
    IFS='|' read -r r p v <<<"$item"
    mx_set "$ioc2" "$sw" "$r" "$p" "$v" "$log" || return 1
  done
  return 0
}
mx_stop() {
  [ -n "$MX_WINPID" ] && taskkill //PID "$MX_WINPID" //T //F >/dev/null 2>&1
  [ -n "$MX_LAUNCHER" ] && kill "$MX_LAUNCHER" 2>/dev/null
  MX_WINPID=""; MX_LAUNCHER=""
  return 0
}

# ---- ST-LINK ----------------------------------------------------------------------------------
# probe_serials: serial numbers of connected ST-LINKs (STM32_Programmer_CLI -l st-link)
probe_serials() {
  "$PROGRAMMER" -l st-link 2>&1 | tr -d '\r' | sed -n 's/.*ST-LINK SN *: *\([0-9A-Fa-f]*\).*/\1/p'
}
# find_console_port <probe_serial>: VCP COM port of that ST-LINK (the VCP's USB serial = probe SN)
find_console_port() {
  [ -n "$1" ] || return 0
  "$PYTHON" - "$1" "${PROBE_VID:-0483}" <<'EOF'
import sys
from serial.tools import list_ports
want, vid = sys.argv[1].upper(), int(sys.argv[2], 16)
for p in list_ports.comports():
    if p.vid == vid and (p.serial_number or "").upper() == want:
        print(p.device); break
EOF
}
resolve_console() {
  CONSOLE="$(find_console_port "$PROBE_SERIAL" 2>/dev/null | tr -d '\r')"
  [ -n "$CONSOLE" ] || CONSOLE="$CONSOLE_PORT"
}
# probe_identity <probe_serial>: read-only hot-plug connect; prints the CLI output
probe_identity() {
  "$PROGRAMMER" -c port=SWD sn="$1" mode=HOTPLUG 2>&1 | tr -d '\r'
}

# write_workspace_gitignore <workspace>: this skill's per-PC / regenerable files
write_workspace_gitignore() {
  # */.vscode/, */*.code-workspace: IDE files with per-PC paths, re-made by open_ide.sh / the extension
  fw_gitignore "$1" "$SKILL_NAME" ".bench/" "*/mx/" "*/.mx/" "*/logs/" "*/.vscode/" "*/*.code-workspace"
}
