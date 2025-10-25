/**************************************************************************
 *  File: graph.c                                           Part of tbaMUD *
 *  Usage: Various graph algorithms.                                       *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "act.h" /* for the do_say command */
#include "constants.h"
#include "graph.h"
#include "fight.h"

/* local functions */
static int VALID_EDGE(room_rnum x, int y);
static void bfs_enqueue(room_rnum room, int dir);
static void bfs_dequeue(void);
static void bfs_clear_queue(void);

/** Helper function to validate room numbers for pathfinding operations.
 * @param room The room number to validate
 * @return Non-zero if the room is valid, 0 if invalid (NOWHERE or out of bounds) */
static int is_valid_room(room_rnum room) { return (room != NOWHERE && room >= 0 && room <= top_of_world); }

/* Calculate movement cost for a room using the updated formula:
 * sector_cost[current_room] * weather_modifier[current_zone] */
int calculate_movement_cost(struct char_data *ch, room_rnum room)
{
    int base_cost;
    float weather_modifier;
    int final_cost;

    if (room == NOWHERE || room > top_of_world) {
        return 1; /* Default cost if invalid room */
    }

    /* Get base movement cost from sector type */
    base_cost = movement_loss[SECT(room)];

    /* Get weather modifier for the character's current location */
    weather_modifier = get_weather_movement_modifier(ch);

    /* Calculate final cost using the new formula */
    final_cost = (int)(base_cost * weather_modifier);

    /* Ensure minimum cost of 1 */
    if (final_cost < 1)
        final_cost = 1;

    return final_cost;
}

/* Advanced pathfinding structures for state-based search */
#define MAX_COLLECTED_KEYS 5               /* Reduced from 50 to limit complexity */
#define MAX_VISITED_STATES 1000            /* Reduced from 1000 to limit memory usage */
#define MAX_PATHFIND_ITERATIONS_LIMIT 2000 /* Maximum compile-time limit for iterations */
#define MAX_ZONE_PATH_LIMIT 60             /* Maximum compile-time limit for zone path */

/* Dynamic scaling functions for pathfinding parameters */
static int get_dynamic_max_pathfind_iterations(void)
{
    /* Scale based on number of rooms, with minimum and maximum bounds */
    int base_iterations = CONFIG_MAX_PATHFIND_ITERATIONS;
    int world_rooms = top_of_world + 1;
    int world_zones = top_of_zone_table + 1;

    /* If configured value is 0, use dynamic scaling */
    if (base_iterations == 0) {
        /* Base formula: rooms * 2 + zones * 10, bounded by limits */
        int dynamic_value = (world_rooms + world_zones);
        return MIN(MAX(dynamic_value, 1000), MAX_PATHFIND_ITERATIONS_LIMIT);
    }

    /* Use configured value, but ensure it's within limits */
    return MIN(MAX(base_iterations, 100), MAX_PATHFIND_ITERATIONS_LIMIT);
}

static int get_dynamic_max_zone_path(void)
{
    /* Scale based on number of zones, with minimum and maximum bounds */
    int base_zones = CONFIG_MAX_ZONE_PATH;
    int world_zones = top_of_zone_table + 1;

    /* If configured value is 0, use dynamic scaling */
    if (base_zones == 0) {
        /* Dynamic scaling: at least 25% of total zones, max 75% */
        int dynamic_value = MAX(world_zones / 4, MIN(world_zones * 3 / 4, 200));
        return MIN(MAX(dynamic_value, 50), MAX_ZONE_PATH_LIMIT);
    }

    /* Use configured value, but ensure it's within limits */
    return MIN(MAX(base_zones, 10), MAX_ZONE_PATH_LIMIT);
}

struct path_state {
    room_rnum room;                         /* Current room */
    int mv_available;                       /* Movement points available */
    int collected_keys[MAX_COLLECTED_KEYS]; /* Array of collected key vnums */
    int num_keys;                           /* Number of keys collected */
    int path_cost;                          /* Total movement cost to reach this state */
    int first_dir;                          /* First direction taken from start */
};

struct state_queue_struct {
    struct path_state state;
    struct state_queue_struct *next;
};

struct bfs_queue_struct {
    room_rnum room;
    char dir;
    struct bfs_queue_struct *next;
};

/* Static queues for advanced pathfinding */
static struct state_queue_struct *state_queue_head = NULL, *state_queue_tail = NULL;

static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0;

/* Global pathfinding statistics for monitoring */
static long pathfind_calls_total = 0;
static long pathfind_cache_hits = 0;
static long advanced_pathfind_calls = 0;
static long zone_optimized_calls = 0;
static long keys_optimized_total = 0;

/* Cache the current time to reduce system calls during pulse processing */
static time_t cached_time = 0;
static int cached_time_pulse = -1;

/* Advanced pathfinding function declarations */
static void state_enqueue(struct path_state *state);
static struct path_state *state_dequeue(void);
static void state_clear_queue(void);
static int has_key_in_state(struct path_state *state, obj_vnum key);
static void add_key_to_state(struct path_state *state, obj_vnum key);
static int can_pass_door(struct char_data *ch, struct path_state *state, room_rnum from, int dir);
static int is_state_visited(struct path_state *state, struct path_state **visited_states, int num_visited);
static struct obj_data *find_key_in_room(room_rnum room, obj_vnum key_vnum) __attribute__((unused));

/* Zone-based optimization functions */
static int get_zones_between(zone_rnum src_zone, zone_rnum target_zone, zone_rnum *zone_path, int max_zones);
static int count_keys_in_zones(zone_rnum *zones, int num_zones);
static int get_required_keys_for_path(struct char_data *ch, room_rnum src, room_rnum target, obj_vnum *required_keys,
                                      int max_keys);
static int zone_has_connection_to(zone_rnum from_zone, zone_rnum to_zone);

/* Cache function declarations */
static void cache_pathfind_result_priority(room_rnum src, room_rnum target, int direction, int priority);

/* Get cached time to reduce system calls - updates once per pulse */
static time_t get_cached_time(void)
{
    extern unsigned long pulse; /* From comm.c */
    if (cached_time_pulse != (int)pulse) {
        cached_time = time(NULL);
        cached_time_pulse = (int)pulse;
    }
    return cached_time;
}

/* Utility macros */
#define MARK(room) (SET_BIT_AR(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define UNMARK(room) (REMOVE_BIT_AR(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define IS_MARKED(room) (ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y) (world[(x)].dir_option[(y)]->to_room)
#define IS_CLOSED(x, y) (EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED))

static int VALID_EDGE(room_rnum x, int y)
{
    if (world[x].dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
        return 0;
    if (CONFIG_TRACK_T_DOORS == FALSE && IS_CLOSED(x, y))
        return 0;
    if (ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK) || IS_MARKED(TOROOM(x, y)))
        return 0;

    return 1;
}

static void bfs_enqueue(room_rnum room, int dir)
{
    struct bfs_queue_struct *curr;

    CREATE(curr, struct bfs_queue_struct, 1);
    curr->room = room;
    curr->dir = dir;
    curr->next = 0;

    if (queue_tail) {
        queue_tail->next = curr;
        queue_tail = curr;
    } else
        queue_head = queue_tail = curr;
}

static void bfs_dequeue(void)
{
    struct bfs_queue_struct *curr;

    curr = queue_head;

    if (!(queue_head = queue_head->next))
        queue_tail = 0;
    free(curr);
}

static void bfs_clear_queue(void)
{
    int cleared = 0;
    while (queue_head && cleared < 10000) { /* Safety limit to prevent infinite loops */
        bfs_dequeue();
        cleared++;
    }
    if (cleared >= 10000) {
        log1("SYSERR: bfs_clear_queue exceeded safety limit, possible memory corruption");
    }
}

/* Advanced state-based pathfinding functions */

static void state_enqueue(struct path_state *state)
{
    struct state_queue_struct *curr;

    CREATE(curr, struct state_queue_struct, 1);
    curr->state = *state; /* Copy the state */
    curr->next = NULL;

    if (state_queue_tail) {
        state_queue_tail->next = curr;
        state_queue_tail = curr;
    } else {
        state_queue_head = state_queue_tail = curr;
    }
}

static struct path_state *state_dequeue(void)
{
    struct state_queue_struct *curr;
    static struct path_state result;

    if (!state_queue_head)
        return NULL;

    curr = state_queue_head;
    result = curr->state; /* Copy the state */

    if (!(state_queue_head = state_queue_head->next))
        state_queue_tail = NULL;

    free(curr);
    return &result;
}

static void state_clear_queue(void)
{
    int cleared = 0;
    while (state_queue_head && cleared < 10000) { /* Safety limit to prevent infinite loops */
        struct state_queue_struct *curr = state_queue_head;
        state_queue_head = state_queue_head->next;
        free(curr);
        cleared++;
    }
    if (cleared >= 10000) {
        log1("SYSERR: state_clear_queue exceeded safety limit, possible memory corruption");
    }
    state_queue_tail = NULL;
}

static int has_key_in_state(struct path_state *state, obj_vnum key)
{
    int i;
    for (i = 0; i < state->num_keys; i++) {
        if (state->collected_keys[i] == key)
            return 1;
    }
    return 0;
}

static void add_key_to_state(struct path_state *state, obj_vnum key)
{
    if (state->num_keys >= MAX_COLLECTED_KEYS)
        return; /* Cannot add more keys */

    if (!has_key_in_state(state, key)) {
        state->collected_keys[state->num_keys] = key;
        state->num_keys++;
    }
}

static int can_pass_door(struct char_data *ch, struct path_state *state, room_rnum from, int dir)
{
    struct room_direction_data *exit;

    if (!world[from].dir_option[dir])
        return 0;

    exit = world[from].dir_option[dir];

    /* If door is not closed or locked, can pass */
    if (!IS_SET(exit->exit_info, EX_CLOSED) || !IS_SET(exit->exit_info, EX_LOCKED))
        return 1;

    /* If locked, check if we have the key */
    if (exit->key == NOTHING)
        return 0; /* No key specified for locked door */

    /* Check if player currently has the key */
    if (has_key(ch, exit->key))
        return 1;

    /* Check if the key is in our collected state */
    return has_key_in_state(state, exit->key);
}

static int is_state_visited(struct path_state *state, struct path_state **visited_states, int num_visited)
{
    int i, j;

    for (i = 0; i < num_visited; i++) {
        if (visited_states[i]->room == state->room && visited_states[i]->num_keys == state->num_keys) {

            /* Check if key sets are the same */
            int keys_match = 1;
            for (j = 0; j < state->num_keys; j++) {
                if (!has_key_in_state(visited_states[i], state->collected_keys[j])) {
                    keys_match = 0;
                    break;
                }
            }
            if (keys_match)
                return 1;
        }
    }
    return 0;
}

static struct obj_data *find_key_in_room(room_rnum room, obj_vnum key_vnum)
{
    struct obj_data *obj;

    if (room == NOWHERE || room > top_of_world)
        return NULL;

    for (obj = world[room].contents; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == key_vnum)
            return obj;
    }
    return NULL;
}

/* Zone-based optimization functions implementation */

/**
 * Check if one zone has direct connection to another zone
 * Returns 1 if connection exists, 0 otherwise
 */
static int zone_has_connection_to(zone_rnum from_zone, zone_rnum to_zone)
{
    room_rnum nr, to_room;
    int first, last, j;

    if (from_zone == NOWHERE || to_zone == NOWHERE || from_zone > top_of_zone_table || to_zone > top_of_zone_table)
        return 0;

    /* Skip zones marked as CLOSED - players and mobs shouldn't access them */
    if (ZONE_FLAGGED(from_zone, ZONE_CLOSED) || ZONE_FLAGGED(to_zone, ZONE_CLOSED))
        return 0;

    if (from_zone == to_zone)
        return 1; /* Same zone is always connected */

    /* Get room range for the source zone */
    first = zone_table[from_zone].bot;
    last = zone_table[from_zone].top;

    /* Check all rooms in the source zone for exits to target zone */
    for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++) {
        if (GET_ROOM_VNUM(nr) >= first) {
            for (j = 0; j < DIR_COUNT; j++) {
                if (world[nr].dir_option[j]) {
                    to_room = world[nr].dir_option[j]->to_room;
                    if (to_room != NOWHERE && world[to_room].zone == to_zone) {
                        return 1; /* Found direct connection */
                    }
                }
            }
        }
    }

    return 0; /* No direct connection found */
}

/**
 * Get the zones between source and target zones using BFS
 * Returns the number of zones in the path, or -1 if no path found
 */
static int get_zones_between(zone_rnum src_zone, zone_rnum target_zone, zone_rnum *zone_path, int max_zones)
{
    static zone_rnum visited_zones[MAX_ZONE_PATH_LIMIT];
    static zone_rnum queue[MAX_ZONE_PATH_LIMIT];
    static int parent[MAX_ZONE_PATH_LIMIT];
    int visited_count = 0, queue_head = 0, queue_tail = 0;
    int i, j;
    zone_rnum current_zone;

    if (src_zone == NOWHERE || target_zone == NOWHERE || !zone_path || max_zones <= 0)
        return -1;

    if (src_zone == target_zone) {
        zone_path[0] = src_zone;
        return 1;
    }

    /* Initialize */
    for (i = 0; i < MAX_ZONE_PATH_LIMIT; i++) {
        visited_zones[i] = NOWHERE;
        parent[i] = -1;
    }

    /* Start BFS */
    queue[queue_tail++] = src_zone;
    visited_zones[visited_count++] = src_zone;
    parent[0] = -1;

    while (queue_head < queue_tail && visited_count < get_dynamic_max_zone_path()) {
        current_zone = queue[queue_head++];

        if (current_zone == target_zone) {
            /* Found target, reconstruct path */
            int path_length = 0;
            int current_idx = -1;

            /* Find current zone index in visited array */
            for (i = 0; i < visited_count; i++) {
                if (visited_zones[i] == target_zone) {
                    current_idx = i;
                    break;
                }
            }

            /* Reconstruct path backwards */
            static zone_rnum temp_path[MAX_ZONE_PATH_LIMIT];
            int temp_length = 0;

            while (current_idx != -1 && temp_length < get_dynamic_max_zone_path()) {
                temp_path[temp_length++] = visited_zones[current_idx];
                current_idx = parent[current_idx];
            }

            /* Reverse path and copy to output */
            path_length = (temp_length > max_zones) ? max_zones : temp_length;
            for (i = 0; i < path_length; i++) {
                zone_path[i] = temp_path[path_length - 1 - i];
            }

            return path_length;
        }

        /* Explore neighboring zones - iterate through all zone indices */
        for (j = 0; j <= top_of_zone_table && visited_count < get_dynamic_max_zone_path(); j++) {
            /* Skip if already visited */
            int already_visited = 0;
            for (i = 0; i < visited_count; i++) {
                if (visited_zones[i] == j) {
                    already_visited = 1;
                    break;
                }
            }

            if (!already_visited && zone_has_connection_to(current_zone, j)) {
                queue[queue_tail++] = j;
                visited_zones[visited_count] = j;

                /* Find parent index - fix: find index of current_zone before incrementing visited_count */
                for (i = 0; i < visited_count; i++) {
                    if (visited_zones[i] == current_zone) {
                        parent[visited_count] = i;
                        break;
                    }
                }

                visited_count++;
                if (queue_tail >= get_dynamic_max_zone_path())
                    break;
            }
        }
    }

    return -1; /* No path found */
}

/**
 * Count the number of ITEM_KEY objects in the specified zones
 */
static int count_keys_in_zones(zone_rnum *zones, int num_zones)
{
    int total_keys = 0;
    int i, j;
    room_rnum nr;
    struct obj_data *obj;

    if (!zones || num_zones <= 0)
        return 0;

    for (i = 0; i < num_zones; i++) {
        zone_rnum zone = zones[i];
        if (zone == NOWHERE || zone > top_of_zone_table)
            continue;

        int first = zone_table[zone].bot;
        int last = zone_table[zone].top;

        /* Count keys in all rooms of this zone */
        for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++) {
            if (GET_ROOM_VNUM(nr) >= first) {
                /* Count keys in room */
                for (obj = world[nr].contents; obj; obj = obj->next_content) {
                    if (GET_OBJ_TYPE(obj) == ITEM_KEY) {
                        total_keys++;
                    }
                }

                /* Count keys carried by NPCs in room */
                struct char_data *ch_in_room;
                for (ch_in_room = world[nr].people; ch_in_room; ch_in_room = ch_in_room->next_in_room) {
                    if (IS_NPC(ch_in_room)) {
                        for (obj = ch_in_room->carrying; obj; obj = obj->next_content) {
                            if (GET_OBJ_TYPE(obj) == ITEM_KEY) {
                                total_keys++;
                            }
                        }

                        /* Check equipped items */
                        for (j = 0; j < NUM_WEARS; j++) {
                            if (GET_EQ(ch_in_room, j) && GET_OBJ_TYPE(GET_EQ(ch_in_room, j)) == ITEM_KEY) {
                                total_keys++;
                            }
                        }
                    }
                }
            }
        }
    }

    return total_keys;
}

/**
 * Get required keys for a specific path using BFS to find locked doors
 * Returns the number of required keys found
 */
static int get_required_keys_for_path(struct char_data *ch, room_rnum src, room_rnum target, obj_vnum *required_keys,
                                      int max_keys)
{
    static room_rnum visited[1000];
    static room_rnum queue[1000];
    static obj_vnum found_keys[MAX_COLLECTED_KEYS];
    int visited_count = 0, queue_head = 0, queue_tail = 0;
    int key_count = 0;
    room_rnum current_room;
    int i, dir;

    if (src == NOWHERE || target == NOWHERE || !required_keys || max_keys <= 0)
        return 0;

    if (src == target)
        return 0;

    /* Initialize */
    for (i = 0; i < 1000; i++)
        visited[i] = NOWHERE;
    for (i = 0; i < MAX_COLLECTED_KEYS; i++)
        found_keys[i] = NOTHING;

    /* Start BFS */
    queue[queue_tail++] = src;
    visited[visited_count++] = src;

    while (queue_head < queue_tail && visited_count < 1000 && key_count < max_keys) {
        current_room = queue[queue_head++];

        if (current_room == target) {
            /* Found target, copy found keys */
            for (i = 0; i < key_count && i < max_keys; i++) {
                required_keys[i] = found_keys[i];
            }
            return key_count;
        }

        /* Explore exits */
        for (dir = 0; dir < DIR_COUNT; dir++) {
            if (!world[current_room].dir_option[dir])
                continue;

            room_rnum next_room = world[current_room].dir_option[dir]->to_room;
            if (next_room == NOWHERE || next_room > top_of_world)
                continue;

            /* Check if already visited */
            int already_visited = 0;
            for (i = 0; i < visited_count; i++) {
                if (visited[i] == next_room) {
                    already_visited = 1;
                    break;
                }
            }
            if (already_visited)
                continue;

            struct room_direction_data *exit = world[current_room].dir_option[dir];

            /* Check for locked door */
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED) &&
                IS_SET(exit->exit_info, EX_LOCKED) && exit->key != NOTHING) {

                /* Check if we already have this key */
                if (!has_key(ch, exit->key)) {
                    /* Add to required keys if not already present */
                    int already_found = 0;
                    for (i = 0; i < key_count; i++) {
                        if (found_keys[i] == exit->key) {
                            already_found = 1;
                            break;
                        }
                    }
                    if (!already_found && key_count < MAX_COLLECTED_KEYS) {
                        found_keys[key_count++] = exit->key;
                    }
                }

                /* Skip this exit if we don't have the key */
                if (!has_key(ch, exit->key))
                    continue;
            }

            /* Add to queue if door can be passed */
            if (!IS_SET(exit->exit_info, EX_CLOSED) || !IS_SET(exit->exit_info, EX_LOCKED) ||
                (exit->key != NOTHING && has_key(ch, exit->key))) {

                if (visited_count < 1000 && queue_tail < 1000) {
                    queue[queue_tail++] = next_room;
                    visited[visited_count++] = next_room;
                }
            }
        }
    }

    /* Copy found keys even if target not reached */
    for (i = 0; i < key_count && i < max_keys; i++) {
        required_keys[i] = found_keys[i];
    }
    return key_count;
}

/* Function to detect the first key that is blocking a path from src to target */
obj_vnum find_blocking_key(struct char_data *ch, room_rnum src, room_rnum target)
{
    int curr_dir;
    room_rnum curr_room;

    if (src == NOWHERE || target == NOWHERE || src > top_of_world || target > top_of_world) {
        return NOTHING;
    }
    if (src == target)
        return NOTHING;

    /* Clear marks first */
    for (curr_room = 0; curr_room <= top_of_world; curr_room++)
        UNMARK(curr_room);

    MARK(src);

    /* Check immediate exits from source for locked doors */
    for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++) {
        if (!world[src].dir_option[curr_dir])
            continue;

        room_rnum next_room = world[src].dir_option[curr_dir]->to_room;
        if (next_room == NOWHERE || next_room > top_of_world)
            continue;

        struct room_direction_data *exit = world[src].dir_option[curr_dir];

        /* Check if this is a locked door that we can't pass */
        if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED) &&
            IS_SET(exit->exit_info, EX_LOCKED)) {

            /* Check if we have the key */
            if (exit->key != NOTHING && !has_key(ch, exit->key)) {
                /* This is a blocking key! */
                return exit->key;
            }
        }
    }

    /* If no immediate blocking key found, do a simple BFS to find first blocking door */
    bfs_clear_queue();

    /* Enqueue first valid steps */
    for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++) {
        if (VALID_EDGE(src, curr_dir)) {
            MARK(TOROOM(src, curr_dir));
            bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
        }
    }

    /* BFS to find target or first blocking door */
    while (queue_head) {
        curr_room = queue_head->room;

        if (curr_room == target) {
            bfs_clear_queue();
            return NOTHING; /* Path exists without key blocking */
        }

        for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++) {
            if (!world[curr_room].dir_option[curr_dir])
                continue;

            room_rnum next_room = world[curr_room].dir_option[curr_dir]->to_room;
            if (next_room == NOWHERE || next_room > top_of_world || IS_MARKED(next_room))
                continue;

            struct room_direction_data *exit = world[curr_room].dir_option[curr_dir];

            /* Check if this is a locked door that blocks the path */
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED) &&
                IS_SET(exit->exit_info, EX_LOCKED)) {

                if (exit->key != NOTHING && !has_key(ch, exit->key)) {
                    bfs_clear_queue();
                    return exit->key; /* Found blocking key */
                }
            }

            /* If not blocked, continue BFS */
            if (!IS_SET(exit->exit_info, EX_CLOSED) || !IS_SET(exit->exit_info, EX_LOCKED) ||
                (exit->key != NOTHING && has_key(ch, exit->key))) {
                MARK(next_room);
                bfs_enqueue(next_room, queue_head->dir);
            }
        }
        bfs_dequeue();
    }

    return NOTHING; /* No blocking key found */
}

/* find_first_step: given a source room and a target room, find the first step
 * on the shortest path from the source to the target. Intended usage: in
 * mobile_activity, give a mob a dir to go if they're tracking another mob or a
 * PC.  Or, a 'track' skill for PCs. */
int find_first_step(room_rnum src, room_rnum target)
{
    int curr_dir;
    room_rnum curr_room;

    if (src == NOWHERE || target == NOWHERE || src > top_of_world || target > top_of_world) {
        log1("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src, target, __FILE__);
        return (BFS_ERROR);
    }
    if (src == target)
        return (BFS_ALREADY_THERE);

    /* clear marks first, some OLC systems will save the mark. */
    for (curr_room = 0; curr_room <= top_of_world; curr_room++)
        UNMARK(curr_room);

    MARK(src);

    /* first, enqueue the first steps, saving which direction we're going. */
    for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++)
        if (VALID_EDGE(src, curr_dir)) {
            MARK(TOROOM(src, curr_dir));
            bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
        }

    /* now, do the classic BFS. */
    while (queue_head) {
        if (queue_head->room == target) {
            curr_dir = queue_head->dir;
            bfs_clear_queue();
            return (curr_dir);
        } else {
            for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++)
                if (VALID_EDGE(queue_head->room, curr_dir)) {
                    MARK(TOROOM(queue_head->room, curr_dir));
                    bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
                }
            bfs_dequeue();
        }
    }

    return (BFS_NO_PATH);
}

/* Enhanced pathfinding that considers movement costs and MV availability
 * Returns the first direction to take, and sets total_cost to the total MV needed for the path */
int find_first_step_enhanced(struct char_data *ch, room_rnum src, room_rnum target, int *total_cost)
{
    int curr_dir;
    room_rnum curr_room;

    *total_cost = 0; /* Initialize total cost */

    if (src == NOWHERE || target == NOWHERE || src > top_of_world || target > top_of_world) {
        log1("SYSERR: Illegal value %d or %d passed to find_first_step_enhanced. (%s)", src, target, __FILE__);
        return (BFS_ERROR);
    }
    if (src == target)
        return (BFS_ALREADY_THERE);

    /* clear marks first, some OLC systems will save the mark. */
    for (curr_room = 0; curr_room <= top_of_world; curr_room++)
        UNMARK(curr_room);

    MARK(src);

    /* first, enqueue the first steps, saving which direction we're going and movement cost */
    for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++)
        if (VALID_EDGE(src, curr_dir)) {
            room_rnum next_room = TOROOM(src, curr_dir);

            MARK(next_room);
            bfs_enqueue(next_room, curr_dir);
        }

    /* now, do BFS looking for target */
    while (queue_head) {
        if (queue_head->room == target) {
            curr_dir = queue_head->dir;

            /* Calculate total cost for this path by tracing back */
            /* For now, we'll use a simple approximation based on the target room cost */
            *total_cost = calculate_movement_cost(ch, target);

            bfs_clear_queue();
            return (curr_dir);
        } else {
            for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++)
                if (VALID_EDGE(queue_head->room, curr_dir)) {
                    MARK(TOROOM(queue_head->room, curr_dir));
                    bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
                }
            bfs_dequeue();
        }
    }

    return (BFS_NO_PATH);
}

/* Advanced state-based pathfinding with key collection tracking
 * Returns the first direction to take, considering keys, doors, and MV costs
 * path_info will contain detailed path information including total cost and required keys */
int find_path_with_keys(struct char_data *ch, room_rnum src, room_rnum target, int *total_cost, int *required_mv,
                        char **path_description)
{
    struct path_state initial_state, *current_state;
    struct path_state **visited_states;
    int num_visited = 0, max_visited = MAX_VISITED_STATES;
    int curr_dir, i, iterations = 0;
    char *desc_buffer;

    /* Zone optimization variables */
    zone_rnum zone_path[MAX_ZONE_PATH_LIMIT];
    zone_rnum src_zone = (src != NOWHERE && src <= top_of_world) ? world[src].zone : NOWHERE;
    zone_rnum target_zone = (target != NOWHERE && target <= top_of_world) ? world[target].zone : NOWHERE;
    int num_zones = 0;
    int keys_in_path_zones = 0;
    obj_vnum required_keys[MAX_COLLECTED_KEYS];
    int num_required_keys = 0;
    int dynamic_key_limit = MAX_COLLECTED_KEYS;

    *total_cost = 0;
    *required_mv = 0;

    /* Allocate description buffer */
    CREATE(desc_buffer, char, MAX_STRING_LENGTH);
    *path_description = desc_buffer;
    strcpy(desc_buffer, "");

    if (src == NOWHERE || target == NOWHERE || src > top_of_world || target > top_of_world) {
        log1("SYSERR: Illegal value %d or %d passed to find_path_with_keys. (%s)", src, target, __FILE__);
        return (BFS_ERROR);
    }

    if (src == target) {
        strcat(desc_buffer, "Você já está no destino!");
        return (BFS_ALREADY_THERE);
    }

    /* OPTIMIZATION: Analyze zone path and key requirements */
    if (src_zone != NOWHERE && target_zone != NOWHERE) {
        zone_optimized_calls++; /* Track optimization usage */
        num_zones = get_zones_between(src_zone, target_zone, zone_path, get_dynamic_max_zone_path());

        if (num_zones > 0) {
            /* Count keys in the zones that are actually in the path */
            keys_in_path_zones = count_keys_in_zones(zone_path, num_zones);

            /* Get specific keys required for this path */
            num_required_keys = get_required_keys_for_path(ch, src, target, required_keys, MAX_COLLECTED_KEYS);

            /* Adjust the dynamic key limit based on actual path analysis */
            if (keys_in_path_zones > 0 && keys_in_path_zones < MAX_COLLECTED_KEYS) {
                dynamic_key_limit = keys_in_path_zones;
                keys_optimized_total += (MAX_COLLECTED_KEYS - dynamic_key_limit); /* Track savings */
            } else if (num_required_keys > 0 && num_required_keys < MAX_COLLECTED_KEYS) {
                dynamic_key_limit = num_required_keys + 2; /* Add small buffer for unexpected keys */
                keys_optimized_total += (MAX_COLLECTED_KEYS - dynamic_key_limit); /* Track savings */
            }

            /* Add optimization info to description */
            char opt_info[256];
            snprintf(opt_info, sizeof(opt_info),
                     "Análise de zona: %d zonas no caminho, %d chaves detectadas, %d chaves necessárias. ", num_zones,
                     keys_in_path_zones, num_required_keys);
            strcat(desc_buffer, opt_info);
        }
    }

    /* If zones are directly connected, use more efficient pathfinding */
    if (num_zones == 2 && zone_has_connection_to(src_zone, target_zone)) {
        /* Direct zone connection - use simpler algorithm */
        int simple_dir = find_first_step_enhanced(ch, src, target, total_cost);
        if (simple_dir >= 0) {
            strcat(desc_buffer, "Caminho direto entre zonas encontrado!");
            return simple_dir;
        }
    }

    /* Initialize visited states tracking */
    CREATE(visited_states, struct path_state *, max_visited);

    /* Clear state queue */
    state_clear_queue();

    /* Initialize starting state with optimized key limit */
    initial_state.room = src;
    initial_state.mv_available = GET_MOVE(ch);
    initial_state.num_keys = 0;
    initial_state.path_cost = 0;
    initial_state.first_dir = -1;

    /* Add any keys the character already has - limit to dynamic limit */
    struct obj_data *obj;
    for (obj = ch->carrying; obj && initial_state.num_keys < dynamic_key_limit; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_KEY) {
            /* OPTIMIZATION: Only add key if it's required or in path zones */
            int key_is_relevant = 0;

            if (num_required_keys > 0) {
                /* Check if this key is in the required list */
                for (i = 0; i < num_required_keys; i++) {
                    if (GET_OBJ_VNUM(obj) == required_keys[i]) {
                        key_is_relevant = 1;
                        break;
                    }
                }
            } else {
                /* No specific requirements known, include all keys (fallback) */
                key_is_relevant = 1;
            }

            if (key_is_relevant) {
                add_key_to_state(&initial_state, GET_OBJ_VNUM(obj));
            }
        }
    }
    if (GET_EQ(ch, WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_KEY &&
        initial_state.num_keys < dynamic_key_limit) {

        obj_vnum held_key = GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD));
        int key_is_relevant = 0;

        if (num_required_keys > 0) {
            for (i = 0; i < num_required_keys; i++) {
                if (held_key == required_keys[i]) {
                    key_is_relevant = 1;
                    break;
                }
            }
        } else {
            key_is_relevant = 1;
        }

        if (key_is_relevant) {
            add_key_to_state(&initial_state, held_key);
        }
    }

    state_enqueue(&initial_state);

    /* Main pathfinding loop with iteration limit */
    while ((current_state = state_dequeue()) != NULL && iterations < get_dynamic_max_pathfind_iterations()) {
        iterations++;

        /* Check if we've reached the target */
        if (current_state->room == target) {
            *total_cost = current_state->path_cost;
            *required_mv = current_state->path_cost;

            snprintf(desc_buffer + strlen(desc_buffer), MAX_STRING_LENGTH - strlen(desc_buffer),
                     "Caminho otimizado encontrado! Custo total: %d MV (iterações: %d)", current_state->path_cost,
                     iterations);

            if (current_state->num_keys > 0) {
                strcat(desc_buffer, " (chaves usadas: ");
                for (i = 0; i < current_state->num_keys && i < 3; i++) { /* Limit displayed keys */
                    char key_info[64];
                    snprintf(key_info, sizeof(key_info), "%s#%d", i > 0 ? ", " : "", current_state->collected_keys[i]);
                    strcat(desc_buffer, key_info);
                }
                if (current_state->num_keys > 3) {
                    strcat(desc_buffer, ", ...");
                }
                strcat(desc_buffer, ")");
            }

            /* Cleanup */
            for (i = 0; i < num_visited; i++)
                if (visited_states[i])
                    free(visited_states[i]);
            free(visited_states);
            state_clear_queue();
            return current_state->first_dir;
        }

        /* Check if this state has been visited with same or better conditions */
        if (is_state_visited(current_state, visited_states, num_visited))
            continue;

        /* Add to visited states - only if we have space */
        if (num_visited < max_visited) {
            CREATE(visited_states[num_visited], struct path_state, 1);
            *(visited_states[num_visited]) = *current_state;
            num_visited++;
        } else {
            /* Too many states visited, abort to prevent memory issues */
            strcat(desc_buffer, "Busca muito complexa, abortada por segurança.");
            break;
        }

        /* Explore neighboring rooms */
        for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++) {
            if (!world[current_state->room].dir_option[curr_dir])
                continue;

            room_rnum next_room = world[current_state->room].dir_option[curr_dir]->to_room;

            if (next_room == NOWHERE || next_room > top_of_world)
                continue;

            if (ROOM_FLAGGED(next_room, ROOM_NOTRACK))
                continue;

            /* Check if we can pass through this door */
            if (!can_pass_door(ch, current_state, current_state->room, curr_dir))
                continue;

            /* Calculate movement cost for this step */
            int step_cost = calculate_movement_cost(ch, next_room);

            /* Check if we have enough MV (considering recovery) */
            if (current_state->mv_available < step_cost && !IS_NPC(ch)) {
                /* Could implement MV recovery waiting logic here */
                continue;
            }

            /* Limit path cost to prevent infinite exploration */
            if (current_state->path_cost + step_cost > GET_LEVEL(ch) * 50) {
                continue; /* Path too expensive */
            }

            /* Create new state for this path */
            struct path_state new_state = *current_state;
            new_state.room = next_room;
            new_state.mv_available -= step_cost;
            new_state.path_cost += step_cost;

            /* Set first direction if this is the first step */
            if (current_state->room == src)
                new_state.first_dir = curr_dir;
            else
                new_state.first_dir = current_state->first_dir;

            /* OPTIMIZATION: Check if there's a key in this room that we should collect */
            if (new_state.num_keys < dynamic_key_limit) {
                struct obj_data *key_obj;
                for (key_obj = world[next_room].contents; key_obj; key_obj = key_obj->next_content) {
                    if (GET_OBJ_TYPE(key_obj) == ITEM_KEY && new_state.num_keys < dynamic_key_limit) {
                        obj_vnum key_vnum = GET_OBJ_VNUM(key_obj);

                        /* OPTIMIZATION: Only collect relevant keys */
                        int should_collect = 0;
                        if (num_required_keys > 0) {
                            for (i = 0; i < num_required_keys; i++) {
                                if (key_vnum == required_keys[i]) {
                                    should_collect = 1;
                                    break;
                                }
                            }
                        } else {
                            /* No specific requirements, collect key (fallback behavior) */
                            should_collect = 1;
                        }

                        if (should_collect) {
                            add_key_to_state(&new_state, key_vnum);
                        }
                    }
                }
            }

            state_enqueue(&new_state);
        }
    }

    /* No path found or iteration limit reached */
    if (iterations >= get_dynamic_max_pathfind_iterations()) {
        strcat(desc_buffer, "Busca interrompida - muito complexa.");
    } else {
        strcat(desc_buffer, "Nenhum caminho encontrado.");
    }

    /* Cleanup */
    for (i = 0; i < num_visited; i++)
        if (visited_states[i])
            free(visited_states[i]);
    free(visited_states);
    state_clear_queue();

    return (BFS_NO_PATH);
}

/* Functions and Commands which use the above functions. */
ACMD(do_track)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int dir, use_advanced = 0;

    /* The character must have the track skill. */
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRACK)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Quem você está tentando rastrear?\r\n");
        send_to_char(ch, "Uso: track <alvo> [advanced]\r\n");
        send_to_char(ch,
                     "      track <alvo> advanced  - para rastreamento avançado com análise de chaves e portas\r\n");
        if (GET_SKILL(ch, SKILL_TRACK) >= 75) {
            send_to_char(ch,
                         "\tyNota: Como rastreador experiente, você usa automaticamente técnicas avançadas.\tn\r\n");
        }
        return;
    }

    /* Check for advanced tracking mode */
    if (*arg2 && !str_cmp(arg2, "advanced")) {
        use_advanced = 1;
        /* Advanced mode requires higher skill */
        if (GET_SKILL(ch, SKILL_TRACK) < 60) {
            send_to_char(ch, "Seu conhecimento de rastreamento não é avançado o suficiente.\r\n");
            return;
        }
    }

    /* Auto-enable advanced mode for expert trackers (SKILL_TRACK >= 75) */
    if (GET_SKILL(ch, SKILL_TRACK) >= 75) {
        use_advanced = 1;
        if (!*arg2) {
            send_to_char(ch, "\tyComo um rastreador experiente, você automaticamente usa técnicas avançadas.\tn\r\n");
        }
    }

    /* The person can't see the victim. */
    if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD))) {
        send_to_char(ch, "Você não encontra nenhum caminho.\r\n");
        return;
    }
    /* We can't track the victim. */
    if (AFF_FLAGGED(vict, AFF_NOTRACK)) {
        send_to_char(ch, "Você não encontra nenhum caminho.\r\n");
        return;
    }

    /* 101 is a complete failure, no matter what the proficiency. */
    if (rand_number(0, 101) >= GET_SKILL(ch, SKILL_TRACK)) {
        int tries = 10;
        /* Find a random direction. :) */
        do {
            dir = rand_number(0, DIR_COUNT - 1);
        } while (!CAN_GO(ch, dir) && --tries);

        /* Expert trackers get additional information even on failure */
        if (GET_SKILL(ch, SKILL_TRACK) >= 75) {
            send_to_char(ch,
                         "Você encontra sinais confusos apontando a %s, mas não tem certeza se é o caminho certo.\r\n",
                         dirs[dir]);
            send_to_char(ch,
                         "\tyDica: O alvo pode estar escondendo seus rastros ou você pode tentar novamente.\tn\r\n");
        } else {
            send_to_char(ch, "Você encontra um caminho a %s daqui!\r\n", dirs[dir]);
        }
        return;
    }

    /* They passed the skill check. */
    if (use_advanced) {
        /* Use advanced pathfinding with key analysis */
        int total_cost = 0, required_mv = 0;
        char *path_description = NULL;

        dir = find_path_with_keys(ch, IN_ROOM(ch), IN_ROOM(vict), &total_cost, &required_mv, &path_description);

        switch (dir) {
            case BFS_ERROR:
                send_to_char(ch, "Hmm.. algo parece ter saído errado.\r\n");
                break;
            case BFS_ALREADY_THERE:
                send_to_char(ch, "Você já está na mesma sala!!\r\n");
                break;
            case BFS_NO_PATH:
                send_to_char(ch, "Você não encontra nenhum caminho, mesmo considerando chaves e portas.\r\n");
                if (path_description && strlen(path_description) > 0) {
                    send_to_char(ch, "Análise: %s\r\n", path_description);
                }
                break;
            default: /* Success! */
                send_to_char(ch, "Você encontra um caminho complexo a %s daqui!\r\n", dirs_pt[dir]);
                if (path_description && strlen(path_description) > 0) {
                    send_to_char(ch, "Análise detalhada: %s\r\n", path_description);
                }

                /* Check if path requires keys not yet collected */
                int next_room_cost = calculate_movement_cost(ch, TOROOM(IN_ROOM(ch), dir));
                if (GET_MOVE(ch) < next_room_cost && !IS_NPC(ch)) {
                    send_to_char(ch,
                                 "Aviso: Você não tem MV suficiente para o primeiro passo. "
                                 "(Necessário: %d MV, Disponível: %d MV)\r\n",
                                 next_room_cost, GET_MOVE(ch));
                }
                break;
        }

        /* Free the allocated description */
        if (path_description)
            free(path_description);

        /* Add integrated zone analysis for advanced mode */
        zone_rnum src_zone = world[IN_ROOM(ch)].zone;
        zone_rnum target_zone = world[IN_ROOM(vict)].zone;

        if (src_zone != target_zone) {
            zone_rnum zone_path[MAX_ZONE_PATH_LIMIT];
            int num_zones = get_zones_between(src_zone, target_zone, zone_path, get_dynamic_max_zone_path());
            obj_vnum required_keys[MAX_COLLECTED_KEYS];
            int num_required_keys;
            int keys_in_zones;

            send_to_char(ch, "\r\n\tg=== ANÁLISE DE ZONA INTEGRADA ===\tn\r\n");
            send_to_char(ch, "Zona atual: (%s)\r\n", zone_table[src_zone].name);
            send_to_char(ch, "Zona destino: (%s)\r\n", zone_table[target_zone].name);

            if (num_zones > 0) {
                send_to_char(ch, "\tyCaminho de zonas encontrado (%d zonas):\tn\r\n", num_zones);
                int i;
                for (i = 0; i < num_zones && i < 5; i++) {
                    send_to_char(ch, "%d. Zona (%s)\r\n", i + 1, zone_table[zone_path[i]].name);
                }
                if (num_zones > 5) {
                    send_to_char(ch, "... e mais %d zonas\r\n", num_zones - 5);
                }

                /* Count keys in path zones */
                keys_in_zones = count_keys_in_zones(zone_path, num_zones);
                send_to_char(ch, "\tcChaves detectadas no caminho: %d\tn\r\n", keys_in_zones);

                /* Get specific required keys */
                num_required_keys =
                    get_required_keys_for_path(ch, IN_ROOM(ch), IN_ROOM(vict), required_keys, MAX_COLLECTED_KEYS);
                if (num_required_keys > 0) {
                    send_to_char(ch, "\trChaves necessárias: %d\tn\r\n", num_required_keys);
                } else {
                    send_to_char(ch, "\tgNenhuma chave específica necessária.\tn\r\n");
                }
            } else {
                send_to_char(ch, "\trNenhum caminho de zona encontrado.\tn\r\n");
                send_to_char(ch, "Zonas podem estar isoladas ou sem conexão direta.\r\n");
            }
        } else {
            send_to_char(ch, "\tcVocês estão na mesma zona.\tn\r\n");
        }

    } else {
        /* Use standard enhanced tracking */
        int total_cost = 0;
        dir = find_first_step_enhanced(ch, IN_ROOM(ch), IN_ROOM(vict), &total_cost);

        switch (dir) {
            case BFS_ERROR:
                send_to_char(ch, "Hmm.. algo parece ter saído errado.\r\n");
                break;
            case BFS_ALREADY_THERE:
                send_to_char(ch, "Você já está na mesma sala!!\r\n");
                break;
            case BFS_NO_PATH:
                send_to_char(ch, "Você não encontra nenhum caminho.\r\n");
                break;
            default: /* Success! */
            {
                /* Check if player has enough MV for the next step */
                int next_room_cost;
                next_room_cost = calculate_movement_cost(ch, TOROOM(IN_ROOM(ch), dir));

                if (GET_MOVE(ch) < next_room_cost && !IS_NPC(ch)) {
                    send_to_char(ch,
                                 "Você encontra um caminho a %s daqui, mas está muito cansad%s para continuar. "
                                 "(Necessário: %d MV, Disponível: %d MV)\r\n",
                                 dirs_pt[dir], OA(ch), next_room_cost, GET_MOVE(ch));
                } else {
                    /* Provide enhanced tracking information */
                    float weather_mod = get_weather_movement_modifier(ch);
                    if (weather_mod > 1.2) {
                        send_to_char(
                            ch,
                            "Você encontra um caminho a %s daqui! As condições climáticas dificultam a viagem. "
                            "(Custo: %d MV)\r\n",
                            dirs_pt[dir], next_room_cost);
                    } else if (weather_mod < 0.9) {
                        send_to_char(ch,
                                     "Você encontra um caminho a %s daqui! As condições climáticas favorecem a viagem. "
                                     "(Custo: %d MV)\r\n",
                                     dirs_pt[dir], next_room_cost);
                    } else {
                        send_to_char(ch, "Você encontra um caminho a %s daqui! (Custo: %d MV)\r\n", dirs_pt[dir],
                                     next_room_cost);
                    }
                }
                break;
            }
        }
    }
}

struct pathfind_cache_entry {
    room_rnum src;
    room_rnum target;
    time_t timestamp;
    int direction;
    int valid;
    int priority; /* 0 = normal, 1 = duty-related (shopkeeper/sentinel returning to post) */
};

static struct pathfind_cache_entry pathfind_cache[PATHFIND_CACHE_SIZE];
static int pathfind_cache_initialized = 0;

static void init_pathfind_cache(void)
{
    int i;
    for (i = 0; i < PATHFIND_CACHE_SIZE; i++) {
        pathfind_cache[i].valid = 0;
        pathfind_cache[i].priority = 0;
    }
    pathfind_cache_initialized = 1;
}

/* Getter functions for pathfinding statistics */
long get_pathfind_calls_total(void) { return pathfind_calls_total; }

long get_pathfind_cache_hits(void) { return pathfind_cache_hits; }

long get_advanced_pathfind_calls(void) { return advanced_pathfind_calls; }

int get_pathfind_cache_valid_entries(void)
{
    int valid_entries = 0;
    if (pathfind_cache_initialized) {
        for (int i = 0; i < PATHFIND_CACHE_SIZE; i++) {
            if (pathfind_cache[i].valid) {
                valid_entries++;
            }
        }
    }
    return valid_entries;
}

static int get_cached_pathfind(room_rnum src, room_rnum target)
{
    int i;
    time_t now = get_cached_time(); /* Use cached time instead of system call */

    if (!pathfind_cache_initialized)
        return -1;

    /* Validate src and target rooms */
    if (!is_valid_room(src) || !is_valid_room(target))
        return -1;

    for (i = 0; i < PATHFIND_CACHE_SIZE; i++) {
        if (pathfind_cache[i].valid && pathfind_cache[i].src == src && pathfind_cache[i].target == target &&
            (now - pathfind_cache[i].timestamp) < PATHFIND_CACHE_TTL) {
            pathfind_cache_hits++;
            return pathfind_cache[i].direction;
        }
    }
    return -1;
}

static void cache_pathfind_result(room_rnum src, room_rnum target, int direction)
{
    /* Don't cache failed pathfinding results to preserve cache slots for successful paths */
    if (direction >= 0) {
        cache_pathfind_result_priority(src, target, direction, 0);
    }
}

static void cache_pathfind_result_priority(room_rnum src, room_rnum target, int direction, int priority)
{
    /* Don't cache failed pathfinding results to preserve cache slots for successful paths */
    if (direction < 0) {
        return;
    }

    /* Validate src and target rooms before caching */
    if (!is_valid_room(src) || !is_valid_room(target)) {
        return;
    }

    int i, oldest_idx = 0;
    time_t now = get_cached_time(); /* Use cached time instead of system call */
    time_t oldest_time = now;
    int found_low_priority = 0;

    if (!pathfind_cache_initialized)
        init_pathfind_cache();

    /* First pass: Find empty slot or lowest priority entry that can be replaced */
    for (i = 0; i < PATHFIND_CACHE_SIZE; i++) {
        if (!pathfind_cache[i].valid) {
            oldest_idx = i;
            found_low_priority = 1;
            break;
        }
        /* If this is a high priority entry, prefer replacing low priority entries */
        if (priority > 0 && pathfind_cache[i].priority == 0) {
            if (!found_low_priority || pathfind_cache[i].timestamp < oldest_time) {
                oldest_time = pathfind_cache[i].timestamp;
                oldest_idx = i;
                found_low_priority = 1;
            }
        }
    }

    /* Second pass: If no low priority entries found, find the oldest entry */
    if (!found_low_priority) {
        for (i = 0; i < PATHFIND_CACHE_SIZE; i++) {
            if (pathfind_cache[i].timestamp < oldest_time) {
                oldest_time = pathfind_cache[i].timestamp;
                oldest_idx = i;
            }
        }
    }

    pathfind_cache[oldest_idx].src = src;
    pathfind_cache[oldest_idx].target = target;
    pathfind_cache[oldest_idx].direction = direction;
    pathfind_cache[oldest_idx].timestamp = now;
    pathfind_cache[oldest_idx].valid = 1;
    pathfind_cache[oldest_idx].priority = priority;
}

/* Enhanced pathfind command for comprehensive pathfinding analysis - improved for players */
ACMD(do_pathfind)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int total_cost = 0, required_mv = 0;
    char *path_description = NULL;
    int dir;
    int skill_level = GET_SKILL(ch, SKILL_TRACK);

    /* The character must have advanced track skill */
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRACK)) {
        send_to_char(ch, "Você não tem conhecimento de rastreamento.\r\n");
        return;
    }

    if (skill_level < 75) {
        send_to_char(
            ch, "Seu conhecimento de rastreamento não é avançado o suficiente para análise detalhada de caminhos.\r\n");
        send_to_char(
            ch, "\tyDica:\tn Use 'track [alvo] advanced' para rastreamento avançado básico (requer nível 60+).\r\n");
        return;
    }

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "\tyAnalisar caminho para quem?\tn\r\n");
        send_to_char(ch, "\tcUso:\tn pathfind <alvo> [analyze|compare]\r\n");
        send_to_char(ch, "      pathfind <alvo> analyze  - análise detalhada de múltiplos caminhos\r\n");
        send_to_char(ch, "      pathfind <alvo> compare  - comparação entre métodos básico e avançado\r\n");
        send_to_char(
            ch, "\r\n\tyNota:\tn Como rastreador experiente, 'track' já usa técnicas avançadas automaticamente.\r\n");
        send_to_char(ch, "       Use 'pathfind' apenas para análise detalhada ou comparação de métodos.\r\n");
        return;
    }

    /* Find the target */
    if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD))) {
        send_to_char(ch, "Você não consegue localizar esse alvo.\r\n");
        return;
    }

    /* We can't track the victim. */
    if (AFF_FLAGGED(vict, AFF_NOTRACK)) {
        send_to_char(ch, "Esse alvo não pode ser rastreado.\r\n");
        return;
    }

    /* Enhanced skill check for expert analysis */
    if (rand_number(0, 101) >= (skill_level - 10)) {
        send_to_char(ch, "Você não consegue fazer uma análise detalhada neste momento.\r\n");
        send_to_char(ch, "\tyDica:\tn Tente novamente ou use o comando 'track' simples.\r\n");
        return;
    }

    send_to_char(ch, "\tgAnalisando caminhos possíveis...\tn\r\n");

    /* Check for compare mode */
    if (*arg2 && !str_cmp(arg2, "compare")) {
        char *comparison = get_path_analysis_summary(ch, IN_ROOM(vict));
        send_to_char(ch, "%s", comparison);
        return;
    }

    /* Use advanced pathfinding with comprehensive analysis */
    dir = find_path_with_keys(ch, IN_ROOM(ch), IN_ROOM(vict), &total_cost, &required_mv, &path_description);

    switch (dir) {
        case BFS_ERROR:
            send_to_char(ch, "\trErro na análise de caminhos.\tn\r\n");
            break;

        case BFS_ALREADY_THERE:
            send_to_char(ch, "\tgVocê já está no mesmo local que o alvo!\tn\r\n");
            break;

        case BFS_NO_PATH:
            send_to_char(ch, "\trAnálise completa: Nenhum caminho viável encontrado.\tn\r\n");
            if (path_description && strlen(path_description) > 0) {
                send_to_char(ch, "\tyDetalhes:\tn %s\r\n", path_description);
            }
            send_to_char(ch, "\r\n\tcPossíveis problemas:\tn\r\n");
            send_to_char(ch, "- Portas trancadas sem chaves acessíveis\r\n");
            send_to_char(ch, "- Barreiras intransponíveis\r\n");
            send_to_char(ch, "- Alvo em área protegida contra rastreamento\r\n");
            send_to_char(ch, "\r\n\tyDica:\tn Tente usar 'track' simples para caminhos mais diretos,\r\n");
            send_to_char(ch, "       ou procure por chaves e aliados que possam ajudar.\r\n");
            break;

        default: /* Success! */
            send_to_char(ch, "\tgCaminho encontrado!\tn\r\n");
            send_to_char(ch, "Direção inicial: %s\r\n", dirs_pt[dir]);

            if (path_description && strlen(path_description) > 0) {
                send_to_char(ch, "Análise detalhada: %s\r\n", path_description);
            }

            /* Provide movement analysis */
            int next_step_cost = calculate_movement_cost(ch, TOROOM(IN_ROOM(ch), dir));
            send_to_char(ch, "Custo do próximo passo: %d MV\r\n", next_step_cost);
            send_to_char(ch, "MV disponível: %d\r\n", GET_MOVE(ch));

            if (GET_MOVE(ch) < next_step_cost) {
                send_to_char(ch, "\trAviso: MV insuficiente para o próximo passo!\tn\r\n");
                /* Calculate recovery time needed */
                int mv_needed = next_step_cost - GET_MOVE(ch);
                int recovery_time = calculate_mv_recovery_time(ch, mv_needed);
                send_to_char(ch, "Tempo estimado para recuperação: %d segundos (%d minutos)\r\n", recovery_time,
                             recovery_time / 60);
            }

            /* Weather impact analysis */
            float weather_mod = get_weather_movement_modifier(ch);
            if (weather_mod > 1.2) {
                send_to_char(ch, "\tyCondições climáticas: DESFAVORÁVEIS (custo +%.0f%%)\tn\r\n",
                             (weather_mod - 1.0) * 100);
            } else if (weather_mod < 0.9) {
                send_to_char(ch, "\tgCondições climáticas: FAVORÁVEIS (custo %.0f%%)\tn\r\n", weather_mod * 100);
            } else {
                send_to_char(ch, "\tcCondições climáticas: NORMAIS\tn\r\n");
            }

            /* Detailed analysis if requested */
            if (*arg2 && !str_cmp(arg2, "analyze")) {
                send_to_char(ch, "\r\n\tcAnálise Detalhada:\tn\r\n");
                send_to_char(ch, "- Algoritmo: Busca com estado de chaves coletadas\r\n");
                send_to_char(ch, "- Considera: Portas, chaves, custos de movimento, MV disponível\r\n");
                send_to_char(ch, "- Otimização: Caminho de menor custo com recursos disponíveis\r\n");

                /* Check for potential keys in inventory */
                struct obj_data *obj;
                int key_count = 0;
                send_to_char(ch, "- Chaves no inventário: ");
                for (obj = ch->carrying; obj; obj = obj->next_content) {
                    if (GET_OBJ_TYPE(obj) == ITEM_KEY) {
                        if (key_count > 0)
                            send_to_char(ch, ", ");
                        send_to_char(ch, "#%d", GET_OBJ_VNUM(obj));
                        key_count++;
                    }
                }
                if (key_count == 0) {
                    send_to_char(ch, "nenhuma");
                }
                send_to_char(ch, "\r\n");
            }
            break;
    }

    /* Free the allocated description */
    if (path_description)
        free(path_description);
}

/* Calculate estimated time for MV recovery in seconds
 * Based on specification: X units every 75 seconds */
int calculate_mv_recovery_time(struct char_data *ch, int mv_needed)
{
    int mv_rate = 1; /* Default: 1 MV per 75 seconds */
    int recovery_time;

    if (mv_needed <= 0)
        return 0;

    /* Calculate base recovery time */
    recovery_time = mv_needed * 75 / mv_rate;

    /* Factor in character constitution and level for better estimates */
    if (GET_CON(ch) > 15) {
        recovery_time = (recovery_time * 85) / 100; /* 15% faster recovery for high CON */
    } else if (GET_CON(ch) < 10) {
        recovery_time = (recovery_time * 115) / 100; /* 15% slower recovery for low CON */
    }

    return recovery_time;
}

/**
 * Special pathfinding for mobs returning to duty posts (shopkeepers/sentinels).
 * Uses high priority caching and bypasses cache limitations for critical duty paths.
 * Returns the best direction or -1 if no path found.
 */
int mob_duty_pathfind(struct char_data *ch, room_rnum target_room)
{
    int basic_dir, chosen_dir;
    int basic_cost = 0;

    if (!ch || !IS_NPC(ch) || target_room == NOWHERE || IN_ROOM(ch) == target_room)
        return -1;

    /* Update global statistics */
    pathfind_calls_total++;

    /* For duty pathfinding, always check cache first with high priority */
    chosen_dir = get_cached_pathfind(IN_ROOM(ch), target_room);
    if (chosen_dir != -1) {
        return chosen_dir;
    }

    /* Try basic pathfinding first - it's much faster */
    basic_dir = find_first_step_enhanced(ch, IN_ROOM(ch), target_room, &basic_cost);

    if (basic_dir >= 0) {
        /* Basic pathfinding worked, cache with high priority */
        chosen_dir = basic_dir;
        cache_pathfind_result_priority(IN_ROOM(ch), target_room, chosen_dir, 1);
    } else {
        /* NPCs no longer use advanced pathfinding to prevent bottlenecks */
        /* Advanced pathfinding is reserved for players only via the track command */
        chosen_dir = -1;
    }

    return chosen_dir;
}

/**
 * Intelligent pathfinding comparison for mobs.
 * Compares basic vs advanced pathfinding and chooses the most efficient method.
 * Returns the best direction or -1 if no path found.
 */
int mob_smart_pathfind(struct char_data *ch, room_rnum target_room)
{
    int basic_dir, chosen_dir;
    int basic_cost = 0;
    static int pathfind_calls = 0;

    if (!ch || !IS_NPC(ch) || target_room == NOWHERE || IN_ROOM(ch) == target_room)
        return -1;

    /* Update global statistics */
    pathfind_calls_total++;

    /* Limit pathfinding frequency to prevent resource exhaustion */
    pathfind_calls++;
    if (pathfind_calls % 20 == 0) {
        /* Only do complex pathfinding every 20th call (reduced from 10th) */
        pathfind_calls = 0;
    } else {
        /* Use simple pathfinding most of the time */
        return find_first_step_enhanced(ch, IN_ROOM(ch), target_room, &basic_cost);
    }

    /* Check cache first */
    chosen_dir = get_cached_pathfind(IN_ROOM(ch), target_room);
    if (chosen_dir != -1) {
        return chosen_dir;
    }

    /* Try basic pathfinding first - it's much faster */
    basic_dir = find_first_step_enhanced(ch, IN_ROOM(ch), target_room, &basic_cost);

    if (basic_dir >= 0) {
        /* Basic pathfinding worked, use it */
        chosen_dir = basic_dir;
    } else {
        /* NPCs no longer use advanced pathfinding to prevent bottlenecks */
        /* Advanced pathfinding is reserved for players only via the track command */
        chosen_dir = -1;
    }

    /* Cache the result */
    cache_pathfind_result(IN_ROOM(ch), target_room, chosen_dir);

    return chosen_dir;
}

/* Generate a comprehensive path analysis summary for players - enhanced after track improvements */
char *get_path_analysis_summary(struct char_data *ch, room_rnum target)
{
    static char summary[MAX_STRING_LENGTH];
    int basic_dir, advanced_dir;
    int basic_cost = 0, advanced_cost = 0, advanced_mv = 0;
    char *advanced_desc = NULL;
    int skill_level = GET_SKILL(ch, SKILL_TRACK);

    /* Clear summary */
    strcpy(summary, "");

    /* Add skill level context */
    if (skill_level >= 75) {
        strcat(summary, "\tyComparação detalhada para rastreador experiente:\tn\r\n");
    } else {
        strcat(summary, "Análise comparativa de caminhos:\r\n");
    }

    /* Try basic pathfinding first */
    basic_dir = find_first_step_enhanced(ch, IN_ROOM(ch), target, &basic_cost);

    /* Try advanced pathfinding */
    advanced_dir = find_path_with_keys(ch, IN_ROOM(ch), target, &advanced_cost, &advanced_mv, &advanced_desc);

    strcat(summary, "=== MÉTODOS DE RASTREAMENTO ===\r\n");

    /* Basic pathfinding results */
    if (basic_dir >= 0 && basic_dir < DIR_COUNT) {
        char basic_info[256];
        snprintf(basic_info, sizeof(basic_info), "\tcRastreamento Básico:\tn %s (custo estimado: %d MV)\r\n",
                 dirs_pt[basic_dir], basic_cost);
        strcat(summary, basic_info);
    } else {
        strcat(summary, "\tcRastreamento Básico:\tn Nenhum caminho encontrado\r\n");
    }

    /* Advanced pathfinding results */
    if (advanced_dir >= 0 && advanced_dir < DIR_COUNT) {
        char advanced_info[256];
        snprintf(advanced_info, sizeof(advanced_info), "\tgRastreamento Avançado:\tn %s (custo total: %d MV)\r\n",
                 dirs_pt[advanced_dir], advanced_cost);
        strcat(summary, advanced_info);

        if (advanced_desc && strlen(advanced_desc) > 0) {
            strcat(summary, "  \tyDetalhes:\tn ");
            strcat(summary, advanced_desc);
            strcat(summary, "\r\n");
        }
    } else {
        strcat(summary, "\tgRastreamento Avançado:\tn Nenhum caminho viável\r\n");
    }

    /* Enhanced recommendation based on skill level and track command improvements */
    strcat(summary, "\r\n=== RECOMENDAÇÃO ===\r\n");
    if (basic_dir == advanced_dir && basic_dir >= 0) {
        strcat(summary, "Ambos os métodos concordam. Caminho direto disponível.\r\n");
        if (skill_level >= 75) {
            strcat(summary, "\tyNota:\tn O comando 'track' automaticamente usa técnicas avançadas para você.\r\n");
        }
    } else if (advanced_dir >= 0 && basic_dir < 0) {
        strcat(summary, "Apenas o rastreamento avançado encontrou caminho.\r\n");
        if (skill_level >= 75) {
            strcat(summary, "\tgUse:\tn 'track [alvo]' (modo avançado automático)\r\n");
        } else if (skill_level >= 60) {
            strcat(summary, "\tgUse:\tn 'track [alvo] advanced'\r\n");
        } else {
            strcat(summary, "\trAtenção:\tn Você precisa de mais experiência para rastreamento avançado.\r\n");
        }
    } else if (basic_dir >= 0 && advanced_dir < 0) {
        strcat(summary, "Rastreamento básico encontrou caminho simples.\r\n");
        strcat(summary, "\tgUse:\tn 'track [alvo]'\r\n");
    } else {
        strcat(summary, "Nenhum caminho viável encontrado.\r\n");
        strcat(summary, "\tyDica:\tn Verifique chaves, portas fechadas ou barreiras mágicas.\r\n");
    }

    /* Additional context about the enhanced track command */
    if (skill_level >= 75) {
        strcat(summary,
               "\r\n\tyLembrete:\tn Como rastreador experiente, 'track' usa automaticamente análise avançada.\r\n");
    }

    /* Clean up */
    if (advanced_desc)
        free(advanced_desc);

    return summary;
}

void hunt_victim(struct char_data *ch)
{
    int dir;
    byte found;
    struct char_data *tmp, *victim;
    room_rnum victim_room;

    if (!ch || !HUNTING(ch) || FIGHTING(ch))
        return;

    /* Cache the victim pointer to avoid multiple dereferences */
    victim = HUNTING(ch);

    /* make sure the char still exists */
    for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
        if (victim == tmp)
            found = TRUE;

    if (!found) {
        char actbuf[MAX_INPUT_LENGTH] = "Maldição!  Perdi minha presa!!";

        do_say(ch, actbuf, 0, 0);
        HUNTING(ch) = NULL;
        return;
    }

    /* Cache victim's room and validate it */
    victim_room = IN_ROOM(victim);
    if (!is_valid_room(victim_room)) {
        HUNTING(ch) = NULL;
        return;
    }

    /* Use intelligent pathfinding for mobs hunting targets */
    if (IS_NPC(ch)) {
        dir = mob_smart_pathfind(ch, victim_room);
        if (dir == -1) {
            /* Fall back to basic pathfinding if smart pathfinding fails */
            dir = find_first_step(IN_ROOM(ch), victim_room);
        }
    } else {
        /* Players still use basic pathfinding for hunting */
        dir = find_first_step(IN_ROOM(ch), victim_room);
    }

    if (dir < 0) {
        char buf[MAX_INPUT_LENGTH];

        /* Re-validate victim still exists before accessing it */
        for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
            if (victim == tmp)
                found = TRUE;

        if (found) {
            snprintf(buf, sizeof(buf), "Maldição!  Eu perdi %s!", ELEA(victim));
            do_say(ch, buf, 0, 0);
        }
        HUNTING(ch) = NULL;
    } else {
        perform_move(ch, dir, 1);

        /* Re-validate victim still exists after movement */
        for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
            if (victim == tmp)
                found = TRUE;

        if (found && IN_ROOM(ch) == IN_ROOM(victim))
            hit(ch, victim, TYPE_UNDEFINED);
        else if (!found)
            HUNTING(ch) = NULL;
    }
}
