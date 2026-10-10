#include "ug2_career_binding.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static CareerSourceRace *add(UG2CareerIndex *cat,const char *id,
                             unsigned route,unsigned stage,unsigned laps,
                             unsigned opponents,unsigned icon) {
    CareerSourceRace *r=&cat->race[cat->unique_races++];
    memset(r,0,sizeof *r);
    snprintf(r->id,sizeof r->id,"%s",id);
    r->track_ids[0]=(uint16_t)route;r->stage=(uint8_t)stage;
    r->laps[0]=(uint8_t)laps;r->opponents=(uint8_t)opponents;
    r->num_stages=1;r->icon_type=(uint8_t)icon;r->cash_value=350;
    return r;
}
int main(void) {
    UG2CareerIndex *c=calloc(1,sizeof *c);assert(c);
    assert(ug2_route_from_path("ROUTESL4RF/Paths4081.bin")==4081);
    assert(ug2_route_from_path("C:\\game\\Paths4002.bin")==4002);
    assert(ug2_route_from_path("Paths4081.bin.bak")==0);
    assert(ug2_route_from_path("Paths4081/abc.bin")==0);
    assert(ug2_route_from_path("../Paths999999.bin")==0);
    assert(ug2_route_from_path("../Paths0.bin")==0);
    const CareerSourceRace *found=NULL;
    add(c,"S3_CIRCUIT_10",4081,3,2,3,3);
    assert(ug2_career_map_count(c,3,4081)==1);
    assert(ug2_career_resolve_circuit(c,3,4081,NULL,2,3,&found)==UG2_BIND_MATCH);
    assert(ug2_career_resolve_circuit(c,3,4081,NULL,0,0,&found)==UG2_BIND_MATCH);
    assert(found && found->laps[0]==2 && found->opponents==3);
    assert(found && found->cash_value==350);
    assert(ug2_career_resolve_circuit(c,2,4081,NULL,2,3,&found)==UG2_BIND_NO_MATCH && !found);
    assert(ug2_career_resolve_circuit(c,3,4081,NULL,1,3,&found)==UG2_BIND_NO_MATCH);
    /* Zero is now an explicit pre-arm wildcard, not a valid race finish.
     * The actual first-place completion path always sends nai > 0. */
    assert(ug2_career_resolve_circuit(c,3,4081,NULL,2,0,&found)==UG2_BIND_MATCH);
    assert(ug2_career_resolve_circuit(c,3,4081,NULL,2,4,&found)==UG2_BIND_NO_MATCH);
    assert(ug2_career_resolve_circuit(c,3,4082,NULL,2,3,&found)==UG2_BIND_NO_MATCH);
    /* One game route may be shared by several races, with different payouts. */
    add(c,"S3_CIRCUIT_11",4081,3,2,3,3)->cash_value=1200;
    assert(ug2_career_map_count(c,3,4081)==2);
    assert(ug2_career_resolve_circuit(c,3,4081,NULL,2,3,&found)==UG2_BIND_AMBIGUOUS && !found);
    assert(ug2_career_resolve_circuit(c,3,4081,NULL,0,0,&found)==UG2_BIND_AMBIGUOUS);
    assert(ug2_career_resolve_circuit(c,3,4081,"S3_CIRCUIT_10",2,3,&found)==UG2_BIND_MATCH);
    assert(found && found->cash_value==350);
    assert(ug2_career_resolve_circuit(c,3,4081,"S3_CIRCUIT_11",2,3,&found)==UG2_BIND_MATCH);
    assert(found && found->cash_value==1200);
    assert(ug2_career_resolve_circuit(c,3,4081,"S3_H_CIRCUIT_11",2,3,&found)==UG2_BIND_NO_MATCH);
    /* URL, sponsored race and drift are discoverable on the map but cannot
     * be paid as if they were the legacy circuit game mode. */
    /* Retail stage 1 opens a 3-lap, 3-opponent circuit on Paths4013.bin.
     * The game must discover these values BEFORE arming a race, then verify
     * the same values at the finish. */
    add(c,"STAGE_1_CIRCUIT_1",4013,1,3,3,3)->cash_value=250;
    assert(ug2_career_resolve_circuit(c,1,4013,NULL,0,0,&found)==UG2_BIND_MATCH);
    assert(found && found->laps[0]==3 && found->opponents==3 &&
           found->cash_value==250);
    assert(ug2_career_resolve_circuit(c,1,4013,NULL,3,3,&found)==UG2_BIND_MATCH);
    assert(ug2_career_resolve_circuit(c,1,4013,NULL,2,3,&found)==UG2_BIND_NO_MATCH);
    add(c,"S3_SPON_CIRCUIT_12",4083,3,2,3,1);
    add(c,"S3_URL_12",4711,3,4,5,2);
    add(c,"S3_DRIFT_12",4312,3,3,0,3);
    /* Hidden, sponsor and wrong behavior cannot be transformed into a
     * normal paid race by supplying the exact career ID. */
    add(c,"S3_H_CIRCUIT_11",4082,3,2,3,3)->is_hidden=1;
    add(c,"S3_CIRCUIT_12",4084,3,2,3,3)->behavior=1;
    add(c,"S3_CIRCUIT_13",4085,3,2,3,3)->is_hidden=1;
    assert(ug2_career_resolve_circuit(c,3,4082,"S3_H_CIRCUIT_11",2,3,&found)==UG2_BIND_NO_MATCH);
    assert(ug2_career_resolve_circuit(c,3,4084,"S3_CIRCUIT_12",2,3,&found)==UG2_BIND_NO_MATCH);
    assert(ug2_career_resolve_circuit(c,3,4085,"S3_CIRCUIT_13",2,3,&found)==UG2_BIND_NO_MATCH);
    assert(ug2_career_map_count(c,3,4083)==1);
    assert(ug2_career_resolve_circuit(c,3,4083,NULL,2,3,&found)==UG2_BIND_NO_MATCH);
    assert(ug2_career_resolve_circuit(c,3,4711,NULL,4,5,&found)==UG2_BIND_NO_MATCH);
    assert(ug2_career_resolve_circuit(c,3,4312,NULL,3,0,&found)==UG2_BIND_NO_MATCH);
    free(c);
    puts("ug2_career_binding_test: PASS");
    return 0;
}
