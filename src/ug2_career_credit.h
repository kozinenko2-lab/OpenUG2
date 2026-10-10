/* Transactional, source-backed UG2 circuit reward.
 * Only verified, available, regular single-route circuit events are payable.
 * A new win is not committed in RAM unless disk persistence succeeds. */
#ifndef OPENUG2_UG2_CAREER_CREDIT_H
#define OPENUG2_UG2_CAREER_CREDIT_H

#include "ug2_career_binding.h"
typedef enum {
    UG2_CREDIT_NOT_ELIGIBLE=0,
    UG2_CREDIT_PAID=1,
    UG2_CREDIT_ALREADY_PAID=2,
    UG2_CREDIT_AMBIGUOUS=3,
    UG2_CREDIT_SAVE_FAILED=4
} UG2CreditResult;

/* Consumes the *actual* finish result and verified race geometry, rather than
 * a user-chosen reward or arbitrary stage change. Caller owns catalog/profile.
 *
 * On UG2_CREDIT_PAID, the profile was committed to save_path and is now
 * updated in RAM; event points to the source record inside catalog.
 * On any other return, profile remains byte-for-byte unchanged. */
UG2CreditResult ug2_career_credit_circuit(
    Career *profile, const UG2CareerIndex *catalog,
    unsigned route, const char *requested_race_id,
    unsigned completed_laps, unsigned actual_opponents,
    int finish_position, const char *save_path,
    const CareerSourceRace **event);
#endif
