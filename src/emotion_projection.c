/**
 * @file emotion_projection.c
 * 4D Relational Decision Space - Emotional Profile Projection Layer
 *
 * Implements the projection of a mob's 20-dimensional emotional vector into
 * the 4D decision space (Valence, Arousal, Dominance, Affiliation) using
 * fixed Emotional Profile matrices and bounded personal drift.
 *
 * Formula:
 *   P_raw       = M_profile                    * E
 *   P_effective = (M_profile + ΔM_personal)    * E   (after drift)
 *
 * See emotion_projection.h for full API documentation.
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "fight.h"
#include "db.h"
#include "constants.h"
#include "emotion_projection.h"

#include <math.h>

/* ========================================================================== */
/*  Emotional Profile projection matrices                                      */
/*                                                                             */
/*  Layout: [profile][axis][emotion]                                           */
/*  Axes:   [0]=Valence  [1]=Arousal  [2]=Dominance  [3]=Affiliation          */
/*  Emotions (EMOTION_TYPE_*):                                                 */
/*    [0]=fear     [1]=anger       [2]=happiness  [3]=sadness                  */
/*    [4]=friend.  [5]=love        [6]=trust      [7]=loyalty                  */
/*    [8]=curiosity[9]=greed       [10]=pride     [11]=compassion              */
/*    [12]=envy    [13]=courage    [14]=excite.   [15]=disgust                 */
/*    [16]=shame   [17]=pain       [18]=horror    [19]=humiliation             */
/*                                                                             */
/*  Each row's positive weights are designed to sum to approximately 1.0 and   */
/*  negative weights to approximately −1.0, so after L1-norm normalisation the */
/*  outputs naturally land in [−100, +100].                                    */
/* ========================================================================== */

/* clang-format off */
static const float emotion_profile_matrices[EMOTION_PROFILE_NUM][DECISION_SPACE_DIMS][20] = {

    /* ------------------------------------------------------------------ */
    /* 0 – NEUTRAL: balanced baseline projection                           */
    /* ------------------------------------------------------------------ */
    {
        /* V  */ { -0.15f, -0.10f, +0.20f, -0.15f, +0.15f, +0.20f, +0.15f, +0.10f,
                   +0.05f, -0.05f, +0.05f, +0.10f, -0.05f, +0.10f, +0.10f, -0.10f,
                   -0.10f, -0.15f, -0.20f, -0.15f },
        /* A  */ { +0.20f, +0.25f, -0.10f, -0.15f, -0.05f, -0.05f,  0.00f,  0.00f,
                   +0.10f, +0.05f, +0.05f, -0.05f, +0.05f, +0.15f, +0.25f, +0.05f,
                   +0.05f, +0.20f, +0.25f, +0.10f },
        /* D  */ { -0.25f, +0.15f, +0.10f, -0.15f, +0.05f, +0.05f, +0.10f, +0.10f,
                   +0.05f, +0.05f, +0.20f,  0.00f, -0.05f, +0.25f, +0.10f, +0.05f,
                   -0.20f, -0.15f, -0.20f, -0.25f },
        /* Af */ { -0.10f, -0.20f, +0.10f, -0.05f, +0.25f, +0.25f, +0.20f, +0.25f,
                   +0.10f, -0.10f, -0.05f, +0.20f, -0.15f, +0.05f, +0.05f, -0.15f,
                   -0.05f, -0.05f, -0.10f, -0.10f },
    },

    /* ------------------------------------------------------------------ */
    /* 1 – AGGRESSIVE: anger converts quickly to Dominance and Arousal     */
    /* ------------------------------------------------------------------ */
    {
        /* V  */ { -0.20f, -0.15f, +0.20f, -0.10f, +0.10f, +0.15f, +0.10f, +0.10f,
                   +0.05f, -0.05f, +0.10f, +0.05f, -0.05f, +0.15f, +0.10f, -0.10f,
                   -0.10f, -0.20f, -0.20f, -0.15f },
        /* A  */ { +0.15f, +0.40f, -0.05f, -0.10f, -0.05f, -0.05f,  0.00f,  0.00f,
                   +0.05f, +0.05f, +0.10f, -0.05f, +0.05f, +0.10f, +0.25f, +0.05f,
                   +0.05f, +0.15f, +0.15f, +0.05f },
        /* D  */ { -0.15f, +0.30f, +0.10f, -0.10f, +0.05f, +0.05f, +0.05f, +0.05f,
                   +0.05f, +0.05f, +0.25f,  0.00f, -0.05f, +0.20f, +0.15f, +0.05f,
                   -0.15f, -0.10f, -0.15f, -0.20f },
        /* Af */ { -0.05f, -0.30f, +0.10f, -0.05f, +0.20f, +0.20f, +0.15f, +0.20f,
                   +0.05f, -0.05f, -0.05f, +0.15f, -0.10f, +0.05f, +0.05f, -0.20f,
                   -0.05f, -0.05f, -0.10f, -0.05f },
    },

    /* ------------------------------------------------------------------ */
    /* 2 – DEFENSIVE: fear → Arousal and Affiliation avoidance             */
    /* ------------------------------------------------------------------ */
    {
        /* V  */ { -0.25f, -0.10f, +0.20f, -0.20f, +0.15f, +0.20f, +0.15f, +0.10f,
                   +0.05f, -0.05f, +0.05f, +0.10f, -0.05f, +0.05f, +0.05f, -0.10f,
                   -0.10f, -0.20f, -0.25f, -0.15f },
        /* A  */ { +0.35f, +0.20f, -0.10f, -0.20f, -0.05f, -0.05f,  0.00f,  0.00f,
                   +0.05f, +0.05f, +0.05f, -0.05f, +0.05f, +0.10f, +0.20f, +0.05f,
                   +0.10f, +0.25f, +0.30f, +0.15f },
        /* D  */ { -0.35f, +0.10f, +0.10f, -0.20f, +0.05f, +0.05f, +0.10f, +0.10f,
                   +0.05f, +0.05f, +0.15f,  0.00f, -0.05f, +0.15f, +0.05f, +0.05f,
                   -0.25f, -0.20f, -0.25f, -0.25f },
        /* Af */ { -0.25f, -0.15f, +0.10f, -0.10f, +0.25f, +0.25f, +0.20f, +0.25f,
                   +0.10f, -0.10f, -0.05f, +0.20f, -0.10f, +0.05f, +0.05f, -0.15f,
                   -0.10f, -0.10f, -0.15f, -0.10f },
    },

    /* ------------------------------------------------------------------ */
    /* 3 – BALANCED: moderate projection across all axes (80% of Neutral)  */
    /* ------------------------------------------------------------------ */
    {
        /* V  */ { -0.12f, -0.08f, +0.16f, -0.12f, +0.12f, +0.16f, +0.12f, +0.08f,
                   +0.04f, -0.04f, +0.04f, +0.08f, -0.04f, +0.08f, +0.08f, -0.08f,
                   -0.08f, -0.12f, -0.16f, -0.12f },
        /* A  */ { +0.16f, +0.20f, -0.08f, -0.12f, -0.04f, -0.04f,  0.00f,  0.00f,
                   +0.08f, +0.04f, +0.04f, -0.04f, +0.04f, +0.12f, +0.20f, +0.04f,
                   +0.04f, +0.16f, +0.20f, +0.08f },
        /* D  */ { -0.20f, +0.12f, +0.08f, -0.12f, +0.04f, +0.04f, +0.08f, +0.08f,
                   +0.04f, +0.04f, +0.16f,  0.00f, -0.04f, +0.20f, +0.08f, +0.04f,
                   -0.16f, -0.12f, -0.16f, -0.20f },
        /* Af */ { -0.08f, -0.16f, +0.08f, -0.04f, +0.20f, +0.20f, +0.16f, +0.20f,
                   +0.08f, -0.08f, -0.04f, +0.16f, -0.12f, +0.04f, +0.04f, -0.12f,
                   -0.04f, -0.04f, -0.08f, -0.08f },
    },

    /* ------------------------------------------------------------------ */
    /* 4 – SENSITIVE: high Affiliation sensitivity; empathy drives all axes */
    /* ------------------------------------------------------------------ */
    {
        /* V  */ { -0.15f, -0.10f, +0.20f, -0.15f, +0.20f, +0.25f, +0.15f, +0.10f,
                   +0.05f, -0.05f, +0.05f, +0.15f, -0.08f, +0.10f, +0.10f, -0.10f,
                   -0.10f, -0.15f, -0.20f, -0.15f },
        /* A  */ { +0.20f, +0.20f, -0.10f, -0.15f, -0.05f, -0.05f,  0.00f,  0.00f,
                   +0.10f, +0.05f, +0.05f, -0.05f, +0.05f, +0.15f, +0.20f, +0.05f,
                   +0.05f, +0.20f, +0.25f, +0.10f },
        /* D  */ { -0.20f, +0.10f, +0.10f, -0.15f, +0.05f, +0.05f, +0.10f, +0.10f,
                   +0.05f, +0.05f, +0.20f,  0.00f, -0.05f, +0.20f, +0.10f, +0.05f,
                   -0.20f, -0.15f, -0.20f, -0.25f },
        /* Af */ { -0.15f, -0.20f, +0.10f, -0.10f, +0.35f, +0.35f, +0.25f, +0.30f,
                   +0.10f, -0.10f, -0.05f, +0.30f, -0.15f, +0.05f, +0.05f, -0.20f,
                   -0.10f, -0.10f, -0.15f, -0.15f },
    },

    /* ------------------------------------------------------------------ */
    /* 5 – CONFIDENT: stable Dominance under stress; fear has reduced pull  */
    /* ------------------------------------------------------------------ */
    {
        /* V  */ { -0.10f, -0.10f, +0.20f, -0.15f, +0.15f, +0.20f, +0.15f, +0.10f,
                   +0.05f, -0.05f, +0.10f, +0.10f, -0.05f, +0.15f, +0.10f, -0.10f,
                   -0.10f, -0.15f, -0.15f, -0.15f },
        /* A  */ { +0.15f, +0.20f, -0.10f, -0.10f, -0.05f, -0.05f,  0.00f,  0.00f,
                   +0.10f, +0.05f, +0.05f, -0.05f, +0.05f, +0.20f, +0.25f, +0.05f,
                   +0.05f, +0.15f, +0.20f, +0.10f },
        /* D  */ { -0.10f, +0.15f, +0.10f, -0.10f, +0.05f, +0.05f, +0.10f, +0.10f,
                   +0.05f, +0.05f, +0.30f,  0.00f, -0.05f, +0.40f, +0.10f, +0.05f,
                   -0.15f, -0.10f, -0.10f, -0.15f },
        /* Af */ { -0.10f, -0.15f, +0.10f, -0.05f, +0.25f, +0.25f, +0.20f, +0.25f,
                   +0.10f, -0.10f, -0.05f, +0.20f, -0.15f, +0.05f, +0.05f, -0.15f,
                   -0.05f, -0.05f, -0.10f, -0.10f },
    },

    /* ------------------------------------------------------------------ */
    /* 6 – GREEDY: Valence tightly coupled to resource emotions             */
    /* ------------------------------------------------------------------ */
    {
        /* V  */ { -0.10f, -0.10f, +0.15f, -0.10f, +0.10f, +0.15f, +0.10f, +0.05f,
                   +0.05f, +0.20f, +0.10f, +0.05f, -0.20f, +0.10f, +0.15f, -0.10f,
                   -0.10f, -0.15f, -0.15f, -0.10f },
        /* A  */ { +0.15f, +0.20f, -0.10f, -0.10f, -0.05f, -0.05f,  0.00f,  0.00f,
                   +0.10f, +0.20f, +0.10f, -0.05f, +0.10f, +0.10f, +0.25f, +0.05f,
                   +0.05f, +0.15f, +0.20f, +0.10f },
        /* D  */ { -0.20f, +0.15f, +0.10f, -0.10f, +0.05f, +0.05f, +0.10f, +0.10f,
                   +0.05f, +0.15f, +0.25f,  0.00f, -0.10f, +0.20f, +0.10f, +0.05f,
                   -0.15f, -0.15f, -0.15f, -0.20f },
        /* Af */ { -0.10f, -0.20f, +0.05f, -0.05f, +0.20f, +0.20f, +0.15f, +0.20f,
                   +0.05f, -0.20f, -0.05f, +0.15f, -0.20f, +0.05f, +0.05f, -0.15f,
                   -0.05f, -0.05f, -0.10f, -0.10f },
    },

    /* ------------------------------------------------------------------ */
    /* 7 – LOYAL: Affiliation strongly weighted by loyalty/friendship       */
    /* ------------------------------------------------------------------ */
    {
        /* V  */ { -0.10f, -0.10f, +0.20f, -0.15f, +0.20f, +0.20f, +0.20f, +0.20f,
                   +0.05f, -0.05f, +0.05f, +0.15f, -0.10f, +0.10f, +0.10f, -0.10f,
                   -0.10f, -0.15f, -0.20f, -0.15f },
        /* A  */ { +0.15f, +0.20f, -0.10f, -0.10f, -0.05f, -0.05f,  0.00f,  0.00f,
                   +0.10f, +0.05f, +0.05f, -0.05f, +0.05f, +0.15f, +0.25f, +0.05f,
                   +0.05f, +0.15f, +0.20f, +0.10f },
        /* D  */ { -0.20f, +0.10f, +0.10f, -0.15f, +0.10f, +0.10f, +0.15f, +0.20f,
                   +0.05f, +0.05f, +0.20f,  0.00f, -0.05f, +0.25f, +0.10f, +0.05f,
                   -0.15f, -0.10f, -0.15f, -0.20f },
        /* Af */ { -0.10f, -0.15f, +0.10f, -0.10f, +0.35f, +0.30f, +0.30f, +0.40f,
                   +0.10f, -0.10f, -0.05f, +0.20f, -0.15f, +0.05f, +0.05f, -0.20f,
                   -0.10f, -0.10f, -0.15f, -0.15f },
    },
};
/* clang-format on */

/* ========================================================================== */
/*  Helper: L1-normalised dot product along one axis                          */
/* ========================================================================== */

/**
 * Compute the weighted projection for a single axis.
 *
 * result = 100 * Σ(row[i] * E[i]) / (Σ|row[i]| * 100)
 *        = Σ(row[i] * E[i]) / Σ|row[i]|
 *
 * This guarantees the result is in [−100, +100] regardless of how many
 * emotions are simultaneously at maximum.
 */
static float project_axis(const float row[20], const float E[20])
{
    float numerator = 0.0f;
    float l1_norm = 0.0f;
    int i;

    for (i = 0; i < 20; i++) {
        numerator += row[i] * E[i];
        l1_norm += fabsf(row[i]);
    }

    if (l1_norm < 1e-6f)
        return 0.0f;

    /* numerator is already in [−(l1_norm*100), +(l1_norm*100)] */
    /* Divide by l1_norm to get [−100, +100] */
    float result = numerator / l1_norm;
    return result < -100.0f ? -100.0f : (result > 100.0f ? 100.0f : result);
}

/* ========================================================================== */
/*  Public API                                                                 */
/* ========================================================================== */

const float (*emotion_get_profile_matrix(int profile))[20]
{
    if (profile < 0 || profile >= EMOTION_PROFILE_NUM)
        profile = EMOTION_PROFILE_NEUTRAL;
    return emotion_profile_matrices[profile];
}

void emotion_compute_raw_projection(struct char_data *mob, float raw_out[DECISION_SPACE_DIMS])
{
    int axis;
    float E[20];
    float effective_row[20];

    if (!mob || !IS_NPC(mob) || !mob->ai_data) {
        for (axis = 0; axis < DECISION_SPACE_DIMS; axis++)
            raw_out[axis] = 0.0f;
        return;
    }

    /* Build emotion vector E from mob's current emotions (0–100 scale) */
    E[0] = (float)mob->ai_data->emotion_fear;
    E[1] = (float)mob->ai_data->emotion_anger;
    E[2] = (float)mob->ai_data->emotion_happiness;
    E[3] = (float)mob->ai_data->emotion_sadness;
    E[4] = (float)mob->ai_data->emotion_friendship;
    E[5] = (float)mob->ai_data->emotion_love;
    E[6] = (float)mob->ai_data->emotion_trust;
    E[7] = (float)mob->ai_data->emotion_loyalty;
    E[8] = (float)mob->ai_data->emotion_curiosity;
    E[9] = (float)mob->ai_data->emotion_greed;
    E[10] = (float)mob->ai_data->emotion_pride;
    E[11] = (float)mob->ai_data->emotion_compassion;
    E[12] = (float)mob->ai_data->emotion_envy;
    E[13] = (float)mob->ai_data->emotion_courage;
    E[14] = (float)mob->ai_data->emotion_excitement;
    E[15] = (float)mob->ai_data->emotion_disgust;
    E[16] = (float)mob->ai_data->emotion_shame;
    E[17] = (float)mob->ai_data->emotion_pain;
    E[18] = (float)mob->ai_data->emotion_horror;
    E[19] = (float)mob->ai_data->emotion_humiliation;

    int profile = mob->ai_data->emotional_profile;
    if (profile < 0 || profile >= EMOTION_PROFILE_NUM)
        profile = EMOTION_PROFILE_NEUTRAL;

    /* Project each axis using the effective matrix M_profile + ΔM_personal */
    for (axis = 0; axis < DECISION_SPACE_DIMS; axis++) {
        int e;
        for (e = 0; e < 20; e++)
            effective_row[e] = emotion_profile_matrices[profile][axis][e] + mob->ai_data->personal_drift[axis][e];
        raw_out[axis] = project_axis(effective_row, E);
    }
}

float emotion_compute_coping_potential(struct char_data *mob)
{
    if (!mob)
        return 50.0f;

    /* HP contribution: 0–60 points */
    float hp_max = (float)GET_MAX_HIT(mob);
    float hp_factor = (hp_max > 0.0f) ? ((float)GET_HIT(mob) / hp_max) * 60.0f : 0.0f;

    /* Numerical advantage in room: −20 to +20 */
    float num_factor = 0.0f;
    if (IN_ROOM(mob) != NOWHERE) {
        struct char_data *vict;
        int allies = 0, enemies = 0;
        for (vict = world[IN_ROOM(mob)].people; vict; vict = vict->next_in_room) {
            if (vict == mob)
                continue;
            if (IS_NPC(vict) == IS_NPC(mob))
                allies++;
            else
                enemies++;
        }
        /* Clamp advantage to ±4 characters → ±20 points */
        int advantage = allies - enemies;
        if (advantage > 4)
            advantage = 4;
        if (advantage < -4)
            advantage = -4;
        num_factor = (float)advantage * 5.0f;
    }

    /* Status-effect penalty: −20 if debilitated */
    float status_factor = 0.0f;
    if (AFF_FLAGGED(mob, AFF_BLIND))
        status_factor -= 10.0f;
    if (AFF_FLAGGED(mob, AFF_POISON))
        status_factor -= 10.0f;
    if (AFF_FLAGGED(mob, AFF_SLEEP))
        status_factor -= 20.0f;
    if (AFF_FLAGGED(mob, AFF_CURSE))
        status_factor -= 5.0f;

    float coping = hp_factor + num_factor + status_factor;
    return coping < 0.0f ? 0.0f : (coping > 100.0f ? 100.0f : coping);
}

void emotion_apply_contextual_modulation(struct char_data *mob, struct char_data *target,
                                         const float raw[DECISION_SPACE_DIMS], float coping_pot,
                                         float effective_out[DECISION_SPACE_DIMS])
{
    int axis;
    for (axis = 0; axis < DECISION_SPACE_DIMS; axis++)
        effective_out[axis] = raw[axis];

    /* --- Dominance: additively modulated by objective Coping Potential ---    */
    /* Formula: D_effective = D_projected + (coping_pot − 50) * 0.4            */
    /* This is a bounded additive blend: coping can shift dominance by at most  */
    /* ±20 pts (when coping_pot = 0 or 100), preserving the profile projection. */
    /* It does NOT overwrite the projected dominance value.                     */
    float dominance_adj = (coping_pot - 50.0f) * 0.4f;
    effective_out[DECISION_AXIS_DOMINANCE] += dominance_adj;

    /* --- Arousal: scaled by environmental intensity --- */
    /* Combat state and crowd density increase urgency.                         */
    float env_intensity = 0.0f;
    if (mob && FIGHTING(mob))
        env_intensity += 0.40f; /* in combat */
    if (mob && IN_ROOM(mob) != NOWHERE) {
        struct char_data *vict;
        int crowd = 0;
        for (vict = world[IN_ROOM(mob)].people; vict; vict = vict->next_in_room)
            if (vict != mob)
                crowd++;
        /* Each extra character beyond 2 adds minor tension (cap at +0.30) */
        float crowd_bonus = (float)(crowd - 2) * 0.05f;
        if (crowd_bonus < 0.0f)
            crowd_bonus = 0.0f;
        if (crowd_bonus > 0.30f)
            crowd_bonus = 0.30f;
        env_intensity += crowd_bonus;
    }
    if (env_intensity > 1.0f)
        env_intensity = 1.0f;
    effective_out[DECISION_AXIS_AROUSAL] *= (1.0f + env_intensity * 0.5f);

    /* --- Affiliation: weighted by relationship memory with target --- */
    if (mob && target && IS_NPC(mob) && mob->ai_data) {
        int rel_trust = get_relationship_emotion(mob, target, EMOTION_TYPE_TRUST);
        int rel_friend = get_relationship_emotion(mob, target, EMOTION_TYPE_FRIENDSHIP);
        /* Positive memories → approach; negative → avoidance */
        float rel_bias = ((float)(rel_trust + rel_friend) - 100.0f) * 0.15f;
        effective_out[DECISION_AXIS_AFFILIATION] += rel_bias;
    }

    /* --- Valence: optionally biased by Shadow Timeline anticipated outcome --- */
    /* Influence coefficient is intentionally small (0.10) to keep reactive     */
    /* emotional behavior dominant over strategic anticipation.                 */
    /* Bias is explicitly clamped to ±SHADOW_FORECAST_VALENCE_CAP so that even */
    /* a corrupted pred_score cannot over-ride the projected valence.           */
    if (mob && IS_NPC(mob) && mob->ai_data) {
        int pred_score = mob->ai_data->last_predicted_score; /* −100 to 100 */
        float forecast_bias = (float)pred_score * 0.10f;
        /* Hard cap: forecast may nudge valence by at most ±10 pts */
        if (forecast_bias > 10.0f)
            forecast_bias = 10.0f;
        if (forecast_bias < -10.0f)
            forecast_bias = -10.0f;
        effective_out[DECISION_AXIS_VALENCE] += forecast_bias;
    }

    /* Clamp all axes to [−100, +100] */
    for (axis = 0; axis < DECISION_SPACE_DIMS; axis++) {
        if (effective_out[axis] < -100.0f)
            effective_out[axis] = -100.0f;
        if (effective_out[axis] > 100.0f)
            effective_out[axis] = 100.0f;
    }

    /* Arousal is non-negative by definition */
    if (effective_out[DECISION_AXIS_AROUSAL] < 0.0f)
        effective_out[DECISION_AXIS_AROUSAL] = 0.0f;
}

/* ========================================================================== */
/*  Public wrapper: compute_emotion_4d_state (declared in utils.h)            */
/* ========================================================================== */

struct emotion_4d_state compute_emotion_4d_state(struct char_data *mob, struct char_data *target)
{
    struct emotion_4d_state state;
    float raw[DECISION_SPACE_DIMS];
    float effective[DECISION_SPACE_DIMS];
    float coping;

    /* Zero-initialise */
    memset(&state, 0, sizeof(state));

    if (!mob || !IS_NPC(mob) || !mob->ai_data)
        return state;

    /* Step 1: Raw projection P_raw = (M_profile + ΔM_personal) * E */
    emotion_compute_raw_projection(mob, raw);

    /* Step 2: Coping Potential (objective situational capacity) */
    coping = emotion_compute_coping_potential(mob);

    /* Step 3: Contextual Modulation */
    emotion_apply_contextual_modulation(mob, target, raw, coping, effective);

    /* Populate state */
    state.raw_valence = raw[DECISION_AXIS_VALENCE];
    state.raw_arousal = raw[DECISION_AXIS_AROUSAL];
    state.raw_dominance = raw[DECISION_AXIS_DOMINANCE];
    state.raw_affiliation = raw[DECISION_AXIS_AFFILIATION];

    state.valence = effective[DECISION_AXIS_VALENCE];
    state.arousal = effective[DECISION_AXIS_AROUSAL];
    state.dominance = effective[DECISION_AXIS_DOMINANCE];
    state.affiliation = effective[DECISION_AXIS_AFFILIATION];
    state.coping_potential = coping;
    state.valid = TRUE;

    return state;
}

/* ========================================================================== */
/*  compute_coping_potential (utils.h declaration wrapper)                    */
/* ========================================================================== */

float compute_coping_potential(struct char_data *mob) { return emotion_compute_coping_potential(mob); }

/* ========================================================================== */
/*  Personal drift                                                             */
/* ========================================================================== */

/* Pre-computed drift scale: PERSONAL_DRIFT_MAX_PCT / 100 */
#define DRIFT_SCALE 0.2f /* 20 / 100 */

void update_personal_drift(struct char_data *mob, int axis, int emotion_type, float event_weight)
{
    float *drift;
    float M_base;
    float max_drift;
    float delta;

    if (!mob || !IS_NPC(mob) || !mob->ai_data)
        return;
    if (axis < 0 || axis >= DECISION_SPACE_DIMS)
        return;
    if (emotion_type < 0 || emotion_type >= 20)
        return;

    int profile = mob->ai_data->emotional_profile;
    if (profile < 0 || profile >= EMOTION_PROFILE_NUM)
        profile = EMOTION_PROFILE_NEUTRAL;

    drift = &mob->ai_data->personal_drift[axis][emotion_type];
    M_base = emotion_profile_matrices[profile][axis][emotion_type];

    /* Bound: ±PERSONAL_DRIFT_MAX_PCT% of the absolute baseline weight.       */
    /* If the baseline weight is zero, use a small absolute cap (0.01).        */
    max_drift = fabsf(M_base) * DRIFT_SCALE;
    if (max_drift < 0.01f)
        max_drift = 0.01f;

    /* Incremental event-weighted update (trauma/repetition scale with weight) */
    delta = event_weight * 0.05f;
    *drift += delta;

    /* Hard clamp */
    if (*drift > max_drift)
        *drift = max_drift;
    if (*drift < -max_drift)
        *drift = -max_drift;

    /* Runtime assertion: drift must remain within bounds after clamping.
     * This catches potential floating-point creep or logic errors in debug builds. */
    if ((*drift > max_drift + 1e-4f) || (*drift < -(max_drift + 1e-4f)))
        log1("SYSERR: 4D personal_drift[%d][%d] out of bounds (%.4f, max=%.4f) for mob %s (#%d)", axis, emotion_type,
             *drift, max_drift, GET_NAME(mob), GET_MOB_VNUM(mob));
}

/* ========================================================================== */
/*  Debug logging                                                              */
/* ========================================================================== */

void log_4d_state(struct char_data *mob, struct char_data *target, const struct emotion_4d_state *state)
{
    if (!mob || !state || !state->valid)
        return;

    const char *target_name = target ? GET_NAME(target) : "(none)";
    const char *profile_name =
        (mob->ai_data && mob->ai_data->emotional_profile >= 0 && mob->ai_data->emotional_profile < EMOTION_PROFILE_NUM)
            ? emotion_profile_types[mob->ai_data->emotional_profile]
            : "Unknown";

    /* PROJ = drift-adjusted projection (M_profile + ΔM_personal)*E, pre-context
     * COPING = objective situational capacity (separate from Dominance axis)
     * EFF  = final state after Contextual Modulation Layer */
    log1(
        "4D-SPACE: mob=%s(#%d) profile=%s target=%s "
        "PROJ[V=%.1f A=%.1f D=%.1f Af=%.1f] "
        "COPING=%.1f "
        "EFF[V=%.1f A=%.1f D=%.1f Af=%.1f]",
        GET_NAME(mob), GET_MOB_VNUM(mob), profile_name, target_name, state->raw_valence, state->raw_arousal,
        state->raw_dominance, state->raw_affiliation, state->coping_potential, state->valence, state->arousal,
        state->dominance, state->affiliation);
}
