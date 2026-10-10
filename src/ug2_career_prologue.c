#include "ug2_career_prologue.h"
#include <string.h>

/* Values independently decoded from the user's original PC GlobalB.lzc.
 * They are not sufficient to detect a completed scripted race. */
static const CareerSourceRace *exact_prologue(
    const UG2CareerIndex *catalog,const char *id,uint32_t key) {
    if(!catalog || !id || catalog->unique_races>UG2_CAREER_MAX_RACES)
        return NULL;
    const CareerSourceRace *match=NULL;
    for(uint32_t i=0;i<catalog->unique_races;i++) {
        const CareerSourceRace *r=&catalog->race[i];
        if(strcmp(r->id,id)!=0)continue;
        /* Duplicate/colliding IDs are never accepted as proof of identity. */
        if(match)return NULL;
        if(r->stage!=0 || r->unlock_method!=0 ||
           r->prerequisite_key!=key || r->behavior!=3 ||
           r->icon_type!=0 || r->is_hidden!=0 ||
           r->num_stages!=1 || r->track_ids[0]!=4000 ||
           r->laps[0]!=0 || r->opponents!=0 || r->cash_value!=0)
            return NULL;
        match=r;
    }
    return match;
}
UG2PrologueState ug2_prologue_state(
    const UG2CareerIndex *cat,const Career *profile,const char *id) {
    if(!cat || !profile || !id || profile->stage!=1)return UG2_PROLOGUE_INVALID;
    if(strcmp(id,"DDAY_EVENT_A") && strcmp(id,"DDAY_EVENT_B"))
        return UG2_PROLOGUE_INVALID;
    /* Validate both endpoints of the A->B chain, not merely the selected
     * object's name: corrupt or partial catalogs never unlock the intro. */
    const CareerSourceRace *a=exact_prologue(cat,"DDAY_EVENT_A",0);
    const CareerSourceRace *b=exact_prologue(
        cat,"DDAY_EVENT_B",ug2_career_bin_hash("DDAY_EVENT_A"));
    if(!a || !b || ug2_career_prerequisite(cat,b)!=a)
        return UG2_PROLOGUE_INVALID;
    if(career_has_win(profile,"UG2_ORIGINAL",id,CAREER_WORLD))
        return UG2_PROLOGUE_DONE;
    if(!strcmp(id,"DDAY_EVENT_B") &&
       !career_has_win(profile,"UG2_ORIGINAL","DDAY_EVENT_A",CAREER_WORLD))
        return UG2_PROLOGUE_LOCKED;
    return UG2_PROLOGUE_READY;
}
UG2PrologueResult ug2_prologue_record_verified_finish(
    Career *profile,const UG2CareerIndex *cat,const char *id,
    unsigned actual_route,int script_finished,const char *save_path) {
    if(!profile || !cat || !id || !save_path || !*save_path ||
       actual_route!=4000 || script_finished!=1)
        return UG2_PROLOGUE_REJECTED;
    UG2PrologueState state=ug2_prologue_state(cat,profile,id);
    if(state==UG2_PROLOGUE_DONE)return UG2_PROLOGUE_ALREADY_RECORDED;
    if(state!=UG2_PROLOGUE_READY)return UG2_PROLOGUE_REJECTED;
    Career next=*profile;
    if(!career_record_original_prologue(&next,id))
        return UG2_PROLOGUE_REJECTED;
    if(!career_save(&next,save_path))
        return UG2_PROLOGUE_SAVE_FAILED;
    *profile=next;
    return UG2_PROLOGUE_RECORDED;
}
