/* No retail data. Test route bindings, safe payouts and ambiguities. */
#include "ug2_career_binding.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void){
    UG2CareerIndex *c=calloc(1,sizeof *c);
    assert(c);
    c->unique_races=1;
    CareerSourceRace *r=&c->race[0];
    strcpy(r->id,"STAGE_1_CIRCUIT_1");
    strcpy(r->trigger,"EVENT_LOCATOR_01");
    r->stage=1;r->num_stages=1;r->track_ids[0]=4013;r->laps[0]=3;
    r->opponents=3;r->cash_value=250;
    UG2CareerBinding m={0};
    assert(ug2_career_route_id("ROUTESL4RF/Paths4013.bin")==4013);
    assert(ug2_career_route_id("Paths4013.bin.bak")==-1);
    assert(ug2_career_route_id("../Paths9999999.bin")==-1);
    assert(ug2_career_bind_circuit(c,r->id,"ROUTESL4RF/Paths4013.bin",1,3,&m));
    assert(!strcmp(m.race_id,"STAGE_1_CIRCUIT_1") &&
           m.route_id==4013 && m.payout==250 && m.laps==3 &&
           m.kind==CAREER_WORLD && m.opponents==3);
    assert(!ug2_career_bind_circuit(c,r->id,"ROUTESL4RF/Paths4013.bin",2,3,&m));
    assert(!ug2_career_bind_circuit(c,r->id,"ROUTESL4RF/Paths4013.bin",1,2,&m));
    assert(!ug2_career_bind_circuit(c,r->id,"ROUTESL4RF/Paths4014.bin",1,3,&m));
    assert(!ug2_career_bind_circuit(c,"S2_FAKE","ROUTESL4RF/Paths4013.bin",1,3,&m));
    assert(ug2_career_bind_circuit(c,NULL,"ROUTESL4RF/Paths4013.bin",1,3,&m));
    c->unique_races=2;
    r=&c->race[1];*r=c->race[0];strcpy(r->id,"STAGE_1_CIRCUIT_2");
    assert(!ug2_career_bind_circuit(c,NULL,"ROUTESL4RF/Paths4013.bin",1,3,&m));
    assert(ug2_career_bind_circuit(c,"STAGE_1_CIRCUIT_1",
                                   "ROUTESL4RF/Paths4013.bin",1,3,&m));
    c->unique_races=3;
    r=&c->race[2];*r=c->race[0];
    strcpy(r->id,"S1_SPON_CIRCUIT_1");r->track_ids[0]=4031;
    assert(ug2_career_bind_circuit(c,r->id,"Paths4031.bin",1,3,&m));
    assert(m.kind==CAREER_SPONSOR);
    strcpy(r->id,"S1_URL_CIRCUIT_1");
    assert(ug2_career_bind_circuit(c,r->id,"Paths4031.bin",1,3,&m));
    assert(m.kind==CAREER_URL);
    strcpy(r->id,"S1_DRIFT_1");
    assert(!ug2_career_bind_circuit(c,r->id,"Paths4031.bin",1,3,&m));
    free(c);
    puts("ug2_career_binding_test: PASS");
    return 0;
}
