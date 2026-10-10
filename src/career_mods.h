/* Optional desktop-authored mod overlay for retail UG2 payouts. Never edits
 * original EA GlobalB data. No .NET/GlobalLib runtime is needed on H700. */
#ifndef OPENUG2_CAREER_MODS_H
#define OPENUG2_CAREER_MODS_H
#include "ug2_career_file.h"
#include <stddef.h>
/* Strict UTF-8/ASCII ID=value lines. On any error the entire catalog stays
 * unchanged. An explicit file path is supplied by --career-mods. */
int career_mods_apply_file(UG2CareerIndex *catalog, const char *path,
                           unsigned *changed);
#endif
