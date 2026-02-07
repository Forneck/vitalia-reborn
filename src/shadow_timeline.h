/**
 * @file shadow_timeline.h
 * Shadow Timeline: Cognitive Future Simulation Layer (RFC-0001)
 *
 * The Shadow Timeline is a non-authoritative, non-causal, isolated simulation layer
 * that allows autonomous entities (players and mobs) to internally explore possible
 * future outcomes of actions without modifying the real world state.
 *
 * Core Principles (Axioms):
 * - ST-1: Observational Only - Never mutates real world state
 * - ST-2: Invariant Preservation - No simulated action may violate global invariants
 * - ST-3: Bounded Cognition - Intentionally incomplete, no exhaustive search
 * - ST-4: Subjectivity - Different entities generate different projections
 * - ST-5: Non-determinism - Repeated evaluations may yield different results
 *
 * Part of Vitalia Reborn MUD engine.
 * Copyright (C) 2026 Vitalia Reborn Design
 */

#ifndef _SHADOW_TIMELINE_H_
#define _SHADOW_TIMELINE_H_

#include "structs.h"

/* Shadow Timeline configuration constants */
#define SHADOW_MAX_HORIZON 5        /**< Maximum projection steps into future */
#define SHADOW_DEFAULT_HORIZON 3    /**< Default projection horizon */
#define SHADOW_MAX_PROJECTIONS 10   /**< Maximum projections per decision */
#define SHADOW_BASE_COST 10         /**< Base cognitive cost per projection */

/* Cognitive capacity constants */
#define COGNITIVE_CAPACITY_MAX 1000     /**< Maximum cognitive capacity */
#define COGNITIVE_CAPACITY_REGEN 50     /**< Capacity regeneration per tick */
#define COGNITIVE_CAPACITY_MIN 100      /**< Minimum to attempt projection */

/* Action feasibility results */
#define ACTION_FEASIBLE 0           /**< Action is valid and can be simulated */
#define ACTION_INVALID_TARGET 1     /**< Target doesn't exist or invalid */
#define ACTION_INVALID_STATE 2      /**< Entity state doesn't allow action */
#define ACTION_VIOLATES_INVARIANT 3 /**< Would violate global invariants */
#define ACTION_IMPOSSIBLE 4         /**< Ontologically impossible */

/* Projection outcome quality scores */
#define OUTCOME_SCORE_MIN -100      /**< Worst possible outcome */
#define OUTCOME_SCORE_MAX 100       /**< Best possible outcome */

/**
 * Action types that can be projected in Shadow Timeline
 */
enum shadow_action_type {
    SHADOW_ACTION_MOVE,        /**< Movement to another room */
    SHADOW_ACTION_ATTACK,      /**< Attack another entity */
    SHADOW_ACTION_FLEE,        /**< Flee from combat */
    SHADOW_ACTION_USE_ITEM,    /**< Use an item or equipment */
    SHADOW_ACTION_CAST_SPELL,  /**< Cast a spell */
    SHADOW_ACTION_SOCIAL,      /**< Perform social action */
    SHADOW_ACTION_TRADE,       /**< Trading/shopping action */
    SHADOW_ACTION_QUEST,       /**< Quest-related action */
    SHADOW_ACTION_WAIT,        /**< Wait/do nothing */
    SHADOW_ACTION_FOLLOW,      /**< Follow another entity */
    SHADOW_ACTION_GROUP,       /**< Group formation action */
    SHADOW_ACTION_MAX          /**< Sentinel value */
};

/**
 * Candidate action for Shadow Timeline projection
 * Represents a single possible action to simulate
 */
struct shadow_action {
    enum shadow_action_type type;  /**< Type of action */
    void *target;                  /**< Target entity/object (or NULL) */
    int direction;                 /**< For movement actions */
    int spell_num;                 /**< For spell casting */
    int item_slot;                 /**< For item usage */
    int cost;                      /**< Cognitive cost to simulate this action */
};

/**
 * Projected outcome of a shadow action
 * Represents the estimated state after executing an action
 */
struct shadow_outcome {
    int score;                     /**< Quality score of outcome (-100 to +100) */
    int hp_delta;                  /**< Estimated HP change */
    int danger_level;              /**< Estimated danger (0-100) */
    int reward_level;              /**< Estimated reward (0-100) */
    
    /* Emotional impact prediction */
    sh_int fear_change;            /**< Predicted fear change */
    sh_int anger_change;           /**< Predicted anger change */
    sh_int happiness_change;       /**< Predicted happiness change */
    
    /* State flags */
    bool leads_to_combat;          /**< Would trigger combat */
    bool leads_to_death;           /**< High probability of death */
    bool achieves_goal;            /**< Advances toward current goal */
    bool obvious;                  /**< Outcome is obvious/predictable */
};

/**
 * Complete projection: action + predicted trajectory
 * π in the formal model: <a, π, C>
 */
struct shadow_projection {
    struct shadow_action action;     /**< The action being projected */
    struct shadow_outcome outcome;   /**< Predicted outcome */
    int horizon;                     /**< How many steps projected */
    int total_cost;                  /**< Total cognitive cost */
    time_t timestamp;                /**< When projection was created */
};

/**
 * Shadow Timeline context for a cognitive entity
 * Contains all projection data for decision-making
 * ST_e(t) in the formal model
 */
struct shadow_context {
    struct char_data *entity;            /**< The entity this context belongs to */
    struct shadow_projection *projections; /**< Array of projections */
    int num_projections;                 /**< Number of valid projections */
    int max_projections;                 /**< Maximum projections allowed */
    int cognitive_budget;                /**< Available cognitive capacity */
    int horizon;                         /**< Current projection horizon */
    bool active;                         /**< Whether context is active */
};

/* Cognitive entity detection */
/**
 * Check if entity can use Shadow Timeline (player or autonomous mob)
 * Only decision-making entities require foresight
 */
#define IS_COGNITIVE_ENTITY(ch) \
    ((ch) && !IS_NPC(ch) || (IS_NPC(ch) && (ch)->ai_data))

/**
 * Get cognitive capacity for entity
 * Based on emotional intelligence and other factors
 */
#define GET_COGNITIVE_CAPACITY(ch) \
    (IS_COGNITIVE_ENTITY(ch) && IS_NPC(ch) && (ch)->ai_data ? \
     (ch)->ai_data->cognitive_capacity : COGNITIVE_CAPACITY_MAX)

/* Core Shadow Timeline functions */

/**
 * Initialize a shadow context for an entity
 * @param ch The cognitive entity
 * @return Pointer to initialized context, or NULL on failure
 */
struct shadow_context *shadow_init_context(struct char_data *ch);

/**
 * Free a shadow context and all associated projections
 * @param ctx The context to free
 */
void shadow_free_context(struct shadow_context *ctx);

/**
 * Generate projections for available actions
 * Core function that populates the shadow context with possible futures
 * @param ctx The shadow context
 * @return Number of projections generated
 */
int shadow_generate_projections(struct shadow_context *ctx);

/**
 * Validate that an action is feasible and doesn't violate invariants
 * Implements ST-2 (Invariant Preservation)
 * @param ch The entity attempting the action
 * @param action The action to validate
 * @return ACTION_FEASIBLE or an error code
 */
int shadow_validate_action(struct char_data *ch, struct shadow_action *action);

/**
 * Execute a projection in isolated simulation
 * Implements ST-1 (Observational Only) - never mutates real state
 * @param ctx The shadow context
 * @param action The action to simulate
 * @param outcome Output parameter for predicted outcome
 * @return TRUE on success, FALSE if simulation failed
 */
bool shadow_execute_projection(struct shadow_context *ctx,
                                struct shadow_action *action,
                                struct shadow_outcome *outcome);

/**
 * Score and rank projections based on entity's goals and state
 * Implements ST-4 (Subjectivity) - different entities score differently
 * @param ctx The shadow context
 */
void shadow_score_projections(struct shadow_context *ctx);

/**
 * Select the best action from generated projections
 * @param ctx The shadow context
 * @return Pointer to best projection, or NULL if none suitable
 */
struct shadow_projection *shadow_select_best_action(struct shadow_context *ctx);

/**
 * Calculate cognitive cost for an action
 * Implements ST-3 (Bounded Cognition)
 * @param ch The entity
 * @param action The action to cost
 * @param horizon Projection horizon
 * @return Cognitive cost
 */
int shadow_calculate_cost(struct char_data *ch, struct shadow_action *action, int horizon);

/**
 * Consume cognitive capacity for a projection
 * @param ctx The shadow context
 * @param cost Amount of capacity to consume
 * @return TRUE if capacity was available, FALSE otherwise
 */
bool shadow_consume_capacity(struct shadow_context *ctx, int cost);

/**
 * Regenerate cognitive capacity over time
 * Called periodically to restore capacity
 * @param ch The entity
 */
void shadow_regenerate_capacity(struct char_data *ch);

/**
 * Check if outcome is "obvious" to the entity
 * Obvious outcomes may be collapsed or treated heuristically
 * @param ch The entity
 * @param action The action
 * @param outcome The predicted outcome
 * @return TRUE if obvious to this entity
 */
bool shadow_is_obvious(struct char_data *ch,
                       struct shadow_action *action,
                       struct shadow_outcome *outcome);

/**
 * Apply subjectivity bias based on entity's emotional state and genetics
 * Implements ST-4 (Subjectivity)
 * @param ch The entity
 * @param outcome The outcome to bias
 */
void shadow_apply_subjectivity(struct char_data *ch, struct shadow_outcome *outcome);

/* Debug and logging functions */

/**
 * Log shadow timeline activity (shadow-only logging)
 * @param ctx The shadow context
 * @param message The log message
 */
void shadow_log(struct shadow_context *ctx, const char *message);

/**
 * Dump shadow context for debugging
 * @param ctx The shadow context
 */
void shadow_dump_context(struct shadow_context *ctx);

#endif /* _SHADOW_TIMELINE_H_ */
