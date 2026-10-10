/* Asset-free regression for PC Underground 2 career file scanner. */
#define _POSIX_C_SOURCE 200809L
#include "ug2_career_file.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void wr16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void wr32(uint8_t *p,uint32_t x){for(int j=0;j<4;j++)p[j]=(uint8_t)(x>>(j*8));}
static size_t append(uint8_t *buf,size_t off,uint32_t tag,const uint8_t *p,size_t n){
    wr32(buf+off,tag);wr32(buf+off+4,(uint32_t)n);
    if(n)memcpy(buf+off+8,p,n);
    return off+8+n;
}
/* Synthetic event, not original data. */
static size_t make_section(uint8_t *out,int second){
    const uint8_t strings[]="UNITTEST_RACE\0EV_UNIT\0SPONSOR_UNIT\0";
    uint8_t race[0x88]={0},stage[0x50]={0},sponsor[0x10]={0};
    uint8_t shops[0xA0*2]={0};
    memcpy(shops,"TEST_CAR_LOT",sizeof("TEST_CAR_LOT"));
    memcpy(shops+0x20,"SCENE_TEST",sizeof("SCENE_TEST"));
    memcpy(shops+0x40,"CARLOT01.BIN",sizeof("CARLOT01.BIN"));
    shops[0x50]=4; /* GlobalLib CARLOT, stage zero */
    shops[0x51]=(uint8_t)(second?1:0); /* multi-section difference */
    wr32(shops+0x3C,0x12345678u);
    memcpy(shops+0xA0,"TEST_CRIB",sizeof("TEST_CRIB"));
    memcpy(shops+0xA0+0x40,"CRIB01.BIN",sizeof("CRIB01.BIN"));
    shops[0xA0+0x50]=0; /* garage */
    shops[0xA0+0x9C]=1; /* blocked until source event */
    wr32(shops+0xA0+0x74,0xAABBCCDDu);
    wr16(race,0);wr16(race+6,(uint16_t)sizeof("UNITTEST_RACE"));
    wr32(race+0x30,1250);race[0x37]=1;race[0x7c]=3;race[0x7e]=1;
    stage[0]=1;stage[1]=1;
    wr16(sponsor,(uint16_t)(sizeof("UNITTEST_RACE")+sizeof("EV_UNIT")));
    wr16(sponsor+2,75);wr16(sponsor+12,125);wr16(sponsor+14,450);
    size_t pos=8;
    pos=append(out,pos,0x34a1d,strings,sizeof strings);
    pos=append(out,pos,0x34a11,race,sizeof race);
    pos=append(out,pos,0x34a18,stage,sizeof stage);
    pos=append(out,pos,0x34a12,shops,sizeof shops);
    if(!second)pos=append(out,pos,0x34a19,sponsor,sizeof sponsor);
    pos=append(out,pos,0x33322211,NULL,0);
    wr32(out,0x80034a10);wr32(out+4,(uint32_t)pos-8);
    return pos;
}
static void test_scan(void){
    uint8_t section[1024]={0},input[4096]={0};
    size_t n=make_section(section,0),pos=0;
    pos=append(input,pos,0x00035000,NULL,0);
    memcpy(input+pos,section,n);pos+=n;
    UG2CareerIndex *c=calloc(1,sizeof *c),*snapshot=calloc(1,sizeof *c);
    assert(c&&snapshot);
    assert(ug2_career_scan(input,pos,c));
    assert(c->career_sections==1&&c->total_race_records==1&&
           c->unique_races==1&&c->stage_records==1&&c->sponsor_records==1 &&
           c->shop_records==2);
    assert(c->race[0].cash_value==1250&&!strcmp(c->race[0].id,"UNITTEST_RACE"));
    assert(c->section[0]==0);
    assert(!strcmp(c->shops[0].name,"TEST_CAR_LOT"));
    assert(c->shops[0].shop_type==4 && c->shops[0].stage==0 &&
           c->shops[0].initially_hidden==0 &&
           !strcmp(c->shops[0].intro_movie,"SCENE_TEST") &&
           !strcmp(c->shops[0].filename,"CARLOT01.BIN") &&
           c->shops[0].trigger_key==0x12345678u);
    assert(c->shops[1].unlocked_by_event==1 &&
           c->shops[1].required_event==0xAABBCCDDu);
    n=make_section(section,1);
    memcpy(input+pos,section,n);pos+=n;
    assert(ug2_career_scan(input,pos,c));
    assert(c->career_sections==2&&c->total_race_records==2&&
           c->unique_races==1&&c->repeated_races==1 &&
           c->shop_records==4 && c->shops[2].section==1 &&
           c->shops[2].initially_hidden==1);
    memcpy(snapshot,c,sizeof *c);
    /* Corrupted shop metadata must never publish a partial catalog. */
    size_t shops_off=8 + 8 + 8+sizeof("UNITTEST_RACE\0EV_UNIT\0SPONSOR_UNIT\0") +
        8+0x88 + 8+0x50 + 8; /* prefix + main + children + shop header */
    uint8_t saved=input[shops_off+0x50];
    input[shops_off+0x50]=99; /* undefined type */
    assert(!ug2_career_scan(input,pos,c));
    assert(!memcmp(snapshot,c,sizeof *c));
    input[shops_off+0x50]=saved;
    assert(!ug2_career_scan(input,pos-1,c));
    assert(!memcmp(snapshot,c,sizeof *c));
    wr32(input+4,UINT32_MAX);
    assert(!ug2_career_scan(input,pos,c));
    assert(!memcmp(snapshot,c,sizeof *c));
    wr32(input+4,0);
    size_t second_pos=pos-n;
    size_t second_race=second_pos+8+8+sizeof("UNITTEST_RACE\0EV_UNIT\0SPONSOR_UNIT\0")+8;
    wr32(input+second_race+0x30,333);
    assert(!ug2_career_scan(input,pos,c));
    assert(!memcmp(snapshot,c,sizeof *c));
    free(snapshot);free(c);
}
static size_t make_jdlz_literals(uint8_t *out,size_t cap,const uint8_t *raw,size_t n){
    size_t at=16;
    if(cap<16||n<8||n>UINT32_MAX)return 0;
    memcpy(out,"JDLZ",4);out[4]=2;out[5]=16;wr32(out+8,(uint32_t)n);
    for(size_t k=0;k<n;){
        if(k==0){if(cap-at<2)return 0;out[at++]=0;out[at++]=0;}
        else if(k%8==0){if(cap-at<1)return 0;out[at++]=0;}
        if(at>=cap)return 0;
        out[at++]=raw[k++];
    }
    wr32(out+12,(uint32_t)at);
    return at;
}
static void test_file(void){
    uint8_t block[1024]={0},packed[2048]={0};
    size_t n=make_section(block,0);
    size_t compressed=make_jdlz_literals(packed,sizeof packed,block,n);
    assert(compressed>0);
    char path[]="/tmp/ug2-globalb-XXXXXX";
    int fd=mkstemp(path);assert(fd>=0);
    FILE *f=fdopen(fd,"wb");assert(f);
    assert(fwrite(packed,1,compressed,f)==compressed);
    assert(fclose(f)==0);
    UG2CareerIndex *c=calloc(1,sizeof *c);assert(c);
    assert(ug2_career_load_file(path,c));
    assert(c->career_sections==1&&c->race[0].cash_value==1250);
    f=fopen(path,"wb");assert(f);
    assert(fwrite(packed,1,compressed-3,f)==compressed-3);
    assert(fclose(f)==0);
    assert(!ug2_career_load_file(path,c));
    assert(c->career_sections==1&&c->race[0].cash_value==1250);
    unlink(path);free(c);
}
int main(void){test_scan();test_file();puts("ug2_career_file_test: PASS");return 0;}
