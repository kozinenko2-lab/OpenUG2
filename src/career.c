/* OpenUG2 independent career state and crash-resilient private save format. */
#define _POSIX_C_SOURCE 200809L
#include "career.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdint.h>

#define CAREER_VERSION 2u
#define CAREER_V1_HEADER 52u
#define CAREER_V2_PREFIX 56u
#define CAREER_V2_CAR_BYTES (CAREER_CAR_MODEL_CAP + CAREER_UPGRADE_SLOTS)
#define CAREER_MAX_FILE (CAREER_V2_PREFIX + CAREER_MAX_EVENTS * 8u + \
                         CAREER_MAX_GARAGE * CAREER_V2_CAR_BYTES + 4u)
static const unsigned char career_magic[8] = {'O','U','G','2','C','R','0','1'};

void career_init(Career *c) {
    if (!c) return;
    memset(c, 0, sizeof *c);
    c->stage = 1;
}
CareerRequirements career_requirements(unsigned stage) {
    /* Research-stage summary, NOT proven retail internal event IDs.
     * The required counters are intentionally distinct. */
    static const CareerRequirements requirements[5] = {
        {5,0,0,0,0}, {10,3,3,1,1}, {20,3,5,2,3},
        {30,3,7,3,6}, {35,3,9,4,10}
    };
    if (stage < 1 || stage > 5) return (CareerRequirements){0};
    return requirements[stage-1];
}
int career_stage_ready(const Career *c) {
    if (!c || c->stage < 1 || c->stage > CAREER_MAX_STAGE) return 0;
    CareerRequirements r = career_requirements(c->stage);
    return c->world_wins >= r.world_wins &&
           c->sponsor_wins >= r.sponsor_wins &&
           c->url_wins >= r.url_wins &&
           c->dvd_covers >= r.dvd_covers &&
           c->visual_rating >= r.stars;
}
int career_advance_stage(Career *c) {
    if (!c || c->stage >= CAREER_MAX_STAGE || !career_stage_ready(c)) return 0;
    c->stage++;
    c->world_wins = c->sponsor_wins = c->url_wins = c->dvd_covers = 0;
    return 1;
}
static uint64_t career_key(const char *track, const char *event, unsigned kind) {
    if (!track || !*track || !event || !*event) return 0;
    /* Unambiguous domain separated FNV-1a over original, case-sensitive IDs. */
    uint64_t h = UINT64_C(14695981039346656037);
    const char *p;
    for (p=track; *p; ++p) { h ^= (unsigned char)*p; h *= UINT64_C(1099511628211); }
    h ^= 0xffu; h *= UINT64_C(1099511628211);
    for (p=event; *p; ++p) { h ^= (unsigned char)*p; h *= UINT64_C(1099511628211); }
    h ^= 0xfeu; h *= UINT64_C(1099511628211);
    h ^= kind; h *= UINT64_C(1099511628211);
    return h ? h : 1;
}
static int has_key(const Career *c, uint64_t key) {
    if (!c || !key || c->seen_count > CAREER_MAX_EVENTS) return 0;
    for (uint32_t i=0; i<c->seen_count; ++i)
        if (c->wins[i].key == key) return 1;
    return 0;
}
int career_has_win(const Career *c, const char *track, const char *event,
                   CareerEventKind kind) {
    if (kind < CAREER_WORLD || kind > CAREER_URL) return 0;
    return has_key(c, career_key(track,event,(unsigned)kind));
}
int career_record_win(Career *c, const char *track, const char *event,
                      CareerEventKind kind, int position, int opponents,
                      uint32_t payout) {
    if (!c || c->stage < 1 || c->stage > 5 ||
        kind < CAREER_WORLD || kind > CAREER_URL ||
        position != 1 || opponents < 1 || c->seen_count >= CAREER_MAX_EVENTS)
        return 0;
    uint64_t key = career_key(track,event,(unsigned)kind);
    if (!key || has_key(c,key)) return 0;
    c->wins[c->seen_count++].key = key;
    c->total_wins++;
    if (kind==CAREER_WORLD) c->world_wins++;
    else if (kind==CAREER_SPONSOR) c->sponsor_wins++;
    else c->url_wins++;
    c->money = UINT32_MAX - c->money < payout ? UINT32_MAX : c->money+payout;
    return 1;
}
int career_record_original_prologue(Career *c, const char *id) {
    if(!c || c->stage!=1 || !id || c->seen_count>=CAREER_MAX_EVENTS ||
       (strcmp(id,"DDAY_EVENT_A") && strcmp(id,"DDAY_EVENT_B")))
        return 0;
    uint64_t key=career_key("UG2_ORIGINAL",id,(unsigned)CAREER_WORLD);
    if(!key || has_key(c,key))return 0;
    c->wins[c->seen_count++].key=key;
    /* These are stage-0 scripted events. They cannot count as ordinary
     * world-race victories and cannot award cash or advance stages. */
    return 1;
}
int career_record_cover(Career *c, const char *location, unsigned stars) {
    if (!c || !location || !*location || stars > 10 ||
        c->stage < 1 || c->stage > 5 || c->seen_count >= CAREER_MAX_EVENTS)
        return 0;
    uint64_t key = career_key("PHOTO", location, 10u);
    if (!key || has_key(c,key)) return 0;
    c->wins[c->seen_count++].key = key;
    c->dvd_covers++;
    if (stars > c->visual_rating) c->visual_rating = stars;
    return 1;
}
static int garage_model_valid(const char *model) {
    if (!model || !*model) return 0;
    size_t i;
    for (i=0; i<CAREER_CAR_MODEL_CAP; i++) {
        unsigned char c=(unsigned char)model[i];
        if (!c) return i>0;
        /* Reject paths, traversal, trailing spaces and noncanonical names. */
        if (!((c>='A' && c<='Z') || (c>='0' && c<='9') ||
              c=='_' || c=='-')) return 0;
    }
    return 0; /* no terminating NUL */
}
int career_garage_owns(const Career *c, const char *model) {
    if (!c || !garage_model_valid(model) || c->garage_count>CAREER_MAX_GARAGE)
        return 0;
    for (uint32_t i=0;i<c->garage_count;i++)
        if (strcmp(c->garage[i].model,model)==0) return 1;
    return 0;
}
const CareerOwnedCar *career_garage_active(const Career *c) {
    if (!c || !c->garage_count || c->garage_count>CAREER_MAX_GARAGE ||
        c->garage_selected>=c->garage_count) return NULL;
    return &c->garage[c->garage_selected];
}
int career_garage_claim_starter(Career *c, const char *model) {
    /* Also accepts upgraded v1 profiles from stages 2..5: old saves
     * never stored a garage, so recovery must not strand the player. */
    if (!c || c->stage<1 || c->stage>5 || c->garage_count ||
        !garage_model_valid(model))
        return 0;
    /* Starter selection is explicit: caller must verify retail catalog. */
    memset(&c->garage[0],0,sizeof c->garage[0]);
    strcpy(c->garage[0].model,model);
    c->garage_count=1;
    c->garage_selected=0;
    return 1;
}
int career_garage_purchase_car(Career *c, const char *model,
                               uint32_t price) {
    if (!c || !garage_model_valid(model) || price==0 || c->money<price ||
        !c->garage_count || c->garage_count>=CAREER_MAX_GARAGE ||
        career_garage_owns(c,model)) return 0;
    /* A shop must separately verify model availability and unlocks. */
    CareerOwnedCar *car=&c->garage[c->garage_count];
    memset(car,0,sizeof *car);
    strcpy(car->model,model);
    c->garage_count++;
    c->money-=price;
    return 1;
}
int career_garage_select(Career *c, uint32_t index) {
    if (!c || index>=c->garage_count || c->garage_count>CAREER_MAX_GARAGE)
        return 0;
    c->garage_selected=index;
    return 1;
}
int career_garage_purchase_upgrade(Career *c, CareerUpgradeSlot slot,
                                   uint8_t tier, uint32_t price) {
    if (!c || slot<CAREER_UPG_ENGINE || slot>CAREER_UPG_TIRES ||
        tier<1 || tier>3 || price==0 || c->money<price) return 0;
    CareerOwnedCar *car=(CareerOwnedCar *)career_garage_active(c);
    if (!car || tier<=car->tier[slot]) return 0;
    /* Purchase and cash mutation form one logical profile update. */
    car->tier[slot]=tier;
    c->money-=price;
    return 1;
}
static void put32(unsigned char *p, uint32_t x) {
    for (int i=0;i<4;i++) p[i]=(unsigned char)(x>>(i*8));
}
static uint32_t get32(const unsigned char *p) {
    uint32_t x=0;
    for (int i=0;i<4;i++) x|=(uint32_t)p[i]<<(i*8);
    return x;
}
static void put64(unsigned char *p, uint64_t x) {
    for (int i=0;i<8;i++) p[i]=(unsigned char)(x>>(i*8));
}
static uint64_t get64(const unsigned char *p) {
    uint64_t x=0;
    for (int i=0;i<8;i++) x|=(uint64_t)p[i]<<(i*8);
    return x;
}
static uint32_t checksum(const unsigned char *data, size_t n) {
    uint32_t v = UINT32_C(2166136261);
    for (size_t i=0;i<n;i++) {v^=data[i];v*=UINT32_C(16777619);}
    return v;
}
static int valid(const Career *c) {
    if (!c || c->stage<1 || c->stage>5 || c->seen_count>CAREER_MAX_EVENTS ||
        c->total_wins>c->seen_count || c->visual_rating>10 ||
        c->world_wins>c->total_wins || c->sponsor_wins>c->total_wins ||
        c->url_wins>c->total_wins || c->dvd_covers>c->seen_count ||
        c->garage_count>CAREER_MAX_GARAGE ||
        (c->garage_count && c->garage_selected>=c->garage_count) ||
        (!c->garage_count && c->garage_selected!=0))
        return 0;
    for (uint32_t i=0;i<c->garage_count;i++) {
        const CareerOwnedCar *car=&c->garage[i];
        if (!garage_model_valid(car->model)) return 0;
        for (unsigned j=0;j<CAREER_UPGRADE_SLOTS;j++)
            if (car->tier[j]>3) return 0;
        for (uint32_t j=0;j<i;j++)
            if (!strcmp(c->garage[j].model,car->model)) return 0;
    }
    for (uint32_t i=0;i<c->seen_count;i++) {
        if (!c->wins[i].key) return 0;
        for(uint32_t j=0;j<i;j++) if(c->wins[i].key==c->wins[j].key) return 0;
    }
    return 1;
}
static size_t encode(const Career *c, unsigned char *buffer) {
    if (!valid(c)) return 0;
    memcpy(buffer,career_magic,8);
    const uint32_t values[12] = {
        CAREER_VERSION,c->stage,c->money,c->world_wins,c->sponsor_wins,
        c->url_wins,c->dvd_covers,c->visual_rating,c->total_wins,c->seen_count,
        c->garage_count,c->garage_selected
    };
    for (int i=0;i<12;i++) put32(buffer+8+i*4,values[i]);
    size_t offset=CAREER_V2_PREFIX;
    for (uint32_t i=0;i<c->seen_count;i++,offset+=8)
        put64(buffer+offset,c->wins[i].key);
    for (uint32_t i=0;i<c->garage_count;i++) {
        /* Fixed-length ASCII model + eight owned package tiers.
         * Noncanonical string tails remain zeroed. */
        memset(buffer+offset,0,CAREER_V2_CAR_BYTES);
        size_t length=strlen(c->garage[i].model);
        memcpy(buffer+offset,c->garage[i].model,length);
        memcpy(buffer+offset+CAREER_CAR_MODEL_CAP,
               c->garage[i].tier,CAREER_UPGRADE_SLOTS);
        offset+=CAREER_V2_CAR_BYTES;
    }
    put32(buffer+offset,checksum(buffer,offset));
    return offset+4;
}
static int decode(Career *out, const unsigned char *bytes, size_t n) {
    if (n<CAREER_V1_HEADER || n>CAREER_MAX_FILE ||
        memcmp(bytes,career_magic,8)) return 0;
    uint32_t version=get32(bytes+8);
    if (version!=1u && version!=CAREER_VERSION) return 0;
    uint32_t count=get32(bytes+44);
    if (count>CAREER_MAX_EVENTS) return 0;
    uint32_t garage_count=0,garage_selected=0;
    size_t offset=48;
    if (version==CAREER_VERSION) {
        if (n<CAREER_V2_PREFIX+4) return 0;
        garage_count=get32(bytes+48);
        garage_selected=get32(bytes+52);
        offset=CAREER_V2_PREFIX;
    }
    if (garage_count>CAREER_MAX_GARAGE ||
        n!=offset+(size_t)count*8+
           (size_t)garage_count*CAREER_V2_CAR_BYTES+4 ||
        get32(bytes+n-4)!=checksum(bytes,n-4)) return 0;
    Career next; career_init(&next);
    next.stage=get32(bytes+12); next.money=get32(bytes+16);
    next.world_wins=get32(bytes+20); next.sponsor_wins=get32(bytes+24);
    next.url_wins=get32(bytes+28); next.dvd_covers=get32(bytes+32);
    next.visual_rating=get32(bytes+36);
    next.total_wins=get32(bytes+40); next.seen_count=count;
    for (uint32_t i=0;i<count;i++,offset+=8)
        next.wins[i].key=get64(bytes+offset);
    next.garage_count=garage_count;
    next.garage_selected=garage_selected;
    for (uint32_t i=0;i<garage_count;i++) {
        CareerOwnedCar *car=&next.garage[i];
        memcpy(car->model,bytes+offset,CAREER_CAR_MODEL_CAP);
        memcpy(car->tier,bytes+offset+CAREER_CAR_MODEL_CAP,CAREER_UPGRADE_SLOTS);
        offset+=CAREER_V2_CAR_BYTES;
    }
    if (!valid(&next)) return 0;
    *out=next;
    return 1;
}
static int load_one(Career *out, const char *path) {
    FILE *f=fopen(path,"rb");
    if (!f) return 0;
    unsigned char data[CAREER_MAX_FILE+1];
    size_t n=fread(data,1,sizeof data,f);
    int io_good = !ferror(f) && feof(f);
    if (fclose(f) != 0) io_good = 0;
    return io_good && decode(out,data,n);
}
static char *suffix(const char *path, const char *ext) {
    if (!path || !*path) return NULL;
    size_t a=strlen(path), b=strlen(ext);
    if (a>4096 || a+b+1<a) return NULL;
    char *s=malloc(a+b+1);
    if (!s) return NULL;
    memcpy(s,path,a);memcpy(s+a,ext,b+1);
    return s;
}
int career_load(Career *c, const char *path) {
    if (!c || !path) return 0;
    Career tmp;
    if (load_one(&tmp,path)) { *c=tmp; return 1; }
    char *backup=suffix(path,".bak");
    if (!backup) return 0;
    int ok=load_one(&tmp,backup);
    free(backup);
    if (ok) *c=tmp;
    return ok;
}
int career_save(const Career *c, const char *path) {
    if (!c || !path) return 0;
    unsigned char data[CAREER_MAX_FILE];
    size_t n=encode(c,data);
    if (!n) return 0;
    char *temp=suffix(path,".tmp"),*bak=suffix(path,".bak");
    if (!temp || !bak) { free(temp);free(bak);return 0; }
    FILE *f=fopen(temp,"wb");
    if (!f) {free(temp);free(bak);return 0;}
    int good=fwrite(data,1,n,f)==n && fflush(f)==0 &&
             fsync(fileno(f))==0;
    if (fclose(f)!=0) good=0;
    if (!good) {unlink(temp);free(temp);free(bak);return 0;}
    /* Back up only a validated old profile; never replace a good backup
     * with a corrupt/truncated primary. If power dies between two renames,
     * career_load falls back to the last-good .bak. */
    Career old;
    int had_valid_primary=load_one(&old,path);
    int moved=0;
    if (had_valid_primary) {
        if (rename(path,bak)!=0) {
            unlink(temp);free(temp);free(bak);return 0;
        }
        moved=1;
    }
    good=rename(temp,path)==0;
    if (!good) {
        if (moved) rename(bak,path);
        unlink(temp);
    }
    free(temp);free(bak);
    return good;
}
