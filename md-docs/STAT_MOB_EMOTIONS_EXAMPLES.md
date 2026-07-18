# Example Output: stat mob emotions

This document shows example outputs for the new `stat <mobname> emotions` command.

## Example 1: Neutral Relationship

A god checking a mob they haven't interacted with much:

```
> stat guard emotions

=== Effective Emotions of guard toward Immortal ===
(Hybrid system: Mood + Relationship modifier)

Basic Emotions:
  Fear:      [Mood: 30] -> [Effective: 30] 
  Anger:     [Mood: 40] -> [Effective: 40] 
  Happiness: [Mood: 55] -> [Effective: 55] 
  Sadness:   [Mood: 20] -> [Effective: 20] 

Social Emotions:
  Friendship:[Mood: 50] -> [Effective: 50] 
  Love:      [Mood: 45] -> [Effective: 45] 
  Trust:     [Mood: 60] -> [Effective: 60] 
  Loyalty:   [Mood: 50] -> [Effective: 50] 

Motivational Emotions:
  Curiosity: [Mood: 40] -> [Effective: 40] 
  Greed:     [Mood: 30] -> [Effective: 30] 
  Pride:     [Mood: 50] -> [Effective: 50] 

Empathic Emotions:
  Compassion:[Mood: 65] -> [Effective: 65] 
  Envy:      [Mood: 20] -> [Effective: 20] 

Arousal Emotions:
  Courage:   [Mood: 60] -> [Effective: 60] 
  Excitement:[Mood: 35] -> [Effective: 35] 

Negative/aversive Emotions:
  Disgust:   [Mood:  0] -> [Effective:  0] 
  Shame:     [Mood:  0] -> [Effective:  0] 
  Pain:      [Mood: 15] -> [Effective: 15] 
  Horror:    [Mood:  0] -> [Effective:  0] 
  Humiliation:[Mood: 0] -> [Effective:  0] 

Note: Effective emotions are used in combat, shopping, quests, and other interactions.
```

**Analysis**: No relationship data yet, so effective emotions match mood exactly.

---

## Example 2: Positive Relationship

A god checking a mob after helping it several times (healed, gave items, positive socials):

```
> stat shopkeeper emotions

=== Effective Emotions of shopkeeper toward Immortal ===
(Hybrid system: Mood + Relationship modifier)

Basic Emotions:
  Fear:      [Mood: 25] -> [Effective: 15] (relationship decreases)
  Anger:     [Mood: 30] -> [Effective: 20] (relationship decreases)
  Happiness: [Mood: 60] -> [Effective: 72] (relationship increases)
  Sadness:   [Mood: 15] -> [Effective: 10] (relationship decreases)

Social Emotions:
  Friendship:[Mood: 55] -> [Effective: 78] (relationship increases)
  Love:      [Mood: 40] -> [Effective: 58] (relationship increases)
  Trust:     [Mood: 65] -> [Effective: 85] (relationship increases)
  Loyalty:   [Mood: 50] -> [Effective: 68] (relationship increases)

Motivational Emotions:
  Curiosity: [Mood: 45] -> [Effective: 52] (relationship increases)
  Greed:     [Mood: 35] -> [Effective: 25] (relationship decreases)
  Pride:     [Mood: 50] -> [Effective: 58] (relationship increases)

Empathic Emotions:
  Compassion:[Mood: 70] -> [Effective: 82] (relationship increases)
  Envy:      [Mood: 20] -> [Effective: 12] (relationship decreases)

Arousal Emotions:
  Courage:   [Mood: 55] -> [Effective: 62] (relationship increases)
  Excitement:[Mood: 40] -> [Effective: 48] (relationship increases)

Negative/aversive Emotions:
  Disgust:   [Mood:  0] -> [Effective:  0] 
  Shame:     [Mood:  0] -> [Effective:  0] 
  Pain:      [Mood: 10] -> [Effective: 10] 
  Horror:    [Mood:  0] -> [Effective:  0] 
  Humiliation:[Mood: 0] -> [Effective:  0] 

Note: Effective emotions are used in combat, shopping, quests, and other interactions.
```

**Analysis**: 
- Strong positive relationship increases positive emotions (Friendship: 55→78, Trust: 65→85)
- Decreases negative emotions (Fear: 25→15, Anger: 30→20)
- Results in better shop prices (Trust 85, Greed 25) and more quest opportunities

---

## Example 3: Negative Relationship

A god checking a mob after attacking it multiple times:

```
> stat aggressive_wolf emotions

=== Effective Emotions of aggressive_wolf toward Immortal ===
(Hybrid system: Mood + Relationship modifier)

Basic Emotions:
  Fear:      [Mood: 35] -> [Effective: 52] (relationship increases)
  Anger:     [Mood: 60] -> [Effective: 82] (relationship increases)
  Happiness: [Mood: 40] -> [Effective: 22] (relationship decreases)
  Sadness:   [Mood: 25] -> [Effective: 38] (relationship increases)

Social Emotions:
  Friendship:[Mood: 45] -> [Effective: 22] (relationship decreases)
  Love:      [Mood: 35] -> [Effective: 18] (relationship decreases)
  Trust:     [Mood: 50] -> [Effective: 28] (relationship decreases)
  Loyalty:   [Mood: 40] -> [Effective: 25] (relationship decreases)

Motivational Emotions:
  Curiosity: [Mood: 30] -> [Effective: 22] (relationship decreases)
  Greed:     [Mood: 25] -> [Effective: 25] 
  Pride:     [Mood: 55] -> [Effective: 48] (relationship decreases)

Empathic Emotions:
  Compassion:[Mood: 50] -> [Effective: 32] (relationship decreases)
  Envy:      [Mood: 30] -> [Effective: 42] (relationship increases)

Arousal Emotions:
  Courage:   [Mood: 65] -> [Effective: 72] (relationship increases)
  Excitement:[Mood: 45] -> [Effective: 38] (relationship decreases)

Negative/aversive Emotions:
  Disgust:   [Mood: 15] -> [Effective: 28] (relationship increases)
  Shame:     [Mood:  5] -> [Effective:  8] (relationship increases)
  Pain:      [Mood: 40] -> [Effective: 48] (relationship increases)
  Horror:    [Mood: 20] -> [Effective: 32] (relationship increases)
  Humiliation:[Mood:10] -> [Effective: 18] (relationship increases)

Note: Effective emotions are used in combat, shopping, quests, and other interactions.
```

**Analysis**:
- Strong negative relationship increases hostile emotions (Anger: 60→82, Fear: 35→52)
- Decreases positive emotions (Trust: 50→28, Friendship: 45→22)
- **Combat impact**: Anger 82 (>70) means 25% chance for extra attack + 15% damage bonus against you
- High Fear + Anger combination makes mob unpredictable (might flee or fight harder)

---

## Example 4: Mixed Relationship (Harassment Case)

A god investigating a harassment report where a player used "cuddle" on an aggressive mob:

```
> stat aggressive_bear emotions

=== Effective Emotions of aggressive_bear toward Immortal ===
(Hybrid system: Mood + Relationship modifier)

Basic Emotions:
  Fear:      [Mood: 40] -> [Effective: 48] (relationship increases)
  Anger:     [Mood: 70] -> [Effective: 88] (relationship increases)
  Happiness: [Mood: 20] -> [Effective: 15] (relationship decreases)
  Sadness:   [Mood: 30] -> [Effective: 35] (relationship increases)

Social Emotions:
  Friendship:[Mood: 30] -> [Effective: 18] (relationship decreases)
  Love:      [Mood: 25] -> [Effective: 32] (relationship increases)
  Trust:     [Mood: 35] -> [Effective: 25] (relationship decreases)
  Loyalty:   [Mood: 40] -> [Effective: 35] (relationship decreases)

Motivational Emotions:
  Curiosity: [Mood: 35] -> [Effective: 42] (relationship increases)
  Greed:     [Mood: 40] -> [Effective: 45] (relationship increases)
  Pride:     [Mood: 60] -> [Effective: 52] (relationship decreases)

Empathic Emotions:
  Compassion:[Mood: 40] -> [Effective: 32] (relationship decreases)
  Envy:      [Mood: 25] -> [Effective: 32] (relationship increases)

Arousal Emotions:
  Courage:   [Mood: 70] -> [Effective: 75] (relationship increases)
  Excitement:[Mood: 30] -> [Effective: 25] (relationship decreases)

Negative/aversive Emotions:
  Disgust:   [Mood: 35] -> [Effective: 52] (relationship increases)
  Shame:     [Mood: 10] -> [Effective: 18] (relationship increases)
  Pain:      [Mood: 25] -> [Effective: 25] 
  Horror:    [Mood: 15] -> [Effective: 22] (relationship increases)
  Humiliation:[Mood:20] -> [Effective: 35] (relationship increases)

Note: Effective emotions are used in combat, shopping, quests, and other interactions.

> stat aggressive_bear

[... normal stat output shows ...]

Emotion Memory: (3/10 slots used)
  [ 5 min ago] Cansian Social+(cuddle)
      Emotions: Anger:85 Happy:15 Sad:32 Friend:18 Love:32 Trust:25
  [ 8 min ago] Cansian Attacked
      Emotions: Anger:70 Fear:45 Horror:20
  [12 min ago] Cansian Social-(taunt)
      Emotions: Anger:75 Disgust:48 Humiliation:32
```

**Analysis**:
- Emotion Memory shows the player "Cansian" used inappropriate social "cuddle" on aggressive mob
- This increased Love slightly (25→32) but also increased Disgust (35→52) and Humiliation (20→35)
- High Anger (88) with high Disgust (52) indicates very negative reaction to inappropriate advances
- Evidence of harassment: using positive/intimate socials on hostile mobs to provoke reaction
- The emotional response pattern confirms inappropriate player behavior

---

## Example 5: Shop Trading Impact

Comparing two players' relationships with same shopkeeper:

**Player A (Good Customer)**:
```
> stat shopkeeper emotions
Trust:     [Mood: 60] -> [Effective: 85] (relationship increases)
Greed:     [Mood: 40] -> [Effective: 28] (relationship decreases)
Friendship:[Mood: 50] -> [Effective: 72] (relationship increases)

Result: Gets 10% discount (Trust >= 60, Friendship >= 60)
```

**Player B (Problem Customer)**:
```
> stat shopkeeper emotions
Trust:     [Mood: 60] -> [Effective: 32] (relationship decreases)
Greed:     [Mood: 40] -> [Effective: 58] (relationship increases)
Friendship:[Mood: 50] -> [Effective: 28] (relationship decreases)

Result: Gets refused service (Trust < 30) or pays 15% more (high Greed)
```

**Explanation**: Same shopkeeper, different relationships = different treatment. The hybrid system provides personalized NPC behavior.

---

## Error Examples

### Permission Denied
```
> stat guard emotions
Você precisa ser nível GOD ou superior para ver emoções efetivas.
```

### Not a Mob
```
> stat player emotions
Effective emotions are only available for mobs.
```

### System Disabled
```
> stat guard emotions
The emotion system is currently disabled.
```

### Mob Not Found
```
> stat nonexistent emotions
Nenhum mobile assim por aqui.
```

---

## Command Comparison

### Regular stat mob
```
> stat guard
[... shows all mob stats, including mood emotions and emotion memory ...]
Emotions:
  Basic: Fear[30] Anger[40] Happiness[55] Sadness[20]
  Social: Friendship[50] Love[45] Trust[60] Loyalty[50]
  [... etc ...]

Emotion Memory: (2/10 slots used)
  [ 2 min ago] PlayerName Healed
      Emotions: [snapshot at time of healing]
```

### New stat emotions
```
> stat guard emotions
[... shows effective emotions toward YOU specifically ...]
  Trust:     [Mood: 60] -> [Effective: 85] (relationship increases)
  [Shows how YOUR relationship modifies each emotion]
```

**Key Difference**: Regular stat shows global mood; stat emotions shows personalized feelings toward you.
