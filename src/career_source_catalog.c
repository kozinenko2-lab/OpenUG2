/* Independent, allocation-free C99 PC career-record metadata decoder.
 * Offsets sourced from MIT-licensed NFSTools/GlobalLib Underground2 gameplay
 * disassemblers (GCareerRace, GCareerStage, Sponsor), not lifted EA code.
 * This decoder consumes ONLY extracted, bounded records. It does not yet
 * parse or decompress GlobalB.lzc, resolve hashes or unlock retail events. */
#include "career_source_catalog.h"
#include <string.h>
#include <stdint.h>
#include <limits.h>

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
           ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static int name_at(const uint8_t *strings, size_t length, uint16_t offset,
                   char out[CAREER_SOURCE_NAME]) {
    if (!strings || offset >= length) return 0;
    size_t i=0;
    while (i<CAREER_SOURCE_NAME && (size_t)offset+i < length) {
        const unsigned char c=strings[offset+i];
        if (!c) {if(!i) return 0;out[i]=0;return 1;}
        if (c<32 || c>=127) return 0; /* metadata IDs use ASCII */
        out[i++]=(char)c;
    }
    return 0; /* unterminated / excessive field length */
}
int career_source_decode_race(const uint8_t *d, size_t len,
                              const uint8_t *s, size_t slen,
                              CareerSourceRace *out) {
    if (!d || !out || len < 0x88) return 0;
    CareerSourceRace next={0};
    if (!name_at(s,slen,le16(d),next.id) ||
        !name_at(s,slen,le16(d+6),next.trigger))
        return 0;
    /* GCareerRace/Disassemble.cs offsets: 0x10: unlock; 0x18: route IDs,
     * 0x30: credit reward, 0x37: stage, 0x7C: opponent count. */
    next.unlock_method=d[0x0c];
    next.prerequisite_key=le32(d+0x10);
    next.required_specific_url=d[0x10];
    next.sponsor_gate=d[0x11];
    next.required_races=d[0x12];
    next.required_urls=d[0x13];
    next.respect=(int32_t)le32(d+0x14);
    for (int i=0;i<4;i++) {
        next.track_ids[i]=le16(d+0x18+i*4);
        next.laps[i]=d[0x1b+i*4];
    }
    const int32_t reward=(int32_t)le32(d+0x30);
    if (reward<0) return 0;
    next.cash_value=(uint32_t)reward;
    next.icon_type=d[0x34];
    next.stage=d[0x37];
    next.num_stages=d[0x7e];
    next.opponents=d[0x7c];
    if (next.stage>5 || next.opponents>5 ||
        next.num_stages<1 || next.num_stages>4) return 0;
    *out=next;
    return 1;
}
int career_source_decode_stage(const uint8_t *d, size_t len,
                               CareerSourceStage *out) {
    if (!d || !out || len<0x50) return 0;
    CareerSourceStage next={0};
    next.id=d[0];
    next.sponsors_to_choose=d[1];
    next.outrun_cash_value=(int16_t)le16(d+2);
    for (int i=0;i<5;i++) {
        next.sponsor_keys[i]=le32(d+0x08+i*4);
        next.map_limits[i]=d[0x30+i];
    }
    next.last_stage_event_key=le32(d+0x28);
    next.max_outruns=d[0x40];
    if (next.id>5 || next.sponsors_to_choose>5) return 0;
    *out=next;
    return 1;
}
int career_source_decode_sponsor(const uint8_t *d, size_t len,
                                 const uint8_t *strings, size_t slen,
                                 CareerSourceSponsor *out) {
    if (!d || !out || len<0x10) return 0;
    CareerSourceSponsor next={0};
    if (!name_at(strings,slen,le16(d),next.id)) return 0;
    next.cash_per_win=(int16_t)le16(d+2);
    next.sign_bonus=(int16_t)le16(d+12);
    next.potential_bonus=(int16_t)le16(d+14);
    memcpy(next.required_race_types,d+4,sizeof next.required_race_types);
    /* No guessed enum assignments or overflow into a uint32 credit count. */
    if (next.cash_per_win<0 || next.sign_bonus<0 ||
        next.potential_bonus<0) return 0;
    *out=next;
    return 1;
}
