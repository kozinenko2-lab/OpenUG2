/* Validated cash-only overlay for owners of the original NFSU2 PC data.
 * Entire update is transactional; partial files never alter source metadata.
 * Format: RETAIL_RACE_ID=1234 ; '#' introduces whole-line comments only. */
#include "career_mods.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define MOD_MAX_BYTES (32u*1024u)
#define MOD_MAX_ENTRIES 256u
#define MOD_MAX_PAYOUT 1000000ul

int career_mods_apply_file(UG2CareerIndex *catalog, const char *path,
                           unsigned *changed) {
    if(changed) *changed=0;
    if(!catalog || !path || !*path ||
       catalog->unique_races>UG2_CAREER_MAX_RACES) return 0;
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long total=ftell(f);
    if(total<0 || total>MOD_MAX_BYTES || fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 0;
    }
    uint32_t *prices=(uint32_t *)malloc(sizeof *prices*
                      (catalog->unique_races?catalog->unique_races:1));
    if(!prices){fclose(f);return 0;}
    uint8_t seen[UG2_CAREER_MAX_RACES]={0};
    for(uint32_t i=0;i<catalog->unique_races;i++)
        prices[i]=catalog->race[i].cash_value;
    char line[256];
    unsigned count=0;int ok=1;
    while(fgets(line,sizeof line,f)) {
        size_t n=strlen(line);
        if(n && line[n-1]!='\n' && !feof(f)) {ok=0;break;}
        while(n && (line[n-1]=='\n'||line[n-1]=='\r'))line[--n]=0;
        char *p=line;
        while(*p==' '||*p=='\t')p++;
        if(!*p||*p=='#'||*p==';')continue;
        if(++count>MOD_MAX_ENTRIES){ok=0;break;}
        char *eq=strchr(p,'=');
        if(!eq){ok=0;break;}
        *eq=0;
        char *tail=eq;
        while(tail>p && (tail[-1]==' '||tail[-1]=='\t')) *--tail=0;
        size_t keylen=strlen(p);
        if(!keylen||keylen>=CAREER_SOURCE_NAME ||
           strspn(p,"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=keylen) {
            ok=0;break;
        }
        char *value=eq+1;
        while(*value==' '||*value=='\t')value++;
        if(!*value||*value<'0'||*value>'9') {ok=0;break;}
        errno=0;char *end=NULL;
        unsigned long amount=strtoul(value,&end,10);
        while(*end==' '||*end=='\t')end++;
        if(errno || *end || amount>MOD_MAX_PAYOUT) {ok=0;break;}
        uint32_t i;
        for(i=0;i<catalog->unique_races;i++)
            if(!strcmp(catalog->race[i].id,p))break;
        if(i==catalog->unique_races || seen[i]){ok=0;break;}
        seen[i]=1;
        prices[i]=(uint32_t)amount;
    }
    if(ferror(f)||fclose(f)!=0)ok=0;
    if(ok)for(uint32_t i=0;i<catalog->unique_races;i++)
        if(seen[i])catalog->race[i].cash_value=prices[i];
    if(ok && changed)*changed=count;
    free(prices);
    return ok;
}
