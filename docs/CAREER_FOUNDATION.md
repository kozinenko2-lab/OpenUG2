# OpenUG2 / H700: prototype career progression (v1)

This document describes implemented code, not a claim of retail NFSU2 career parity.

## Available now

- New `src/career.c/.h` model, independent of SDL2, GLES2 and original game files.
- Unique credited events use (track bundle, event route identifier, category).
  Categories: `CAREER_WORLD`, `CAREER_SPONSOR`, `CAREER_URL`. Replays never
  count twice or grant an additional prototype prize.
- A finish qualifies only when player places **1st with at least one AI opponent**.
  Only the existing legacy circuit-AI finish is wired into the game currently.
  The procedural/scripted sprint tracker does *not* receive career rewards,
  because its complete opponent/placement rules are not implemented.
- Existing pre-race UI displays stage (out of 5), credited world wins in that
  stage and cash when `--career-save` is enabled. Free-roam HUD shows cash.
- Prototype race award: **500 credits** per first-time, verified AI circuit
  victory. This is a development placeholder, **not an EA-verified payout**.
- Data model covers independent world/sponsor/URL wins, photo cover identities,
  visual-rating stars and gates for five stages. Only WORLD AI circuit results
  presently enter those counters automatically. Future shop/photo systems can
  submit independently verified events via the typed API.
- `career_advance_stage` requires all stage-specific counters. It *does not*
  invent events, create an inaccessible district, award fictional contracts or
  count solo event completions. Stages above 1 will generally remain gated
  until sponsor, URL and magazine integration is written.
- Portable little-endian version 1 profile with length/count/checksum validation,
  strict upper bound of 256 unique events, fsync and same-directory atomic rename.
  The previous valid primary is retained as `.bak`. Loading falls back to
  `.bak` when the primary is missing or invalid. This is an OpenUG2-specific
  profile: **do not rename it to an original NFSU2 save file**.

## Device launch and layout

`portmaster/OpenUG2.sh` passes
`--career-save "$GAMEDIR/saves/career.dat"` and creates the `saves` directory.
Files are stored alongside the game:

```text
openug2/
  nfsu2
  game/           # user-owned retail game data, never committed
  saves/
    career.dat
    career.dat.bak
  logs/
    OpenUG2.log
```

Desktop runs without `--career-save` do not load or write a profile and do
not change the upstream default gameplay behavior. Changing the profile path
creates or uses a separate user profile. Do not share `career.dat` between
simultaneously running processes; writing is single-process.

## Tests

Run `make career-test` without EA assets or graphics. Assertions cover:

- disallowed solo/second-place results, duplicate wins, unique per track/mode
- five world wins to reach stage 2; 10 world, 3 sponsored, 3 URL, one
  unique photograph and 1-star visual rating for stage 3
- no stage advancement before all gates; counters reset only on promotion
- save/restart/checksum/backup recovery after truncated primary
- max unique event entries and money overflow saturation

This foundation does **not** complete garage/shops, sponsor contracts, retail
URL championship logic, authored sprint AI, stage-specific photo magazines,
final Caleb race, or end-to-end playability. Research stage requirements must
be reconciled with retail game data/events before advertising career parity.

## Next slices

1. Parse and tag authored race IDs to a persistent catalog with explicit types.
2. Add fully ranked AI and a reliable finish/result state for sprint and URL.
3. Sponsor offers/contracts and stage event requirements verified from assets.
4. Player-owned vehicle garage with persisted upgrades/purchases and shop flow.
5. Save-version migration for inventory, narrative triggers, unlocks and ending.
