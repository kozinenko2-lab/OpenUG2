/* H700-compatible, allocation-bounded career sidecar mods.
 * GlobalLib is a MIT C# .NET Framework editor and stays a PC authoring
 * reference; this deliberately tiny C99 reader has no external dependencies. */
#include "ug2_career_mod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>

#define UG2_MOD_MAX_SIZE (64L*1024L)
#define UG2_MOD_MAX_CASH 10000000ul

static int mod_id_valid(const char *s) {
    if (!s || !*s) return 0;
    size_t n=0;
    for (;s[n];n++) {
        unsigned char c=(unsigned char)s[n];
        if(n>=CAREER_SOURCE_NAME-1 ||
           !((c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_'))
            return 0;
    }
    return n>0;
}
int ug2_career_apply_mods_file(UG2CareerIndex *index, const char *path,
                               unsigned *changed) {
    if(!index || !path || !*path ||
       index->unique_races==0 || index->unique_races>UG2_CAREER_MAX_RACES)
        return 0;
    FILE *f=fopen(path,"rb");
    if(!f) return 0;
    int good=0;
    if(fseek(f,0,SEEK_END)!=0) goto end;
    long bytes=ftell(f);
    if(bytes<0 || bytes>UG2_MOD_MAX_SIZE || fseek(f,0,SEEK_SET)!=0)
        goto end;
    UG2CareerIndex *scratch=(UG2CareerIndex *)malloc(sizeof *scratch);
    if(!scratch) goto end;
    memcpy(scratch,index,sizeof *scratch);
    unsigned char seen[UG2_CAREER_MAX_RACES]={0};
    unsigned count=0;
    char line[256];
    while(fgets(line,sizeof line,f)) {
        size_t n=strlen(line);
        if(n && line[n-1]!='\n' && !feof(f)) goto reject; /* long line */
        char *p=line;
        while(*p && isspace((unsigned char)*p)) p++;
        if(*p=='#' || !*p) continue;
        char cmd[16]={0},id[CAREER_SOURCE_NAME]={0},num[32]={0},extra[2]={0};
        if(sscanf(p,"%15s %63s %31s %1s",cmd,id,num,extra)!=3 ||
           strcmp(cmd,"cash") || !mod_id_valid(id) || !*num)
            goto reject;
        for(const char *c=num;*c;c++)if(*c<'0'||*c>'9')goto reject;
        errno=0;
        char *after=NULL;
        unsigned long cash=strtoul(num,&after,10);
        if(errno || !after || *after || cash>UG2_MOD_MAX_CASH)
            goto reject;
        uint32_t match=UG2_CAREER_MAX_RACES;
        for(uint32_t i=0;i<scratch->unique_races;i++){
            if(!memchr(scratch->race[i].id,0,CAREER_SOURCE_NAME))
                goto reject;
            if(!strcmp(scratch->race[i].id,id)) {match=i;break;}
        }
        if(match==UG2_CAREER_MAX_RACES || seen[match])goto reject;
        seen[match]=1;
        scratch->race[match].cash_value=(uint32_t)cash;
        count++;
    }
    if(ferror(f))goto reject;
    memcpy(index,scratch,sizeof *index);
    if(changed)*changed=count;
    good=1;
reject:
    free(scratch);
end:
    fclose(f);
    return good;
}
