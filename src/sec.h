/**
 * @file sec.h
 * SEC – Sistema de Emoções Concorrentes (Competing Emotions System)
 *
 * A deterministic, 4D-driven internal emotional inference layer for mobs.
 * SEC receives post-hysteresis 4D results and partitions Arousal into
 * competing emotional projections (fear, anger, happiness), maintaining
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
 *   Emotional smooth.  Medium       α≈0.40  Behavioral continuity (F,An,H)
 *   Passive decay      Slow         λ=0.05  Homeostatic convergence to base
 *   Persistent trait   Very slow    δ=0.01  Structural memory (Disgust)
 *
 * Arousal partition (all values from post-hysteresis 4D state, normalised to [0, 1]):
 *   A = r->arousal / 100       — contextually deformed (environment, combat, helplessness)
 *   D = (dominance + 100) / 200
 *   V = (valence  + 100) / 200
 *
 *   w_fear  = A * (1 − D)
 *   w_anger = A * D * (1 − V)
 *   w_happy = A * V * D
 *   → fear_target + anger_target + happiness_target = A  (guaranteed)
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
