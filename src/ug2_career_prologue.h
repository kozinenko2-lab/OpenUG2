/* The original Underground 2 DDAY prologue has no competitive lap/AI finish.
 * This module recognizes original catalog identities and persists externally
 * VERIFIED scripted completions; it does not invent a finish detector. */
#ifndef OPENUG2_UG2_CAREER_PROLOGUE_H
#define OPENUG2_UG2_CAREER_PROLOGUE_H
#include "ug2_career_binding.h"

typedef enum {
    UG2_PROLOGUE_INVALID=0,
    UG2_PROLOGUE_LOCKED=1,
    UG2_PROLOGUE_READY=2,
    UG2_PROLOGUE_DONE=3
} UG2PrologueState;

typedef enum {
    UG2_PROLOGUE_REJECTED=0,
    UG2_PROLOGUE_RECORDED=1,
    UG2_PROLOGUE_ALREADY_RECORDED=2,
    UG2_PROLOGUE_SAVE_FAILED=3
} UG2PrologueResult;

/* DDAY_EVENT_A has no predecessor; DDAY_EVENT_B requires A. Both have
 * stage=0, route=4000, zero laps/opponents/cash in original PC GlobalB.
 * Current profile stage is 1 even while completing retail stage-0 prologue.
 * Does not mutate either object. */
UG2PrologueState ug2_prologue_state(const UG2CareerIndex *catalog,
                                   const Career *profile,
                                   const char *original_id);

/* Only a future, authoritative *scripted-event finish* handler may call this,
 * not generic route completion, --event/--career-race, collision, checkpoints,
 * or free roam. 'script_finished' must come from verified original scenario
 * logic; the current engine deliberately does NOT invoke this function.
 * Original route 4000 alone cannot distinguish A from B.
 * On success: save first, publish RAM second. Never adds money or wins needed
 * for stage progression, only the original ID completion marker. */
UG2PrologueResult ug2_prologue_record_verified_finish(
    Career *profile, const UG2CareerIndex *catalog,
    const char *original_id, unsigned actual_route,
    int script_finished, const char *save_path);
#endif
