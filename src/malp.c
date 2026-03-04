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
            /* Apply decay relative to current magnitude: multiply by the power-law
             * shape factor derived from last_updated.  This preserves relative trait
             * strength between older and newer traits rather than overwriting magnitude
             * with a time-only curve that always starts at 1.0 after an update. */
            {
                float decay_factor = powerlaw_intensity(t->last_updated, hl);
                if (decay_factor > 1.0f)
                    decay_factor = 1.0f;
                t->magnitude *= decay_factor;
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
        float S = compute_salience(arousal, rehearsal, social_w, age_hours);                                           \
        if (S < theta)                                                                                                 \
            break;                                                                                                     \
        float valence = slot_valence(mem);                                                                             \
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
            _e->interaction_type = (mem)->interaction_type;                                                            \
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

    if (malp) {
        float intensity = malp->intensity;
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
    }

    /* Apply MPLP approach/avoidance modifier */
    float approach = get_mplp_approach_modifier(mob, agent_id, agent_type);
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

    /* Apply MPLP arousal bias */
    float arousal_bias = get_mplp_arousal_bias(mob, agent_id, agent_type);
    if (arousal_bias > 0.15f) {
        int arb_delta = (int)(arousal_bias * (float)MPLP_EMOTION_DELTA_MAX);
        if (arb_delta > 0)
            adjust_emotion(mob, &mob->ai_data->emotion_excitement, arb_delta);
    }
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

        e->last_retrieved = time(NULL);
        /* Consuming the update closes the window for this cycle */
        e->recon_ticks_left = 0;
        break;
    }
}
