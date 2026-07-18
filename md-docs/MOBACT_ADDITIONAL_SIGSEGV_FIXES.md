# Additional SIGSEGV Fixes for mobact.c mobile_activity()

## Overview

This document describes additional segmentation fault (SIGSEGV) fixes applied to the `mobile_activity()` function in `src/mobact.c` beyond the previous extensive fixes documented in other files.

## Issue

The GitHub issue requested: "We need continue looking for segmentation fault (SIGSEGV) cases within mobact.c mobile_activity() main loop."

## Analysis Methodology

A comprehensive code review was performed focusing on:
1. Array accesses without bounds checking (world[], shop_index[], mob_proto[], obj_proto[], zone_table[])
2. Helper functions that access arrays without validating parameters
3. Pointer dereferences without NULL checks
4. Iteration patterns that could lead to use-after-free
5. Pattern consistency across the codebase

## Fixes Applied

### Fix #1: Room Validation in Helper Functions (CRITICAL)

**Location**: `get_mob_in_room_by_rnum()` at line 3189 and `get_mob_in_room_by_vnum()` at line 3214

**Problem**: 
These helper functions directly accessed `world[room].people` without validating that the `room` parameter is within valid bounds (0 to top_of_world). While most callers do check `IN_ROOM(ch)`, the functions themselves lacked defensive programming.

**Before**:
```c
struct char_data *get_mob_in_room_by_rnum(room_rnum room, mob_rnum rnum)
{
    struct char_data *i;
    for (i = world[room].people; i; i = i->next_in_room) {  // ← No bounds check!
        if (IS_NPC(i) && GET_MOB_RNUM(i) == rnum) {
            return i;
        }
    }
    return NULL;
}
```

**After**:
```c
struct char_data *get_mob_in_room_by_rnum(room_rnum room, mob_rnum rnum)
{
    struct char_data *i;
    
    /* Safety check: Validate room before accessing world array */
    if (room == NOWHERE || room < 0 || room > top_of_world) {
        return NULL;
    }
    
    for (i = world[room].people; i; i = i->next_in_room) {
        if (IS_NPC(i) && GET_MOB_RNUM(i) == rnum) {
            return i;
        }
    }
    return NULL;
}
```

**Impact**: Prevents potential segfault if these functions are called with invalid room numbers due to bugs or edge cases.

**Severity**: CRITICAL - Direct array access violation

---

### Fix #2: Shop Index Bounds Check

**Location**: `handle_duty_routine()` at line 2042

**Problem**:
Code checked `shop_nr != -1` before accessing `shop_index[shop_nr]` but didn't verify that `shop_nr` is within the valid range (0 to top_shop).

**Before**:
```c
if (shop_nr != -1 && is_shop_open(shop_nr) && shop_index[shop_nr].in_room) {
    // ← Missing bounds check for shop_nr
```

**After**:
```c
if (shop_nr != -1 && shop_nr <= top_shop && is_shop_open(shop_nr) && shop_index[shop_nr].in_room) {
    // ← Now validates shop_nr is within bounds
```

**Impact**: Prevents array out-of-bounds access if `find_shop_by_keeper()` returns an invalid shop number.

**Severity**: MEDIUM - Could cause segfault with corrupted shop data

---

### Fix #3: mob_proto Array Bounds Check

**Location**: `mob_process_wishlist_goals()` at line 3701

**Problem**:
Code accessed `mob_proto[target_mob]` with only a NOBODY check, lacking validation that `target_mob` is a valid array index.

**Before**:
```c
if (target_mob != NOBODY) {
    if (GET_LEVEL(ch) >= mob_proto[target_mob].player.level - 5) {
        // ← No bounds check on target_mob
```

**After**:
```c
if (target_mob != NOBODY && target_mob >= 0 && target_mob < top_of_mobt) {
    if (GET_LEVEL(ch) >= mob_proto[target_mob].player.level - 5) {
        // ← Now validates target_mob is within bounds
```

**Impact**: Prevents segfault when accessing mob prototype data with invalid mob rnum.

**Severity**: MEDIUM - Could occur if `find_mob_with_item()` returns corrupted data

---

### Fix #4: obj_proto Array Bounds Check

**Location**: `mob_process_wishlist_goals()` at line 3722

**Problem**:
Code accessed `obj_proto[obj_rnum]` with only a NOTHING check, lacking validation that `obj_rnum` is a valid array index.

**Before**:
```c
obj_rnum = real_object(desired_item->vnum);
if (obj_rnum != NOTHING) {
    item_cost = GET_OBJ_COST(&obj_proto[obj_rnum]);
    // ← No bounds check on obj_rnum
```

**After**:
```c
obj_rnum = real_object(desired_item->vnum);
if (obj_rnum != NOTHING && obj_rnum >= 0 && obj_rnum < top_of_objt) {
    item_cost = GET_OBJ_COST(&obj_proto[obj_rnum]);
    // ← Now validates obj_rnum is within bounds
```

**Impact**: Prevents segfault when calculating item costs with invalid object rnum.

**Severity**: MEDIUM - Could occur if object tables are corrupted

---

### Fix #5: Bounds Check Consistency

**Changes**: Modified bounds checks to use `<` instead of `<=` operator

**Reasoning**:
Code review identified that the codebase consistently uses `< top_of_mobt` and `< top_of_objt` rather than `<= top_of_mobt` and `<= top_of_objt`. This matches the for-loop pattern `for (i = 0; i < top_of_objt; i++)` used throughout the code.

**Examples from codebase**:
- `src/act.wizard.c:5341`: `mob->ai_data->goal_target_mob_rnum < top_of_mobt`
- `src/act.wizard.c:2465`: `for (i = 0; i < top_of_objt; i++)`

**Severity**: LOW - Code quality/consistency fix

## Verification

### Build Testing
```bash
$ cmake -B build -S .
$ cmake --build build
# Result: ✅ Build successful, 0 errors, 0 warnings in mobact.c
```

### Security Analysis
```bash
$ codeql analyze
# Result: ✅ 0 security issues found
```

### Code Quality
- ✅ Code formatted with `clang-format` per project standards
- ✅ All changes follow existing patterns in the codebase
- ✅ Minimal, surgical changes - no refactoring
- ✅ No behavioral changes to game logic

## Code Review Results

Two rounds of code review were performed:

**Round 1**: Identified bounds check inconsistency (Fix #5)
**Round 2**: ✅ No issues found

## Areas Verified Safe

The following areas were thoroughly reviewed and found to be adequately protected:

### Already Protected by Previous Fixes
✅ Character extraction checks (MOB_NOTDEADYET/PLR_NOTDEADYET throughout)
✅ Room validation at loop entry and before complex operations
✅ Safe iteration patterns (next_ch, next_vict, next_target saved pointers)
✅ Nested iteration safety over character_list
✅ Hunt victim race conditions (HUNT_VICTIM_RACE_CONDITION_FIX.md)
✅ Death trap handling (DEATH_TRAP_FIX.md)
✅ Shopping system segfaults (MOBACT_SHOPPING_SEGFAULT_FIX.md)

### Verified Safe in This Review
✅ **mob_index accesses**: Protected by IS_MOB check which includes `GET_MOB_RNUM(ch) <= top_of_mobt`
✅ **Memory iteration**: Safe NULL checks in memory record loops (line 1188-1189)
✅ **Object iteration**: Proper next_content pointer saving before list modifications (lines 648, 662, 665, 745)
✅ **zone_table accesses**: Room validation ensures zones are valid since world[room].zone is only accessed after room validation
✅ **AI data access**: Consistent null checks (`!ch->ai_data`) before dereferencing
✅ **Pointer chains**: All dereferences have proper null checks before access

## Testing Recommendations

While these are defensive fixes with no behavioral changes, the following testing is recommended:

1. **Mob AI Behavior Testing**
   - Mobs with shopping goals
   - Mobs hunting other mobs
   - Mobs accepting and completing quests
   - Mobs with wishlist-based goals

2. **Edge Case Testing**
   - Load corrupted world files with invalid room numbers
   - Test with missing shops referenced by mobs
   - Test with invalid mob/object vnums in quest data

3. **Stability Testing**
   - Long-running server (24+ hours) with heavy mob activity
   - Monitor for crashes or segfaults
   - Check syslog for any "SYSERR" messages

## Related Documentation

This fix complements extensive previous work:
- `MOBILE_ACTIVITY_FIX.md` - NULL pointer and iterator bugs
- `MOBACT_EXTRACT_FIX.md` - Character extraction safety
- `MOBACT_RACE_CONDITION_SUMMARY.md` - Nested iteration safety
- `MOBACT_SHOPPING_SEGFAULT_FIX.md` - Shopping system crashes
- `MOBACT_UNSAFE_CASES_FIX.md` - Additional unsafe patterns
- `DEATH_TRAP_FIX.md` - Death trap handling
- `HUNT_VICTIM_RACE_CONDITION_FIX.md` - Hunt victim safety

## Conclusion

All identified potential segmentation fault cases in `mobile_activity()` have been addressed. The function now has comprehensive protection against:

- ❌ Invalid room numbers causing world array violations
- ❌ Invalid shop numbers causing shop_index violations  
- ❌ Invalid mob rnums causing mob_proto violations
- ❌ Invalid object rnums causing obj_proto violations
- ❌ Character extraction during iteration
- ❌ NULL pointer dereferences

**Status**: ✅ COMPLETE - No further SIGSEGV risks identified in mobile_activity() main loop.

---

**Author**: GitHub Copilot
**Date**: 2025-10-25
**Commit**: 195c980
**Files Modified**: `src/mobact.c`
