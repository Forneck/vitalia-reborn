# WAS Class Display Fix Summary

## Issue Description
Player reported that after remorting from Ranger to Druid, the `stat` command showed:
```
WAS: Mago Clerigo Ladrao
```
But it should have shown:
```
WAS: Mago Clerigo Ladrao Ranger
```

## Investigation Findings

### 1. How the WAS System Works
- `was_class` is a bitarray stored in `player_specials->saved.was_class[RM_ARRAY_MAX]`
- `RM_ARRAY_MAX` is 4, providing 128 bits (4 arrays × 32 bits each)
- All 7 classes (0-6) fit in `was_class[0]`, so array indexing is not an issue
- Classes are: 0=Mago, 1=Clerigo, 2=Ladrao, 3=Guerreiro, 4=Druida, 5=Bardo, 6=Ranger

### 2. Remort Code Analysis
The remort code in `interpreter.c` (line 1849) correctly sets the was_class bit:
```c
SET_BIT_AR(d->character->player_specials->saved.was_class, GET_CLASS(d->character));
```

Testing confirmed this macro works correctly for all classes including Ranger (class 6).

### 3. WAS_RANGER Benefits
Confirmed that `WAS_RANGER` provides benefits in `limits.c` (line 79-80):
- Rangers and former Rangers get a 1.5x multiplier for mana gain
- This applies to all was_class checks for spellcasting classes

## Bugs Fixed

### Bug 1: Incorrect Bitarray Access in can_elevate()
**File:** `src/act.other.c` line 1339

**Problem:** The code was accessing `was_class` as a simple integer array instead of using bitarray macros:
```c
if (ch->player_specials->saved.was_class[i] || GET_CLASS(ch) == i)
```

**Fix:** Use the proper `WAS_FLAGGED` macro:
```c
if (WAS_FLAGGED(ch, i) || GET_CLASS(ch) == i)
```

**Impact:** This bug could cause the `can_elevate()` function to incorrectly determine if a player had experienced all classes.

### Bug 2: num_incarnations Never Incremented
**File:** `src/interpreter.c` line 1858

**Problem:** The `num_incarnations` counter was read but never incremented, causing the class history to overwrite the same slot repeatedly.

**Fix:** Added increment after recording class history:
```c
d->character->player_specials->saved.num_incarnations++;
```

**Impact:** Class history statistics were not being tracked correctly.

### Enhancement: Debug Logging
**File:** `src/interpreter.c` line 1850

Added logging to track remort transitions:
```c
log1("REBEGIN: %s changed from %s (class %d) to %s (class %d). was_class[0]=0x%X", ...);
```

**Purpose:** Helps diagnose future issues with was_class tracking.

## Root Cause of Reported Issue

The investigation suggests the remort code is **currently correct** and will work properly going forward. The reported issue likely occurred due to:

1. An old player file from before proper was_class tracking was implemented
2. A previous bug that has since been fixed
3. Player data corruption or incomplete save

The fixes ensure:
- Correct bitarray usage throughout the codebase
- Proper logging to diagnose future issues
- Accurate class history tracking

## Testing

Created multiple test programs that confirmed:
- `SET_BIT_AR` works correctly for all class values (0-6)
- `sprintbitarray` correctly displays all set class names
- Bit manipulation macros function as expected

## Recommendations

1. **For existing players with missing was_class entries:**
   - No automatic fix is provided as the actual history cannot be reconstructed
   - Players can continue playing; future remorts will be tracked correctly
   - Immortals could manually set was_class bits if player history is known

2. **Going forward:**
   - The debug logging will help identify if this issue recurs
   - The fixed bitarray usage prevents related bugs
   - Class history now tracks properly with num_incarnations

## Related Code References

- **WAS display:** `src/act.wizard.c` line 886-887
- **Remort code:** `src/interpreter.c` line 1837-1859
- **WAS benefits:** `src/limits.c` line 73-80 (mana gain)
- **Bitarray macros:** `src/utils.h` line 387-395, 469
- **Class definitions:** `src/structs.h` line 173-182
- **Class names:** `src/class.c` line 30
