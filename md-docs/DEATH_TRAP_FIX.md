# Death Trap Fix Documentation

## Issue Summary

Death traps (rooms flagged with ROOM_DEATH) were not properly killing mobs when they entered, and players were not being returned to the menu and their hometown loadroom after dying.

## Root Cause

The issue was introduced as a side effect of the recent segmentation fault fix in `mobile_activity()`. While the segfault fix correctly added null checks to prevent crashes, it did not account for characters that had been marked for extraction.

When a character (mob or player) enters a death trap:
1. The `do_simple_move` function (act.movement.c:367-374) correctly detects the ROOM_DEATH flag
2. It calls `death_cry(ch)` to notify nearby rooms
3. It calls `extract_char(ch)` which marks the character for extraction by setting the `MOB_NOTDEADYET` or `PLR_NOTDEADYET` flag
4. The actual extraction happens later in `extract_pending_chars()`

However, the `mobile_activity()` function in mobact.c was continuing to process mobs even after they had been marked for extraction. This could cause:
- Mobs to perform actions after entering a death trap
- Potential access to freed memory
- Death trap mechanics not working as intended

## Solution

Added a check for the `MOB_NOTDEADYET` and `PLR_NOTDEADYET` flags in the `mobile_activity()` function at line 145 in src/mobact.c:

```c
/* Skip mobs that have been marked for extraction (e.g., from death traps) */
if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
    continue;
```

This check is placed early in the loop, right after the basic validity checks, ensuring that:
1. Mobs that enter death traps are immediately skipped from further processing
2. Players are also protected if they somehow enter the mob activity loop
3. The pattern matches existing code in other parts of the codebase (fight.c, quest.c, interpreter.c)

## Changes Made

- **File:** src/mobact.c
- **Lines added:** 4 (3 lines of code + 1 comment)
- **Build status:** Clean compilation with no warnings or errors
- **Security scan:** 0 CodeQL alerts
- **Code formatting:** Applied clang-format

## How Death Traps Work

1. **Player enters death trap:**
   - `perform_move` → `do_simple_move` detects ROOM_DEATH flag
   - Logs the death to immortals
   - Increments player's death trap counter (GET_DTS)
   - Calls `death_cry()` to notify nearby rooms
   - Calls `extract_char()` which sets PLR_NOTDEADYET flag
   - Later in game loop, `extract_pending_chars()` calls `extract_char_final()`
   - `extract_char_final()` sets player state to CON_MENU
   - Player sees the main menu and can select character or quit

2. **Mob enters death trap:**
   - Same process as player
   - `extract_char()` sets MOB_NOTDEADYET flag
   - Mob is now skipped by `mobile_activity()` (thanks to our fix)
   - `extract_pending_chars()` removes mob from game
   - Mob count for that prototype is decremented

## Testing

The fix can be tested by:
1. Creating or finding a room with ROOM_DEATH flag set (bit 0 in room flags)
2. Having a mob wander into the room (or forcing it there with immortal commands)
3. Verifying the mob disappears and the death cry message is shown
4. Checking that no errors appear in the syslog
5. For players: entering the death trap and verifying return to menu

## Related Files

- **src/act.movement.c** (lines 367-374): Death trap detection and extraction
- **src/handler.c** (lines 1043-1094): Character extraction functions
- **src/fight.c** (line 278): death_cry() implementation
- **src/mobact.c** (line 145): Fixed - now skips extracted characters

## References

- SEGFAULT_FIX_SUMMARY.txt: Previous fix that inadvertently affected death traps
- MOBILE_ACTIVITY_FIX.md: Details on the segfault fix in mobile_activity()
