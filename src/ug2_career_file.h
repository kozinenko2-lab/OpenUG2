/* Original PC Underground 2 GlobalB.lzc career reader (no EA data embedded).
 * Only recorded metadata; no monetary rewards without a verified race result. */
#ifndef OPENUG2_UG2_CAREER_FILE_H
#define OPENUG2_UG2_CAREER_FILE_H
#include <stddef.h>
#include <stdint.h>
#include "career_source_catalog.h"

#define UG2_CAREER_MAX_RACES 512u
#define UG2_CAREER_MAX_SECTIONS 16u
#define UG2_CAREER_MAX_SHOPS 128u

/* Original CareerManager SHOP_BLOCK 0x00034A12 record (0xA0 bytes).
 * This is metadata only, NOT a coordinate/finished-trigger detector.
 * Keep both retail CareerManager sections: the same shop can differ in
 * stage/hidden status across sections and cannot be silently merged. */
typedef struct {
    char name[32];            /* collection name at +0x00 */
    char intro_movie[24];     /* optional cutscene at +0x20 */
    char filename[16];        /* filename at +0x40 */
    uint32_t trigger_key;     /* raw hashed trigger at +0x3C */
    uint32_t required_event;  /* raw hashed event at +0x74 */
    uint8_t shop_type;       /* GlobalLib eWorldShopType; 4 = CARLOT */
    uint8_t initially_hidden;
    uint8_t unlocked_by_event;
    uint8_t stage;            /* raw stage; values >5 exist in retail */
    uint8_t section;          /* original CareerManager section index */
} UG2CareerShop;

typedef struct {
    CareerSourceRace race[UG2_CAREER_MAX_RACES];
    uint8_t section[UG2_CAREER_MAX_RACES];
    uint32_t unique_races;
    uint32_t repeated_races;
    uint32_t total_race_records;
    uint32_t stage_records;
    uint32_t sponsor_records;
    uint32_t career_sections;
    UG2CareerShop shops[UG2_CAREER_MAX_SHOPS];
    uint32_t shop_records;
} UG2CareerIndex;

/* Reads original JDLZ or uncompressed GlobalB bytes; validates both section
 * headers and each supported career record before publishing anything in out.
 * Returns zero on absent/corrupt/malformed files; out is untouched on failure. */
int ug2_career_load_file(const char *path, UG2CareerIndex *out);
/* Scans already decompressed GlobalB bytes (does not take ownership). */
int ug2_career_scan(const uint8_t *bytes, size_t size, UG2CareerIndex *out);
#endif
