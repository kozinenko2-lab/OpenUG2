/* Map retail Underground 2 GCareerRace IDs to authored Paths####.bin.
 * Never infer a reward from a route alone if multiple retail races share it. */
#ifndef OPENUG2_UG2_CAREER_BINDING_H
#define OPENUG2_UG2_CAREER_BINDING_H
#include "ug2_career_file.h"
#include "career.h"
typedef enum {
    UG2_BIND_NO_MATCH=0,
    UG2_BIND_MATCH=1,
    UG2_BIND_AMBIGUOUS=-1
} UG2BindResult;
/* Returns 0 on an invalid or missing Paths####.bin suffix. */
unsigned ug2_route_from_path(const char *path);
/* Count ALL known original events on this stage/route, including events that
 * the current prototype cannot simulate (drift, URLs, drag, sprints).
 * This is informational and does not unlock/award anything. */
unsigned ug2_career_map_count(const UG2CareerIndex *cat,
                               unsigned stage,unsigned route);
/* Resolve only original single-route WORLD/CIRCUIT events presently supported
 * by circuit AI. Exact stage, original lap count, opponents and route must
 * match. A supplied race_id chooses among authored stage/route collisions.
 * For pre-race discovery ONLY, passing laps=0 and/or opponents=0 ignores
 * that dimension; the finish/payout path must pass confirmed positive values.
 * No side effects; the returned pointer belongs to the catalog. */
UG2BindResult ug2_career_resolve_circuit(
    const UG2CareerIndex *cat, unsigned stage, unsigned route,
    const char *race_id, unsigned laps, unsigned opponents,
    const CareerSourceRace **found);
#endif
