/* Small, guarded overlay reader for H700 (no .NET, heap under 150 KiB).
 * NFSTools/GlobalLib (MIT) documents the original data; this independent
 * overlay is not a writer for any original EA-owned binary.
 */
#include "ug2_mods.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ascii_name(const char *s) {
    size_t n=0;
    if(!s) return 0;
    while(s[n]) {
        unsigned char c=(unsigned char)s[n];
        if(n>=CAREER_SOURCE_NAME-1 ||
           !((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))
            return 0;
        n++;
    }
    return n>0;
}
static int parse_cash(const char *s,uint32_t *out) {
    if(!s || !*s || !out) return 0;
    uint64_t v=0;
    for(const unsigned char *p=(const unsigned char*)s;*p;p++){
        if(*p<'0'||*p>'9')return 0;
        v=v*10u+(*p-'0');
        if(v>UG2_MOD_MAX_CASH)return 0;
    }
    *out=(uint32_t)v;
    return 1;
}
static int parse_file(FILE *f, UG2CareerIndex *copy, UG2ModInfo *info) {
    char line[256];
    unsigned seen_ids[UG2_CAREER_MAX_RACES]={0};
    unsigned seen_prototype=0, seen_header=0, lines=0, updates=0;
    long consumed=0;
    while(fgets(line,sizeof line,f)){
        size_t n=strlen(line);
        consumed+=(long)n;
        if(consumed>UG2_MOD_FILE_MAX || n==sizeof(line)-1 && line[n-1]!='\n')
            return 0; /* no truncated tokens */
        if(++lines>1024) return 0;
        while(n && (line[n-1]=='\n'||line[n-1]=='\r'))line[--n]=0;
        if(!n || line[0]=='#') continue;
        if(!seen_header) {
            if(strcmp(line,"OPENUG2_CAREER_MOD_V1"))return 0;
            seen_header=1; continue;
        }
        char *eq=strchr(line,'=');
        if(!eq||eq==line||!eq[1]||strchr(eq+1,'='))return 0;
        *eq++=0;
        uint32_t val=0;
        if(!parse_cash(eq,&val))return 0;
        if(!strcmp(line,"@PROTOTYPE_CIRCUIT_CASH")){
            if(seen_prototype++)return 0;
            info->prototype_circuit_cash=val;continue;
        }
        if(!ascii_name(line))return 0;
        unsigned found=0,idx=0;
        for(uint32_t i=0;i<copy->unique_races;i++){
            if(strcmp(copy->race[i].id,line)==0){idx=i;found++;}
        }
        if(found!=1||seen_ids[idx]++)return 0;
        copy->race[idx].cash_value=val;
        updates++;
    }
    if(ferror(f)||!seen_header)return 0;
    info->edited_races=updates;
    return 1;
}
int ug2_mod_apply_file(const UG2CareerIndex *original, const char *patch,
                       UG2CareerIndex *out, UG2ModInfo *info) {
    if(!original || !patch || !*patch || !out || !info ||
       original->unique_races>UG2_CAREER_MAX_RACES ||
       original->unique_races==0) return 0;
    FILE *f=fopen(patch,"rb");
    if(!f)return 0;
    UG2CareerIndex *copy=(UG2CareerIndex*)malloc(sizeof *copy);
    if(!copy){fclose(f);return 0;}
    *copy=*original;
    UG2ModInfo change={UG2_MOD_BASE_PROTOTYPE_CASH,0};
    int good=parse_file(f,copy,&change);
    if(fclose(f)!=0)good=0;
    if(good){*out=*copy;*info=change;}
    free(copy);
    return good;
}
