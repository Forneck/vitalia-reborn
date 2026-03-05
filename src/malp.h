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

/* ── Dominant-actor feedback loop protection ────────────────────────────── */
/**
 * Ratio of one actor's rehearsal to total MALP rehearsal above which that actor
 * is considered cognitively dominant.  When dominance > this threshold the
 * actor's MALP emotion-effect intensity is multiplied by MALP_DOMINANCE_DAMPENING,
 * preventing a single actor from monopolising the NPC's cognitive landscape.
 */
#define MALP_DOMINANCE_THRESHOLD 0.75f

/**
 * Intensity multiplier applied to MALP emotion effects when the triggering actor
 * is cognitively dominant (rehearsal share > MALP_DOMINANCE_THRESHOLD).
 * Reduces the per-call delta by 30 % while keeping the memory influence non-zero.
 */
#define MALP_DOMINANCE_DAMPENING 0.70f

/**
 * Minimum elapsed seconds between consecutive MALP/MPLP emotion-effect
 * applications for the same actor.
 *
 * Prevents rapid re-triggering of emotional feedback loops when an actor is
 * continuously present in the room.  During the cooldown window the NPC's
 * natural emotion-homeostasis system (update_mob_emotion_passive) returns
 * emotions toward their baselines without external amplification.
 *
 * 120 s ≈ 2 real minutes; sufficient for baseline recovery before the next
 * MALP-driven arousal spike can occur.
 */
#define MALP_SOCIAL_COOLDOWN_SECS 120

/**
 * Regulation-timer value (in update ticks) set after a successful self-regulation
 * behaviour (justify / deflect / apologize / reframe / nervous-laugh).
 *
 * Replaces the old formula (8 − 5 × reg_strength = 1..8 ticks) which was too
 * short to interrupt rapid shame/fear → reflection → shame/fear loops.
 *
 * At CONFIG_MOB_EMOTION_UPDATE_CHANCE = 30 % and PULSE_MOB_EMOTION = 4 s,
 * 60 ticks ≈ 13 minutes of real time between successive self-reflection episodes.
 */
#define MALP_REGULATION_COOLDOWN 60

/* ── Rehearsal saturation, decay & dampening ─────────────────────────────── */
/**
 * Hard cap on raw rehearsal count (MALP entries and MPLP rehearsal_count).
 * Prevents unbounded linear growth (salience lock / overflow).
 * At this value effective salience is already near-maximal via log(1+rehearsal).
 */
#define MALP_MAX_REHEARSAL 10000

/**
 * Divisor used to compute per-tick passive rehearsal decay rate for both
 * MALP entries (rehearsal field) and MPLP traits (rehearsal_count field):
 *   decay_amount = 1 + (rehearsal / MALP_REHEARSAL_DECAY_DIVISOR)
 * Strong memories (large rehearsal) decay faster in absolute terms, but
 * the log-based salience ensures they remain relevant longer.
 * At rehearsal = 10000: decay = 11/tick; at rehearsal = 100: decay = 1/tick.
 */
#define MALP_REHEARSAL_DECAY_DIVISOR 1000

/**
 * Multiplicative dampening applied to rehearsal during reconsolidation.
 * Every time a memory is reconsolidated (rewritten), its raw rehearsal count
 * is multiplied by this factor, modelling the interference/rewriting cost.
 * 0.95 → 5 % reduction per reconsolidation event.
 */
#define MALP_RECON_DAMPENING_FACTOR 0.95f

/* ── Peak-End Rule weights for episodic valence consolidation ────────────── */
/**
 * Weight applied to the peak emotional moment in episodic valence (Kahneman
 * Peak-End Rule).  Peak is the slot with the highest arousal across all
 * episodic slots for this agent.
 * Defined so that MALP_PEAK_END_PEAK_WEIGHT + MALP_PEAK_END_END_WEIGHT == 1.0f.
 */
#define MALP_PEAK_END_PEAK_WEIGHT 0.60f
/**
 * Weight applied to the final (most recent) event in episodic valence
 * (Kahneman Peak-End Rule).  End is the slot with the latest timestamp.
 * Derived from MALP_PEAK_END_PEAK_WEIGHT so the two weights always sum to 1.0f.
 */
#define MALP_PEAK_END_END_WEIGHT (1.0f - MALP_PEAK_END_PEAK_WEIGHT)

/* ── Social episode type classification thresholds ───────────────────────── */
/**
 * Consolidated valence above this threshold → INTERACT_SOCIAL_POSITIVE.
 * Applied after Peak-End computation to reclassify social episodes so that the
 * stored interaction_type reflects the overall interaction outcome rather than
 * the type of the first recorded event.
 */
#define MALP_SOCIAL_VALENCE_POS_THRESHOLD 0.20f
/**
 * Consolidated valence below this threshold → INTERACT_SOCIAL_NEGATIVE.
 * Values between NEG_THRESHOLD and POS_THRESHOLD → INTERACT_SOCIAL_NEUTRAL.
 */
#define MALP_SOCIAL_VALENCE_NEG_THRESHOLD -0.20f

/* ── OCEAN modulation constants ──────────────────────────────────────────── */
/**
 * Conscientiousness threshold boost: high-C mobs require higher salience
 * before consolidating memories (fewer impulsive memories).
 * θ_eff = θ_cons + MALP_C_THETA_BOOST × C_final   (C_final ∈ [0,1])
 * At C=1: threshold rises by +0.10, preventing overly quick long-term judgements.
 */
#define MALP_C_THETA_BOOST 0.10f

/**
 * Neuroticism rumination scale: negative MPLP half-life is multiplied by
 * (1 + MALP_N_RUMINATION_SCALE × N_final).
 * At N=1: half-life doubles; traumatic avoidance traits persist much longer.
 */
#define MALP_N_RUMINATION_SCALE 1.0f

/* ── Emotion-delta limits applied via adjust_emotion() ───────────────────── */
/** Maximum single-tick emotion boost from a negative MALP retrieval */
#define MALP_EMOTION_DELTA_MAX 8
/** Minimum emotion boost applied from MALP-driven effect (floor) */
#define MALP_EMOTION_DELTA_MIN 2
/** MPLP approach/avoidance emotion modifier cap */
#define MPLP_EMOTION_DELTA_MAX 5

/* ── Context-trait (EXHIBITION_RESPONSE / MODESTY_RESPONSE) tunables ─────── */
/** Learning rate for the running-average valence in context-global MPLP traits. */
#define MPLP_VALENCE_LEARNING_RATE 0.20f
/** Trait magnitude threshold above which a personality bias is applied to emotions. */
#define MPLP_PERSONALITY_BIAS_THRESHOLD 0.15f
/** Trust floor for an exhibition social to be welcomed (→ positive reinforcement). */
#define MPLP_EXHIBITION_TRUST_THRESHOLD 50
/** Love floor for an exhibition social to be welcomed. */
#define MPLP_EXHIBITION_LOVE_THRESHOLD 40
/** Disgust/shame level above which a display social is considered unwelcome. */
#define MPLP_EXHIBITION_DISGUST_THRESHOLD 50
/** Trust floor used for explicit-contact consent check (modesty reinforcement). */
#define MPLP_MODESTY_CONSENT_TRUST 70
/** Love floor used for explicit-contact consent check (modesty reinforcement). */
#define MPLP_MODESTY_CONSENT_LOVE 60
/** Disgust/shame threshold above which non-blocked explicit contact still feels unwelcome. */
#define MPLP_MODESTY_DISGUST_THRESHOLD 40

/* ── Gender-expression trait tunables ───────────────────────────────────── */
/** Anger/pride threshold above which a gender-norm violation is considered a provocation. */
#define MPLP_GENDER_NORM_ANGER_THRESHOLD 45
/** Trust threshold below which gender-expression socials raise suspicion/confusion. */
#define MPLP_GENDER_NORM_TRUST_THRESHOLD 35
/** Curiosity threshold above which androgynous expression triggers a positive response. */
#define MPLP_ANDROGYNY_CURIOSITY_THRESHOLD 50
/** Scaling multiplier for GENDER_NORM_SENSITIVITY amplification of gender-expression deltas. */
#define MPLP_GENDER_NORM_AMPLIFY_MULTIPLIER 2.0f

/* ── Cue-score weights for P_ret computation in get_malp_by_agent() ─────── */
/** Weight of memory intensity in cue score (primary strength factor) */
#define MALP_CUE_WEIGHT_INTENSITY 0.60f
/** Weight of arousal state-match in cue score (state-dependent recall; Bower 1981) */
#define MALP_CUE_WEIGHT_AROUSAL 0.25f
/** Weight of retrieval recency in cue score (within-hour accessibility bonus) */
#define MALP_CUE_WEIGHT_RECENCY 0.15f

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
 * Retrieve the NPC's context-global exhibition-response trait.
 *
 * Measures the accumulated tendency to react positively (+) or negatively (−)
 * to display, confidence, and attention-seeking body-language socials
 * (catwalk, sexy, twerk, shakeass, dancesensual, femininity, masculinity, etc.).
 *
 * @param mob  The NPC to query.
 * @return     Signed float in [−1.0, +1.0]:
 *               > 0 → flirtatious / enjoys display behaviour
 *               < 0 → conservative / dislikes attention-seeking behaviour
 */
float get_mplp_exhibition_response(struct char_data *mob);

/**
 * Retrieve the NPC's context-global modesty-response trait.
 *
 * Measures the accumulated tendency to prefer reserved behaviour (+) or to be
 * tolerant of explicit physical contact (−).  Reinforced by grope, fondle, rub,
 * massage, french, sex, and similar explicit-contact socials.
 *
 * @param mob  The NPC to query.
 * @return     Signed float in [−1.0, +1.0]:
 *               > 0 → prudish / offended by explicit socials
 *               < 0 → tolerant of explicit interactions
 */
float get_mplp_modesty_response(struct char_data *mob);

/**
 * Retrieve the NPC's context-global masculinity-response trait.
 *
 * Measures the accumulated tendency to react positively (+) or negatively (−)
 * to masculine-coded socials (masculinity, flex, taunt, strut, imposing postures).
 *
 * @param mob  The NPC to query.
 * @return     Signed float in [−1.0, +1.0]:
 *               > 0 → admires / responds positively to masculine expression
 *               < 0 → indifferent or dislikes masculine display
 */
float get_mplp_masculinity_response(struct char_data *mob);

/**
 * Retrieve the NPC's context-global femininity-response trait.
 *
 * Measures the accumulated tendency to react positively (+) or negatively (−)
 * to feminine-coded socials (femininity, curtsy, catwalk, flirt, wink, blow).
 *
 * @param mob  The NPC to query.
 * @return     Signed float in [−1.0, +1.0]:
 *               > 0 → admires / responds positively to feminine expression
 *               < 0 → indifferent or dislikes feminine display
 */
float get_mplp_femininity_response(struct char_data *mob);

/**
 * Retrieve the NPC's context-global androgyny-tolerance trait.
 *
 * Measures tolerance (+) or intolerance (−) for gender-mixed or androgynous
 * expression (e.g., a masculine NPC performing feminine gestures, or vice-versa).
 * High androgyny_tolerance leads to curiosity/acceptance; low leads to confusion.
 *
 * @param mob  The NPC to query.
 * @return     Signed float in [−1.0, +1.0]:
 *               > 0 → accepts / curious about androgynous expression
 *               < 0 → confused / mildly uncomfortable with mixed expression
 */
float get_mplp_androgyny_tolerance(struct char_data *mob);

/**
 * Retrieve the NPC's context-global gender-norm sensitivity trait.
 *
 * Measures how strongly the NPC reacts when gender-norm expectations are violated
 * (e.g., a flirtatious/seductive social performed by the unexpected gender).
 * High sensitivity produces stronger emotional reactions (positive or negative).
 *
 * @param mob  The NPC to query.
 * @return     Unsigned float in [0.0, 1.0]:
 *               0.0 → unaffected by gender-norm context
 *               1.0 → very strongly amplified reactions to norm violations
 */
float get_mplp_gender_norm_sensitivity(struct char_data *mob);

/**
 * Reinforce a context-global MPLP trait (anchor = MPLP_GLOBAL_ANCHOR).
 *
 * Creates the trait slot if it does not exist; otherwise applies a Hebbian
 * magnitude increment (0.15 × salience, capped at +0.30 per call) and
 * updates the running-average valence.  The encoded base_magnitude is
 * updated so power-law decay restarts from this point.
 *
 * @param mob        The NPC whose MPLP is being updated.
 * @param trait_type One of: MPLP_TRAIT_EXHIBITION_RESPONSE, MPLP_TRAIT_MODESTY_RESPONSE,
 *                   MPLP_TRAIT_MASCULINITY_RESPONSE, MPLP_TRAIT_FEMININITY_RESPONSE,
 *                   MPLP_TRAIT_ANDROGYNY_TOLERANCE, or MPLP_TRAIT_GENDER_NORM_SENSITIVITY.
 * @param valence    Signed valence of the current experience (−1..+1).
 * @param salience   Salience weight of the event (0..1); scales the magnitude delta.
 */
void reinforce_mplp_context_trait(struct char_data *mob, int trait_type, float valence, float salience);

/**
 * Apply MALP/MPLP-derived emotion effects through the appraisal pipeline.
 *
 * Called when 'actor' interacts with 'mob'.  All emotion deltas go through
 * adjust_emotion() (no pipeline bypass).
 *
 * If 'interaction_valence' is non-zero AND the relevant MALP entry has an open
 * reconsolidation window, retrieve_and_reconsolidate() is called automatically
 * to apply a bounded valence update — wiring all reconsolidation semantics into
 * a single call site.  Pass 0.0f to suppress reconsolidation for neutral events.
 *
 * @param mob                 The NPC experiencing the memory effect.
 * @param actor               The character whose presence triggers the memory.
 * @param interaction_valence Signed valence of the current interaction (−1..+1).
 *                            Negative for aversive events, positive for beneficial,
 *                            0.0f for neutral/no reconsolidation update.
 */
void apply_malp_emotion_effects(struct char_data *mob, struct char_data *actor, float interaction_valence);

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
