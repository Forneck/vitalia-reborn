/**************************************************************************
 *  File: gedit.c                                           Part of tbaMUD *
 *  Usage: Oasis OLC - Goal editor for mob instances.                     *
 *                                                                         *
 * Copyright 2024 Vitalia Reborn Project                                   *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "handler.h"
#include "constants.h"
#include "screen.h"
#include "modify.h"
#include "quest.h"

/* local functions */
static void gedit_setup(struct descriptor_data *d, struct char_data *mob);
static void gedit_disp_menu(struct descriptor_data *d);
static void gedit_disp_goals(struct descriptor_data *d);
static void gedit_save_internally(struct descriptor_data *d);

/* Goal editor modes */
#define GEDIT_MAIN_MENU 0
#define GEDIT_GOAL_CHOICE 1
#define GEDIT_GOAL_ROOM 2
#define GEDIT_GOAL_ITEM 3
#define GEDIT_GOAL_TARGET 4
#define GEDIT_GOAL_TIMER 5
#define GEDIT_CONFIRM_SAVE 6

/* Main entry point for goal editor */
ACMD(do_oasis_gedit)
{
    struct char_data *mob;
    struct descriptor_data *d;
    char arg[MAX_INPUT_LENGTH];

    /* No building as a mob or while being forced. */
    if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
        return;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Edit goals for which mob?\r\n");
        return;
    }

    /* Find the mob by keyword in the same room */
    mob = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);
    if (!mob) {
        send_to_char(ch, "There is no one here by that name.\r\n");
        return;
    }

    if (!IS_NPC(mob)) {
        send_to_char(ch, "Goals can only be edited for NPCs.\r\n");
        return;
    }

    /* Check if someone else is already editing this mob's goals */
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) == CON_GEDIT) {
            if (d->olc && OLC_MOB(d) == mob) {
                send_to_char(ch, "That mob's goals are currently being edited by %s.\r\n", GET_NAME(d->character));
                return;
            }
        }
    }

    d = ch->desc;

    /* Give descriptor an OLC structure. */
    if (d->olc) {
        mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: do_oasis_gedit: Player already had olc structure.");
        free(d->olc);
    }

    CREATE(d->olc, struct oasis_olc_data, 1);

    gedit_setup(d, mob);
    gedit_disp_menu(d);
    STATE(d) = CON_GEDIT;

    /* Display the OLC messages to the players in the same room as the
       builder and also log it. */
    act("$n starts editing goals for $N.", TRUE, d->character, 0, mob, TO_ROOM);
    SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

    mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "OLC: %s starts editing goals for %s (%d)", GET_NAME(ch),
           GET_NAME(mob), GET_MOB_VNUM(mob));
}

static void gedit_setup(struct descriptor_data *d, struct char_data *mob)
{
    /* Store reference to the actual mob instance */
    OLC_MOB(d) = mob;
    OLC_NUM(d) = GET_MOB_VNUM(mob);
    OLC_VAL(d) = FALSE; /* Has changed flag */

    /* Create AI data if it doesn't exist */
    if (!mob->ai_data) {
        CREATE(mob->ai_data, struct mob_ai_data, 1);
        memset(mob->ai_data, 0, sizeof(struct mob_ai_data));
        init_mob_ai_data(mob);
    }

    /* Always clear goal_obj to prevent accessing stale pointers. */
    mob->ai_data->goal_obj = NULL;
}

static void gedit_disp_menu(struct descriptor_data *d)
{
    struct char_data *mob;
    room_vnum room_display;
    mob_vnum target_display;

    mob = OLC_MOB(d);
    get_char_colors(d->character);
    clear_screen(d);

    /* Convert rnum to vnum for display */
    room_display =
        (mob->ai_data->goal_destination != NOWHERE) ? GET_ROOM_VNUM(mob->ai_data->goal_destination) : NOWHERE;
    target_display = (mob->ai_data->goal_target_mob_rnum != NOBODY && mob->ai_data->goal_target_mob_rnum >= 0 &&
                      mob->ai_data->goal_target_mob_rnum < top_of_mobt)
                         ? mob_index[mob->ai_data->goal_target_mob_rnum].vnum
                         : NOBODY;

    write_to_output(d,
                    "-- Goal Editor for: %s%s%s [%s%d%s] --\r\n"
                    "\r\n"
                    "%s1%s) Current Goal    : %s%s (%d)%s\r\n"
                    "%s2%s) Goal Room      : %s%d%s %s\r\n"
                    "%s3%s) Goal Item      : %s%d%s %s\r\n"
                    "%s4%s) Goal Target    : %s%d%s %s\r\n"
                    "%s5%s) Goal Timer     : %s%d%s\r\n"
                    "\r\n"
                    "%sL%s) List all available goals\r\n"
                    "%sQ%s) Quit (save changes)\r\n"
                    "Enter choice : ",

                    cyn, GET_NAME(mob), nrm, yel, GET_MOB_VNUM(mob), nrm,

                    grn, nrm, yel,
                    (mob->ai_data->current_goal >= 0 && mob->ai_data->current_goal <= 9 &&
                     goal_names[mob->ai_data->current_goal] && *goal_names[mob->ai_data->current_goal] != '\n')
                        ? goal_names[mob->ai_data->current_goal]
                        : "Unknown",
                    mob->ai_data->current_goal, nrm,

                    grn, nrm, yel, room_display, nrm, room_display == NOWHERE ? "(none)" : "",

                    grn, nrm, yel, mob->ai_data->goal_item_vnum, nrm,
                    mob->ai_data->goal_item_vnum == NOTHING ? "(none)" : "",

                    grn, nrm, yel, target_display, nrm, target_display == NOBODY ? "(none)" : "",

                    grn, nrm, yel, mob->ai_data->goal_timer, nrm,

                    grn, nrm, grn, nrm);

    OLC_MODE(d) = GEDIT_MAIN_MENU;
}

static void gedit_disp_goals(struct descriptor_data *d)
{
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(d, "-- Available Goals --\r\n\r\n");

    /* GOAL_NONE 0 */
    write_to_output(d, "%s0%s) %s%s%s - No required fields\r\n", grn, nrm, yel, goal_names[0], nrm);

    /* GOAL_GOTO_SHOP_TO_SELL 1 */
    write_to_output(d,
                    "%s1%s) %s%s%s - Requires: goal_destination (room), goal_obj (object), "
                    "goal_target_mob_rnum (shopkeeper)\r\n",
                    grn, nrm, yel, goal_names[1], nrm);

    /* GOAL_RETURN_TO_POST 2 */
    write_to_output(d, "%s2%s) %s%s%s - Requires: goal_destination (room)\r\n", grn, nrm, yel, goal_names[2], nrm);

    /* GOAL_HUNT_TARGET 3 */
    write_to_output(d, "%s3%s) %s%s%s - Requires: goal_target_mob_rnum (target), goal_item_vnum (optional)\r\n", grn,
                    nrm, yel, goal_names[3], nrm);

    /* GOAL_GOTO_SHOP_TO_BUY 4 */
    write_to_output(d,
                    "%s4%s) %s%s%s - Requires: goal_destination (room), goal_target_mob_rnum (shopkeeper), "
                    "goal_item_vnum (item)\r\n",
                    grn, nrm, yel, goal_names[4], nrm);

    /* GOAL_POST_QUEST 5 */
    write_to_output(d, "%s5%s) %s%s%s - Requires: goal_item_vnum (item for quest)\r\n", grn, nrm, yel, goal_names[5],
                    nrm);

    /* GOAL_GET_GOLD 6 */
    write_to_output(d, "%s6%s) %s%s%s - No required fields (uses mob AI to collect gold)\r\n", grn, nrm, yel,
                    goal_names[6], nrm);

    /* GOAL_GOTO_QUESTMASTER 7 */
    write_to_output(d, "%s7%s) %s%s%s - Requires: goal_destination (room), goal_item_vnum (item for quest)\r\n", grn,
                    nrm, yel, goal_names[7], nrm);

    /* GOAL_ACCEPT_QUEST 8 */
    write_to_output(d, "%s8%s) %s%s%s - Requires: goal_destination (room), goal_target_mob_rnum (questmaster)\r\n", grn,
                    nrm, yel, goal_names[8], nrm);

    /* GOAL_COMPLETE_QUEST 9 */
    write_to_output(d, "%s9%s) %s%s%s - No required fields (handled by quest system)\r\n", grn, nrm, yel, goal_names[9],
                    nrm);

    /* GOAL_MINE 10 */
    write_to_output(d, "%s10%s) %s%s%s - No required fields (mob must have mining skill)\r\n", grn, nrm, yel,
                    goal_names[10], nrm);

    /* GOAL_FISH 11 */
    write_to_output(d, "%s11%s) %s%s%s - No required fields (mob must have fishing skill)\r\n", grn, nrm, yel,
                    goal_names[11], nrm);

    /* GOAL_FORAGE 12 */
    write_to_output(d, "%s12%s) %s%s%s - No required fields (mob must have forage skill)\r\n", grn, nrm, yel,
                    goal_names[12], nrm);

    /* GOAL_EAVESDROP 13 */
    write_to_output(d, "%s13%s) %s%s%s - No required fields (mob must have eavesdrop skill)\r\n", grn, nrm, yel,
                    goal_names[13], nrm);

    /* GOAL_COLLECT_KEY 14 */
    write_to_output(d,
                    "%s14%s) %s%s%s - Requires: goal_destination (room), goal_item_vnum (key), "
                    "goal_target_mob_rnum (optional)\r\n",
                    grn, nrm, yel, goal_names[14], nrm);

    /* GOAL_FOLLOW 15 */
    write_to_output(d, "%s15%s) %s%s%s - No required fields (mob will follow nearby characters)\r\n", grn, nrm, yel,
                    goal_names[15], nrm);

    write_to_output(d, "\r\nEnter goal number (0-15) or Q to return to main menu: ");
    OLC_MODE(d) = GEDIT_GOAL_CHOICE;
}

void gedit_parse(struct descriptor_data *d, char *arg)
{
    int number, value;
    room_rnum rnum;

    switch (OLC_MODE(d)) {
        case GEDIT_MAIN_MENU:
            switch (*arg) {
                case 'q':
                case 'Q':
                    if (OLC_VAL(d)) {
                        write_to_output(d, "Do you wish to save your changes? : ");
                        OLC_MODE(d) = GEDIT_CONFIRM_SAVE;
                    } else {
                        cleanup_olc(d, CLEANUP_ALL);
                    }
                    return;

                case '1':
                    gedit_disp_goals(d);
                    return;

                case '2':
                    write_to_output(d, "Enter room vnum (-1 to clear): ");
                    OLC_MODE(d) = GEDIT_GOAL_ROOM;
                    return;

                case '3':
                    write_to_output(d, "Enter item vnum (-1 to clear): ");
                    OLC_MODE(d) = GEDIT_GOAL_ITEM;
                    return;

                case '4':
                    write_to_output(d, "Enter target mob vnum (-1 to clear): ");
                    OLC_MODE(d) = GEDIT_GOAL_TARGET;
                    return;

                case '5':
                    write_to_output(d, "Enter timer value (0-10000): ");
                    OLC_MODE(d) = GEDIT_GOAL_TIMER;
                    return;

                case 'l':
                case 'L':
                    gedit_disp_goals(d);
                    return;

                default:
                    gedit_disp_menu(d);
                    return;
            }

        case GEDIT_GOAL_CHOICE:
            if (*arg == 'q' || *arg == 'Q') {
                gedit_disp_menu(d);
                return;
            }

            number = atoi(arg);
            if (number < 0 || number > 15) {
                write_to_output(d, "Goal must be between 0 and 15. Try again: ");
                return;
            }

            /* When changing the goal, clear all goal-related fields to prevent
             * accessing stale pointers or invalid values from previous goals.
             * Also clear original_* fields to avoid stale key collection context. */
            OLC_MOB(d)->ai_data->current_goal = number;
            OLC_MOB(d)->ai_data->goal_obj = NULL;
            OLC_MOB(d)->ai_data->goal_destination = NOWHERE;
            OLC_MOB(d)->ai_data->goal_target_mob_rnum = NOBODY;
            OLC_MOB(d)->ai_data->goal_item_vnum = NOTHING;
            OLC_MOB(d)->ai_data->goal_timer = 0;
            /* Clear original_* fields as well */
            OLC_MOB(d)->ai_data->original_goal = -1;
            OLC_MOB(d)->ai_data->original_obj = NULL;
            OLC_MOB(d)->ai_data->original_destination = NOWHERE;
            OLC_MOB(d)->ai_data->original_target_mob = NOBODY;
            OLC_MOB(d)->ai_data->original_item_vnum = NOTHING;
            OLC_VAL(d) = TRUE;
            write_to_output(d, "Goal set to: %s (%d)\r\n", goal_names[number], number);
            write_to_output(d, "Goal parameters cleared.\r\n");

            /* Inform user which fields need to be set for this goal */
            switch (number) {
                case 0: /* GOAL_NONE */
                    write_to_output(d, "No additional parameters required.\r\n");
                    break;
                case 1: /* GOAL_GOTO_SHOP_TO_SELL */
                    write_to_output(d, "Required: Set goal room (2), goal object via AI, and target mob rnum (4)\r\n");
                    break;
                case 2: /* GOAL_RETURN_TO_POST */
                    write_to_output(d, "Required: Set goal room (2)\r\n");
                    break;
                case 3: /* GOAL_HUNT_TARGET */
                    write_to_output(d, "Required: Set target mob rnum (4). Optional: goal item vnum (3)\r\n");
                    break;
                case 4: /* GOAL_GOTO_SHOP_TO_BUY */
                    write_to_output(d, "Required: Set goal room (2), target mob rnum (4), and goal item vnum (3)\r\n");
                    break;
                case 5: /* GOAL_POST_QUEST */
                    write_to_output(d, "Required: Set goal item vnum (3)\r\n");
                    break;
                case 6: /* GOAL_GET_GOLD */
                    write_to_output(d, "No additional parameters required (uses mob AI).\r\n");
                    break;
                case 7: /* GOAL_GOTO_QUESTMASTER */
                    write_to_output(d, "Required: Set goal room (2) and goal item vnum (3)\r\n");
                    break;
                case 8: /* GOAL_ACCEPT_QUEST */
                    write_to_output(d, "Required: Set goal room (2) and target mob rnum (4)\r\n");
                    break;
                case 9: /* GOAL_COMPLETE_QUEST */
                    write_to_output(d, "No additional parameters required (handled by quest system).\r\n");
                    break;
                case 10: /* GOAL_MINE */
                    write_to_output(d, "No additional parameters required (mob must have mining skill).\r\n");
                    break;
                case 11: /* GOAL_FISH */
                    write_to_output(d, "No additional parameters required (mob must have fishing skill).\r\n");
                    break;
                case 12: /* GOAL_FORAGE */
                    write_to_output(d, "No additional parameters required (mob must have forage skill).\r\n");
                    break;
                case 13: /* GOAL_EAVESDROP */
                    write_to_output(d, "No additional parameters required (mob must have eavesdrop skill).\r\n");
                    break;
                case 14: /* GOAL_COLLECT_KEY */
                    write_to_output(
                        d, "Required: Set goal room (2) and goal item vnum (3). Optional: target mob rnum (4)\r\n");
                    break;
                case 15: /* GOAL_FOLLOW */
                    write_to_output(d, "No additional parameters required (mob will follow nearby characters).\r\n");
                    break;
                default:
                    write_to_output(d, "Set parameters as needed.\r\n");
                    break;
            }

            gedit_disp_menu(d);
            return;

        case GEDIT_GOAL_ROOM:
            value = atoi(arg);
            if (value == -1) {
                OLC_MOB(d)->ai_data->goal_destination = NOWHERE;
                write_to_output(d, "Goal room cleared.\r\n");
            } else {
                rnum = real_room(value);
                if (rnum == NOWHERE) {
                    write_to_output(d, "Room %d does not exist. Try again: ", value);
                    return;
                }
                OLC_MOB(d)->ai_data->goal_destination = rnum;
                write_to_output(d, "Goal room set to %d.\r\n", value);
            }
            OLC_VAL(d) = TRUE;
            gedit_disp_menu(d);
            return;

        case GEDIT_GOAL_ITEM:
            value = atoi(arg);
            if (value == -1) {
                OLC_MOB(d)->ai_data->goal_item_vnum = NOTHING;
                write_to_output(d, "Goal item cleared.\r\n");
            } else {
                /* TODO: Add item vnum validation if needed */
                OLC_MOB(d)->ai_data->goal_item_vnum = value;
                write_to_output(d, "Goal item set to %d.\r\n", value);
            }
            OLC_VAL(d) = TRUE;
            gedit_disp_menu(d);
            return;

        case GEDIT_GOAL_TARGET:
            value = atoi(arg);
            if (value == -1) {
                OLC_MOB(d)->ai_data->goal_target_mob_rnum = NOBODY;
                write_to_output(d, "Goal target cleared.\r\n");
            } else {
                /* Convert VNUM to RNUM */
                mob_rnum rnum = real_mobile(value);
                if (rnum == NOBODY) {
                    write_to_output(d, "Mob vnum %d does not exist. Try again: ", value);
                    return;
                }
                OLC_MOB(d)->ai_data->goal_target_mob_rnum = rnum;
                write_to_output(d, "Goal target set to mob vnum %d.\r\n", value);
            }
            OLC_VAL(d) = TRUE;
            gedit_disp_menu(d);
            return;

        case GEDIT_GOAL_TIMER:
            value = atoi(arg);
            if (value < 0 || value > 10000) {
                write_to_output(d, "Timer must be between 0 and 10000. Try again: ");
                return;
            }
            OLC_MOB(d)->ai_data->goal_timer = value;
            write_to_output(d, "Goal timer set to %d.\r\n", value);
            OLC_VAL(d) = TRUE;
            gedit_disp_menu(d);
            return;

        case GEDIT_CONFIRM_SAVE:
            switch (*arg) {
                case 'y':
                case 'Y':
                    gedit_save_internally(d);
                    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits goals for %s (%d)",
                           GET_NAME(d->character), GET_NAME(OLC_MOB(d)), GET_MOB_VNUM(OLC_MOB(d)));
                    write_to_output(d, "Goal changes saved.\r\n");
                    break;
                case 'n':
                case 'N':
                    write_to_output(d, "Goal changes discarded.\r\n");
                    break;
                default:
                    write_to_output(d, "Invalid choice! Please enter 'Y' or 'N': ");
                    return;
            }
            cleanup_olc(d, CLEANUP_ALL);
            return;

        default:
            mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: gedit_parse: Reached default case!");
            break;
    }
}

static void gedit_save_internally(struct descriptor_data *d)
{
    /* Goal data is already saved directly to the mob instance,
       so we don't need to do anything special here.
       The changes are immediately live. */
}