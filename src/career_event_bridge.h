/* Original UG2 event bridge; no guessed route names or retail assets. */
#ifndef OPENUG2_CAREER_EVENT_BRIDGE_H
#define OPENUG2_CAREER_EVENT_BRIDGE_H
#include "ug2_career_file.h"
#include "career.h"
typedef struct {
    CareerSourceRace race; /* backed by user's own GlobalB.lzc */
    CareerEventKind kind;
    uint16_t route_id;
} UG2CareerSelection;

/* Exact ID and current-stage binding; reject multi-stage events until all
 * component races can be verified independently. */
int ug2_career_select(const UG2CareerIndex *catalog, const char *race_id,
                      unsigned profile_stage, UG2CareerSelection *out);
/* Identify a world event only if a single authored career event at this
 * route/stage matches. An ambiguous route must be selected by exact ID. */
int ug2_career_unique_route(const UG2CareerIndex *catalog, unsigned stage,
                            uint16_t route_id, UG2CareerSelection *out);
/* Prove the actual AI circuit file (ROUTES..../PathsNNNN.bin) matches
 * the selected original WorldEvent ID. Never infer it from an AI filename. */
int ug2_career_path_matches(const UG2CareerSelection *selected,
                            const char *circuit_path);
/* Untrusted solo gate-crossing, free roam, and ghost AI must never award money.
 * Caller must establish both original world_race.finished and first place
 * in a real AI race. Never changes a Career on rejection. */
int ug2_career_award(Career *profile,const UG2CareerSelection *selected,
                     uint16_t world_route_id,int world_finished,
                     int verified_ai_finish,int finish_place,int ai_opponents);
#endif
