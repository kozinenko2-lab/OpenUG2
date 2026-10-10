# Career profile v2: unified garage and career state

## Implemented (engine/backend, not yet a shop UI)

The v2 profile extends v1 while keeping achievements, credits and stage counters.
The same `career.dat` holds the user's garage and performance package tiers,
so a purchase debits money and adds ownership in **one save transaction**. It
does not touch retail NFSU2 save files or distribute EA assets.

- `career_garage_claim_starter(profile, "FOCUS")`: grants exactly one starter
  to an empty profile after the *caller* verifies dealer eligibility. This
  also supports migrated v1 profiles saved at a later career stage.
  No specific starter is auto-assigned; "HUMMER" remains the old technical
  launcher default and is **not** considered a valid retail starter.
- `career_garage_purchase_car(profile, model, price)`: requires an existing
  starter, available garage slot (8), an unowned model, and enough credits.
  Catalog/unlock restrictions must be enforced by a future dealer UI.
- `career_garage_select(profile,index)`: stores the active owned vehicle ID.
- `career_garage_purchase_upgrade(profile,slot,tier,price)`: eight performance
  categories, three ownership tiers each, one car at a time. These tiers do
  not yet alter actual car physics/rendering; that requires applying proven
  game-data performance/visual modification catalogs.
- The model name is a canonical uppercase CARS directory component, max 31
  bytes; traversal, separators, lowercase and invalid bytes are rejected.

**No gameplay screens yet**: The garage inventory is not the car selector,
dealer, or performance shop. There is no approved unlock/catalog price map
yet. The existing menu's unbounded debug car switch remains a separate
development control and must not be mistaken for career ownership.
Furthermore a loaded garage selection is not yet auto-applied to the
rendered vehicle on boot. Those are follow-up integration milestones.

## File format

- Shared magic `OUG2CR01`, version field 2.
- Previous v1 fields remain in their original order and offsets; two new
  32-bit fields describe garage count and active slot, followed by saved
  achievement hashes and fixed-width car records (32-byte ID, 8 upgrade tiers).
- Checksum covers the complete payload; strict length validation; duplicate
  and invalid model IDs are rejected before a profile is accepted.
- v1 profiles migrate in memory with an **empty garage**, preserving winnings
  and credits. Next save writes v2.
- Valid previous versions are kept in `.bak` on save; a broken primary can
  restore the prior valid profile. A backup made before the garage existed
  naturally cannot reconstruct post-v1 purchases.

## Safe shop transaction pattern

```c
Career next = career;
if (career_garage_purchase_car(&next, known_unlocked_model, verified_price)
    && career_save(&next, career_save_path)) {
    career = next; // publish only after disk commit
}
```

This example is a **required future integration contract**, not a claim
that the current dealer menu uses this API. If saved balance and currently
loaded car disagree, the host must defer the change until the matching
car's resources have been loaded and validated.

## CI

`make career-test career-garage-test` validates the previous achievements,
starter choice, repeat/invalid purchases, insufficient credits, separate
car upgrade sets, profile restart, legacy v1 migration, corrupted primary
recovery and eight-car limit, without graphics or EA files.

## Remaining for actual playable career

1. First-car dealer screen: region-specific *actual* starter list, choose
   starter via SDL2 gamepad, save profile and load chosen car.
2. Garage screen: navigate owned cars, switch only on successful car resource
   loading, persist selection and installed parts.
3. Shops/car lot: source-verified unlocks/prices/tier caps, physical application
   of upgrades and purchased cosmetic inventory.
4. Event catalog: actual career races and AI-ranked results, sponsors, URL,
   payouts and stage gates. Do not credit scripted/solo test events.
5. Story/final race and end-to-end finish testing on real H700 hardware.

Full Russian milestone plan: `docs/FULL_CAREER_ROADMAP_RU.md`.
