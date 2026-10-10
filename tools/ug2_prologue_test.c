/* Synthetic, asset-independent original DDAY chain and save regressions. */
#define _POSIX_C_SOURCE 200809L
#include "ug2_career_prologue.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static CareerSourceRace *add(UG2CareerIndex *c,const char *id) {
    assert(c && c->unique_races<UG2_CAREER_MAX_RACES);
    CareerSourceRace *r=&c->race[c->unique_races++];
    memset(r,0,sizeof *r);
    snprintf(r->id,sizeof r->id,"%s",id);
    r->stage=0;r->behavior=3;r->icon_type=0;
    r->num_stages=1;r->track_ids[0]=4000;
    return r;
}
int main(void) {
    UG2CareerIndex *cat=calloc(1,sizeof *cat);
    assert(cat);
    CareerSourceRace *a=add(cat,"DDAY_EVENT_A");
    CareerSourceRace *b=add(cat,"DDAY_EVENT_B");
    b->prerequisite_key=ug2_career_bin_hash(a->id);
    assert(b->prerequisite_key==UINT32_C(0xdd60e402));
    assert(ug2_career_bin_hash(b->id)==UINT32_C(0xdd60e403));
    CareerSourceRace *intro=add(cat,"STAGE_1_CIRCUIT_1");
    intro->stage=1;intro->behavior=0;intro->icon_type=3;
    intro->laps[0]=3;intro->track_ids[0]=4013;
    intro->opponents=3;intro->cash_value=250;
    intro->prerequisite_key=ug2_career_bin_hash(b->id);
    assert(ug2_career_prerequisite(cat,intro)==b);

    Career c;career_init(&c);c.money=123;
    assert(ug2_prologue_state(cat,&c,a->id)==UG2_PROLOGUE_READY);
    assert(ug2_prologue_state(cat,&c,b->id)==UG2_PROLOGUE_LOCKED);
    assert(ug2_prologue_state(cat,&c,intro->id)==UG2_PROLOGUE_INVALID);
    const CareerSourceRace *found=NULL;
    assert(ug2_career_resolve_circuit_for_profile(
        cat,&c,1,4013,intro->id,3,3,&found)==UG2_BIND_NO_MATCH);

    char dir[]="/tmp/openug2-prologue-XXXXXX";
    assert(mkdtemp(dir));
    char path[512],unwritable[512],backup[512];
    snprintf(path,sizeof path,"%s/career.dat",dir);
    snprintf(backup,sizeof backup,"%s/career.dat.bak",dir);
    snprintf(unwritable,sizeof unwritable,"%s/missing/career.dat",dir);
    Career before=c;
    assert(ug2_prologue_record_verified_finish(
        &c,cat,b->id,4000,1,path)==UG2_PROLOGUE_REJECTED);
    assert(ug2_prologue_record_verified_finish(
        &c,cat,a->id,4001,1,path)==UG2_PROLOGUE_REJECTED);
    assert(ug2_prologue_record_verified_finish(
        &c,cat,a->id,4000,0,path)==UG2_PROLOGUE_REJECTED);
    assert(ug2_prologue_record_verified_finish(
        &c,cat,a->id,4000,1,unwritable)==UG2_PROLOGUE_SAVE_FAILED);
    assert(!memcmp(&c,&before,sizeof c));
    assert(access(path,F_OK)!=0);
    assert(ug2_prologue_record_verified_finish(
        &c,cat,a->id,4000,1,path)==UG2_PROLOGUE_RECORDED);
    assert(c.seen_count==1 && c.money==123 && c.stage==1 &&
           c.total_wins==0 && c.world_wins==0 && c.sponsor_wins==0 &&
           c.url_wins==0);
    assert(career_has_win(&c,"UG2_ORIGINAL",a->id,CAREER_WORLD));
    assert(!career_has_win(&c,"UG2_ORIGINAL",b->id,CAREER_WORLD));
    assert(ug2_prologue_state(cat,&c,a->id)==UG2_PROLOGUE_DONE);
    assert(ug2_prologue_state(cat,&c,b->id)==UG2_PROLOGUE_READY);
    before=c;
    assert(ug2_prologue_record_verified_finish(
        &c,cat,a->id,4000,1,path)==UG2_PROLOGUE_ALREADY_RECORDED);
    assert(!memcmp(&c,&before,sizeof c));

    Career loaded;career_init(&loaded);
    assert(career_load(&loaded,path));
    assert(career_has_win(&loaded,"UG2_ORIGINAL",a->id,CAREER_WORLD));
    assert(ug2_prologue_state(cat,&loaded,b->id)==UG2_PROLOGUE_READY);
    before=c;
    assert(ug2_prologue_record_verified_finish(
        &c,cat,b->id,4000,1,unwritable)==UG2_PROLOGUE_SAVE_FAILED);
    assert(!memcmp(&c,&before,sizeof c));
    assert(ug2_prologue_record_verified_finish(
        &c,cat,b->id,4000,1,path)==UG2_PROLOGUE_RECORDED);
    assert(c.seen_count==2 && c.money==123 && c.total_wins==0 &&
           c.world_wins==0 && c.stage==1);
    assert(career_load(&loaded,path));
    assert(career_has_win(&loaded,"UG2_ORIGINAL",b->id,CAREER_WORLD));
    assert(ug2_career_resolve_circuit_for_profile(
        cat,&loaded,1,4013,intro->id,3,3,&found)==UG2_BIND_MATCH &&
           found==intro);
    assert(ug2_prologue_record_verified_finish(
        &loaded,cat,b->id,4000,1,path)==UG2_PROLOGUE_ALREADY_RECORDED);

    /* Corrupt/mismatched source values cannot unlock or record the chain. */
    b->prerequisite_key=0x12345678u;
    assert(ug2_prologue_state(cat,&loaded,b->id)==UG2_PROLOGUE_INVALID);
    before=loaded;
    assert(ug2_prologue_record_verified_finish(
        &loaded,cat,b->id,4000,1,path)==UG2_PROLOGUE_REJECTED);
    assert(!memcmp(&loaded,&before,sizeof loaded));
    b->prerequisite_key=ug2_career_bin_hash(a->id);
    a->opponents=1;
    assert(ug2_prologue_state(cat,&loaded,a->id)==UG2_PROLOGUE_INVALID);
    a->opponents=0;
    add(cat,"DDAY_EVENT_A");
    assert(ug2_prologue_state(cat,&loaded,b->id)==UG2_PROLOGUE_INVALID);
    assert(ug2_prologue_state(NULL,&loaded,b->id)==UG2_PROLOGUE_INVALID);
    assert(ug2_prologue_state(cat,NULL,b->id)==UG2_PROLOGUE_INVALID);

    unlink(path);unlink(backup);rmdir(dir);
    free(cat);
    puts("ug2_prologue_test: PASS");
    return 0;
}
