# OpenUG2 PortMaster launcher for H700 (RG40XX H)

Inspired by Detoy's experimental R36S PortMaster package and its implementation
of `control.txt`, firmware-specific `mod_${CFW_NAME}.txt`,
`get_controls`, `pm_platform_helper`, per-arch SDL2 libraries and gamepad
config. Upstream: https://github.com/Detoy/OpenUG2

This port contains **no EA-owned assets**. You need your own retail **PC** copy
of NFS Underground 2 (the Xbox Switch-recomp asset layout is incompatible).

Suggested package on SD:

```
ports/
  OpenUG2.sh
  openug2/
    nfsu2.aarch64                 # H700-sysroot built binary (not Ubuntu CI!)
    gamecontrollerdb-h700.txt
    game/
      TRACKS/
      CARS/
      ...                        # your local installed game resources
    saves/
      career.dat
    logs/
      OpenUG2.log
```

Port metadata `port.json` and `gameinfo.xml` are in the top-level
`portmaster/` staging directory and should be copied to the distribution
root if packaging for PortMaster; do not distribute your game data.

Launcher configuration is at the beginning of `OpenUG2.sh`:
`RESOLUTION=640x480`, `WORLD_RADIUS=500`, `TEXTURE_CACHE_MB=96`,
`TRACK=STREAML4RA`. OpenGL ES2 + SDL2 are required. Keep the correct lib
versions for your firmware under `libs.aarch64` or `libs` as appropriate.
The script searches several MuOS/Knulli/DarkOS/R36S control.txt locations.
It prefers the `nfsu2.$DEVICE_ARCH` binary when PortMaster exposes a matching
architecture. Native SDL2 gamepad input is used; GPTOKEYB is not forced.

**Important**: the CI-generated Ubuntu ARM64 smoke binary is not automatically
compatible with firmware's libc or graphics stack. A proper target sysroot
and physical Mali H700 testing are still required. The experimental engine
does not yet provide a full retail career.

Run `bash tools/portmaster_mock_test.sh` from the repo root to verify script
behavior with a fake CFW and game executable, without proprietary assets.
