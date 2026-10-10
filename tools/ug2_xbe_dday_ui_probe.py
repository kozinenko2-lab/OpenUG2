#!/usr/bin/env python3
"""Xbox UG2 NTSC-U original UI confirmation and DDAY prologue guard audit.

Read-only fixed-revision opcode/call verification, no proprietary executable data
embedded and no assumption that a UI accept is an in-world event completion.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

EXPECTED_SHA256 = "c8cab1bfe7cf26553e84eb4ef2a26d16f6b06260f7494c583802bc00fd81dd31"

def xbe_load(path: Path):
    """Map raw XBE sections to game virtual addresses, read-only."""
    if not path.is_file() or path.stat().st_size > 64 * 1024 * 1024:
        raise ValueError("missing or oversized Xbox XBE")
    data = path.read_bytes()
    if len(data) < 0x180 or data[:4] != b"XBEH":
        raise ValueError("invalid XBEH header")
    def read32(off):
        return struct.unpack_from("<I", data, off)[0]
    base = read32(0x104)
    n = read32(0x11C)
    table = read32(0x120) - base
    if not 1 <= n <= 256 or table < 0 or table + 0x38*n > len(data):
        raise ValueError("invalid section directory")
    sections = []
    for i in range(n):
        va, _, raw, size, _ = struct.unpack_from("<5I", data, table + 0x38*i + 4)
        if raw > len(data) or size > len(data) - raw:
            raise ValueError("invalid section bounds")
        sections.append((va, raw, size))
    def at(va, length):
        for first, raw, size in sections:
            if first <= va and length <= size and va - first <= size - length:
                return data[raw + va - first:raw + va - first + length]
        raise ValueError("unmapped game address: 0x%08X" % va)
    return data, at

# Source machine code in this revision contains the mixed-case labels in .rdata,
# whereas the corresponding UI message IDs use their uppercase hashes.
UI_NAMES = {
    "Accept_Button": 0xD72F002A,
    "Cancel_Button": 0x63857DE0,
}
CALLS = {
    0x000B3753: 0x000B3650,  # UI accept-case -> callback
    0x000B366A: 0x00111080,  # 5-status lookup
    0x0011108A: 0x00111010,  # find exact prior-result ID
    0x001596FF: 0x00111010,  # result table write-or-lookup
    0x000B34B7: 0x001596F0,  # callback -> original event handling
    0x0015E670: 0x00113120,  # intro state != 0 and != 9
    0x0015E69F: 0x00140DE0,  # 8-state dispatch
    0x00132745: 0x0007A9C0,  # modal gate 0
    0x00132752: 0x0007A9C0,  # modal gate 1
    0x0016D335: 0x0015E5F0,  # periodic intro update
    0x00157B11: 0x00111010,  # DDAY A record status lookup
    0x00157B33: 0x00110880,  # locate selected A/B event
    0x00157B41: 0x0014D320,  # activate A or B
    0x000AFC4E: 0x001117D0,  # alternate status-1 lookup
    0x000AFC64: 0x00111060,  # alternate selected A/B lookup
    0x000AFC6F: 0x0014D320,  # alternate A/B activation
    0x001117D7: 0x00111010,  # status-1 predicate delegate
}
SIGNATURES = {
    "ui_command_type": (0x000B36C7, "3d1072400c"),
    "ui_event_subtype_field": (0x000B3700, "8b450c8b4010"),
    "ui_accept_hash": (0x000B370D, "3d2a002fd7"),
    "ui_cancel_hash": (0x000B3706, "3de07d8563"),
    "result_status5_lookup": (0x00111093, "80780605"),
    "result_status5_write": (0x00159723, "c6400605"),
    "confirm_arg_flag_guard": (0x000B3672, "84c97408"),
    "intro_mode6_guard": (0x0015E635, "833d348e420006"),
    "intro_world1_guard": (0x0015E63E, "833d38c3410001"),
    "intro_activity_mode4_guard": (0x0015E665, "833904"),
    "intro_active_if_1_to_8": (0x00113120, "8b0185c0740883f809"),
    "intro_modal_guard": (0x00132730, "a1b85b3e0085c056"),
    "DDAY_mode6_selector": (0x00157AF0, "a1a0bd3f00"),
    "DDAY_A_status1": (0x00157B1A, "80780601"),
    "DDAY_A_plus_one": (0x00157B2C, "81c102e460dd"),
    "DDAY_A_status1_alternate": (0x001117E5, "80fa01"),
    "DDAY_A_plus_one_alternate": (0x000AFC5E, "0502e460dd"),
}

def hash_id(identifier: str) -> int:
    h = 0xFFFFFFFF
    for b in identifier.encode("ascii"):
        h = (33 * h + b) & 0xFFFFFFFF
    return h

def confirmation_parameter(has_result_flag: bool, already_status5: bool) -> int:
    """Branch logic at Xbox 0xB3672..0xB3680, NOT a career win predicate."""
    return int(bool(has_result_flag) and not bool(already_status5))

def intro_state_active(state: int) -> bool:
    """Exact predicate used by 0x113120: active iff state != 0 and != 9."""
    return state not in (0, 9)

def resume_event_hash(dday_a_status1: bool) -> int:
    """Hash chosen by Xbox 0x157B25..0x157B32; NOT a win predicate."""
    return (0xDD60E402 + int(bool(dday_a_status1))) & 0xFFFFFFFF

def analyze(xbe: Path) -> dict:
    data, at = xbe_load(xbe)
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_SHA256:
        raise ValueError("XBE revision not verified: " + digest)
    # Validated switch table at Xbox 0x157B68 (mode 1..6).
    switch = struct.unpack("<6I", at(0x00157B68, 24))
    if switch != (0x157B02, 0x157B53, 0x157B67,
                  0x157B58, 0x157B67, 0x157B07):
        raise ValueError("DDAY resume selector switch table differs")
    names = []
    for label, expected in UI_NAMES.items():
        actual = hash_id(label.upper())
        if actual != expected:
            raise ValueError("unexpected UI hash: " + label)
        if (label.encode("ascii") + b"\0") not in data:
            raise ValueError("original UI string absent: " + label)
        names.append({"resource_label": label, "message_hash": "0x%08X" % actual})
    calls = []
    for site, expected in CALLS.items():
        b = at(site, 5)
        if b[0] != 0xE8:
            raise ValueError("not CALL rel32: 0x%08X" % site)
        actual = (site + 5 + struct.unpack("<i", b[1:])[0]) & 0xFFFFFFFF
        if actual != expected:
            raise ValueError("wrong CALL target at 0x%08X" % site)
        calls.append({"site": "0x%08X" % site, "target": "0x%08X" % actual})
    landmarks = []
    for name, (va, signature) in SIGNATURES.items():
        expected = bytes.fromhex(signature)
        if at(va, len(expected)) != expected:
            raise ValueError("machine-code signature mismatch for " + name)
        landmarks.append({"label": name, "virtual_address": "0x%08X" % va})
    return {"sha256": digest, "ui_messages": names, "verified_calls": calls,
            "DDAY_resume_selectors": [{"A_record_status1": int(status),
                                       "selected_event_hash": "0x%08X" % resume_event_hash(status)}
                                      for status in (False, True)],
            "verified_landmarks": landmarks,
            "confirmation_result_arg": [
                {"flag20": int(flag), "already_status5": int(done),
                 "arg": confirmation_parameter(flag, done)}
                for flag in (False, True) for done in (False, True)
            ],
            "analysis_limit": "UI confirmation callback is not a race finish or a saved DDAY win."
           }

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("xbe", type=Path)
    p.add_argument("--json", action="store_true")
    args = p.parse_args()
    try:
        record = analyze(args.xbe)
    except (OSError, ValueError) as error:
        p.error(str(error))
    if args.json:
        print(json.dumps(record, indent=2, ensure_ascii=False))
    else:
        print("Original Xbox XBE:", record["sha256"])
        for row in record["ui_messages"]:
            print("UI:", row["resource_label"], row["message_hash"])
        for row in record["verified_calls"]:
            print("CALL:", row["site"], "->", row["target"])
        print(record["analysis_limit"])

if __name__ == "__main__":
    main()
