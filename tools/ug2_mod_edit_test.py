#!/usr/bin/env python3
"""Asset-free smoke tests for the standalone PC career mod editor."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
EDITOR = ROOT / "tools/ug2_mod_edit.py"


def main():
    with tempfile.TemporaryDirectory(prefix="ug2-mod-test-") as tmp:
        folder = Path(tmp)
        orig = folder / "GlobalB.lzc"
        orig.write_bytes(b"synthetic and NOT original EA game data")
        expected = orig.read_bytes()
        tool = folder / "fake_ug2_career_dump"
        tool.write_text(
            "#!/usr/bin/env python3\n"
            "print('career_sections: 1')\n"
            "print('0,S3_SPRINT_6,stage=3,prize=350,opponents=3')\n"
            "print('0,S4_SPON_DRAG_7,stage=4,prize=8000,opponents=3')\n"
        )
        tool.chmod(0o700)
        mod = folder / "mods" / "career_rewards.txt"
        def run(*args):
            return subprocess.run(
                [sys.executable, str(EDITOR), "--globalb", str(orig),
                 "--mod", str(mod), "--dumper", str(tool), *args],
                capture_output=True, text=True,
            )
        assert run("list").returncode == 0
        assert run("set", "S3_SPRINT_6", "1600").returncode == 0
        assert run("set-prototype", "875").returncode == 0
        output = mod.read_text()
        assert output.startswith("OPENUG2_CAREER_MOD_V1\n")
        assert "S3_SPRINT_6=1600\n" in output
        assert "@PROTOTYPE_CIRCUIT_CASH=875\n" in output
        assert run("show").returncode == 0
        before = mod.read_bytes()
        assert run("set", "MISSING_RACE", "1000").returncode != 0
        assert run("set", "S3_SPRINT_6", "-100").returncode != 0
        assert run("set", "S3_SPRINT_6", "10000001").returncode != 0
        assert mod.read_bytes() == before
        assert orig.read_bytes() == expected
        other = folder / "other.txt"
        other.write_text("leave intact")
        mod.unlink()
        mod.symlink_to(other)
        assert run("set", "S3_SPRINT_6", "1000").returncode != 0
        assert other.read_text() == "leave intact"
        mod.unlink()
        mod.write_text("OPENUG2_CAREER_MOD_V1\nNOT_ORIGINAL=123\n")
        assert run("set", "S3_SPRINT_6", "1000").returncode != 0
        assert orig.read_bytes() == expected
    print("ug2_mod_edit_test: PASS")


if __name__ == "__main__":
    main()
