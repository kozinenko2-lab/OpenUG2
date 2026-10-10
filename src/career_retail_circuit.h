/* Safe binding between authored UG2 career metadata and an AI circuit win.
 * The route's numerical Paths ID is matched against GCareerRace.TrackID_Stage1.
 * Unknown or ambiguous IDs NEVER earn credits. */
#ifndef OPENUG2_CAREER_RETAIL_CIRCUIT_H
#define OPENUG2_CAREER_RETAIL_CIRCUIT_H
#include "career.h"
#include "ug2_career_file.h"

typedef struct {
    const CareerSourceRace *race;  /* pointer borrowed from immutable index */
    unsigned route_id;
} CareerRetailCircuit;

/* 1 uniquely matched; 0 unsupported, wrong stage/type or ambiguous.
 * No owned assets required; no name-based assumptions about event triggers. */
int career_retail_circuit_lookup(const UG2CareerIndex *index,
                                 unsigned stage, const char *circuit_path,
                                 CareerRetailCircuit *out);

/* Atomic in-RAM crediting, requires verified first-place against >= authored
 * opponents. The caller must persist a COPY via career_save before committing
 * the updated profile, as for other transactions. */
int career_retail_circuit_win(Career *profile, const UG2CareerIndex *index,
                              const char *circuit_path, int finish_place,
                              int actual_opponents, CareerRetailCircuit *out);
#endif
