/* Synthetic mapping and end-to-end profile accounting; no EA assets. */
#include "career_event_map.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(UG2CareerIndex *i,unsigned n,const char *id,
                uint16_t track,unsigned stage,unsigned behavior,int cash,
                unsigned unlock,unsigned opponents) {
    CareerSourceRace *r=&i->race[n];
    memset(r,0,sizeof *r);
    snprintf(r->id,sizeof r->id,"%s",id);
    r->track_ids[0]=track; r->stage=(uint8_t)stage;
    r->event_behavior=(uint8_t)behavior;
    r->num_stages=1; r->cash_value=(uint32_t)cash;
    r->unlock_method=(uint8_t)unlock;r->opponents=(uint8_t)opponents;
}
int main(void){
    UG2CareerIndex *idx=calloc(1,sizeof *idx);
    assert(idx);
    uint16_t path=0;
    assert(career_event_path_id("ROUTESL4RF/Paths4013.bin",&path)&&path==4013);
    assert(career_event_path_id("Paths4101.bin",&path)&&path==4101);
    assert(!career_event_path_id("../Paths4013.bin",&path));
    assert(!career_event_path_id("Paths4013.BIN",&path));
    assert(!career_event_path_id("Paths40130.bin",&path));
    assert(!career_event_path_id("Paths401x.bin",&path));
    assert(!career_event_path_id("Paths0001.bin",&path));
    put(idx,0,"STAGE_1_CIRCUIT_1",4013,1,0,250,0,3);
    put(idx,1,"STAGE_1_SPRINT_1",4101,1,1,250,0,3);
    put(idx,2,"S2_CIRCUIT_2",4014,2,0,300,1,3);
    put(idx,3,"S2_SPON_CIRCUIT_2",4014,2,0,1000,2,3);
    idx->unique_races=4;
    const CareerSourceRace *r=NULL;
    assert(career_event_resolve(idx,4013,1,0,NULL,&r)==CAREER_EVENT_MATCHED);
    assert(r && !strcmp(r->id,"STAGE_1_CIRCUIT_1"));
    assert(career_event_can_credit(r,1,3));
    Career c; career_init(&c);
    assert(career_record_win(&c,"RETAIL","STAGE_1_CIRCUIT_1",CAREER_WORLD,
                             1,3,r->cash_value));
    assert(c.money==250 && c.world_wins==1);
    assert(!career_record_win(&c,"RETAIL","STAGE_1_CIRCUIT_1",CAREER_WORLD,
                              1,3,r->cash_value));
    assert(c.money==250);
    assert(career_event_resolve(idx,4101,1,0,NULL,&r)==CAREER_EVENT_MISSING);
    assert(career_event_resolve(idx,4101,1,1,NULL,&r)==CAREER_EVENT_MATCHED);
    assert(!career_event_can_credit(r,1,3)); /* sprint AI not proven */
    assert(career_event_resolve(idx,4014,2,0,NULL,&r)==CAREER_EVENT_AMBIGUOUS);
    assert(r==NULL);
    assert(career_event_resolve(idx,4014,2,0,"S2_CIRCUIT_2",&r)==CAREER_EVENT_MATCHED);
    assert(career_event_can_credit(r,2,3));
    assert(!career_event_can_credit(r,2,2));
    assert(!career_event_can_credit(r,1,3));
    assert(career_event_resolve(idx,4014,2,0,"S2_SPON_CIRCUIT_2",&r)==CAREER_EVENT_MATCHED);
    assert(!career_event_can_credit(r,2,3));
    assert(career_event_resolve(idx,4014,2,0,"NOT_THE_SAME_RACE",&r)==CAREER_EVENT_MISSING);
    assert(r==NULL);
    free(idx);
    puts("career_event_map_test: PASS");
    return 0;
}
