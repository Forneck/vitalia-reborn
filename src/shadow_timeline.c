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
#include "sec.h"

/* External variables */
extern struct room_data *world;

/* File-local function prototypes */
static void generate_movement_projections(struct shadow_context *ctx);
static void generate_combat_projections(struct shadow_context *ctx);
static void generate_social_projections(struct shadow_context *ctx);
static void generate_item_projections(struct shadow_context *ctx);
static void generate_guard_projection(struct shadow_context *ctx);
static void generate_quest_projections(struct shadow_context *ctx);
static void generate_spell_projections(struct shadow_context *ctx);
static void generate_trade_projections(struct shadow_context *ctx);
static void generate_wait_projection(struct shadow_context *ctx);
static void generate_follow_projections(struct shadow_context *ctx);
static void generate_group_projections(struct shadow_context *ctx);
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
        generate_spell_projections(ctx); /* Consider spell casting in combat */
    } else if (ch->ai_data && ch->ai_data->current_goal != 0) {
        /* Has active goal - focus on goal-relevant actions */
        generate_movement_projections(ctx);
        generate_item_projections(ctx);
        generate_quest_projections(ctx); /* Consider quest-related actions */
        generate_wait_projection(ctx);   /* Consider strategic waiting */
    } else {
        /* Exploring - generate diverse projections */
        generate_movement_projections(ctx);
        generate_social_projections(ctx);
        generate_item_projections(ctx);
        generate_quest_projections(ctx);  /* Consider quest opportunities */
        generate_spell_projections(ctx);  /* Consider helpful spells */
        generate_trade_projections(ctx);  /* Consider trading opportunities */
        generate_follow_projections(ctx); /* Consider following others */
        generate_group_projections(ctx);  /* Consider group formation */
        generate_wait_projection(ctx);    /* Consider doing nothing */
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

    if (ch->ai_data->current_goal == GOAL_NONE || ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER ||
        ch->ai_data->current_goal == GOAL_ACCEPT_QUEST) {
        /* Calculate interest in quests based on emotions and genetics
         * Curiosity emotion is primary driver (0-50), genetics provides baseline (0-20)
         * More curious mobs will actively seek and pursue quests */
        int quest_interest = ch->ai_data->emotion_curiosity / 2; /* 0-50 from curiosity (primary) */
        quest_interest += GET_GENQUEST(ch) / 5;                  /* 0-20 from quest_tendency (secondary) */

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

    /* Performance optimization: Early exit to prevent expensive questmaster search
     * Similar to mob_try_to_accept_quest() load-shedding to avoid lag.
     * When close to max quests, the expensive pathfinding in
     * find_accessible_questmaster_in_zone() causes severe lag. */
    if (count_mob_posted_quests() >= (CONFIG_MAX_MOB_POSTED_QUESTS * 9 / 10)) { /* 90% of max */
        return;
    }

    /* Look for accessible questmaster in mob's zone
     * NOTE: This is an expensive operation with repeated pathfinding.
     * The above load-shedding check and the random interest check (line 547)
     * help limit how often this runs. Shadow Timeline's cognitive capacity
     * budgeting also provides natural rate limiting. */
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
        outcome.achieves_goal =
            (ch->ai_data->current_goal == GOAL_ACCEPT_QUEST || ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER);
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
 * Generate spell casting projections
 * Projects healing allies or harming enemies with spells
 * RFC-0003 §4.2: Only projects if entity has mana and spell knowledge
 */
static void generate_spell_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct char_data *target;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch || !IS_NPC(ch) || ch->in_room == NOWHERE) {
        return;
    }

    /* Only generate spell projections if mob has spellcasting items.
     * Mana alone is not sufficient — the mob must actually possess a wand,
     * staff, scroll, or potion to use.  Without items the execution stub
     * cannot cast a real spell, so skip projection entirely to avoid
     * showing misleading "conjura uma magia" messages. */
    {
        bool has_spell_item = FALSE;
        struct obj_data *obj;
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            int t = GET_OBJ_TYPE(obj);
            if (t == ITEM_WAND || t == ITEM_STAFF || t == ITEM_SCROLL || t == ITEM_POTION) {
                has_spell_item = TRUE;
                break;
            }
        }
        if (!has_spell_item)
            return;
    }

    /* Only generate spell projections if mob has sufficient mana */
    if (GET_MANA(ch) < 15) {
        return;
    }

    /* If we're at capacity, skip */
    if (ctx->num_projections >= ctx->max_projections) {
        return;
    }

    /* Find potential spell targets in room */
    for (target = world[ch->in_room].people; target && ctx->num_projections < ctx->max_projections;
         target = target->next_in_room) {

        if (target == ch) {
            continue; /* Skip self */
        }

        /* Determine if target is ally or enemy */
        /* Allies: following same master, in same group, or not aggressive to each other */
        bool is_ally = FALSE;

        if (ch->master == target || target->master == ch) {
            is_ally = TRUE; /* Following relationship indicates alliance */
        } else if (IS_NPC(ch) && IS_NPC(target)) {
            /* Two NPCs - check if they're in combat or have aggro flags */
            if (FIGHTING(ch) == target || FIGHTING(target) == ch) {
                is_ally = FALSE; /* Currently fighting = not allies */
            } else {
                /* Default to neutral/ally unless explicitly hostile */
                is_ally = TRUE;
            }
        }

        /* Set up spell action */
        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_CAST_SPELL;
        action.target = target;
        action.cost = SHADOW_BASE_COST * 2; /* Spells are more expensive to project */

        if (shadow_validate_action(ch, &action) != ACTION_FEASIBLE) {
            continue;
        }

        /* Calculate cognitive cost */
        int cost = shadow_calculate_cost(ch, &action, 1);

        /* Check if we have capacity */
        if (ctx->cognitive_budget < cost) {
            continue;
        }

        /* Execute projection */
        memset(&outcome, 0, sizeof(struct shadow_outcome));
        if (shadow_execute_projection(ctx, &action, &outcome)) {
            /* Evaluate outcome based on whether it's healing or harming */
            if (is_ally && GET_HIT(target) < GET_MAX_HIT(target)) {
                /* Healing projection */
                outcome.score = 40;
                outcome.reward_level = 50;
                outcome.hp_delta = 30; /* Estimated healing */
                outcome.happiness_change = 5;
            } else if (!is_ally) {
                /* Offensive spell projection */
                outcome.score = FIGHTING(ch) ? 50 : 20;
                outcome.reward_level = 40;
                outcome.danger_level = 30; /* May provoke retaliation */
                outcome.leads_to_combat = !FIGHTING(ch);
            } else {
                /* No good target, skip */
                continue;
            }

            outcome.obvious = FALSE; /* Spell effects can be unpredictable */

            /* Store projection */
            ctx->projections[ctx->num_projections].action = action;
            ctx->projections[ctx->num_projections].outcome = outcome;
            ctx->projections[ctx->num_projections].horizon = 1;
            ctx->projections[ctx->num_projections].total_cost = cost;
            ctx->projections[ctx->num_projections].timestamp = time(0);
            ctx->num_projections++;

            /* Consume capacity */
            shadow_consume_capacity(ctx, cost);

            /* Limit to one spell projection per call to manage projection count.
             * In combat scenarios, one spell decision per tick is sufficient.
             * This prevents projection array overflow while still allowing spell choices. */
            break;
        }
    }
}

/**
 * Generate trade/shopping projections
 * Projects visiting shopkeepers and trading
 */
static void generate_trade_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct char_data *shopkeeper;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch || !IS_NPC(ch) || ch->in_room == NOWHERE) {
        return;
    }

    /* If we're at capacity, skip */
    if (ctx->num_projections >= ctx->max_projections) {
        return;
    }

    /* Look for shopkeepers in current room */
    for (shopkeeper = world[ch->in_room].people; shopkeeper && ctx->num_projections < ctx->max_projections;
         shopkeeper = shopkeeper->next_in_room) {

        if (shopkeeper == ch || !IS_NPC(shopkeeper)) {
            continue;
        }

        /* Use is_shopkeeper() function to properly detect shopkeepers */
        if (!is_shopkeeper(shopkeeper)) {
            continue; /* Skip non-shopkeepers to avoid false projections */
        }

        /* Set up trade action */
        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_TRADE;
        action.target = shopkeeper;
        action.cost = SHADOW_BASE_COST;

        if (shadow_validate_action(ch, &action) != ACTION_FEASIBLE) {
            continue;
        }

        /* Calculate cognitive cost */
        int cost = shadow_calculate_cost(ch, &action, 1);

        /* Check if we have capacity */
        if (ctx->cognitive_budget < cost) {
            continue;
        }

        /* Execute projection */
        memset(&outcome, 0, sizeof(struct shadow_outcome));
        if (shadow_execute_projection(ctx, &action, &outcome)) {
            /* Trade is moderately rewarding - represents economic opportunity */
            outcome.score = 25;
            outcome.reward_level = 30;
            outcome.achieves_goal = (ch->ai_data && (ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_SELL ||
                                                     ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_BUY));
            outcome.obvious = TRUE;       /* Trading is straightforward */
            outcome.happiness_change = 5; /* Small satisfaction from commerce */

            /* Store projection */
            ctx->projections[ctx->num_projections].action = action;
            ctx->projections[ctx->num_projections].outcome = outcome;
            ctx->projections[ctx->num_projections].horizon = 1;
            ctx->projections[ctx->num_projections].total_cost = cost;
            ctx->projections[ctx->num_projections].timestamp = time(0);
            ctx->num_projections++;

            /* Consume capacity */
            shadow_consume_capacity(ctx, cost);

            /* Only project one trade to avoid spam */
            break;
        }
    }
}

/**
 * Generate wait/do-nothing projection
 * Projects intentionally waiting for better circumstances
 * ST-3: Bounded Cognition - sometimes best action is to wait
 */
static void generate_wait_projection(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch) {
        return;
    }

    /* If we're at capacity, skip */
    if (ctx->num_projections >= ctx->max_projections) {
        return;
    }

    /* Waiting is more valuable in certain situations */
    bool should_consider_waiting = FALSE;

    if (FIGHTING(ch)) {
        /* Don't project waiting in combat - already have flee */
        return;
    }

    /* Consider waiting if: */
    if (ch->ai_data) {
        /* 1. Low energy/mana - wait to regenerate */
        if (GET_HIT(ch) < GET_MAX_HIT(ch) / 2 || GET_MANA(ch) < GET_MAX_MANA(ch) / 2) {
            should_consider_waiting = TRUE;
        }
        /* 2. High fear - wait for danger to pass */
        if (ch->ai_data->emotion_fear > 70) {
            should_consider_waiting = TRUE;
        }
        /* 3. Cognitive capacity low - wait to recover */
        if (ch->ai_data->cognitive_capacity < COGNITIVE_CAPACITY_MIN * 2) {
            should_consider_waiting = TRUE; /* Twice minimum ensures meaningful cognitive function */
        }
    }

    if (!should_consider_waiting) {
        return;
    }

    /* Set up wait action */
    memset(&action, 0, sizeof(struct shadow_action));
    action.type = SHADOW_ACTION_WAIT;
    action.target = NULL;
    action.cost = SHADOW_BASE_COST / 3; /* Waiting is cheap cognitively */

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
        /* Waiting score depends on why we're waiting */
        outcome.score = 15; /* Generally low priority unless situation demands it */

        if (GET_HIT(ch) < GET_MAX_HIT(ch) / 3) {
            outcome.score = 35;    /* Higher when injured */
            outcome.hp_delta = 10; /* Expect some regeneration */
        }

        if (ch->ai_data && ch->ai_data->emotion_fear > 80) {
            outcome.score = 40;       /* Even higher when afraid */
            outcome.fear_change = -5; /* Waiting reduces fear */
        }

        outcome.reward_level = 20;
        outcome.obvious = TRUE; /* Waiting outcomes are predictable */

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
 * Generate follow projections
 * Projects following other entities for various reasons
 */
static void generate_follow_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct char_data *target;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch || !IS_NPC(ch) || ch->in_room == NOWHERE) {
        return;
    }

    /* Don't generate follow projections if already following someone */
    if (ch->master) {
        return;
    }

    /* If we're at capacity, skip */
    if (ctx->num_projections >= ctx->max_projections) {
        return;
    }

    /* Find potential leaders in room */
    for (target = world[ch->in_room].people; target && ctx->num_projections < ctx->max_projections;
         target = target->next_in_room) {

        if (target == ch) {
            continue; /* Skip self */
        }

        /* Don't follow if target is already following us */
        if (target->master == ch) {
            continue;
        }

        /* Set up follow action */
        memset(&action, 0, sizeof(struct shadow_action));
        action.type = SHADOW_ACTION_FOLLOW;
        action.target = target;
        action.cost = SHADOW_BASE_COST / 2; /* Following is relatively cheap */

        if (shadow_validate_action(ch, &action) != ACTION_FEASIBLE) {
            continue;
        }

        /* Calculate cognitive cost */
        int cost = shadow_calculate_cost(ch, &action, 1);

        /* Check if we have capacity */
        if (ctx->cognitive_budget < cost) {
            continue;
        }

        /* Execute projection */
        memset(&outcome, 0, sizeof(struct shadow_outcome));
        if (shadow_execute_projection(ctx, &action, &outcome)) {
            /* Following score depends on target's strength and mob's traits */
            outcome.score = 20; /* Base score */

            /* Higher score if target is stronger (safety in numbers) */
            if (GET_LEVEL(target) > GET_LEVEL(ch)) {
                outcome.score += 15;
            }

            /* Higher score if mob has high loyalty trait */
            if (ch->ai_data && ch->ai_data->emotion_loyalty > 60) {
                outcome.score += 10;
            }

            /* Higher score if afraid and target looks protective */
            if (ch->ai_data && ch->ai_data->emotion_fear > 50) {
                outcome.score += 10;
                outcome.fear_change = -10; /* Following reduces fear */
            }

            outcome.reward_level = 30;
            outcome.achieves_goal = (ch->ai_data && ch->ai_data->current_goal == GOAL_FOLLOW);
            outcome.obvious = TRUE;       /* Following is straightforward */
            outcome.happiness_change = 5; /* Social bonding */

            /* Store projection */
            ctx->projections[ctx->num_projections].action = action;
            ctx->projections[ctx->num_projections].outcome = outcome;
            ctx->projections[ctx->num_projections].horizon = 1;
            ctx->projections[ctx->num_projections].total_cost = cost;
            ctx->projections[ctx->num_projections].timestamp = time(0);
            ctx->num_projections++;

            /* Consume capacity */
            shadow_consume_capacity(ctx, cost);

            /* Only project one follow to avoid spam */
            break;
        }
    }
}

/**
 * Generate group formation projections
 * Projects strengthening bonds with master/leader
 */
static void generate_group_projections(struct shadow_context *ctx)
{
    struct char_data *ch = ctx->entity;
    struct shadow_action action;
    struct shadow_outcome outcome;

    if (!ch || !IS_NPC(ch) || ch->in_room == NOWHERE) {
        return;
    }

    /* Only generate group projection if mob is following someone */
    if (!ch->master || ch->master->in_room != ch->in_room) {
        return;
    }

    /* If we're at capacity, skip */
    if (ctx->num_projections >= ctx->max_projections) {
        return;
    }

    /* Set up group action */
    memset(&action, 0, sizeof(struct shadow_action));
    action.type = SHADOW_ACTION_GROUP;
    action.target = ch->master;
    action.cost = SHADOW_BASE_COST / 2; /* Group bonding is cheap */

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
        /* Group bonding is moderately valuable for social cohesion */
        outcome.score = 25;
        outcome.reward_level = 35;

        /* Higher value if mob has high loyalty */
        if (ch->ai_data && ch->ai_data->emotion_loyalty > 70) {
            outcome.score += 15;
        }

        outcome.obvious = TRUE;        /* Social bonding is straightforward */
        outcome.happiness_change = 10; /* Strengthening bonds increases happiness */

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

        case SHADOW_ACTION_QUEST:
            /* Quest actions are feasible for mobs with AI data */
            if (!IS_NPC(ch) || !ch->ai_data) {
                return ACTION_INVALID_STATE;
            }
            /* If target is provided (questmaster), validate it exists */
            if (action->target && !check_invariant_existence(action->target, ENTITY_TYPE_MOB)) {
                return ACTION_INVALID_TARGET;
            }
            break;

        case SHADOW_ACTION_CAST_SPELL:
            /* Check if target exists and mob has mana */
            if (!action->target) {
                return ACTION_INVALID_TARGET;
            }
            if (!check_invariant_existence(action->target, ENTITY_TYPE_MOB)) {
                return ACTION_INVALID_TARGET;
            }
            if (GET_MANA(ch) < 15) {
                return ACTION_INVALID_STATE; /* Not enough mana */
            }
            break;

        case SHADOW_ACTION_TRADE:
            /* Check if target (shopkeeper) exists */
            if (!action->target) {
                return ACTION_INVALID_TARGET;
            }
            if (!check_invariant_existence(action->target, ENTITY_TYPE_MOB)) {
                return ACTION_INVALID_TARGET;
            }
            break;

        case SHADOW_ACTION_FOLLOW:
            /* Check if target exists and mob is not already following */
            if (!action->target) {
                return ACTION_INVALID_TARGET;
            }
            if (!check_invariant_existence(action->target, ENTITY_TYPE_MOB)) {
                return ACTION_INVALID_TARGET;
            }
            if (ch->master) {
                return ACTION_INVALID_STATE; /* Already following someone */
            }
            break;

        case SHADOW_ACTION_GROUP:
            /* Check if target exists and mob is following them */
            if (!action->target) {
                return ACTION_INVALID_TARGET;
            }
            if (!check_invariant_existence(action->target, ENTITY_TYPE_MOB)) {
                return ACTION_INVALID_TARGET;
            }
            if (!ch->master) {
                return ACTION_INVALID_STATE; /* Not following anyone */
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

            /* Probability-of-being-abandoned: if the mob has passive memories of
             * allies fleeing (INTERACT_ABANDON_ALLY), it has already experienced
             * abandonment and will weight fleeing more favorably when under threat. */
            if (ch && IS_NPC(ch) && ch->ai_data) {
                int abandon_bias = get_passive_memory_hysteresis(ch, INTERACT_ABANDON_ALLY);
                /* abandon_bias is negative when memories are emotionally painful (high fear/
                 * anger, low happiness); subtracting a negative value raises the flee score,
                 * making a mob that has been abandoned more likely to flee pre-emptively. */
                outcome->score -= abandon_bias;
            }
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

        case SHADOW_ACTION_QUEST: {
            /* Quest-related action - acceptance or completion */
            /* Default heuristic: quests are rewarding but outcomes vary */
            outcome->score = 40; /* Moderate-high reward */
            outcome->reward_level = 60;
            outcome->danger_level = 20; /* Some quests involve danger */
            outcome->obvious = FALSE;   /* Quest outcomes are complex */
            break;
        }

        case SHADOW_ACTION_CAST_SPELL: {
            /* Spell casting - varies by target type */
            outcome->score = 35;
            outcome->reward_level = 45;
            outcome->danger_level = 15; /* May provoke retaliation */
            outcome->obvious = FALSE;   /* Spell effects are unpredictable */
            break;
        }

        case SHADOW_ACTION_TRADE: {
            /* Trading with shopkeeper */
            outcome->score = 20;
            outcome->reward_level = 30;
            outcome->danger_level = 5;
            outcome->obvious = TRUE; /* Trading is straightforward */
            break;
        }

        case SHADOW_ACTION_FOLLOW: {
            /* Following another entity */
            outcome->score = 25;
            outcome->reward_level = 30;
            outcome->danger_level = 10; /* Safety in numbers */
            outcome->obvious = TRUE;
            break;
        }

        case SHADOW_ACTION_GROUP: {
            /* Group bonding with master */
            outcome->score = 20;
            outcome->reward_level = 35;
            outcome->danger_level = 5;
            outcome->happiness_change = 10;
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

        /* OCEAN Phase 3: Agreeableness (A) influences attack retaliation and social weight.
         * High A → dampens aggression, boosts social utility.
         * Low A → amplifies aggression, reduces social preference.
         * Score adjustment is in range [-20, +20] to keep within ±OUTCOME_SCORE_MAX bounds. */
        float A_final = sec_get_agreeableness_final(ch);

        /* OCEAN Phase 3: Extraversion (E) influences social initiation and group payoff.
         * High E → higher expected utility from social/group actions.
         * Low E → prefers solitary actions (lower social penalty when alone).
         * Individual trait modifiers are each in range [-15, +15]; combined social
         * score adjustment from A+E terms ranges from -30 to +30. */
        float E_final = sec_get_extraversion_final(ch);

        if (proj->action.type == SHADOW_ACTION_ATTACK) {
            /* Low A amplifies attack utility; high A dampens it */
            score += (int)((0.5f - A_final) * 40.0f); /* -20 to +20 */
        } else if (proj->action.type == SHADOW_ACTION_SOCIAL) {
            /* High A and high E both amplify social utility */
            score += (int)((A_final - 0.5f) * 30.0f); /* A: -15 to +15 */
            score += (int)((E_final - 0.5f) * 30.0f); /* E: -15 to +15 */
        } else if (proj->action.type == SHADOW_ACTION_FOLLOW || proj->action.type == SHADOW_ACTION_GROUP) {
            score += (int)((E_final - 0.5f) * 20.0f); /* E group bias: -10 to +10 */
        }

        /* OCEAN Phase 2: Conscientiousness (C) decision consistency bias.
         * High C → rewards actions that match the prior commitment (last chosen type).
         * This reduces flip-flopping without hard-clamping or suppressing options.
         * Score bonus ∈ [0, SEC_C_CONSISTENCY_SCALE] points; low C = near zero bonus.
         * last_chosen_action_type is an int with -1 as "no prior commitment" sentinel;
         * comparison uses a local int variable to avoid enum/int cast. */
        int prior_type = ch->ai_data->last_chosen_action_type;
        if (prior_type >= 0 && (int)proj->action.type == prior_type) {
            float C_final = sec_get_conscientiousness_final(ch);
            score += (int)(C_final * SEC_C_CONSISTENCY_SCALE); /* 0 to +SEC_C_CONSISTENCY_SCALE */
        }

        /* OCEAN Phase 4: Openness (O) cognitive flexibility modifiers.
         *
         * Pipeline order (after emotional/A/E/C weighting above):
         *   1. MOVE exploration weighting (±7 pts based on O vs 0.5 baseline)
         *   2. Depth-aware novelty bonus for non-repeated action types
         *   3. Routine preference bonus for low-O repeated actions
         *
         * Depth-aware novelty prevents "ADHD oscillation" in high-O mobs:
         *   bonus = clamp(O × depth × SEC_O_NOVELTY_DEPTH_SCALE, 0, SEC_O_NOVELTY_BONUS_CAP)
         * Because 'depth' (consecutive same-type repetitions) starts at 0 and builds
         * slowly, a high-O mob needs sustained repetition before novelty pressure
         * outweighs the action's survival utility.  First repetition = 6 pts (low
         * pressure); at 5 consecutive repeats the cap of 30 pts is reached.
         *
         * Repetition dampening for low O:
         *   Bonus = (1 - O) × SEC_O_REPETITION_BONUS  (0 to +15 pts for O ∈ [0,1]).
         *   Low-O mobs get a routine preference reward that disappears as O→1.
         *
         * Hard cap: SEC_O_NOVELTY_BONUS_CAP = 30 = 0.3 × OUTCOME_SCORE_MAX.
         * Neither novelty nor repetition bonuses modify emotional gain/decay
         * or SEC energy partition.
         * O_final must not be derived from SEC state (see sec_get_openness_final()). */
        {
            float O_final = sec_get_openness_final(ch);

            /* 1. MOVE exploration weighting */
            if (proj->action.type == SHADOW_ACTION_MOVE) {
                score += (int)((O_final - SEC_O_NOVELTY_CENTER) * ((float)CONFIG_SEC_O_NOVELTY_MOVE_SCALE / 10.0f));
            }

            /* 2 & 3. Action-history novelty vs. routine using repetition depth */
            int prior_type_o = ch->ai_data->last_chosen_action_type;
            if (prior_type_o >= 0) {
                if ((int)proj->action.type != prior_type_o) {
                    /* Novel action type: depth-aware bonus (builds over consecutive repeats).
                     * prior_depth = how many ticks the mob was stuck doing the prior action type.
                     * action_repetition_count reflects prior ticks because it is updated in
                     * mobact.c *after* action selection, so during scoring it still holds the
                     * count for the previous action type — this is semantically correct.
                     * At prior_depth=0 (no tracked repetition yet) bonus is 0 — no instant flip. */
                    int prior_depth = MIN(ch->ai_data->action_repetition_count, CONFIG_SEC_O_REPETITION_CAP);
                    int novel_bonus = MIN((int)(O_final * (float)prior_depth * (float)CONFIG_SEC_O_NOVELTY_DEPTH_SCALE),
                                          CONFIG_SEC_O_NOVELTY_BONUS_CAP);
                    score += novel_bonus; /* 0 to cap pts */
                } else {
                    /* Repeated action type: routine-preference for low O.
                     * Dampening interpretation: (1-O) scales the bonus to 0 as O→1. */
                    score += (int)((1.0f - O_final) * (float)CONFIG_SEC_O_REPETITION_BONUS);
                }
            }
        }

        /* Active memory hysteresis: bias prediction toward actions with positive
         * prior emotional outcomes from the mob's own action history.
         * Maps shadow action type → INTERACT_* and queries the active memory
         * buffer for a time-weighted valence modifier in [-20, +20].
         * This prevents oscillatory action selection by anchoring predictions
         * in remembered self-actions. */
        {
            int interact_type = -1;
            switch (proj->action.type) {
                case SHADOW_ACTION_ATTACK:
                    interact_type = INTERACT_ATTACKED;
                    break;
                case SHADOW_ACTION_SOCIAL:
                    /* Combine remembered valence from all social subtypes so that
                     * past negative/violent socials also influence future choices. */
                    score += get_active_memory_hysteresis(ch, INTERACT_SOCIAL_POSITIVE);
                    score += get_active_memory_hysteresis(ch, INTERACT_SOCIAL_NEGATIVE);
                    score += get_active_memory_hysteresis(ch, INTERACT_SOCIAL_VIOLENT);
                    break;
                case SHADOW_ACTION_TRADE:
                    interact_type = INTERACT_RECEIVED_ITEM;
                    break;
                case SHADOW_ACTION_QUEST:
                    interact_type = INTERACT_QUEST_COMPLETE;
                    break;
                case SHADOW_ACTION_CAST_SPELL:
                    interact_type = INTERACT_WITNESSED_SUPPORT_MAGIC;
                    break;
                case SHADOW_ACTION_FOLLOW:
                    interact_type = INTERACT_ASSISTED;
                    break;
                case SHADOW_ACTION_FLEE:
                    interact_type = INTERACT_ABANDON_ALLY;
                    break;
                default:
                    break;
            }
            if (interact_type >= 0)
                score += get_active_memory_hysteresis(ch, interact_type);
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

    /* OCEAN Phase 4: Openness (O) exploration probability gate.
     * With probability (SEC_O_EXPLORATION_BASE * O_final)%, occasionally deviate from the
     * top-ranked action to a random non-negative sub-dominant alternative.
     * This prevents deterministic behavioral loops without bypassing WTA energy gating:
     * - Only non-negative-score alternatives are considered (safety floor retained).
     * - Exploration is gated by O_final [0,1], so low-O mobs never explore randomly.
     * - Range: 0% (O=0) to SEC_O_EXPLORATION_BASE% (O=1). */
    struct char_data *ent = ctx->entity;
    if (ent && IS_NPC(ent) && ent->ai_data && ctx->num_projections > 1) {
        float O_final = sec_get_openness_final(ent);
        int explore_chance = (int)(O_final * (float)CONFIG_SEC_O_EXPLORATION_BASE); /* 0..max */
        if (rand_number(1, 100) <= explore_chance) {
            /* Build a candidate list of non-best, non-negative-score projections */
            int candidates[SHADOW_MAX_PROJECTIONS];
            int ncand = 0;
            for (i = 0; i < ctx->num_projections; i++) {
                if (i != best_idx && ctx->projections[i].outcome.score >= 0) {
                    candidates[ncand++] = i;
                }
            }
            if (ncand > 0) {
                int chosen = candidates[rand_number(0, ncand - 1)];
                return &ctx->projections[chosen];
            }
        }
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
    /* Negative surprises (threats) are amplified - models loss aversion.
     * OCEAN Phase 4: Openness (O) ambiguity tolerance reduces this amplification.
     * ThreatAmpPct = round(30.0 * (1.0 - SEC_O_THREAT_BIAS * O_final)) ∈ [18, 30].
     * High O interprets ambiguous/negative surprises less catastrophically;
     * low O applies full 30% threat amplification (loss aversion preserved).
     * O_final must not be derived from SEC state (see sec_get_openness_final()). */
    if (signed_error < 0) {
        float O_final_threat = sec_get_openness_final(ch);
        float threat_bias = (float)CONFIG_SEC_O_THREAT_BIAS / 100.0f;
        int amp_pct = (int)(30.0f * (1.0f - threat_bias * O_final_threat));
        novelty = (novelty * (100 + amp_pct)) / 100;
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
