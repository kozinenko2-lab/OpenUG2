#!/usr/bin/env python3
"""Read-only first-pass DDAY symbol-reference census on the user's own Xbox XBE.

This emits *candidate pointer sites*, not control-flow or completed-event
proof. No game binary is bundled, modified, or uploaded by this script.
"""
import argparse
import datetime
import hashlib
import json
import struct
from pathlib import Path

LABELS = (
    "DDAY_EVENT_A", "DDAY_EVENT_B", "DDAY_PLAYER_CAR",
    "CAREER_STAGE0_INTRO", "FREE_ROAM_STAGE0A", "FREE_ROAM_STAGE0",
    "CAREER_SHOP_CAR_LOT", "CAREER_SHOP_CRIB",
    "CARLOT01", "CRIB01", "SHOP_TRIGGER", "WMTriggerZoneElement",
    "UICareerCarLot", "CAREER_SMS_MESSAGE", "SMS_INSTRUCTION",
)
RETAIL_ENTRY_XOR = 0xA8FC57AB
MAX_XBE_BYTES = 64 * 1024 * 1024

def read_xbe(path):
    if not path.is_file() or path.stat().st_size > MAX_XBE_BYTES:
        raise ValueError("XBE missing or larger than 64 MiB")
    data = path.read_bytes()
    if len(data) < 0x180 or data[:4] != b"XBEH":
        raise ValueError("not an Xbox XBEH executable")
    def u32(off):
        if off < 0 or off + 4 > len(data):
            raise ValueError("XBE header pointer outside the file")
        return struct.unpack_from("<I", data, off)[0]
    base = u32(0x104)
    cert = u32(0x118) - base
    if cert < 0 or cert + 0xAC > len(data):
        raise ValueError("invalid certificate")
    size = u32(cert)
    if size < 0xAC or cert + size > len(data):
        raise ValueError("invalid certificate size")
    title = data[cert + 12: cert + 92].decode("utf-16le", "replace").split("\0", 1)[0]
    title_id = u32(cert + 8)
    region_mask = u32(cert + 0xA0)
    count = u32(0x11C)
    table = u32(0x120) - base
    if not 1 <= count <= 256 or table < 0 or table + 0x38 * count > len(data):
        raise ValueError("invalid section directory")
    sections = []
    for i in range(count):
        pos = table + 0x38 * i
        va, virtual_size, raw, raw_size, name_va = struct.unpack_from("<5I", data, pos + 4)
        name_pos = name_va - base
        if not 0 <= name_pos < len(data) or raw + raw_size > len(data):
            raise ValueError("invalid section")
        end = data.find(b"\0", name_pos, min(len(data), name_pos + 32))
        if end < 0:
            raise ValueError("unterminated section name")
        sections.append((data[name_pos:end].decode("ascii", "replace"), va, raw, raw_size))
    code_sections = [(n, va, raw, length) for n, va, raw, length in sections if n == ".text"]
    if len(code_sections) != 1:
        raise ValueError("missing unique .text section")
    _, text_va, text_raw, text_len = code_sections[0]
    text = data[text_raw:text_raw + text_len]

    def raw_to_va(raw_pos):
        for _, va, raw, length in sections:
            if raw <= raw_pos < raw + length:
                return va + raw_pos - raw
        return None

    refs = {}
    for label in LABELS:
        records = []
        start = 0
        needle = label.encode("ascii") + b"\0"
        while True:
            found = data.find(needle, start)
            if found < 0:
                break
            start = found + 1
            va = raw_to_va(found)
            if va is None:
                continue
            pointer = struct.pack("<I", va)
            sites = []
            j = 0
            while True:
                k = text.find(pointer, j)
                if k < 0:
                    break
                sites.append("0x%08X" % (text_va + k))
                j = k + 1
            records.append(dict(string_va="0x%08X" % va, code_candidate_sites=sites))
        if records:
            refs[label] = records
    return dict(
        file_bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
        title=title, title_id="0x%08X" % title_id,
        region_mask="0x%08X" % region_mask,
        timestamp_utc=datetime.datetime.fromtimestamp(u32(0x114),
                       datetime.timezone.utc).isoformat(),
        decoded_retail_entry="0x%08X" % (u32(0x128) ^ RETAIL_ENTRY_XOR),
        entry_matches_nfsu2_sw=(u32(0x128) ^ RETAIL_ENTRY_XOR) == 0x0021B1CE,
        references=refs,
        note="Candidate pointer sites only: no script-finish or shop-geometry proof."
    )

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("xbe", type=Path)
    p.add_argument("--json", action="store_true")
    args = p.parse_args()
    try:
        data = read_xbe(args.xbe)
    except (OSError, ValueError) as e:
        p.error(str(e))
    if args.json:
        print(json.dumps(data, indent=2))
    else:
        for key in ("title", "title_id", "region_mask", "decoded_retail_entry",
                    "entry_matches_nfsu2_sw", "timestamp_utc", "sha256"):
            print("%s: %s" % (key, data[key]))
        for label, records in data["references"].items():
            for rec in records:
                print("%s @%s: %s" % (label, rec["string_va"],
                      " ".join(rec["code_candidate_sites"]) or "no direct .text site"))
        print(data["note"])

if __name__ == "__main__":
    main()
