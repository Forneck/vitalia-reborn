# Emotion Contagion System Documentation

## Status

**Status**: ✅ Implemented (February 2026)  
**Source**: `src/utils.c` — `update_mob_emotion_contagion()` (lines 6306–6455)  
**Integration**: `src/mobact.c` — called every 4 seconds during mob update cycle

## Overview

The Emotion Contagion system in Vitalia Reborn simulates how emotions spread between mobs (NPCs) in the same room, creating emergent group dynamics and realistic emotional responses. This system is inspired by real-world psychological phenomena where emotions are "caught" from others in social situations.

**Key Features:**
- Emotions spread between mobs in the same room
- Stronger contagion within groups (party members)
- Leader emotions have 2x influence on followers
- Modulated by Emotional Intelligence (EI)
- Performance-optimized for crowded areas (>20 NPCs)
- Four primary emotions spread: Fear, Happiness, Anger, and Excitement

## Activation

Emotion contagion is automatically triggered during mob updates when:
1. `CONFIG_MOB_CONTEXTUAL_SOCIALS` is enabled
2. Mob has valid `ai_data` structure
3. Mob is in a valid room
4. Room has fewer than 20 NPCs (performance threshold)

The contagion update is called from `mobact.c` during the emotion processing cycle (every 4 seconds):

```c
/* Emotion contagion - mobs influence each other's emotions */
if (CONFIG_MOB_CONTEXTUAL_SOCIALS && ch->ai_data) {
    update_mob_emotion_contagion(ch);
}
```

## How Emotion Contagion Works

### Core Mechanism

Emotions "spread" from mob to mob through a weighted averaging system. Each mob in a room contributes to an emotional "atmosphere" that influences all other mobs present.

**Formula (General Contagion):**
```
avg_emotion = SUM(all_nearby_mobs.emotion) / mob_count
transfer_amount = (avg_emotion × contagion_rate) / 100
adjusted_transfer = adjust_emotion(mob, transfer_amount)  // Applies EI modulation
final_emotion = CLAMP(current_emotion + adjusted_transfer, 0, 100)
```

### Contagion Rates by Emotion

Different emotions have different contagion rates, reflecting their psychological "infectiousness":

| Emotion | Crowd Contagion Rate | Group Contagion Rate | Leader Influence Rate |
|---------|---------------------|---------------------|----------------------|
| **Fear** | 5-10% | 12-20% | 15-25% |
| **Happiness** | 8-15% | 10-15% | 12-20% |
| **Anger** | 3-7% | N/A | 10-18% |
| **Excitement** | 7-12% | N/A | N/A |

**Why these rates?**
- **Fear** spreads most strongly (especially in groups) - panic is contagious
- **Happiness** spreads well in crowds (social bonding)
- **Anger** spreads moderately (tension escalation)
- **Excitement** spreads in active environments

## Three Layers of Contagion

### 1. Crowd Contagion (All Nearby Mobs)

This is the baseline emotional influence from all mobs in the same room, regardless of group affiliation.

#### Fear Contagion (Crowd)
```c
if (mob_count > 0 && total_fear > 0) {
    int avg_fear = total_fear / mob_count;
    int fear_transfer = (avg_fear * rand_number(5, 10)) / 100;
    
    // Bonus in crowds (3+ mobs)
    if (mob_count >= 3)
        fear_transfer += rand_number(1, 3);
    
    adjust_emotion(mob, &mob->ai_data->emotion_fear, fear_transfer);
}
```

**Example:**
- Room has 4 mobs with fear levels: [60, 70, 40, 80]
- Average fear = (60+70+40+80)/4 = 62.5
- Transfer to each mob = 62.5 × 7% (random 5-10%) = ~4.4 fear
- Crowd bonus (4 mobs >= 3) = +2 fear
- **Total transfer: ~6-7 fear per tick**

#### Happiness Contagion (Crowd)
```c
if (mob_count >= 2 && total_happiness > 0) {
    int avg_happiness = total_happiness / mob_count;
    int happiness_transfer = (avg_happiness * rand_number(8, 15)) / 100;
    
    // Stronger in larger crowds
    if (mob_count >= 4)
        happiness_transfer += rand_number(2, 4);
    
    adjust_emotion(mob, &mob->ai_data->emotion_happiness, happiness_transfer);
}
```

**Example:**
- Room has 5 mobs with happiness: [70, 80, 60, 90, 50]
- Average happiness = 70
- Transfer = 70 × 12% = 8.4 happiness
- Large crowd bonus (5 >= 4) = +3 happiness
- **Total transfer: ~11-12 happiness per tick**

#### Anger Contagion (Crowd)
```c
if (mob_count > 0 && total_anger > 0) {
    int avg_anger = total_anger / mob_count;
    int anger_transfer = (avg_anger * rand_number(3, 7)) / 100;
    adjust_emotion(mob, &mob->ai_data->emotion_anger, anger_transfer);
}
```

**Example:**
- Room has 3 mobs with anger: [50, 65, 80]
- Average anger = 65
- Transfer = 65 × 5% = ~3.25 anger
- **Total transfer: ~3 anger per tick**

#### Excitement Contagion (Crowd)
```c
if (mob_count >= 2 && total_excitement > 0) {
    int avg_excitement = total_excitement / mob_count;
    int excitement_transfer = (avg_excitement * rand_number(7, 12)) / 100;
    adjust_emotion(mob, &mob->ai_data->emotion_excitement, excitement_transfer);
}
```

**Example:**
- Room has 3 mobs with excitement: [60, 75, 80]
- Average excitement = 71.7
- Transfer = 71.7 × 9.5% = ~6.8 excitement
- **Total transfer: ~7 excitement per tick**

### 2. Group Contagion (Same Group Members)

Mobs in the same group (party) have stronger emotional bonds, leading to enhanced contagion effects.

#### Fear Contagion (Group)
```c
if (group_member_count > 0) {
    int avg_group_fear = group_fear / group_member_count;
    int group_fear_transfer = (avg_group_fear * rand_number(12, 20)) / 100;
    adjust_emotion(mob, &mob->ai_data->emotion_fear, group_fear_transfer);
}
```

**Why stronger in groups?**
- Group members are more emotionally connected
- Fear spreads rapidly in coordinated units (military psychology)
- Panic can cascade through a party

**Example:**
- Group of 4 mobs with fear: [50, 60, 70, 40]
- Average group fear = 55
- Transfer = 55 × 16% (random 12-20%) = ~8.8 fear
- **Total transfer: ~9 fear per tick**
- This is **50% stronger** than crowd contagion for the same emotion level

#### Happiness Contagion (Group)
```c
if (group_member_count > 0) {
    int avg_group_happiness = group_happiness / group_member_count;
    int group_happiness_transfer = (avg_group_happiness * rand_number(10, 15)) / 100;
    adjust_emotion(mob, &mob->ai_data->emotion_happiness, group_happiness_transfer);
}
```

**Why in groups?**
- Social bonding creates shared positive emotions
- Success of group members boosts morale of all

**Example:**
- Group of 3 mobs with happiness: [70, 80, 60]
- Average = 70
- Transfer = 70 × 12.5% = ~8.75 happiness
- **Total transfer: ~9 happiness per tick**

### 3. Leader Influence (2x Stronger)

Group leaders have disproportionate emotional influence on followers, simulating leadership dynamics.

#### Leader's Fear Influence
```c
if (has_leader && leader_fear > 50) {
    int leader_fear_transfer = (leader_fear * rand_number(15, 25)) / 100;
    adjust_emotion(mob, &mob->ai_data->emotion_fear, leader_fear_transfer);
}
```

**Leadership Fear Cascade:**
- When leader shows fear (>50), followers become more afraid
- Transfer rate: **15-25%** of leader's fear
- This is **2x stronger** than normal group contagion

**Example:**
- Leader has fear = 80
- Transfer to followers = 80 × 20% = 16 fear
- **Each follower gains ~16 fear from leader's panic**

#### Leader's Courage Influence
```c
if (has_leader && leader_fear < 30) {
    int courage_transfer = rand_number(3, 8);
    adjust_emotion(mob, &mob->ai_data->emotion_fear, -courage_transfer);
    adjust_emotion(mob, &mob->ai_data->emotion_courage, courage_transfer / 2);
}
```

**Leadership Courage Boost:**
- Brave leader (fear < 30) emboldens followers
- Reduces follower fear by 3-8 points
- Increases follower courage by 1-4 points
- **Courageous leaders stabilize the group emotionally**

#### Leader's Happiness Influence
```c
if (has_leader && leader_happiness > 50) {
    int leader_happiness_transfer = (leader_happiness * rand_number(12, 20)) / 100;
    adjust_emotion(mob, &mob->ai_data->emotion_happiness, leader_happiness_transfer);
}
```

**Leadership Morale:**
- Happy leader (>50) boosts follower morale
- Transfer rate: 12-20% of leader's happiness
- **Positive leaders create positive groups**

#### Leader's Anger Influence
```c
if (has_leader && leader_anger > 60) {
    int leader_anger_transfer = (leader_anger * rand_number(10, 18)) / 100;
    adjust_emotion(mob, &mob->ai_data->emotion_anger, leader_anger_transfer);
}
```

**Leadership Anger:**
- Angry leader (>60) riles up followers
- Transfer rate: 10-18% of leader's anger
- **Aggressive leaders create aggressive groups**

## Emotional Intelligence (EI) Role

### What is EI?

**Emotional Intelligence (Inteligência Emocional)** is a genetic trait that determines how strongly a mob reacts to emotional stimuli, including contagion.

**Range:** 10-95 (stored in `genetics.emotional_intelligence`)

**Effects on Contagion:**
- **Low EI (0-35):** Volatile reactions - amplifies contagion by 120-150%
- **Average EI (36-65):** Normal reactions - no modification (100%)
- **High EI (66-100):** Measured reactions - dampens contagion to 70-90%

### EI Modulation Formula

All emotion changes (including contagion) pass through the `adjust_emotion()` function:

```c
void adjust_emotion(struct char_data *mob, int *emotion_ptr, int amount)
{
    int ei = GET_GENEMOTIONAL_IQ(mob);
    
    if (ei < 35) {
        // Low EI: Volatile, extreme emotional reactions
        amount = (amount * (120 + rand_number(0, 30))) / 100;
    } else if (ei > 65) {
        // High EI: Measured, controlled emotional responses
        amount = (amount * (70 + rand_number(0, 20))) / 100;
    }
    // Average EI (36-65): No modification to amount
    
    *emotion_ptr = URANGE(0, *emotion_ptr + amount, 100);
}
```

### EI Examples

**Low EI Mob (EI = 25):**
- Receives 10 fear from contagion
- Amplified to: 10 × 135% = **13-14 fear**
- **Highly susceptible to emotional contagion**
- Becomes panicked quickly in fearful crowds

**Average EI Mob (EI = 50):**
- Receives 10 fear from contagion
- No modification: **10 fear**
- **Normal emotional responses**

**High EI Mob (EI = 80):**
- Receives 10 fear from contagion
- Dampened to: 10 × 80% = **8 fear**
- **Emotionally stable, resists contagion**
- Remains calm in panicked crowds

### EI Adaptive Learning

EI can change over time based on emotional experiences:

```c
void adjust_emotional_intelligence(struct char_data *mob, int change)
{
    int current_ei = mob->ai_data->genetics.emotional_intelligence;
    change = URANGE(-2, change, 3);  // Cap at ±3 per event
    mob->ai_data->genetics.emotional_intelligence = URANGE(10, current_ei + change, 95);
}
```

**EI increases when:**
- Emotions remain stable (not extreme) over time
- Successfully regulates emotions after stress
- **Probability:** 5% chance per tick if emotions balanced

**EI decreases when:**
- Emotions reach extreme levels (>80) frequently
- Emotional breakdowns occur
- **Probability:** 2% chance per tick if 4+ emotions >70

## Emotional Profiles and Contagion

Emotional profiles define baseline personalities that affect how mobs participate in contagion.

### Profile Effects on Contagion

| Profile | Fear Susceptibility | Happiness Spread | Anger Spread | Notes |
|---------|-------------------|------------------|--------------|-------|
| NEUTRAL (0) | Normal | Normal | Normal | Genetics-based |
| AGGRESSIVE (1) | Low (courage=70) | Low (happiness=20) | High (anger=75) | Spreads anger, resists fear |
| DEFENSIVE (2) | Very High (fear=70) | Low | Low | Amplifies panic in groups |
| BALANCED (3) | Normal | Normal | Normal | Stable emotional participant |
| SENSITIVE (4) | High | Very High (happiness=75) | Low (anger=15) | Spreads joy, vulnerable to fear |
| CONFIDENT (5) | Very Low (courage=80) | High | Low | Calms fearful groups |
| GREEDY (6) | Low | Low | Moderate | Self-focused, less affected |
| LOYAL (7) | Moderate | High (happiness varies) | Low | Strong group bonding |

### Profile Interaction Examples

**Scenario: Fearful Group**
- Room has 4 mobs: 2 DEFENSIVE (fear=70), 1 CONFIDENT (fear=15), 1 BALANCED (fear=50)
- Average fear = (70+70+15+50)/4 = 51.25
- DEFENSIVE mobs amplify their fear further (high base + contagion)
- CONFIDENT mob barely affected due to high courage baseline
- BALANCED mob gains moderate fear
- **Result:** DEFENSIVE mobs panic, CONFIDENT stays calm, BALANCED slightly worried

**Scenario: Happy Group**
- Group of 3: 2 SENSITIVE (happiness=75), 1 AGGRESSIVE (happiness=20)
- Average happiness = (75+75+20)/3 = 56.7
- SENSITIVE mobs spread happiness very effectively
- AGGRESSIVE mob gains some happiness but remains relatively unhappy
- **Result:** Overall positive mood despite one angry member

## Contagion Constants and Configuration

### Hardcoded Constants

Located in `utils.c`, function `update_mob_emotion_contagion()`:

| Constant | Value | Description |
|----------|-------|-------------|
| MAX_ROOM_NPCS_FOR_CONTAGION | 20 | Performance threshold - skip contagion if room has >20 NPCs |
| CROWD_BONUS_THRESHOLD | 3 | Number of mobs needed for crowd bonuses |
| LARGE_CROWD_THRESHOLD | 4 | Number of mobs for larger crowd bonuses |
| MIN_MOBS_FOR_HAPPINESS_CONTAGION | 2 | Minimum mobs for happiness to spread |
| MIN_MOBS_FOR_EXCITEMENT_CONTAGION | 2 | Minimum mobs for excitement to spread |

### Contagion Rate Ranges

**Fear:**
- Crowd: 5-10% + bonus (1-3 if mob_count >= 3)
- Group: 12-20%
- Leader (high fear): 15-25%
- Leader (low fear/courage): -(3-8)

**Happiness:**
- Crowd: 8-15% + bonus (2-4 if mob_count >= 4)
- Group: 10-15%
- Leader: 12-20%

**Anger:**
- Crowd: 3-7%
- Leader: 10-18%

**Excitement:**
- Crowd: 7-12%

### Configuration via CEDIT

While contagion rates are hardcoded, related systems are configurable:

```
CONFIG_MOB_CONTEXTUAL_SOCIALS (enable/disable entire emotion system)
CONFIG_EMOTION_DECAY_RATE_MULTIPLIER (50-200%, affects emotion persistence)
CONFIG_EXTREME_EMOTION_THRESHOLD (default 80, when emotions decay faster)
CONFIG_EXTREME_DECAY_MULTIPLIER (100-300%, how fast extremes decay)
```

## Performance Optimization

### Room Size Threshold

```c
/* Performance optimization: Skip contagion in very crowded rooms (>20 NPCs)
 * to avoid performance bottlenecks. Emotions still update passively. */
int room_npc_count = 0;
for (other = world[IN_ROOM(mob)].people; other; other = other->next_in_room) {
    if (IS_NPC(other))
        room_npc_count++;
    if (room_npc_count > 20)
        return; /* Too crowded for emotion contagion processing */
}
```

**Why this limit?**
- Contagion is O(N²) - each mob scans all other mobs
- At 20 mobs: 20 × 20 = 400 comparisons per tick
- At 50 mobs: 50 × 50 = 2500 comparisons per tick (6x more expensive)
- Rooms with >20 NPCs are rare (boss fights, city centers)
- Passive decay still functions in crowded rooms

### CPU Impact

**Per mob per tick (4 seconds):**
- Scan room for other mobs: O(N) where N = room NPCs
- Calculate averages: O(N)
- Apply transfers: O(1)
- **Total: O(N) per mob, O(N²) per room**

**Typical room (5 mobs):**
- 5 mobs × 5 scans = 25 operations
- ~0.01ms total CPU time
- **Negligible impact**

**Large room (15 mobs):**
- 15 mobs × 15 scans = 225 operations
- ~0.05ms total CPU time
- **Still acceptable**

**Very large room (30 mobs):**
- Contagion disabled automatically
- Only passive decay runs: O(1) per mob
- **Performance protected**

## Contagion in Practice

### Example 1: Battle Panic

**Setup:**
- Party of 4 mobs fights a dragon
- Dragon roars, all mobs gain fear from attack event (+15-20 fear)
- Initial fear levels: [40, 50, 35, 45]

**Tick 1 (4 seconds after roar):**
- Average fear = (40+50+35+45)/4 = 42.5
- Crowd contagion: +3 fear to each (7% of 42.5)
- Group contagion: +5 fear to each (12% of 42.5)
- **New fear: [48, 58, 43, 53]**

**Tick 2 (8 seconds):**
- Dragon attacks again: +15 fear to victim
- Average fear = (48+73+43+53)/4 = 54.25
- Contagion: +4 fear (crowd) + 6 fear (group)
- **New fear: [58, 79, 53, 63]**
- One mob approaching panic threshold (80)

**Tick 3 (12 seconds):**
- Average fear = (58+79+53+63)/4 = 63.25
- Contagion: +5 + 7 = +12 fear
- **New fear: [70, 91, 65, 75]**
- One mob exceeds panic threshold → triggers flee behavior

**Result:** Panic cascaded through the group in ~12 seconds via contagion amplification

### Example 2: Leader Courage

**Setup:**
- Party of 5 mobs with confident leader (EI=75, fear=20)
- Rest of group has fear levels: [60, 55, 65, 50]

**Without Leader:**
- Average fear = (60+55+65+50)/4 = 57.5
- Group contagion: +9 fear per mob
- Fear continues rising

**With Leader:**
- Leader fear = 20 (< 30, triggers courage influence)
- Courage transfer: -5 fear to each follower
- Group average fear includes leader: (20+60+55+65+50)/5 = 50
- Net contagion: +8 fear (from group) - 5 (from leader courage)
- **Result: +3 fear per tick instead of +9**
- Leader's presence reduces fear escalation by **67%**

### Example 3: Sensitive Mob Spreads Joy

**Setup:**
- Tavern with 6 mobs: 1 SENSITIVE bard (happiness=85), 5 patrons (happiness=40-50)

**Initial State:**
- Bard performs, gains happiness to 85
- Patrons average happiness = 45

**Tick 1:**
- Average happiness = (85+45+45+47+50+43)/6 = 52.5
- Happiness contagion: 52.5 × 12% = +6.3 happiness
- Large crowd bonus: +3 happiness
- **Each patron gains ~9 happiness**

**Tick 2:**
- Average happiness = (85+54+54+56+59+52)/6 = 60
- Contagion: 60 × 12% + 3 = ~10 happiness
- **Patrons continue gaining happiness**

**Result:** One joyful mob in a crowd can elevate everyone's mood through contagion

## Functions Reference

### Primary Function

```c
void update_mob_emotion_contagion(struct char_data *mob)
```
**Location:** `src/utils.c`, lines 6306-6455

**Purpose:** Updates a single mob's emotions based on contagion from nearby mobs

**Parameters:**
- `mob` - The mob being influenced by others' emotions

**Called From:** `mobact.c` during mob update cycle (every 4 seconds)

**Requirements:**
- `CONFIG_MOB_CONTEXTUAL_SOCIALS` must be enabled
- Mob must be NPC with valid `ai_data`
- Mob must be in valid room
- Room must have <20 NPCs

**Process:**
1. Scan room for all NPCs
2. Calculate average emotions (fear, happiness, anger, excitement)
3. Identify group members
4. Identify group leader
5. Apply crowd contagion (all mobs)
6. Apply group contagion (same group)
7. Apply leader influence (if has leader)

### Helper Function

```c
void adjust_emotion(struct char_data *mob, int *emotion_ptr, int amount)
```
**Location:** `src/utils.c`, lines 5755-5777

**Purpose:** Applies emotion change with EI modulation

**Parameters:**
- `mob` - The mob whose emotion to adjust
- `emotion_ptr` - Pointer to the specific emotion variable
- `amount` - Raw emotion change (positive or negative)

**Effects:**
- Applies EI-based multiplier (70-150%)
- Clamps result to 0-100 range
- Updates emotion in-place

## Integration with Other Systems

### Passive Decay

Contagion adds emotions, passive decay removes them. Together they create equilibrium:

```c
void update_mob_emotion_passive(struct char_data *mob)
```
- Called every 4 seconds (same as contagion)
- Emotions drift toward baseline values
- Extreme emotions (>80) decay faster (150% rate)
- Balances contagion to prevent saturation

**Example:**
- Contagion adds +8 fear per tick
- Passive decay removes -4 fear per tick
- **Net change: +4 fear per tick**
- Fear stabilizes when contagion = decay

### Combat System

Combat events trigger initial emotions that then spread via contagion:

```c
update_mob_emotion_attacked(mob, attacker);  // +15 fear, +20 anger
update_mob_emotion_contagion(mob);           // Spreads to nearby mobs
```

**Result:** One mob being attacked can cause fear to ripple through an entire group

### Social System

Socials are selected based on current emotions (including contagion effects):

```c
if (emotion_fear >= 70)
    perform_fearful_social(mob);  // Cower, tremble
```

**Result:** Contagion can cause coordinated group behaviors (all mobs cowering together)

### Memory System

Contagion does NOT create emotion memories (it's ambient, not directed):

- Contagion affects mood emotions
- Relationship emotions (toward specific entities) are not directly affected
- However, hybrid system combines mood + relationship for behavior

## Debugging and Testing

### View Mob Emotions
```
stat mob <name>
```
Shows all 20 emotions including fear, happiness, anger, excitement

### Check Contagion in Action
1. Gather multiple mobs in one room
2. Trigger emotion in one mob (e.g., attack it → fear increases)
3. Wait 4-8 seconds (1-2 ticks)
4. `stat mob` on nearby mobs to see contagion spread

### Configuration
```
cedit
```
Toggle `CONFIG_MOB_CONTEXTUAL_SOCIALS` to enable/disable entire emotion system

### Manual Testing Commands
```
medit <vnum>
```
Set mob Emotional Intelligence (affects contagion susceptibility)
Set emotional profile (affects baseline emotions)

## Common Issues

**Problem:** Emotions not spreading between mobs
**Solution:** 
- Verify `CONFIG_MOB_CONTEXTUAL_SOCIALS` is enabled
- Check mobs have `ai_data` initialized
- Confirm mobs are in same room
- Ensure room has <20 NPCs

**Problem:** Emotions spreading too fast
**Solution:**
- Increase mob EI (reduces contagion impact)
- Use CONFIDENT or BALANCED emotional profiles
- Check that decay rates are appropriate

**Problem:** Emotions spreading too slowly
**Solution:**
- Lower mob EI (amplifies contagion)
- Use DEFENSIVE or SENSITIVE profiles
- Verify `CONFIG_EMOTION_DECAY_RATE_MULTIPLIER` isn't too high

**Problem:** Group members not affected by leader
**Solution:**
- Verify mobs are in same group (`GROUP()` pointer matches)
- Check leader is correctly identified (`GROUP_LEADER()`)
- Ensure leader emotions are in threshold range (fear >50 or <30, etc.)

## Best Practices

1. **Use EI strategically:**
   - Elite guards: High EI (70-80) - stay calm under pressure
   - Novice guards: Low EI (30-40) - panic easily
   - Civilians: Average EI (45-55) - normal reactions

2. **Design group dynamics:**
   - Give leaders high EI and courage for stability
   - Mix profiles to create interesting group behaviors
   - Consider contagion when placing multiple mobs

3. **Balance with decay:**
   - Default decay rates work for most cases
   - Adjust `CONFIG_EMOTION_DECAY_RATE_MULTIPLIER` for faster/slower emotion shifts
   - Extreme threshold (80) prevents permanent saturation

4. **Performance awareness:**
   - Keep most rooms under 20 NPCs
   - Contagion automatically disabled in crowds
   - Boss fights with >20 mobs still work (passive decay continues)

5. **Test emotional cascades:**
   - Trigger strong emotion in one mob
   - Observe group reactions
   - Adjust EI and profiles to get desired behavior

## Summary

The Emotion Contagion system creates realistic group dynamics through three layers of emotional influence:

1. **Crowd Contagion:** Ambient emotions affect all nearby mobs
2. **Group Contagion:** Stronger bonds within parties
3. **Leader Influence:** Leaders have 2x emotional impact on followers

**Emotional Intelligence (EI)** modulates susceptibility to contagion:
- Low EI (0-35): 120-150% amplification (volatile)
- Average EI (36-65): 100% (normal)
- High EI (66-100): 70-90% dampening (stable)

**Emotional Profiles** define baseline personalities that interact with contagion to create diverse mob behaviors.

The system is **performance-optimized** for typical gameplay (O(N²) complexity, automatically disabled in rooms with >20 NPCs) and **balances with passive decay** to prevent saturation while maintaining dynamic emotional states.

**Files:**
- `src/utils.c` - Core contagion logic (lines 6306-6455)
- `src/utils.h` - Function declarations
- `src/mobact.c` - Integration into mob update cycle
- `src/structs.h` - Data structures (emotion_memory, mob_ai_data, genetics)

**Related Documentation:**
- `EMOTION_SYSTEM_TECHNICAL_SUMMARY.md` - Overall emotion system
- `EMOTION_GENETICS_INTERACTION.md` - EI and genetics details
- `EMOTIONAL_PROFILES.md` - Profile system and thresholds
- `HYBRID_EMOTION_SYSTEM.md` - Mood vs relationship emotions
