/* Data-free regression: original authored race IDs, no EA game assets. */
#include "career_retail_circuit.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void add(UG2CareerIndex *idx,const char *name,unsigned stage,
                unsigned track,unsigned cash,unsigned opponents) {
    assert(idx->unique_races<UG2_CAREER_MAX_RACES);
    CareerSourceRace *e=&idx->race[idx->unique_races++];
    memset(e,0,sizeof *e);
    snprintf(e->id,sizeof e->id,"%s",name);
    e->stage=(uint8_t)stage;
    e->track_ids[0]=(uint16_t)track;
    e->num_stages=1;
    e->cash_value=cash;
    e->opponents=(uint8_t)opponents;
}
int main(void) {
    UG2CareerIndex *index=(UG2CareerIndex *)calloc(1,sizeof *index);
    assert(index);
    add(index,"STAGE_1_CIRCUIT_1",1,4013,250,3);
    add(index,"S2_CIRCUIT_2",2,4082,350,3);
    CareerRetailCircuit got;
    memset(&got,0x55,sizeof got);
    assert(career_retail_circuit_lookup(index,1,"ROUTESL4RF/Paths4013.bin",&got));
    assert(got.route_id==4013&&!strcmp(got.race->id,"STAGE_1_CIRCUIT_1"));
    assert(!career_retail_circuit_lookup(index,1,"ROUTESL4RF/Paths4082.bin",&got));
    assert(!career_retail_circuit_lookup(index,1,"ROUTESL4RF/Paths4013.bin/../X",&got));
    assert(!career_retail_circuit_lookup(index,1,"Paths4013.bin",&got));
    assert(!career_retail_circuit_lookup(index,1,"ROUTESL4RF/Paths4013.bin.bad",&got));
    assert(!career_retail_circuit_lookup(index,1,"ROUTESL4RF/Paths401X.bin",&got));
    Career save;career_init(&save);
    assert(!career_retail_circuit_win(&save,index,"ROUTESL4RF/Paths4013.bin",2,4,NULL));
    assert(!career_retail_circuit_win(&save,index,"ROUTESL4RF/Paths4013.bin",1,2,NULL));
    assert(save.money==0 && save.total_wins==0);
    assert(career_retail_circuit_win(&save,index,"ROUTESL4RF/Paths4013.bin",1,4,&got));
    assert(save.money==250 && save.world_wins==1 && save.total_wins==1);
    assert(!career_retail_circuit_win(&save,index,"ROUTESL4RF/Paths4013.bin",1,4,&got));
    assert(save.money==250 && save.total_wins==1);
    /* Original short-event and sponsor variants may share the same track.
     * Refuse to attribute an AI circuit finish to one guessed original ID. */
    add(index,"STAGE_1_SHORT_1",1,4013,900,3);
    assert(!career_retail_circuit_lookup(index,1,"ROUTESL4RF/Paths4013.bin",&got));
    assert(!career_retail_circuit_win(&save,index,"ROUTESL4RF/Paths4013.bin",1,4,&got));
    assert(save.money==250);
    /* A reused route from a different stage is NOT ambiguous at this stage. */
    index->unique_races=2;
    save.stage=2;
    assert(career_retail_circuit_lookup(index,2,"ROUTESL4RA/Paths4082.bin",&got));
    assert(career_retail_circuit_win(&save,index,"ROUTESL4RA/Paths4082.bin",1,3,&got));
    assert(save.money==600 && save.total_wins==2 && save.world_wins==2);
    index->race[1].num_stages=2;
    assert(!career_retail_circuit_lookup(index,2,"ROUTESL4RA/Paths4082.bin",&got));
    index->race[1].num_stages=1;
    strcpy(index->race[1].id,"S2_SPRINT_1");
    assert(!career_retail_circuit_lookup(index,2,"ROUTESL4RA/Paths4082.bin",&got));
    strcpy(index->race[1].id,"S2_CIRCUIT_2");
    index->race[1].cash_value=0;
    assert(!career_retail_circuit_lookup(index,2,"ROUTESL4RA/Paths4082.bin",&got));
    index->race[1].cash_value=350;
    index->unique_races=UG2_CAREER_MAX_RACES+1u;
    assert(!career_retail_circuit_lookup(index,2,"ROUTESL4RA/Paths4082.bin",&got));
    free(index);
    puts("career_retail_circuit_test: PASS");
    return 0;
}
