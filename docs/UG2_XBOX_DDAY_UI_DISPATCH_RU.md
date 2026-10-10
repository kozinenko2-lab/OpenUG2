# Xbox Underground 2: DDAY UI confirmation and intro guards (iteration 21)

Source: user-owned original Xbox NTSC-U `default.xbe`, SHA-256
`c8cab1bfe7cf26553e84eb4ef2a26d16f6b06260f7494c583802bc00fd81dd31`.
**Static code analysis**, not original game runtime traces. Addresses are Xbox x86 virtual
addresses, NOT filesystem offsets, not PortMaster ARM64 addresses, and not map coordinates.
No Xbox executable, lifted commercial game code or gameplay assets are committed.

## Critical refinement: the upstream callback is reached from UI

The branch in `0x000B36A0` first compares a message type with the unresolved hash
`0x0C407210`, reads the message sub-ID at `message+0x10` and dispatches:

| Compared 32-bit value | Xbox resource string | Uppercase BinaryHash |
| --- | --- | --- |
| `0xD72F002A` | `Accept_Button` | `ACCEPT_BUTTON` |
| `0x63857DE0` | `Cancel_Button` | `CANCEL_BUTTON` |

Both resource strings exist in this XBE and compute to those exact hashes using
the confirmed Xbox routine at `0x0004CDC0` on **uppercase** labels (start
`0xFFFFFFFF`, repeatedly multiply 33 and add ASCII byte). The original
mixed-case resource names hash differently, so case matters.

The accept branch at `0x000B370D` invokes `0x000B3753 -> 0x000B3650`
only when its additional `0x00112320` / `0x003FEE30` checks select
that path. `0x000B3650` calls `0x00111080`, then
`0x000B3490 -> 0x001596F0`.

**Correction to interpretation:** This is a **UI accept-message path into
event processing**, NOT evidence that `0x001596F0` is universally called
when a street race finishes. Earlier event-result terminology was too broad.

## The status-5 record and confirmation argument

- `0x00111010`: scans a table starting at `this+0x7B00`, with
  `this+0x7AFC` entries, 8 bytes per entry, comparing the first u32
  with the input ID. Returns the matching record or null.
- `0x00111080`: gets `result+0x38`, calls `0x00111010`,
  and returns true only if a record exists and `record+0x06 == 5`.
- `0x001596F0`: looks up or inserts an 8-byte record and writes its
  `+0x06` byte to **5** (at `0x00159723`).
- `0x000B3650`: supplies argument **1** to `0x000B3490` only if
  `result+0x20 != 0` AND the status-5 lookup returns **false**.
  All other combinations supply 0.

| result+0x20 | Previously status 5 | Argument |
| ---: | ---: | ---: |
| 0 | 0 | 0 |
| 0 | 1 | 0 |
| 1 | 0 | 1 |
| 1 | 1 | 0 |

The argument is stored into a separate byte by `0x000B3490`, but
`0x001596F0` is called either way when the original callback is reached.
**Neither status 5 nor this argument is proven to mean race victory or
persistent career completion.** Further semantic and caller analysis needed.

## Entry gates for the 8-state intro dispatcher

Confirmed periodic upstream direct call:
`0x0016D335 -> 0x0015E5F0`.

In `0x0015E5F0` the path to `0x0015E69F -> 0x00140DE0`
is constrained by all of these observed tests:

- `*(uint32*)0x428E34 == 6`
- `*(uint32*)0x41C338 == 1`
- `(*(ptr*)0x44940C)->field_0x234 == 0`
- `*(ptr*)0x3CC470 == NULL`
- first field of object referenced at `0x449438` equals **4**
- `0x00113120(this+0x713C)` returns true: the state is neither **0**
  nor **9**. State **1..8** selects the original dispatcher;
  state 0 or 9 instead enters an alternate timer/update branch.

A prerequisite inside the 8-state machine itself is
`0x00132730`, whose checks use two direct calls to
`0x0007A9C0` (arguments **0** and **1**), plus state of external
objects. The exact gameplay meaning of this subsystem remains unverified.
None of these conditions supplies a valid car-lot X/Y/Z position.

## Reproduce locally

```sh
python3 tools/ug2_xbe_dday_ui_test.py
python3 tools/ug2_xbe_dday_ui_probe.py /path/to/your/default.xbe --json
```

Probe validates exact SHA-256, both original resource labels and the uppercase
message hashes, **17 direct CALLs**, **17 machine code signatures**, and
the four outcomes of the 0/1 status/flag argument. Unknown revisions are
rejected, no user files modified. CI regression runs without game assets.


## Independent original A/B resume selector (additional confirmation)

Two more **direct** Xbox paths choose which DDAY event becomes active.
These are independent from the UI Accept_Button callback:

1. At `0x00157AF0`, a 6-entry switch dispatches on global
   `0x3FBDA0`. Its **sixth** case enters `0x00157B07`. That branch
   looks up `DDAY_EVENT_A = 0xDD60E402` with `0x00111010`.
   If the found 8-byte result record has `record+0x06 == 1`, the
   code adds **1** to the hash (thus selecting `DDAY_EVENT_B =
   0xDD60E403`); otherwise it selects A. It then calls
   `0x00110880` and `0x0014D320` to activate the chosen event.
   The verified six jump-table targets at `0x00157B68` are:
   `157B02, 157B53, 157B67, 157B58, 157B67, 157B07`.
2. At `0x000AFC33`, a different code path calls
   `0x001117D0(DDAY_EVENT_A)`. That helper itself calls
   `0x00111010` and returns whether `record+0x06 == 1`.
   The code again selects `0xDD60E402 + result`, looks up the
   corresponding race with `0x00111060`, then activates it
   through `0x0014D320`.

This gives a concrete Xbox **resumption/selection rule**:

| Observed status of A record | Event selected for activation |
| --- | --- |
| absent or `!= 1` | `DDAY_EVENT_A` |
| `== 1` | `DDAY_EVENT_B` |

**Important:** here the selector checks *status 1*, whereas the
UI confirmation deduplication logic earlier checks *status 5*.
Their exact semantics and persistence are not established;
neither may be mapped to our H700 career save flags until verified.

The added validator now checks **17 direct CALLs**, **17 opcode
landmarks**, and the six-entry selector jump-table. It still does not
prove an original shop trigger location, a career win, or save write.

## Port decision

No auto-completion of DDAY A/B on `ACCEPT_BUTTON`, stage-0 route 4000,
the 8-state timer or playing SCENE05. Keep
`ug2_prologue_record_verified_finish` gated on an independently
verified script completion. Next: locate source/allocator of the message
result object, identify UI acceptance vs real world trigger callbacks,
and find the actual career persistence path.
