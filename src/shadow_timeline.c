/**
 * @file shadow_timeline.c
 * Shadow Timeline: Cognitive Future Simulation Layer (RFC-0001)
 *
 * Implementation of the Shadow Timeline system for cognitive future simulation.
 * This system allows autonomous entities to internally explore possible future
 * outcomes without modifying the real world state.
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "shadow_timeline.h"
#include "act.h"
#include "fight.h"

/* External variables */
extern struct room_data *world;

/* File-local function prototypes */
static void generate_movement_projections(struct shadow_context *ctx);
static void generate_combat_projections(struct shadow_context *ctx);
static void generate_social_projections(struct shadow_context *ctx);
static void generate_item_projections(struct shadow_context *ctx);
static int score_projection_for_entity(struct char_data *ch, struct shadow_projection *proj);
static bool check_invariant_existence(void *entity, int entity_type);
static bool check_invariant_location(struct char_data *ch, room_rnum room);
static bool check_invariant_action(struct char_data *ch, enum shadow_action_type type);

/**
 * Initialize a shadow context for an entity
 * Allocates memory and sets up projection tracking
 */
struct shadow_context *shadow_init_context(struct char_data *ch)
{
    struct shadow_context *ctx;

    if (!IS_COGNITIVE_ENTITY(ch)) {
        return NULL;
    }

    CREATE(ctx, struct shadow_context, 1);
    if (!ctx) {
        return NULL;
    }

    ctx->entity = ch;
    ctx->max_projections = SHADOW_MAX_PROJECTIONS;
    CREATE(ctx->projections, struct shadow_projection, ctx->max_projections);

    if (!ctx->projections) {
        free(ctx);
        return NULL;
    }

    ctx->num_projections = 0;
    ctx->horizon = SHADOW_DEFAULT_HORIZON;
    ctx->active = TRUE;

    /* Set cognitive budget based on entity's capacity */
    if (IS_NPC(ch) && ch->ai_data) {
        ctx->cognitive_budget = ch->ai_data->cognitive_capacity;
    } else {
        ctx->cognitive_budget = COGNITIVE_CAPACITY_MAX;
    }

    return ctx;
}

/**
 * Free a shadow context and all associated projections
 * ST-1: Ensures no memory leaks in shadow system
 */
void shadow_free_context(struct shadow_context *ctx)
{
    if (!ctx) {
        return;
    }

    if (ctx->projections) {
        free(ctx->projections);
    }

    free(ctx);
}

/**
 * Generate projections for available actions
 * Core function implementing bounded cognition (ST-3)
 */
int shadow_generate_projections(struct shadow_context *ctx)
{
    if (!ctx || !ctx->active || !ctx->entity) {
        return 0;
    }

    struct char_data *ch = ctx->entity;

    /* Reset projection count */
    ctx->num_projections = 0;

    /* Check if entity has enough cognitive capacity */
    if (ctx->cognitive_budget < COGNITIVE_CAPACITY_MIN) {
        shadow_log(ctx, "Insufficient cognitive capacity for projections");
        return 0;
    }

    /* Generate different types of projections based on context */
    /* Bounded by cognitive capacity - won't generate exhaustive search */

    if (FIGHTING(ch)) {
        /* In combat - focus on combat projections */
        generate_combat_projections(ctx);
    } else if (ch->ai_data && ch->ai_data->current_goal != 0) {
        /* Has active goal - focus on goal-relevant actions */
        generate_movement_projections(ctx);
        generate_item_projections(ctx);
    } else {
        /* Exploring - generate diverse projections */
        generate_movement_projections(ctx);
        generate_social_projections(ctx);
        generate_item_projections(ctx);
    }

    /* Score and rank projections based on entity's subjective evaluation */
    shadow_score_projections(ctx);

    return ctx->num_projections;
}

/**
 * Generate movement action projections
 * Projects possible movements to adjacent rooms
 */
static void generate_movement_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct shadow_action action;
    struct shadow_outcome outcome;
    int dir;

    if (!ch || ch->in_room == NOWHERE) {
        return;
    }

    /* Try each direction */
    for (dir = 0; dir < NUM_OF_DIRS && ctx->num_projections < ctx->max_projections; dir++) {
        if (!CAN_GO(ch, dir)) {
            continue;
        }

        /* Set up movement action */
        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_MOVE;
        action.direction = dir;
        action.target = NULL;
        action.cost = SHADOW_BASE_COST;

        /* Validate action */
        if (shadow_validate_action(ch, &action) != ACTION_FEASIBLE) {
            continue;
        }

        /* Calculate cognitive cost */
        int cost = shadow_calculate_cost(ch, &action, ctx->horizon);

        /* Check if we have capacity */
        if (ctx->cognitive_budget < cost) {
            break; /* Out of cognitive capacity */
        }

        /* Execute projection in isolated simulation */
        memset(&outcome, 0, sizeof(struct shadow_outcome));
        if (shadow_execute_projection(ctx, &action, &outcome)) {
            /* Store projection */
            ctx->projections[ctx->num_projections].action = action;
            ctx->projections[ctx->num_projections].outcome = outcome;
            ctx->projections[ctx->num_projections].horizon = ctx->horizon;
            ctx->projections[ctx->num_projections].total_cost = cost;
            ctx->projections[ctx->num_projections].timestamp = time(0);
            ctx->num_projections++;

            /* Consume capacity */
            shadow_consume_capacity(ctx, cost);
        }
    }
}

/**
 * Generate combat action projections
 * Projects possible combat actions (attack, flee, etc.)
 */
static void generate_combat_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch || !FIGHTING(ch)) {
        return;
    }

    /* Project attacking current opponent */
    if (ctx->num_projections < ctx->max_projections) {
        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_ATTACK;
        action.target = FIGHTING(ch);
        action.cost = SHADOW_BASE_COST;

        if (shadow_validate_action(ch, &action) == ACTION_FEASIBLE) {
            int cost = shadow_calculate_cost(ch, &action, 1); /* Short horizon in combat */

            if (ctx->cognitive_budget >= cost) {
                memset(&outcome, 0, sizeof(struct shadow_outcome));
                if (shadow_execute_projection(ctx, &action, &outcome)) {
                    ctx->projections[ctx->num_projections].action = action;
                    ctx->projections[ctx->num_projections].outcome = outcome;
                    ctx->projections[ctx->num_projections].horizon = 1;
                    ctx->projections[ctx->num_projections].total_cost = cost;
                    ctx->projections[ctx->num_projections].timestamp = time(0);
                    ctx->num_projections++;
                    shadow_consume_capacity(ctx, cost);
                }
            }
        }
    }

    /* Project fleeing */
    if (ctx->num_projections < ctx->max_projections) {
        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_FLEE;
        action.target = NULL;
        action.cost = SHADOW_BASE_COST / 2; /* Fleeing is simpler */

        if (shadow_validate_action(ch, &action) == ACTION_FEASIBLE) {
            int cost = shadow_calculate_cost(ch, &action, 1);

            if (ctx->cognitive_budget >= cost) {
                memset(&outcome, 0, sizeof(struct shadow_outcome));
                if (shadow_execute_projection(ctx, &action, &outcome)) {
                    ctx->projections[ctx->num_projections].action = action;
                    ctx->projections[ctx->num_projections].outcome = outcome;
                    ctx->projections[ctx->num_projections].horizon = 1;
                    ctx->projections[ctx->num_projections].total_cost = cost;
                    ctx->projections[ctx->num_projections].timestamp = time(0);
                    ctx->num_projections++;
                    shadow_consume_capacity(ctx, cost);
                }
            }
        }
    }
}

/**
 * Generate social action projections
 * Projects possible social interactions
 */
static void generate_social_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct char_data *target;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch || ch->in_room == NOWHERE) {
        return;
    }

    /* Find potential social targets in room */
    for (target = world[ch->in_room].people; target && ctx->num_projections < ctx->max_projections;
         target = target->next_in_room) {

        if (target == ch || !IS_NPC(target)) {
            continue; /* Skip self and players */
        }

        /* Project a friendly social action */
        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_SOCIAL;
        action.target = target;
        action.cost = SHADOW_BASE_COST / 3; /* Socials are cheap */

        if (shadow_validate_action(ch, &action) == ACTION_FEASIBLE) {
            int cost = shadow_calculate_cost(ch, &action, 1);

            if (ctx->cognitive_budget >= cost) {
                memset(&outcome, 0, sizeof(struct shadow_outcome));
                if (shadow_execute_projection(ctx, &action, &outcome)) {
                    ctx->projections[ctx->num_projections].action = action;
                    ctx->projections[ctx->num_projections].outcome = outcome;
                    ctx->projections[ctx->num_projections].horizon = 1;
                    ctx->projections[ctx->num_projections].total_cost = cost;
                    ctx->projections[ctx->num_projections].timestamp = time(0);
                    ctx->num_projections++;
                    shadow_consume_capacity(ctx, cost);
                }
            }
        }

        /* Only project one social to avoid overwhelming the system */
        break;
    }
}

/**
 * Generate item usage projections
 * Projects possible item interactions
 */
static void generate_item_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct obj_data *obj;
    struct shadow_action action;
    struct shadow_outcome outcome;
    int count = 0;

    if (!ch) {
        return;
    }

    /* Project using items in inventory */
    for (obj = ch->carrying; obj && ctx->num_projections < ctx->max_projections && count < 3; obj = obj->next_content) {

        /* Only consider usable items (food, potions, etc.) */
        if (GET_OBJ_TYPE(obj) != ITEM_FOOD && GET_OBJ_TYPE(obj) != ITEM_POTION && GET_OBJ_TYPE(obj) != ITEM_SCROLL) {
            continue;
        }

        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_USE_ITEM;
        action.target = obj;
        action.cost = SHADOW_BASE_COST;

        if (shadow_validate_action(ch, &action) == ACTION_FEASIBLE) {
            int cost = shadow_calculate_cost(ch, &action, 1);

            if (ctx->cognitive_budget >= cost) {
                memset(&outcome, 0, sizeof(struct shadow_outcome));
                if (shadow_execute_projection(ctx, &action, &outcome)) {
                    ctx->projections[ctx->num_projections].action = action;
                    ctx->projections[ctx->num_projections].outcome = outcome;
                    ctx->projections[ctx->num_projections].horizon = 1;
                    ctx->projections[ctx->num_projections].total_cost = cost;
                    ctx->projections[ctx->num_projections].timestamp = time(0);
                    ctx->num_projections++;
                    shadow_consume_capacity(ctx, cost);
                    count++;
                }
            }
        }
    }
}

/**
 * Validate that an action is feasible and doesn't violate invariants
 * Implements ST-2 (Invariant Preservation)
 */
int shadow_validate_action(struct char_data *ch, struct shadow_action *action)
{
    if (!ch || !action) {
        return ACTION_INVALID_STATE;
    }

    /* Check entity existence invariant */
    if (!check_invariant_existence(ch, ENTITY_TYPE_MOB)) {
        return ACTION_VIOLATES_INVARIANT;
    }

    /* Validate based on action type */
    switch (action->type) {
        case SHADOW_ACTION_MOVE:
            /* Check if movement is possible */
            if (ch->in_room == NOWHERE) {
                return ACTION_INVALID_STATE;
            }
            if (action->direction < 0 || action->direction >= NUM_OF_DIRS) {
                return ACTION_IMPOSSIBLE;
            }
            if (!CAN_GO(ch, action->direction)) {
                return ACTION_INVALID_STATE;
            }
            /* Check location invariant for destination */
            if (!check_invariant_location(ch, world[ch->in_room].dir_option[action->direction]->to_room)) {
                return ACTION_VIOLATES_INVARIANT;
            }
            break;

        case SHADOW_ACTION_ATTACK:
            /* Check if target exists and is valid */
            if (!action->target) {
                return ACTION_INVALID_TARGET;
            }
            if (!check_invariant_existence(action->target, ENTITY_TYPE_MOB)) {
                return ACTION_INVALID_TARGET;
            }
            /* Can't attack in safe rooms */
            if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
                return ACTION_INVALID_STATE;
            }
            break;

        case SHADOW_ACTION_FLEE:
            /* Can only flee if fighting */
            if (!FIGHTING(ch)) {
                return ACTION_INVALID_STATE;
            }
            break;

        case SHADOW_ACTION_USE_ITEM:
            /* Check if item exists */
            if (!action->target) {
                return ACTION_INVALID_TARGET;
            }
            if (!check_invariant_existence(action->target, -1)) {
                return ACTION_INVALID_TARGET;
            }
            break;

        case SHADOW_ACTION_SOCIAL:
            /* Check if target exists */
            if (action->target && !check_invariant_existence(action->target, ENTITY_TYPE_MOB)) {
                return ACTION_INVALID_TARGET;
            }
            break;

        case SHADOW_ACTION_WAIT:
            /* Always feasible */
            break;

        default:
            /* Unknown action type */
            return ACTION_IMPOSSIBLE;
    }

    /* Action-specific invariant checks */
    if (!check_invariant_action(ch, action->type)) {
        return ACTION_VIOLATES_INVARIANT;
    }

    return ACTION_FEASIBLE;
}

/**
 * Execute a projection in isolated simulation
 * Implements ST-1 (Observational Only) - NEVER mutates real state
 */
bool shadow_execute_projection(struct shadow_context *ctx, struct shadow_action *action, struct shadow_outcome *outcome)
{
    if (!ctx || !action || !outcome || !ctx->entity) {
        return FALSE;
    }

    struct char_data *ch = ctx->entity;

    /* Initialize outcome */
    memset(outcome, 0, sizeof(struct shadow_outcome));
    outcome->score = 0;
    outcome->obvious = FALSE;

    /* Simulate action without modifying real state */
    /* This is a lightweight heuristic simulation, not full state copy */

    switch (action->type) {
        case SHADOW_ACTION_MOVE: {
            /* Predict movement outcome */
            room_rnum dest = world[ch->in_room].dir_option[action->direction]->to_room;

            /* Check for danger in destination */
            if (ROOM_FLAGGED(dest, ROOM_DEATH)) {
                outcome->danger_level = 100;
                outcome->leads_to_death = TRUE;
                outcome->hp_delta = -GET_HIT(ch);
                outcome->fear_change = 50;
                outcome->score = OUTCOME_SCORE_MIN;
            } else {
                /* Basic movement is safe */
                outcome->danger_level = 10;
                outcome->hp_delta = 0;
                outcome->achieves_goal = (ch->ai_data && ch->ai_data->goal_destination == dest);
                outcome->score = outcome->achieves_goal ? 30 : 5;
                outcome->obvious = TRUE;
            }
            break;
        }

        case SHADOW_ACTION_ATTACK: {
            /* Predict combat outcome */
            struct char_data *victim = (struct char_data *)action->target;

            if (victim) {
                /* Simple heuristic: compare HP and level */
                int power_diff = GET_LEVEL(ch) - GET_LEVEL(victim);
                int hp_diff = GET_HIT(ch) - GET_HIT(victim);

                if (power_diff > 5 || hp_diff > 100) {
                    /* Likely to win */
                    outcome->danger_level = 20;
                    outcome->hp_delta = -10;     /* Minor damage */
                    outcome->anger_change = -10; /* Satisfies anger */
                    outcome->score = 40;
                } else if (power_diff < -5 || hp_diff < -100) {
                    /* Likely to lose */
                    outcome->danger_level = 80;
                    outcome->hp_delta = -50;
                    outcome->leads_to_death = TRUE;
                    outcome->fear_change = 30;
                    outcome->score = -60;
                } else {
                    /* Even match */
                    outcome->danger_level = 50;
                    outcome->hp_delta = -25;
                    outcome->score = 0;
                }
                outcome->leads_to_combat = TRUE;
            }
            break;
        }

        case SHADOW_ACTION_FLEE: {
            /* Predict flee outcome */
            outcome->danger_level = 30; /* Fleeing has some risk */
            outcome->hp_delta = 0;
            outcome->fear_change = -20; /* Reduces fear */
            outcome->leads_to_combat = FALSE;
            outcome->score = 20;
            outcome->obvious = TRUE;
            break;
        }

        case SHADOW_ACTION_USE_ITEM: {
            /* Predict item usage */
            struct obj_data *obj = (struct obj_data *)action->target;

            if (obj) {
                if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
                    outcome->hp_delta = 5; /* Minor healing */
                    outcome->score = 10;
                    outcome->obvious = TRUE;
                } else if (GET_OBJ_TYPE(obj) == ITEM_POTION) {
                    outcome->hp_delta = 20; /* More healing */
                    outcome->score = 25;
                    outcome->obvious = TRUE;
                }
            }
            break;
        }

        case SHADOW_ACTION_SOCIAL: {
            /* Predict social interaction */
            outcome->danger_level = 5;
            outcome->happiness_change = 5;
            outcome->score = 10;
            outcome->obvious = TRUE;
            break;
        }

        case SHADOW_ACTION_WAIT: {
            /* Waiting is safe but unproductive */
            outcome->score = 0;
            outcome->obvious = TRUE;
            break;
        }

        default:
            return FALSE;
    }

    /* Apply subjectivity based on entity's state */
    shadow_apply_subjectivity(ch, outcome);

    /* Check if outcome is obvious to this entity */
    outcome->obvious = shadow_is_obvious(ch, action, outcome);

    return TRUE;
}

/**
 * Score and rank projections based on entity's goals and state
 * Implements ST-4 (Subjectivity)
 */
void shadow_score_projections(struct shadow_context *ctx)
{
    if (!ctx || ctx->num_projections == 0) {
        return;
    }

    int i;
    for (i = 0; i < ctx->num_projections; i++) {
        ctx->projections[i].outcome.score = score_projection_for_entity(ctx->entity, &ctx->projections[i]);
    }
}

/**
 * Score a projection based on entity's subjective preferences
 */
static int score_projection_for_entity(struct char_data *ch, struct shadow_projection *proj)
{
    if (!ch || !proj) {
        return 0;
    }

    int score = proj->outcome.score;

    /* Adjust based on entity's emotional state */
    if (ch->ai_data) {
        /* Fear influences danger perception */
        if (ch->ai_data->emotion_fear > 50) {
            score -= proj->outcome.danger_level / 2;
        }

        /* Anger influences aggression preference */
        if (ch->ai_data->emotion_anger > 50 && proj->action.type == SHADOW_ACTION_ATTACK) {
            score += 20;
        }

        /* Courage influences risk-taking */
        if (ch->ai_data->emotion_courage > 50) {
            score += proj->outcome.danger_level / 4; /* Risk-taker bonus */
        }

        /* Goal-oriented bonus */
        if (proj->outcome.achieves_goal) {
            score += 50;
        }
    }

    return URANGE(OUTCOME_SCORE_MIN, score, OUTCOME_SCORE_MAX);
}

/**
 * Select the best action from generated projections
 */
struct shadow_projection *shadow_select_best_action(struct shadow_context *ctx)
{
    if (!ctx || ctx->num_projections == 0) {
        return NULL;
    }

    int best_idx = 0;
    int best_score = ctx->projections[0].outcome.score;
    int i;

    for (i = 1; i < ctx->num_projections; i++) {
        if (ctx->projections[i].outcome.score > best_score) {
            best_score = ctx->projections[i].outcome.score;
            best_idx = i;
        }
    }

    /* Don't return actions that are clearly bad */
    if (best_score < -50) {
        return NULL;
    }

    return &ctx->projections[best_idx];
}

/**
 * Calculate cognitive cost for an action
 * Implements ST-3 (Bounded Cognition)
 */
int shadow_calculate_cost(struct char_data *ch, struct shadow_action *action, int horizon)
{
    if (!ch || !action) {
        return 0;
    }

    int base_cost = action->cost;

    /* Cost increases with horizon */
    int cost = base_cost * (horizon > 0 ? horizon : 1);

    /* Obvious actions are cheaper */
    if (action->type == SHADOW_ACTION_WAIT || action->type == SHADOW_ACTION_SOCIAL) {
        cost /= 2;
    }

    /* Complex actions are more expensive */
    if (action->type == SHADOW_ACTION_CAST_SPELL) {
        cost *= 2;
    }

    /* Emotional intelligence affects efficiency */
    if (ch->ai_data && ch->ai_data->genetics.emotional_intelligence > 50) {
        cost = cost * 80 / 100; /* 20% discount */
    }

    return MAX(1, cost);
}

/**
 * Consume cognitive capacity for a projection
 */
bool shadow_consume_capacity(struct shadow_context *ctx, int cost)
{
    if (!ctx || cost < 0) {
        return FALSE;
    }

    if (ctx->cognitive_budget < cost) {
        return FALSE;
    }

    ctx->cognitive_budget -= cost;
    return TRUE;
}

/**
 * Regenerate cognitive capacity over time
 * Called periodically by the game loop
 */
void shadow_regenerate_capacity(struct char_data *ch)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return;
    }

    /* Regenerate capacity */
    ch->ai_data->cognitive_capacity += COGNITIVE_CAPACITY_REGEN;

    /* Cap at maximum */
    if (ch->ai_data->cognitive_capacity > COGNITIVE_CAPACITY_MAX) {
        ch->ai_data->cognitive_capacity = COGNITIVE_CAPACITY_MAX;
    }
}

/**
 * Check if outcome is "obvious" to the entity
 * Obvious outcomes can be collapsed or treated heuristically
 */
bool shadow_is_obvious(struct char_data *ch, struct shadow_action *action, struct shadow_outcome *outcome)
{
    if (!ch || !action || !outcome) {
        return FALSE;
    }

    /* Simple actions are obvious */
    if (action->type == SHADOW_ACTION_WAIT || action->type == SHADOW_ACTION_SOCIAL) {
        return TRUE;
    }

    /* Death traps are obvious if entity is intelligent */
    if (outcome->leads_to_death && ch->ai_data && ch->ai_data->genetics.emotional_intelligence > 40) {
        return TRUE;
    }

    /* Safe movements are obvious */
    if (action->type == SHADOW_ACTION_MOVE && outcome->danger_level < 20) {
        return TRUE;
    }

    return FALSE;
}

/**
 * Apply subjectivity bias based on entity's emotional state and genetics
 * Implements ST-4 (Subjectivity)
 */
void shadow_apply_subjectivity(struct char_data *ch, struct shadow_outcome *outcome)
{
    if (!ch || !outcome || !ch->ai_data) {
        return;
    }

    /* Fear amplifies perceived danger */
    if (ch->ai_data->emotion_fear > 50) {
        outcome->danger_level = MIN(100, outcome->danger_level * 130 / 100);
    }

    /* Courage reduces perceived danger */
    if (ch->ai_data->emotion_courage > 50) {
        outcome->danger_level = outcome->danger_level * 70 / 100;
    }

    /* Greed amplifies perceived rewards */
    if (ch->ai_data->emotion_greed > 50) {
        outcome->reward_level = MIN(100, outcome->reward_level * 120 / 100);
    }

    /* Happiness makes outcomes seem more positive */
    if (ch->ai_data->emotion_happiness > 60) {
        outcome->score += 10;
    }

    /* Sadness makes outcomes seem more negative */
    if (ch->ai_data->emotion_sadness > 60) {
        outcome->score -= 10;
    }
}

/**
 * Check entity existence invariant
 * Ensures entity exists and has valid prototype
 */
static bool check_invariant_existence(void *entity, int entity_type)
{
    if (!entity) {
        return FALSE;
    }

    /* For mobs, check if they have valid rnum */
    if (entity_type == ENTITY_TYPE_MOB) {
        struct char_data *ch = (struct char_data *)entity;
        if (IS_NPC(ch) && GET_MOB_RNUM(ch) == NOBODY) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * Check location invariant
 * Ensures no simultaneous multi-location
 */
static bool check_invariant_location(struct char_data *ch, room_rnum room)
{
    if (!ch) {
        return FALSE;
    }

    /* Check that destination room is valid */
    if (room == NOWHERE || room < 0 || room > top_of_world) {
        return FALSE;
    }

    /* Entity can only be in one room at a time */
    /* This is enforced by the simulation - we're just validating */
    return TRUE;
}

/**
 * Check action-specific invariants
 */
static bool check_invariant_action(struct char_data *ch, enum shadow_action_type type)
{
    if (!ch) {
        return FALSE;
    }

    /* All validated action types preserve invariants */
    /* Complex invariant checks would go here */

    return TRUE;
}

/**
 * Log shadow timeline activity (shadow-only logging)
 * For debugging and analysis
 */
void shadow_log(struct shadow_context *ctx, const char *message)
{
    if (!ctx || !message) {
        return;
    }

    /* Shadow-only logging - doesn't affect real world */
    /* In production, this could be disabled or use separate log file */
    /* For now, it's a no-op to avoid log spam */

    (void)ctx;     /* Suppress unused warning */
    (void)message; /* Suppress unused warning */
}

/**
 * Dump shadow context for debugging
 */
void shadow_dump_context(struct shadow_context *ctx)
{
    if (!ctx) {
        return;
    }

    /* Debug dump - only in development */
    /* Would output context state for analysis */

    (void)ctx; /* Suppress unused warning */
}
