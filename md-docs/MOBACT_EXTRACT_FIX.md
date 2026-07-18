# Mobile Activity Extract Character Safety Checks

## Problem Summary

The `mobile_activity()` function in `mobact.c` processes all mobs in the game world each tick. The issue reported that the function needed additional safety checks to prevent crashes caused by:

1. **Indirect character extraction**: Functions called within the main loop (like `hit()`, `call_ACMD()`, etc.) can indirectly cause `extract_char(ch)` to be called, which marks the character for extraction by setting the `MOB_NOTDEADYET` or `PLR_NOTDEADYET` flag.
2. **Pointer corruption**: If a character is extracted during the loop iteration, the loop's `next_ch` pointer could become invalid, leading to crashes or undefined behavior.

## Root Cause Analysis

### How extract_char() Works

When `extract_char(ch)` is called, it doesn't immediately remove the character from memory. Instead, it:
1. Sets the `MOB_NOTDEADYET` or `PLR_NOTDEADYET` flag
2. Increments the `extractions_pending` counter
3. The actual extraction happens later in `extract_pending_chars()` called after the main loop

This deferred extraction pattern means that **any function that leads to character death or removal must be followed by a safety check** in the main loop.

### Extraction Paths Identified

The analysis found these critical paths where `extract_char()` can be triggered:

#### 1. Combat Path: hit() → die() → extract_char()
```
hit(ch, victim, TYPE_UNDEFINED)
  └─> damage(ch, victim, dam, ...)
      └─> die(victim)
          └─> death_cry(victim)
              └─> extract_char(victim)
```

**When this happens:**
- Combat damage kills a character
- Character dies in a fight
- **Important**: The victim is extracted, but if the victim is `ch` (self), then `ch` is marked for extraction

#### 2. Movement Path: mob_goal_oriented_roam() → death trap
```
mob_goal_oriented_roam(ch, target_room)
  └─> perform_move_IA(ch, direction, ...)
      └─> perform_move(ch, direction, ...)
          └─> do_simple_move(ch, direction, ...)
              └─> [if ROOM_DEATH] extract_char(ch)
```

**When this happens:**
- Mob moves into a death trap room
- Mob triggers a trap in movement
- `ch` itself is extracted

#### 3. Command Path: call_ACMD() → scripts/triggers
```
call_ACMD(do_mine, ch, "", 0, 0)
  └─> [potentially] trigger mob scripts
      └─> [potentially] extract_char(ch)
```

**When this happens:**
- Resource gathering commands might trigger world scripts
- Scripts could potentially cause character extraction
- Less common but theoretically possible

## Solution: Comprehensive Safety Checks

Added safety checks after **every operation** that could indirectly call `extract_char()`:

### 1. After hit() Calls (8 locations)

**Pattern used:**
```c
hit(ch, target, TYPE_UNDEFINED);
/* Safety check: hit() can indirectly cause extract_char */
if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
    continue;
```

**Locations:**
1. **Line 261**: GOAL_HUNT_TARGET - Mob attacking its hunt target
2. **Line 564**: GOAL_COLLECT_KEY - Mob attacking another mob to get a key
3. **Line 870**: Aggressive mob attacking a victim
4. **Line 932**: MOB_MEMORY - Mob attacking a remembered enemy
5. **Line 951**: Charmed mob rebellion - Attacking its master
6. **Line 1944**: mob_assist_allies() - Helping a group member in combat
7. **Line 2953**: Quest completion - AQ_MOB_KILL/AQ_MOB_KILL_BOUNTY
8. **Line 3020**: Quest completion - AQ_ROOM_CLEAR (clearing hostile mobs)

### 2. After call_ACMD() Calls (1 location)

**Pattern used:**
```c
call_ACMD(do_mine, ch, "", 0, 0);
/* Safety check: call_ACMD functions might indirectly trigger extract_char
 * through complex chains (e.g., triggering scripts, special procedures, etc.) */
if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
    continue;
```

**Location:**
- **Lines 329-340**: Resource gathering (do_mine, do_fishing, do_forage, do_eavesdrop)

**Why this is needed:**
Although these commands don't directly call `extract_char()`, they could potentially trigger:
- World scripts via DG Scripts system
- Special procedures
- Custom triggers

### 3. After mob_goal_oriented_roam() Calls (3 locations)

**Pattern used:**
```c
mob_goal_oriented_roam(ch, dest);
/* Safety check: mob_goal_oriented_roam uses perform_move which can trigger death traps */
if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
    continue;
```

**Locations:**
1. **Line 602**: Goal-oriented movement toward a destination
2. **Line 896**: Random roaming behavior
3. **Line 913**: After mob_try_to_sell_junk() which can internally call mob_goal_oriented_roam()

**Why this is critical:**
Movement functions can trigger death traps directly:
- `ROOM_DEATH` flag causes immediate extraction
- Certain sector types (lava, quicksand) can kill
- Frozen rooms or other environmental hazards

### 4. Existing Safety Checks (Already Present)

The code already had these safety checks in place:

1. **Line 145**: Skip mobs marked NOTDEADYET at loop start
2. **Line 171**: Check after spec_proc execution
3. **Line 638**: Check after hunt_victim()
4. **Line 880**: Check after aggressive mob combat
5. **Line 940**: Check after MOB_MEMORY combat
6. **Line 957**: Check after charmed mob rebellion

## Why This Prevents Crashes

### The Iterator Bug Pattern

Without safety checks, this crash scenario occurs:

```c
for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;
    
    hit(ch, victim, TYPE_UNDEFINED);  // ← Can mark ch for extraction
    
    // If ch is extracted, ch->next might be corrupted
    // next_ch points to garbage memory
    // Next iteration: CRASH when accessing ch->ai_data, etc.
}
```

### How Safety Checks Fix It

With safety checks:

```c
for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;
    
    hit(ch, victim, TYPE_UNDEFINED);
    
    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
        continue;  // ← Skip to next iteration BEFORE accessing ch
    
    // Safe to access ch here
    ch->ai_data->...
}
```

The `continue` statement immediately advances to the next character **before** any further access to `ch`, preventing access to potentially corrupted memory.

## Pointer Corruption Investigation

The issue also mentioned potential pointer corruption bugs:

### 1. next_ch Corruption

**Analysis:** The `next_ch = ch->next` pattern at the start of each iteration is safe because:
- It's executed BEFORE any operations that could extract `ch`
- The safety checks use `continue` to skip further processing
- The `next` pointer is saved before any dangerous operations

**Conclusion:** No corruption possible with current implementation.

### 2. ch->master Corruption

**Analysis:** The `ch->master` pointer could become invalid if:
- The master character is extracted during the loop
- However, masters are typically players or other mobs in the character_list
- The deferred extraction pattern means the master pointer remains valid until after the entire loop

**Additional safety:** The code already checks master validity before dereferencing:
```c
if (ch->master && AFF_FLAGGED(ch, AFF_CHARM) && ...)
```

**Conclusion:** No additional fixes needed for master pointer.

### 3. vict Pointer Corruption

**Analysis:** The victim iteration loops use the safe pattern:
```c
for (vict = world[IN_ROOM(ch)].people; vict && !found;) {
    if (!vict) {
        break;  // Early safety check
    }
    struct char_data *next_vict = vict->next_in_room;
    // ... operations ...
    vict = next_vict;
}
```

**Conclusion:** Already safe due to existing fixes in MOBILE_ACTIVITY_FIX.md.

## Files Modified

- `src/mobact.c` - Added 12 new safety checks after potentially dangerous operations

## Testing

1. **Compilation**: Code compiles successfully with both CMake and autotools with no errors
2. **Code Formatting**: Formatted with clang-format according to project style
3. **Security Analysis**: CodeQL found 0 security issues
4. **Build Systems**:
   - ✅ CMake build successful
   - ✅ Autotools build successful

## Prevention Guidelines

To prevent similar bugs in the future when adding new AI behaviors:

1. **After any hit() call**: Always add safety check
2. **After any movement**: Check for extraction (death traps)
3. **After ACMD calls**: Check if they could trigger scripts
4. **After any function that modifies world state**: Consider if extraction is possible
5. **Use the pattern**: 
   ```c
   dangerous_operation();
   if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
       continue;
   ```

## Impact

This fix prevents crashes when:
- Mobs die in combat during their AI processing
- Mobs move into death traps while pursuing goals
- Mobs trigger environmental hazards
- World scripts or triggers cause character extraction
- Any indirect path to `extract_char()` is triggered during mob AI processing

The fix is defensive and adds minimal overhead - just a flag check after operations that could cause extraction.

## References

- Original issue: "Mobact.c - Additional extraction points and pointer corruption"
- Related fix: MOBILE_ACTIVITY_FIX.md (iterator null pointer bugs)
- Related fix: SIGSEGV_FIX.md (room validation issues)
- Stack trace would show: `mobile_activity()` in the call stack when crashes occur
