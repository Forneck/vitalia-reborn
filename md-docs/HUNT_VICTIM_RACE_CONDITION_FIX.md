# Hunt Victim and Mobile Activity Race Condition Fixes

## Problem Summary

The game was experiencing intermittent segmentation faults in `mobile_activity()` that could be related to:
1. Mob level-up system
2. Track/hunting system  
3. Pathfinding cache system
4. Experimental auction settings (investigated but found not to be a cause)

Investigation revealed several race conditions where character pointers could become dangling between validation and usage.

## Root Causes Identified

### 1. Race Condition in hunt_victim() (graph.c)

**Location**: `src/graph.c`, function `hunt_victim()`

**Problem**: The function validated that `HUNTING(ch)` exists in the character list, but then dereferenced it multiple times without re-validation:

```c
// Validated once at start
for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
    if (HUNTING(ch) == tmp)
        found = TRUE;

// Then used multiple times without re-validation:
dir = mob_smart_pathfind(ch, IN_ROOM(HUNTING(ch)));  // Line 1858
dir = find_first_step(IN_ROOM(ch), IN_ROOM(HUNTING(ch)));  // Line 1861 & 1865
snprintf(buf, sizeof(buf), "...", ELEA(HUNTING(ch)));  // Line 1871
if (IN_ROOM(ch) == IN_ROOM(HUNTING(ch)))  // Line 1876
    hit(ch, HUNTING(ch), TYPE_UNDEFINED);  // Line 1877
```

**Why this causes segfaults**:
- Between validation and usage, the hunted character could be extracted (e.g., death, quit, zone reset)
- Functions like `do_say()` and `perform_move()` can trigger spec_procs that extract characters
- Accessing `IN_ROOM(HUNTING(ch))` or `ELEA(HUNTING(ch))` with a dangling pointer causes immediate segfault

### 2. Unsafe Iteration in check_mob_level_up() (mobact.c)

**Location**: `src/mobact.c`, function `check_mob_level_up()`

**Problem**: The viewer loop didn't cache the next pointer before calling `act()`:

```c
for (viewer = world[IN_ROOM(ch)].people; viewer; viewer = viewer->next_in_room) {
    if (IS_MOB(viewer) || viewer == ch)
        continue;
    act("$n parece ter ficado mais experiente!", TRUE, ch, 0, viewer, TO_VICT);
}
```

**Why this causes segfaults**:
- If `act()` triggers extraction of `viewer` or causes room changes
- Accessing `viewer->next_in_room` after `viewer` is freed causes segfault
- While rare, this can happen with triggers or special procedures

### 3. Insufficient Validation in Pathfinding Cache

**Location**: `src/graph.c`, functions `get_cached_pathfind()` and `cache_pathfind_result_priority()`

**Problem**: The cache didn't validate room numbers before use, potentially storing or retrieving invalid data.

## Solutions Implemented

### 1. Fixed hunt_victim() Race Condition

**Changes**:
1. Cache the victim pointer and room number at the start
2. Validate the victim's room is valid before pathfinding
3. Re-validate victim exists before accessing it in error messages
4. Re-validate victim exists after movement before attacking

**Code**:
```c
void hunt_victim(struct char_data *ch)
{
    int dir;
    byte found;
    struct char_data *tmp, *victim;
    room_rnum victim_room;

    if (!ch || !HUNTING(ch) || FIGHTING(ch))
        return;

    /* Cache the victim pointer to avoid multiple dereferences */
    victim = HUNTING(ch);

    /* Validate victim exists */
    for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
        if (victim == tmp)
            found = TRUE;

    if (!found) {
        /* ... clear hunting ... */
        return;
    }

    /* Cache victim's room and validate it */
    victim_room = IN_ROOM(victim);
    if (victim_room == NOWHERE || victim_room < 0 || victim_room > top_of_world) {
        HUNTING(ch) = NULL;
        return;
    }

    /* Use cached victim_room for pathfinding */
    dir = mob_smart_pathfind(ch, victim_room);
    
    if (dir < 0) {
        /* Re-validate victim before accessing */
        for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
            if (victim == tmp)
                found = TRUE;
        
        if (found) {
            snprintf(buf, sizeof(buf), "Maldição!  Eu perdi %s!", ELEA(victim));
            do_say(ch, buf, 0, 0);
        }
        HUNTING(ch) = NULL;
    } else {
        perform_move(ch, dir, 1);
        
        /* Re-validate victim after movement */
        for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
            if (victim == tmp)
                found = TRUE;

        if (found && IN_ROOM(ch) == IN_ROOM(victim))
            hit(ch, victim, TYPE_UNDEFINED);
        else if (!found)
            HUNTING(ch) = NULL;
    }
}
```

**Key improvements**:
- Single validation stores result in `victim` variable
- Room cached before any operations that could modify game state
- Re-validation before every access after potentially-extracting operations
- Clear HUNTING pointer if victim is gone

### 2. Fixed check_mob_level_up() Iteration

**Changes**:
1. Cache next pointer before calling `act()`
2. Use safe iteration pattern

**Code**:
```c
struct char_data *viewer, *next_viewer;
for (viewer = world[IN_ROOM(ch)].people; viewer; viewer = next_viewer) {
    next_viewer = viewer->next_in_room;
    if (IS_MOB(viewer) || viewer == ch)
        continue;
    act("$n parece ter ficado mais experiente!", TRUE, ch, 0, viewer, TO_VICT);
}
```

**Key improvements**:
- `next_viewer` cached before `act()` call
- Safe even if `viewer` is extracted during `act()`
- Follows standard safe iteration pattern used elsewhere in codebase

### 3. Added Pathfinding Cache Validation

**Changes**:
1. Validate room numbers before cache lookup
2. Validate room numbers before storing in cache

**Code**:
```c
static int get_cached_pathfind(room_rnum src, room_rnum target)
{
    /* ... */
    
    /* Validate src and target rooms */
    if (src == NOWHERE || target == NOWHERE || src < 0 || target < 0 || 
        src > top_of_world || target > top_of_world)
        return -1;
    
    /* ... cache lookup ... */
}

static void cache_pathfind_result_priority(room_rnum src, room_rnum target, int direction, int priority)
{
    /* ... */
    
    /* Validate src and target rooms before caching */
    if (src == NOWHERE || target == NOWHERE || src < 0 || target < 0 || 
        src > top_of_world || target > top_of_world)
        return;
    
    /* ... cache storage ... */
}
```

**Key improvements**:
- Prevents caching or using invalid room data
- Protects against edge cases with zone resets or world changes
- Minimal performance impact (simple range checks)

## Why These Bugs Were Intermittent

The segmentation faults were intermittent because they required specific timing:

1. **Mob must be hunting a target** - Not all mobs use the hunting system
2. **Target must be extracted during validation and use** - Requires precise timing
3. **Spec procs or triggers must fire** - `do_say()` or `perform_move()` must trigger extraction
4. **Level-up must occur with nearby players** - Viewer loop issue only manifests with witnesses
5. **Cache corruption** - Invalid room numbers must be passed to pathfinding

Different servers, load patterns, and game states would trigger these conditions at different rates.

## Files Modified

- `src/graph.c`:
  - Fixed `hunt_victim()` race condition (lines 1834-1879)
  - Added room validation to `get_cached_pathfind()` (lines 1401-1417)
  - Added room validation to `cache_pathfind_result_priority()` (lines 1427-1475)

- `src/mobact.c`:
  - Fixed `check_mob_level_up()` iteration pattern (lines 122-129)

## Testing

1. **Compilation**: Code compiles successfully with no warnings
2. **Code Formatting**: Formatted with `clang-format` according to project style
3. **Build System**: Successfully builds with both CMake and autotools
4. **Defensive Programming**: All changes follow defensive programming principles
5. **Pattern Consistency**: Changes match safe patterns used elsewhere in codebase

## Investigation of Other Potential Causes

### Experimental Auction System

**Finding**: The auction system (`CONFIG_NEW_AUCTION_SYSTEM`) is purely a feature flag that doesn't affect mobile_activity execution. It only controls whether certain auction commands are available to players.

**Evidence**:
- Auction code in `auction.c` and `act.auction.c` only checks the flag for command availability
- No direct interaction with mob AI or mobile_activity loop
- No memory management differences based on flag setting

**Conclusion**: Not a cause of the segfaults.

### Track System Issues

**Finding**: The track system itself is stable, but the hunting system that uses it had the race condition documented above.

**Evidence**:
- Track command (`do_track`) properly validates targets
- Pathfinding functions have proper bounds checking
- The issue was in `hunt_victim()` which uses the pathfinding system

**Conclusion**: Track/pathfinding system was safe; the issue was in its consumer (`hunt_victim()`).

## Prevention Guidelines

To prevent similar bugs in future development:

1. **Cache pointers that will be used multiple times** - Avoid repeated macro dereferences
2. **Re-validate after state-changing operations** - Especially after functions that can extract characters
3. **Use safe iteration patterns** - Always cache `next` pointer before operations on current item
4. **Validate parameters before use** - Check room numbers, pointers before accessing them
5. **Document race condition risks** - Note functions that can cause extraction
6. **Code review checklist**:
   - Look for: Pointer validated once, used multiple times
   - Look for: `for (p = list; p; p = p->next) { /* uses p without caching next */ }`
   - Look for: Functions called between validation and use

## Impact

These fixes prevent crashes when:
- Mobs are hunting targets that get extracted
- Mobs level up with nearby players who disconnect or move
- Pathfinding cache receives invalid room numbers
- Any character list iteration occurs during world state changes

The fixes are surgical and defensive, adding minimal overhead while significantly improving stability.
