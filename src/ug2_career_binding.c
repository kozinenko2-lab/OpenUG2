#include "ug2_career_binding.h"
#include <ctype.h>
#include <string.h>
#include <limits.h>

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
static int original_circuit_supported(const CareerSourceRace *r,
                                       unsigned stage,unsigned route,
                                       unsigned laps,unsigned opponents) {
    if(!r || r->stage!=stage || r->icon_type!=3 ||
       r->num_stages!=1 || r->track_ids[0]!=route ||
       (laps && r->laps[0]!=laps) ||
       (opponents && r->opponents!=opponents) ||
       !r->opponents || !r->laps[0] || r->laps[0]>16 ||
       !r->cash_value ||
       r->cash_value>1000000u) return 0;
    /* Until we have other original game-mode implementations, award only
     * regular 0x3414c circuit routes. Drift/drag/street/URL/sponsor cannot
     * be made equivalent to circuit AI by renaming an event. */
    return strstr(r->id,"_CIRCUIT_")!=NULL &&
           strstr(r->id,"_SPON_")==NULL &&
           strstr(r->id,"_URL_")==NULL;
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
