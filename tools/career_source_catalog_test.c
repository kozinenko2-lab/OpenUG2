/* Synthetic records only. No game files are required or distributed. */
#include "career_source_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static void p16(uint8_t *p, unsigned n) {
    p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8);
}
static void p32(uint8_t *p, uint32_t n) {
    for(int i=0;i<4;i++) p[i]=(uint8_t)(n>>(8*i));
}
static void race_test(void) {
    static const uint8_t names[]="RACE_ALPHA\0START_A\0";
    const unsigned trigger_at=(unsigned)sizeof("RACE_ALPHA");
    uint8_t d[0x88]={0};
    p16(d,0);p16(d+6,trigger_at);
    d[0x0c]=2; p32(d+0x10,0x12345678u);
    d[0x12]=6;d[0x13]=2;p32(d+0x14,100);
    p16(d+0x18,0x1234);d[0x1b]=3;
    p16(d+0x1c,0x4567);d[0x1f]=2;
    p32(d+0x30,1250);
    d[0x34]=3;d[0x37]=2;d[0x7c]=3;d[0x7e]=2;
    CareerSourceRace out;
    memset(&out,0xa5,sizeof out);
    assert(career_source_decode_race(d,sizeof d,names,sizeof names,&out));
    assert(!strcmp(out.id,"RACE_ALPHA") && !strcmp(out.trigger,"START_A"));
    assert(out.stage==2 && out.cash_value==1250 &&
           out.opponents==3 && out.num_stages==2);
    assert(out.track_ids[0]==0x1234 && out.laps[0]==3);
    assert(out.track_ids[1]==0x4567 && out.laps[1]==2);
    assert(out.unlock_method==2 && out.prerequisite_key==0x12345678u);
    assert(out.required_races==6 && out.required_urls==2);
    CareerSourceRace copy=out;
    assert(!career_source_decode_race(d,sizeof d-1,names,sizeof names,&out));
    assert(!career_source_decode_race(d,sizeof d,names,3,&out));
    assert(!memcmp(&copy,&out,sizeof copy));
    p32(d+0x30,UINT32_MAX); /* negative signed payout */
    assert(!career_source_decode_race(d,sizeof d,names,sizeof names,&out));
    assert(!memcmp(&copy,&out,sizeof copy));
    p32(d+0x30,1250);
    d[0x7c]=6;
    assert(!career_source_decode_race(d,sizeof d,names,sizeof names,&out));
    d[0x7c]=0; /* downhill drift with solo scoring is a legitimate record */
    assert(career_source_decode_race(d,sizeof d,names,sizeof names,&out));
    /* No arbitrary buffer overreads from bad string pointers. */
    p16(d+6,65535);
    assert(!career_source_decode_race(d,sizeof d,names,sizeof names,&out));
}
static void stage_test(void) {
    uint8_t d[0x50]={0};
    d[0]=3;d[1]=2;p16(d+2,325);p32(d+8,0x0abc1234);
    p32(d+0x28,0x55667788);
    d[0x30]=3;d[0x31]=1;d[0x32]=2;d[0x33]=4;d[0x34]=5;
    d[0x40]=8;
    CareerSourceStage c;
    memset(&c,0xb6,sizeof c);
    assert(career_source_decode_stage(d,sizeof d,&c));
    assert(c.id==3 && c.sponsors_to_choose==2 && c.outrun_cash_value==325);
    assert(c.sponsor_keys[0]==0x0abc1234 &&
           c.last_stage_event_key==0x55667788 && c.max_outruns==8);
    assert(c.map_limits[0]==3 && c.map_limits[2]==2 &&
           c.map_limits[4]==5);
    CareerSourceStage original=c;
    assert(!career_source_decode_stage(d,sizeof d-1,&c));
    assert(!memcmp(&c,&original,sizeof c));
    d[1]=6;assert(!career_source_decode_stage(d,sizeof d,&c));
}
static void sponsor_test(void) {
    static const uint8_t names[]="SPONSOR_A\0";
    uint8_t d[16]={0};
    p16(d,0);p16(d+2,140);d[4]=1;d[5]=2;d[6]=3;
    p16(d+12,1000);p16(d+14,2500);
    CareerSourceSponsor s;
    assert(career_source_decode_sponsor(d,sizeof d,names,sizeof names,&s));
    assert(!strcmp(s.id,"SPONSOR_A"));
    assert(s.cash_per_win==140 && s.sign_bonus==1000 &&
           s.potential_bonus==2500);
    assert(s.required_race_types[0]==1 && s.required_race_types[2]==3);
    assert(!career_source_decode_sponsor(d,15,names,sizeof names,&s));
    p16(d+2,65535);
    assert(!career_source_decode_sponsor(d,sizeof d,names,sizeof names,&s));
}
typedef struct {int races,stages,sponsors;} Visits;
static int visit(const CareerSourceEntry *e, void *ctx) {
    Visits *v=ctx;
    if(e->kind==CAREER_SOURCE_RACE) {
        assert(!strcmp(e->data.race.id,"RACE"));
        assert(e->data.race.cash_value==1234);
        v->races++;
    } else if(e->kind==CAREER_SOURCE_STAGE) {
        assert(e->data.stage.id==2);
        v->stages++;
    } else if(e->kind==CAREER_SOURCE_SPONSOR) {
        assert(!strcmp(e->data.sponsor.id,"SPONSOR"));
        v->sponsors++;
    } else assert(0);
    return 1;
}
static size_t write_child(uint8_t *dst,uint32_t id,const uint8_t *data,size_t n){
    p32(dst,id);p32(dst+4,(uint32_t)n);
    if(n) memcpy(dst+8,data,n);
    return n+8;
}
static void main_block_test(void) {
    static const uint8_t labels[]="RACE\0TRIGGER\0SPONSOR\0";
    uint8_t race[0x88]={0},stage[0x50]={0},sponsor[0x10]={0},none[4]={0};
    p16(race,0);p16(race+6,(unsigned)sizeof("RACE"));
    p32(race+0x30,1234);race[0x37]=2;
    race[0x7c]=3;race[0x7e]=1;
    stage[0]=2;stage[1]=1;
    p16(sponsor,(unsigned)(sizeof("RACE")+sizeof("TRIGGER")));
    p16(sponsor+2,100);
    uint8_t block[1024]={0};
    size_t at=8;
    at+=write_child(block+at,CAREER_SOURCE_BLOCK_STRINGS,labels,sizeof labels);
    at+=write_child(block+at,CAREER_SOURCE_BLOCK_RACES,race,sizeof race);
    at+=write_child(block+at,CAREER_SOURCE_BLOCK_STAGES,stage,sizeof stage);
    at+=write_child(block+at,CAREER_SOURCE_BLOCK_SPONSORS,sponsor,sizeof sponsor);
    at+=write_child(block+at,0x44556677u,none,sizeof none); /* unknown skipped */
    p32(block,CAREER_SOURCE_BLOCK_MAIN);
    p32(block+4,(uint32_t)at-8);
    CareerSourceCounts c={0};
    Visits visits={0};
    assert(career_source_iterate_main_block(block,at,visit,&visits,&c));
    assert(c.races==1 && c.stages==1 && c.sponsors==1);
    assert(visits.races==1 && visits.stages==1 && visits.sponsors==1);
    /* One corrupt source record must fail before any callbacks. */
    race[0x7e]=9;
    size_t race_at=8+8+sizeof labels+8;
    memcpy(block+race_at,race,sizeof race);
    visits=(Visits){0};
    assert(!career_source_iterate_main_block(block,at,visit,&visits,&c));
    assert(!visits.races && !visits.stages && !visits.sponsors);
    race[0x7e]=1;memcpy(block+race_at,race,sizeof race);
    /* Broken child length, broken main size and truncated input all rejected. */
    size_t length_at=8+8+sizeof labels+4;
    p32(block+length_at,UINT32_MAX);
    assert(!career_source_iterate_main_block(block,at,visit,&visits,&c));
    p32(block+length_at,(uint32_t)sizeof race);
    p32(block+4,(uint32_t)at); /* overdeclared main length */
    assert(!career_source_iterate_main_block(block,at,visit,&visits,&c));
    p32(block+4,(uint32_t)at-8);
    assert(!career_source_iterate_main_block(block,at-1,visit,&visits,&c));
    assert(career_source_iterate_main_block(block,at,NULL,NULL,&c));
}
int main(void) {
    race_test();stage_test();sponsor_test();main_block_test();
    puts("career_source_catalog_test: PASS");
    return 0;
}
