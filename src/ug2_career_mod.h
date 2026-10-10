#ifndef OPENUG2_UG2_CAREER_MOD_H
#define OPENUG2_UG2_CAREER_MOD_H
#include "ug2_career_file.h"
/* Simple opt-in override format: exact ORIGINAL_RACE_ID=positive_reward.
 * Atomic validation; never rewrites the user-owned GlobalB.lzc.
 * Returns 1 on success (including comments-only), 0 on invalid/unsafe input.
 * Catalog unchanged on failure. */
int ug2_career_mod_apply_file(const char *path, UG2CareerIndex *catalog,
                              unsigned *changed);
#endif
