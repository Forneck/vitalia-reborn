/**
 * @file moral_reasoner.h
 * Moral Reasoning Domain - Rule-based moral judgment system
 *
 * Based on Shultz & Daley's moral reasoning model (1990).
 * Qualitatively simulates moral reasoning for autonomous mob decision-making.
 * Integrates with Shadow Timeline (RFC-0003) for moral action evaluation.
 *
 * Theory Sources:
 * - Darley, J.M. & Shultz, T.R. (1990). Moral rules: Their content and
 *   acquisition. Annual Review of Psychology, 41, 525-556.
 * - Shultz, T.R. (1990). A rule base model of judging harm-doing.
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#ifndef _MORAL_REASONER_H_
#define _MORAL_REASONER_H_

#include "structs.h"

/* Moral judgment values */
#define MORAL_JUDGMENT_INNOCENT 0
#define MORAL_JUDGMENT_GUILTY 1

/* Mental state types */
#define MENTAL_STATE_NEITHER 0
#define MENTAL_STATE_NEGLIGENT 1
#define MENTAL_STATE_RECKLESS 2
#define MENTAL_STATE_INTEND 3

/* Foreseeability levels */
#define FORESEEABILITY_NONE 0
#define FORESEEABILITY_LOW 1
#define FORESEEABILITY_HIGH 2

/* Boolean values for moral attributes */
#define MORAL_NO 0
#define MORAL_YES 1

/**
 * Moral scenario - represents a harm-doing situation to be judged
 * Contains all predicates needed for moral reasoning
 */
struct moral_scenario {
    /* Core harm predicates */
    int sufficient_for_harm;
    int produce_harm;
    int necessary_for_harm;

    /* Intent and planning */
    int plan_known;
    int plan_include_harm;
    int harm_caused_as_planned;
    int mental_state;

    /* Control and responsibility */
    int external_cause;
    int external_force;
    int control_perpetrator;
    int careful;

    /* Foreseeability */
    int foreseeability;
    int foresee_intervention;

    /* Intervention */
    int intervening_contribution;
    int someone_else_cause_harm;
    int outrank_perpetrator;

    /* Benefits and harm magnitude */
    int severity_harm;
    int benefit_victim;
    int benefit_protagonist;

    /* Goal justification */
    int achieve_goal;
    int goal_outweigh_harm;
    int goal_achieveable_less_harmful;

    /* Social/hierarchical */
    int monitor;
};

/**
 * Moral judgment result with reasoning
 */
struct moral_judgment {
    int guilty;                /* MORAL_JUDGMENT_GUILTY or MORAL_JUDGMENT_INNOCENT */
    int responsibility_score;  /* 0-100: degree of responsibility */
    int blameworthiness_score; /* 0-100: degree of blame */

    /* Reasoning components */
    int caused_harm;
    int was_responsible;
    int was_blameworthy;
    int was_vicarious;
    int was_justified;
};

/* Core moral reasoning functions */

/**
 * Evaluate if entity caused harm in scenario
 * @param scenario The moral scenario
 * @return TRUE if entity caused harm
 */
bool moral_cause(struct moral_scenario *scenario);

/**
 * Evaluate if entity was responsible for harm
 * @param scenario The moral scenario
 * @return TRUE if responsible
 */
bool moral_responsible(struct moral_scenario *scenario);

/**
 * Evaluate if harm was an accident
 * @param scenario The moral scenario
 * @return TRUE if accident
 */
bool moral_accident(struct moral_scenario *scenario);

/**
 * Evaluate if harm was foreseeable
 * @param scenario The moral scenario
 * @return TRUE if foreseeable
 */
bool moral_foreseeable(struct moral_scenario *scenario);

/**
 * Evaluate if entity was reckless
 * @param scenario The moral scenario
 * @return TRUE if reckless
 */
bool moral_reckless(struct moral_scenario *scenario);

/**
 * Evaluate if entity was negligent
 * @param scenario The moral scenario
 * @return TRUE if negligent
 */
bool moral_negligent(struct moral_scenario *scenario);

/**
 * Evaluate if entity intended harm
 * @param scenario The moral scenario
 * @return TRUE if intended
 */
bool moral_intend(struct moral_scenario *scenario);

/**
 * Evaluate if entity had strong intent
 * @param scenario The moral scenario
 * @return TRUE if strong intent
 */
bool moral_strong_intend(struct moral_scenario *scenario);

/**
 * Evaluate if entity had weak intent
 * @param scenario The moral scenario
 * @return TRUE if weak intent
 */
bool moral_weak_intend(struct moral_scenario *scenario);

/**
 * Evaluate if action was voluntary
 * @param scenario The moral scenario
 * @return TRUE if voluntary
 */
bool moral_voluntary(struct moral_scenario *scenario);

/**
 * Evaluate if there was an intervening cause
 * @param scenario The moral scenario
 * @return TRUE if intervening cause
 */
bool moral_intervening_cause(struct moral_scenario *scenario);

/**
 * Evaluate vicarious responsibility
 * @param scenario The moral scenario
 * @return TRUE if vicariously responsible
 */
bool moral_vicarious(struct moral_scenario *scenario);

/**
 * Evaluate if entity was blameworthy
 * @param scenario The moral scenario
 * @return TRUE if blameworthy
 */
bool moral_blameworthy(struct moral_scenario *scenario);

/**
 * Evaluate if entity deserves vicarious blame
 * @param scenario The moral scenario
 * @return TRUE if deserves vicarious blame
 */
bool moral_vicarious_blame(struct moral_scenario *scenario);

/**
 * Evaluate if action was justified
 * @param scenario The moral scenario
 * @return TRUE if justified
 */
bool moral_justified(struct moral_scenario *scenario);

/**
 * Main function: determine if entity is guilty of harm-doing
 * @param scenario The moral scenario to evaluate
 * @param judgment Output parameter for complete judgment with reasoning
 * @return TRUE if guilty, FALSE if innocent
 */
bool moral_evaluate_guilt(struct moral_scenario *scenario, struct moral_judgment *judgment);

/* Mob AI integration functions */

/**
 * Convert mob action into moral scenario for evaluation
 * @param actor The mob performing the action
 * @param victim The target of the action (may be NULL)
 * @param action_type Type of action (attack, steal, help, etc.)
 * @param scenario Output parameter for scenario
 */
void moral_build_scenario_from_action(struct char_data *actor, struct char_data *victim, int action_type,
                                      struct moral_scenario *scenario);

/**
 * Evaluate moral cost of a proposed action
 * Returns penalty/bonus for Shadow Timeline action scoring
 * @param actor The mob considering the action
 * @param victim The potential victim
 * @param action_type Type of action
 * @return Moral cost (-100 to +100, negative is bad, positive is good)
 */
int moral_evaluate_action_cost(struct char_data *actor, struct char_data *victim, int action_type);

/**
 * Update mob's alignment based on moral judgment of recent action
 * @param ch The mob whose alignment to update
 * @param judgment The moral judgment of their action
 */
void moral_adjust_alignment(struct char_data *ch, struct moral_judgment *judgment);

/**
 * Update mob's reputation based on moral judgment
 * @param ch The mob whose reputation to update
 * @param judgment The moral judgment
 */
void moral_adjust_reputation(struct char_data *ch, struct moral_judgment *judgment);

/**
 * Update mob's emotions based on moral judgment of action performed
 * Mobs feel shame for guilty actions, pride for moral actions, etc.
 * @param ch The mob whose emotions to update
 * @param judgment The moral judgment of their action
 */
void moral_adjust_emotions(struct char_data *ch, struct moral_judgment *judgment);

/**
 * Evaluate and record a completed moral action for learning
 * This is a convenience function that evaluates moral judgment and stores it in memory
 * Call this after an action is completed to enable moral learning
 * @param actor The mob that performed the action
 * @param target The target of the action (may be NULL)
 * @param action_type Type of moral action performed
 */
void moral_record_action(struct char_data *actor, struct char_data *target, int action_type);

/**
 * Check if mob's moral conviction would prevent action
 * Used to filter out actions that conflict with moral identity
 * @param ch The mob
 * @param action_type Type of action to evaluate
 * @return TRUE if action is morally acceptable to this mob
 */
bool moral_is_action_acceptable(struct char_data *ch, int action_type);

/* Memory-based moral learning functions */

/**
 * Store moral judgment in emotion memory system
 * Records the moral judgment for learning from past actions
 * @param mob The mob that performed the action
 * @param target The target of the action (may be NULL)
 * @param action_type Type of moral action performed
 * @param judgment The moral judgment result
 */
void moral_store_judgment_in_memory(struct char_data *mob, struct char_data *target, int action_type,
                                    struct moral_judgment *judgment);

/**
 * Calculate moral learning bias from past similar actions
 * Analyzes past moral judgments to bias future decisions
 * @param ch The mob
 * @param action_type Type of action being considered
 * @return Bias adjustment (-100 to +100): negative discourages, positive encourages
 */
int moral_get_learned_bias(struct char_data *ch, int action_type);

/**
 * Calculate regret level based on emotional changes after action
 * Higher regret means the action caused negative emotional outcomes
 * @param ch The mob
 * @param pre_shame Shame level before action
 * @param pre_disgust Disgust level before action
 * @param pre_happiness Happiness level before action
 * @return Regret level (0-100)
 */
int moral_calculate_regret(struct char_data *ch, int pre_shame, int pre_disgust, int pre_happiness);

/**
 * Check if mob has learned to avoid this action type
 * Returns TRUE if past experiences strongly suggest avoiding this action
 * @param ch The mob
 * @param action_type Type of action
 * @return TRUE if action should be avoided based on learning
 */
bool moral_has_learned_avoidance(struct char_data *ch, int action_type);

/**
 * Get count of guilty vs innocent judgments for action type from memory
 * Used to assess moral learning patterns
 * @param ch The mob
 * @param action_type Type of action
 * @param out_guilty Output: count of guilty judgments
 * @param out_innocent Output: count of innocent judgments
 */
void moral_get_action_history(struct char_data *ch, int action_type, int *out_guilty, int *out_innocent);

/* Action type constants for moral evaluation */
#define MORAL_ACTION_NONE 0
#define MORAL_ACTION_ATTACK 1
#define MORAL_ACTION_STEAL 2
#define MORAL_ACTION_HELP 3
#define MORAL_ACTION_HEAL 4
#define MORAL_ACTION_TRADE 5
#define MORAL_ACTION_DECEIVE 6
#define MORAL_ACTION_SACRIFICE_SELF 7
#define MORAL_ACTION_ABANDON_ALLY 8
#define MORAL_ACTION_BETRAY 9
#define MORAL_ACTION_DEFEND 10

#endif /* _MORAL_REASONER_H_ */
