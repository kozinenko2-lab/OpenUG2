#define _POSIX_C_SOURCE 200809L
#include "ug2_career_mod.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
static void save(const char *path,const char *s){
    FILE *f=fopen(path,"wb");assert(f);
    assert(fwrite(s,1,strlen(s),f)==strlen(s));
    assert(fclose(f)==0);
}
int main(void) {
    UG2CareerIndex *c=(UG2CareerIndex*)calloc(1,sizeof *c);assert(c);
    c->unique_races=2;
    strcpy(c->race[0].id,"S3_CIRCUIT_10");
    strcpy(c->race[1].id,"S3_CIRCUIT_11");
    c->race[0].cash_value=350;c->race[1].cash_value=350;
    char path[]="/tmp/ug2-mod-test-XXXXXX";
    int fd=mkstemp(path);assert(fd>=0);close(fd);
    unsigned changed=77;
    save(path,"# game data stays read-only\nS3_CIRCUIT_10=450\nS3_CIRCUIT_11 = 1700\n");
    assert(ug2_career_mod_apply_file(path,c,&changed));
    assert(changed==2 && c->race[0].cash_value==450 &&
           c->race[1].cash_value==1700);
    save(path,"S3_CIRCUIT_10=500\nUNKNOWN=200\n");
    assert(!ug2_career_mod_apply_file(path,c,&changed));
    assert(c->race[0].cash_value==450&&c->race[1].cash_value==1700);
    save(path,"S3_CIRCUIT_10=500\nS3_CIRCUIT_10=100\n");
    assert(!ug2_career_mod_apply_file(path,c,&changed));
    assert(c->race[0].cash_value==450);
    save(path,"S3_CIRCUIT_10=1000001\n");
    assert(!ug2_career_mod_apply_file(path,c,&changed));
    save(path,"S3_CIRCUIT_10=-20\n");
    assert(!ug2_career_mod_apply_file(path,c,&changed));
    save(path,"S3_CIRCUIT_10=0\n");
    assert(!ug2_career_mod_apply_file(path,c,&changed));
    save(path,"# no overrides\n");
    changed=17;
    assert(ug2_career_mod_apply_file(path,c,&changed)&&changed==0);
    unlink(path);free(c);
    puts("ug2_career_mod_test: PASS");
    return 0;
}
