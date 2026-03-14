# RFC-1001: NPC Psychology & Behavior Scientific Checklist
# Vitalia Reborn MUD Engine

**Document Version:** 5.0  
**Date:** 2026-03-14  
**Status:** Updated Analysis & Implementation Reference  
**Purpose:** Professional evaluation of NPC psychological systems for future development planning

> **Revision Note (v5.0):** This document has been updated to reflect the Anchoring Bias
> implementation.  `first_valence` is now stored in every `malp_entry` (set once on creation,
> never updated).  `apply_anchoring_bias()` in `shadow_timeline.c` reads this field and pulls
> Shadow Timeline projections toward the NPC's original reaction, scaled by MPLP
> `SUSPICION_BIAS` and inverse `FORGIVENESS_RATE`.  All five cognitive biases are now fully
> implemented; the Cognitive Bias Module is complete.

> **Revision Note (v4.0):** This document has been updated to reflect the Shadow Timeline
> Cognitive Bias Module implemented as part of the same work cycle.  All four cognitive biases
> (confirmation, availability heuristic, attribution, negativity) now also distort Shadow
> Timeline action-score projections in `shadow_apply_cognitive_biases()` in
> `src/shadow_timeline.c`, called inside `shadow_score_projections()` before moral evaluation.
> The Cognitive Bias system is now **fully complete** across both social (gossip) and cognitive
> (decision-projection) domains.  All status markers, recommendations, and references have been
> updated accordingly.

> **Revision Note (v3.0):** This document has been updated to reflect the Cognitive Bias Gossip
> module implemented after v2.0 (2026-03-07).  Confirmation bias, availability heuristic,
> negativity bias, and attribution bias now modulate social information transmission
> (`try_social_gossip()` in `src/malp.c`) across three stages: topic selection, narrative
> encoding, and reception.  Raw MALP/MPLP data is never mutated by gossip.  All status markers,
> recommendations, and references have been revised accordingly.

> **Revision Note (v2.0):** This document has been updated to reflect systems implemented after
> v1.0 (2026-02-17).  Newly active systems include: Big Five (OCEAN) all five traits, the SEC
> Tetradic Emotion Model, the 4D modified-PAD Relational Decision Space (with Affiliation as the
> fourth axis), the MALP/MPLP Long-Term Emotional Memory system (RFC-1002), and the Emotion
> Contagion system.  All status markers, recommendations, and references have been revised
> accordingly.

---

## Executive Summary

This document provides a comprehensive scientific analysis of the NPC (Non-Player Character) psychology and behavior systems currently implemented in Vitalia Reborn MUD. It evaluates the engine against contemporary psychology, neuroscience, and behavioral science literature.

**Key Findings:**
- ✅ **20-dimensional emotion system** with hybrid mood + relationship memory
- ✅ **Shultz & Darley (1990) moral reasoning** implementation with guilt/blame evaluation
- ✅ **Shadow Timeline RFC-0003** cognitive future simulation system
- ✅ **Group moral dynamics** with peer pressure and collective responsibility
- ✅ **Genetic trait system** with 11 heritable behavioral tendencies
- ✅ **Big Five (OCEAN) personality** — all five traits active (N, C, A, E fully; O in Shadow Timeline)
- ✅ **SEC Tetradic Emotion Model** — four-timescale competing-emotion engine with lateral inhibition
- ✅ **4D modified-PAD Relational Decision Space** — Valence, Arousal, Dominance + **Affiliation** (4th axis)
- ✅ **MALP/MPLP Long-Term Emotional Memory** (RFC-1002) — episodic consolidation with salience, Hebbian trait formation, Peak-End Rule, and reconsolidation
- ✅ **Emotion Contagion** — three-layer emotional spreading (crowd, group, leader)
- ✅ **Cognitive Bias Module** — availability heuristic, negativity bias, confirmation bias, attribution bias, and anchoring bias (first-impression persistence) applied to both NPC social information transmission (gossip) and Shadow Timeline decision projections across all scoring stages
- ⚠️  **FANN neural networks** included but not integrated
- ❌ **DSM-5/ICD-11 neurodivergence** models not implemented

---

## 1. Cognitive and Emotional Foundations

### [✅] Which cognitive behaviors are currently simulated in NPCs/mobs?

**Implemented:**

1. **Attention** (Shadow Timeline RFC-0003):
   - `attention_bias` field in `mob_ai_data` (-50 to +50)
   - Long-term adaptation to prediction patterns
   - Code: `src/structs.h:1268`, `src/shadow_timeline.h:35-149`

2. **Working Memory** (Emotion Memory System):
   - **Dual 20-slot circular buffers** per mob (`EMOTION_MEMORY_SIZE = 20`):
     - `memories[]` — passive buffer (received/witnessed interactions)
     - `active_memories[]` — active buffer (actions performed by the mob)
   - Stores entity interactions with full 20-emotion snapshots
   - Exponential memory intensity decay (`intensity = exp(-λ·age_hours)`) via `MEMORY_DECAY_LAMBDA = 0.70` for normal events and `MEMORY_DECAY_LAMBDA_MAJOR = 0.23` for major events; slots cleared when intensity falls below threshold — no hard 1-hour cutoff
   - Code: `src/structs.h:1115`, `src/structs.h:1219-1266`

3. **Planning** (Goal-Oriented AI):
   - 15 goal types: `GOAL_GOTO_SHOP_TO_SELL`, `GOAL_HUNT_TARGET`, `GOAL_ACCEPT_QUEST`, etc.
   - Goal destination tracking with pathfinding
   - Goal timer to prevent getting stuck
   - Code: `src/structs.h:995-1011`, `src/mobact.c:68-75`

4. **Cognitive Flexibility** (Shadow Timeline Projections):
   - Multiple action projections evaluated (up to 10)
   - Dynamic action selection based on context
   - Adaptive feedback from prediction errors
   - Code: `src/shadow_timeline.h:42-149`

**Example Implementation:**
```c
// Working memory — dual 20-slot circular buffers
struct emotion_memory memories[EMOTION_MEMORY_SIZE];      // passive (received)
struct emotion_memory active_memories[EMOTION_MEMORY_SIZE]; // active (performed)
int memory_index;
int active_memory_index;

// Attention and prediction error tracking
int recent_prediction_error;  // 0-100 smoothed novelty
int attention_bias;            // -50 to +50 long-term adaptation
```

**NOT Implemented:**
- Explicit dual-process theory (System 1 vs System 2)
- Selective attention filtering (all stimuli processed equally)

> **Partially addressed since v1.0:** Executive function inhibition is now partially modelled
> via Conscientiousness (C) in the OCEAN system — high-C mobs slow emotional responses
> (reduced α), require higher arousal to consolidate long-term memories, and apply a
> deliberation bias against impulsive actions.  See the OCEAN section for details.

---

### [✅] Are basic, social, motivational, and empathic emotions modeled quantitatively?

**YES - 20 Distinct Emotions (0-100 scale):**

**Basic Emotions** (4):
- `emotion_fear` - affects fleeing, cautious behavior
- `emotion_anger` - affects aggression, attacking
- `emotion_happiness` - affects positive socials
- `emotion_sadness` - affects withdrawn behavior

**Social Emotions** (4):
- `emotion_friendship` - nearby entity relations
- `emotion_love` - protective behavior
- `emotion_trust` - trade, following
- `emotion_loyalty` - group cohesion

**Motivational Emotions** (3):
- `emotion_curiosity` - exploration, quest acceptance
- `emotion_greed` - looting, hoarding
- `emotion_pride` - reputation-based behavior

**Empathic Emotions** (2):
- `emotion_compassion` - healing, helping
- `emotion_envy` - desire for others' possessions

**Arousal Emotions** (2):
- `emotion_courage` - opposite of fear
- `emotion_excitement` - roaming, activity

**Negative/Aversive Emotions** (5):
- `emotion_disgust` - inappropriate/repulsive reactions
- `emotion_shame` - unwanted/degrading situations
- `emotion_pain` - physical suffering
- `emotion_horror` - extreme fear/revulsion
- `emotion_humiliation` - degradation, dignity loss

**Mood System:**
- `overall_mood` (-100 to +100) derived from emotion averages
- Updated periodically via `mood_timer`

**Code:** `src/structs.h:1194-1233`

**Example:**
```c
// Emotional profile initialization
int emotional_profile;  // EMOTION_PROFILE_AGGRESSIVE, SENSITIVE, etc.

// Extreme states
int berserk_timer;    // Berserk rage state
int paralyzed_timer;  // Paralyzed by fear
```

---

### [✅] Are personality traits mapped using validated models (e.g., Big Five)?

**FULLY IMPLEMENTED — All Five OCEAN Traits Active**

> **Changed since v1.0**: This item was previously ❌.  All five traits are now implemented.

The engine's `struct mob_personality` (defined in `src/structs.h:1064-1079`) implements the Big Five
(OCEAN) model.  Trait base values are Gaussian-generated at spawn, can be adjusted via the builder
(MEDIT), and are modulated at runtime by the SEC competing-emotions layer.

**Scientific basis:** McCrae & Costa (1987); Costa & McCrae (1992) NEO-PI-R; Goldberg (1990)
"An alternative description of personality: The Big Five factor structure."

| Trait | Status | Range | Genetic Basis | Runtime Modulation |
|-------|--------|-------|---------------|--------------------|
| Neuroticism (N) | ✅ Phase 1 ACTIVE | 0.0–1.0 | 0.70×(1−bravery) + 0.30×(1−EI) | +SEC stress signal (fear+anger, capped ±0.05) |
| Conscientiousness (C) | ✅ Phase 2 ACTIVE | 0.0–1.0 | Gaussian spawn + MEDIT modifier | −SEC disgust (capped ±0.05) |
| Agreeableness (A) | ✅ Phase 3 ACTIVE | 0.0–1.0 | Gaussian spawn + MEDIT modifier | k₃×happiness − k₄×anger (capped ±0.10) |
| Extraversion (E) | ✅ Phase 3 ACTIVE | 0.0–1.0 | Gaussian spawn + MEDIT modifier | k₁×happiness − k₂×fear (capped ±0.10) |
| Openness (O) | ✅ Phase 4 ACTIVE | 0.0–1.0 | Gaussian spawn + MEDIT modifier | O_mod = 0 (reserved; never per-tick derived) |

**Three-component model (A, E, O, C):**
```
Trait_final = clamp(Trait_base + Trait_builder_modifier/100 + Trait_emotional_modulation, 0, 1)
```

**Behavioral effects by trait:**

*Neuroticism (N):*
- Amplifies negative emotion responses: `E_raw *= (1 + β_N × N_final)`
- High-N mobs hold fear and anger longer (SEC decay resistance, `SEC_N_FEAR/ANGER_DECAY_COEFF`)
- High-N slows negative MPLP trait decay (MALP N rumination scale)
- Code: `src/sec.h:217-243`, `src/malp.h:176-181`

*Conscientiousness (C):*
- Slows emotional reactivity (lower α = more gradual state changes)
- Raises MALP consolidation threshold θ_eff (fewer impulsive memories)
- Adds a decision-consistency bonus in Shadow Timeline utility scoring
- Code: `src/sec.h:185-215`, `src/malp.h:169-174`

*Agreeableness (A):*
- Damps anger gain: `anger_gain *= (SEC_A_ANGER_DAMP_MAX − A_final)`
- Accelerates anger forgiveness decay
- Reduces aggressive initiation probability; raises grouping bonus
- Code: `src/sec.h:128-183`

*Extraversion (E):*
- Scales social action probability (±20 pp around neutral)
- Awards happiness gain after positive social bonding interactions
- Code: `src/sec.h:142-163`

*Openness (O):*
- Drives novelty preference in Shadow Timeline MOVE scoring
- Depth-aware anti-repetition bonus prevents "ADHD oscillation"
- Structurally constrained: O_mod must never derive from SEC emotions
- Code: `src/sec.h:245-295`

**Code:** `src/structs.h:1034-1079`, `src/sec.h:127-295`, `src/malp.h:169-181`

**References:**
- McCrae, R.R. & Costa, P.T. (1987). Validation of the five-factor model of personality across instruments and observers. *Journal of Personality and Social Psychology*, 52(1), 81–90.
- Costa, P.T. & McCrae, R.R. (1992). *NEO Personality Inventory–Revised (NEO-PI-R) professional manual*. Psychological Assessment Resources.
- Goldberg, L.R. (1990). An alternative description of personality: The Big Five factor structure. *Journal of Personality and Social Psychology*, 59(6), 1216–1229.

---

### [✅] Are temperament (biologically innate) and character (socially learned) differentiated?

**YES - Differentiated via Genetics vs Emotion Memory**

**Temperament (Innate) - `struct mob_genetics`:**
11 heritable genes (0-100):
- `wimpy_tendency` - flee tendency
- `brave_prevalence` - courage prevalence
- `group_tendency` - social affiliation
- `loot_tendency`, `equip_tendency`, `roam_tendency`
- `use_tendency`, `trade_tendency`, `quest_tendency`
- `adventurer_tendency`, `follow_tendency`, `healing_tendency`
- `emotional_intelligence` - volatility & stabilization

**Character (Learned) - Three-Layer Memory System:**
- **Episodic buffer (short-term):** Dual 20-slot buffers track recent interactions with full
  emotional snapshots and moral judgments
- **MALP (Active Long-Term Memory):** High-salience episodes are consolidated into explicit memory
  entries retrievable for social reasoning and dialogue
- **MPLP (Passive Long-Term Memory):** Hebbian co-occurrence builds implicit trait modifiers
  (approach/avoidance/arousal bias) that bias behaviour without conscious access
- Moral learning from guilt/innocence judgments, with regret driving avoidance of repeated harmful
  actions

**Code:** 
- Genetics: `src/structs.h:1016-1031`
- Memory: `src/structs.h:1119-1166`
- Learning: `src/moral_reasoner.c` (moral_record_action, moral_get_learned_bias)

**Example:**
```c
// Temperament: Innate genetics
struct mob_genetics {
    int brave_prevalence;       // Born brave or cowardly
    int emotional_intelligence; // Innate EI capacity
};

// Character Layer 1: Short-term episodic memory (runtime only)
struct emotion_memory memories[EMOTION_MEMORY_SIZE];       // passive
struct emotion_memory active_memories[EMOTION_MEMORY_SIZE]; // active
sh_int moral_was_guilty;   // Learned moral patterns
sh_int moral_regret_level; // Consequences shape behavior

// Character Layer 2: Active Long-Term Memory (MALP — RFC-1002)
struct malp_entry *malp;   // Explicit episodic/semantic entries (salience-consolidated)

// Character Layer 3: Passive Long-Term Memory (MPLP — RFC-1002)
struct mplp_trait *mplp;   // Implicit trait modifiers (Hebbian co-occurrence)
```

---

### [✅] How does long-term memory influence decision-making, emotional responses, and moral reasoning?

**Implemented via Three-Layer Memory System (Short-Term Episodic + MALP + MPLP):**

#### Layer 1 — Episodic Buffer (Short-Term)

**1. Decision-Making Influence:**
- **Moral Learning Bias:**
  ```c
  int moral_get_learned_bias(struct char_data *ch, int action_type);
  // Returns bias (-100 to +100) based on past guilty/innocent actions
  ```
  - Guilty actions → negative bias (discourages repetition)
  - Innocent actions → positive bias (encourages repetition)
  - Weighted by recency (recent memories stronger)

**2. Emotional Response Influence:**
- Memories store complete emotion snapshots (20 emotions)
- Entity recognition: "I remember this player attacked me"
- Emotion modulation: Prior interactions bias current emotions
- Trust/friendship decay or growth based on history

**3. Moral Reasoning Influence:**
- **Regret Calculation:**
  ```c
  moral_regret_level = 
      (shame increase * 2) +
      (disgust increase) +
      (happiness decrease) +
      (severity of outcome / 2)
  ```
- **Learned Avoidance:**
  ```c
  bool moral_has_learned_avoidance(struct char_data *ch, int action_type);
  // TRUE if: 2+ guilty with 0 innocent OR 3+ actions with regret >70
  ```

**4. Memory Decay:**
- Recent interactions (< 5 min): 100% weight
- Fading (15-30 min): 60% weight
- Old (> 60 min): 0% weight (forgotten)

**Code:** 
- `src/moral_reasoner.c`: Lines with moral_get_learned_bias, moral_record_action
- `MORAL_LEARNING_EXAMPLE.md`: Full usage examples

**Example Scenario:**
```
Guard attacks hungry thief → Judgment: GUILTY → Shame +30, Regret 60
Next consideration: Learned bias -45 (discouraged from attacking)
After 3 guilty attacks: Learned avoidance TRUE (refuses to attack)
```

---

#### Layer 2 — MALP: Active Long-Term Memory (RFC-1002)

> **Added since v1.0.** The MALP system provides explicit episodic and semantic long-term memory,
> closing the gap between the short-term buffer and truly persistent social cognition.

The MALP (Memória Ativa de Longo Prazo) consolidates high-salience episodic slots into named
memory entries that can be retrieved deliberately for social reasoning and emotional responses.

**Salience-driven consolidation:**
```
S = normalize( w_a×arousal + w_r×log(1+rehearsal) + w_s×social_weight )
    × (1 + age_hours)^(−0.40)
```
Default weights: `w_a = 0.50`, `w_r = 0.25`, `w_s = 0.25`; normaliser ≈ 1.35.

**Key mechanisms:**
1. **Consolidation gate** — Only episodes with S ≥ θ_cons (default 0.65) enter MALP.
   High-C mobs have an elevated effective threshold (θ_eff = θ_cons + 0.10 × C_final).
2. **Reconsolidation** — Retrieving a MALP entry above P_ret ≥ θ_react (0.55) opens a
   reconsolidation window.  Concurrent interactions can then update the stored valence and
   intensity within bounded limits (MALP_RECON_MAX_DELTA = 0.25), modelling the
   reconsolidation principle (Nader et al., 2000).
3. **Power-law forgetting** — Intensity decays as `intensity × (1 + age_hours)^(−0.40)`.
   High-N mobs show slower negative-trait decay (rumination); high-A mobs show accelerated
   positive-valence reconsolidation.
4. **Feedback loop protection** — Cognitive dominance dampening (MALP_DOMINANCE_THRESHOLD = 0.75)
   prevents a single actor from monopolising the NPC's emotional landscape.
5. **Peak-End Rule** — Episodic valence is consolidated using Kahneman's Peak-End Rule
   (peak weight = 0.60, end weight = 0.40), matching human retrospective memory biases
   (Kahneman et al., 1993).

**Decision-making impact:**
```c
// apply_malp_emotion_effects() — called on interaction with actor
// Retrieves memory, checks dominance/cooldown, applies bounded emotion delta
void apply_malp_emotion_effects(struct char_data *mob, struct char_data *actor,
                                float interaction_valence);
```

**Code:** `src/malp.h`, `src/malp.c`, `md-docs/RFC-1002_MALP_MPLP.md`

**References:**
- Nader, K., Schafe, G.E. & LeDoux, J.E. (2000). Fear memories require protein synthesis in the amygdala for reconsolidation after retrieval. *Nature*, 406, 722–726.
- Kahneman, D., Fredrickson, B.L., Schreiber, C.A. & Redelmeier, D.A. (1993). When more pain is preferred to less: Adding a better end. *Psychological Science*, 4(6), 401–405.
- Bower, G.H. (1981). Mood and memory. *American Psychologist*, 36(2), 129–148. *(state-dependent recall; P_ret cue score)*

---

#### Layer 3 — MPLP: Passive Long-Term Memory (RFC-1002)

The MPLP (Memória Passiva de Longo Prazo) implements implicit long-term trait modifiers formed by
Hebbian co-occurrence — when the same agent appears with consistent valence above a rehearsal
threshold, a persistent implicit trait is crystallised.  These traits bias behaviour automatically
without being narrated.

**Hebbian rule:**
```
Δmagnitude = 0.15 × salience   (capped at +0.30 per call)
```
Running-average valence update:
```
valence_new = valence_old × (1 − α) + current_valence × α   (α = MPLP_VALENCE_LEARNING_RATE = 0.20)
```

**Agent-anchored trait types** (tied to a specific actor):
| Type | Effect |
|------|--------|
| MPLP_TRAIT_AVOIDANCE | Mob actively avoids this agent |
| MPLP_TRAIT_APPROACH | Mob seeks proximity to this agent |
| MPLP_TRAIT_AROUSAL_BIAS | Arousal increases on agent presence |

**Context-global trait types** (anchor = MPLP_GLOBAL_ANCHOR, not agent-specific):
| Type | Effect |
|------|--------|
| MPLP_TRAIT_EXHIBITION_RESPONSE | Enjoyment/aversion to display/confidence socials |
| MPLP_TRAIT_MODESTY_RESPONSE | Preference for reserved vs. explicit contact |
| MPLP_TRAIT_MASCULINITY_RESPONSE | Reaction to masculine-coded socials |
| MPLP_TRAIT_FEMININITY_RESPONSE | Reaction to feminine-coded socials |
| MPLP_TRAIT_ANDROGYNY_TOLERANCE | Tolerance for mixed/androgynous expression |
| MPLP_TRAIT_GENDER_NORM_SENSITIVITY | Amplified reaction to gender-norm violations |

**Code:** `src/structs.h:1299-1355`, `src/malp.h`, `src/malp.c`

**References:**
- Hebb, D.O. (1949). *The Organization of Behavior: A Neuropsychological Theory*. Wiley.
- Squire, L.R. (1992). Declarative and nondeclarative memory: Multiple brain systems supporting learning and memory. *Journal of Cognitive Neuroscience*, 4(3), 232–243.

---

### [✅] Are cognitive biases and heuristics simulated?

> **Added since v2.0.** Four cognitive biases from `mob_ai_data.biases` are now applied to
> NPC gossip (`try_social_gossip()` in `src/malp.c`) across three psychologically-grounded
> stages.  Raw MALP/MPLP entries are never mutated; bias acts only on the locally-copied
> transmission values, satisfying the "no fabrication of base memory" invariant.

**Implemented in gossip social transmission (Phase 1 / 2 / 3):**

#### Phase 1 — Topic Selection (source chooses what to gossip about)

| Bias | Effect | Constant |
|------|--------|----------|
| **Availability heuristic** (Tversky & Kahneman 1973; Rosnow & Fine 1976) | Recency factor `1/(1+age_h/24)` × `availability_bias` × intensity added to the MALP selection score — recent and emotionally vivid memories are preferred. | `MALP_GOSSIP_AVAILABILITY_SELECT_SCALE = 0.30f` |
| **Negativity bias** (Baumeister et al. 2001) | Negative-valence entries (`valence < 0`) receive a selection bonus proportional to `negativity_bias × |valence| × intensity`. | `MALP_GOSSIP_NEGATIVITY_SELECT_SCALE = 0.20f` |
| **Confirmation bias** (Echterhoff & Higgins 2009) | Entries whose sign matches the source's dominant social lean (`MPLP_TRAIT_TRUST_BIAS`) receive a small preference boost — audience-tuning / shared-reality effect. | `MALP_GOSSIP_CONFIRMATION_SELECT_SCALE = 0.15f` |

#### Phase 2 — Encoding (source distorts a local copy of `gossip_valence` — raw MALP untouched)

| Bias | Effect | Constant |
|------|--------|----------|
| **Attribution bias** (Ross 1977; Jones & Harris 1967) | Negative valence amplified by `(1 + attribution_bias × scale)` — "bad entity by nature" framing. Only negative valence is affected. | `MALP_GOSSIP_ATTRIBUTION_ENC_SCALE = 0.25f` |
| **Negativity bias** (Baumeister et al. 2001) | Further amplifies negative `gossip_valence` in transmission; also boosts `gossip_intensity` by up to `negativity_bias × 10%`. | `MALP_GOSSIP_NEGATIVITY_ENC_SCALE = 0.15f`, `MALP_GOSSIP_NEGATIVITY_INTENSITY_BOOST = 0.10f` |

#### Phase 3 — Reception (`lambda_scale` multiplier applied to `transfer_weight` and `gossip_salience`)

| Bias | Effect | Constant |
|------|--------|----------|
| **Confirmation bias** (Echterhoff & Higgins 2009; Fiedler et al. 2004) | `lambda_scale` boosted when gossip confirms existing belief (same valence sign as prior MALP entry); dampened to ≥ 0.1 when it contradicts. | `MALP_GOSSIP_CONFIRMATION_REC_SCALE = 0.40f` |
| **Negativity bias** (Baumeister et al. 2001; Rozin & Royzman 2001) | `lambda_scale` boosted for negative incoming gossip — listeners assign greater evidential weight to threat-related information. | `MALP_GOSSIP_NEGATIVITY_REC_SCALE = 0.25f` |

**Safety invariants preserved:**
- `lambda` (= `transfer_weight × lambda_scale`) is always capped at `MALP_GOSSIP_WEIGHT_CAP = 0.35f`.
- Raw MALP valence/intensity are never written by gossip encoding.
- Gossip can never raise listener's memory above MALP_GOSSIP_WEIGHT_CAP regardless of bias strength.

**Example implementation:**
```c
/* Phase 1 – availability heuristic: recent memories score higher */
if (source->ai_data->biases.availability_bias > 0.0f && e->timestamp > 0) {
    float age_hours = (float)(now - e->timestamp) / 3600.0f;
    float recency   = 1.0f / (1.0f + age_hours / 24.0f);
    score += source->ai_data->biases.availability_bias * recency
             * MALP_GOSSIP_AVAILABILITY_SELECT_SCALE * e->intensity;
}

/* Phase 2 – attribution bias: negative events framed as character flaws */
if (source->ai_data->biases.attribution_bias > 0.0f && gossip_valence < 0.0f)
    gossip_valence *= (1.0f + source->ai_data->biases.attribution_bias
                       * MALP_GOSSIP_ATTRIBUTION_ENC_SCALE);

/* Phase 3 – confirmation bias: confirming gossip accepted more readily */
if (gossip_negative == prior_negative)
    lambda_scale += listener->ai_data->biases.confirmation_bias
                    * MALP_GOSSIP_CONFIRMATION_REC_SCALE;
```

**Code:** `src/malp.c:1578–1889` (`try_social_gossip()`), `src/malp.h:197–226` (constants)

#### Shadow Timeline — Decision Projection Biases

The same four biases are applied a second time, during **action-score projection**, inside
`shadow_apply_cognitive_biases()` (`src/shadow_timeline.c:2294–2303`), which is called
within `shadow_score_projections()` — *before* moral evaluation — implementing the cognitive
pipeline: **Shadow Timeline → Cognitive Bias → Moral reasoning → Action**.

| Bias | Effect on projection score | Constant |
|------|---------------------------|----------|
| **Availability heuristic** | Scans all MALP episodes; most salient `recency × intensity_amp` drives a proportional score multiplier, inflating projections when vivid memories are fresh. | `COGBIAS_AVAILABILITY_MAX = 0.4f` |
| **Confirmation bias** | Reads MPLP `TRUST_BIAS` / `SUSPICION_BIAS` toward the target plus MALP episodic valence to form `prior_valence ∈ [−1,+1]`; matching outcome direction → score boosted; contradicting → score dampened. | `COGBIAS_CONFIRMATION_MAX = 20` pts |
| **Attribution bias (fundamental attribution error)** | Self-serving: NPC always adds `bias × 0.5 × MAX` to its own action scores; when target danger > 30 the NPC attributes danger to the enemy's inherent personality and subtracts a danger-proportional penalty. | `COGBIAS_ATTRIBUTION_MAX = 15` pts |
| **Negativity bias** | `score < 0 → × (1 + bias)`; `score > 0 → × (1 − bias × 0.5)` (floor 0.5×); additionally inflates `danger_level` for negative projections. | `COGBIAS_NEGATIVITY_MAX = 0.2f` (danger amplifier) |

Application order (per issue spec — "Humans bias first, rationalize later"):
1. Availability → 2. Confirmation → 3. Attribution → 4. Negativity

All score mutations are clamped to `[OUTCOME_SCORE_MIN, OUTCOME_SCORE_MAX]` = `[-100, 100]`.
`CONFIG_MOB_4D_DEBUG` logs `COGBIAS-AVAIL:`, `COGBIAS-CONF:`, `COGBIAS-ATTR:`, `COGBIAS-NEG:`.

**Code:** `src/shadow_timeline.c:2013–2395` (Cognitive Bias Module), `src/shadow_timeline.h:167–173` (constants)

#### Anchoring Bias — First-Impression Persistence

The fifth and final cognitive bias.  Unlike confirmation bias (which favours the *current*
belief) or availability (which favours *recent* memories), anchoring favours what came
**first**.  The NPC's brain locks onto the very first interaction valence with an entity and
uses it as a reference point that resists revision regardless of subsequent evidence.

**Storage**: `malp_entry.first_valence` (in `src/structs.h`) is set once on MALP entry
creation and never updated.  This is the meta-memory that holds the first impression.

**Formula**:
```
anchor_residual = first_valence × anchoring_bias × resistance × intensity_floor × ANCHORING_MAX
```

Where:
- `first_valence` = MALP `first_valence` (−1..+1) for the projection target
- `resistance` = `(suspicion_bias + (1 − forgiveness_rate)) / 2` — measures how stubborn the
  NPC is; high suspicion and low forgiveness produce the strongest anchoring
- `intensity_floor` = `max(0.3, malp_intensity)` — anchor retains ≥ 30% potency even as the
  episodic memory fades (anchors outlast normal decay)
- `COGBIAS_ANCHORING_MAX = 25` pts — largest of all bias deltas, reflecting anchoring's
  documented dominance in human belief revision

**Application order** (applied last per specification):
1. Availability → 2. Confirmation → 3. Attribution → 4. Negativity → **5. Anchoring**

**Emergent NPC archetypes**:
- *Grateful ally*: first help → positive anchor; still favours actor even after insults
- *Unforgiving enemy*: first attack → negative anchor; remains suspicious even after aid
- *Stubborn grudge-holder*: high suspicion + low forgiveness = near-permanent anchoring
- *Naive loyalist*: low suspicion + high forgiveness = anchoring fades quickly
- *Traumatised*: major-event first impression → very strong, long-lived anchor

`CONFIG_MOB_4D_DEBUG` logs `COGBIAS-ANCH:` with first_val, resistance, intensity, delta.

**Code:** `src/shadow_timeline.c:2283–2395` (`apply_anchoring_bias()` + updated pipeline),
`src/shadow_timeline.h:172` (`COGBIAS_ANCHORING_MAX`),
`src/structs.h:1387` (`malp_entry.first_valence`), `src/malp.c:604` (first_valence set on entry creation),
`src/quest.c:3510` (`biases.anchoring_bias` default initialisation)

---

### [✅] Is there a structured emotional inference layer above the raw 20-emotion system?

**YES — SEC: Sistema de Emoções Concorrentes (Competing Emotions System)**

> **Added since v1.0.** The SEC is a deterministic, four-timescale engine sitting on top of the
> 20-emotion system.  It feeds post-hysteresis 4D results into a tetradic arousal partition,
> enforces Lateral Inhibition, and drives downstream behaviour via three read-only outputs.

**Scientific basis:** The SEC draws on the dimensional appraisal tradition (Russell, 1980;
Mehrabian & Russell, 1974) and the Lateral Inhibition model of competing emotional states
(Bower, 1981; Lang et al., 1998).

#### Four-timescale model

| Layer | Timescale | Rate | Purpose |
|-------|-----------|------|---------|
| Arousal partition | Fast | — | Active state projection from D×V quadrants |
| Emotional smoothing | Medium | α ≈ 0.40 | Behavioural continuity (F, Sd, An, H) |
| Passive decay | Slow | λ = 0.05 | Homeostatic convergence toward profile baseline |
| Persistent trait | Very slow | δ = 0.01 | Structural memory (Disgust — outside partition) |

#### Tetradic arousal partition

```
A = raw_arousal / 100        (pre-modulation; budget proportional to intrinsic state)
D = (dominance  + 100) / 200 (normalised Dominance)
V = (valence    + 100) / 200 (normalised Valence)

w_fear    = A × (1−D) × V        — low dominance, high valence  (threat / uncertainty)
w_sadness = A × (1−D) × (1−V)   — low dominance, low valence   (passive loss / grief)
w_anger   = A × D × (1−V)       — high dominance, low valence  (active loss / fight)
w_happy   = A × D × V           — high dominance, high valence (approach / reward)

Guarantee: w_fear + w_sadness + w_anger + w_happy = A  (energy conserved)
```

The Sadness quadrant (low D, low V) models the "passive loss" appraisal — the state arising when a
goal is unrecoverable and the mob has no coping capacity.  A concrete example: a mob that has been
guarding a treasure witnesses a thief escape through a locked gate; the goal (protect the treasure)
is irreversibly lost, and the mob has no remaining action that can recover it.  The resulting state
is low-dominance (nothing left to do) and low-valence (bad outcome) — precisely the Sadness quadrant.

#### Lateral Inhibition and the Arousal Budget

The four SEC emotions compete for a shared arousal-partition budget `A = raw_arousal / 100.0`,
enforced and renormalized in the SEC update path (`sec_update_partitioned()` / `sec_update()`).
When one SEC component grows, the others are inhibited so that
`w_fear + w_sadness + w_anger + w_happy = A` remains conserved.  Using `raw_arousal` rather than
effective arousal keeps this budget proportional to the mob's intrinsic state, preventing the
contextual multiplier from trivially locking A = 1.0 in every combat tick.  Separately,
`adjust_emotion()` clamps the *raw* fear/sadness/anger/happiness integer values to [0, 100] each,
but that per-emotion tetrad cap is distinct from the SEC arousal-partition budget.

#### Downstream read-only outputs

| Getter | Effect |
|--------|--------|
| `sec_get_4d_modifier()` | Behaviour intensity multiplier ∈ [0.5, 2.0] |
| `sec_get_flee_bias()` | Flee tendency ∈ [0.0, 1.0] |
| `sec_get_lethargy_bias()` | Suppresses social/idle pulses when Sadness dominates |
| `sec_get_target_bias()` | Target-selection weight ∈ [0.5, 1.5] |
| `sec_get_dominant_emotion()` | Winner-Takes-All filter for social category selection |
| `sec_get_*_final()` | Final OCEAN trait values with emotional modulation applied |

**Code:** `src/sec.h`, `src/sec.c`, `md-docs/SEC_TETRADIC_EMOTION_MODEL.md`

**References:**
- Bower, G.H. (1981). Mood and memory. *American Psychologist*, 36(2), 129–148.
- Lang, P.J., Bradley, M.M. & Cuthbert, B.N. (1998). Emotion, motivation, and anxiety: Brain mechanisms and psychophysiology. *Biological Psychiatry*, 44(12), 1248–1263.

---

### [✅] Is there a structured dimensional model for emotional decision-making?

**YES — 4D Modified-PAD Relational Decision Space (Valence, Arousal, Dominance, Affiliation)**

> **Added since v1.0.** The engine uses a **modified PAD model** extended with a fourth axis,
> **Affiliation**, reflecting the relational orientation toward the interaction target.

**Scientific basis:** Mehrabian & Russell (1974) PAD model (Pleasure-Arousal-Dominance).  The
Vitalia engine extends the canonical three-dimensional space with a fourth axis — Affiliation
(approach/avoidance orientation toward the specific target) — motivated by Horowitz's (1979)
interpersonal circumplex and social neuroscience research on affiliative motivation.

**Axes:**

| Axis | Range | Meaning |
|------|-------|---------|
| Valence | −100 to +100 | Positive vs. negative evaluation of the interaction |
| Arousal | 0 to 100 | Calm to highly activated |
| Dominance | −100 to +100 | Perceived control / assertiveness (subjective) |
| **Affiliation** | **−100 to +100** | **Relational orientation: avoidance → approach** |

A fifth quasi-axis, **Coping Potential** (0–100), provides the *objective* situational capacity
modulator used to adjust the subjective Dominance axis at the Contextual Modulation Layer.  This
separation prevents feedback-loop conflation between perceived and actual control.

**Projection pipeline:**
```
P_base      = M_profile × E
P_raw       = (M_profile + ΔM_personal) × E
P_effective = ContextualMod(P_raw, mob, target, environment, memory, shadow)
```

Where `M_profile` is the profile projection matrix, `ΔM_personal` is the bounded personal drift
(±20% of profile weight), and `E` is the current 20-emotion vector.

**Contextual Modulation Layer:**
```
Dominance   = clamp(raw_D + (coping_potential − 50) × 0.4)
Arousal     = clamp(raw_A × (1 + env_intensity × 0.5))
Affiliation = clamp(raw_Af + relationship_bias)
Valence     = clamp(raw_V + shadow_forecast_bias)
```

The result feeds into the SEC partition, the Shadow Timeline utility scorer, and hysteresis
logic, creating a coherent pipeline from raw emotions to concrete behaviour selection.

**Code:** `src/structs.h:1131-1173`, `src/emotion_projection.h`, `src/emotion_projection.c`

**References:**
- Mehrabian, A. & Russell, J.A. (1974). *An Approach to Environmental Psychology*. MIT Press.
- Horowitz, L.M. (1979). On the cognitive structure of interpersonal problems treated in psychotherapy. *Journal of Consulting and Clinical Psychology*, 47(1), 5–15.
- Russell, J.A. (1980). A circumplex model of affect. *Journal of Personality and Social Psychology*, 39(6), 1161–1178.

---

## 2. Morality and Decision-Making

### [✅] How is the Shultz & Darley moral reasoning system integrated with emotional states?

**Implemented via `moral_reasoner.c`:**

**Shultz & Darley (1990) Core Predicates:**

1. **Harm Causation:**
   ```c
   bool moral_cause(struct moral_scenario *scenario);
   // Rule: produce_harm OR necessary_for_harm OR sufficient_for_harm
   ```

2. **Intent & Planning:**
   ```c
   bool moral_strong_intend(struct moral_scenario *scenario);
   // Rule: mental_state==INTEND OR (plan_known AND plan_include_harm AND harm_as_planned)
   ```

3. **Negligence & Recklessness:**
   ```c
   bool moral_reckless(struct moral_scenario *scenario);
   bool moral_negligent(struct moral_scenario *scenario);
   ```

4. **Responsibility & Blameworthiness:**
   ```c
   bool moral_responsible(struct moral_scenario *scenario);
   // Caused harm + (responsible OR vicarious) + foreseeable
   
   bool moral_blameworthy(struct moral_scenario *scenario);
   // responsible + NOT justified + (negligent OR reckless OR strong_intend)
   ```

**Integration with Emotions:**

```c
// When moral judgment is made, emotions are updated:
void moral_apply_emotional_consequences(struct char_data *ch, struct moral_judgment *judgment) {
    if (judgment->guilty) {
        // Increase shame based on blameworthiness
        ch->ai_data->emotion_shame += (judgment->blameworthiness_score / 2);
        ch->ai_data->emotion_disgust += (judgment->responsibility_score / 3);
        
        // Regret calculation from emotion changes
        int regret = (shame_increase * 2) + disgust_increase + ...;
    } else {
        // Innocent: Increase pride and happiness
        ch->ai_data->emotion_pride += 10;
        ch->ai_data->emotion_happiness += 15;
    }
}
```

**Recorded in Emotion Memory:**
```c
struct emotion_memory {
    ...
    sh_int moral_action_type;      // MORAL_ACTION_ATTACK, STEAL, etc.
    sh_int moral_was_guilty;       // 1=guilty, 0=innocent
    sh_int moral_blameworthiness;  // 0-100
    sh_int moral_outcome_severity; // Actual harm caused
    sh_int moral_regret_level;     // Calculated regret
};
```

**Code:** `src/moral_reasoner.c` (lines 1-500), `src/moral_reasoner.h` (lines 1-150)

**References:**
- Darley, J.M. & Shultz, T.R. (1990). Moral rules: Their content and acquisition. *Annual Review of Psychology*, 41, 525-556.

---

### [✅] Are moral dilemmas or conflicting priorities realistically handled?

**YES - via Shadow Timeline Multi-Projection System**

**Mechanism:**
1. Multiple actions are projected (up to 10 projections)
2. Each projection includes moral cost evaluation
3. Mob selects action with best overall score (combines moral cost + pragmatic outcomes)
4. Conflicting priorities are implicitly resolved by scoring

**Example Dilemma:**
```c
// Starving mob sees food guarded by friendly NPC
Projection 1: ATTACK guard → Score: +60 (food) -70 (moral cost) = -10
Projection 2: ASK for food   → Score: +40 (maybe food) +20 (moral) = +60
Projection 3: STEAL when away → Score: +50 (food) -30 (moral cost) = +20

// Mob chooses Projection 2 (ASK) - best overall score
```

**Moral vs Survival Conflicts:**
- High compassion mobs prioritize moral over survival
- Evil mobs prioritize survival/benefit over morality
- Alignment affects action score weighting

**Group Conflict Resolution:**
- Peer pressure can override individual morality
- Leader influence can force group consensus
- Dissent mechanism allows refusal if conflict is extreme

**Code:** `src/shadow_timeline.c` (action selection), `src/moral_reasoner.c` (moral_evaluate_action_cost)

---

### [✅] Are guilt, shame, regret, or empathy modeled quantitatively?

**YES - All Four are Quantitatively Modeled:**

**1. Guilt (via Moral Judgment):**
- Binary: `moral_judgment.guilty` (0=innocent, 1=guilty)
- Intensity: `moral_judgment.blameworthiness_score` (0-100)
- Triggers emotional consequences (shame increase)

**2. Shame (Emotion):**
- `emotion_shame` (0-100 scale)
- Increases when judged guilty
- Baseline affected by emotional profile
- Code: `src/structs.h:1222`

**3. Regret (Calculated):**
- Computed from emotion changes after action:
  ```c
  regret = (shame_delta * 2) + disgust_delta + 
           (happiness_loss) + (outcome_severity / 2)
  ```
- Stored in emotion memory: `moral_regret_level` (0-100)
- Drives learned avoidance behavior
- Code: `src/moral_reasoner.c` (regret calculation)

**4. Empathy/Compassion (Emotion):**
- `emotion_compassion` (0-100 scale)
- Affects moral cost of harmful actions
- High compassion → strong negative bias for harm
- Influences healing tendency and helping behavior
- Code: `src/structs.h:1214`

**Example:**
```c
// Good guard attacks innocent
moral_judgment: GUILTY, blameworthiness=75
emotion_shame: 15 → 45 (+30)
emotion_disgust: 10 → 25 (+15)
emotion_happiness: 60 → 40 (-20)
outcome_severity: 50 (moderate harm)

moral_regret_level = (30*2) + 15 + 20 + 25 = 120 (capped at 100)
// High regret → strong learned avoidance
```

---

### [✅] Does moral judgment adapt based on past experiences and social feedback?

**YES - via Moral Learning System:**

**1. Individual Learning (Experience-Based):**
```c
int moral_get_learned_bias(struct char_data *ch, int action_type) {
    // Scans emotion memory for past actions of same type
    // Calculates bias from guilt/innocence history and regret
    
    for each memory of action_type:
        if (guilty) {
            bias -= blameworthiness + (regret_bonus);
        } else {
            bias += 30 + (happiness_bonus);
        }
    
    // Weight by recency (recent memories stronger)
    apply_time_decay_weighting();
    
    return bias;  // -100 to +100
}
```

**2. Social Learning (Group Feedback):**
- **Group Reputation Tracking:** Groups gain/lose reputation based on collective actions
- **Peer Pressure:** Group moral stance influences individual decisions
- **Leader Influence:** Charismatic leaders can override individual morality
- **Collective Responsibility:** All group members share guilt/innocence from group actions

**3. Adaptation Mechanism:**
```c
// After each action:
moral_record_action(actor, victim, action_type);
// → Stores moral judgment in emotion memory
// → Next time: moral_get_learned_bias uses this history
// → Behavior adapts (avoids guilty actions, repeats innocent ones)
```

**Emergent Behaviors:**
- **Redemption Arcs:** Evil mobs can learn compassion after repeated guilty judgments
- **Corruption:** Good mobs in evil groups gradually accept immoral actions
- **Moral Rigidity:** High-EI mobs resist peer pressure, maintain moral consistency

**Code:** 
- Individual: `src/moral_reasoner.c` (moral_get_learned_bias, moral_has_learned_avoidance)
- Social: `src/moral_reasoner.c` (moral_apply_peer_pressure, moral_record_group_action)
- Documentation: `MORAL_LEARNING_EXAMPLE.md`, `GROUP_MORAL_DYNAMICS_SUMMARY.md`

---

### [✅] Is moral reasoning modulated by physiological or environmental states?

**YES - Partial Implementation:**

**Physiological States (Implemented):**
1. **Emotional State Modulation:**
   - High fear → moral reasoning impaired (survival prioritized)
   - High anger → reckless behavior, lower moral threshold
   - High compassion → stricter moral judgments
   - Code: Emotions affect moral cost in `moral_evaluate_action_cost()`

2. **Berserk State:**
   - `berserk_timer` > 0 → moral reasoning bypassed
   - Extreme anger state eliminates moral constraints
   - Code: `src/structs.h:1236`

**Environmental States (Partial):**
1. **Weather Effects on Emotions:**
   - Weather affects emotions → emotions affect moral reasoning (indirect)
   - Seasonal affective disorder trait influences baseline emotions
   - Code: `src/structs.h:1239-1247` (weather adaptation system)

**NOT Implemented:**
- Fatigue effects on moral judgment
- Hunger/thirst affecting moral threshold
- Pain intensity modulating decision-making
- Sleep deprivation impairment

**Recommendation:**
Add physiological state modifiers:
```c
struct physiological_state {
    int fatigue_level;      // 0-100
    int hunger_level;       // 0-100
    int pain_level;         // 0-100
    int stress_level;       // 0-100 (cortisol proxy)
};

// In moral evaluation:
int moral_threshold = base_threshold + 
                     (fatigue / 5) +      // Fatigue lowers standards
                     (hunger / 3) +       // Hunger justifies theft
                     (pain / 4) +         // Pain impairs judgment
                     (stress / 2);        // Stress increases recklessness
```

---

## 3. Social Interactions and Group Dynamics

### [✅] Are peer pressure, conformity, and social norms implemented in group decision-making?

**YES - Comprehensive Group Moral Dynamics System:**

**1. Peer Pressure Calculation:**
```c
int moral_apply_peer_pressure(struct char_data *actor, struct char_data *victim, int action_type);
// Returns: -100 to +100 (negative=discourage, positive=encourage)

Algorithm:
1. For each group member:
   - Calculate individual moral stance (based on alignment, emotions, learned biases)
   - Weight by loyalty level
   - Apply emotion modifiers (compassion, courage, loyalty)
2. Aggregate stances with group size amplification
   - 3+ members: +10% pressure
   - 5+ members: +20% pressure
3. Add leader influence bonus
4. Return average peer pressure
```

**2. Conformity Factors:**
- Group size amplification (larger groups = more conformity)
- Loyalty affects conformity strength (high loyalty = more conforming)
- Emotional intelligence provides resistance (high EI = more independent)
- Alignment differences create internal conflict

**3. Social Norms (Implicit):**
- Group reputation defines "normal" behavior for the group
- Good reputation groups discourage harmful actions
- Evil reputation groups normalize violence/theft
- Members internalize group norms over time via moral learning

**4. Integration with Shadow Timeline:**
```c
// Peer pressure automatically applied in action evaluation:
int total_cost = individual_moral_cost + peer_pressure + leader_influence;

// Example:
Individual moral cost: -70 (strong opposition)
Peer pressure: +60 (group encourages)
Leader influence: +30 (charismatic leader)
Total: -70 + 60 + 30 = +20 (action is taken despite individual opposition)
```

**Code:** `src/moral_reasoner.c` (lines 1099-1200), `GROUP_MORAL_DYNAMICS_SUMMARY.md`

**References:**
- Asch, S.E. (1951). Effects of group pressure upon the modification and distortion of judgments.
- Cialdini, R.B. (2001). Influence: Science and practice.

---

### [✅] Is reputation tracking applied within groups, and does it affect behavior?

**YES - Dual Reputation System:**

**1. Individual Reputation:**
```c
struct mob_ai_data {
    int reputation;  // 0-100: individual moral standing
};
```
- Affects quest posting eligibility
- Influences NPC interactions (trade, cooperation)
- Updated based on individual moral judgments

**2. Group Reputation:**
```c
struct group_data {
    int moral_reputation;        // 0-100: group's moral standing
    int collective_guilt_count;  // Number of guilty group actions
    int collective_good_count;   // Number of moral group actions
    time_t last_moral_action;    // Timestamp
};
```

**Reputation Effects on Behavior:**

1. **Inter-Group Interactions:**
   ```c
   int reputation_modifier = calculate_reputation_modifier(group1, group2);
   // Good vs Good: +20 (cooperation bonus, avoid conflict)
   // Evil vs Good: -10 (evil groups prey on good groups)
   // Evil vs Evil: +0 (mutual distrust, but no bonus)
   ```

2. **Intra-Group Dynamics:**
   - High reputation attracts like-minded members
   - Low reputation causes good members to dissent/leave
   - Reputation affects peer pressure strength

3. **Player Interactions:**
   - High reputation groups are friendly to players
   - Low reputation groups are hostile/suspicious
   - Reputation affects quest availability from group members

**Reputation Dynamics:**
- Initialized based on leader alignment
- Updated after each group action (±5 to ±15 per action)
- Collective responsibility: all members' alignments shift with group reputation

**Code:** `src/handler.c` (group creation), `src/moral_reasoner.c` (reputation updates)

---

### [❌] Are in-group vs. out-group biases simulated?

**PARTIALLY IMPLEMENTED**

**Current Implementation:**
- **Group loyalty emotion** affects within-group behavior
- **Inter-group reputation modifiers** create preferential treatment
- **Peer pressure system** implicitly favors in-group consensus

**NOT Implemented:**
- Explicit in-group favoritism mechanics
- Out-group dehumanization or hostility bias
- Minimal group paradigm effects
- Us-vs-them competitive framing

**Partial Proxy:**
```c
// Groups with high moral reputation favor each other:
if (group1->moral_reputation > 70 && group2->moral_reputation > 70) {
    interaction_modifier += 20;  // In-group (good groups stick together)
}

// Evil groups prey on good groups:
if (attacker_group->moral_reputation < 30 && victim_group->moral_reputation > 70) {
    interaction_modifier -= 10;  // Out-group (evil exploits good)
}
```

**Recommendation:**
Add explicit in-group/out-group bias system:
```c
struct group_affiliation {
    int in_group_favoritism;   // 0-100: strength of in-group bias
    int out_group_hostility;   // 0-100: hostility to outsiders
    int tribal_identity;       // Faction/clan membership
};
```

---

### [✅] Can NPCs model Theory of Mind (understand that others have different knowledge or beliefs)?

**PARTIAL - via Shadow Timeline Subjectivity:**

**Implemented:**
1. **Subjective Action Scoring:**
   - Different entities score the same action differently
   - Based on individual emotions, alignment, genetics
   - Code: `src/shadow_timeline.h:12-18` (ST-4: Subjectivity principle)

2. **Entity-Specific Memory:**
   - Mobs remember different interactions with different entities
   - Entity-specific emotion snapshots
   - Code: `src/structs.h:1119-1166` (emotion_memory with entity_id)

**NOT Fully Implemented:**
- No explicit "what does the other entity know?" modeling
- No false belief tasks
- No deception based on knowledge asymmetry
- No perspective-taking beyond emotional differences

**Example of Current Capability:**
```c
// Mob A remembers Player X attacked them (hostile)
// Mob A remembers Player Y healed them (friendly)
// → Different behaviors toward X vs Y based on memory

// BUT:
// Mob A doesn't model: "Player X doesn't know I have a key"
// Mob A doesn't model: "Player Y falsely believes I'm trustworthy"
```

**Recommendation:**
Add knowledge state tracking:
```c
struct entity_knowledge_model {
    long entity_id;
    bool knows_secret[MAX_SECRETS];    // What does this entity know?
    int trust_level;                   // Do I trust their beliefs?
    int deception_intent;              // Am I trying to deceive them?
};
```

---

### [❌] Are long-term social attachments and relational memory considered?

**PARTIALLY — Explicit Long-Term via MALP/MPLP; No Persistent Cross-Session Relationships**

> **Updated since v1.0**: The MALP/MPLP system (RFC-1002) substantially addresses intra-session
> long-term memory.  Cross-session persistence remains absent by design.

**MALP/MPLP Coverage:**
- MALP maintains explicit social memories with salience-weighted consolidation
- Positive approach biases and negative avoidance biases accumulate via MPLP Hebbian learning
- MPLP traits decay continuously with a half-life of 168 h by default
  (`mplp_decay_halflife`, configurable via `CONFIG_MPLP_DECAY_HALFLIFE`), so they can persist
  longer than 7 days with reduced influence; they are runtime-only (not saved across server
  reboots)

**Design Philosophy (preserved from v1.0):**
```c
/**
 * Memories are RUNTIME-ONLY (not persisted to disk)
 * Zone reset/reboot resets mob memories — this is intentional.
 * Allows strategic gameplay: players can wait for zone reset to "reset" relationships.
 */
```

**NOT Implemented:**
- Cross-session persistent relationships (disk-saved)
- Long-term attachment styles (secure, anxious, avoidant)

---

### [✅] Is emotional spread between NPCs modelled?

**YES — Three-Layer Emotion Contagion System**

> **Added since v1.0.**

**Scientific basis:** Hatfield, Cacioppo & Rapson (1994) emotional contagion theory;
Barsade (2002) ripple effect in group dynamics.

The Emotion Contagion system (`update_mob_emotion_contagion()` in `src/utils.c`) simulates how
emotions spread between mobs in the same room across three layers:

| Layer | Scope | Emotions spread |
|-------|-------|-----------------|
| Crowd | All mobs in room | Fear (5–10%), Happiness (8–15%), Anger (3–7%), Excitement (7–12%) |
| Group | Party/group members | Fear (12–20%), Happiness (10–15%) — stronger in-group signal |
| Leader | Leader → followers | Fear (15–25%), Happiness (12–20%), Anger (10–18%) — 2× amplifier |

**Emotional Intelligence modulation:** High-EI mobs are more resistant to contagion; the
`adjust_emotion()` pipeline applies EI dampening to incoming contagion signals.

**Performance gate:** Contagion is skipped when more than 20 NPCs share a room.

**Code:** `src/utils.c:update_mob_emotion_contagion()`, `md-docs/EMOTION_CONTAGION.md`

**References:**
- Hatfield, E., Cacioppo, J.T. & Rapson, R.L. (1994). *Emotional Contagion*. Cambridge University Press.
- Barsade, S.G. (2002). The ripple effect: Emotional contagion and its influence on group behavior. *Administrative Science Quarterly*, 47(4), 644–675.

---

## 4. Simulation of Mental Variability

### [❌] Are deviations in behavior, anxiety, impulsivity, phobias, or compulsions simulated?

**NOT CLINICALLY IMPLEMENTED**

**Current State:**
- **Emotions exist** (fear, courage, etc.) but not clinical anxiety disorders
- **Genetic tendencies** affect behavior but not DSM-5 criteria-based
- **No phobia system** (generalized fear, not specific phobias)
- **No compulsion system** (no OCD-like behaviors)

**Partial Proxies:**
1. **Anxiety-like Behavior:**
   - High fear emotion → cautious, flee-prone behavior
   - `wimpy_tendency` genetic trait → flee easily
   - NOT: Generalized Anxiety Disorder, Panic Disorder criteria

2. **Impulsivity:**
   - Low emotional intelligence → volatile emotions
   - Berserk state → impulsive aggression
   - NOT: Impulse Control Disorders, ADHD criteria

**Recommendation:**
Implement clinical trait system:
```c
struct mental_health_profile {
    // Anxiety spectrum (DSM-5)
    int generalized_anxiety;     // 0-100 (GAD symptoms)
    int social_anxiety;          // 0-100 (Social Phobia)
    int specific_phobias[10];    // Phobia targets (darkness, heights, etc.)
    
    // Impulse control
    int impulsivity_score;       // 0-100 (ADHD, ICD traits)
    int compulsion_strength[5];  // OCD-like behaviors
    
    // Mood disorders
    int depression_severity;     // 0-100 (MDD criteria)
    int mania_level;             // 0-100 (Bipolar traits)
};
```

**Ethical Concern:**
Clinical mental health should NOT be "gamified" (e.g., "anxiety bar"). If implemented, use:
- Research-based symptom clusters
- Respectful representation
- Clear distinction from personality traits
- Educational value, not stigmatization

---

### [✅] Are mental states constrained by ethical or mechanical limits?

**YES - Ethical Design Constraints:**

**1. No Exploitative Mechanics:**
- Emotions are NOT gamified as "mana bars" to exploit
- Mental states are NOT resources to manipulate for advantage
- Psychological suffering is NOT trivialized

**2. Mechanical Limits:**
- Emotion values clamped 0-100 (no overflow)
- Cognitive capacity has minimum threshold (prevents "zero cognition" bugs)
- Memory buffer fixed size (prevents memory bloat)
- Shadow Timeline horizon limited (bounded cognition by design)

**3. Ethical Limits:**
- No perpetual suffering states (emotions decay over time)
- No permanent psychological damage (memories reset on zone reset)
- No exploitation of trauma responses for gameplay advantage

**Code Evidence:**
```c
// Emotion clamping
emotion = MAX(0, MIN(100, emotion));

// Cognitive capacity minimum
if (cognitive_capacity < COGNITIVE_CAPACITY_MIN) {
    return;  // Cannot project if cognitively exhausted
}

// Memory decay (prevents perpetual trauma)
time_diff = current_time - memory->timestamp;
if (time_diff > 3600) {  // 1 hour
    weight = 0;  // Forgotten
}
```

---

## Recommendations for Future Development

### Completed Enhancements (v2.0)

The following items from the v1.0 recommendation list have been implemented and are now ✅ active:

- **Big Five Personality Mapping** — all five OCEAN traits implemented (N, C, A, E fully; O in Shadow Timeline).
- **Long-Term Emotional Memory (MALP/MPLP)** — RFC-1002 fully implemented with salience consolidation, Peak-End Rule, reconsolidation, and Hebbian implicit trait formation.

### Completed Enhancements (v3.0)

The following items from the v2.0 recommendation list have been implemented and are now ✅ active:

- **Cognitive Biases in Gossip (Social Domain)** — confirmation bias, availability heuristic,
  negativity bias, and attribution bias modulate `try_social_gossip()` across three stages
  (topic selection, encoding, reception).

### Completed Enhancements (v4.0)

The following items from the v3.0 recommendation list have been implemented and are now ✅ active:

- **Cognitive Biases in Shadow Timeline Projections** — the same four biases (confirmation,
  availability, attribution, negativity) are applied to every Shadow Timeline action-score
  projection in `shadow_apply_cognitive_biases()` before moral evaluation.

### Completed Enhancements (v5.0)

The following items from the v4.0 recommendation list have been implemented and are now ✅ active:

- **Anchoring Bias** — first-impression valence (`malp_entry.first_valence`) is captured once on
  MALP entry creation and used by `apply_anchoring_bias()` to pull Shadow Timeline projections
  toward the NPC's original reaction.  Resistance is modulated by MPLP `SUSPICION_BIAS` and
  inverse `FORGIVENESS_RATE`.  The Cognitive Bias Module is now **fully complete** — all five
  biases (confirmation, availability, attribution, negativity, anchoring) are implemented across
  both the social (gossip) domain and the Shadow Timeline decision domain.

### Priority 1: High Impact, Moderate Complexity

1. **Physiological Needs System:**
   - Add hunger, thirst, fatigue, sleep deprivation
   - Integrate with moral decision-making (desperation justifies theft)
   - Enables survival-driven emergent behavior
   - **Estimated Effort:** 3-4 weeks

### Priority 2: Medium Impact, Low Complexity

3. **Inhibitory Control System (Enhancement):**
   - Conscientiousness (C) provides partial inhibitory control.  Full executive-function
     inhibition (prepotent response suppression) would extend this to non-deliberative contexts.
   - Modulated by stress, fatigue, emotional state
   - **Estimated Effort:** 1-2 weeks

4. **Flow State & Burnout:**
   - Challenge-skill balance detection for flow
   - Chronic stress accumulation for burnout
   - Enables performance psychology simulation
   - **Estimated Effort:** 2 weeks

5. **Persistent Cross-Session Relationships (optional, disk-saved):**
   - Attachment styles (secure, anxious, avoidant) for key NPCs
   - **Estimated Effort:** 2-3 weeks

### Priority 3: High Impact, High Complexity

6. **Theory of Mind:**
   - Model other entities' knowledge states
   - False belief tasks, deception based on knowledge asymmetry
   - Enables strategic social behavior
   - **Estimated Effort:** 4-6 weeks

7. **FANN Neural Network Integration:**
   - Connect existing FANN library to mob behavior
   - Train networks on behavioral patterns
   - Enables adaptive learning beyond rules
   - **Estimated Effort:** 6-8 weeks

8. **Neurodivergence Profiles (Optional, Ethical):**
   - Research-based ASD, ADHD models
   - Respectful representation, educational value
   - Consult neurodivergent community for input
   - **Estimated Effort:** 8-12 weeks (including consultation)

### Priority 4: Low Priority, High Complexity

9. **Neurochemical Simulation:**
    - Cortisol, dopamine, serotonin, oxytocin
    - Modulate emotions and decision-making
    - High complexity for uncertain benefit
    - **Estimated Effort:** 6-8 weeks

---

## Conclusion

**Overall Assessment: ★★★★★ (5/5 Stars)**

**Strengths:**
1. ✅ Comprehensive 20-emotion system with quantitative modeling
2. ✅ Research-based moral reasoning (Shultz & Darley 1990)
3. ✅ Memory-based learning with regret and avoidance
4. ✅ Group moral dynamics with peer pressure and collective responsibility
5. ✅ Shadow Timeline cognitive future simulation (RFC-0003)
6. ✅ Genetic trait system differentiates temperament from character
7. ✅ Full Big Five (OCEAN) personality — all five traits implemented and active
8. ✅ SEC Tetradic Emotion Model with Lateral Inhibition
9. ✅ 4D modified-PAD Relational Decision Space with Affiliation axis
10. ✅ MALP/MPLP Long-Term Emotional Memory (RFC-1002) with salience consolidation, Peak-End Rule, reconsolidation, and Hebbian implicit traits
11. ✅ Emotion Contagion (three-layer: crowd, group, leader)
12. ✅ **Cognitive Bias Module** — all five biases fully implemented across both domains:
    - *Availability*, *Negativity*, *Confirmation*, *Attribution*: applied to gossip (three-phase
      pipeline: topic selection, encoding, reception) and to Shadow Timeline projection scoring
    - *Anchoring*: first-impression `first_valence` stored in MALP meta-memory; pulls projections
      toward original reaction scaled by MPLP suspicion and inverse forgiveness; outlasts normal
      memory decay via 30% intensity floor
    - Raw MALP/MPLP data invariants preserved throughout — biases affect scoring only
13. ✅ Ethical design: no stigmatization, respectful representation
14. ✅ Transparent, documented, traceable systems

**Remaining Weaknesses:**
1. ❌ No physiological needs (hunger, fatigue, sleep)
2. ❌ No flow states or burnout simulation
3. ❌ No neurochemical modeling (cortisol, dopamine, etc.)
4. ❌ No cross-session persistent relationships (intentional design trade-off)
5. ❌ No neurodivergence profiles (ASD, ADHD)

**Partial Implementation:**
- ⚠️  FANN neural networks available but not yet integrated into mob behaviour

**Scientific Rigor:**
- Strong theoretical foundations (Shultz & Darley, Asch, Simon, Kahneman)
- Research-based emotion categories (Ekman, Plutchik)
- PAD dimensional model extended with Affiliation axis (Mehrabian & Russell; Horowitz)
- MALP/MPLP grounded in memory consolidation and reconsolidation literature (Nader et al., 2000; Squire, 1992)
- Hebbian implicit learning in MPLP (Hebb, 1949)
- Peak-End Rule for episodic valence (Kahneman et al., 1993)
- Emotion Contagion theory (Hatfield et al., 1994; Barsade, 2002)
- Big Five OCEAN model (Costa & McCrae, 1992)
- Cognitive Bias Module (gossip + Shadow Timeline projections) grounded in social cognition literature (Tversky & Kahneman, 1973; Baumeister et al., 2001; Ross, 1977; Echterhoff & Higgins, 2009; Rozin & Royzman, 2001); Anchoring bias grounded in Tversky & Kahneman (1974) "Judgment under Uncertainty: Heuristics and Biases"
- Ethical design principles followed
- Clear distinction between emotions and clinical conditions
- Comprehensive documentation with code references

**Production Readiness:**
- ✅ Stable, well-tested emotion and moral systems
- ✅ Performance-optimized (minimal memory/CPU overhead)
- ✅ Modular design allows incremental enhancements
- ✅ Open source, transparent, ethically documented
- ✅ OCEAN personality fully integrated with no performance regression
- ✅ MALP/MPLP memory systems with runtime-only design (no disk I/O overhead)
- ✅ Emotion Contagion with crowd-size performance gate (>20 NPCs = skip)
- ✅ Cognitive Bias Module zero-cost when biases are 0.0f (all scale paths branch-free skipped); applies to both gossip (malp.c) and Shadow Timeline (shadow_timeline.c)

**Recommendation:**
The Vitalia Reborn NPC psychology system is **production-ready** with strong foundations. Priority 1 and 2 enhancements would elevate it to cutting-edge status. The system provides realistic, adaptive mob behavior suitable for deep, immersive gameplay.

---

**End of RFC-1001**

**Document History:**
- v1.0 (2026-02-17): Initial comprehensive analysis for professional evaluation
- v2.0 (2026-03-07): Updated to reflect Big Five OCEAN (all 5 traits), SEC Tetradic Emotion Model,
  4D modified-PAD Relational Decision Space (Affiliation as 4th axis), MALP/MPLP Long-Term Memory
  (RFC-1002), Emotion Contagion system, and dual 20-slot memory buffers; added scientific
  references for all new systems; updated Recommendations and Conclusion.
- v3.0 (2026-03-14): Updated to reflect Cognitive Bias Gossip Module — confirmation bias,
  availability heuristic, negativity bias, and attribution bias now modulate `try_social_gossip()`
  across three stages (topic selection, encoding, reception); raw MALP/MPLP data invariants
  preserved; added scientific references (Tversky & Kahneman 1973, Baumeister et al. 2001,
  Ross 1977, Echterhoff & Higgins 2009, Rozin & Royzman 2001, Fiedler et al. 2004, Jones &
  Harris 1967, Rosnow & Fine 1976); updated Recommendations (Cognitive Biases partially done),
  Conclusion Strengths, and Remaining Weaknesses accordingly.
- v4.0 (2026-03-14): Updated to reflect Shadow Timeline Cognitive Bias Module — all four biases
  (confirmation, availability, attribution, negativity) now also applied to Shadow Timeline
  action-score projections in `shadow_apply_cognitive_biases()` (src/shadow_timeline.c:2294)
  called inside `shadow_score_projections()` before moral evaluation; removed "NOT YET
  implemented" Shadow Timeline note; added "Anchoring Bias" as new Priority 1 item 2; updated
  Completed Enhancements (v4.0), Conclusion Strengths, Production Readiness, and Scientific Rigor.
- v5.0 (2026-03-14): Implemented Anchoring Bias — fifth and final cognitive bias.  Added
  `first_valence` field to `malp_entry` (set once on creation, never updated); added
  `anchoring_bias` field to `cognitive_biases` struct; implemented `apply_anchoring_bias()` in
  `shadow_timeline.c` (applied last in pipeline, scales by MPLP SUSPICION_BIAS and inverse
  FORGIVENESS_RATE, with 30% intensity floor); added `COGBIAS_ANCHORING_MAX=25`;
  initialised `biases.anchoring_bias` in `init_mob_ai_data()`.  Cognitive Bias Module is now
  **fully complete**; removed anchoring from Remaining Weaknesses and Priority 1;
  added Completed Enhancements (v5.0); updated scientific references (Tversky & Kahneman 1974).

**For Questions or Discussion:**
- GitHub Issues: https://github.com/Forneck/vitalia-reborn/issues
- Development Team: See CONTRIBUTORS.md

**Academic References Cited:**
See individual sections for detailed citations.  Key works include:
- Asch (1951), Cialdini (2001) — Social conformity and peer pressure
- Barsade (2002) — Emotional contagion in group dynamics
- Baumeister, Bratslavsky, Finkenauer & Vohs (2001) — Bad is stronger than good (negativity bias)
- Bower (1981) — Mood, memory, and state-dependent recall
- Costa & McCrae (1992) — Big Five NEO-PI-R
- Csikszentmihalyi (1990) — Flow theory
- Darley & Shultz (1990) — Moral rules and blame evaluation
- DSM-5 (2013), ICD-11 (2018) — Clinical reference (future use)
- Echterhoff & Higgins (2009) — Shared reality and audience-tuning in social communication (confirmation bias in gossip)
- Ekman (1992) — Basic emotion categories
- Fiedler, Walther, Armbruster, Fay & Naumann (2004) — The calibration of information in epistemic contexts (confirmation receptivity)
- Goldberg (1990) — Alternative Big Five description
- Hatfield, Cacioppo & Rapson (1994) — Emotional contagion theory
- Hebb (1949) — Hebbian learning rule (implicit memory formation)
- Horowitz (1979) — Interpersonal circumplex (Affiliation axis basis)
- Jones & Harris (1967) — The attribution of attitudes (correspondent inference / fundamental attribution error)
- Kahneman et al. (1993) — Peak-End Rule for episodic memory
- Lang, Bradley & Cuthbert (1998) — Lateral inhibition and competing emotions
- McCrae & Costa (1987) — Big Five validation
- Mehrabian & Russell (1974) — PAD (Pleasure-Arousal-Dominance) dimensional model
- Nader, Schafe & LeDoux (2000) — Memory reconsolidation
- Rosnow & Fine (1976) — Rumor and gossip: The social psychology of hearsay (availability heuristic in gossip)
- Ross (1977) — The intuitive psychologist and his shortcomings: Distortions in the attribution process
- Rozin & Royzman (2001) — Negativity bias, negativity dominance, and contagion
- Russell (1980) — Circumplex model of affect
- Simon (1956) — Bounded rationality
- Squire (1992) — Declarative and nondeclarative memory systems
- Tversky & Kahneman (1973) — Availability: A heuristic for judging frequency and probability
