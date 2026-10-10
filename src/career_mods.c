/* Safe C99 local payout overrides inspired by the MIT GlobalLib authoring
 * approach; this is NOT a binary GlobalB editor or a .NET runtime. */
#include "career_mods.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
int ug2_career_mod_apply_stream(UG2CareerIndex *catalog, FILE *f) {
    if(!catalog || !f || catalog->unique_races>UG2_CAREER_MAX_RACES)return 0;
    UG2CareerIndex *next=malloc(sizeof *next);
    if(!next)return 0;
    memcpy(next,catalog,sizeof *next);
    unsigned char seen[UG2_CAREER_MAX_RACES]={0};
    char line[256];
    int ok=1;
    while(fgets(line,sizeof line,f)) {
        size_t n=strlen(line);
        if(!n || (line[n-1]!='\n' && !feof(f))) {ok=0;break;}
        while(n && (line[n-1]=='\r'||line[n-1]=='\n'))line[--n]=0;
        char *key=line;
        while(*key==' '||*key=='\t')key++;
        if(!*key || *key=='#')continue;
        char *eq=strchr(key,'=');
        if(!eq || strchr(eq+1,'=')) {ok=0;break;}
        *eq++=0;
        char *key_end=key+strlen(key);
        while(key_end>key && isspace((unsigned char)key_end[-1]))*--key_end=0;
        while(*eq==' '||*eq=='\t')eq++;
        if(!*key || !*eq || strlen(key)>=CAREER_SOURCE_NAME){ok=0;break;}
        for(char *p=key;*p;p++)
            if(!((*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='_')){ok=0;break;}
        if(!ok)break;
        char *end=NULL;
        unsigned long value=strtoul(eq,&end,10);
        if(end==eq || value==0 || value>1000000ul){ok=0;break;}
        while(*end==' '||*end=='\t')end++;
        if(*end && *end!='#'){ok=0;break;}
        size_t ix=0;
        for(;ix<next->unique_races;ix++)
            if(strcmp(next->race[ix].id,key)==0)break;
        if(ix>=next->unique_races || seen[ix]) {ok=0;break;}
        seen[ix]=1;
        next->race[ix].cash_value=(uint32_t)value;
    }
    if(ferror(f))ok=0;
    if(ok)*catalog=*next;
    free(next);
    return ok;
}
int ug2_career_mod_apply_file(UG2CareerIndex *catalog,const char *path) {
    if(!path || !*path)return 0;
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    int good=ug2_career_mod_apply_stream(catalog,f);
    if(fclose(f))good=0;
    return good;
}
