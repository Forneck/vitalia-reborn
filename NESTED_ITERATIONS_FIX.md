# Nested character_list Iteration Safety Fix

## Problem Summary

The `mobile_activity()` function in `mobact.c` contained unsafe nested iterations over the global `character_list`. This could lead to segmentation faults when characters were extracted during iteration.

### Issue Context

The issue was reported as a potential race condition between `beware_lightning()` and `mobile_activity()`. However, investigation revealed that:

1. **Not a race condition**: The heartbeat() function is NOT reentrant - it's called sequentially from the main game loop. Therefore, `beware_lightning()` and `mobile_activity()` cannot run simultaneously.

2. **The real issue**: Unsafe nested iterations within `mobile_activity()` itself that don't follow the safe iteration pattern.

## Root Cause Analysis

### Safe vs Unsafe Iteration Patterns

**Safe Pattern (used in main loop):**
```c
for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;  // ← Save next pointer BEFORE operations
    
    /* Safety checks */
    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
        continue;
    
    /* Operations that might call extract_char() */
    // ...
}
```

**Unsafe Pattern (found in nested loops):**
```c
for (target = character_list; target; target = target->next) {
    // ← No safety variable saved
    // ← No extraction checks
    // ← Room validity checked AFTER using as array index
    
    if (IS_NPC(target) && IN_ROOM(target) != NOWHERE &&
        world[IN_ROOM(target)].zone == ...) {  // ← CRASH if target extracted
        // ...
    }
}
```

### Why Unsafe Patterns Cause Segfaults

1. **Pointer corruption**: If `extract_char(target)` is called during iteration:
   - The `target` pointer is marked for extraction
   - `target->next` becomes unreliable
   - Next iteration accesses invalid memory

2. **Array bounds violation**: Accessing `world[IN_ROOM(target)]` before validating:
   - If `target` is extracted, `IN_ROOM(target)` may be NOWHERE (-1)
   - This causes array underflow: `world[-1]` = undefined behavior

3. **Deferred extraction**: Characters are marked but not immediately freed:
   - The `NOTDEADYET` flag is set
   - Actual memory deallocation happens later in `extract_pending_chars()`
   - But character data may already be corrupted

## Vulnerable Code Locations

### 1. Bounty Quest Posting (Line 708-723)

**Original Code:**
```c
struct char_data *target;
for (target = character_list; target; target = target->next) {
    if (IS_NPC(target) && target != ch && IN_ROOM(target) != NOWHERE &&
        world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone && 
        MOB_FLAGGED(target, MOB_AGGRESSIVE) && ...) {
        // ...
    }
}
```

**Problem:**
- No `next_target` safety variable
- Room validation happens AFTER dereferencing
- No extraction flag checks

### 2. Friendly Mob Quest (Line 761-774)

Same unsafe pattern as #1.

### 3. Protection Quest (Line 778-791)

Same unsafe pattern as #1.

### 4. Kill Quest (Line 817-830)

Same unsafe pattern as #1.

### 5. find_questmaster_by_vnum() Function (Line 2714-2728)

**Original Code:**
```c
for (i = character_list; i; i = i->next) {
    if (IS_NPC(i) && GET_MOB_VNUM(i) == vnum) {
        if (mob_index[GET_MOB_RNUM(i)].func == questmaster || ...) {
            return i;
        }
    }
}
```

**Problem:**
- Called from within `mobile_activity()`
- No safety variable
- No validation checks

## Solution: Comprehensive Safety Fixes

Applied the safe iteration pattern to all 5 vulnerable locations:

### Pattern Applied

```c
struct char_data *target, *next_target;
for (target = character_list; target; target = next_target) {
    next_target = target->next;  // ← Save next pointer immediately
    
    /* Safety check: Skip characters marked for extraction */
    if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
        continue;
    
    /* Safety check: Validate room BEFORE accessing world array */
    if (!IS_NPC(target) || target == ch || 
        IN_ROOM(target) == NOWHERE || 
        IN_ROOM(target) < 0 || 
        IN_ROOM(target) > top_of_world)
        continue;
    
    /* Now safe to use target and world[IN_ROOM(target)] */
    if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone && ...) {
        // ...
    }
}
```

### Key Improvements

1. **Safety Variable**: `next_target = target->next` saved immediately
   - Protects against pointer corruption
   - Ensures valid iteration even if target is extracted

2. **Extraction Checks**: Test `NOTDEADYET` flags first
   - Skips characters already marked for extraction
   - Prevents accessing corrupted character data

3. **Room Validation**: Check room bounds BEFORE array access
   - Prevents array underflow/overflow
   - Ensures `world[IN_ROOM(target)]` is valid

4. **Early Continue**: Skip invalid targets before any operations
   - Minimizes unnecessary processing
   - Reduces risk of side effects

## Files Modified

- `src/mobact.c` - Fixed 5 instances of unsafe character_list iteration:
  1. Line 708-723: Bounty quest posting
  2. Line 761-774: Friendly mob quest posting
  3. Line 778-791: Protection quest posting
  4. Line 817-830: Kill quest posting
  5. Line 2714-2728: find_questmaster_by_vnum() function

## Testing

1. **Compilation**: 
   - ✅ Code compiles successfully with GCC
   - ✅ No warnings generated
   
2. **Code Formatting**: 
   - ✅ Formatted with clang-format according to project style
   
3. **Build Systems**:
   - ✅ Autotools build successful
   - ✅ CMake build successful

## Impact

This fix prevents crashes when:
- Mobs are extracted during quest posting operations
- Character list changes during nested iterations
- Room assignments become invalid during iteration
- Multiple mobs process AI simultaneously

### Performance Impact

Minimal - the changes add:
- One pointer assignment per iteration (`next_target = target->next`)
- 2-3 flag checks per iteration (extraction and room validation)
- Early exit logic reduces wasted processing on invalid targets

### Defensive Programming

The fix follows defensive programming principles:
- **Fail-safe**: Validates data before use
- **Predictable**: Always checks the same conditions in the same order
- **Consistent**: Uses the same pattern as the main loop
- **Clear**: Safety checks are explicitly commented

## Prevention Guidelines

To prevent similar bugs when adding new code:

1. **Always use safety variables** when iterating character_list:
   ```c
   for (ch = character_list; ch; ch = next_ch) {
       next_ch = ch->next;  // ← REQUIRED
       // ...
   }
   ```

2. **Check extraction flags early**:
   ```c
   if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
       continue;
   ```

3. **Validate room before array access**:
   ```c
   if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
       continue;
   ```

4. **Apply consistently** to ALL character_list iterations, including:
   - Main loops in functions
   - Nested loops
   - Helper functions that search character_list

5. **Code review checklist**:
   - [ ] Does the loop save `next` pointer before operations?
   - [ ] Are extraction flags checked?
   - [ ] Is room validity checked before array access?
   - [ ] Does the pattern match safe iterations elsewhere?

## Related Fixes

This fix builds upon previous safety improvements:

- **MOBACT_EXTRACT_FIX.md**: Added safety checks after operations that can extract characters
- **MOBILE_ACTIVITY_FIX.md**: Fixed null pointer bugs in victim loops
- **SIGSEGV_FIX.md**: Added room validation checks

Together, these fixes create a comprehensive defense against iterator-related crashes.

## References

- Original issue: "Interactions with mobact - potential race issue between beware_lightning and mobile_activity"
- Analysis revealed: Not a race condition, but unsafe nested iterations
- Pattern borrowed from: Main loop in `mobile_activity()` (line 138)
- Testing: Builds successfully with both autotools and CMake
