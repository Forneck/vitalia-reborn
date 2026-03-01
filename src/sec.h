/**
 * @file sec.h
 * SEC – Sistema de Emoções Concorrentes (Competing Emotions System)
 *
 * A deterministic, 4D-driven internal emotional inference layer for mobs.
 * SEC receives post-hysteresis 4D results and partitions Arousal into
 * competing emotional projections (fear, sadness, anger, happiness), maintaining
 * bounded emotional state with passive decay toward a personality baseline.
 *
 * Design constraints:
 *  - SEC exposes only read-only getters; no external system writes sec_state.
 *  - SEC does not choose targets, execute combat logic, or call flee.
 *  - SEC does not modify hysteresis logic directly.
 *  - Input must be a post-hysteresis emotion_4d_state.
 *
 * ── Four-timescale model ──────────────────────────────────────────────────
 *
 *   Layer              Timescale    Rate    Purpose
 *   ─────────────────  ──────────   ──────  ───────────────────────────────
 *   Arousal partition  Fast         —       Active state projection (V,D,A)
 *   Emotional smooth.  Medium       α≈0.40  Behavioral continuity (F,Sd,An,H)
 *   Passive decay      Slow         λ=0.05  Homeostatic convergence to base
 *   Persistent trait   Very slow    δ=0.01  Structural memory (Disgust)
 *
 * Arousal partition (all values from post-hysteresis 4D state, normalised to [0, 1]):
 *   A = r->raw_arousal / 100  — pre-modulation projection
 *   D = (dominance + 100) / 200
 *   V = (valence  + 100) / 200
 *
 *   Tetradic 4-way split (quadrants in D×V space):
 *   w_fear    = A * (1 − D) * V        — low dominance, high valence  (threat/uncertainty)
 *   w_sadness = A * (1 − D) * (1 − V)  — low dominance, low valence   (passive loss / grief)
 *   w_anger   = A * D * (1 − V)        — high dominance, low valence  (active loss / fight)
 *   w_happy   = A * D * V              — high dominance, high valence  (approach / reward)
 *   → fear_target + sadness_target + anger_target + happiness_target = A  (guaranteed)
 *
 * ── Lateral Inhibition and the Arousal Budget ─────────────────────────────
 *
 * The four competing SEC emotions (Fear, Sadness, Anger, Happiness) share a single
 * Arousal budget A ∈ [0, 1].  True Lateral Inhibition requires that increasing
 * one emotion forces the others to decrease proportionally.  This is only
 * possible if the budget is strictly bounded.
 *
 * Raw-emotion invariant (enforced in adjust_emotion()):
 *   emotion_fear + emotion_sadness + emotion_anger + emotion_happiness <= 100  (one Arousal unit)
 *
 * SEC budget source: raw_arousal (pre-modulation) rather than the effective
 * arousal (post-modulation).  The contextual Arousal multiplier (combat
 * intensity, crowd density) can inflate the effective arousal to 100 in nearly
 * every combat tick, which would fix A = 1.0 permanently and make Lateral
 * Inhibition mathematically impossible — multiple partition slots could
 * simultaneously saturate their ceiling.  Using raw_arousal keeps A
 * proportional to the mob's intrinsic emotional state.
 *
 * Emotional smoothing (α) applied after partition each tick:
 *   emotion_new = emotion_old * (1 − α) + target * α
 *
 * Helplessness:
 *   target = 1 − D
 *   h_new  = h_old * (1 − α_h) + target * α_h   (exponential smoothing)
 *
 * Passive decay (when A < ε):
 *   emotion_new = emotion_old * (1 − λ) + baseline * λ
 *   Ensures asymptotic convergence to baseline.  No divergence, no oscillation.
 *
 * Persistent trait — Disgust (δ update, out-of-partition):
 *   disgust_new = disgust_old * (1 − δ) + event_value * δ
 *   Very small δ ensures disgust changes slowly, accumulating structural memory.
 *   Disgust does not compete for Arousal.
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#ifndef _SEC_H_
#define _SEC_H_

#include "structs.h"

/* sec_state and sec_baseline are defined in structs.h (inside mob_ai_data) */

/* ── Dominant-emotion constants (used by Winner-Takes-All filter) ─────────── */

/** No dominant emotion — arousal is below threshold. */
#define SEC_DOMINANT_NONE 0
/** Fear is the dominant SEC emotion. */
#define SEC_DOMINANT_FEAR 1
/** Anger is the dominant SEC emotion. */
#define SEC_DOMINANT_ANGER 2
/** Happiness is the dominant SEC emotion. */
#define SEC_DOMINANT_HAPPINESS 3
/** Sadness is the dominant SEC emotion (low valence + low dominance / passive loss). */
#define SEC_DOMINANT_SADNESS 4

/**
 * Winner-Takes-All threshold: a social category's driving emotion must be
 * at least this fraction of the dominant SEC weight to be allowed.
 * Below this ratio the selector falls back to neutral socials.
 */
#define SEC_WTA_THRESHOLD 0.60f

/**
 * Arousal epsilon: normalised arousal below this value is treated as
 * quiescent.  Shared by passive-decay guard, partition normalisation, and
 * the WTA filter so all three use the same "emotionally inert" definition.
 */
#define SEC_AROUSAL_EPSILON 0.05f

/**
 * sec_get_4d_modifier() weighting coefficients.
 * Sadness has a stronger suppressive effect than helplessness because it
 * represents a sustained passive-loss state (grief/apathy) rather than a
 * momentary loss-of-control signal.  At sadness=1 the modifier is reduced
 * by 0.8×0.5 = 0.40 (nearly the maximum downward range of the modifier).
 */
#define SEC_MOD_HELPLESSNESS_WEIGHT 0.5f /**< Helplessness contribution to 4D modifier suppression */
#define SEC_MOD_SADNESS_WEIGHT 0.8f      /**< Sadness contribution to 4D modifier suppression */

/**
 * Lethargy suppression threshold: when sec_get_lethargy_bias() equals or
 * exceeds this value the mob is strongly lethargic and skips all social
 * pulse actions entirely.  Below this threshold the social chance is
 * scaled down proportionally by the lethargy bias.
 */
#define SEC_LETHARGY_SUPPRESS_THRESHOLD 0.70f

/* ── OCEAN A/E modulation constants ─────────────────────────────────────── */

/**
 * Agreeableness anger-gain damping: Anger_gain *= (SEC_A_ANGER_DAMP_MAX - A_final).
 * At A=0: gain * SEC_A_ANGER_DAMP_MAX (full escalation).
 * At A=1: gain * SEC_A_ANGER_DAMP_MIN (minimal escalation).
 */
#define SEC_A_ANGER_DAMP_MAX 1.2f
#define SEC_A_ANGER_DAMP_MIN 0.2f

/**
 * Agreeableness forgiveness decay: decay *= (SEC_A_FORGIVE_BASE + A_final).
 * At A=0: decay * SEC_A_FORGIVE_BASE (slowest forgiveness).
 * At A=1: decay * (SEC_A_FORGIVE_BASE + 1.0) (fastest forgiveness).
 */
#define SEC_A_FORGIVE_BASE 0.8f

/**
 * Extraversion social-probability sensitivity.
 * Social chance modifier = (E_final - SEC_E_SOCIAL_CENTER) * SEC_E_SOCIAL_SCALE.
 * Range: [-SEC_E_SOCIAL_SCALE/2, +SEC_E_SOCIAL_SCALE/2] percentage points.
 */
#define SEC_E_SOCIAL_CENTER 0.5f
#define SEC_E_SOCIAL_SCALE 20.0f

/**
 * Extraversion social reward gain (Phase 3).
 * After a positive social interaction, happiness += clamp((E_final - 0.5) * scale, 0, 5).
 * High E → small but consistent happiness gain from successful social bonding.
 * Scale of 10 → max reward of +5 happiness at E=1.0; neutral at E=0.5; no penalty below.
 */
#define SEC_E_SOCIAL_REWARD_SCALE 10.0f

/**
 * Agreeableness aggressive initiation resistance (Phase 3).
 * impulse_threshold -= (int)((A_final - 0.5) * SEC_A_AGGR_SCALE).
 * High A → lower impulse threshold (less likely to initiate attacks).
 * Low A  → higher impulse threshold (more likely to initiate attacks).
 * Range: [-10, +10] percentage-point modification to impulse_threshold.
 */
#define SEC_A_AGGR_SCALE 20.0f

/**
 * Agreeableness group cooperation bonus (Phase 3).
 * grouping_bonus = (int)((A_final - 0.5) * SEC_A_GROUP_SCALE).
 * High A → higher chance of forming/joining groups (cooperative disposition).
 * Low A  → lower chance (solitary/territorial disposition).
 * Range: [-10, +10] percentage points added to grouping chance check.
 */
#define SEC_A_GROUP_SCALE 20.0f

/* ── OCEAN C modulation constants ───────────────────────────────────────── */

/**
 * Conscientiousness SEC alpha regulation.
 * alpha_effective = SEC_EMOTION_ALPHA * (SEC_C_ALPHA_SCALE_MAX - C_final * SEC_C_ALPHA_SCALE_RANGE).
 * At C=0: alpha_effective = SEC_EMOTION_ALPHA * SEC_C_ALPHA_SCALE_MAX (fastest response).
 * At C=1: alpha_effective = SEC_EMOTION_ALPHA * (SEC_C_ALPHA_SCALE_MAX - SEC_C_ALPHA_SCALE_RANGE)
 *                         = SEC_EMOTION_ALPHA * 0.70 (most resistant to spikes).
 * alpha_effective is always clamped to [SEC_C_ALPHA_MIN, SEC_C_ALPHA_MAX] to prevent
 * freeze-state or instability.
 */
#define SEC_C_ALPHA_SCALE_MAX 1.20f   /* multiplier at C=0 */
#define SEC_C_ALPHA_SCALE_RANGE 0.50f /* total range across [0,1] */
#define SEC_C_ALPHA_MIN 0.05f         /* never allow α to reach 0 */
#define SEC_C_ALPHA_MAX 0.95f         /* never allow α to reach 1 */

/**
 * Conscientiousness emotional persistence (decay resistance).
 * decay_effective = base_decay * (SEC_C_DECAY_BASE - SEC_C_DECAY_RANGE * C_final).
 * At C=0: decay * SEC_C_DECAY_BASE        (fastest decay — volatile).
 * At C=1: decay * (SEC_C_DECAY_BASE - SEC_C_DECAY_RANGE) = decay * 0.60 (slowest — persistent).
 */
#define SEC_C_DECAY_BASE 1.20f  /* multiplier at C=0 */
#define SEC_C_DECAY_RANGE 0.60f /* total range across [0,1] */

/**
 * Conscientiousness decision consistency bias in Shadow Timeline utility.
 * score bonus = SEC_C_CONSISTENCY_SCALE * C_final when action matches prior commitment.
 * Range: [0, +SEC_C_CONSISTENCY_SCALE] score points.
 */
#define SEC_C_CONSISTENCY_SCALE 12

/* ── OCEAN N modulation constants ───────────────────────────────────────── */

/**
 * Neuroticism decay resistance for negative emotions.
 * Negative-emotion decay *= (SEC_N_DECAY_BASE - coeff * N_final).
 * At N=0: decay * SEC_N_DECAY_BASE (normal speed).
 * At N=1: decay * (SEC_N_DECAY_BASE - coeff)  (slowest — holds negative states).
 * Lower bound (SEC_N_DECAY_MIN_SCALE) prevents decay from reaching 0.
 */
#define SEC_N_FEAR_DECAY_BASE 1.20f  /* at N=0 */
#define SEC_N_FEAR_DECAY_COEFF 0.60f /* total range; at N=1: 1.20-0.60 = 0.60 */
#define SEC_N_ANGER_DECAY_BASE 1.20f
#define SEC_N_ANGER_DECAY_COEFF 0.50f /* at N=1: 1.20-0.50 = 0.70 */
#define SEC_N_DECAY_MIN_SCALE 0.20f   /* lower bound: decay never drops below 20% */

/**
 * Neuroticism emotional modulation cap.
 * N_mod driven by SEC fear+anger stress signal (prevents recursive loop).
 * |N_mod| <= SEC_N_MOD_CAP to prevent structural inversion.
 */
#define SEC_N_MOD_CAP 0.05f

/**
 * Neuroticism stress signal scale: fraction of (sec.fear + sec.anger) used as modulation.
 * N_mod = clamp(SEC_N_STRESS_SCALE * (fear + anger), -SEC_N_MOD_CAP, +SEC_N_MOD_CAP).
 */
#define SEC_N_STRESS_SCALE 0.05f

/* ── OCEAN O modulation constants ───────────────────────────────────────── */

/**
 * Openness novelty weighting in Shadow Timeline MOVE scoring.
 * High O → favours exploring novel/unfamiliar rooms; low O → prefers familiar territory.
 * Novelty bonus = SEC_O_NOVELTY_SCALE * (O_final - SEC_O_NOVELTY_CENTER) per MOVE action.
 * Range: [-SEC_O_NOVELTY_SCALE/2, +SEC_O_NOVELTY_SCALE/2] score points.
 *
 * NOTE: Openness must NEVER be derived from SEC emotional state (no O_mod from fear/anger).
 * O_mod is reserved for extremely slow long-term adaptation only (±0.05 cap, not per-tick).
 * For now O_mod = 0; this constant documents the architectural constraint.
 */
#define SEC_O_NOVELTY_CENTER 0.5f /* baseline O value — no bonus/penalty */
#define SEC_O_NOVELTY_SCALE 14.0f /* max bonus/penalty ∈ [-7, +7] score points */
#define SEC_O_MOD_CAP 0.05f       /* max |O_mod|: slow adaptation only, never per-tick */

/**
 * Openness action-history modifiers (applied in shadow_score_outcome).
 *
 * Depth-aware novelty model — prevents oscillation in high-O mobs:
 *   novelty_bonus = clamp(O_final × depth × SEC_O_NOVELTY_DEPTH_SCALE, 0, SEC_O_NOVELTY_BONUS_CAP)
 *   where depth = min(action_repetition_count, SEC_O_REPETITION_CAP).
 *
 * Behaviour by repetition depth (at O=1.0):
 *   depth=0 (no prior): 0 pts — no pressure to switch
 *   depth=1:  6 pts — gentle pressure
 *   depth=3: 18 pts — moderate pressure
 *   depth=5: 30 pts — hard cap (= 0.3 × OUTCOME_SCORE_MAX)
 * At O=0.5: all values halved.  At O=0: bonus is always 0.
 *
 * Because the bonus only builds after sustained same-type repetitions, high-O
 * mobs do NOT flip on every tick: novelty pressure must exceed the action's
 * base survival utility before a switch occurs — preventing "ADHD oscillation."
 *
 * SEC_O_REPETITION_BONUS: score bonus for *repeating* the last action type (low O = routine).
 *   At O=0: +SEC_O_REPETITION_BONUS pts.  At O=1: 0 pts.
 *
 * SEC_O_EXPLORATION_BASE: base exploration probability (integer %).
 *   ExplorationChance = SEC_O_EXPLORATION_BASE × O_final → 0% at O=0, 20% at O=1.
 *   Used in shadow_select_best_action() to occasionally pick a sub-dominant action.
 *   Must not bypass WTA energy gating.
 *
 * SEC_O_THREAT_BIAS: scaling factor for threat amplification reduction.
 *   ThreatAmpPct = 30 × (1 - SEC_O_THREAT_BIAS × O_final) ∈ [18, 30] percent.
 */
#define SEC_O_NOVELTY_DEPTH_SCALE 6 /* pts per repetition-depth step at O=1 */
#define SEC_O_NOVELTY_BONUS_CAP 30  /* hard cap = 0.3 × OUTCOME_SCORE_MAX */
#define SEC_O_REPETITION_CAP 5      /* bonus plateaus after this many consecutive repeats */
#define SEC_O_REPETITION_BONUS 15   /* pts at O=0 for same action type as last tick */
#define SEC_O_EXPLORATION_BASE 20   /* % × O_final; range [0, 20%] exploration chance */
#define SEC_O_THREAT_BIAS 0.4f      /* reduces threat amp: 30%×(1-0.4×O) ∈ [18%,30%] */

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * Initialise SEC state and baseline for a mob.
 * Must be called once after mob_ai_data is allocated and the emotional
 * profile has been set.
 */
void sec_init(struct char_data *mob);

/**
 * Update SEC state from a post-hysteresis 4D result.
 * Must be called every tick after compute_emotion_4d_state() and the
 * helplessness deformation step have been applied.
 *
 * @param mob  The NPC to update.
 * @param r    Post-hysteresis 4D state (must have r->valid == TRUE).
 */
void sec_update(struct char_data *mob, const struct emotion_4d_state *r);

/**
 * Apply passive decay toward the personality baseline.
 * Should be called every tick; internally guards on the arousal threshold
 * (SEC_AROUSAL_EPSILON) so it is safe to call unconditionally.
 */
void sec_passive_decay(struct char_data *mob);

/* ── Read-only modulation getters ────────────────────────────────────────── */

/**
 * Return a 4D behaviour intensity multiplier ∈ [0.5, 2.0].
 * Anger boosts intensity; fear and helplessness reduce it.
 */
float sec_get_4d_modifier(struct char_data *mob);

/**
 * Return a flee-tendency bias ∈ [0.0, 1.0].
 * High fear + high helplessness → bias approaches 1.0.
 */
float sec_get_flee_bias(struct char_data *mob);

/**
 * Return a lethargy bias ∈ [0.0, 1.0] driven by the sadness partition.
 * High sadness → bias approaches 1.0, suppressing idle and aggressive pulses.
 * This is the "Lethargy Buffer" that models grief, apathy, and post-loss states.
 */
float sec_get_lethargy_bias(struct char_data *mob);

/**
 * Return a target-selection weight multiplier ∈ [0.5, 1.5].
 * High anger → prefer this target; low anger → neutral.
 *
 * @param mob    The acting NPC.
 * @param target Candidate target (reserved for future per-target modulation).
 */
float sec_get_target_bias(struct char_data *mob, struct char_data *target);

/**
 * Return the final Agreeableness value for a mob, incorporating:
 *   Trait_base (genetic) + Trait_builder_modifier/100 + Trait_emotional_modulation
 * Result is clamped to [0.0, 1.0].  |Trait_emotional_modulation| <= 0.10.
 *
 * Modulation formula: A_mod = k3*happiness - k4*anger  (k3=0.10, k4=0.10)
 *
 * @param mob  The NPC.
 */
float sec_get_agreeableness_final(struct char_data *mob);

/**
 * Return the final Extraversion value for a mob, incorporating:
 *   Trait_base (genetic) + Trait_builder_modifier/100 + Trait_emotional_modulation
 * Result is clamped to [0.0, 1.0].  |Trait_emotional_modulation| <= 0.10.
 *
 * Modulation formula: E_mod = k1*happiness - k2*fear  (k1=0.10, k2=0.10)
 *
 * @param mob  The NPC.
 */
float sec_get_extraversion_final(struct char_data *mob);

/**
 * Return the final Conscientiousness value for a mob, incorporating:
 *   Trait_base (genetic) + Trait_builder_modifier/100 + Trait_emotional_modulation
 * Result is clamped to [0.0, 1.0].  |Trait_emotional_modulation| <= 0.05.
 *
 * Modulation formula: C_mod = clamp(-0.05 * SEC_disgust, -0.05, +0.05)
 * Disgust erodes conscientiousness slightly under persistent aversive states.
 *
 * @param mob  The NPC.
 */
float sec_get_conscientiousness_final(struct char_data *mob);

/**
 * Return the final Neuroticism value for a mob, incorporating:
 *   Trait_base (genetic, derived from bravery/EI) + Trait_builder_modifier/100
 *   + Trait_emotional_modulation
 * Result is clamped to [0.0, 1.0].  |Trait_emotional_modulation| <= SEC_N_MOD_CAP (0.05).
 *
 * Modulation formula:
 *   N_mod = clamp(SEC_N_STRESS_SCALE * (sec.fear + sec.anger), -SEC_N_MOD_CAP, +SEC_N_MOD_CAP)
 * The SEC fear+anger signal acts as a bounded, one-way stress signal; because it is
 * capped at ±0.05 it cannot drive a recursive amplification loop.
 *
 * @param mob  The NPC.
 */
float sec_get_neuroticism_final(struct char_data *mob);

/**
 * Return the final Openness value for a mob, incorporating:
 *   Trait_base (Gaussian-generated at spawn) + Trait_builder_modifier/100 + O_mod
 * Result is clamped to [0.0, 1.0].  |O_mod| <= SEC_O_MOD_CAP (0.05).
 *
 * STRUCTURAL CONSTRAINT: O_mod must NEVER be derived from SEC emotional state.
 * Openness is a cognitive trait (flexibility, novelty tolerance), not an emotional one.
 * Deriving O from fear/anger would create an SEC ↔ utility feedback loop.
 * For now O_mod = 0.0; the field is reserved for slow long-term adaptation only.
 *
 * Openness influences:
 *   - MOVE novelty weighting in Shadow Timeline (exploration probability)
 *   - Ambiguity tolerance in utility scoring (future use)
 *
 * Openness must NOT influence:
 *   - Emotional gain or decay
 *   - SEC energy partition
 *   - Dominance axis
 *   - WTA gating
 *
 * @param mob  The NPC.
 */
float sec_get_openness_final(struct char_data *mob);

/**
 * Return the dominant SEC emotion (SEC_DOMINANT_*).
 *
 * Returns SEC_DOMINANT_NONE when the total arousal partition is below
 * SEC_AROUSAL_EPSILON, indicating an emotionally quiescent state.
 * Used by the social-action Winner-Takes-All filter to prevent contradictory
 * behaviours when opposing raw emotions are simultaneously elevated.
 *
 * @param mob  The NPC to query.
 */
int sec_get_dominant_emotion(struct char_data *mob);

#endif /* _SEC_H_ */
