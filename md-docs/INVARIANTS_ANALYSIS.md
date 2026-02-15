# Vitalia Reborn MUD Engine: Invariants Analysis

This document provides comprehensive answers to questions about local and temporal invariants in the Vitalia Reborn MUD engine codebase, based on code inspection of the CircleMUD/tbaMUD-derived architecture.

## Table of Contents

1. [Local Invariants](#local-invariants)
   - [Spatial / Room-Level Invariants](#spatial--room-level-invariants)
   - [Zone-Level Invariants](#zone-level-invariants)
   - [Container / Inventory Invariants](#container--inventory-invariants)
   - [Interaction-Scoped Invariants](#interaction-scoped-invariants)

2. [Temporal Invariants](#temporal-invariants)
   - [Time Representation](#time-representation)
   - [Persistence Across Time](#persistence-across-time)
   - [Temporal Identity and Causality](#temporal-identity-and-causality)
   - [Reset vs Time Invariants](#reset-vs-time-invariants)
   - [Authority Over Time](#authority-over-time)
   - [Absolute Temporal Limits](#absolute-temporal-limits)

---

## Local Invariants

### Spatial / Room-Level Invariants

#### 1. Can an entity exist in a room that violates the room flags (e.g., NO_MOB, PRIVATE, PEACEFUL, NO_MAGIC), or are these flags only enforced at entry time?

**Answer:** Room flags are **primarily enforced at entry time**, not as post-entry constraints. Entities can exist in rooms that violate flags if they were placed there through other means.

**Evidence:**
- `act.movement.c:306`: ROOM_PRIVATE is checked during movement
- `act.offensive.c:603`: ROOM_PEACEFUL is checked when initiating combat
- NO_MOB, NO_MAGIC, and PEACEFUL flags prevent **actions** (entering, attacking, casting) but don't prevent entities from existing in the room if placed by zone resets, admin commands, or scripts
- Zone resets (`db.c:reset_zone()`) do not validate room flags when placing mobs

**Conclusion:** Flags are **action-preventive**, not **state-preventive**. Mobs loaded during zone reset can exist in NO_MOB rooms.

---

#### 2. When a room reset occurs, are entities currently present in the room evaluated against reset rules, or are resets only additive (load-if-missing)?

**Answer:** Resets are **conditionally additive based on global prototype counts**, not based on room occupancy evaluation.

**Evidence:**
- `db.c:3075-3120`: The 'M' (mob) command checks `mob_index[ZCMD.arg1].number < ZCMD.arg2`
- This evaluates the **global count** across the entire world, not the count in the specific room
- If the global count is below max, the mob is loaded regardless of what's already there
- Existing entities in the room are **not removed or reconciled**

**Conclusion:** Room resets are additive load operations. Existing mobs in the room are not destroyed or moved.

---

#### 3. Can a room temporarily contain more instances of an entity than its local Max due to external loads or scripts?

**Answer:** **Yes, absolutely.** The Max value is a **global prototype limit**, not a per-room limit.

**Evidence:**
- `db.c:3085`: `mob_index[ZCMD.arg1].number < ZCMD.arg2` checks global count
- Scripts, admin commands, and other zone resets can all create entities independently
- There is no per-room enforcement of maximum entity counts

**Conclusion:** Rooms can exceed any conceptual "local max" through concurrent operations, script actions, or admin intervention.

---

#### 4. Is there any automatic reconciliation after a room reset that removes excess entities, or is this only checked at load time?

**Answer:** **No automatic reconciliation exists.** Excess entities persist until naturally removed.

**Evidence:**
- `db.c:reset_zone()` has no cleanup phase that evaluates and removes excess entities
- The only constraint is the load-time check
- Once loaded, entities remain until explicitly removed via extraction

**Conclusion:** The system is **load-conservative** but **never corrective**.

---

#### 5. Does the room ever "own" an entity, or is ownership always attributed to zone / world-level bookkeeping?

**Answer:** **Rooms contain entities but don't own them.** Ownership is tracked at the **world level** through global linked lists.

**Evidence:**
- `db.c:59`: `struct char_data *character_list = NULL;` - global list
- `db.c:64`: `struct obj_data *object_list = NULL;` - global list
- Rooms have `people` and `contents` lists, but these are **membership lists**, not ownership
- Zone bookkeeping tracks reset intentions, not runtime ownership

**Conclusion:** Rooms are **containers** in the linked list sense. True ownership is global.

---

#### 6. Can an entity be present in a room without being registered in the room's entity list (desync cases)?

**Answer:** **Theoretically possible through bugs**, but the codebase has safeguards to prevent this.

**Evidence:**
- `handler.c:char_to_room()` and `obj_to_room()` are the canonical functions
- Direct manipulation of `IN_ROOM()` without calling these functions could cause desync
- Error logging exists: `handler.c:1081` checks for NOWHERE extraction
- Most operations use atomic functions that maintain consistency

**Conclusion:** The architecture **intends** strict list integrity. Desyncs are bugs, not design features.

---

#### 7. Are visibility rules (blindness, darkness, invisibility) capable of changing perceived cardinality without changing real cardinality?

**Answer:** **Yes, visibility rules affect perception only, not reality.**

**Evidence:**
- Visibility is checked during action resolution, not during entity enumeration
- The actual room lists are unchanged by visibility
- Functions like `CAN_SEE(ch, vict)` gate perception, not existence

**Conclusion:** Perceived cardinality ≠ real cardinality. Visibility is a **perceptual filter**.

---

### Zone-Level Invariants

#### 8. Is Max enforced per zone, per room, or globally for a given prototype?

**Answer:** **Max is enforced globally per prototype across the entire world**.

**Evidence:**
- `db.c:3085`: `mob_index[ZCMD.arg1].number < ZCMD.arg2`
- `mob_index` and `obj_index` are global arrays tracking **total instances**
- Multiple zones compete for the same pool

**Conclusion:** Max is a **global budget** for each prototype vnum.

---

#### 9. During a zone reset, is the current live count recalculated dynamically or cached?

**Answer:** **The live count is dynamically read from the global index** at the moment of the reset check.

**Evidence:**
- `db.c:3085`: Direct read of `mob_index[ZCMD.arg1].number` during reset
- The `.number` field is incremented in `read_mobile()`, decremented in `extract_char_final()`

**Conclusion:** There is no caching. The count is **always current** at the moment of evaluation.

---

#### 10. If a zone reset is skipped (due to players present, delay, or flags), does the zone drift from its intended invariant state?

**Answer:** **Yes, zones drift from their intended state when resets are skipped.**

**Evidence:**
- `db.c:3009`: Reset only occurs if `reset_mode == 2 || is_empty(zone)`
- If players camp in a zone with `reset_mode == 1`, the zone **never resets**
- Mobs die, objects decay, and the zone becomes progressively depleted

**Conclusion:** Zones **actively drift** under reset suppression. This is intentional design.

---

#### 11. Can a zone reset destroy entities that were not created by that zone's reset table?

**Answer:** **No. Zone resets only create entities, never destroy them.**

**Evidence:**
- `db.c:3051-3370`: `reset_zone()` only has creation commands
- There is no command to destroy existing entities
- Entities persist across resets unless they die/decay naturally

**Conclusion:** Zone resets are **additive operations only**.

---

#### 12. Are objects given to mobs during a zone reset considered zone-owned or mob-owned?

**Answer:** **Objects are mob-owned once given.**

**Evidence:**
- `db.c:3185-3220`: 'G' command uses `obj_to_char()` which sets `object->carried_by = mob`
- Once in inventory, the object is subject to mob behavior
- The zone reset table has no ongoing reference to the object after creation

**Conclusion:** Post-creation, objects follow standard ownership rules. Zone reset is a **one-time initialization**.

---

#### 13. Can a zone reset indirectly violate a global invariant (e.g., by duplicating a unique item)?

**Answer:** **Yes, if the zone reset logic is misconfigured.**

**Evidence:**
- The codebase has no `ITEM_UNIQUE` flag or unique item tracking
- Multiple zones can have reset commands for the same object vnum
- If multiple zones set high max values, duplicates will be created

**Conclusion:** Unique item semantics are **convention-based**, not engine-enforced.

---

### Container / Inventory Invariants

#### 14. Can an object exist without a container or room reference (i.e., "nowhere") outside of transitional code paths?

**Answer:** **Transitional "nowhere" states are intentional** for brief moments, but sustained nowhere existence indicates a bug.

**Evidence:**
- `handler.c:544`: `IN_ROOM(object) = NOWHERE` when moving to character
- Objects in character inventory have `IN_ROOM(obj) == NOWHERE`
- Objects in containers have `IN_ROOM(obj) == NOWHERE`, `obj->in_obj != NULL`

**Conclusion:** NOWHERE is a **valid transitional state** for inventories/containers.

---

#### 15. Is dropping an object a single atomic operation or a remove-then-add sequence?

**Answer:** **Dropping is a non-atomic two-step sequence**.

**Evidence:**
- Dropping involves: 1) `obj_from_char(obj)`, 2) `obj_to_room(obj, room)`
- These are separate function calls
- No transactional wrapper ensures atomicity

**Conclusion:** The operation is **logically atomic** but **mechanically sequential**.

---

#### 16. If an operation fails midway (e.g., drop fails after removal), can the object be lost or duplicated?

**Answer:** **Objects can be lost if an operation fails mid-sequence**, but duplication is unlikely.

**Evidence:**
- `handler.c:558-576`: `obj_from_char()` uses `REMOVE_FROM_LIST`
- If subsequent `obj_to_room()` is never called, the object is lost
- Duplication would require adding to two lists, which the design prevents

**Conclusion:** **Loss is possible**, duplication is not (under normal operation).

---

#### 17. Can an object be simultaneously referenced by multiple containers due to pointer aliasing?

**Answer:** **No, not under normal operation.**

**Evidence:**
- `handler.c:888-891`: Each object has single `in_obj`, `carried_by`, `IN_ROOM()` pointers
- The architecture **prohibits** multi-container membership by design

**Conclusion:** Multi-container membership would be a critical bug.

---

#### 18. Are inventory limits (weight, slots) enforced strictly or only checked at entry time?

**Answer:** **Limits are checked at entry time**, but can be exceeded through indirect means.

**Evidence:**
- Weight and item count checks occur in commands like `do_get()`
- Effects that increase object weight after carrying can cause violations
- No automatic dropping occurs

**Conclusion:** Limits are **preventative** but not **corrective**.

---

#### 19. Can resets or scripts insert items into inventories ignoring normal constraints?

**Answer:** **Yes. Zone resets and scripts can bypass weight/slot limits.**

**Evidence:**
- `db.c:3185-3220`: 'G' command directly calls `obj_to_char()` without capacity checks
- Scripts using `%load%` can force objects into inventories

**Conclusion:** Normal commands respect limits, but **privileged operations bypass them**.

---

### Interaction-Scoped Invariants

#### 20. Does an interaction (combat, social action, spell) lock the state of involved entities for its duration?

**Answer:** **No explicit locking exists.** Interactions assume stable state but don't enforce it.

**Evidence:**
- Combat is managed through `FIGHTING(ch)` pointers
- No mutex or lock mechanism prevents state changes
- State changes can occur mid-interaction if triggered by other events

**Conclusion:** No locking. State changes during interactions are possible.

---

#### 21. Can an entity be destroyed or moved while it is the target of an ongoing interaction?

**Answer:** **Yes, absolutely.**

**Evidence:**
- `handler.c:extract_char()` can be called at any time
- Extraction nulls references like `FIGHTING(ch)`, but this is cleanup after extraction
- Movement functions don't check for ongoing interactions

**Conclusion:** Entities can be removed during interactions. Code must validate pointers.

---

#### 22. Are emotional or memory updates transactional with the interaction, or can they partially apply?

**Answer:** **Updates are non-transactional and can partially apply.**

**Evidence:**
- Emotional/memory systems update state via direct field modifications
- No rollback mechanism exists

**Conclusion:** State updates are **immediate and irreversible**.

---

#### 23. If an interaction references an entity by pointer, can that pointer become invalid before completion?

**Answer:** **Yes. Pointers can become invalid if the entity is extracted.**

**Evidence:**
- `extract_char()` calls `free(ch)`, invalidating all pointers
- No reference counting or pointer invalidation notification

**Conclusion:** Pointer invalidation is a **real risk**. Defensive programming is essential.

---

#### 24. Are triggers allowed to spawn or destroy entities during the same interaction frame?

**Answer:** **Yes. Triggers can spawn or destroy entities at any time.**

**Evidence:**
- Trigger system allows execution of arbitrary commands
- Triggers can call `%load%`, `%purge%`, `%teleport%`, etc.

**Conclusion:** Triggers have **full entity manipulation power** during interactions.

---

## Temporal Invariants

### Time Representation

#### 25. Is time represented as a single global monotonic counter (T), or are there multiple clocks?

**Answer:** **Multiple time systems coexist**.

**Evidence:**
- **Real Time**: `time_t` values from system call
- **Game Time**: Pulse counter system (`PULSE_ZONE`, `PULSE_VIOLENCE`, etc.)
- **MUD Time**: Fictional calendar (`hours`, `day`, `month`, `year`)

**Conclusion:** Three distinct time systems: real time, pulse time, and MUD time.

---

#### 26. Can different subsystems observe different "current times"?

**Answer:** **No. All subsystems observe the same time values**.

**Evidence:**
- Time variables are global
- All subsystems read from the same variables
- Pulse counters are maintained centrally

**Conclusion:** Time is **globally consistent** at any given instant.

---

#### 27. Is time ever rolled back, or is time travel implemented?

**Answer:** **Time is never rolled back.** No time travel mechanics exist.

**Evidence:**
- Time variables are monotonically increasing
- No functions to decrease time or rewind counters

**Conclusion:** Time is **strictly forward-moving**.

---

#### 28. Are resets keyed to absolute time, relative time, or event queues?

**Answer:** **Resets are keyed to relative time managed through event queues.**

**Evidence:**
- `db.h`: `struct zone_data { int lifespan; int age; }`
- Zone `age` increments every real-time minute
- When `age >= lifespan`, zone is added to reset queue

**Conclusion:** Resets use **relative time** and are dispatched via an **event queue**.

---

### Persistence Across Time

#### 29. Which properties of an entity are persisted across ticks without recomputation?

**Answer:** **Most properties are persisted; only derived/temporary values are recomputed.**

**Evidence:**
- Stats, inventory, equipment, position, affects are persisted
- Derived values (AC, hit/dam modifiers) are recomputed
- `players.c:save_char()` writes all persistent properties

**Conclusion:** Core entity state is **persistent**. Only **derived properties** are recomputed.

---

#### 30. Are emotional states recomputed, decayed, or strictly accumulated over time?

**Answer:** **Emotional states decay over time.**

**Evidence:**
- Emotion system has decay mechanics
- `PULSE_MOB_EMOTION (4 RL_SEC)` triggers periodic updates
- Emotions drift toward neutral over time

**Conclusion:** Emotions are **dynamic** with both accumulation and decay.

---

#### 31. Does an entity remember interactions that occurred in a timeline that is no longer current?

**Answer:** **Not applicable** - no branching timelines exist.

**Evidence:**
- Mob memory system tracks past interactions
- Memories persist until zone reset

**Conclusion:** Entities remember **all interactions in the single linear timeline**.

---

#### 32. Can a destroyed entity leave behind temporal effects?

**Answer:** **Generally no**, but some edge cases exist.

**Evidence:**
- `handler.c:extract_char()` removes all affects from the character
- Affects applied to other entities persist

**Conclusion:** Direct effects are removed. Effects on **other entities** persist.

---

#### 33. Are object timers evaluated against absolute time or local elapsed time?

**Answer:** **Object timers use local elapsed time** (tick count).

**Evidence:**
- `handler.c:1024`: `GET_OBJ_TIMER(obj) -= use`
- Timers are relative: "ticks remaining"

**Conclusion:** Object timers are **tick-based**, not timestamp-based.

---

### Temporal Identity and Causality

#### 34-38. Time Travel and Causality Questions

**Answer:** **Not applicable.** No time travel mechanics exist in the codebase.

**Evidence:**
- No time shift capabilities
- No retroactive state changes
- Causality is strictly linear and forward

**Conclusion:** Causality is **strictly linear**. Past cannot be altered.

---

### Reset vs Time Invariants

#### 39. Is a reset considered a temporal boundary?

**Answer:** **A reset is just another event**, not a temporal boundary.

**Evidence:**
- Resets are triggered by standard event queue
- Players continue existing through resets
- Global time continues forward

**Conclusion:** Resets are **world state updates**, not timeline resets.

---

#### 40-41. Reset and Time Travel

**Answer:** **Not applicable** - no time travel exists.

**Conclusion:** Resets are **replayable, non-destructive operations**.

---

#### 42. Does a reset reassert invariants?

**Answer:** **Resets attempt to approximate invariants**, not strictly reassert them.

**Evidence:**
- Resets are additive (load if under max) not corrective
- Existing entities are not removed

**Conclusion:** Resets **restore missing elements** but don't enforce strict state.

---

#### 43. Can an entity persist across resets?

**Answer:** **Yes, absolutely.**

**Evidence:**
- Zone resets don't destroy existing entities
- Player characters always persist

**Conclusion:** Entities persist **by default**.

---

### Authority Over Time

#### 44. Can admins violate temporal invariants?

**Answer:** **Yes, admins have unrestricted power.**

**Evidence:**
- Admin commands bypass all constraints
- Can load duplicates, set arbitrary values, etc.

**Conclusion:** Admins are **temporally and spatially omnipotent**.

---

#### 45. Can scripts alter historical state?

**Answer:** **Scripts can only alter current state.**

**Evidence:**
- DG Scripts operate on current world state
- No historical record that scripts can modify

**Conclusion:** Scripts are **present-focused**.

---

#### 46-47. Automatic Reconciliation

**Answer:** **No automatic reconciliation exists.**

**Evidence:**
- No contradiction detection
- Current state is always authoritative

**Conclusion:** Contradictions are **tolerated**. Present state is truth.

---

#### 48. Is player observation authoritative?

**Answer:** **No. Server state is authoritative.**

**Evidence:**
- No observation logging
- Server state is truth, not player perception

**Conclusion:** **Server state is authoritative**.

---

### Absolute Temporal Limits

#### 49. Minimum reachable time?

**Answer:** **Yes, the last boot.**

**Evidence:**
- `boot_time` is set at startup
- Pulse counters reset at boot

**Conclusion:** Real and pulse time have minimum at boot.

---

#### 50. Maximum future time?

**Answer:** **Practical limits exist due to integer overflow**, but no explicit maximum.

**Conclusion:** Practical limits are **extremely distant** (centuries).

---

#### 51-52. Temporal Isolation

**Answer:** **No explicit isolation mechanism exists.**

**Conclusion:** Isolation (if it occurs) is a **bug**.

---

#### 53. What events are immutable?

**Answer:** **All events are immutable once processed.**

**Evidence:**
- No rollback or undo mechanism
- State changes can be reverted by admin, but events remain in history

**Conclusion:** **All events are immutable**.

---

## Summary

The Vitalia Reborn MUD engine follows a **pragmatic, non-transactional architecture**:

- **Invariants are weakly enforced**: Limits are preventative but not corrective
- **Time is linear and forward-only**: No time travel or alternate timelines
- **State changes are immediate**: No transactions, no rollback
- **Resets are additive, not corrective**: Restore missing elements, don't purge excess
- **Admin and scripts have unrestricted power**: Can violate any invariant
- **Memory management is manual**: Desyncs are possible bugs

This design prioritizes **flexibility and performance** over strict consistency, appropriate for a real-time multiplayer game.
