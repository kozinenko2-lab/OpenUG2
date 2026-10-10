/* Portable, text-only Underground 2 career mod overlays.
 * Intentionally not an EA GlobalB editor: user's original file is read-only. */
#ifndef OPENUG2_UG2_MODS_H
#define OPENUG2_UG2_MODS_H
#include <stddef.h>
#include <stdint.h>
#include "ug2_career_file.h"
#define UG2_MOD_MAX_CASH 10000000u
#define UG2_MOD_FILE_MAX 65536u
#define UG2_MOD_BASE_PROTOTYPE_CASH 500u

typedef struct {
    uint32_t prototype_circuit_cash; /* current verified AI circuit, not retail */
    unsigned edited_races;           /* catalog only; keyed by retail event name */
} UG2ModInfo;

/* File format:
 * OPENUG2_CAREER_MOD_V1
 * # comment
 * S3_SPRINT_6=1000
 * @PROTOTYPE_CIRCUIT_CASH=750
 *
 * Race names must resolve unambiguously to an existing original-game event.
 * Changes are applied to a private copy, never to GlobalB.lzc.
 * 1=applied; 0=invalid/missing. On failure, out and info are unmodified.
 * The caller must allocate sizeof(UG2CareerIndex) memory for both pointers.
 */
int ug2_mod_apply_file(const UG2CareerIndex *original, const char *patch,
                       UG2CareerIndex *out, UG2ModInfo *info);
#endif
