# Spell Variant Chains in Spedit

## Overview

The spedit system now fully supports creating **chained spell variants**, allowing immortals to create progression trees of related spells that players can discover through experimentation.

## What Are Chained Variants?

Chained variants allow you to create spell discovery paths like:

```
fireball (base spell)
  └─> waterball (variant of fireball)
       └─> iceball (variant of waterball)
            └─> frostball (variant of iceball)
```

Players must learn each spell in the chain before they can discover the next variant.

## How It Works

### For Immortals (Creating Variants)

1. **Create the base spell** (if it doesn't exist)
   ```
   > spedit fireball
   ```

2. **Create the first variant**
   ```
   > spedit new waterball
   > (set up spell parameters)
   > R) Enter prerequisite spell VNUM: <fireball_vnum>
   > V) Is this spell discoverable? 1
   > Q) Save
   ```

3. **Create chained variants** (variants of variants)
   ```
   > spedit new iceball
   > (set up spell parameters)
   > R) Enter prerequisite spell VNUM: <waterball_vnum>
   > V) Is this spell discoverable? 1
   > Q) Save
   ```

### For Players (Discovering Variants)

Players discover variants through the `experiment` command:

1. Player learns `fireball` normally (through leveling, training, etc.)
2. Player uses `syllables` to see magical words
3. Player experiments with `aqua` syllables: `experiment aquaflamma`
4. If successful, learns `waterball`
5. Repeat process: player can now experiment to discover `iceball` using waterball as prerequisite

## Spedit Menu Options

### R) Prerequisite
- Set the spell VNUM that players must know to discover this spell
- Enter `0` to remove prerequisite (make it a base spell)
- **Supports chaining**: the prerequisite can itself be a variant
- System validates against circular dependencies

### V) Discoverable
- Set whether this spell can be discovered through experimentation
- `0` = No (must be learned normally)
- `1` = Yes (can be discovered via experiment command)
- Only works if a prerequisite is also set

## Variant Chain Display

When editing a spell with a prerequisite, spedit displays the full chain:

```
Variant Chain: fireball[54] -> waterball[155] -> iceball[156]

-- Spell Number : [156]
...
R) Prerequisite      : waterball (155)
V) Discoverable      : Yes
```

This helps you visualize the progression path.

## Safety Features

### Circular Dependency Prevention

The system prevents you from creating circular chains:

```
✗ fireball -> waterball -> fireball  (blocked)
✗ iceball -> iceball                 (blocked)
```

If you try to create a circular dependency, spedit will show an error and reject the change.

### Chain Depth Warnings

- Maximum tracked depth: 100 levels (virtually unlimited for practical use)
- Warning shown if chain exceeds 100 (likely indicates a problem)

### Validation Warnings

When you use the `W) Warnings` menu option, spedit checks:

- ✓ Discoverable spells have prerequisites set
- ✓ Spells with prerequisites are marked discoverable (or warns if not)
- ✓ Spell is not its own prerequisite
- ✓ All prerequisites exist in the database

## Example Variant Chains

### Elemental Shield Progression
```
fireshield (base)
  ├─> watershield (water variant)
  ├─> earthshield (earth variant)
  └─> airshield (air variant)
       └─> stormshield (advanced air variant)
```

### Healing Spell Progression
```
cure light (base)
  └─> cure serious (improved healing)
       └─> cure critical (advanced healing)
            └─> heal (master healing)
```

### Damage Spell Evolution
```
magic missile (base)
  └─> acid arrow (elemental evolution)
       └─> acid storm (area effect evolution)
```

## Best Practices

1. **Logical Progression**: Make sure variant chains make thematic sense
   - Elements should relate (fire → lava → magma)
   - Power should increase gradually
   - Effects should be connected

2. **Balance**: 
   - Later variants should be more powerful but not overpowered
   - Consider mana costs, damage, duration, etc.
   - Test the full chain for game balance

3. **Discovery Difficulty**:
   - Syllable combinations should be logical
   - Don't make chains too long (3-5 spells is good)
   - Consider at what level players can reasonably discover each variant

4. **Documentation**:
   - Use clear spell names that show relationships
   - Document the intended progression in design notes
   - Test with players to ensure discovery is fun

## Technical Details

### Database Format

Variant information is saved to `lib/misc/spells`:

```
1 P 156 1 2 8194 8 0           # Spell header
2 iceball                       # Spell name
57 155                          # DB_CODE_PREREQUISITE_SPELL (waterball)
58 1                            # DB_CODE_DISCOVERABLE (yes)
...
```

### Code Flow

1. Player uses `experiment <syllables>`
2. System checks all discoverable spells
3. For each spell, verifies player knows the prerequisite
4. If syllables match and prerequisite known, learns the spell
5. Discovery is logged for monitoring

### Chain Resolution

When checking prerequisites, the system:
- Only checks immediate prerequisite (direct parent)
- Doesn't require knowing the entire chain history
- Allows flexible discovery order if prerequisites are met

## Troubleshooting

### "Player can't discover variant"
- ✓ Check spell is marked discoverable (V = 1)
- ✓ Check prerequisite is set correctly
- ✓ Verify player knows the prerequisite spell
- ✓ Test syllable matching (use `syllables` command)

### "Circular dependency error"
- ✓ Check your chain doesn't loop back
- ✓ Use W) Warnings to validate the chain
- ✓ Draw out your chain on paper first

### "Chain too long warning"
- ✓ Review your chain structure
- ✓ Consider breaking into multiple shorter chains
- ✓ Check for accidental loops

## Conclusion

Chained spell variants add depth to spell progression and give players rewarding discovery mechanics. Use them to create interesting magical progression trees that enhance gameplay and encourage experimentation!
