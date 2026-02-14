# Emotion System Technical Summary

**Quick Reference for Developers**

## System Overview

- **20 emotions** tracked per mob (0-100 scale)
- **Hybrid model:** Mood (global) + Relationship (per-entity)
- **120+ config parameters** adjustable via cedit
- **Update frequency:** Passive decay every 4 seconds
- **Memory buffer:** 10 interactions per mob

## Core Formulas

### Emotion Update (Event-Driven)
```c
base_amount = rand_number(min, max);
// EI modulation
if (ei < 35) amount *= 1.20-1.50;
else if (ei > 65) amount *= 0.70-0.90;
emotion = CLAMP(emotion + amount, 0, 100);
```

### Passive Decay (Every 4 seconds)
```c
base_decay = CONFIG_DECAY_RATE_<EMOTION>;  // 0-10
if (emotion > 80) base_decay *= 1.50;      // Extreme multiplier
base_decay *= global_multiplier;           // 50-200%
emotion -= rand_number(1, base_decay);     // Toward baseline
```

### Hybrid Emotion Retrieval
```c
// Get effective emotion toward specific target
relationship = weighted_average_of_memories(target);
modifier = (relationship - 50) * 0.6;
effective = CLAMP(mood + modifier, 0, 100);
```

### Memory Weighting
```c
age = now - memory->timestamp;
weight = (age < 300) ? 10 : (age < 600) ? 7 : 
         (age < 1800) ? 5 : (age < 3600) ? 3 : 1;
if (major_event) weight *= 2;
avg = SUM(emotion * weight) / SUM(weight);
```

## Key Functions

| Function | Purpose |
|----------|---------|
| `adjust_emotion(mob, &emotion, amount)` | Apply emotion change with EI modulation |
| `update_mob_emotion_passive(mob)` | Decay all emotions toward baselines |
| `get_effective_emotion_toward(mob, target, type)` | Get hybrid emotion (mood + relationship) |
| `update_mob_emotion_attacked(mob, attacker)` | Event: being attacked |
| `update_mob_emotion_healed(mob, healer)` | Event: being healed |
| `apply_weather_to_mood(mob, weather, sunlight)` | Climate effects on mood |

## Configuration Categories

1. **Decay System (11 params)**
   - Global multiplier: 50-200%
   - Extreme threshold: 0-100 (default 80)
   - Extreme multiplier: 100-300% (default 150)
   - Per-emotion decay rates: 0-10

2. **Display Thresholds (20 params)**
   - Visual indicators: 0-100 (default 70-80)
   - Purely cosmetic, no behavior impact

3. **Combat/Flee (10 params)**
   - Fear/courage thresholds: 50/70
   - Horror panic threshold: 80
   - HP% modifiers: ±10-25%

4. **Pain System (8 params)**
   - Damage % thresholds: 5/10/25/50
   - Pain amounts: 1-5 / 5-15 / 15-30 / 30-50

5. **Memory (10 params)**
   - Age weights: 10/7/5/3/1 (recent to ancient)
   - Time thresholds: 5min/10min/30min/60min
   - Baseline offset: 50 (neutral point)

6. **Weather (3 params)**
   - Enable/disable flag
   - Effect multiplier: 0-200%
   - Per-emotion sensitivity

## Homeostasis Mechanism

**All emotions drift toward personality-defined baselines:**

| Emotion | Baseline |
|---------|----------|
| Fear | `wimpy_tendency / 2` (0-50) |
| Anger | Alignment-based (evil=35, neutral=25, good=15) |
| Happiness | Alignment-based (good=40, neutral=30, evil=15) |
| Sadness | 10 (universal) |
| Pain/Horror/etc | 0 (full decay) |

**Prevents:**
- ✅ Permanent saturation
- ✅ Runaway loops
- ✅ Emotional "stuck" states

## Emotional Intelligence (EI)

**Range:** 10-95

**Effects:**
- **Low EI (0-35):** 120-150% reaction intensity (volatile)
- **Average EI (36-65):** 100% reaction intensity (normal)
- **High EI (66-100):** 70-90% reaction intensity (measured)

**Adaptive Learning:**
- Stable emotions → +1 EI (5% chance per tick)
- Extreme emotions → -1 EI (2% chance per tick)

## Extreme Emotion Handling

**Threshold:** 80+ (configurable)

**Effects:**
1. **Faster decay:** 150% decay rate (default)
2. **Special behaviors:**
   - Fear 100: Paralyze (1 tick)
   - Anger 100: Berserk (+25% damage, extra attack)
3. **Emotional breakdown:** 4+ emotions >70 → confusion affect

## Memory System

**Capacity:** 10 entries per mob (circular buffer)

**Stored Data:**
- Entity ID (player/mob)
- Interaction type (attacked, healed, gift, etc.)
- Complete emotion snapshot (all 20 emotions)
- Timestamp
- Major event flag (2x weight)

**Decay:** Old memories overwritten when buffer full

## Weather Integration

**Effects:** Applied on zone weather update

**Modifiers:**
- Indoor: 50% reduction
- Adaptation: Up to 50% reduction over 1 week
- Climate: 80-150% based on native climate
- Preferred weather: Happiness boost, fear reduction

**Formula:**
```c
effect = base_effect * weather_mult * indoor_mult * 
         adaptation_mult * climate_mult / 10000;
```

## Combat Integration

### Flee Behavior
```c
modifier = 0;
if (fear >= 70) modifier += 15;  // Flee 15% earlier
else if (fear >= 50) modifier += 10;
if (courage >= 70) modifier -= 15;  // Flee 15% later
else if (courage >= 50) modifier -= 10;
if (horror >= 80) modifier += 25;  // Panic flee
flee_at_hp = base_flee_hp + modifier;
```

### Anger Bonuses
- Attack frequency: 25% chance extra attack (anger >= 70)
- Damage bonus: +15% damage (anger >= 70)

### Pain Penalties
- Accuracy: +1/+2/+4 THAC0 (pain 30/50/70)
- Damage: -5%/-10%/-20% (pain 30/50/70)

## Typical Values

**Normal mob in combat:**
- Fear: 20-40 (calm) → 50-70 (combat) → 20-40 (post-combat decay)
- Anger: 25 (neutral) → 50-80 (fighting) → 25 (baseline)
- Pain: 0 → 15-40 (damaged) → 0 (heals over time)

**Decay timeline:**
- Extreme emotion (90) → below threshold (79) in ~6 ticks (24 seconds) with 150% decay
- Moderate emotion (60) → baseline (30) in ~15 ticks (60 seconds) with normal decay
- Minor emotion (40) → baseline (30) in ~5 ticks (20 seconds) with normal decay

## Event Examples

### Being Attacked
```c
fear += rand_number(5, 15);           // +5-15 fear
anger += rand_number(10, 20);         // +10-20 anger
happiness -= rand_number(5, 15);      // -5-15 happiness
trust -= rand_number(10, 20);         // -10-20 trust (toward attacker)
pain += damage_based_amount;          // +0-50 pain (based on damage %)
```

### Being Healed
```c
happiness += rand_number(10, 20);     // +10-20 happiness
trust += rand_number(10, 15);         // +10-15 trust (toward healer)
friendship += rand_number(10, 15);    // +10-15 friendship
fear -= rand_number(5, 10);           // -5-10 fear
anger -= rand_number(5, 10);          // -5-10 anger
pain -= rand_number(15, 30);          // -15-30 pain
```

### Weather (Storm)
```c
fear += rand_number(3, 6);            // +3-6 fear (per zone update)
sadness += rand_number(2, 4);         // +2-4 sadness
happiness -= rand_number(2, 5);       // -2-5 happiness
// Modified by weather_mult, indoor, adaptation, climate
```

## Performance

**Memory per mob:** ~1.2 KB
- 20 emotions × 4 bytes = 80 bytes
- 10 memories × 100 bytes = 1000 bytes
- Genetics/prefs = ~120 bytes

**CPU per tick (4 seconds):**
- Decay update: ~20 comparisons, 10 random calls
- O(1) per mob, no iteration
- Negligible impact (<0.1ms per 100 mobs)

**Memory access:**
- O(10) linear search for relationship lookup
- O(1) for mood access
- Cache-friendly sequential memory

## Testing Commands

```
stat mob <name>              # Show all emotions
cedit                        # Configure emotion parameters
cedit emotion                # Emotion-specific config menu
medit <vnum>                 # Set mob EI, profiles, etc.
```

## Common Issues

**Problem:** Emotions stuck at extremes  
**Solution:** Check decay multiplier (should be 100%), extreme threshold (should be 80)

**Problem:** Too volatile behavior  
**Solution:** Increase mob EI (higher = more stable)

**Problem:** No emotional diversity  
**Solution:** Set varied personality profiles, adjust baselines

**Problem:** Relationships don't persist  
**Solution:** Zone resets clear memories (by design), check memory buffer isn't full

## Best Practices

1. **Always use hybrid function for per-target decisions:**
   ```c
   int emotion = get_effective_emotion_toward(mob, target, EMOTION_TYPE_FEAR);
   ```

2. **Use mood for general state checks:**
   ```c
   int mood_fear = mob->ai_data->emotion_fear;
   ```

3. **Check system enabled:**
   ```c
   if (!CONFIG_MOB_CONTEXTUAL_SOCIALS) return;
   ```

4. **Validate pointers:**
   ```c
   if (!mob || !IS_NPC(mob) || !mob->ai_data) return;
   ```

5. **Use adjust_emotion(), not direct assignment:**
   ```c
   adjust_emotion(mob, &mob->ai_data->emotion_fear, 10);  // ✅ Good
   mob->ai_data->emotion_fear += 10;                       // ❌ Bad (bypasses EI)
   ```

## Files

- **structs.h:** emotion_config_data, mob_ai_data structures
- **utils.h:** Function declarations, CONFIG_EMOTION_* macros
- **utils.c:** Core emotion functions (lines 5755-6300)
- **fight.c:** Combat emotion triggers
- **magic.c:** Spell emotion triggers
- **act.social.c:** Social emotion triggers
- **weather.c:** Climate emotion effects
- **mobact.c:** Passive emotion updates, contagion
- **cedit.c:** Configuration editor

## References

- RFC-1000_EMOTION_SYSTEM_ARCHITECTURE_RESPONSE.md (comprehensive)
- EMOTION_CONFIG_SYSTEM.md (configuration details)
- HYBRID_EMOTION_SYSTEM.md (developer guide)
- EMOTION_SYSTEM_TODO.md (feature roadmap)
