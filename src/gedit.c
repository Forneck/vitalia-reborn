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
}

static void gedit_disp_menu(struct descriptor_data *d)
{
    struct char_data *mob;

    mob = OLC_MOB(d);
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(
        d,
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
        (mob->ai_data->current_goal >= 0 && mob->ai_data->current_goal <= 9 && goal_names[mob->ai_data->current_goal] &&
         *goal_names[mob->ai_data->current_goal] != '\n')
            ? goal_names[mob->ai_data->current_goal]
            : "Unknown",
        mob->ai_data->current_goal, nrm,

        grn, nrm, yel, mob->ai_data->goal_destination, nrm, mob->ai_data->goal_destination == NOWHERE ? "(none)" : "",

        grn, nrm, yel, mob->ai_data->goal_item_vnum, nrm, mob->ai_data->goal_item_vnum == NOTHING ? "(none)" : "",

        grn, nrm, yel, mob->ai_data->goal_target_mob_rnum, nrm,
        mob->ai_data->goal_target_mob_rnum == NOBODY ? "(none)" : "",

        grn, nrm, yel, mob->ai_data->goal_timer, nrm,

        grn, nrm, grn, nrm);

    OLC_MODE(d) = GEDIT_MAIN_MENU;
}

static void gedit_disp_goals(struct descriptor_data *d)
{
    int i;

    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(d, "-- Available Goals --\r\n\r\n");

    for (i = 0; i <= 9; i++) {
        write_to_output(d, "%s%d%s) %s%s%s\r\n", grn, i, nrm, yel, goal_names[i], nrm);
    }

    write_to_output(d, "\r\nEnter goal number (0-9) or Q to return to main menu: ");
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
                    write_to_output(d, "Enter target mob rnum (-1 to clear): ");
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
            if (number < 0 || number > 9) {
                write_to_output(d, "Goal must be between 0 and 9. Try again: ");
                return;
            }

            OLC_MOB(d)->ai_data->current_goal = number;
            OLC_VAL(d) = TRUE;
            write_to_output(d, "Goal set to: %s (%d)\r\n", goal_names[number], number);
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
                /* TODO: Add mob rnum validation if needed */
                OLC_MOB(d)->ai_data->goal_target_mob_rnum = value;
                write_to_output(d, "Goal target set to %d.\r\n", value);
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