#include "career_event_map.h"
#include <ctype.h>
#include <string.h>

/* World.c loads a numbered authored route (not the EV_* location trigger).
 * Reject every non-canonical path, including relative traversal. */
int career_event_path_id(const char *path, uint16_t *out) {
    if(!path || !out || strstr(path,"..")) return 0;
    const char *name=strrchr(path,'/');
    const char *back=strrchr(path,'\\');
    if(back && (!name || back>name)) name=back;
    name=name?name+1:path;
    if(strncmp(name,"Paths",5)!=0 || strlen(name)!=13 ||
       strcmp(name+9,".bin")!=0) return 0;
    unsigned id=0;
    for(int i=5;i<9;i++){
        if(name[i]<'0' || name[i]>'9')return 0;
        id=id*10u+(unsigned)(name[i]-'0');
    }
    if(id<4000 || id>4999)return 0;
    *out=(uint16_t)id;
    return 1;
}
CareerEventMatch career_event_resolve(const UG2CareerIndex *index,
                        uint16_t track_id, unsigned stage, unsigned behavior,
                        const char *preferred_id, const CareerSourceRace **out) {
    if(out) *out=NULL;
    if(!index || !out || !track_id || stage<1 || stage>5 ||
       behavior>5 || index->unique_races>UG2_CAREER_MAX_RACES)
        return CAREER_EVENT_MISSING;
    const CareerSourceRace *found=NULL;
    for(uint32_t i=0;i<index->unique_races;i++){
        const CareerSourceRace *r=&index->race[i];
        if(r->stage!=stage || r->event_behavior!=behavior ||
           r->track_ids[0]!=track_id || r->num_stages!=1)
            continue;
        if(preferred_id && strcmp(r->id,preferred_id)!=0) continue;
        if(found) return CAREER_EVENT_AMBIGUOUS;
        found=r;
    }
    if(!found) return CAREER_EVENT_MISSING;
    *out=found;
    return CAREER_EVENT_MATCHED;
}
int career_event_can_credit(const CareerSourceRace *r,
                            unsigned stage, int opponents) {
    if(!r || !r->id[0] || !r->cash_value || r->stage!=stage ||
       r->event_behavior!=0 || r->num_stages!=1 ||
       r->opponents<1 || (int)r->opponents!=opponents) return 0;
    /* Unimplemented branches: sponsor registration, URL, hidden events, SUV
     * class restrictions and any conditional unlock which needs real state. */
    if(strstr(r->id,"_SPON_") || strstr(r->id,"_URL_") ||
       strstr(r->id,"_SUV_") || strstr(r->id,"_H_")) return 0;
    /* GlobalLib eUnlockCondition::AT_STAGE_START is 1.
     * The special intro race uses SPECIFIC_RACE_WON=0 and is handled
     * explicitly; other 0 (specific prior race) is NOT equivalent. */
    if(r->unlock_method==1) return 1;
    return stage==1 && !strcmp(r->id,"STAGE_1_CIRCUIT_1") &&
           r->unlock_method==0;
}
