/**
 * @file malp.h
 * MALP/MPLP – Memória Ativa/Passiva de Longo Prazo (RFC-1002)
 *
 * Implements Active Long-Term Memory (MALP) and Passive/Implicit Long-Term
 * Memory (MPLP) for NPC believability and emergent social mechanics.
 *
 * Key design points (from RFC-1002):
 *  - Salience-driven consolidation: S = w_a*arousal + w_r*log(1+rehearsal)
 *    + w_s*social_weight, multiplied by power-law age decay.
 *  - Separate explicit (MALP) and implicit (MPLP) stores.
 *  - Trait generation via Hebbian co-occurrence (rehearsal >= threshold).
 *  - Reconsolidation window opened on MALP retrieval above θ_react.
 *  - All emotion deltas go through adjust_emotion() (no pipeline bypass).
 *  - OCEAN modulation: high-N slows negative-trait decay (rumination);
 *    high-C raises effective consolidation threshold; high-A accelerates
 *    positive reconsolidation updates.
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#ifndef _MALP_H_
#define _MALP_H_

#include "structs.h"

/* ── Salience weights (RFC-1002 §7.1 defaults) ───────────────────────────── */
/** Arousal contribution weight in salience formula (default: 0.50) */
#define MALP_W_AROUSAL 0.50f
/** Rehearsal (log) contribution weight in salience formula (default: 0.25) */
#define MALP_W_REHEARSAL 0.25f
/** Social weight contribution in salience formula (default: 0.25) */
#define MALP_W_SOCIAL 0.25f

/** Normalisation denominator: max raw salience ≈ w_a + w_r*log(11) + w_s ≈ 1.35 */
#define MALP_SALIENCE_NORM 1.35f

/** Social weight for PLAYER interactions (higher; players are more salient) */
#define MALP_SOCIAL_WEIGHT_PLAYER 1.0f
/** Social weight for MOB-to-mob interactions */
#define MALP_SOCIAL_WEIGHT_MOB 0.5f

/** Power-law age-decay exponent for salience (lower → slower forgetting) */
#define MALP_AGE_DECAY_EXPONENT 0.40f

/* ── Reconsolidation ─────────────────────────────────────────────────────── */
/** Retrieval probability threshold above which reconsolidation window opens */
#define MALP_THETA_REACT 0.55f
/** Sigmoid steepness k for P_ret computation */
#define MALP_PRET_K 5.0f
/** Reconsolidation λ scaling base (λ = base * salience * rehearsal_factor) */
#define MALP_RECON_LAMBDA_BASE 0.30f
/** Maximum valence change per reconsolidation window (prevents instant flips) */
#define MALP_RECON_MAX_DELTA 0.25f

/* ── Pruning / growth ────────────────────────────────────────────────────── */
/** Initial dynamic-array capacity for new mobs (grows as needed) */
#define MALP_INITIAL_CAPACITY 8
/** Growth factor when MALP array needs to expand */
#define MALP_GROWTH_FACTOR 2
/** Initial dynamic-array capacity for MPLP traits */
#define MPLP_INITIAL_CAPACITY 4
/** Maximum MPLP traits per mob (hard cap) */
#define MPLP_MAX_PER_MOB 50
/** Arousal threshold above which a new MALP entry is forced to HIGH persistence */
#define MALP_HIGH_PERSIST_AROUSAL 0.85f

/* ── Emotion-delta limits applied via adjust_emotion() ───────────────────── */
/** Maximum single-tick emotion boost from a negative MALP retrieval */
#define MALP_EMOTION_DELTA_MAX 8
/** Minimum emotion boost applied from MALP-driven effect (floor) */
#define MALP_EMOTION_DELTA_MIN 2
/** MPLP approach/avoidance emotion modifier cap */
#define MPLP_EMOTION_DELTA_MAX 5

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * Free MALP/MPLP dynamic arrays for a mob.
 * Call from free_char() before freeing ai_data.
 *
 * @param mob  The NPC whose MALP/MPLP stores to free.
 */
void malp_free(struct char_data *mob);

/**
 * Compute salience score S ∈ [0,1] for an episodic memory slot.
 *
 * S = normalize(w_a*arousal + w_r*log(1+rehearsal) + w_s*social_weight)
 *     × pow(1 + age_hours, −MALP_AGE_DECAY_EXPONENT)
 *
 * @param arousal      Normalised arousal 0..1
 * @param rehearsal    Co-occurrence / re-activation count
 * @param social_weight Social-agent salience weight (MALP_SOCIAL_WEIGHT_*)
 * @param age_hours    Age of the episodic slot in fractional hours
 * @return             Salience S ∈ [0,1]
 */
float compute_salience(float arousal, int rehearsal, float social_weight, float age_hours);

/**
 * Consolidation tick: evaluate passive/active episodic slots and consolidate
 * high-salience entries into MALP; generate MPLP traits via Hebbian rule.
 *
 * Call periodically from update_mob_emotion_passive().
 *
 * @param mob  The NPC to consolidate memories for.
 */
void consolidator_tick(struct char_data *mob);

/**
 * Decay MALP intensities and MPLP magnitudes using power-law forgetting.
 * OCEAN N modulates negative-trait MPLP decay (high-N → slower).
 *
 * Automatically called from consolidator_tick().
 *
 * @param mob  The NPC whose long-term memories to decay.
 */
void malp_decay_tick(struct char_data *mob);

/**
 * Return the first MALP entry matching agent_id/agent_type, or NULL.
 *
 * Also opens/extends the reconsolidation window if retrieval probability
 * P_ret >= MALP_THETA_REACT.
 *
 * @param mob        The NPC owner.
 * @param agent_id   Entity ID to look up.
 * @param agent_type ENTITY_TYPE_PLAYER or ENTITY_TYPE_MOB.
 * @return           Pointer into mob->ai_data->malp (do NOT free), or NULL.
 */
struct malp_entry *get_malp_by_agent(struct char_data *mob, long agent_id, int agent_type);

/**
 * Return the approach/avoidance modifier for an agent from MPLP.
 *
 * Positive = approach bias; Negative = avoidance bias.
 * Range: [−1.0, +1.0].
 *
 * @param mob        The NPC owner.
 * @param agent_id   Entity ID.
 * @param agent_type ENTITY_TYPE_PLAYER or ENTITY_TYPE_MOB.
 * @return           Net approach-avoidance modifier.
 */
float get_mplp_approach_modifier(struct char_data *mob, long agent_id, int agent_type);

/**
 * Return the arousal bias from MPLP for an agent.
 *
 * Positive = arousal increase when agent is present; 0 = no bias.
 * Range: [0.0, 1.0].
 *
 * @param mob        The NPC owner.
 * @param agent_id   Entity ID.
 * @param agent_type ENTITY_TYPE_PLAYER or ENTITY_TYPE_MOB.
 * @return           Arousal bias modifier.
 */
float get_mplp_arousal_bias(struct char_data *mob, long agent_id, int agent_type);

/**
 * Apply MALP/MPLP-derived emotion effects through the appraisal pipeline.
 *
 * Called when 'actor' enters the same room or interacts with 'mob'.
 * All emotion deltas go through adjust_emotion() (no pipeline bypass).
 *
 * @param mob    The NPC experiencing the memory effect.
 * @param actor  The character whose presence triggers the memory.
 */
void apply_malp_emotion_effects(struct char_data *mob, struct char_data *actor);

/**
 * Perform safe reconsolidation: update a MALP entry's valence/intensity
 * within the reconsolidation window, bounded to prevent instant inversions.
 *
 * @param mob            The NPC owner.
 * @param agent_id       Agent the MALP entry is about.
 * @param agent_type     ENTITY_TYPE_PLAYER or ENTITY_TYPE_MOB.
 * @param delta_valence  Signed valence change (clamped internally).
 * @param new_salience   New salience score driving the λ update weight.
 */
void retrieve_and_reconsolidate(struct char_data *mob, long agent_id, int agent_type, float delta_valence,
                                float new_salience);

#endif /* _MALP_H_ */
