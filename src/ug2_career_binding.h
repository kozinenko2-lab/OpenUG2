/* Bind a real PC Underground 2 career event to a loaded circuit route.
 * A shared route ID by itself NEVER uniquely identifies an authored event. */
#ifndef OPENUG2_UG2_CAREER_BINDING_H
#define OPENUG2_UG2_CAREER_BINDING_H
#include "ug2_career_file.h"
#include "career.h"
typedef struct {
    char race_id[CAREER_SOURCE_NAME];
    char trigger[CAREER_SOURCE_NAME];
    uint16_t route_id;
    uint8_t laps;
    uint8_t opponents;
    CareerEventKind kind;
    uint32_t payout;
} UG2CareerBinding;
/* Explicit ID preferred. If requested_id==NULL the match MUST be unique
 * among all qualifying events; ambiguity or mismatches disable rewards.
 * Stage and opposing AI count must match the retail entry exactly.
 * UI/read-only map inspection may use opponents=-1; NO payout from a
 * map inspection itself. Only circuit classes are currently supported. */
int ug2_career_bind_circuit(const UG2CareerIndex *catalog,
                           const char *requested_id, const char *route_path,
                           unsigned stage, int opponents,
                           UG2CareerBinding *out);
/* Lookup the game's real world-map event number (from WEvent.id). This is
 * read-only metadata for map/HUD. A completed single-player checkpoint event
 * is NOT proof of race victory and MUST NOT pay money. Unresolvable/ambiguous
 * route-stage pairs return zero, unless requested_id disambiguates them. */
int ug2_career_map_lookup(const UG2CareerIndex *catalog,
                          const char *requested_id, uint16_t map_event_id,
                          unsigned stage, UG2CareerBinding *out);
int ug2_career_route_id(const char *route_path);
#endif
