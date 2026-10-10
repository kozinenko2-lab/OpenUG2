/* No proprietary game data. Synthetic IDs in an in-memory career catalog. */
#define _POSIX_C_SOURCE 200809L
#include "ug2_career_mod.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void write_file(const char *path,const char *s){
    FILE *f=fopen(path,"wb");assert(f);
    assert(fwrite(s,1,strlen(s),f)==strlen(s));
    assert(fclose(f)==0);
}
int main(void){
    UG2CareerIndex *c=calloc(1,sizeof *c),*old=calloc(1,sizeof *old);
    assert(c&&old);
    c->unique_races=2;
    strcpy(c->race[0].id,"STAGE1_RACE_A");
    strcpy(c->race[1].id,"STAGE1_RACE_B");
    c->race[0].cash_value=125;
    c->race[1].cash_value=350;
    char filename[]="/tmp/openug2modXXXXXX";
    int fd=mkstemp(filename);assert(fd>=0);close(fd);
    unsigned count=987;
    write_file(filename,
        "# Local career overrides (engine files remain unchanged)\n"
        "cash STAGE1_RACE_A 1500\n"
        "cash STAGE1_RACE_B 0\n");
    assert(ug2_career_apply_mods_file(c,filename,&count));
    assert(count==2&&c->race[0].cash_value==1500&&
           c->race[1].cash_value==0);
    memcpy(old,c,sizeof *old);
    const char *invalid[]={
        "cash STAGE1_RACE_A 120\ncash STAGE1_RACE_A 130\n",
        "cash NOT_IN_THE_RETAIL_CATALOG 500\n",
        "cash ../STAGE1_RACE_A 20\n",
        "cash STAGE1_RACE_A -1\n",
        "cash STAGE1_RACE_A 10000001\n",
        "cash STAGE1_RACE_A 12 foo\n",
        "stage STAGE1_RACE_A 12\n",
        "cash STAGE1_RACE_A 1x\n",
        "cash STAGE1_RACE_A 999999999999999999999\n"
    };
    for(size_t i=0;i<sizeof invalid/sizeof invalid[0];i++) {
        write_file(filename,invalid[i]);
        count=987;
        assert(!ug2_career_apply_mods_file(c,filename,&count));
        assert(count==987 && !memcmp(c,old,sizeof *old));
    }
    write_file(filename,"cash STAGE1_RACE_A 88\ncash WRONG_RACE 1\n");
    assert(!ug2_career_apply_mods_file(c,filename,&count));
    assert(!memcmp(c,old,sizeof *old));
    assert(!ug2_career_apply_mods_file(c,"/nonexistent/path",&count));
    char oversized[400];memset(oversized,'X',sizeof oversized);
    FILE *f=fopen(filename,"wb");assert(f);
    assert(fwrite(oversized,1,sizeof oversized,f)==sizeof oversized);
    fclose(f);
    assert(!ug2_career_apply_mods_file(c,filename,&count));
    assert(!memcmp(c,old,sizeof *old));
    write_file(filename,"# only comments\n\n");
    assert(ug2_career_apply_mods_file(c,filename,&count) && count==0);
    assert(!memcmp(c,old,sizeof *old));
    unlink(filename);
    free(old);free(c);
    puts("ug2_career_mod_test: PASS");
    return 0;
}
