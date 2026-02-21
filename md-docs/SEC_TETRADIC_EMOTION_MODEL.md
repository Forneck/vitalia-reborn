# SEC – Tetradic Emotion Model

**File:** `src/sec.h` / `src/sec.c`  
**System:** SEC – Sistema de Emoções Concorrentes (Competing Emotions System)  
**Upgrade:** Triad (Fear + Anger + Happiness) → **Tetrad (Fear + Sadness + Anger + Happiness)**

---

## Overview

The SEC is a deterministic, 4D-driven internal emotional inference layer that sits on top of the
20-emotion mood system. It receives post-hysteresis 4D results and partitions the mob's Arousal
budget into four competing emotional intensities, enforcing energy conservation via Lateral
Inhibition. SEC drives three types of downstream output:

1. **4D behaviour modifier** – scales behavioral intensity (0.5×–2.0×).
2. **WTA (Winner-Takes-All) filter** – gates social category selection.
3. **Lethargy bias** – suppresses idle and social pulses when Sadness dominates.

---

## Architecture: Four-Timescale Model

| Layer | Timescale | Rate | Purpose |
|---|---|---|---|
| Arousal partition | Fast | — | Active state projection from D×V quadrants |
| Emotional smoothing | Medium | α ≈ 0.40 | Behavioral continuity (F, Sd, An, H) |
| Passive decay | Slow | λ = 0.05 | Homeostatic convergence toward profile baseline |
| Persistent trait | Very slow | δ = 0.01 | Structural memory (Disgust — outside partition) |

---

## Tetradic Partition Formula

The Arousal budget `A` is split across four emotions using a **D×V quadrant map**:

```
A = raw_arousal / 100           (pre-modulation; keeps budget proportional to intrinsic state)
D = (dominance + 100) / 200     (normalised Dominance)
V = (valence  + 100) / 200      (normalised Valence)

w_fear    = A * (1 − D) * V         — low dominance, high valence  (threat / uncertainty)
w_sadness = A * (1 − D) * (1 − V)  — low dominance, low valence   (passive loss / grief)  ← NEW
w_anger   = A * D * (1 − V)        — high dominance, low valence  (active loss / fight)
w_happy   = A * D * V              — high dominance, high valence  (approach / reward)

Guarantee: w_fear + w_sadness + w_anger + w_happy = A
```

### Why these quadrants?

| Quadrant | D | V | Appraisal | Behavioural meaning |
|---|---|---|---|---|
| Fear | Low | High | Threat present, no escape | Vigilance, flight tendency |
| **Sadness** | **Low** | **Low** | **Goal lost, no recovery path** | **Passive withdrawal, apathy** |
| Anger | High | Low | Goal blocked, can fight back | Attack escalation |
| Happiness | High | High | Goal achieved / safe | Approach, social initiation |

**Sadness triggers precisely when Goal Incongruence (low V) AND low Coping Potential (low D)
combine** — the "passive loss" appraisal described in the SEC specification. This is the
`[Thief escaped]` scenario from the issue brief.

---

## Lateral Inhibition — 4-Emotion Budget

The four competing SEC emotions share a single Arousal budget of **100 integer units**
(enforced in `adjust_emotion()`, `utils.c`).

**Invariant:** `emotion_fear + emotion_sadness + emotion_anger + emotion_happiness ≤ 100`

When any tetrad emotion is increased above the budget ceiling, the **excess is drained from the
other three proportionally**:

```c
/* Drain proportionally; use integer rounding to preserve exact total. */
reduce[i] = excess * *ptrs[i] / sum_others;   // first two competitors
reduce[2] = excess - drained;                 // remainder to third (exact)
```

**Draining Effect of Sadness:** When Sadness rises (low V + low D stimulus), it
mathematically suppresses Anger and Happiness simultaneously — this is the "Lethargy Buffer".
Example log scenario from the issue brief:

```
[SEC Check: Treasure Stolen]
[Appraisal: Goal Incongruence: High | Coping: Low (Thief escaped)]
[Result: Sadness +80, Anger 10, Fear 10, Happiness 0]
[Effect: Aglandiir sighs deeply and ignores your presence.]
```

---

## Arousal Budget Source — Raw vs. Effective

The budget uses **`raw_arousal`** (pre-modulation) rather than the effective arousal
(post-modulation). The contextual Arousal multiplier (combat intensity, crowd density) can inflate
the effective arousal to 100 in nearly every combat tick, which would fix `A = 1.0` permanently
and make Lateral Inhibition mathematically impossible. Using `raw_arousal` keeps `A` proportional
to the mob's intrinsic emotional state.

---

## Emotional Smoothing — α with Energy Conservation

After the partition targets are computed each tick, `sec_update_partitioned()` applies
α-smoothing then **renormalises to Arousal**:

```
Step 1: new_x = old_x * (1 − α) + target_x * α          (for each of 4 emotions)
Step 2: W_total = new_fear + new_sad + new_anger + new_happy
Step 3: if W_total > ε: each new_x *= A / W_total         (renormalise to budget)
```

Lateral inhibition **emerges naturally** from renormalisation: when the Sadness target grows,
the other three are scaled down proportionally.

**Conscientiousness regulation:** `α_effective = α * (1.20 − C_final * 0.50)`. High-C mobs
resist sudden spikes (α ≈ 0.84 of base), keeping emotional reactions measured.

---

## Passive Decay

When `raw_arousal < SEC_AROUSAL_EPSILON` (mob is emotionally quiescent), all four partition
values decay toward their personality baseline:

```c
sec.sadness = sec.sadness * (1 − λ) + sec_base.sadness_base * λ
```

This allows a grieving mob to slowly return to its profile's resting sadness level, rather than
being locked at the event-driven peak indefinitely.

---

## Profile Baselines (8 Profiles × 4 Emotions)

| Profile | fear_base | anger_base | happiness_base | sadness_base | Sum |
|---|---|---|---|---|---|
| 0 NEUTRAL | 0.20 | 0.20 | 0.30 | 0.10 | 0.80 |
| 1 AGGRESSIVE | 0.10 | 0.60 | 0.15 | 0.05 | 0.90 |
| 2 DEFENSIVE | 0.60 | 0.10 | 0.10 | 0.15 | 0.95 |
| 3 BALANCED | 0.25 | 0.25 | 0.35 | 0.10 | 0.95 |
| 4 SENSITIVE | 0.35 | 0.15 | 0.35 | 0.15 | 1.00 |
| 5 CONFIDENT | 0.10 | 0.30 | 0.45 | 0.05 | 0.90 |
| 6 GREEDY | 0.20 | 0.40 | 0.20 | 0.10 | 0.90 |
| 7 LOYAL | 0.15 | 0.20 | 0.50 | 0.15 | 1.00 |

All profile baseline sums are ≤ 1.0 (achievable rest states within the Arousal budget).

---

## Dominant Emotion — Winner-Takes-All

`sec_get_dominant_emotion()` returns the emotion with the highest SEC weight:

```
Tie-breaking priority:  Anger > Fear > Sadness > Happiness

SEC_DOMINANT_NONE     (0)  — total arousal < SEC_AROUSAL_EPSILON (0.05)
SEC_DOMINANT_FEAR     (1)
SEC_DOMINANT_ANGER    (2)
SEC_DOMINANT_HAPPINESS (3)
SEC_DOMINANT_SADNESS  (4)  ← NEW
```

Sadness is placed above Happiness in the priority chain: passive-loss states should suppress
approach behaviours before allowing positive affect to dominate.

---

## SEC Getters — Downstream Effects

### `sec_get_4d_modifier()` — Behaviour Intensity [0.5, 2.0]

```c
mod = 1.0 + (anger − fear − helplessness * 0.5 − sadness * 0.8) * 0.5
```

High Sadness subtracts `sadness * SEC_MOD_SADNESS_WEIGHT (0.8)` from the modifier, reducing
behavioural arousal — this implements the "High Sadness lowers the 4D Arousal value" requirement
from the acceptance criteria.

### `sec_get_flee_bias()` — Flee Tendency [0.0, 1.0]

```c
bias = (fear + helplessness) * 0.5
```

Unchanged by the tetradic upgrade (flee is driven by fear/helplessness, not sadness).

### `sec_get_lethargy_bias()` — Lethargy Suppression [0.0, 1.0] ← NEW

```c
bias = sadness + helplessness * SEC_MOD_HELPLESSNESS_WEIGHT (0.5)
```

Used in `mob_emotion_activity()`:

- `lethargy_bias ≥ SEC_LETHARGY_SUPPRESS_THRESHOLD (0.70)` → **skip social pulse entirely**
- `lethargy_bias > 0` → scale `social_chance` down by `(1 − lethargy_bias)`

This implements the "Mobs with high Sadness show reduced frequency in Idle or Aggressive pulse
triggers" acceptance criterion.

### `sec_get_target_bias()` — Target Selection Weight [0.5, 1.5]

Driven by Anger only; unchanged by the tetradic upgrade.

---

## WTA Filter Update — Social Category Mapping

The mob social system maps social categories to SEC dominant emotions. After the tetradic upgrade,
`sad_socials` and `mourning_socials` are correctly mapped to `SEC_DOMINANT_SADNESS` rather than
the old approximation of `SEC_DOMINANT_FEAR`:

| Social category | SEC emotion (before) | SEC emotion (after) |
|---|---|---|
| sad_socials | `SEC_DOMINANT_FEAR` | **`SEC_DOMINANT_SADNESS`** |
| mourning_socials | `SEC_DOMINANT_FEAR` | **`SEC_DOMINANT_SADNESS`** |
| fearful_socials | `SEC_DOMINANT_FEAR` | `SEC_DOMINANT_FEAR` (unchanged) |
| submissive_socials | `SEC_DOMINANT_FEAR` | `SEC_DOMINANT_FEAR` (unchanged) |

---

## Data Structures

### `sec_state` (in `structs.h`)

```c
struct sec_state {
    float fear;         /* Projected fear intensity from Arousal partition [0, 1] */
    float anger;        /* Projected anger intensity from Arousal partition [0, 1] */
    float happiness;    /* Projected happiness intensity from Arousal partition [0, 1] */
    float sadness;      /* Projected sadness intensity: low V + low D (passive loss) [0, 1] */
    float helplessness; /* Smoothed helplessness: (1 − Dominance_normalised) */
    float disgust;      /* Persistent disgust trait (mirrors emotion_disgust; out-of-partition) */
};
```

### `sec_baseline` (in `structs.h`)

```c
struct sec_baseline {
    float fear_base;      /* Resting level [0, 1] */
    float anger_base;     /* Resting level [0, 1] */
    float happiness_base; /* Resting level [0, 1] */
    float sadness_base;   /* Resting level [0, 1] */
};
```

---

## Public API

| Function | Purpose |
|---|---|
| `sec_init(mob)` | Initialise SEC state and baseline from emotional profile |
| `sec_update(mob, r)` | Partition Arousal into 4 emotions from post-hysteresis 4D state |
| `sec_passive_decay(mob)` | Decay all 4 emotions toward baseline when quiescent |
| `sec_get_dominant_emotion(mob)` | WTA: return `SEC_DOMINANT_*` constant |
| `sec_get_4d_modifier(mob)` | Behaviour intensity multiplier ∈ [0.5, 2.0] |
| `sec_get_flee_bias(mob)` | Flee tendency ∈ [0.0, 1.0] |
| `sec_get_lethargy_bias(mob)` | Lethargy (sadness) suppression bias ∈ [0.0, 1.0] |
| `sec_get_target_bias(mob, target)` | Target selection weight ∈ [0.5, 1.5] |
| `sec_get_agreeableness_final(mob)` | OCEAN A with emotional modulation |
| `sec_get_extraversion_final(mob)` | OCEAN E with emotional modulation |
| `sec_get_conscientiousness_final(mob)` | OCEAN C (disgust-eroded) |
| `sec_get_neuroticism_final(mob)` | OCEAN N (fear+anger stress signal) |
| `sec_get_openness_final(mob)` | OCEAN O (no per-tick modulation by design) |

---

## Key Named Constants

| Constant | Value | Purpose |
|---|---|---|
| `SEC_DOMINANT_SADNESS` | 4 | WTA result for sadness-dominant state |
| `SEC_MOD_SADNESS_WEIGHT` | 0.8 | Sadness suppression coefficient in 4D modifier |
| `SEC_MOD_HELPLESSNESS_WEIGHT` | 0.5 | Helplessness weight in lethargy bias |
| `SEC_LETHARGY_SUPPRESS_THRESHOLD` | 0.70 | Lethargy bias above which social pulses are skipped |
| `SEC_AROUSAL_EPSILON` | 0.05 | Arousal below this → quiescent (decay guard, WTA, partition) |
| `SEC_WTA_THRESHOLD` | 0.60 | Minimum fraction of dominant SEC weight to allow a social |

---

## Active Loss vs. Passive Loss — Differentiating the Two States

The tetradic model satisfies the issue requirement: "The system differentiates between Active Loss
(Anger/High Coping) and Passive Loss (Sadness/Low Coping)."

| Scenario | D (Dominance) | V (Valence) | Dominant SEC | Behaviour |
|---|---|---|---|---|
| Attacked but winning | High | Low | Anger | Extra attacks, damage boost |
| Attacked and losing | Low | Low | **Sadness** | Lethargy, withdrawal |
| Treasure stolen, thief caught | High | Low | Anger | Chase, aggression |
| Treasure stolen, thief escaped | Low | Low | **Sadness** | Sighs, ignores players |
| Threat visible, can flee | Low | High | Fear | Flee bias increases |
| Safe, goal achieved | High | High | Happiness | Positive socials |

---

## Relationship to the 20-Emotion Mood System

The SEC is a **separate inference layer** that runs on top of the 20-emotion mood system:

```
20-emotion mood layer (emotion_fear, emotion_sadness, ...)
       │
       ▼
emotion_projection.c → compute_emotion_4d_state() → emotion_4d_state {valence, arousal, dominance, …}
       │
       ▼
sec_update(mob, &r)   → sec_state {fear, sadness, anger, happiness, helplessness, disgust}
       │
       ├── sec_get_dominant_emotion()  →  WTA social gating (mobact.c)
       ├── sec_get_4d_modifier()       →  behaviour intensity scaling
       ├── sec_get_flee_bias()         →  flee threshold adjustment
       ├── sec_get_lethargy_bias()     →  idle/social pulse suppression
       └── sec_get_*_final()           →  OCEAN trait modulation (shadow_timeline.c)
```

The `emotion_sadness` mood field (0–100 integer) is part of the 20-emotion layer and decays
toward a profile-defined baseline independently. The `sec.sadness` float [0,1] is the
arousal-partitioned SEC projection derived from it via the 4D state.

---

## Migration Notes: Triad → Tetrad

For reference, the old triad partition formula was:

```c
/* OLD — triad (pre-upgrade): */
w_fear  = A * (1 − D)
w_anger = A * D * (1 − V)
w_happy = A * D * V
```

The old formula had **no low-D + low-V quadrant**: any loss-state with low Dominance collapsed
entirely into Fear, preventing differentiation between threat-fear and grief-sadness. The new
formula corrects this by splitting the `(1 − D)` budget between Fear (high V) and Sadness (low V).

### Files Changed

| File | Change |
|---|---|
| `src/structs.h` | `sec_state` gains `float sadness`; `sec_baseline` gains `float sadness_base` |
| `src/sec.h` | `SEC_DOMINANT_SADNESS 4`; named constants; `sec_get_lethargy_bias()` declaration |
| `src/sec.c` | Partition formula, baselines table, init, decay, modifier, dominant, new getter |
| `src/utils.c` | Lateral Inhibition extended from triad to tetrad |
| `src/mobact.c` | WTA: sad/mourning → `SEC_DOMINANT_SADNESS`; lethargy bias in pulse gating |

---

## Related Documentation

- `HYBRID_EMOTION_SYSTEM.md` — 20-emotion mood + relationship layers (separate from SEC)
- `EMOTION_SYSTEM_TECHNICAL_SUMMARY.md` — quick reference for decay, memory, combat integration
- `BIG_FIVE_PERSONALITY_SYSTEM.md` — OCEAN trait modulation driven by SEC outputs
- `SHADOW_TIMELINE.md` — shadow timeline uses `sec_get_*_final()` for utility scoring
- `EMOTIONAL_PROFILES.md` — builder guide for setting mob profiles (affects SEC baselines)
- `src/sec.h` — authoritative source for all SEC constants, API, and design rationale
