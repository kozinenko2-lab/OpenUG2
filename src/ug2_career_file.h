/* Original PC Underground 2 GlobalB.lzc career reader (no EA data embedded).
 * Only recorded metadata; no monetary rewards without a verified race result. */
#ifndef OPENUG2_UG2_CAREER_FILE_H
#define OPENUG2_UG2_CAREER_FILE_H
#include <stddef.h>
#include <stdint.h>
#include "career_source_catalog.h"

#define UG2_CAREER_MAX_RACES 512u
#define UG2_CAREER_MAX_SECTIONS 16u

typedef struct {
    CareerSourceRace race[UG2_CAREER_MAX_RACES];
    uint8_t section[UG2_CAREER_MAX_RACES];
    uint32_t unique_races;
    uint32_t repeated_races;
    uint32_t total_race_records;
    uint32_t stage_records;
    uint32_t sponsor_records;
    uint32_t career_sections;
} UG2CareerIndex;

/* Reads original JDLZ or uncompressed GlobalB bytes; validates both section
 * headers and each supported career record before publishing anything in out.
 * Returns zero on absent/corrupt/malformed files; out is untouched on failure. */
int ug2_career_load_file(const char *path, UG2CareerIndex *out);
/* Scans already decompressed GlobalB bytes (does not take ownership). */
int ug2_career_scan(const uint8_t *bytes, size_t size, UG2CareerIndex *out);
#endif
