#!/usr/bin/env python3
"""Read-only Xbox XBE DDAY control-flow anchor validator.

Verifies a known NTSC-U revision and its existing CALL rel32 sites,
jump-table, and short machine-code landmarks. Does not execute the XBE.
The report does NOT establish shop positions, a DDAY completion or saves.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

KNOWN_SHA256 = "c8cab1bfe7cf26553e84eb4ef2a26d16f6b06260f7494c583802bc00fd81dd31"
CALLS = {
    0x000B3680: 0x000B3490,  # event-result dispatcher
    0x000B34B7: 0x001596F0,  # processing event result
    0x0015976D: 0x00110A00,  # optional match of subentry type 4
    0x0015979B: 0x0014D6C0,  # result accounting (gated)
    0x001597ED: 0x00110880,  # original 0x88-byte record lookup
    0x001597F8: 0x0014D320,  # activates DDAY B
    0x00140DC6: 0x0013F8C0,
    0x00140FC2: 0x0013F8C0,
    0x0015E69F: 0x00140DE0,  # timed intro state machine
}
STATES = (
    0x00140DFA, 0x00140E20, 0x00140E53, 0x00140EB9,
    0x00140EF9, 0x00140F61, 0x00140FD6, 0x00141012,
)
SIGS = {
    "event_id_load": (0x1597A0, "8b86388700008b7808"),
    "clear_B_pointer": (0x1597BA, "c7863887000000000000"),
    "match_subentry_type4": (0x110A15, "8078fc04"),
    "match_payload_hash": (0x110A1B, "8b79383b38"),
    "activate_new_event": (0x14D328, "89b138870000"),
    "state_machine_finishes_at_9": (0x140D52, "c70109000000"),
}

def audit(xbe: Path):
    if not xbe.is_file() or xbe.stat().st_size > 64 * 1024 * 1024:
        raise ValueError("missing or oversized retail XBE")
    data = xbe.read_bytes()
    if len(data) < 0x180 or data[:4] != b"XBEH":
        raise ValueError("not a valid XBEH header")
    digest = hashlib.sha256(data).hexdigest()
    if digest != KNOWN_SHA256:
        raise ValueError("XBE revision not matched; no hardcoded-address claims: " + digest)
    def rd32(off):
        return struct.unpack_from("<I", data, off)[0]
    base = rd32(0x104)
    table = rd32(0x120) - base
    n = rd32(0x11C)
    if not 1 <= n <= 256 or table < 0 or table + n * 0x38 > len(data):
        raise ValueError("corrupted section directory")
    sections = []
    for i in range(n):
        p = table + i * 0x38
        va, _, raw, size, _ = struct.unpack_from("<5I", data, p + 4)
        if raw > len(data) or size > len(data) - raw:
            raise ValueError("corrupted section bounds")
        sections.append((va, raw, size))
    def at(va, length):
        for addr, raw, size in sections:
            if addr <= va and length <= size and va - addr <= size - length:
                off = raw + va - addr
                return data[off:off + length]
        raise ValueError("unmapped address: 0x%08X" % va)
    refs = []
    for addr, expected in CALLS.items():
        data5 = at(addr, 5)
        if data5[0] != 0xE8:
            raise ValueError("not a direct CALL: 0x%08X" % addr)
        relative = struct.unpack_from("<i", data5, 1)[0]
        target = (addr + 5 + relative) & 0xFFFFFFFF
        if target != expected:
            raise ValueError("unexpected CALL target at 0x%08X" % addr)
        refs.append({"site": "0x%08X" % addr, "target": "0x%08X" % target})
    state_targets = struct.unpack("<8I", at(0x141020, 32))
    if state_targets != STATES:
        raise ValueError("different 8-state jump-table")
    landmarks = []
    for name, (addr, signature) in SIGS.items():
        expected = bytes.fromhex(signature)
        if at(addr, len(expected)) != expected:
            raise ValueError("different machine-code signature: " + name)
        landmarks.append({"name": name, "address": "0x%08X" % addr})
    return {
        "sha256": digest, "bytes": len(data), "calls": refs,
        "jump_table": [{"state": i+1, "target": "0x%08X" % x}
                       for i, x in enumerate(state_targets)],
        "landmarks": landmarks,
        "note": "Static anchors only; script-finish and shop geometry remain unproven.",
    }

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("default_xbe", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    try:
        report = audit(args.default_xbe)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print("Retail Xbox XBE:", report["sha256"])
        for row in report["calls"]:
            print("CALL", row["site"], "->", row["target"])
        for row in report["jump_table"]:
            print("STATE", row["state"], "->", row["target"])
        print(report["note"])

if __name__ == "__main__":
    main()
