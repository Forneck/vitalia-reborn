/**
 * @file sec.c
 * SEC – Sistema de Emoções Concorrentes (Competing Emotions System)
 *
 * Implements the four-timescale internal emotional inference layer for mobs,
 * driven by post-hysteresis 4D results.  See sec.h for full design documentation.
 *
 * Timescales:
 *   Fast      – Arousal partition computed from 4D axes every tick.
 *   Medium    – Emotional smoothing (α) applied to fear/anger/happiness and
 *               helplessness, providing behavioral continuity.
 *   Slow      – Passive decay (λ) toward personality baseline when A < ε.
 *   Very slow – Persistent trait update (δ) for Disgust.
 *
 * Partition guarantee: fear_target + anger_target + happiness_target = A ∈ [0,1].
 * After α smoothing the smoothed values may temporarily deviate from A while
 * converging — the bound [0,1] is always enforced by clamp.
 * Disgust is structurally independent of the Arousal partition.
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
#define SEC_AROUSAL_EPSILON 0.05f

/** Passive-decay rate λ (slow timescale): blend speed toward baseline. */
#define SEC_DECAY_LAMBDA 0.05f

/** Emotional-smoothing rate α (medium timescale): applied each tick to
 *  fear, anger, and happiness projections for behavioral continuity. */
#define SEC_EMOTION_ALPHA 0.40f

/** Helplessness exponential-smoothing rate (medium-fast timescale). */
#define SEC_HELPLESSNESS_ALPHA 0.30f

/** Persistent-trait update rate δ (very slow timescale): used for Disgust. */
#define SEC_DISGUST_DELTA 0.01f

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

    /* Zero the volatile state. */
    ai->sec.fear = 0.0f;
    ai->sec.anger = 0.0f;
    ai->sec.happiness = 0.0f;
    ai->sec.helplessness = 0.0f;
    /* Disgust: persistent trait seeded from the emotion vector on spawn.
     * Subsequent updates use the slow δ update rule, not direct mirroring. */
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

    /* ── Fast timescale: Arousal partition ─────────────────────────────── */

    /* A: use the post-hysteresis 4D arousal — it is contextually deformed by
     * environment, combat intensity, and the helplessness step, so it better
     * reflects the mob's actual activation level than a raw emotion average.
     * D and V also come from the same post-hysteresis 4D state. */
    float A = sec_clamp(r->arousal / 100.0f, 0.0f, 1.0f);
    float D = sec_clamp((r->dominance + 100.0f) / 200.0f, 0.0f, 1.0f);
    float V = sec_clamp((r->valence + 100.0f) / 200.0f, 0.0f, 1.0f);

    /* Partition weights — guarantee: w_fear+w_anger+w_happy = A. */
    float w_fear = A * (1.0f - D);
    float w_anger = A * D * (1.0f - V);
    float w_happy = A * V * D;
    float total = w_fear + w_anger + w_happy;

    float t_fear, t_anger, t_happy;
    if (total > 0.0f) {
        t_fear = sec_clamp(A * (w_fear / total), 0.0f, 1.0f);
        t_anger = sec_clamp(A * (w_anger / total), 0.0f, 1.0f);
        t_happy = sec_clamp(A * (w_happy / total), 0.0f, 1.0f);
    } else {
        t_fear = t_anger = t_happy = 0.0f;
    }

    /* ── Medium timescale: Emotional smoothing (α) ──────────────────────── */

    float a = SEC_EMOTION_ALPHA;
    ai->sec.fear = sec_clamp(ai->sec.fear * (1.0f - a) + t_fear * a, 0.0f, 1.0f);
    ai->sec.anger = sec_clamp(ai->sec.anger * (1.0f - a) + t_anger * a, 0.0f, 1.0f);
    ai->sec.happiness = sec_clamp(ai->sec.happiness * (1.0f - a) + t_happy * a, 0.0f, 1.0f);

    /* Helplessness: medium timescale smoothing of (1 − D). */
    float h_target = 1.0f - D;
    ai->sec.helplessness = sec_clamp(
        ai->sec.helplessness * (1.0f - SEC_HELPLESSNESS_ALPHA) + h_target * SEC_HELPLESSNESS_ALPHA, 0.0f, 1.0f);

    /* ── Very slow timescale: Persistent trait — Disgust (δ) ────────────── */

    /* Disgust does not compete for Arousal.  It slowly converges toward the
     * current emotion_disgust value using a very small δ, accumulating
     * structural memory across events without volatile oscillation. */
    float disgust_event = sec_clamp(ai->emotion_disgust / 100.0f, 0.0f, 1.0f);
    ai->sec.disgust =
        sec_clamp(ai->sec.disgust * (1.0f - SEC_DISGUST_DELTA) + disgust_event * SEC_DISGUST_DELTA, 0.0f, 1.0f);
}

void sec_passive_decay(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    struct mob_ai_data *ai = mob->ai_data;

    /* Guard: only decay when the mob is calm (low arousal).
     * Uses the same post-hysteresis 4D arousal as sec_update(). */
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
