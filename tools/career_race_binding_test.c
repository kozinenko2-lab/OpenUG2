#include "career_race_binding.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void add(UG2CareerIndex *idx,const char *name,unsigned stage,
                unsigned id,unsigned laps,unsigned opponents,
                unsigned behavior,unsigned icon,unsigned payout) {
    assert(idx->unique_races<UG2_CAREER_MAX_RACES);
    CareerSourceRace *r=&idx->race[idx->unique_races++];
    memset(r,0,sizeof *r);
    snprintf(r->id,sizeof r->id,"%s",name);
    r->stage=(uint8_t)stage;r->track_ids[0]=(uint16_t)id;
    r->num_stages=1;r->laps[0]=(uint8_t)laps;
    r->opponents=(uint8_t)opponents;r->behavior=(uint8_t)behavior;
    r->icon_type=(uint8_t)icon;r->cash_value=payout;
}
int main(void) {
    UG2CareerIndex *idx=(UG2CareerIndex *)calloc(1,sizeof *idx);
    assert(idx);
    unsigned id=0;
    assert(career_route_track_id("ROUTESL4RA/Paths4013.bin",&id)&&id==4013);
    assert(career_route_track_id("Paths4061.bin",&id)&&id==4061);
    assert(!career_route_track_id("Paths4061.bin.bak",&id));
    assert(!career_route_track_id("../Paths12.bin",&id));
    assert(!career_route_track_id("Paths999999999999.bin",&id));
    assert(!career_route_track_id("ROUTES/Other4061.bin",&id));
    add(idx,"STAGE_1_CIRCUIT_1",1,4013,2,3,0,3,250);
    assert(career_event_candidates(idx,1,4013)==1);
    const CareerSourceRace *r=career_resolve_circuit(idx,1,
                    "ROUTESL4RA/Paths4013.bin",2,3,NULL);
    assert(r && !strcmp(r->id,"STAGE_1_CIRCUIT_1") && r->cash_value==250);
    assert(!career_resolve_circuit(idx,1,"Paths4013.bin",3,3,NULL));
    assert(!career_resolve_circuit(idx,1,"Paths4013.bin",2,2,NULL));
    assert(!career_resolve_circuit(idx,2,"Paths4013.bin",2,3,NULL));
    assert(!career_resolve_circuit(idx,1,"Paths4013.bin",2,3,"NOT_A_RACE"));
    add(idx,"S4_CIRCUIT_21",4,4061,3,3,0,3,2200);
    add(idx,"S4_CIRCUIT_22",4,4061,3,3,0,3,2500);
    assert(career_event_candidates(idx,4,4061)==2);
    assert(!career_resolve_circuit(idx,4,"Paths4061.bin",3,3,NULL));
    r=career_resolve_circuit(idx,4,"Paths4061.bin",3,3,"S4_CIRCUIT_22");
    assert(r && r->cash_value==2500);
    add(idx,"S4_SPON_CIRCUIT_7",4,4062,3,3,0,1,10000);
    add(idx,"S4_URL_2",4,4702,3,5,0,2,10000);
    add(idx,"S4_SUV_3",4,4063,3,3,0,3,10000);
    add(idx,"S4_H_CIRCUIT_1",4,4064,3,3,0,3,10000);
    add(idx,"S4_CIRCUIT_24",4,4065,3,3,1,3,10000);
    for(unsigned k=4062;k<=4065;k++)
        assert(!career_resolve_circuit(idx,4,"Paths4062.bin",3,3,NULL));
    assert(!career_resolve_circuit(idx,4,"Paths4063.bin",3,3,NULL));
    assert(!career_resolve_circuit(idx,4,"Paths4064.bin",3,3,NULL));
    assert(!career_resolve_circuit(idx,4,"Paths4065.bin",3,3,NULL));
    free(idx);
    puts("career_race_binding_test: PASS");
    return 0;
}
