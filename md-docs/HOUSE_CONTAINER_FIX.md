# House Container Bug Fix

## Issue
**Original Problem (Issue: "Casas bug")**: When the MUD crashes (SIGSEGV), items saved inside containers in houses are incorrectly placed directly in the room instead of remaining inside their containers upon server restart.

**Impact**: Players would find all their items dumped on the floor of their house instead of neatly organized in containers after a server crash.

## Root Cause Analysis

### Before the Fix
1. **Saving**: `House_save()` did not track container nesting depth
   - Always passed `location=0` to the save function
   - Recursion order was incorrect (containers before siblings)
   - Result: All objects were saved with `location=0` (top-level)

2. **Loading**: `House_load()` ignored location information
   - Used simple `obj_to_room()` for all objects
   - No container hierarchy reconstruction
   - Result: All objects placed directly in the room

### The Fix
Modified house save/load to work like the proven player crash-save/load system:

1. **Saving** (`House_save()`):
   - Added `int location` parameter to track nesting depth
   - Uses `MIN(0, location) - 1` for contained objects (negative values indicate depth)
   - Fixed recursion order to save siblings before children (matching `Crash_save()`)
   - Example: Item in a bag gets location=-1, item in a bag within a bag gets location=-2

2. **Loading** (`House_load()` and new `House_load_obj()`):
   - Maintains `cont_row[]` array to track pending container contents
   - Reconstructs container hierarchy from location values
   - Places objects in correct containers or room as appropriate
   - Handles edge cases (missing containers, non-container objects with contents)

## Technical Details

### Location Values
- `location = 0`: Object at room level (top-level)
- `location < 0`: Object inside a container (depth indicated by magnitude)
- `location > 0`: Not used for houses (used for equipped items on players)

### Container Reconstruction Algorithm
1. Objects are loaded in the order they were saved
2. Objects with negative location are added to `cont_row[]` array
3. When a container is loaded, pending contents from appropriate `cont_row[]` are placed inside it
4. Maximum nesting depth: 5 levels (`MAX_BAG_ROWS`)

## Files Modified
- `src/house.h`: Updated `House_save()` function signature
- `src/house.c`: 
  - Modified `House_save()` to track container depth
  - Modified `House_load()` to reconstruct containers
  - Added `House_load_obj()` helper function
  - Added `MAX_BAG_ROWS` constant

## Testing Notes
- Code compiles successfully with CMake
- Code formatted with clang-format per project standards
- No security vulnerabilities detected by CodeQL
- Logic mirrors the proven `Crash_save()`/`handle_obj()` pattern

## Suggested Testing
To verify the fix works correctly:
1. Create a house in the game
2. Place multiple containers in the house
3. Put items inside the containers (test multiple nesting levels)
4. Force a crash-save (`House_crashsave()` is called)
5. Restart the server and verify items are still in their containers

## Related Code
- `src/objsave.c`: Contains `Crash_save()` and `handle_obj()` functions that this fix mirrors
- `src/handler.c`: Contains `obj_to_obj()` and `obj_to_room()` functions used for object placement
