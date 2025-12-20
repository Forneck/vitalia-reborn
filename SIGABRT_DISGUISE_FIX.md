# SIGABRT / User Command Descriptor Corruption Fix

## Issue Summary

**Symptom**: Program crashed with SIGABRT during `calloc()` in `new_descriptor()`, and the users command displayed corrupted descriptor numbers (e.g., `15342574698436` instead of 1-999).

**Trigger**: Player quitting while in disguise.

## Root Cause Analysis

The disguise system (SKILL_DISGUISE) allows thieves to disguise themselves as NPCs by using mob corpses. When disguised:
1. The player's original descriptions (short_descr, long_descr, description) are saved in a global `disguise_list`
2. The player's descriptions are replaced with the mob's descriptions (via `strdup()`)
3. The AFF_DISGUISE flag is set

**The Bug**: When a player quit while disguised:
1. The character was saved WITH the disguise state (AFF_DISGUISE flag + mob descriptions)
2. The `disguise_list` entry (containing original descriptions) was freed during cleanup
3. On next login, the player had AFF_DISGUISE but NO corresponding `disguise_list` entry
4. This stale state could lead to:
   - Memory corruption when trying to restore/access descriptions
   - Heap corruption from double-free or use-after-free scenarios
   - SIGABRT during subsequent allocations (heap metadata corruption)
   - Corrupted descriptor numbers displayed in users command

## Solution

Automatically remove disguise before the character is saved, preventing stale state:

### 1. Normal Quit (do_quit)
When a player types `quit`:
- Check if player has AFF_DISGUISE
- If disguised, show message: "Seu disfarce se desfaz enquanto você se prepara para partir..."
- Call `remove_disguise()` to properly restore original descriptions
- Character is saved without disguise state

### 2. Linkdeath/Extract (extract_char)  
When a player loses connection or is extracted:
- Check if player has AFF_DISGUISE
- If disguised, silently remove the disguise affect
- Manually free and NULL disguise descriptions
- Clean up `disguise_list` entry via `cleanup_disguise_data()`
- Character is saved without disguise state

### 3. Defensive Login Check (enter_player_game)
When a player logs in:
- Check if player has AFF_DISGUISE flag
- Use `has_disguise_data()` to verify if corresponding `disguise_list` entry exists
- If no entry found (stale state from previous session):
  - Log warning for immortals
  - Remove AFF_DISGUISE affect
  - Free and NULL any stale disguise descriptions
  - This prevents crashes from stale state

### 4. Additional Safety Measures
- **ProtocolDestroy**: Added NULL pointer check to prevent crashes
- **do_users**: Added validation to skip descriptors with invalid desc_num (< 0 or > MAX_DESC_NUM)
- **MAX_DESC_NUM constant**: Defined as 999 for consistent validation

## Files Modified

1. **src/protocol.c**
   - Added NULL check in `ProtocolDestroy()`

2. **src/comm.h**
   - Added `MAX_DESC_NUM` constant (999)

3. **src/act.informative.c**  
   - Added descriptor validation in `do_users` command

4. **src/act.other.c**
   - Modified `do_quit` to remove disguise before quitting
   - Added `has_disguise_data()` helper function
   - Made `restore_original_descriptions()` non-static for use in cleanup paths

5. **src/handler.c**
   - Modified `extract_char_final` (character cleanup/saving) to remove disguise before saving

6. **src/interpreter.c**
   - Modified `enter_player_game` to clean up stale disguise state

7. **src/act.h**
   - Added `has_disguise_data()` function declaration
   - Added `restore_original_descriptions()` function declaration

## Testing

### Build Testing
- Clean build completed successfully
- No compilation errors or warnings
- All object files linked correctly

### Code Review
- Addressed feedback about using constants instead of magic numbers
- Added explanatory comments for cleanup logic
- Ensured consistent error handling

### Security Scan
- CodeQL analysis: **0 vulnerabilities found**
- No heap corruption risks from changes
- Proper memory management verified

## Impact

### Before Fix
- Rare but critical crashes when players quit while disguised
- Heap corruption causing SIGABRT in random allocations
- Corrupted descriptor numbers in users command
- Unpredictable behavior on subsequent logins

### After Fix
- Disguise automatically removed before quit with user-friendly message
- No stale state saved to player files
- Defensive checks prevent issues from legacy save files
- Clean heap management prevents corruption
- Stable descriptor tracking

## Technical Notes

### Descriptor Number Cycling
- Descriptor numbers cycle from 1 to 999 (when reaching 1000, resets to 1)
- `MAX_DESC_NUM` constant ensures consistent validation
- Corrupted values (like `15342574698436`) indicate memory corruption

### Disguise System Flow
```
Normal Flow:
1. Player disguises -> saves originals in disguise_list, replaces with mob descriptions
2. Player undisguises -> restores originals from disguise_list, frees mob descriptions
3. disguise_list entry is cleaned up

Fixed Flow (Quit While Disguised):
1. Player disguises -> saves originals, replaces with mob descriptions
2. Player quits -> removes disguise, restores originals, saves clean state
3. disguise_list entry is cleaned up
4. Next login -> normal state, no stale data
```

### Memory Management
- Original descriptions: Stored in `disguise_list`, freed on undisguise or cleanup
- Disguise descriptions: Allocated via `strdup()`, freed when removed
- Character descriptions after fix: Always clean state (original or NULL)

## Prevention

To prevent similar issues in the future:
1. Always clean up temporary state before saving characters
2. Add defensive checks on login for any stateful systems
3. Use helper functions to encapsulate state checking
4. Document state lifecycle and cleanup responsibilities
5. Test edge cases like logout during special states

## References

- Issue: "Sigabrt / user command" - descriptor numbers corrupted
- Related: Player quit while in disguise
- System: SKILL_DISGUISE (Thief macabre disguise skill)
- Code locations: act.other.c (disguise), handler.c (extract), interpreter.c (login)
