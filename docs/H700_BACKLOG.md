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
- [x] Implement opt-in soft LRU world-texture cache budget with active-resident
      pinning, conservative size estimates and asset-free GL regression.
- [x] Avoid building a third resident while the previous one is retiring
      under the H700/low-memory profile; allow desktop scheduling unchanged.
- [x] Add Linux process RSS/high-water logging at retirement, activation and
      periodic intervals; run asset-free transactional guard tests in ARM64 CI.
- [ ] Measure real H700 RSS and Mali GPU memory; adjust texture limits and
      resident finish/retirement quotas to avoid visible load pauses.
- [x] Scope all GPU texture-cache lookups and misses to game-data root and
      STREAM bundle; add synthetic red/green same-key cross-track regression.
- [ ] Confirm in-process track-switch correctness with legally owned NFSU2
      assets and real H700 GPU while previous/new residents overlap.
- [ ] Support hot-reloading changed files under the same bundle identity
      (currently archives must remain immutable during a game session).
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

- [x] Add independent versioned v1 career profile with checksum, safe rename,
      backup recovery, credits, unique race achievements and stage counters.
- [x] Connect confirmed legacy circuit-AI first-place win to one-time
      prototype payout; display career stage/cash in pre-race UI and HUD.
- [ ] Classify authored race IDs, URL and sponsorship contracts from actual
      game assets; current auto-award is restricted to existing AI circuits.
- [x] Create versioned v2 garage-in-profile persistence with v1 migration,
      per-car performance ownership, credit debits and atomic save.
- [x] Fix menu car/track selection to queue changes before committing successful
      in-process asset loads (avoid same-index no-op on pending requests).
- [ ] Wire controller-driven starter selection and dealer/garage navigation to
      model ownership; loaded profile must select/render owned active car.
- [ ] Apply purchased performance tiers to real car physics and store cosmetics.
- [ ] Add shop inventory/pricing/unlock registry verified against source game
      data; do not treat arbitrary available CARS folders as owned/unlocked.
- [ ] Build authored event unlock registry and source-verified race rewards.
- [ ] Validate stage gate requirements and retail payouts against game behavior.
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
