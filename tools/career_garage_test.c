/* Asset-free ownership, purchase, durability, and v1 migration regression. */
#define _POSIX_C_SOURCE 200809L
#include "career.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>

static void put32(unsigned char *p, uint32_t v) {
    for (int i=0;i<4;i++) p[i]=(unsigned char)(v>>(8*i));
}
static void put64(unsigned char *p, uint64_t v) {
    for (int i=0;i<8;i++) p[i]=(unsigned char)(v>>(8*i));
}
static uint32_t checksum(const unsigned char *p, size_t n) {
    uint32_t v=UINT32_C(2166136261);
    for (size_t i=0;i<n;i++) {v^=p[i];v*=UINT32_C(16777619);}
    return v;
}
static void write_old_v1(const char *path) {
    unsigned char bytes[60]={0};
    memcpy(bytes,"OUG2CR01",8);
    put32(bytes+8,1);  /* v1 */
    put32(bytes+12,1); /* stage 1 */
    put32(bytes+16,3000);
    put32(bytes+20,1); /* one world race */
    put32(bytes+40,1); /* one unique race */
    put32(bytes+44,1); /* one achievement */
    put64(bytes+48,UINT64_C(0x123456789abcdef0));
    put32(bytes+56,checksum(bytes,56));
    FILE *f=fopen(path,"wb");assert(f);
    assert(fwrite(bytes,1,sizeof bytes,f)==sizeof bytes);
    assert(fclose(f)==0);
}
static void test_garage_purchases(void) {
    Career c;career_init(&c);
    assert(!career_garage_active(&c));
    assert(!career_garage_purchase_car(&c,"FOCUS",2000));
    /* A career migrated from v1 at stage 3 still needs one real starter. */
    Career legacy; career_init(&legacy);
    legacy.stage=3;
    assert(career_garage_claim_starter(&legacy,"MIATA"));
    assert(!career_garage_claim_starter(&legacy,"FOCUS"));
    assert(!career_garage_claim_starter(&c,"../FOCUS"));
    assert(!career_garage_claim_starter(&c,"focus"));
    assert(!career_garage_claim_starter(&c,""));
    assert(career_garage_claim_starter(&c,"FOCUS"));
    assert(!career_garage_claim_starter(&c,"MIATA"));
    assert(career_garage_owns(&c,"FOCUS"));
    assert(!career_garage_owns(&c,"../FOCUS"));
    assert(career_garage_active(&c) && !strcmp(career_garage_active(&c)->model,"FOCUS"));
    assert(!career_garage_purchase_car(&c,"FOCUS",50));
    assert(!career_garage_purchase_upgrade(&c,CAREER_UPG_ENGINE,1,500));
    c.money=2000;
    assert(!career_garage_purchase_car(&c,"MIATA",0));
    assert(!career_garage_purchase_car(&c,"MIATA",2500));
    assert(career_garage_purchase_car(&c,"MIATA",1200));
    assert(c.money==800 && c.garage_count==2);
    assert(career_garage_select(&c,1));
    assert(!career_garage_select(&c,8));
    assert(!career_garage_purchase_upgrade(&c,CAREER_UPG_ENGINE,4,100));
    assert(!career_garage_purchase_upgrade(&c,CAREER_UPG_ENGINE,1,0));
    assert(!career_garage_purchase_upgrade(&c,CAREER_UPG_ENGINE,1,900));
    assert(career_garage_purchase_upgrade(&c,CAREER_UPG_ENGINE,1,600));
    assert(c.money==200 && career_garage_active(&c)->tier[CAREER_UPG_ENGINE]==1);
    assert(!career_garage_purchase_upgrade(&c,CAREER_UPG_ENGINE,1,10));
    assert(!career_garage_purchase_upgrade(&c,CAREER_UPG_ENGINE,0,10));
    assert(!career_garage_purchase_upgrade(&c,(CareerUpgradeSlot)99,2,10));
    assert(career_garage_select(&c,0));
    assert(career_garage_active(&c)->tier[CAREER_UPG_ENGINE]==0);
    assert(career_garage_select(&c,1));
    assert(career_garage_active(&c)->tier[CAREER_UPG_ENGINE]==1);
    /* Purchase failures were atomic and cannot counterfeit a new car. */
    assert(c.garage_count==2 && c.money==200);
}
static void test_v1_v2_roundtrip(void) {
    char dir[]="/tmp/openug2-garage-XXXXXX";
    assert(mkdtemp(dir));
    char path[256],backup[280];
    snprintf(path,sizeof path,"%s/career.dat",dir);
    snprintf(backup,sizeof backup,"%s.bak",path);
    write_old_v1(path);
    Career migrated;
    career_init(&migrated);
    assert(career_load(&migrated,path));
    assert(migrated.stage==1 && migrated.money==3000);
    assert(migrated.total_wins==1 && migrated.wins[0].key==UINT64_C(0x123456789abcdef0));
    assert(migrated.garage_count==0);
    assert(career_garage_claim_starter(&migrated,"FOCUS"));
    assert(career_garage_purchase_car(&migrated,"MIATA",2000));
    assert(career_garage_select(&migrated,1));
    assert(career_garage_purchase_upgrade(&migrated,CAREER_UPG_NITROUS,2,400));
    assert(migrated.money==600);
    assert(career_save(&migrated,path));
    /* Save again so the backup also contains v2 garage ownership. */
    assert(career_save(&migrated,path));
    Career loaded; career_init(&loaded);
    assert(career_load(&loaded,path));
    assert(loaded.garage_count==2 && loaded.garage_selected==1);
    assert(loaded.money==600 && loaded.total_wins==1 &&
           loaded.wins[0].key==UINT64_C(0x123456789abcdef0));
    assert(career_garage_active(&loaded)->tier[CAREER_UPG_NITROUS]==2);
    assert(!strcmp(career_garage_active(&loaded)->model,"MIATA"));
    /* Break primary; restore intact v2 garage from last-good backup. */
    FILE *broken=fopen(path,"wb");assert(broken);
    assert(fwrite("bad",1,3,broken)==3 && fclose(broken)==0);
    career_init(&loaded);
    assert(career_load(&loaded,path));
    assert(loaded.money==600 && loaded.garage_count==2);
    /* Serialized model bytes and tiers must be validated before activation. */
    assert(!unlink(path));
    assert(!unlink(backup));
    assert(!rmdir(dir));
}
static void test_garage_capacity(void) {
    Career c;career_init(&c);
    assert(career_garage_claim_starter(&c,"FOCUS"));
    c.money=UINT32_MAX;
    for (int i=1;i<CAREER_MAX_GARAGE;i++) {
        char name[16];
        snprintf(name,sizeof name,"CAR%d",i);
        assert(career_garage_purchase_car(&c,name,1));
    }
    assert(c.garage_count==CAREER_MAX_GARAGE);
    assert(!career_garage_purchase_car(&c,"EXTRA",1));
    assert(!career_garage_purchase_car(&c,"../TRAVERSAL",1));
}
int main(void) {
    test_garage_purchases();
    test_v1_v2_roundtrip();
    test_garage_capacity();
    puts("career_garage_test: PASS");
    return 0;
}
