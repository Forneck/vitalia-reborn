# Hunt Victim Extraction Safety Fix

## Problem Summary

The game was experiencing segmentation faults in `mobile_activity()` when mobs were hunting targets, particularly when the target was sleeping (!AWAKE). The issue occurred in the `hunt_victim()` function when action stacking occurred during combat with a sleeping target, and the hunting mob was extracted.

## Root Cause

**Location**: `src/graph.c`, function `hunt_victim()`, lines 1914-1915

**Problem**: When a hunting mob reached its target's room and called `hit(ch, victim, TYPE_UNDEFINED)`, there was no handling for extraction that could occur during the `hit()` call, especially when the victim was not awake.

```c
// Original problematic code:
if (found && IN_ROOM(ch) == IN_ROOM(victim))
    hit(ch, victim, TYPE_UNDEFINED);
else if (!found)
    HUNTING(ch) = NULL;
```

**Why this causes segfaults**:

1. **Action Stacking with Sleeping Targets**: When a mob attacks a sleeping target (!AWAKE), action stacking can occur in the combat system
2. **Extraction During Hit**: The `hit()` function can indirectly cause extraction of `ch` (the hunter) through:
   - Death in combat
   - Special procedures triggered by combat
   - DG Scripts triggered by combat events
   - Retaliatory effects or counter-attacks
3. **No Post-Hit Validation**: After `hit()` returned, the code didn't check if `ch` was extracted
4. **Continued Processing**: The calling function `mobile_activity()` would continue processing `ch` even if it had been extracted during `hunt_victim()`, leading to accessing freed memory

**Specific Scenario**:
```
1. Mob A is hunting Player B
2. Player B is sleeping (POS_SLEEPING)
3. Mob A catches up to Player B's room
4. hunt_victim() calls hit(Mob A, Player B, TYPE_UNDEFINED)
5. Action stacking occurs due to sleeping target state
6. The hit triggers a special procedure/trigger that extracts Mob A
7. hunt_victim() returns without detecting extraction
8. mobile_activity() tries to access Mob A -> SIGSEGV
```

## Solution Implemented

### Changes to hunt_victim() in src/graph.c

Added comprehensive safety checks after the `hit()` call to detect and handle extraction:

```c
if (found && IN_ROOM(ch) == IN_ROOM(victim)) {
    hit(ch, victim, TYPE_UNDEFINED);

    /* Safety check: hit() can indirectly cause extract_char for ch or victim
     * through death, special procedures, triggers, etc.
     * This is critical when victim is !AWAKE since action stacking can occur.
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
} else if (!found) {
    HUNTING(ch) = NULL;
}
```

### Key Improvements

1. **Post-Hit Ch Validation**: After `hit()`, verify `ch` still exists
   - Checks if `ch` is in the character list
   - Returns immediately if `ch` was extracted
   - Prevents further operations on a freed pointer

2. **Post-Hit Extraction Flag Check**: Verify `ch` isn't marked for extraction
   - Checks `MOB_NOTDEADYET` and `PLR_NOTDEADYET` flags
   - Returns immediately if extraction is pending
   - Prevents operations on characters scheduled for cleanup

3. **Post-Hit Victim Validation**: Verify `victim` still exists
   - Re-validates victim is in the character list
   - Clears hunting if victim was extracted
   - Prevents dangling pointer in HUNTING(ch)

4. **Allows Attacking Sleeping Targets**: The fix does NOT prevent attacking sleeping targets
   - This is intentional game behavior for hunting mobs
   - The issue is not about attacking sleeping targets, but about handling extraction during such attacks
   - The safety checks specifically handle the action stacking scenario that can occur with sleeping targets

## Why Sleeping Targets Are Allowed

Unlike the initial implementation, this fix correctly allows hunters to attack sleeping targets because:

1. **Intended Game Behavior**: Hunting mobs should be able to attack any target they catch up to, regardless of the target's state
2. **Consistent with Aggressive Mobs**: Aggressive mobs can attack sleeping targets (see MOB_WIMPY check in mobact.c)
3. **Real Issue is Extraction**: The problem is not attacking sleeping targets, but the action stacking and extraction that can occur
4. **Proper Solution**: Handle extraction safely rather than preventing the attack

The comment in the code explicitly notes: "This is critical when victim is !AWAKE since action stacking can occur."

## Impact and Benefits

### Prevents Crashes When:
- Mobs hunt targets that are sleeping
- Action stacking occurs during combat with sleeping targets
- Combat triggers extraction through special procedures
- Death traps or environmental effects extract characters during combat
- DG Scripts extract characters during combat events
- Any combat event causes character extraction

### Maintains Consistent Behavior:
- Hunting mobs can attack sleeping targets (intended behavior)
- Extraction is properly detected and handled
- No dangling pointers in hunting system
- Safe return from hunt_victim() prevents cascading failures in mobile_activity()

### Performance Impact:
- Minimal: Added checks are simple pointer comparisons and flag checks
- Only executed when hunting mob catches its target
- No impact on normal mob processing

## Testing Recommendations

1. **Sleeping Target Test**: Have a mob hunt a player, have player sleep before mob arrives, verify attack occurs
2. **Extraction During Hit**: Create a trigger that extracts the hunter during combat with sleeping target
3. **Victim Extraction**: Create a trigger that extracts the victim during combat
4. **Long-Term Stability**: Run server with extensive mob hunting activity, including sleeping targets

## Files Modified

- `src/graph.c`: Modified `hunt_victim()` function (lines 1914-1944)
  - Added post-hit validation for ch extraction
  - Added post-hit validation for ch extraction flags
  - Added post-hit validation for victim extraction
  - Added proper hunting cleanup for all edge cases
  - Updated comments to clarify the action stacking issue with sleeping targets

## Related Documentation

- `HUNT_VICTIM_RACE_CONDITION_FIX.md` - Previous fix for victim validation
- `MOBACT_EXTRACT_FIX.md` - General mob extraction safety
- `MOBACT_ACT_EXTRACTION_FIX.md` - act() extraction safety

## Prevention Guidelines for Future Development

1. **Always Validate After State-Changing Operations**: Any function that can extract characters needs validation after
2. **Don't Over-Restrict Behavior**: Focus on handling edge cases rather than preventing intended behavior
3. **Clear Tracking Pointers**: When targets become invalid, clear pointers to prevent reuse
4. **Early Return on Extraction**: Don't continue processing after detecting extraction
5. **Document Action Stacking Risks**: Note when sleeping or incapacitated targets might cause action stacking

## Conclusion

This fix addresses the critical segmentation fault in the hunting system by:
1. Properly validating character existence after combat
2. Detecting and handling extraction during hit()
3. Clearing hunting pointers when targets become invalid
4. **Allowing hunters to attack sleeping targets** (intended game behavior)
5. Specifically handling the action stacking scenario that occurs with sleeping targets

The fix is defensive, minimal, and follows the established patterns in the codebase for handling character extraction safely. Most importantly, it correctly addresses the real issue (extraction during action stacking) rather than preventing intended game behavior (attacking sleeping targets).

