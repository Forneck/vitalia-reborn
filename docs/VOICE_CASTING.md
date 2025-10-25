# Voice Casting Feature

## Overview

Voice casting allows players to cast spells by speaking the magic syllables using the `say` command instead of typing the full `cast` command. This feature makes spell casting more immersive and realistic.

## How It Works

When a player uses the `say` command, the system automatically checks if the spoken text matches any known spell syllables. If a match is found and the player knows that spell, it will be cast automatically.

### Spell Syllables

Each spell has a unique set of syllables that are derived from its name using a syllable transformation system. For example:
- "magic missile" might become "kari telum"
- "fireball" might become "ignis globus"
- "cure light wounds" might become "medere dies wounds"

The exact syllables depend on the syllable mapping defined in `spell_parser.c`.

## Usage

### Basic Voice Casting

To cast a spell on yourself or the default target:
```
say <syllables>
```

Example:
```
say abra kadabra
```

### Voice Casting with Target

To cast a spell on a specific target:
```
say <syllables> <target>
```

Example:
```
say abra kadabra orc
say ignis globus guard
```

## Features

1. **Automatic Spell Detection**: The system automatically detects when spoken text matches spell syllables
2. **Target Support**: Players can specify targets by saying the syllables followed by the target name
3. **Validation**: All normal spell casting validations apply:
   - Mana requirements
   - Position requirements (standing, fighting, etc.)
   - Spell knowledge requirements
   - Target validity
4. **Feedback**: Players receive appropriate messages when:
   - Spell is successfully cast
   - Syllables match but player doesn't know the spell
   - Spell fails due to insufficient mana, wrong position, etc.

## Implementation Details

### Function: `check_voice_cast()`

Located in `src/spell_parser.c`, this function:
1. Checks if the player is a PC (not NPC)
2. Parses the spoken text to extract syllables and optional target
3. Matches syllables against all known spells
4. Verifies the player knows the spell
5. Constructs a proper `cast` command with the spell name and target
6. Passes the command through the normal spell casting system

### Integration

Voice casting is integrated into the `say` command in `src/act.comm.c`:
```c
/* Check if the spoken text triggers voice casting */
if (!IS_NPC(ch) && check_voice_cast(ch, argument)) {
    /* Voice casting was triggered, don't process as normal say */
    return;
}
```

## Testing

### Manual Test Cases

1. **Test Basic Voice Casting**:
   - Learn a spell
   - Say the spell's syllables
   - Verify the spell is cast

2. **Test Voice Casting with Target**:
   - Learn an offensive spell
   - Say the syllables followed by a mob name
   - Verify the spell targets the correct mob

3. **Test Unknown Spell**:
   - Say syllables for a spell you don't know
   - Verify you get the "energia estranha" message

4. **Test Insufficient Mana**:
   - Reduce your mana to near zero
   - Say spell syllables
   - Verify you get the "não tem energia" message

5. **Test Position Requirements**:
   - Sit down
   - Try to cast a spell that requires standing
   - Verify you get the appropriate position error

## Configuration

Voice casting is enabled by default for all players. NPCs cannot use voice casting.

The minimum length for voice cast detection is 5 characters to avoid false positives with short utterances.

## Backward Compatibility

The voice casting feature is fully backward compatible:
- Players can still use the traditional `cast` command
- Voice casting without targets works exactly as before
- The addition of target support doesn't break existing functionality

## Security Considerations

Voice casting goes through the same validation and security checks as normal spell casting:
- All spell restrictions apply
- No way to bypass mana, position, or knowledge requirements
- Commands are processed through the standard `command_interpreter`
- No privilege escalation or exploitation vectors
