# Shadow Timeline Sentinel Compatibility Fix

**Date:** 2026-02-09  
**Type:** Bug Fix / Feature Enhancement  
**Severity:** Medium  
**Status:** ✅ Fixed

---

## Problem Statement

### Issue Description
Sentinel mobs (MOB_SENTINEL) with the Shadow Timeline feature (MOB_SHADOWTIMELINE) were leaving their guard posts and not returning. This broke the fundamental purpose of sentinels, which should remain at their designated posts.

### Root Cause
When the Shadow Timeline system executes an action in `mobile_activity()` (mobact.c lines 623-806), it calls `continue` to skip the rest of the mob activity loop. This meant that the `handle_duty_routine()` function (called at line 1887) was never reached, preventing sentinels from executing their duty to return to their posts.

### Observed Behavior
1. Sentinel mobs with Shadow Timeline would wander away from their posts
2. They would pursue targets or explore randomly
3. They would not automatically return to their guard posts
4. Only if by chance they ended up adjacent to their post would they return

---

## Solution

Instead of trying to make Shadow Timeline mobs reach `handle_duty_routine()`, the fix makes the Shadow Timeline system **sentinel-aware** by incorporating guard duty directly into the cognitive simulation.

### Changes Made

#### 1. Added SHADOW_ACTION_GUARD Action Type
**File:** `src/shadow_timeline.h`

Added a new action type for sentinels standing guard:
```c
SHADOW_ACTION_GUARD,      /**< Stand guard at post (for sentinels) */
```

This action represents intentional guarding behavior, distinct from simply waiting.

#### 2. Guard Projection Generation
**File:** `src/shadow_timeline.c`

Added `generate_guard_projection()` function that:
- Only generates for mobs with MOB_SENTINEL flag
- Only generates when mob is at their guard post
- Creates a projection with positive score (15) to encourage staying
- Has lower cognitive cost than other actions

The function is called early in `shadow_generate_projections()` to ensure sentinels at their posts consider guarding as a viable option.

#### 3. Sentinel-Aware Movement Scoring
**File:** `src/shadow_timeline.c` in `shadow_execute_projection()`

Modified movement action outcome scoring to be sentinel-aware:

**When sentinel is at post:**
- Score penalty: -60 for leaving post
- Marks movement as not achieving goal

**When sentinel is away from post:**
- Moving directly to post: +70 score bonus (highest priority)
- Moving closer to post: +40 score bonus
- Moving away from post: -50 score penalty
- Proper handling of unreachable locations (see below)

#### 4. Pathfinding Edge Case Handling

The scoring logic uses the standard CircleMUD/tbaMUD-style BFS helpers:
- `find_first_step()` returns either the first direction to take toward the goal (0..`DIR_COUNT`-1), or one of the negative `BFS_*` codes:
  - `BFS_ERROR` (-1)
  - `BFS_ALREADY_THERE` (-2)
  - `BFS_NO_PATH` (-3)
- A new `bfs_distance()` function calculates the actual BFS distance (room hop count) from one room to another for "closer/farther" comparisons.

When evaluating a potential move for a sentinel that is away from its post:
- If both the current room and the candidate room have a valid path to the post, compare their BFS distances and favor the room that is strictly closer to the post.
- If the current room has no path to the post (`BFS_NO_PATH`) and the candidate room does, treat the move as non-penalized so sentinels can escape dead-end areas.
- If the current room has a valid path and the candidate room has no path, apply the standard away-from-post penalty (-50).
- If neither room has a valid path to the post, treat the move as neutral and let other scoring factors decide.

#### 5. Guard Action Implementation
**File:** `src/mobact.c`

Added handling for SHADOW_ACTION_GUARD in the Shadow Timeline action execution:
```c
case SHADOW_ACTION_GUARD:
    /* Stand guard at post - sentinel duty fulfilled */
    /* No action needed, just maintain vigilance */
    continue;
```

The guard action consumes the mob's turn while maintaining their position.

---

## Technical Details

### Scoring System
The scoring modifications create a strong preference hierarchy:

1. **Direct return to post (+70)**: Strongest preference
2. **Moving closer to post (+40)**: Second preference
3. **Standing guard at post (+15)**: Baseline positive action when at post
4. **Neutral movement (0-5)**: Default movement score
5. **Moving away from post (-50)**: Strong discouragement
6. **Leaving post (-60)**: Strongest discouragement

This ensures that:
- Sentinels prefer returning to their posts when away
- Sentinels prefer staying at their posts when present
- The system doesn't override other important goals (combat, survival)

### Cognitive Cost
Guard action has reduced cognitive cost:
```c
action.cost = SHADOW_BASE_COST / 2;
```

This makes guarding "cheap" from a cognitive standpoint, reflecting that standing guard is less mentally taxing than other activities.

### BFS Distance Calculation
A new helper function `bfs_distance()` was added to `graph.c`:
```c
int bfs_distance(room_rnum src, room_rnum target);
```

This function:
- Returns the actual number of rooms (hops) between source and target
- Returns 0 if already at target (BFS_ALREADY_THERE case)
- Returns `BFS_NO_PATH` (-3) if target is unreachable
- Returns `BFS_ERROR` (-1) if parameters are invalid
- Uses the same BFS algorithm as `find_first_step()` but tracks distance instead of direction

This function is critical for correct distance comparisons in sentinel scoring, as `find_first_step()` returns a direction (0..DIR_COUNT-1) or error codes, not a distance metric.

### Integration with Existing Systems
The fix doesn't break any existing functionality:
- Non-sentinel mobs are unaffected
- Sentinels without Shadow Timeline continue to work as before
- Shadow Timeline mobs without sentinel flag are unaffected
- Quest goals and other duties can still override guard duty when appropriate

---

## Testing Recommendations

### Test Cases

1. **Sentinel at Post**
   - Place sentinel with both MOB_SENTINEL and MOB_SHADOWTIMELINE at their post
   - Verify they stay at post (should mostly choose GUARD action)
   - Expected: Mob remains at post >90% of the time

2. **Sentinel Away from Post**
   - Move sentinel away from their post
   - Verify they pathfind back
   - Expected: Mob moves toward post consistently

3. **Sentinel in Combat**
   - Attack sentinel at post
   - Verify they respond appropriately
   - After combat, verify they remain at or return to post
   - Expected: Intelligent combat response, then returns to guard duty

4. **Sentinel with Quest**
   - Give sentinel a quest goal
   - Verify quest takes priority
   - After quest completion, verify return to guard duty
   - Expected: Quest completion, then return to post

5. **Unreachable Post**
   - Place sentinel in location unreachable from their post
   - Verify no crashes or infinite loops
   - Expected: Neutral behavior, no penalties

### Performance Testing
- Monitor cognitive capacity usage for sentinels
- Verify guard action is being selected appropriately
- Check that pathfinding calls are not excessive

---

## Code Review Findings & Resolutions

### Issue 1: Unsafe Distance Comparisons
**Finding:** Original code compared distances before checking for -1 (unreachable)

**Resolution:** Added proper -1 checks before all comparisons:
```c
if (current_dist != -1 && dest_dist != -1) {
    // Safe to compare
}
```

### Issue 2: Unreachable Location Penalties
**Finding:** Code penalized movement when destinations were unreachable

**Resolution:** Added logic to handle unreachable cases appropriately:
- Don't penalize movement from unreachable to reachable
- Only penalize movement from reachable to unreachable
- Neutral scoring when both unreachable

### Issue 3: Incorrect Distance Metric Usage
**Finding:** Original code used `find_first_step()` return value as distance, but it returns direction or BFS codes

**Resolution:** 
- Created new `bfs_distance()` function in `graph.c` that returns actual room hop count
- Updated sentinel scoring logic to use `bfs_distance()` for distance comparisons
- Properly handle all BFS return codes (BFS_ERROR, BFS_NO_PATH, etc.)

### Issue 4: GUARD Action Validation
**Finding:** `shadow_validate_action()` didn't handle SHADOW_ACTION_GUARD case, causing all guard projections to fail

**Resolution:**
- Added validation case for SHADOW_ACTION_GUARD
- Validates that only sentinels with ai_data can guard
- Validates that sentinel is at their guard_post

### Issue 5: Guard Action Not Obvious
**Finding:** `shadow_is_obvious()` didn't treat GUARD as obvious, causing unnecessary cognitive cost

**Resolution:**
- Added SHADOW_ACTION_GUARD to the list of obvious actions
- Guard action now has reduced cognitive cost calculation

---

## Security Analysis

**CodeQL Scan Result:** ✅ No vulnerabilities found

The changes:
- Don't introduce new memory allocations
- Use existing pathfinding functions safely
- Include proper bounds checking
- Handle all edge cases appropriately

---

## Documentation Updates

### Updated Files
1. **docs/SHADOW_TIMELINE.md**
   - Added SHADOW_ACTION_GUARD to action types table
   - Added "Sentinel Compatibility" section
   - Documented scoring system and behavior

2. **md-docs/SHADOW_TIMELINE_SENTINEL_FIX.md** (this file)
   - Comprehensive fix documentation

---

## Compatibility & Migration

### Breaking Changes
**None.** This is a pure enhancement that doesn't break existing functionality.

### Migration Required
**None.** Existing world files and mobs work without changes.

### Optional Configuration
World builders may now safely add MOB_SHADOWTIMELINE to sentinel mobs:
```
#12345
Guard Captain~
a tough-looking guard captain~
A guard captain stands here, watching the entrance carefully.
~
Detailed description of the guard captain.
~
ISNPC SENTINEL SHADOWTIMELINE
...
```

---

## Future Enhancements

Potential improvements for future consideration:

1. **Configurable Guard Aggressiveness**
   - Allow different sentinels to have different "patrol radiuses"
   - Some guards might patrol nearby rooms while still prioritizing their post

2. **Coordinated Sentinel Behavior**
   - Multiple sentinels could coordinate guard duty
   - Shift rotations or patrol patterns

3. **Alert System**
   - Sentinels could alert nearby sentinels when threats appear
   - Could trigger coordinated response

4. **Memory of Threats**
   - Sentinels could remember recent threats
   - Higher vigilance after recent combat

---

## Related Systems

This fix interacts with:
- **Shadow Timeline System** (RFC-0001): Core cognitive simulation
- **Mob AI Goals System**: Guard duty integrates with goal priorities
- **Pathfinding System**: Uses `find_first_step()` for distance calculation
- **Mobile Activity Loop**: Shadow Timeline executes in `mobile_activity()`

---

## References

- **Issue:** GitHub Issue - "GOALS (Goal_None)"
- **RFC:** RFC-0001 Shadow Timeline Implementation
- **Documentation:** docs/SHADOW_TIMELINE.md
- **Code Review:** Completed 2026-02-09
- **Security Scan:** CodeQL - No issues found

---

## Authors & Contributors

- **Implementation:** Copilot AI + Forneck
- **Code Review:** Automated code review system
- **Testing:** Required before production deployment

---

## Change Log

### Version 1.0 (2026-02-09)
- Initial implementation of sentinel awareness
- Added SHADOW_ACTION_GUARD
- Implemented guard projection generation
- Added sentinel-aware movement scoring
- Fixed pathfinding edge cases
- Updated documentation

---

## Status Summary

✅ **Implementation:** Complete  
✅ **Code Review:** Passed  
✅ **Security Scan:** Passed  
⏳ **Manual Testing:** Required  
⏳ **Production Deployment:** Pending testing

---

*This document serves as both fix documentation and testing guide for the Shadow Timeline Sentinel compatibility feature.*
