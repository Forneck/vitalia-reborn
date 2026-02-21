/**************************************************************************
 *  File: act.movement.c                                    Part of tbaMUD *
 *  Usage: Movement commands, door handling, & sleep/rest/etc state.       *
 *                                                                         *
 *  All rights reserved.  See license complete information.                *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "fight.h"
#include "oasis.h" /* for buildwalk */
#include "ann.h"
#include "quest.h"
#include "mud_event.h"
#include "dg_event.h"

/* local only functions */
/* do_simple_move utility functions */
/* do_gen_door utility functions */
static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd);
static int ok_jam(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd);

/* simple function to determine if char can walk on water */
int has_boat(struct char_data *ch)
{
    struct obj_data *obj;
    int i;

    if (GET_LEVEL(ch) > LVL_IMMORT)
        return (1);

    if (AFF_FLAGGED(ch, AFF_WATERWALK) || AFF_FLAGGED(ch, AFF_FLYING))
        return (1);

    if (IS_DEAD(ch))
        return (1);

    /* non-wearable boats in inventory will do it */
    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
            return (1);

    /* and any boat you're wearing will do it too */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
            return (1);

    return (0);
}

/* Simple function to determine if char can fly. */
int has_flight(struct char_data *ch)
{
    struct obj_data *obj;
    int i;

    if (GET_LEVEL(ch) > LVL_IMMORT)
        return (1);

    if (AFF_FLAGGED(ch, AFF_FLYING))
        return (1);

    if (MOB_FLAGGED(ch, MOB_CAN_FLY))
        return (1);

    if (AFF_FLAGGED(ch, AFF_LIGHT))
        return (1);

    if (IS_DEAD(ch))
        return (1);

    /* Non-wearable flying items in inventory will do it. */
    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (OBJAFF_FLAGGED(obj, AFF_FLYING))
            return (1);

    /* Any equipped objects with AFF_FLYING will do it too. */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_FLYING))
            return (1);

    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WINGS)
            return (1);

    return (0);
}

/* Simple function to determine if char can scuba. */
int has_scuba(struct char_data *ch)
{
    struct obj_data *obj;
    int i;

    if (GET_LEVEL(ch) > LVL_IMMORT)
        return (1);

    if (IS_DEAD(ch))
        return (1);

    if (AFF_FLAGGED(ch, AFF_BREATH))
        return (1);

    /* Non-wearable scuba items in inventory will do it. */
    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (OBJAFF_FLAGGED(obj, AFF_BREATH) && (find_eq_pos(ch, obj, NULL) < 0))
            return (1);

    /* Any equipped objects with AFF_BREATH will do it too. */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_BREATH))
            return (1);

    return (0);
}

/** Move a PC/NPC character from their current location to a new location. This
 * is the standard movement locomotion function that all normal walking
 * movement by characters should be sent through. This function also defines
 * the move cost of normal locomotion as:
 * ( (move cost for source room) + (move cost for destination) ) / 2
 *
 * @pre Function assumes that ch has no master controlling character, that
 * ch has no followers (in other words followers won't be moved by this
 * function) and that the direction traveled in is one of the valid, enumerated
 * direction.
 * @param ch The character structure to attempt to move.
 * @param dir The defined direction (NORTH, SOUTH, etc...) to attempt to
 * move into.
 * @param need_specials_check If TRUE will cause
 * @retval int 1 for a successful move (ch is now in a new location)
 * or 0 for a failed move (ch is still in the original location). */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
    /* Begin Local variable definitions */
    /*---------------------------------------------------------------------*/
    /* Used in our special proc check. By default, we pass a NULL argument
       when checking for specials */
    char spec_proc_args[MAX_INPUT_LENGTH] = "";
    /* The room the character is currently in and will move from... */
    room_rnum was_in = IN_ROOM(ch);
    /* ... and the room the character will move into. */
    room_rnum going_to;

    /* Validate exit before accessing to_room to prevent segfault */
    if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE) {
        log1("SYSERR: do_simple_move: ch=%s attempting invalid move to dir=%d", GET_NAME(ch), dir);
        return 0;
    }

    going_to = EXIT(ch, dir)->to_room;
    /* How many movement points are required to travel from was_in to
       going_to. We redefine this later when we need it. */
    int need_movement = 0;
    /* Contains the "leave" message to display to the was_in room. */
    char leave_message[SMALL_BUFSIZE];
    int i, attempt; /*---------------------------------------------------------------------*/
    /* End Local variable definitions */

    /* Begin checks that can prevent a character from leaving the was_in room.
     */
    /* Future checks should be implemented within this section and return 0.  */
    /*---------------------------------------------------------------------*/
    /* Check for special routines that might activate because of the move and
       also might prevent the movement. Special requires commands, so we pass
       in the "command" equivalent of the direction (ie. North is '1' in the
       command list, but NORTH is defined as '0'). Note -- only check if
       following; this avoids 'double spec-proc' bug */
    if (need_specials_check && special(ch, dir + 1, spec_proc_args))
        return 0;

    /* Leave Trigger Checks: Does a leave trigger block exit from the room? */
    if (!leave_mtrigger(ch, dir) ||
        IN_ROOM(ch) != was_in) /* prevent
                                                                                          teleport
                                                                                          crashes */
        return 0;
    if (!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir) ||
        IN_ROOM(ch) != was_in) /* prevent
                                                                                                          teleport
                                                                                                          crashes
                                                                                                        */
        return 0;
    if (!leave_otrigger(&world[IN_ROOM(ch)], ch, dir) ||
        IN_ROOM(ch) != was_in) /* prevent
                                                                                                          teleport
                                                                                                          crashes
                                                                                                        */
        return 0;

    /* Charm effect: Does it override the movement? */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && was_in == IN_ROOM(ch->master)) {
        send_to_char(ch, "A idéia de deixar seu mestre faz você chorar.\r\n");
        act("$n desata a chorar.", FALSE, ch, 0, 0, TO_ROOM);
        return (0);
    }

    /* Whirlwind: Cancel whirlwind on movement attempt
     * Only check if character has events to avoid expensive lookup for most characters */
    if (ch && ch->events) {
        struct mud_event_data *pMudEvent = char_has_mud_event(ch, eWHIRLWIND);
        if (pMudEvent && pMudEvent->pEvent) {
            event_cancel(pMudEvent->pEvent);
            send_to_char(ch, "Você para de girar ao tentar se mover.\r\n");
            return (0);
        }
    }

    if (SECT(was_in) == SECT_ICE && !FLYING(ch) && !rand_number(0, 2)) {
        act("Você escorrega no gelo e cai sentad$r.", FALSE, ch, 0, 0, TO_CHAR);
        GET_POS(ch) = POS_SITTING;
        return (0);
    }

    /* Water, No Swimming Rooms: Does the deep water prevent movement? */
    if ((SECT(was_in) == SECT_WATER_NOSWIM) || (SECT(going_to) == SECT_WATER_NOSWIM)) {
        if (!has_boat(ch)) {
            send_to_char(ch, "Você precisa de um bote para ir para lá.\r\n");
            return (0);
        }
    }

    /* Flying Required: Does lack of flying prevent movement? */
    if ((SECT(was_in) == SECT_FLYING) || (SECT(going_to) == SECT_FLYING)) {
        if (!has_flight(ch)) {
            send_to_char(ch, "Você precisa estar voando para ir lá!\r\n");
            return (0);
        }
    }

    if ((SECT(was_in) == SECT_AIR_FLOW) && !rand_number(0, 10) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        for (i = 0; i < 6; i++) {
            attempt = rand_number(0, DIR_COUNT - 1); /* Select a random
                                                                                                direction */
            if (CAN_GO(ch, attempt) && VALID_ROOM_RNUM(EXIT(ch, attempt)->to_room) &&
                !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
                going_to = EXIT(ch, attempt)->to_room;
                send_to_char(ch, "Você foi carregad%s pela corrente de ar.", OA(ch));
                act("$n foi carregad$r pela corrente de ar.", FALSE, ch, 0, 0, TO_ROOM);
                break;
            }
        }
    }

    if ((ROOM_FLAGGED(going_to, ROOM_NO_FLY) || SECT(going_to) == SECT_UNDERWATER) && AFF_FLAGGED(ch, AFF_FLYING)) {
        send_to_char(ch, "Você não pode ir nesta sala voando.\r\n");
        return (0);
    }
    /* Underwater Room: Does lack of underwater breathing prevent movement? */
    if ((SECT(was_in) == SECT_UNDERWATER) || (SECT(going_to) == SECT_UNDERWATER)) {
        if (!has_scuba(ch) && !IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
            send_to_char(ch, "Você precisa respirar em baixo d'agua para ir lá!\r\n");
            return (0);
        }
    }

    /* Houses: Can the player walk into the house? */
    if (ROOM_FLAGGED(was_in, ROOM_ATRIUM) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        if (!House_can_enter(ch, GET_ROOM_VNUM(going_to))) {
            send_to_char(ch, "Propriedade privada -- não ultrapasse!\r\n");
            return (0);
        }
    }

    /* Check zone level recommendations */
    if ((ZONE_MINLVL(GET_ROOM_ZONE(going_to)) != -1) && ZONE_MINLVL(GET_ROOM_ZONE(going_to)) > GET_LEVEL(ch)) {
        send_to_char(ch, "Você está entrando em uma área está acima do seu nível recomendado.\r\n");
    }

    /* Check zone flag restrictions */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_CLOSED)) {
        send_to_char(ch, "Uma força misteriosa te empurra de volta! Esta área é fora dos limites!\r\n");
        return (0);
    }
    if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_NOIMMORT) && (GET_LEVEL(ch) >= LVL_IMMORT) &&
        (GET_LEVEL(ch) < LVL_GRGOD)) {
        send_to_char(ch, "Uma força misteriosa te empurra de volta! Esta área é fora dos limites!\r\n");
        return (0);
    }

    /* Room Size Capacity: Is the room full of people already? */
    if (ROOM_FLAGGED(going_to, ROOM_TUNNEL) && num_pc_in_room(&(world[going_to])) >= CONFIG_TUNNEL_SIZE &&
        GET_LEVEL(ch) < LVL_IMMORT) {
        if (CONFIG_TUNNEL_SIZE > 1)
            send_to_char(ch, "Não há espaço suficiente para mais ninguém na sala!\r\n");
        else
            send_to_char(ch, "Não há espaço suficiente para mais de uma pessoa na sala!\r\n");
        return (0);
    }

    if (ROOM_FLAGGED(going_to, ROOM_PRIVATE) && num_pc_in_room(&(world[going_to])) >= 2 && GET_LEVEL(ch) < LVL_GRGOD)
        send_to_char(ch, "Há uma conversa privada ocorrendo nesta sala.\r\n");

    /* Room Level Requirements: Is ch privileged enough to enter the room? */
    if (ROOM_FLAGGED(going_to, ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GOD) {
        send_to_char(ch, "Você não tem permissão para usar esta sala!");
        return (0);
    }

    /* All checks passed, nothing will prevent movement now other than lack of
       move points. */
    /* move points needed is avg. move loss for src and destination sect type */
    need_movement = (movement_loss[SECT(was_in)] + movement_loss[SECT(going_to)]) / 2;

    if (IS_DEAD(ch) || AFF_FLAGGED(ch, AFF_LIGHT))
        need_movement = 0;
    else if (AFF_FLAGGED(ch, AFF_FLYING))
        need_movement = 1;

    /* Move Point Requirement Check */
    if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
        if (need_specials_check && ch->master)
            send_to_char(ch, "Você está muito cansad%s para seguir.\r\n", OA(ch));
        else
            send_to_char(ch, "Você está muito cansad%s.\r\n", OA(ch));

        return (0);
    }

    /*---------------------------------------------------------------------*/
    /* End checks that can prevent a character from leaving the was_in room. */

    /* Begin: the leave operation. */
    /*---------------------------------------------------------------------*/
    /* If applicable, subtract movement cost. */
    if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch))
        GET_MOVE(ch) -= need_movement;

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) {
        if ((EXIT(ch, dir))->keyword) {
            send_to_char(ch, "Você atravessa %s com facilidade.\r\n\n", fname(EXIT(ch, dir)->keyword));
        } else
            send_to_char(ch, "Você atravessa o caminho com facilidade.\r\n\n");
    }

    /* Generate the leave message and display to others in the was_in room. */
    if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
        snprintf(leave_message, sizeof(leave_message), "$n segue para %s.", dirs_pt[dir]);
        act(leave_message, TRUE, ch, 0, 0, TO_ROOM);
    }

    char_from_room(ch);
    char_to_room(ch, going_to);
    /*---------------------------------------------------------------------*/
    /* End: the leave operation. The character is now in the new room. */

    /* Begin: Post-move operations. */
    /*---------------------------------------------------------------------*/
    /* Post Move Trigger Checks: Check the new room for triggers. Assumptions:
       The character has already truly left the was_in room. If the entry
       trigger "prevents" movement into the room, it is the triggers job to
       provide a message to the original was_in room. */
    if (!entry_mtrigger(ch) || !enter_wtrigger(&world[going_to], ch, dir)) {
        char_from_room(ch);
        char_to_room(ch, was_in);
        return 0;
    }

    /* Display arrival information to anyone in the destination room... */
    if (!AFF_FLAGGED(ch, AFF_SNEAK))
        act("$n chegou.", TRUE, ch, 0, 0, TO_ROOM);

    /* ... and the room description to the character. */
    if (ch->desc != NULL)
        look_at_room(ch, 0);

    /* Autoquest trigger checks: Must be after look_at_room so quest messages appear after room description */
    if (!IS_NPC(ch)) {
        autoquest_trigger_check(ch, 0, 0, AQ_ROOM_FIND);
        autoquest_trigger_check(ch, 0, 0, AQ_MOB_FIND);
    } else {
        /* Mob quest trigger checks */
        mob_autoquest_trigger_check(ch, NULL, NULL, AQ_ROOM_FIND);
        mob_autoquest_trigger_check(ch, NULL, NULL, AQ_MOB_FIND);
        mob_autoquest_trigger_check(ch, NULL, NULL, AQ_ROOM_CLEAR);
        mob_autoquest_trigger_check(ch, NULL, NULL, AQ_MAGIC_GATHER);
    }

    /* Emotion trigger: Entering dangerous or safe areas (Environmental 2.2) */
    if (IS_NPC(ch) && ch->ai_data && CONFIG_MOB_CONTEXTUAL_SOCIALS) {
        /* Check for dangerous areas - death traps, dangerous sectors, high-level zones */
        if (ROOM_FLAGGED(going_to, ROOM_DEATH) || SECT(going_to) == SECT_LAVA || SECT(going_to) == SECT_QUICKSAND ||
            SECT(going_to) == SECT_UNDERWATER) {
            update_mob_emotion_entered_dangerous_area(ch);
        }
        /* Check for safe areas - peaceful rooms, cities, indoors */
        else if (ROOM_FLAGGED(going_to, ROOM_PEACEFUL) || ROOM_FLAGGED(going_to, ROOM_HEAL) ||
                 SECT(going_to) == SECT_CITY || SECT(going_to) == SECT_INSIDE) {
            /* Only trigger if coming from a dangerous area */
            if (!ROOM_FLAGGED(was_in, ROOM_PEACEFUL) && !ROOM_FLAGGED(was_in, ROOM_HEAL) && SECT(was_in) != SECT_CITY &&
                SECT(was_in) != SECT_INSIDE) {
                update_mob_emotion_entered_safe_area(ch);
            }
        }
    }

    /* ... and Kill the player if the room is a death trap. */
    if (ROOM_FLAGGED(going_to, ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMMORT) {
        mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(going_to),
               world[going_to].name);
        GET_DTS(ch)++;
        death_cry(ch);

        /* Mob emotion updates and mourning for death trap deaths (experimental feature) */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS) {
            /* Notify mobs in the room about the death trap death */
            struct char_data *witness, *next_witness;
            bool should_mourn;

            /* Use safe iteration pattern since act() and socials can trigger extraction */
            for (witness = world[going_to].people; witness; witness = next_witness) {
                next_witness = witness->next_in_room;

                if (!IS_NPC(witness) || !witness->ai_data || witness == ch)
                    continue;

                /* Check if witness should mourn */
                should_mourn = FALSE;

                /* Group members mourn */
                if (GROUP(witness) && GROUP(ch) && GROUP(witness) == GROUP(ch)) {
                    should_mourn = TRUE;
                }
                /* Same alignment with decent relationship */
                else if (IS_NPC(ch) && GET_ALIGNMENT(witness) * GET_ALIGNMENT(ch) > 0 &&
                         witness->ai_data->emotion_friendship >= 40) {
                    should_mourn = TRUE;
                }
                /* High compassion mobs mourn anyone non-evil */
                else if (witness->ai_data->emotion_compassion >= 70 && !IS_EVIL(ch)) {
                    should_mourn = TRUE;
                }

                if (should_mourn) {
                    mob_mourn_death(witness, ch);

                    /* Safety check: mob_mourn_death can trigger extraction via socials/scripts */
                    if (MOB_FLAGGED(witness, MOB_NOTDEADYET) || PLR_FLAGGED(witness, PLR_NOTDEADYET))
                        continue;

                    /* Revalidate ai_data after potential extraction */
                    if (!witness->ai_data)
                        continue;
                }

                /* Update witness emotions based on the death */
                if (IS_NPC(ch) && witness->ai_data) {
                    /* Witnessing mob death in trap: ally case already handled inside mob_mourn_death above. */
                    if (GROUP(witness) && GROUP(ch) && GROUP(witness) == GROUP(ch)) {
                        /* Safety check: emotion update shouldn't cause extraction, but verify */
                        if (MOB_FLAGGED(witness, MOB_NOTDEADYET) || PLR_FLAGGED(witness, PLR_NOTDEADYET))
                            continue;

                        /* Extra fear from death trap */
                        if (witness->ai_data) {
                            witness->ai_data->emotion_fear =
                                URANGE(0, witness->ai_data->emotion_fear + rand_number(10, 20), 100);
                        }
                    }
                }
            }
        }

        extract_char(ch);
        return (0);
    }

    /* At this point, the character is safe and in the room. */
    /* Fire memory and greet triggers, check and see if the greet trigger
       prevents movement, and if so, move the player back to the previous
       room. */
    entry_memory_mtrigger(ch);
    if (!greet_mtrigger(ch, dir)) {
        char_from_room(ch);
        char_to_room(ch, was_in);
        look_at_room(ch, 0);
        /* Failed move, return a failure */
        return (0);
    } else
        greet_memory_mtrigger(ch);
    /*---------------------------------------------------------------------*/
    /* End: Post-move operations. */

    /* Only here is the move successful *and* complete. Return success for
       calling functions to handle post move operations. */
    return (1);
}

int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
    room_rnum was_in;
    struct follow_type *k, *next;

    if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
        return (0);
    else if (!CONFIG_DIAGONAL_DIRS && IS_DIAGONAL(dir))
        send_to_char(ch, "Ah, você não pode ir para esta direção...\r\n");
    else if (AFF_FLAGGED(ch, AFF_PARALIZE))
        send_to_char(ch, "Você não pode se mover!\r\n");
    else if ((!EXIT(ch, dir) && !buildwalk(ch, dir)) || EXIT(ch, dir)->to_room == NOWHERE)
        send_to_char(ch, "Ah, você não pode ir para esta direção...\r\n");
    else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) &&
             (GET_LEVEL(ch) < LVL_IMMORT || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)))) {
        if (EXIT(ch, dir)->keyword)
            send_to_char(ch, "%s parece estar fechado.\r\n", fname(EXIT(ch, dir)->keyword));
        else
            send_to_char(ch, "Este caminho parece estar fechado.\r\n");
    } else {
        if (!ch->followers)
            return (do_simple_move(ch, dir, need_specials_check));

        was_in = IN_ROOM(ch);
        if (!do_simple_move(ch, dir, need_specials_check))
            return (0);

        for (k = ch->followers; k; k = next) {
            next = k->next;
            if ((IN_ROOM(k->follower) == was_in) && (GET_POS(k->follower) >= POS_STANDING)) {
                act("Você segue $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
                perform_move(k->follower, dir, 1);
            }
        }
        return (1);
    }
    return (0);
}

ACMD(do_move)
{
    /* These subcmd defines are mapped precisely to the direction defines. */
    perform_move(ch, subcmd, 0);
}

static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
    int door;

    if (*dir) {                                                       /* a direction was specified */
        if ((door = search_block(dir, dirs, FALSE)) == -1) {          /* Partial Match */
            if ((door = search_block(dir, autoexits, FALSE)) == -1) { /* Check 'short' dirs too */
                send_to_char(ch, "Isso não é uma direção.\r\n");
                return (-1);
            }
        }
        if (EXIT(ch, door)) { /* Braces added according to indent. -gg */
            if (EXIT(ch, door)->keyword) {
                if (is_name(type, EXIT(ch, door)->keyword))
                    return (door);
                else {
                    send_to_char(ch, "Você não vê %s aqui.\r\n", type);
                    return (-1);
                }
            } else
                return (door);
        } else {
            send_to_char(ch, "Você não vê nada que possa %s aqui.\r\n", cmdname);
            return (-1);
        }
    } else { /* try to locate the keyword */
        if (!*type) {
            send_to_char(ch, "O que você deseja %s?\r\n", cmdname);
            return (-1);
        }
        for (door = 0; door < DIR_COUNT; door++) {
            if (EXIT(ch, door)) {
                if (EXIT(ch, door)->keyword) {
                    if (isname(type, EXIT(ch, door)->keyword)) {
                        if ((!IS_NPC(ch)) && (!PRF_FLAGGED(ch, PRF_AUTODOOR)))
                            return door;
                        else if (is_abbrev(cmdname, "open")) {
                            if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
                                return door;
                            else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
                                return door;
                        } else if ((is_abbrev(cmdname, "close")) && (!(IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))))
                            return door;
                        else if ((is_abbrev(cmdname, "lock")) && (!(IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))))
                            return door;
                        else if ((is_abbrev(cmdname, "unlock")) && (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)))
                            return door;
                        else if ((is_abbrev(cmdname, "pick")) && (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)))
                            return door;
                        else {
                            /* Door found with matching keyword but doesn't meet command requirements.
                             * Return it anyway to allow do_gen_door to handle appropriate error messages. */
                            return door;
                        }
                    }
                }
            }
        }

        if ((!IS_NPC(ch)) && (!PRF_FLAGGED(ch, PRF_AUTODOOR)))
            send_to_char(ch, "Não há %s nesta direção.\r\n", type);
        else if (is_abbrev(cmdname, "open"))
            send_to_char(ch, "Não há %s nesta direção que você possa abrir.\r\n", type);
        else if (is_abbrev(cmdname, "close"))
            send_to_char(ch, "Não há %s nesta direção que você possa fechar.\r\n", type);
        else if (is_abbrev(cmdname, "lock"))
            send_to_char(ch, "Não há %s nesta direção que você possa trancar.\r\n", type);
        else if (is_abbrev(cmdname, "unlock"))
            send_to_char(ch, "Não há %s nesta direção que você possa destrancar.\r\n", type);
        else
            send_to_char(ch, "Não há %s nesta direção que você possa arrombar.\r\n", type);

        return (-1);
    }
}

int has_key(struct char_data *ch, obj_vnum key)
{
    struct obj_data *o;

    for (o = ch->carrying; o; o = o->next_content)
        if (GET_OBJ_VNUM(o) == key)
            return (1);

    if (GET_EQ(ch, WEAR_HOLD))
        if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
            return (1);

    return (0);
}

#define NEED_OPEN (1 << 0)
#define NEED_CLOSED (1 << 1)
#define NEED_UNLOCKED (1 << 2)
#define NEED_LOCKED (1 << 3)
#define JAM_DISLODGE_CHANCE 10 /* 10% chance per attempt to dislodge jam */

/* cmd_door is required external from act.movement.c */
const char *cmd_door[] = {"open", "close", "unlock", "lock", "pick", "jam"};

static const int flags_door[] = {NEED_CLOSED | NEED_UNLOCKED, NEED_OPEN,
                                 NEED_CLOSED | NEED_LOCKED,   NEED_CLOSED | NEED_UNLOCKED,
                                 NEED_CLOSED | NEED_LOCKED,   NEED_CLOSED};

#define EXITN(room, door) (world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)                                                                                     \
    ((obj) ? (REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) : (REMOVE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(room, obj, door)                                                                                    \
    ((obj) ? (SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) : (SET_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)                                                                                     \
    ((obj) ? (SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) : (SET_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define UNLOCK_DOOR(room, obj, door)                                                                                   \
    ((obj) ? (REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) : (REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define TOGGLE_LOCK(room, obj, door)                                                                                   \
    ((obj) ? (TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) : (TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
/* Jamming only applies to room exits (doors), not containers - containers lack the structure for jamming */
#define JAM_DOOR(room, obj, door) ((obj) ? (0) : (SET_BIT(EXITN(room, door)->exit_info, EX_JAMMED)))
#define UNJAM_DOOR(room, obj, door) ((obj) ? (0) : (REMOVE_BIT(EXITN(room, door)->exit_info, EX_JAMMED)))

/* Helper function to unjam a door and its opposite side */
static void unjam_door_both_sides(struct char_data *ch, int door)
{
    room_rnum other_room;
    struct room_direction_data *back;

    REMOVE_BIT(EXITN(IN_ROOM(ch), door)->exit_info, EX_JAMMED);

    other_room = EXIT(ch, door)->to_room;
    if (other_room != NOWHERE) {
        back = world[other_room].dir_option[rev_dir[door]];
        if (back && back->to_room == IN_ROOM(ch))
            REMOVE_BIT(EXITN(other_room, rev_dir[door])->exit_info, EX_JAMMED);
    }
}

void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
    char buf[MAX_STRING_LENGTH];
    size_t len;
    room_rnum other_room = NOWHERE;
    struct room_direction_data *back = NULL;

    if (!door_mtrigger(ch, scmd, door))
        return;

    if (!door_wtrigger(ch, scmd, door))
        return;

    len = snprintf(buf, sizeof(buf), "$n %s ", doors_pt[scmd]);
    if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
        if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
            if (back->to_room != IN_ROOM(ch))
                back = NULL;

    switch (scmd) {
        case SCMD_OPEN:
            OPEN_DOOR(IN_ROOM(ch), obj, door);
            if (back)
                OPEN_DOOR(other_room, obj, rev_dir[door]);
            send_to_char(ch, "%s", CONFIG_OK);
            break;

        case SCMD_CLOSE:
            CLOSE_DOOR(IN_ROOM(ch), obj, door);
            if (back)
                CLOSE_DOOR(other_room, obj, rev_dir[door]);
            send_to_char(ch, "%s", CONFIG_OK);
            break;

        case SCMD_LOCK:
            LOCK_DOOR(IN_ROOM(ch), obj, door);
            if (back)
                LOCK_DOOR(other_room, obj, rev_dir[door]);
            send_to_char(ch, "*Click*\r\n");
            break;

        case SCMD_UNLOCK:
            UNLOCK_DOOR(IN_ROOM(ch), obj, door);
            if (back)
                UNLOCK_DOOR(other_room, obj, rev_dir[door]);
            send_to_char(ch, "*Click*\r\n");
            break;

        case SCMD_PICK:
            TOGGLE_LOCK(IN_ROOM(ch), obj, door);
            if (back)
                TOGGLE_LOCK(other_room, obj, rev_dir[door]);
            send_to_char(ch, "A fechadura facilmente rende-se às suas habilidades.\r\n");
            len = strlcpy(buf, "$n habilmente arromba a fechadura ", sizeof(buf));
            break;

        case SCMD_JAM:
            JAM_DOOR(IN_ROOM(ch), obj, door);
            if (back)
                JAM_DOOR(other_room, obj, rev_dir[door]);
            send_to_char(ch, "Você habilmente trava a fechadura com uma cunha.\r\n");
            len = strlcpy(buf, "$n habilmente trava a fechadura ", sizeof(buf));
            break;
    }

    /* Notify the room. */
    if (len < sizeof(buf))
        snprintf(buf + len, sizeof(buf) - len, "%s%s.", obj ? "" : " ",
                 obj                       ? "$p"
                 : EXIT(ch, door)->keyword ? "$F"
                                           : dirs_pt[door]);
    if (!obj || IN_ROOM(obj) != NOWHERE)
        act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

    /* Notify the other room */
    if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE))
        send_to_room(EXIT(ch, door)->to_room, "Você assiste %s ser %s pelo outro lado.\r\n",
                     back->keyword ? fname(back->keyword) : dirs_pt[rev_dir[door]],
                     scmd == SCMD_OPEN    ? "aberta"
                     : scmd == SCMD_CLOSE ? "fechada"
                     : scmd == SCMD_LOCK  ? "trancada"
                                          : "arrombada");
}

static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
{
    int percent, skill_lvl;

    if (scmd != SCMD_PICK)
        return (1);

    /* Check if the player has the pick lock skill */
    if (GET_SKILL(ch, SKILL_PICK_LOCK) == 0) {
        send_to_char(ch, "Você não sabe como arrombar fechaduras.\r\n");
        return (0);
    }

    percent = rand_number(1, 101);
    skill_lvl = GET_SKILL(ch, SKILL_PICK_LOCK) + dex_app_skill[GET_DEX(ch)].p_locks;

    if (keynum == NOTHING)
        send_to_char(ch, "Estranho - você não parece encontrar o buraco da fechadura.\r\n");
    else if (pickproof)
        send_to_char(ch, "Estranho - você não parece encontrar o buraco da fechadura.\r\n");
    else if ((percent > skill_lvl) || PLR_FLAGGED(ch, PLR_TRNS))
        send_to_char(ch, "Você falhou em destravar a fechadura.\r\n");
    else
        return (1);

    return (0);
}

static int ok_jam(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
{
    int percent, skill_lvl;

    if (scmd != SCMD_JAM)
        return (1);

    /* Check if the player has the jam lock skill */
    if (GET_SKILL(ch, SKILL_JAM_LOCK) == 0) {
        send_to_char(ch, "Você não sabe como travar fechaduras.\r\n");
        return (0);
    }

    percent = rand_number(1, 101);
    skill_lvl = GET_SKILL(ch, SKILL_JAM_LOCK) + dex_app_skill[GET_DEX(ch)].p_locks;

    if (keynum == NOTHING)
        send_to_char(ch, "Não há fechadura para travar.\r\n");
    else if (pickproof)
        send_to_char(ch, "A fechadura é muito complexa para ser travada.\r\n");
    else if ((percent > skill_lvl) || PLR_FLAGGED(ch, PLR_TRNS))
        send_to_char(ch, "Você falhou em travar a fechadura.\r\n");
    else
        return (1);

    return (0);
}

#define DOOR_IS_OPENABLE(ch, obj, door)                                                                                \
    ((obj) ? ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))                            \
           : (EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door)                                                                                    \
    ((obj) ? (!OBJVAL_FLAGGED(obj, CONT_CLOSED)) : (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)                                                                                \
    ((obj) ? (!OBJVAL_FLAGGED(obj, CONT_LOCKED)) : (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door)                                                                               \
    ((obj) ? (OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_JAMMED(ch, obj, door) ((obj) ? (0) : (EXIT_FLAGGED(EXIT(ch, door), EX_JAMMED)))
#define DOOR_IS_CLOSED(ch, obj, door) (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door) (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door) ((obj) ? (GET_OBJ_VAL(obj, 2)) : (EXIT(ch, door)->key))

ACMD(do_gen_door)
{
    int door = -1;
    obj_vnum keynum;
    char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
    struct obj_data *obj = NULL;
    struct char_data *victim = NULL;

    skip_spaces(&argument);

    if (!ALIVE(ch)) {
        act("Como??? Você está mort$r!!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!*argument) {
        send_to_char(ch, "%c%s Oque?\r\n", UPPER(*doors_pt[subcmd]), doors_pt[subcmd] + 1);
        return;
    }
    two_arguments(argument, type, dir);
    if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
        door = find_door(ch, type, dir, doors_pt[subcmd]);

    if ((obj) && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)) {
        obj = NULL;
        door = find_door(ch, type, dir, doors_pt[subcmd]);
    }

    /* Jamming only works on doors, not containers */
    if ((obj) && (subcmd == SCMD_JAM)) {
        send_to_char(ch, "Você não pode travar isso!\r\n");
        return;
    }

    if ((obj) || (door >= 0)) {
        keynum = DOOR_KEY(ch, obj, door);
        int is_pickproof;
        int is_jammed;
        is_pickproof = DOOR_IS_PICKPROOF(ch, obj, door);
        is_jammed = DOOR_IS_JAMMED(ch, obj, door);

        if (!(DOOR_IS_OPENABLE(ch, obj, door)))
            send_to_char(ch, "Você não pode %s  isso!!\r\n", doors_pt[subcmd]);
        else if (!DOOR_IS_OPEN(ch, obj, door) && IS_SET(flags_door[subcmd], NEED_OPEN))
            send_to_char(ch, "Mas já está fechado!\r\n");
        else if (!DOOR_IS_CLOSED(ch, obj, door) && IS_SET(flags_door[subcmd], NEED_CLOSED))
            send_to_char(ch, "Mas já está aberto!\r\n");
        else if (is_jammed && IS_SET(flags_door[subcmd], NEED_UNLOCKED)) {
            send_to_char(ch, "A fechadura está travada e não pode ser aberta!\r\n");
            /* Chance to dislodge the jam with repeated attempts */
            if (!obj && door >= 0 && rand_number(1, 100) <= JAM_DISLODGE_CHANCE) {
                unjam_door_both_sides(ch, door);
                send_to_char(ch, "A cunha se solta e cai no chão com um *clique*!\r\n");
                act("A cunha da fechadura $F se solta e cai no chão!", FALSE, ch, 0,
                    EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : dirs_pt[door], TO_ROOM);
            }
        } else if (is_jammed && (subcmd == SCMD_JAM))
            send_to_char(ch, "A fechadura já está travada!\r\n");
        else if (!(DOOR_IS_LOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_LOCKED))
            send_to_char(ch, "Oh.. não estava trancada, depois de tudo..\r\n");
        else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
                 ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (has_key(ch, keynum))) {
            send_to_char(ch, "Parece estar trancado, mas você tem a chave.\r\n");
            do_doorcmd(ch, obj, door, SCMD_UNLOCK);
            do_doorcmd(ch, obj, door, subcmd);
        } else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
                   ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (!has_key(ch, keynum))) {
            send_to_char(ch, "Você não parece ter a chave apropriada.\r\n");
        } else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
                   (GET_LEVEL(ch) < LVL_IMMORT || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))))
            send_to_char(ch, "Parece estar trancado.\r\n");
        else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
                 ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
            send_to_char(ch, "Você não parece ter a chave apropriada.\r\n");
        else if (ok_pick(ch, keynum, is_pickproof, subcmd) && ok_jam(ch, keynum, is_pickproof, subcmd))
            do_doorcmd(ch, obj, door, subcmd);
    }
    return;
}

ACMD(do_bash_door)
{
    int door = -1;
    char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
    struct obj_data *obj = NULL;
    struct char_data *victim = NULL;
    int percent, skill_lvl;
    room_rnum other_room = NOWHERE;
    struct room_direction_data *back = NULL;

    skip_spaces(&argument);

    if (!ALIVE(ch)) {
        act("Como??? Você está mort$r!!", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!*argument) {
        send_to_char(ch, "Derrubar o que?\r\n");
        return;
    }

    two_arguments(argument, type, dir);
    if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
        door = find_door(ch, type, dir, "derrubar");

    if ((obj) && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)) {
        obj = NULL;
        door = find_door(ch, type, dir, "derrubar");
    }

    /* Bash only works on doors, not containers */
    if (obj) {
        send_to_char(ch, "Você não pode derrubar isso!\r\n");
        return;
    }

    if (door < 0) {
        send_to_char(ch, "Derrubar o que?\r\n");
        return;
    }

    if (!EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)) {
        send_to_char(ch, "Não há porta para derrubar!\r\n");
        return;
    }

    if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
        send_to_char(ch, "A porta já está aberta!\r\n");
        return;
    }

    /* Get references to the other side */
    if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
        if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
            if (back->to_room != IN_ROOM(ch))
                back = NULL;

    /* Skill check for bash */
    percent = rand_number(1, 101);
    skill_lvl = GET_SKILL(ch, SKILL_BASH);

    if (skill_lvl <= 0) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    if (percent > skill_lvl) {
        send_to_char(ch, "Você falha ao tentar derrubar a porta!\r\n");
        act("$n tenta derrubar $F mas falha!", FALSE, ch, 0,
            EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "a porta", TO_ROOM);
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        return;
    }

    /* Success! */
    /* First, unjam if jammed */
    if (EXIT_FLAGGED(EXIT(ch, door), EX_JAMMED)) {
        unjam_door_both_sides(ch, door);
        send_to_char(ch, "A cunha voa para longe com o impacto!\r\n");
        act("$n derruba $F com força, fazendo a cunha voar!", FALSE, ch, 0,
            EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "a porta", TO_ROOM);
    }

    /* Then, unlock if locked */
    if (EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)) {
        REMOVE_BIT(EXITN(IN_ROOM(ch), door)->exit_info, EX_LOCKED);
        if (back)
            REMOVE_BIT(EXITN(other_room, rev_dir[door])->exit_info, EX_LOCKED);
    }

    /* Finally, open the door */
    REMOVE_BIT(EXITN(IN_ROOM(ch), door)->exit_info, EX_CLOSED);
    if (back)
        REMOVE_BIT(EXITN(other_room, rev_dir[door])->exit_info, EX_CLOSED);

    send_to_char(ch, "Você derruba a porta com um golpe poderoso!\r\n");
    act("$n derruba $F!", FALSE, ch, 0, EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "a porta", TO_ROOM);

    /* Notify the other room */
    if (back)
        send_to_room(other_room, "A %s é derrubada com força pelo outro lado!\r\n",
                     back->keyword ? fname(back->keyword) : "porta");

    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

ACMD(do_enter)
{
    char buf[MAX_INPUT_LENGTH];
    int door;
    struct obj_data *obj;
    char *tmp = buf;
    room_rnum target_room_rnum;

    one_argument(argument, buf);

    if (*buf) {
        int number = get_number(&tmp);
        /* an argument was supplied, search for door keyword */
        if ((obj = get_obj_in_list_vis(ch, buf, &number, world[IN_ROOM(ch)].contents)) && CAN_SEE_OBJ(ch, obj) &&
            GET_OBJ_TYPE(obj) == ITEM_PORTAL) {
            target_room_rnum = real_room(GET_OBJ_VAL(obj, 0));
            if (target_room_rnum != NOWHERE) {
                /* Check level requirement */
                int min_level = GET_OBJ_VAL(obj, 1);
                if (GET_LEVEL(ch) < min_level) {
                    send_to_char(ch, "Voce nao tem nivel suficiente para usar este portal (minimo: %d).\r\n",
                                 min_level);
                    return;
                }
                act("$n entra em $p, e desaparece.", TRUE, ch, obj, 0, TO_ROOM);
                char_from_room(ch);
                char_to_room(ch, target_room_rnum);
                act("$n aparece saindo de $p.", TRUE, ch, obj, 0, TO_ROOM);
                look_at_room(ch, 1);
                /* Autoquest trigger checks after look_at_room so quest messages appear after room description */
                if (!IS_NPC(ch)) {
                    autoquest_trigger_check(ch, 0, 0, AQ_ROOM_FIND);
                    autoquest_trigger_check(ch, 0, 0, AQ_MOB_FIND);
                }
                return;
            }
        }

        for (door = 0; door < DIR_COUNT; door++)
            if (EXIT(ch, door))
                if (EXIT(ch, door)->keyword)
                    if (!str_cmp(EXIT(ch, door)->keyword, buf)) {
                        perform_move(ch, door, 1);
                        return;
                    }

        send_to_char(ch, "Não há %s aqui.\r\n", buf);
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
        send_to_char(ch, "Você já está em um lugar fechado.\r\n");
    else {
        /* try to locate an entrance */
        for (door = 0; door < DIR_COUNT; door++)
            if (EXIT(ch, door))
                if (VALID_ROOM_RNUM(EXIT(ch, door)->to_room))
                    if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
                        ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
                        perform_move(ch, door, 1);
                        return;
                    }
        send_to_char(ch, "Você não parece encontrar algo para entrar.\r\n");
    }
}

ACMD(do_leave)
{
    int door;

    if (OUTSIDE(ch))
        send_to_char(ch, "Você já está do lado de fora.. para onde deseja ir?\r\n");
    else {
        for (door = 0; door < DIR_COUNT; door++)
            if (EXIT(ch, door))
                if (VALID_ROOM_RNUM(EXIT(ch, door)->to_room))
                    if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
                        !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
                        perform_move(ch, door, 1);
                        return;
                    }
        send_to_char(ch, "Não há saídas óbvias para fora daqui.\r\n");
    }
}

ACMD(do_stand)
{
    switch (GET_POS(ch)) {
        case POS_STANDING:
            send_to_char(ch, "Você já está de pé.\r\n");
            break;
        case POS_SITTING:
            send_to_char(ch, "Você se levanta.\r\n");
            act("$n se levanta.", TRUE, ch, 0, 0, TO_ROOM);
            /* Were they sitting in something? */
            char_from_furniture(ch);
            /* Will be sitting after a successful bash and may still be fighting. */
            GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
            break;
        case POS_RESTING:
            send_to_char(ch, "Você termina seu descanso e se levanta.\r\n");
            act("$n termina seu descanso, e se levanta.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            /* Were they sitting in something. */
            char_from_furniture(ch);
            break;
        case POS_MEDITING:
        case POS_SLEEPING:
            send_to_char(ch, "Você precisa acordar antes!\r\n");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Você não considera lutar como estando de pé?\r\n");
            break;
        default:
            send_to_char(ch, "Você para de flutuar, e coloca seus pés no chão.\r\n");
            act("$n para de flutuar, e coloca seus pés no chão.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
    }
}

ACMD(do_sit)
{
    char arg[MAX_STRING_LENGTH];
    struct obj_data *furniture;
    struct char_data *tempch;
    int found;

    one_argument(argument, arg);

    if (!(furniture = get_obj_in_list_vis(ch, arg, NULL, world[ch->in_room].contents)))
        found = 0;
    else
        found = 1;

    switch (GET_POS(ch)) {
        case POS_STANDING:
            if (found == 0) {
                send_to_char(ch, "Você se senta.\r\n");
                act("$n se senta.", FALSE, ch, 0, 0, TO_ROOM);
                GET_POS(ch) = POS_SITTING;
            } else {
                if (GET_OBJ_TYPE(furniture) != ITEM_FURNITURE) {
                    send_to_char(ch, "Você não pode sentar aí!\r\n");
                    return;
                } else if (GET_OBJ_VAL(furniture, 1) > GET_OBJ_VAL(furniture, 0)) {
                    /* Val 1 is current number sitting, 0 is max in sitting. */
                    act("Parece que $p já está no limite.", TRUE, ch, furniture, 0, TO_CHAR);
                    log1("SYSERR: Mobilia %d com muita gente.", GET_OBJ_VNUM(furniture));
                    return;
                } else if (GET_OBJ_VAL(furniture, 1) == GET_OBJ_VAL(furniture, 0)) {
                    act("Não tem lugar para sentar em $p.", TRUE, ch, furniture, 0, TO_CHAR);
                    return;
                } else {
                    if (OBJ_SAT_IN_BY(furniture) == NULL)
                        OBJ_SAT_IN_BY(furniture) = ch;
                    for (tempch = OBJ_SAT_IN_BY(furniture); tempch != ch; tempch = NEXT_SITTING(tempch)) {
                        if (NEXT_SITTING(tempch))
                            continue;
                        NEXT_SITTING(tempch) = ch;
                    }
                    act("Você se senta em $p.", TRUE, ch, furniture, 0, TO_CHAR);
                    act("$n senta em $p.", TRUE, ch, furniture, 0, TO_ROOM);
                    SITTING(ch) = furniture;
                    NEXT_SITTING(ch) = NULL;
                    GET_OBJ_VAL(furniture, 1) += 1;
                    GET_POS(ch) = POS_SITTING;
                }
            }
            break;
        case POS_SITTING:
            send_to_char(ch, "Você já está sentad%s.\r\n", OA(ch));
            break;
        case POS_RESTING:
            send_to_char(ch, "Você termina seu descanso e senta-se.\r\n");
            act("$n termina seu descanso.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
        case POS_MEDITING:
        case POS_SLEEPING:
            send_to_char(ch, "Você deve acordar antes.\r\n");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Sentar-se durante a luta? Você é louc%s?", OA(ch));
            break;
        default:
            send_to_char(ch, "Você para de flutuar e senta-se.\r\n");
            act("$n para de flutuar e senta-se.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
    }
}

ACMD(do_rest)
{
    switch (GET_POS(ch)) {
        case POS_STANDING:
            send_to_char(ch, "Você se senta e descansa seus ossos cansados.\r\n");
            act("$n se senta e descansa.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
        case POS_SITTING:
            send_to_char(ch, "Você descansa seus ossos cansados.\r\n");
            act("$n descansa.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
        case POS_RESTING:
            send_to_char(ch, "Você já está descansando.\r\n");
            break;
        case POS_MEDITING:
        case POS_SLEEPING:
            send_to_char(ch, "Você deve acordar antes.\r\n");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Descansar durante a luta? Você é louc%s?\r\n", OA(ch));
            break;
        default:
            send_to_char(ch, "Você para de flutuar e para para descansar seus ossos cansados.\r\n");
            act("$n para de flutuar e densansa.", FALSE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
    }
}

ACMD(do_meditate) /* IMPLEMENTED */
{
    int percent, prob;

    if (!IS_NPC(ch)) {
        if (!GET_SKILL(ch, SKILL_MEDITATE)) {
            send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
            return;
        }

        percent = rand_number(1, 101); /* 101% is a complete failure */
        prob = GET_SKILL(ch, SKILL_MEDITATE);
    } else {
        percent = 0;
        prob = 100;
    }

    switch (GET_POS(ch)) {
        case POS_STANDING:
        case POS_SITTING:
        case POS_RESTING:
            if (percent > prob) {
                send_to_char(ch, "Você tenta meditar, e acaba caindo no sono...");
                act("$n deita-se e dorme.", TRUE, ch, 0, 0, TO_ROOM);
                GET_POS(ch) = POS_SLEEPING;
                GET_WAIT_STATE(ch) += 5 * PULSE_VIOLENCE;
            } else {
                send_to_char(ch, "Você entra em um transe profundo.");
                act("$n deita-se e entra em um transe profundo.", TRUE, ch, 0, 0, TO_ROOM);
                GET_POS(ch) = POS_MEDITING;
            }
            break;
        case POS_MEDITING:
            send_to_char(ch, "Você já está meditando.\r\n");
            break;
        case POS_SLEEPING:
            send_to_char(ch, "Você sonha estar meditando.\r\n");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "O que?! Isso só funciona em filmes!");
            break;
        default:
            if (percent > prob) {
                send_to_char(ch, "Você para de flutuar, deita-se e acaba caindo no sono.");
                act("$n para de flutuar. deita-se e dorme.", TRUE, ch, 0, 0, TO_ROOM);
                GET_POS(ch) = POS_SLEEPING;
                GET_WAIT_STATE(ch) += 5 * PULSE_VIOLENCE;
            } else {
                send_to_char(ch, "Você para de flutuar, deita-se e medita.");
                act("$n para de flutuar, deita-se e medita.", TRUE, ch, 0, 0, TO_ROOM);
                GET_POS(ch) = POS_MEDITING;
            }
            break;
    }
}

ACMD(do_sleep)
{
    switch (GET_POS(ch)) {
        case POS_STANDING:
        case POS_SITTING:
        case POS_RESTING:
            send_to_char(ch, "Você dorme.\r\n");
            act("$n deita-se e dorme.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SLEEPING;
            break;
        case POS_SLEEPING:
            send_to_char(ch, "Você já está dormindo.\r\n");
            break;
        case POS_MEDITING:
            send_to_char(ch, "Por que não acordar antes?\r\n");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Dormir durante a luta? Vocé é louc%s\r\n", OA(ch));
            break;
        default:
            send_to_char(ch, "Você para de flutuar, deita-se e dorme.\r\n");
            act("$n para de flutuar, deita-se e dorme.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SLEEPING;
            break;
    }
}

ACMD(do_wake)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int self = 0;

    one_argument(argument, arg);
    if (*arg) {
        if (GET_POS(ch) == POS_SLEEPING)
            send_to_char(ch, "Talvez você deva acordar você mesm%s primeiro.", OA(ch));
        else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
            send_to_char(ch, "%s", CONFIG_NOPERSON);
        else if (vict == ch)
            self = 1;
        else if (AWAKE(vict))
            act("$L já está acordad$R.", FALSE, ch, 0, vict, TO_CHAR);
        else if (AFF_FLAGGED(vict, AFF_SLEEP) || (GET_POS(vict) == POS_MEDITING && GET_LEVEL(ch) < LVL_DEMIGOD))
            act("Você não consegue acordá-l$R!", FALSE, ch, 0, vict, TO_CHAR);
        else if (GET_POS(vict) < POS_SLEEPING)
            act("$L está em uma condição muito ruim!", FALSE, ch, 0, vict, TO_CHAR);
        else {
            act("Você $R acorda.", FALSE, ch, 0, vict, TO_CHAR);
            act("Você é acordad$R por $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
            act("$n acorda $N.", FALSE, ch, 0, vict, TO_NOTVICT);
            GET_POS(vict) = POS_SITTING;
        }
        if (!self)
            return;
    }
    if (AFF_FLAGGED(ch, AFF_SLEEP))
        send_to_char(ch, "Você não pode acordar!\r\n");
    else if (GET_POS(ch) > POS_SLEEPING)
        send_to_char(ch, "Você já está acordad%s...\r\n", OA(ch));
    else {
        send_to_char(ch, "Você acorda e senta-se.\r\n");
        act("$n acorda.", TRUE, ch, 0, 0, TO_ROOM);
        GET_POS(ch) = POS_SITTING;
    }
}

ACMD(do_follow)
{
    char buf[MAX_INPUT_LENGTH];
    struct char_data *leader;

    one_argument(argument, buf);

    if (*buf) {
        if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
            send_to_char(ch, "%s", CONFIG_NOPERSON);
            return;
        }
    } else {
        if (ch->master != (char_data *)NULL) {
            send_to_char(ch, "Você já está seguindo %s.\r\n", GET_NAME(ch->master));
        } else {
            send_to_char(ch, "Quem você deseja seguir?\r\n");
        }
        return;
    }

    if (ch->master == leader) {
        act("Você já está seguindo $M.", FALSE, ch, 0, leader, TO_CHAR);
        return;
    }
    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
        act("Mas você só se sente bem seguindo $N!", FALSE, ch, 0, ch->master, TO_CHAR);
    } else { /* Not Charmed follow person */
        if (leader == ch) {
            if (!ch->master) {
                send_to_char(ch, "Você já está seguindo você mesm%s.\r\n", OA(ch));
                return;
            }
            stop_follower(ch);
        } else {
            if (circle_follow(ch, leader)) {
                send_to_char(ch, "Sinto muito, mas seguir em círculos não é permitido.\r\n");
                return;
            }
            if (ch->master)
                stop_follower(ch);

            add_follower(ch, leader);
        }
    }
}

ACMD(do_unfollow)
{
    if (ch->master) {
        if (AFF_FLAGGED(ch, AFF_CHARM)) {
            send_to_char(ch, "Mas você só se sente bem seguindo %s.\r\n", GET_NAME(ch->master));
        } else {
            stop_follower(ch);
        }
    } else {
        send_to_char(ch, "Você não está  seguindo ningúem.\r\n");
    }
    return;
}

int start_flying(struct char_data *ch)
{
    if (!has_flight(ch) || AFF_FLAGGED(ch, AFF_FLYING) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_FLY))
        return (0);

    SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    send_to_char(ch, "Você levanta vôo.\r\n");
    act("$n levanta vôo.", TRUE, ch, 0, 0, TO_ROOM);
    return (1);
}

int stop_flying(struct char_data *ch)
{
    if (!AFF_FLAGGED(ch, AFF_FLYING))
        return (0);

    if (SECT(IN_ROOM(ch)) == SECT_AIR_FLOW || SECT(IN_ROOM(ch)) == SECT_FLYING) {
        send_to_char(ch, "Você tenta parar de voar, mas algo lhe mantêm no ar.\r\n");
        return 0;
    } else {
        /* Remove the fly spell affect so player must cast fly again */
        if (affected_by_spell(ch, SPELL_FLY))
            affect_from_char(ch, SPELL_FLY);

        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);

        /* Check if player can still fly after removing spell (e.g., via items) */
        if (has_flight(ch)) {
            SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
            send_to_char(ch, "Você tenta aterrisar, mas seus itens mágicos mantêm você no ar.\r\n");
            return (0);
        }

        send_to_char(ch, "Você aterrisa.\r\n");
        act("$n aterrissa.", TRUE, ch, 0, 0, TO_ROOM);
    }
    return (1);
}

ACMD(do_fly)
{
    if (subcmd == SCMD_FLY) {
        if (!has_flight(ch)) {
            send_to_char(ch, "Você tenta, mas não consegue.\r\n");
        } else if (AFF_FLAGGED(ch, AFF_FLYING)) {
            send_to_char(ch, "Você já está voando!\r\n");
        } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_FLY)) {
            send_to_char(ch, "Você não pode voar aqui.\r\n");
        } else {
            start_flying(ch);
        }
    } else if (subcmd == SCMD_LAND) {
        if (!AFF_FLAGGED(ch, AFF_FLYING)) {
            send_to_char(ch, "Você não está voando porque quer... não dá para fazer nada!\r\n");
        } else {
            stop_flying(ch);
        }
    }
}

/* -- jr - May 15, 2000
 * SPY skill.
 */
ACMD(do_spy)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int dir, need_movement;
    room_rnum was_in = IN_ROOM(ch);
    /* ... and the room the character will move into. */
    room_rnum dest;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SPY)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    one_argument(argument, arg);

    if (((dir = search_block(arg, dirs, FALSE)) < 0) && ((dir = search_block(arg, autoexits, FALSE)) < 0)) {
        send_to_char(ch, "Espiar aonde?\r\n");
        return;
    }

    if (!EXIT(ch, dir) || !(dest = EXIT(ch, dir)->to_room)) {
        send_to_char(ch, "Espiar aonde?\r\n");
        return;
    }

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) {
        if (is_name(arg, EXIT(ch, dir)->keyword)) {
            sprintf(buf, "%s parece estar fechada. \r\n", EXIT(ch, dir)->keyword ? EXIT(ch, dir)->keyword : "porta");
            CAP(buf);
            send_to_char(ch, "%s", buf);
        } else
            send_to_char(ch, "Este caminho parece estar fechado.\r\n");

        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        need_movement = (movement_loss[SECT(was_in)] + movement_loss[SECT(dest)]) / 2;

        if (GET_MOVE(ch) < need_movement) {
            act("Você está muito cansad$r para isso.", FALSE, ch, NULL, NULL, TO_CHAR);
            return;
        }

        GET_MOVE(ch) -= need_movement;
    }

    if (rand_number(1, 101) > GET_SKILL(ch, SKILL_SPY)) {
        send_to_char(ch, "Você tenta espiar na sala ao lado mas não consegue.\r\n");
        return;
    }

    was_in = IN_ROOM(ch);
    char_from_room(ch);
    char_to_room(ch, dest);
    send_to_char(ch, "Você espia a outra sala e vê: \r\n\r\n");
    look_at_room(ch, 1);
    char_from_room(ch);
    char_to_room(ch, was_in);
    act("$n dá uma espiada na sala ao lado.", TRUE, ch, NULL, NULL, TO_ROOM);
}

/* Danger Sense: Check for death traps in adjacent rooms
 * Returns 1 if danger was detected and message was shown, 0 otherwise.
 * At skill < 75: shows a vague warning without revealing the direction.
 * At skill >= 75: shows the specific direction(s) of the death trap(s). */
int check_danger_sense(struct char_data *ch)
{
    int dir;
    room_rnum dest;
    int danger_count = 0;
    char directions[256] = "";
    char buf[MAX_STRING_LENGTH];
    size_t len = 0;
    int ret;
    int skill_level;

    /* Only works for thieves with the danger sense skill */
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DANGER_SENSE))
        return 0;

    /* Ensure character is in a valid room */
    if (IN_ROOM(ch) == NOWHERE)
        return 0;

    skill_level = GET_SKILL(ch, SKILL_DANGER_SENSE);

    /* Check proficiency - higher skill level = better detection chance */
    if (rand_number(1, 101) > skill_level)
        return 0;

    /* Check all directions for death traps */
    for (dir = 0; dir < NUM_OF_DIRS; dir++) {
        if (!EXIT(ch, dir) || !(dest = EXIT(ch, dir)->to_room))
            continue;

        /* Validate the destination room number before accessing world array */
        if (!VALID_ROOM_RNUM(dest))
            continue;

        /* Check if the destination room is a death trap */
        if (ROOM_FLAGGED(dest, ROOM_DEATH)) {
            if (danger_count > 0) {
                ret = snprintf(directions + len, sizeof(directions) - len, ", ");
                if (ret < 0 || (size_t)ret >= sizeof(directions) - len)
                    break;
                len += ret;
            }

            ret = snprintf(directions + len, sizeof(directions) - len, "%s", dirs_pt[dir]);
            if (ret < 0 || (size_t)ret >= sizeof(directions) - len)
                break;
            len += ret;

            danger_count++;
        }
    }

    /* Alert the character if danger was sensed */
    if (danger_count > 0) {
        /* At skill >= 75 show the specific direction; below that, only a vague warning */
        if (skill_level >= 75) {
            if (danger_count == 1) {
                snprintf(buf, sizeof(buf), "@RVocê sente um arrepio na espinha... há PERIGO mortal vindo do %s!@n\r\n",
                         directions);
            } else {
                snprintf(buf, sizeof(buf),
                         "@RVocê sente um arrepio na espinha... há PERIGO mortal vindo das direções %s!@n\r\n",
                         directions);
            }
        } else {
            snprintf(buf, sizeof(buf), "@RVocê sente um arrepio na espinha... há PERIGO mortal por perto!@n\r\n");
        }
        send_to_char(ch, "%s", buf);
        return 1;
    }
    return 0;
}

/* Danger Sense for Flee: Check if a specific direction leads to a death trap and if danger sense prevents it
 * Returns 1 if danger sense successfully detected and blocks the death trap, 0 otherwise */
int check_danger_sense_prevents_flee(struct char_data *ch, int dir)
{
    room_rnum dest;

    /* Only works for non-NPC characters with the danger sense skill */
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DANGER_SENSE))
        return 0;

    /* Ensure character is in a valid room */
    if (IN_ROOM(ch) == NOWHERE)
        return 0;

    /* Check if the direction is valid */
    if (!EXIT(ch, dir) || !(dest = EXIT(ch, dir)->to_room))
        return 0;

    /* Validate the destination room number before accessing world array */
    if (!VALID_ROOM_RNUM(dest))
        return 0;

    /* Check if the destination room is a death trap */
    if (!ROOM_FLAGGED(dest, ROOM_DEATH))
        return 0;

    /* Check proficiency - higher skill level = better detection chance */
    if (rand_number(1, 101) > GET_SKILL(ch, SKILL_DANGER_SENSE))
        return 0; /* Skill check failed, danger sense doesn't prevent flee */

    /* Skill check succeeded - warn and prevent fleeing to death trap */
    send_to_char(ch,
                 "@RSeu senso de perigo te alerta! Você recusa-se a fugir para %s - há uma ARMADILHA MORTAL lá!@n\r\n",
                 dirs_pt[dir]);
    return 1;
}
