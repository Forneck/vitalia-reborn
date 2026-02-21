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
 * Arousal partition (all values normalised to [0, 1]):
 *   A = arousal / 100
 *   D = (dominance + 100) / 200
 *   V = (valence  + 100) / 200
 *
 *   w_fear  = A * (1 − D)
 *   w_anger = A * D * (1 − V)
 *   w_happy = A * V * D
 *   → fear + anger + happiness = A  (guaranteed)
 *
 * Helplessness:
 *   target = 1 − D
 *   h_new  = h_old * (1 − α) + target * α   (exponential smoothing)
 *
 * Passive decay (when A < ε):
 *   emotion = emotion * (1 − λ) + baseline * λ
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#ifndef _SEC_H_
#define _SEC_H_

#include "structs.h"

/* sec_state and sec_baseline are defined in structs.h (inside mob_ai_data) */

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

#endif /* _SEC_H_ */
