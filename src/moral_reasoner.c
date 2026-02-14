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

/* Severity calculation constants */
#define SEVERITY_SCALING_FACTOR 10 /* Divisor for HP-based severity calculation */
#define DEFAULT_SEVERITY_HARM 5    /* Default severity when victim is unknown */

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
 * Evaluate moral cost of a proposed action
 * Returns value for Shadow Timeline scoring: negative = immoral, positive = moral
 */
int moral_evaluate_action_cost(struct char_data *actor, struct char_data *victim, int action_type)
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

    return moral_cost;
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
