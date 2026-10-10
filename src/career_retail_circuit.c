/* Exact retail routing, not a generated table or inferred event trigger. */
#include "career_retail_circuit.h"
#include <string.h>
#include <stdint.h>

static int route_id_from_path(const char *path, unsigned *out) {
    if (!path || !out || strncmp(path,"ROUTES",6)!=0) return 0;
    const char *slash=strchr(path,'/');
    if (!slash || slash==path+6 || strchr(slash+1,'/') ||
        strncmp(slash+1,"Paths",5)!=0) return 0;
    const char *digits=slash+6;
    unsigned n=0;
    for(int i=0;i<4;i++) {
        if(digits[i]<'0'||digits[i]>'9')return 0;
        n=n*10+(unsigned)(digits[i]-'0');
    }
    if(strcmp(digits+4,".bin")!=0 || n==0 || n>65535u)return 0;
    *out=n;return 1;
}
int career_retail_circuit_lookup(const UG2CareerIndex *idx,
                                 unsigned stage, const char *path,
                                 CareerRetailCircuit *out) {
    if(!idx || !out || idx->unique_races>UG2_CAREER_MAX_RACES ||
       stage<1 || stage>5) return 0;
    unsigned track_id=0;
    if(!route_id_from_path(path,&track_id))return 0;
    const CareerSourceRace *only=NULL;
    int matched=0;
    /* Count *every* event using the same route and stage. Sponsors, URLs,
     * remixes and multiple stage routes can reuse a numerical track ID.
     * A circuit victory is not evidence for which one was intended. */
    for(uint32_t i=0;i<idx->unique_races;i++) {
        const CareerSourceRace *r=&idx->race[i];
        if(r->stage!=stage)continue;
        int same=0;
        unsigned n=r->num_stages>4?4:r->num_stages;
        for(unsigned j=0;j<n;j++)if(r->track_ids[j]==track_id)same=1;
        if(same){matched++;only=r;}
        if(matched>1)return 0; /* fail closed on ambiguous retail event */
    }
    if(matched!=1 || !only || only->num_stages!=1 ||
       only->track_ids[0]!=track_id ||
       !strstr(only->id,"_CIRCUIT_") ||
       only->opponents<1 || only->opponents>5 ||
       only->cash_value==0) return 0;
    CareerRetailCircuit result={only,track_id};
    *out=result;
    return 1;
}
int career_retail_circuit_win(Career *profile,const UG2CareerIndex *idx,
                              const char *path,int place,int opponents,
                              CareerRetailCircuit *out) {
    if(!profile || !idx || place!=1 || opponents<1 || opponents>5)return 0;
    CareerRetailCircuit matched;
    if(!career_retail_circuit_lookup(idx,profile->stage,path,&matched) ||
       opponents<matched.race->opponents)return 0;
    /* Namespaced as authored game race ID so one road used in two stages
     * cannot overwrite the earlier win or give an arbitrary replay payout. */
    if(!career_record_win(profile,"UG2_ORIGINAL",matched.race->id,
                          CAREER_WORLD,place,opponents,matched.race->cash_value))
        return 0;
    if(out)*out=matched;
    return 1;
}
