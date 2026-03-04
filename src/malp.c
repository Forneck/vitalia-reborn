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
    /* Sum range: 0..700 each; normalise to −1..+1 */
    float v = (float)(pos - neg) / 700.0f;
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
    float exponent = (float)M_LN2 / logf(1.0f + (float)half_life_hours);
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
            e->intensity = powerlaw_intensity(e->timestamp, hl);
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
            t->magnitude = powerlaw_intensity(t->last_updated, hl);
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

    /* Process passive memory buffer */
    for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        struct emotion_memory *mem = &ai->memories[i];
        if (mem->timestamp == 0 || mem->entity_id == 0)
            continue;

        float age_hours = (float)(now - mem->timestamp) / 3600.0f;
        float arousal = slot_arousal(mem);
        float social_w = (mem->entity_type == ENTITY_TYPE_PLAYER) ? MALP_SOCIAL_WEIGHT_PLAYER : MALP_SOCIAL_WEIGHT_MOB;
        int rehearsal = count_rehearsal(mob, mem->entity_id, mem->entity_type);
        float S = compute_salience(arousal, rehearsal, social_w, age_hours);

        if (S < theta)
            continue;

        /* ── Consolidate into MALP ─────────────────────────────────────── */
        float valence = slot_valence(mem);

        /* Find existing MALP entry for this agent */
        struct malp_entry *existing = NULL;
        int j;
        for (j = 0; j < ai->malp_count; j++) {
            if (ai->malp[j].agent_id == mem->entity_id && ai->malp[j].agent_type == mem->entity_type) {
                existing = &ai->malp[j];
                break;
            }
        }

        if (existing) {
            /* Update existing MALP entry (incremental mixing) */
            float lambda = 0.3f * S; /* λ proportional to salience */
            if (lambda > 0.5f)
                lambda = 0.5f;
            existing->valence = (1.0f - lambda) * existing->valence + lambda * valence;
            existing->arousal = (1.0f - lambda) * existing->arousal + lambda * arousal;
            existing->salience = S; /* refresh to current */
            existing->rehearsal = rehearsal;
            /* Upgrade persistence if warranted */
            if (mem->major_event || arousal >= MALP_HIGH_PERSIST_AROUSAL) {
                existing->persistence = MALP_PERSIST_HIGH;
                existing->major_event = 1;
            }
        } else {
            /* Create new MALP entry */
            if (!malp_grow(mob)) {
                log1("MALP: memory allocation failure for mob %s (current count: %d)", GET_NAME(mob),
                     mob->ai_data->malp_count);
                return;
            }
            struct malp_entry *e = &ai->malp[ai->malp_count];
            memset(e, 0, sizeof(struct malp_entry));
            e->agent_id = mem->entity_id;
            e->agent_type = mem->entity_type;
            e->interaction_type = mem->interaction_type;
            e->major_event = mem->major_event;
            e->timestamp = mem->timestamp;
            e->last_retrieved = 0;
            e->valence = valence;
            e->arousal = arousal;
            e->salience = S;
            e->intensity = S; /* initial intensity equals salience */
            e->rehearsal = rehearsal;
            e->recon_ticks_left = 0;
            if (mem->major_event || arousal >= MALP_HIGH_PERSIST_AROUSAL)
                e->persistence = MALP_PERSIST_HIGH;
            else if (S >= 0.80f)
                e->persistence = MALP_PERSIST_MEDIUM;
            else
                e->persistence = MALP_PERSIST_LOW;
            ai->malp_count++;
        }

        /* ── Generate MPLP trait (Hebbian co-occurrence) ──────────────── */
        if (rehearsal >= rehearsal_threshold && mem->entity_id != 0) {
            /* Find or create MPLP trait for this agent */
            struct mplp_trait *trait = NULL;
            int trait_type = (valence >= 0.0f) ? MPLP_TRAIT_APPROACH : MPLP_TRAIT_AVOIDANCE;

            for (j = 0; j < ai->mplp_count; j++) {
                if (ai->mplp[j].anchor_agent_id == mem->entity_id && ai->mplp[j].agent_type == mem->entity_type) {
                    trait = &ai->mplp[j];
                    break;
                }
            }

            if (!trait && ai->mplp_count < MPLP_MAX_PER_MOB) {
                if (!mplp_grow(mob)) {
                    log1("MPLP: memory allocation failure for mob %s (current count: %d)", GET_NAME(mob),
                         mob->ai_data->mplp_count);
                    continue;
                }
                trait = &ai->mplp[ai->mplp_count];
                memset(trait, 0, sizeof(struct mplp_trait));
                trait->anchor_agent_id = mem->entity_id;
                trait->agent_type = mem->entity_type;
                trait->trait_type = trait_type;
                trait->magnitude = 0.0f;
                trait->valence = valence;
                trait->rehearsal_count = rehearsal;
                trait->persistence = MALP_PERSIST_LOW;
                trait->last_updated = now;
                ai->mplp_count++;
            } else if (trait) {
                /* Update existing trait: Hebbian increment proportional to S */
                float delta = 0.15f * S;
                if (delta > 0.3f)
                    delta = 0.3f;
                trait->magnitude += delta;
                if (trait->magnitude > 1.0f)
                    trait->magnitude = 1.0f;
                /* Running-average valence */
                float alpha_t = 0.2f;
                trait->valence = (1.0f - alpha_t) * trait->valence + alpha_t * valence;
                trait->rehearsal_count = rehearsal;
                trait->last_updated = now;
                /* Upgrade trait type if valence shifted significantly */
                trait->trait_type = (trait->valence >= 0.0f) ? MPLP_TRAIT_APPROACH : MPLP_TRAIT_AVOIDANCE;
                /* Persistence upgrade for high-arousal negative traits */
                if (arousal >= MALP_HIGH_PERSIST_AROUSAL && trait->valence < 0.0f)
                    trait->persistence = MALP_PERSIST_HIGH;
                else if (S >= 0.80f && trait->persistence < MALP_PERSIST_MEDIUM)
                    trait->persistence = MALP_PERSIST_MEDIUM;
                /* Add arousal-bias sub-trait if arousal is consistently high */
                if (arousal >= 0.70f) {
                    bool found_arousal = FALSE;
                    for (j = 0; j < ai->mplp_count; j++) {
                        if (ai->mplp[j].anchor_agent_id == mem->entity_id &&
                            ai->mplp[j].agent_type == mem->entity_type &&
                            ai->mplp[j].trait_type == MPLP_TRAIT_AROUSAL_BIAS) {
                            ai->mplp[j].magnitude += 0.05f * S;
                            if (ai->mplp[j].magnitude > 1.0f)
                                ai->mplp[j].magnitude = 1.0f;
                            ai->mplp[j].last_updated = now;
                            found_arousal = TRUE;
                            break;
                        }
                    }
                    if (!found_arousal && ai->mplp_count < MPLP_MAX_PER_MOB) {
                        if (mplp_grow(mob)) {
                            struct mplp_trait *ab = &ai->mplp[ai->mplp_count];
                            memset(ab, 0, sizeof(struct mplp_trait));
                            ab->anchor_agent_id = mem->entity_id;
                            ab->agent_type = mem->entity_type;
                            ab->trait_type = MPLP_TRAIT_AROUSAL_BIAS;
                            ab->magnitude = 0.10f * S;
                            ab->valence = valence;
                            ab->rehearsal_count = rehearsal;
                            ab->persistence = MALP_PERSIST_LOW;
                            ab->last_updated = now;
                            ai->mplp_count++;
                        }
                    }
                }
            }
        }
    }

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

    /* Compute retrieval probability P_ret = sigmoid(k * (intensity + cue_match)) */
    float cue_score = best->intensity; /* simplified: use intensity as P_ret proxy */
    float P_ret = 1.0f / (1.0f + expf(-MALP_PRET_K * (cue_score - 0.5f)));

    /* Open reconsolidation window if P_ret >= threshold */
    if (P_ret >= MALP_THETA_REACT && best->recon_ticks_left <= 0) {
        int window = CONFIG_MALP_RECON_WINDOW_TICKS;
        if (window < 1)
            window = 60;
        best->recon_ticks_left = window;
        best->last_retrieved = time(NULL);
        best->rehearsal++;
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

void apply_malp_emotion_effects(struct char_data *mob, struct char_data *actor)
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
