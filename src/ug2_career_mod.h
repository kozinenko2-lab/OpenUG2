/* Lightweight sidecar mods for the extracted PC UG2 career catalog.
 * No C#/.NET runtime, EA files or binary rewriting required on H700. */
#ifndef OPENUG2_UG2_CAREER_MOD_H
#define OPENUG2_UG2_CAREER_MOD_H
#include "ug2_career_file.h"

/* Syntax: cash <EXACT_EVENT_ID> <0..10000000>
 * Comments begin with #, blank lines ignored. Each ID may occur only once.
 * This edits catalog metadata IN MEMORY; real race payout wiring is a
 * separate task. Returns 0 and leaves index unchanged on any error. */
int ug2_career_apply_mods_file(UG2CareerIndex *index, const char *path,
                               unsigned *changed);
#endif
