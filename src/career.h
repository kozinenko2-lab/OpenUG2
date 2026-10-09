/* OpenUG2 prototype career profile. Asset-free; no EA saves are read/written.
 * Stage requirements are provisional and independently adjustable as retail
 * event IDs, sponsorship and photo triggers become verified. */
#ifndef OPENUG2_CAREER_H
#define OPENUG2_CAREER_H

#include <stdint.h>

#define CAREER_MAX_EVENTS 256
#define CAREER_MAX_STAGE 5
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
    uint32_t total_wins;        /* unique completed event identities */
    CareerWin wins[CAREER_MAX_EVENTS];
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
int career_record_cover(Career *c, unsigned stars);
int career_has_win(const Career *c, const char *track, const char *event,
                   CareerEventKind kind);

/* Portable LE v1 with checksum and bounded event list.
 * Writes .tmp + fsync + rename; retains a valid previous copy in .bak.
 * Reads backup if primary is unreadable/corrupt. The caller provides a file
 * path within the game folder; no retail save formats are involved. */
int career_save(const Career *c, const char *path);
int career_load(Career *c, const char *path);

#endif
