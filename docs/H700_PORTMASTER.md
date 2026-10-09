# OpenUG2 on Anbernic H700 / PortMaster

This branch is an experimental ARM64 + SDL2 + GLES2 bring-up. **Not a complete
Need for Speed Underground 2 game and not a verified H700 runtime release.**

## Intended device

Anbernic RG40XX H; Allwinner H700/Cortex-A53 (ARMv8-A); 1 GiB RAM;
native Mali OpenGL ES 2.0; 640x480. Linux systems such as muOS, dArkOS and
Knulli differ in their SDL video stack and runtime libraries.

## What this branch changes

- `--world-radius METRES` runtime argument, valid 250-4000; upstream desktop
  default 1400 metres remains unchanged. At 500m, the resident policy is load
  500m, draw 300m, cell 150m, safety margin 50m.
- SDL_GameController left stick, D-pad, trigger/shoulder accelerator and brake,
  B handbrake and A nitrous, with device plug/unplug handling.
- GLES2-compatible temporary front-end menu (`make gles-menu`).
- `portmaster/OpenUG2.sh` with editable RESOLUTION=640x480 and WORLD_RADIUS=500.
- `--texture-cache-mb N` soft world texture budget (0 disables trimming);
  launcher defaults to 96 MiB. Active resident textures are always protected,
  and trimming waits until background/candidate/retired residents are absent.
  Accounting estimates uncompressed RGB/RGBA+mip size, **not actual Mali VRAM**.
  Driver memory, GPU buffers and temporary CPU decoded textures are excluded.
- Asset-free GL regression confirms inactive GL names are deleted but pinned
  active resident names remain valid.
- The world-texture cache now keys entries by **(game-data root, STREAM
  bundle, texture key)** instead of the texture key alone. Changing tracks
  in-process cannot borrow pixels from an older track with a colliding key.
  Negative (missing) lookup entries are namespaced too.
- Synthetic two-track/same-key tests exercise different red/green images,
  same-track reuse, old/new world coexistence and safe post-switch eviction.
  Source files must stay unchanged during one running game session.
- Handheld controller mapping and diagnostic log in `openug2/logs/OpenUG2.log`.
- CI on **native Ubuntu ARM64** tests GLES2 compilation and synthetic test
  targets, but its binary may depend on a newer glibc than custom handheld OSes.

## Reproducible build without commercial data

On Debian/Ubuntu ARM64 with packages `libsdl2-dev`, `libgles2-mesa-dev`,
`zlib1g-dev`, `xxd` and build tools:

```sh
make gles-menu
bash -n portmaster/OpenUG2.sh
make world-group-test district-collision-test world-instance-test
```

This validates ARM64 + GLES2 API compatibility; it is *not* a guaranteed
PortMaster console binary. For H700 deployment cross-link against the exact
PortMaster SDK/sysroot and ensure appropriate glibc and actual vendor libGLESv2
on device. Do not include dummy/link-only GLES stubs in the runtime package.

## Layout on muOS (adjust for other systems)

```text
/mnt/sdcard/ROMS/ports/OpenUG2.sh
/mnt/sdcard/ports/openug2/nfsu2
/mnt/sdcard/ports/openug2/gamecontrollerdb-h700.txt
/mnt/sdcard/ports/openug2/game/CARS/          # owned retail game
/mnt/sdcard/ports/openug2/game/TRACKS/        # owned retail game
/mnt/sdcard/ports/openug2/game/GLOBAL/        # owned retail game
/mnt/sdcard/ports/openug2/logs/OpenUG2.log
```

No EA game assets or executable are stored in this repository. Verify
the SDL controller GUID and control mapping on each firmware independently.

## Game completion work remaining

| Milestone | Required acceptance checks |
| --- | --- |
| P0 handheld bring-up | real screen, input, audio, GLES shaders; frame pacing, stable region swaps |
| P0 memory | measured peak RSS within 1 GiB device budget; bounded texture cache and loading peaks |
| P1 world | all eight STREAM regions; deterministic seams/collision and rescue from unsupported ground |
| P1 races | authored circuit, sprint, drag, drift, Street X and URL; AI, finish & results |
| P2 career | progression stages, unlocks, race rewards, sponsorship, garage and shops |
| P2 persistence | native versioned save/load, backup-safe writes, malformed file tests |
| P3 story & finish | stage progression including final Caleb race; cutscenes/audio when assets available |
| P3 end-to-end | clean new profile to career completion on actual H700, resume after restart |

Do not claim game completion until all required event families and the ending
have been tested with legally acquired game files.

---

## По-русски

Это экспериментальная ветка для H700. Сборка ARM64/GLES2 не гарантирует
запуск на конкретной прошивке без аппаратного теста.

В `portmaster/OpenUG2.sh` можно изменять:

```bash
RESOLUTION="640x480"
WORLD_RADIUS="500"
TEXTURE_CACHE_MB="96"
```

После изменений **пересборка не нужна**. Радиус 500 м — это область загрузки
мира, видимость примерно 300 м. Файл лога находится в папке игры:
`openug2/logs/OpenUG2.log`. Ресурсы оригинальной игры добавляются пользователем
из собственной копии NFS Underground 2.

Следующие задачи: реальный запуск на H700, измерения RAM/FPS, ограничение
кэша текстур, корректные переходы между районами, все гонки, гараж,
экономика, карьера, сохранения и финал. Изменения оформляются малыми
проверяемыми итерациями. **Пока игра не является полностью проходимой.**
