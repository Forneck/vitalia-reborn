# FIT_EVOLVE Configuration Analysis

## Summary
CONFIG_FIT_EVOLVE is a **dead configuration variable** - it is defined, can be configured via CEDIT, and is saved/loaded properly, but it is **never used in any game logic**.

## Current Status

### Definition and Configuration
- **Defined in:** `src/utils.h:1222` - `#define CONFIG_FIT_EVOLVE config_info.play.fit_evolve`
- **Struct field:** `src/structs.h:1780` - `int fit_evolve;` in `struct game_data`
- **Default value:** `src/config.c:42` - `int fit_evolve = NO;`

### CEDIT Integration (Properly Implemented)
- **Loading:** `src/cedit.c:85` - Loaded into OLC_CONFIG during edit session
- **Saving:** `src/cedit.c:275` - Saved from OLC_CONFIG to global CONFIG_
- **Toggling:** `src/cedit.c:2258` - Can be toggled via CEDIT menu option '9' in Game Play Options
- **Menu display:** `src/cedit.c:1101` - Displayed as "Fit Evolve" option

### File Persistence (Properly Implemented)
- **Loading from file:** `src/db.c:4835` - `CONFIG_FIT_EVOLVE = num;` when reading "fit_evolve" tag
- **Default loading:** `src/db.c:4402` - `CONFIG_FIT_EVOLVE = fit_evolve;` 
- **NOT SAVED TO FILE:** The variable is not written to the config file in `save_config()` function

## Problem
**CONFIG_FIT_EVOLVE is never referenced anywhere in the game logic.** A search for all uses reveals:
```
src/utils.h:1222        - Macro definition
src/db.c:4402          - Default value loading
src/db.c:4835          - Config file reading
src/config.h:87        - External declaration
src/structs.h:1780     - Struct member
src/config.c:42        - Default value definition
src/cedit.c:85         - CEDIT loading
src/cedit.c:275        - CEDIT saving
src/cedit.c:1101       - CEDIT menu display
src/cedit.c:2258       - CEDIT toggling
```

**No actual game logic uses this configuration!**

## Missing Implementation
The variable needs to be saved to disk in the `save_config()` function. Currently missing:
```c
fprintf(fl,
        "* Fit Evolve configuration\n"
        "fit_evolve = %d\n\n",
        CONFIG_FIT_EVOLVE);
```

## Recommendations

### Option 1: Implement FIT_EVOLVE Functionality
If this was intended for evolutionary/genetic algorithms for mob behavior:
1. Define what "fit evolve" means in the game context
2. Implement the feature (e.g., mobs evolving based on survival/success)
3. Use CONFIG_FIT_EVOLVE to enable/disable the feature
4. Add the missing save_config() fprintf statement

### Option 2: Remove Dead Code
If this feature is not planned:
1. Remove CONFIG_FIT_EVOLVE from all files
2. Remove fit_evolve from struct game_data
3. Remove the CEDIT menu option
4. Clean up all references

### Option 3: Document as Reserved/Future
If this is reserved for future use:
1. Add the missing save_config() fprintf statement  
2. Document in code comments that it's reserved for future implementation
3. Keep the configuration infrastructure in place

## Related Variables
There is no "EVOLVE_FIT" variable in the codebase - only FIT_EVOLVE exists.
