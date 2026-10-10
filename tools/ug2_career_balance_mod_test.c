/* Asset-free tests for H700 career mod import without .NET or game files. */
#define _POSIX_C_SOURCE 200809L
#include "ug2_career_balance_mod.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static int write_text(const char *path,const char *content){
    FILE *f=fopen(path,"wb");
    if(!f)return 0;
    int ok=fwrite(content,1,strlen(content),f)==strlen(content);
    if(fclose(f)!=0)ok=0;
    return ok;
}
int main(void){
    UG2CareerIndex *db=calloc(1,sizeof *db);
    assert(db);
    db->unique_races=2;
    strcpy(db->race[0].id,"STAGE_1_CIRCUIT_1");
    strcpy(db->race[1].id,"S2_CIRCUIT_2");
    db->race[0].cash_value=250;
    db->race[1].cash_value=300;
    char path[]="/tmp/openug2-mods-XXXXXX";
    int fd=mkstemp(path);assert(fd>=0);close(fd);
    assert(write_text(path,"# player-owned career payout overrides\nrace_id,cash\n"
                   "STAGE_1_CIRCUIT_1,500\nS2_CIRCUIT_2,1200\n"));
    assert(ug2_career_apply_balance_mod(db,path)==2);
    assert(db->race[0].cash_value==500&&db->race[1].cash_value==1200);
    assert(write_text(path,"race_id,cash\nSTAGE_1_CIRCUIT_1,750\n"
                   "S2_CIRCUIT_2,INVALID\n"));
    assert(!ug2_career_apply_balance_mod(db,path));
    assert(db->race[0].cash_value==500&&db->race[1].cash_value==1200);
    assert(write_text(path,"race_id,cash\nSTAGE_1_CIRCUIT_1,750\n"
                   "STAGE_1_CIRCUIT_1,800\n"));
    assert(!ug2_career_apply_balance_mod(db,path));
    assert(db->race[0].cash_value==500);
    assert(write_text(path,"race_id,cash\nUNKNOWN_RACE,1\n"));
    assert(!ug2_career_apply_balance_mod(db,path));
    assert(write_text(path,"race_id,cash\nS2_CIRCUIT_2,1000001\n"));
    assert(!ug2_career_apply_balance_mod(db,path));
    assert(write_text(path,"race_id,cash\nSTAGE_1_CIRCUIT_1,0\n"));
    assert(ug2_career_apply_balance_mod(db,path)==1);
    assert(db->race[0].cash_value==0 && db->race[1].cash_value==1200);
    unlink(path);free(db);
    puts("ug2_career_balance_mod_test: PASS");
    return 0;
}
