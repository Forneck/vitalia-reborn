# Danger Sense Feature

## Status

**Status**: ✅ Implemented (February 2026)  
**Skill vnum**: `SKILL_DANGER_SENSE` — `#260`  
**Source**: `src/act.movement.c` — `check_danger_sense()`, `check_danger_sense_prevents_flee()`  
**Integration**: `src/act.informative.c` (look command), `src/act.offensive.c` (flee command)

---

## Overview

**Danger Sense** is a Thief skill (skill number 260) that grants characters an instinctive awareness of death-trap rooms adjacent to their current location.  It provides two complementary protections:

1. **Passive warning** — When looking at a room (the `look` command), the character receives an in-game warning if any adjacent room is flagged `ROOM_DEATH`.
2. **Flee prevention** — When fleeing from combat, the skill blocks the character from accidentally escaping into a death-trap room.

---

## Skill Details

| Property | Value |
|----------|-------|
| Skill number | 260 (`SKILL_DANGER_SENSE`) |
| Class restriction | Thief (assigned in `src/spells_assign.c`) |
| Proficiency range | 1–100 (higher = better detection chance) |
| Detection check | `rand_number(1, 101) > skill_level` → miss |

---

## Passive Warning (look command)

`check_danger_sense(struct char_data *ch)` is called from `act.informative.c` during `look_at_room()`.

### Behaviour by skill level

| Skill level | Message shown |
|-------------|---------------|
| 1 – 74 | Vague warning: *"Você sente um arrepio na espinha... há PERIGO mortal por perto!"* |
| 75 – 100 | Directional warning: *"Você sente um arrepio na espinha... há PERIGO mortal vindo do norte!"* |

Multiple death-trap directions are listed together (e.g., *"norte, leste"*).

### Detection roll

The function performs a single probability check against the character's skill level before scanning exits.  A natural result above the skill value means the danger is not perceived that look cycle; the character will get another chance on the next look.

### Returns

- `1` — Danger detected; warning sent to character.
- `0` — Danger not detected or not applicable (NPC, skill absent, no valid room).

---

## Flee Prevention

`check_danger_sense_prevents_flee(struct char_data *ch, int dir)` is called from `act.offensive.c` inside the flee-direction selection loop.

### Behaviour

If the chosen flee direction leads to a `ROOM_DEATH` room **and** the skill check succeeds, the function:

1. Sends an in-game message: *"Seu senso de perigo te alerta! Você recusa-se a fugir para norte - há uma ARMADILHA MORTAL lá!"*
2. Returns `1`, causing the flee loop to skip that direction and try the next available exit.

If every available exit is a death trap and all are blocked by danger sense, the character will not flee (standard no-exit-found behaviour applies).

### Returns

- `1` — Danger detected; flee to this direction prevented.
- `0` — No danger, skill check failed, or direction/room invalid.

---

## NPC Behaviour

The two functions return early for NPCs (`IS_NPC(ch)`).  Mobs have a separate mechanism that prevents them from wandering into death-trap rooms via `act.offensive.c`:

```c
/* Prevent mobs from fleeing into death traps */
if (ROOM_FLAGGED(dest, ROOM_DEATH)) continue;
```

This check is unconditional for mobs and does not require any skill.

---

## Builder Notes: Configuring Death-Trap Rooms

A room becomes a death trap by setting the `ROOM_DEATH` (also written as `DEATH`) flag in the zone file or via OLC (`redit`).

### OLC (redit) procedure

1. `redit <vnum>` — open the room for editing.
2. Select **Room flags** (option `F` in standard OLC menus).
3. Toggle **DEATH** (flag number varies by build; search for "DEATH" in the flag list).
4. Save the room (`Q`, then `zedit save <zone>`).

### Zone file syntax (`.wld` files)

Room flags in `.wld` files are stored as **numeric bitvectors** (or ASCII-flag letters) in the second field of the room header line.  `ROOM_DEATH` is bit 1 (0-indexed), so its bitvector value is **`2`**.  A minimal death-trap room looks like:

```
#12345
Some Dangerous Corridor~
You feel uneasy here.~
0 2 0
S
```

> **Note:** Editing `.wld` files directly is error-prone.  Always prefer OLC (`redit`) for toggling flags — the in-game editor writes the correct bitvector for you.

### Warnings and conventions

- **Always** surround death-trap rooms with at least one non-death-trap buffer room so players can receive the Danger Sense warning before it is too late.
- Death-trap rooms should be intentional design choices (puzzle areas, forbidden zones) and **not** used to patch broken area geometry.
- Consider adding descriptive text to rooms leading into death traps (e.g., a foreboding warning in the room description) for players without the skill.
- Immortals are not killed by death traps (protected in `act.movement.c` by level check).
- The mud logs every death-trap kill: `mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", ...)`.

---

## Related Files

| File | Role |
|------|------|
| `src/act.movement.c` | `check_danger_sense()`, `check_danger_sense_prevents_flee()` implementation |
| `src/act.informative.c` | Calls `check_danger_sense()` during `look_at_room()` |
| `src/act.offensive.c` | Calls `check_danger_sense_prevents_flee()` in flee loop |
| `src/spells.h` | `#define SKILL_DANGER_SENSE 260` |
| `src/spells_assign.c` | Assigns skill to Thief class |
| `src/act.h` | Function prototypes for `check_danger_sense*` |
