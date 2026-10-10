# Underground 2 / Xbox default.xbe — DDAY address map (10 October 2026)

Status: **first static reverse-engineering pass**. Xbox code has not been executed. No proprietary binary, generated lifted C, or commercial assets have been committed.

## User-supplied retail Xbox executable

- Signed `XBEH`, certificate title `NFS Underground 2`
- Xbox Title ID `0x4541005A`, region mask `0x00000001` (North America)
- Image timestamp **2004-10-17 09:49:07 UTC**
- Original size 4,038,656 bytes, 14 sections
- SHA-256 `c8cab1bfe7cf26553e84eb4ef2a26d16f6b06260f7494c583802bc00fd81dd31`
- Decoded retail entrypoint **`0x0021B1CE`**, matching `NFSU2_ENTRY_POINT` in [antoxa2584x/nfsu2-sw](https://github.com/antoxa2584x/nfsu2-sw/blob/main/src/main.c). A matching entry point **alone does not prove every byte is identical**.

## Xbox virtual addresses (not ARM64/H700 addresses!)

| Identifier | String VA | Direct immediate-pointer sites in .text |
| --- | --- | --- |
| `DDAY_EVENT_A` | `0x00354E40` | `0x0011FC8D`, `0x00140DB4`, `0x00140FB0`, `0x001597D2` |
| `DDAY_EVENT_B` | `0x00354E30` | `0x0011FCA3`, `0x0013F8FE`, `0x0013F952`, `0x0014D562`, `0x001597AA`, `0x001597E3` |
| `DDAY_PLAYER_CAR` | `0x00351580` | `0x000F2C83` |
| `FREE_ROAM_STAGE0A` | `0x0035A404` | `0x00182146` |
| `FREE_ROAM_STAGE0` | `0x0035A418` | `0x0018213F` |
| `CARLOT01` | `0x00346780` | `0x00094F7B` |
| `CRIB01` | `0x00346778` | `0x00094F85`, `0x000FEC9C` |
| `WMTriggerZoneElement` | `0x0034E168` | `0x002DA2F3` |
| `UICareerCarLot` | `0x00352FDC` | `0x002DD273` |

Strings `CAREER_STAGE0_INTRO`, `CAREER_SHOP_CAR_LOT`, `CAREER_SHOP_CRIB` and `SHOP_TRIGGER` are also present, but appear as data-table references. No direct inline pointer site was found, which **does not mean there is no handler**.

## Manually inspected candidate routines (x86 disassembly)

- **`0x0004CDC0`: verified 32-bit EA BinaryHash.** Starts at `0xFFFFFFFF` and repeats `hash = hash*33 + next_ascii_byte`. It produces `DDAY_EVENT_A=0xDD60E402`, `DDAY_EVENT_B=0xDD60E403`, `TRIGGER_CC_CAR_LOT_1=0xFFCC6A4B`. These match the retail PC `GlobalB.lzc` IDs, giving a **cross-version identity bridge**.
- **`0x0013F8C0`: candidate active-event transition handler.** It reads a pointer from object offset `+0x8738`, compares its `+0x08` event ID to the supplied ID, and conditionally invokes `0x136540` or `0x1365C0`. Precise effects are unverified.
- **`0x00140D50`: candidate prologue step transition.** Refers to `SMS_INSTRUCTION` and forwards the hash of `DDAY_EVENT_A` to `0x0013F8C0`. Not evidence that A was completed.
- **`0x00140DE0`: eight-state dispatcher**, jump table at `0x00141020`, with one branch forwarding A to `0x0013F8C0`. Interpret states only after caller analysis.
- **`0x001597A0–0x001597FC`: distinct handling for DDAY A and B.** Code clears the active-event pointer on B, whereas the A path computes the hash for B and calls `0x110880` then `0x14D320`. This is a promising candidate for the **A→B handoff**, not yet a confirmed save or completion.
- **`0x0018211E–0x00182150`: initial world selection** of `FREE_ROAM_STAGE0` or `FREE_ROAM_STAGE0A`, controlled in part by a byte at `0x40696D`.

These are **Xbox source addresses**, not byte offsets in the user's PC GlobalB, and not functions portable directly to OpenGL ES2.

## Run the analyzer locally

```sh
python3 tools/ug2_xbe_dday_probe.py /path/to/your/default.xbe
python3 tools/ug2_xbe_dday_probe.py /path/to/your/default.xbe --json
```

The utility validates XBE headers, maps sections, reads NUL-terminated identifiers, and lists candidate 32-bit pointer occurrences from .text. It does not disassemble control flow, execute code, modify files or copy proprietary bytes to the repository.

## What remains unknown

The real spatial location, radius, and callback of `TRIGGER_CC_CAR_LOT_1`, the exact semantics of the 8 prologue states, which event proves A/B completed, and when retail Xbox persists progress. Do not substitute route 4000, a hash collision, or the presence of a video file for an actual scripted completion. The verified-save-only prologue logic in our H700 port remains unchanged.

**Next short iteration:** find incoming calls and surrounding control flow for `0x001597A0`, `0x0013F8C0` and `0x00140D50`, and establish the concrete A→B condition before any gameplay integration.
