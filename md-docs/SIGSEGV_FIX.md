# SIGSEGV Crash Fix Documentation

## Problem Summary

The game was experiencing segmentation faults (SIGSEGV) when:
1. Players moved between rooms
2. Players looked at rooms with invalid exits (to_room = -1/NOWHERE)
3. Players used the eavesdrop command on invalid exits

## Root Cause

The code was accessing the `world[]` array with invalid indices when room exits had `to_room` set to `-1` (NOWHERE). The NOWHERE constant is used to indicate that an exit doesn't lead anywhere, but the code wasn't checking for this before using it as an array index.

### Crash Locations

1. **act.informative.c:640** - `look_at_room()` function
   - When checking for nearby death rooms, the code didn't validate that `to_room != NOWHERE`
   - `ROOM_FLAGGED(EXIT(ch, i)->to_room, ROOM_DEATH)` would expand to `world[-1].room_flags`

2. **act.movement.c:154** - `do_simple_move()` function
   - Directly accessed `EXIT(ch, dir)->to_room` without validation
   - If EXIT was NULL or to_room was NOWHERE, would cause undefined behavior

3. **act.other.c:1634** - `do_eavesdrop()` function
   - Didn't check `to_room != NOWHERE` before accessing world array

## Solution

Added defensive checks before accessing room data through exits:

### Pattern Used
```c
// Old (unsafe):
if (EXIT(ch, i) && EXIT(ch, i)->to_room && ROOM_FLAGGED(...))

// New (safe):
if (EXIT(ch, i) && EXIT(ch, i)->to_room != NOWHERE && ROOM_FLAGGED(...))
```

### Changes Made

1. **act.informative.c:640**
   ```c
   // Added NOWHERE check before ROOM_FLAGGED
   if (EXIT(ch, i) && EXIT(ch, i)->to_room != NOWHERE && 
       ROOM_FLAGGED(EXIT(ch, i)->to_room, ROOM_DEATH))
   ```

2. **act.movement.c:154-162**
   ```c
   // Added validation at start of do_simple_move()
   if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE) {
       log1("SYSERR: do_simple_move: ch=%s attempting invalid move to dir=%d", 
            GET_NAME(ch), dir);
       return 0;
   }
   going_to = EXIT(ch, dir)->to_room;
   ```

3. **act.other.c:1633-1641**
   ```c
   // Added NOWHERE check with proper error message
   else if (EXIT(ch, dir)->to_room != NOWHERE) {
       target_room = EXIT(ch, dir)->to_room;
       // ... rest of code
   } else {
       send_to_char(ch, "Não há uma sala nessa direção...\r\n");
   }
   ```

## Testing

- ✅ Code compiles successfully
- ✅ Code formatted with clang-format
- ✅ CodeQL security analysis passed with 0 alerts
- ✅ Follows existing code patterns (e.g., mobact.c:1271)

## Impact

This fix prevents crashes when:
- World files have exits with invalid destinations (to_room = -1)
- Builders create incomplete exits during world editing
- Scripts or triggers create temporary invalid exits
- Players attempt to move through or interact with invalid exits

The fix is defensive and adds minimal overhead - just an additional comparison before array access.
