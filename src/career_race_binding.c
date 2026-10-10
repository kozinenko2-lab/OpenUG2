#include "career_race_binding.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

int career_route_track_id(const char *path, unsigned *out) {
    if (!path || !out) return 0;
    size_t n=strlen(path);
    if(n<13 || n>255) return 0;
    const char *base=strrchr(path,'/');
    base=base?base+1:path;
    if(strncmp(base,"Paths",5)) return 0;
    const char *p=base+5;
    unsigned id=0, digits=0;
    for(;*p>='0' && *p<='9';p++) {
        if(++digits>5 || id>(UINT_MAX-(unsigned)(*p-'0'))/10) return 0;
        id=id*10+(unsigned)(*p-'0');
    }
    if(!digits || strcmp(p,".bin") || id<4000 || id>9999) return 0;
    *out=id;
    return 1;
}
unsigned career_event_candidates(const UG2CareerIndex *idx,
                                 unsigned stage, unsigned track_id) {
    if(!idx || idx->unique_races>UG2_CAREER_MAX_RACES ||
       stage<1 || stage>5 || track_id<4000 || track_id>9999) return 0;
    unsigned n=0;
    for(unsigned i=0;i<idx->unique_races;i++) {
        const CareerSourceRace *r=&idx->race[i];
        if(r->stage!=stage) continue;
        for(unsigned s=0;s<r->num_stages && s<4;s++) {
            if(r->track_ids[s]==track_id) {n++;break;}
        }
    }
    return n;
}
/* Conservative classification using author-stored event ID and GlobalLib's
 * eEventIconType.REGULAR=3 and eEventBehaviorType.CIRCUIT=0.
 * Avoid assuming hidden/SUV/sponsor/URL eligibility from a track alone. */
static int normal_circuit(const CareerSourceRace *r,unsigned stage) {
    if(!r || r->behavior!=0 || r->icon_type!=3 ||
       r->stage!=stage || r->num_stages!=1 || !r->opponents ||
       r->cash_value==0 || r->laps[0]==0) return 0;
    char prefix[32];
    int length=stage==1?snprintf(prefix,sizeof prefix,"STAGE_1_CIRCUIT_")
                       :snprintf(prefix,sizeof prefix,"S%u_CIRCUIT_",stage);
    if(length<1 || (size_t)length>=sizeof prefix ||
       strncmp(r->id,prefix,(size_t)length)) return 0;
    const char *suffix=r->id+length;
    if(!*suffix) return 0;
    for(;*suffix;suffix++) if(*suffix<'0'||*suffix>'9')return 0;
    return 1;
}
const CareerSourceRace *career_resolve_circuit(const UG2CareerIndex *idx,
                               unsigned stage, const char *route,
                               unsigned completed_laps, unsigned opponents,
                               const char *selected_race) {
    unsigned track;
    if(!idx || idx->unique_races>UG2_CAREER_MAX_RACES ||
       !career_route_track_id(route,&track) ||
       stage<1 || stage>5 || !completed_laps || !opponents) return NULL;
    const CareerSourceRace *found=NULL;
    for(unsigned i=0;i<idx->unique_races;i++) {
        const CareerSourceRace *r=&idx->race[i];
        if(!normal_circuit(r,stage) ||
           r->track_ids[0]!=track || r->laps[0]!=completed_laps ||
           r->opponents!=opponents) continue;
        if(selected_race && *selected_race &&
           strcmp(selected_race,r->id)) continue;
        if(found) return NULL; /* ambiguous: no guess, no payment */
        found=r;
    }
    return found;
}
