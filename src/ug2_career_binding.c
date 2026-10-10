#include "ug2_career_binding.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

/* The OpenUG2 path catalog produces ROUTES<region>/PathsNNNN.bin. Names
 * from the retail GlobalLib file reference the same numeric track IDs.
 * Do not accept 'Paths4013.bin.bak' or suffixes as a real circuit. */
int ug2_career_route_id(const char *route_path) {
    if(!route_path) return -1;
    const char *p=strrchr(route_path,'/');
    p=p?p+1:route_path;
    if(strncmp(p,"Paths",5)!=0)return -1;
    p+=5;
    int id=0, digits=0;
    while(*p>='0' && *p<='9'){
        if(++digits>5 || id>65535/10)return -1;
        id=id*10+(*p++-'0');
        if(id>65535)return -1;
    }
    return digits>0 && id>0 && strcmp(p,".bin")==0?id:-1;
}
static int kind_from_name(const char *id, CareerEventKind *kind) {
    /* Source-verified retail NFSU2 naming conventions, not generic
     * numeric world-event type guesses. */
    if(strstr(id,"_SPON_")) *kind=CAREER_SPONSOR;
    else if(strstr(id,"_URL_")) *kind=CAREER_URL;
    else *kind=CAREER_WORLD;
    return 1;
}
int ug2_career_bind_circuit(const UG2CareerIndex *catalog,
                           const char *requested_id, const char *route_path,
                           unsigned stage, int opponents,
                           UG2CareerBinding *out) {
    if(!catalog || !out || stage<1 || stage>CAREER_MAX_STAGE ||
       catalog->unique_races>UG2_CAREER_MAX_RACES || opponents>5 ||
       opponents < -1 || (requested_id && !requested_id[0])) return 0;
    int route=ug2_career_route_id(route_path);
    if(route<0)return 0;
    int found=-1;
    for(uint32_t i=0;i<catalog->unique_races;i++){
        const CareerSourceRace *e=catalog->race+i;
        if(requested_id && strcmp(requested_id,e->id)) continue;
        if(e->stage!=stage || e->num_stages!=1 ||
           e->track_ids[0]!=(uint16_t)route || e->laps[0]<1 ||
           e->laps[0]>9 || e->cash_value==0 ||
           e->opponents==0 || (opponents>=0 && e->opponents!=opponents) ||
           !strstr(e->id,"CIRCUIT")) continue;
        if(found>=0)return 0; /* shared route: never guess who won */
        found=(int)i;
    }
    if(found<0)return 0;
    const CareerSourceRace *e=catalog->race+found;
    UG2CareerBinding next={0};
    memcpy(next.race_id,e->id,sizeof next.race_id);
    memcpy(next.trigger,e->trigger,sizeof next.trigger);
    next.route_id=(uint16_t)route;
    next.laps=e->laps[0];
    next.opponents=e->opponents;
    next.payout=e->cash_value;
    kind_from_name(e->id,&next.kind);
    *out=next;
    return 1;
}

int ug2_career_map_lookup(const UG2CareerIndex *catalog,
                          const char *requested_id, uint16_t map_event_id,
                          unsigned stage, UG2CareerBinding *out) {
    if(!catalog || !out || !map_event_id || stage<1 ||
       stage>CAREER_MAX_STAGE || catalog->unique_races>UG2_CAREER_MAX_RACES)
        return 0;
    int found=-1, chosen_stage=0;
    for(uint32_t i=0;i<catalog->unique_races;i++){
        const CareerSourceRace *r=catalog->race+i;
        if(r->stage!=stage || !r->cash_value ||
           (requested_id && strcmp(r->id,requested_id)!=0)) continue;
        int matched=-1;
        /* URL and special event scripts can have multiple authored routes.
         * Every candidate must come from a real TrackID_StageX field. */
        for(unsigned j=0;j<r->num_stages && j<4;j++)
            if(r->track_ids[j]==map_event_id && r->laps[j]>0){
                if(matched>=0)return 0; /* one ambiguous record */
                matched=(int)j;
            }
        if(matched<0)continue;
        if(found>=0)return 0; /* shared route ID: explicit race name required */
        found=(int)i;chosen_stage=matched;
    }
    if(found<0)return 0;
    const CareerSourceRace *r=catalog->race+found;
    UG2CareerBinding next={0};
    memcpy(next.race_id,r->id,sizeof next.race_id);
    memcpy(next.trigger,r->trigger,sizeof next.trigger);
    next.route_id=map_event_id;
    next.laps=r->laps[chosen_stage];
    next.opponents=r->opponents;
    next.payout=r->cash_value;
    kind_from_name(r->id,&next.kind);
    *out=next;
    return 1;
}
