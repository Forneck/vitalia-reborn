/**
 * @file emotion_projection.h
 * 4D Relational Decision Space - Emotional Profile Projection Layer
 *
 * Implements the structured projection of mob emotional vectors into a
 * 4-dimensional decision space (Valence, Arousal, Dominance, Affiliation).
 *
 * Core formula:
 *   P_raw      = M_profile * E
 *   P_effective = (M_profile + ΔM_personal) * E
 *
 * Where:
 *   M_profile    = Emotional Profile projection matrix (fixed, per profile type)
 *   ΔM_personal  = Personal drift matrix (bounded ±PERSONAL_DRIFT_MAX_PCT%)
 *   E            = Current emotional vector (20 components, each 0–100)
 *   P_raw        = Raw 4D projection before contextual modulation
 *   P_effective  = Final 4D state after contextual modulation
 *
 * Axis semantics:
 *   Valence     – positive (+100) vs. negative (−100) evaluation of interaction
 *   Arousal     – calm (0) to highly activated (100)
 *   Dominance   – perceived control / assertiveness bias (−100 to +100)
 *   Affiliation – relational orientation toward target (−100 avoidance, +100 approach)
 *
 * Contextual Modulation Layer (applied after raw projection):
 *   Dominance   adjusted by Coping Potential (actual HP, mana, status, numbers)
 *   Arousal     scaled by environmental intensity (combat, crowd, threat proximity)
 *   Affiliation weighted by relationship memory with current target
 *   Valence     optionally biased by Shadow Timeline forecast
 *
 * Note: Dominance = perceived control (subjective).
 *       Coping Potential = actual situational capacity (objective modulator).
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#ifndef _EMOTION_PROJECTION_H_
#define _EMOTION_PROJECTION_H_

#include "structs.h"

/**
 * Retrieve the immutable profile projection matrix for the given profile type.
 *
 * @param profile  EMOTION_PROFILE_* constant (0–EMOTION_PROFILE_NUM-1)
 * @return         Pointer to float[DECISION_SPACE_DIMS][20] matrix,
 *                 or the NEUTRAL matrix if profile is out of range.
 */
const float (*emotion_get_profile_matrix(int profile))[20];

/**
 * Compute the raw 4D projection vector P_raw = (M_profile + ΔM_personal) * E.
 *
 * Each axis value is normalized to [−100, +100] by dividing the weighted sum
 * by the L1-norm of the effective row and scaling to 100.
 *
 * @param mob     The mob whose emotions and drift are used.
 * @param raw_out Output: raw[DECISION_SPACE_DIMS] – values before modulation.
 */
void emotion_compute_raw_projection(struct char_data *mob, float raw_out[DECISION_SPACE_DIMS]);

/**
 * Compute the objective coping potential of a mob.
 *
 * Coping Potential is the actual situational capacity that modulates the
 * perceived Dominance axis.  It reflects HP ratio, mana ratio (mobs that
 * use mana), status effects, and numerical group advantage.
 *
 * Scale: 0 (completely overwhelmed) to 100 (full capacity).
 *
 * @param mob  The mob to evaluate.
 * @return     Coping potential in [0, 100].
 */
float emotion_compute_coping_potential(struct char_data *mob);

/**
 * Apply the Contextual Modulation Layer to a raw 4D vector.
 *
 * Adjustments made:
 *   Dominance   = clamp(raw_D + (coping_potential − 50) * 0.4)
 *   Arousal     = clamp(raw_A * (1 + env_intensity * 0.5))
 *   Affiliation = clamp(raw_Af + relationship_bias)
 *   Valence     = clamp(raw_V + shadow_forecast_bias) (if shadow data available)
 *
 * @param mob             The mob.
 * @param target          Target of the interaction (may be NULL).
 * @param raw             Raw 4D values (input).
 * @param coping_pot      Coping potential from emotion_compute_coping_potential().
 * @param effective_out   Output: modulated 4D values.
 */
void emotion_apply_contextual_modulation(struct char_data *mob, struct char_data *target,
                                         const float raw[DECISION_SPACE_DIMS], float coping_pot,
                                         float effective_out[DECISION_SPACE_DIMS]);

#endif /* _EMOTION_PROJECTION_H_ */
