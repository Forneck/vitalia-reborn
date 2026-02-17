# Big Five (OCEAN) Personality System - Phase 1: Neuroticism

## Overview

The Big Five personality model (OCEAN) provides a structural personality layer for mobs that operates independently of the reactive emotional system. This creates stable temperament parameters that modulate how mobs experience and process events.

**Phase 1 Status**: Only **Neuroticism (N)** is currently implemented and functional.

## Personality Traits (OCEAN Model)

### Implemented

#### Neuroticism (N) - Emotional Sensitivity [ACTIVE]
- **Range**: 0.0 to 1.0 (float)
- **Low N (0.0)**: Emotionally stable, calm, not easily upset
- **High N (1.0)**: Emotionally reactive, anxious, threat-sensitive
- **Function**: Amplifies negative emotional intensity only
- **Effect**: Higher N → stronger negative emotional responses to same stimuli

### Reserved for Future Phases

#### Openness (O) - Openness to Experience [Phase 4]
- **Planned Use**: Prediction error weighting, curiosity modulation
- **Current Value**: 0.0 (neutral)

#### Conscientiousness (C) - Self-Discipline [Phase 2]
- **Planned Use**: Inhibitory control, goal persistence
- **Current Value**: 0.0 (neutral)

#### Extraversion (E) - Social Engagement [Phase 3]
- **Planned Use**: Social reward gain, interaction frequency
- **Current Value**: 0.0 (neutral)

#### Agreeableness (A) - Compassion/Cooperation [Phase 3]
- **Planned Use**: Interpersonal aggression modulation
- **Current Value**: 0.0 (neutral)

## Neuroticism (Phase 1) - Technical Details

### Core Function: Emotional Gain Amplifier

Neuroticism acts as a gain multiplier applied to negative emotions ONLY. Positive emotions are completely unaffected.

#### Emotional Gain Formula
```
E_raw = E_base * (1.0 + (β * N))
```

Where:
- `E_base` = Base emotion value after EI modulation
- `β` = Emotion-specific gain coefficient (see below)
- `N` = Neuroticism value [0.0, 1.0]
- `E_raw` = Amplified raw emotion (before soft clamp)

#### Gain Coefficients (β) by Emotion Type

**Full Gain (β = 0.4)** - Primary threat responses:
- Fear (EMOTION_TYPE_FEAR)
- Sadness (EMOTION_TYPE_SADNESS)
- Shame (EMOTION_TYPE_SHAME)
- Humiliation (EMOTION_TYPE_HUMILIATION)
- Pain (EMOTION_TYPE_PAIN)
- Horror (EMOTION_TYPE_HORROR)

**Reduced Gain (β = 0.25)** - Secondary aversive emotions:
- Disgust (EMOTION_TYPE_DISGUST)
- Envy (EMOTION_TYPE_ENVY)

**Lower Gain (β = 0.2)** - Approach-oriented negative:
- Anger (EMOTION_TYPE_ANGER)

**No Gain (β = 0.0)** - Positive emotions:
- Happiness, Pride, Friendship, Love, Trust, Loyalty, Curiosity, Compassion, Courage, Excitement, Greed

### Soft Saturation Clamp

To prevent runaway values while preserving gradient near the cap, a piecewise soft saturation function is applied:

```
if E_raw ≤ 80:
    E_eff = E_raw  (no compression, linear)
    
if E_raw > 80:
    excess = E_raw - 80
    E_eff = 80 + 20 * (excess / (excess + k))
    where k = 50 (EMOTION_SOFT_CLAMP_K)
```

**Properties**:
- Values 0-80 pass through unchanged
- Values >80 are smoothly compressed toward 100
- As E_raw → ∞, E_eff → 100 (asymptotic approach)
- Maximum effective value is <100 (e.g., E_raw=140 → E_eff≈90.91)

### Processing Pipeline

The emotion adjustment pipeline operates in this order:

1. **EI Modulation**: Emotional Intelligence affects volatility
   - Low EI (0-35): 120-150% of change amount
   - Average EI (36-65): 100% of change amount
   - High EI (66-100): 70-90% of change amount

2. **Calculate Base**: `base_emotion = current_emotion + EI_modulated_amount`

3. **Neuroticism Gain**: Apply β gain if negative emotion
   - `E_raw = base_emotion * (1.0 + β * N)`

4. **Soft Saturation**: Apply compression if E_raw > 80
   - Prevents hard 100 cap
   - Preserves gradient

5. **Final Clamp**: `E_final = CLAMP(E_eff, 0, 100)`

### Initialization from Genetics

Neuroticism is calculated from genetic traits at mob creation:

```c
// Inverse bravery component (0.0 to 1.0)
float inverse_bravery = (100.0f - brave_prevalence) / 100.0f;

// Low EI component (0.0 to 1.0, only if EI < 50)
float low_ei_factor = 0.0f;
if (emotional_intelligence < 50) {
    low_ei_factor = (50.0f - emotional_intelligence) / 50.0f;
}

// Weighted combination: 70% bravery, 30% EI
N = (inverse_bravery * 0.7f) + (low_ei_factor * 0.3f);
N = CLAMP(N, 0.0f, 1.0f);
```

**Rationale**:
- Low bravery → Higher threat sensitivity (primary)
- Low EI → Higher emotional volatility (secondary)
- Weighted 70/30 to emphasize courage-based temperament

### Example Calculations

#### Example 1: Low Neuroticism Mob
- Genetics: brave_prevalence=80, emotional_intelligence=60
- Neuroticism: N = ((100-80)/100)*0.7 + 0*0.3 = 0.14
- Fear stimulus: base=50 → raw=50*(1+0.4*0.14)=52.8 → final=52.8
- **Result**: Minimal amplification, emotionally stable

#### Example 2: High Neuroticism Mob
- Genetics: brave_prevalence=20, emotional_intelligence=30
- Neuroticism: N = ((100-20)/100)*0.7 + ((50-30)/50)*0.3 = 0.56 + 0.12 = 0.68
- Fear stimulus: base=50 → raw=50*(1+0.4*0.68)=63.6 → final=63.6
- **Result**: Significant amplification, threat-sensitive

#### Example 3: Extreme High Base + High N
- Neuroticism: N = 1.0
- Fear stimulus: base=90 → raw=90*(1+0.4*1.0)=126 → soft clamp activated
- Soft clamp: excess=126-80=46 → final=80+20*(46/(46+50))=89.58
- **Result**: Compressed below 100, gradient preserved

## Integration with Existing Systems

### No Impact On:
- ✅ Long-term memory (LTM) system
- ✅ Moral reasoning (Shultz & Darley)
- ✅ Emotion decay system
- ✅ Shadow Timeline projections
- ✅ Emotional competition logic

### Transparent Integration:
- Works through existing `adjust_emotion()` function
- No changes to combat, quest, or social systems
- Personality layer operates "under the hood"

## Configuration Constants

Defined in `src/structs.h`:

```c
#define NEUROTICISM_GAIN_FEAR 0.4f
#define NEUROTICISM_GAIN_SADNESS 0.4f
#define NEUROTICISM_GAIN_SHAME 0.4f
#define NEUROTICISM_GAIN_HUMILIATION 0.4f
#define NEUROTICISM_GAIN_PAIN 0.4f
#define NEUROTICISM_GAIN_HORROR 0.4f
#define NEUROTICISM_GAIN_DISGUST 0.25f
#define NEUROTICISM_GAIN_ENVY 0.25f
#define NEUROTICISM_GAIN_ANGER 0.2f
#define EMOTION_SOFT_CLAMP_K 50.0f
```

## Behavioral Impact

### Observable Differences

Two mobs with identical genetics but different Neuroticism will exhibit:

1. **Different Fear Responses**: High-N mobs flee sooner under identical threat
2. **Different Pain Reactions**: High-N mobs show more distress from same damage
3. **Different Shame Sensitivity**: High-N mobs more affected by humiliation
4. **Identical Positive Emotions**: Both mobs equally happy/proud under same conditions

### Example Scenario

**Setup**: Two guards attacked by same player, dealing 20 damage each

**Guard A** (N=0.2, brave=70, EI=50):
- Fear base: 15 (from attack) → raw=15*(1+0.4*0.2)=16.2 → final=16
- Pain base: 20 (from damage) → raw=20*(1+0.4*0.2)=21.6 → final=22
- **Behavior**: Stands ground, fights normally

**Guard B** (N=0.8, brave=30, EI=35):
- Fear base: 15 (from attack) → raw=15*(1+0.4*0.8)=19.8 → final=20
- Pain base: 20 (from damage) → raw=20*(1+0.4*0.8)=26.4 → final=26
- **Behavior**: More likely to flee, shows distress

## Performance Considerations

- **Computation Cost**: Minimal (3 float multiplications + 1 conditional per emotion change)
- **Memory Footprint**: 20 bytes per mob (5 floats in personality struct)
- **No Runtime Mutation**: Neuroticism is stable after initialization
- **Safe for High Density**: No performance degradation in zones with many mobs

## Testing & Validation

### Validation Checklist

✅ Mobs with different N show different negative emotional intensities under identical stimuli  
✅ Positive emotions remain unaffected by N  
✅ Emotional values never hard-cap at 100  
✅ Emotional competition system remains stable  
✅ No performance degradation in high-mob-density zones  
✅ Compilation successful without warnings  
✅ Gradient preserved at extreme values  

### Test Scenarios Validated

1. **N=0.0, 0.5, 1.0 with base fear=50**: Linear amplification confirmed
2. **Happiness with varying N**: No change (positive preservation)
3. **Different β values**: Fear(0.4) > Disgust(0.25) > Anger(0.2)
4. **High base values (60-95)**: Soft compression active above 80
5. **Gradient at extremes**: Smooth curve, no clipping

## Future Phases

### Phase 2: Conscientiousness (C)
- **Function**: Inhibitory control on impulsive actions
- **Effect**: High C → better self-control, goal persistence
- **Integration**: Modify goal abandonment probability

### Phase 3: Extraversion (E) & Agreeableness (A)
- **Extraversion**: Social reward gain, interaction frequency
- **Agreeableness**: Interpersonal aggression modulation
- **Integration**: Social system and combat decision logic

### Phase 4: Openness (O)
- **Function**: Prediction error weighting, curiosity
- **Integration**: Shadow Timeline exploration bonus

## Implementation Files

- **`src/structs.h`**: Personality struct definition, configuration constants
- **`src/utils.c`**: Core functions (gain, soft clamp, pipeline)
- **`src/utils.h`**: Function prototypes
- **`src/quest.c`**: Personality initialization (`init_mob_ai_data()`)

## References

- Issue: "Feat: Big Five (OCEAN) – Phase 1 Implementation"
- Related: RFC-1001 (Emotional System)
- Genetic System: `md-docs/EMOTION_GENETICS_INTERACTION.md`
- Emotion Config: `md-docs/EMOTION_CONFIG_SYSTEM.md`

## Changelog

### Version 1.0 (Phase 1) - February 2026
- ✅ Neuroticism implementation
- ✅ Emotional gain amplification for negative emotions
- ✅ Soft saturation clamp (piecewise compression)
- ✅ Genetic initialization formula
- ✅ Integration with adjust_emotion() pipeline
- ✅ Documentation and testing

### Upcoming
- ⏳ Phase 2: Conscientiousness
- ⏳ Phase 3: Extraversion & Agreeableness
- ⏳ Phase 4: Openness
- ⏳ Builder tools (CEDIT integration for viewing/debugging N values)
