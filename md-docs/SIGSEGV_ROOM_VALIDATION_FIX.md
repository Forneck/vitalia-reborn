# SIGSEGV Room Validation Fix

## Problem Summary

The MUD server was experiencing segmentation faults (SIGSEGV) in `mobile_activity()` and related functions. The issue description noted:

> Still the Program received signal SIGSEGV, Segmentation fault.
> 0x000000000050ba16 in mobile_activity()
> 
> So there must be some edge cases and unsafe that usually doesn't happens when running without sudo access.

## Root Cause

Multiple locations in the code were accessing `world[IN_ROOM(ch)]` without validating that `IN_ROOM(ch)` was a valid room index. This could cause:

1. **Out-of-bounds array access** when `IN_ROOM(ch)` is negative or greater than `top_of_world`
2. **Invalid memory access** when `IN_ROOM(ch) == NOWHERE` (which equals -1 or 65535 depending on configuration)
3. **Race conditions** when mobs are extracted or moved during operations

### Why This Was Hard to Reproduce

The crashes only occurred in specific edge cases:
- Running with elevated privileges (sudo) may expose different memory patterns
- Timing-dependent: only when mobs are extracted during iteration
- State-dependent: only when certain game states trigger vulnerable code paths
- Different machines have different memory layouts

## Solution

Added defensive room validation checks before all `world[IN_ROOM(ch)]` accesses in:

### 1. Quest Posting Code (Lines 718-894)
**Issue:** Accessed `world[IN_ROOM(ch)].zone` without validation
**Fix:** Added check at start of quest posting block and within loops comparing zones
```c
if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
    continue;
```

### 2. mob_handle_grouping() (Lines 1396-1550)
**Issue:** Accessed `world[IN_ROOM(ch)].people` when searching for group members
**Fix:** Added checks before accessing people list in both group scenarios
- Scenario 1: Leader merging into larger group
- Scenario 2: Solo mob joining existing group

### 3. mob_try_and_loot() (Line 2074)
**Issue:** Accessed `world[IN_ROOM(ch)].contents` without validation
**Fix:** Added check at function start
```c
if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
    return FALSE;
```

### 4. mob_assist_allies() (Line 2001)
**Issue:** Accessed `world[IN_ROOM(ch)].people` when helping other NPCs
**Fix:** Wrapped helper code in room validation check

### 5. perform_move_IA() (Line 1633)
**Issue:** Accessed `world[IN_ROOM(ch)].sector_type` for learning after movement
**Fix:** Added check before accessing post-movement room data

### 6. mob_goal_oriented_roam() (Line 1687)
**Issue:** Accessed `world[to_room].zone` and `world[IN_ROOM(ch)].zone` for zone comparison
**Fix:** 
- Enhanced to_room check to include `!= NOWHERE && >= 0`
- Added ch room validation before zone comparison

### 7. mob_try_to_accept_quest() (Line 2968)
**Issue:** Accessed `world[IN_ROOM(ch)].zone` when searching for questmasters
**Fix:** Added check before zone access

### 8. mob_process_wishlist_goals() (Line 3209)
**Issue:** Accessed `world[IN_ROOM(ch)].zone` when processing wishlist
**Fix:** Added validation before zone access

### 9. mob_try_sacrifice() (Line 3428)
**Issue:** Accessed `world[IN_ROOM(ch)].contents` when finding corpses
**Fix:** Added check before room contents iteration

### 10. find_bank_nearby() (Line 3698)
**Issue:** Accessed `world[IN_ROOM(ch)].contents` when searching for banks
**Fix:** Added validation at function start

### 11. find_best_leader_for_new_group() (Line 1341)
**Issue:** Accessed `world[IN_ROOM(ch)].people` when evaluating potential group members
**Fix:** Added check before people list access

## Validation Pattern

All checks follow this pattern (matching existing code style):
```c
if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
    return/continue/break; // appropriate for context
```

## Testing Results

✅ **Compilation:** Clean build with no errors or warnings related to changes
✅ **Code Formatting:** All changes formatted with `clang-format -i`
✅ **Security Scan:** CodeQL analysis found 0 alerts
✅ **Pattern Consistency:** Matches existing defensive checks in the codebase

## Code Review Feedback

One comment received: Consider using `VALID_ROOM_RNUM(room)` macro instead of manual checks.

**Response:** Valid point for future refactoring. However:
- Existing code in mobact.c uses explicit pattern extensively (lines 121, 149, 190, 247, 316, etc.)
- Changing all instances would make PR much larger
- Our changes maintain consistency with immediate context

**Recommendation:** Address in future cleanup PR to standardize all room validation across codebase.

## Impact

This fix prevents crashes when:
- Mobs post or accept quests
- Mobs form groups or search for leaders  
- Mobs loot items from rooms
- Mobs assist allies in combat
- Mobs move between rooms with learning
- Mobs use banks or sacrifice corpses
- Any world array access during edge case states

The fixes are defensive, add minimal overhead (just comparison operations), and follow established patterns in the codebase.

## Files Modified

- `src/mobact.c`: Added 15+ room validation checks across 11 functions

## References

- Original issue: SIGSEGV in mobile_activity() 
- Related fixes: MOBILE_ACTIVITY_FIX.md, SIGSEGV_FIX.md
- Stack trace: 0x000000000050ba16 in mobile_activity()
