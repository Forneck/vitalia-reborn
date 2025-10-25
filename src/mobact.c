/**************************************************************************
 *  File: mobact.c                                          Part of tbaMUD *
 *  Usage: Functions for generating intelligent (?) behavior in mobiles.   *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "graph.h"
#include "fight.h"
#include "shop.h"
#include "graph.h"
#include "spedit.h"
#include "shop.h"
#include "quest.h"
#include "spec_procs.h"

/* local file scope only function prototypes */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack);

/* External function prototypes */
void call_ACMD(void (*function)(), struct char_data *ch, char *argument, int cmd, int subcmd);

struct mob_upgrade_plan find_best_upgrade_for_mob(struct char_data *ch);
struct char_data *find_best_median_leader(struct char_data *ch);
bool mob_handle_grouping(struct char_data *ch);
bool mob_share_gear_with_group(struct char_data *ch);
bool perform_move_IA(struct char_data *ch, int dir, bool should_close_behind, int was_in);
bool mob_goal_oriented_roam(struct char_data *ch, room_rnum target_room);
bool handle_duty_routine(struct char_data *ch);
bool mob_follow_leader(struct char_data *ch);
bool mob_try_stealth_follow(struct char_data *ch);
bool mob_assist_allies(struct char_data *ch);
bool mob_try_and_loot(struct char_data *ch);
bool mob_try_and_upgrade(struct char_data *ch);
bool mob_manage_inventory(struct char_data *ch);
bool mob_handle_item_usage(struct char_data *ch);
bool mob_try_to_sell_junk(struct char_data *ch);
struct obj_data *find_unblessed_weapon_or_armor(struct char_data *ch);
struct obj_data *find_cursed_item_in_inventory(struct char_data *ch);
struct obj_data *find_bank_nearby(struct char_data *ch);
bool mob_use_bank(struct char_data *ch);
struct char_data *get_mob_in_room_by_rnum(room_rnum room, mob_rnum rnum);
struct char_data *get_mob_in_room_by_vnum(room_rnum room, mob_vnum vnum);
struct char_data *find_questmaster_by_vnum(mob_vnum vnum);
void mob_process_wishlist_goals(struct char_data *ch);
bool mob_try_to_accept_quest(struct char_data *ch);
bool mob_process_quest_completion(struct char_data *ch, qst_rnum quest_rnum);
bool mob_try_donate(struct char_data *ch, struct obj_data *obj);
bool mob_try_sacrifice(struct char_data *ch, struct obj_data *corpse);
bool mob_try_junk(struct char_data *ch, struct obj_data *obj);
bool mob_try_drop(struct char_data *ch, struct obj_data *obj);

/* Function to find where a key can be obtained */
room_rnum find_key_location(obj_vnum key_vnum, int *source_type, mob_vnum *carrying_mob);
bool mob_set_key_collection_goal(struct char_data *ch, obj_vnum key_vnum, int original_goal, room_rnum original_dest);

/** Function to handle mob leveling when they gain enough experience.
 * Mobs automatically distribute improvements to stats and abilities
 * since they don't have practice points like players.
 * @param ch The mob character to potentially level up. */
void check_mob_level_up(struct char_data *ch)
{
    if (!IS_MOB(ch) || !ch->ai_data)
        return;

    /* Calculate experience needed for next level (simplified formula) */
    int exp_needed = GET_LEVEL(ch) * 1000;

    if (GET_EXP(ch) >= exp_needed && GET_LEVEL(ch) < LVL_IMMORT - 1) {
        GET_LEVEL(ch)++;
        GET_EXP(ch) -= exp_needed;

        /* Automatically improve stats based on genetics tendencies */
        if (ch->ai_data->genetics.brave_prevalence > 50) {
            GET_STR(ch) = MIN(GET_STR(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.quest_tendency > 50) {
            GET_INT(ch) = MIN(GET_INT(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.use_tendency > 50) {
            GET_WIS(ch) = MIN(GET_WIS(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.roam_tendency > 50) {
            GET_DEX(ch) = MIN(GET_DEX(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.equip_tendency > 50) {
            GET_CON(ch) = MIN(GET_CON(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.trade_tendency > 50) {
            GET_CHA(ch) = MIN(GET_CHA(ch) + 1, 25);
        }

        /* Improve hit points based on constitution */
        int hp_gain = rand_number(1, 10) + con_app[GET_CON(ch)].hitp;
        GET_MAX_HIT(ch) += MAX(hp_gain, 1);
        GET_HIT(ch) = GET_MAX_HIT(ch); /* Full heal on level up */

        /* Improve mana for spellcasting mobs */
        if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC) {
            int mana_gain = rand_number(1, 8) + int_app[GET_INT(ch)].learn;
            GET_MAX_MANA(ch) += MAX(mana_gain, 1);
            GET_MANA(ch) = GET_MAX_MANA(ch);
        }

        /* Skills automatically improve with level - handled in get_mob_skill() */

        /* Message for nearby players to see the mob improving */
        if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world && world[IN_ROOM(ch)].people) {
            struct char_data *viewer;
            for (viewer = world[IN_ROOM(ch)].people; viewer; viewer = viewer->next_in_room) {
                if (IS_MOB(viewer) || viewer == ch)
                    continue;
                act("$n parece ter ficado mais experiente!", TRUE, ch, 0, viewer, TO_VICT);
            }
        }
    }
}

void mobile_activity(void)
{
    struct char_data *ch, *next_ch, *vict;
    int found;
    memory_rec *names;

    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;

        if (!ch || !IS_MOB(ch))
            continue;

        /* Skip mobs that have been marked for extraction (e.g., from death traps) */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Safety check: Skip mobs that are not in a valid room */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world) {
            log1("SYSERR: Mobile %s (#%d) is in NOWHERE or invalid room in mobile_activity().",
                 ch ? GET_NAME(ch) : "NULL", ch ? GET_MOB_VNUM(ch) : -1);
            continue;
        }

        /* Examine call for special procedure */
        if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
            if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
                log1("SYSERR: %s (#%d): Attempting to call non-existing mob function.", GET_NAME(ch), GET_MOB_VNUM(ch));
                REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
            } else {
                char actbuf[MAX_INPUT_LENGTH] = "";
                if ((mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, actbuf))
                    continue; /* go to next char */
            }
        }

        // *** ADDED SAFETY CHECK ***
        // A spec proc might have extracted ch and returned FALSE.
        // The check at the top of the loop already caught other flags,
        // but we must re-check here before proceeding.
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        if (FIGHTING(ch) || !AWAKE(ch))
            continue;

        /* Check if mob can level up from gained experience */
        check_mob_level_up(ch);

        if (ch->ai_data && ch->ai_data->duty_frustration_timer > 0) {
            ch->ai_data->duty_frustration_timer--;
        }

        if (ch->ai_data && ch->ai_data->quest_posting_frustration_timer > 0) {
            ch->ai_data->quest_posting_frustration_timer--;
        }

        if (ch->ai_data && ch->ai_data->current_goal != GOAL_NONE) {
            /* Re-verify room validity before complex AI operations */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world) {
                ch->ai_data->current_goal = GOAL_NONE;
                continue;
            }

            /* Increment goal timer and check for timeout */
            ch->ai_data->goal_timer++;

            /* If stuck on shopping goal for too long (50 ticks = ~5 minutes), abandon it */
            if ((ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_SELL ||
                 ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_BUY ||
                 ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER || ch->ai_data->current_goal == GOAL_ACCEPT_QUEST ||
                 ch->ai_data->current_goal == GOAL_COMPLETE_QUEST || ch->ai_data->current_goal == GOAL_COLLECT_KEY) &&
                ch->ai_data->goal_timer > 50) {
                act("$n parece frustrado e desiste da viagem.", FALSE, ch, 0, 0, TO_ROOM);

                /* If abandoning quest completion, fail the quest */
                if (ch->ai_data->current_goal == GOAL_COMPLETE_QUEST) {
                    fail_mob_quest(ch, "timeout");
                }

                /* If abandoning key collection, restore original goal if it exists */
                if (ch->ai_data->current_goal == GOAL_COLLECT_KEY && ch->ai_data->original_goal != GOAL_NONE) {
                    ch->ai_data->current_goal = ch->ai_data->original_goal;
                    ch->ai_data->goal_destination = ch->ai_data->original_destination;
                    ch->ai_data->goal_obj = ch->ai_data->original_obj;
                    ch->ai_data->goal_target_mob_rnum = ch->ai_data->original_target_mob;
                    ch->ai_data->goal_item_vnum = ch->ai_data->original_item_vnum;
                    ch->ai_data->goal_timer = 0;
                    /* Clear original goal data */
                    ch->ai_data->original_goal = GOAL_NONE;
                    ch->ai_data->original_destination = NOWHERE;
                    ch->ai_data->original_obj = NULL;
                    ch->ai_data->original_target_mob = NOBODY;
                    ch->ai_data->original_item_vnum = NOTHING;
                    continue; /* Continue with restored goal */
                } else {
                    ch->ai_data->current_goal = GOAL_NONE;
                    ch->ai_data->goal_destination = NOWHERE;
                    ch->ai_data->goal_obj = NULL;
                    ch->ai_data->goal_target_mob_rnum = NOBODY;
                    ch->ai_data->goal_item_vnum = NOTHING;
                    ch->ai_data->goal_timer = 0;
                    continue; /* Allow other priorities this turn */
                }
            }

            /* Handle goals that don't require movement */
            if (ch->ai_data->current_goal == GOAL_HUNT_TARGET || ch->ai_data->current_goal == GOAL_GET_GOLD) {

                /* For hunting, implement basic hunting behavior */
                if (ch->ai_data->current_goal == GOAL_HUNT_TARGET) {
                    /* Look for the target mob in the current room */
                    struct char_data *target = NULL;
                    struct char_data *temp_char;

                    /* Verify room validity before accessing people list */
                    if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                        for (temp_char = world[IN_ROOM(ch)].people; temp_char; temp_char = temp_char->next_in_room) {
                            if (!IS_NPC(temp_char))
                                continue;
                            if (GET_MOB_RNUM(temp_char) == ch->ai_data->goal_target_mob_rnum) {
                                target = temp_char;
                                break;
                            }
                        }
                    }

                    if (target && !FIGHTING(ch)) {
                        /* Attack the target */
                        act("$n se concentra em $N com olhos determinados.", FALSE, ch, 0, target, TO_ROOM);
                        hit(ch, target, TYPE_UNDEFINED);
                        /* Safety check: hit() can indirectly cause extract_char */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    } else if (ch->ai_data->goal_timer > 100) {
                        /* Give up hunting after too long */
                        ch->ai_data->current_goal = GOAL_NONE;
                        ch->ai_data->goal_target_mob_rnum = NOBODY;
                        ch->ai_data->goal_item_vnum = NOTHING;
                        ch->ai_data->goal_timer = 0;
                    }
                    continue;
                }

                /* For getting gold, prioritize normal looting behavior */
                if (ch->ai_data->current_goal == GOAL_GET_GOLD) {
                    /* The mob's normal looting behavior will be more active */
                    /* This is handled elsewhere in the code */
                    if (ch->ai_data->goal_timer > 200) { /* Give up after longer time */
                        ch->ai_data->current_goal = GOAL_NONE;
                        ch->ai_data->goal_item_vnum = NOTHING;
                        ch->ai_data->goal_timer = 0;
                    }
                    continue;
                }
            }

            /* Handle resource gathering goals */
            if (ch->ai_data->current_goal == GOAL_MINE || ch->ai_data->current_goal == GOAL_FISH ||
                ch->ai_data->current_goal == GOAL_FORAGE || ch->ai_data->current_goal == GOAL_EAVESDROP) {

                /* Check if mob can perform the resource action */
                bool can_perform = FALSE;

                if (ch->ai_data->current_goal == GOAL_MINE) {
                    /* Check if location is suitable for mining */
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && GET_SKILL(ch, SKILL_MINE) > 0) {
                        can_perform = TRUE;
                    }
                } else if (ch->ai_data->current_goal == GOAL_FISH) {
                    /* Check if there's water nearby */
                    if ((SECT(IN_ROOM(ch)) == SECT_WATER_SWIM || SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) &&
                        GET_SKILL(ch, SKILL_FISHING) > 0) {
                        can_perform = TRUE;
                    }
                } else if (ch->ai_data->current_goal == GOAL_FORAGE) {
                    /* Check if location has vegetation */
                    if (SECT(IN_ROOM(ch)) != SECT_CITY && SECT(IN_ROOM(ch)) != SECT_INSIDE &&
                        GET_SKILL(ch, SKILL_FORAGE) > 0) {
                        can_perform = TRUE;
                    }
                } else if (ch->ai_data->current_goal == GOAL_EAVESDROP) {
                    /* Check if there are other people to eavesdrop on */
                    struct char_data *temp_char;
                    /* Verify room validity before accessing people list */
                    if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                        for (temp_char = world[IN_ROOM(ch)].people; temp_char; temp_char = temp_char->next_in_room) {
                            if (temp_char == ch)
                                continue;
                            if (GET_POS(temp_char) >= POS_RESTING && GET_SKILL(ch, SKILL_EAVESDROP) > 0) {
                                can_perform = TRUE;
                                break;
                            }
                        }
                    }
                }

                if (can_perform && rand_number(1, 100) <= 30) { /* 30% chance to perform action each round */
                    /* Perform the appropriate resource action */
                    switch (ch->ai_data->current_goal) {
                        case GOAL_MINE:
                            call_ACMD(do_mine, ch, "", 0, 0);
                            break;
                        case GOAL_FISH:
                            call_ACMD(do_fishing, ch, "", 0, 0);
                            break;
                        case GOAL_FORAGE:
                            call_ACMD(do_forage, ch, "", 0, 0);
                            break;
                        case GOAL_EAVESDROP:
                            call_ACMD(do_eavesdrop, ch, "", 0, 0);
                            break;
                    }

                    /* Safety check: call_ACMD functions might indirectly trigger extract_char
                     * through complex chains (e.g., triggering scripts, special procedures, etc.) */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;

                    /* After performing action, continue with this goal for a while longer */
                    if (ch->ai_data->goal_timer > rand_number(30, 60)) {
                        /* Sometimes switch to a different goal or give up */
                        ch->ai_data->current_goal = GOAL_NONE;
                        ch->ai_data->goal_timer = 0;
                    }
                } else if (ch->ai_data->goal_timer > 100) {
                    /* Give up if can't perform action or been trying too long */
                    ch->ai_data->current_goal = GOAL_NONE;
                    ch->ai_data->goal_timer = 0;
                }
                continue;
            }

            room_rnum dest = ch->ai_data->goal_destination;

            /* Validate destination room */
            if (dest != NOWHERE && (dest < 0 || dest > top_of_world)) {
                ch->ai_data->current_goal = GOAL_NONE;
                ch->ai_data->goal_destination = NOWHERE;
                continue;
            }

            /* Já chegou ao destino? */
            if (dest != NOWHERE && IN_ROOM(ch) == dest) {
                if (ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_SELL) {
                    /* Usa a memória para encontrar o lojista correto. */
                    struct char_data *keeper = get_mob_in_room_by_rnum(IN_ROOM(ch), ch->ai_data->goal_target_mob_rnum);
                    if (keeper && ch->ai_data->goal_obj) {
                        int shop_rnum = find_shop_by_keeper(keeper->nr);

                        /* Check if this shop actually buys this type of item */
                        bool shop_buys_this_item = FALSE;
                        if (shop_rnum != -1 && shop_rnum <= top_shop && shop_index[shop_rnum].type) {
                            for (int i = 0; i < MAX_TRADE && SHOP_BUYTYPE(shop_rnum, i) != NOTHING; i++) {
                                if (SHOP_BUYTYPE(shop_rnum, i) == GET_OBJ_TYPE(ch->ai_data->goal_obj)) {
                                    shop_buys_this_item = TRUE;
                                    break;
                                }
                            }
                        }

                        if (shop_buys_this_item) {
                            /* Shop buys this item, proceed with sale */
                            shopping_sell(ch->ai_data->goal_obj->name, ch, keeper, shop_rnum);
                            /* Clear goal_obj pointer as the object has been extracted by shopping_sell */
                            ch->ai_data->goal_obj = NULL;
                            ch->ai_data->genetics.trade_tendency += 1;
                            ch->ai_data->genetics.trade_tendency = MIN(ch->ai_data->genetics.trade_tendency, 100);
                        } else {
                            /* Shop doesn't buy this item, find a new shop */
                            int new_shop_rnum = find_best_shop_to_sell(ch, ch->ai_data->goal_obj);
                            if (new_shop_rnum != -1 && new_shop_rnum <= top_shop && shop_index[new_shop_rnum].in_room) {
                                room_rnum new_target_room = real_room(SHOP_ROOM(new_shop_rnum, 0));
                                if (new_target_room != NOWHERE && find_first_step(IN_ROOM(ch), new_target_room) != -1) {
                                    /* Found a new shop, update goal */
                                    ch->ai_data->goal_destination = new_target_room;
                                    ch->ai_data->goal_target_mob_rnum = SHOP_KEEPER(new_shop_rnum);
                                    act("$n percebe que esta loja não compra o que tem e decide procurar outra.", FALSE,
                                        ch, 0, 0, TO_ROOM);
                                    continue; /* Continue with updated goal */
                                }
                            }
                            /* No suitable shop found, abandon goal */
                            act("$n parece frustrado por não conseguir vender os seus itens.", FALSE, ch, 0, 0,
                                TO_ROOM);
                            ch->ai_data->current_goal = GOAL_NONE;
                            ch->ai_data->goal_destination = NOWHERE;
                            ch->ai_data->goal_obj = NULL;
                            ch->ai_data->goal_target_mob_rnum = NOBODY;
                            ch->ai_data->goal_item_vnum = NOTHING;
                            ch->ai_data->goal_timer = 0;
                            continue;
                        }
                        /* After selling, check if there are more items to sell to this same shop */
                        struct obj_data *next_item_to_sell = NULL;
                        int min_score = 10;

                        if (shop_rnum != -1 && shop_index[shop_rnum].type) {
                            /* Look for more junk to sell to this same shop */
                            struct obj_data *obj;
                            for (obj = ch->carrying; obj; obj = obj->next_content) {
                                if (OBJ_FLAGGED(obj, ITEM_NODROP) ||
                                    (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains))
                                    continue;

                                /* Check if this shop buys this type of item */
                                bool shop_buys_this = FALSE;
                                for (int i = 0; SHOP_BUYTYPE(shop_rnum, i) != NOTHING; i++) {
                                    if (SHOP_BUYTYPE(shop_rnum, i) == GET_OBJ_TYPE(obj)) {
                                        shop_buys_this = TRUE;
                                        break;
                                    }
                                }

                                if (shop_buys_this) {
                                    int score = evaluate_item_for_mob(ch, obj);
                                    if (score < min_score) {
                                        min_score = score;
                                        next_item_to_sell = obj;
                                    }
                                }
                            }

                            /* If we found another item to sell to this shop, continue selling */
                            if (next_item_to_sell) {
                                ch->ai_data->goal_obj = next_item_to_sell;
                                continue; /* Continue with current goal, don't clear it */
                            }
                        }
                    }
                } else if (ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_BUY) {
                    /* Chegou à loja para comprar um item da wishlist */
                    struct char_data *keeper = get_mob_in_room_by_rnum(IN_ROOM(ch), ch->ai_data->goal_target_mob_rnum);
                    if (keeper && ch->ai_data->goal_item_vnum != NOTHING) {
                        /* Tenta comprar o item */
                        char buy_command[MAX_INPUT_LENGTH];
                        act("$n olha os produtos da loja.", FALSE, ch, 0, 0, TO_ROOM);
                        sprintf(buy_command, "%d", ch->ai_data->goal_item_vnum);
                        shopping_buy(buy_command, ch, keeper, find_shop_by_keeper(keeper->nr));

                        /* Safety check: shopping operations could indirectly cause extract_char
                         * through scripts, triggers, or special procedures */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;

                        /* Remove o item da wishlist se a compra foi bem sucedida */
                        remove_item_from_wishlist(ch, ch->ai_data->goal_item_vnum);
                        act("$n parece satisfeito com a sua compra.", FALSE, ch, 0, 0, TO_ROOM);
                    }
                } else if (ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER) {
                    /* Chegou ao questmaster para postar uma quest */
                    if (ch->ai_data->goal_item_vnum != NOTHING) {
                        /* Encontra o item na wishlist para obter a prioridade correta */
                        struct mob_wishlist_item *wishlist_item =
                            find_item_in_wishlist(ch, ch->ai_data->goal_item_vnum);
                        int reward = wishlist_item ? wishlist_item->priority * 2 : ch->ai_data->goal_item_vnum;

                        /* Posta a quest no questmaster */
                        mob_posts_quest(ch, ch->ai_data->goal_item_vnum, reward);
                        act("$n fala com o questmaster e entrega um pergaminho.", FALSE, ch, 0, 0, TO_ROOM);
                    }
                } else if (ch->ai_data->current_goal == GOAL_ACCEPT_QUEST) {
                    /* Chegou ao questmaster para aceitar uma quest */
                    struct char_data *questmaster =
                        get_mob_in_room_by_rnum(IN_ROOM(ch), ch->ai_data->goal_target_mob_rnum);
                    if (questmaster && !GET_QUEST(ch)) {
                        /* Procura por quests disponíveis neste questmaster */
                        qst_vnum available_quest = find_available_quest_by_qmnum(ch, GET_MOB_VNUM(questmaster), 1);
                        if (available_quest != NOTHING) {
                            qst_rnum quest_rnum = real_quest(available_quest);
                            if (quest_rnum != NOTHING && mob_should_accept_quest(ch, quest_rnum)) {
                                set_mob_quest(ch, quest_rnum);
                                act("$n fala com $N e aceita uma tarefa.", FALSE, ch, 0, questmaster, TO_ROOM);

                                /* Automatically transition to quest completion goal */
                                ch->ai_data->current_goal = GOAL_COMPLETE_QUEST;
                                ch->ai_data->goal_destination = NOWHERE;
                                ch->ai_data->goal_target_mob_rnum = NOBODY;
                                ch->ai_data->goal_item_vnum = NOTHING;
                                ch->ai_data->goal_timer = 0;
                                continue; /* Process quest completion immediately */
                            }
                        } else {
                            act("$n fala com $N mas parece não haver tarefas disponíveis.", FALSE, ch, 0, questmaster,
                                TO_ROOM);
                        }
                    }
                } else if (ch->ai_data->current_goal == GOAL_COMPLETE_QUEST) {
                    /* Process quest completion based on quest type */
                    if (GET_QUEST(ch) != NOTHING) {
                        qst_rnum quest_rnum = real_quest(GET_QUEST(ch));
                        if (quest_rnum != NOTHING) {
                            if (mob_process_quest_completion(ch, quest_rnum)) {
                                /* Quest completed, goal will be cleared in mob_complete_quest */
                                continue;
                            }
                        } else {
                            /* Invalid quest, clear it */
                            clear_mob_quest(ch);
                        }
                    } else {
                        /* No quest, clear goal */
                        ch->ai_data->current_goal = GOAL_NONE;
                    }
                } else if (ch->ai_data->current_goal == GOAL_COLLECT_KEY) {
                    /* Arrived at key location - try to collect the key */
                    struct obj_data *key_obj = NULL, *next_obj = NULL;
                    obj_vnum target_key = ch->ai_data->goal_item_vnum;
                    bool key_collected = FALSE;

                    /* Look for the key on the ground */
                    for (key_obj = world[IN_ROOM(ch)].contents; key_obj; key_obj = next_obj) {
                        next_obj = key_obj->next_content; /* Save next pointer before obj_from_room */
                        if (GET_OBJ_TYPE(key_obj) == ITEM_KEY && GET_OBJ_VNUM(key_obj) == target_key) {
                            obj_from_room(key_obj);
                            obj_to_char(key_obj, ch);
                            act("$n pega $p do chão.", FALSE, ch, key_obj, 0, TO_ROOM);
                            key_collected = TRUE;
                            break;
                        }
                    }

                    /* If not found on ground, look in containers */
                    if (!key_collected) {
                        struct obj_data *container, *next_container;
                        for (container = world[IN_ROOM(ch)].contents; container; container = next_container) {
                            next_container = container->next_content;
                            if (GET_OBJ_TYPE(container) == ITEM_CONTAINER && !OBJVAL_FLAGGED(container, CONT_CLOSED)) {
                                for (key_obj = container->contains; key_obj; key_obj = next_obj) {
                                    next_obj = key_obj->next_content; /* Save next pointer before obj_from_obj */
                                    if (GET_OBJ_TYPE(key_obj) == ITEM_KEY && GET_OBJ_VNUM(key_obj) == target_key) {
                                        obj_from_obj(key_obj);
                                        obj_to_char(key_obj, ch);
                                        act("$n pega $p de $P.", FALSE, ch, key_obj, container, TO_ROOM);
                                        key_collected = TRUE;
                                        break;
                                    }
                                }
                                if (key_collected)
                                    break;
                            }
                        }
                    }

                    /* If not found in containers, look for mob carrying it */
                    if (!key_collected && ch->ai_data->goal_target_mob_rnum != NOBODY) {
                        struct char_data *target_mob =
                            get_mob_in_room_by_rnum(IN_ROOM(ch), ch->ai_data->goal_target_mob_rnum);
                        if (target_mob && !FIGHTING(ch)) {
                            /* Attack the mob to get the key */
                            act("$n ataca $N para obter algo que precisa.", FALSE, ch, 0, target_mob, TO_ROOM);
                            hit(ch, target_mob, TYPE_UNDEFINED);
                            /* Safety check: hit() can indirectly cause extract_char */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;
                            /* Don't clear goal yet - continue fighting until key is obtained */
                            continue;
                        }
                    }

                    /* If key was collected, restore original goal */
                    if (key_collected) {
                        if (ch->ai_data->original_goal != GOAL_NONE) {
                            ch->ai_data->current_goal = ch->ai_data->original_goal;
                            ch->ai_data->goal_destination = ch->ai_data->original_destination;
                            ch->ai_data->goal_obj = ch->ai_data->original_obj;
                            ch->ai_data->goal_target_mob_rnum = ch->ai_data->original_target_mob;
                            ch->ai_data->goal_item_vnum = ch->ai_data->original_item_vnum;
                            ch->ai_data->goal_timer = 0;

                            /* Clear original goal data */
                            ch->ai_data->original_goal = GOAL_NONE;
                            ch->ai_data->original_destination = NOWHERE;
                            ch->ai_data->original_obj = NULL;
                            ch->ai_data->original_target_mob = NOBODY;
                            ch->ai_data->original_item_vnum = NOTHING;

                            act("$n parece satisfeito por ter encontrado o que procurava.", FALSE, ch, 0, 0, TO_ROOM);
                            continue; /* Continue with restored goal */
                        }
                    }
                    /* If key not found, goal will be cleared below */
                }
                /* Limpa o objetivo, pois foi concluído. */
                ch->ai_data->current_goal = GOAL_NONE;
                ch->ai_data->goal_destination = NOWHERE;
                ch->ai_data->goal_obj = NULL;
                ch->ai_data->goal_target_mob_rnum = NOBODY;
                ch->ai_data->goal_item_vnum = NOTHING;
                ch->ai_data->goal_timer = 0;
            } else {
                /* Ainda não chegou. Continua a vaguear em direção ao objetivo. */
                mob_goal_oriented_roam(ch, dest);
                /* Safety check: mob_goal_oriented_roam uses perform_move which can trigger death traps */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    continue;
            }

            continue; /* O turno do mob foi gasto a trabalhar no seu objetivo. */
        }

        if (mob_index[GET_MOB_RNUM(ch)].func == shop_keeper) {
            int shop_nr = find_shop_by_keeper(GET_MOB_RNUM(ch));
            if (shop_nr != -1 && !is_shop_open(shop_nr)) {
                /* --- IA ECONÓMICA: GESTÃO DE STOCK --- */

                /* O lojista verifica o seu inventário à procura de lixo para destruir. */
                struct obj_data *current_obj, *next_obj;
                for (current_obj = ch->carrying; current_obj; current_obj = next_obj) {
                    next_obj = current_obj->next_content;
                    if (OBJ_FLAGGED(current_obj, ITEM_TRASH)) {
                        act("$n joga $p fora.", FALSE, ch, current_obj, 0, TO_ROOM);
                        extract_obj(current_obj);
                    }
                }
                continue;
            }
        }

        if GROUP (ch) {
            mob_follow_leader(ch);
        }

        /* Try stealth following (for non-grouped following behavior) */
        mob_try_stealth_follow(ch);

        mob_assist_allies(ch);

        mob_try_and_loot(ch);

        /* hunt a victim, if applicable */
        hunt_victim(ch);

        // *** ADDED SAFETY CHECK ***
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Wishlist-based goal planning */
        if (ch->ai_data && rand_number(1, 100) <= 10) { /* 10% chance per tick */
            mob_process_wishlist_goals(ch);
        }

        /* Quest acceptance - try to find and accept quests occasionally */
        if (ch->ai_data && rand_number(1, 100) <= 3) { /* 3% chance per tick to seek quests */
            mob_try_to_accept_quest(ch);
        }

        /* Mob quest processing */
        if (ch->ai_data && rand_number(1, 100) <= 5) { /* 5% chance per tick to check quests */
            /* Check if mob has a quest and handle quest-related goals */
            if (GET_MOB_QUEST(ch) != NOTHING) {
                /* Decrement quest timer if applicable */
                if (ch->ai_data->quest_timer > 0) {
                    ch->ai_data->quest_timer--;
                    if (ch->ai_data->quest_timer <= 0) {
                        /* Quest timeout */
                        act("$n parece desapontado por não completar uma tarefa a tempo.", TRUE, ch, 0, 0, TO_ROOM);
                        fail_mob_quest(ch, "timeout");
                    }
                }
            } else {
                /* Mob doesn't have a quest, check if it should try to find one */
                if (GET_GENQUEST(ch) > 30 && GET_GENADVENTURER(ch) > 20) {
                    /* Look for available mob-posted quests */
                    qst_rnum rnum;
                    for (rnum = 0; rnum < total_quests; rnum++) {
                        if (IS_SET(QST_FLAGS(rnum), AQ_MOB_POSTED) && mob_should_accept_quest(ch, rnum)) {
                            set_mob_quest(ch, rnum);
                            act("$n parece determinado e parte em uma missão.", TRUE, ch, 0, 0, TO_ROOM);
                            break;
                        }
                    }
                }
            }
        }

        /* Mob combat quest posting - chance to post bounty/revenge quests */
        if (ch->ai_data && rand_number(1, 100) <= 2) { /* 2% chance per tick to consider posting combat quests */
            /* Check if mob should post a player kill quest (revenge for being attacked) */
            if (GET_GENBRAVE(ch) > 50 && GET_GENQUEST(ch) > 40) {
                /* Check if mob was recently attacked by a player (has hostile memory) */
                struct char_data *attacker = HUNTING(ch);
                if (attacker && !IS_NPC(attacker) && GET_GOLD(ch) > 200) {
                    /* Post a player kill quest for revenge */
                    int reward = MIN(GET_GOLD(ch) / 3, 500 + rand_number(0, 300));
                    mob_posts_combat_quest(ch, AQ_PLAYER_KILL, NOTHING, reward);
                    HUNTING(ch) = NULL; /* Clear hunting after posting quest */
                }
            }

            /* Check if mob should post a bounty quest against hostile mobs in area */
            if (GET_GENQUEST(ch) > 60 && GET_GOLD(ch) > 300) {
                /* Safety check: Validate ch's room before accessing world array */
                if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                    struct char_data *target, *next_target;
                    /* Look for aggressive mobs in the same zone */
                    for (target = character_list; target; target = next_target) {
                        next_target = target->next;

                        /* Safety check: Skip characters marked for extraction */
                        if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                            continue;

                        /* Safety check: Validate room before accessing world array */
                        if (!IS_NPC(target) || target == ch || IN_ROOM(target) == NOWHERE || IN_ROOM(target) < 0 ||
                            IN_ROOM(target) > top_of_world)
                            continue;

                        if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone &&
                            MOB_FLAGGED(target, MOB_AGGRESSIVE) && GET_ALIGNMENT(target) < -200 &&
                            GET_LEVEL(target) >= GET_LEVEL(ch) - 5) {

                            /* Post bounty quest against this aggressive mob */
                            int reward = MIN(GET_GOLD(ch) / 4, 400 + GET_LEVEL(target) * 10);
                            mob_posts_combat_quest(ch, AQ_MOB_KILL_BOUNTY, GET_MOB_VNUM(target), reward);
                            break; /* Only post one bounty quest per tick */
                        }
                    }
                }
            }
        }

        /* Additional quest posting - exploration, protection, and general kill quests */
        if (ch->ai_data && rand_number(1, 100) <= 3) { /* 3% chance per tick for other quest types */
            /* Safety check: Validate ch's room before accessing world array */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                continue;

            struct char_data *target;
            int reward;

            /* Check genetics and decide what type of quest to post */
            if (GET_GENADVENTURER(ch) > 50 && rand_number(1, 100) <= 30) {
                /* Post exploration quests */
                if (rand_number(1, 100) <= 40) {
                    /* AQ_OBJ_FIND quest - find a random object in the zone */
                    zone_rnum mob_zone = world[IN_ROOM(ch)].zone;
                    obj_vnum obj_target = NOTHING;

                    /* Find a suitable object in the zone to search for */
                    for (int i = zone_table[mob_zone].bot; i <= zone_table[mob_zone].top; i++) {
                        if (real_object(i) != NOTHING) {
                            obj_target = i;
                            break;
                        }
                    }

                    if (obj_target != NOTHING && GET_GOLD(ch) > 100) {
                        reward = MIN(GET_GOLD(ch) / 6, 200 + GET_LEVEL(ch) * 5);
                        mob_posts_exploration_quest(ch, AQ_OBJ_FIND, obj_target, reward);
                    }
                } else if (rand_number(1, 100) <= 50) {
                    /* AQ_ROOM_FIND quest - explore a room in the zone */
                    zone_rnum mob_zone = world[IN_ROOM(ch)].zone;
                    room_rnum room_target =
                        zone_table[mob_zone].bot + rand_number(0, zone_table[mob_zone].top - zone_table[mob_zone].bot);

                    if (GET_GOLD(ch) > 75) {
                        reward = MIN(GET_GOLD(ch) / 8, 150 + GET_LEVEL(ch) * 3);
                        mob_posts_exploration_quest(ch, AQ_ROOM_FIND, world[real_room(room_target)].number, reward);
                    }
                } else {
                    /* AQ_MOB_FIND quest - find a friendly mob */
                    struct char_data *next_target;
                    for (target = character_list; target; target = next_target) {
                        next_target = target->next;

                        /* Safety check: Skip characters marked for extraction */
                        if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                            continue;

                        /* Safety check: Validate room before accessing world array */
                        if (!IS_NPC(target) || target == ch || IN_ROOM(target) == NOWHERE || IN_ROOM(target) < 0 ||
                            IN_ROOM(target) > top_of_world)
                            continue;

                        /* Safety check: Validate ch's room before zone comparison */
                        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                            break;

                        if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone && GET_ALIGNMENT(target) > 0 &&
                            !MOB_FLAGGED(target, MOB_AGGRESSIVE)) {

                            if (GET_GOLD(ch) > 80) {
                                reward = MIN(GET_GOLD(ch) / 7, 120 + GET_LEVEL(target) * 4);
                                mob_posts_exploration_quest(ch, AQ_MOB_FIND, GET_MOB_VNUM(target), reward);
                            }
                            break; /* Only post one find quest per tick */
                        }
                    }
                }
            } else if (GET_GENBRAVE(ch) > 60 && rand_number(1, 100) <= 25) {
                /* Post protection quests */
                if (rand_number(1, 100) <= 60) {
                    /* AQ_MOB_SAVE quest - protect a weak mob */
                    struct char_data *next_target;
                    for (target = character_list; target; target = next_target) {
                        next_target = target->next;

                        /* Safety check: Skip characters marked for extraction */
                        if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                            continue;

                        /* Safety check: Validate room before accessing world array */
                        if (!IS_NPC(target) || target == ch || IN_ROOM(target) == NOWHERE || IN_ROOM(target) < 0 ||
                            IN_ROOM(target) > top_of_world)
                            continue;

                        /* Safety check: Validate ch's room before zone comparison */
                        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                            break;

                        if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone &&
                            GET_LEVEL(target) < GET_LEVEL(ch) && GET_ALIGNMENT(target) > 200) {

                            if (GET_GOLD(ch) > 120) {
                                reward = MIN(GET_GOLD(ch) / 5, 250 + GET_LEVEL(target) * 6);
                                mob_posts_protection_quest(ch, AQ_MOB_SAVE, GET_MOB_VNUM(target), reward);
                            }
                            break; /* Only post one save quest per tick */
                        }
                    }
                } else {
                    /* AQ_ROOM_CLEAR quest - clear a dangerous room */
                    zone_rnum mob_zone = world[IN_ROOM(ch)].zone;
                    room_rnum dangerous_room = NOWHERE;

                    /* Find a room with aggressive mobs */
                    for (int r = zone_table[mob_zone].bot; r <= zone_table[mob_zone].top; r++) {
                        room_rnum real_r = real_room(r);
                        if (real_r != NOWHERE) {
                            for (target = world[real_r].people; target; target = target->next_in_room) {
                                if (IS_NPC(target) && MOB_FLAGGED(target, MOB_AGGRESSIVE)) {
                                    dangerous_room = r;
                                    break;
                                }
                            }
                            if (dangerous_room != NOWHERE)
                                break;
                        }
                    }

                    if (dangerous_room != NOWHERE && GET_GOLD(ch) > 150) {
                        reward = MIN(GET_GOLD(ch) / 4, 300 + GET_LEVEL(ch) * 8);
                        mob_posts_protection_quest(ch, AQ_ROOM_CLEAR, dangerous_room, reward);
                    }
                }
            } else if (GET_GENQUEST(ch) > 40 && rand_number(1, 100) <= 20) {
                /* Post general kill quests */
                struct char_data *next_target;
                for (target = character_list; target; target = next_target) {
                    next_target = target->next;

                    /* Safety check: Skip characters marked for extraction */
                    if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                        continue;

                    /* Safety check: Validate room before accessing world array */
                    if (!IS_NPC(target) || target == ch || IN_ROOM(target) == NOWHERE || IN_ROOM(target) < 0 ||
                        IN_ROOM(target) > top_of_world)
                        continue;

                    /* Safety check: Validate ch's room before zone comparison */
                    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                        break;

                    if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone && GET_ALIGNMENT(target) < -100 &&
                        GET_LEVEL(target) >= GET_LEVEL(ch) - 10) {

                        if (GET_GOLD(ch) > 100) {
                            reward = MIN(GET_GOLD(ch) / 5, 200 + GET_LEVEL(target) * 8);
                            mob_posts_general_kill_quest(ch, GET_MOB_VNUM(target), reward);
                        }
                        break; /* Only post one kill quest per tick */
                    }
                }
            }
        }

        mob_handle_grouping(ch);

        /* Aggressive Mobs */
        if (!MOB_FLAGGED(ch, MOB_HELPER) && (!AFF_FLAGGED(ch, AFF_BLIND) || !AFF_FLAGGED(ch, AFF_CHARM))) {
            found = FALSE;
            /* Re-verify room validity before accessing room data */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                continue;

            for (vict = world[IN_ROOM(ch)].people; vict && !found;) {
                /* Check vict validity before dereferencing */
                if (!vict) {
                    break;
                }

                struct char_data *next_vict = vict->next_in_room; /* Save next pointer before any actions */

                //	if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
                if (!CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE)) {
                    vict = next_vict;
                    continue;
                }

                if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict)) {
                    vict = next_vict;
                    continue;
                }

                if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) || (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
                    (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
                    (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict))) {

                    /* Can a master successfully control the charmed monster? */
                    if (aggressive_mob_on_a_leash(ch, ch->master, vict)) {
                        vict = next_vict;
                        continue;
                    }

                    if (vict == ch) {
                        vict = next_vict;
                        continue;
                    }

                    // if (IS_NPC(vict))
                    // continue;

                    if (rand_number(0, 20) <= GET_CHA(vict)) {
                        act("$n olha para $N com indiferença.", FALSE, ch, 0, vict, TO_NOTVICT);
                        act("$N olha para você com indiferença.", FALSE, vict, 0, ch, TO_CHAR);
                    } else {
                        hit(ch, vict, TYPE_UNDEFINED);
                        /* Safety check: hit() can indirectly cause extract_char */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                        found = TRUE;
                    }
                }

                vict = next_vict; /* Move to next victim safely */
            }
        }

        // *** ADDED SAFETY CHECK ***
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        mob_try_and_upgrade(ch);

        mob_share_gear_with_group(ch);

        /* Bank usage for mobs with high trade genetics */
        mob_use_bank(ch);

        if (handle_duty_routine(ch)) {
            continue;
        }

        /* Prioridade de Vaguear (Roam) */
        if (!mob_try_to_sell_junk(ch)) {
            mob_goal_oriented_roam(ch, NOWHERE);
            /* Safety check: mob_goal_oriented_roam uses perform_move which can trigger death traps */
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                continue;
        } else {
            /* Safety check: mob_try_to_sell_junk can call mob_goal_oriented_roam internally */
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                continue;
        }

        /* Mob Memory */
        if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
            found = FALSE;
            /* Re-verify room validity before accessing room data */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                continue;

            for (vict = world[IN_ROOM(ch)].people; vict && !found;) {
                /* Check vict validity before dereferencing */
                if (!vict) {
                    break;
                }

                struct char_data *next_vict = vict->next_in_room; /* Save next pointer before any actions */

                if (!CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE)) {
                    vict = next_vict;
                    continue;
                }

                for (names = MEMORY(ch); names && !found; names = names->next) {
                    if (!names)
                        break; /* Safety check - names became NULL */

                    if (names->id != GET_IDNUM(vict))
                        continue;

                    /* Can a master successfully control the charmed monster? */
                    if (aggressive_mob_on_a_leash(ch, ch->master, vict))
                        continue;

                    found = TRUE;
                    act("''Ei!  Você é o demônio que me atacou!!!', exclama $n.", FALSE, ch, 0, 0, TO_ROOM);
                    hit(ch, vict, TYPE_UNDEFINED);
                    /* Safety check: hit() can indirectly cause extract_char */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                }

                vict = next_vict; /* Move to next victim safely */
            }
        }

        // *** ADDED SAFETY CHECK ***
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Charmed Mob Rebellion: In order to rebel, there need to be more charmed
         * monsters than the person can feasibly control at a time.  Then the
         * mobiles have a chance based on the charisma of their leader.
         * 1-4 = 0, 5-7 = 1, 8-10 = 2, 11-13 = 3, 14-16 = 4, 17-19 = 5, etc. */
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
            num_followers_charmed(ch->master) > (GET_CHA(ch->master) - 2) / 3) {
            if (!aggressive_mob_on_a_leash(ch, ch->master, ch->master)) {
                if (CAN_SEE(ch, ch->master) && !PRF_FLAGGED(ch->master, PRF_NOHASSLE))
                    hit(ch, ch->master, TYPE_UNDEFINED);
                /* Safety check: hit() can indirectly cause extract_char */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    continue;
                stop_follower(ch);
            }
        }

        // *** ADDED SAFETY CHECK ***
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Resource gathering goal assignment for idle mobs */
        if (ch->ai_data && ch->ai_data->current_goal == GOAL_NONE && rand_number(1, 1000) <= 10) {
            /* 1% chance per tick for mob to start resource gathering if they have no other goals */

            /* Check genetics to determine preferred resource activity */
            int activity_choice = rand_number(1, 100);

            if (ch->ai_data->genetics.loot_tendency > 60 && activity_choice <= 30) {
                /* High loot tendency mobs prefer mining for valuable materials */
                if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && GET_SKILL(ch, SKILL_MINE) > 0) {
                    ch->ai_data->current_goal = GOAL_MINE;
                    ch->ai_data->goal_timer = 0;
                    act("$n olha ao redor procurando minerais.", FALSE, ch, 0, 0, TO_ROOM);
                }
            } else if (SECT(IN_ROOM(ch)) == SECT_WATER_SWIM || SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) {
                /* Near water, try fishing */
                if (GET_SKILL(ch, SKILL_FISHING) > 0 && activity_choice <= 40) {
                    ch->ai_data->current_goal = GOAL_FISH;
                    ch->ai_data->goal_timer = 0;
                    act("$n olha para a água pensativamente.", FALSE, ch, 0, 0, TO_ROOM);
                }
            } else if (SECT(IN_ROOM(ch)) != SECT_CITY && SECT(IN_ROOM(ch)) != SECT_INSIDE) {
                /* In wilderness, try foraging */
                if (GET_SKILL(ch, SKILL_FORAGE) > 0 && activity_choice <= 50) {
                    ch->ai_data->current_goal = GOAL_FORAGE;
                    ch->ai_data->goal_timer = 0;
                    act("$n examina a vegetação local.", FALSE, ch, 0, 0, TO_ROOM);
                }
            } else if (ch->ai_data->genetics.loot_tendency > 40) {
                /* In social areas, try eavesdropping for information */
                struct char_data *temp_char;
                bool has_targets = FALSE;
                /* Verify room validity before accessing people list */
                if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                    for (temp_char = world[IN_ROOM(ch)].people; temp_char; temp_char = temp_char->next_in_room) {
                        if (temp_char == ch)
                            continue;
                        if (GET_POS(temp_char) >= POS_RESTING) {
                            has_targets = TRUE;
                            break;
                        }
                    }
                }

                if (has_targets && GET_SKILL(ch, SKILL_EAVESDROP) > 0 && activity_choice <= 25) {
                    ch->ai_data->current_goal = GOAL_EAVESDROP;
                    ch->ai_data->goal_timer = 0;
                    act("$n discretamente presta atenção ao que acontece ao redor.", FALSE, ch, 0, 0, TO_ROOM);
                }
            }
        }

        /* Evil mobs (alignment -350) try to poison drink containers */
        if (GET_ALIGNMENT(ch) <= -350 && rand_number(1, 100) <= 10) {
            struct char_data *victim;
            struct obj_data *container;

            /* Verify room validity before accessing people list */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                continue;

            /* Look for potential victims in the room */
            for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room) {
                if (victim == ch || IS_NPC(victim))
                    continue;

                /* Look for drink containers in victim's inventory */
                for (container = victim->carrying; container; container = container->next_content) {
                    if (GET_OBJ_TYPE(container) == ITEM_DRINKCON && GET_OBJ_VAL(container, 1) > 0 && /* Has liquid */
                        GET_OBJ_VAL(container, 3) == 0) { /* Not already poisoned */

                        /* 50% chance to successfully poison if evil enough */
                        if (rand_number(1, 100) <= 50) {
                            GET_OBJ_VAL(container, 3) = 1; /* Poison it */

                            /* Subtle message - victim might not notice */
                            if (rand_number(1, 100) <= 20) {
                                act("$n parece sussurrar algo enquanto se aproxima de você.", FALSE, ch, 0, victim,
                                    TO_VICT);
                            }

                            /* Observer message */
                            act("$n se aproxima discretamente de $N por um momento.", FALSE, ch, 0, victim, TO_NOTVICT);
                            return; /* Only poison one container per round */
                        }
                    }
                }
            }
        }

        /* Add new mobile actions here */

    } /* end for() */
}

/* Mob Memory Routines */
/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
    memory_rec *tmp;
    bool present = FALSE;

    if (IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
        return;

    for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
        if (tmp->id == GET_IDNUM(victim))
            present = TRUE;

    if (!present) {
        CREATE(tmp, memory_rec, 1);
        tmp->next = MEMORY(ch);
        tmp->id = GET_IDNUM(victim);
        MEMORY(ch) = tmp;
    }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
    memory_rec *curr, *prev = NULL;

    if (!(curr = MEMORY(ch)))
        return;

    while (curr && curr->id != GET_IDNUM(victim)) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
        return; /* person wasn't there at all. */

    if (curr == MEMORY(ch))
        MEMORY(ch) = curr->next;
    else
        prev->next = curr->next;

    free(curr);
}

/* erase ch's memory */
void clearMemory(struct char_data *ch)
{
    memory_rec *curr, *next;

    curr = MEMORY(ch);

    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    MEMORY(ch) = NULL;
}

/* An aggressive mobile wants to attack something.  If they're under the
 * influence of mind altering PC, then see if their master can talk them out
 * of it, eye them down, or otherwise intimidate the slave. */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack)
{
    static int snarl_cmd;
    int dieroll;

    if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
        return (FALSE);

    if (!snarl_cmd)
        snarl_cmd = find_command("snarl");

    /* Sit. Down boy! HEEEEeeeel! */
    dieroll = rand_number(1, 20);
    if (dieroll != 1 && (dieroll == 20 || dieroll > 10 - GET_CHA(master) + GET_INT(slave))) {
        if (snarl_cmd > 0 && attack && !rand_number(0, 3)) {
            char victbuf[MAX_NAME_LENGTH + 1];

            strncpy(victbuf, GET_NAME(attack), sizeof(victbuf)); /* strncpy: OK */
            victbuf[sizeof(victbuf) - 1] = '\0';

            do_action(slave, victbuf, snarl_cmd, 0);
        }

        /* Success! But for how long? Hehe. */
        return (TRUE);
    }

    /* So sorry, now you're a player killer... Tsk tsk. */
    return (FALSE);
}

/**
 * Verifica se um mob possui qualquer item do tipo munição.
 * @param ch O mob a ser verificado.
 * @return TRUE se tiver munição, FALSE caso contrário.
 */
bool mob_has_ammo(struct char_data *ch)
{
    struct obj_data *obj;

    /* Primeiro, verifica o item mais óbvio: a aljava (quiver) equipada. */
    if (GET_EQ(ch, WEAR_QUIVER) != NULL) {
        return TRUE;
    }

    /* Se não, percorre o inventário. */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_AMMO) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Verifica se dois mobs são compatíveis para se agruparem, baseado
 * no nível e no alinhamento.
 */
bool are_groupable(struct char_data *ch, struct char_data *target)
{
    /* Regras básicas de compatibilidade (não agrupa com jogadores, etc.) */
    if (!target || ch == target || !IS_NPC(target) || target->master != NULL)
        return FALSE;

    /* REGRA DE ALINHAMENTO */
    bool align_ok = FALSE;
    if (IS_NEUTRAL(ch) || IS_NEUTRAL(target))
        align_ok = TRUE; /* Neutros podem com todos. */
    else if ((IS_GOOD(ch) && IS_GOOD(target)) || (IS_EVIL(ch) && IS_EVIL(target)))
        align_ok = TRUE; /* Bem com Bem, Mal com Mal. */

    if (!align_ok)
        return FALSE;

    /* REGRA DE NÍVEL (simples, para verificações 1 a 1) */
    if (abs(GET_LEVEL(ch) - GET_LEVEL(target)) > 15)
        return FALSE;

    return TRUE;
}

/**
 * Verifica se um novo membro ('prospect') é compatível em nível com um
 * grupo já existente, respeitando a regra de 15 níveis de diferença
 * entre o novo mínimo e o novo máximo do grupo potencial.
 */
bool is_level_compatible_with_group(struct char_data *prospect, struct group_data *group)
{
    if (!prospect || !group || !group->members || group->members->iSize == 0)
        return FALSE;

    struct char_data *member;
    struct iterator_data iterator;

    int min_level = GET_LEVEL(prospect);
    int max_level = GET_LEVEL(prospect);

    /* Itera pelos membros existentes para encontrar o min/max atual do grupo */
    member = (struct char_data *)merge_iterator(&iterator, group->members);
    while (member) {
        if (GET_LEVEL(member) < min_level)
            min_level = GET_LEVEL(member);
        if (GET_LEVEL(member) > max_level)
            max_level = GET_LEVEL(member);
        member = (struct char_data *)next_in_list(&iterator);
    }

    /* Retorna TRUE se a nova diferença total do grupo for 15 ou menos. */
    return ((max_level - min_level) <= 15);
}

/**
 * Avalia todos os mobs solitários numa sala para encontrar o melhor candidato
 * a líder para um novo grupo, com base na regra do "líder mediano".
 * @param ch O mob que está a iniciar a verificação.
 * @return Um ponteiro para o melhor candidato a líder, ou NULL se nenhum grupo for viável.
 */
struct char_data *find_best_leader_for_new_group(struct char_data *ch)
{
    struct char_data *vict, *leader_candidate;
    int min_level = -1, max_level = -1;
    int count = 0;
    struct char_data *potential_members[51]; /* Buffer para potenciais membros */

    /* Safety check: Validate room before accessing people list */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return NULL;

    /* 1. Reúne todos os candidatos (mobs solitários e compatíveis) na sala. */
    for (vict = world[IN_ROOM(ch)].people; vict && count < 50; vict = vict->next_in_room) {
        if (!IS_NPC(vict) || vict->master != NULL || GROUP(vict))
            continue;

        /* Usa a nossa função 'are_groupable' para verificar alinhamento e nível 1-a-1 */
        if (!are_groupable(ch, vict))
            continue;

        potential_members[count++] = vict;
        if (min_level == -1 || GET_LEVEL(vict) < min_level)
            min_level = GET_LEVEL(vict);
        if (max_level == -1 || GET_LEVEL(vict) > max_level)
            max_level = GET_LEVEL(vict);
    }
    potential_members[count] = NULL;

    if (count <= 1)
        return ch; /* Se estiver sozinho, ele pode liderar. */

    /* 2. Se a diferença de nível já for válida, o de nível mais alto lidera. */
    if ((max_level - min_level) <= 15) {
        struct char_data *best_leader = NULL;
        for (int i = 0; i < count; i++) {
            if (best_leader == NULL || GET_LEVEL(potential_members[i]) > GET_LEVEL(best_leader)) {
                best_leader = potential_members[i];
            }
        }
        return best_leader;
    }

    /* 3. Se a diferença for grande, procura o "líder mediano" de nível mais alto. */
    struct char_data *best_median = NULL;
    for (int i = 0; i < count; i++) {
        leader_candidate = potential_members[i];
        if ((max_level - GET_LEVEL(leader_candidate)) <= 15 && (GET_LEVEL(leader_candidate) - min_level) <= 15) {
            if (best_median == NULL || GET_LEVEL(leader_candidate) > GET_LEVEL(best_median)) {
                best_median = leader_candidate;
            }
        }
    }

    return best_median; /* Pode retornar NULL se nenhum grupo for viável */
}

/**
 * A IA principal para um mob tentar formar ou juntar-se a um grupo.
 * Retorna TRUE se uma ação de grupo foi tentada/realizada.
 */
bool mob_handle_grouping(struct char_data *ch)
{
    if (MOB_FLAGGED(ch, MOB_SENTINEL))
        return FALSE;

    if (!ch->ai_data)
        return FALSE;

    /* Don't form groups in death traps or near dangerous areas to reduce cleanup overhead */
    if (IN_ROOM(ch) != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH))
        return FALSE;

    /* Check adjacent rooms for death traps - avoid grouping near danger */
    int i;
    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (EXIT(ch, i) && EXIT(ch, i)->to_room != NOWHERE && ROOM_FLAGGED(EXIT(ch, i)->to_room, ROOM_DEATH)) {
            /* Only skip grouping 50% of the time near death traps to avoid completely killing group formation */
            if (rand_number(1, 100) <= 50)
                return FALSE;
            break;
        }
    }

    /* Verifica a chance de tentar agrupar-se. */
    const int CURIOSIDADE_MINIMA_GRUPO = 5;
    if (rand_number(1, 100) > MAX(GET_GENGROUP(ch), CURIOSIDADE_MINIMA_GRUPO))
        return FALSE;

    struct char_data *vict, *best_target_leader = NULL;
    bool best_is_local = FALSE;
    int max_group_size = 6;

    /* CENÁRIO 1: O mob está num grupo. */
    if (GROUP(ch)) {
        /* Se ele é um líder de um grupo muito pequeno (só ele), ele pode tentar uma fusão. */
        if (GROUP_LEADER(GROUP(ch)) == ch && GROUP(ch)->members->iSize <= 1) {
            struct char_data *vict, *best_target_leader = NULL;
            /* Safety check: Validate room before accessing people list */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return FALSE;

            /* Procura por outros grupos maiores na sala. */
            for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
                if (GROUP(vict) && GROUP_LEADER(GROUP(vict)) == vict && vict != ch) {
                    if (is_level_compatible_with_group(ch, GROUP(vict)) && are_groupable(ch, vict)) {
                        /* Encontrou um grupo maior e compatível. É uma boa opção. */
                        if (best_target_leader == NULL ||
                            GROUP(vict)->members->iSize > GROUP(best_target_leader)->members->iSize) {
                            best_target_leader = vict;
                        }
                    }
                }
            }
            if (best_target_leader) {
                /* Decisão tática de abandonar a própria liderança para se juntar a um grupo mais forte. */
                act("$n avalia o grupo de $N e decide que é mais forte juntar-se a eles.", TRUE, ch, 0,
                    best_target_leader, TO_ROOM);
                leave_group(ch); /* Abandona o seu próprio grupo solitário. */
                join_group(ch, GROUP(best_target_leader));
                return TRUE;
            }
        }
    }
    /* CENÁRIO 2: O mob está sozinho. */
    else {

        /* Safety check: Validate room before accessing people list */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
            return FALSE;

        /* 1. Procura pelo MELHOR grupo existente para se juntar. */
        for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
            if (GROUP(vict) && GROUP_LEADER(GROUP(vict)) == vict && IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN) &&
                GROUP(vict)->members->iSize < max_group_size && is_level_compatible_with_group(ch, GROUP(vict)) &&
                are_groupable(ch, vict)) {

                /* Safety check: Validate vict's room before zone comparison */
                if (IN_ROOM(vict) == NOWHERE || IN_ROOM(vict) < 0 || IN_ROOM(vict) > top_of_world)
                    continue;

                bool is_local = (world[IN_ROOM(ch)].zone == world[IN_ROOM(vict)].zone);

                if (best_target_leader == NULL) {
                    best_target_leader = vict;
                    best_is_local = is_local;
                } else if (is_local && !best_is_local) {
                    best_target_leader = vict;
                    best_is_local = TRUE;
                } else if (is_local == best_is_local && GET_LEVEL(vict) > GET_LEVEL(best_target_leader)) {
                    best_target_leader = vict;
                }
            }
        }

        if (best_target_leader) {
            /* Lógica de Aceitação do Líder */
            int chance_aceitar =
                100 - (GROUP(best_target_leader)->members->iSize * 15) + GET_GENGROUP(best_target_leader);
            if (rand_number(1, 120) <= chance_aceitar) {
                join_group(ch, GROUP(best_target_leader));
                act("$n junta-se ao grupo de $N.", TRUE, ch, 0, best_target_leader, TO_ROOM);
                return TRUE;
            }
        } else {
            /* 2. Não encontrou grupo. Verifica se é possível formar um novo. */
            struct char_data *leader_to_be = find_best_leader_for_new_group(ch);

            if (leader_to_be != NULL && leader_to_be == ch) {
                /* O próprio mob 'ch' é o melhor candidato a líder, então ele cria o grupo. */
                struct group_data *new_group;
                CREATE(new_group, struct group_data, 1);
                add_to_list(new_group, group_list);
                new_group->leader = NULL;
                new_group->members = create_list();
                SET_BIT(new_group->group_flags, GROUP_ANON);
                SET_BIT(new_group->group_flags, GROUP_OPEN);
                SET_BIT(new_group->group_flags, GROUP_NPC); /* Marca como um grupo de mobs */

                join_group(ch, new_group);
                act("$n parece estar a formar um grupo e à procura de companheiros.", TRUE, ch, 0, 0, TO_ROOM);
                return TRUE;
            }
        }
    }
    return FALSE;
}

/**
 * Um membro do grupo avalia o seu inventário completo (incluindo contentores) e
 * partilha o melhor upgrade possível com o companheiro de equipa que mais
 * beneficiaria dele.
 * VERSÃO FINAL COM GESTÃO DE CONTENTORES.
 * Retorna TRUE se uma ação de partilha foi realizada.
 */
bool mob_share_gear_with_group(struct char_data *ch)
{
    /* A IA só age se o mob estiver num grupo, tiver itens e não for encantado. */
    if (!GROUP(ch) || !GROUP(ch)->members || GROUP(ch)->members->iSize == 0 || ch->carrying == NULL || !ch->ai_data ||
        AFF_FLAGGED(ch, AFF_CHARM)) {
        return FALSE;
    }

    /* Chance de executar a avaliação para não sobrecarregar. */
    if (rand_number(1, 100) > 25) {
        return FALSE;
    }

    struct obj_data *item_to_give = NULL;
    struct char_data *receiver = NULL;
    struct obj_data *container_source = NULL; /* De onde o item será tirado */
    int max_improvement_for_group = 10;       /* Só partilha se for uma melhoria significativa */

    struct obj_data *item;
    struct char_data *member;
    struct iterator_data iterator, inner_iterator;

    /* 1. O mob avalia todo o seu inventário para encontrar a melhor oportunidade de partilha. */
    for (item = ch->carrying; item; item = item->next_content) {
        /* Avalia o item atual do inventário principal. */
        member = (struct char_data *)merge_iterator(&iterator, GROUP(ch)->members);
        while (member) {
            if (ch != member) {
                int wear_pos = find_eq_pos(member, item, NULL);
                if (wear_pos != -1) {
                    int improvement =
                        evaluate_item_for_mob(member, item) - evaluate_item_for_mob(member, GET_EQ(member, wear_pos));
                    if (improvement > max_improvement_for_group) {
                        max_improvement_for_group = improvement;
                        item_to_give = item;
                        receiver = member;
                        container_source = NULL;
                    }
                }
            }
            member = (struct char_data *)next_in_list(&iterator);
        }
        remove_iterator(&iterator);

        /* Se o item for um contentor, avalia os itens lá dentro. */
        if (GET_OBJ_TYPE(item) == ITEM_CONTAINER && !OBJVAL_FLAGGED(item, CONT_CLOSED)) {
            struct obj_data *contained_item;
            for (contained_item = item->contains; contained_item; contained_item = contained_item->next_content) {
                member = (struct char_data *)merge_iterator(&inner_iterator, GROUP(ch)->members);
                while (member) {
                    if (ch == member || IN_ROOM(ch) != IN_ROOM(member)) {
                        member = (struct char_data *)next_in_list(&inner_iterator);
                        continue;
                    }
                    int wear_pos = find_eq_pos(member, contained_item, NULL);
                    if (wear_pos != -1) {
                        int improvement = evaluate_item_for_mob(member, contained_item) -
                                          evaluate_item_for_mob(member, GET_EQ(member, wear_pos));
                        if (improvement > max_improvement_for_group) {
                            max_improvement_for_group = improvement;
                            item_to_give = contained_item;
                            receiver = member;
                            container_source = item; /* Lembra-se de que o item está neste contentor. */
                        }
                    }
                    member = (struct char_data *)next_in_list(&inner_iterator);
                }
                remove_iterator(&inner_iterator);
            }
        }
    }

    /* 2. Se, depois de analisar tudo, encontrou uma boa partilha, executa-a. */
    if (item_to_give && receiver) {
        /* Se o item estiver num contentor, tira-o primeiro. */
        if (container_source) {
            obj_from_obj(item_to_give);
            obj_to_char(item_to_give, ch);
            act("$n tira $p de $P.", TRUE, ch, item_to_give, container_source, TO_ROOM);
        }

        /* Agora, o item está no inventário principal. Executa a partilha. */
        perform_give(ch, receiver, item_to_give);

        /* APRENDIZAGEM: Comportamento cooperativo é recompensado. */
        ch->ai_data->genetics.group_tendency += 3;
        ch->ai_data->genetics.group_tendency = MIN(ch->ai_data->genetics.group_tendency, 100);

        return TRUE; /* Ação de partilha foi realizada. */
    }

    return FALSE;
}

/**
 * Função de apoio que executa o movimento, táticas pós-movimento, e aprendizagem.
 * VERSÃO FINAL COM APRENDIZAGEM AMBIENTAL.
 * @param ch O mob que se move.
 * @param dir A direção do movimento.
 * @param should_close_behind TRUE se a IA decidiu que deve fechar a porta.
 * @param was_in A sala de onde o mob veio.
 * @return TRUE se o mob se moveu, FALSE caso contrário.
 */
bool perform_move_IA(struct char_data *ch, int dir, bool should_close_behind, int was_in)
{
    int old_pos = GET_POS(ch);

    if (perform_move(ch, dir, 1)) {
        /* O movimento foi bem-sucedido. */

        /* TÁTICA PÓS-MOVIMENTO: fechar a porta atrás de si (já implementado). */
        if (should_close_behind) {
            int back_door = rev_dir[dir];
            if (EXIT(ch, back_door) && EXIT(ch, back_door)->to_room == was_in && EXIT(ch, back_door)->keyword != NULL &&
                !IS_SET(EXIT(ch, back_door)->exit_info, EX_DNCLOSE)) {
                do_doorcmd(ch, NULL, back_door, SCMD_CLOSE);
            }
        }

        /******************************************************************
         * APRENDIZAGEM PÓS-MOVIMENTO (VERSÃO FINAL E REFINADA)
         ******************************************************************/
        if (ch->ai_data) {
            /* Safety check: Validate room before accessing world array */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return TRUE; /* Movement succeeded but skip learning */

            int roam_change = 0;
            int current_sect = world[IN_ROOM(ch)].sector_type;

            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) || current_sect == SECT_LAVA || current_sect == SECT_QUICKSAND) {
                roam_change = -20; /* Penalidade severa por perigo mortal. */
                act("$n grita de dor ao entrar na armadilha!", FALSE, ch, 0, 0, TO_ROOM);
            } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FROZEN)) {
                roam_change = -10; /* Penalidade média por desconforto. */
                act("$n treme de frio ao entrar na sala gelada.", FALSE, ch, 0, 0, TO_ROOM);
            } else if (current_sect == SECT_ICE && GET_POS(ch) < old_pos) {
                roam_change = -5; /* Penalidade leve por escorregar. */
            } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HEAL)) {
                roam_change = 10; /* Recompensa média por encontrar um santuário. */
                act("$n parece revigorado ao entrar nesta sala.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                roam_change = 1; /* Recompensa normal por exploração segura. */
            }

            ch->ai_data->genetics.roam_tendency += roam_change;
            ch->ai_data->genetics.roam_tendency = MIN(MAX(GET_GENROAM(ch), 0), 100);
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * IA de exploração orientada a objetivos. O mob agora vagueia com um propósito.
 * Se um 'target_room' for fornecido, ele tentará navegar até lá.
 * Se não, ele usa a sua lógica de exploração padrão.
 * VERSÃO FINAL COM NAVEGAÇÃO POR OBJETIVO E PATHFINDING INTELIGENTE.
 * Retorna TRUE se uma ação de roam foi executada.
 */
bool mob_goal_oriented_roam(struct char_data *ch, room_rnum target_room)
{
    static int roam_throttle = 0;

    if (ch->master != NULL || FIGHTING(ch) || GET_POS(ch) < POS_STANDING)
        return FALSE;

    /* Throttle pathfinding calls to prevent resource exhaustion */
    roam_throttle++;
    if (target_room != NOWHERE && roam_throttle % 4 != 0) {
        /* Only do complex pathfinding every 4th call for goal-oriented movement */
        return FALSE;
    }

    int direction = -1;
    bool has_goal = FALSE;

    /* --- FASE 1: DEFINIÇÃO DO OBJETIVO --- */

    /* Se um destino específico foi dado, essa é a prioridade máxima. */
    if (target_room != NOWHERE && IN_ROOM(ch) != target_room) {
        /* Use intelligent pathfinding that compares basic vs advanced methods */
        direction = mob_smart_pathfind(ch, target_room);
        if (direction == -1) {
            /* Fall back to simple pathfinding if smart pathfinding fails */
            direction = find_first_step(IN_ROOM(ch), target_room);

            /* If both pathfinding methods fail, check if it's due to missing keys */
            if (direction == -1 && ch->ai_data && ch->ai_data->current_goal != GOAL_COLLECT_KEY) {
                /* Check if we need a key to reach the target */
                obj_vnum blocking_key = find_blocking_key(ch, IN_ROOM(ch), target_room);
                if (blocking_key != NOTHING) {
                    /* Set key collection goal with current goal as original */
                    if (mob_set_key_collection_goal(ch, blocking_key, ch->ai_data->current_goal, target_room)) {
                        return TRUE; /* Goal changed to key collection */
                    }
                }
            }
        }
        has_goal = TRUE;
    } else {
        /* Se nenhum destino foi dado, usa a lógica de exploração padrão. */
        if (MOB_FLAGGED(ch, MOB_SENTINEL) && IN_ROOM(ch) != real_room(ch->ai_data->guard_post)) {
            /* Use specialized duty pathfinding for sentinels returning to post */
            direction = mob_duty_pathfind(ch, real_room(ch->ai_data->guard_post));
            if (direction == -1) {
                direction = find_first_step(IN_ROOM(ch), real_room(ch->ai_data->guard_post));
            }
            has_goal = TRUE;
        } else {
            int base_roam = MOB_FLAGGED(ch, MOB_SENTINEL) ? 1 : 25;
            int need_bonus = (GET_EQ(ch, WEAR_WIELD) == NULL ? 20 : 0) + (!GROUP(ch) ? 10 : 0);
            int final_chance = MIN(base_roam + GET_GENROAM(ch) + need_bonus, 90);

            if (rand_number(1, 100) <= final_chance) {
                direction = rand_number(0, DIR_COUNT - 1);
                has_goal = TRUE;
            }
        }
    }

    /* --- FASE 2: EXECUÇÃO DA AÇÃO SE HOUVER UM OBJETIVO --- */
    if (has_goal && direction >= 0 && direction < DIR_COUNT) { /* Verificação de segurança para direction */
        struct room_direction_data *exit;
        room_rnum to_room;

        if ((exit = EXIT(ch, direction)) && (to_room = exit->to_room) != NOWHERE && to_room >= 0 &&
            to_room <= top_of_world) {

            /* GESTÃO DE VOO (Ação que consome o turno) */
            if (AFF_FLAGGED(ch, AFF_FLYING) && ROOM_FLAGGED(to_room, ROOM_NO_FLY))
                stop_flying(ch);
            if (!AFF_FLAGGED(ch, AFF_FLYING) && world[to_room].sector_type == SECT_CLIMBING)
                start_flying(ch);

            /* RESOLUÇÃO DE PORTAS (UMA AÇÃO DE CADA VEZ, E APENAS EM PORTAS REAIS) */
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED)) {
                if (!IS_SET(exit->exit_info, EX_DNOPEN)) {
                    if (IS_SET(exit->exit_info, EX_LOCKED) && has_key(ch, exit->key)) {
                        do_doorcmd(ch, NULL, direction, SCMD_UNLOCK);
                    }
                    if (!IS_SET(exit->exit_info, EX_LOCKED)) {
                        do_doorcmd(ch, NULL, direction, SCMD_OPEN);
                    }
                }
            }

            /* Se, depois de tudo, a porta ainda estiver fechada, a IA não pode passar. */
            if (IS_SET(exit->exit_info, EX_CLOSED)) {
                return FALSE;
            }

            /* Verificações Finais de Caminho */
            if ((IS_SET(exit->exit_info, EX_HIDDEN) && rand_number(1, 20) > GET_WIS(ch)) ||
                (world[to_room].sector_type == SECT_UNDERWATER && !has_scuba(ch))) {
                return FALSE;
            }

            /* Safety check: Validate ch's room before zone comparison */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return FALSE;

            if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && (world[to_room].zone != world[IN_ROOM(ch)].zone)) {
                /* For shopping goals, be more willing to cross zones */
                int zone_cross_chance = (ch->ai_data && ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_SELL) ? 25 : 1;
                if (rand_number(1, 100) > zone_cross_chance) {
                    return TRUE; /* Hesitou e gastou o turno. */
                }
            }

            /* Movimento Final */
            int was_in = IN_ROOM(ch);
            bool should_close = (GET_INT(ch) > 12 && IS_SET(exit->exit_info, EX_ISDOOR) &&
                                 !IS_SET(exit->exit_info, EX_DNCLOSE) && rand_number(1, 100) <= 25);
            return perform_move_IA(ch, direction, should_close, was_in);
        }
    }

    return FALSE;
}

/**
 * Gere a rotina de dever para lojistas e sentinelas, agora com um
 * "temporizador de frustração" para evitar que fiquem presos.
 * Retorna TRUE se uma ação de dever foi executada ou se o mob deve ficar parado.
 */
bool handle_duty_routine(struct char_data *ch)
{
    bool is_shopkeeper = (mob_index[GET_MOB_RNUM(ch)].func == shop_keeper);
    bool is_sentinel = MOB_FLAGGED(ch, MOB_SENTINEL);

    if (!is_shopkeeper && !is_sentinel)
        return FALSE; /* Não tem dever, a IA prossegue. */

    /******************************************************************
     * LÓGICA DE FRUSTRAÇÃO
     ******************************************************************/
    /* Se o mob está frustrado, ele "desiste" de voltar para casa por agora. */
    if (ch->ai_data && ch->ai_data->duty_frustration_timer > 0) {
        return FALSE; /* Permite que o mob execute outras IAs (loot, roam, etc.). */
    }

    /******************************************************************
     * SENTINELS COM GOALS DE QUEST TÊM PRIORIDADE TEMPORÁRIA
     ******************************************************************/
    /* Allow sentinels to temporarily abandon their post for quest activities */
    if (is_sentinel && ch->ai_data &&
        (ch->ai_data->current_goal == GOAL_POST_QUEST || ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER ||
         ch->ai_data->current_goal == GOAL_ACCEPT_QUEST || ch->ai_data->current_goal == GOAL_COMPLETE_QUEST)) {
        return FALSE; /* Let quest-related AI take priority over guard duty */
    }

    bool is_on_duty = FALSE;
    room_rnum home_room = NOWHERE;

    if (is_shopkeeper) {
        int shop_nr = find_shop_by_keeper(GET_MOB_RNUM(ch));
        if (shop_nr != -1 && is_shop_open(shop_nr) && shop_index[shop_nr].in_room) {
            is_on_duty = TRUE;
            home_room = real_room(SHOP_ROOM(shop_nr, 0));
        }
    } else { /* É um Sentinela */
        is_on_duty = TRUE;
        if (ch->ai_data)
            home_room = real_room(ch->ai_data->guard_post);
    }

    if (is_on_duty) {
        /* Verifica se o mob já está no seu posto. */
        if ((is_shopkeeper && ok_shop_room(find_shop_by_keeper(GET_MOB_RNUM(ch)), GET_ROOM_VNUM(IN_ROOM(ch)))) ||
            (is_sentinel && IN_ROOM(ch) == home_room)) {
            return TRUE; /* Está no posto, não faz mais nada. Fim do turno. */
        }

        /* Se não está no posto, tenta voltar usando pathfinding inteligente. */
        if (home_room != NOWHERE) {
            static int duty_pathfind_calls = 0;
            int direction = -1;

            /* Use specialized duty pathfinding with priority caching */
            direction = mob_duty_pathfind(ch, home_room);

            /* Fallback to throttled pathfinding if duty pathfinding fails */
            if (direction == -1) {
                duty_pathfind_calls++;
                if (duty_pathfind_calls % 5 == 0) {
                    /* Only do complex pathfinding every 5th call for duty movement */
                    direction = mob_smart_pathfind(ch, home_room);
                } else {
                    /* Use simple pathfinding most of the time */
                    direction = find_first_step(IN_ROOM(ch), home_room);
                }
            }

            if (direction == -1) {
                /* Fall back to basic pathfinding if duty pathfinding fails */
                direction = find_first_step(IN_ROOM(ch), home_room);

                /* If both pathfinding methods fail, check if it's due to missing keys */
                if (direction == -1 && ch->ai_data && ch->ai_data->current_goal != GOAL_COLLECT_KEY) {
                    obj_vnum blocking_key = find_blocking_key(ch, IN_ROOM(ch), home_room);
                    if (blocking_key != NOTHING) {
                        /* Set key collection goal with return to post as original */
                        if (mob_set_key_collection_goal(ch, blocking_key, GOAL_RETURN_TO_POST, home_room)) {
                            return TRUE; /* Goal changed to key collection */
                        }
                    }
                }
            }

            if (direction >= 0) {
                perform_move(ch, direction, 1);
            } else {
                /******************************************************************
                 * O CAMINHO FALHOU! O MOB FICA FRUSTRADO.
                 *******************************************************************/
                if (ch->ai_data) {
                    ch->ai_data->duty_frustration_timer = 6; /* Fica frustrado por 6 pulsos de IA. */
                }
                return FALSE; /* Permite que outras IAs sejam executadas. */
            }
            return TRUE; /* Tentou voltar, consome o turno. */
        }
    }

    /* Se não está de serviço, está de "folga". */
    return FALSE;
}

/**
 * Lógica para um membro de grupo seguir o seu líder se estiverem em salas diferentes.
 * VERSÃO REFINADA: Implementa a "Hierarquia de Comando".
 * @param ch O mob a executar a ação.
 * @return TRUE se o mob tentou mover-se, consumindo o seu turno de IA.
 */
bool mob_follow_leader(struct char_data *ch)
{
    /* A função só se aplica a membros de um grupo (que não são o líder). */
    if (!GROUP(ch) || GROUP_LEADER(GROUP(ch)) == ch)
        return FALSE;

    struct char_data *leader = GROUP_LEADER(GROUP(ch));

    /* Verifica se o líder é válido e se está numa sala diferente. */
    if (leader != NULL && IN_ROOM(ch) != IN_ROOM(leader)) {

        /******************************************************************/
        /* REFINAMENTO: A IA agora entende a "Hierarquia de Comando".     */
        /******************************************************************/
        bool duty_is_overridden_by_player = FALSE;

        /* Se o mob está encantado, o seu dever é seguir o mestre. */
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master == leader) {
            duty_is_overridden_by_player = TRUE;
        }
        /* Se o líder é um jogador (o grupo não tem a flag NPC), o dever é seguir o líder. */
        if (!IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_NPC)) {
            duty_is_overridden_by_player = TRUE;
        }

        /* Um Sentinela só pode ser "teimoso" se não estiver sob ordens diretas de um jogador. */
        if (MOB_FLAGGED(ch, MOB_SENTINEL) && !duty_is_overridden_by_player && rand_number(1, 100) > 2) {
            return FALSE; /* Ficou no posto, leal ao seu dever original. */
        }

        /* Tenta encontrar o caminho até ao líder usando pathfinding inteligente. */
        static int follow_pathfind_calls = 0;
        int direction = -1;

        /* Throttle follow pathfinding to reduce resource usage */
        follow_pathfind_calls++;
        if (follow_pathfind_calls % 6 == 0) {
            /* Only do complex pathfinding every 6th call for following */
            direction = mob_smart_pathfind(ch, IN_ROOM(leader));
        } else {
            /* Use simple pathfinding most of the time for following */
            direction = find_first_step(IN_ROOM(ch), IN_ROOM(leader));
        }

        if (direction == -1) {
            /* Fall back to basic pathfinding if smart pathfinding fails */
            direction = find_first_step(IN_ROOM(ch), IN_ROOM(leader));

            /* If both pathfinding methods fail, check if it's due to missing keys */
            if (direction == -1 && ch->ai_data && ch->ai_data->current_goal != GOAL_COLLECT_KEY) {
                obj_vnum blocking_key = find_blocking_key(ch, IN_ROOM(ch), IN_ROOM(leader));
                if (blocking_key != NOTHING) {
                    /* Don't set a formal goal for following, just fail gracefully */
                    /* Following is a lower priority activity */
                    return FALSE;
                }
            }
        }

        if (direction >= 0) {
            room_rnum to_room;
            if (EXIT(ch, direction) && (to_room = EXIT(ch, direction)->to_room) <= top_of_world) {

                /* Regra especial para salas NOMOB. */
                if (ROOM_FLAGGED(to_room, ROOM_NOMOB)) {
                    /* Só pode entrar se o líder for um jogador (o que já é verificado acima). */
                    if (IS_NPC(leader)) {
                        return FALSE;
                    }
                }

                perform_move(ch, direction, 1);
                return TRUE; /* Ação de seguir foi executada. */
            }
        }
    }

    return FALSE; /* Nenhuma ação de seguir foi executada. */
}

/**
 * Tenta fazer o mob seguir outro personagem (player ou mob) sem formar grupo.
 * Usado para comportamento furtivo/evil (espionagem, roubo, etc).
 * Retorna TRUE se o mob começou ou continuou a seguir alguém.
 */
bool mob_try_stealth_follow(struct char_data *ch)
{
    /* Safety check: Validate room */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    /* Só funciona para mobs com ai_data e que não estão em combate */
    if (!ch->ai_data || FIGHTING(ch))
        return FALSE;

    /* Não segue se já está em grupo ou encantado */
    if (GROUP(ch) || AFF_FLAGGED(ch, AFF_CHARM))
        return FALSE;

    /* Verifica a tendência de seguir */
    int follow_tendency = GET_GENFOLLOW(ch);

    /* Mobs sentinelas não devem seguir (ficam no posto) */
    if (MOB_FLAGGED(ch, MOB_SENTINEL) && rand_number(1, 100) > 2)
        return FALSE;

    /* Se já está seguindo alguém, verifica se deve continuar */
    if (ch->master) {
        /* Safety check: Validate master pointer and room */
        if (!ch->master || IN_ROOM(ch->master) == NOWHERE || IN_ROOM(ch->master) < 0 ||
            IN_ROOM(ch->master) > top_of_world) {
            /* Master is invalid or in invalid room, stop following */
            stop_follower(ch);
            return FALSE;
        }

        /* Check if mob can still see the target */
        if (!CAN_SEE(ch, ch->master)) {
            /* Lost visibility of target, stop following so we can try a new target later */
            /* Small decrease - visibility loss can be temporary/environmental */
            ch->ai_data->genetics.follow_tendency -= 1;
            ch->ai_data->genetics.follow_tendency = MAX(ch->ai_data->genetics.follow_tendency, 0);
            stop_follower(ch);
            return FALSE;
        }

        /* Se o alvo saiu da sala, segue ele */
        if (IN_ROOM(ch) != IN_ROOM(ch->master)) {
            int direction = find_first_step(IN_ROOM(ch), IN_ROOM(ch->master));
            if (direction >= 0 && direction < DIR_COUNT) {
                room_rnum to_room;
                /* Safety check: Validate exit before accessing */
                if (EXIT(ch, direction) && (to_room = EXIT(ch, direction)->to_room) != NOWHERE && to_room >= 0 &&
                    to_room <= top_of_world) {
                    /* Resolve portas se necessário */
                    if (IS_SET(EXIT(ch, direction)->exit_info, EX_ISDOOR) &&
                        IS_SET(EXIT(ch, direction)->exit_info, EX_CLOSED)) {
                        if (!IS_SET(EXIT(ch, direction)->exit_info, EX_DNOPEN)) {
                            if (IS_SET(EXIT(ch, direction)->exit_info, EX_LOCKED) &&
                                has_key(ch, EXIT(ch, direction)->key)) {
                                do_doorcmd(ch, NULL, direction, SCMD_UNLOCK);
                            }
                            if (!IS_SET(EXIT(ch, direction)->exit_info, EX_LOCKED)) {
                                do_doorcmd(ch, NULL, direction, SCMD_OPEN);
                            }
                        }
                    }

                    /* Tenta mover-se discretamente */
                    if (!IS_SET(EXIT(ch, direction)->exit_info, EX_CLOSED)) {
                        perform_move(ch, direction, 1);
                        /* Reward for successfully following/moving with target */
                        ch->ai_data->genetics.follow_tendency += 1;
                        ch->ai_data->genetics.follow_tendency = MIN(ch->ai_data->genetics.follow_tendency, 100);
                        return TRUE;
                    }
                } else {
                    /* Cannot find valid path, stop following */
                    /* Moderate decrease for path failure */
                    ch->ai_data->genetics.follow_tendency -= 2;
                    ch->ai_data->genetics.follow_tendency = MAX(ch->ai_data->genetics.follow_tendency, 0);
                    stop_follower(ch);
                    return FALSE;
                }
            } else {
                /* Cannot find path to target, stop following */
                /* Moderate decrease for path failure */
                ch->ai_data->genetics.follow_tendency -= 2;
                ch->ai_data->genetics.follow_tendency = MAX(ch->ai_data->genetics.follow_tendency, 0);
                stop_follower(ch);
                return FALSE;
            }
        }
        /* Já está na mesma sala que o alvo, mantém a observação */
        return TRUE;
    }

    /* Não está seguindo ninguém, decide se deve começar */
    /* Chance base ajustada pela tendência genética */
    if (rand_number(1, 100) > follow_tendency)
        return FALSE;

    /* Procura um alvo adequado para seguir na sala */
    struct char_data *target = NULL;
    struct char_data *vict;
    int best_score = 0;

    /* Safety check: Validate room before iterating people list */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
        /* Safety check: Validate vict pointer */
        if (!vict || vict == ch)
            continue;

        /* Only follow targets that are visible to the mob */
        if (!CAN_SEE(ch, vict))
            continue;

        /* Safety check: Validate vict's room */
        if (IN_ROOM(vict) == NOWHERE || IN_ROOM(vict) < 0 || IN_ROOM(vict) > top_of_world)
            continue;

        /* Preferência por players, especialmente se o mob for evil */
        int score = 0;

        if (!IS_NPC(vict)) {
            score += 50; /* Players são alvos mais interessantes */

            /* Mobs evil têm maior interesse em seguir players good */
            if (GET_ALIGNMENT(ch) < -200 && IS_GOOD(vict))
                score += 30;

            /* Mobs com alto nível de roaming também seguem mais */
            score += GET_GENROAM(ch) / 5;
        } else {
            /* Outros mobs podem ser seguidos também */
            score += 20;

            /* Prefere mobs com itens valiosos ou de nível maior */
            if (GET_LEVEL(vict) > GET_LEVEL(ch))
                score += 10;
        }

        /* Evita seguir mobs agressivos ou em combate */
        if (FIGHTING(vict) || (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_AGGRESSIVE)))
            score -= 50;

        if (score > best_score) {
            best_score = score;
            target = vict;
        }
    }

    /* Se encontrou um bom alvo, começa a seguir */
    if (target && best_score > 30) {
        /* Verifica se não criaria um loop circular de seguimento */
        if (circle_follow(ch, target)) {
            return FALSE;
        }

        /* Stealth-aware following: Check if mob has sneak/hide affects */
        bool is_stealthy = AFF_FLAGGED(ch, AFF_SNEAK) || AFF_FLAGGED(ch, AFF_HIDE);

        /* Manually set up following relationship (like add_follower but with conditional messages) */
        if (!ch->master) {
            struct follow_type *k;
            ch->master = target;
            CREATE(k, struct follow_type, 1);
            k->follower = ch;
            k->next = target->followers;
            target->followers = k;

            /* Messages are conditional based on stealth */
            if (!is_stealthy) {
                /* Normal visible following */
                if (IS_NPC(ch)) {
                    /* NPCs don't get the "You will now follow" message */
                } else {
                    act("Você agora irá seguir $N.", FALSE, ch, 0, target, TO_CHAR);
                }
                if (CAN_SEE(target, ch))
                    act("$n começa a seguir você.", TRUE, ch, 0, target, TO_VICT);
                act("$n começa a seguir $N.", TRUE, ch, 0, target, TO_NOTVICT);
            } else {
                /* Stealthy following - only notify if target can see through stealth */
                if (CAN_SEE(target, ch)) {
                    /* Target can see through the stealth */
                    act("$n começa a seguir você silenciosamente.", TRUE, ch, 0, target, TO_VICT);
                }
                /* No message to room when sneaking/hiding */
            }
        }

        /* Aumenta levemente a tendência de seguir se teve sucesso (menor peso que a frustração) */
        ch->ai_data->genetics.follow_tendency += 1;
        ch->ai_data->genetics.follow_tendency = MIN(ch->ai_data->genetics.follow_tendency, 100);
        return TRUE;
    }

    /* Se não encontrou ninguém para seguir, pequena chance de esquecer/reduzir interesse */
    if (rand_number(1, 100) <= 2) { /* 2% chance por tick de reduzir naturalmente */
        ch->ai_data->genetics.follow_tendency -= 1;
        ch->ai_data->genetics.follow_tendency = MAX(ch->ai_data->genetics.follow_tendency, 0);
    }

    return FALSE;
}

/**
 * A IA principal para um mob ocioso decidir ajudar aliados em combate.
 * Consolida as lógicas de grupo, charmed e helper.
 * Retorna TRUE se o mob entrou em combate.
 */
bool mob_assist_allies(struct char_data *ch)
{
    struct char_data *ally_in_trouble = NULL;
    struct char_data *target_to_attack = NULL;
    int max_threat_level = 0;

    /* A IA só age se o mob estiver ocioso e puder ver. */
    if (FIGHTING(ch) || AFF_FLAGGED(ch, AFF_BLIND)) {
        return FALSE;
    }

    /* PRIORIDADE 1: Ajudar o Mestre (se estiver encantado) */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && FIGHTING(ch->master)) {
        if (IN_ROOM(ch) == IN_ROOM(ch->master)) {
            ally_in_trouble = ch->master;
            target_to_attack = FIGHTING(ch->master);
        }
    }

    /* PRIORIDADE 2: Ajudar o Grupo (se não tiver um mestre para ajudar) */
    else if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize) {
        struct char_data *member;
        struct iterator_data iterator;

        member = (struct char_data *)merge_iterator(&iterator, GROUP(ch)->members);
        while (member) {
            if (ch != member && IN_ROOM(ch) == IN_ROOM(member) && FIGHTING(member)) {
                /* Lógica de "priorizar a maior ameaça" */
                if (GET_LEVEL(FIGHTING(member)) > max_threat_level) {
                    max_threat_level = GET_LEVEL(FIGHTING(member));
                    ally_in_trouble = member;
                    target_to_attack = FIGHTING(member);
                }
            }
            member = (struct char_data *)next_in_list(&iterator);
        }
        remove_iterator(&iterator);
    }

    /* PRIORIDADE 3: Ajudar outros NPCs (se tiver a flag MOB_HELPER) */
    else if (MOB_FLAGGED(ch, MOB_HELPER)) {
        /* Safety check: Validate room before accessing people list */
        if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
            struct char_data *vict;
            for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
                if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
                    continue;
                if (IS_NPC(FIGHTING(vict))) /* Não ajuda mobs que lutam contra outros mobs */
                    continue;

                ally_in_trouble = vict;
                target_to_attack = FIGHTING(vict);
                break; /* Ajuda o primeiro que encontrar. */
            }
        }
    }

    /* Se encontrou alguém para ajudar, entra em combate. */
    if (ally_in_trouble && target_to_attack) {
        act("$n vê que $N está em apuros e corre para ajudar!", FALSE, ch, 0, ally_in_trouble, TO_NOTVICT);
        hit(ch, target_to_attack, TYPE_UNDEFINED);
        /* Safety check: hit() can indirectly cause extract_char */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            return TRUE;
        return TRUE;
    }

    return FALSE;
}

/**
 * A IA tenta saquear o melhor item da sala, com base nas suas necessidades
 * e na sua genética. A sua "vontade" de procurar é aumentada se tiver
 * uma necessidade urgente (ex: sem arma, com vida baixa).
 * Retorna TRUE se uma ação de saque foi bem-sucedida.
 */
bool mob_try_and_loot(struct char_data *ch)
{
    /* Safety check: Validate room before accessing world array */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    /* A IA só age se houver itens na sala, se o mob tiver genética e não estiver em combate. */
    if (!world[IN_ROOM(ch)].contents || !ch->ai_data || FIGHTING(ch))
        return FALSE;

    /* 1. Calcula a tendência base do mob (genética + instinto). */
    int base_tendency = MOB_FLAGGED(ch, MOB_SCAVENGER) ? 25 : 0;
    int genetic_tendency = GET_GENLOOT(ch);
    int effective_tendency = base_tendency + genetic_tendency;

    /* 2. Calcula o bónus com base nas necessidades urgentes. */
    int need_bonus = 0;
    if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.8)
        need_bonus += 15; /* Precisa de itens de cura. */

    if (GET_EQ(ch, WEAR_WIELD) == NULL)
        need_bonus += 25; /* Precisa desesperadamente de uma arma. */

    if (GET_EQ(ch, WEAR_BODY) == NULL)
        need_bonus += 15; /* Precisa de armadura. */

    if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_FIREWEAPON && !mob_has_ammo(ch))
        need_bonus += 50; /* Necessidade MUITO ALTA por munição! */

    /* 3. A chance final é a combinação de tudo, com um mínimo de curiosidade. */
    int final_chance = MAX(effective_tendency + need_bonus, 10);
    final_chance = MIN(final_chance, 95); /* Limita a 95% para não ser garantido. */

    if (rand_number(1, 100) <= final_chance) {
        int max_score = 0;
        struct obj_data *obj, *best_obj = NULL;

        /* A IA agora procura ativamente pelo item que melhor satisfaz a sua necessidade. */
        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
            int current_score = evaluate_item_for_mob(ch, obj);
            if (current_score > max_score) {
                best_obj = obj;
                max_score = current_score;
            }
        }

        if (best_obj != NULL) {
            /* Chama a função do jogo para pegar o item, garantindo todas as verificações. */
            if (perform_get_from_room(ch, best_obj)) {
                /* Aprendizagem Positiva: A decisão foi boa e bem-sucedida. */
                ch->ai_data->genetics.loot_tendency += 2;
                ch->ai_data->genetics.loot_tendency = MIN(ch->ai_data->genetics.loot_tendency, 100);
                return TRUE; /* Ação bem-sucedida, consome o turno. */
            }
            //} else {
            /* Aprendizagem Negativa: A necessidade não foi satisfeita. */
            //    ch->ai_data->genetics.loot_tendency -= 1;
            //    ch->ai_data->genetics.loot_tendency = MAX(ch->ai_data->genetics.loot_tendency, 0);
        }
    }

    return FALSE; /* Nenhuma ação de saque foi executada. */
}

/**
 * A IA entra numa "sessão de equipamento", onde percorre o seu inventário
 * (incluindo contentores) várias vezes para encontrar e vestir todos os upgrades possíveis.
 * VERSÃO FINAL COMPLETA.
 * Retorna TRUE se conseguiu equipar pelo menos um item.
 */
bool mob_try_and_upgrade(struct char_data *ch)
{
    if (!ch->ai_data || AFF_FLAGGED(ch, AFF_CHARM))
        return FALSE;

    /* A chance de sequer pensar em se equipar é baseada na genética. */
    if (rand_number(1, 100) > MIN(MAX(GET_GENEQUIP(ch), 5), 90))
        return FALSE;

    bool performed_an_upgrade_this_pulse = FALSE;
    bool keep_trying = TRUE;
    int max_iterations = 10; /* Prevent infinite loops by limiting iterations */
    int iteration_count = 0;

    /* O loop 'while' garante que o mob continua a tentar equipar até estar otimizado. */
    while (keep_trying && iteration_count < max_iterations) {
        iteration_count++;

        /* Pede à nossa função de busca para encontrar o melhor plano de upgrade. */
        struct mob_upgrade_plan plan = find_best_upgrade_for_mob(ch);

        /* Se encontrou um upgrade viável (melhoria > 0), executa-o. */
        if (plan.item_to_equip && plan.improvement_score > 0) {

            /* PASSO 1: Se o item estiver num contentor, tira-o primeiro. */
            if (plan.container) {
                obj_from_obj(plan.item_to_equip);
                obj_to_char(plan.item_to_equip, ch);
            }

            /* PASSO 2: Remove o item antigo que está no slot. */
            struct obj_data *equipped_item = GET_EQ(ch, plan.wear_pos);
            if (equipped_item) {
                perform_remove(ch, plan.wear_pos);
            }

            /* PASSO 3: Equipa o novo item no slot agora vazio. */
            perform_wear(ch, plan.item_to_equip, plan.wear_pos);

            performed_an_upgrade_this_pulse = TRUE;
            /* keep_trying continua TRUE para que ele re-avalie o inventário. */

        } else {
            /* Se não encontrou mais upgrades viáveis, para a sessão. */
            keep_trying = FALSE;
        }
    } /* Fim do loop 'while' */

    /* Log if we hit the iteration limit to help debug infinite loops */
    if (iteration_count >= max_iterations) {
        log1("SYSERR: mob_try_and_upgrade hit iteration limit for mob %s (vnum %d)", GET_NAME(ch), GET_MOB_VNUM(ch));
    }

    /* A aprendizagem acontece uma vez no final da sessão. */
    if (performed_an_upgrade_this_pulse) {
        ch->ai_data->genetics.equip_tendency = MIN(ch->ai_data->genetics.equip_tendency + 2, 100);
        /* Se mob não se equipar, fica frustrado demais e a tendencia cai a 0. Devemos evitar isso.
         }
         else {
            ch->ai_data->genetics.equip_tendency = MAX(ch->ai_data->genetics.equip_tendency - 1, 0);
        */
    }

    /* Retorna TRUE se a IA "pensou" em se equipar, para consumir o seu foco neste pulso. */
    return TRUE;
}

/**
 * A IA de gestão de inventário. O mob tenta organizar os seus itens,
 * guardando-os no melhor contentoir que possui.
 * Retorna TRUE se uma ação de organização foi realizada.
 */
bool mob_manage_inventory(struct char_data *ch)
{
    /* A IA só age se tiver genética e se o inventário estiver a ficar cheio. */
    if (!ch->ai_data || !ch->carrying || IS_CARRYING_N(ch) < (CAN_CARRY_N(ch) * 0.8))
        return FALSE;

    /* Check if mob should sacrifice corpses in the room first */
    if (rand_number(1, 100) <= 15) { /* 15% chance to check for corpses */
        if (mob_try_sacrifice(ch, NULL)) {
            return TRUE; /* Mob sacrificed something, that's enough for this pulse */
        }
    }

    /* Check if inventory is extremely full - may need to donate or junk items */
    bool inventory_overfull = (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) * 0.95);
    bool inventory_heavy = (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch) * 0.95);

    if (inventory_overfull || inventory_heavy) {
        /* Try to donate valuable items if mob has good alignment */
        if (GET_ALIGNMENT(ch) > 0 && rand_number(1, 100) <= 20) {
            if (mob_try_donate(ch, NULL)) {
                return TRUE;
            }
        }

        /* Try to junk worthless items if desperate */
        if (rand_number(1, 100) <= 30) {
            if (mob_try_junk(ch, NULL)) {
                return TRUE;
            }
        }

        /* Last resort - drop something */
        if (rand_number(1, 100) <= 40) {
            if (mob_try_drop(ch, NULL)) {
                return TRUE;
            }
        }
    }

    /* A "vontade" de se organizar é baseada no use_tendency. */
    if (rand_number(1, 100) > GET_GENUSE(ch)) /* GET_GENUSE será a nova macro */
        return FALSE;

    struct obj_data *obj, *container = NULL, *best_container = NULL;
    int max_capacity = 0;

    /* 1. Encontra o melhor contentor aberto no inventário. */
    for (container = ch->carrying; container; container = container->next_content) {
        if (GET_OBJ_TYPE(container) == ITEM_CONTAINER && !OBJVAL_FLAGGED(container, CONT_CLOSED)) {
            if (GET_OBJ_VAL(container, 0) > max_capacity) {
                max_capacity = GET_OBJ_VAL(container, 0);
                best_container = container;
            }
        }
    }

    /* Se não encontrou um contentor utilizável, não pode fazer nada. */
    if (!best_container)
        return FALSE;

    bool item_stored = FALSE;

    /* 2. Percorre o inventário novamente para guardar os itens. */
    struct obj_data *next_obj_to_store;
    for (obj = ch->carrying; obj; obj = next_obj_to_store) {
        next_obj_to_store = obj->next_content;

        /* Não guarda o próprio contentor, nem itens amaldiçoados, nem outros contentores. */
        if (obj == best_container || OBJ_FLAGGED(obj, ITEM_NODROP) || GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
            continue;

        /* Verifica se o item cabe no contentor (lógica de perform_put). */
        if ((GET_OBJ_WEIGHT(best_container) + GET_OBJ_WEIGHT(obj)) <= GET_OBJ_VAL(best_container, 0)) {
            perform_put(ch, obj, best_container);
            item_stored = TRUE;
        }
    }

    /* A IA aprende que organizar é útil. */
    if (item_stored && ch->ai_data) {
        ch->ai_data->genetics.use_tendency += 3;
        ch->ai_data->genetics.use_tendency = MIN(ch->ai_data->genetics.use_tendency, 100);
    }

    return item_stored;
}

/**
 * Procura em todo o inventário de um mob (incluindo contentores) pelo
 * melhor upgrade de equipamento possível.
 * @param ch O mob que está a procurar.
 * @return Uma estrutura mob_upgrade_plan com o plano para o melhor upgrade.
 */
struct mob_upgrade_plan find_best_upgrade_for_mob(struct char_data *ch)
{
    struct mob_upgrade_plan best_plan = {NULL, NULL, -1, 0};
    struct obj_data *item, *contained_item, *equipped_item;
    int wear_pos, score;

    /* Loop através de todos os itens (no inventário principal e dentro de contentores) */
    for (item = ch->carrying; item; item = item->next_content) {
        /* Primeiro, avalia o item atual do inventário principal. */
        wear_pos = find_eq_pos(ch, item, NULL);
        if (wear_pos != -1) {
            equipped_item = GET_EQ(ch, wear_pos);
            /* Verifica se a troca é possível antes de avaliar. */
            if (equipped_item == NULL ||
                (!OBJ_FLAGGED(equipped_item, ITEM_NODROP) && IS_CARRYING_N(ch) < CAN_CARRY_N(ch))) {
                score = evaluate_item_for_mob(ch, item) - evaluate_item_for_mob(ch, equipped_item);
                if (score > best_plan.improvement_score) {
                    best_plan.improvement_score = score;
                    best_plan.item_to_equip = item;
                    best_plan.wear_pos = wear_pos;
                    best_plan.container = NULL; /* Item está no inventário principal. */
                }
            }
        }

        /* Se o item for um contentor aberto, procura dentro dele. */
        if (GET_OBJ_TYPE(item) == ITEM_CONTAINER && !OBJVAL_FLAGGED(item, CONT_CLOSED)) {
            for (contained_item = item->contains; contained_item; contained_item = contained_item->next_content) {
                wear_pos = find_eq_pos(ch, contained_item, NULL);
                if (wear_pos != -1) {
                    equipped_item = GET_EQ(ch, wear_pos);
                    /* Ao tirar um item, o número de itens não aumenta, por isso a verificação de CAN_CARRY_N não é
                     * necessária aqui. */
                    if (equipped_item == NULL || !OBJ_FLAGGED(equipped_item, ITEM_NODROP)) {
                        score = evaluate_item_for_mob(ch, contained_item) - evaluate_item_for_mob(ch, equipped_item);
                        if (score > best_plan.improvement_score) {
                            best_plan.improvement_score = score;
                            best_plan.item_to_equip = contained_item;
                            best_plan.wear_pos = wear_pos;
                            best_plan.container = item; /* Lembra-se de onde o item está! */
                        }
                    }
                }
            }
        }
    }
    return best_plan;
}

/**
 * Procura no inventário de um personagem por um item amaldiçoado
 * (que tenha a flag ITEM_NODROP).
 * @param ch O personagem cujo inventário será verificado.
 * @return Um ponteiro para o primeiro item amaldiçoado encontrado, ou NULL se não houver nenhum.
 */
struct obj_data *find_cursed_item_in_inventory(struct char_data *ch)
{
    struct obj_data *obj;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
            return obj;
        }
    }
    return NULL;
}

/**
 * Procura no inventário de um personagem por uma arma ou armadura que ainda
 * não tenha sido abençoada (sem a flag ITEM_BLESS).
 * @param ch O personagem cujo inventário será verificado.
 * @return Um ponteiro para o primeiro item válido encontrado, ou NULL se não houver.
 */
struct obj_data *find_unblessed_weapon_or_armor(struct char_data *ch)
{
    struct obj_data *obj;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON || GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
            if (!OBJ_FLAGGED(obj, ITEM_BLESS)) {
                return obj;
            }
        }
    }
    return NULL;
}

/*
 * staff  - [0] level   [1] max charges [2] num charges [3] spell num
 * wand   - [0] level   [1] max charges [2] num charges [3] spell num        * scroll - [0] level   [1] spell num   [2]
 * spell num   [3] spell num        * potion - [0] level   [1] spell num   [2] spell num   [3] spell num        * Staves
 * and wands will default to level 14 if the level is not specified;*/

/**
 * Retorna todos os números de magia contidos em um item mágico.
 * @param obj O objeto a ser verificado.
 * @param spells Array para armazenar os números das magias (deve ter pelo menos 3 elementos).
 * @return O número de magias válidas encontradas, ou 0 se o item não for mágico.
 */
int get_all_spells_from_item(struct obj_data *obj, int *spells)
{
    if (!obj || !spells)
        return 0;

    int count = 0;

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WAND:
        case ITEM_STAFF:
            // Wands and staves only have one spell in slot 3
            if (GET_OBJ_VAL(obj, 3) > 0) {
                spells[count++] = GET_OBJ_VAL(obj, 3);
            }
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
            // Scrolls and potions can have spells in slots 1, 2, and 3
            for (int slot = 1; slot <= 3; slot++) {
                if (GET_OBJ_VAL(obj, slot) > 0) {
                    spells[count++] = GET_OBJ_VAL(obj, slot);
                }
            }
            break;

        default:
            break;
    }

    return count;
}

/**
 * Retorna o número da magia contida em um item mágico (wand, staff, scroll, potion).
 * Para scrolls e potions, retorna a primeira magia válida encontrada.
 * @param obj O objeto a ser verificado.
 * @return O número da magia, ou -1 se o item não for mágico.
 */
int get_spell_from_item(struct obj_data *obj)
{
    if (!obj)
        return -1;

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WAND:
        case ITEM_STAFF:
            return GET_OBJ_VAL(obj, 3);

        case ITEM_SCROLL:
        case ITEM_POTION:
            // For scrolls and potions, return the first valid spell found
            for (int slot = 1; slot <= 3; slot++) {
                if (GET_OBJ_VAL(obj, slot) > 0) {
                    return GET_OBJ_VAL(obj, slot);
                }
            }
            return -1;

        default:
            return -1;
    }
}

/**
 * A IA principal para um mob decidir, usar e aprender com itens.
 * VERSÃO FINAL COM LÓGICA DE SUPORTE E PREPARAÇÃO DE ITENS.
 * Retorna TRUE se uma ação foi executada.
 */
bool mob_handle_item_usage(struct char_data *ch)
{
    if (!ch->carrying || !ch->ai_data)
        return FALSE;
    if (rand_number(1, 100) > MAX(GET_GENUSE(ch), 15))
        return FALSE;

    struct obj_data *obj, *item_to_use = NULL, *target_obj = NULL, *best_target_obj = NULL;
    struct char_data *target_char = NULL, *best_target_char = NULL;
    int best_score = 0;
    int spellnum_to_cast = -1;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        int spells[3];   // Maximum 3 spells for scrolls/potions
        int spell_count = get_all_spells_from_item(obj, spells);

        if (spell_count <= 0)
            continue;

        // Evaluate each spell in the item
        for (int spell_idx = 0; spell_idx < spell_count; spell_idx++) {
            int current_score = 0;
            target_char = NULL;   // Reset targets for each iteration
            target_obj = NULL;
            int skillnum = spells[spell_idx];

            struct str_spells *spell = get_spell_by_vnum(skillnum);
            if (!spell)
                continue;

            /* --- Início da Árvore de Decisão Tática Expandida --- */
            if (FIGHTING(ch)) {
                /* ** MODO DE COMBATE ** */

                // PRIORIDADE 1: EMERGÊNCIA - Cura crítica
                if (IS_SET(spell->mag_flags, MAG_POINTS) && GET_HIT(ch) < (GET_MAX_HIT(ch) * 0.3)) {
                    current_score = 200;
                    target_char = ch;
                }
                // PRIORIDADE 2: Cura moderada
                else if (IS_SET(spell->mag_flags, MAG_POINTS) && GET_HIT(ch) < (GET_MAX_HIT(ch) * 0.7)) {
                    current_score = 150;
                    target_char = ch;
                }

                // PRIORIDADE 3: Proteções específicas de alinhamento
                else if (skillnum == SPELL_PROT_FROM_EVIL && IS_GOOD(ch) && IS_EVIL(FIGHTING(ch)) &&
                         !IS_AFFECTED(ch, AFF_PROTECT_EVIL)) {
                    current_score = 140;
                    target_char = ch;
                } else if (skillnum == SPELL_PROT_FROM_GOOD && IS_EVIL(ch) && IS_GOOD(FIGHTING(ch)) &&
                           !IS_AFFECTED(ch, AFF_PROTECT_GOOD)) {
                    current_score = 140;
                    target_char = ch;
                }

                // PRIORIDADE 4: Proteções defensivas supremas
                else if (skillnum == SPELL_SANCTUARY && !IS_AFFECTED(ch, AFF_SANCTUARY)) {
                    current_score = 130;
                    target_char = ch;
                } else if (skillnum == SPELL_GLOOMSHIELD && !IS_AFFECTED(ch, AFF_GLOOMSHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_STONESKIN && !IS_AFFECTED(ch, AFF_STONESKIN)) {
                    current_score = 120;
                    target_char = ch;
                }

                // PRIORIDADE 5: Escudos de dano
                else if (skillnum == SPELL_FIRESHIELD && !IS_AFFECTED(ch, AFF_FIRESHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_THISTLECOAT && !IS_AFFECTED(ch, AFF_THISTLECOAT)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_WINDWALL && !IS_AFFECTED(ch, AFF_WINDWALL)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_SOUNDBARRIER && !IS_AFFECTED(ch, AFF_SOUNDBARRIER)) {
                    current_score = 110;
                    target_char = ch;
                }

                // PRIORIDADE 6: Buffs de combate
                else if (skillnum == SPELL_STRENGTH && !IS_AFFECTED(ch, skillnum)) {
                    current_score = 105;
                    target_char = ch;
                } else if (skillnum == SPELL_ARMOR && !IS_AFFECTED(ch, skillnum)) {
                    current_score = 100;
                    target_char = ch;
                }

                // PRIORIDADE 7: Debuffs táticos inimigos
                else if (skillnum == SPELL_BLINDNESS && !IS_AFFECTED(FIGHTING(ch), AFF_BLIND)) {
                    current_score = 95;
                    target_char = FIGHTING(ch);
                } else if (skillnum == SPELL_SLEEP && !IS_AFFECTED(FIGHTING(ch), AFF_SLEEP)) {
                    current_score = 95;
                    target_char = FIGHTING(ch);
                } else if (skillnum == SPELL_PARALYSE && !IS_AFFECTED(FIGHTING(ch), AFF_PARALIZE)) {
                    current_score = 95;
                    target_char = FIGHTING(ch);
                } else if (skillnum == SPELL_CURSE && !IS_AFFECTED(FIGHTING(ch), AFF_CURSE)) {
                    current_score = 90;
                    target_char = FIGHTING(ch);
                } else if (skillnum == SPELL_POISON && !IS_AFFECTED(FIGHTING(ch), AFF_POISON)) {
                    current_score = 85;
                    target_char = FIGHTING(ch);
                }

                // PRIORIDADE 8: Dano direto
                else if (spell->mag_flags & MAG_DAMAGE) {
                    // Danos mais altos para magias mais poderosas
                    if (skillnum == SPELL_DISINTEGRATE || skillnum == SPELL_FIREBALL ||
                        skillnum == SPELL_LIGHTNING_BOLT) {
                        current_score = 80;
                        target_char = FIGHTING(ch);
                    } else if (skillnum == SPELL_DISPEL_EVIL || skillnum == SPELL_DISPEL_GOOD ||
                               skillnum == SPELL_HARM) {
                        current_score = 75;
                        target_char = FIGHTING(ch);
                    } else {
                        current_score = 70;
                        target_char = FIGHTING(ch);
                    }
                }

            } else {
                /* ** MODO DE PREPARAÇÃO (Fora de Combate) ** */

                // PRIORIDADE 1: Remover problemas sérios
                if (IS_AFFECTED(ch, AFF_CURSE) && skillnum == SPELL_REMOVE_CURSE) {
                    current_score = 180;
                    target_char = ch;
                } else if (IS_AFFECTED(ch, AFF_POISON) && skillnum == SPELL_REMOVE_POISON) {
                    current_score = 170;
                    target_char = ch;
                } else if (IS_AFFECTED(ch, AFF_BLIND) && skillnum == SPELL_CURE_BLIND) {
                    current_score = 160;
                    target_char = ch;
                }

                // PRIORIDADE 2: Limpar itens amaldiçoados
                else if (IS_SET(spell->mag_flags, MAG_UNAFFECTS) && skillnum == SPELL_REMOVE_CURSE) {
                    struct obj_data *cursed_item = find_cursed_item_in_inventory(ch);
                    if (cursed_item) {
                        current_score = 150;
                        target_obj = cursed_item;
                    }
                }

                // PRIORIDADE 3: Proteções defensivas supremas
                else if (skillnum == SPELL_SANCTUARY && !IS_AFFECTED(ch, AFF_SANCTUARY)) {
                    current_score = 140;
                    target_char = ch;
                } else if (skillnum == SPELL_STONESKIN && !IS_AFFECTED(ch, AFF_STONESKIN)) {
                    current_score = 135;
                    target_char = ch;
                } else if (skillnum == SPELL_GLOOMSHIELD && !IS_AFFECTED(ch, AFF_GLOOMSHIELD)) {
                    current_score = 130;
                    target_char = ch;
                }

                // PRIORIDADE 4: Escudos de dano e proteções
                else if (skillnum == SPELL_FIRESHIELD && !IS_AFFECTED(ch, AFF_FIRESHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_THISTLECOAT && !IS_AFFECTED(ch, AFF_THISTLECOAT)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_WINDWALL && !IS_AFFECTED(ch, AFF_WINDWALL)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_SOUNDBARRIER && !IS_AFFECTED(ch, AFF_SOUNDBARRIER)) {
                    current_score = 120;
                    target_char = ch;
                }

                // PRIORIDADE 5: Proteções de alinhamento
                else if (skillnum == SPELL_PROT_FROM_EVIL && IS_GOOD(ch) && !IS_AFFECTED(ch, AFF_PROTECT_EVIL)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_PROT_FROM_GOOD && IS_EVIL(ch) && !IS_AFFECTED(ch, AFF_PROTECT_GOOD)) {
                    current_score = 115;
                    target_char = ch;
                }

                // PRIORIDADE 6: Melhoramento de itens
                else if (IS_SET(spell->mag_flags, MAG_MANUAL) && (skillnum == SPELL_BLESS_OBJECT) && IS_GOOD(ch)) {
                    struct obj_data *item_to_buff = find_unblessed_weapon_or_armor(ch);
                    if (item_to_buff) {
                        current_score = 110;
                        target_obj = item_to_buff;
                    }
                } else if (skillnum == SPELL_ENCHANT_WEAPON) {
                    // Procura arma não encantada
                    struct obj_data *weapon = GET_EQ(ch, WEAR_WIELD);
                    if (weapon && GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {
                        current_score = 105;
                        target_obj = weapon;
                    }
                }

                // PRIORIDADE 7: Buffs físicos
                else if (skillnum == SPELL_STRENGTH && !IS_AFFECTED(ch, skillnum)) {
                    current_score = 100;
                    target_char = ch;
                } else if (skillnum == SPELL_ARMOR && !IS_AFFECTED(ch, skillnum)) {
                    current_score = 95;
                    target_char = ch;
                }

                // PRIORIDADE 8: Habilidades de detecção
                else if (skillnum == SPELL_DETECT_INVIS && !IS_AFFECTED(ch, AFF_DETECT_INVIS)) {
                    current_score = 90;
                    target_char = ch;
                } else if (skillnum == SPELL_DETECT_MAGIC && !IS_AFFECTED(ch, AFF_DETECT_MAGIC)) {
                    current_score = 85;
                    target_char = ch;
                } else if (skillnum == SPELL_DETECT_ALIGN && !IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
                    current_score = 85;
                    target_char = ch;
                } else if (skillnum == SPELL_SENSE_LIFE && !IS_AFFECTED(ch, AFF_SENSE_LIFE)) {
                    current_score = 80;
                    target_char = ch;
                } else if (skillnum == SPELL_INFRAVISION && !IS_AFFECTED(ch, AFF_INFRAVISION)) {
                    current_score = 75;
                    target_char = ch;
                }

                // PRIORIDADE 9: Habilidades de movimento/utilidade
                else if (skillnum == SPELL_FLY && !IS_AFFECTED(ch, AFF_FLYING)) {
                    current_score = 70;
                    target_char = ch;
                } else if (skillnum == SPELL_WATERWALK && !IS_AFFECTED(ch, AFF_WATERWALK)) {
                    current_score = 65;
                    target_char = ch;
                } else if (skillnum == SPELL_BREATH && !IS_AFFECTED(ch, AFF_BREATH)) {
                    current_score = 60;
                    target_char = ch;
                }

                // PRIORIDADE 10: Habilidades furtivas/táticas
                else if (skillnum == SPELL_INVISIBLE && !IS_AFFECTED(ch, AFF_INVISIBLE)) {
                    current_score = 55;
                    target_char = ch;
                }

                // PRIORIDADE 11: Buffs genéricos restantes
                else if (IS_SET(spell->mag_flags, MAG_AFFECTS) && IS_SET(spell->targ_flags, TAR_SELF_ONLY)) {
                    // Verifica qualquer magia de buff defensivo genérico
                    if (!IS_AFFECTED(ch, skillnum)) {
                        current_score = 50;
                        target_char = ch;
                    }
                }
            }

            if (current_score > best_score) {
                best_score = current_score;
                item_to_use = obj;
                spellnum_to_cast = skillnum;
                // Store the correct target
                best_target_char = target_char;
                best_target_obj = target_obj;
            }
        }   // End of spell evaluation loop
    }

    if (item_to_use) {
        if (is_last_consumable(ch, item_to_use)) {
            /* Bloco comentado: Se fosse o ultimo e a tendencia caisse, tendencia cai pra 0
            ch->ai_data->genetics.use_tendency = MAX(ch->ai_data->genetics.use_tendency - 1, 0);
            */
            return FALSE;
        }

        if (cast_spell(ch, best_target_char, best_target_obj, spellnum_to_cast)) {
            ch->ai_data->genetics.use_tendency += 2;
        } else {
            ch->ai_data->genetics.use_tendency -= 2;
        }
        ch->ai_data->genetics.use_tendency = MIN(MAX(ch->ai_data->genetics.use_tendency, 0), 100);
        return TRUE;
    }
    return FALSE;
}

/**
 * Encontra um mob específico em uma sala pelo seu número real (rnum).
 */
struct char_data *get_mob_in_room_by_rnum(room_rnum room, mob_rnum rnum)
{
    struct char_data *i;
    for (i = world[room].people; i; i = i->next_in_room) {
        if (IS_NPC(i) && GET_MOB_RNUM(i) == rnum) {
            return i;
        }
    }
    return NULL;
}

/**
 * Find a mob in a room by its virtual number (vnum).
 * @param room The room number to search in
 * @param vnum The virtual number of the mob to find
 * @return Pointer to the mob if found, NULL otherwise
 */
struct char_data *get_mob_in_room_by_vnum(room_rnum room, mob_vnum vnum)
{
    struct char_data *i;
    for (i = world[room].people; i; i = i->next_in_room) {
        if (IS_NPC(i) && GET_MOB_VNUM(i) == vnum) {
            return i;
        }
    }
    return NULL;
}

/**
 * Find a questmaster mob anywhere in the world by its virtual number.
 * @param vnum The virtual number of the questmaster to find
 * @return Pointer to the questmaster if found, NULL otherwise
 */
struct char_data *find_questmaster_by_vnum(mob_vnum vnum)
{
    struct char_data *i, *next_i;

    /* Search through all characters in the world */
    for (i = character_list; i; i = next_i) {
        next_i = i->next;

        /* Safety check: Skip characters marked for extraction */
        if (MOB_FLAGGED(i, MOB_NOTDEADYET) || PLR_FLAGGED(i, PLR_NOTDEADYET))
            continue;

        /* Safety check: Validate room before using the character */
        if (!IS_NPC(i) || IN_ROOM(i) == NOWHERE || IN_ROOM(i) < 0 || IN_ROOM(i) > top_of_world)
            continue;

        if (GET_MOB_VNUM(i) == vnum) {
            /* Check if this mob is a questmaster (has quest special procedure) */
            if (mob_index[GET_MOB_RNUM(i)].func == questmaster || mob_index[GET_MOB_RNUM(i)].func == temp_questmaster) {
                return i;
            }
        }
    }
    return NULL;
}

/**
 * A IA de economia. O mob tenta vender os seus itens de menor valor.
 * VERSÃO FINAL COM CORREÇÃO DE BUGS.
 * Retorna TRUE se o mob definiu um objetivo ou executou uma venda.
 */
bool mob_try_to_sell_junk(struct char_data *ch)
{

    /* 1. GATILHO: A IA só age se tiver genética e se o inventário estiver > 80% cheio. */
    bool inventory_full = (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) * 0.8);
    bool inventory_heavy = (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch) * 0.8);

    if (!ch->ai_data || !ch->carrying || (!inventory_full && !inventory_heavy))
        return FALSE;

    if (rand_number(1, 100) > MAX(GET_GENTRADE(ch), 5))
        return FALSE;

    struct obj_data *obj, *item_to_sell = NULL;
    int min_score = 10;

    /* 2. ANÁLISE DE INVENTÁRIO: Encontra o pior item para vender. */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (OBJ_FLAGGED(obj, ITEM_NODROP) || (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains))
            continue;

        int score = evaluate_item_for_mob(ch, obj);
        if (score < min_score) {
            min_score = score;
            item_to_sell = obj;
        }
    }

    if (!item_to_sell)
        return FALSE; /* Não tem "lixo" para vender. */

    /* 3. ANÁLISE DE MERCADO: Encontra a melhor loja para vender o item. */
    /* CORREÇÃO: Usamos 'int' para podermos verificar o -1. */
    int best_shop_rnum = find_best_shop_to_sell(ch, item_to_sell);

    /* If the best shop is not reachable, try to find an alternative */
    if (best_shop_rnum == -1 || best_shop_rnum < 0 || best_shop_rnum > top_shop ||
        !shop_index[best_shop_rnum].in_room || SHOP_ROOM(best_shop_rnum, 0) == NOWHERE) {
        return FALSE; /* No shops available */
    }

    room_rnum target_shop_room = real_room(SHOP_ROOM(best_shop_rnum, 0));

    /* Check if we can reach the shop, if not try to find an alternative */
    if (target_shop_room == NOWHERE || find_first_step(IN_ROOM(ch), target_shop_room) == -1) {
        /* Try to find a second-best shop that is reachable */
        best_shop_rnum = -1;
        float best_profit = 0.0;

        for (int snum = 0; snum <= top_shop; snum++) {
            if (!is_shop_open(snum))
                continue;

            /* Validate shop index entry is properly initialized */
            if (!shop_index[snum].type || !shop_index[snum].in_room) {
                continue;
            }

            /* Check if shop buys this type of item */
            bool buys_this_type = FALSE;
            for (int i = 0; i < MAX_TRADE && SHOP_BUYTYPE(snum, i) != NOTHING; i++) {
                if (SHOP_BUYTYPE(snum, i) == GET_OBJ_TYPE(item_to_sell)) {
                    buys_this_type = TRUE;
                    break;
                }
            }
            if (!buys_this_type)
                continue;

            room_rnum shop_location = real_room(SHOP_ROOM(snum, 0));
            if (shop_location == NOWHERE)
                continue;

            /* This time, prioritize reachability over profit */
            if (find_first_step(IN_ROOM(ch), shop_location) != -1) {
                float current_profit = GET_OBJ_COST(item_to_sell) * SHOP_BUYPROFIT(snum);
                if (current_profit > best_profit) {
                    best_profit = current_profit;
                    best_shop_rnum = snum;
                }
            }
        }

        if (best_shop_rnum == -1) {
            return FALSE; /* No reachable shops found */
        }

        /* Validate shop before accessing its data */
        if (!shop_index[best_shop_rnum].in_room) {
            return FALSE; /* Shop data is corrupted */
        }

        target_shop_room = real_room(SHOP_ROOM(best_shop_rnum, 0));
    }

    if (best_shop_rnum >= 0 && best_shop_rnum <= top_shop && target_shop_room != NOWHERE) {
        /* Se ainda não está na loja, o objetivo é ir para lá. */
        act("$n olha para a sua mochila e parece estar a planejar uma viagem.", FALSE, ch, 0, 0, TO_ROOM);
        ch->ai_data->current_goal = GOAL_GOTO_SHOP_TO_SELL;
        ch->ai_data->goal_destination = target_shop_room;
        ch->ai_data->goal_obj = item_to_sell;
        ch->ai_data->goal_target_mob_rnum = SHOP_KEEPER(best_shop_rnum);
        ch->ai_data->goal_timer = 0; /* Reset timer for new goal */

        mob_goal_oriented_roam(ch, target_shop_room);
        return TRUE;
    }

    return FALSE;
}

/*
 * ===============================================
 * WISHLIST GOAL PROCESSING FOR MOB AI
 * ===============================================
 */

/**
 * Makes a mob occasionally decide to seek out and accept quests.
 * Called when a mob has no current goal and not already on a quest.
 * @param ch The mob that might accept a quest
 * @return TRUE if the mob decides to pursue quest acceptance, FALSE otherwise
 */
bool mob_try_to_accept_quest(struct char_data *ch)
{
    struct char_data *questmaster;
    zone_rnum mob_zone;

    if (!IS_NPC(ch) || !ch->ai_data || ch->ai_data->current_goal != GOAL_NONE) {
        return FALSE; /* Already has a goal or not an AI mob */
    }

    /* Don't accept quests if already on one */
    if (GET_QUEST(ch)) {
        return FALSE;
    }

    /* Only occasionally try to accept quests - about 5% chance per call */
    if (rand() % 100 > 5) {
        return FALSE;
    }

    /* Check if frustrated from recent quest activities */
    if (ch->ai_data->quest_posting_frustration_timer > 0) {
        return FALSE;
    }

    /* Safety check: Validate room before accessing world array */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    /* Look for accessible questmasters in the current zone */
    mob_zone = world[IN_ROOM(ch)].zone;
    questmaster = find_accessible_questmaster_in_zone(ch, mob_zone);

    if (questmaster && questmaster != ch) {
        /* Check if this questmaster has available quests for this mob */
        qst_vnum available_quest = find_available_quest_by_qmnum(ch, GET_MOB_VNUM(questmaster), 1);
        if (available_quest != NOTHING) {
            qst_rnum quest_rnum = real_quest(available_quest);
            if (quest_rnum != NOTHING && mob_should_accept_quest(ch, quest_rnum)) {
                /* Set goal to go to questmaster and accept quest */
                ch->ai_data->current_goal = GOAL_ACCEPT_QUEST;
                ch->ai_data->goal_destination = IN_ROOM(questmaster);
                ch->ai_data->goal_target_mob_rnum = GET_MOB_RNUM(questmaster);
                ch->ai_data->goal_timer = 0;
                act("$n parece estar à procura de trabalho.", FALSE, ch, 0, 0, TO_ROOM);
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 * Process quest completion for a mob with an active quest.
 * Handles different quest types and implements quest completion logic.
 * @param ch The mob attempting to complete a quest
 * @param quest_rnum The real number of the quest being completed
 * @return TRUE if quest completion was attempted (even if not successful), FALSE if quest could not be processed
 */
bool mob_process_quest_completion(struct char_data *ch, qst_rnum quest_rnum)
{
    struct char_data *target_mob, *questmaster;
    struct obj_data *target_obj;
    room_rnum target_room;
    int quest_type = QST_TYPE(quest_rnum);

    if (!IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* Handle quest completion based on quest type */
    switch (quest_type) {
        case AQ_OBJ_FIND:
            /* Check if mob has the required object */
            target_obj = get_obj_in_list_num(QST_TARGET(quest_rnum), ch->carrying);
            if (target_obj) {
                /* Object found, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else {
                /* Object not found, add to wishlist and seek it */
                add_item_to_wishlist(ch, QST_TARGET(quest_rnum), 100); /* High priority */
                ch->ai_data->current_goal = GOAL_NONE;                 /* Let wishlist system handle it */
                return TRUE;
            }
            break;

        case AQ_ROOM_FIND:
            /* Check if mob is in the target room */
            target_room = real_room(QST_TARGET(quest_rnum));
            if (target_room != NOWHERE && IN_ROOM(ch) == target_room) {
                /* Target room reached, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else if (target_room != NOWHERE) {
                /* Move toward target room */
                ch->ai_data->goal_destination = target_room;
                mob_goal_oriented_roam(ch, target_room);
                return TRUE;
            }
            break;

        case AQ_MOB_FIND:
            /* Check if target mob is in current room */
            target_mob = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_TARGET(quest_rnum));
            if (target_mob) {
                /* Target mob found, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else {
                /* Target mob not found, seek it by using the normal mob AI movement */
                /* Use mob_goal_oriented_roam instead of manual char_from_room/char_to_room
                 * to avoid the bug where IN_ROOM(ch) becomes NOWHERE after char_from_room */
                mob_goal_oriented_roam(ch, NOWHERE); /* Roam randomly */
                /* Safety check: mob_goal_oriented_roam uses perform_move which can trigger death traps */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    return TRUE;
            }
            return TRUE;
            break;

        case AQ_MOB_KILL:
        case AQ_MOB_KILL_BOUNTY:
            /* Check if target mob is in current room and attack it */
            target_mob = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_TARGET(quest_rnum));
            if (target_mob && !FIGHTING(ch)) {
                /* Attack the target mob */
                act("$n olha para $N com determinação.", FALSE, ch, 0, target_mob, TO_ROOM);
                hit(ch, target_mob, TYPE_UNDEFINED);
                /* Safety check: hit() can indirectly cause extract_char */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    return TRUE;
                return TRUE;
            } else if (!target_mob) {
                /* Target mob not found, seek it by using the normal mob AI movement */
                /* Use mob_goal_oriented_roam instead of manual char_from_room/char_to_room
                 * to avoid the bug where IN_ROOM(ch) becomes NOWHERE after char_from_room */
                mob_goal_oriented_roam(ch, NOWHERE); /* Roam randomly */
                /* Safety check: mob_goal_oriented_roam uses perform_move which can trigger death traps */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    return TRUE;
            }
            return TRUE;
            break;

        case AQ_OBJ_RETURN:
            /* Check if mob has the required object to return */
            target_obj = get_obj_in_list_num(QST_TARGET(quest_rnum), ch->carrying);
            if (target_obj) {
                /* Has object, find questmaster to return it to */
                questmaster = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_RETURNMOB(quest_rnum));
                if (!questmaster) {
                    questmaster = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_MASTER(quest_rnum));
                }

                if (questmaster) {
                    /* Return object to questmaster */
                    act("$n entrega $p para $N.", FALSE, ch, target_obj, questmaster, TO_ROOM);
                    obj_from_char(target_obj);
                    extract_obj(target_obj);
                    mob_complete_quest(ch);
                    ch->ai_data->current_goal = GOAL_NONE;
                    return TRUE;
                } else {
                    /* Need to find questmaster - go to questmaster location */
                    questmaster = find_questmaster_by_vnum(QST_RETURNMOB(quest_rnum));
                    if (!questmaster) {
                        questmaster = find_questmaster_by_vnum(QST_MASTER(quest_rnum));
                    }

                    if (questmaster) {
                        ch->ai_data->goal_destination = IN_ROOM(questmaster);
                        mob_goal_oriented_roam(ch, IN_ROOM(questmaster));
                        return TRUE;
                    }
                }
            } else {
                /* Don't have object, add to wishlist and seek it */
                add_item_to_wishlist(ch, QST_TARGET(quest_rnum), 100); /* High priority */
                ch->ai_data->current_goal = GOAL_NONE;                 /* Let wishlist system handle it */
                return TRUE;
            }
            break;

        case AQ_ROOM_CLEAR:
            /* Kill all mobs in target room */
            target_room = real_room(QST_TARGET(quest_rnum));
            if (target_room != NOWHERE) {
                if (IN_ROOM(ch) == target_room) {
                    /* In target room, check for hostile mobs */
                    struct char_data *temp_mob;
                    bool found_hostile = FALSE;

                    for (temp_mob = world[target_room].people; temp_mob; temp_mob = temp_mob->next_in_room) {
                        if (IS_NPC(temp_mob) && temp_mob != ch && !AFF_FLAGGED(temp_mob, AFF_CHARM)) {
                            /* Found a mob to kill */
                            if (!FIGHTING(ch)) {
                                act("$n ataca $N para limpar a área.", FALSE, ch, 0, temp_mob, TO_ROOM);
                                hit(ch, temp_mob, TYPE_UNDEFINED);
                                /* Safety check: hit() can indirectly cause extract_char */
                                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                    return TRUE;
                            }
                            found_hostile = TRUE;
                            break;
                        }
                    }

                    if (!found_hostile) {
                        /* Room cleared, complete quest */
                        mob_complete_quest(ch);
                        ch->ai_data->current_goal = GOAL_NONE;
                        return TRUE;
                    }
                } else {
                    /* Move toward target room */
                    ch->ai_data->goal_destination = target_room;
                    mob_goal_oriented_roam(ch, target_room);
                    return TRUE;
                }
            }
            break;

        default:
            /* Unknown quest type, abandon quest */
            fail_mob_quest(ch, "unknown quest type");
            ch->ai_data->current_goal = GOAL_NONE;
            return TRUE;
    }

    return FALSE;
}

/**
 * Processa a wishlist de um mob e define um objetivo baseado no item de maior prioridade.
 * Implementa a árvore de decisão tática descrita no issue.
 * @param ch O mob cujo wishlist será processado
 */
void mob_process_wishlist_goals(struct char_data *ch)
{
    struct mob_wishlist_item *desired_item;
    mob_rnum target_mob;
    shop_rnum target_shop;
    room_rnum shop_room;
    obj_rnum obj_rnum;
    int item_cost, required_gold;

    if (!IS_NPC(ch) || !ch->ai_data || ch->ai_data->current_goal != GOAL_NONE) {
        return; /* Já tem um objetivo ou não é um mob com AI */
    }

    /* Check quest posting frustration timer - prevent quest posting after fleeing */
    if (ch->ai_data->quest_posting_frustration_timer > 0) {
        return; /* Still frustrated from fleeing, cannot post quests */
    }

    /* Priority 1: If mob has an active quest, prioritize quest completion */
    if (GET_QUEST(ch) != NOTHING) {
        qst_rnum quest_rnum = real_quest(GET_QUEST(ch));
        if (quest_rnum != NOTHING) {
            int quest_type = QST_TYPE(quest_rnum);

            /* For object-based quests, add quest objects to wishlist with high priority */
            if (quest_type == AQ_OBJ_FIND || quest_type == AQ_OBJ_RETURN) {
                obj_vnum quest_obj_vnum = QST_TARGET(quest_rnum);

                /* Check if mob already has the quest object */
                struct obj_data *quest_obj = get_obj_in_list_num(quest_obj_vnum, ch->carrying);
                if (!quest_obj) {
                    /* Add quest object to wishlist with maximum priority */
                    add_item_to_wishlist(ch, quest_obj_vnum, 200); /* Higher than normal max priority */

                    /* Process this quest object as highest priority item */
                    desired_item = get_top_wishlist_item(ch);
                    if (desired_item && desired_item->vnum == quest_obj_vnum) {
                        /* Continue with normal wishlist processing for quest object */
                        /* This will make the mob seek the quest object through normal means */
                    }
                } else if (quest_type == AQ_OBJ_RETURN) {
                    /* Has object for return quest, transition to quest completion goal */
                    ch->ai_data->current_goal = GOAL_COMPLETE_QUEST;
                    ch->ai_data->goal_timer = 0;
                    return;
                }
            } else {
                /* For non-object quests (mob kill, room find, etc.), transition to quest completion */
                ch->ai_data->current_goal = GOAL_COMPLETE_QUEST;
                ch->ai_data->goal_timer = 0;
                return;
            }
        }
    }

    desired_item = get_top_wishlist_item(ch);
    if (!desired_item) {
        return; /* Wishlist vazia */
    }

    /* Passo 1: Identificar o objetivo */
    /* Passo 2: Encontrar fontes para o item */

    /* Tenta encontrar um mob que dropa o item */
    target_mob = find_mob_with_item(desired_item->vnum);

    /* Tenta encontrar uma loja que vende o item */
    target_shop = find_shop_selling_item(desired_item->vnum);

    /* Passo 3: Avaliar as opções e escolher o melhor plano */

    /* Opção 1: Caçar um mob */
    if (target_mob != NOBODY) {
        /* Verifica se consegue matar o alvo (simplificado) */
        if (GET_LEVEL(ch) >= mob_proto[target_mob].player.level - 5) {
            /* Pode caçar este mob */
            ch->ai_data->current_goal = GOAL_HUNT_TARGET;
            ch->ai_data->goal_target_mob_rnum = target_mob;
            ch->ai_data->goal_item_vnum = desired_item->vnum;
            ch->ai_data->goal_timer = 0;
            act("$n parece estar a planear uma caça.", FALSE, ch, 0, 0, TO_ROOM);
            return;
        }
    }

    /* Opção 2: Comprar numa loja */
    if (target_shop != NOTHING && target_shop <= top_shop && shop_index[target_shop].in_room) {
        /* Verifica se pode chegar à loja e tem ouro suficiente */
        shop_room = real_room(SHOP_ROOM(target_shop, 0));

        if (shop_room != NOWHERE && !ROOM_FLAGGED(shop_room, ROOM_NOTRACK)) {
            /* Calcula o custo do item */
            obj_rnum = real_object(desired_item->vnum);
            if (obj_rnum != NOTHING) {
                item_cost = GET_OBJ_COST(&obj_proto[obj_rnum]);
                if (item_cost <= 0)
                    item_cost = 1;
                item_cost = (int)(item_cost * shop_index[target_shop].profit_buy);

                if (GET_GOLD(ch) >= item_cost) {
                    /* Tem ouro suficiente, vai comprar */
                    ch->ai_data->current_goal = GOAL_GOTO_SHOP_TO_BUY;
                    ch->ai_data->goal_destination = shop_room;
                    ch->ai_data->goal_target_mob_rnum = SHOP_KEEPER(target_shop);
                    ch->ai_data->goal_item_vnum = desired_item->vnum;
                    ch->ai_data->goal_timer = 0;
                    act("$n examina a sua bolsa e sorri.", FALSE, ch, 0, 0, TO_ROOM);
                    return;
                } else {
                    /* Não tem ouro suficiente, precisa conseguir mais */
                    required_gold = item_cost - GET_GOLD(ch);
                    if (required_gold <= GET_LEVEL(ch) * 100) { /* Meta razoável */
                        ch->ai_data->current_goal = GOAL_GET_GOLD;
                        ch->ai_data->goal_item_vnum = desired_item->vnum;
                        ch->ai_data->goal_timer = 0;
                        act("$n conta as suas moedas e franze o sobrolho.", FALSE, ch, 0, 0, TO_ROOM);
                        return;
                    }
                }
            }
        }
    }

    /* Opção 3: Postar uma quest (implementação aprimorada) */
    if (GET_GOLD(ch) >= desired_item->priority * 2) {
        /* Safety check: Validate room before accessing world array */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
            return;

        /* Tem ouro suficiente para oferecer uma recompensa */
        zone_rnum mob_zone = world[IN_ROOM(ch)].zone;
        struct char_data *accessible_qm = find_accessible_questmaster_in_zone(ch, mob_zone);

        if (accessible_qm && accessible_qm != ch) {
            /* Há um questmaster acessível, vai até ele para postar */
            ch->ai_data->current_goal = GOAL_GOTO_QUESTMASTER;
            ch->ai_data->goal_destination = IN_ROOM(accessible_qm);
            ch->ai_data->goal_item_vnum = desired_item->vnum;
            ch->ai_data->goal_timer = 0;
            act("$n parece estar a considerar contratar aventureiros.", FALSE, ch, 0, 0, TO_ROOM);
            return; /* Vai tentar chegar ao questmaster */
        } else {
            /* Não há questmaster acessível, torna-se questmaster próprio */
            ch->ai_data->current_goal = GOAL_POST_QUEST;
            ch->ai_data->goal_item_vnum = desired_item->vnum;
            ch->ai_data->goal_timer = 0;
            act("$n parece estar a considerar contratar aventureiros.", FALSE, ch, 0, 0, TO_ROOM);

            /* Simula a postagem da quest */
            mob_posts_quest(ch, desired_item->vnum, desired_item->priority * 2);

            /* Reseta o objetivo após "postar" a quest */
            ch->ai_data->current_goal = GOAL_NONE;
            return;
        }
    }

    /* Se chegou aqui, não consegue obter o item por agora */
    /* Remove da wishlist itens muito antigos (>1 hora) */
    if (time(0) - desired_item->added_time > 3600) {
        remove_item_from_wishlist(ch, desired_item->vnum);
    }
}

/*
 * ===============================================
 * MOB COMMAND USAGE FUNCTIONS
 * ===============================================
 */

/**
 * Makes a mob try to donate an item to one of the donation rooms
 * @param ch The mob
 * @param obj The object to donate (NULL to find one automatically)
 * @return TRUE if donation was attempted
 */
bool mob_try_donate(struct char_data *ch, struct obj_data *obj)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* If no object specified, find a suitable item to donate */
    if (!obj) {
        struct obj_data *item;
        int min_score = 10000;

        /* Find the least valuable item to donate */
        for (item = ch->carrying; item; item = item->next_content) {
            if (!CAN_WEAR(item, ITEM_WEAR_TAKE) || OBJ_FLAGGED(item, ITEM_NODROP)) {
                continue;
            }

            int score = evaluate_item_for_mob(ch, item);
            if (score < min_score) {
                min_score = score;
                obj = item;
            }
        }

        if (!obj) {
            return FALSE; /* No suitable item found */
        }
    }

    /* Use the donate command */
    char cmd_buf[MAX_INPUT_LENGTH];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", obj->name);
    do_drop(ch, cmd_buf, 0, SCMD_DONATE);

    return TRUE;
}

/**
 * Makes a mob try to sacrifice a corpse
 * @param ch The mob
 * @param corpse The corpse to sacrifice (NULL to find one automatically)
 * @return TRUE if sacrifice was attempted
 */
bool mob_try_sacrifice(struct char_data *ch, struct obj_data *corpse)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* If no corpse specified, find one in the room */
    if (!corpse) {
        struct obj_data *obj;

        /* Safety check: Validate room before accessing world array */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
            return FALSE;

        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
            if (IS_CORPSE(obj)) {
                corpse = obj;
                break;
            }
        }

        if (!corpse) {
            return FALSE; /* No corpse found */
        }
    }

    /* Use the sacrifice command */
    char cmd_buf[MAX_INPUT_LENGTH];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", corpse->name);
    do_sac(ch, cmd_buf, 0, 0);

    return TRUE;
}

/**
 * Makes a mob try to junk (destroy) an item
 * @param ch The mob
 * @param obj The object to junk (NULL to find one automatically)
 * @return TRUE if junk was attempted
 */
bool mob_try_junk(struct char_data *ch, struct obj_data *obj)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* If no object specified, find a suitable item to junk */
    if (!obj) {
        struct obj_data *item;
        int min_score = 10000;

        /* Find the least valuable item to junk */
        for (item = ch->carrying; item; item = item->next_content) {
            if (!CAN_WEAR(item, ITEM_WEAR_TAKE) || OBJ_FLAGGED(item, ITEM_NODROP)) {
                continue;
            }

            int score = evaluate_item_for_mob(ch, item);
            if (score < min_score && GET_OBJ_COST(item) < 50) { /* Only junk cheap items */
                min_score = score;
                obj = item;
            }
        }

        if (!obj) {
            return FALSE; /* No suitable item found */
        }
    }

    /* Use the junk command */
    char cmd_buf[MAX_INPUT_LENGTH];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", obj->name);
    do_drop(ch, cmd_buf, 0, SCMD_JUNK);

    return TRUE;
}

/**
 * Makes a mob try to drop an item
 * @param ch The mob
 * @param obj The object to drop (NULL to find one automatically)
 * @return TRUE if drop was attempted
 */
bool mob_try_drop(struct char_data *ch, struct obj_data *obj)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* If no object specified, find a suitable item to drop */
    if (!obj) {
        struct obj_data *item;

        /* Drop a random item from inventory */
        int count = 0;
        for (item = ch->carrying; item; item = item->next_content) {
            if (CAN_WEAR(item, ITEM_WEAR_TAKE) && !OBJ_FLAGGED(item, ITEM_NODROP)) {
                count++;
            }
        }

        if (count == 0) {
            return FALSE; /* No droppable items */
        }

        int target = rand_number(1, count);
        count = 0;
        for (item = ch->carrying; item; item = item->next_content) {
            if (CAN_WEAR(item, ITEM_WEAR_TAKE) && !OBJ_FLAGGED(item, ITEM_NODROP)) {
                count++;
                if (count == target) {
                    obj = item;
                    break;
                }
            }
        }

        if (!obj) {
            return FALSE;
        }
    }

    /* Use the drop command */
    char cmd_buf[MAX_INPUT_LENGTH];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", obj->name);
    do_drop(ch, cmd_buf, 0, SCMD_DROP);

    return TRUE;
}

/* Source types for key location */
#define KEY_SOURCE_ROOM 1      /* Key is on the ground in a room */
#define KEY_SOURCE_CONTAINER 2 /* Key is inside a container */
#define KEY_SOURCE_MOB 3       /* Key is carried by a mob */

/**
 * Find where a specific key can be obtained
 * @param key_vnum The vnum of the key to find
 * @param source_type Pointer to store the source type (KEY_SOURCE_*)
 * @param carrying_mob Pointer to store the mob vnum if carried by a mob
 * @return room number where the key can be found, or NOWHERE if not found
 */
room_rnum find_key_location(obj_vnum key_vnum, int *source_type, mob_vnum *carrying_mob)
{
    room_rnum room;
    struct obj_data *obj;
    struct char_data *mob;

    if (source_type)
        *source_type = 0;
    if (carrying_mob)
        *carrying_mob = NOBODY;

    /* Search all rooms for the key */
    for (room = 0; room <= top_of_world; room++) {
        /* Check objects on room floor */
        for (obj = world[room].contents; obj; obj = obj->next_content) {
            if (GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == key_vnum) {
                if (source_type)
                    *source_type = KEY_SOURCE_ROOM;
                return room;
            }

            /* Check inside containers */
            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
                struct obj_data *contained;
                for (contained = obj->contains; contained; contained = contained->next_content) {
                    if (GET_OBJ_TYPE(contained) == ITEM_KEY && GET_OBJ_VNUM(contained) == key_vnum) {
                        if (source_type)
                            *source_type = KEY_SOURCE_CONTAINER;
                        return room;
                    }
                }
            }
        }

        /* Check mobs in this room */
        for (mob = world[room].people; mob; mob = mob->next_in_room) {
            if (!IS_NPC(mob))
                continue;

            /* Check mob inventory */
            for (obj = mob->carrying; obj; obj = obj->next_content) {
                if (GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == key_vnum) {
                    if (source_type)
                        *source_type = KEY_SOURCE_MOB;
                    if (carrying_mob)
                        *carrying_mob = GET_MOB_VNUM(mob);
                    return room;
                }
            }

            /* Check mob equipment */
            for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
                obj = GET_EQ(mob, wear_pos);
                if (obj && GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == key_vnum) {
                    if (source_type)
                        *source_type = KEY_SOURCE_MOB;
                    if (carrying_mob)
                        *carrying_mob = GET_MOB_VNUM(mob);
                    return room;
                }
            }
        }
    }

    return NOWHERE; /* Key not found anywhere */
}

/**
 * Set a key collection goal for a mob, storing the original goal to return to
 * @param ch The mob
 * @param key_vnum The key to collect
 * @param original_goal The goal to return to after collecting the key
 * @param original_dest The original destination
 * @return TRUE if goal was set successfully
 */
bool mob_set_key_collection_goal(struct char_data *ch, obj_vnum key_vnum, int original_goal, room_rnum original_dest)
{
    int source_type;
    mob_vnum carrying_mob;
    room_rnum key_location;

    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* Find where the key is located */
    key_location = find_key_location(key_vnum, &source_type, &carrying_mob);

    if (key_location == NOWHERE) {
        return FALSE; /* Key not found anywhere */
    }

    /* Store the original goal information */
    ch->ai_data->original_goal = original_goal;
    ch->ai_data->original_destination = original_dest;
    ch->ai_data->original_obj = ch->ai_data->goal_obj;
    ch->ai_data->original_target_mob = ch->ai_data->goal_target_mob_rnum;
    ch->ai_data->original_item_vnum = ch->ai_data->goal_item_vnum;

    /* Set new key collection goal */
    ch->ai_data->current_goal = GOAL_COLLECT_KEY;
    ch->ai_data->goal_destination = key_location;
    ch->ai_data->goal_item_vnum = key_vnum;
    ch->ai_data->goal_timer = 0;

    /* Set appropriate target based on source type */
    switch (source_type) {
        case KEY_SOURCE_ROOM:
        case KEY_SOURCE_CONTAINER:
            ch->ai_data->goal_target_mob_rnum = NOBODY;
            ch->ai_data->goal_obj = NULL;
            break;
        case KEY_SOURCE_MOB:
            ch->ai_data->goal_target_mob_rnum = real_mobile(carrying_mob);
            ch->ai_data->goal_obj = NULL;
            break;
    }

    act("$n parece estar procurando por algo específico.", FALSE, ch, 0, 0, TO_ROOM);

    return TRUE;
}

/**
 * Find a bank object (ATM/cashcard) in the same room as the mob or in the mob's inventory
 * @param ch The mob looking for a bank
 * @return Pointer to a bank object, or NULL if none found
 */
struct obj_data *find_bank_nearby(struct char_data *ch)
{
    struct obj_data *obj;

    if (!ch || !IS_NPC(ch)) {
        return NULL;
    }

    /* Safety check: Validate room before accessing world array */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return NULL;

    /* First check objects in the same room as the mob */
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
        /* Check if this object has the bank special procedure */
        if (GET_OBJ_SPEC(obj) == bank) {
            return obj;
        }
    }

    /* Then check objects in the mob's inventory */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        /* Check if this object has the bank special procedure */
        if (GET_OBJ_SPEC(obj) == bank) {
            return obj;
        }
    }

    return NULL; /* No bank found in room or inventory */
}

/**
 * Makes a mob use banking services if available and appropriate
 * @param ch The mob that might use the bank
 * @return TRUE if bank was used, FALSE otherwise
 */
bool mob_use_bank(struct char_data *ch)
{
    struct obj_data *bank_obj;
    int current_gold, bank_gold;
    int deposit_amount, withdraw_amount;

    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* Only mobs with high trade genetics use banks */
    if (GET_GENTRADE(ch) <= 50) {
        return FALSE;
    }

    /* Only mobs with very high trade genetics should use banks */
    if (GET_GENTRADE(ch) <= 60) {
        return FALSE;
    }

    /* Only intelligent mobs should use banks (Intelligence > 10) */
    if (GET_INT(ch) <= 10) {
        return FALSE;
    }

    /* Don't use bank if fighting or not awake */
    if (FIGHTING(ch) || !AWAKE(ch)) {
        return FALSE;
    }

    /* Find a bank in the same room or inventory */
    bank_obj = find_bank_nearby(ch);
    if (!bank_obj) {
        return FALSE; /* No bank available */
    }

    current_gold = GET_GOLD(ch);
    bank_gold = GET_BANK_GOLD(ch);

    /* Decision logic: deposit if carrying too much, withdraw if too little */
    if (current_gold > 5000 && rand_number(1, 100) <= 30) {
        /* Deposit excess gold */
        deposit_amount = current_gold / 2; /* Deposit half of current gold */

        /* Simulate the deposit command */
        decrease_gold(ch, deposit_amount);
        increase_bank(ch, deposit_amount);

        act("$n faz uma transação bancária.", TRUE, ch, 0, FALSE, TO_ROOM);

        return TRUE;

    } else if (current_gold < 100 && bank_gold > 500 && rand_number(1, 100) <= 20) {
        /* Withdraw gold when running low */
        withdraw_amount = MIN(bank_gold / 3, 1000); /* Withdraw 1/3 of bank balance, max 1000 */

        /* Simulate the withdraw command */
        increase_gold(ch, withdraw_amount);
        decrease_bank(ch, withdraw_amount);

        act("$n faz uma transação bancária.", TRUE, ch, 0, FALSE, TO_ROOM);

        return TRUE;
    }

    return FALSE; /* No banking action taken */
}
