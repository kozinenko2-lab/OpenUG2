#!/usr/bin/env python3
"""Asset-free smoke test for the Xbox DDAY XBE static-audit guard rails."""
import tempfile
from pathlib import Path
import ug2_xbe_dday_flow_probe as probe

def must_reject(contents):
    with tempfile.TemporaryDirectory() as temp:
        bad = Path(temp) / "default.xbe"
        bad.write_bytes(contents)
        try:
            probe.audit(bad)
        except ValueError:
            return
        raise AssertionError("an unsupported XBE was incorrectly accepted")

def main():
    assert len(probe.CALLS) == 9
    assert len(probe.STATES) == 8 and len(set(probe.STATES)) == 8
    assert len(probe.SIGS) == 6
    assert 0x001597F8 in probe.CALLS
    assert probe.CALLS[0x001597F8] == 0x0014D320
    assert probe.CALLS[0x0015979B] == 0x0014D6C0
    must_reject(b"")
    must_reject(b"XBEH" + b"\0" * 0x100)
    must_reject(b"XBEH" + b"\0" * 0x200)
    print("ug2_xbe_dday_flow_test: PASS (asset-free invalid cases)")
if __name__ == "__main__":
    main()
