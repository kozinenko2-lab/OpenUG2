/* OpenUG2 original-world trigger reference probe.
 *
 * READ-ONLY: scans selected original game archives for *references* to shop
 * and DDAY hashes/labels. A hit is NOT a location, spatial trigger, script
 * completion or permission to award a race win. Intended for optional,
 * bounded diagnostics on H700 and desktop; no whole-file allocations.
 *
 * This tool intentionally does not interpret compressed archives or attempt
 * to extract/provide commercial game data.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

enum { PROBE_BUFFER=65536, PROBE_RING=32, PROBE_DEPTH=9,
       PROBE_MAX_FILES=10000, PROBE_MAX_HITS=128 };
typedef struct {
    uint64_t byte_limit, bytes, files, matches;
    unsigned truncated, errors, skipped_symlinks;
    FILE *output;
} TriggerProbe;

static const struct {uint32_t hash;const char *label;} hash_needles[] = {
    {UINT32_C(0xFFCC6A4B),"shop_trigger_CC_CAR_LOT_1"},
    {UINT32_C(0xF98D0C92),"shop_trigger_CRIB_1_opaque"},
    {UINT32_C(0xDD60E402),"DDAY_EVENT_A"},
    {UINT32_C(0xDD60E403),"DDAY_EVENT_B"},
    {UINT32_C(0xEC17E618),"shop_name_CC_CAR_LOT_1_raw"},
    {UINT32_C(0x55446B8F),"shop_name_CRIB_1"}
};
static const char *string_needles[] = {
    "TRIGGER_CC_CAR_LOT_1","CC_CAR_LOT_1","CRIB_1"
};
static int probe_suffix(const char *name) {
    const char *dot=strrchr(name,'.');
    return dot && (!strcasecmp(dot,".bun") || !strcasecmp(dot,".bin") ||
                   !strcasecmp(dot,".lzc") || !strcasecmp(dot,".dat"));
}
static void probe_hit(TriggerProbe *probe,const char *path,
                      uint64_t offset,const char *needle,const char *encoding) {
    if(probe->matches>=PROBE_MAX_HITS) {probe->truncated=1;return;}
    probe->matches++;
    fprintf(probe->output,"REF file=%s offset=%" PRIu64
            " encoding=%s identity=%s (candidate only)\n",
            path,offset,encoding,needle);
}
static int probe_string_match(const char ring[PROBE_RING],
                              uint64_t seen,const char *needle) {
    const size_t len=strlen(needle);
    if(seen<len || len>PROBE_RING ||
       ring[(seen-1u)%PROBE_RING]!=needle[len-1u])return 0;
    for(size_t k=0;k<len;k++)
        if(ring[(seen-len+k)%PROBE_RING]!=needle[k])return 0;
    return 1;
}
/* Public for the synthetic unit test by including this file with
 * UG2_PROBE_TESTING defined, otherwise only the CLI uses it. */
static int probe_file(TriggerProbe *probe,const char *path) {
    if(!probe || !path)return 0;
    FILE *f=fopen(path,"rb");
    if(!f){probe->errors++;return 0;}
    uint8_t bytes[PROBE_BUFFER];
    char ring[PROBE_RING]={0};
    uint32_t word=0;
    uint64_t seen=0;
    probe->files++;
    int ok=1;
    while(probe->bytes<probe->byte_limit && probe->matches<PROBE_MAX_HITS) {
        uint64_t remaining=probe->byte_limit-probe->bytes;
        size_t want=remaining<sizeof bytes?(size_t)remaining:sizeof bytes;
        size_t count=fread(bytes,1,want,f);
        for(size_t i=0;i<count;i++) {
            uint8_t b=bytes[i];
            word=(word>>8) | ((uint32_t)b<<24);
            ring[seen%PROBE_RING]=(char)b;
            seen++;probe->bytes++;
            if(seen>=4)for(size_t j=0;j<sizeof hash_needles/sizeof *hash_needles;j++)
                if(word==hash_needles[j].hash)
                    probe_hit(probe,path,seen-4u,hash_needles[j].label,"LE32");
            for(size_t j=0;j<sizeof string_needles/sizeof *string_needles;j++)
                if(probe_string_match(ring,seen,string_needles[j]))
                    probe_hit(probe,path,seen-strlen(string_needles[j]),
                              string_needles[j],"ASCII");
            if(probe->matches>=PROBE_MAX_HITS)break;
        }
        if(count<want) {
            if(ferror(f)){probe->errors++;ok=0;}
            break;
        }
    }
    /* The budget boundary or hit limit means results are incomplete, even
     * when a particular file happened to end at exactly the byte boundary. */
    if(probe->bytes>=probe->byte_limit || probe->matches>=PROBE_MAX_HITS)
        probe->truncated=1;
    if(fclose(f)!=0){probe->errors++;ok=0;}
    return ok;
}
static int probe_walk(TriggerProbe *probe,const char *path,int depth) {
    if(!probe || !path)return 0;
    if(depth>PROBE_DEPTH) {probe->truncated=1;return 0;}
    if(probe->bytes>=probe->byte_limit || probe->files>=PROBE_MAX_FILES ||
       probe->matches>=PROBE_MAX_HITS) {
        probe->truncated=1;return 0;
    }
    struct stat st;
    if(lstat(path,&st)!=0){probe->errors++;return 0;}
    if(S_ISLNK(st.st_mode)){probe->skipped_symlinks++;return 1;}
    if(S_ISREG(st.st_mode))return probe_suffix(path)?probe_file(probe,path):1;
    if(!S_ISDIR(st.st_mode))return 1;
    DIR *d=opendir(path);
    if(!d){probe->errors++;return 0;}
    int ok=1;
    struct dirent *ent;
    while((ent=readdir(d))) {
        if(!strcmp(ent->d_name,".")||!strcmp(ent->d_name,".."))continue;
        char next[4096];
        int n=snprintf(next,sizeof next,"%s/%s",path,ent->d_name);
        if(n<=0 || (size_t)n>=sizeof next){probe->errors++;ok=0;continue;}
        if(!probe_walk(probe,next,depth+1))ok=0;
        if(probe->bytes>=probe->byte_limit ||
           probe->matches>=PROBE_MAX_HITS) {
            probe->truncated=1;break;
        }
    }
    if(closedir(d)!=0){probe->errors++;ok=0;}
    return ok;
}
static int probe_root(TriggerProbe *probe,const char *root) {
    if(!probe || !root || !*root || !probe->output)return 0;
    const char *dirs[]={"TRACKS","GLOBAL","FRONTEND"};
    struct stat st;
    if(lstat(root,&st)!=0 || !S_ISDIR(st.st_mode)) {
        fprintf(probe->output,"ERROR missing game root: %s\n",root);
        return 0;
    }
    fprintf(probe->output,"OpenUG2 original-world reference audit\n");
    fprintf(probe->output,"No reference is proof of a shop location or script finish.\n");
    fprintf(probe->output,"Scanning raw .BUN/.BIN/.LZC/.DAT; compressed bytes are not unpacked.\n");
    int present=0;
    for(size_t i=0;i<sizeof dirs/sizeof *dirs;i++){
        char p[4096];
        int n=snprintf(p,sizeof p,"%s/%s",root,dirs[i]);
        if(n<=0 || (size_t)n>=sizeof p){probe->errors++;continue;}
        if(lstat(p,&st)!=0)continue;
        if(!S_ISDIR(st.st_mode)){probe->errors++;continue;}
        present++;
        (void)probe_walk(probe,p,0);
        if(probe->truncated || probe->bytes>=probe->byte_limit)break;
    }
    fprintf(probe->output,"SUMMARY files=%" PRIu64 " bytes=%" PRIu64
            " references=%" PRIu64 " errors=%u symlinks_skipped=%u "
            "complete=%s\n",probe->files,probe->bytes,probe->matches,
            probe->errors,probe->skipped_symlinks,
            (present && !probe->truncated && !probe->errors)?"yes":"no");
    if(!present)fprintf(probe->output,"ERROR no TRACKS/GLOBAL/FRONTEND directories\n");
    return present && !probe->errors;
}
#ifndef UG2_PROBE_TESTING
int main(int argc,char **argv) {
    if(argc!=2 && argc!=4) {
        fprintf(stderr,"usage: %s GAME_ROOT [--max-mib 32..4096]\n",argv[0]);
        return 2;
    }
    unsigned mib=512;
    if(argc==4){
        if(strcmp(argv[2],"--max-mib"))return 2;
        char *end=NULL;errno=0;
        unsigned long value=strtoul(argv[3],&end,10);
        if(errno || !argv[3][0] || !end || *end ||
           value<32 || value>4096)return 2;
        mib=(unsigned)value;
    }
    TriggerProbe probe={0};
    probe.output=stdout;
    probe.byte_limit=(uint64_t)mib*UINT64_C(1024)*1024u;
    if(!probe_root(&probe,argv[1]))return 1;
    /* A partial audit must never be misreported as a complete negative. */
    return probe.truncated?3:0;
}
#endif
