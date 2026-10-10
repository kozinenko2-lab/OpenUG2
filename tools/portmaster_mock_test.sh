#!/usr/bin/env bash
# Asset-free smoke test: simulate R36S/MuOS PortMaster with an ARM64 mock binary.
set -euxo pipefail
root="$(mktemp -d)"
trap 'rm -rf "$root"' EXIT
mkdir -p "$root/ports/openug2/game/TRACKS" "$root/ports/openug2/game/CARS" \
         "$root/ports/openug2/libs.aarch64" "$root/pm"
cp portmaster/OpenUG2.sh "$root/ports/OpenUG2.sh"
cat > "$root/pm/control.txt" <<'EOF'
CFW_NAME=MockCFW
DEVICE_ARCH=aarch64
get_controls() {
    export sdl_controllerconfig='03000000mock,Mock Controller,a:b0,b:b1'
}
pm_platform_helper() {
    echo "PM_HELPER:$1"
}
pm_finish() {
    echo "PM_FINISH"
}
EOF
cat > "$root/pm/mod_MockCFW.txt" <<'EOF'
export MOCK_MOD_APPLIED=yes
EOF
cat > "$root/ports/openug2/nfsu2.aarch64" <<'EOF'
#!/usr/bin/env bash
printf 'MOCK_BINARY\n'
printf 'EXEC_PATH:%s\n' "$0"
printf 'SDL_CONFIG:%s\n' "${SDL_GAMECONTROLLERCONFIG:-}"
printf 'CFW_MOD:%s\n' "${MOCK_MOD_APPLIED:-}"
printf 'XDG_HOME:%s\n' "${XDG_DATA_HOME:-}"
printf 'LD_PATH:%s\n' "${LD_LIBRARY_PATH:-}"
printf 'ARG:%s\n' "$@"
EOF
chmod +x "$root/ports/openug2/nfsu2.aarch64"
PORTMASTER_CONTROL_DIR="$root/pm" bash "$root/ports/OpenUG2.sh"
log="$root/ports/openug2/logs/OpenUG2.log"
test -s "$log"
grep -Fq 'MOCK_BINARY' "$log"
grep -Fq 'PM_FINISH' "$log"
grep -Fq 'PM_HELPER:' "$log"
grep -Fq 'SDL_CONFIG:03000000mock,Mock Controller,a:b0,b:b1' "$log"
grep -Fq 'CFW_MOD:yes' "$log"
grep -Fq 'ARG:--career-save' "$log"
grep -Fq 'ARG:--texture-cache-mb' "$log"
grep -Fq 'ARG:640x480' "$log"
grep -Fq 'ARG:--hud' "$log"
grep -Fq "EXEC_PATH:$root/ports/openug2/nfsu2.aarch64" "$log"
test -d "$root/ports/openug2/saves"
test -d "$root/ports/openug2/conf"
grep -Fq "libs.aarch64" "$log"
# A user-authored mod overlay is detected, passed once and never copied to
# the repo. Plain launcher without the file must not set --career-mod.
! grep -Fq 'ARG:--career-mod' "$log"
mkdir -p "$root/ports/openug2/mods"
printf '# test-only overlay\\n' > "$root/ports/openug2/mods/career_rewards.cfg"
PORTMASTER_CONTROL_DIR="$root/pm" bash "$root/ports/OpenUG2.sh"
grep -Fq 'ARG:--career-mod' "$log"
grep -Fq "ARG:$root/ports/openug2/mods/career_rewards.cfg" "$log"
# Missing assets must reject launch without running the game.
rm -r "$root/ports/openug2/game/CARS"
set +e
PORTMASTER_CONTROL_DIR="$root/pm" bash "$root/ports/OpenUG2.sh"
status=$?
set -e
test "$status" -eq 3
grep -Fq 'Missing retail NFS Underground 2 data' "$log"
! grep -Fq 'MOCK_BINARY' "$log"
echo "portmaster_mock_test: PASS"
