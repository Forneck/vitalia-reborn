# Shoot Command Safety Fix - Extraction Prevention

## Issue Addressed
**GitHub Issue**: Bows and shoot - Review bow skill/shoot command for segmentation faults

**Problem**: The `do_shoot` command could cause segmentation faults when interacting with mobile activity due to unsafe character extraction handling. The command temporarily moves the shooter to another room to check visibility, calls `damage()` which can trigger extraction, and then continues to use potentially freed memory.

## Root Cause

The `do_shoot()` function in `src/act.offensive.c` had multiple critical race conditions:

1. **Room Manipulation Without Validation** (lines 1187-1234 original)
   - Temporarily moved `ch` to target room to find victim
   - Moved `ch` back to original room
   - No checks if `ch` was extracted during room changes

2. **Unsafe Post-Damage Operations** (line 1221 original)
   - Called `damage(ch, vict, ...)` which can trigger:
     - Death of either character
     - Special procedures
     - DG Script triggers
     - Mobile activity interactions
   - Continued using `ch` and `vict` without validation

3. **Unsafe Hunt/Trigger Calls** (lines 1222-1225 original)
   - Called `remember(ch, vict)` without validation
   - Called `hunt_victim(ch)` which can cause extraction (known issue from graph.c fixes)
   - Called `hitprcnt_mtrigger(vict)` which can trigger scripts
   - No extraction checks after these calls

4. **Unsafe Equipment Access** (lines 1226-1230 original)
   - Accessed `GET_EQ(ch, WEAR_QUIVER)` without verifying `ch` still exists
   - Used cached `ammo` pointer after potential extraction

## Solution Implemented

### File Modified
- **src/act.offensive.c**: Modified `do_shoot()` function (lines 1111-1245)

### Changes Made

1. **Added Temporary Character Pointer** (line 1113)
   ```c
   struct char_data *vict, *tmp;
   ```
   Used for safe iteration through character_list to validate existence

2. **Fixed Self-Shoot Early Return** (lines 1190-1193)
   - Now properly moves `ch` back to original room before returning
   - Prevents `ch` being stuck in wrong room

3. **Post-Room-Change Validation** (lines 1199-1213)
   - Validates `ch` still exists in character_list after room movements
   - Validates `ch`'s room is valid (not < 0, not NOWHERE, not >= top_of_world)
   - Returns early if extraction detected or room invalid

4. **Post-Damage Character Validation** (lines 1241-1256)
   - Validates `ch` still exists in character_list after `damage()`
   - Checks extraction flags (MOB_NOTDEADYET, PLR_NOTDEADYET)
   - Returns immediately if `ch` was extracted or marked for extraction

5. **Post-Damage Victim Validation** (lines 1258-1272)
   - Validates `vict` still exists before using it
   - Only calls `remember()`, `hunt_victim()`, and `hitprcnt_mtrigger()` if victim valid
   - Prevents use-after-free on victim pointer

6. **Post-Trigger Character Validation** (lines 1274-1287)
   - Re-validates `ch` after `remember()`/`hunt_victim()`/`hitprcnt_mtrigger()` calls
   - These functions can trigger extraction through scripts or mobile activity
   - Checks extraction flags again
   - Returns if extraction detected

7. **Safe Ammo Handling** (lines 1289-1296)
   - Re-fetches ammo from equipment slot instead of using cached pointer
   - Only processes ammo if `ch` is still valid and has ammo equipped
   - Prevents use-after-free on ammo object

8. **Fixed Room Return Logic** (lines 1298-1302)
   - Only moves `ch` back if victim wasn't found
   - Prevents duplicate room movements when victim is found

### Code Pattern

The fix follows the established pattern from `hunt_victim()` in `src/graph.c`:

```c
/* After potentially unsafe operation */
for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
    if (ch == tmp)
        found = TRUE;

if (!found) {
    /* ch was extracted, cannot continue */
    return;
}

/* Check extraction flags */
if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET)) {
    return;
}
```

## Prevents Crashes When:

1. **During Room Movement**
   - Shooter is extracted while temporarily in target room
   - Shooter's original room becomes invalid

2. **During Combat**
   - Shooter dies from counter-attack
   - Victim dies from the arrow
   - Special procedures extract characters
   - Death traps activate

3. **During Script Execution**
   - DG Scripts extract shooter during damage triggers
   - DG Scripts extract victim during damage triggers
   - Script-triggered death effects

4. **During Mobile Activity**
   - `hunt_victim()` triggers extraction through mobile activity
   - Mobiles performing actions on shooter during shoot
   - Action stacking with sleeping/incapacitated targets

5. **During Equipment Access**
   - Shooter extracted before ammo removal
   - Ammo object no longer exists when accessed

## Testing Recommendations

Test scenarios to validate the fix:

1. **Basic Shooting**
   - Shoot at normal targets in adjacent rooms
   - Verify no crashes with valid targets

2. **Death Scenarios**
   - Shoot target that dies from arrow
   - Shoot while at low HP (shooter might die from counter)
   - Shoot in death trap rooms

3. **Mobile Activity Interaction**
   - Shoot while aggressive mobs are hunting
   - Shoot sleeping/paralyzed targets (action stacking risk)
   - Shoot with multiple mobs in combat

4. **Script Interactions**
   - Shoot targets with death triggers
   - Shoot in rooms with special procedures
   - Shoot with equipment that has triggers

5. **Edge Cases**
   - Shoot with last arrow (ammo extraction)
   - Shoot at self (should be prevented)
   - Shoot in invalid directions
   - Shoot with corrupted room data

## Related Fixes

This fix follows the pattern established in:
- `MOBACT_HUNT_VICTIM_AWAKE_FIX.md` - hunt_victim() extraction safety
- `MOBACT_REVIEW_SUMMARY.md` - mobile activity extraction patterns
- `EXTRACT_REVIEW_SAFETY_FIX.md` - general extraction safety patterns

## Performance Impact

Minimal - adds character_list iteration only when necessary:
- Once after room movements (cheap, happens before combat)
- Once after damage() call (necessary safety check)
- Once after trigger calls (necessary safety check)
- These iterations are O(n) where n = active characters, but only occur during actual shoot actions

## Code Review Considerations

### Validation Pattern Duplication
The character validation pattern (iterating through character_list to check existence) is repeated three times in this fix. While a helper function could reduce duplication, this pattern is:
- Already used in the existing codebase (e.g., `hunt_victim()` in `src/graph.c`)
- Consistent with the project's safety check patterns
- Simple and explicit, making the safety checks clear at each critical point
- Kept inline to maintain minimal modifications per project guidelines

A future refactoring could extract this into a `validate_char_exists()` helper function if the pattern becomes more widespread.

## Compatibility

- Maintains all existing game behavior
- Does not change shoot mechanics or damage calculation
- Only adds safety checks to prevent crashes
- Compatible with all existing world data and scripts
