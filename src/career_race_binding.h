/* Source-verified bindings between GlobalB career race ID and game routes.
 * No invented payouts; aliases, hidden/sponsor/URL events are excluded. */
#ifndef OPENUG2_CAREER_RACE_BINDING_H
#define OPENUG2_CAREER_RACE_BINDING_H
#include "ug2_career_file.h"
#include <stddef.h>
#include <stdint.h>

/* Strict parse of ROUTES.../PathsNNNN.bin or PathsNNNN.bin. */
int career_route_track_id(const char *path, unsigned *id);
/* Number of authored records on a map event, independent of engine support.
 * Used for diagnostics and UI; NEVER authorizes a cash award. */
unsigned career_event_candidates(const UG2CareerIndex *idx,
                                 unsigned stage, unsigned track_id);
/* Resolvable only for a single NORMAL circuit of the current stage,
 * exact track, exact completed lap count and actual opponent count.
 * A known race ID can disambiguate events sharing the same path, but never
 * bypasses event stage, type, lap or opponent validation. */
const CareerSourceRace *career_resolve_circuit(const UG2CareerIndex *idx,
                               unsigned stage, const char *route,
                               unsigned completed_laps, unsigned opponents,
                               const char *selected_race);
#endif
