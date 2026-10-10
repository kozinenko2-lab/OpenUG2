#include "ug2_career_balance_mod.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
/* Stage, route, opponents and all original race IDs remain immutable.
 * Only a user-authored sidecar numeric reward may change. */
int ug2_career_apply_balance_mod(UG2CareerIndex *catalog,const char *path){
    if(!catalog || !path || !*path || !catalog->unique_races ||
       catalog->unique_races>UG2_CAREER_MAX_RACES) return 0;
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char line[256];
    uint32_t payouts[UG2_CAREER_MAX_RACES];
    uint8_t seen[UG2_CAREER_MAX_RACES]={0};
    for(uint32_t i=0;i<catalog->unique_races;i++)
        payouts[i]=catalog->race[i].cash_value;
    int good=1, header=0, rows=0;
    while(fgets(line,sizeof line,f)){
        size_t n=strlen(line);
        if(n==sizeof line-1 && line[n-1]!='\n'){good=0;break;}
        while(n && (line[n-1]=='\n'||line[n-1]=='\r'))line[--n]=0;
        if(!n||line[0]=='#')continue;
        if(!header){
            if(strcmp(line,"race_id,cash")) good=0;
            header=1;
            if(!good)break;
            continue;
        }
        char *comma=strchr(line,',');
        if(!comma || strchr(comma+1,',')){good=0;break;}
        *comma++=0;
        size_t idlen=strlen(line);
        if(!idlen || idlen>=CAREER_SOURCE_NAME || !*comma ||
           *comma=='+'||*comma=='-'){good=0;break;}
        for(size_t i=0;i<idlen;i++){
            const char ch=line[i];
            if(!((ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch=='_')){
                good=0;break;
            }
        }
        if(!good)break;
        for(const char *p=comma;*p;p++)if(*p<'0'||*p>'9'){good=0;break;}
        if(!good)break;
        errno=0;
        char *end=NULL;
        unsigned long amount=strtoul(comma,&end,10);
        if(errno || !end || *end || amount>1000000UL){good=0;break;}
        unsigned found=0;
        for(uint32_t i=0;i<catalog->unique_races;i++)
            if(!strcmp(catalog->race[i].id,line)){
                if(seen[i]){good=0;break;}
                seen[i]=1;payouts[i]=(uint32_t)amount;found=1;rows++;
                break;
            }
        if(!good||!found){good=0;break;}
    }
    if(ferror(f) || !header || rows==0)good=0;
    if(fclose(f)!=0)good=0;
    if(!good)return 0;
    for(uint32_t i=0;i<catalog->unique_races;i++)
        catalog->race[i].cash_value=payouts[i];
    return rows;
}
