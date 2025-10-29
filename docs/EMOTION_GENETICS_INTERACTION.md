# Emotion and Genetics Interaction System

This document describes how mob emotions and genetic traits interact in the Vitalia Reborn emotion system.

## Overview

The emotion system (15 emotions, 0-100 scale) is initialized based on genetic traits and then dynamically updated based on events. Genetics provide the baseline personality, while emotions fluctuate based on experiences.

## Genetics → Emotions Mapping (Initialization)

### Fear & Courage System
- **emotion_fear** ← `wimpy_tendency / 2` (0-50 range)
  - Higher wimpy_tendency = more fearful baseline
  - Affects: fleeing behavior, cautious socials
- **emotion_courage** ← `brave_prevalence / 2` (0-50 range)
  - Higher brave_prevalence = braver baseline
  - Affects: standing ground, aggressive behavior

**Interaction:** Fear and courage are inversely related to genetics but can both be present at different levels.

### Social Emotions
- **emotion_friendship** ← `group_tendency / 3` (0-33 range)
  - Social mobs start more friendly
- **emotion_loyalty** ← `group_tendency / 2` (0-50 range)
  - High group_tendency = more loyal to groups
- **emotion_trust** ← `trade_tendency / 2` (0-50 range)
  - Trading-oriented mobs trust more easily

**Interaction:** These three emotions work together - high group_tendency creates mobs that form strong social bonds.

### Motivational Emotions
- **emotion_curiosity** ← `(quest_tendency + adventurer_tendency) / 4` (0-50 range)
  - Quest-oriented and adventurous mobs are naturally curious
  - Affects: exploration, quest acceptance
- **emotion_greed** ← `loot_tendency / 2` (0-50 range)
  - Looting behavior correlates with greed
  - Affects: item hoarding, trade behavior
- **emotion_pride** ← `rand_number(10, 40)` + influenced by reputation
  - Not directly genetic but affected by achievements

### Empathic Emotions
- **emotion_compassion** ← `healing_tendency / 2` (0-50 range)
  - Healer mobs are more compassionate
  - Affects: mourning, comforting socials
- **emotion_envy** ← `rand_number(0, 25)` + influenced by greed
  - Partially random, enhanced by high greed

### Arousal Emotions
- **emotion_excitement** ← `roam_tendency / 2` (0-50 range)
  - Roaming mobs have higher baseline excitement
  - Affects: activity level, restlessness

## Emotions → Genetics Feedback (Not Yet Implemented)

Currently, emotions do NOT modify genetics. This is a one-way initialization. Potential future enhancements:

1. **High courage over time** could increase `brave_prevalence`
2. **Persistent fear** could increase `wimpy_tendency`
3. **Strong friendship emotions** could boost `group_tendency`
4. **High greed** could increase `loot_tendency`

## Alignment Influence

Alignment modifies emotion baselines after genetic initialization:

### Good Alignment
- `emotion_happiness` += 30-50 (base happier)
- `emotion_anger` = 0-20 (less angry)
- `emotion_compassion` += 20 (more compassionate)

### Evil Alignment
- `emotion_anger` += 30-50 (base angrier)
- `emotion_happiness` = 0-20 (less happy)
- `emotion_greed` += 20 (greedier)

### Neutral Alignment
- Balanced emotions (10-30 range)

## Event-Driven Emotion Changes

After initialization, emotions change based on:

1. **Combat Events**
   - Being attacked: ↑ fear, ↑ anger, ↓ happiness, ↓ trust
   - Attacking: ↑ courage, ↓ fear
   - High brave_prevalence: reduces fear increase

2. **Social Events**
   - Receiving healing: ↑ happiness, ↑ trust, ↑ friendship
   - Witnessing death: ↑ sadness, ↑ fear, ↑ anger (if high loyalty)
   - Player socials: affect based on social type

3. **Passive Decay**
   - Emotions gradually return toward genetic/alignment baselines
   - Prevents permanent emotional states

## Genetic-Emotion Behavioral Synergies

### Example 1: Brave Warrior
```
Genetics: brave_prevalence=80, wimpy_tendency=10
Initial Emotions: courage=40, fear=5
After combat: courage increases further, fear barely rises
Behavior: Stands ground, performs aggressive socials
```

### Example 2: Social Healer
```
Genetics: group_tendency=80, healing_tendency=70
Initial Emotions: friendship=26, loyalty=40, compassion=35
After healing ally: happiness+20, trust+15
Behavior: Comforts wounded, mourns dead companions
```

### Example 3: Greedy Looter
```
Genetics: loot_tendency=90, trade_tendency=30
Initial Emotions: greed=45, trust=15
Sees high-reputation player: envy increases
Behavior: Envious socials, item hoarding
```

## Contextual Social Selection Priority

When choosing socials (mobact.c):
1. **Extreme emotions** (70+) override everything
2. **Moderate emotions** (40-70) combine with reputation/alignment
3. **Genetics baseline** influences through emotion initialization
4. **Position/reputation** fine-tune the selection

Example Decision Tree:
```
IF emotion_fear >= 70 AND emotion_courage < 40
  → fearful_socials (cower, tremble)
ELSE IF emotion_pride >= 75
  → proud_socials (strut, flex)
ELSE IF target_reputation >= 60 AND emotion_friendship >= 50
  → positive_socials (bow, smile)
```

## Summary

**One-Way Flow (Current):**
Genetics → Initial Emotions → Dynamic Events → Updated Emotions → Behavior

**Potential Two-Way Flow (Future):**
Genetics ↔ Emotions (persistent emotions could modify genetics over time)

The system creates emergent personality: genetics provide the foundation, alignment adds moral tendencies, and experiences shape moment-to-moment behavior.
