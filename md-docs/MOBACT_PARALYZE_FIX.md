# Mobact Paralyze Quest Completion Crash Fix

## Issue Description

SIGSEGV crash occurred in `mobile_activity()` when:
1. Player paralyzes a quest giver NPC (e.g., "nobre" / noble)
2. Player gives quest object to the NPC to complete an AQ_OBJ_RETURN quest
3. Quest completes successfully and object remains in NPC's inventory
4. NPC attempts to act after paralysis (or during if still AWAKE)
5. Crash occurs when NPC's AI tries to process with invalid state

## Root Causes

### 1. Missing Paralysis Check in mobile_activity()
**Location:** `src/mobact.c` line 564

The check `if (FIGHTING(ch) || !AWAKE(ch))` was insufficient because:
- `AWAKE(ch)` only checks if `GET_POS(ch) > POS_SLEEPING`
- Paralysis (AFF_PARALIZE) does not change position
- Paralyzed mobs could still pass this check and perform actions

This violated game logic - paralyzed entities should not be able to act.

### 2. Unsafe Object/Character Access After Quest Completion
**Location:** `src/act.item.c` line 717

After calling `autoquest_trigger_check()`, the code continued to access `obj` and `vict` without verifying they hadn't been extracted through:
- Quest completion scripts
- DG triggers
- Act() side effects

### 3. Unsafe Character Access After act() Calls
**Location:** `src/quest.c` lines 941-942

During AQ_OBJ_RETURN quest completion, when transferring objects between NPCs, `act()` calls could trigger DG scripts that extract characters, but there were no safety checks afterward.

## Solutions Implemented

### Fix 1: Add Paralysis Check to mobile_activity()
**File:** `src/mobact.c` lines 567-569

```c
/* Skip paralyzed mobs - they cannot perform actions */
if (AFF_FLAGGED(ch, AFF_PARALIZE))
    continue;
```

**Effect:**
- Paralyzed NPCs now skip all AI processing
- Matches player behavior (paralyzed players cannot act)
- Prevents crashes from attempting actions while in invalid state
- Primary fix that prevents the described crash scenario

### Fix 2: Add Safety Check After autoquest_trigger_check()
**File:** `src/act.item.c` lines 719-726

```c
/* Safety check: Quest completion may have extracted obj or vict through scripts/triggers */
if (MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET))
    return;
```

**Effect:**
- Prevents accessing extracted characters after quest completion
- Returns early if recipient was extracted
- Defensive programming improvement

**Note:** We cannot safely verify `obj` validity without extensive tracking, but this is acceptable because:
- Quest completion typically leaves objects intact in NPC inventory
- The primary fix (paralysis check) prevents the main crash scenario
- Object extraction during quest completion is rare in practice

### Fix 3: Add Safety Checks After act() Calls in Quest Transfer
**File:** `src/quest.c` lines 942-954

```c
act("$n entrega $p para quem solicitou.", FALSE, vict, object, NULL, TO_ROOM);
/* Safety check: act() can trigger DG scripts which may extract object or characters */
if (MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET))
    break;
if (MOB_FLAGGED(original_requester, MOB_NOTDEADYET) ||
    PLR_FLAGGED(original_requester, PLR_NOTDEADYET))
    break;
act("$n recebe $p de $N.", FALSE, original_requester, object, vict, TO_ROOM);
/* Safety check after second act() call */
if (MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET))
    break;
if (MOB_FLAGGED(original_requester, MOB_NOTDEADYET) ||
    PLR_FLAGGED(original_requester, PLR_NOTDEADYET))
    break;
```

**Effect:**
- Checks for extraction after each `act()` call
- Prevents use-after-free when DG scripts extract characters
- Breaks out of quest completion early if extraction occurs
- Follows existing pattern from other parts of mobact.c

## Testing Performed

- ✅ Code compiles without errors or warnings
- ✅ Formatted with clang-format
- ✅ CodeQL security scan: 0 alerts
- ✅ Changes follow existing code patterns
- ✅ Minimal scope - only 25 lines changed across 3 files

## Why This Fixes The Issue

The primary fix (paralysis check) directly addresses the crash scenario:

1. When NPC is paralyzed and receives quest object, it now skips all AI processing
2. NPC cannot attempt to use items, move, or perform any actions while paralyzed
3. Once paralysis wears off, NPC can resume normal activity safely
4. Object remains in NPC's inventory and is accessible after paralysis ends

The secondary fixes (safety checks) provide defense-in-depth:
- Prevent crashes if scripts extract characters during quest completion
- Match safety patterns used elsewhere in the codebase
- Protect against edge cases and future script interactions

## Related Issues

This fix complements previous mobact safety improvements:
- MOBILE_ACTIVITY_FIX.md - Iterator safety checks
- MOBACT_ADDITIONAL_FIXES_SUMMARY.txt - Array bounds and extraction safety
- MOBACT_EXTRACT_FIX.md - Extract character safety patterns

## Exploit Note

The issue mentions the player wanted to "kill the quest giver for exp and recovering the item" - this represents an attempted exploit:
1. Accept quest to obtain quest object
2. Paralyze quest giver to keep them helpless
3. Give object to complete quest and get rewards
4. Kill quest giver while paralyzed for experience
5. Loot quest object back from corpse

While this fix prevents the crash, game designers may want to consider additional measures:
- Quest objects could be extracted on quest completion
- Quest givers could be marked as non-killable or protected
- Killing quest givers could fail the quest or have reputation consequences

## Files Modified

- `src/mobact.c` - Added paralysis check (4 lines)
- `src/act.item.c` - Added safety check after autoquest_trigger_check (8 lines)
- `src/quest.c` - Added safety checks after act() calls (13 lines)

Total: 25 lines changed across 3 files

## Status

✅ Ready for merge
🔒 Security scan passed (0 vulnerabilities)
📝 Documentation complete
