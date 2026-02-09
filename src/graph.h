/**
 * @file graph.h
 * Header file for Various graph algorithms.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 *
 * @todo the functions here should perhaps be a part of another module?
 */
#ifndef _GRAPH_H_
#define _GRAPH_H_

/* Pathfinding cache constants */
/* Simple pathfinding cache to reduce redundant calculations */
#define PATHFIND_CACHE_SIZE 100 /* Reduced from 1000 to minimize linear search overhead */
#define PATHFIND_CACHE_TTL 120  /* Cache entries valid for 2 minutes to reduce recomputation */
ACMD(do_track);
ACMD(do_pathfind);
void hunt_victim(struct char_data *ch);
int find_first_step(room_rnum src, room_rnum target);
int bfs_distance(room_rnum src, room_rnum target);
int find_first_step_enhanced(struct char_data *ch, room_rnum src, room_rnum target, int *total_cost);
int find_path_with_keys(struct char_data *ch, room_rnum src, room_rnum target, int *total_cost, int *required_mv,
                        char **path_description);
int calculate_movement_cost(struct char_data *ch, room_rnum room);
int calculate_mv_recovery_time(struct char_data *ch, int mv_needed);
char *get_path_analysis_summary(struct char_data *ch, room_rnum target);
int mob_smart_pathfind(struct char_data *ch, room_rnum target_room);
int mob_duty_pathfind(struct char_data *ch, room_rnum target_room);
obj_vnum find_blocking_key(struct char_data *ch, room_rnum src, room_rnum target);

/* Zone-based optimization functions */
int get_zones_between_rooms(room_rnum src, room_rnum target, zone_rnum *zone_path, int max_zones);
int count_keys_in_zone_path(zone_rnum *zones, int num_zones);
int get_path_required_keys(struct char_data *ch, room_rnum src, room_rnum target, obj_vnum *required_keys,
                           int max_keys);

/* Pathfinding statistics getter functions */
long get_pathfind_calls_total(void);
long get_pathfind_cache_hits(void);
long get_advanced_pathfind_calls(void);
int get_pathfind_cache_valid_entries(void);

#endif /* _GRAPH_H_*/
