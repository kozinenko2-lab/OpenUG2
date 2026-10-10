/* Synthetic IDs and prizes. No EA game assets. */
#include "career_event_bridge.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(UG2CareerIndex *c,unsigned i,const char *name,
                unsigned stage,unsigned track,unsigned reward,unsigned count,
                unsigned stages){
    CareerSourceRace *r=&c->race[i];memset(r,0,sizeof *r);
    snprintf(r->id,sizeof r->id,"%s",name);
    snprintf(r->trigger,sizeof r->trigger,"TRIGGER_%u",i);
    r->stage=(uint8_t)stage;r->track_ids[0]=(uint16_t)track;
    r->cash_value=reward;r->opponents=(uint8_t)count;
    r->num_stages=(uint8_t)stages;
}
int main(void){
    UG2CareerIndex *c=calloc(1,sizeof *c);
    assert(c);
    c->unique_races=5;
    put(c,0,"STAGE_1_CIRCUIT_1",1,4001,250,3,1);
    put(c,1,"S3_DRAG_4",3,4201,350,3,1);
    put(c,2,"S3_SPON_DRAG_6",3,4201,1500,3,1);
    put(c,3,"S4_URL_3",4,4702,5000,3,2);
    put(c,4,"S5_SPRINT_21",5,4142,2000,3,1);
    UG2CareerSelection s={0},last={0};
    Career profile;career_init(&profile);
    assert(ug2_career_select(c,"STAGE_1_CIRCUIT_1",1,&s));
    assert(s.route_id==4001 && s.race.cash_value==250 && s.kind==CAREER_WORLD);
    last=s;
    assert(!ug2_career_select(c,"STAGE_1_CIRCUIT_1",2,&s));
    assert(!memcmp(&s,&last,sizeof s));
    assert(!ug2_career_select(c,"UNLISTED",1,&s));
    assert(ug2_career_unique_route(c,1,4001,&s));
    assert(!ug2_career_unique_route(c,3,4201,&s)); /* two rewards: ambiguous */
    assert(!ug2_career_select(c,"S4_URL_3",4,&s));   /* multi-stage URL blocked */
    unsigned money=profile.money;
    assert(!ug2_career_award(&profile,&s,4001,1,1,1,3));
    assert(profile.money==money && profile.seen_count==0);
    s=last;
    assert(!ug2_career_award(&profile,&s,4002,1,1,1,3));
    assert(!ug2_career_award(&profile,&s,4001,0,1,1,3));
    assert(!ug2_career_award(&profile,&s,4001,1,0,1,3));
    assert(!ug2_career_award(&profile,&s,4001,1,1,2,3));
    assert(!ug2_career_award(&profile,&s,4001,1,1,1,2));
    assert(profile.money==money && !profile.seen_count);
    assert(ug2_career_award(&profile,&s,4001,1,1,1,3));
    assert(profile.money==money+250 && profile.world_wins==1);
    assert(!ug2_career_award(&profile,&s,4001,1,1,1,3)); /* repeat */
    assert(profile.money==money+250 && profile.seen_count==1);
    profile.stage=3;
    assert(ug2_career_select(c,"S3_SPON_DRAG_6",3,&s));
    assert(s.kind==CAREER_SPONSOR && s.route_id==4201);
    assert(ug2_career_award(&profile,&s,4201,1,1,1,3));
    assert(profile.money==1750 && profile.sponsor_wins==1 && profile.seen_count==2);
    assert(ug2_career_select(c,"S3_DRAG_4",3,&s));
    assert(ug2_career_award(&profile,&s,4201,1,1,1,3));
    assert(profile.money==2100 && profile.world_wins==2 && profile.seen_count==3);
    /* Same path identity across stages must NEVER select a stage's wrong race. */
    profile.stage=1;
    assert(!ug2_career_award(&profile,&s,4201,1,1,1,3));
    assert(profile.money==2100);
    free(c);
    puts("career_event_bridge_test: PASS");
    return 0;
}
