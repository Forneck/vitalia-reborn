# Example: Creating a Chained Spell Variant System

This example demonstrates how to create a chained spell variant system using the new spedit features.

## Example: Elemental Shield Evolution

We'll create a progression: `fireshield` → `watershield` → `steamshield`

### Step 1: Verify Base Spell Exists

Assuming `fireshield` (vnum 54) already exists as a base spell.

### Step 2: Create First Variant (watershield)

```
> spedit new
Enter new spell VNUM: 155
Do you want to create a new spell (vnum: 155)? y

-- Spell Number      : [155]
T) Type              : [SPELL]
1) Status            : Unavailable
2) Name              : Undefined
...

2) Enter spell name: watershield

E) School: Choose Abjuration (same as fireshield)
L) Element: Choose Water (2)

R) Prerequisite: 54
   "Enter prerequisite spell VNUM (0 for none): 54"
   System confirms: Prerequisite set to fireshield[54]

V) Discoverable: 1
   "Is this spell discoverable through experimentation? (0-No, 1-Yes): 1"

(Configure other spell properties similar to fireshield but with water theme)

W) Check warnings - should show no errors

Q) Save
```

### Step 3: Create Second Variant (steamshield - variant of watershield)

```
> spedit new
Enter new spell VNUM: 156
Do you want to create a new spell (vnum: 156)? y

2) Enter spell name: steamshield

E) School: Choose Abjuration
L) Element: Choose Water (2) or Fire (1) - steam is water+fire

R) Prerequisite: 155
   "Enter prerequisite spell VNUM (0 for none): 155"
   
   System displays:
   "Variant Chain: fireshield[54] -> watershield[155] -> steamshield[156]"

V) Discoverable: 1

(Configure other spell properties as a powered-up version)

Q) Save
```

## Player Discovery Flow

### Player Discovers watershield

1. Player has learned `fireshield` (vnum 54) through normal means
2. Player uses: `syllables`
   ```
   You know the following spells:
   fireshield                     -> ignisaegis
   ```
3. Player experiments: `experiment aquaaegis`
4. Success! Player learns `watershield` at 15% proficiency
5. Player uses `syllables` again:
   ```
   You know the following spells:
   fireshield                     -> ignisaegis
   watershield                    -> aquaaegis
   ```

### Player Discovers steamshield

1. Player now knows both `fireshield` and `watershield`
2. Player experiments: `experiment vaporaegis`
   (assuming "steam" → "vapor" in syllable table)
3. Success! Player learns `steamshield` at 15% proficiency
4. Player can now cast all three spells in the chain

## Immortal Verification

### Check the chain in spedit

```
> spedit 156

Variant Chain: fireshield[54] -> watershield[155] -> steamshield[156]

-- Spell Number      : [156]
R) Prerequisite      : watershield (155)
V) Discoverable      : Yes
```

### Check for issues

```
W) Warnings

Should show no errors if properly configured:
- ✓ Spell is discoverable and has prerequisite
- ✓ No circular dependencies
- ✓ Chain is valid
```

### View all variants

```
> splist all

Will show:
54) [   54] fireshield           SPELL    No          Yes         Mu Cl
155) [  155] watershield          SPELL    No          Yes         Mu Cl  
156) [  156] steamshield          SPELL    No          Yes         Mu Cl
```

## Advanced Example: Multi-Branch Chain

Create branching evolutions:

```
fireshield (base)
  ├─> watershield
  │     └─> steamshield
  │           └─> boilingshield
  ├─> iceshield
  │     └─> frostshield
  └─> magmashield
        └─> lavashield
```

Each variant requires only its immediate parent:
- `watershield` requires `fireshield`
- `iceshield` requires `fireshield`
- `steamshield` requires `watershield`
- `frostshield` requires `iceshield`

This creates multiple discovery paths from a single base spell!

## Common Pitfalls to Avoid

### ❌ Circular Dependencies
```
spell A prerequisite = B
spell B prerequisite = A
```
System will block this!

### ❌ Self-Reference
```
spell A prerequisite = A
```
System will block this!

### ❌ Forgetting Discoverable Flag
```
Prerequisite set but discoverable = 0
```
Players can't discover it! System will warn you.

### ❌ No Prerequisite but Discoverable
```
Prerequisite = 0 but discoverable = 1
```
Players can never meet requirements! System will warn you.

## Tips for Game Masters

1. **Plan Your Chains**: Draw them on paper first
2. **Test Discovery**: Use a test character to verify syllables work
3. **Balance Power**: Each variant should be incrementally better
4. **Theme Consistency**: Make sure variants make thematic sense
5. **Document**: Keep notes on your variant trees

## Conclusion

The chained variant system adds depth to spell progression while maintaining simplicity for both immortals and players. The validation system ensures you can't create invalid chains, and the visualization helps you understand your spell hierarchies at a glance.
