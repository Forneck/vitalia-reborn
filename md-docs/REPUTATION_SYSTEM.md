# Reputation System - Technical Documentation

## Overview

The reputation system tracks the standing of players and mobs in the world, ranging from 0 (notorious) to 100 (legendary). Reputation affects NPC interactions, shop availability, and social dynamics.

## Reputation Storage

### Players
- Stored in: `ch->player_specials->saved.reputation` (0-100)
- Calculated via: `calculate_player_reputation(ch)`
- Modified via: `modify_player_reputation(ch, amount)`
- Accessed via: `GET_REPUTATION(ch)` or `GET_PLAYER_REPUTATION(ch)`

### Mobs
- Stored in: `ch->ai_data->reputation` (0-100)
- Accessed via: `GET_REPUTATION(ch)` or `GET_MOB_REPUTATION(ch)`
- Direct modification: `ch->ai_data->reputation = MIN(100, value)`

## Initial Reputation Calculation (Players)

Calculated on first check if not already set:

```c
Base reputation = 0

// Quest completion bonus
+= MIN(completed_quests * 2, 30)  // Max +30 from quests

// Karma influence
+= URANGE(-25, karma / 20, 25)     // ±25 from karma system

// Level-based bonus (for experienced players)
if (level >= 20)
    += (level - 15) / 2            // Higher level = better reputation

Final reputation = URANGE(0, total, 100)
```

## Reputation Gain Mechanisms

### Combat-Based Gains

1. **Killing Evil Mobs** (+1 to +2)
   - Source: fight.c
   - Condition: Player kills evil-aligned mob
   - Bonus: +1-3 additional if mob is 5+ levels higher

2. **Defeating High-Reputation Opponents** (+2 to +4)
   - Mob defeats mob with reputation ≥50
   - Scales with opponent's reputation level

3. **Mob Defeating High-Reputation Player** (+5)
   - Mob gains significant reputation
   - Triggers bragging behavior if CONFIG_MOB_CONTEXTUAL_SOCIALS enabled

### Helpful Actions

4. **Healing Others** (+1 to +3)
   - Source: magic.c (mag_points)
   - Base: +1 for any healing spell on another character
   - Bonus: +1-2 additional if healing high-reputation target (≥70)
   - Does not apply to self-healing

5. **Giving Valuable Items** (+1 to +2)
   - Source: act.item.c (perform_give)
   - 100-999 gold value: +1
   - 1000+ gold value: +1-2
   - Bonus: +1 if giving to high-reputation NPC (≥60)

6. **Donating Gold** (+1 to +4)
   - Source: act.item.c (perform_give_gold)
   - 100-499 gold: +1
   - 500-999 gold: +1-3
   - 1000+ gold: +2-4

### Quest System

7. **Quest Completion** (+1 to +10)
   - Source: quest.c (mob_complete_quest)
   - Random bonus based on quest difficulty
   - Applies to mob quests primarily

## Reputation Loss Mechanisms

### Combat-Based Losses

1. **Dying** (-1 to -3)
   - Source: fight.c (raw_kill)
   - Players: -1 to -3
   - Mobs: -1 to -2

2. **Killing Good Mobs** (-1 to -3)
   - Source: fight.c
   - Murdering good-aligned creatures damages reputation

3. **Player Killing (PvP)** (-5 to -10)
   - Base: -5 to -10
   - Additional: -5 to -15 if victim had high reputation (≥60)
   - Severe penalty for killing reputable players

### Quest System

4. **Quest Failure** (-1 to -5)
   - Source: quest.c (mob_fail_quest)
   - Random penalty for failing quests

### Karma System

5. **Negative Karma** (up to -25)
   - Applied during initial reputation calculation
   - Karma / 20 = reputation modifier

## Reputation Thresholds & Effects

### Shop Restrictions (shop.c)

- **<10 (Very Bad)**: All shopkeepers refuse service
  - "Você é conhecido por má conduta. Vá embora!"
  
- **<20 + Evil Alignment**: Shopkeepers refuse service
  - "Sua reputação é péssima. Não faço negócios com gente como você!"

### Social Behavior (mobact.c)

- **≥60 (Respectable)**: 
  - Triggers positive socials from friendly mobs
  - Mobs more likely to trust and help
  
- **≥70 (Honored)**:
  - Bonus reputation when others heal you
  - NPCs show extra respect
  
- **≥80 (Legendary)**:
  - Mobs brag enthusiastically when defeating you
  - Maximum quest reward bonuses (120%)

- **<20 (Disreputable)**:
  - Triggers negative socials from mobs
  - Shop restrictions begin

### Contextual Social System (CONFIG_MOB_CONTEXTUAL_SOCIALS)

When enabled, mob social behavior is influenced by target reputation:
- High reputation (≥60) → positive socials
- Low reputation (<20) → negative socials
- Combined with alignment and emotion states

## Alignment-Reputation Interaction

### Current System
- Evil/Good alignment affects initial calculation via karma
- Alignment determines which kills increase vs decrease reputation
- Shop restrictions are stricter for evil + low reputation

### Design Philosophy
Reputation represents **social standing** and **fame**, not moral alignment:
- A notorious evil character can have high reputation
- A good character who fails often can have low reputation
- Reputation measures how well-known and respected you are, not whether you're good or evil

## Gap Analysis & Future Enhancements

### Current Gaps

1. **No Class-Based Reputation**
   - Clerics/Paladins should gain more from healing
   - Thieves might gain from stealth/subterfuge
   - Warriors from honorable combat

2. **Limited Evil Reputation Paths**
   - Evil characters primarily lose reputation
   - No "infamous" reputation for successful evil deeds
   - Could add separate "infamy" track

3. **No Reputation Decay**
   - Reputation doesn't naturally decrease over time
   - Once gained, it's permanent (except through negative actions)

4. **Player Reputation Rarely Modified**
   - Most runtime changes are for mobs
   - Players rely on initial calculation
   - Need more dynamic player reputation events

### Proposed Enhancements

#### Class-Based Reputation Modifiers
```c
// Clerics/Paladins
- Healing: 2x reputation gain
- Protecting innocents: +reputation
- Using evil spells: -reputation

// Thieves/Rogues  
- Successful stealth: +reputation among rogues
- Getting caught: -reputation
- Honorable theft targets: neutral/positive

// Warriors
- Honorable duels: +reputation
- Cowardly behavior: -reputation
- Protecting weaker allies: +reputation

// Mages
- Sharing knowledge: +reputation
- Magical mishaps: -reputation
- Powerful spell displays: +reputation
```

#### Alignment-Specific Paths
- **Good Path**: Gains from helping, protecting, healing
- **Evil Path**: Gains from intimidation, conquest, fear (infamy)
- **Neutral Path**: Gains from balance, mediation, fairness

#### Time-Based Decay
- Reputation slowly decays toward 50 (neutral) if inactive
- Prevents permanent legendary status without maintenance
- Rate could be configurable

#### Dynamic Player Events
- Saving NPCs from danger
- Defending towns/villages
- Breaking laws (theft, murder)
- Completing difficult challenges
- Social interactions (bribes, threats, promises)

## Implementation Files

- **utils.c**: Core reputation calculation and modification functions
- **fight.c**: Combat-related reputation changes
- **magic.c**: Healing reputation gains
- **act.item.c**: Generosity (giving) reputation gains
- **quest.c**: Quest completion/failure reputation changes
- **shop.c**: Reputation-based shop restrictions
- **mobact.c**: Reputation-influenced social behavior

## Configuration

No specific configuration flags for reputation system itself, but related:
- `CONFIG_MOB_CONTEXTUAL_SOCIALS`: Enables reputation-influenced mob socials
- Shop behavior is always active (no config flag)

## Testing Recommendations

1. Test reputation boundaries (0, 10, 20, 60, 70, 80, 100)
2. Verify shop restrictions at thresholds
3. Check that both players and mobs gain/lose reputation appropriately
4. Test alignment interactions with reputation
5. Verify mob social behavior reflects reputation levels
6. Test that dying reduces reputation
7. Verify healing others increases reputation
8. Check giving items/gold increases reputation

## Notes for Builders

- Mob initial reputation should reflect their role/importance
- High-reputation mobs (≥50) are valuable targets for other mobs
- Shop restrictions based on reputation add roleplay depth
- Consider reputation when designing quest rewards
- NPC reactions should reflect player reputation levels

## Alignment-Specific Reputation Paths (Implemented)

### Evil Character Reputation (Infamy System)

Evil-aligned characters can build reputation through "evil deeds" that would harm the reputation of good characters:

**Evil Reputation Gains:**
- **Stealing (successful)**: +1-2 reputation (infamy)
  - From good targets: +1-2 (double gain)
  - Any target: +1
- **Backstabbing**: +1-3 reputation
  - Good targets: +1-3
  - Any target: +1-2
- **Poisoning (taint)**: +1-3 reputation
  - Good targets: +2-3
  - Any target: +1-2
- **Killing evil mobs**: +1-2 (proving strength)
- **Defeating strong opponents**: +1-3 bonus

Evil characters still lose reputation for:
- Getting caught stealing: -3-6
- Getting caught poisoning: -4-8
- Dying: -1-3
- Attacking allies: -5-10

### Good/Neutral Character Reputation

Good and neutral characters LOSE reputation for "dishonorable" actions:

**Reputation Penalties for Good/Neutral:**
- **Stealing**: -2-4 reputation
  - From good targets: Additional -1-3
  - Getting caught: -3-6
- **Backstabbing**: -3-5 reputation
  - Good targets: Additional -2-4
- **Poisoning**: -3-6 reputation
  - Good targets: Additional -2-4
  - Getting caught: -4-8

**Reputation Gains (All Alignments):**
- Killing evil mobs: +1-2
- Healing others: +1-3
- Giving gold/items: +1-4
- Completing quests: +1-10

### Universal Reputation Losses

**All Alignments Lose Reputation For:**
- **Attacking allies/group members**: -5-10 (severe)
- **Dying**: -1-3
- **Killing other players (PvP)**: -5-25
- **Quest failure**: -1-5

### Design Philosophy

The system allows for "infamous villains" with high reputation:
- Evil characters build reputation through feared deeds
- Good characters build reputation through heroic deeds
- Reputation measures **fame/notoriety**, not morality
- Alignment determines which actions build vs. damage reputation

### Examples

**Evil Rogue "Shadowblade" (reputation 75)**
- Successful theft from nobles: +reputation
- Backstabbing guards: +reputation
- Poisoning rival's drink: +reputation
- Feared and respected in underworld

**Good Paladin "Lightbringer" (reputation 85)**
- Defeating evil creatures: +reputation
- Healing wounded: +reputation
- Giving to poor: +reputation
- Stealing/backstabbing: -reputation (against code)

**Neutral Mercenary "Greywind" (reputation 60)**
- Combat prowess: +reputation
- Healing allies: +reputation
- Backstabbing for hire: -reputation (dishonorable)
- Known for reliability, not methods

## Class-Based Reputation Bonuses (Implemented)

Each class gains additional reputation for actions aligned with their theme:

### Warrior
- **Combat Kills**: +1 bonus reputation for kills
- **Defeating Stronger Opponents**: Additional +1 if target is higher level
- **Theme**: Combat prowess and martial skill

### Magic User
- **Magic Casting**: +1 bonus reputation
- **Scholarly Pursuits**: +1-2 bonus reputation
- **Theme**: Advancing magical knowledge and understanding world laws

### Cleric
- **Healing Others**: +1 bonus reputation
- **Generosity (Charity)**: +1 bonus reputation
- **Acts of Faith**: +1-2 bonus reputation
- **Theme**: Faith in the Gods and helping others

### Thief
- **Stealing (Evil only)**: +1-2 bonus reputation
- **Backstabbing (Evil only)**: +1-2 bonus reputation
- **Poisoning (Evil only)**: +1 bonus reputation
- **Theme**: Masters of stealth, cunning, and shadow arts

### Druid
- **Healing Others**: +1 bonus reputation
- **Nature Interaction**: +1 bonus reputation
- **Theme**: Connection to animals and nature

### Bard
- **Quest Completion**: +1 bonus reputation (spreading tales)
- **Social Performance**: +1-2 bonus reputation
- **Generosity (Patronage)**: +1 bonus reputation
- **Theme**: Music, arts, and cultural influence

### Ranger
- **Combat Kills (Animals)**: +1 bonus reputation for hunting
- **Quest Completion**: +1 bonus reputation (tracking)
- **Nature Interaction**: +1 bonus reputation
- **Theme**: Hunt, movement expertise, and flora mastery

### Examples

**Warrior "Ironblade" killing evil mob:**
- Base: +1-2 reputation
- Warrior bonus: +1
- Total: +2-3 reputation

**Cleric "Lightkeeper" healing wounded:**
- Base: +1-3 reputation
- Cleric bonus: +1
- Total: +2-4 reputation

**Evil Thief "Shadowstep" stealing from noble:**
- Base (evil): +1-2 reputation
- Thief bonus: +1-2
- Total: +2-4 reputation (infamy)

**Bard "Songweaver" completing quest:**
- Base: +1-10 reputation
- Bard bonus: +1 (spreading tales)
- Total: +2-11 reputation

**Ranger "Wolfheart" hunting beasts:**
- Base: +1-2 reputation
- Ranger bonus: +1 (hunting)
- Total: +2-3 reputation

### Implementation

Class bonuses stack with alignment bonuses and base reputation gains. The `get_class_reputation_modifier()` function calculates the appropriate bonus based on action type and class, encouraging players to pursue class-appropriate activities for maximum reputation growth.
