/* Only route IDs, race IDs, stages and payouts read from original UG2
 * GlobalB.lzc can enter the save. The type is derived from the ORIGINAL
 * career name, not route-prefix guesses (routes are shared between stages). */
#include "career_event_bridge.h"
#include <string.h>
#include <stdint.h>

static int classify(const char *name, CareerEventKind *out) {
    if(!name || !out || !name[0])return 0;
    if(strstr(name,"_SPON_")) *out=CAREER_SPONSOR;
    else if(strstr(name,"_URL_")) *out=CAREER_URL;
    else *out=CAREER_WORLD;
    return 1;
}
static int valid_race(const CareerSourceRace *r, unsigned stage) {
    /* Multi-stage URL and endgame events need individual route-result binding
     * before being eligible for career payouts. Not a guessing exercise. */
    return r && stage>=1 && stage<=CAREER_MAX_STAGE &&
           r->stage==stage && r->num_stages==1 &&
           r->track_ids[0]>0 && r->opponents>0 &&
           r->cash_value>0 && r->cash_value<=1000000u &&
           r->id[0] && r->trigger[0];
}
static int fill(const CareerSourceRace *r, unsigned stage,
                UG2CareerSelection *out) {
    if(!valid_race(r,stage) || !out)return 0;
    UG2CareerSelection next={0};
    if(!classify(r->id,&next.kind))return 0;
    next.route_id=r->track_ids[0];
    next.race=*r;
    *out=next;
    return 1;
}
int ug2_career_select(const UG2CareerIndex *catalog,const char *race_id,
                      unsigned profile_stage,UG2CareerSelection *out) {
    if(!catalog || !race_id || !race_id[0] || !out ||
       catalog->unique_races>UG2_CAREER_MAX_RACES) return 0;
    const CareerSourceRace *found=NULL;
    for(uint32_t i=0;i<catalog->unique_races;i++) {
        const CareerSourceRace *r=&catalog->race[i];
        if(strcmp(r->id,race_id)==0) {
            if(found)return 0;
            found=r;
        }
    }
    return fill(found,profile_stage,out);
}
int ug2_career_unique_route(const UG2CareerIndex *catalog,unsigned stage,
                            uint16_t route_id,UG2CareerSelection *out) {
    if(!catalog || !out || !route_id ||
       catalog->unique_races>UG2_CAREER_MAX_RACES)return 0;
    const CareerSourceRace *found=NULL;
    for(uint32_t i=0;i<catalog->unique_races;i++) {
        const CareerSourceRace *r=&catalog->race[i];
        if(valid_race(r,stage) && r->track_ids[0]==route_id) {
            if(found)return 0; /* no arbitrarily selected sponsor rewards */
            found=r;
        }
    }
    return fill(found,stage,out);
}
int ug2_career_award(Career *profile,const UG2CareerSelection *selected,
                     uint16_t world_route_id,int world_finished,
                     int verified_ai_finish,int finish_place,int ai_opponents) {
    if(!profile || !selected || !world_route_id ||
       world_route_id!=selected->route_id ||
       selected->race.track_ids[0]!=selected->route_id ||
       !valid_race(&selected->race,profile->stage) ||
       !world_finished || !verified_ai_finish || finish_place!=1 ||
       ai_opponents<1 || ai_opponents<selected->race.opponents)
       return 0;
    CareerEventKind kind;
    if(!classify(selected->race.id,&kind) || kind!=selected->kind)return 0;
    /* Domain-separated by literal retail race identity rather than a route:
     * distinct stages may share Paths4201 and must NOT collapse their wins. */
    return career_record_win(profile,"ORIGINAL_UG2",
                             selected->race.id,kind,finish_place,
                             ai_opponents,selected->race.cash_value);
}
