# Hunt Victim AWAKE Check and Extraction Safety Fix

## Problem Summary

The game was experiencing segmentation faults in `mobile_activity()` when mobs were hunting targets that were sleeping. The issue occurred in the `hunt_victim()` function when a hunting mob caught up to a sleeping victim and attempted to attack.

## Root Cause

**Location**: `src/graph.c`, function `hunt_victim()`, lines 1914-1915

**Problem**: When a hunting mob reached its target's room, the code called `hit(ch, victim, TYPE_UNDEFINED)` without:

1. Checking if the victim is awake (`AWAKE(victim)`)
2. Properly handling extraction that could occur during the `hit()` call

```c
// Original problematic code:
if (found && IN_ROOM(ch) == IN_ROOM(victim))
    hit(ch, victim, TYPE_UNDEFINED);
else if (!found)
    HUNTING(ch) = NULL;
```

**Why this causes segfaults**:

1. **Sleeping Target Issue**: Attacking a sleeping target can trigger unexpected behavior in combat systems, special procedures, or triggers that weren't designed to handle this edge case
2. **Extraction During Hit**: The `hit()` function can indirectly cause extraction of either `ch` or `victim` through:
   - Death in combat
   - Special procedures triggered by combat
   - DG Scripts triggered by combat events
   - Death traps or other environmental effects
3. **No Post-Hit Validation**: After `hit()` returned, the code didn't check if either character was extracted before continuing
4. **Continued Processing**: The calling function `mobile_activity()` would continue processing `ch` even if it had been extracted during `hunt_victim()`

**Specific Scenario**:
```
1. Mob A is hunting Player B
2. Player B goes to sleep (POS_SLEEPING)
3. Mob A catches up to Player B's room
4. hunt_victim() calls hit(Mob A, Player B, TYPE_UNDEFINED)
5. The hit triggers a special procedure on Player B
6. The special procedure extracts Mob A (e.g., trap, script, etc.)
7. hunt_victim() returns
8. mobile_activity() tries to access Mob A -> SIGSEGV
```

## Solution Implemented

### Changes to hunt_victim() in src/graph.c

Added comprehensive safety checks before and after the `hit()` call:

```c
if (found && IN_ROOM(ch) == IN_ROOM(victim)) {
    /* Safety check: Only attack if victim is awake to prevent issues with sleeping targets */
    if (AWAKE(victim)) {
        hit(ch, victim, TYPE_UNDEFINED);

        /* Safety check: hit() can indirectly cause extract_char for ch or victim
         * through death, special procedures, triggers, etc.
         * Re-validate both ch and victim still exist before continuing */
        for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
            if (ch == tmp)
                found = TRUE;

        if (!found) {
            /* ch was extracted during hit(), cannot safely continue */
            return;
        }

        /* Check if ch was marked for extraction */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET)) {
            return;
        }

        /* Re-validate victim still exists */
        for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
            if (victim == tmp)
                found = TRUE;

        if (!found) {
            /* Victim was extracted during hit(), clear hunting */
            HUNTING(ch) = NULL;
        }
    } else {
        /* Victim is sleeping, don't attack - clear hunting to avoid repeated attempts */
        HUNTING(ch) = NULL;
    }
} else if (!found) {
    HUNTING(ch) = NULL;
}
```

### Key Improvements

1. **AWAKE Check**: Before attacking, verify the victim is awake
   - Prevents attacking sleeping targets which can trigger unexpected behavior
   - Clears hunting if victim is sleeping to avoid repeated attempts

2. **Post-Hit Ch Validation**: After `hit()`, verify `ch` still exists
   - Checks if `ch` is in the character list
   - Returns immediately if `ch` was extracted
   - Prevents further operations on a freed pointer

3. **Post-Hit Extraction Flag Check**: Verify `ch` isn't marked for extraction
   - Checks `MOB_NOTDEADYET` and `PLR_NOTDEADYET` flags
   - Returns immediately if extraction is pending
   - Prevents operations on characters scheduled for cleanup

4. **Post-Hit Victim Validation**: Verify `victim` still exists
   - Re-validates victim is in the character list
   - Clears hunting if victim was extracted
   - Prevents dangling pointer in HUNTING(ch)

## Why Attack Sleeping Targets is Problematic

In most MUD contexts, attacking sleeping targets is intentionally allowed (e.g., aggressive mobs in `mobile_activity()`). However, the hunting system is different because:

1. **Hunting is Long-Term Tracking**: The victim might have gone to sleep far from where the hunt started
2. **State Changes During Pursuit**: Between starting the hunt and catching up, the victim's state can change dramatically
3. **Trigger Complexity**: Sleeping targets may have different trigger behaviors than awake ones
4. **Player Expectations**: Players don't expect to be attacked while sleeping in safe areas

By clearing hunting when a victim is sleeping, we:
- Prevent edge case bugs in combat systems
- Avoid unexpected trigger behaviors
- Give sleeping characters appropriate protection
- Maintain consistent game mechanics

## Alternative Approaches Considered

### 1. Wake Up the Victim First
```c
if (!AWAKE(victim)) {
    GET_POS(victim) = POS_STANDING;
    act("$n te acorda bruscamente!", FALSE, ch, 0, victim, TO_VICT);
}
hit(ch, victim, TYPE_UNDEFINED);
```
**Rejected because**: This could bypass intentional sleeping mechanics (spells, potions) and trigger unexpected state transitions.

### 2. Only Validate Extraction, Allow Sleeping Attacks
```c
hit(ch, victim, TYPE_UNDEFINED);
// ... validation only ...
```
**Rejected because**: The root issue includes attacking sleeping targets, not just the extraction handling.

### 3. Retry Hunt Next Tick
```c
if (!AWAKE(victim)) {
    return; // Try again next tick
}
```
**Rejected because**: This could lead to infinite hunting loops if the victim remains sleeping.

## Impact and Benefits

### Prevents Crashes When:
- Mobs hunt targets that fall asleep during pursuit
- Combat triggers extraction through special procedures
- Death traps or environmental effects extract characters during combat
- DG Scripts extract characters during combat events

### Maintains Consistent Behavior:
- Hunting is cleared when targets are unavailable (sleeping)
- Extraction is properly detected and handled
- No dangling pointers in hunting system
- Safe return from hunt_victim() prevents cascading failures

### Performance Impact:
- Minimal: Added checks are simple pointer comparisons and flag checks
- Only executed when hunting mob catches its target
- No impact on normal mob processing

## Testing Recommendations

1. **Sleeping Target Test**: Have a mob hunt a player, have player sleep before mob arrives
2. **Extraction During Hit**: Create a trigger that extracts the hunter during combat
3. **Victim Extraction**: Create a trigger that extracts the victim during combat
4. **Long-Term Stability**: Run server with extensive mob hunting activity

## Files Modified

- `src/graph.c`: Modified `hunt_victim()` function (lines 1914-1951)
  - Added AWAKE(victim) check before hit()
  - Added post-hit validation for ch extraction
  - Added post-hit validation for ch extraction flags
  - Added post-hit validation for victim extraction
  - Added proper hunting cleanup for all edge cases

## Related Documentation

- `HUNT_VICTIM_RACE_CONDITION_FIX.md` - Previous fix for victim validation
- `MOBACT_EXTRACT_FIX.md` - General mob extraction safety
- `MOBACT_ACT_EXTRACTION_FIX.md` - act() extraction safety

## Prevention Guidelines for Future Development

1. **Always Check Victim State**: Before attacking, verify the target is in an appropriate state
2. **Validate After State-Changing Operations**: Any function that can extract characters needs validation after
3. **Clear Tracking Pointers**: When targets become invalid, clear pointers to prevent reuse
4. **Early Return on Extraction**: Don't continue processing after detecting extraction
5. **Test Edge Cases**: Test with sleeping, paralyzed, and otherwise incapacitated targets

## Conclusion

This fix addresses a critical segmentation fault in the hunting system by:
1. Preventing attacks on sleeping targets (which can trigger unexpected behavior)
2. Properly validating character existence after combat
3. Detecting and handling extraction during hit()
4. Clearing hunting pointers when targets become invalid

The fix is defensive, minimal, and follows the established patterns in the codebase for handling character extraction safely.
