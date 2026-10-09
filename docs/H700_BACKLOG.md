# OpenUG2 PortMaster/H700 - engineering backlog

Status: **prototype, not fully playable**. This document is the issue tracker
until GitHub Issues are enabled for this fork. All EA assets remain local.

## P0: H700 bring-up and crash diagnostics

- [x] ARM64/GLES2 target and standalone controller-driven menu introduced.
- [x] Runtime 640x480 resolution and 500m resident world radius controls.
- [x] SDL_GameController car input and optional known H700 mapping.
- [x] Native ARM64 build regression CI established (see Actions result).
- [ ] Build with a release-compatible H700 libc/sysroot (Ubuntu CI ELF is not a console release).
- [ ] Run on RG40XX H/MuOS, verify real Mali EGL/GLES2, fullscreen, sound, controller.
- [ ] Capture FPS frame-time median/p99 and peak RSS while driving + changing cells.
- [ ] Limit world texture cache memory use and evict inactive resources safely.
- [ ] Validate collision arrays and async resident replacement across region swaps.

**Gate:** Device-tested playable roaming loop + logs and measured memory headroom.

## P1: World completeness and race vertical slice

- [ ] Audit each of the eight STREAML4R* regions with legal local game assets.
- [ ] Test roads, support meshes, curbs, walls, recoveries and district boundary cases.
- [ ] One complete event: frontend start > route > opponents > gates > results > free roam.
- [ ] Make author-defined racing event lookup and finish logic deterministic.
- [ ] Implement and test all required race families: circuit, sprint, drag,
      drift, Street X, URL; validate opponents for every type.
- [ ] Make the entire flow usable with the gamepad, including menu, map, garage.

**Gate:** No event needed by the retail career remains unplayable.

## P2: Native career and persistence

- [ ] Design versioned internal profile for money, garage, progress, unlocks and tuning.
- [ ] Crash-safe atomic save, backup and failed-checksum handling; no retail save overwrite.
- [ ] Implement payouts, sponsors, unlocks, shops, car purchases and upgrades.
- [ ] Career stages and authored story progression; final Caleb event and ending.
- [ ] New-profile-to-ending end-to-end tests; restart/resume and save corruption tests.

**Gate:** Fully finished single-player career with original user-owned resources.

## Acceptance discipline

Every runtime claim must cite an observed hardware log or reproducible CI run.
No proprietary asset, EXE, extracted texture or music enters this public repository.
Original OpenUG2 clean-room design and its MIT license are preserved.
Keep implementation tasks small, preserve tested defaults on desktop, document
configuration migration, and avoid speculative gameplay behavior that cannot
be supported by shipped data or independently verified observations.
