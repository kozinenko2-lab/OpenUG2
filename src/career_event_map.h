/* Bridge retail GCareerRace metadata to OpenUG2's authored Routes*/Paths*.bin.
 * The route ID alone is NOT a unique race identity (sponsors, repeat stages).
 * Only exact, unique, supported events are eligible for cash mutations. */
#ifndef OPENUG2_CAREER_EVENT_MAP_H
#define OPENUG2_CAREER_EVENT_MAP_H
#include "career.h"
#include "ug2_career_file.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    CAREER_EVENT_MISSING=0,
    CAREER_EVENT_MATCHED=1,
    CAREER_EVENT_AMBIGUOUS=-1,
    CAREER_EVENT_BLOCKED=-2
} CareerEventMatch;

/* Accept path like ROUTESL4RF/Paths4013.bin, rejecting malformed suffixes,
 * path traversal, other IDs and partial decimals. */
int career_event_path_id(const char *path, uint16_t *track_id);

/* Resolve by the original first TrackID, stage and behavior (GlobalLib enum).
 * If preferred_id!=NULL an exact retail race identifier is mandatory.
 * Never resolve ambiguous matches by array order. */
CareerEventMatch career_event_resolve(const UG2CareerIndex *index,
                        uint16_t track_id, unsigned stage, unsigned behavior,
                        const char *preferred_id, const CareerSourceRace **out);

/* Existing engine currently proves only circuit-AI victories. Do not credit
 * sponsor, URL, hidden, SUV or multi-stage races without their unlock flows.
 * Reject a race if the selected rival count differs from authored metadata. */
int career_event_can_credit(const CareerSourceRace *race,
                            unsigned current_stage, int opponents);
#endif
