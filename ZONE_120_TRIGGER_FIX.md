# Zone 120 Trigger Fix Summary

## Issue Report
Room 12017 in zone 120 (The Coliseum of Rome) was generating trigger errors:
- `[ Room 12017 :: wdamage: target not found ]`
- `[ Room 12017 :: no target found for wsend ]`

## Root Cause Analysis

### Problem
Trigger #12000 (Armadilha Mortal Leões) was using mob-trigger syntax in a room trigger:
```
%damage% %actor% %stunned%
%send% %actor% Os leões ficam entediados...
```

### Technical Details
In DG Scripts, there are two different syntaxes for damage and send commands:

**Mob/Object Triggers:**
- Use `%damage%`, `%send%`, `%echoaround%` directly with variables
- `%actor%` is automatically resolved as a character pointer
- Example: `%damage% %actor% 10`

**Room Triggers:**
- Use `wdamage`, `wsend`, `wechoaround` commands
- Require explicit character NAME as first argument (not a variable)
- Character is looked up via `get_char_by_room(room, name)`
- Example: `wdamage %actor.name% 10`

### Code Reference
From `src/dg_wldcmd.c`:

```c
// Line 532-536: wdamage implementation
ch = get_char_by_room(room, name);
if (!ch) {
    wld_log(room, "wdamage: target not found");
    return;
}

// Line 149-157: wsend implementation
if ((ch = get_char_by_room(room, buf))) {
    if (subcmd == SCMD_WSEND)
        sub_write(msg, ch, TRUE, TO_CHAR);
} else
    wld_log(room, "no target found for wsend");
```

## Solution

### Changes Made
**File:** `lib/world/trg/120.trg`

**Before:**
```
wait 1 sec
eval stunned %actor.hitp% - 1
%damage% %actor% %stunned%
wait 5 sec
%send% %actor% Os leões ficam entediados quando você para de se debater e te deixam para morrer.
```

**After:**
```
wait 1 sec
eval stunned %actor.hitp% - 1
wdamage %actor.name% %stunned%
wait 5 sec
wsend %actor.name% Os leões ficam entediados quando você para de se debater e te deixam para morrer.
```

### Key Changes
1. Line 8: `%damage% %actor% %stunned%` → `wdamage %actor.name% %stunned%`
2. Line 10: `%send% %actor%` → `wsend %actor.name%`

## Trigger Information

### Trigger #12000 Details
- **Name:** Armadilha Mortal Leões - 12017
- **Type:** Room trigger (type 2)
- **Activation:** Greet (g) - fires when someone enters the room
- **Probability:** 100% chance
- **Attached to:** Room 12017 (A Cova do Leão - The Lion's Pit)
- **Purpose:** Near-death trap that stuns the player when entering the lion pit

### Trigger Behavior
1. Player enters room 12017 (lion pit)
2. Wait 1 second
3. Calculate damage (player's HP - 1, leaving them at 1 HP)
4. Apply damage to player
5. Wait 5 seconds
6. Send message: "Os leões ficam entediados quando você para de se debater e te deixam para morrer."
   (Translation: "The lions get bored when you stop struggling and leave you to die.")

## Testing
Build completed successfully:
```bash
cd /home/runner/work/vitalia-reborn/vitalia-reborn
./configure
cd src && make -j4
# Result: ../bin/circle executable created successfully
```

## Prevention Guidelines

### For Future Trigger Development

**Room Triggers (Type 2):**
- Use `wdamage <char_name> <amount>`
- Use `wsend <char_name> <message>`
- Use `wechoaround <char_name> <message>`
- Always specify `%actor.name%` or other explicit character name

**Mob Triggers (Type 0):**
- Use `%damage% %actor% <amount>`
- Use `%send% %actor% <message>`
- Use `%echoaround% %actor% <message>`
- Variables like `%actor%` work directly

**Object Triggers (Type 1):**
- Same syntax as mob triggers
- Use `%damage% %actor% <amount>`
- Use `%send% %actor% <message>`

### Quick Reference
| Trigger Type | Damage Command | Send Command | Target Syntax |
|--------------|----------------|--------------|---------------|
| Room (2)     | `wdamage`      | `wsend`      | `%actor.name%` |
| Mob (0)      | `%damage%`     | `%send%`     | `%actor%` |
| Object (1)   | `%damage%`     | `%send%`     | `%actor%` |

## Related Files
- **Trigger file:** `lib/world/trg/120.trg`
- **Room file:** `lib/world/wld/120.wld` (room 12017)
- **Source code:** `src/dg_wldcmd.c` (room trigger command implementations)
- **Documentation:** `lib/text/help/help.hlp` (WDAMAGE, WSEND entries)

## References
- CircleMUD DG Scripts documentation
- tbaMUD trigger documentation in `tbadoc/` directory
- Similar working examples in other zone triggers (e.g., zone 95, zone 98)

## Additional Notes
- The trigger correctly uses `eval` for arithmetic operations (line 7)
- This follows the established pattern where room triggers need explicit character lookups
- The fix maintains game balance by keeping the near-death trap functionality intact
- Portuguese language strings preserved as per codebase conventions
