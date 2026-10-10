#!/bin/bash
# OpenUG2 - H700 PortMaster launcher. Edit values below without recompiling.
RESOLUTION="640x480"
WORLD_RADIUS="500"
TEXTURE_CACHE_MB="96"
TRACK="STREAML4RA"
CAR="HUMMER"
TRAFFIC="0"

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
GAMEDIR=""
for candidate in \
    /mnt/sdcard/ports/openug2 \
    /mnt/mmc/ports/openug2 \
    /mnt/SDCARD/ports/openug2 \
    /mnt/sdcard/roms/ports/openug2 \
    /mnt/mmc/roms/ports/openug2 \
    "$SCRIPT_DIR/openug2" \
    "$SCRIPT_DIR"; do
    if [ -x "$candidate/nfsu2" ]; then GAMEDIR="$candidate"; break; fi
done
if [ -z "$GAMEDIR" ]; then
    echo "OpenUG2: ARM64 executable not found in ports/openug2." >&2
    exit 1
fi

# Some PortMaster platforms have these files; direct shell launches do not.
if [ -f /opt/system/Tools/PortMaster/control.txt ]; then
    source /opt/system/Tools/PortMaster/control.txt
    if declare -F get_controls >/dev/null 2>&1; then get_controls; fi
fi
if [ -f /opt/system/Tools/PortMaster/portmaster.sh ]; then
    source /opt/system/Tools/PortMaster/portmaster.sh
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
if [ -d "$GAMEDIR/libs" ]; then
    export LD_LIBRARY_PATH="$GAMEDIR/libs:$LD_LIBRARY_PATH"
fi
# Don't force SDL_VIDEODRIVER; allow the firmware to select the H700 backend.
"$GAMEDIR/nfsu2" "$DATA" \
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
