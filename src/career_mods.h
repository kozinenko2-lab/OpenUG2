/* Local, opt-in career reward mods. No EA binary files are ever rewritten. */
#ifndef OPENUG2_CAREER_MODS_H
#define OPENUG2_CAREER_MODS_H
#include "ug2_career_file.h"
#include <stdio.h>
/* Simple newline-separated EVENT_ID=INTEGER; # comments allowed.
 * Strictly validates all IDs against the user's loaded original catalog,
 * refusing duplicates, invalid values and malformed lines. Atomic update. */
int ug2_career_mod_apply_stream(UG2CareerIndex *catalog, FILE *file);
int ug2_career_mod_apply_file(UG2CareerIndex *catalog, const char *path);
#endif
