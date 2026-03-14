/**
 * @file malp.c
 * MALP/MPLP – Memória Ativa/Passiva de Longo Prazo (RFC-1002)
 *
 * Implements Active Long-Term Memory (MALP) and Passive/Implicit Long-Term
 * Memory (MPLP) for NPC believability and emergent social mechanics.
 *
 * Psychological model:
 *  - Arousal-weighted consolidation (amygdala modulation; McGaugh 2004).
 *  - Power-law forgetting (Ebbinghaus; Anderson & Schooler 1991).
 *  - Reconsolidation via retrieval window (Nader et al.; Schwabe 2012).
 *  - Hebbian-esque implicit trait formation (MPLP).
 *  - Peak-End episodic valence weighting (Kahneman 1993): episode valence =
 *    peak_valence × MALP_PEAK_END_PEAK_WEIGHT +
 *    end_valence × MALP_PEAK_END_END_WEIGHT.
 *  - OCEAN modulation: N ↑ → slower negative-trait decay; C ↑ → higher
 *    effective θ_cons; A ↑ → faster positive reconsolidation.
 *
 * Integration invariant (RFC-1002 §8):
 *  All MALP/MPLP-derived emotion changes go through adjust_emotion()
 *  following the full SEC pipeline (EI → N gain → rate-limit → soft clamp).
 *  No direct writes to mob->ai_data->emotion_* from this module.
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#include "conf.h"
#include "sysdep.h"
#include <math.h>
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "sec.h"
#include "malp.h"
#include "comm.h"
#include "dg_scripts.h"

/* ── Internal helpers ────────────────────────────────────────────────────── */

/**
 * Return mob's current peak arousal as a normalised float in [0, 1].
 *
 * Peak arousal is the maximum of fear, anger, and excitement, divided by 100.
 * Used for state-dependent memory retrieval (Bower 1981) and for computing
 * new_salience during reconsolidation.
 */
static float get_mob_peak_arousal(const struct char_data *mob)
{
    int peak = mob->ai_data->emotion_fear;
    if (mob->ai_data->emotion_anger > peak)
        peak = mob->ai_data->emotion_anger;
    if (mob->ai_data->emotion_excitement > peak)
        peak = mob->ai_data->emotion_excitement;
    return (float)peak / 100.0f;
}

/**
 * Ensure mob->ai_data->malp has room for at least one more entry.
 *
 * Capacity is tracked implicitly: the smallest power-of-two multiple of
 * MALP_INITIAL_CAPACITY that is >= malp_count is the current capacity.
 * When malp_count reaches that cap (i.e., malp_count >= cap), all slots
 * are occupied and we realloc to cap * MALP_GROWTH_FACTOR before the
 * caller appends.  This is safe: the condition is TRUE only when there
 * is no room for one more element.
 *
 * Returns TRUE on success, FALSE on allocation failure.
 */
static bool malp_grow(struct char_data *mob)
{
    struct mob_ai_data *ai = mob->ai_data;
    if (ai->malp == NULL) {
        ai->malp = calloc(MALP_INITIAL_CAPACITY, sizeof(struct malp_entry));
        if (!ai->malp)
            return FALSE;
        ai->malp_count = 0;
        return TRUE;
    }
    /* Find current implicit capacity (smallest power-of-2 × INITIAL >= count). */
    int cap = MALP_INITIAL_CAPACITY;
    while (cap < ai->malp_count)
        cap *= MALP_GROWTH_FACTOR;
    /* If all slots are occupied, expand the array. */
    if (ai->malp_count >= cap) {
        int newcap = cap * MALP_GROWTH_FACTOR;
        struct malp_entry *tmp = realloc(ai->malp, newcap * sizeof(struct malp_entry));
        if (!tmp)
            return FALSE;
        ai->malp = tmp;
    }
    return TRUE;
}

/**
 * Ensure mob->ai_data->mplp has room for at least one more entry.
 * Uses the same implicit-capacity heuristic as malp_grow().
 */
static bool mplp_grow(struct char_data *mob)
{
    struct mob_ai_data *ai = mob->ai_data;
    if (ai->mplp == NULL) {
        ai->mplp = calloc(MPLP_INITIAL_CAPACITY, sizeof(struct mplp_trait));
        if (!ai->mplp)
            return FALSE;
        ai->mplp_count = 0;
        return TRUE;
    }
    /* Find current implicit capacity. */
    int cap = MPLP_INITIAL_CAPACITY;
    while (cap < ai->mplp_count)
        cap *= MALP_GROWTH_FACTOR;
    /* If all slots are occupied, expand the array. */
    if (ai->mplp_count >= cap) {
        int newcap = cap * MALP_GROWTH_FACTOR;
        struct mplp_trait *tmp = realloc(ai->mplp, newcap * sizeof(struct mplp_trait));
        if (!tmp)
            return FALSE;
        ai->mplp = tmp;
    }
    return TRUE;
}

/**
 * Prune MALP array to stay within CONFIG_MALP_LIMIT_PER_MOB.
 * Removes the entry with the lowest current intensity.
 */
static void malp_prune(struct char_data *mob)
{
    struct mob_ai_data *ai = mob->ai_data;
    int limit = CONFIG_MALP_LIMIT_PER_MOB;
    if (limit < 1)
        limit = 200;

    while (ai->malp_count > limit) {
        /* Find minimum-intensity entry */
        int min_idx = 0;
        int i;
        for (i = 1; i < ai->malp_count; i++) {
            if (ai->malp[i].intensity < ai->malp[min_idx].intensity)
                min_idx = i;
        }
        /* Remove by shifting */
        if (min_idx < ai->malp_count - 1) {
            memmove(&ai->malp[min_idx], &ai->malp[min_idx + 1],
                    (ai->malp_count - min_idx - 1) * sizeof(struct malp_entry));
        }
        ai->malp_count--;
    }
}

/**
 * Compute the episodic-slot arousal (normalised 0..1) from a memory snapshot.
 * Uses the maximum of the high-arousal emotions.
 */
static float slot_arousal(const struct emotion_memory *mem)
{
    int peak = mem->fear_level;
    if (mem->anger_level > peak)
        peak = mem->anger_level;
    if (mem->horror_level > peak)
        peak = mem->horror_level;
    if (mem->pain_level > peak)
        peak = mem->pain_level;
    if (mem->excitement_level > peak)
        peak = mem->excitement_level;
    return (float)peak / 100.0f;
}

/**
 * Compute episodic-slot valence (−1..+1) from emotion snapshot.
 * Positive emotions push valence up; negative push it down.
 */
static float slot_valence(const struct emotion_memory *mem)
{
    int pos = mem->happiness_level + mem->friendship_level + mem->love_level + mem->trust_level + mem->courage_level +
              mem->pride_level + mem->excitement_level;
    int neg = mem->fear_level + mem->anger_level + mem->sadness_level + mem->disgust_level + mem->shame_level +
              mem->pain_level + mem->horror_level + mem->humiliation_level + mem->envy_level;
    /* pos: 7 emotions (0..700), neg: 9 emotions (0..900).
     * (pos - neg) range is −900..+700; normalise by the larger side (900)
     * to keep v ∈ [−1, +1] without biasing toward negative. */
    float v = (float)(pos - neg) / 900.0f;
    if (v > 1.0f)
        v = 1.0f;
    if (v < -1.0f)
        v = -1.0f;
    return v;
}

/**
 * Reclassify a social episode's interaction_type based on consolidated valence.
 *
 * After Peak-End consolidation the stored interaction_type should reflect the
 * overall outcome of the episode rather than the type of the first recorded
 * event.  Non-social types (ATTACKED, HEALED, etc.) are returned unchanged so
 * that objective event categories are never overwritten.
 *
 * Only INTERACT_SOCIAL_POSITIVE, INTERACT_SOCIAL_NEGATIVE, and
 * INTERACT_SOCIAL_NEUTRAL are reclassified; INTERACT_SOCIAL_VIOLENT is
 * intentionally excluded because physical violence is an objective fact that
 * should not be softened by a positive valence outcome.
 *
 * Classification rule (from RFC-1002 §9.3):
 *   valence >  MALP_SOCIAL_VALENCE_POS_THRESHOLD → INTERACT_SOCIAL_POSITIVE
 *   valence <  MALP_SOCIAL_VALENCE_NEG_THRESHOLD → INTERACT_SOCIAL_NEGATIVE
 *   otherwise                                    → INTERACT_SOCIAL_NEUTRAL
 */
static int classify_social_itype(int itype, float valence)
{
    switch (itype) {
        case INTERACT_SOCIAL_POSITIVE:
        case INTERACT_SOCIAL_NEGATIVE:
        case INTERACT_SOCIAL_NEUTRAL:
            if (valence > MALP_SOCIAL_VALENCE_POS_THRESHOLD)
                return INTERACT_SOCIAL_POSITIVE;
            if (valence < MALP_SOCIAL_VALENCE_NEG_THRESHOLD)
                return INTERACT_SOCIAL_NEGATIVE;
            return INTERACT_SOCIAL_NEUTRAL;
        default:
            return itype;
    }
}

/**
 * Compute Peak-End episodic valence for an agent across all episodic buffers.
 *
 * Implements the Kahneman (1993) Peak-End Rule: experienced episodes are
 * remembered primarily by their most intense (peak) moment and their final
 * (end) moment rather than by a duration-weighted average.
 *
 *   episode_valence = peak_valence × MALP_PEAK_END_PEAK_WEIGHT
 *                   + end_valence  × MALP_PEAK_END_END_WEIGHT
 *
 * "Peak" is the slot with the highest arousal (|emotional intensity|).
 * "End"  is the slot with the most recent timestamp.
 *
 * Falls back to slot_valence(fallback) when fewer than 2 slots are found for
 * the agent (no meaningful peak/end distinction possible).
 */
static float compute_peak_end_valence(struct char_data *mob, long agent_id, int agent_type,
                                      const struct emotion_memory *fallback)
{
    struct mob_ai_data *ai = mob->ai_data;
    float peak_valence = 0.0f;
    float end_valence = 0.0f;
    float peak_arousal = -1.0f;
    time_t end_time = 0;
    int found = 0;
    int i;

    for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        const struct emotion_memory *m = &ai->memories[i];
        if (m->timestamp == 0 || m->entity_id != agent_id || m->entity_type != agent_type)
            continue;
        float a = slot_arousal(m);
        float v = slot_valence(m);
        if (a > peak_arousal) {
            peak_arousal = a;
            peak_valence = v;
        }
        if (m->timestamp > end_time) {
            end_time = m->timestamp;
            end_valence = v;
        }
        found++;
    }
    for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        const struct emotion_memory *m = &ai->active_memories[i];
        if (m->timestamp == 0 || m->entity_id != agent_id || m->entity_type != agent_type)
            continue;
        float a = slot_arousal(m);
        float v = slot_valence(m);
        if (a > peak_arousal) {
            peak_arousal = a;
            peak_valence = v;
        }
        if (m->timestamp > end_time) {
            end_time = m->timestamp;
            end_valence = v;
        }
        found++;
    }

    /* Need at least 2 slots to distinguish peak from end; with a single slot
     * the two are identical and no weighting gain is possible. */
    if (found < 2)
        return slot_valence(fallback);
    return peak_valence * MALP_PEAK_END_PEAK_WEIGHT + end_valence * MALP_PEAK_END_END_WEIGHT;
}

/**
 * Count how many episodic slots (passive + active) belong to the given agent.
 */
static int count_rehearsal(struct char_data *mob, long agent_id, int agent_type)
{
    struct mob_ai_data *ai = mob->ai_data;
    int count = 0;
    int i;
    for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        if (ai->memories[i].timestamp != 0 && ai->memories[i].entity_id == agent_id &&
            ai->memories[i].entity_type == agent_type)
            count++;
    }
    for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        if (ai->active_memories[i].timestamp != 0 && ai->active_memories[i].entity_id == agent_id &&
            ai->active_memories[i].entity_type == agent_type)
            count++;
    }
    /* Also count existing MALP rehearsal for this agent */
    if (ai->malp) {
        for (i = 0; i < ai->malp_count; i++) {
            if (ai->malp[i].agent_id == agent_id && ai->malp[i].agent_type == agent_type)
                count += ai->malp[i].rehearsal;
        }
    }
    return count;
}

/**
 * Compute power-law decay intensity for an entry created at 'timestamp'.
 *
 * Derivation: Ebbinghaus-style power-law forgetting I(t) = (1 + t)^(−c).
 * At t = half_life_hours, I = 0.5:
 *   0.5 = (1 + half_life)^(−c)
 *   c   = log(2) / log(1 + half_life)        [= M_LN2 / log(1+half_life)]
 *   I(t) = (1 + t)^(−c)
 *
 * This gives slower forgetting than simple exponential (log(intensity) is
 * linear in log(1+t)), matching empirical human forgetting curves better.
 *
 * @param timestamp       Creation/update time of the memory entry.
 * @param half_life_hours Age at which intensity reaches 0.5.
 * @return                Intensity ∈ [0, 1].
 */
static float powerlaw_intensity(time_t timestamp, int half_life_hours)
{
    if (timestamp == 0 || half_life_hours <= 0)
        return 0.0f;
    time_t now = time(NULL);
    float age_hours = (float)(now - timestamp) / 3600.0f;
    if (age_hours < 0.0f)
        age_hours = 0.0f;
    float exponent = logf(2.0f) / logf(1.0f + (float)half_life_hours);
    float intensity = powf(1.0f + age_hours, -exponent);
    if (intensity < 0.0f)
        intensity = 0.0f;
    if (intensity > 1.0f)
        intensity = 1.0f;
    return intensity;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void malp_free(struct char_data *mob)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data)
        return;
    if (mob->ai_data->malp) {
        free(mob->ai_data->malp);
        mob->ai_data->malp = NULL;
        mob->ai_data->malp_count = 0;
    }
    if (mob->ai_data->mplp) {
        free(mob->ai_data->mplp);
        mob->ai_data->mplp = NULL;
        mob->ai_data->mplp_count = 0;
    }
}

float compute_salience(float arousal, int rehearsal, float social_weight, float age_hours)
{
    /* Clamp inputs */
    if (arousal < 0.0f)
        arousal = 0.0f;
    if (arousal > 1.0f)
        arousal = 1.0f;
    if (social_weight < 0.0f)
        social_weight = 0.0f;
    if (social_weight > 1.0f)
        social_weight = 1.0f;
    if (rehearsal < 0)
        rehearsal = 0;
    if (age_hours < 0.0f)
        age_hours = 0.0f;

    float raw =
        MALP_W_AROUSAL * arousal + MALP_W_REHEARSAL * logf(1.0f + (float)rehearsal) + MALP_W_SOCIAL * social_weight;

    float normalized = raw / MALP_SALIENCE_NORM;
    if (normalized > 1.0f)
        normalized = 1.0f;

    /* Power-law age decay */
    float age_factor = powf(1.0f + age_hours, -MALP_AGE_DECAY_EXPONENT);

    float S = normalized * age_factor;
    if (S < 0.0f)
        S = 0.0f;
    if (S > 1.0f)
        S = 1.0f;
    return S;
}

void malp_decay_tick(struct char_data *mob)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data)
        return;

    struct mob_ai_data *ai = mob->ai_data;
    float N_final = sec_get_neuroticism_final(mob);
    int i;

    /* Decay MALP entries */
    if (ai->malp) {
        int half_std = CONFIG_MALP_DECAY_HALFLIFE_STD;
        int half_major = CONFIG_MALP_DECAY_HALFLIFE_MAJOR;
        if (half_std < 1)
            half_std = 24;
        if (half_major < 1)
            half_major = 72;

        int alive = 0;
        for (i = 0; i < ai->malp_count; i++) {
            struct malp_entry *e = &ai->malp[i];
            if (e->agent_id == 0)
                continue;
            int hl = (e->major_event || e->persistence == MALP_PERSIST_HIGH) ? half_major : half_std;
            /* Decay from encoded salience (strength at consolidation), not from 1.0.
             * powerlaw_intensity() computes the shape-factor (0..1); scaling by
             * salience preserves relative strength between stronger and weaker memories. */
            e->intensity = e->salience * powerlaw_intensity(e->timestamp, hl);
            /* Tick down reconsolidation window */
            if (e->recon_ticks_left > 0)
                e->recon_ticks_left--;
            /* Passive rehearsal decay: rate scales with current rehearsal so that
             * strong memories erode faster in absolute terms but log-salience keeps
             * them cognitively relevant; weak memories fade to zero quickly. */
            if (e->rehearsal > 0) {
                int rdecay = 1 + (e->rehearsal / MALP_REHEARSAL_DECAY_DIVISOR);
                e->rehearsal -= rdecay;
                if (e->rehearsal < 0)
                    e->rehearsal = 0;
            }
            if (e->intensity >= 0.05f)
                alive++;
        }

        /* Compact array: remove dead entries (intensity < 0.05) */
        int w = 0;
        for (i = 0; i < ai->malp_count; i++) {
            if (ai->malp[i].agent_id != 0 && ai->malp[i].intensity >= 0.05f) {
                if (w != i)
                    ai->malp[w] = ai->malp[i];
                w++;
            }
        }
        ai->malp_count = w;
        (void)alive; /* used for logging if needed */
    }

    /* Decay MPLP traits — high-N slows negative-trait decay (rumination) */
    if (ai->mplp) {
        int half_mplp = CONFIG_MPLP_DECAY_HALFLIFE;
        if (half_mplp < 1)
            half_mplp = 168;

        int w = 0;
        for (i = 0; i < ai->mplp_count; i++) {
            struct mplp_trait *t = &ai->mplp[i];
            if (t->anchor_agent_id == 0)
                continue;
            int hl = half_mplp;
            /* Neuroticism rumination: negative-valence MPLP decays slower for high-N mobs */
            if (t->valence < 0.0f) {
                float n_scale = 1.0f + MALP_N_RUMINATION_SCALE * N_final; /* range [1.0, 2.0] */
                hl = (int)((float)hl * n_scale);
            }
            if (t->persistence == MALP_PERSIST_HIGH)
                hl = (int)((float)hl * 1.5f);
            /* Decay from encoded base_magnitude (strength at last reinforcement), exactly
             * as MALP uses e->salience as its reference value.  This ensures the power-law
             * curve is evaluated from a single fixed origin rather than being reapplied
             * multiplicatively on every tick (which would cause compounding over-decay). */
            t->magnitude = t->base_magnitude * powerlaw_intensity(t->last_updated, hl);
            if (t->magnitude > 1.0f)
                t->magnitude = 1.0f;
            /* Passive rehearsal decay: rate scales with current rehearsal_count
             * so that well-rehearsed traits erode faster in absolute terms and
             * can re-enter the formation window sooner once interactions stop
             * (rehearsal_count falls back below the threshold). */
            if (t->rehearsal_count > 0) {
                int rdecay = 1 + (t->rehearsal_count / MALP_REHEARSAL_DECAY_DIVISOR);
                t->rehearsal_count -= rdecay;
                if (t->rehearsal_count < 0)
                    t->rehearsal_count = 0;
            }
            /* Contextual modifier decay: ctx[] values decay faster than the global
             * magnitude so that situational biases fade without erasing the baseline
             * personality.  ctx[0]=GLOBAL is never written so the loop skips index 0. */
            for (int c = 1; c < MPLP_CTX_MAX; c++) {
                float cv = t->ctx[c];
                if (cv == 0.0f)
                    continue;
                if (cv > -0.01f && cv < 0.01f) {
                    t->ctx[c] = 0.0f;
                    continue;
                }
                t->ctx[c] = cv * MPLP_CTX_DECAY_RATE;
            }
            if (t->magnitude >= 0.02f) {
                if (w != i)
                    ai->mplp[w] = ai->mplp[i];
                w++;
            }
        }
        ai->mplp_count = w;
    }
}

void consolidator_tick(struct char_data *mob)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    struct mob_ai_data *ai = mob->ai_data;
    float theta = (float)CONFIG_MALP_THETA_CONS / 100.0f;
    if (theta <= 0.0f)
        theta = 0.65f;

    /* Conscientiousness raises effective consolidation threshold (fewer impulsive memories) */
    float C_final = sec_get_conscientiousness_final(mob);
    theta = theta + MALP_C_THETA_BOOST * C_final; /* C=1.0 → threshold +MALP_C_THETA_BOOST */
    if (theta > 0.95f)
        theta = 0.95f;

    int rehearsal_threshold = CONFIG_MALP_REHEARSAL_THRESHOLD;
    if (rehearsal_threshold < 1)
        rehearsal_threshold = 3;

    time_t now = time(NULL);
    int i;

    /*
     * CONSOLIDATE_SLOT(mem): process one episodic memory slot.
     * Implemented as a macro to share the consolidation logic between the
     * passive and active memory buffers without code duplication.
     * Uses do-while(0) so 'break' exits cleanly on early-out conditions.
     * Note: mem->entity_id is already validated by the first guard at the
     * top of the macro; no duplicate check is needed below.
     */
#define CONSOLIDATE_SLOT(mem)                                                                                          \
    do {                                                                                                               \
        if ((mem)->timestamp == 0 || (mem)->entity_id == 0)                                                            \
            break;                                                                                                     \
        float age_hours = (float)(now - (mem)->timestamp) / 3600.0f;                                                   \
        float arousal = slot_arousal(mem);                                                                             \
        float social_w =                                                                                               \
            ((mem)->entity_type == ENTITY_TYPE_PLAYER) ? MALP_SOCIAL_WEIGHT_PLAYER : MALP_SOCIAL_WEIGHT_MOB;           \
        int rehearsal = count_rehearsal(mob, (mem)->entity_id, (mem)->entity_type);                                    \
        if (rehearsal > MALP_MAX_REHEARSAL)                                                                            \
            rehearsal = MALP_MAX_REHEARSAL;                                                                            \
        float S = compute_salience(arousal, rehearsal, social_w, age_hours);                                           \
        if (S < theta)                                                                                                 \
            break;                                                                                                     \
        float valence = compute_peak_end_valence(mob, (mem)->entity_id, (mem)->entity_type, mem);                      \
        /* Find existing MALP entry for this agent */                                                                  \
        struct malp_entry *existing = NULL;                                                                            \
        int _j;                                                                                                        \
        for (_j = 0; _j < ai->malp_count; _j++) {                                                                      \
            if (ai->malp[_j].agent_id == (mem)->entity_id && ai->malp[_j].agent_type == (mem)->entity_type) {          \
                existing = &ai->malp[_j];                                                                              \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
        if (existing) {                                                                                                \
            float lambda = 0.3f * S;                                                                                   \
            if (lambda > 0.5f)                                                                                         \
                lambda = 0.5f;                                                                                         \
            existing->valence = (1.0f - lambda) * existing->valence + lambda * valence;                                \
            existing->arousal = (1.0f - lambda) * existing->arousal + lambda * arousal;                                \
            existing->salience = S;                                                                                    \
            existing->rehearsal = rehearsal;                                                                           \
            existing->interaction_type = classify_social_itype(existing->interaction_type, valence);                   \
            if ((mem)->major_event || arousal >= MALP_HIGH_PERSIST_AROUSAL) {                                          \
                existing->persistence = MALP_PERSIST_HIGH;                                                             \
                existing->major_event = 1;                                                                             \
            }                                                                                                          \
        } else {                                                                                                       \
            if (!malp_grow(mob)) {                                                                                     \
                log1("MALP: memory allocation failure for mob %s (current count: %d)", GET_NAME(mob),                  \
                     mob->ai_data->malp_count);                                                                        \
                break;                                                                                                 \
            }                                                                                                          \
            struct malp_entry *_e = &ai->malp[ai->malp_count];                                                         \
            memset(_e, 0, sizeof(struct malp_entry));                                                                  \
            _e->agent_id = (mem)->entity_id;                                                                           \
            _e->agent_type = (mem)->entity_type;                                                                       \
            _e->interaction_type = classify_social_itype((mem)->interaction_type, valence);                            \
            _e->major_event = (mem)->major_event;                                                                      \
            _e->timestamp = (mem)->timestamp;                                                                          \
            _e->last_retrieved = 0;                                                                                    \
            _e->valence = valence;                                                                                     \
            _e->arousal = arousal;                                                                                     \
            _e->salience = S;                                                                                          \
            _e->intensity = S;                                                                                         \
            _e->rehearsal = rehearsal;                                                                                 \
            _e->recon_ticks_left = 0;                                                                                  \
            if ((mem)->major_event || arousal >= MALP_HIGH_PERSIST_AROUSAL)                                            \
                _e->persistence = MALP_PERSIST_HIGH;                                                                   \
            else if (S >= 0.80f)                                                                                       \
                _e->persistence = MALP_PERSIST_MEDIUM;                                                                 \
            else                                                                                                       \
                _e->persistence = MALP_PERSIST_LOW;                                                                    \
            ai->malp_count++;                                                                                          \
        }                                                                                                              \
        /* Hebbian MPLP trait formation */                                                                             \
        if (rehearsal >= rehearsal_threshold) {                                                                        \
            struct mplp_trait *trait = NULL;                                                                           \
            int _trait_type = (valence >= 0.0f) ? MPLP_TRAIT_APPROACH : MPLP_TRAIT_AVOIDANCE;                          \
            /* Each agent has at most one approach/avoidance MPLP entry; its type can                                  \
             * flip as the running-average valence shifts.  We exclude AROUSAL_BIAS                                    \
             * entries, which are tracked separately for the same agent. */                                            \
            for (_j = 0; _j < ai->mplp_count; _j++) {                                                                  \
                if (ai->mplp[_j].anchor_agent_id == (mem)->entity_id &&                                                \
                    ai->mplp[_j].agent_type == (mem)->entity_type &&                                                   \
                    ai->mplp[_j].trait_type != MPLP_TRAIT_AROUSAL_BIAS) {                                              \
                    trait = &ai->mplp[_j];                                                                             \
                    break;                                                                                             \
                }                                                                                                      \
            }                                                                                                          \
            if (!trait && ai->mplp_count < MPLP_MAX_PER_MOB) {                                                         \
                if (!mplp_grow(mob)) {                                                                                 \
                    log1("MPLP: memory allocation failure for mob %s (current count: %d)", GET_NAME(mob),              \
                         mob->ai_data->mplp_count);                                                                    \
                } else {                                                                                               \
                    trait = &ai->mplp[ai->mplp_count];                                                                 \
                    memset(trait, 0, sizeof(struct mplp_trait));                                                       \
                    trait->anchor_agent_id = (mem)->entity_id;                                                         \
                    trait->agent_type = (mem)->entity_type;                                                            \
                    trait->trait_type = _trait_type;                                                                   \
                    trait->magnitude = 0.0f;                                                                           \
                    trait->base_magnitude = 0.0f;                                                                      \
                    trait->valence = valence;                                                                          \
                    trait->rehearsal_count = rehearsal;                                                                \
                    trait->persistence = MALP_PERSIST_LOW;                                                             \
                    trait->last_updated = now;                                                                         \
                    ai->mplp_count++;                                                                                  \
                }                                                                                                      \
            } else if (trait) {                                                                                        \
                float _delta = 0.15f * S;                                                                              \
                if (_delta > 0.3f)                                                                                     \
                    _delta = 0.3f;                                                                                     \
                trait->magnitude += _delta;                                                                            \
                if (trait->magnitude > 1.0f)                                                                           \
                    trait->magnitude = 1.0f;                                                                           \
                /* Update encoded reference: decay will now be measured from this point */                             \
                trait->base_magnitude = trait->magnitude;                                                              \
                float _alpha = 0.2f;                                                                                   \
                trait->valence = (1.0f - _alpha) * trait->valence + _alpha * valence;                                  \
                trait->rehearsal_count = rehearsal;                                                                    \
                trait->last_updated = now;                                                                             \
                trait->trait_type = (trait->valence >= 0.0f) ? MPLP_TRAIT_APPROACH : MPLP_TRAIT_AVOIDANCE;             \
                if (arousal >= MALP_HIGH_PERSIST_AROUSAL && trait->valence < 0.0f)                                     \
                    trait->persistence = MALP_PERSIST_HIGH;                                                            \
                else if (S >= 0.80f && trait->persistence < MALP_PERSIST_MEDIUM)                                       \
                    trait->persistence = MALP_PERSIST_MEDIUM;                                                          \
                if (arousal >= 0.70f) {                                                                                \
                    bool _found_ab = FALSE;                                                                            \
                    for (_j = 0; _j < ai->mplp_count; _j++) {                                                          \
                        if (ai->mplp[_j].anchor_agent_id == (mem)->entity_id &&                                        \
                            ai->mplp[_j].agent_type == (mem)->entity_type &&                                           \
                            ai->mplp[_j].trait_type == MPLP_TRAIT_AROUSAL_BIAS) {                                      \
                            ai->mplp[_j].magnitude += 0.05f * S;                                                       \
                            if (ai->mplp[_j].magnitude > 1.0f)                                                         \
                                ai->mplp[_j].magnitude = 1.0f;                                                         \
                            ai->mplp[_j].base_magnitude = ai->mplp[_j].magnitude;                                      \
                            ai->mplp[_j].last_updated = now;                                                           \
                            _found_ab = TRUE;                                                                          \
                            break;                                                                                     \
                        }                                                                                              \
                    }                                                                                                  \
                    if (!_found_ab && ai->mplp_count < MPLP_MAX_PER_MOB && mplp_grow(mob)) {                           \
                        struct mplp_trait *_ab = &ai->mplp[ai->mplp_count];                                            \
                        memset(_ab, 0, sizeof(struct mplp_trait));                                                     \
                        _ab->anchor_agent_id = (mem)->entity_id;                                                       \
                        _ab->agent_type = (mem)->entity_type;                                                          \
                        _ab->trait_type = MPLP_TRAIT_AROUSAL_BIAS;                                                     \
                        _ab->magnitude = 0.10f * S;                                                                    \
                        _ab->base_magnitude = _ab->magnitude;                                                          \
                        _ab->valence = valence;                                                                        \
                        _ab->rehearsal_count = rehearsal;                                                              \
                        _ab->persistence = MALP_PERSIST_LOW;                                                           \
                        _ab->last_updated = now;                                                                       \
                        ai->mplp_count++;                                                                              \
                    }                                                                                                  \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

    /* Process passive memory buffer (received/witnessed interactions) */
    for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        CONSOLIDATE_SLOT(&ai->memories[i]);
    }

    /* Process active memory buffer (actions performed by this mob) */
    for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        CONSOLIDATE_SLOT(&ai->active_memories[i]);
    }

#undef CONSOLIDATE_SLOT

    /* Prune to stay within limit */
    malp_prune(mob);
    /* Decay intensities */
    malp_decay_tick(mob);
}

struct malp_entry *get_malp_by_agent(struct char_data *mob, long agent_id, int agent_type)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !mob->ai_data->malp || agent_id == 0)
        return NULL;

    struct mob_ai_data *ai = mob->ai_data;
    struct malp_entry *best = NULL;
    float best_intensity = 0.0f;
    int i;

    for (i = 0; i < ai->malp_count; i++) {
        struct malp_entry *e = &ai->malp[i];
        if (e->agent_id != agent_id || e->agent_type != agent_type)
            continue;
        if (e->intensity > best_intensity) {
            best_intensity = e->intensity;
            best = e;
        }
    }

    if (!best)
        return NULL;

    /* Compute retrieval probability P_ret = sigmoid(k * (cue_score − 0.5)).
     *
     * cue_score combines three factors (weights defined in malp.h):
     *  MALP_CUE_WEIGHT_INTENSITY × intensity     — primary strength of the memory
     *  MALP_CUE_WEIGHT_AROUSAL   × arousal_match — state-dependent recall (Bower 1981):
     *                                               easier when mob's current arousal
     *                                               matches the arousal stored in memory
     *  MALP_CUE_WEIGHT_RECENCY   × recency        — recently-retrieved memories are
     *                                               more accessible (within 1 game-hour)
     */
    {
        float mob_arousal = get_mob_peak_arousal(mob);

        /* Arousal match: 1.0 when identical, 0.0 when completely opposite */
        float arousal_match = 1.0f - fabsf(mob_arousal - best->arousal);
        if (arousal_match < 0.0f)
            arousal_match = 0.0f;

        /* Recency: linear decay from 1.0 to 0.0 over the first game-hour.
         * Guard against negative hours_since (clock skew / ntp step). */
        float recency = 0.0f;
        if (best->last_retrieved > 0) {
            float hours_since = (float)(time(NULL) - best->last_retrieved) / 3600.0f;
            if (hours_since >= 0.0f && hours_since < 1.0f)
                recency = 1.0f - hours_since;
        }

        float cue_score = best->intensity * MALP_CUE_WEIGHT_INTENSITY + arousal_match * MALP_CUE_WEIGHT_AROUSAL +
                          recency * MALP_CUE_WEIGHT_RECENCY;
        if (cue_score > 1.0f)
            cue_score = 1.0f;

        float P_ret = 1.0f / (1.0f + expf(-MALP_PRET_K * (cue_score - 0.5f)));

        /* Open or extend the reconsolidation window when P_ret >= threshold.
         * If the window is already open but smaller than the configured duration
         * (e.g., from a previous retrieval), reset it to the full duration so
         * that repeated, qualifying retrievals keep the entry mutable. */
        if (P_ret >= MALP_THETA_REACT) {
            int window = CONFIG_MALP_RECON_WINDOW_TICKS;
            if (window < 1)
                window = 60;
            if (best->recon_ticks_left <= 0 || best->recon_ticks_left < window)
                best->recon_ticks_left = window;
            best->last_retrieved = time(NULL);
            /* Saturate rehearsal at hard cap to prevent salience lock */
            if (best->rehearsal < MALP_MAX_REHEARSAL)
                best->rehearsal++;
        }
    }

    return best;
}

float get_mplp_approach_modifier(struct char_data *mob, long agent_id, int agent_type)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !mob->ai_data->mplp || agent_id == 0)
        return 0.0f;

    struct mob_ai_data *ai = mob->ai_data;
    float modifier = 0.0f;
    int i;

    for (i = 0; i < ai->mplp_count; i++) {
        struct mplp_trait *t = &ai->mplp[i];
        if (t->anchor_agent_id != agent_id || t->agent_type != agent_type)
            continue;
        if (t->trait_type == MPLP_TRAIT_APPROACH)
            modifier += t->magnitude;
        else if (t->trait_type == MPLP_TRAIT_AVOIDANCE)
            modifier -= t->magnitude;
    }

    if (modifier > 1.0f)
        modifier = 1.0f;
    if (modifier < -1.0f)
        modifier = -1.0f;
    return modifier;
}

float get_mplp_arousal_bias(struct char_data *mob, long agent_id, int agent_type)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !mob->ai_data->mplp || agent_id == 0)
        return 0.0f;

    struct mob_ai_data *ai = mob->ai_data;
    float bias = 0.0f;
    int i;

    for (i = 0; i < ai->mplp_count; i++) {
        struct mplp_trait *t = &ai->mplp[i];
        if (t->anchor_agent_id != agent_id || t->agent_type != agent_type)
            continue;
        if (t->trait_type == MPLP_TRAIT_AROUSAL_BIAS)
            bias += t->magnitude;
    }

    if (bias > 1.0f)
        bias = 1.0f;
    if (bias < 0.0f)
        bias = 0.0f;
    return bias;
}

float get_mplp_exhibition_response(struct char_data *mob)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !mob->ai_data->mplp)
        return 0.0f;

    struct mob_ai_data *ai = mob->ai_data;
    float modifier = 0.0f;
    int i;

    for (i = 0; i < ai->mplp_count; i++) {
        struct mplp_trait *t = &ai->mplp[i];
        if (t->anchor_agent_id != MPLP_GLOBAL_ANCHOR)
            continue;
        if (t->trait_type != MPLP_TRAIT_EXHIBITION_RESPONSE)
            continue;
        /* Signed contribution: valence determines direction, magnitude determines strength */
        modifier += (t->valence >= 0.0f ? 1.0f : -1.0f) * t->magnitude;
    }

    if (modifier > 1.0f)
        modifier = 1.0f;
    if (modifier < -1.0f)
        modifier = -1.0f;
    return modifier;
}

float get_mplp_modesty_response(struct char_data *mob)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !mob->ai_data->mplp)
        return 0.0f;

    struct mob_ai_data *ai = mob->ai_data;
    float modifier = 0.0f;
    int i;

    for (i = 0; i < ai->mplp_count; i++) {
        struct mplp_trait *t = &ai->mplp[i];
        if (t->anchor_agent_id != MPLP_GLOBAL_ANCHOR)
            continue;
        if (t->trait_type != MPLP_TRAIT_MODESTY_RESPONSE)
            continue;
        modifier += (t->valence >= 0.0f ? 1.0f : -1.0f) * t->magnitude;
    }

    if (modifier > 1.0f)
        modifier = 1.0f;
    if (modifier < -1.0f)
        modifier = -1.0f;
    return modifier;
}

/**
 * Return non-zero if the given context-global MPLP trait is unsigned (range 0..1).
 * Unsigned traits represent strength amplifiers or unipolar biases where a negative
 * value carries no distinct meaning (e.g. sensitivity, aversion, expectation).
 */
static int is_unsigned_mplp_trait(int trait_type)
{
    switch (trait_type) {
        case MPLP_TRAIT_GENDER_NORM_SENSITIVITY:
        case MPLP_TRAIT_STATUS_SENSITIVITY:
        case MPLP_TRAIT_SUSPICION_BIAS:
        case MPLP_TRAIT_BETRAYAL_SENSITIVITY:
        case MPLP_TRAIT_LOYALTY_EXPECTATION:
        case MPLP_TRAIT_INGROUP_BIAS:
        case MPLP_TRAIT_OUTGROUP_AVERSION:
        case MPLP_TRAIT_RECIPROCITY_EXPECTATION:
        case MPLP_TRAIT_REVENGE_TENDENCY:
        case MPLP_TRAIT_FORGIVENESS_RATE:
        case MPLP_TRAIT_DISTRESS_AVERSION:
        case MPLP_TRAIT_COMPASSION_BIAS:
            return 1;
        default:
            return 0;
    }
}

/**
 * Return non-zero if the given trait type is a valid context-global MPLP trait
 * (i.e., anchored with MPLP_GLOBAL_ANCHOR and accepted by reinforce_mplp_context_trait()).
 * Context-global traits span from MPLP_TRAIT_EXHIBITION_RESPONSE (3) through
 * MPLP_TRAIT_COMPASSION_BIAS (28).  Agent-anchored traits (AVOIDANCE=0, APPROACH=1,
 * AROUSAL_BIAS=2) are excluded.
 */
static int is_context_global_trait_type(int trait_type)
{
    return (trait_type >= MPLP_TRAIT_EXHIBITION_RESPONSE && trait_type <= MPLP_TRAIT_COMPASSION_BIAS);
}

/**
 * Helper: retrieve the signed modifier for a single context-global trait type.
 * Returns a value in [-1, +1] for signed traits, or [0, 1] for unsigned traits.
 */
static float get_context_trait(struct char_data *mob, int trait_type)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !mob->ai_data->mplp)
        return 0.0f;

    struct mob_ai_data *ai = mob->ai_data;
    float result = 0.0f;
    int i;
    int is_unsigned = is_unsigned_mplp_trait(trait_type);

    for (i = 0; i < ai->mplp_count; i++) {
        struct mplp_trait *t = &ai->mplp[i];
        if (t->anchor_agent_id != MPLP_GLOBAL_ANCHOR)
            continue;
        if (t->trait_type != trait_type)
            continue;
        /* Unsigned traits (0..1): always accumulate positive magnitude */
        if (is_unsigned)
            result += t->magnitude;
        else
            result += (t->valence >= 0.0f ? 1.0f : -1.0f) * t->magnitude;
    }

    /* Clamp: unsigned traits to [0,1], signed traits to [-1,+1] */
    float lo = is_unsigned ? 0.0f : -1.0f;
    if (result > 1.0f)
        result = 1.0f;
    if (result < lo)
        result = lo;
    return result;
}

float get_mplp_masculinity_response(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_MASCULINITY_RESPONSE);
}

float get_mplp_femininity_response(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_FEMININITY_RESPONSE);
}

float get_mplp_androgyny_tolerance(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_ANDROGYNY_TOLERANCE);
}

float get_mplp_gender_norm_sensitivity(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_GENDER_NORM_SENSITIVITY);
}

/* ── Category 1: Hierarchy / Social Power ────────────────────────────────── */

float get_mplp_dominance(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_DOMINANCE); }

float get_mplp_submission(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_SUBMISSION); }

float get_mplp_authority_response(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_AUTHORITY_RESPONSE);
}

float get_mplp_status_sensitivity(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_STATUS_SENSITIVITY);
}

/* ── Category 2: Social Trust System ─────────────────────────────────────── */

float get_mplp_trust_bias(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_TRUST_BIAS); }

float get_mplp_suspicion_bias(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_SUSPICION_BIAS); }

float get_mplp_betrayal_sensitivity(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_BETRAYAL_SENSITIVITY);
}

float get_mplp_loyalty_expectation(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_LOYALTY_EXPECTATION);
}

/* ── Category 3: Social Norm Sensitivity ─────────────────────────────────── */

float get_mplp_politeness_response(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_POLITENESS_RESPONSE);
}

float get_mplp_rudeness_response(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_RUDENESS_RESPONSE); }

/* ── Category 4: Social Identity Bias ────────────────────────────────────── */

float get_mplp_ingroup_bias(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_INGROUP_BIAS); }

float get_mplp_outgroup_aversion(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_OUTGROUP_AVERSION); }

float get_mplp_novel_agent_interest(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_NOVEL_AGENT_INTEREST);
}

/* ── Category 5: Reciprocity System ──────────────────────────────────────── */

float get_mplp_reciprocity_expectation(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_RECIPROCITY_EXPECTATION);
}

float get_mplp_gratitude_response(struct char_data *mob)
{
    return get_context_trait(mob, MPLP_TRAIT_GRATITUDE_RESPONSE);
}

float get_mplp_revenge_tendency(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_REVENGE_TENDENCY); }

float get_mplp_forgiveness_rate(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_FORGIVENESS_RATE); }

/* ── Category 6: Empathy System ──────────────────────────────────────────── */

float get_mplp_empathy_response(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_EMPATHY_RESPONSE); }

float get_mplp_distress_aversion(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_DISTRESS_AVERSION); }

float get_mplp_compassion_bias(struct char_data *mob) { return get_context_trait(mob, MPLP_TRAIT_COMPASSION_BIAS); }

void reinforce_mplp_context_trait(struct char_data *mob, int trait_type, float valence, float salience)
{
    /* Delegate to the context-aware variant with MPLP_CTX_GLOBAL so that the
     * full delta applies to the global magnitude (no context split). */
    reinforce_mplp_context_trait_ctx(mob, trait_type, valence, salience, MPLP_CTX_GLOBAL);
}

int get_mplp_context_from_interact_type(int interact_type)
{
    switch (interact_type) {
        case INTERACT_ATTACKED:
        case INTERACT_HEALED:
        case INTERACT_RESCUED:
        case INTERACT_ASSISTED:
        case INTERACT_ALLY_DIED:
        case INTERACT_WITNESSED_DEATH:
        case INTERACT_ABANDON_ALLY:
        case INTERACT_SACRIFICE_SELF:
            return MPLP_CTX_COMBAT;

        case INTERACT_RECEIVED_ITEM:
        case INTERACT_STOLEN_FROM:
            return MPLP_CTX_TRADE;

        case INTERACT_QUEST_COMPLETE:
        case INTERACT_QUEST_FAIL:
        case INTERACT_BETRAYAL:
        case INTERACT_DECEIVE:
            return MPLP_CTX_QUEST;

        case INTERACT_WITNESSED_OFFENSIVE_MAGIC:
        case INTERACT_WITNESSED_SUPPORT_MAGIC:
            return MPLP_CTX_MAGIC;

        case INTERACT_SOCIAL_POSITIVE:
        case INTERACT_SOCIAL_NEGATIVE:
        case INTERACT_SOCIAL_VIOLENT:
        case INTERACT_SOCIAL_NEUTRAL:
            return MPLP_CTX_SOCIAL;

        default:
            return MPLP_CTX_GLOBAL;
    }
}

void reinforce_mplp_context_trait_ctx(struct char_data *mob, int trait_type, float valence, float salience,
                                      int ctx_type)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data)
        return;
    if (!CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;
    if (!is_context_global_trait_type(trait_type))
        return;
    if (ctx_type < 0 || ctx_type >= MPLP_CTX_MAX)
        ctx_type = MPLP_CTX_GLOBAL;

    struct mob_ai_data *ai = mob->ai_data;
    struct mplp_trait *trait = NULL;
    int i;
    time_t now = time(NULL);

    /* Clamp inputs */
    if (salience > 1.0f)
        salience = 1.0f;
    if (salience < 0.0f)
        salience = 0.0f;
    if (valence > 1.0f)
        valence = 1.0f;
    if (valence < -1.0f)
        valence = -1.0f;

    /* Find existing global trait slot */
    for (i = 0; i < ai->mplp_count; i++) {
        if (ai->mplp[i].anchor_agent_id == MPLP_GLOBAL_ANCHOR && ai->mplp[i].trait_type == trait_type) {
            trait = &ai->mplp[i];
            break;
        }
    }

    if (!trait) {
        if (ai->mplp_count >= MPLP_MAX_PER_MOB)
            return;
        if (!mplp_grow(mob))
            return;
        trait = &ai->mplp[ai->mplp_count];
        memset(trait, 0, sizeof(struct mplp_trait));
        trait->anchor_agent_id = MPLP_GLOBAL_ANCHOR;
        trait->agent_type = ENTITY_TYPE_GLOBAL;
        trait->trait_type = trait_type;
        trait->magnitude = 0.0f;
        trait->base_magnitude = 0.0f;
        trait->valence = valence;
        trait->rehearsal_count = 1;
        trait->persistence = MALP_PERSIST_LOW;
        trait->last_updated = now;
        ai->mplp_count++;
        /* Fall through so the first experience applies the reinforcement delta */
    }

    float delta = 0.15f * salience;
    if (delta > 0.30f)
        delta = 0.30f;

    /* Global personality: receives the larger fraction */
    float global_delta = delta * MPLP_CTX_GLOBAL_WEIGHT;
    trait->magnitude += global_delta;
    if (trait->magnitude > 1.0f)
        trait->magnitude = 1.0f;
    trait->base_magnitude = trait->magnitude;

    /* Contextual modifier: receives the smaller fraction when context is specific */
    if (ctx_type != MPLP_CTX_GLOBAL) {
        float ctx_delta = delta * MPLP_CTX_LOCAL_WEIGHT;
        int is_unsigned = is_unsigned_mplp_trait(trait_type);
        if (is_unsigned) {
            trait->ctx[ctx_type] += ctx_delta;
            if (trait->ctx[ctx_type] > 1.0f)
                trait->ctx[ctx_type] = 1.0f;
            if (trait->ctx[ctx_type] < 0.0f)
                trait->ctx[ctx_type] = 0.0f;
        } else {
            float signed_delta = (valence >= 0.0f ? ctx_delta : -ctx_delta);
            trait->ctx[ctx_type] += signed_delta;
            if (trait->ctx[ctx_type] > 1.0f)
                trait->ctx[ctx_type] = 1.0f;
            if (trait->ctx[ctx_type] < -1.0f)
                trait->ctx[ctx_type] = -1.0f;
        }
    } else {
        /* GLOBAL context: apply full delta (no split) to magnitude */
        float extra = delta * MPLP_CTX_LOCAL_WEIGHT;
        trait->magnitude += extra;
        if (trait->magnitude > 1.0f)
            trait->magnitude = 1.0f;
        trait->base_magnitude = trait->magnitude;
    }

    /* Running-average valence update */
    float alpha = MPLP_VALENCE_LEARNING_RATE;
    trait->valence = (1.0f - alpha) * trait->valence + alpha * valence;

    if (trait->rehearsal_count < MALP_MAX_REHEARSAL)
        trait->rehearsal_count++;
    trait->last_updated = now;

    /* Elevate persistence as the trait strengthens */
    if (trait->magnitude >= 0.70f && trait->persistence < MALP_PERSIST_HIGH)
        trait->persistence = MALP_PERSIST_HIGH;
    else if (trait->magnitude >= 0.40f && trait->persistence < MALP_PERSIST_MEDIUM)
        trait->persistence = MALP_PERSIST_MEDIUM;
}

float get_mplp_trait_with_ctx(struct char_data *mob, int trait_type, int ctx_type)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !mob->ai_data->mplp)
        return 0.0f;
    if (ctx_type < 0 || ctx_type >= MPLP_CTX_MAX)
        ctx_type = MPLP_CTX_GLOBAL;

    struct mob_ai_data *ai = mob->ai_data;
    float result = 0.0f;
    int i;
    int is_unsigned = is_unsigned_mplp_trait(trait_type);

    /* Accumulation mirrors get_context_trait(): in normal operation at most one
     * MPLP_GLOBAL_ANCHOR slot exists per trait_type; summing across any extras
     * combines their contributions, consistent with the single-anchor guarantee
     * maintained by reinforce_mplp_context_trait_ctx(). */
    for (i = 0; i < ai->mplp_count; i++) {
        struct mplp_trait *t = &ai->mplp[i];
        if (t->anchor_agent_id != MPLP_GLOBAL_ANCHOR)
            continue;
        if (t->trait_type != trait_type)
            continue;
        float global_part = is_unsigned ? t->magnitude : (t->valence >= 0.0f ? 1.0f : -1.0f) * t->magnitude;
        float ctx_part = (ctx_type != MPLP_CTX_GLOBAL) ? t->ctx[ctx_type] : 0.0f;
        result += global_part + ctx_part;
    }

    float lo = is_unsigned ? 0.0f : -1.0f;
    if (result > 1.0f)
        result = 1.0f;
    if (result < lo)
        result = lo;
    return result;
}

float mplp_get_effective_trait(struct char_data *mob, struct char_data *actor, int trait_type, int ctx_type)
{
    /* Base trait value (0.0 when mob has no MPLP or trait is absent) */
    float base = get_mplp_trait_with_ctx(mob, trait_type, ctx_type);

    /* Without a valid NPC mob or actor we cannot compute a reputation modifier.
     * Non-NPC mobs have no MPLP and no ai_data, so return base (0.0) directly. */
    if (!mob || !IS_NPC(mob) || !actor)
        return base;

    /* Get actor reputation, clamped defensively to the valid 0–100 range */
    int rep = GET_REPUTATION(actor);
    if (rep < 0)
        rep = 0;
    if (rep > 100)
        rep = 100;

    /* Normalize reputation to [−1.0, +1.0] using MPLP_REP_POSITIVE_THRESHOLD as
     * the neutral midpoint: rep == MPLP_REP_POSITIVE_THRESHOLD → 0 (no bias). */
    float rep_norm = ((float)rep - (float)MPLP_REP_POSITIVE_THRESHOLD) / (float)MPLP_REP_POSITIVE_THRESHOLD;

    /* Trait-specific reputation delta.
     * Positive rep_norm (high reputation) raises prosocial traits and lowers
     * adversarial ones; negative rep_norm does the opposite.
     * Traits not listed here are returned unmodified (rep-neutral). */
    float rep_delta = 0.0f;
    switch (trait_type) {
        case MPLP_TRAIT_TRUST_BIAS:
            rep_delta = rep_norm * MPLP_REP_BIAS_SCALE;
            break;
        case MPLP_TRAIT_SUSPICION_BIAS:
            /* High rep reduces baseline suspicion */
            rep_delta = -rep_norm * MPLP_REP_BIAS_SCALE;
            break;
        case MPLP_TRAIT_FORGIVENESS_RATE:
            rep_delta = rep_norm * (MPLP_REP_BIAS_SCALE * 0.5f);
            break;
        case MPLP_TRAIT_REVENGE_TENDENCY:
            /* High rep dampens revenge impulse */
            rep_delta = -rep_norm * (MPLP_REP_BIAS_SCALE * 0.5f);
            break;
        case MPLP_TRAIT_COMPASSION_BIAS:
            rep_delta = rep_norm * (MPLP_REP_BIAS_SCALE * 0.4f);
            break;
        case MPLP_TRAIT_GRATITUDE_RESPONSE:
            rep_delta = rep_norm * (MPLP_REP_BIAS_SCALE * 0.4f);
            break;
        case MPLP_TRAIT_EMPATHY_RESPONSE:
            rep_delta = rep_norm * (MPLP_REP_BIAS_SCALE * 0.3f);
            break;
        case MPLP_TRAIT_OUTGROUP_AVERSION:
            /* High rep reduces outgroup aversion toward this actor */
            rep_delta = -rep_norm * (MPLP_REP_BIAS_SCALE * 0.4f);
            break;
        case MPLP_TRAIT_BETRAYAL_SENSITIVITY:
            /* Negative rep heightens betrayal sensitivity */
            rep_delta = -rep_norm * (MPLP_REP_BIAS_SCALE * 0.3f);
            break;
        default:
            return base;
    }

    float effective = base + rep_delta;

    /* Clamp to valid range: [0, 1] for unsigned traits, [−1, 1] for signed */
    int is_unsigned = is_unsigned_mplp_trait(trait_type);
    float lo = is_unsigned ? 0.0f : -1.0f;
    if (effective > 1.0f)
        effective = 1.0f;
    if (effective < lo)
        effective = lo;

    return effective;
}

void apply_malp_emotion_effects(struct char_data *mob, struct char_data *actor, float interaction_valence)
{
    if (!mob || !actor || !IS_NPC(mob) || !mob->ai_data)
        return;
    if (!CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    /* Determine actor entity identifiers */
    long agent_id;
    int agent_type;
    if (IS_NPC(actor)) {
        agent_type = ENTITY_TYPE_MOB;
        agent_id = char_script_id(actor);
    } else {
        agent_type = ENTITY_TYPE_PLAYER;
        agent_id = GET_IDNUM(actor);
    }
    if (agent_id == 0)
        return;

    /* Retrieve best MALP entry (opens reconsolidation window if P_ret >= threshold) */
    struct malp_entry *malp = get_malp_by_agent(mob, agent_id, agent_type);

    /* ── Per-actor social cooldown ───────────────────────────────────────────
     * If MALP emotion effects for this actor were applied within the last
     * MALP_SOCIAL_COOLDOWN_SECS seconds, skip ALL effects (MALP and MPLP).
     * This prevents rapid reinforcement feedback loops when the same actor is
     * continuously present in the room and MALP/MPLP effects accumulate faster
     * than the homeostasis system can restore baseline emotions. */
    if (malp && malp->last_applied > 0 && (time(NULL) - malp->last_applied) < (time_t)MALP_SOCIAL_COOLDOWN_SECS)
        return;

    /* ── Dominant-actor dampening ────────────────────────────────────────────
     * Compute the rehearsal share of this actor across all MALP entries.
     * If one actor holds more than MALP_DOMINANCE_THRESHOLD of total rehearsal,
     * their effective emotion-effect intensity is multiplied by
     * MALP_DOMINANCE_DAMPENING (0.70).  This allows strong memories to keep
     * influencing behaviour while preventing one actor from monopolising
     * NPC cognition (salience lock).
     * The check requires malp_count > 1: when only one entry exists it holds
     * 100 % by definition, which is realistic (not a dominance problem). */
    float dominance_factor = 1.0f;
    if (malp && mob->ai_data->malp_count > 1) {
        int total_rehearsal = 0;
        int di;
        for (di = 0; di < mob->ai_data->malp_count; di++)
            total_rehearsal += mob->ai_data->malp[di].rehearsal;
        if (total_rehearsal > 0) {
            float dom = (float)malp->rehearsal / (float)total_rehearsal;
            if (dom > MALP_DOMINANCE_THRESHOLD)
                dominance_factor = MALP_DOMINANCE_DAMPENING;
        }
    }

    if (malp) {
        float intensity = malp->intensity * dominance_factor;
        /* Scale effect by intensity */
        int delta = (int)(intensity * (float)MALP_EMOTION_DELTA_MAX);
        if (delta < MALP_EMOTION_DELTA_MIN)
            delta = MALP_EMOTION_DELTA_MIN;

        /* Negative memory → fear / anger boost; positive → trust / happiness boost.
         * All changes go through adjust_emotion() (full SEC pipeline). */
        if (malp->valence < -0.2f) {
            /* Aversive memory: raise fear or anger depending on dominance */
            if (malp->arousal >= 0.6f)
                adjust_emotion(mob, &mob->ai_data->emotion_fear, delta);
            else
                adjust_emotion(mob, &mob->ai_data->emotion_anger, (int)(delta * 0.7f));
        } else if (malp->valence > 0.2f) {
            /* Positive memory: small trust / happiness boost */
            adjust_emotion(mob, &mob->ai_data->emotion_trust, (int)(delta * 0.5f));
            adjust_emotion(mob, &mob->ai_data->emotion_happiness, (int)(delta * 0.4f));
        }

        /* Reconsolidation: if the MALP entry is labile (window is open) and the
         * current interaction carries a valence signal, perform a bounded valence
         * update.  new_salience is derived from the mob's current arousal state so
         * that emotionally-charged reconsolidation carries proportionally more weight.
         *
         * Passing interaction_valence = 0.0f explicitly suppresses reconsolidation
         * for neutral interactions that should not alter stored memories. */
        if (malp->recon_ticks_left > 0 && interaction_valence != 0.0f) {
            float social_w = (agent_type == ENTITY_TYPE_PLAYER) ? MALP_SOCIAL_WEIGHT_PLAYER : MALP_SOCIAL_WEIGHT_MOB;
            float mob_arousal = get_mob_peak_arousal(mob);
            float new_salience = compute_salience(mob_arousal, malp->rehearsal, social_w, 0.0f);
            retrieve_and_reconsolidate(mob, agent_id, agent_type, interaction_valence, new_salience);
        }

        /* Stamp the application time so the per-actor cooldown can be enforced */
        malp->last_applied = time(NULL);
    }

    /* Apply MPLP approach/avoidance modifier (dampened for dominant actors) */
    float approach = get_mplp_approach_modifier(mob, agent_id, agent_type) * dominance_factor;
    if (approach < -0.15f) {
        /* Avoidance trait: mild fear increase */
        int av_delta = (int)((-approach) * (float)MPLP_EMOTION_DELTA_MAX);
        if (av_delta > 0)
            adjust_emotion(mob, &mob->ai_data->emotion_fear, av_delta);
    } else if (approach > 0.15f) {
        /* Approach trait: mild friendship increase */
        int ap_delta = (int)(approach * (float)MPLP_EMOTION_DELTA_MAX);
        if (ap_delta > 0)
            adjust_emotion(mob, &mob->ai_data->emotion_friendship, ap_delta);
    }

    /* Apply MPLP arousal bias (dampened for dominant actors) */
    float arousal_bias = get_mplp_arousal_bias(mob, agent_id, agent_type) * dominance_factor;
    if (arousal_bias > 0.15f) {
        int arb_delta = (int)(arousal_bias * (float)MPLP_EMOTION_DELTA_MAX);
        if (arb_delta > 0)
            adjust_emotion(mob, &mob->ai_data->emotion_excitement, arb_delta);
    }

    /* ── Reputation-biased context-global trait effects ──────────────────────
     * Evaluate the NPC's prosocial and adversarial personality traits with a
     * temporary reputation-based overlay (via mplp_get_effective_trait) and
     * apply corresponding emotion impulses.  Base traits are NOT modified. */
    float eff_trust = mplp_get_effective_trait(mob, actor, MPLP_TRAIT_TRUST_BIAS, MPLP_CTX_GLOBAL) * dominance_factor;
    float eff_suspicion =
        mplp_get_effective_trait(mob, actor, MPLP_TRAIT_SUSPICION_BIAS, MPLP_CTX_GLOBAL) * dominance_factor;
    float eff_forgiveness =
        mplp_get_effective_trait(mob, actor, MPLP_TRAIT_FORGIVENESS_RATE, MPLP_CTX_GLOBAL) * dominance_factor;
    float eff_revenge =
        mplp_get_effective_trait(mob, actor, MPLP_TRAIT_REVENGE_TENDENCY, MPLP_CTX_GLOBAL) * dominance_factor;

    if (eff_trust > MPLP_PERSONALITY_BIAS_THRESHOLD) {
        int d = (int)(eff_trust * (float)MPLP_EMOTION_DELTA_MAX);
        if (d > 0) {
            adjust_emotion(mob, &mob->ai_data->emotion_trust, d);
            adjust_emotion(mob, &mob->ai_data->emotion_friendship, (int)(d * 0.5f));
        }
    } else if (eff_trust < -MPLP_PERSONALITY_BIAS_THRESHOLD) {
        int d = (int)((-eff_trust) * (float)MPLP_EMOTION_DELTA_MAX);
        if (d > 0) {
            adjust_emotion(mob, &mob->ai_data->emotion_fear, (int)(d * 0.5f));
            adjust_emotion(mob, &mob->ai_data->emotion_anger, (int)(d * 0.3f));
        }
    }
    if (eff_suspicion > MPLP_PERSONALITY_BIAS_THRESHOLD) {
        int d = (int)(eff_suspicion * (float)MPLP_EMOTION_DELTA_MAX);
        if (d > 0)
            adjust_emotion(mob, &mob->ai_data->emotion_fear, (int)(d * 0.4f));
    }
    if (eff_forgiveness > MPLP_PERSONALITY_BIAS_THRESHOLD) {
        int d = (int)(eff_forgiveness * (float)MPLP_EMOTION_DELTA_MAX);
        if (d > 0)
            adjust_emotion(mob, &mob->ai_data->emotion_happiness, (int)(d * 0.3f));
    }
    if (eff_revenge > MPLP_PERSONALITY_BIAS_THRESHOLD) {
        int d = (int)(eff_revenge * (float)MPLP_EMOTION_DELTA_MAX);
        if (d > 0)
            adjust_emotion(mob, &mob->ai_data->emotion_anger, (int)(d * 0.4f));
    }

    if (CONFIG_MOB_4D_DEBUG)
        log1("MPLP-REP: mob=%s actor=%s rep=%d trust_eff=%.2f sus_eff=%.2f forg_eff=%.2f rev_eff=%.2f", GET_NAME(mob),
             GET_NAME(actor), GET_REPUTATION(actor), eff_trust, eff_suspicion, eff_forgiveness, eff_revenge);
}

void retrieve_and_reconsolidate(struct char_data *mob, long agent_id, int agent_type, float delta_valence,
                                float new_salience)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || agent_id == 0)
        return;

    struct mob_ai_data *ai = mob->ai_data;
    if (!ai->malp)
        return;

    int i;
    for (i = 0; i < ai->malp_count; i++) {
        struct malp_entry *e = &ai->malp[i];
        if (e->agent_id != agent_id || e->agent_type != agent_type)
            continue;
        /* Only update within the reconsolidation window */
        if (e->recon_ticks_left <= 0)
            continue;

        /* λ_upd = MALP_RECON_LAMBDA_BASE * salience * rehearsal_factor */
        float rehearsal_factor = MIN(1.0f, (float)e->rehearsal / 5.0f);
        float lambda = MALP_RECON_LAMBDA_BASE * new_salience * (0.5f + 0.5f * rehearsal_factor);

        /* Agreeableness accelerates positive reconsolidation (forgiveness) */
        float A_final = sec_get_agreeableness_final(mob);
        if (delta_valence > 0.0f)
            lambda *= (1.0f + 0.5f * A_final);

        /* Clamp lambda to avoid over-correction */
        if (lambda > 0.5f)
            lambda = 0.5f;

        /* Bounded update */
        float clamped_delta = delta_valence;
        if (clamped_delta > MALP_RECON_MAX_DELTA)
            clamped_delta = MALP_RECON_MAX_DELTA;
        if (clamped_delta < -MALP_RECON_MAX_DELTA)
            clamped_delta = -MALP_RECON_MAX_DELTA;

        float old_valence = e->valence;
        e->valence = (1.0f - lambda) * old_valence + lambda * (old_valence + clamped_delta);
        /* Clamp valence to −1..+1 */
        if (e->valence > 1.0f)
            e->valence = 1.0f;
        if (e->valence < -1.0f)
            e->valence = -1.0f;

        /* Slightly boost intensity to reflect re-engagement */
        e->intensity += 0.05f * new_salience;
        if (e->intensity > 1.0f)
            e->intensity = 1.0f;

        /* Reconsolidation dampening: every rewrite reduces raw rehearsal slightly,
         * modelling the interference/rewriting cost of memory reconsolidation.
         * Round to nearest integer to avoid abruptly zeroing small values. */
        e->rehearsal = (int)((float)e->rehearsal * MALP_RECON_DAMPENING_FACTOR + 0.5f);

        e->last_retrieved = time(NULL);
        /* Consuming the update closes the window for this cycle */
        e->recon_ticks_left = 0;
        break;
    }
}

bool try_social_gossip(struct char_data *source, struct char_data *listener)
{
    /* ── Guard conditions ────────────────────────────────────────────────────
     * Both entities must be valid awake mobs with initialised AI data.
     * We require source to have at least one MALP entry to gossip about. */
    if (!source || !listener)
        return FALSE;
    if (!IS_NPC(source) || !IS_NPC(listener))
        return FALSE;
    if (!source->ai_data || !listener->ai_data)
        return FALSE;
    if (!CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return FALSE;
    if (!source->ai_data->malp || source->ai_data->malp_count == 0)
        return FALSE;

    /* ── Find source's strongest eligible MALP entry (the gossip "topic") ───
     * We exclude:
     *  - empty slots (agent_id == 0)
     *  - entries about the listener itself (would be a self-referential loop)
     *  - entries below the minimum intensity threshold
     *
     * Cognitive biases shape topic selection (Phase 1):
     *  – Availability bias: recency factor boosts selection score for recent entries.
     *  – Negativity bias: negative-valence entries get an extra selection bonus.
     *  – Confirmation bias: entries matching the source's prior belief direction
     *    receive a small preference boost.
     * The base score remains the MALP intensity, so bias only tilts selection —
     * it cannot select a memory that is below MALP_GOSSIP_MIN_INTENSITY. */
    long listener_id = char_script_id(listener);
    struct malp_entry *best = NULL;
    float best_score = MALP_GOSSIP_MIN_INTENSITY; /* lower bound */
    int i;
    time_t now = time(NULL); /* used for recency calc and timestamps */

    /* Source's dominant prior belief toward the gossip target is needed for
     * confirmation-bias topic selection.  We compute it lazily after selecting
     * the best entry (see confirmation-bias encoding below), but for selection
     * we use a simpler per-entry valence-sign alignment test. */
    for (i = 0; i < source->ai_data->malp_count; i++) {
        struct malp_entry *e = &source->ai_data->malp[i];
        float score;

        if (e->agent_id == 0)
            continue;
        if (e->agent_id == listener_id && e->agent_type == ENTITY_TYPE_MOB)
            continue; /* don't gossip about the person you're talking to */

        score = e->intensity;

        /* ── Availability Bias (Tversky & Kahneman 1973; Rosnow & Fine 1976):
         * Mobs with higher availability_bias preferentially recall and share
         * recent / emotionally vivid memories.  Uses the same 24-hour recency
         * decay function as the Shadow Timeline module. */
        if (source->ai_data->biases.availability_bias > 0.0f && e->timestamp > 0) {
            float age_hours = (float)(now - e->timestamp) / 3600.0f;
            if (age_hours < 0.0f)
                age_hours = 0.0f;
            float recency = 1.0f / (1.0f + age_hours / 24.0f);
            score += source->ai_data->biases.availability_bias * recency * MALP_GOSSIP_AVAILABILITY_SELECT_SCALE *
                     e->intensity;
        }

        /* ── Negativity Bias — selection (Baumeister et al. 2001):
         * Negative memories feel more urgent to share; they occupy more
         * cognitive space and are retrieved preferentially. */
        if (source->ai_data->biases.negativity_bias > 0.0f && e->valence < 0.0f) {
            score += source->ai_data->biases.negativity_bias * (-e->valence) * MALP_GOSSIP_NEGATIVITY_SELECT_SCALE *
                     e->intensity;
        }

        /* ── Confirmation Bias — selection (Echterhoff & Higgins 2009):
         * Sources prefer sharing stories that reinforce their dominant attitude
         * about the target (audience-tuning / shared-reality effect).
         * We approximate this by checking whether the entry's valence aligns
         * with the source's overall MPLP disposition toward this entity.
         * The bonus is small — it only tips the scales when scores are close. */
        if (source->ai_data->biases.confirmation_bias > 0.0f) {
            float src_trust = mplp_get_effective_trait(source, NULL, MPLP_TRAIT_TRUST_BIAS, MPLP_CTX_SOCIAL);
            bool src_positive = (src_trust >= 0.5f); /* source's general social lean */
            bool entry_positive = (e->valence >= 0.0f);
            if (src_positive == entry_positive) {
                score +=
                    source->ai_data->biases.confirmation_bias * MALP_GOSSIP_CONFIRMATION_SELECT_SCALE * e->intensity;
            }
        }

        if (score > best_score) {
            best_score = score;
            best = e;
        }
    }

    if (!best)
        return FALSE; /* no eligible topic */

    long target_id = best->agent_id;
    int target_type = best->agent_type;

    /* ── Compute transfer weight ─────────────────────────────────────────────
     *
     *   transfer_weight = trust(B,A) * reputation_factor(A)
     *                     * intensity(A,C) * (1 − suspicion(B))
     *
     * trust(B,A):          listener's effective TRUST_BIAS toward source,
     *                      mapped from [−1,+1] → [0,1].
     * reputation_factor(A): source's global reputation / 100 ∈ [0,1].
     * intensity(A,C):       source MALP intensity for the topic entity.
     * suspicion(B):         listener's baseline SUSPICION_BIAS ∈ [0,1].
     */
    float eff_trust = mplp_get_effective_trait(listener, source, MPLP_TRAIT_TRUST_BIAS, MPLP_CTX_SOCIAL);
    float trust_factor = (eff_trust + 1.0f) / 2.0f; /* [−1,+1] → [0,1] */

    float rep_factor = (float)GET_REPUTATION(source) / 100.0f;

    float emotional_intensity = best->intensity;

    float suspicion = mplp_get_effective_trait(listener, NULL, MPLP_TRAIT_SUSPICION_BIAS, MPLP_CTX_SOCIAL);
    if (suspicion < 0.0f)
        suspicion = 0.0f;
    if (suspicion > 1.0f)
        suspicion = 1.0f;

    float transfer_weight = trust_factor * rep_factor * emotional_intensity * (1.0f - suspicion);

    if (transfer_weight > MALP_GOSSIP_WEIGHT_CAP)
        transfer_weight = MALP_GOSSIP_WEIGHT_CAP;
    if (transfer_weight < MALP_GOSSIP_WEIGHT_MIN)
        return FALSE; /* insufficient credibility / too suspicious */

    /* ── Gossip cooldown per listener–target pair ────────────────────────────
     * Reuse the MALP last_applied timestamp on the listener's existing entry
     * for the target.  This prevents the same gossip from flooding the
     * listener repeatedly before the memory can decay naturally. */
    struct mob_ai_data *lai = listener->ai_data;
    struct malp_entry *existing = NULL;
    for (i = 0; i < lai->malp_count; i++) {
        struct malp_entry *e = &lai->malp[i];
        if (e->agent_id == target_id && e->agent_type == target_type) {
            existing = e;
            break;
        }
    }

    if (existing && existing->last_applied > 0 && (now - existing->last_applied) < (time_t)CONFIG_MOB_GOSSIP_COOLDOWN)
        return FALSE; /* cooldown not elapsed for this listener–target pair */

    /* ── Update listener's MALP entry about the target (C) ──────────────────
     * Gossip attenuates both intensity and valence.  The update uses lambda =
     * transfer_weight (× listener receptivity scale) so that highly trusted
     * sources with belief-confirming content cause larger shifts.
     * When no entry exists for C, a new LOW-persistence entry is created with
     * reduced intensity (MALP_GOSSIP_INTENSITY_SCALE × transfer_weight). */
    float gossip_valence = best->valence;
    float gossip_intensity = best->intensity * MALP_GOSSIP_INTENSITY_SCALE * transfer_weight;

    /* ── Source Cognitive Biases: gossip ENCODING (Phase 2) ─────────────────
     *
     * The source distorts the transmitted valence through its cognitive biases
     * BEFORE delivery.  Raw MALP data is never modified — only the local
     * gossip_valence copy used for this transmission is adjusted.
     *
     * Attribution Bias (Ross 1977; Jones & Harris 1967):
     *   Mobs attribute others' bad behaviour to character, not circumstance.
     *   When retelling negative events the story becomes more extreme because
     *   the source frames the target as "just a bad entity by nature."
     *   Only negative valence is amplified (positive attributions are neutral). */
    if (source->ai_data->biases.attribution_bias > 0.0f && gossip_valence < 0.0f) {
        gossip_valence *= (1.0f + source->ai_data->biases.attribution_bias * MALP_GOSSIP_ATTRIBUTION_ENC_SCALE);
        if (gossip_valence < -1.0f)
            gossip_valence = -1.0f;
    }

    /* Negativity Bias — encoding (Baumeister et al. 2001):
     * Sources emphasise the negative emotional charge of bad memories when
     * retelling them, amplifying the negative valence in transmission. */
    if (source->ai_data->biases.negativity_bias > 0.0f && gossip_valence < 0.0f) {
        gossip_valence *= (1.0f + source->ai_data->biases.negativity_bias * MALP_GOSSIP_NEGATIVITY_ENC_SCALE);
        if (gossip_valence < -1.0f)
            gossip_valence = -1.0f;

        /* Negative gossip is also shared with slightly more emotional intensity. */
        gossip_intensity *= 1.0f + source->ai_data->biases.negativity_bias * MALP_GOSSIP_NEGATIVITY_INTENSITY_BOOST;
        if (gossip_intensity > 1.0f)
            gossip_intensity = 1.0f;
    }

    /* ── Listener Cognitive Biases: gossip RECEPTION (Phase 3) ──────────────
     *
     * lambda_scale multiplies transfer_weight inside each MALP/MPLP update
     * step.  It is always capped so that lambda never exceeds MALP_GOSSIP_WEIGHT_CAP,
     * preserving the "no overwrite of first-hand memory" invariant.
     *
     * Confirmation Bias (Echterhoff & Higgins 2009; Fiedler et al. 2004):
     *   Gossip that confirms the listener's existing belief about the target
     *   is accepted more readily (higher lambda); contradicting gossip is
     *   discounted (lower lambda).  Applies only when a prior entry exists. */
    float lambda_scale = 1.0f;

    if (listener->ai_data->biases.confirmation_bias > 0.0f && existing) {
        bool gossip_negative = (gossip_valence < 0.0f);
        bool prior_negative = (existing->valence < 0.0f);
        if (gossip_negative == prior_negative) {
            /* Confirms existing belief → boost receptivity */
            lambda_scale += listener->ai_data->biases.confirmation_bias * MALP_GOSSIP_CONFIRMATION_REC_SCALE;
        } else {
            /* Contradicts existing belief → dampen receptivity */
            float damp = listener->ai_data->biases.confirmation_bias * MALP_GOSSIP_CONFIRMATION_REC_SCALE * 0.5f;
            lambda_scale -= damp;
            if (lambda_scale < 0.1f)
                lambda_scale = 0.1f;
        }
    }

    /* Negativity Bias — reception (Baumeister et al. 2001; Rozin & Royzman 2001):
     * Listeners process negative information more deeply and assign it greater
     * evidential weight regardless of the source's credibility. */
    if (listener->ai_data->biases.negativity_bias > 0.0f && gossip_valence < 0.0f) {
        lambda_scale += listener->ai_data->biases.negativity_bias * MALP_GOSSIP_NEGATIVITY_REC_SCALE;
    }

    if (existing) {
        float lambda = transfer_weight * lambda_scale;
        if (lambda > MALP_GOSSIP_WEIGHT_CAP)
            lambda = MALP_GOSSIP_WEIGHT_CAP;
        existing->valence = (1.0f - lambda) * existing->valence + lambda * gossip_valence;
        if (existing->valence > 1.0f)
            existing->valence = 1.0f;
        if (existing->valence < -1.0f)
            existing->valence = -1.0f;
        existing->intensity += gossip_intensity;
        if (existing->intensity > 1.0f)
            existing->intensity = 1.0f;
        if (existing->rehearsal < MALP_MAX_REHEARSAL)
            existing->rehearsal++;
        existing->last_applied = now;
    } else {
        /* Create a new second-hand MALP entry for the listener about C */
        if (!malp_grow(listener)) {
            log1("GOSSIP: MALP allocation failure for listener %s — skipping gossip transfer", GET_NAME(listener));
            return FALSE;
        }
        struct malp_entry *gossip_entry = &lai->malp[lai->malp_count];
        memset(gossip_entry, 0, sizeof(struct malp_entry));
        gossip_entry->agent_id = target_id;
        gossip_entry->agent_type = target_type;
        gossip_entry->interaction_type = best->interaction_type;
        gossip_entry->major_event = 0; /* second-hand knowledge is never a major event */
        gossip_entry->timestamp = now;
        gossip_entry->last_retrieved = 0;
        gossip_entry->valence = gossip_valence;
        gossip_entry->arousal = best->arousal * transfer_weight;
        gossip_entry->salience = best->salience * transfer_weight;
        gossip_entry->intensity = gossip_intensity;
        gossip_entry->rehearsal = 1;
        gossip_entry->recon_ticks_left = 0;
        gossip_entry->persistence = MALP_PERSIST_LOW;
        gossip_entry->last_applied = now;
        lai->malp_count++;
        existing = gossip_entry; /* point to freshly created entry for MPLP step */
    }

    /* ── Update listener's agent-anchored MPLP toward the target (C) ────────
     * Gossip nudges the listener's approach/avoidance bias toward C.
     * Positive gossip → approach; negative gossip → avoidance.
     * Magnitude delta is small (transfer_weight × 0.10) to keep the
     * "no overwrite of base personality" invariant. */
    struct mplp_trait *approach_trait = NULL;
    for (i = 0; i < lai->mplp_count; i++) {
        struct mplp_trait *t = &lai->mplp[i];
        if (t->anchor_agent_id == target_id && t->agent_type == target_type &&
            (t->trait_type == MPLP_TRAIT_APPROACH || t->trait_type == MPLP_TRAIT_AVOIDANCE)) {
            approach_trait = t;
            break;
        }
    }

    float approach_delta = transfer_weight * 0.10f;

    if (approach_trait) {
        approach_trait->magnitude += approach_delta;
        if (approach_trait->magnitude > 1.0f)
            approach_trait->magnitude = 1.0f;
        approach_trait->base_magnitude = approach_trait->magnitude;
        approach_trait->valence = (1.0f - MALP_GOSSIP_VALENCE_BLEND_RATE) * approach_trait->valence +
                                  MALP_GOSSIP_VALENCE_BLEND_RATE * gossip_valence;
        approach_trait->trait_type = (approach_trait->valence >= 0.0f) ? MPLP_TRAIT_APPROACH : MPLP_TRAIT_AVOIDANCE;
        approach_trait->last_updated = now;
    } else if (lai->mplp_count < MPLP_MAX_PER_MOB && mplp_grow(listener)) {
        struct mplp_trait *gossip_trait = &lai->mplp[lai->mplp_count];
        memset(gossip_trait, 0, sizeof(struct mplp_trait));
        gossip_trait->anchor_agent_id = target_id;
        gossip_trait->agent_type = target_type;
        gossip_trait->trait_type = (gossip_valence >= 0.0f) ? MPLP_TRAIT_APPROACH : MPLP_TRAIT_AVOIDANCE;
        gossip_trait->magnitude = approach_delta;
        gossip_trait->base_magnitude = approach_delta;
        gossip_trait->valence = gossip_valence;
        gossip_trait->rehearsal_count = 1;
        gossip_trait->persistence = MALP_PERSIST_LOW;
        gossip_trait->last_updated = now;
        lai->mplp_count++;
    }

    /* ── Reinforce listener's context-global MPLP traits ────────────────────
     * Gossip shapes the listener's general social personality over time.
     * Positive gossip strengthens TRUST_BIAS and NOVEL_AGENT_INTEREST.
     * Negative gossip strengthens SUSPICION_BIAS and OUTGROUP_AVERSION.
     * All increments use the context-aware variant (MPLP_CTX_SOCIAL) so the
     * effect is anchored in the social context without polluting other contexts.
     * lambda_scale is applied so that cognitively receptive listeners are also
     * shaped more by the gossip at the personality level. */
    float gossip_salience = best->salience * transfer_weight * lambda_scale;
    if (gossip_salience > 1.0f)
        gossip_salience = 1.0f;
    if (gossip_valence >= 0.0f) {
        reinforce_mplp_context_trait_ctx(listener, MPLP_TRAIT_TRUST_BIAS, gossip_valence, gossip_salience,
                                         MPLP_CTX_SOCIAL);
        reinforce_mplp_context_trait_ctx(listener, MPLP_TRAIT_NOVEL_AGENT_INTEREST, gossip_valence,
                                         gossip_salience * 0.5f, MPLP_CTX_SOCIAL);
    } else {
        reinforce_mplp_context_trait_ctx(listener, MPLP_TRAIT_SUSPICION_BIAS, -gossip_valence, gossip_salience,
                                         MPLP_CTX_SOCIAL);
        reinforce_mplp_context_trait_ctx(listener, MPLP_TRAIT_OUTGROUP_AVERSION, -gossip_valence,
                                         gossip_salience * 0.5f, MPLP_CTX_SOCIAL);
    }

    if (CONFIG_MOB_4D_DEBUG)
        log1(
            "GOSSIP: source=%s listener=%s target_id=%ld target_type=%d "
            "raw_valence=%.2f enc_valence=%.2f weight=%.2f lambda_scale=%.2f intensity=%.2f",
            GET_NAME(source), GET_NAME(listener), target_id, target_type, best->valence, gossip_valence,
            transfer_weight, lambda_scale, gossip_intensity);

    return TRUE;
}
