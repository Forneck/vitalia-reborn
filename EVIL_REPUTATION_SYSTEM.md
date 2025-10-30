# Evil Player Reputation System

## Overview
This document describes the enhanced reputation system that provides evil-aligned players of all classes with reputation routes (infamy) for performing evil actions.

## Problem Solved
Previously, only thieves benefited from evil actions in the reputation system. Evil players of other classes (Mago, Clérigo, Guerreiro, Druida, Bardo, Ranger) would lose reputation for evil actions, making being evil only disadvantageous.

## New Features

### Evil Class Variants Supported
- **Necromancers** (Evil Magic Users) - Gain reputation for dark/necromantic magic
- **Fallen Clerics** (Evil Clerics) - Gain reputation for harmful divine magic and dark spells
- **Evil Warriors** - Gain reputation for killing good-aligned targets
- **Blighted Druids** (Evil Druids) - Gain reputation for harmful nature magic
- **Dark Rangers** (Evil Rangers) - Gain reputation for hunting good targets
- **Evil Bards** - Existing system applies (can gain reputation through social performance)
- **Assassins** (Evil Thieves) - Existing system enhanced

### New Reputation Action Types

#### CLASS_REP_DARK_MAGIC (12)
For necromantic and unholy magical actions:
- **Evil Magic Users**: +1-2 bonus reputation
- **Evil Clerics**: +1 bonus reputation
- Triggered by spells with SCHOOL_NECROMANCY or ELEMENT_UNHOLY

#### CLASS_REP_HARM_SPELL (13)
For harmful divine/natural magic:
- **Evil Clerics**: +1-2 bonus reputation
- **Evil Druids**: +1 bonus reputation
- Triggered by harmful spells with SCHOOL_NECROMANCY or ELEMENT_UNHOLY

### Evil Spells Recognized

The system automatically identifies evil spells using their school and element properties:

**Necromancy School (SCHOOL_NECROMANCY):**
- Curse
- Chill Touch
- Energy Drain
- Poison
- Animate Dead
- Darkness
- Vampiric Touch
- Speak with Dead
- Resurrection/Revive (necromantic life restoration)
- Wither
- Death Touch

**Unholy Element (ELEMENT_UNHOLY):**
- Fear
- Harm
- Dispel Good
- Animate Dead
- Gloomshield
- Darkness
- Vampiric Touch
- Speak with Dead
- Death Touch
- Curse of Weakness

### Reputation Gains

#### For Damage Spells (mag_damage)
Evil casters using necromancy/unholy spells:
- Against good targets: +1-3 reputation (+class bonus)
- Against other targets: +1-2 reputation (+class bonus)

Evil magic users using any offensive magic:
- Against good targets: +1 reputation (+class bonus)

#### For Harmful Affect Spells (mag_affects)
Evil casters using necromancy/unholy debuffs:
- Against good targets: +1-2 reputation (+class bonus)
- Against other targets: +1 reputation (+class bonus)

#### For Combat Kills
Evil warriors/rangers:
- Killing good-aligned mobs: +2-4 reputation (+class bonus)
- Killing evil-aligned mobs: +1-2 reputation (+class bonus)
- Bonus for high-level targets: +1-3 reputation (+class bonus)

### Reputation Penalties

Good/Neutral characters still lose reputation for evil actions:
- Using dark magic: -3-6 reputation
- Extra penalty for targeting good: -2-4 reputation
- Killing good-aligned mobs: -1-3 reputation

## Configuration

The system respects the `CONFIG_DYNAMIC_REPUTATION` setting. Enable it in your configuration to activate the dynamic reputation system.

## Class-Specific Bonuses

Each class receives bonuses for actions aligned with their evil path:

| Class | Evil Action | Bonus |
|-------|------------|-------|
| Magic User | Dark magic spells | +1-2 |
| Magic User | Any offensive magic vs good | +1 |
| Cleric | Dark magic spells | +1 |
| Cleric | Harmful spells | +1-2 |
| Warrior | Killing good targets | +1 |
| Ranger | Hunting (any kill) | +1 |
| Druid | Harmful spells | +1 |
| Thief | Stealth actions | +1-2 |
| Thief | Poisoning | +1 |

## Technical Implementation

### Files Modified
- `src/utils.h` - Added CLASS_REP_DARK_MAGIC and CLASS_REP_HARM_SPELL constants
- `src/utils.c` - Enhanced get_class_reputation_modifier() with evil class support
- `src/magic.c` - Added reputation gains in mag_damage() and mag_affects()
- `src/fight.c` - Enhanced combat reputation for evil killers

### Key Functions
- `get_class_reputation_modifier()` - Returns class-specific reputation bonus
- `modify_player_reputation()` - Modifies player reputation value
- `mag_damage()` - Handles damage spell reputation
- `mag_affects()` - Handles affect spell reputation

## Player Experience

### Evil Magic User (Necromancer)
Cast animate dead on corpse → Gain reputation
Cast energy drain on good cleric → Gain more reputation
Build up "infamy" as a feared necromancer

### Fallen Cleric
Cast curse on good paladin → Gain reputation
Cast harm on innocent villager → Gain reputation
Build up "infamy" as a dark priest

### Evil Warrior
Kill good-aligned guards → Gain reputation
Defeat righteous paladins → Gain more reputation
Build up "infamy" as a feared slayer

## Balance Notes

- Evil players now have a viable path to gain reputation (infamy)
- The system maintains penalties for evil actions by good/neutral characters
- Targeting good-aligned victims provides higher reputation gains
- Class bonuses ensure each class has appropriate evil paths
- The system is symmetric: evil actions by evil characters = reputation gain, same actions by good characters = reputation loss
