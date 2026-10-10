#!/usr/bin/env python3
"""Small desktop editor for OpenUG2 career overlays, not for EA's GlobalB.
Uses the independently built local UG2 career dumper to validate every race ID.
No .NET runtime, external Python packages, or proprietary example records.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HEADER = "OPENUG2_CAREER_MOD_V1"
IDENT = re.compile(r"[A-Z0-9_]{1,63}\Z")
CASH = re.compile(r"[0-9]{1,8}\Z")
DATA = re.compile(r"^[0-9]+,([A-Z0-9_]+),stage=([0-9]+),prize=([0-9]+),opponents=([0-9]+)$")
MAX_CASH = 10_000_000
PROTOTYPE = "@PROTOTYPE_CIRCUIT_CASH"


def read_catalog(globalb: Path, dumper: Path):
    if not globalb.is_file():
        raise ValueError(f"original GlobalB file not found: {globalb}")
    if not dumper.is_file():
        raise ValueError("build the catalog reader first: make ug2-career-dump")
    p = subprocess.run([str(dumper.resolve()), str(globalb.resolve())],
                       capture_output=True, text=True, encoding="utf-8",
                       errors="replace", timeout=30, check=False)
    if p.returncode:
        raise ValueError("GlobalB career could not be verified (unsupported file)")
    catalog = {}
    for line in p.stdout.splitlines():
        match = DATA.fullmatch(line.strip())
        if match:
            event, stage, cash, opponents = match.groups()
            if event in catalog:
                raise ValueError(f"duplicate retail event in catalog: {event}")
            catalog[event] = (int(stage), int(cash), int(opponents))
    if not catalog or len(catalog) > 512:
        raise ValueError("missing/invalid original NFSU2 career catalog")
    return catalog


def read_mod(path: Path, catalog: dict):
    values = {}
    if not path.exists():
        return values
    if path.is_symlink():
        raise ValueError("refusing symlink overlay")
    if path.stat().st_size > 65536:
        raise ValueError("overlay exceeds 64 KiB")
    rows = path.read_text(encoding="ascii").splitlines()
    seen_header = False
    for raw in rows:
        if not raw or raw.startswith("#"):
            continue
        if not seen_header:
            if raw != HEADER:
                raise ValueError("incorrect mod header")
            seen_header = True
            continue
        if "=" not in raw:
            raise ValueError(f"malformed setting: {raw!r}")
        key, value = raw.split("=", 1)
        if key != PROTOTYPE and (not IDENT.fullmatch(key) or key not in catalog):
            raise ValueError(f"unknown race ID: {key}")
        if key in values:
            raise ValueError(f"duplicate setting: {key}")
        if not CASH.fullmatch(value) or int(value) > MAX_CASH:
            raise ValueError(f"invalid payout: {value!r}")
        values[key] = int(value)
    if not seen_header:
        raise ValueError("missing mod header")
    return values


def safe_save(dest: Path, original: Path, values: dict):
    # Do not replace user-owned game archives or follow symlink targets.
    if dest.suffix.lower() != ".txt" or dest.resolve() == original.resolve():
        raise ValueError("mod must be a separate .txt file, not a game archive")
    if dest.exists() and dest.is_symlink():
        raise ValueError("refusing symlink overlay")
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp_name = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="ascii", newline="\n", dir=str(dest.parent),
            prefix=".ug2mods-", delete=False,
        ) as tmp:
            tmp_name = tmp.name
            tmp.write(HEADER + "\n")
            tmp.write("# OpenUG2 read-only GlobalB overlay; generated locally\n")
            for key in sorted(values):
                tmp.write(f"{key}={values[key]}\n")
            tmp.flush()
            os.fsync(tmp.fileno())
        os.replace(tmp_name, dest)
    finally:
        if tmp_name and os.path.exists(tmp_name):
            os.unlink(tmp_name)


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--globalb", required=True, type=Path,
                   help="legally owned PC NFSU2/GLOBAL/GlobalB.lzc")
    p.add_argument("--mod", type=Path,
                   default=Path("portmaster/openug2/mods/career_rewards.txt"))
    p.add_argument("--dumper", type=Path, default=Path("build/ug2_career_dump"))
    sub = p.add_subparsers(dest="action", required=True)
    ls = sub.add_parser("list", help="show original race IDs, stages and payouts")
    ls.add_argument("--filter", default="")
    upd = sub.add_parser("set", help="set a verified retail race's modded reward")
    upd.add_argument("race")
    upd.add_argument("cash")
    prot = sub.add_parser("set-prototype", help="change the playable AI-circuit payout")
    prot.add_argument("cash")
    sub.add_parser("show", help="show current overrides")
    args = p.parse_args(argv)
    try:
        catalog = read_catalog(args.globalb, args.dumper)
        if args.action == "list":
            for race, (stage, cash, opponents) in catalog.items():
                if args.filter.upper() in race:
                    print(f"{race:32s} stage={stage} cash={cash} opponents={opponents}")
            print(f"Verified original race IDs: {len(catalog)}")
            return 0
        values = read_mod(args.mod, catalog)
        if args.action == "show":
            for key, amount in sorted(values.items()):
                print(f"{key}={amount}")
            return 0
        key = args.race if args.action == "set" else PROTOTYPE
        amount = args.cash
        if key != PROTOTYPE and key not in catalog:
            raise ValueError(f"unknown event in original UG2: {key}")
        if not CASH.fullmatch(amount) or int(amount) > MAX_CASH:
            raise ValueError("cash must be 0..10000000")
        values[key] = int(amount)
        safe_save(args.mod, args.globalb, values)
        print(f"Overlay updated: {args.mod} | {key}={amount}")
        if key != PROTOTYPE:
            print("Note: retail event rewards take effect only when that "
                  "event ID is connected to a verified race finish.")
        return 0
    except (ValueError, OSError, UnicodeError, subprocess.TimeoutExpired) as e:
        print(f"mod editor: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
