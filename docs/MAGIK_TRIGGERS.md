# Magik Triggers Documentation

## Overview
This document describes the DG Script triggers that replace the C-based `magik` special procedure. The magik system allows players to speak magic words to manipulate doors (open, close, lock, unlock) in specific rooms.

## Original Special Procedure
The `magik` special procedure read room extra fields named "magik" with patterns and commands to manipulate exits based on speech. The format was:
```
OX magik "<pattern>" <action> <direction>
```

## Implementation
Rather than using room extra fields, these triggers are implemented as individual room speech triggers that can be attached to specific rooms. This provides better maintainability and flexibility.

## Trigger Ranges

Triggers are organized by zone in their respective .trg files:

- **Zone 12** (`12.trg`): Triggers 1200-1203
- **Zone 45** (`45.trg`): Triggers 4510-4517 (new magik triggers, 4500-4508 are existing)
- **Zone 69** (`69.trg`): Triggers 6902-6911 (6900-6901 are existing)
- **Zone 114** (`114.trg`): Triggers 11400-11401
- **Zone 176** (`176.trg`): Triggers 17600-17601

## Triggers by Zone

### Zone 12 (Reino dos Imortais)
**File**: `lib/world/trg/12.trg`  
**Room Usage**: Room 1200 (suggested)

| Trigger | Keyword | Action | Direction |
|---------|---------|--------|-----------|
| 1200 | abra | Open | North |
| 1201 | fecha | Close | North |
| 1202 | tranca | Lock | North |
| 1203 | destranca | Unlock | North |

**Example Usage**:
```
> diz abra
A porta se abre.

> diz fecha  
A porta se fecha.

> diz tranca
Você escuta um ruído vindo da porta.

> diz destranca
Você escuta um ruído vindo da porta.
```

### Zone 45 (Gondolin)
**File**: `lib/world/trg/45.trg`  
**Room Usage**: Specific rooms with magical password doors

Note: Triggers 4500-4507 already exist with similar functionality but incomplete implementation. New triggers 4510-4517 provide improved magik functionality.

| Trigger | Keyword | Action | Direction |
|---------|---------|--------|-----------|
| 4510 | Ulmo/ulmo | Unlock | North |
| 4511 | Turgon/turgon | Open | North |
| 4512 | fecha | Close | North |
| 4513 | tranca | Lock | North |
| 4514 | Ulmo/ulmo | Unlock | South |
| 4515 | Turgon/turgon | Open | South |
| 4516 | fecha | Close | South |
| 4517 | tranca | Lock | South |

**Lore**: Uses names from Tolkien's legendarium - Ulmo (Vala of Waters) and Turgon (King of Gondolin) as magical passwords.

### Zone 69 (Sapo Gigante)
**File**: `lib/world/trg/69.trg`  
**Room Usage**: Multiple rooms, each with its own trapdoor requiring a specific biological classification password

Note: Triggers 6900-6901 already exist for other purposes. Magik triggers start at 6902.

| Trigger | Keyword | Action | Direction | Notes |
|---------|---------|--------|-----------|-------|
| 6902 | BUFONIDAE | Unlock | Down | One room with BUFONIDAE door |
| 6903 | BUFONIDAE | Open | Down | Same room |
| 6904 | OPISTHOCOELA | Unlock | Down | Different room with OPISTHOCOELA door |
| 6905 | OPISTHOCOELA | Open | Down | Same room |
| 6906 | PIPIDAE | Unlock | Down | Different room with PIPIDAE door |
| 6907 | PIPIDAE | Open | Down | Same room |
| 6908 | BREVICIPITIDAE | Unlock | Down | Different room with BREVICIPITIDAE door |
| 6909 | BREVICIPITIDAE | Open | Down | Same room |
| 6910 | SALIENTIA | Unlock | Down | Different room with SALIENTIA door |
| 6911 | SALIENTIA | Open | Down | Same room |

**Lore**: Uses taxonomic names from amphibian classification - appropriate for a giant frog/toad zone. Each room has a different trapdoor that requires its specific taxonomic password.

**Example Usage** (in room with BUFONIDAE door):
```
> diz BUFONIDAE
Você escuta um ruído vindo do alçapão.
O alçapão se abre.
```

**Note**: Each taxonomic name works only in its designated room. The triggers should be attached to the appropriate rooms during the attachment phase.

### Zone 114 (Hindu/Buddhist Temple)
**File**: `lib/world/trg/114.trg`  
**Room Usage**: Rooms with Sanskrit/meditation-based doors

| Trigger | Keyword | Action | Direction |
|---------|---------|--------|-----------|
| 11400 | avarohana | Unlock | East |
| 11401 | avarohana | Open | East |

**Lore**: "Avarohana" is a Sanskrit term related to descending musical scales or descent in meditation.

### Zone 176 (Chinese Temple)
**File**: `lib/world/trg/176.trg`  
**Room Usage**: Rooms with Chinese phrase-based doors

| Trigger | Keyword | Action | Direction |
|---------|---------|--------|-----------|
| 17600 | baohe lin tsung lee-dah | Unlock | East |
| 17601 | baohe lin tsung lee-duh | Open | East |

**Lore**: Uses Chinese-sounding phrases as magical passwords.

## Technical Implementation

### Door Flags
Triggers use the `%door%` command with flag combinations:
- `a` - Door exists (EX_ISDOOR)
- `ab` - Door exists and is closed (EX_ISDOOR | EX_CLOSED)
- `abc` - Door exists, closed, and locked (EX_ISDOOR | EX_CLOSED | EX_LOCKED)

### Speech Detection
All triggers use `%speech.contains(<keyword>)%` to match spoken words. The system is case-insensitive where both uppercase and lowercase variants are checked.

### Bidirectional Operation
Each trigger manipulates both sides of a door:
1. The room where speech occurs
2. The opposite room (if it exists)

This ensures consistent door state across both rooms.

### Messages
- **Open**: "A porta se abre." / "O alçapão se abre."
- **Close**: "A porta se fecha."
- **Lock/Unlock**: "Você escuta um ruído vindo da porta." / "...do alçapão."

## Attachment Instructions

Triggers should be attached to appropriate rooms using the `T <trigger_vnum>` format in world files. The user has indicated that attachment will be done separately.

**Example attachment** (to be done later):
```
#1200
A Sala de Reunião dos Deuses~
...room description...
~
12 8 0 0 0 0
D0
...door description...
~
porta~
2 -1 1204
T 90000
T 90001
T 90002
T 90003
S
```

## Notes

1. **Multiple Keywords**: Some triggers check for both uppercase and lowercase variants to be case-insensitive (e.g., "Ulmo" and "ulmo").

2. **Phrase Matching**: Zone 176 triggers use complete phrases which must be spoken exactly as specified.

3. **Order Dependency**: For zones with multiple actions on the same exit (unlock then open), players must speak both keywords in sequence.

4. **Error Handling**: Triggers check if exits exist before attempting to manipulate them, preventing errors.

5. **Echo Consistency**: Messages are in Portuguese, matching the MUD's language theme.

6. **Room-Specific Triggers**: Each trigger is designed for a specific room. For example, in Zone 69, each taxonomic name (BUFONIDAE, OPISTHOCOELA, etc.) is meant for a different room with its own trapdoor. During attachment, builders should assign the appropriate trigger pair (unlock + open) to each room.

## Original Specification Reference

The triggers implement the following original magik extra field specifications:

```
Zone 12:
  OX magik "*abra!*" open north
  OX magik "*fecha!*" close north
  OX magik "*tranca!*" lock north
  OX magik "*destranca!*" unlock north

Zone 45:
  OX magik "*Ulmo*" unlock north/south
  OX magik "*Turgon*" open north/south
  OX magik "*fecha*" close north/south
  OX magik "*tranca*" lock north/south

Zone 69:
  OX magik "*<TAXONOMY>*" unlock down
  OX magik "*<TAXONOMY>*" open down
  (Multiple taxonomic names)

Zone 114:
  OX magik "*avarohana*" unlock east
  OX magik "*avarohana*" open east

Zone 176:
  OX magik "baohe lin tsung lee-dah" unlock east
  OX magik "baohe lin tsung lee-duh" open east
```
