#!/bin/bash
# OpenUG2 - H700 PortMaster launcher. Edit values below without recompiling.
RESOLUTION="640x480"
WORLD_RADIUS="500"
TEXTURE_CACHE_MB="96"
TRACK="STREAML4RA"
CAR="HUMMER"
TRAFFIC="0"

# PortMaster layout based on Detoy/OpenUG2 (R36S), plus H700 fallback paths.
# Native SDL2 handles the controller; GPTOKEYB is not required.
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
XDG_DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
CONTROLFOLDER=""
for candidate in "${PORTMASTER_CONTROL_DIR:-}" \
    /opt/system/Tools/PortMaster \
    /opt/tools/PortMaster \
    "$XDG_DATA_HOME/PortMaster" \
    /roms/ports/PortMaster \
    /roms2/ports/PortMaster; do
    if [ -n "$candidate" ] && [ -f "$candidate/control.txt" ]; then
        CONTROLFOLDER="$candidate"
        break
    fi
done
if [ -n "$CONTROLFOLDER" ]; then
    source "$CONTROLFOLDER/control.txt"
    if [ -n "${CFW_NAME:-}" ] && [ -f "$CONTROLFOLDER/mod_$CFW_NAME.txt" ]; then
        source "$CONTROLFOLDER/mod_$CFW_NAME.txt"
    fi
    if declare -F get_controls >/dev/null 2>&1; then get_controls; fi
fi
if [ -f /opt/system/Tools/PortMaster/portmaster.sh ]; then
    source /opt/system/Tools/PortMaster/portmaster.sh
fi
PM_ROOT=""
if [ -n "${directory:-}" ]; then
    if [[ "$directory" == /* ]]; then PM_ROOT="$directory"
    else PM_ROOT="/$directory"; fi
fi
DEVICE_ARCH="${DEVICE_ARCH:-}"
GAMEDIR=""
for candidate in \
    "$SCRIPT_DIR/openug2" \
    "$SCRIPT_DIR" \
    "$PM_ROOT/ports/openug2" \
    /roms/ports/openug2 \
    /roms2/ports/openug2 \
    /mnt/sdcard/ports/openug2 \
    /mnt/mmc/ports/openug2 \
    /mnt/SDCARD/ports/openug2 \
    /mnt/sdcard/roms/ports/openug2 \
    /mnt/mmc/roms/ports/openug2; do
    [ -d "$candidate" ] || continue
    if [ -x "$candidate/nfsu2" ] ||
       { [ -n "$DEVICE_ARCH" ] && [ -x "$candidate/nfsu2.$DEVICE_ARCH" ]; }; then
        GAMEDIR="$candidate"
        break
    fi
done
if [ -z "$GAMEDIR" ]; then
    echo "OpenUG2: executable missing in ports/openug2" >&2
    exit 1
fi
BIN="$GAMEDIR/nfsu2"
if [ -n "$DEVICE_ARCH" ] && [ -x "$GAMEDIR/nfsu2.$DEVICE_ARCH" ]; then
    BIN="$GAMEDIR/nfsu2.$DEVICE_ARCH"
fi
if [ -n "${sdl_controllerconfig:-}" ]; then
    export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
fi
export XDG_DATA_HOME="$GAMEDIR/conf"
mkdir -p "$XDG_DATA_HOME" || exit 1
if [ -n "$DEVICE_ARCH" ] && [ -d "$GAMEDIR/libs.$DEVICE_ARCH" ]; then
    export LD_LIBRARY_PATH="$GAMEDIR/libs.$DEVICE_ARCH:${LD_LIBRARY_PATH:-}"
fi
if [ -d "$GAMEDIR/libs" ]; then
    export LD_LIBRARY_PATH="$GAMEDIR/libs:${LD_LIBRARY_PATH:-}"
fi

LOGDIR="$GAMEDIR/logs"
mkdir -p "$LOGDIR" "$GAMEDIR/saves" || exit 1
LOGFILE="$LOGDIR/OpenUG2.log"
exec >"$LOGFILE" 2>&1
printf 'OpenUG2 H700 launcher\n'
printf 'Resolution: %s; world load radius: %s metres\n' "$RESOLUTION" "$WORLD_RADIUS"
printf 'Game dir: %s\n' "$GAMEDIR"
date 2>/dev/null || true

if [[ ! "$RESOLUTION" =~ ^[0-9]+x[0-9]+$ ]]; then
    echo "Invalid RESOLUTION; expected WIDTHxHEIGHT"; exit 2
fi
IFS=x read -r W H <<< "$RESOLUTION"
if (( 10#$W < 320 || 10#$H < 240 )); then
    echo "Resolution too small"; exit 2
fi
if [[ ! "$WORLD_RADIUS" =~ ^[0-9]+$ ]]; then
    echo "WORLD_RADIUS must be a number"; exit 2
fi
if (( 10#$WORLD_RADIUS < 250 || 10#$WORLD_RADIUS > 4000 )); then
    echo "WORLD_RADIUS must be 250..4000 metres"; exit 2
fi

DATA="$GAMEDIR/game"
if [[ ! "$TEXTURE_CACHE_MB" =~ ^[0-9]+$ ]] || (( 10#$TEXTURE_CACHE_MB > 1024 )); then
    echo "TEXTURE_CACHE_MB must be 0..1024"; exit 2
fi
if [ ! -d "$DATA/TRACKS" ] || [ ! -d "$DATA/CARS" ]; then
    echo "Missing retail NFS Underground 2 data: TRACKS/CARS under $DATA"
    exit 3
fi

cd "$GAMEDIR" || exit 1
# Optional machine-readable game mods: only applied to an in-memory verified
# original UG2 catalog; GlobalB.lzc and career saves are never rewritten.
CAREER_EXTRA=()
if [ -f "$GAMEDIR/mods/career_rewards.cfg" ]; then
    CAREER_EXTRA+=(--career-mod "$GAMEDIR/mods/career_rewards.cfg")
fi
# Set this explicitly when multiple career races use the same map route.
# Example: --career-race S3_CIRCUIT_10 with --circuit .../Paths4081.bin.
CAREER_RACE_ID=""
CAREER_CIRCUIT=""
if [ -n "$CAREER_RACE_ID" ]; then
    CAREER_EXTRA+=(--career-race "$CAREER_RACE_ID")
fi
if [ -n "$CAREER_CIRCUIT" ]; then
    CAREER_EXTRA+=(--circuit "$CAREER_CIRCUIT")
fi
# Run firmware-specific platform helper if provided by PortMaster.
if declare -F pm_platform_helper >/dev/null 2>&1; then
    pm_platform_helper "$BIN"
fi
# Don't force SDL_VIDEODRIVER; let firmware select its EGL/GLES2 backend.
"$BIN" "$DATA" \
    "${CAREER_EXTRA[@]}" \
    --resolution "$RESOLUTION" \
    --world-radius "$WORLD_RADIUS" \
    --texture-cache-mb "$TEXTURE_CACHE_MB" \
    --career-save "$GAMEDIR/saves/career.dat" \
    --track "$TRACK" \
    --car "$CAR" \
    --traffic "$TRAFFIC" \
    --tier baseline \
    --vehicle-quality low \
    --weather-quality low \
    --texture-detail 1 \
    --no-road-reflections \
    --no-paint-shine \
    --rain 0 \
    --hud
status=$?
printf 'OpenUG2 exit status: %d\n' "$status"
if declare -F pm_finish >/dev/null 2>&1; then pm_finish; fi
exit "$status"
