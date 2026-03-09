# Testing Guide: `appear` Command — Room-Local Visibility

The `appear` command makes an invisible player visible **only within their current
room**. Unlike `visible`, it does **not** remove the global `AFF_INVISIBLE` flag.
Players in other rooms still cannot see the character in the `who` list.

When the player moves to a different room the room-local state (`AFF_APPEARED`) is
automatically cleared, turning them globally invisible again.

## Prerequisites

1. Build the MUD server:
   ```bash
   cmake -B build -S . && cmake --build build
   ```
2. Log in with **three or four** test accounts at immortal level (or use
   `wizat`/`trans` to position mortal test characters):
   - **Player A** — the invisible player who will use `appear`
   - **Player B** — in a **different** room (3001), **no** `detect invisibility`
   - **Player C** — in the **same** room (3008), **no** `detect invisibility`
   - **Player D** *(optional, needed for Scenario 3)* — in the **same** room (3008),
     **with** `detect invisibility`

## Behaviour Reference

| Recipient | `visible` (global) | `appear` (room-local) |
|---|---|---|
| Same room, any detect state | Sees `"$n aparece lentamente."` | Sees `"$n aparece lentamente."` ✅ |
| Different room | Never sees room message | Never sees room message ✅ |
| Same room — `who` after | Player A visible ✅ | Player A **visible** ✅ |
| **Different** room — `who` after | Player A visible ✅ | Player A **invisible** ✅ |
| After player moves to new room | stays visible | reverts to invisible ✅ |

---

## Test Scenario 1: Different-Room Player — `who` After `appear`

**Objective:** Verify that Player B (room 3001, no detect_invis) **cannot** see
Player A in the `who` list after `appear` (because Player A is only room-locally
visible in 3008).

### Setup

```
# Immortal console — position characters
trans PlayerA 3008       # Move Player A to room 3008
trans PlayerB 3001       # Move Player B to room 3001
# Cast invisible on Player A
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
   **Expected:** Player A is **NOT** listed (invisible).

3. As **Player A** (room 3008), run:
   ```
   appear
   ```
   **Expected feedback to Player A:**
   ```
   Você se torna visível nesta sala.
   ```

4. As **Player B** (room 3001), run:
   ```
   who
   ```

### Expected Results

- ✅ Player A still **does NOT appear** in Player B's `who` list
  (`AFF_INVISIBLE` was NOT removed; `CAN_SEE(B, A)` is FALSE because B is in a
  different room and does not have detect_invis)
- ✅ Player B receives **no room message** about Player A appearing
- ✅ `stat char PlayerA` shows **both** `AFF_INVISIBLE` and `AFF_APPEARED`

### Failure Indicators

- ❌ Player A appears in Player B's `who` list (invisibility was incorrectly removed)
- ❌ Player B receives `"$n aparece lentamente."` from a different room

---

## Test Scenario 2: Same-Room Player — Room Message + `who`

**Objective:** Verify that Player C (room 3008, no detect_invis) **does** see the
appearance message AND can see Player A in `who` after `appear`.

### Setup

```
trans PlayerA 3008
trans PlayerC 3008
at 3008 cast 'invisible' PlayerA
```

### Test Steps

1. Confirm Player A is invisible and Player C has no detect_invis:
   ```
   at 3008 stat char PlayerA     # must have AFF_INVISIBLE, no AFF_APPEARED
   at 3008 stat char PlayerC     # must NOT have AFF_DETECT_INVIS
   ```
2. As **Player C** (room 3008), keep the terminal open to see arriving messages.

3. As **Player A** (room 3008), run:
   ```
   appear
   ```

4. Check **Player C**'s terminal.

5. As **Player C**, run:
   ```
   who
   ```

### Expected Results

- ✅ Player C **receives** `"<PlayerA> aparece lentamente."` with the correct name
  (because `AFF_APPEARED` was set before `act()` was called, so `INVIS_OK` passes
  for same-room viewers and `PERS` resolves to the player's name)
- ✅ Player A **appears** in Player C's `who` list
  (`CAN_SEE(C, A)` is TRUE: `AFF_APPEARED` set AND `IN_ROOM(C) == IN_ROOM(A)`)

### Failure Indicators

- ❌ Player C sees `"alguem aparece lentamente."` instead of the player's name
- ❌ Player A does not appear in Player C's `who` list

---

## Test Scenario 3: Same-Room Player With detect_invis

**Objective:** Verify that Player D (room 3008, **has** detect_invis) also sees the
appearance message.

### Setup

```
trans PlayerA 3008
trans PlayerD 3008
at 3008 cast 'invisible' PlayerA
at 3008 cast 'detect invisibility' PlayerD
```

### Test Steps

1. As **Player A** (room 3008), run:
   ```
   appear
   ```

### Expected Results

- ✅ Player D receives `"<PlayerA> aparece lentamente."`
- ✅ Player A appears in Player D's `who` list

---

## Test Scenario 4: Room-Local State Cleared on Movement

**Objective:** Verify that `AFF_APPEARED` is removed when Player A moves to another
room, restoring full invisibility.

### Setup

```
# Continue from Scenario 2 — Player A has AFF_INVISIBLE + AFF_APPEARED in room 3008
# Ensure room 3008 has an exit (e.g. north to another room)
```

### Test Steps

1. As **Player A**, move to a different room:
   ```
   north      # or any direction with an exit
   ```
2. As **Player C** (still in 3008), run:
   ```
   who
   ```
3. Check `stat char PlayerA`:
   ```
   at <new_room> stat char PlayerA
   ```

### Expected Results

- ✅ `AFF_APPEARED` is **cleared** from Player A (only `AFF_INVISIBLE` remains)
- ✅ Player A **no longer appears** in Player C's `who` list
- ✅ Players in the new room see the normal departure/arrival messages but cannot
  see Player A in `who` (still invisible)

### Failure Indicators

- ❌ Player A still appears in Player C's `who` after moving to another room
- ❌ `AFF_APPEARED` still set after movement

---

## Test Scenario 5: Contrast With `visible` Command

**Objective:** Confirm that `visible` (unlike `appear`) removes `AFF_INVISIBLE`
globally and broadcasts to the whole room.

### Setup

```
trans PlayerA 3008
trans PlayerB 3001
at 3008 cast 'invisible' PlayerA
```

### Test Steps

1. As **Player A**, run:
   ```
   visible
   ```
2. Check **Player B**'s `who` output.

### Expected Results

- ✅ `AFF_INVISIBLE` and `AFF_APPEARED` are both cleared from Player A
- ✅ Player B **does** see Player A in `who` (global invisibility removed)

---

## Test Scenario 6: `appear` Already Called (Idempotent)

**Objective:** Verify that running `appear` twice gives a clear message.

### Setup

```
trans PlayerA 3008
at 3008 cast 'invisible' PlayerA
```

### Test Steps

1. As **Player A**, run `appear` twice:
   ```
   appear
   appear
   ```

### Expected Results

- ✅ First call: `"Você se torna visível nesta sala."`
- ✅ Second call: `"Você já está visível nesta sala."`

---

## Code Path Summary

```
do_appear (act.other.c)
  ├─ SET_BIT_AR(AFF_FLAGS(ch), AFF_APPEARED)  ← room-local flag set FIRST
  ├─ act("$n aparece lentamente.", FALSE, ..., TO_ROOM)
  │    └─ For each 'to' in same room:
  │         PERS(ch, to) → CAN_SEE(to, ch) → INVIS_OK(to, ch)
  │           → AFF_INVISIBLE set BUT AFF_APPEARED set AND IN_ROOM(to)==IN_ROOM(ch)
  │           → TRUE → shows player's real name ✅
  │         hide_invisible=FALSE → no CAN_SEE filter, all same-room players get msg ✅
  └─ AFF_INVISIBLE stays set → CAN_SEE from different rooms = FALSE → not in who ✅

char_from_room (handler.c)
  └─ REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_APPEARED)  ← cleared on room change ✅

do_visible (act.other.c)
  └─ appear(ch)  [fight.c:986]
       ├─ REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE)   ← global flag removed
       ├─ REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_APPEARED)    ← room-local flag also cleared
       └─ act("$n aparece lentamente.", FALSE, ..., TO_ROOM)  ← everyone sees it

INVIS_OK(sub, obj) [utils.h]
  Old: (!AFF_INVISIBLE(obj) || AFF_DETECT_INVIS(sub)) && ...
  New: (!AFF_INVISIBLE(obj) || AFF_DETECT_INVIS(sub)
        || (AFF_APPEARED(obj) && IN_ROOM(sub)==IN_ROOM(obj))) && ...
```

---

## Success Criteria

- ✅ Scenario 1: Player B (different room) does **not** see Player A in `who`
- ✅ Scenario 2: Player C (same room, no detect_invis) sees the appearance message
  and sees Player A in `who`
- ✅ Scenario 3: Player D (same room, detect_invis) sees the appearance message
- ✅ Scenario 4: Moving clears `AFF_APPEARED`, restoring global invisibility
- ✅ Scenario 5: `visible` still removes invisibility globally
- ✅ Scenario 6: Running `appear` twice is idempotent

---

**Testing Completed By:** _______________  
**Date:** _______________  
**Overall Result:** ⬜ Pass / ⬜ Fail  
**Notes:** _______________________________________________
