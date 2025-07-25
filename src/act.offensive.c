/**************************************************************************
 *  File: act.offensive.c                                   Part of tbaMUD *
 *  Usage: Player-level commands of an offensive nature.                   *
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
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "act.h"
#include "fight.h"
#include "mud_event.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "graph.h"

ACMD(do_assist)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *helpee, *opponent;

    if (FIGHTING(ch)) {
        send_to_char(ch, "Você já está lutando!  Como você pretende dar assistência a mais alguém?\r\n");
        return;
    }
    if IS_DEAD (ch) {
        send_to_char(ch, "Você não pode dar assistência a ninguém, você está mort%s!", OA(ch));
        return;
    }
    one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Para quem você deseja dar assistência?\r\n");
    else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
        send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (helpee == ch)
        send_to_char(ch, "Você não pode ajudar a si mesm%s!\r\n", OA(ch));
    else {
        /*
         * Hit the same enemy the person you're helping is.
         */
        if (FIGHTING(helpee))
            opponent = FIGHTING(helpee);
        else
            for (opponent = world[IN_ROOM(ch)].people; opponent && (FIGHTING(opponent) != helpee);
                 opponent = opponent->next_in_room)
                ;

        if (!opponent)
            act("Mas $L não está lutando com ninguém!", FALSE, ch, 0, helpee, TO_CHAR);
        else if (!CAN_SEE(ch, opponent))
            act("Você não pode ver com quem $L está lutando!", FALSE, ch, 0, helpee, TO_CHAR);
        /* prevent accidental pkill */
        else if (!CONFIG_PK_ALLOWED && !IS_NPC(opponent))
            send_to_char(ch, "Use 'murder' se você deseja realmente atacar!\r\n");
        else {
            send_to_char(ch, "Você entra para a luta!\r\n");
            act("$N da assistência a você !", 0, helpee, 0, ch, TO_CHAR);
            act("$n da assistência a  $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
            hit(ch, opponent, TYPE_UNDEFINED);
        }
    }
}

/* -- VP -- jr - 23/06/99 * Rotina reestruturada, para eliminar bugs. -- jr -
   Apr 16, 2000 * Nova reestrutura��o da rotina. -- Cansian - Jun, 11,
   2020 * Atualizado para nova estrutura de grupo. */
ACMD(do_gassist)
{
    struct char_data *k, *helpee = NULL, *opponent;
    struct group_data *group;

    if ((group = GROUP(ch)) == NULL) {
        send_to_char(ch, "Mas você não é membro de um grupo!\r\n");
        return;
    }

    if (FIGHTING(ch)) {
        send_to_char(ch, "Você já está lutando!  Como você pretente dar assistência a alguém?\r\n");
        return;
    }

    if IS_DEAD (ch) {
        send_to_char(ch, "Você não pode dar assistência a ninguém, você está mort%s!", OA(ch));
        return;
    }
    if (GROUP(ch))
        while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL) {
            if ((k != ch) && CAN_SEE(ch, k) && (IN_ROOM(k) == IN_ROOM(ch)) && FIGHTING(k)) {
                if (!CAN_SEE(ch, FIGHTING(k)))
                    act("Você não pode ver com quem $N está lutando.", FALSE, ch, NULL, k, TO_CHAR);
                else {
                    helpee = k;
                    break;
                }
            }
        }

    if (!helpee) {
        send_to_char(ch, "Você não vê ninguém lutando em seu grupo.\r\n");
        return;
    } else {
        opponent = FIGHTING(helpee);
        act("Você dá assistência a $N.", FALSE, ch, 0, helpee, TO_CHAR);

        if (!CONFIG_PK_ALLOWED && !IS_NPC(opponent)) /* prevent accidental
                                                                                                        pkill */
            act("Use 'murder' se voce realmente deseja atacar $N.", FALSE, ch, 0, opponent, TO_CHAR);
        else {
            act("$N dá assistência a você!", FALSE, helpee, 0, ch, TO_CHAR);
            send_to_group(NULL, group, "%s dá assistência a um membro do grupo!\r\n", GET_NAME(ch));
            act("$n dá assistência a $N.", TRUE, ch, 0, helpee, TO_NOTVICT);
            hit(ch, opponent, TYPE_UNDEFINED);
        }
    }
}

ACMD(do_hit)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;

    one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Bater em quem?\r\n");
    else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
        send_to_char(ch, "Esta pessoa não parece estar aqui.\r\n");
    else if (vict == ch) {
        send_to_char(ch, "Você bate em si mesm%s... AI!\r\n", OA(ch));
        act("$n bate em si mesm$r, e diz AI!", FALSE, ch, 0, vict, TO_ROOM);
    } else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
        act("$N é amig$R e você não pode atacá-l$R.", FALSE, ch, 0, vict, TO_CHAR);
    else {
        if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
            return; /* you can't order a charmed pet to attack a
                               player */

        if (!CONFIG_PK_ALLOWED && !IS_NPC(vict) && !IS_NPC(ch)) {
            if (!SCMD_MURDER)
                send_to_char(ch, "Use 'murder' se você realmente deseja atacar outro jogador.\r\n");
            else
                check_killer(ch, vict);
        }

        if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
            if (GET_DEX(ch) > GET_DEX(vict) ||
                (GET_DEX(ch) == GET_DEX(vict) &&
                 rand_number(1, 2) ==
                     1))                       /* if
                                                                                                                                                  faster
                                                                                                                                                */
                hit(ch, vict, TYPE_UNDEFINED); /* first */
            else
                hit(vict, ch, TYPE_UNDEFINED); /* or the victim is first */
            WAIT_STATE(ch, PULSE_VIOLENCE + 2);
        } else
            send_to_char(ch, "Você está fazendo o melhor que você pode!\r\n");
    }
}

ACMD(do_kill)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;

    if (GET_LEVEL(ch) < LVL_GRGOD || IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        do_hit(ch, argument, cmd, subcmd);
        return;
    }
    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Matar quem?\r\n");
    } else {
        if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
            send_to_char(ch, "Esta pessoa não está aqui.\r\n");
        else if (ch == vict)
            send_to_char(ch, "Sua mamãe vai ficar triste... :(\r\n");
        else {
            act("Você esmigalha $N em pedaços!  Ah!  Sangue!", FALSE, ch, 0, vict, TO_CHAR);
            act("$N esmigalha você em pedaços!", FALSE, vict, 0, ch, TO_CHAR);
            act("$nbrutalmente destrói $N!", FALSE, ch, 0, vict, TO_NOTVICT);
            raw_kill(vict, ch);
        }
    }
}

ACMD(do_backstab)
{
    char buf[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int percent, prob;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    one_argument(argument, buf);

    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Esfaquear as costas de quem?\r\n");
        return;
    }
    if (vict == ch) {
        send_to_char(ch, "Como você pretende pegar você mesmo de surpresa?\r\n");
        return;
    }
    if (!CONFIG_PK_ALLOWED && !IS_NPC(vict)) /* prevent accidental pkill */
        act("Use 'murder' se voce realmente deseja atacar $N.", FALSE, ch, 0, vict, TO_CHAR);
    if (!GET_EQ(ch, WEAR_WIELD)) {
        send_to_char(ch, "Voce precisa empunhar uma arma para fazer isso.\r\n");
        return;
    }
    if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT) {
        send_to_char(ch, "Somente armas perfurantes podem ser usadas para esfaquear alguém pelas costas.\r\n");
        return;
    }
    if (FIGHTING(vict)) {
        send_to_char(ch, "Você não pode pegar de surpresa uma pessoa lutando -- ela provavelmente está alerta!\r\n");
        return;
    }

    if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) {
        act("Você percebe $N tentando pegar você desprevenid$r!", FALSE, vict, 0, ch, TO_CHAR);
        act("$l percebeu você tentando pegá-l$r desprevenid$r!", FALSE, vict, 0, ch, TO_VICT);
        act("$n percebeu que $N iria pegá-l$r desprevenid$r!", FALSE, vict, 0, ch, TO_NOTVICT);
        hit(vict, ch, TYPE_UNDEFINED);
        return;
    }

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_BACKSTAB);

    if (AWAKE(vict) && (percent > prob))
        damage(ch, vict, 0, SKILL_BACKSTAB);
    else
        hit(ch, vict, SKILL_BACKSTAB);

    if (GET_LEVEL(ch) < LVL_GOD)
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

/*
 * BACKFLIP skill
 */
ACMD(do_backflip)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int percent, prob;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKFLIP) || !GET_SKILL(ch, SKILL_BACKSTAB) || PLR_FLAGGED(ch, PLR_TRNS)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    } else if (GET_POS(ch) != POS_FIGHTING) {
        send_to_char(ch, "Você deve estar lutando para ter sucesso.\r\n");
        return;
    }

    one_argument(argument, arg);

    if (*arg) {
        if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL) {
            send_to_char(ch, "Quem?\r\n");
            return;
        }
    } else if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
        vict = FIGHTING(ch);
    } else {
        send_to_char(ch, "Quem?\r\n");
        return;
    }

    if (vict == ch) {
        send_to_char(ch, "Tá... entendí... faz muito sentido...\r\n");
        return;
    }
    if (FIGHTING(vict) == NULL || FIGHTING(vict) != ch) {
        act("$N não está lutando com você...", FALSE, ch, NULL, vict, TO_CHAR);
        return;
    }

    if (IS_DEAD(ch)) {
        act("Como você pretende fazer isso? Você está mort$r!", FALSE, ch, NULL, vict, TO_CHAR);
        return;
    }

    if (char_has_mud_event(ch, eBACKFLIP)) {
        send_to_char(ch, "Você já está se escondendo de alguém!\r\n");
        return;
    }

    percent = rand_number(1, 101);
    prob = GET_SKILL(ch, SKILL_BACKFLIP);

    percent += (25 - GET_DEX(ch));
    percent += (25 - GET_STR(ch)) / 2;

    if (percent > prob) {
        act("Você tenta fazer uma cambalhota por sobre $N, mas perde o equilíbrio e cai sentad$r.", FALSE, ch, NULL,
            vict, TO_CHAR);
        act("$n faz um lindo giro no ar e cai sentad$r no chão, que ridículo...", FALSE, ch, NULL, vict, TO_ROOM);
        GET_POS(ch) = POS_SITTING;
        WAIT_STATE(ch, PULSE_VIOLENCE * 3);
    } else {
        act("Que cambalhota perfeita! $U$N não sabe onde você está e ficou perdid$R!", FALSE, ch, NULL, vict, TO_CHAR);
        act("$n faz uma cambalhota por sobre $N, que fica confus$R!", FALSE, ch, NULL, vict, TO_NOTVICT);
        act("$n faz uma cambalhota e você $r perde de vista.", FALSE, ch, NULL, vict, TO_VICT);
        /* it is assured that vict will be fighting ch */
        stop_fighting(vict);
        stop_fighting(ch);
        /* NEW_EVENT() will add a new mud event to the event list of the
           character. This function below adds a new event of "eBACKFLIP", to
           "ch", and passes "NULL" as additional data. The event will be
           called in "3 * PASSES_PER_SEC" or 3 seconds */
        NEW_EVENT(eBACKFLIP, ch, NULL, 2 * PASSES_PER_SEC);
        WAIT_STATE(ch, PULSE_VIOLENCE * 4);
    }
}

EVENTFUNC(event_backflip)
{
    struct char_data *ch, *tch;
    struct mud_event_data *pMudEvent;
    struct list_data *room_list;

    /* This is just a dummy check, but we'll do it anyway */
    if (event_obj == NULL)
        return 0;

    /* For the sake of simplicity, we will place the event data in easily
       referenced pointers */
    pMudEvent = (struct mud_event_data *)event_obj;
    ch = (struct char_data *)pMudEvent->pStruct;

    /* When using a list, we have to make sure to allocate the list as it uses
       dynamic memory */
    room_list = create_list();

    /* We search through the "next_in_room", and grab all NPCs and add them to
       our list */
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
        if (IS_NPC(tch) && (FIGHTING(tch) == ch))
            add_to_list(tch, room_list);

    /* If our list is empty or has "0" entries, we free it from memory and
       close off our event */
    if (room_list->iSize == 0) {
        free_list(room_list);
        return 0;
    }

    /* We spit out some ugly colour, making use of the new colour options, to
       let the player know they are performing their backflip */
    send_to_char(ch, "Você da uma cambalhota e se esconde.\r\n");

    /* Lets grab some a random NPC from the list, and stop them */
    tch = random_from_list(room_list);
    stop_fighting(tch);

    /* Now that our backflip is done, let's free out list */
    free_list(room_list);

    /* The "return" of the event function is the time until the event is
       called again. If we return 0, then the event is freed and removed from
       the list, but any other numerical response will be the delay until the
       next call */
    if (GET_SKILL(ch, SKILL_BACKFLIP) < rand_number(1, 101)) {
        send_to_char(ch, "Você para de se esconder.\r\n");
        if (IN_ROOM(tch) == IN_ROOM(ch))
            hit(ch, tch, TYPE_UNDEFINED);
        return 0;
    } else
        return 1 * PASSES_PER_SEC;
}

ACMD(do_order)
{
    char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
    bool found = FALSE;
    struct char_data *vict;
    struct follow_type *k;

    half_chop(argument, name, message);

    if (!*name || !*message)
        send_to_char(ch, "Ordenar quem a fazer o quê?\r\n");
    else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "seguidores") &&
             !is_abbrev(name, "followers"))
        send_to_char(ch, "Esta pessoa não está aqui.\r\n");
    else if (ch == vict)
        send_to_char(ch, "Você evidentemente sofre de esquisofrenia.\r\n");
    else {
        if (AFF_FLAGGED(ch, AFF_CHARM)) {
            send_to_char(ch, "Seu superior não irá aprovar você dando ordens.\r\n");
            return;
        }
        if (vict) {
            char buf[MAX_STRING_LENGTH];

            snprintf(buf, sizeof(buf), "$N ordena você a '%s'", message);
            act(buf, FALSE, vict, 0, ch, TO_CHAR);
            act("$n dá uma ordem a $N ", FALSE, ch, 0, vict, TO_ROOM);

            if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
                act("$n tem um olhar indiferente.", FALSE, vict, 0, 0, TO_ROOM);
            else {
                send_to_char(ch, "%s", CONFIG_OK);
                command_interpreter(vict, message);
            }
        } else { /* This is order "followers" */
            char buf[MAX_STRING_LENGTH];

            snprintf(buf, sizeof(buf), "$ndita a ordem '%s'.", message);
            act(buf, FALSE, ch, 0, 0, TO_ROOM);

            for (k = ch->followers; k; k = k->next) {
                if (IN_ROOM(ch) == IN_ROOM(k->follower))
                    if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
                        found = TRUE;
                        command_interpreter(k->follower, message);
                    }
            }
            if (found)
                send_to_char(ch, "%s", CONFIG_OK);
            else
                send_to_char(ch, "Ninguém aqui é fiel a você!\r\n");
        }
    }
}

ACMD(do_flee)
{
    int i, attempt, loss;
    struct char_data *was_fighting;

    if (GET_POS(ch) < POS_FIGHTING) {
        send_to_char(ch, "Você não está em condições de fugir!\r\n");
        return;
    }

    for (i = 0; i < 6; i++) {
        attempt = rand_number(0, DIR_COUNT - 1); /* Seleciona uma direção aleatória */
        if (CAN_GO(ch, attempt) && (!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH) ||
                                    !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_NOMOB))) {
            act("$n entra em pânico e tenta fugir!", TRUE, ch, 0, 0, TO_ROOM);
            was_fighting = FIGHTING(ch);

            if (AFF_FLAGGED(ch, AFF_PARALIZE)) {
                send_to_char(ch, "Você está paralisado! Não pode fugir! Comece a rezar...\r\n");
                act("$n não pode fugir, $l está paralisad$r!", TRUE, ch, 0, 0, TO_ROOM);
            }

            if (do_simple_move(ch, attempt, TRUE)) {
                send_to_char(ch, "Você foge de pernas para o ar.\r\n");

                /************************************************
                 * Genética: Mob conseguiu fugir com sucesso.   *
                 * Aumenta a tendência de fugir (wimpy).        *
                 ************************************************/
                if (IS_NPC(ch) && ch->ai_data) {
                    ch->ai_data->genetics.wimpy_tendency += 1;
                    /* Garante que o valor não passa de 100 */
                    if (GET_GENWIMPY(ch) > 100)
                        ch->ai_data->genetics.wimpy_tendency = 100;
                }

                if (was_fighting && !IS_NPC(ch)) {
                    loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
                    loss *= GET_LEVEL(was_fighting);
                    send_to_char(ch, "Você perdeu %ld pontos de experiência.\r\n", (long)loss);
                    gain_exp(ch, -loss);
                }

                if (FIGHTING(ch))
                    stop_fighting(ch);
                if (was_fighting && ch == FIGHTING(was_fighting))
                    stop_fighting(was_fighting);
            } else {
                send_to_char(ch, "Você tenta fugir, mas não consegue!\r\n");
                act("$n tenta fugir mas não consegue!", TRUE, ch, 0, 0, TO_ROOM);

                /************************************************
                 * Genética: Mob falhou a tentativa de fugir.   *
                 ************************************************/
                if (IS_NPC(ch) && ch->ai_data) {
                    if (MOB_FLAGGED(ch, MOB_BRAVE)) {
                        ch->ai_data->genetics.wimpy_tendency += 3;
                    } else {
                        ch->ai_data->genetics.wimpy_tendency -= 3;
                    }
                    /* Garante que o valor não fica abaixo de 0 */
                    if (GET_GENWIMPY(ch) < 0)
                        ch->ai_data->genetics.wimpy_tendency = 0;
                }
            }
            return;
        }
    }

    send_to_char(ch, "%sPANICO!  Você não tem escapatória!%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));

    /****************************************************************
     * Genética: Mob entrou em pânico e não encontrou saídas.       *
     * Isto também conta como uma falha.                            *
     ****************************************************************/
    if (IS_NPC(ch) && ch->ai_data) {
        if (MOB_FLAGGED(ch, MOB_BRAVE)) {
            ch->ai_data->genetics.wimpy_tendency += 2; /* Mob corajoso tem penalidade por pânico */
        } else {
            ch->ai_data->genetics.wimpy_tendency -= 2; /* Penalidade por pânico */
        }
        /* Garante que o valor não fica abaixo de 0 */
        if (GET_GENWIMPY(ch) < 0)
            ch->ai_data->genetics.wimpy_tendency = 0;
    }
}

ACMD(do_bash)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int percent, prob;

    one_argument(argument, arg);

    if (!IS_NPC(ch) && !GET_SKILL(ch, SKILL_BASH)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "Este lugar é muito calmo, sem violências...\r\n");
        return;
    }
    if (!GET_EQ(ch, WEAR_WIELD)) {
        send_to_char(ch, "Você precisa empunhar uma arma para ter sucesso.\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
            vict = FIGHTING(ch);
        } else {
            send_to_char(ch, "Derrubar quem?\r\n");
            return;
        }
    }
    if (vict == ch) {
        send_to_char(ch, "Sem gracinhas hoje...\r\n");
        return;
    }
    if (MOB_FLAGGED(vict, MOB_NOKILL)) {
        send_to_char(ch, "Você não pode lutar.\r\n");
        return;
    }
    if (!CONFIG_PK_ALLOWED && !IS_NPC(vict)) {
        /* prevent accidental pkill */
        act("Use 'murder' se voce realmente deseja atacar $N.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    percent = rand_number(1, 101); /* 101% is a complete failure */
    if (!IS_NPC(ch))
        prob = GET_SKILL(ch, SKILL_BASH);
    else
        prob = GET_LEVEL(ch);

    if (MOB_FLAGGED(vict, MOB_NOBASH))
        percent = 101;

    if (percent > prob) {
        damage(ch, vict, 0, SKILL_BASH);
        if (GET_LEVEL(ch) < LVL_GOD) {
            GET_POS(ch) = POS_SITTING;
            WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        }
    } else {
        /*
         * If we bash a player and they wimp out, they will move to the previous
         * room before we set them sitting.  If we try to set the victim sitting
         * first to make sure they don't flee, then we can't bash them!  So now
         * we only set them sitting if they didn't flee. -gg 9/21/98
         */
        if (damage(ch, vict, 1, SKILL_BASH) > 0) { /* -1 = dead, 0 = miss */
            WAIT_STATE(vict, PULSE_VIOLENCE * 2);
            if (IN_ROOM(ch) == IN_ROOM(vict))
                GET_POS(vict) = POS_SITTING;
        }
    }

    if (GET_LEVEL(ch) < LVL_GOD)
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

/*
 * -- jr - Dec 29, 2001
 * Slash the opponent 2 to 5 (avg 3) times on first round, with greater
 * damage per slash.
 *
 * This is the first skill with improvement enabled.
 */
ACMD(do_combo)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    struct obj_data *weapon;
    int perc, prob, hits, basedam, dam, i;

    one_argument(argument, arg);

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_COMBO_ATTACK)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }
    if (!(weapon = GET_EQ(ch, WEAR_WIELD))) {
        send_to_char(ch, "Você precisa empunhar uma arma para ter sucesso.\r\n");
        return;
    }
    if (FIGHTING(ch)) {
        send_to_char(ch, "Você não sabe como fazer isso durante a luta...\r\n");
        return;
    }
    if (GET_OBJ_TYPE(weapon) != ITEM_WEAPON || GET_OBJ_VAL(weapon, 3) + TYPE_HIT != TYPE_SLASH) {
        act("$p não é uma arma apropriada para isso.", FALSE, ch, weapon, NULL, TO_CHAR);
        return;
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL) {
        send_to_char(ch, "Atacar quem?\r\n");
        return;
    }
    if (vict == ch) {
        send_to_char(ch, "Sem gracinhas hoje...\r\n");
        return;
    }
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "Este lugar é muito calmo, sem violências...\r\n");
        return;
    }
    if (!CONFIG_PK_ALLOWED && !IS_NPC(vict)) {
        /* prevent accidental pkill */
        act("Use 'murder' se voce realmente deseja atacar $N.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    perc = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_COMBO_ATTACK);

    if (GET_CLASS(ch) != CLASS_WARRIOR)
        perc += 5;
    if (MOB_FLAGGED(vict, MOB_AWARE))
        perc += 10;
    if (GET_LEVEL(vict) > GET_LEVEL(ch))
        perc += GET_LEVEL(vict) - GET_LEVEL(ch);

    if (perc > prob) {
        damage(ch, vict, 0, SKILL_COMBO_ATTACK);
        if (GET_LEVEL(ch) < LVL_GOD) {
            WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
            return;
        }
    }

    /*
     * Calculate how many hits.
     * After throwing 1 million times, the following expression gave:
     * 2:  249657 (24.97%)
     * 3:  459093 (45.91%)
     * 4:  249631 (24.96%)
     * 5:   41619 ( 4.16%)
     */
    hits = 2 + !rand_number(0, 1) + !rand_number(0, 2) + !rand_number(0, 3);

    /*
     * Calculate base damage.
     */
    basedam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
    basedam += GET_DAMROLL(ch);

    for (i = 0; i < hits; i++) {
        /*
         * Calculate how much damage.
         * Note the dice is thrown twice.
         */
        dam = basedam;
        dam += dice(GET_OBJ_VAL(weapon, 1), GET_OBJ_VAL(weapon, 2));
        dam += dice(GET_OBJ_VAL(weapon, 1), GET_OBJ_VAL(weapon, 2));

        if ((dam = damage(ch, vict, dam, TYPE_SLASH)) < 0) /* -1 = died */
            break;

        if (IN_ROOM(ch) != IN_ROOM(vict))
            break;
    }

    WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
}

ACMD(do_rescue)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict, *tmp_ch;
    int percent, prob;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_RESCUE) || PLR_FLAGGED(ch, PLR_TRNS)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    if (IS_DEAD(ch)) {
        send_to_char(ch, "Você não pode ajudar ninguém, você está mort%s!\r\n", OA(ch));
        return;
    }

    one_argument(argument, arg);

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Quem você deseja resgatar?\r\n");
        return;
    }
    if (vict == ch) {
        send_to_char(ch, "Fugir não seria melhor?\r\n");
        return;
    }
    if (FIGHTING(ch) == vict) {
        send_to_char(ch, "Como você pretende resgatar alguém que você está tentando matar?\r\n");
        return;
    }
    for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch && (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room)
        ;

    if ((FIGHTING(vict) != NULL) && (FIGHTING(ch) == FIGHTING(vict)) && (tmp_ch == NULL)) {
        tmp_ch = FIGHTING(vict);
        if (FIGHTING(tmp_ch) == ch) {
            send_to_char(ch, "Mas você já resgatou %s de %s.\r\n", GET_NAME(vict), GET_NAME(FIGHTING(ch)));
            return;
        }
    }

    if (!tmp_ch) {
        act("Mas ninguém está lutando com $N!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if ((!CONFIG_PK_ALLOWED && !IS_NPC(tmp_ch))) {
        act("Use 'murder' se você realmente deseja atacar $N.", FALSE, ch, 0, tmp_ch, TO_CHAR);
        return;
    }
    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_RESCUE);

    if (percent > prob) {
        send_to_char(ch, "Você falhou no resgate!\r\n");
        return;
    }
    send_to_char(ch, "Banzai!  Ao resgate...\r\n");
    act("Você foi resgatado por $N! Você fica confuso...", FALSE, vict, 0, ch, TO_CHAR);
    act("$n heroicamente resgata $N!", FALSE, ch, 0, vict, TO_NOTVICT);

    if (FIGHTING(vict) == tmp_ch)
        stop_fighting(vict);
    if (FIGHTING(tmp_ch))
        stop_fighting(tmp_ch);
    if (FIGHTING(ch))
        stop_fighting(ch);

    set_fighting(ch, tmp_ch);
    set_fighting(tmp_ch, ch);

    if (GET_LEVEL(ch) < LVL_GOD) {
        WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
    }
}

EVENTFUNC(event_whirlwind)
{
    struct char_data *ch, *tch;
    struct mud_event_data *pMudEvent;
    struct list_data *room_list;
    int count;

    /* This is just a dummy check, but we'll do it anyway */
    if (event_obj == NULL)
        return 0;

    /* For the sake of simplicity, we will place the event data in easily
       referenced pointers */
    pMudEvent = (struct mud_event_data *)event_obj;
    ch = (struct char_data *)pMudEvent->pStruct;

    /* When using a list, we have to make sure to allocate the list as it uses
       dynamic memory */
    room_list = create_list();

    /* We search through the "next_in_room", and grab all NPCs and add them to
       our list */
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
        if (IS_NPC(tch))
            add_to_list(tch, room_list);

    /* If our list is empty or has "0" entries, we free it from memory and
       close off our event */
    if (room_list->iSize == 0) {
        free_list(room_list);
        send_to_char(ch, "Não tem ninguem na sala para o ataque giratório!\r\n");
        return 0;
    }

    /* We spit out some ugly colour, making use of the new colour options, to
       let the player know they are performing their whirlwind strike */
    //	send_to_char(ch, "\t[f313]Você inicia um poderoso \t[f014]\t[b451]ATAQUE GIRATÓRIO!!\tn\r\n");
    send_to_char(ch, "Você está girando incontrolavelmente!!\r\n");

    /* Lets grab some a random NPC from the list, and hit() them up */
    for (count = dice(1, 4); count > 0; count--) {
        tch = random_from_list(room_list);
        hit(ch, tch, TYPE_UNDEFINED);
    }

    /* Now that our attack is done, let's free out list */
    free_list(room_list);

    /* The "return" of the event function is the time until the event is
       called again. If we return 0, then the event is freed and removed from
       the list, but any other numerical response will be the delay until the
       next call */
    if (GET_SKILL(ch, SKILL_WHIRLWIND) < rand_number(1, 101)) {
        send_to_char(ch, "Você para de girar.\r\n");
        return 0;
    } else
        return 1.5 * PASSES_PER_SEC;
}

/* The "Whirlwind" skill is designed to provide a basic understanding of the
   mud event and list systems. */
ACMD(do_whirlwind)
{

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_WHIRLWIND)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "Este lugar é muito calmo, sem violências...\r\n");
        return;
    }

    if (GET_POS(ch) < POS_FIGHTING) {
        send_to_char(ch, "Você precisa estar em pé para realizar um ataque giratório.\r\n");
        return;
    }

    /* First thing we do is check to make sure the character is not in the
       middle of a whirl wind attack. "char_had_mud_event() will sift through
       the character's event list to see if an event of type "eWHIRLWIND"
       currently exists. */
    if (char_has_mud_event(ch, eWHIRLWIND)) {
        send_to_char(ch, "Você já está tentando isto!\r\n");
        return;
    }

    send_to_char(ch, "Você começa a girar rapidamente.\r\n");
    act("$n começa a girar rapidamente!", FALSE, ch, 0, 0, TO_ROOM);

    /* NEW_EVENT() will add a new mud event to the event list of the
       character. This function below adds a new event of "eWHIRLWIND", to
       "ch", and passes "NULL" as additional data. The event will be called in
       "3 * PASSES_PER_SEC" or 3 seconds */
    NEW_EVENT(eWHIRLWIND, ch, NULL, 3 * PASSES_PER_SEC);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_kick)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int percent, prob;

    if (!IS_NPC(ch) && !GET_SKILL(ch, SKILL_KICK)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
            vict = FIGHTING(ch);
        } else {
            send_to_char(ch, "Chutar quem?\r\n");
            return;
        }
    }
    if (vict == ch) {
        send_to_char(ch, "Chega de gracinhas hoje...\r\n");
        return;
    }

    else if ((!CONFIG_PK_ALLOWED && !IS_NPC(vict))) {
        act("Use 'murder' se você realmente deseja atacar $N.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    /* 101% is a complete failure */
    percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + rand_number(1, 101);

    if (IS_NPC(ch))
        prob = GET_LEVEL(ch);
    else
        prob = GET_SKILL(ch, SKILL_KICK);

    if (percent > prob)
        damage(ch, vict, 0, SKILL_KICK);
    else
        damage(ch, vict, GET_LEVEL(ch), SKILL_KICK);

    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_bandage)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int percent, prob;
    if (!GET_SKILL(ch, SKILL_BANDAGE)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    if (GET_POS(ch) != POS_STANDING) {
        send_to_char(ch, "Você não está na posição adequada!\r\n");
        return;
    }
    if (IS_DEAD(ch)) {
        act("Você está mort$r!! Não pode fazer curativos em ninguém!", FALSE, ch, NULL, NULL, TO_CHAR);
        return;
    }
    one_argument(argument, arg);
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Quem você quer fazer estabilizar a condição?\r\n");
        return;
    }
    if (IS_DEAD(vict)) {
        act("$L está mort$R!! Como você pretende fazer curativos??", FALSE, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (GET_HIT(vict) >= 0) {
        send_to_char(ch, "Você só pode estabilizar quem está \x1B[0;31mpróximo da morte!\tn\r\n");
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_BANDAGE);
    if (percent <= prob) {
        act("A sua tentativa de estabilizar a condição de $N falha.", FALSE, ch, 0, vict, TO_CHAR);
        act("$n tenta estabilizar a condição de $N mas falha miseravelmente.", TRUE, ch, 0, vict, TO_NOTVICT);
        act("Alguém tenta estabilizar a tua condição mas falha miseravelmente.", TRUE, ch, 0, vict, TO_VICT);
        damage(vict, vict, 2, TYPE_SUFFERING);
        return;
    }

    act("Você consegue estabilizar $N com sucesso.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n estabiliza a condição de $N, que parece um pouco melhor agora.", TRUE, ch, 0, vict, TO_NOTVICT);
    act("Alguém tenta estabilizar a tua condição, e você se sente melhor agora.", FALSE, ch, 0, vict, TO_VICT);
    GET_HIT(vict) = 0;
}

ACMD(do_trip)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    int percent, prob;

    one_argument(argument, arg);

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRIP)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    } else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
            vict = FIGHTING(ch);
        } else {
            send_to_char(ch, "Dar uma rasteira em quem?\r\n");
            return;
        }
    } else if (vict == ch) {
        send_to_char(ch, "Haha... Muito engraçado... Você é palhaço de circo?\r\n");
        return;
    } else if ((!CONFIG_PK_ALLOWED && !IS_NPC(vict))) {
        act("Use 'murder' se você realmente deseja atacar $N.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_TRIP);

    if (MOB_FLAGGED(vict, MOB_NOBASH) || MOB_FLAGGED(vict, MOB_MOUNTABLE) || MOB_FLAGGED(vict, MOB_CAN_FLY))
        percent = 101;

    if (percent > prob) {
        damage(ch, vict, 0, SKILL_TRIP);
        if (damage(ch, ch, 2, TYPE_UNDEFINED) > 0 && (GET_LEVEL(ch) < LVL_GOD)) {
            GET_POS(ch) = POS_SITTING;
            GET_WAIT_STATE(ch) += 3 * PULSE_VIOLENCE;
        }
    } else {
        if (damage(ch, vict, 2, SKILL_TRIP) > 0) {   // -1 = dead, 0 = miss
            GET_WAIT_STATE(vict) += 2 * PULSE_VIOLENCE;
            if (IN_ROOM(ch) == IN_ROOM(vict))
                GET_POS(vict) = POS_SITTING;
        }

        if (GET_LEVEL(ch) < LVL_GOD)
            GET_WAIT_STATE(ch) += 4 * PULSE_VIOLENCE;
    }
}

/* -- jr - Sep 15, 2000
 * updated by Cansian
 * SEIZE skill.
 */
ACMD(do_seize)
{
    struct char_data *vict;
    int percent, prob;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SEIZE) || PLR_FLAGGED(ch, PLR_TRNS)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "Este lugar é muito calmo, sem violências...\r\n");
        return;
    }

    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
            vict = FIGHTING(ch);
        else {
            send_to_char(ch, "Imobilizar quem?\r\n");
            return;
        }
    }
    if (vict == ch) {
        send_to_char(ch, "Ta... entendi... faz muito sentido...\r\n");
        return;
    }

    if (IS_DEAD(ch)) {
        act("Você não pode imobilizar $N, você está mort$r!", FALSE, ch, NULL, vict, TO_CHAR);
        return;
    } else if (IS_DEAD(vict)) {
        act("Como você pretende imobilizar $N? $E está mort$R!", FALSE, ch, NULL, vict, TO_CHAR);
        return;
    } else if (AFF_FLAGGED(vict, AFF_PARALIZE)) {
        act("$E já está paralizad$R. Como você pretende imobilizar alguém assim?", FALSE, ch, NULL, NULL, TO_CHAR);
        return;
    }
    /*
    else if (IS_MOUNTING(ch)) {
      act("Você está montando, não há como imobilizar alguém!", FALSE, ch, NULL, NULL, TO_CHAR);
      return;
    } else if (IS_MOUNTED(vict)) {
      act("$N está montad$R. Não há como agarrá-l$R!", FALSE, ch, NULL, vict, TO_CHAR);
      return;
   */
    percent = rand_number(1, 101) - dex_app_skill[GET_DEX(ch)].seize;
    prob = GET_SKILL(ch, SKILL_SEIZE);

    if (GET_STR(vict) > GET_STR(ch))   // vict is stronger than ch
        percent += (GET_STR(vict) - GET_STR(ch)) * 10;

    if (MOB_FLAGGED(vict, MOB_NOBASH))
        percent = 101;

    if (percent > prob) {
        act("Você tenta imobilizar $N, mas acaba sendo arremessad$r longe.", FALSE, ch, NULL, vict, TO_CHAR);
        act("$n tenta imobilizar $N, mas é arremessad$r longe.", FALSE, ch, NULL, vict, TO_NOTVICT);
        act("$n tenta imobilizar você, mas você facimente $r arremessa longe.", FALSE, ch, NULL, vict, TO_VICT);
        GET_POS(ch) = POS_SITTING;
        GET_WAIT_STATE(ch) += 2 * PULSE_VIOLENCE;
    } else {
        int immovable = rand_number(2, 4);

        act("Você conseguiu imobilizar $N!", FALSE, ch, NULL, vict, TO_CHAR);
        act("$n segura $N por trás, imobilizando-$R!", FALSE, ch, NULL, vict, TO_NOTVICT);
        act("$n segura você por trás, lhe deixando imóvel!", FALSE, ch, NULL, vict, TO_VICT);
        GET_WAIT_STATE(ch) += immovable * PULSE_VIOLENCE;
        GET_WAIT_STATE(vict) += immovable * PULSE_VIOLENCE;
    }
}

ACMD(do_shoot)
{
    struct char_data *vict;
    struct obj_data *ammo, *fireweapon;
    int percent, prob;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct affected_type af;
    int dam = 0;
    int door;
    bool found = FALSE;

    room_rnum vict_room = IN_ROOM(ch);
    room_rnum was_room = IN_ROOM(ch);
    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Atirar em qual direção?\r\n");
        return;
    }
    if (!*arg2) {
        send_to_char(ch, "Atirar em quem?\r\n");
        return;
    }
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BOWS)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    } else if (IS_AFFECTED(ch, AFF_BLIND)) {
        send_to_char(ch, "Você não pode ver nada! Você está ceg%s!\r\n", OA(ch));
        return;
    } else if ((ammo = GET_EQ(ch, WEAR_QUIVER)) == NULL) {
        send_to_char(ch, "Você está com a bolsa de munições vazia.\r\n");
        return;
    }

    fireweapon = GET_EQ(ch, WEAR_WIELD);
    if (!fireweapon) {
        send_to_char(ch, "Você precisa de uma arma!\r\n");
        return;
    } else if (GET_OBJ_TYPE(fireweapon) != ITEM_FIREWEAPON) {
        send_to_char(ch, "Este tipo de arma não permite atirar.\r\n");
        return;
    }

    percent = rand_number(1, 101); /* 101% is a complete failure */
    if (!IS_NPC(ch))
        prob = GET_SKILL(ch, SKILL_BOWS);
    else
        prob = GET_LEVEL(ch);

    prob += GET_OBJ_VAL(fireweapon, 0);

    if (percent > prob) {
        send_to_char(ch, "Você tenta atirar mas se atrapalha.\r\n");
        return;
    }

    if ((door = search_block(arg1, dirs, FALSE)) == -1) {          /* Partial Match */
        if ((door = search_block(arg1, autoexits, FALSE)) == -1) { /* Check 'short' dirs too */
            send_to_char(ch, "Isso não é uma direção.\r\n");
            return;
        }
    }
    if (world[IN_ROOM(ch)].dir_option[door] && world[IN_ROOM(ch)].dir_option[door]->to_room != NOWHERE &&
        !IS_SET(world[IN_ROOM(ch)].dir_option[door]->exit_info, EX_CLOSED) &&
        !IS_SET(world[IN_ROOM(ch)].dir_option[door]->exit_info, EX_HIDDEN)) {
        vict_room = world[IN_ROOM(ch)].dir_option[door]->to_room;
        if (IS_DARK(vict_room) && !CAN_SEE_IN_DARK(ch)) {
            if (world[vict_room].people)
                send_to_char(ch, "%s: Está tudo escuro... Mas você escuta ruídos\r\n", dirs_pt[door]);
            else
                send_to_char(ch, "%s: Está tudo escuro...\r\n", dirs_pt[door]);
            found = TRUE;
        } else {
            if (world[vict_room].people) {

                char_from_room(ch);
                char_to_room(ch, vict_room);
                if ((vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM)) != NULL) {
                    if (vict == ch) {
                        send_to_char(ch, "Melhor não tentar isso...\r\n");
                        return;
                    }
                    char_from_room(ch);
                    char_to_room(ch, was_room);
                    send_to_char(ch, "Você se concentra e atira.\r\n");
                    act("$n se concentra e atira.", FALSE, ch, 0, 0, TO_ROOM);
                    dam += GET_DAMROLL(ch);
                    dam += dice(GET_OBJ_VAL(ammo, 1), GET_OBJ_VAL(ammo, 2));
                    if (GET_POS(vict) < POS_FIGHTING)
                        dam *= 1 + (POS_FIGHTING - GET_POS(vict)) / 3;
                    /* at least 1 hp damage min per hit */
                    dam = MAX(1, dam);
                    if (OBJ_FLAGGED(ammo, ITEM_POISONED) && !MOB_FLAGGED(vict, MOB_NO_POISON)) {
                        new_affect(&af);
                        af.spell = SPELL_POISON;
                        af.modifier = -2;
                        af.location = APPLY_STR;

                        if (AFF_FLAGGED(vict, AFF_POISON))
                            af.duration = 1;
                        else {
                            /* Use level-based duration to match MAGIA-POISON help documentation */
                            af.duration = GET_LEVEL(ch);
                            act("Você se sente doente.", FALSE, vict, 0, ch, TO_CHAR);
                            act("$n fica muito doente!", TRUE, vict, 0, ch, TO_ROOM);
                        }
                        SET_BIT_AR(af.bitvector, AFF_POISON);
                        affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);
                    }
                    damage(ch, vict, dam, GET_OBJ_VAL(ammo, 3) + TYPE_HIT);
                    remember(ch, vict);
                    if (rand_number(0, 1) == 1)
                        hunt_victim(ch);
                    hitprcnt_mtrigger(vict);
                    if (GET_OBJ_VAL(ammo, 0) > 1) {
                        GET_OBJ_WEIGHT(ammo) -= (GET_OBJ_WEIGHT(ammo) / GET_OBJ_VAL(ammo, 0));
                        GET_OBJ_VAL(ammo, 0)--;
                    } else
                        extract_obj(ammo);
                    found = TRUE;
                }
                char_from_room(ch);
                char_to_room(ch, was_room);
            } else {
                send_to_char(ch, "Você tenta mas não consegue acertar ninguém.\r\n");
                return;
            }
        }
    }   // end of if

    if (!found) {
        send_to_char(ch, "Você não vê nada próximo.\r\n");
    }
}
