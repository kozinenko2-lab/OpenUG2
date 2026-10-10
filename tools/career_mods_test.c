/* Synthetic original catalog and safe sidecar override tests. */
#define _POSIX_C_SOURCE 200809L
#include "career_mods.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static void write_file(const char *path,const char *s){
    FILE *f=fopen(path,"wb");assert(f);
    size_t n=strlen(s);assert(fwrite(s,1,n,f)==n);assert(!fclose(f));
}
int main(void){
    UG2CareerIndex *c=calloc(1,sizeof *c);
    assert(c);
    c->unique_races=2;
    strcpy(c->race[0].id,"STAGE_1_CIRCUIT_1");
    strcpy(c->race[1].id,"S2_CIRCUIT_2");
    c->race[0].cash_value=250;c->race[1].cash_value=300;
    char path[]="/tmp/ug2-cash-mod-XXXXXX";
    int fd=mkstemp(path);assert(fd>=0);close(fd);
    unsigned count=0;
    write_file(path,"# custom career payouts\nSTAGE_1_CIRCUIT_1 = 420\nS2_CIRCUIT_2=500\n");
    assert(career_mods_apply_file(c,path,&count)&&count==2);
    assert(c->race[0].cash_value==420&&c->race[1].cash_value==500);
    write_file(path,"STAGE_1_CIRCUIT_1=100\nUNKNOWN=700\n");
    assert(!career_mods_apply_file(c,path,&count)&&count==0);
    assert(c->race[0].cash_value==420&&c->race[1].cash_value==500);
    write_file(path,"STAGE_1_CIRCUIT_1=50\nSTAGE_1_CIRCUIT_1=25000\n");
    assert(!career_mods_apply_file(c,path,&count)&&count==0);
    assert(c->race[0].cash_value==420);
    write_file(path,"STAGE_1_CIRCUIT_1=-200\n");
    assert(!career_mods_apply_file(c,path,&count));
    write_file(path,"STAGE_1_CIRCUIT_1=1000001\n");
    assert(!career_mods_apply_file(c,path,&count));
    write_file(path,"STAGE_1_CIRCUIT_1=100wrong\n");
    assert(!career_mods_apply_file(c,path,&count));
    assert(c->race[0].cash_value==420);
    write_file(path,"S2_CIRCUIT_2=0\n");
    assert(career_mods_apply_file(c,path,&count)&&count==1);
    assert(c->race[0].cash_value==420&&c->race[1].cash_value==0);
    unlink(path);free(c);
    puts("career_mods_test: PASS");
    return 0;
}
