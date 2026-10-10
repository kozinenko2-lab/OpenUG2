/* Standalone career progression regression, no game data or graphics needed. */
#define _POSIX_C_SOURCE 200809L
#include "career.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static void test_progression(void) {
    Career c;
    career_init(&c);
    assert(c.stage == 1 && c.money == 0 && !career_stage_ready(&c));
    assert(!career_record_win(&c, "L4RA", "Paths4175", CAREER_WORLD, 1, 0, 500));
    assert(!career_record_win(&c, "L4RA", "Paths4175", CAREER_WORLD, 2, 3, 500));
    assert(!career_record_win(&c, "", "Paths4175", CAREER_WORLD, 1, 3, 500));
    assert(!career_record_win(&c, "L4RA", "", CAREER_WORLD, 1, 3, 500));
    for (int i=0; i<5; i++) {
        char event[40];
        snprintf(event,sizeof event,"Stage1/Event%d",i);
        assert(career_record_win(&c,"STREAML4RA",event,CAREER_WORLD,1,3,500));
        assert(career_has_win(&c,"STREAML4RA",event,CAREER_WORLD));
        assert(!career_record_win(&c,"STREAML4RA",event,CAREER_WORLD,1,3,500));
        assert(c.money == (uint32_t)(500*(i+1)));
    }
    assert(career_stage_ready(&c));
    assert(career_advance_stage(&c));
    assert(c.stage==2 && c.world_wins==0 && c.money==2500);
    assert(!career_advance_stage(&c));
    assert(career_record_win(&c,"STREAML4RB","Stage1/Event0",CAREER_WORLD,1,3,500));
    assert(c.world_wins==1); /* identical event name, different track */
    for (int i=1; i<10; i++) {
        char event[40];
        snprintf(event,sizeof event,"Stage2/Event%d",i);
        assert(career_record_win(&c,"STREAML4RB",event,CAREER_WORLD,1,3,100));
    }
    assert(!career_stage_ready(&c));
    for (int i=0;i<3;i++) {
        char name[40];
        snprintf(name,sizeof name,"Sponsor%d",i);
        assert(career_record_win(&c,"STREAML4RB",name,CAREER_SPONSOR,1,3,250));
        snprintf(name,sizeof name,"URL%d",i);
        assert(career_record_win(&c,"STREAML4RB",name,CAREER_URL,1,3,250));
    }
    assert(!career_stage_ready(&c));
    assert(career_record_cover(&c,"photo/first",1));
    assert(!career_record_cover(&c,"photo/first",10));
    assert(c.dvd_covers==1 && c.visual_rating==1);
    assert(career_stage_ready(&c));
    assert(career_advance_stage(&c) && c.stage==3);
    assert(c.world_wins==0 && c.url_wins==0 && c.dvd_covers==0);
    assert(c.total_wins==5+10+6 && c.seen_count==c.total_wins+1);
    assert(!career_record_cover(&c,"photo/bad",11));
    CareerRequirements r=career_requirements(5);
    assert(r.world_wins==35 && r.sponsor_wins==3 &&
           r.url_wins==9 && r.dvd_covers==4 && r.stars==10);
    assert(!career_requirements(0).world_wins);
    assert(!career_requirements(6).world_wins);
}

static void test_save_recovery(void) {
    char root[]="/tmp/openug2-career-XXXXXX";
    assert(mkdtemp(root));
    char file[256],backup[265],temp[265];
    snprintf(file,sizeof file,"%s/profile.bin",root);
    snprintf(backup,sizeof backup,"%s.bak",file);
    snprintf(temp,sizeof temp,"%s.tmp",file);
    Career p, loaded;
    career_init(&p);
    assert(!career_load(&loaded,file));
    assert(career_record_win(&p,"L4RA","Paths4175",CAREER_WORLD,1,2,500));
    assert(career_save(&p,file));
    assert(career_load(&loaded,file));
    assert(loaded.money==500 && loaded.stage==1);
    assert(loaded.total_wins==1 && loaded.seen_count==1);
    assert(!career_load(&loaded,"/this/file/cannot/exist"));
    assert(career_record_win(&p,"L4RA","Paths4176",CAREER_WORLD,1,2,500));
    assert(career_save(&p,file));
    assert(access(backup,F_OK)==0);
    assert(career_load(&loaded,file) && loaded.money==1000);
    /* A truncated primary must leave previous successful save usable. */
    FILE *bad=fopen(file,"wb"); assert(bad);
    assert(fwrite("corrupt",1,7,bad)==7);
    assert(fclose(bad)==0);
    career_init(&loaded);
    assert(career_load(&loaded,file));
    assert(loaded.money==500 && loaded.total_wins==1);
    /* Invalid primary must not overwrite a good backup on subsequent save. */
    assert(career_save(&p,file));
    assert(career_load(&loaded,file) && loaded.money==1000);
    FILE *bak=fopen(backup,"rb");assert(bak);
    assert(fgetc(bak)=='O');fclose(bak);
    /* Overflow cash is saturated; save format keeps full uint32 range. */
    p.money=UINT32_MAX-3;
    assert(career_record_win(&p,"L4RA","Paths4177",CAREER_WORLD,1,2,1000));
    assert(p.money==UINT32_MAX);
    assert(career_save(&p,file));
    assert(career_load(&loaded,file) && loaded.money==UINT32_MAX);
    /* Create failures do not damage existing profiles. */
    assert(!career_save(&p,"/this/does/not/exist/profile.bin"));
    assert(career_load(&loaded,file));
    assert(!access(file,F_OK));
    assert(!unlink(file));
    assert(!unlink(backup));
    if(access(temp,F_OK)==0) assert(!unlink(temp));
    assert(!rmdir(root));
}
static void test_capacity(void) {
    Career c;career_init(&c);
    for (int i=0;i<CAREER_MAX_EVENTS;i++) {
        char key[32];
        snprintf(key,sizeof key,"verified/%d",i);
        assert(career_record_win(&c,"L4RA",key,CAREER_WORLD,1,1,0));
    }
    assert(c.seen_count==CAREER_MAX_EVENTS);
    assert(!career_record_win(&c,"L4RA","one-more",CAREER_WORLD,1,1,0));
    assert(!career_record_cover(&c,"photo-one-more",10));
}
int main(void) {
    test_progression();
    test_save_recovery();
    test_capacity();
    puts("career_test: PASS");
    return 0;
}
