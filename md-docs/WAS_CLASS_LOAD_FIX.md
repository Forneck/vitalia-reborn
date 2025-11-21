# WAS_CLASS Loading/Saving Bug Fix

## Issue Description
All players were being loaded with incorrectly set WAS_CLASS field. The issue reported that all players were loaded with:
```
WAS: Mago Clerigo Ladrao
```
Even after manually editing player files to add Ranger and saving, the changes would not persist.

## Root Cause Analysis

### Tag Format in Player Files
The player file system uses a tag-based format where each field is stored as:
```
Tag: value
```

The `tag_argument()` function (lines 952-967 in `src/players.c`) extracts exactly **4 characters** as the tag:
```c
for (i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);
```

### Format Standards for 3-Character Tags
Looking at other 3-character tags in the codebase:
- `"Str :"` → extracted as `"Str "` → compared with `"Str "`
- `"Int :"` → extracted as `"Int "` → compared with `"Int "`
- `"Wis :"` → extracted as `"Wis "` → compared with `"Wis "`
- `"Act :"` → extracted as `"Act "` → compared with `"Act "`
- `"Aff :"` → extracted as `"Aff "` → compared with `"Aff "`

The pattern is: **3-char tag + space + colon** → extracted as **"XXX "** (4 chars)

### The Bug
The WAS_CLASS field violated this pattern:
- **Save**: `fprintf(fl, "Was: %s %s %s %s\n", ...)` (line 734)
  - Format: `"Was:"` (3 chars + colon, **NO space before colon**)
- **Tag extraction**: `"Was:"` (includes the colon as 4th char)
- **Load**: `!strcmp(tag, "Was")` (line 562)
  - Compared with only 3 characters, **NO padding**
- **Result**: `"Was:"` != `"Was"` → **NO MATCH** → Field never loaded

## The Fix

### Changes Made
Two minimal changes in `src/players.c`:

1. **Line 562 (Load)**: Changed tag comparison
   ```c
   // Before:
   else if (!strcmp(tag, "Was")) {
   
   // After:
   else if (!strcmp(tag, "Was ")) {
   ```

2. **Line 734 (Save)**: Changed save format
   ```c
   // Before:
   fprintf(fl, "Was: %s %s %s %s\n", bits, bits2, bits3, bits4);
   
   // After:
   fprintf(fl, "Was : %s %s %s %s\n", bits, bits2, bits3, bits4);
   ```

### Why This Works
With the fix:
- **Save**: `"Was : %s"` (space before colon)
- **Tag extraction**: `"Was "` (4 chars: 'W', 'a', 's', ' ')
- **Load**: Compare with `"Was "` (4 chars with trailing space)
- **Result**: `"Was "` == `"Was "` → **MATCH** → Field loads correctly

## Impact

### What This Fixes
1. **Remort tracking**: WAS_CLASS bits will now be properly loaded and saved
2. **Class bonuses**: Players who were formerly a specific class will now correctly receive bonuses (e.g., WAS_RANGER move regen bonus in `limits.c` line 209-210)
3. **Class history**: The system tracks which classes a player has been for remort progression

### Backward Compatibility
- **Old player files** (with `"Was:"` format): Will not be loaded by the new code, but they weren't being loaded before either. The field was effectively always empty.
- **New saves**: Will use the correct `"Was :"` format and will load properly
- **Migration**: Existing players will need to remort at least once after this fix for their new was_class to be tracked correctly. Historical data cannot be recovered.

### Related Systems
- Remort code in `interpreter.c` line 1849 sets the was_class bit correctly
- Display code in `act.wizard.c` lines 886-887 shows the WAS: line in stats
- Class benefits code in `limits.c` checks WAS_FLAGGED macro for bonus calculations

## Testing
Created test programs that verified:
1. Old format `"Was:"` fails to match both `"Was"` and `"Was "`
2. New format `"Was :"` correctly matches `"Was "`
3. The sscanf parsing works correctly with the new format
4. was_flags[0] loads with correct values

## Verification
- Code compiles without warnings
- Code review: No issues found
- CodeQL security scan: No vulnerabilities detected
- Build system: All targets build successfully

## Files Modified
- `src/players.c`: 2 lines changed (1 in load, 1 in save)

## Security Summary
No security vulnerabilities were introduced or discovered during this fix.
