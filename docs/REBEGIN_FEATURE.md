# Rebegin/Remort Feature Documentation

## Overview
The rebegin/remort feature allows transcended players (PLR_TRNS flag) to restart their character's life with a new class while retaining some abilities from their previous incarnation.

## How It Works

### Requirements
- Player must have the PLR_TRNS (transcended) flag set
- Player cannot be in combat when attempting to rebegin
- Player must be a PC (not NPC)

### Rebegin Process

1. **Command**: Type `rebegin` to start the process
2. **Information Display**: The system shows the rebegin rules from lib/text/rebegin
3. **Skill Selection**: Player chooses one skill/spell from their current class to retain
4. **Class Selection**: Player chooses a new class (cannot repeat previously used classes)
5. **Attribute Reroll**: System rolls new attributes and allows player to accept or reroll
6. **Finalization**: Character is reset to level 1 with new class but retains the selected skill

### Key Features

#### Class Tracking
- The `was_class` array tracks all previously used classes
- Players cannot select a class they've already experienced
- This ensures players must try all classes before repeating

#### Skill Retention
- Players can choose one skill/spell from their current class
- The selected skill is stored in the `retained_skills` array
- Retained skills persist across all future incarnations
- This allows for cross-class builds (e.g., a Warrior with Mage spells)

#### Character Reset
- Level reset to 1
- Experience reset to 1
- Gold and bank gold cleared
- Attributes rerolled (with player acceptance)
- HP/Mana/Movement restored to maximum
- Remort counter incremented

### File Structure

#### New Data Fields
- `retained_skills[MAX_SKILLS+1]` in player_special_data_saved
- Saved/loaded using "RtSk:" tag in player files

#### Modified Files
- `src/act.other.c`: Added do_rebegin() command and show_class_skills()
- `src/interpreter.c`: Added command table entry and CON_RB_* state handlers
- `src/players.c`: Added save/load for retained_skills
- `src/structs.h`: Added retained_skills array to player structure
- `src/act.h`: Added ACMD declaration for do_rebegin

#### State Flow
1. CON_RB_SKILL: Select skill to retain
2. CON_RB_NEW_CLASS: Choose new class
3. CON_RB_REROLL: Roll and accept new attributes  
4. CON_RB_QHOME: Finalize rebegin process

### Admin Notes

#### Setting Transcended Flag
Use the `set` command to grant transcendence:
```
set <player> trans
```

#### Debugging
- Check `was_class` array to see player's class history
- Check `retained_skills` array to see cross-class abilities
- Use `stat <player>` to view character data

### Translation
The feature is implemented in Portuguese as per the original game text:
- "rebegin" command
- Portuguese prompts and messages
- Uses existing Portuguese class menu and skill names

### Technical Implementation
- Minimal code changes following CircleMUD/tbaMUD patterns
- Reuses existing character creation infrastructure
- Safe file I/O with proper error handling
- Maintains compatibility with existing save files