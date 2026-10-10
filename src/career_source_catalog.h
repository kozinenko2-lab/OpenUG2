/* Asset-free, bounds-checked decoder for extracted PC NFSU2 career records.
 * Field offsets independently reimplemented from the public NFS Tools
 * GlobalLib MIT specification; no binary assets or C# source is embedded:
 * https://github.com/NFSTools/GlobalLib
 *
 * Important: the caller must *first* locate a career record and its string
 * table inside the decompressed/validated GlobalB game archive. This does
 * not scan arbitrary bytes or classify event types by guessing at IDs. */
#ifndef OPENUG2_CAREER_SOURCE_CATALOG_H
#define OPENUG2_CAREER_SOURCE_CATALOG_H
#include <stddef.h>
#include <stdint.h>

#define CAREER_SOURCE_NAME 64
typedef struct {
    char id[CAREER_SOURCE_NAME];
    char trigger[CAREER_SOURCE_NAME];
    uint8_t stage;            /* raw BelongsToStage, 0..5 */
    uint8_t opponents;
    uint8_t icon_type;        /* raw eEventIconType; mapping not verified */
    uint8_t unlock_method;    /* raw eUnlockCondition; mapping not verified */
    uint8_t required_races;
    uint8_t required_urls;
    uint8_t required_specific_url;
    uint8_t sponsor_gate;
    uint8_t num_stages;
    uint8_t laps[4];
    uint16_t track_ids[4];
    uint32_t cash_value;
    uint32_t prerequisite_key; /* raw hashed specific-event ID for one mode */
    int32_t respect;
} CareerSourceRace;
typedef struct {
    uint8_t id;               /* raw stage number from extracted record */
    uint8_t sponsors_to_choose;
    int16_t outrun_cash_value;
    uint32_t sponsor_keys[5]; /* hashed names; requires separate dictionary */
    uint32_t last_stage_event_key;
    uint8_t map_limits[5];    /* circuit, drag, StreetX, drift, sprint */
    uint8_t max_outruns;
} CareerSourceStage;
typedef struct {
    char id[CAREER_SOURCE_NAME];
    int16_t cash_per_win;
    int16_t sign_bonus;
    int16_t potential_bonus;
    uint8_t required_race_types[3]; /* raw enum; not yet decoded */
} CareerSourceSponsor;

/* Return 1 on a safe decode, 0 on malformed/truncated data.
 * On failure out is left unchanged. These are individual records,
 * not the complete GlobalB.lzc container! */
int career_source_decode_race(const uint8_t *data, size_t length,
                              const uint8_t *strings, size_t string_length,
                              CareerSourceRace *out);
int career_source_decode_stage(const uint8_t *data, size_t length,
                               CareerSourceStage *out);
int career_source_decode_sponsor(const uint8_t *data, size_t length,
                                 const uint8_t *strings, size_t string_length,
                                 CareerSourceSponsor *out);

#endif
