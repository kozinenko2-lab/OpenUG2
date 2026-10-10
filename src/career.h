/* OpenUG2 prototype career profile. Asset-free; no EA saves are read/written.
 * Stage requirements are provisional and independently adjustable as retail
 * event IDs, sponsorship and photo triggers become verified. */
#ifndef OPENUG2_CAREER_H
#define OPENUG2_CAREER_H

#include <stdint.h>

#define CAREER_MAX_EVENTS 256
#define CAREER_MAX_STAGE 5
#define CAREER_MAX_GARAGE 8
#define CAREER_CAR_MODEL_CAP 32
#define CAREER_UPGRADE_SLOTS 8
/* Stable upgrade slot identities; exact tier/price catalog is still TODO. */
typedef enum {
    CAREER_UPG_ENGINE, CAREER_UPG_ECU, CAREER_UPG_TRANSMISSION,
    CAREER_UPG_TURBO, CAREER_UPG_NITROUS, CAREER_UPG_SUSPENSION,
    CAREER_UPG_BRAKES, CAREER_UPG_TIRES
} CareerUpgradeSlot;
typedef struct {
    char model[CAREER_CAR_MODEL_CAP];  /* validated game CARS directory ID */
    uint8_t tier[CAREER_UPGRADE_SLOTS]; /* zero=stock; 1..3 purchased */
} CareerOwnedCar;
typedef enum {
    CAREER_WORLD = 0, CAREER_SPONSOR = 1, CAREER_URL = 2
} CareerEventKind;
typedef struct {
    uint64_t key;  /* hash of track + event name + kind */
} CareerWin;
typedef struct {
    uint32_t stage;             /* 1..5; final Caleb race is not yet implemented */
    uint32_t money;             /* prototype account, not retail NFSU2 pricing */
    uint32_t world_wins, sponsor_wins, url_wins, dvd_covers;
    uint32_t visual_rating;     /* stars, 0..10 */
    uint32_t total_wins;        /* unique completed race identities */
    uint32_t seen_count;        /* race + photo identities in wins[] */
    CareerWin wins[CAREER_MAX_EVENTS];
    uint32_t garage_count;
    uint32_t garage_selected;
    CareerOwnedCar garage[CAREER_MAX_GARAGE];
} Career;
typedef struct {
    uint32_t world_wins, sponsor_wins, url_wins, dvd_covers, stars;
} CareerRequirements;

/* 1 = new credited win; 0 = retry, no confirmed win or invalid input.
 * A race without at least one opponent cannot advance career. All category
 * fields are validated independently; no automatic sponsor/URL classification.
 * Payout is caller supplied; repeated wins give no additional prototype cash. */
void career_init(Career *c);
CareerRequirements career_requirements(unsigned stage);
int career_stage_ready(const Career *c);
int career_advance_stage(Career *c);  /* one stage; requires all verified gates */
int career_record_win(Career *c, const char *track, const char *event,
                      CareerEventKind kind, int position, int opponents,
                      uint32_t payout);
int career_record_cover(Career *c, const char *location, unsigned stars);
int career_has_win(const Career *c, const char *track, const char *event,
                   CareerEventKind kind);

/* Store only an independently verified original DDAY scripted completion.
 * The caller MUST establish true script completion. Stage-0 events have no
 * laps or opponents; this does not fake competitive wins or alter money,
 * career stage or race counters. Canonical domain allows hashed unlock checks
 * to use career_has_win(profile,"UG2_ORIGINAL",id,CAREER_WORLD). */
int career_record_original_prologue(Career *c, const char *id);

/* Garage inventory shares the career save: purchases never change cash
 * without changing ownership in the same transaction. All game-facing
 * callers must save a copy before publishing the change in RAM.
 * Starter must be chosen by the game/menu, not silently set to Hummer. */
int career_garage_claim_starter(Career *c, const char *model);
int career_garage_purchase_car(Career *c, const char *model, uint32_t price);
int career_garage_select(Career *c, uint32_t index);
int career_garage_purchase_upgrade(Career *c, CareerUpgradeSlot slot,
                                   uint8_t tier, uint32_t price);
int career_garage_owns(const Career *c, const char *model);
const CareerOwnedCar *career_garage_active(const Career *c);

/* Portable LE v2 with checksum and bounded event/garage lists.
 * Reads v1 profiles with empty garage and migrates on next save.
 * Note: v1 never stored owned car names, so player must pick a starter.
 *
 *
 * Writes .tmp + fsync + rename; retains a valid previous copy in .bak.
 * Reads backup if primary is unreadable/corrupt. The caller provides a file
 * path within the game folder; no retail save formats are involved. */
int career_save(const Career *c, const char *path);
int career_load(Career *c, const char *path);

#endif
