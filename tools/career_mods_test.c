/* Modding regression: no game assets needed, no retail file is modified. */
#include "career_mods.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
static void run_case(UG2CareerIndex *c,const char *text,int success,unsigned reward) {
    FILE *f=tmpfile();assert(f);
    assert(fwrite(text,1,strlen(text),f)==strlen(text));
    rewind(f);
    int ok=ug2_career_mod_apply_stream(c,f);
    fclose(f);
    assert(ok==success);
    assert(c->race[0].cash_value==reward);
}
int main(void) {
    UG2CareerIndex *c=calloc(1,sizeof *c);assert(c);
    c->unique_races=2;
    strcpy(c->race[0].id,"STAGE_1_CIRCUIT_1");
    strcpy(c->race[1].id,"S3_SPRINT_6");
    c->race[0].cash_value=250;
    c->race[1].cash_value=350;
    run_case(c,"# Mod only affects this process\nSTAGE_1_CIRCUIT_1 = 500\n",1,500);
    run_case(c,"STAGE_1_CIRCUIT_1=900\nUNKNOWN=1\n",0,500);
    run_case(c,"STAGE_1_CIRCUIT_1=900\nSTAGE_1_CIRCUIT_1=999\n",0,500);
    run_case(c,"STAGE_1_CIRCUIT_1=0\n",0,500);
    run_case(c,"STAGE_1_CIRCUIT_1=999999999999999999999999\n",0,500);
    run_case(c,"../../WRONG=200\n",0,500);
    run_case(c,"STAGE_1_CIRCUIT_1=700\nS3_SPRINT_6=444\n",1,700);
    assert(c->race[1].cash_value==444);
    run_case(c,"STAGE_1_CIRCUIT_1=700\nS3_SPRINT_6=0\n",0,700);
    assert(c->race[1].cash_value==444);
    free(c);
    puts("career_mods_test: PASS");
}
