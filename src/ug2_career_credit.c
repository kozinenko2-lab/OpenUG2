/* OpenUG2: verify a retail race and persist reward as one transaction.
 * Does not touch original game archives or retail save formats. */
#include "ug2_career_credit.h"
#include <string.h>

UG2CreditResult ug2_career_credit_circuit(
    Career *profile, const UG2CareerIndex *catalog,
    unsigned route, const char *requested_race_id,
    unsigned completed_laps, unsigned actual_opponents,
    int finish_position, const char *save_path,
    const CareerSourceRace **event) {
    if(event)*event=NULL;
    if(!profile || !catalog || !save_path || !*save_path ||
       !event || completed_laps==0 || actual_opponents==0 ||
       finish_position!=1 || profile->stage<1 ||
       profile->stage>CAREER_MAX_STAGE)
        return UG2_CREDIT_NOT_ELIGIBLE;
    const CareerSourceRace *resolved=NULL;
    UG2BindResult match=ug2_career_resolve_circuit_for_profile(
        catalog,profile,profile->stage,route,requested_race_id,
        completed_laps,actual_opponents,&resolved);
    if(match==UG2_BIND_AMBIGUOUS)return UG2_CREDIT_AMBIGUOUS;
    if(match!=UG2_BIND_MATCH || !resolved)return UG2_CREDIT_NOT_ELIGIBLE;
    *event=resolved;
    /* The original event identity, not the current STREAM district, is
     * persisted. A race replayed in another region must not pay twice. */
    if(career_has_win(profile,"UG2_ORIGINAL",resolved->id,CAREER_WORLD))
        return UG2_CREDIT_ALREADY_PAID;
    Career next=*profile;
    if(!career_record_win(&next,"UG2_ORIGINAL",resolved->id,
                          CAREER_WORLD,finish_position,(int)actual_opponents,
                          resolved->cash_value))
        return UG2_CREDIT_NOT_ELIGIBLE;
    /* Stage gates, sponsor wins, covers and non-circuit modes are still TODO;
     * never silently apply provisional stage advancement here. */
    if(!career_save(&next,save_path))return UG2_CREDIT_SAVE_FAILED;
    *profile=next;
    return UG2_CREDIT_PAID;
}
