# Emotional Profiles for Mobs

## Overview

Emotional profiles provide pre-configured personality archetypes for mobs that align with the emotion-driven behavior thresholds. By setting an emotional profile, builders can create mobs with consistent personalities without manually configuring all 15+ emotion values.

## Using Emotional Profiles

To set an emotional profile for a mob, add the `EmotionProfile` field to the mob file:

```
EmotionProfile: 1
```

Valid profile values are 0-7:
- **0**: NEUTRAL (default) - balanced emotions, genetics-based initialization
- **1**: AGGRESSIVE - hostile, untrusting, greedy
- **2**: DEFENSIVE - cautious, fearful, distrustful
- **3**: BALANCED - moderate all emotions
- **4**: SENSITIVE - empathetic, friendly, trusting
- **5**: CONFIDENT - brave, proud, loyal
- **6**: GREEDY - acquisitive, envious, selfish
- **7**: LOYAL - faithful, trusting, friendly

## Profile Details

### 1. AGGRESSIVE Profile (EmotionProfile: 1)

**Personality**: Hostile, untrusting, materialistic

**Emotion Settings**:
- **Anger**: 75 (HIGH) - Triggers negative socials and aggressive behavior
- **Trust**: 25 (LOW) - Refuses service to players in shops, refuses quests
- **Friendship**: 25 (LOW) - Unlikely to join groups
- **Loyalty**: 25 (LOW) - Abandons groups easily when hurt/scared
- **Compassion**: 15 (Very low) - Not helpful
- **Greed**: 75 (HIGH) - Expensive shop prices (15% markup)
- **Courage**: 70 (HIGH) - Stands ground in combat
- **Happiness**: 20 (Low) - Less likely to initiate positive socials

**Best For**: Bandits, aggressive merchants, hostile guards, antagonists

### 2. DEFENSIVE Profile (EmotionProfile: 2)

**Personality**: Cautious, fearful, untrusting

**Emotion Settings**:
- **Fear**: 70 (HIGH) - Flees earlier in combat
- **Trust**: 25 (LOW) - Refuses service, refuses quests
- **Courage**: 20 (LOW) - Very cautious
- **Curiosity**: 25 (LOW) - Avoids accepting quests
- **Sadness**: 50 (Moderate) - Somewhat withdrawn

**Best For**: Scared villagers, paranoid NPCs, victims, timid creatures

### 3. BALANCED Profile (EmotionProfile: 3)

**Personality**: Moderate, well-adjusted, stable

**Emotion Settings**:
- All emotions set to moderate values (40-55)
- No extreme behaviors triggered
- Normal shop prices
- Normal grouping behavior

**Best For**: General-purpose NPCs, neutral townspeople, balanced characters

### 4. SENSITIVE Profile (EmotionProfile: 4)

**Personality**: Empathetic, friendly, trusting, helpful

**Emotion Settings**:
- **Compassion**: 75 (HIGH) - Very helpful
- **Friendship**: 75 (HIGH) - Joins groups easily (+15% chance)
- **Trust**: 65 (HIGH) - Good shop prices (10% discount), trusts with quests
- **Loyalty**: 75 (HIGH) - Stays in group even when hurt
- **Happiness**: 75 (HIGH) - Initiates positive socials more (+15% chance)
- **Anger**: 15 (LOW) - Not aggressive
- **Greed**: 20 (LOW) - Generous prices

**Best For**: Healers, helpers, friendly NPCs, companions, support characters

### 5. CONFIDENT Profile (EmotionProfile: 5)

**Personality**: Brave, proud, loyal, adventurous

**Emotion Settings**:
- **Courage**: 80 (Very high) - Rarely flees
- **Pride**: 75 (HIGH) - Proud behavior
- **Fear**: 15 (LOW) - Fearless
- **Loyalty**: 75 (HIGH) - Stays with group
- **Trust**: 65 (HIGH) - Trusting
- **Curiosity**: 65 (Above average) - Accepts quests

**Best For**: Leaders, warriors, guardians, confident characters

### 6. GREEDY Profile (EmotionProfile: 6)

**Personality**: Acquisitive, envious, selfish

**Emotion Settings**:
- **Greed**: 80 (Very high) - Expensive prices (15% markup)
- **Envy**: 75 (HIGH) - Refuses to group with better-equipped players
- **Compassion**: 15 (LOW) - Selfish
- **Friendship**: 30 (LOW) - Not friendly
- **Loyalty**: 30 (LOW) - Self-serving, abandons groups
- **Curiosity**: 70 (HIGH) - Seeks opportunities/quests

**Best For**: Merchants, thieves, selfish characters, opportunists

### 7. LOYAL Profile (EmotionProfile: 7)

**Personality**: Faithful, trustworthy, friendly

**Emotion Settings**:
- **Loyalty**: 85 (Very high) - Never abandons group (-30 flee modifier)
- **Trust**: 75 (HIGH) - Great shop prices (10% discount/bonus)
- **Friendship**: 80 (HIGH) - Joins groups easily (+20% chance)
- **Love**: 50 (Moderate-high) - May follow loved players if reaches 80+
- **Compassion**: 65 (HIGH) - Helpful
- **Greed**: 20 (LOW) - Fair prices

**Best For**: Companions, guards, faithful servants, loyal followers

## How Profiles Interact with Genetics

When an emotional profile is set (non-NEUTRAL), the profile is applied first, then genetics-based adjustments are added on top:

1. Profile sets base emotions
2. Genetics provide fine-tuning:
   - `wimpy_tendency` increases fear slightly
   - `brave_prevalence` increases courage slightly
   - `group_tendency` increases friendship and loyalty slightly

This allows profiles to provide consistent personalities while still allowing genetics to influence behavior.

## Behavior Threshold Reference

The profiles are designed to align with emotion-driven behavior thresholds:

### Trading Behavior (shop.c)
- **Trust ≥60**: Better shop prices (10% discount/bonus)
- **Trust <30**: Shopkeeper refuses service
- **Greed ≥70**: Increased prices (15% markup)
- **Friendship ≥70**: Discounts (15% discount/bonus)

### Quest Behavior (quest.c)
- **Trust <30**: Refuses to give quests
- **Trust ≥60**: Better quest rewards (+15% gold)
- **Curiosity ≥70**: More likely to offer quests

### Social Initiation (mobact.c)
- **Happiness ≥70**: More positive socials (+15% chance)
- **Anger ≥70**: More negative socials (+10% chance)
- **Sadness ≥70**: Withdrawn, less social (-15% chance)
- **Love ≥80**: Follows loved players

### Group Behavior (mobact.c, fight.c)
- **Friendship ≥70**: More likely to join groups (+15-20% chance)
- **Loyalty ≥70**: Stays in group when hurt (-30 abandon chance)
- **Loyalty <30**: Abandons group when scared (+30 abandon chance)
- **Envy ≥70**: Refuses to group with better-equipped players

## Examples

### Friendly Shopkeeper
```
EmotionProfile: 4
```
Creates a sensitive, helpful shopkeeper who gives good prices and is friendly to customers.

### Hostile Bandit
```
EmotionProfile: 1
```
Creates an aggressive bandit with high prices, low trust, and hostile behavior.

### Loyal Guard
```
EmotionProfile: 7
```
Creates a faithful guard who stays with groups and protects others.

### Scared Villager
```
EmotionProfile: 2
```
Creates a fearful villager who is cautious and doesn't trust strangers.

## Notes

- If `EmotionProfile` is not specified or set to 0, mobs use the traditional genetics-based emotion initialization
- Profiles provide starting emotions; they can change during gameplay through interactions
- The CONFIG_MOB_CONTEXTUAL_SOCIALS flag must be enabled for emotion-driven behaviors to work
- All thresholds are configurable via CEDIT for server-specific tuning
