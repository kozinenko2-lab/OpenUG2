/* Asset-free modding test: never processes proprietary GlobalB bytes. */
#define _POSIX_C_SOURCE 200809L
#include "ug2_mods.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void put(const char *path,const char *s) {
    FILE *f=fopen(path,"wb");assert(f);
    assert(fwrite(s,1,strlen(s),f)==strlen(s));assert(!fclose(f));
}
int main(void) {
    UG2CareerIndex *src=calloc(1,sizeof *src);
    UG2CareerIndex *mod=calloc(1,sizeof *mod);
    assert(src && mod);
    src->unique_races=2;
    strcpy(src->race[0].id,"S3_SPRINT_6");
    src->race[0].cash_value=350;
    strcpy(src->race[1].id,"S4_SPON_DRAG_7");
    src->race[1].cash_value=8000;
    *mod=*src;
    UG2ModInfo info={42,42};
    char filename[]="/tmp/ug2-mod-test-XXXXXX";
    int fd=mkstemp(filename);assert(fd>=0);assert(!close(fd));
    put(filename,"# comment\nOPENUG2_CAREER_MOD_V1\n"
                 "S3_SPRINT_6=1700\n@PROTOTYPE_CIRCUIT_CASH=875\n");
    assert(ug2_mod_apply_file(src,filename,mod,&info));
    assert(src->race[0].cash_value==350);
    assert(mod->race[0].cash_value==1700 &&
           mod->race[1].cash_value==8000);
    assert(info.edited_races==1 && info.prototype_circuit_cash==875);
    put(filename,"OPENUG2_CAREER_MOD_V1\n"
                 "S3_SPRINT_6=900\nS3_SPRINT_6=800\n");
    assert(!ug2_mod_apply_file(src,filename,mod,&info));
    assert(mod->race[0].cash_value==1700 && info.prototype_circuit_cash==875);
    put(filename,"OPENUG2_CAREER_MOD_V1\nMISSING_RACE=999\n");
    assert(!ug2_mod_apply_file(src,filename,mod,&info));
    put(filename,"OPENUG2_CAREER_MOD_V1\nS3_SPRINT_6=-1\n");
    assert(!ug2_mod_apply_file(src,filename,mod,&info));
    put(filename,"OPENUG2_CAREER_MOD_V1\nS3_SPRINT_6=10000001\n");
    assert(!ug2_mod_apply_file(src,filename,mod,&info));
    put(filename,"OPENUG2_CAREER_MOD_V1\n@PROTOTYPE_CIRCUIT_CASH=0\n");
    assert(ug2_mod_apply_file(src,filename,mod,&info));
    assert(info.edited_races==0 && info.prototype_circuit_cash==0);
    put(filename,"OPENUG2_CAREER_MOD_V1\nS4_SPON_DRAG_7=6000\n");
    assert(ug2_mod_apply_file(src,filename,mod,&info));
    assert(mod->race[0].cash_value==350 && mod->race[1].cash_value==6000);
    assert(info.prototype_circuit_cash==UG2_MOD_BASE_PROTOTYPE_CASH);
    put(filename,"OPENUG2_CAREER_MOD_V1\nS4_SPON_DRAG_7=6000junk\n");
    assert(!ug2_mod_apply_file(src,filename,mod,&info));
    put(filename,"OPENUG2_CAREER_MOD_V1\n#only comments\n");
    assert(ug2_mod_apply_file(src,filename,mod,&info));
    assert(info.edited_races==0 && info.prototype_circuit_cash==500);
    char oversized[420];memset(oversized,'Z',sizeof oversized-1);
    oversized[sizeof oversized-1]=0;
    put(filename,oversized);
    assert(!ug2_mod_apply_file(src,filename,mod,&info));
    assert(!ug2_mod_apply_file(src,"/does-not-exist/overrides",mod,&info));
    assert(!ug2_mod_apply_file(NULL,filename,mod,&info));
    unlink(filename);free(mod);free(src);
    puts("ug2_mods_test: PASS");
    return 0;
}
