/* Career metadata from the user's own PC Underground 2 GlobalB.lzc.
 * No EA resources, game records or decompiled executable code are embedded. */
#include "ug2_career_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#define UG2_GLOBALB_MAX_BYTES (64u * 1024u * 1024u)
#define UG2_CAREER_MAIN_TAG UINT32_C(0x80034A10)

static uint32_t rd32(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1]<<8) |
           ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}
static int jdlz_decode(const uint8_t *src, size_t n, uint8_t *dst, size_t cap) {
    if (!src || !dst || n<16 || memcmp(src,"JDLZ",4) ||
        src[4]!=2 || src[5]!=0x10 || rd32(src+8)!=cap ||
        rd32(src+12)!=n || cap<8 || cap>UG2_GLOBALB_MAX_BYTES) return 0;
    size_t ip=16, op=0;
    unsigned flag1=1, flag2=1;
    while(op<cap) {
        if(flag1==1) {
            if(ip>=n) return 0;
            flag1=(unsigned)src[ip++] | 0x100u;
        }
        if(flag2==1) {
            if(ip>=n) return 0;
            flag2=(unsigned)src[ip++] | 0x100u;
        }
        if(flag1&1u) {
            if(n-ip<2) return 0;
            unsigned a=src[ip++],b=src[ip++];
            size_t length,distance;
            if(flag2&1u) {
                length=(size_t)(b|((a&0xf0u)<<4))+3u;
                distance=(a&0x0fu)+1u;
            } else {
                distance=(size_t)(b|((a&0xe0u)<<3))+17u;
                length=(a&0x1fu)+3u;
            }
            if(!distance || distance>op || length>cap-op) return 0;
            for(size_t k=0;k<length;k++) {dst[op]=dst[op-distance];op++;}
            flag2>>=1;
        } else {
            if(ip>=n) return 0;
            dst[op++]=src[ip++];
        }
        flag1>>=1;
    }
    /* A normal file may carry a short zero-filled alignment trailer. */
    if(n-ip>4) return 0;
    while(ip<n) if(src[ip++]!=0) return 0;
    return 1;
}
static uint8_t *read_globalb(const char *path, size_t *sz) {
    if (!path || !sz) return NULL;
    *sz=0;
    FILE *f=fopen(path,"rb");
    if(!f) return NULL;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return NULL;}
    long l=ftell(f);
    if(l<8 || (unsigned long)l>UG2_GLOBALB_MAX_BYTES || fseek(f,0,SEEK_SET)!=0){
        fclose(f);return NULL;
    }
    size_t length=(size_t)l;
    uint8_t *src=(uint8_t *)malloc(length);
    if(!src){fclose(f);return NULL;}
    int ok=fread(src,1,length,f)==length;
    if(fclose(f)!=0) ok=0;
    if(!ok) {free(src);return NULL;}
    if(length>=4 && memcmp(src,"JDLZ",4)==0){
        if(length<16){free(src);return NULL;}
        size_t usize=rd32(src+8);
        if(usize<8 || usize>UG2_GLOBALB_MAX_BYTES){free(src);return NULL;}
        uint8_t *unpacked=(uint8_t *)malloc(usize);
        if(!unpacked){free(src);return NULL;}
        ok=jdlz_decode(src,length,unpacked,usize);
        free(src);
        if(!ok){free(unpacked);return NULL;}
        *sz=usize;return unpacked;
    }
    *sz=length;
    return src;
}

typedef struct {
    UG2CareerIndex *index;
    uint32_t section;
    int invalid;
} Collector;
static int collect_entry(const CareerSourceEntry *entry, void *ctx) {
    Collector *c=(Collector *)ctx;
    UG2CareerIndex *dst=c->index;
    if(entry->kind==CAREER_SOURCE_RACE) {
        const CareerSourceRace *race=&entry->data.race;
        dst->total_race_records++;
        for(uint32_t i=0;i<dst->unique_races;i++) {
            CareerSourceRace *prior=&dst->race[i];
            if(strcmp(prior->id,race->id)==0) {
                if(prior->cash_value!=race->cash_value ||
                   prior->stage!=race->stage || prior->opponents!=race->opponents) {
                    c->invalid=1;return 0;
                }
                dst->repeated_races++;
                return 1;
            }
        }
        if(dst->unique_races>=UG2_CAREER_MAX_RACES){c->invalid=1;return 0;}
        uint32_t id=dst->unique_races++;
        dst->race[id]=*race;
        dst->section[id]=(uint8_t)c->section;
    } else if(entry->kind==CAREER_SOURCE_STAGE) {
        dst->stage_records++;
    } else if(entry->kind==CAREER_SOURCE_SPONSOR) {
        dst->sponsor_records++;
    } else return 0;
    return 1;
}
int ug2_career_scan(const uint8_t *bytes, size_t size, UG2CareerIndex *out) {
    if(!bytes || !out || size<8 || size>UG2_GLOBALB_MAX_BYTES) return 0;
    unsigned seen=0;
    /* Prevalidate the complete top-level section tree before making
     * any attempt to publish a result. A failed scan leaves out intact. */
    for(int pass=0;pass<2;pass++) {
        UG2CareerIndex *next=NULL;
        if(pass==1){next=(UG2CareerIndex *)calloc(1,sizeof *next);if(!next)return 0;}
        Collector col={next,0,0};
        size_t off=0;
        while(off<size) {
            if(size-off<8){free(next);return 0;}
            uint32_t tag=rd32(bytes+off);
            size_t len=rd32(bytes+off+4);
            if(len>size-off-8){free(next);return 0;}
            if(tag==UG2_CAREER_MAIN_TAG) {
                if(col.section>=UG2_CAREER_MAX_SECTIONS){free(next);return 0;}
                CareerSourceCounts n={0};
                int valid=career_source_iterate_main_block(bytes+off,8+len,
                    pass?collect_entry:NULL,pass?&col:NULL,&n);
                if(!valid){free(next);return 0;}
                if(pass==1) next->career_sections++;
                col.section++;
            }
            off+=8+len;
        }
        if(pass==0) seen=col.section;
        else {
            if(!seen || col.section!=seen || col.invalid){free(next);return 0;}
            *out=*next;
            free(next);
        }
    }
    return 1;
}
int ug2_career_load_file(const char *path, UG2CareerIndex *out) {
    if(!out)return 0;
    size_t size=0;
    uint8_t *bytes=read_globalb(path,&size);
    if(!bytes)return 0;
    int ok=ug2_career_scan(bytes,size,out);
    free(bytes);
    return ok;
}
