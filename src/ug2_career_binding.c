#include "ug2_career_binding.h"
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

/* UG2 binary hash, confirmed against original DDAY_EVENT_B.
 * GlobalLib Utils/Bin.cs: start 0xffffffff, multiply by 33, append each
 * ASCII byte, unsigned 32-bit wrap. No hash is sufficient proof of victory. */
uint32_t ug2_career_bin_hash(const char *name) {
    if(!name || !*name) return 0;
    uint32_t key=UINT32_MAX;
    for(const unsigned char *p=(const unsigned char *)name;*p;p++) {
        if(*p>=128u) return 0;
        key=key*UINT32_C(33) + (uint32_t)*p;
    }
    return key;
}
const CareerSourceRace *ug2_career_prerequisite(
    const UG2CareerIndex *cat,const CareerSourceRace *event) {
    if(!cat || !event || cat->unique_races>UG2_CAREER_MAX_RACES ||
       event->unlock_method!=0 || !event->prerequisite_key) return NULL;
    const CareerSourceRace *candidate=NULL;
    for(uint32_t i=0;i<cat->unique_races;i++) {
        const CareerSourceRace *r=&cat->race[i];
        if(ug2_career_bin_hash(r->id)!=event->prerequisite_key)continue;
        if(candidate)return NULL; /* hash collision or duplicate */
        candidate=r;
    }
    return candidate;
}
unsigned ug2_route_from_path(const char *path) {
    if(!path)return 0;
    const char *base=strrchr(path,'/');
    const char *back=strrchr(path,'\\');
    if(!base || (back && back>base))base=back;
    base=base?base+1:path;
    if(strncmp(base,"Paths",5)!=0)return 0;
    base+=5;
    unsigned route=0,digits=0;
    while(*base>='0' && *base<='9') {
        if(++digits>5)return 0;
        route=route*10u+(unsigned)(*base++-'0');
    }
    return route>=1000 && digits==4 && strcmp(base,".bin")==0 ? route:0;
}
unsigned ug2_career_map_count(const UG2CareerIndex *cat,
                              unsigned stage,unsigned route) {
    if(!cat || stage<1 || stage>5 || route<1000 ||
       cat->unique_races>UG2_CAREER_MAX_RACES)return 0;
    unsigned count=0;
    for(unsigned i=0;i<cat->unique_races;i++) {
        const CareerSourceRace *r=&cat->race[i];
        if(r->stage!=stage)continue;
        for(unsigned j=0;j<r->num_stages && j<4;j++)
            if(r->track_ids[j]==route){++count;break;}
    }
    return count;
}
/* GlobalLib eUnlockCondition values:
 * 0 SPECIFIC_RACE_WON    (hashed prerequisite needs a verified dependency)
 * 1 AT_STAGE_START      (the +0x10 byte names the previous stage)
 * 2 SPONSOR_CHOSEN
 * 3 REQUIRED_RACES_WON
 * 4 REQUIRED_URL_WON
 * Do not treat a selectable Paths route or --career-race as proof of unlock.
 * This stage-start rule is limited to the byte pattern verified in the
 * original PC UG2 GlobalB.lzc; unsupported conditions fail closed. */
static int original_circuit_stage_start(const CareerSourceRace *r,
                                         unsigned stage) {
    return r && stage>=2 && stage<=5 &&
           r->unlock_method==1 &&
           r->required_specific_url==stage-1 &&
           r->sponsor_gate==0 &&
           r->required_races==0 && r->required_urls==0;
}
static int original_circuit_supported(const CareerSourceRace *r,
                                       unsigned stage,unsigned route,
                                       unsigned laps,unsigned opponents) {
    if(!r || !original_circuit_stage_start(r,stage) ||
       r->stage!=stage || r->icon_type!=3 ||
       r->behavior!=0 || r->is_hidden!=0 ||
       r->num_stages!=1 || r->track_ids[0]!=route ||
       (laps && r->laps[0]!=laps) ||
       (opponents && r->opponents!=opponents) ||
       !r->opponents || !r->laps[0] || r->laps[0]>16 ||
       !r->cash_value ||
       r->cash_value>1000000u) return 0;
    /* Accept only plain, non-hidden regular circuits with a canonical
     * retail identifier. Hidden events, sponsors, SUV and other types
     * require separate unlock checks and dedicated gameplay systems.
     * This resolver intentionally rejects SPECIFIC_RACE_WON (including the
     * stage-1 intro circuit) until prerequisite hash tracking exists.
     * Passing --career-race NEVER bypasses those gates. */
    char prefix[32];
    int n=stage==1?snprintf(prefix,sizeof prefix,"STAGE_1_CIRCUIT_")
                  :snprintf(prefix,sizeof prefix,"S%u_CIRCUIT_",stage);
    if(n<=0 || (size_t)n>=sizeof prefix ||
       strncmp(r->id,prefix,(size_t)n)!=0) return 0;
    const char *p=r->id+n;
    if(!*p) return 0;
    for(;*p;p++) if(*p<'0' || *p>'9') return 0;
    return 1;
}
UG2BindResult ug2_career_resolve_circuit(
    const UG2CareerIndex *cat,unsigned stage,unsigned route,
    const char *race_id,unsigned laps,unsigned opponents,
    const CareerSourceRace **found) {
    if(found)*found=NULL;
    if(!cat || !found || cat->unique_races>UG2_CAREER_MAX_RACES ||
       stage<1 || stage>5 || route<1000)return UG2_BIND_NO_MATCH;
    const CareerSourceRace *selected=NULL;
    for(unsigned i=0;i<cat->unique_races;i++) {
        const CareerSourceRace *r=&cat->race[i];
        if(!original_circuit_supported(r,stage,route,laps,opponents))continue;
        if(race_id && *race_id && strcmp(r->id,race_id))continue;
        if(selected)return UG2_BIND_AMBIGUOUS;
        selected=r;
    }
    if(!selected)return UG2_BIND_NO_MATCH;
    *found=selected;
    return UG2_BIND_MATCH;
}
