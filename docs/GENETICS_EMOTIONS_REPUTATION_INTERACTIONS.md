# Genetics, Emotions, and Reputation Interactions in Vitalia Reborn

## Overview

This document analyzes how the three systems (genetic pool/behavior probabilities, emotions, and reputation) interact with each other, identifying synergies, antagonisms, and potential conflicts.

## System Components

### 1. Genetics (ai_data behavior probabilities, 0-100)
- wimpy_tendency, brave_prevalence
- group_tendency, trade_tendency  
- quest_tendency, adventurer_tendency
- loot_tendency, healing_tendency
- roam_tendency
- **Nature**: Static personality traits, set at mob initialization
- **Purpose**: Define innate behavioral tendencies

### 2. Emotions (15 emotions, 0-100 scale)
- Basic: fear, anger, happiness, sadness
- Social: friendship, love, trust, loyalty
- Motivational: curiosity, greed, pride
- Empathic: compassion, envy
- Arousal: courage, excitement
- **Nature**: Dynamic, fluctuate based on events
- **Purpose**: Drive real-time behavioral responses

### 3. Reputation (0-100 scale for both players and mobs)
- **Nature**: Gradually built/lost through actions
- **Purpose**: Measure fame/notoriety and influence interactions

## Interaction Matrix

### Genetics → Emotions (INITIALIZATION)

**Synergistic Relationships:**
```
wimpy_tendency → emotion_fear (÷2)
  High wimpy = High baseline fear
  
brave_prevalence → emotion_courage (÷2)
  High bravery = High baseline courage
  
group_tendency → emotion_friendship (÷3), emotion_loyalty (÷2)
  Social genetics = Social emotions
  
trade_tendency → emotion_trust (÷2)
  Trading inclination = Trusting nature
  
quest_tendency + adventurer_tendency → emotion_curiosity (÷4)
  Exploratory genetics = Curious emotions
  
loot_tendency → emotion_greed (÷2)
  Greedy genetics = Greedy emotions
  
healing_tendency → emotion_compassion (÷2)
  Healing genetics = Compassionate emotions
  
roam_tendency → emotion_excitement (÷2)
  Wandering genetics = Excited emotions
```

**No Conflicts**: Genetics provide baseline values, emotions build upon them. The division factors (÷2, ÷3) prevent overwhelming emotional states.

### Genetics → Reputation (INDIRECT)

**Behavioral Influence:**
- High loot_tendency → More looting behavior → Potential reputation changes
- High healing_tendency → More healing → Reputation gains (if dynamic reputation enabled)
- High group_tendency → More social behavior → Potential social reputation
- High quest_tendency → More quest completion → Reputation gains

**No Direct Conflicts**: Genetics influence *what actions mobs take*, reputation tracks *outcomes of those actions*.

### Emotions → Reputation (EVENT-DRIVEN)

**Synergistic Cycles:**

1. **Healing Cycle (Positive)**
   - Mob with high emotion_compassion → More likely to help
   - Helping/healing → +reputation (if dynamic reputation enabled)
   - +reputation → More trust from others → More emotion_trust
   
2. **Combat Cycle (Warriors)**
   - High emotion_courage → Engages in combat
   - Successful kills → +reputation
   - +reputation → emotion_pride increases
   
3. **Fear Cycle (Negative)**
   - High emotion_fear → Flees combat
   - Fleeing → No reputation gains
   - Low reputation → emotion_sadness increases

**Antagonistic Relationships:**

1. **Greed vs. Generosity**
   - High emotion_greed → Less likely to give
   - Not giving → No generosity reputation gains
   - Conflict: Greedy mobs can't build reputation through charity
   
2. **Anger vs. Social Reputation**
   - High emotion_anger → Aggressive socials
   - Aggressive behavior → Damages emotion_friendship in witnesses
   - Reduces social reputation potential

### Reputation → Emotions (FEEDBACK)

**Reputation Influences Emotional State:**

1. **High Reputation (60+)**
   - Witnesses show respect → emotion_pride increases
   - More positive socials → emotion_happiness increases
   - Trust from others → emotion_trust increases
   
2. **Low Reputation (<20)**
   - Others avoid/fear mob → emotion_sadness increases
   - Negative treatment → emotion_anger may increase
   - Isolation → emotion_friendship decreases

**Potential Conflict:**
- Mob with high reputation but high emotion_envy → May sabotage own reputation
- Mob with low reputation trying to build emotion_love → Difficult due to avoidance

### Class-Based Modifiers (NEW LAYER)

**Synergistic with Genetics + Emotions:**

**Warrior:**
- Genetics: high brave_prevalence → High courage emotion → Combat actions → +Warrior class bonus → Accelerated reputation
- **Synergy**: Brave genetics → Courageous combat → Class bonus amplifies reputation gain

**Cleric:**
- Genetics: high healing_tendency → High compassion emotion → Healing actions → +Cleric class bonus → Enhanced reputation
- **Synergy**: Healing genetics → Compassionate healing → Class bonus stacks

**Thief (Evil):**
- Genetics: high loot_tendency → High greed emotion → Stealing actions → +Thief class bonus → Evil reputation (infamy)
- **Synergy**: Greedy genetics → Cunning theft → Class bonus for evil characters

**Ranger:**
- Genetics: high roam_tendency + adventurer_tendency → High curiosity/excitement → Exploration/hunting → +Ranger bonus
- **Synergy**: Wandering genetics → Exploratory behavior → Class tracking bonus

**Druid:**
- Genetics: high healing_tendency → High compassion → Nature interaction + healing → +Druid bonus
- **Synergy**: Compassionate genetics → Nature affinity → Enhanced reputation

**Bard:**
- Genetics: high group_tendency + trade_tendency → High friendship/trust → Social performance → +Bard bonus
- **Synergy**: Social genetics → Charismatic performance → Cultural reputation

**Magic User:**
- Genetics: high quest_tendency + adventurer_tendency → High curiosity → Scholarly pursuits → +Magic User bonus
- **Synergy**: Inquisitive genetics → Magical research → Academic reputation

## Alignment Integration

**Alignment modifies ALL three systems:**

1. **Genetics Interpretation:**
   - Evil: High loot_tendency = opportunistic, High brave_prevalence = ruthless
   - Good: High healing_tendency = altruistic, High group_tendency = protective
   
2. **Emotional Baselines (Initialization):**
   - Good: +happiness, +compassion, -anger
   - Evil: +anger, +greed, -compassion
   - Neutral: Balanced emotions
   
3. **Reputation Paths:**
   - Evil: Gains reputation through feared deeds (stealing, backstabbing, poisoning)
   - Good: Gains reputation through heroic deeds (healing, generosity, defeating evil)
   - Neutral: Balanced reputation paths

**No Conflict**: Alignment provides context for interpreting behaviors across all systems.

## Potential Conflicts & Resolutions

### Conflict 1: Contradictory Emotions
**Issue:** High emotion_fear + High emotion_courage simultaneously
**Resolution:** Passive decay (10% per tick) returns emotions to genetics-based baseline
**Impact:** Temporary emotional states don't permanently override personality

### Conflict 2: Reputation vs. Emotional State
**Issue:** High reputation mob with high emotion_anger behaving antisocially
**Resolution:** Both are valid - famous individuals can be angry; reputation measures fame, not likability
**Impact:** Creates complex, realistic characters (e.g., feared warlord with high reputation)

### Conflict 3: Genetics vs. Dynamic Behavior
**Issue:** Wimpy mob (high wimpy_tendency) gaining emotion_courage through combat
**Resolution:** Genetics = baseline tendency, Emotions = current state; courage can temporarily override wimpiness
**Impact:** Allows character growth while maintaining core personality

### Conflict 4: Class Bonuses Overwhelming Base Values
**Issue:** Class bonuses (+1-2) might dominate base reputation gains (+1-4)
**Resolution:** Class bonuses are proportional (25-50% boost), not replacement; base actions still primary
**Impact:** Classes enhance but don't replace core reputation mechanics

### Conflict 5: Evil Characters with High Compassion
**Issue:** Evil alignment (-compassion at init) vs. healing tendency (high compassion emotion)
**Resolution:** Alignment modifies baseline, but events can shift emotions; evil healers are rare but possible
**Impact:** Allows nuanced characters (evil cleric healing allies for tactical reasons)

## Performance Considerations

**Toggle System (CONFIG_DYNAMIC_REPUTATION + CONFIG_MOB_CONTEXTUAL_SOCIALS):**
- Emotions: Controlled by CONFIG_MOB_CONTEXTUAL_SOCIALS
- Dynamic reputation: Controlled by CONFIG_DYNAMIC_REPUTATION
- Quest reputation: Always active (baseline system)
- Genetics: Always active (initialization only, no runtime cost)

**Computational Cost:**
1. **Low**: Genetics (one-time initialization)
2. **Medium**: Reputation (modified during specific events with cooldowns)
3. **High**: Emotions (decay every tick + event updates)

**Optimization:**
- Emotion decay: Batched per tick (not per second)
- Reputation: Anti-exploit cooldowns limit frequency
- Class modifiers: Simple addition, minimal overhead

## Synergistic Benefits

1. **Realistic Character Depth:**
   - Genetics (personality) + Emotions (current state) + Reputation (social standing) = Multi-dimensional NPCs
   
2. **Dynamic Storytelling:**
   - Mob witnesses ally death → Emotions shift (sadness/anger) → Behavior changes (mourning/revenge) → Reputation impact
   
3. **Player Feedback Loop:**
   - Player heals mob → emotion_trust increases → Mob more friendly → Player gains reputation → More positive interactions
   
4. **Class Identity:**
   - Genetics predispose certain classes → Emotions reinforce class behavior → Class bonuses reward alignment → Clear progression paths

## Conclusion

**No Fundamental Conflicts**

The three systems operate on different timescales and serve distinct purposes:
- **Genetics**: Static personality foundation
- **Emotions**: Dynamic short-term state
- **Reputation**: Gradual long-term consequence

**Key Interactions:**
- Genetics → Emotions (initialization & baseline)
- Emotions → Behavior (real-time decisions)
- Behavior → Reputation (accumulated outcomes)
- Reputation → Social dynamics (how others respond)
- Class → All systems (amplification through thematic bonuses)

**Design Philosophy:**
All three systems are **complementary**, not competitive. They create a layered behavior model where:
1. Genetics define WHO the character is
2. Emotions determine HOW they FEEL right now
3. Reputation tracks WHAT they've DONE
4. Class determines HOW WELL they do it

This creates emergent complexity without conflict.
