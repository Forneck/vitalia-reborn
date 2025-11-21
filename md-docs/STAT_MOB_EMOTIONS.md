# Stat Mob Emotions Command

## Overview

The `stat <mobname> emotions` command allows administrators (LVL_GOD and above) to view the **effective emotions** that a mob feels toward the command issuer. This uses the hybrid emotion system which combines:

1. **Mood (Global State)**: The mob's general emotional baseline
2. **Relationship (Per-Player)**: Specific feelings toward individual players based on interaction history

## Command Syntax

```
stat <mobname> emotions
```

### Examples

```
stat maria emotions
stat fido emotions
stat 2.guard emotions
```

## Output Format

The command displays all 20 emotions in the following format:

```
=== Effective Emotions of <mobname> toward <yourname> ===
(Hybrid system: Mood + Relationship modifier)

Basic Emotions:
  Fear:      [Mood: 30] -> [Effective: 45] (relationship increases)
  Anger:     [Mood: 50] -> [Effective: 35] (relationship decreases)
  Happiness: [Mood: 60] -> [Effective: 60] 
  Sadness:   [Mood: 20] -> [Effective: 20] 

Social Emotions:
  Friendship:[Mood: 70] -> [Effective: 85] (relationship increases)
  Love:      [Mood: 40] -> [Effective: 55] (relationship increases)
  Trust:     [Mood: 65] -> [Effective: 80] (relationship increases)
  Loyalty:   [Mood: 50] -> [Effective: 50] 

...and so on for all 20 emotions
```

## Understanding the Output

### Mood vs Effective

- **Mood**: The mob's general emotional state (0-100)
- **Effective**: The actual emotion used in game mechanics toward you (0-100)
- The difference shows how your relationship with the mob modifies its emotions

### Relationship Modifiers

- **(relationship increases)**: Your relationship makes this emotion higher than mood
- **(relationship decreases)**: Your relationship makes this emotion lower than mood
- **(blank)**: Your relationship doesn't significantly affect this emotion

### The 20 Emotions

#### Basic Emotions
- **Fear**: Affects fleeing, cautious behavior
- **Anger**: Affects aggressive socials, attacking, combat bonuses
- **Happiness**: Affects positive socials
- **Sadness**: Affects withdrawn behavior

#### Social Emotions
- **Friendship**: Affects grouping, positive interactions
- **Love**: Affects protective behavior, following
- **Trust**: Affects trading, quest acceptance
- **Loyalty**: Affects staying in group when hurt

#### Motivational Emotions
- **Curiosity**: Affects exploration, quest offering
- **Greed**: Affects trading prices
- **Pride**: Affects behavior toward weaker entities

#### Empathic Emotions
- **Compassion**: Affects healing behavior
- **Envy**: Affects reactions to better-equipped players

#### Arousal Emotions
- **Courage**: Reduces flee probability
- **Excitement**: Affects social initiation

#### Negative/Aversive Emotions
- **Disgust**: Affects negative reactions
- **Shame**: Affects withdrawn behavior
- **Pain**: Reduces combat effectiveness (accuracy and damage)
- **Horror**: Triggers panic flee
- **Humiliation**: Affects social withdrawal

## Use Cases

### 1. Debugging Player Harassment

As mentioned in the original issue, if a player uses inappropriate socials on aggressive mobs:

```
> stat aggressive_mob emotions
```

This will show if the mob's anger/disgust toward you has increased, which would be recorded in the emotion memory system.

### 2. Understanding Shop Prices

If a shopkeeper is giving you bad prices:

```
> stat shopkeeper emotions
```

Check the Trust and Greed values - low trust or high greed means higher prices for you specifically.

### 3. Quest Debugging

If a quest giver won't give you a quest:

```
> stat questgiver emotions
```

Low Trust value (< 30) will cause the mob to refuse quest offers.

### 4. Combat Behavior Analysis

If a mob seems particularly aggressive toward you:

```
> stat enemy emotions
```

High Anger (>= 70) gives the mob extra attacks and damage bonuses against you specifically.

### 5. Relationship Building

Track how your helpful actions affect mob emotions:

```
> heal mob
> stat mob emotions
(Check if Trust, Friendship, and Love increased)
```

## Technical Details

### Hybrid System Formula

```
effective_emotion = mood_emotion + (relationship_emotion - 50) * 0.6
```

- Mood provides the base (from mob's general state)
- Relationship acts as modifier (-30 to +30 range after scaling)
- Result is clamped to 0-100

### Relationship Data Source

The relationship modifier comes from the **Emotion Memory** system:
- Circular buffer of 10 recent interactions per mob
- Each memory stores all 20 emotions at time of interaction
- Weighted by age (recent = more weight) and importance (major events = 2x weight)
- Displayed in regular `stat mob` command

### Requirements

- Must be LVL_GOD or higher
- Emotion system must be enabled (CONFIG_MOB_CONTEXTUAL_SOCIALS)
- Target must be a mob with AI data

## Related Commands

- `stat mob <mobname>`: Shows full mob stats including global mood and emotion memory
- `stat player <playername>`: Shows player stats
- `emotionconfig`: Configure emotion system parameters (if available)

## Error Messages

```
"Você precisa ser nível GOD ou superior para ver emoções efetivas."
```
You need to be LVL_GOD or higher to use this command.

```
"Effective emotions are only available for mobs."
```
You tried to use this command on a player or non-mob entity.

```
"This mob has no AI data."
```
The mob doesn't have the emotion system initialized.

```
"The emotion system is currently disabled."
```
The CONFIG_MOB_CONTEXTUAL_SOCIALS flag is disabled.

```
"Nenhum mobile assim por aqui."
```
The specified mob was not found in the world.

## Implementation Notes

- Added in response to issue about clarifying mob emotions in stat command
- Uses existing `get_effective_emotion_toward()` function from the hybrid system
- Shows effective emotions toward the command issuer specifically
- Helps administrators debug emotion-based interactions and player behavior
- Particularly useful for investigating harassment cases via emotion memory

## Example Scenario

A player reports being unable to trade with a shopkeeper. As an admin:

```
> goto shopkeeper_room
> stat shopkeeper emotions

=== Effective Emotions of shopkeeper toward God ===
(Hybrid system: Mood + Relationship modifier)

Basic Emotions:
  Fear:      [Mood: 20] -> [Effective: 20] 
  Anger:     [Mood: 30] -> [Effective: 30] 
  Happiness: [Mood: 60] -> [Effective: 60] 
  Sadness:   [Mood: 10] -> [Effective: 10] 

Social Emotions:
  Friendship:[Mood: 50] -> [Effective: 50] 
  Love:      [Mood: 40] -> [Effective: 40] 
  Trust:     [Mood: 70] -> [Effective: 70]  <-- Good! Above 60 threshold
  Loyalty:   [Mood: 50] -> [Effective: 50] 

Motivational Emotions:
  Curiosity: [Mood: 40] -> [Effective: 40] 
  Greed:     [Mood: 30] -> [Effective: 30]  <-- Normal level
  Pride:     [Mood: 45] -> [Effective: 45] 
```

In this case, the shopkeeper has normal emotions toward you. The problem might be with the other player's relationship, so you'd use `stat shopkeeper` to check their emotion memory for that player's ID.
