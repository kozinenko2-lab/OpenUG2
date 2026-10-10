/* End-to-end, asset-free: retail event identity -> earned cash -> save.
 * The original GlobalB and game's graphical assets are never required. */
#define _POSIX_C_SOURCE 200809L
#include "ug2_career_credit.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static CareerSourceRace *add(UG2CareerIndex *cat,const char *name,
                             unsigned route,unsigned stage,unsigned prize) {
    assert(cat->unique_races < UG2_CAREER_MAX_RACES);
    CareerSourceRace *r=&cat->race[cat->unique_races++];
    memset(r,0,sizeof *r);
    snprintf(r->id,sizeof r->id,"%s",name);
    r->stage=(uint8_t)stage;
    r->cash_value=prize;
    r->unlock_method=1; /* AT_STAGE_START; stage-specific */
    r->required_specific_url=(uint8_t)(stage-1);
    r->num_stages=1;
    r->track_ids[0]=(uint16_t)route;
    r->laps[0]=2;
    r->opponents=3;
    r->icon_type=3;
    return r;
}
int main(void) {
    UG2CareerIndex *cat=calloc(1,sizeof *cat);
    assert(cat);
    CareerSourceRace *a=add(cat,"S3_CIRCUIT_10",4081,3,350);
    CareerSourceRace *b=add(cat,"S3_CIRCUIT_11",4081,3,1500);
    CareerSourceRace *hidden=add(cat,"S3_H_CIRCUIT_13",4084,3,600);
    hidden->is_hidden=1;
    CareerSourceRace *drift=add(cat,"S3_DRIFT_7",4312,3,700);
    drift->behavior=3;
    Career c;career_init(&c);c.stage=3;c.money=100;
    char directory[]="/tmp/openug2-credit-test-XXXXXX";
    assert(mkdtemp(directory));
    char path[512],absent[512];
    snprintf(path,sizeof path,"%s/career.dat",directory);
    snprintf(absent,sizeof absent,"%s/no-such-directory/career.dat",directory);
    const CareerSourceRace *event=NULL;
    Career before=c;
    assert(ug2_career_credit_circuit(&c,cat,4081,NULL,2,3,1,path,&event)
           ==UG2_CREDIT_AMBIGUOUS && !event);
    assert(!memcmp(&c,&before,sizeof c));
    assert(ug2_career_credit_circuit(&c,cat,4081,a->id,2,3,2,path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE && !event); /* runner-up */
    assert(ug2_career_credit_circuit(&c,cat,4081,a->id,2,0,1,path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE); /* no opponents */
    assert(ug2_career_credit_circuit(&c,cat,4081,a->id,1,3,1,path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE); /* different laps */
    a->unlock_method=3; /* race-count gate not yet implemented */
    assert(ug2_career_credit_circuit(&c,cat,4081,a->id,2,3,1,path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE);
    a->unlock_method=1;
    a->required_specific_url=3; /* wrong stage-start marker */
    assert(ug2_career_credit_circuit(&c,cat,4081,a->id,2,3,1,path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE);
    a->required_specific_url=2;

    assert(ug2_career_credit_circuit(&c,cat,4082,a->id,2,3,1,path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE); /* wrong physical route */
    assert(ug2_career_credit_circuit(&c,cat,4084,hidden->id,2,3,1,path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE); /* hidden */
    assert(ug2_career_credit_circuit(&c,cat,4312,drift->id,2,3,1,path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE); /* unsupported gameplay */
    assert(!memcmp(&c,&before,sizeof c));

    assert(ug2_career_credit_circuit(&c,cat,4081,a->id,2,3,1,absent,&event)
           ==UG2_CREDIT_SAVE_FAILED);
    assert(event==a && !memcmp(&c,&before,sizeof c));
    assert(access(path,F_OK)!=0);
    assert(ug2_career_credit_circuit(&c,cat,4081,a->id,2,3,1,path,&event)
           ==UG2_CREDIT_PAID);
    assert(event==a && c.money==450 && c.total_wins==1 &&
           c.world_wins==1 && c.stage==3);
    Career loaded;career_init(&loaded);
    assert(career_load(&loaded,path) && loaded.money==450 &&
           loaded.total_wins==1 && loaded.stage==3);
    before=c;
    assert(ug2_career_credit_circuit(&c,cat,4081,a->id,2,3,1,path,&event)
           ==UG2_CREDIT_ALREADY_PAID);
    assert(!memcmp(&c,&before,sizeof c));
    assert(ug2_career_credit_circuit(&loaded,cat,4081,a->id,2,3,1,path,&event)
           ==UG2_CREDIT_ALREADY_PAID);
    assert(ug2_career_credit_circuit(&c,cat,4081,b->id,2,3,1,path,&event)
           ==UG2_CREDIT_PAID);
    assert(event==b && c.money==1950 && c.total_wins==2 && c.stage==3);
    assert(career_load(&loaded,path) && loaded.money==1950 &&
           loaded.total_wins==2 && loaded.stage==3);
    assert(ug2_career_credit_circuit(&c,cat,4081,b->id,2,3,1,path,&event)
           ==UG2_CREDIT_ALREADY_PAID);
    /* Original stage-1 intro: requires a verifiably recorded DDAY_EVENT_B
     * win, NOT merely an original predecessor key in the catalog. */
    CareerSourceRace *pre=add(cat,"DDAY_EVENT_B",4000,0,0);
    pre->opponents=0; /* scripted prologue, not supported as AI circuit */
    CareerSourceRace *intro=add(cat,"STAGE_1_CIRCUIT_1",4013,1,250);
    intro->unlock_method=0;
    intro->prerequisite_key=ug2_career_bin_hash(pre->id);
    intro->laps[0]=3;
    Career first;career_init(&first);
    char intro_path[512];
    snprintf(intro_path,sizeof intro_path,"%s/intro.dat",directory);
    Career initial=first;
    assert(ug2_career_credit_circuit(&first,cat,4013,intro->id,
                                     3,3,1,intro_path,&event)
           ==UG2_CREDIT_NOT_ELIGIBLE && !event);
    assert(!memcmp(&first,&initial,sizeof first));
    /* Synthetic confirmed precursor: production may only call
     * career_record_win after independently verifying its finish. */
    assert(career_record_win(&first,"UG2_ORIGINAL",pre->id,
                             CAREER_WORLD,1,1,0));
    assert(career_save(&first,intro_path));
    Career loaded_first;career_init(&loaded_first);
    assert(career_load(&loaded_first,intro_path));
    first=loaded_first;
    Career before_intro=first;
    assert(ug2_career_credit_circuit(&first,cat,4013,intro->id,
                                     3,3,1,absent,&event)
           ==UG2_CREDIT_SAVE_FAILED);
    assert(event==intro && !memcmp(&first,&before_intro,sizeof first));
    assert(ug2_career_credit_circuit(&first,cat,4013,intro->id,
                                     3,3,1,intro_path,&event)
           ==UG2_CREDIT_PAID);
    assert(event==intro && first.money==250 && first.total_wins==2 &&
           first.stage==1);
    assert(career_load(&loaded_first,intro_path) &&
           loaded_first.money==250 && loaded_first.total_wins==2);
    assert(ug2_career_credit_circuit(&first,cat,4013,intro->id,
                                     3,3,1,intro_path,&event)
           ==UG2_CREDIT_ALREADY_PAID);

    unlink(intro_path);
    snprintf(intro_path,sizeof intro_path,"%s/intro.dat.bak",directory);
    unlink(intro_path);
    unlink(path);
    snprintf(path,sizeof path,"%s/career.dat.bak",directory);
    unlink(path);
    rmdir(directory);
    free(cat);
    puts("ug2_career_credit_test: PASS");
    return 0;
}
