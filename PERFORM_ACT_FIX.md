# PERFORM_ACT Segmentation Fault Fix

## Problem Summary

The game was experiencing segmentation faults (SIGSEGV) in the `perform_act()` function when:
1. Characters stopped following other characters
2. Gender-specific text codes ($x or $X) were used in act() messages
3. Specifically triggered by `stop_follower()` calling `act()` with messages like:
   - `"Você percebe que $N é $X(um,uma) estúpid$R!"`
   - `"$n percebe que $N é $X(um,uma) estúpid$R!"`

## Stack Trace

```
Program received signal SIGSEGV, Segmentation fault.
0x0000000000484ad5 in perform_act ()
#0  0x0000000000484ad5 in perform_act ()
#1  0x0000000000485695 in act ()
#2  0x0000000000592d2f in stop_follower ()
#3  0x0000000000593132 in die_follower ()
#4  0x00000000004e3650 in extract_char_final ()
#5  0x00000000004e3ad9 in extract_pending_chars ()
#6  0x000000000047e019 in heartbeat ()
#7  0x000000000047dd53 in game_loop ()
#8  0x000000000047ce7b in init_game ()
#9  0x000000000047c6a2 in main ()
```

## Root Cause

In `src/comm.c:perform_act()`, line 2692 declares:
```c
char *s;
```

This pointer was **never initialized**, yet it was dereferenced in two places:
- Line 2805 (case 'x'): `*s++ = *orig;`
- Line 2833 (case 'X'): `*s++ = *orig;`

### What are $x and $X?

These are gender-specific text codes used in the act() messaging system:
- `$x(male_text,female_text)` - uses character's gender
- `$X(male_text,female_text)` - uses victim's gender

Example: `$X(um,uma)` expands to "um" for males, "uma" for females.

### The Bug

When the code processed $x or $X:
1. It would set `i = ""` (empty string)
2. Parse the gender-specific format `(option1,option2)`
3. Try to write the selected text via `*s++ = *orig`
4. **CRASH** - `s` was uninitialized, pointing to random memory

## Solution

Initialize `s` to point to the current position in the output buffer (`buf`), then update `buf` after writing:

### Changes Made

**Case 'x' (line 2789-2816):**
```c
case 'x':
    i = "";
    s = buf;  // ← ADDED: Initialize s to current buffer position
    if (*(++orig) != '(') {
        log1("Illegal $x(...) code to act(): %s", origback);
        break;
    }
    // ... gender checking and parsing ...
    for (orig++; *orig && *orig != ',' && *orig != ')'; orig++)
        *s++ = *orig;  // Write selected text
    buf = s;  // ← ADDED: Update buf to new position
    // ... rest of parsing ...
```

**Case 'X' (line 2817-2844):**
```c
case 'X':
    i = "";
    s = buf;  // ← ADDED: Initialize s to current buffer position
    if (*(++orig) != '(') {
        log1("Illegal $X(...) code to act(): %s", origback);
        break;
    }
    // ... gender checking and parsing ...
    for (orig++; *orig && *orig != ',' && *orig != ')'; orig++)
        *s++ = *orig;  // Write selected text
    buf = s;  // ← ADDED: Update buf to new position
    // ... rest of parsing ...
```

## Code Changes Summary

- **File:** `src/comm.c`
- **Function:** `perform_act()`
- **Lines Added:** 4 (2 initializations + 2 buffer updates)
- **Lines Modified:** 0
- **Impact:** Surgical fix with minimal code changes

### Exact Changes:
1. Line 2791: Added `s = buf;`
2. Line 2806: Added `buf = s;`
3. Line 2819: Added `s = buf;`
4. Line 2834: Added `buf = s;`

## Why This Fix Works

1. **Initialization**: `s = buf` sets the pointer to the current output position
2. **Writing**: `*s++ = *orig` safely writes characters and advances the pointer
3. **Synchronization**: `buf = s` ensures the main buffer pointer stays in sync
4. **No side effects**: Since `i = ""`, the subsequent `while ((*buf = *(i++)))` loop does nothing, which is correct

## Testing Performed

✅ Code compiles successfully with CMake build system  
✅ Code formatted with clang-format  
✅ CodeQL security scan: **0 alerts**  
✅ Build completes with no new warnings  
✅ Fix addresses the exact crash location in stack trace  

## How to Reproduce (Before Fix)

1. Have a character follow another character
2. Cause the follower to stop following (die, unfollow, etc.)
3. The `stop_follower()` function calls `act()` with gender-specific messages
4. `perform_act()` processes the $X code
5. Dereferences uninitialized pointer `s`
6. **CRASH** - SIGSEGV

## Impact

This fix prevents crashes when:
- Characters stop following other characters (common game event)
- NPCs are dismissed or die while following
- Any act() message uses $x or $X gender codes
- Portuguese language gender-specific messages are displayed

## Related Code

The gender-specific codes are used throughout the codebase:

**utils.c (stop_follower):**
```c
act("Você percebe que $N é $X(um,uma) estúpid$R!", FALSE, ch, 0, ch->master, TO_CHAR);
act("$n percebe que $N é $X(um,uma) estúpid$R!", FALSE, ch, 0, ch->master, TO_NOTVICT);
```

**castle.c:**
```c
act("Você foi salv$r por $N, $X(seu,sua) leal amig$R!", FALSE, ch_victim, 0, ch_hero, TO_CHAR);
```

**spell_parser.c:**
```c
send_to_char(ch, "Você tem medo de machucar $X(seu mestre,sua mestra)!\r\n");
```

## Historical Context

This appears to be a long-standing bug in the gender-specific text implementation, likely dating back to when the $x/$X codes were added for Portuguese language support. The uninitialized pointer went undetected because:

1. The code path is only triggered with specific message formats
2. Memory layout differences between systems affected crash frequency
3. Compiler optimizations could mask or expose the bug
4. Not all act() calls use gender-specific codes

## Technical Notes

### Why wasn't this caught earlier?

- **Conditional execution**: Only crashes when $x or $X codes are used
- **Language-specific**: More common in Portuguese messages
- **Intermittent**: Memory layout affects whether uninitialized pointer causes immediate crash
- **Compiler differences**: Some compilers might zero-initialize stack variables

### Prevention

This type of bug is preventable with:
- Compiler warnings: `-Wuninitialized` (though may not catch all cases)
- Static analysis: Tools like CodeQL, cppcheck
- Code review: Always initialize pointers before use
- Testing: Exercise all code paths, especially language-specific features

## See Also

- `SEGFAULT_FIX_SUMMARY.txt` - Other segfault fixes
- `SIGSEGV_FIX.md` - Room exit validation fixes
- `MOBILE_ACTIVITY_FIX.md` - Iterator null checks
- `src/comm.c:perform_act()` - The fixed function
- `src/utils.c:stop_follower()` - Trigger location
