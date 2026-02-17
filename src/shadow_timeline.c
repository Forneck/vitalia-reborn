/**
 * @file shadow_timeline.c
 * Shadow Timeline: Cognitive Future Simulation Layer
 *
 * RFC-0003 COMPLIANT - Implementation conformant to RFC-0003 normative specification
 * (with acceptable partial implementations for §8.2 and §9.1, see RFC_0003_DEFINITION.md §18.2)
 * Implementation: RFC-0001 (completed 2026-02-07)
 * Architecture: RFC-0003 (normative 2026-02-09)
 *
 * This system allows autonomous entities to internally explore possible future
 * outcomes without modifying the real world state.
 *
 * RFC-0003 Compliance Statement:
 * This implementation complies with RFC-0003 "Shadow Timeline — Definition,
 * Scope, and Authority" normative requirements. See RFC_0003_DEFINITION.md §18
 * for detailed compliance assessment.
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
#include "moral_reasoner.h"
#include "act.h"
#include "fight.h"
#include "graph.h"
#include "quest.h"

/* External variables */
extern struct room_data *world;

/* File-local function prototypes */
static void generate_movement_projections(struct shadow_context *ctx);
static void generate_combat_projections(struct shadow_context *ctx);
static void generate_social_projections(struct shadow_context *ctx);
static void generate_item_projections(struct shadow_context *ctx);
static void generate_guard_projection(struct shadow_context *ctx);
static void generate_quest_projections(struct shadow_context *ctx);
static int score_projection_for_entity(struct char_data *ch, struct shadow_projection *proj);
static bool check_invariant_existence(void *entity, int entity_type);
static bool check_invariant_location(struct char_data *ch, room_rnum room);
static bool check_invariant_action(struct char_data *ch, enum shadow_action_type type);

/**
 * Initialize a shadow context for an entity
 * RFC-0003 §6.1: Only autonomous decision-making entities may consult
 * RFC-0003 §4.1: Domain separation - context is external to entity
 * Allocates memory and sets up projection tracking
 */
struct shadow_context *shadow_init_context(struct char_data *ch)
{
    struct shadow_context *ctx;

    /* RFC-0003 §6.2: Verify cognitive requirement */
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

    /* RFC-0003 §7.2: Set cognitive budget based on entity's capacity */
    if (IS_NPC(ch) && ch->ai_data) {
        ctx->cognitive_budget = ch->ai_data->cognitive_capacity;
    } else {
        ctx->cognitive_budget = COGNITIVE_CAPACITY_MAX;
    }

    return ctx;
}

/**
 * Free a shadow context and all associated projections
 * RFC-0003 §10.1: Ensures no recording - projections are ephemeral
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

    /* Check if sentinel is at their post - prioritize guard duty */
    if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SENTINEL) && ch->ai_data) {
        room_rnum guard_post = real_room(ch->ai_data->guard_post);
        if (guard_post != NOWHERE && ch->in_room == guard_post) {
            /* Sentinel at post - prioritize guarding */
            generate_guard_projection(ctx);
        }
    }

    if (FIGHTING(ch)) {
        /* In combat - focus on combat projections */
        generate_combat_projections(ctx);
    } else if (ch->ai_data && ch->ai_data->current_goal != 0) {
        /* Has active goal - focus on goal-relevant actions */
        generate_movement_projections(ctx);
        generate_item_projections(ctx);
        generate_quest_projections(ctx); /* Consider quest-related actions */
    } else {
        /* Exploring - generate diverse projections */
        generate_movement_projections(ctx);
        generate_social_projections(ctx);
        generate_item_projections(ctx);
        generate_quest_projections(ctx); /* Consider quest opportunities */
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
 * Generate guard action projection for sentinels
 * Projects standing guard at post (intentional waiting)
 */
static void generate_guard_projection(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch || !IS_NPC(ch)) {
        return;
    }

    /* Only generate guard action for sentinels at their post */
    if (!MOB_FLAGGED(ch, MOB_SENTINEL) || !ch->ai_data) {
        return;
    }

    room_rnum guard_post = real_room(ch->ai_data->guard_post);
    if (guard_post == NOWHERE || ch->in_room != guard_post) {
        return;
    }

    /* If we're at capacity, skip */
    if (ctx->num_projections >= ctx->max_projections) {
        return;
    }

    /* Set up guard action */
    memset(&action, 0, sizeof(struct shadow_action));
    action.type = SHADOW_ACTION_GUARD;
    action.target = NULL;
    action.cost = SHADOW_BASE_COST / 2; /* Guarding costs less than other actions */

    /* Validate action */
    if (shadow_validate_action(ch, &action) != ACTION_FEASIBLE) {
        return;
    }

    /* Calculate cognitive cost */
    int cost = shadow_calculate_cost(ch, &action, 1);

    /* Check if we have capacity */
    if (ctx->cognitive_budget < cost) {
        return;
    }

    /* Execute projection */
    memset(&outcome, 0, sizeof(struct shadow_outcome));
    if (shadow_execute_projection(ctx, &action, &outcome)) {
        /* Store projection */
        ctx->projections[ctx->num_projections].action = action;
        ctx->projections[ctx->num_projections].outcome = outcome;
        ctx->projections[ctx->num_projections].horizon = 1;
        ctx->projections[ctx->num_projections].total_cost = cost;
        ctx->projections[ctx->num_projections].timestamp = time(0);
        ctx->num_projections++;

        /* Consume capacity */
        shadow_consume_capacity(ctx, cost);
    }
}

/**
 * Generate quest-related action projections
 * Projects quest acceptance and completion opportunities
 * RFC-0003 §6.1: Only autonomous entities with goals project quest actions
 */
static void generate_quest_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return;
    }

    /* If we're at capacity, skip */
    if (ctx->num_projections >= ctx->max_projections) {
        return;
    }

    /* Generate quest completion projection if mob has active quest */
    if (ch->ai_data->current_quest != NOTHING) {
        /* Set up quest completion action */
        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_QUEST;
        action.target = NULL; /* Quest completion doesn't need a target */
        action.cost = SHADOW_BASE_COST;

        /* Validate action */
        if (shadow_validate_action(ch, &action) != ACTION_FEASIBLE) {
            return;
        }

        /* Calculate cognitive cost */
        int cost = shadow_calculate_cost(ch, &action, 1);

        /* Check if we have capacity */
        if (ctx->cognitive_budget < cost) {
            return;
        }

        /* Execute projection */
        memset(&outcome, 0, sizeof(struct shadow_outcome));
        if (shadow_execute_projection(ctx, &action, &outcome)) {
            /* Quest completion is highly rewarding */
            outcome.score = 60;
            outcome.reward_level = 80;
            outcome.achieves_goal = (ch->ai_data->current_goal == GOAL_COMPLETE_QUEST);
            outcome.obvious = FALSE; /* Quest outcomes can be complex */

            /* Store projection */
            ctx->projections[ctx->num_projections].action = action;
            ctx->projections[ctx->num_projections].outcome = outcome;
            ctx->projections[ctx->num_projections].horizon = 1;
            ctx->projections[ctx->num_projections].total_cost = cost;
            ctx->projections[ctx->num_projections].timestamp = time(0);
            ctx->num_projections++;

            /* Consume capacity */
            shadow_consume_capacity(ctx, cost);
        }
        return; /* Don't generate quest acceptance if already has quest */
    }

    /* Generate quest acceptance projection if mob doesn't have quest and is interested */
    /* Only consider quest acceptance if:
     * 1. No active goal (exploring) OR goal is quest-related
     * 2. Has sufficient curiosity or quest tendency
     * 3. Cognitive capacity allows expensive questmaster search */

    bool should_consider_quest = FALSE;

    if (ch->ai_data->current_goal == GOAL_NONE || 
        ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER ||
        ch->ai_data->current_goal == GOAL_ACCEPT_QUEST) {
        /* Calculate interest in quests based on genetics and emotions */
        int quest_interest = GET_GENQUEST(ch) / 5; /* 0-20 from quest_tendency */
        quest_interest += ch->ai_data->emotion_curiosity / 10; /* 0-10 from curiosity */
        
        /* Random check with interest-based threshold */
        if (rand_number(0, 100) < quest_interest) {
            should_consider_quest = TRUE;
        }
    }

    if (!should_consider_quest) {
        return;
    }

    /* Safety check: Validate room before world array access */
    if (ch->in_room == NOWHERE || ch->in_room < 0 || ch->in_room > top_of_world) {
        return;
    }

    /* Look for accessible questmaster in mob's zone
     * NOTE: This is an expensive operation, but Shadow Timeline already has
     * cognitive capacity budgeting to limit frequency */
    zone_rnum mob_zone = world[ch->in_room].zone;
    struct char_data *questmaster = find_accessible_questmaster_in_zone(ch, mob_zone);

    if (!questmaster || questmaster == ch) {
        return; /* No accessible questmaster found */
    }

    /* Find available quest from this questmaster */
    qst_vnum available_quest = find_mob_available_quest_by_qmnum(ch, GET_MOB_VNUM(questmaster));
    if (available_quest == NOTHING) {
        return; /* No quests available */
    }

    qst_rnum quest_rnum = real_quest(available_quest);
    if (quest_rnum == NOTHING || !mob_should_accept_quest(ch, quest_rnum)) {
        return; /* Quest not suitable or mob doesn't want it */
    }

    /* Set up quest acceptance action */
    memset(&action, 0, sizeof(struct shadow_action));
    action.type = SHADOW_ACTION_QUEST;
    action.target = questmaster; /* Target is the questmaster to talk to */
    action.cost = SHADOW_BASE_COST;

    /* Validate action */
    if (shadow_validate_action(ch, &action) != ACTION_FEASIBLE) {
        return;
    }

    /* Calculate cognitive cost */
    int cost = shadow_calculate_cost(ch, &action, 1);

    /* Check if we have capacity */
    if (ctx->cognitive_budget < cost) {
        return;
    }

    /* Execute projection */
    memset(&outcome, 0, sizeof(struct shadow_outcome));
    if (shadow_execute_projection(ctx, &action, &outcome)) {
        /* Quest acceptance is moderately rewarding - represents new opportunities */
        outcome.score = 30;
        outcome.reward_level = 50;
        outcome.achieves_goal = (ch->ai_data->current_goal == GOAL_ACCEPT_QUEST ||
                                ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER);
        outcome.obvious = TRUE; /* Accepting a quest is straightforward */

        /* Increase happiness (new opportunity) and curiosity satisfaction */
        outcome.happiness_change = 10;

        /* Store projection */
        ctx->projections[ctx->num_projections].action = action;
        ctx->projections[ctx->num_projections].outcome = outcome;
        ctx->projections[ctx->num_projections].horizon = 1;
        ctx->projections[ctx->num_projections].total_cost = cost;
        ctx->projections[ctx->num_projections].timestamp = time(0);
        ctx->num_projections++;

        /* Consume capacity */
        shadow_consume_capacity(ctx, cost);
    }
}


/**
 * Validate that an action is feasible and doesn't violate invariants
 * RFC-0003 §5.3: Invariant Enforcement - MUST discard actions violating invariants
 * Implements ST-2 (Invariant Preservation)
 */
int shadow_validate_action(struct char_data *ch, struct shadow_action *action)
{
    if (!ch || !action) {
        return ACTION_INVALID_STATE;
    }

    /* RFC-0003 §5.3: Check entity existence invariant (absolute invariant) */
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
            if (!check_invariant_existence(action->target, ENTITY_TYPE_ANY)) {
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

        case SHADOW_ACTION_GUARD:
            /* Guard action is only feasible for sentinels at their guard post */
            if (!IS_NPC(ch) || !MOB_FLAGGED(ch, MOB_SENTINEL)) {
                return ACTION_INVALID_STATE;
            }
            if (!ch->ai_data) {
                return ACTION_INVALID_STATE;
            }
            /* Verify sentinel is at their guard post */
            {
                room_rnum guard_post = real_room(ch->ai_data->guard_post);
                if (guard_post == NOWHERE || ch->in_room != guard_post) {
                    return ACTION_INVALID_STATE;
                }
            }
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
            room_rnum dest = NOWHERE;

            /* Defensive checks: ensure valid room, direction, and exit before dereferencing */
            if (ch->in_room != NOWHERE && action->direction >= 0 && action->direction < NUM_OF_DIRS && world &&
                world[ch->in_room].dir_option[action->direction] != NULL) {
                dest = world[ch->in_room].dir_option[action->direction]->to_room;
            }

            /* If destination is invalid or unknown, treat as low-value/unsafe to attempt */
            if (dest == NOWHERE) {
                outcome->danger_level = 0;
                outcome->hp_delta = 0;
                outcome->leads_to_death = FALSE;
                outcome->fear_change = 0;
                outcome->achieves_goal = FALSE;
                outcome->score = OUTCOME_SCORE_MIN;
                outcome->obvious = FALSE;
                break;
            }

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

                /* Sentinel awareness: heavily penalize moving away from post */
                if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SENTINEL) && ch->ai_data) {
                    room_rnum guard_post = real_room(ch->ai_data->guard_post);

                    /* If sentinel is at their post, strongly discourage leaving */
                    if (ch->in_room == guard_post) {
                        outcome->score -= 60; /* Heavy penalty for leaving post */
                        outcome->achieves_goal = FALSE;
                    }
                    /* If sentinel is away from post, check if this move brings them closer */
                    else if (guard_post != NOWHERE) {
                        if (dest == guard_post) {
                            /* Moving directly to post - highest priority! */
                            outcome->score += 70;
                            outcome->achieves_goal = TRUE;
                        } else {
                            /*
                             * Use BFS distance (room hops) rather than find_first_step()
                             * which returns a direction or BFS_* codes, not distance.
                             */
                            int current_dist = bfs_distance(ch->in_room, guard_post);
                            int dest_dist = bfs_distance(dest, guard_post);

                            /* Ignore scoring if either call failed catastrophically */
                            if (current_dist != BFS_ERROR && dest_dist != BFS_ERROR) {
                                if (current_dist == BFS_NO_PATH && dest_dist != BFS_NO_PATH) {
                                    /*
                                     * Currently unreachable, moving to a room from which the
                                     * post is reachable. Keep this neutral and let other
                                     * factors decide.
                                     */
                                } else if (current_dist != BFS_NO_PATH && dest_dist == BFS_NO_PATH) {
                                    /* Moving from reachable to unreachable - penalize */
                                    outcome->score -= 50;
                                } else if (current_dist != BFS_NO_PATH && dest_dist != BFS_NO_PATH) {
                                    /*
                                     * Both rooms have valid paths to post.
                                     * Compare actual distances.
                                     */
                                    if (dest_dist < current_dist) {
                                        /* Moving closer to post - good */
                                        outcome->score += 40;
                                    } else if (dest_dist > current_dist) {
                                        /* Moving away from post - penalize */
                                        outcome->score -= 50;
                                    }
                                    /* If equal distance, neutral - let other factors decide */
                                }
                                /*
                                 * If both rooms are unreachable (BFS_NO_PATH), keep
                                 * this heuristic neutral and let other factors decide.
                                 */
                            }
                        }
                    }
                }
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

        case SHADOW_ACTION_GUARD: {
            /* Standing guard - safe and fulfills sentinel duty */
            outcome->score = 15; /* Better than waiting, fulfills duty */
            outcome->danger_level = 5;
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
    int moral_cost = 0;

    /* Apply moral reasoning to action evaluation */
    if (IS_NPC(ch) && proj->action.target) {
        struct char_data *target = (struct char_data *)proj->action.target;
        int action_type = MORAL_ACTION_ATTACK; /* Default */

        /* Map shadow action types to moral action types */
        switch (proj->action.type) {
            case SHADOW_ACTION_ATTACK:
                action_type = MORAL_ACTION_ATTACK;
                break;
            case SHADOW_ACTION_TRADE:
                action_type = MORAL_ACTION_TRADE;
                break;
            case SHADOW_ACTION_SOCIAL:
                action_type = MORAL_ACTION_HELP;
                break;
            case SHADOW_ACTION_FLEE:
                /* Fleeing could be abandoning allies */
                if (ch->master || ch->followers) {
                    action_type = MORAL_ACTION_ABANDON_ALLY;
                } else {
                    action_type = MORAL_ACTION_DEFEND;
                }
                break;
            default:
                action_type = -1; /* No moral evaluation */
                break;
        }

        /* Evaluate moral cost if applicable */
        if (action_type >= 0) {
            moral_cost = moral_evaluate_action_cost(ch, target, action_type);
            score += moral_cost;
        }
    }

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

        /* Compassion influences helping behavior */
        if (ch->ai_data->emotion_compassion > 60 && proj->action.type == SHADOW_ACTION_SOCIAL) {
            score += 15;
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
    struct char_data *ch;
    int available;

    if (!ctx || cost < 0) {
        return FALSE;
    }

    ch = ctx->entity;
    available = ctx->cognitive_budget;

    /* If we have a backing NPC with ai_data, also respect its stored capacity */
    if (ch && IS_NPC(ch) && ch->ai_data) {
        if (ch->ai_data->cognitive_capacity < available) {
            available = ch->ai_data->cognitive_capacity;
        }
    }

    if (available < cost) {
        return FALSE;
    }

    ctx->cognitive_budget -= cost;

    /* Persist consumption back onto the mob so capacity is tracked across ticks */
    if (ch && IS_NPC(ch) && ch->ai_data) {
        if (ch->ai_data->cognitive_capacity >= cost) {
            ch->ai_data->cognitive_capacity -= cost;
        } else {
            ch->ai_data->cognitive_capacity = 0;
        }
    }

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
    if (action->type == SHADOW_ACTION_WAIT || action->type == SHADOW_ACTION_SOCIAL ||
        action->type == SHADOW_ACTION_GUARD) {
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

/**
 * Mob uses Shadow Timeline to select next action
 * High-level convenience function for mob AI
 *
 * @param ch         The mob/character making a decision.
 * @param out_action Caller-provided storage where the chosen action will be
 *                   written on success.
 *
 * @return TRUE on success (out_action filled), FALSE on failure (out_action
 *         is not modified).
 */
bool mob_shadow_choose_action(struct char_data *ch, struct shadow_action *out_action)
{
    struct shadow_context *ctx;
    struct shadow_projection *best;

    if (!ch || !out_action) {
        return FALSE;
    }

    if (!IS_COGNITIVE_ENTITY(ch)) {
        return FALSE;
    }

    /* Initialize shadow context */
    ctx = shadow_init_context(ch);
    if (!ctx) {
        return FALSE;
    }

    /* Generate projections for available actions */
    if (shadow_generate_projections(ctx) == 0) {
        shadow_free_context(ctx);
        return FALSE;
    }

    /* Select best action */
    best = shadow_select_best_action(ctx);
    if (!best) {
        shadow_free_context(ctx);
        return FALSE;
    }

    /* Copy selected action to caller-provided storage */
    *out_action = best->action;

    /* Store predicted score and outcome flags for feedback system */
    if (ch->ai_data) {
        ch->ai_data->last_predicted_score = best->outcome.score;
        ch->ai_data->last_outcome_obvious = best->outcome.obvious;
    }

    /* Free context */
    shadow_free_context(ctx);

    return TRUE;
}

/**
 * Evaluate real outcome after action execution
 * Computes a score based on actual state changes
 * @param ch The entity that executed the action
 * @return Real outcome score (-100 to 100)
 */
int shadow_evaluate_real_outcome(struct char_data *ch)
{
    int real_score;
    int hp_delta;

    if (!ch || !ch->ai_data) {
        return 0;
    }

    /* Compute HP delta */
    hp_delta = GET_HIT(ch) - ch->ai_data->last_hp_snapshot;

    /* Base score is HP delta */
    real_score = hp_delta;

    /* Apply penalties for negative situations */
    if (FIGHTING(ch)) {
        real_score -= 10;
    }

    /* Penalty for being at low HP (< 25% max) */
    if (GET_HIT(ch) < GET_MAX_HIT(ch) / 4) {
        real_score -= 20;
    }

    /* Clamp result to [-100, 100] */
    if (real_score > 100) {
        real_score = 100;
    }
    if (real_score < -100) {
        real_score = -100;
    }

    /* Store in ai_data */
    ch->ai_data->last_real_score = real_score;

    return real_score;
}

/**
 * Update prediction error feedback with precision weighting and valence-specific adaptation
 * Implements adaptive learning through prediction-error signals with asymmetric learning
 * @param ch The entity
 * @param real_score The actual outcome score
 * @param obvious Whether the outcome was obvious/predictable
 */
void shadow_update_feedback(struct char_data *ch, int real_score, bool obvious)
{
    int predicted;
    int signed_error;
    int novelty;
    int delta_bias;

    if (!ch || !ch->ai_data) {
        return;
    }

    /* Step 1: Calculate signed prediction error (valence-specific) */
    predicted = ch->ai_data->last_predicted_score;
    signed_error = real_score - predicted;

    /* Step 2: Base novelty from absolute error */
    novelty = MIN(abs(signed_error), 100);

    /* Step 3: Apply valence-specific weighting (asymmetric learning) */
    /* Negative surprises (threats) are amplified - models loss aversion */
    if (signed_error < 0) {
        novelty = (novelty * 130) / 100; /* 30% amplification for threats */
    }
    /* Positive surprises (rewards) are slightly dampened */
    else if (signed_error > 0) {
        novelty = (novelty * 90) / 100; /* 10% reduction for rewards */
    }

    /* Step 4: Apply precision weighting (predictability modulation) */
    if (obvious) {
        /* 30% reduction for obvious outcomes */
        novelty = (novelty * 70) / 100;
    }

    /* Clamp novelty to [0, 100] after all modifications */
    if (novelty > 100) {
        novelty = 100;
    }
    if (novelty < 0) {
        novelty = 0;
    }

    /* Step 5: Update smoothed prediction error (70% memory, 30% new signal) */
    ch->ai_data->recent_prediction_error = (ch->ai_data->recent_prediction_error * 7 + novelty * 3) / 10;

    /* Ensure bounded [0, 100] */
    if (ch->ai_data->recent_prediction_error > 100) {
        ch->ai_data->recent_prediction_error = 100;
    }
    if (ch->ai_data->recent_prediction_error < 0) {
        ch->ai_data->recent_prediction_error = 0;
    }

    /* Step 6: Update attention bias (long-term adaptation) */
    delta_bias = (novelty - 50) / 15;
    ch->ai_data->attention_bias += delta_bias;

    /* Clamp attention bias to [-50, 50] */
    if (ch->ai_data->attention_bias > 50) {
        ch->ai_data->attention_bias = 50;
    }
    if (ch->ai_data->attention_bias < -50) {
        ch->ai_data->attention_bias = -50;
    }
}
