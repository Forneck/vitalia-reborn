/**
 * @file sec.c
 * SEC – Sistema de Emoções Concorrentes (Competing Emotions System)
 *
 * Implements the internal emotional inference layer for mobs, driven by
 * post-hysteresis 4D results.  See sec.h for full design documentation.
 *
 * Arousal partition guarantee: fear + anger + happiness = A ∈ [0, 1].
 * Helplessness is an exponentially smoothed projection of (1 − Dominance).
 * Passive decay blends all projections toward the personality baseline
 * when Arousal is below SEC_AROUSAL_EPSILON.
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "sec.h"

/* ── Tuning constants ────────────────────────────────────────────────────── */

/** Passive-decay activates when normalised arousal < ε. */
#define SEC_AROUSAL_EPSILON 0.10f

/** Passive-decay rate λ: blend speed toward baseline per tick. */
#define SEC_DECAY_LAMBDA 0.05f

/** Helplessness exponential-smoothing rate α. */
#define SEC_HELPLESSNESS_ALPHA 0.30f

/* ── Internal helpers ────────────────────────────────────────────────────── */

/** Clamp f to [lo, hi]. */
static float sec_clamp(float f, float lo, float hi) { return f < lo ? lo : (f > hi ? hi : f); }

/* ── Baseline table ──────────────────────────────────────────────────────── */

/*
 * Default personality baselines per emotional profile.
 * Each row: { fear_base, anger_base, happiness_base }.
 * Columns are intentionally kept distinct so the passive-decay convergence
 * reflects the emotional colour of each profile.
 */
/* clang-format off */
static const float sec_profile_baselines[EMOTION_PROFILE_NUM][3] = {
    /* 0 NEUTRAL    */ { 0.20f, 0.20f, 0.30f },
    /* 1 AGGRESSIVE */ { 0.10f, 0.60f, 0.15f },
    /* 2 COWARDLY   */ { 0.60f, 0.10f, 0.10f },
    /* 3 BALANCED   */ { 0.25f, 0.25f, 0.35f },
    /* 4 SENSITIVE  */ { 0.35f, 0.15f, 0.40f },
    /* 5 CONFIDENT  */ { 0.10f, 0.30f, 0.45f },
    /* 6 GREEDY     */ { 0.20f, 0.40f, 0.20f },
    /* 7 LOYAL      */ { 0.15f, 0.20f, 0.50f },
};
/* clang-format on */

/* ── Public API ──────────────────────────────────────────────────────────── */

void sec_init(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    struct mob_ai_data *ai = mob->ai_data;

    /* Zero the live state. */
    ai->sec.fear = 0.0f;
    ai->sec.anger = 0.0f;
    ai->sec.happiness = 0.0f;
    ai->sec.helplessness = 0.0f;
    /* Disgust is a persistent trait seeded from the emotion vector. */
    ai->sec.disgust = sec_clamp(ai->emotion_disgust / 100.0f, 0.0f, 1.0f);

    /* Set baseline from emotional profile. */
    int profile = ai->emotional_profile;
    if (profile < 0 || profile >= EMOTION_PROFILE_NUM)
        profile = EMOTION_PROFILE_NEUTRAL;

    ai->sec_base.fear_base = sec_profile_baselines[profile][0];
    ai->sec_base.anger_base = sec_profile_baselines[profile][1];
    ai->sec_base.happiness_base = sec_profile_baselines[profile][2];
}

void sec_update(struct char_data *mob, const struct emotion_4d_state *r)
{
    if (!IS_NPC(mob) || !mob->ai_data || !r || !r->valid)
        return;

    struct mob_ai_data *ai = mob->ai_data;

    /* Normalise 4D axes to [0, 1]. */
    float A = sec_clamp(r->arousal / 100.0f, 0.0f, 1.0f);
    float D = sec_clamp((r->dominance + 100.0f) / 200.0f, 0.0f, 1.0f);
    float V = sec_clamp((r->valence + 100.0f) / 200.0f, 0.0f, 1.0f);

    /* Arousal partition weights. */
    float w_fear = A * (1.0f - D);
    float w_anger = A * D * (1.0f - V);
    float w_happy = A * V * D;
    float total = w_fear + w_anger + w_happy;

    /*
     * Normalise against total weight and scale by A so that
     * fear + anger + happiness = A ∈ [0, 1] is always guaranteed.
     * When total == 0 (A == 0) all projections are zero.
     */
    if (total > 0.0f) {
        ai->sec.fear = sec_clamp(A * (w_fear / total), 0.0f, 1.0f);
        ai->sec.anger = sec_clamp(A * (w_anger / total), 0.0f, 1.0f);
        ai->sec.happiness = sec_clamp(A * (w_happy / total), 0.0f, 1.0f);
    } else {
        ai->sec.fear = 0.0f;
        ai->sec.anger = 0.0f;
        ai->sec.happiness = 0.0f;
    }

    /* Helplessness: exponential smoothing of (1 − D). */
    float h_target = 1.0f - D;
    ai->sec.helplessness = ai->sec.helplessness * (1.0f - SEC_HELPLESSNESS_ALPHA) + h_target * SEC_HELPLESSNESS_ALPHA;
    ai->sec.helplessness = sec_clamp(ai->sec.helplessness, 0.0f, 1.0f);

    /* Disgust is a persistent trait; refresh from the emotion vector each tick. */
    ai->sec.disgust = sec_clamp(ai->emotion_disgust / 100.0f, 0.0f, 1.0f);
}

void sec_passive_decay(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    struct mob_ai_data *ai = mob->ai_data;

    /* Guard: only decay when the mob is calm (low arousal). */
    float A = sec_clamp(ai->last_4d_state.arousal / 100.0f, 0.0f, 1.0f);
    if (A >= SEC_AROUSAL_EPSILON)
        return;

    float lm = SEC_DECAY_LAMBDA;

    ai->sec.fear = ai->sec.fear * (1.0f - lm) + ai->sec_base.fear_base * lm;
    ai->sec.anger = ai->sec.anger * (1.0f - lm) + ai->sec_base.anger_base * lm;
    ai->sec.happiness = ai->sec.happiness * (1.0f - lm) + ai->sec_base.happiness_base * lm;
    /* Helplessness decays toward 0 (no external resting value). */
    ai->sec.helplessness = ai->sec.helplessness * (1.0f - lm);
}

/* ── Read-only getters ───────────────────────────────────────────────────── */

float sec_get_4d_modifier(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 1.0f;

    const struct sec_state *s = &mob->ai_data->sec;

    /*
     * Anger boosts behavioural intensity; fear and helplessness reduce it.
     * Modifier ∈ [0.5, 2.0] so it never fully suppresses nor doubles effort.
     */
    float mod = 1.0f + (s->anger - s->fear - s->helplessness * 0.5f) * 0.5f;
    return sec_clamp(mod, 0.5f, 2.0f);
}

float sec_get_flee_bias(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 0.0f;

    const struct sec_state *s = &mob->ai_data->sec;

    float bias = (s->fear + s->helplessness) * 0.5f;
    return sec_clamp(bias, 0.0f, 1.0f);
}

float sec_get_target_bias(struct char_data *mob, struct char_data *target)
{
    (void)target; /* Reserved for future per-target memory modulation. */

    if (!IS_NPC(mob) || !mob->ai_data)
        return 1.0f;

    const struct sec_state *s = &mob->ai_data->sec;

    /* Anger shifts selection weight above neutral (0.5 base + anger ∈ [0,1]). */
    float bias = 0.5f + s->anger * 1.0f;
    return sec_clamp(bias, 0.5f, 1.5f);
}
