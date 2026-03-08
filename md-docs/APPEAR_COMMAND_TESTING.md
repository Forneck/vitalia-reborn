# Testing Guide: `appear` Command — Silent Invisibility Removal

This guide covers manual test scenarios for the `appear` command added to restore
the silent-visibility behaviour that was lost during the tbaMUD migration.

## Prerequisites

1. Build the MUD server:
   ```bash
   cmake -B build -S . && cmake --build build
   ```
2. Log in with **three or four** test characters at immortal level (or use
   `wizat`/`trans` to position mortal test characters):
   - **Player A** — the invisible player who will use `appear`
   - **Player B** — in a **different** room, **no** `detect invisibility`
   - **Player C** — in the **same** room, **no** `detect invisibility`
   - **Player D** *(optional, needed for Scenario 3)* — in the **same** room,
     **with** `detect invisibility`

## Behaviour Reference

| Recipient | `visible` | `appear` |
|---|---|---|
| Same room, has `detect_invis` | Sees `"$n aparece lentamente."` | Sees `"$n aparece lentamente."` |
| Same room, no `detect_invis`  | Sees `"$n aparece lentamente."` ❌ | Sees nothing ✅ |
| Different room, any           | Never sees room message (correct) | Never sees room message ✅ |
| **Everyone** — `who` command  | Sees Player A (now visible) ✅ | Sees Player A (now visible) ✅ |

---

## Test Scenario 1: Different-Room Player — `who` After `appear`

**Objective:** Verify that after `appear`, Player B (room 3001, no detect_invis)
can see Player A in the `who` list.

### Setup

```
# Immortal console — position characters
trans PlayerA 3008       # Move Player A to room 3008
trans PlayerB 3001       # Move Player B to room 3001 (different room)
# Cast invisible on Player A (as immortal or with wand)
at 3008 cast 'invisible' PlayerA
```

### Test Steps

1. Confirm Player A is invisible:
   ```
   at 3008 stat char PlayerA      # AFF_INVISIBLE flag must be present
   ```
2. As **Player B** (room 3001), run:
   ```
   who
   ```
   **Expected:** Player A is **NOT** listed (still invisible).

3. As **Player A** (room 3008), run:
   ```
   appear
   ```
   **Expected feedback to Player A:**
   ```
   Você encerra a magia da invisibilidade.
   ```

4. As **Player B** (room 3001), run:
   ```
   who
   ```

### Expected Results

- ✅ Player A **does appear** in Player B's `who` list after `appear`
  (invisibility was removed; `CAN_SEE` now returns TRUE regardless of room)
- ✅ Player B receives **no room message** about Player A appearing
  (`TO_ROOM` sends only to the same room)
- ✅ Player A's `AFF_INVISIBLE` flag is cleared:
  ```
  at 3008 stat char PlayerA      # AFF_INVISIBLE must be gone
  ```

### Failure Indicators

- ❌ Player A still not visible in `who` after `appear`
- ❌ Player B receives `"$n aparece lentamente."` (message leaked across rooms)

---

## Test Scenario 2: Same-Room Player Without detect_invis

**Objective:** Verify that Player C (room 3008, no detect_invis) does **not** see
the appearance message, but **does** see Player A in `who` after `appear`.

### Setup

```
trans PlayerA 3008
trans PlayerC 3008
at 3008 cast 'invisible' PlayerA
```

### Test Steps

1. Confirm Player A is invisible and Player C lacks detect_invis:
   ```
   at 3008 stat char PlayerA     # must have AFF_INVISIBLE
   at 3008 stat char PlayerC     # must NOT have AFF_DETECT_INVIS
   ```
2. As **Player C** (room 3008), observe the room (no special command needed —
   just keep the terminal open).

3. As **Player A** (room 3008), run:
   ```
   appear
   ```

4. Check **Player C**'s terminal output.

5. As **Player C**, run:
   ```
   who
   ```

### Expected Results

- ✅ Player C receives **no** `"$n aparece lentamente."` message
  (act() skips recipients who fail `CAN_SEE` while `AFF_INVISIBLE` is still set)
- ✅ Player A **does appear** in Player C's `who` list after `appear`
  (flag removed after the message was sent)

### Failure Indicators

- ❌ Player C sees `"$n aparece lentamente."` — same as `visible` leaking the
  message to non-detect-invis players
- ❌ Player A does not appear in `who` after `appear`

---

## Test Scenario 3: Same-Room Player With detect_invis

**Objective:** Verify that Player D (room 3008, **has** detect_invis) **does** see
the appearance message (they already knew the invisible player was there).

### Setup

```
trans PlayerA 3008
trans PlayerD 3008
at 3008 cast 'invisible' PlayerA
at 3008 cast 'detect invisibility' PlayerD
```

### Test Steps

1. Confirm Player D has detect_invis:
   ```
   at 3008 stat char PlayerD     # must have AFF_DETECT_INVIS
   ```
2. As **Player A** (room 3008), run:
   ```
   appear
   ```

### Expected Results

- ✅ Player D receives `"<PlayerA> aparece lentamente."`
  (they already had visibility of the invisible player via detect_invis)
- ✅ Player A appears in Player D's `who` list

### Failure Indicators

- ❌ Player D receives no message (the appear message is missing for detect_invis users)

---

## Test Scenario 4: Contrast With `visible` Command

**Objective:** Confirm that `visible` (unlike `appear`) broadcasts the message to
**everyone** in the room regardless of detect_invis.

### Setup

```
trans PlayerA 3008
trans PlayerC 3008      # no detect_invis
at 3008 cast 'invisible' PlayerA
```

### Test Steps

1. As **Player A**, run:
   ```
   visible
   ```
2. Check **Player C**'s terminal output.

### Expected Results

- ✅ Player C **does** receive `"<PlayerA> aparece lentamente."` (this is the
  intentional behaviour of `visible` — it announces to the whole room)

### Notes

`visible` calls the internal `appear(ch)` helper (in `fight.c`, line 986) which removes
flags *first* and then calls `act(..., FALSE, ...)` (`hide_invisible=FALSE`), so
`CAN_SEE` is not checked and everyone in the room receives the message.

`appear` (the new command) calls `act(..., TRUE, ...)` *before* removing the
flag, so only recipients who pass `CAN_SEE` (i.e. have detect_invis) receive it.

---

## Code Path Summary

```
do_appear (act.other.c)
  ├─ act("$n aparece lentamente.", hide_invisible=TRUE, ..., TO_ROOM)
  │    └─ comm.c: for each 'to' in same room:
  │         if (hide_invisible && !CAN_SEE(to, ch)) continue  ← skips no-detect_invis
  │         perform_act(...)
  ├─ affect_from_char(ch, SPELL_INVISIBLE)   ← removes spell
  └─ REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE)  ← flag gone → visible in who

do_visible (act.other.c)
  └─ appear(ch)  [fight.c:986]
       ├─ affect_from_char(ch, SPELL_INVISIBLE)   ← removes spell FIRST
       ├─ REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE)  ← flag gone FIRST
       └─ act("$n aparece lentamente.", hide_invisible=FALSE, ..., TO_ROOM)
            └─ hide_invisible=FALSE → CAN_SEE not checked → everyone in room sees it
```

---

## Success Criteria

- ✅ Scenario 1 passes: Player B (different room) can see Player A in `who`
- ✅ Scenario 2 passes: Player C (same room, no detect_invis) sees no message but
  can see Player A in `who`
- ✅ Scenario 3 passes: Player D (same room, detect_invis) sees the message
- ✅ Scenario 4 passes: `visible` still broadcasts to everyone in the room

---

**Testing Completed By:** _______________  
**Date:** _______________  
**Overall Result:** ⬜ Pass / ⬜ Fail  
**Notes:** _______________________________________________
