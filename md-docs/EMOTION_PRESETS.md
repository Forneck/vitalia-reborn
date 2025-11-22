# Emotion Configuration Presets (Global World Mood)

## Overview

Emotion configuration presets in CEDIT set the **global behavioral thresholds** that apply to ALL mobs in the game world. Think of these as setting the "world mood" or "behavioral standards" for your entire MUD.

These presets are **NOT** for individual mob personalities (use `EmotionProfile` in mob files for that). Instead, they define what emotion levels are considered "high" or "low" across the entire server.

## Accessing Presets

In CEDIT, navigate to:
```
> cedit
Main Menu -> E) Emotion System Configuration -> P) Load Configuration Preset
```

## How Presets Work

When you load a preset, it configures the global thresholds that determine mob behavior. For example:

- **Balanced Preset**: `trade_trust_high_threshold = 60`
  - Any mob with trust ≥60 gives better shop prices
  
- **Aggressive Preset**: `trade_trust_high_threshold = 70`
  - Now mobs need trust ≥70 to give better prices (harder to get discounts)

- **Mercantile Preset**: `trade_trust_high_threshold = 50`
  - Now mobs only need trust ≥50 (easier to get discounts)

**The preset affects ALL mobs**, but each mob's individual emotions determine whether they meet the threshold.

## Available Presets

### 1. Aggressive - Harsh World
**World Mood**: Combat-oriented, difficult, greedy

**Effect on ALL Mobs**:
- Harder to earn trust bonuses in shops (threshold: 70 vs 60)
- Mobs refuse service less often (threshold: 40 vs 30)
- Mobs are more greedy with prices (threshold: 60 vs 70)
- Harder to get friendship discounts (threshold: 75 vs 70)
- Less curious about quests (threshold: 75 vs 70)
- Rarely happy enough for positive socials (threshold: 80 vs 70)
- More likely to show anger (threshold: 60 vs 70)
- Harder to stay loyal in groups (threshold: 75 vs 70)
- Easier to abandon groups (threshold: 40 vs 30)

**Best For**: Challenging servers, hardcore gameplay, PvP-focused worlds

### 2. Defensive - Cautious World
**World Mood**: Fearful, cautious, distrusting

**Effect on ALL Mobs**:
- Easier to get trust bonuses (threshold: 50 vs 60)
- More likely to refuse service (threshold: 20 vs 30)
- Less greedy (threshold: 80 vs 70)
- Easier friendship discounts (threshold: 60 vs 70)
- Less curious about quests (threshold: 80 vs 70)
- More likely to be sad/withdrawn (threshold: 60 vs 70)
- Groups less stable (easier abandon: 35 vs 30)

**Best For**: Survival themes, post-apocalyptic settings, paranoid atmospheres

### 3. Balanced - Standard World (DEFAULT)
**World Mood**: Neutral, fair, standard gameplay

**Effect on ALL Mobs**:
- All thresholds at default balanced values
- Trust high: 60, low: 30
- Greed high: 70
- Friendship high: 70
- Curiosity high: 70
- Social emotions high: 70
- Love follow: 80
- Loyalty high: 70, low: 30

**Best For**: General purpose, new servers, mixed gameplay styles

### 4. Sensitive - Emotional World
**World Mood**: Reactive, emotional, responsive

**Effect on ALL Mobs**:
- Very responsive to emotions (lower thresholds = easier to trigger)
- Easy trust bonuses (threshold: 55 vs 60)
- More curious about quests (threshold: 65 vs 70)
- More emotional socials (thresholds: 60 vs 70)
- Easier to follow when in love (threshold: 70 vs 80)
- Very social groups (easier join: 60 vs 70)

**Best For**: Roleplay-heavy servers, social gameplay, drama-focused MUDs

### 5. Mercantile - Trading-Focused World
**World Mood**: Commerce-friendly, trusting, fair

**Effect on ALL Mobs**:
- **VERY** easy trust bonuses for trading (threshold: 50 vs 60)
- Rarely refuse service (threshold: 20 vs 30)
- Not very greedy (threshold: 80 vs 70)
- Easy friendship discounts (threshold: 55 vs 70)
- Friendly socials (threshold: 65 vs 70)
- Moderate group behavior

**Best For**: Trading-focused servers, merchant guilds, economy-heavy gameplay

### 6. Hermit - Isolated World
**World Mood**: Antisocial, withdrawn, distrusting

**Effect on ALL Mobs**:
- **VERY** hard to get trust bonuses (threshold: 75 vs 60)
- Often refuse service (threshold: 15 vs 30)
- More greedy (threshold: 65 vs 70)
- Very hard to get discounts (threshold: 80 vs 70)
- Not curious about quests (threshold: 85 vs 70)
- Often refuse quests (threshold: 15 vs 30)
- Rarely happy (threshold: 85 vs 70)
- Often withdrawn/sad (threshold: 55 vs 70)
- Rarely follows (threshold: 90 vs 80)
- Very hard to join groups (threshold: 80 vs 70)
- Often abandon groups (threshold: 25 vs 30)

**Best For**: Lonely/isolated worlds, survival horror, lone wolf gameplay

### 7. Loyal - Group-Focused World
**World Mood**: Cooperative, faithful, social

**Effect on ALL Mobs**:
- Easy trust bonuses (threshold: 55 vs 60)
- Friendly and helpful (threshold: 65 vs 70)
- **VERY** easy to stay in groups (threshold: 60 vs 70)
- Hard to abandon groups (threshold: 35 vs 30)
- **VERY** easy to join groups (threshold: 55 vs 70)
- Not envious (threshold: 80 vs 70)
- More happy socials (threshold: 65 vs 70)
- Easier to follow (threshold: 70 vs 80)

**Best For**: Party-based gameplay, clan-focused servers, cooperative MUDs

## Understanding the Impact

### Example: Shop Pricing

**Scenario**: A shopkeeper mob has `emotion_trust = 65`

- **Aggressive Preset** (trust_high = 70): Mob doesn't meet threshold → Normal prices
- **Balanced Preset** (trust_high = 60): Mob meets threshold → 10% discount
- **Mercantile Preset** (trust_high = 50): Mob meets threshold → 10% discount

**Same mob, different world moods = different behavior!**

### Example: Group Loyalty

**Scenario**: A guard mob has `emotion_loyalty = 65`

- **Hermit Preset** (loyalty_high = 80): Mob doesn't meet threshold → May abandon when hurt
- **Balanced Preset** (loyalty_high = 70): Mob doesn't meet threshold → May abandon when hurt  
- **Loyal Preset** (loyalty_high = 60): Mob meets threshold → Stays with group (−30 flee chance)

### Example: Quest Refusal

**Scenario**: A questmaster mob has `emotion_trust = 28`

- **Hermit Preset** (trust_low = 15): Mob doesn't meet threshold → Offers quests
- **Balanced Preset** (trust_low = 30): Mob meets threshold → Refuses quest
- **Mercantile Preset** (trust_low = 20): Mob meets threshold → Refuses quest

## Combining with Mob Profiles

Presets work WITH the `EmotionProfile` system in mob files:

1. **EmotionProfile** (in mob file): Sets individual mob's emotion levels (0-100)
2. **CEDIT Preset**: Sets global thresholds for what's considered "high" or "low"

**Example**:
```
Mob File: EmotionProfile: 4 (Sensitive)
  → Mob has trust = 65, friendship = 75, loyalty = 75

Balanced Preset:
  → trust_high = 60 (mob qualifies for discount)
  → friendship_high = 70 (mob qualifies for extra discount)
  → loyalty_high = 70 (mob stays with group)

Aggressive Preset:
  → trust_high = 70 (mob doesn't qualify for discount)
  → friendship_high = 75 (mob just qualifies for extra discount)
  → loyalty_high = 75 (mob just qualifies to stay with group)
```

Same mob personality, different world standards!

## Switching Presets

You can change presets at any time through CEDIT. Changes take effect immediately for all mobs.

**Use Cases**:
- Start server with Balanced, switch to Aggressive for a war event
- Use Mercantile during a trading festival
- Switch to Hermit during an apocalypse storyline
- Use Loyal during a siege where groups must stick together

## Preset Configuration Details

Each preset configures **66 total values**:
- 20 Visual indicator thresholds (when emotions show in descriptions)
- 10 Combat flee behavior values (fear/courage/horror)
- 12 Pain system values (damage thresholds and pain amounts)
- 10 Memory system values (memory weights and age thresholds)
- **16 NEW Behavior thresholds** (trading, quests, socials, groups)

The new behavior thresholds are:

### Trading (4 values)
- `trade_trust_high_threshold`: Trust for better shop prices
- `trade_trust_low_threshold`: Trust for service refusal
- `trade_greed_high_threshold`: Greed for price increases
- `trade_friendship_high_threshold`: Friendship for discounts

### Quests (4 values)
- `quest_curiosity_high_threshold`: Curiosity for quest offers
- `quest_loyalty_high_threshold`: Loyalty for remembering helpers
- `quest_trust_high_threshold`: Trust for better rewards
- `quest_trust_low_threshold`: Trust for quest refusal

### Social Initiation (4 values)
- `social_happiness_high_threshold`: Happiness for positive socials
- `social_anger_high_threshold`: Anger for negative socials
- `social_sadness_high_threshold`: Sadness for withdrawal
- `social_love_follow_threshold`: Love for following players

### Group Behavior (4 values)
- `group_loyalty_high_threshold`: Loyalty to stay when hurt
- `group_loyalty_low_threshold`: Loyalty threshold for abandoning
- `group_friendship_high_threshold`: Friendship for joining groups
- `group_envy_high_threshold`: Envy for refusing better players

## Summary

**Global Presets** = World-wide behavioral standards affecting ALL mobs
**Mob Profiles** = Individual personality of specific mobs

Choose your preset based on the overall "feel" you want for your world!
