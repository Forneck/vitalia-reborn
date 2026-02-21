/**
 * @file sec.c
 * SEC – Sistema de Emoções Concorrentes (Competing Emotions System)
 *
 * Implements the four-timescale internal emotional inference layer for mobs,
 * driven by post-hysteresis 4D results.  See sec.h for full design documentation.
 *
 * Timescales:
 *   Fast      – Arousal partition computed from 4D axes every tick.
 *   Medium    – Emotional smoothing (α) applied to fear/sadness/anger/happiness and
 *               helplessness, providing behavioral continuity.
 *   Slow      – Passive decay (λ) toward personality baseline when A < ε.
 *   Very slow – Persistent trait update (δ) for Disgust.
 *
 * Partition guarantee: fear_target + sadness_target + anger_target + happiness_target = A ∈ [0,1].
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
#include "db.h"
#include "sec.h"

/* ── Tuning constants ────────────────────────────────────────────────────── */

/* SEC_AROUSAL_EPSILON is declared in sec.h so external consumers (e.g. mobact.c)
 * can use the same threshold without an additional header. */

/** Passive-decay rate λ (slow timescale): blend speed toward baseline. */
#define SEC_DECAY_LAMBDA 0.05f

/** Emotional-smoothing rate α (medium timescale): applied each tick to
 *  fear, sadness, anger, and happiness projections for behavioral continuity. */
#define SEC_EMOTION_ALPHA 0.40f

/** Helplessness exponential-smoothing rate (medium-fast timescale). */
#define SEC_HELPLESSNESS_ALPHA 0.30f

/** Persistent-trait update rate δ (very slow timescale): used for Disgust. */
#define SEC_DISGUST_DELTA 0.01f

/* ── Internal helpers ────────────────────────────────────────────────────── */

/** Clamp f to [lo, hi]. */
static float sec_clamp(float f, float lo, float hi) { return f < lo ? lo : (f > hi ? hi : f); }

/**
 * Apply α-smoothing to fear/sadness/anger/happiness targets and re-normalise so that
 * their sum equals A (Arousal).  This is the energy-conservation step that
 * prevents emotional "state leakage".
 *
 * Algorithm:
 *   1. Exponential smoothing with SEC_EMOTION_ALPHA.
 *   2. Compute W_total = new_fear + new_sad + new_anger + new_happy.
 *   3. Renormalise: each weight *= A / W_total  (W_total is the divisor).
 *
 * Guarantee: after this call, sec.fear + sec.sadness + sec.anger + sec.happiness = A.
 * Lateral inhibition emerges naturally: when the Sadness partition grows (low D + low V),
 * the Anger and Happiness partitions shrink to keep the sum constant.
 */
static void sec_update_partitioned(struct mob_ai_data *ai, float A, float t_fear, float t_sad, float t_anger,
                                   float t_happy, float C_final)
{
    /* Conscientiousness alpha regulation: high C resists sudden emotional spikes.
     * alpha_effective = SEC_EMOTION_ALPHA * (SEC_C_ALPHA_SCALE_MAX - C_final * SEC_C_ALPHA_SCALE_RANGE)
     * At C=0: alpha * 1.20 (responsive).
     * At C=1: alpha * 0.70 (resistant).
     * Clamped to [SEC_C_ALPHA_MIN, SEC_C_ALPHA_MAX] to prevent freeze-state (α→0) or
     * instability (α→1).  This modifies timing, never the energy sum. */
    float a = ((float)CONFIG_SEC_EMOTION_ALPHA / 100.0f) * (SEC_C_ALPHA_SCALE_MAX - C_final * SEC_C_ALPHA_SCALE_RANGE);
    a = sec_clamp(a, SEC_C_ALPHA_MIN, SEC_C_ALPHA_MAX);

    /* Step 1: exponential smoothing toward partition targets. */
    float new_fear = ai->sec.fear * (1.0f - a) + t_fear * a;
    float new_sad = ai->sec.sadness * (1.0f - a) + t_sad * a;
    float new_anger = ai->sec.anger * (1.0f - a) + t_anger * a;
    float new_happy = ai->sec.happiness * (1.0f - a) + t_happy * a;

    /* Step 2: compute W_total — the sum of smoothed weights before normalisation. */
    float W_total = new_fear + new_sad + new_anger + new_happy;
    float scale = 0.0f;

    /* Step 3: renormalise to Arousal. */
    if (W_total > SEC_AROUSAL_EPSILON) {
        scale = A / W_total;
        new_fear *= scale;
        new_sad *= scale;
        new_anger *= scale;
        new_happy *= scale;
    }

    if (CONFIG_MOB_4D_DEBUG)
        log1("SEC-PARTITION: W_total=%.4f A=%.4f scale=%.4f fear=%.4f sad=%.4f anger=%.4f happy=%.4f alpha=%.3f C=%.2f",
             W_total, A, scale, new_fear, new_sad, new_anger, new_happy, a, C_final);

    ai->sec.fear = sec_clamp(new_fear, 0.0f, 1.0f);
    ai->sec.sadness = sec_clamp(new_sad, 0.0f, 1.0f);
    ai->sec.anger = sec_clamp(new_anger, 0.0f, 1.0f);
    ai->sec.happiness = sec_clamp(new_happy, 0.0f, 1.0f);
}

/* ── Baseline table ──────────────────────────────────────────────────────── */

/*
 * Default personality baselines per emotional profile.
 * Each row: { fear_base, anger_base, happiness_base, sadness_base }.
 * Columns are intentionally kept distinct so the passive-decay convergence
 * reflects the emotional colour of each profile.
 */
/* clang-format off */
static const float sec_profile_baselines[EMOTION_PROFILE_NUM][4] = {
    /* 0 NEUTRAL    */ { 0.20f, 0.20f, 0.30f, 0.10f },
    /* 1 AGGRESSIVE */ { 0.10f, 0.60f, 0.15f, 0.05f },
    /* 2 DEFENSIVE  */ { 0.60f, 0.10f, 0.10f, 0.15f },
    /* 3 BALANCED   */ { 0.25f, 0.25f, 0.35f, 0.10f },
    /* 4 SENSITIVE  */ { 0.35f, 0.15f, 0.35f, 0.15f },
    /* 5 CONFIDENT  */ { 0.10f, 0.30f, 0.45f, 0.05f },
    /* 6 GREEDY     */ { 0.20f, 0.40f, 0.20f, 0.10f },
    /* 7 LOYAL      */ { 0.15f, 0.20f, 0.50f, 0.15f },
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
    ai->sec.sadness = 0.0f;
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
    ai->sec_base.sadness_base = sec_profile_baselines[profile][3];
}

/* ── OCEAN Phase 2: Conscientiousness final value getter ─────────────────── */

float sec_get_conscientiousness_final(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 0.5f;

    const struct mob_personality *p = &mob->ai_data->personality;
    const struct sec_state *s = &mob->ai_data->sec;

    float base = p->conscientiousness;
    float builder_mod = (float)p->conscientiousness_modifier / 100.0f;

    /* C_mod: persistent disgust slightly erodes self-discipline.
     * Cap is half that of A/E (±0.05) — C is the most structurally stable trait.
     * Disgust accumulates slowly (SEC_DISGUST_DELTA = 0.01) so this modulation
     * can only invert across many ticks of sustained aversion. */
#define SEC_C_MOD_CAP 0.05f
    float c_mod = sec_clamp(-s->disgust * 0.05f, -SEC_C_MOD_CAP, SEC_C_MOD_CAP);

    return sec_clamp(base + builder_mod + c_mod, 0.0f, 1.0f);
}

void sec_update(struct char_data *mob, const struct emotion_4d_state *r)
{
    if (!IS_NPC(mob) || !mob->ai_data || !r || !r->valid)
        return;

    struct mob_ai_data *ai = mob->ai_data;

    /* ── Fast timescale: Arousal partition ─────────────────────────────── */

    /* A: use the pre-modulation raw 4D arousal as the SEC partition budget.
     * The contextual Arousal multiplier (environment, combat intensity) inflates
     * the effective arousal artificially — using it would make A ≈ 1.0 in almost
     * every combat tick, collapsing the budget to its ceiling and making Lateral
     * Inhibition mathematically impossible (multiple emotions could saturate their
     * partition simultaneously).  The raw projection, before the multiplier, gives
     * a budget that is proportional to the mob's intrinsic emotional state so that
     * the dominant emotion only fills the partition when the underlying emotions
     * genuinely warrant it.
     * D and V still come from the post-contextual effective axes (dominance and
     * valence are legitimately context-deformed). */
    float A = sec_clamp(r->raw_arousal / 100.0f, 0.0f, 1.0f);
    float D = sec_clamp((r->dominance + 100.0f) / 200.0f, 0.0f, 1.0f);
    float V = sec_clamp((r->valence + 100.0f) / 200.0f, 0.0f, 1.0f);

    /* Partition weights — tetradic 4-way split in D×V space.
     * Guarantee: w_fear + w_sad + w_anger + w_happy = A.
     *
     *   w_fear    = A * (1-D) * V       — low dominance, high valence  (threat/uncertainty)
     *   w_sadness = A * (1-D) * (1-V)   — low dominance, low valence   (passive loss / grief)
     *   w_anger   = A * D    * (1-V)    — high dominance, low valence  (active loss / fight)
     *   w_happy   = A * D    * V        — high dominance, high valence  (approach / reward)
     *
     * Sadness rises when Goal Incongruence (low V) AND low Coping Potential (low D) combine,
     * creating the "passive loss" state described in the SEC design specification.
     * High Sadness mathematically suppresses Anger and Happiness via renormalisation.
     * Helplessness and Disgust are updated separately outside this partition. */
    float w_fear = A * (1.0f - D) * V;
    float w_sad = A * (1.0f - D) * (1.0f - V);
    float w_anger = A * D * (1.0f - V);
    float w_happy = A * D * V;

    float t_fear = sec_clamp(w_fear, 0.0f, 1.0f);
    float t_sad = sec_clamp(w_sad, 0.0f, 1.0f);
    float t_anger = sec_clamp(w_anger, 0.0f, 1.0f);
    float t_happy = sec_clamp(w_happy, 0.0f, 1.0f);

    /* ── Medium timescale: Emotional smoothing (α) with energy conservation ─ */

    /* sec_update_partitioned() applies α-smoothing then renormalises the three
     * emotions to sum to A, enforcing the Arousal partition invariant even
     * across tick boundaries.  Lateral inhibition (Anger ∝ D, Fear ∝ 1−D)
     * emerges automatically from the renormalisation step.
     * C_final is passed so Conscientiousness can regulate the α rate; the
     * clamped effective α and C_final are logged inside sec_update_partitioned()
     * when CONFIG_MOB_4D_DEBUG is on. */
    float C_final = sec_get_conscientiousness_final(mob);
    sec_update_partitioned(ai, A, t_fear, t_sad, t_anger, t_happy, C_final);

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

    /* Guard: only decay when the mob is calm (low raw arousal).
     * Uses the pre-modulation raw_arousal so that the contextual Arousal
     * multiplier (combat, crowd density) does not suppress decay when the mob's
     * intrinsic emotional state is genuinely low. */
    float A = sec_clamp(ai->last_4d_state.raw_arousal / 100.0f, 0.0f, 1.0f);
    if (A >= SEC_AROUSAL_EPSILON)
        return;

    float lm = SEC_DECAY_LAMBDA;

    ai->sec.fear = ai->sec.fear * (1.0f - lm) + ai->sec_base.fear_base * lm;
    ai->sec.anger = ai->sec.anger * (1.0f - lm) + ai->sec_base.anger_base * lm;
    ai->sec.happiness = ai->sec.happiness * (1.0f - lm) + ai->sec_base.happiness_base * lm;
    ai->sec.sadness = ai->sec.sadness * (1.0f - lm) + ai->sec_base.sadness_base * lm;
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
     * Anger boosts behavioural intensity; fear, helplessness, and sadness reduce it.
     * Sadness acts as the "Lethargy Buffer": high sadness suppresses both approach
     * and threat behaviours, modelling the low-arousal / low-coping passive loss state.
     * Modifier ∈ [0.5, 2.0] so it never fully suppresses nor doubles effort.
     */
    float mod = 1.0f + (s->anger - s->fear - s->helplessness * SEC_MOD_HELPLESSNESS_WEIGHT -
                        s->sadness * SEC_MOD_SADNESS_WEIGHT) *
                           0.5f;
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

float sec_get_lethargy_bias(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 0.0f;

    /* Lethargy is driven primarily by the sadness partition; helplessness
     * contributes at half weight since it captures lost-control without
     * the grief component.  Range [0, 1]: at sadness=1 the mob is fully
     * lethargic and should skip most idle/aggressive pulse actions. */
    const struct sec_state *s = &mob->ai_data->sec;
    float bias = s->sadness + s->helplessness * 0.5f;
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

/* ── OCEAN Phase 3: Agreeableness & Extraversion final value getters ──────── */

/*
 * Maximum |emotional modulation| for A and E.
 * Ensures personality cannot be fully inverted by a single emotional spike.
 * k1-k4 are configurable via cedit; this cap is a hard structural guarantee.
 */
#define SEC_OCEAN_MOD_CAP 0.10f

float sec_get_agreeableness_final(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 0.5f;

    const struct mob_personality *p = &mob->ai_data->personality;
    const struct sec_state *s = &mob->ai_data->sec;

    float base = p->agreeableness;
    float builder_mod = (float)p->agreeableness_modifier / 100.0f;

    /* A_mod = k3*happiness - k4*anger, capped at ±SEC_OCEAN_MOD_CAP.
     * k3 and k4 are server-configurable via cedit (default 10 = 0.10). */
    float k3 = (float)CONFIG_OCEAN_AE_K3 / 100.0f;
    float k4 = (float)CONFIG_OCEAN_AE_K4 / 100.0f;
    float a_mod = k3 * s->happiness - k4 * s->anger;
    a_mod = sec_clamp(a_mod, -SEC_OCEAN_MOD_CAP, SEC_OCEAN_MOD_CAP);

    return sec_clamp(base + builder_mod + a_mod, 0.0f, 1.0f);
}

float sec_get_extraversion_final(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 0.5f;

    const struct mob_personality *p = &mob->ai_data->personality;
    const struct sec_state *s = &mob->ai_data->sec;

    float base = p->extraversion;
    float builder_mod = (float)p->extraversion_modifier / 100.0f;

    /* E_mod = k1*happiness - k2*fear, capped at ±SEC_OCEAN_MOD_CAP.
     * k1 and k2 are server-configurable via cedit (default 10 = 0.10). */
    float k1 = (float)CONFIG_OCEAN_AE_K1 / 100.0f;
    float k2 = (float)CONFIG_OCEAN_AE_K2 / 100.0f;
    float e_mod = k1 * s->happiness - k2 * s->fear;
    e_mod = sec_clamp(e_mod, -SEC_OCEAN_MOD_CAP, SEC_OCEAN_MOD_CAP);

    return sec_clamp(base + builder_mod + e_mod, 0.0f, 1.0f);
}

/* ── OCEAN Phase 4: Openness final value getter ──────────────────────────── */

float sec_get_openness_final(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 0.5f;

    const struct mob_personality *p = &mob->ai_data->personality;

    float base = p->openness;
    float builder_mod = (float)p->openness_modifier / 100.0f;

    /* O_mod = 0.0f by architectural constraint.
     * Openness must NEVER be derived from SEC emotional state.
     * The SEC_O_MOD_CAP constant (0.05) documents the maximum allowed adaptation
     * if a slow long-term O_mod is implemented in the future; for now it is zero
     * to prevent any risk of SEC ↔ utility feedback loops. */
    float o_mod = 0.0f;

    return sec_clamp(base + builder_mod + o_mod, 0.0f, 1.0f);
}

/* ── OCEAN Phase 1: Neuroticism final value getter ───────────────────────── */

float sec_get_neuroticism_final(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 0.5f;

    const struct mob_personality *p = &mob->ai_data->personality;
    const struct sec_state *s = &mob->ai_data->sec;

    float base = p->neuroticism;
    float builder_mod = (float)p->neuroticism_modifier / 100.0f;

    /* N_mod: bounded stress signal from the current SEC fear+anger state.
     * Formula: N_mod = clamp(SEC_N_STRESS_SCALE * (fear + anger), -cap, +cap).
     * The cap (±0.05) prevents this from driving a recursive amplification loop:
     *   high N → more fear/anger → N_mod rises → N_final rises → …
     * The cap limits the feedback to at most 0.05 gain, which is absorbed by
     * the structural range and cannot cause runaway escalation. */
    float stress = s->fear + s->anger;
    float n_mod = sec_clamp(SEC_N_STRESS_SCALE * stress, -SEC_N_MOD_CAP, SEC_N_MOD_CAP);

    return sec_clamp(base + builder_mod + n_mod, 0.0f, 1.0f);
}

/* ── OCEAN Phase 3: A/E getters are above; C getter is near sec_update ──── */

int sec_get_dominant_emotion(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return SEC_DOMINANT_NONE;

    const struct sec_state *s = &mob->ai_data->sec;

    /* Quiescent: total arousal partition is negligible. */
    if (s->fear + s->sadness + s->anger + s->happiness < SEC_AROUSAL_EPSILON)
        return SEC_DOMINANT_NONE;

    /* Return the emotion with the highest SEC weight.
     * Tie-breaking priority: Anger > Fear > Sadness > Happiness.
     * Rationale: aggressive/threat responses take precedence; sadness is placed
     * above happiness because passive-loss states should suppress approach
     * behaviours before allowing positive affect to dominate. */
    if (s->anger >= s->fear && s->anger >= s->sadness && s->anger >= s->happiness)
        return SEC_DOMINANT_ANGER;
    if (s->fear >= s->sadness && s->fear >= s->happiness)
        return SEC_DOMINANT_FEAR;
    if (s->sadness >= s->happiness)
        return SEC_DOMINANT_SADNESS;
    return SEC_DOMINANT_HAPPINESS;
}
