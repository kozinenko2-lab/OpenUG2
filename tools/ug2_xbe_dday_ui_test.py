#!/usr/bin/env python3
"""Asset-free checks; real Xbox game data intentionally absent from CI."""
import tempfile
from pathlib import Path
from ug2_xbe_dday_ui_probe import (
    analyze, hash_id, confirmation_parameter, intro_state_active,
    CALLS, SIGNATURES,
)

def main():
    assert hash_id("ACCEPT_BUTTON") == 0xD72F002A
    assert hash_id("CANCEL_BUTTON") == 0x63857DE0
    assert hash_id("Accept_Button") != hash_id("ACCEPT_BUTTON")
    assert len(CALLS) == 10
    assert len(SIGNATURES) == 12
    expected = (0, 0, 1, 0)
    actual = tuple(confirmation_parameter(flag, done)
                   for flag in (False, True) for done in (False, True))
    assert actual == expected
    assert not intro_state_active(0)
    assert not intro_state_active(9)
    assert all(intro_state_active(i) for i in range(1, 9))
    with tempfile.TemporaryDirectory() as tmp:
        for contents in (b"", b"XBEH" + b"\0" * 200,
                         b"XBEH" + b"\0" * 4096):
            filename = Path(tmp) / "default.xbe"
            filename.write_bytes(contents)
            try:
                analyze(filename)
            except ValueError:
                pass
            else:
                raise AssertionError("invalid XBE was incorrectly accepted")
    print("ug2_xbe_dday_ui_test: PASS")

if __name__ == "__main__":
    main()
