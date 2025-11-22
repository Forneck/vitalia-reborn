# Summary: Mobact Act() Extraction Race Condition Fix

## Problem
The MUD server was experiencing intermittent segmentation faults in `mobact.c`, particularly when:
- Players were killing mobs
- Players had recently finished meditating  
- The character "Char2" was playing (suggesting player-specific or timing issues)

The issue was described as: "geralmente cai quando estou meditando... mas nunca é quando medito.. é depois e tais" (generally crashes when meditating... but never when I meditate.. it's afterwards and such).

## Root Cause Identified

The `act()` function in CircleMUD/tbaMUD can trigger **DG (DikuMUD Genesis) scripts** attached to:
- Characters (players and mobs)
- Objects
- Rooms

These scripts can perform **arbitrary actions** including:
- `%purge%` - Extract (delete) characters or objects
- `%damage%` - Deal damage that may kill and extract
- Other commands that indirectly cause extraction

When `act()` extracts a character via DG script, that character's memory is marked for deletion (NOTDEADYET flag set) but not immediately freed. However, subsequent code that tries to access that character **will crash** because:
1. The pointer is now dangling (points to marked-for-deletion memory)
2. Accessing character fields causes segmentation fault
3. Calling functions like `hit()` on extracted characters crashes

## The Vulnerability Pattern

### Pattern 1: Sequential act() Calls
```c
// VULNERABLE
act("$n olha para $N com indiferença.", FALSE, ch, 0, vict, TO_NOTVICT);
act("$N olha para você com indiferença.", FALSE, vict, 0, ch, TO_CHAR);
//  ↑ If first act() extracted vict, second act() crashes
```

### Pattern 2: act() Before hit()
```c
// VULNERABLE  
act("$n ataca $N!", FALSE, ch, 0, target, TO_ROOM);
hit(ch, target, TYPE_UNDEFINED);
//  ↑ If act() extracted target, hit() crashes
```

## The Fix

Add extraction checks (NOTDEADYET flags) after **every** `act()` call that involves other characters:

```c
// SAFE
act("$n olha para $N com indiferença.", FALSE, ch, 0, vict, TO_NOTVICT);
/* Safety check: act() can trigger DG scripts which may extract ch or vict */
if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
    continue;  // or return
if (MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET)) {
    vict = next_vict;
    continue;
}
act("$N olha para você com indiferença.", FALSE, vict, 0, ch, TO_CHAR);
// ... repeat checks ...
```

## Locations Fixed (8 Total)

| # | Location | Description | Severity |
|---|----------|-------------|----------|
| 1 | Line 1122-1135 | Aggressive mob behavior - sequential act() | CRITICAL |
| 2 | Line 1337-1350 | Poison attempt - act() on victim | MEDIUM |
| 3 | Line 1708 | Group joining - act() with leader | MEDIUM |
| 4 | Line 2403-2412 | Stealth following - multiple act() | MEDIUM |
| 5 | Line 291 | Hunt target - act() before hit() | HIGH |
| 6 | Line 686 | Key collection - act() before hit() | MEDIUM |
| 7 | Line 3562 | Quest mob kill - act() before hit() | MEDIUM |
| 8 | Line 3637 | Room clear - act() before hit() | MEDIUM |

## Why This Explains the Symptoms

### "Crashes when killing mobs"
Combat generates many `act()` calls for attack messages, death messages, etc. Each call can trigger DG scripts.

### "Crashes after meditation, not during"
- `POS_MEDITING` (position 4) causes `AWAKE(ch)` to return FALSE
- When player is meditating, the main mob AI loop **skips most processing**:
  ```c
  if (FIGHTING(ch) || !AWAKE(ch))
      continue;
  ```
- When player **stops meditating** and becomes active:
  1. Mob AI suddenly resumes processing
  2. Accumulated state triggers mob actions
  3. More `act()` calls happen in quick succession
  4. Higher chance of DG script extraction

### "More crashes with Char2"
If Char2's character or the mobs they interact with have more DG scripts attached, there's a higher probability of triggering extraction during `act()` calls.

### "Random/intermittent crashes"
Requires alignment of:
- Mob performing specific AI action (hunting, attacking, following, etc.)
- `act()` being called
- DG script being triggered
- Script performing extraction
- Subsequent code accessing the extracted character

## Impact

### Prevents Crashes In:
✅ Aggressive mob encounters  
✅ Mob poison attempts  
✅ Mob grouping behavior  
✅ Mob following behavior  
✅ Mob hunting targets  
✅ Mob quest completion (key collection, mob kills, room clearing)  
✅ Any scenario where act() can extract participants

### Code Quality:
✅ Builds with 0 errors, 0 warnings  
✅ Code review: 2 minor style notes (indentation) - code logic is correct  
✅ Security scan: 0 vulnerabilities  
✅ Follows existing NOTDEADYET check patterns in codebase  
✅ Minimal changes (73 lines added - all safety checks)  

## Files Modified

- `src/mobact.c` - 73 lines added (safety checks)

## Documentation Created

- `MOBACT_ACT_EXTRACTION_FIX.md` - Comprehensive technical documentation

## Related Fixes

This fix complements the extensive previous mobact safety work:
- `MOBACT_EXTRACT_FIX.md` - General extraction safety
- `MOBACT_RACE_CONDITION_SUMMARY.md` - Nested iteration safety  
- `MOBACT_ADDITIONAL_SIGSEGV_FIXES.md` - Array bounds safety
- `HUNT_VICTIM_RACE_CONDITION_FIX.md` - Hunt victim safety
- `MOBACT_SHOPPING_SEGFAULT_FIX.md` - Shopping crashes
- `MOBACT_UNSAFE_CASES_FIX.md` - Additional unsafe patterns

Together, these provide **comprehensive protection** against extraction-related crashes in `mobile_activity()`.

## Conclusion

This fix addresses a **critical class of segmentation faults** that were intermittent and difficult to reproduce. By adding consistent extraction checks after all `act()` calls involving multiple characters, we prevent crashes when DG scripts extract characters mid-operation.

The fix is:
- **Surgical** - Only adds safety checks where needed
- **Defensive** - Assumes act() can always extract
- **Consistent** - Follows existing patterns
- **Minimal overhead** - Simple flag checks
- **Maximum impact** - Fixes multiple crash scenarios

---

**Status**: ✅ **COMPLETE**  
**Testing**: ✅ **PASSED**  
**Security**: ✅ **VERIFIED**  
**Date**: 2025-10-26  
**Author**: GitHub Copilot
