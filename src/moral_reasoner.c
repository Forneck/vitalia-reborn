/**
 * @file moral_reasoner.c
 * Moral Reasoning Domain - Implementation
 *
 * Rule-based moral judgment system based on Shultz & Daley's model.
 * Provides moral reasoning capabilities for autonomous mob decision-making.
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "moral_reasoner.h"
#include "handler.h"
#include "db.h"
#include "lists.h"

/* Severity calculation constants */
#define SEVERITY_SCALING_FACTOR 10 /* Divisor for HP-based severity calculation */
#define DEFAULT_SEVERITY_HARM 5    /* Default severity when victim is unknown */

/* Forward declarations for internal functions */
static int moral_evaluate_action_cost_internal(struct char_data *actor, struct char_data *victim, int action_type,
                                               bool include_group_dynamics);

/* Helper function to check if value indicates yes */
static bool is_yes(int value) { return value == MORAL_YES; }

/* Helper function to check if value indicates no */
static bool is_no(int value) { return value == MORAL_NO; }

/* Helper function to compare numerical values */
static bool greater_than(int a, int b) { return a > b; }

/**
 * Evaluate if entity caused harm
 * Rule: cause(X) :- produce_harm(X,y) OR necessary_for_harm(X,y) OR sufficient_for_harm(X,y)
 */
bool moral_cause(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return is_yes(scenario->produce_harm) || is_yes(scenario->necessary_for_harm) ||
           is_yes(scenario->sufficient_for_harm);
}

/**
 * Evaluate if harm was foreseeable
 * Rule: foreseeable(X) :- foreseeability(X,high) OR foreseeability(X,low)
 */
bool moral_foreseeable(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return (scenario->foreseeability == FORESEEABILITY_HIGH || scenario->foreseeability == FORESEEABILITY_LOW);
}

/**
 * Evaluate if entity had strong intent
 * Rule: strong_intend(X) :- mental_state(X,intend) OR
 *                          (plan_known(X,y) AND plan_include_harm(X,y) AND harm_caused_as_planned(X,y))
 */
bool moral_strong_intend(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    if (scenario->mental_state == MENTAL_STATE_INTEND)
        return TRUE;

    return (is_yes(scenario->plan_known) && is_yes(scenario->plan_include_harm) &&
            is_yes(scenario->harm_caused_as_planned));
}

/**
 * Evaluate if entity was reckless
 * Rule: reckless(X) :- mental_state(X,reckless) OR
 *                      (NOT careful(X,y) AND NOT strong_intend(X) AND foreseeability(X,high))
 */
bool moral_reckless(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    if (scenario->mental_state == MENTAL_STATE_RECKLESS)
        return TRUE;

    return (is_no(scenario->careful) && !moral_strong_intend(scenario) &&
            scenario->foreseeability == FORESEEABILITY_HIGH);
}

/**
 * Evaluate if entity was negligent
 * Rule: negligent(X) :- mental_state(X,negligent) OR
 *                       (NOT careful(X,y) AND NOT strong_intend(X) AND foreseeability(X,low))
 */
bool moral_negligent(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    if (scenario->mental_state == MENTAL_STATE_NEGLIGENT)
        return TRUE;

    return (is_no(scenario->careful) && !moral_strong_intend(scenario) &&
            scenario->foreseeability == FORESEEABILITY_LOW);
}

/**
 * Helper: check if intent should be discounted
 * Rule: discount_intent(X) :- external_cause(X,y)
 */
static bool moral_discount_intent(struct moral_scenario *scenario) { return is_yes(scenario->external_cause); }

/**
 * Helper: evaluate weak intent conditions (part 1)
 * Rule: weak_intend1(X) :- NOT discount_intent(X) OR monitor(X,y) OR benefit_protagonist(X,y)
 */
static bool moral_weak_intend1(struct moral_scenario *scenario)
{
    return !moral_discount_intent(scenario) || is_yes(scenario->monitor) || is_yes(scenario->benefit_protagonist);
}

/**
 * Evaluate if entity had weak intent
 * Rule: weak_intend(X) :- weak_intend1(X) AND NOT reckless(X) AND NOT negligent(X)
 */
bool moral_weak_intend(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return moral_weak_intend1(scenario) && !moral_reckless(scenario) && !moral_negligent(scenario);
}

/**
 * Evaluate if entity intended harm
 * Rule: intend(X) :- strong_intend(X) OR weak_intend(X)
 */
bool moral_intend(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return moral_strong_intend(scenario) || moral_weak_intend(scenario);
}

/**
 * Evaluate if harm was an accident
 * Rule: accident(X) :- NOT intend(X) AND NOT reckless(X) AND NOT negligent(X)
 */
bool moral_accident(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return !moral_intend(scenario) && !moral_reckless(scenario) && !moral_negligent(scenario);
}

/**
 * Evaluate if action was voluntary
 * Rule: voluntary(X) :- NOT external_force(X,y)
 */
bool moral_voluntary(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return is_no(scenario->external_force);
}

/**
 * Evaluate if there was an intervening cause
 * Rule: intervening_cause(X) :- intervening_contribution(X,y) AND NOT foresee_intervention(X,y)
 */
bool moral_intervening_cause(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return is_yes(scenario->intervening_contribution) && is_no(scenario->foresee_intervention);
}

/**
 * Evaluate if entity was responsible for harm
 * Rule: responsible(X) :- cause(X) AND NOT accident(X) AND voluntary(X) AND
 *                         foreseeable(X) AND NOT intervening_cause(X)
 */
bool moral_responsible(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return moral_cause(scenario) && !moral_accident(scenario) && moral_voluntary(scenario) &&
           moral_foreseeable(scenario) && !moral_intervening_cause(scenario);
}

/**
 * Evaluate vicarious responsibility
 * Rule: vicarious(X) :- someone_else_cause_harm(X,y) AND outrank_perpetrator(X,y) AND
 *                       control_perpetrator(X,y)
 */
bool moral_vicarious(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return is_yes(scenario->someone_else_cause_harm) && is_yes(scenario->outrank_perpetrator) &&
           is_yes(scenario->control_perpetrator);
}

/**
 * Evaluate if action was justified
 * Rule: justified(X) :- achieve_goal(X,y) AND goal_outweigh_harm(X,y) AND
 *                       NOT goal_achieveable_less_harmful(X,y)
 */
bool moral_justified(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return is_yes(scenario->achieve_goal) && is_yes(scenario->goal_outweigh_harm) &&
           is_no(scenario->goal_achieveable_less_harmful);
}

/**
 * Evaluate if entity was blameworthy
 * Rule: blameworthy(X) :- responsible(X) AND NOT justified(X) AND
 *                         severity_harm(X,H) AND benefit_victim(X,V) AND H > V
 */
bool moral_blameworthy(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return moral_responsible(scenario) && !moral_justified(scenario) &&
           greater_than(scenario->severity_harm, scenario->benefit_victim);
}

/**
 * Evaluate if entity deserves vicarious blame
 * Rule: vicarious_blame(X) :- vicarious(X) AND NOT justified(X) AND
 *                             severity_harm(X,H) AND benefit_victim(X,V) AND H > V
 */
bool moral_vicarious_blame(struct moral_scenario *scenario)
{
    if (!scenario)
        return FALSE;

    return moral_vicarious(scenario) && !moral_justified(scenario) &&
           greater_than(scenario->severity_harm, scenario->benefit_victim);
}

/**
 * Main moral evaluation function
 * Rule: guilty(X) :- blameworthy(X) OR vicarious_blame(X)
 */
bool moral_evaluate_guilt(struct moral_scenario *scenario, struct moral_judgment *judgment)
{
    if (!scenario || !judgment)
        return FALSE;

    /* Initialize judgment */
    judgment->guilty = MORAL_JUDGMENT_INNOCENT;
    judgment->responsibility_score = 0;
    judgment->blameworthiness_score = 0;
    judgment->caused_harm = FALSE;
    judgment->was_responsible = FALSE;
    judgment->was_blameworthy = FALSE;
    judgment->was_vicarious = FALSE;
    judgment->was_justified = FALSE;

    /* Evaluate components */
    judgment->caused_harm = moral_cause(scenario);
    judgment->was_responsible = moral_responsible(scenario);
    judgment->was_blameworthy = moral_blameworthy(scenario);
    judgment->was_vicarious = moral_vicarious_blame(scenario);
    judgment->was_justified = moral_justified(scenario);

    /* Determine guilt */
    if (judgment->was_blameworthy || judgment->was_vicarious) {
        judgment->guilty = MORAL_JUDGMENT_GUILTY;
    }

    /* Calculate responsibility score (0-100) */
    if (judgment->caused_harm)
        judgment->responsibility_score += 25;
    if (judgment->was_responsible)
        judgment->responsibility_score += 50;
    if (moral_intend(scenario))
        judgment->responsibility_score += 25;

    /* Calculate blameworthiness score (0-100) */
    if (judgment->was_responsible)
        judgment->blameworthiness_score += 40;
    if (!judgment->was_justified)
        judgment->blameworthiness_score += 30;
    if (scenario->severity_harm > 0)
        judgment->blameworthiness_score += MIN(30, scenario->severity_harm * 3);

    return judgment->guilty == MORAL_JUDGMENT_GUILTY;
}

/**
 * Build moral scenario from mob action
 * Translates game state into moral reasoning predicates
 */
void moral_build_scenario_from_action(struct char_data *actor, struct char_data *victim, int action_type,
                                      struct moral_scenario *scenario)
{
    if (!actor || !scenario)
        return;

    /* Initialize all fields to neutral/no */
    scenario->sufficient_for_harm = MORAL_NO;
    scenario->produce_harm = MORAL_NO;
    scenario->necessary_for_harm = MORAL_NO;
    scenario->plan_known = MORAL_NO;
    scenario->plan_include_harm = MORAL_NO;
    scenario->harm_caused_as_planned = MORAL_NO;
    scenario->mental_state = MENTAL_STATE_NEITHER;
    scenario->external_cause = MORAL_NO;
    scenario->external_force = MORAL_NO;
    scenario->control_perpetrator = MORAL_NO;
    scenario->careful = MORAL_YES; /* Assume careful by default */
    scenario->foreseeability = FORESEEABILITY_NONE;
    scenario->foresee_intervention = MORAL_NO;
    scenario->intervening_contribution = MORAL_NO;
    scenario->someone_else_cause_harm = MORAL_NO;
    scenario->outrank_perpetrator = MORAL_NO;
    scenario->severity_harm = 0;
    scenario->benefit_victim = 0;
    scenario->benefit_protagonist = MORAL_NO;
    scenario->achieve_goal = MORAL_NO;
    scenario->goal_outweigh_harm = MORAL_NO;
    scenario->goal_achieveable_less_harmful = MORAL_NO;
    scenario->monitor = MORAL_NO;

    /* Map action type to moral predicates */
    switch (action_type) {
        case MORAL_ACTION_ATTACK:
            scenario->produce_harm = MORAL_YES;
            scenario->sufficient_for_harm = MORAL_YES;
            scenario->plan_known = MORAL_YES;
            scenario->plan_include_harm = MORAL_YES;
            scenario->harm_caused_as_planned = MORAL_YES;
            scenario->mental_state = MENTAL_STATE_INTEND;
            scenario->foreseeability = FORESEEABILITY_HIGH;
            scenario->severity_harm = victim ? (GET_MAX_HIT(victim) / SEVERITY_SCALING_FACTOR) : DEFAULT_SEVERITY_HARM;

            /* Check if justified by alignment */
            if (victim && IS_EVIL(actor) && IS_GOOD(victim)) {
                scenario->achieve_goal = MORAL_YES;
                scenario->goal_outweigh_harm = MORAL_YES;
            } else if (victim && IS_GOOD(actor) && IS_EVIL(victim)) {
                scenario->achieve_goal = MORAL_YES;
                scenario->goal_outweigh_harm = MORAL_YES;
            }
            break;

        case MORAL_ACTION_STEAL:
            scenario->produce_harm = MORAL_YES;
            scenario->necessary_for_harm = MORAL_YES;
            scenario->plan_known = MORAL_YES;
            scenario->plan_include_harm = MORAL_YES;
            scenario->mental_state = MENTAL_STATE_INTEND;
            scenario->benefit_protagonist = MORAL_YES;
            scenario->foreseeability = FORESEEABILITY_HIGH;
            scenario->severity_harm = 3;
            break;

        case MORAL_ACTION_HELP:
        case MORAL_ACTION_HEAL:
            /* Helping actions are morally positive */
            scenario->benefit_victim = 5;
            scenario->achieve_goal = MORAL_YES;
            scenario->foreseeability = FORESEEABILITY_HIGH;
            break;

        case MORAL_ACTION_DECEIVE:
            scenario->produce_harm = MORAL_YES;
            scenario->plan_known = MORAL_YES;
            scenario->mental_state = MENTAL_STATE_INTEND;
            scenario->benefit_protagonist = MORAL_YES;
            scenario->foreseeability = FORESEEABILITY_LOW;
            scenario->severity_harm = 2;
            break;

        case MORAL_ACTION_BETRAY:
            scenario->produce_harm = MORAL_YES;
            scenario->sufficient_for_harm = MORAL_YES;
            scenario->plan_known = MORAL_YES;
            scenario->plan_include_harm = MORAL_YES;
            scenario->mental_state = MENTAL_STATE_INTEND;
            scenario->benefit_protagonist = MORAL_YES;
            scenario->foreseeability = FORESEEABILITY_HIGH;
            scenario->severity_harm = 7;
            break;

        case MORAL_ACTION_ABANDON_ALLY:
            scenario->necessary_for_harm = MORAL_YES;
            scenario->mental_state = MENTAL_STATE_RECKLESS;
            scenario->foreseeability = FORESEEABILITY_HIGH;
            scenario->severity_harm = 4;
            scenario->benefit_protagonist = MORAL_YES;
            break;

        case MORAL_ACTION_DEFEND:
            /* Defensive actions are morally justified */
            scenario->achieve_goal = MORAL_YES;
            scenario->goal_outweigh_harm = MORAL_YES;
            scenario->goal_achieveable_less_harmful = MORAL_NO;
            scenario->foreseeability = FORESEEABILITY_HIGH;
            break;

        case MORAL_ACTION_SACRIFICE_SELF:
            /* Self-sacrifice is morally praiseworthy */
            scenario->benefit_victim = 10;
            scenario->achieve_goal = MORAL_YES;
            scenario->goal_outweigh_harm = MORAL_YES;
            break;
    }

    /* Adjust based on actor's traits and emotions if available */
    if (IS_NPC(actor) && actor->ai_data) {
        /* Careful mobs are less likely to be reckless/negligent */
        /* Fear increases carefulness, anger decreases it */
        int carefulness = actor->ai_data->genetics.emotional_intelligence;
        carefulness += actor->ai_data->emotion_fear / 5;
        carefulness -= actor->ai_data->emotion_anger / 5;

        if (carefulness > 50) {
            scenario->careful = MORAL_YES;
        } else {
            scenario->careful = MORAL_NO;
        }

        /* Monitor behavior for mobs with high awareness or compassion */
        if (actor->ai_data->genetics.emotional_intelligence > 70 || actor->ai_data->emotion_compassion > 60) {
            scenario->monitor = MORAL_YES;
        }

        /* High anger can make actions more reckless */
        if (actor->ai_data->emotion_anger > 70 && !is_no(scenario->careful)) {
            scenario->mental_state = MENTAL_STATE_RECKLESS;
        }

        /* High compassion affects harm perception and benefit calculations */
        if (actor->ai_data->emotion_compassion > 70 && victim) {
            /* Compassionate mobs perceive harm more severely */
            scenario->severity_harm = (scenario->severity_harm * 120) / 100;
        }

        /* Greed affects benefit calculations */
        if (actor->ai_data->emotion_greed > 60) {
            scenario->benefit_protagonist = MORAL_YES;
        }

        /* Love and loyalty affect actions against loved ones */
        if (victim && (actor->ai_data->emotion_love > 70 || actor->ai_data->emotion_loyalty > 70)) {
            /* Check if victim is master or follower */
            if (victim == actor->master || victim->master == actor) {
                /* Harming loved ones increases severity dramatically */
                scenario->severity_harm = (scenario->severity_harm * 200) / 100;
            }
        }
    }
}

/**
 * Evaluate moral cost of a proposed action (internal helper)
 * @param actor The mob considering the action
 * @param victim The potential victim
 * @param action_type Type of action
 * @param include_group_dynamics Whether to include group dynamics (prevent recursion)
 * @return Moral cost (-100 to +100, negative is bad, positive is good)
 */
static int moral_evaluate_action_cost_internal(struct char_data *actor, struct char_data *victim, int action_type,
                                               bool include_group_dynamics)
{
    struct moral_scenario scenario;
    struct moral_judgment judgment;
    int moral_cost = 0;

    if (!actor)
        return 0;

    /* Build scenario from action */
    moral_build_scenario_from_action(actor, victim, action_type, &scenario);

    /* Evaluate moral judgment */
    moral_evaluate_guilt(&scenario, &judgment);

    /* Calculate cost based on judgment and actor's alignment */
    if (judgment.guilty == MORAL_JUDGMENT_GUILTY) {
        /* Guilty actions have negative moral cost */
        moral_cost = -judgment.blameworthiness_score;

        /* Good-aligned mobs are more averse to guilty actions */
        if (IS_GOOD(actor)) {
            moral_cost *= 2;
        }
        /* Evil-aligned mobs are less concerned */
        else if (IS_EVIL(actor)) {
            moral_cost /= 2;
        }

        /* Emotional modifiers for guilty actions */
        if (IS_NPC(actor) && actor->ai_data) {
            /* Shame and compassion increase aversion to guilty actions */
            if (actor->ai_data->emotion_shame > 50) {
                moral_cost -= (actor->ai_data->emotion_shame - 50) / 2;
            }
            if (actor->ai_data->emotion_compassion > 60) {
                moral_cost -= (actor->ai_data->emotion_compassion - 60) / 3;
            }

            /* Pride can reduce sensitivity to moral judgments */
            if (actor->ai_data->emotion_pride > 70) {
                moral_cost = (moral_cost * 80) / 100; /* 20% reduction */
            }

            /* Anger can override moral concerns */
            if (actor->ai_data->emotion_anger > 80) {
                moral_cost = (moral_cost * 60) / 100; /* 40% reduction when very angry */
            }

            /* Disgust amplifies moral aversion to certain actions */
            if (actor->ai_data->emotion_disgust > 60 &&
                (action_type == MORAL_ACTION_BETRAY || action_type == MORAL_ACTION_DECEIVE)) {
                moral_cost -= (actor->ai_data->emotion_disgust - 60) / 2;
            }
        }
    } else {
        /* Innocent/helpful actions have positive moral value */
        if (action_type == MORAL_ACTION_HELP || action_type == MORAL_ACTION_HEAL) {
            moral_cost = 30;
            if (IS_GOOD(actor))
                moral_cost += 20;

            /* Compassion and love increase value of helpful actions */
            if (IS_NPC(actor) && actor->ai_data) {
                if (actor->ai_data->emotion_compassion > 60) {
                    moral_cost += (actor->ai_data->emotion_compassion - 60) / 3;
                }
                if (actor->ai_data->emotion_love > 60) {
                    moral_cost += (actor->ai_data->emotion_love - 60) / 4;
                }
            }
        } else if (action_type == MORAL_ACTION_SACRIFICE_SELF) {
            moral_cost = 50;
            if (IS_GOOD(actor))
                moral_cost += 30;

            /* Loyalty and love dramatically increase value of self-sacrifice */
            if (IS_NPC(actor) && actor->ai_data) {
                if (actor->ai_data->emotion_loyalty > 70) {
                    moral_cost += (actor->ai_data->emotion_loyalty - 70) / 2;
                }
                if (actor->ai_data->emotion_love > 70) {
                    moral_cost += (actor->ai_data->emotion_love - 70) / 2;
                }
            }
        } else if (action_type == MORAL_ACTION_DEFEND) {
            moral_cost = 20;

            /* Courage and loyalty increase value of defense */
            if (IS_NPC(actor) && actor->ai_data) {
                if (actor->ai_data->emotion_courage > 60) {
                    moral_cost += (actor->ai_data->emotion_courage - 60) / 3;
                }
                if (actor->ai_data->emotion_loyalty > 60) {
                    moral_cost += (actor->ai_data->emotion_loyalty - 60) / 4;
                }
            }
        }
    }

    /* Apply memory-based moral learning bias */
    if (IS_NPC(actor) && actor->ai_data) {
        int learned_bias = moral_get_learned_bias(actor, action_type);

        /* Apply learned bias to moral cost */
        moral_cost += learned_bias;

        /* Strong learned avoidance can completely override current judgment */
        if (moral_has_learned_avoidance(actor, action_type)) {
            /* Add strong negative penalty for actions learned to be harmful */
            moral_cost -= 50;
        }
    }

    /* Apply group moral dynamics if actor is in a group (only if not disabled for recursion prevention) */
    if (include_group_dynamics && IS_NPC(actor) && actor->group && actor->group->members) {
        /* Calculate individual moral cost before group influence */
        int individual_cost = moral_cost;

        /* Peer pressure from group members */
        int peer_pressure = moral_get_peer_pressure(actor, action_type);
        moral_cost += peer_pressure;

        /* Group reputation affects interactions */
        if (victim && victim->group) {
            int reputation_mod = moral_get_group_reputation_modifier(actor->group, victim->group);
            moral_cost += reputation_mod;
        } else if (victim) {
            /* Single target, but acting group still has reputation */
            int reputation_mod = moral_get_group_reputation_modifier(actor->group, NULL);
            moral_cost += reputation_mod / 2; /* Reduced effect vs individuals */
        }

        /* Check for potential dissent */
        /* Calculate what the group's moral stance is (includes peer pressure) */
        int group_moral_cost = individual_cost + peer_pressure;

        /* If individual would strongly dissent from group's stance, reduce the group's influence */
        if (moral_would_dissent_from_group(actor, group_moral_cost, action_type, victim)) {
            /* Independent thinker - reduce peer pressure by 50% */
            moral_cost -= peer_pressure / 2;
        }
    }

    return moral_cost;
}

/**
 * Evaluate moral cost of a proposed action
 * Returns value for Shadow Timeline scoring: negative = immoral, positive = moral
 */
int moral_evaluate_action_cost(struct char_data *actor, struct char_data *victim, int action_type)
{
    return moral_evaluate_action_cost_internal(actor, victim, action_type, TRUE);
}

/**
 * Update mob's alignment based on moral judgment
 */
void moral_adjust_alignment(struct char_data *ch, struct moral_judgment *judgment)
{
    if (!ch || !judgment || !IS_NPC(ch))
        return;

    int alignment_change = 0;

    if (judgment->guilty == MORAL_JUDGMENT_GUILTY) {
        /* Guilty actions push toward evil */
        alignment_change = -judgment->blameworthiness_score / 10;
    } else if (judgment->responsibility_score > 50) {
        /* Responsible but innocent actions push toward good */
        alignment_change = judgment->responsibility_score / 20;
    }

    /* Apply change with bounds checking */
    GET_ALIGNMENT(ch) = MAX(-1000, MIN(1000, GET_ALIGNMENT(ch) + alignment_change));
}

/**
 * Update mob's reputation based on moral judgment
 */
void moral_adjust_reputation(struct char_data *ch, struct moral_judgment *judgment)
{
    if (!ch || !judgment || !IS_NPC(ch) || !ch->ai_data)
        return;

    int reputation_change = 0;

    if (judgment->guilty == MORAL_JUDGMENT_GUILTY) {
        /* Evil alignment gains reputation from guilty acts */
        if (IS_EVIL(ch)) {
            reputation_change = judgment->blameworthiness_score / 25;
        } else {
            /* Good/neutral lose reputation */
            reputation_change = -(judgment->blameworthiness_score / 20);
        }
    } else {
        /* Good alignment gains reputation from moral acts */
        if (IS_GOOD(ch) && judgment->responsibility_score > 30) {
            reputation_change = judgment->responsibility_score / 30;
        }
    }

    /* Apply change with bounds checking */
    ch->ai_data->reputation = MAX(0, MIN(100, ch->ai_data->reputation + reputation_change));
}

/**
 * Update mob's emotions based on moral judgment
 * Emotional responses to moral actions create realistic psychological feedback
 */
void moral_adjust_emotions(struct char_data *ch, struct moral_judgment *judgment)
{
    if (!ch || !judgment || !IS_NPC(ch) || !ch->ai_data)
        return;

    /* Guilty actions trigger shame and potentially other negative emotions */
    if (judgment->guilty == MORAL_JUDGMENT_GUILTY) {
        /* Shame increases for guilty actions, especially for good-aligned mobs */
        int shame_increase = judgment->blameworthiness_score / 10;
        if (IS_GOOD(ch)) {
            shame_increase = (shame_increase * 150) / 100; /* 50% more shame for good mobs */
        } else if (IS_EVIL(ch)) {
            shame_increase = (shame_increase * 40) / 100; /* Evil mobs feel less shame */
        }
        ch->ai_data->emotion_shame = MIN(100, ch->ai_data->emotion_shame + shame_increase);

        /* Disgust with self for particularly egregious actions */
        if (judgment->blameworthiness_score > 70) {
            ch->ai_data->emotion_disgust = MIN(100, ch->ai_data->emotion_disgust + 5);
        }

        /* Guilt reduces happiness */
        ch->ai_data->emotion_happiness = MAX(0, ch->ai_data->emotion_happiness - shame_increase / 2);

        /* If action harmed loved ones, increase sadness and reduce love/loyalty */
        if (judgment->was_responsible && judgment->blameworthiness_score > 50) {
            ch->ai_data->emotion_sadness = MIN(100, ch->ai_data->emotion_sadness + shame_increase / 3);
        }

        /* Pride might decrease for good-aligned mobs who act badly */
        if (IS_GOOD(ch)) {
            ch->ai_data->emotion_pride = MAX(0, ch->ai_data->emotion_pride - shame_increase / 2);
        }
    } else {
        /* Innocent/moral actions trigger positive emotions */

        /* Pride increases for morally good actions */
        if (judgment->responsibility_score > 50) {
            int pride_increase = judgment->responsibility_score / 15;
            if (IS_GOOD(ch)) {
                pride_increase = (pride_increase * 120) / 100;
            }
            ch->ai_data->emotion_pride = MIN(100, ch->ai_data->emotion_pride + pride_increase);

            /* Happiness increases from doing good */
            ch->ai_data->emotion_happiness = MIN(100, ch->ai_data->emotion_happiness + pride_increase / 2);

            /* Shame decreases when doing good deeds */
            ch->ai_data->emotion_shame = MAX(0, ch->ai_data->emotion_shame - pride_increase / 3);
        }

        /* Compassionate actions reinforce compassion */
        if (judgment->responsibility_score > 40 && !judgment->was_justified) {
            /* Pure altruistic actions increase compassion */
            ch->ai_data->emotion_compassion = MIN(100, ch->ai_data->emotion_compassion + 2);
        }
    }

    /* Vicarious responsibility creates complex emotions */
    if (judgment->was_vicarious) {
        /* Shame for failing to prevent harm by subordinates */
        ch->ai_data->emotion_shame = MIN(100, ch->ai_data->emotion_shame + 5);
        /* Anger at subordinate who caused the problem */
        ch->ai_data->emotion_anger = MIN(100, ch->ai_data->emotion_anger + 3);
    }
}

/**
 * Evaluate and record a completed moral action for learning
 * Convenience wrapper that evaluates judgment and stores in memory
 */
void moral_record_action(struct char_data *actor, struct char_data *target, int action_type)
{
    struct moral_scenario scenario;
    struct moral_judgment judgment;

    if (!actor || !IS_NPC(actor) || !actor->ai_data)
        return;

    /* Build scenario from the completed action */
    moral_build_scenario_from_action(actor, target, action_type, &scenario);

    /* Evaluate moral judgment */
    moral_evaluate_guilt(&scenario, &judgment);

    /* Store judgment in memory for learning */
    moral_store_judgment_in_memory(actor, target, action_type, &judgment);

    /* Apply standard moral adjustments (alignment, reputation) */
    moral_adjust_alignment(actor, &judgment);
    moral_adjust_reputation(actor, &judgment);
}

/**
 * Check if action is morally acceptable to this mob
 * Used to filter actions based on moral identity
 */
bool moral_is_action_acceptable(struct char_data *ch, int action_type)
{
    if (!ch || !IS_NPC(ch))
        return TRUE;

    int alignment = GET_ALIGNMENT(ch);

    /* Strong good alignment mobs reject harmful actions */
    if (alignment > 500) {
        switch (action_type) {
            case MORAL_ACTION_ATTACK:
            case MORAL_ACTION_STEAL:
            case MORAL_ACTION_DECEIVE:
            case MORAL_ACTION_BETRAY:
            case MORAL_ACTION_ABANDON_ALLY:
                return FALSE;
        }
    }

    /* Strong evil alignment mobs reject altruistic actions */
    if (alignment < -500) {
        switch (action_type) {
            case MORAL_ACTION_SACRIFICE_SELF:
                return FALSE;
            case MORAL_ACTION_HELP:
            case MORAL_ACTION_HEAL:
                /* Only reject if no personal benefit */
                return FALSE;
        }
    }

    return TRUE;
}

/* ========================================================================== */
/*                      MEMORY-BASED MORAL LEARNING                           */
/* ========================================================================== */

/**
 * Store moral judgment in emotion memory system
 * This links the moral reasoning system with the emotion memory system
 * to enable learning from past moral decisions
 */
void moral_store_judgment_in_memory(struct char_data *mob, struct char_data *target, int action_type,
                                    struct moral_judgment *judgment)
{
    struct emotion_memory *memory;
    int pre_shame, pre_disgust, pre_happiness;

    if (!mob || !IS_NPC(mob) || !mob->ai_data || !judgment)
        return;

    /* Store pre-action emotional state for regret calculation */
    pre_shame = mob->ai_data->emotion_shame;
    pre_disgust = mob->ai_data->emotion_disgust;
    pre_happiness = mob->ai_data->emotion_happiness;

    /* First, store the interaction in emotion memory (if there's a target) */
    /* This creates the memory entry with the current emotional state */
    if (target) {
        /* Determine interaction type based on action */
        int interaction_type = INTERACT_ATTACKED; /* Default */
        int major_event = judgment->guilty ? 1 : 0;

        switch (action_type) {
            case MORAL_ACTION_ATTACK:
                interaction_type = INTERACT_ATTACKED;
                break;
            case MORAL_ACTION_STEAL:
                interaction_type = INTERACT_STOLEN_FROM;
                major_event = 1;
                break;
            case MORAL_ACTION_HELP:
            case MORAL_ACTION_HEAL:
                interaction_type = INTERACT_ASSISTED;
                break;
            case MORAL_ACTION_BETRAY:
                interaction_type = INTERACT_ATTACKED;
                major_event = 1;
                break;
            case MORAL_ACTION_DEFEND:
                interaction_type = INTERACT_ATTACKED;
                break;
        }

        /* Store interaction memory (this will create the base memory entry) */
        add_emotion_memory(mob, target, interaction_type, major_event, NULL);
    } else {
        /* No target - create a memory entry manually for the action */
        memory = &mob->ai_data->memories[mob->ai_data->memory_index];

        /* Initialize memory slot */
        memory->entity_type = ENTITY_TYPE_MOB;
        memory->entity_id = 0; /* No specific target */
        memory->interaction_type = INTERACT_ATTACKED;
        memory->major_event = judgment->guilty ? 1 : 0;
        memory->timestamp = time(0);
        memory->social_name[0] = '\0';

        /* Store emotion snapshot */
        memory->fear_level = mob->ai_data->emotion_fear;
        memory->anger_level = mob->ai_data->emotion_anger;
        memory->happiness_level = mob->ai_data->emotion_happiness;
        memory->sadness_level = mob->ai_data->emotion_sadness;
        memory->friendship_level = mob->ai_data->emotion_friendship;
        memory->love_level = mob->ai_data->emotion_love;
        memory->trust_level = mob->ai_data->emotion_trust;
        memory->loyalty_level = mob->ai_data->emotion_loyalty;
        memory->curiosity_level = mob->ai_data->emotion_curiosity;
        memory->greed_level = mob->ai_data->emotion_greed;
        memory->pride_level = mob->ai_data->emotion_pride;
        memory->compassion_level = mob->ai_data->emotion_compassion;
        memory->envy_level = mob->ai_data->emotion_envy;
        memory->courage_level = mob->ai_data->emotion_courage;
        memory->excitement_level = mob->ai_data->emotion_excitement;
        memory->disgust_level = mob->ai_data->emotion_disgust;
        memory->shame_level = mob->ai_data->emotion_shame;
        memory->pain_level = mob->ai_data->emotion_pain;
        memory->horror_level = mob->ai_data->emotion_horror;
        memory->humiliation_level = mob->ai_data->emotion_humiliation;

        /* Advance circular buffer */
        mob->ai_data->memory_index = (mob->ai_data->memory_index + 1) % EMOTION_MEMORY_SIZE;
    }

    /* Now update the most recent memory with moral judgment data */
    /* The memory_index was just incremented, so we need the previous slot */
    int prev_index = (mob->ai_data->memory_index - 1 + EMOTION_MEMORY_SIZE) % EMOTION_MEMORY_SIZE;
    memory = &mob->ai_data->memories[prev_index];

    /* Store moral judgment information */
    memory->moral_action_type = action_type;
    memory->moral_was_guilty = judgment->guilty;
    memory->moral_blameworthiness = judgment->blameworthiness_score;

    /* Calculate outcome severity based on judgment */
    /* Higher blameworthiness = more severe moral outcome */
    memory->moral_outcome_severity = judgment->blameworthiness_score;

    /* Calculate regret from emotional changes that occurred */
    /* We need to apply the moral judgment's emotional effects first */
    moral_adjust_emotions(mob, judgment);

    /* Now calculate regret based on emotional state change */
    memory->moral_regret_level = moral_calculate_regret(mob, pre_shame, pre_disgust, pre_happiness);

    /* Back-fill the corresponding active memory slot.
     * When the mob was the actor (which is always the case when moral_store_judgment_in_memory
     * is called), add_active_emotion_memory() should have recorded the action just before
     * the moral evaluation.  Find the most recently written active memory slot that matches
     * the interaction type and hasn't been judged yet, then stamp it with the same moral data.
     * We walk backwards from active_memory_index so we catch the freshest entry first. */
    {
        int active_interact;
        int ai;
        int found_active;
        time_t now_t;

        /* Map action_type → INTERACT_* the same way we did for passive memory */
        active_interact = INTERACT_ATTACKED; /* default */
        switch (action_type) {
            case MORAL_ACTION_ATTACK:
            case MORAL_ACTION_BETRAY:
            case MORAL_ACTION_DEFEND:
                active_interact = INTERACT_ATTACKED;
                break;
            case MORAL_ACTION_STEAL:
                active_interact = INTERACT_STOLEN_FROM;
                break;
            case MORAL_ACTION_HELP:
            case MORAL_ACTION_HEAL:
                active_interact = INTERACT_ASSISTED;
                break;
        }

        found_active = 0;
        now_t = time(0);

        for (ai = 0; ai < EMOTION_MEMORY_SIZE && !found_active; ai++) {
            /* Walk backwards from the slot just written */
            int slot = (mob->ai_data->active_memory_index - 1 - ai + EMOTION_MEMORY_SIZE * 2) % EMOTION_MEMORY_SIZE;
            struct emotion_memory *amem = &mob->ai_data->active_memories[slot];

            /* Only consider recent, unjudged, type-matching entries */
            if (amem->timestamp == 0)
                continue;
            if (amem->interaction_type != active_interact)
                continue;
            if (amem->moral_was_guilty >= 0)
                continue; /* already judged */
            if ((int)(now_t - amem->timestamp) > 60)
                continue; /* more than 60 s old — wrong action */

            amem->moral_action_type = action_type;
            amem->moral_was_guilty = judgment->guilty;
            amem->moral_blameworthiness = judgment->blameworthiness_score;
            amem->moral_outcome_severity = judgment->blameworthiness_score;
            amem->moral_regret_level = memory->moral_regret_level; /* reuse computed value */
            found_active = 1;
        }
    }
}

/**
 * Calculate regret level based on emotional changes after action
 * Higher regret = action caused negative emotional consequences
 */
int moral_calculate_regret(struct char_data *ch, int pre_shame, int pre_disgust, int pre_happiness)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data)
        return 0;

    int regret = 0;

    /* Increased shame indicates regret */
    int shame_increase = ch->ai_data->emotion_shame - pre_shame;
    if (shame_increase > 0) {
        regret += shame_increase * 2; /* Shame is strong indicator of regret */
    }

    /* Increased disgust indicates regret (with self) */
    int disgust_increase = ch->ai_data->emotion_disgust - pre_disgust;
    if (disgust_increase > 0) {
        regret += disgust_increase;
    }

    /* Decreased happiness indicates regret */
    int happiness_decrease = pre_happiness - ch->ai_data->emotion_happiness;
    if (happiness_decrease > 0) {
        regret += happiness_decrease / 2;
    }

    /* Bound to 0-100 range */
    return MIN(100, MAX(0, regret));
}

/**
 * Get count of past guilty vs innocent judgments for action type
 * Scans emotion memories to find patterns
 */
void moral_get_action_history(struct char_data *ch, int action_type, int *out_guilty, int *out_innocent)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data || !out_guilty || !out_innocent)
        return;

    *out_guilty = 0;
    *out_innocent = 0;

    time_t current_time = time(0);

    /* Scan all memories for this action type */
    for (int i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        struct emotion_memory *mem = &ch->ai_data->memories[i];

        /* Skip empty/old slots */
        if (mem->timestamp == 0)
            continue;

        /* Only count recent memories (within last 30 minutes) */
        int age_seconds = current_time - mem->timestamp;
        if (age_seconds > 1800) /* 30 minutes */
            continue;

        /* Check if this memory is about the action type we care about */
        if (mem->moral_action_type == action_type && mem->moral_was_guilty >= 0) {
            if (mem->moral_was_guilty == MORAL_JUDGMENT_GUILTY) {
                (*out_guilty)++;
            } else {
                (*out_innocent)++;
            }
        }
    }
}

/**
 * Calculate moral learning bias from past actions
 * Mobs learn to avoid actions that resulted in guilt and regret
 * Mobs are encouraged to repeat actions that were innocent and positive
 */
int moral_get_learned_bias(struct char_data *ch, int action_type)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data)
        return 0;

    int total_weight = 0;
    int weighted_bias = 0;
    time_t current_time = time(0);

    /* Scan memories for this action type */
    for (int i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        struct emotion_memory *mem = &ch->ai_data->memories[i];

        /* Skip empty slots or entries without moral judgment */
        if (mem->timestamp == 0 || mem->moral_action_type != action_type || mem->moral_was_guilty < 0)
            continue;

        /* Calculate memory age and weight */
        int age_seconds = current_time - mem->timestamp;

        /* Skip very old memories (beyond 1 hour) */
        if (age_seconds > 3600)
            continue;

        /* Recent memories have more weight */
        int weight = 100;
        if (age_seconds < 300) { /* < 5 minutes */
            weight = 100;
        } else if (age_seconds < 900) { /* < 15 minutes */
            weight = 80;
        } else if (age_seconds < 1800) { /* < 30 minutes */
            weight = 60;
        } else { /* < 60 minutes */
            weight = 40;
        }

        /* Major events have double weight */
        if (mem->major_event) {
            weight *= 2;
        }

        total_weight += weight;

        /* Calculate bias from this memory */
        int memory_bias = 0;

        if (mem->moral_was_guilty == MORAL_JUDGMENT_GUILTY) {
            /* Guilty actions generate negative bias (avoid repeating) */
            memory_bias = -mem->moral_blameworthiness;

            /* High regret amplifies the negative bias */
            if (mem->moral_regret_level > 50) {
                memory_bias -= (mem->moral_regret_level - 50) / 2;
            }

            /* High outcome severity amplifies avoidance */
            if (mem->moral_outcome_severity > 60) {
                memory_bias -= (mem->moral_outcome_severity - 60) / 3;
            }
        } else {
            /* Innocent actions generate positive bias (encourage repeating) */
            memory_bias = 30; /* Base encouragement for innocent actions */

            /* Low regret increases positive bias */
            if (mem->moral_regret_level < 20) {
                memory_bias += 10;
            }

            /* Positive emotional outcomes increase bias */
            if (mem->happiness_level > 60) {
                memory_bias += (mem->happiness_level - 60) / 5;
            }
        }

        /* Accumulate weighted bias */
        weighted_bias += memory_bias * weight;
    }

    /* Also scan active memories (actor-perspective) at 30% weight so self-
     * initiated actions contribute to learned bias alongside passive ones. */
    for (int i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        struct emotion_memory *mem = &ch->ai_data->active_memories[i];

        if (mem->timestamp == 0 || mem->moral_action_type != action_type || mem->moral_was_guilty < 0)
            continue;

        int age_seconds = current_time - mem->timestamp;
        if (age_seconds > 3600)
            continue;

        int weight = 100;
        if (age_seconds < 300) {
            weight = 100;
        } else if (age_seconds < 900) {
            weight = 80;
        } else if (age_seconds < 1800) {
            weight = 60;
        } else {
            weight = 40;
        }

        if (mem->major_event)
            weight *= 2;

        /* Active memories contribute at 30% of passive weight */
        weight = weight * 3 / 10;
        if (weight == 0)
            continue;

        total_weight += weight;

        int memory_bias = 0;
        if (mem->moral_was_guilty == MORAL_JUDGMENT_GUILTY) {
            memory_bias = -mem->moral_blameworthiness;
            if (mem->moral_regret_level > 50)
                memory_bias -= (mem->moral_regret_level - 50) / 2;
            if (mem->moral_outcome_severity > 60)
                memory_bias -= (mem->moral_outcome_severity - 60) / 3;
        } else {
            memory_bias = 30;
            if (mem->moral_regret_level < 20)
                memory_bias += 10;
            if (mem->happiness_level > 60)
                memory_bias += (mem->happiness_level - 60) / 5;
        }

        weighted_bias += memory_bias * weight;
    }

    /* Calculate final bias */
    if (total_weight == 0)
        return 0;

    int final_bias = weighted_bias / total_weight;

    /* Clamp to reasonable range */
    return MAX(-100, MIN(100, final_bias));
}

/**
 * Check if mob has learned to strongly avoid this action type
 * Returns TRUE if past experiences strongly suggest avoiding
 */
bool moral_has_learned_avoidance(struct char_data *ch, int action_type)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data)
        return FALSE;

    int guilty_count = 0;
    int innocent_count = 0;
    int high_regret_count = 0;

    moral_get_action_history(ch, action_type, &guilty_count, &innocent_count);

    /* Scan for high regret instances */
    time_t current_time = time(0);
    for (int i = 0; i < EMOTION_MEMORY_SIZE; i++) {
        struct emotion_memory *mem = &ch->ai_data->memories[i];

        if (mem->timestamp == 0 || mem->moral_action_type != action_type)
            continue;

        int age_seconds = current_time - mem->timestamp;
        if (age_seconds > 1800) /* 30 minutes */
            continue;

        if (mem->moral_regret_level > 70) {
            high_regret_count++;
        }
    }

    /* Strong avoidance learned if:
     * - At least 2 guilty judgments with no innocent ones, OR
     * - At least 3 instances with high regret
     */
    if (guilty_count >= 2 && innocent_count == 0)
        return TRUE;

    if (high_regret_count >= 3)
        return TRUE;

    return FALSE;
}

/* ========================================================================== */
/*                      GROUP MORAL DYNAMICS IMPLEMENTATION                   */
/* ========================================================================== */

/**
 * Initialize group moral reputation for new groups
 * Sets default values based on initial members' alignments
 */
void moral_init_group_reputation(struct group_data *group)
{
    if (!group || !group->members)
        return;

    /* Start with neutral reputation */
    group->moral_reputation = 50;
    group->collective_guilt_count = 0;
    group->collective_good_count = 0;
    group->last_moral_action = 0;

    /* Adjust based on leader's alignment if present */
    if (group->leader && IS_NPC(group->leader)) {
        int alignment = GET_ALIGNMENT(group->leader);
        if (alignment > 500) {
            group->moral_reputation = 70; /* Good leader = good reputation */
        } else if (alignment < -500) {
            group->moral_reputation = 30; /* Evil leader = bad reputation */
        }
    }
}

/**
 * Calculate peer pressure influence on moral decision
 * Considers group members' alignments, emotions, and moral histories
 */
int moral_get_peer_pressure(struct char_data *ch, int action_type)
{
    if (!ch || !IS_NPC(ch) || !ch->group || !ch->group->members)
        return 0;

    int total_pressure = 0;
    int member_count = 0;
    struct list_data *members = ch->group->members;

    /* Reset iterator before iterating group members */
    clear_simple_list();

    /* Iterate through group members */
    for (struct char_data *member = (struct char_data *)simple_list(members); member;
         member = (struct char_data *)simple_list(NULL)) {

        /* Skip self */
        if (member == ch)
            continue;

        /* Skip non-NPCs or members without AI */
        if (!IS_NPC(member) || !member->ai_data)
            continue;

        member_count++;

        /* Calculate this member's moral stance on the action */
        int member_stance = 0;

        /* Alignment-based stance */
        int alignment = GET_ALIGNMENT(member);
        switch (action_type) {
            case MORAL_ACTION_ATTACK:
            case MORAL_ACTION_STEAL:
            case MORAL_ACTION_BETRAY:
                /* Evil approves, good disapproves */
                if (alignment < -300)
                    member_stance += 20;
                else if (alignment > 300)
                    member_stance -= 20;
                break;

            case MORAL_ACTION_HELP:
            case MORAL_ACTION_HEAL:
            case MORAL_ACTION_SACRIFICE_SELF:
                /* Good approves, evil disapproves */
                if (alignment > 300)
                    member_stance += 20;
                else if (alignment < -300)
                    member_stance -= 20;
                break;
        }

        /* Emotion-based stance */
        /* High loyalty increases conformity pressure */
        if (member->ai_data->emotion_loyalty > 70) {
            member_stance = (member_stance * 150) / 100; /* 50% more influence */
        }

        /* High compassion opposes harmful actions */
        if (member->ai_data->emotion_compassion > 70 &&
            (action_type == MORAL_ACTION_ATTACK || action_type == MORAL_ACTION_BETRAY)) {
            member_stance -= 15;
        }

        /* High courage encourages defensive/protective actions */
        if (member->ai_data->emotion_courage > 70 && action_type == MORAL_ACTION_DEFEND) {
            member_stance += 15;
        }

        /* Check member's learned biases */
        int learned_bias = moral_get_learned_bias(member, action_type);
        /* Reduce learned bias influence (it's personal, not group pressure) */
        member_stance += learned_bias / 3;

        total_pressure += member_stance;
    }

    /* Average the pressure */
    if (member_count == 0)
        return 0;

    int average_pressure = total_pressure / member_count;

    /* Leader has extra influence (but leaders don't influence themselves) */
    if (ch != ch->group->leader) {
        int leader_bonus = moral_get_leader_influence(ch, ch->group->leader, action_type);
        average_pressure += leader_bonus;
    }

    /* Group size amplifies pressure (larger groups = more conformity) */
    if (member_count >= 5) {
        average_pressure = (average_pressure * 120) / 100; /* 20% boost */
    } else if (member_count >= 3) {
        average_pressure = (average_pressure * 110) / 100; /* 10% boost */
    }

    /* Clamp to reasonable range */
    return MAX(-100, MIN(100, average_pressure));
}

/**
 * Calculate moral conformity pressure from leader
 * Strong leaders can override individual moral judgments
 */
int moral_get_leader_influence(struct char_data *follower, struct char_data *leader, int action_type)
{
    if (!follower || !leader || !IS_NPC(follower) || !IS_NPC(leader))
        return 0;

    if (!follower->ai_data || !leader->ai_data)
        return 0;

    int influence = 0;

    /* Level difference increases influence */
    int level_diff = GET_LEVEL(leader) - GET_LEVEL(follower);
    influence += MIN(20, MAX(0, level_diff * 2));

    /* Follower's loyalty increases susceptibility to influence */
    int loyalty = follower->ai_data->emotion_loyalty;
    if (loyalty > 80) {
        influence += 15;
    } else if (loyalty > 60) {
        influence += 10;
    } else if (loyalty < 30) {
        influence -= 10; /* Low loyalty reduces influence */
    }

    /* Follower's emotional intelligence affects independence */
    int ei = follower->ai_data->genetics.emotional_intelligence;
    if (ei > 80) {
        /* High EI = more independent thinking */
        influence = (influence * 70) / 100;
    } else if (ei < 30) {
        /* Low EI = more susceptible to influence */
        influence = (influence * 130) / 100;
    }

    /* Leader's reputation amplifies influence */
    if (leader->ai_data->reputation > 70) {
        influence = (influence * 120) / 100;
    }

    /* Leader's alignment influences moral direction */
    int leader_alignment = GET_ALIGNMENT(leader);
    if ((action_type == MORAL_ACTION_ATTACK || action_type == MORAL_ACTION_BETRAY) && leader_alignment < -500) {
        /* Evil leader encourages harmful actions */
        influence = abs(influence); /* Make it positive (encouraging) */
    } else if ((action_type == MORAL_ACTION_HELP || action_type == MORAL_ACTION_HEAL) && leader_alignment > 500) {
        /* Good leader encourages helpful actions */
        influence = abs(influence);
    }

    return MAX(-50, MIN(50, influence));
}

/**
 * Check if mob would dissent from group moral decision
 * Strong moral convictions can cause dissent
 */
bool moral_would_dissent_from_group(struct char_data *ch, int group_action_cost, int action_type,
                                    struct char_data *victim)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data)
        return FALSE;

    /* Calculate individual moral stance with victim context */
    /* Use internal version without group dynamics to prevent infinite recursion */
    int individual_cost = moral_evaluate_action_cost_internal(ch, victim, action_type, FALSE);

    /* If individual strongly opposes but group encourages, potential dissent */
    if (individual_cost < -60 && group_action_cost > 20) {
        /* Check if moral conviction is strong enough */

        /* High compassion + good alignment = strong dissent potential */
        if (ch->ai_data->emotion_compassion > 70 && GET_ALIGNMENT(ch) > 500) {
            return TRUE;
        }

        /* Learned avoidance = strong conviction */
        if (moral_has_learned_avoidance(ch, action_type)) {
            return TRUE;
        }

        /* High emotional intelligence = more independent moral reasoning */
        if (ch->ai_data->genetics.emotional_intelligence > 80) {
            return TRUE;
        }
    }

    /* Low loyalty makes dissent more likely */
    if (ch->ai_data->emotion_loyalty < 30 && abs(individual_cost - group_action_cost) > 50) {
        return TRUE;
    }

    return FALSE;
}

/**
 * Evaluate collective moral responsibility for group action
 * All group members share in the moral judgment
 */
bool moral_evaluate_group_action(struct char_data *leader, struct char_data *victim, int action_type,
                                 struct moral_judgment *judgment)
{
    if (!leader || !judgment)
        return FALSE;

    /* Evaluate the action using standard moral reasoning */
    struct moral_scenario scenario;
    moral_build_scenario_from_action(leader, victim, action_type, &scenario);
    moral_evaluate_guilt(&scenario, judgment);

    /* Check if this is truly a group action */
    if (!leader->group || !leader->group->members)
        return FALSE; /* No group, no collective responsibility */

    /* Group action if:
     * 1. Leader initiated it, OR
     * 2. Multiple members participated
     */

    /* For now, treat any action by grouped mob as group action */
    /* TODO: Could add more sophisticated "group action" detection */

    return TRUE; /* Group shares responsibility */
}

/**
 * Update group moral reputation based on collective action
 * Group reputation affects how others view and interact with the group
 */
void moral_update_group_reputation(struct group_data *group, struct moral_judgment *judgment)
{
    if (!group || !judgment)
        return;

    int reputation_change = 0;

    if (judgment->guilty == MORAL_JUDGMENT_GUILTY) {
        /* Guilty group actions damage reputation */
        group->collective_guilt_count++;
        reputation_change = -(judgment->blameworthiness_score / 10);

        /* Repeated guilty actions compound reputation loss */
        if (group->collective_guilt_count > 5) {
            reputation_change -= 5;
        }
    } else {
        /* Innocent/moral group actions improve reputation */
        if (judgment->responsibility_score > 50) {
            group->collective_good_count++;
            reputation_change = judgment->responsibility_score / 15;

            /* Consistent moral behavior improves reputation more */
            if (group->collective_good_count > 5) {
                reputation_change += 3;
            }
        }
    }

    /* Apply reputation change with bounds */
    group->moral_reputation = MAX(0, MIN(100, group->moral_reputation + reputation_change));

    /* Track last moral action timestamp */
    group->last_moral_action = time(0);
}

/**
 * Get group moral reputation modifier for inter-group interactions
 * Groups with bad reputations face penalties, good reputations get bonuses
 */
int moral_get_group_reputation_modifier(struct group_data *acting_group, struct group_data *target_group)
{
    if (!acting_group)
        return 0;

    int modifier = 0;

    /* Acting group's reputation affects their actions */
    int reputation = acting_group->moral_reputation;

    if (reputation > 70) {
        /* Good reputation = positive interactions */
        modifier = (reputation - 70) / 3; /* +0 to +10 */
    } else if (reputation < 30) {
        /* Bad reputation = negative interactions */
        modifier = (reputation - 30) / 2; /* -15 to 0 */
    }

    /* If there's a target group, consider their reputation too */
    if (target_group) {
        int target_rep = target_group->moral_reputation;

        /* Good groups are less likely to attack other good groups */
        if (reputation > 60 && target_rep > 60) {
            modifier += 20; /* Strong positive modifier to prevent good-vs-good fights */
        }

        /* Evil groups prey on good groups */
        if (reputation < 40 && target_rep > 60) {
            modifier -= 10; /* Encouraged to attack good groups */
        }

        /* Reputation difference matters */
        int rep_diff = reputation - target_rep;
        modifier += rep_diff / 10; /* +/- based on reputation difference */
    }

    return MAX(-50, MIN(50, modifier));
}

/**
 * Record group action in all members' moral memories
 * Distributes collective responsibility across group
 */
void moral_record_group_action(struct group_data *group, struct char_data *victim, int action_type,
                               struct moral_judgment *judgment)
{
    if (!group || !group->members || !judgment)
        return;

    /* Reset simple_list iterator before iterating group members */
    clear_simple_list();

    /* Iterate through all group members */
    for (struct char_data *member = (struct char_data *)simple_list(group->members); member;
         member = (struct char_data *)simple_list(NULL)) {

        /* Skip non-NPCs or members without AI */
        if (!IS_NPC(member) || !member->ai_data)
            continue;

        /* Record in individual moral memory */
        /* Note: This uses the same judgment for all members (collective responsibility) */
        moral_store_judgment_in_memory(member, victim, action_type, judgment);

        /* Adjust individual alignment based on group action */
        moral_adjust_alignment(member, judgment);

        /* Group leader gets extra reputation impact */
        if (member == group->leader) {
            moral_adjust_reputation(member, judgment);
        }
    }

    /* Update group reputation */
    moral_update_group_reputation(group, judgment);
}
