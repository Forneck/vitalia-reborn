# Percentual Load Feature

## Overview

The Percentual Load feature adds support for percentage-based probability loading of mobs and objects during zone resets. This provides more dynamic and random gameplay experiences.

## Usage

### Zone File Format

In zone files (`.zon`), the second argument of load commands (M, O, G, E, P) can now accept negative values to represent percentage chance:

- **Positive values** (traditional): Absolute maximum count
  - Example: `M 0 3010 5 3062` - Load mob 3010 until there are 5 instances in the world, place in room 3062
  
- **Negative values** (new): Percentage chance to load
  - Example: `M 0 3010 -50 3062` - 50% chance to load mob 3010 in room 3062 on each reset
  - Example: `O 0 3019 -75 3062` - 75% chance to load object 3019 in room 3062 on each reset

### Supported Commands

The percentual load feature works with the following zone reset commands:

- **M** (Load Mobile): `M <if_flag> <mob_vnum> <max/percentage> <room>`
- **O** (Load Object to room): `O <if_flag> <obj_vnum> <max/percentage> <room>`
- **G** (Give object to mob): `G <if_flag> <obj_vnum> <max/percentage> -1`
- **E** (Equip mob with object): `E <if_flag> <obj_vnum> <max/percentage> <position>`
- **P** (Put object in object): `P <if_flag> <obj_vnum> <max/percentage> <container_vnum>`

### Examples

#### Traditional Absolute Loading
```
M 0 3010 1 3062    # Load mob 3010, max 1 instance, in room 3062
O 0 3019 99 3062   # Load object 3019, max 99 instances, in room 3062
G 1 3015 30 -1     # Give object 3015 to last loaded mob, max 30 instances
```

#### New Percentage-Based Loading
```
M 0 3010 -100 3062  # 100% chance to load mob 3010 in room 3062 (always loads)
M 0 3011 -50 3062   # 50% chance to load mob 3011 in room 3062
O 0 3019 -25 3062   # 25% chance to load object 3019 in room 3062
G 1 3015 -75 -1     # 75% chance to give object 3015 to last loaded mob
E 1 3020 -60 16     # 60% chance to equip last loaded mob with object 3020 at position 16
```

#### Mixed Usage
You can mix both traditional and percentual loading in the same zone:
```
M 0 3010 1 3062      # Always load 1 mob (traditional)
G 1 3015 -50 -1      # 50% chance to give item (percentual)
E 1 3020 -75 16      # 75% chance to equip item (percentual)
```

## Zone Editor (zedit)

When viewing zone commands in the zone editor (zedit), the display will show:
- Traditional loads: `Load <name> [<vnum>], Max : <number>`
- Percentual loads: `Load <name> [<vnum>], Chance : <percentage>%`

## Technical Details

### Implementation

The feature is implemented by checking if `arg2` (the max/count parameter) is negative:
- If `arg2 < 0`: The absolute value represents the percentage chance (1-100)
- If `arg2 >= 0`: Traditional behavior - absolute maximum count

The random number generator (`rand_number()`) is used to determine if the percentage check succeeds.

### Backward Compatibility

This feature is **fully backward compatible** with existing zone files:
- All existing zone files with positive values continue to work as before
- Only new or modified zone files need to use negative values for percentual loading

### Valid Percentage Range

- Minimum: -1 (1% chance)
- Maximum: -100 (100% chance, always loads)
- Values like -150 would give a 150% chance, but are capped at 100% effective chance

## Benefits

1. **Dynamic Content**: Items and mobs can appear randomly, making exploration more interesting
2. **Rare Items**: Create rare drops by using low percentages (e.g., -5 for 5% chance)
3. **Varied Encounters**: Mobs may or may not be present, creating different gameplay experiences
4. **Backward Compatible**: No need to modify existing zones unless you want to use the feature

## Migration Guide

To convert existing zones to use percentual loading:

1. Identify items/mobs you want to be random
2. Change the max value to a negative percentage
   - If you want it to load 50% of the time: use -50
   - If you want it to load 10% of the time: use -10
   - If you want it to always load: use -100 or keep the positive value
3. Test the zone by forcing resets to see the random behavior

## Example Zone

Here's an example of a zone using both traditional and percentual loading:

```
#999
Builder~
Test Zone with Percentual Load~
9990 9999 15 2 j 0 0 0 -1 -1 0
M 0 9900 1 9990         # Always load the shopkeeper (traditional)
G 1 9901 -100 -1        # Always give basic sword (100% chance)
G 1 9902 -50 -1         # 50% chance to give rare potion
E 1 9903 -25 16         # 25% chance to equip magical dagger
M 0 9910 -75 9991       # 75% chance to load a guard
M 0 9911 -25 9991       # 25% chance to load a rare mob
O 0 9920 -10 9992       # 10% chance to load treasure chest
S
```

In this example:
- The shopkeeper always loads (traditional max: 1)
- The shopkeeper always gets a basic sword (100% chance)
- The shopkeeper has a 50% chance to have a rare potion
- The shopkeeper has a 25% chance to be equipped with a magical dagger
- In room 9991, there's a 75% chance a guard appears
- In room 9991, there's a 25% chance a rare mob appears
- In room 9992, there's a 10% chance a treasure chest spawns
