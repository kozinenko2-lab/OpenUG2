/* Asset-free, bounded runtime overlays inspired by GlobalLib's editable
 * career metadata. Never writes the original GlobalB or profile on failure. */
#include "ug2_career_mod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define UG2_MOD_MAX_BYTES 65536u
typedef struct {unsigned index;uint32_t payout;} Override;
static int legal_name(const char *p) {
    if(!*p)return 0;
    size_t n=0;
    for(;*p;p++,n++) {
        unsigned char c=(unsigned char)*p;
        if(n>=CAREER_SOURCE_NAME-1 ||
           !((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return 0;
    }
    return 1;
}
static char *trim(char *p) {
    while(*p==' '||*p=='\t')p++;
    size_t n=strlen(p);
    while(n && (p[n-1]==' '||p[n-1]=='\t'||p[n-1]=='\r'))p[--n]=0;
    return p;
}
int ug2_career_mod_apply_file(const char *path,UG2CareerIndex *cat,
                              unsigned *changed) {
    if(!path||!cat||cat->unique_races>UG2_CAREER_MAX_RACES)return 0;
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    unsigned char *data=(unsigned char *)malloc(UG2_MOD_MAX_BYTES+1);
    if(!data){fclose(f);return 0;}
    size_t n=fread(data,1,UG2_MOD_MAX_BYTES+1,f);
    int ok=!ferror(f)&&feof(f)&&n<=UG2_MOD_MAX_BYTES &&
           !memchr(data,0,n);
    if(fclose(f)!=0)ok=0;
    if(!ok){free(data);return 0;}
    data[n]=0;
    Override overrides[UG2_CAREER_MAX_RACES];
    unsigned used=0;
    /* Parsing is atomic: apply only after EVERY line has validated. */
    size_t at=0;
    while(at<n) {
        size_t end=at;
        while(end<n && data[end]!='\n') {
            if(data[end]<9 || (data[end]>13 && data[end]<32) ||
               data[end]>126 || end-at>160){ok=0;break;}
            end++;
        }
        if(!ok)break;
        char hold=(char)data[end];
        data[end]=0;
        char *line=trim((char *)data+at);
        if(*line && *line!='#') {
            char *eq=strchr(line,'=');
            if(!eq || strchr(eq+1,'=')){ok=0;break;}
            *eq=0;char *name=trim(line), *number=trim(eq+1);
            if(!legal_name(name)||!*number){ok=0;break;}
            uint32_t money=0;
            for(const char *p=number;*p;p++) {
                if(*p<'0'||*p>'9'||money>100000u) {ok=0;break;}
                money=money*10u+(uint32_t)(*p-'0');
            }
            if(!ok||!money||money>1000000u){ok=0;break;}
            unsigned idx=cat->unique_races;
            for(unsigned i=0;i<cat->unique_races;i++) {
                if(strcmp(cat->race[i].id,name)==0){idx=i;break;}
            }
            if(idx==cat->unique_races){ok=0;break;} /* typo must not be silent */
            for(unsigned i=0;i<used;i++)
                if(overrides[i].index==idx){ok=0;break;}
            if(!ok||used>=UG2_CAREER_MAX_RACES){ok=0;break;}
            overrides[used++]=(Override){idx,money};
        }
        data[end]=(unsigned char)hold;
        at=end<n?end+1:n;
    }
    free(data);
    if(!ok)return 0;
    for(unsigned i=0;i<used;i++)
        cat->race[overrides[i].index].cash_value=overrides[i].payout;
    if(changed)*changed=used;
    return 1;
}
