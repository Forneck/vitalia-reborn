# Percentual Load Feature - Implementation Summary

## Overview
The Percentual Load feature has been successfully implemented for the Vitalia Reborn MUD engine. This feature allows zone builders to specify percentage-based probability for loading mobs and objects during zone resets, instead of only absolute maximum counts.

## What Changed

### Core Engine Changes
1. **src/db.c** - Modified `reset_zone()` function
   - Added percentage-based loading logic for M, O, G, E, P commands
   - Uses negative values in arg2 to represent percentage (e.g., -50 = 50% chance)
   - Maintains full backward compatibility with positive values

2. **src/zedit.c** - Updated zone editor display
   - Shows "Chance: X%" for percentage-based loads (negative values)
   - Shows "Max: X" for traditional loads (positive values)

### Documentation
1. **docs/PERCENTUAL_LOAD.md** - Complete feature documentation
   - Usage guide with examples
   - Technical details
   - Migration guide for existing zones
   
2. **docs/example_zone_percentual.zon** - Example zone file
   - Demonstrates both traditional and percentual loading
   - Shows various use cases

3. **changelog.txt** - Updated with feature announcement

### Testing & Validation
1. **validate_percentual_load.sh** - Validation script
   - Checks implementation integrity
   - Verifies binary compilation
   - Confirms documentation exists

## How It Works

### Zone File Syntax
```
M 0 3010 -50 3062   # 50% chance to load mob 3010 in room 3062
O 0 3019 -75 3062   # 75% chance to load object 3019 in room 3062
G 1 3015 -30 -1     # 30% chance to give object 3015 to last loaded mob
E 1 3020 -60 16     # 60% chance to equip object 3020 at position 16
P 1 3081 -40 3080   # 40% chance to put object 3081 in container 3080
```

### Implementation Logic
- Check if arg2 < 0
- If negative: Use absolute value as percentage, generate random number 1-100
- If random number ≤ percentage: Load the mob/object
- If positive: Use traditional max count behavior

## Backward Compatibility
✅ **100% backward compatible**
- All existing zone files continue to work without modification
- Only new or modified zones need to use negative values
- Positive values maintain exact same behavior as before

## Benefits
1. **Dynamic Content**: Items and mobs can appear randomly
2. **Rare Items**: Easy to create rare drops with low percentages
3. **Varied Encounters**: Different gameplay experiences on each zone reset
4. **Builder Friendly**: Simple syntax, easy to understand and use

## Testing Status
✅ Code compiles successfully
✅ No security vulnerabilities detected (CodeQL)
✅ Validation script passes all checks
✅ Code formatted according to project standards

## Example Use Cases

### Rare Boss
```
M 0 5000 -10 5001   # 10% chance to spawn raid boss
```

### Random Treasure
```
O 0 1234 -5 3000    # 5% chance for legendary item
O 0 1235 -25 3000   # 25% chance for rare item
O 0 1236 -50 3000   # 50% chance for uncommon item
```

### Variable NPC Equipment
```
M 0 3000 1 3001     # Always spawn shopkeeper
E 1 3010 -100 16    # Always equipped with basic weapon
E 1 3011 -50 5      # 50% chance to wear magic armor
E 1 3012 -10 11     # 10% chance to have rare amulet
```

## Files Modified
- src/db.c (109 additions, 4 deletions)
- src/zedit.c (58 additions, 21 deletions)
- docs/PERCENTUAL_LOAD.md (new file, 128 lines)
- docs/example_zone_percentual.zon (new file)
- changelog.txt (updated)
- validate_percentual_load.sh (new file)

## Next Steps for Zone Builders
1. Read docs/PERCENTUAL_LOAD.md for detailed information
2. Review docs/example_zone_percentual.zon for syntax examples
3. Update existing zones with percentage-based loads where desired
4. Test zone resets to verify behavior
5. Adjust percentages based on desired rarity

## Issue Resolution
This implementation fully addresses issue #[issue_number] requesting "Percentual load" functionality for zone resets. The feature provides a clean, backward-compatible solution that enhances the zone building system without breaking any existing content.
