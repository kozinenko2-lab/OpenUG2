/* Asset-free boundary/endianness/path safety test for optional audit. */
#define _POSIX_C_SOURCE 200809L
#define UG2_PROBE_TESTING
#include "ug2_trigger_probe.c"
#include <assert.h>
#include <unistd.h>

static void write_bytes(const char *path,const unsigned char *data,size_t n) {
    FILE *f=fopen(path,"wb");assert(f);
    assert(fwrite(data,1,n,f)==n);
    assert(fclose(f)==0);
}
static char *capture(FILE *f) {
    assert(fflush(f)==0);
    assert(fseek(f,0,SEEK_END)==0);
    long n=ftell(f);assert(n>=0);
    assert(fseek(f,0,SEEK_SET)==0);
    char *s=malloc((size_t)n+1);assert(s);
    assert(fread(s,1,(size_t)n,f)==(size_t)n);
    s[n]=0;
    return s;
}
int main(void) {
    char dir[]="/tmp/ug2-trigger-probe-XXXXXX";
    assert(mkdtemp(dir));
    char tracks[512],global[512],front[512];
    char bun[512],bin[512],skip[512],ln[512];
    snprintf(tracks,sizeof tracks,"%s/TRACKS",dir);
    snprintf(global,sizeof global,"%s/GLOBAL",dir);
    snprintf(front,sizeof front,"%s/FRONTEND",dir);
    assert(mkdir(tracks,0700)==0 && mkdir(global,0700)==0 &&
           mkdir(front,0700)==0);
    snprintf(bun,sizeof bun,"%s/TRACKS/STREAML4RA.BUN",dir);
    snprintf(bin,sizeof bin,"%s/GLOBAL/test.bin",dir);
    snprintf(skip,sizeof skip,"%s/FRONTEND/noise.txt",dir);
    snprintf(ln,sizeof ln,"%s/FRONTEND/linked.BIN",dir);
    unsigned char *buf=calloc(1,PROBE_BUFFER+100);assert(buf);
    /* Deliberately split the 4-byte store across the 64K buffer boundary. */
    const unsigned char carlot[]={0x4B,0x6A,0xCC,0xFF};
    memcpy(buf+PROBE_BUFFER-2,carlot,sizeof carlot);
    /* Big-endian bytes are NOT the little-endian signature. */
    buf[64]=0xFF;buf[65]=0xCC;buf[66]=0x6A;buf[67]=0x4B;
    memcpy(buf+200,"TRIGGER_CC_CAR_LOT_1",strlen("TRIGGER_CC_CAR_LOT_1"));
    memcpy(buf+1400,"CRIB_1",strlen("CRIB_1"));
    write_bytes(bun,buf,PROBE_BUFFER+100);
    free(buf);
    const unsigned char key[]={0x92,0x0C,0x8D,0xF9,
                               0x02,0xE4,0x60,0xDD};
    write_bytes(bin,key,sizeof key);
    write_bytes(skip,carlot,sizeof carlot);
    /* Directory traversal must never follow symlinks out of the root. */
    assert(symlink("/etc/passwd",ln)==0);

    FILE *f=tmpfile();assert(f);
    TriggerProbe probe={0};probe.output=f;
    probe.byte_limit=UINT64_C(1024)*1024u;
    assert(probe_root(&probe,dir));
    assert(probe.files==2 && probe.errors==0 &&
           probe.skipped_symlinks==1 && probe.truncated==0);
    assert(probe.matches==6); /* 1 LE32 car lot, 1 ASCII car lot trigger,
                                 1 nested ASCII car lot label, 1 ASCII CRIB,
                                 1 opaque CRIB key, 1 DDAY A key */
    char *txt=capture(f);
    assert(strstr(txt,"offset=65534 encoding=LE32 identity=shop_trigger_CC_CAR_LOT_1"));
    assert(strstr(txt,"offset=200 encoding=ASCII identity=TRIGGER_CC_CAR_LOT_1"));
    assert(strstr(txt,"shop_trigger_CRIB_1_opaque"));
    assert(strstr(txt,"DDAY_EVENT_A"));
    assert(strstr(txt,"complete=yes"));
    assert(!strstr(txt,"noise.txt") && !strstr(txt,"linked.BIN"));
    free(txt);fclose(f);

    f=tmpfile();assert(f);
    TriggerProbe limited={0};limited.output=f;limited.byte_limit=64u;
    assert(probe_root(&limited,dir));
    assert(limited.truncated && limited.bytes==64 &&
           limited.matches==0 && limited.files==1);
    txt=capture(f);
    assert(strstr(txt,"complete=no"));
    free(txt);fclose(f);

    f=tmpfile();assert(f);
    TriggerProbe missing={0};missing.output=f;missing.byte_limit=512u;
    assert(!probe_root(&missing,"/missing-ug2-path-for-test"));
    fclose(f);
    unlink(ln);unlink(skip);unlink(bin);unlink(bun);
    rmdir(front);rmdir(global);rmdir(tracks);rmdir(dir);
    puts("ug2_trigger_probe_test: PASS");
    return 0;
}
