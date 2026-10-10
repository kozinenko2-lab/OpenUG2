/* Optional offline GlobalLib-compatible balance editing for H700.
 * A user-authored CSV modifies an IN-MEMORY catalog, never retail GlobalB.
 * Format: first header exactly race_id,cash, then ID,CASH records.
 * No original EA-owned assets or C#/.NET runtime needed on Anbernic. */
#ifndef OPENUG2_CAREER_BALANCE_MOD_H
#define OPENUG2_CAREER_BALANCE_MOD_H
#include "ug2_career_file.h"
int ug2_career_apply_balance_mod(UG2CareerIndex *catalog,const char *csv_path);
#endif
