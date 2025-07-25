/**************************************************************************
 *  File: act.item.c                                        Part of tbaMUD *
 *  Usage: Object handling routines -- get/drop and container handling.    *
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
#include "constants.h"
#include "dg_scripts.h"
#include "oasis.h"
#include "act.h"
#include "quest.h"

/* local function prototypes */
/* do_get utility functions */
static int can_take_obj(struct char_data *ch, struct obj_data *obj);
void get_check_money(struct char_data *ch, struct obj_data *obj);
static void get_from_container(struct char_data *ch, struct obj_data *cont, char *arg, int mode, int amount);
static void get_from_room(struct char_data *ch, char *arg, int amount);
static void perform_get_from_container(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode);
/* do_give utility functions */
static struct char_data *give_find_vict(struct char_data *ch, char *arg);
void perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj);
void perform_give_gold(struct char_data *ch, struct char_data *vict, int amount);
/* do_drop utility functions */
int perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *sname, room_rnum RDR);
void perform_drop_gold(struct char_data *ch, int amount, byte mode, room_rnum RDR);
/* do_put utility functions */
void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont);
/* do_remove utility functions */
void perform_remove(struct char_data *ch, int pos);
/* do_wear utility functions */
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
static void wear_message(struct char_data *ch, struct obj_data *obj, int where);

void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont)
{

    if (!drop_otrigger(obj, ch))
        return;

    if (!obj) /* object might be extracted by drop_otrigger */
        return;

    if ((GET_OBJ_VAL(cont, 0) > 0) && (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0)))
        act("$p não cabe em $P.", FALSE, ch, obj, cont, TO_CHAR);
    else if (OBJ_FLAGGED(obj, ITEM_NODROP) && IN_ROOM(cont) != NOWHERE)
        act("Você não pode se livrar de $p, parece ser uma MALDIÇÃO!", FALSE, ch, obj, NULL, TO_CHAR);
    else {
        obj_from_char(obj);
        obj_to_obj(obj, cont);

        act("$n coloca $p em $P.", TRUE, ch, obj, cont, TO_ROOM);

        /* Yes, I realize this is strange until we have auto-equip on rent.
           -gg */
        if (OBJ_FLAGGED(obj, ITEM_NODROP) && !OBJ_FLAGGED(cont, ITEM_NODROP)) {
            SET_BIT_AR(GET_OBJ_EXTRA(cont), ITEM_NODROP);
            act("Você sente algo estranho ao colocar $p em $P.", FALSE, ch, obj, cont, TO_CHAR);
        } else
            act("Você coloca $p em $P.", FALSE, ch, obj, cont, TO_CHAR);
    }
}

/* The following put modes are supported: 1) put <object> <container> 2) put
   all.<object> <container> 3) put all <container> The <container> must be in
   inventory or on ground. All objects to be put into container must be in
   inventory. */
ACMD(do_put)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    struct obj_data *obj, *next_obj, *cont, *quiver;
    struct char_data *tmp_char;
    int obj_dotmode, cont_dotmode, found = 0, howmany = 1;
    char *theobj, *thecont;

    one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments
                                                              */

    if (*arg3 && is_number(arg1)) {
        howmany = atoi(arg1);
        theobj = arg2;
        thecont = arg3;
    } else {
        theobj = arg1;
        thecont = arg2;
    }
    obj_dotmode = find_all_dots(theobj);
    cont_dotmode = find_all_dots(thecont);

    if (!*theobj)
        send_to_char(ch, "Colocar o que aonde?\r\n");
    else if (cont_dotmode != FIND_INDIV)
        send_to_char(ch, "Você pode apenas colocar coisas em um recipiente de cada vez.\r\n");
    else if (!*thecont) {
        send_to_char(ch, "Aonde você pretende colocar isso?\r\n");
    } else {
        if (strcmp(thecont, "quiver")) {
            generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
            if (!cont)
                send_to_char(ch, "Você não vê um(a) %s aqui.\r\n", thecont);
            else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) && (GET_OBJ_TYPE(cont) != ITEM_CORPSE))
                act("$p não é um recipiente.", FALSE, ch, cont, 0, TO_CHAR);
            else if (OBJVAL_FLAGGED(cont, CONT_CLOSED) &&
                     (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
                send_to_char(ch, "Seria melhor abrir isso primeiro!\r\n");
            else {
                if (obj_dotmode == FIND_INDIV) { /* put <obj> <container> */
                    if (!(obj = get_obj_in_list_vis(ch, theobj, NULL, ch->carrying)))
                        send_to_char(ch, "Não há mais %s em seu inventário.\r\n", theobj);
                    else if (obj == cont && howmany == 1)
                        send_to_char(ch, "Você tenta dobrar %s mas não consegue.\r\n", obj->short_description);
                    else {
                        while (obj && howmany) {
                            next_obj = obj->next_content;
                            if (obj != cont) {
                                howmany--;
                                perform_put(ch, obj, cont);
                            }
                            obj = get_obj_in_list_vis(ch, theobj, NULL, next_obj);
                        }
                    }
                } else {
                    for (obj = ch->carrying; obj; obj = next_obj) {
                        next_obj = obj->next_content;
                        if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
                            (obj_dotmode == FIND_ALL || isname(theobj, obj->name))) {
                            found = 1;
                            perform_put(ch, obj, cont);
                        }
                    }
                    if (!found) {
                        if (obj_dotmode == FIND_ALL)
                            send_to_char(ch, "Não há mais nada em seu inventário.\r\n");
                        else
                            send_to_char(ch, "Não há %s em seu inventário.\r\n", theobj);
                    }
                }
            }
        } else {
            generic_find(theobj, FIND_OBJ_INV, ch, &tmp_char, &obj);
            if (!obj)
                send_to_char(ch, "Você não tem %s para colocar ai.\r\n", theobj);
            else if (GET_OBJ_TYPE(obj) == ITEM_AMMO) {
                if ((quiver = GET_EQ(ch, WEAR_QUIVER)) != NULL) {
                    if (GET_OBJ_VNUM(obj) == GET_OBJ_VNUM(quiver)) {
                        GET_OBJ_VAL(quiver, 0) += GET_OBJ_VAL(obj, 0);
                        GET_OBJ_WEIGHT(quiver) += GET_OBJ_WEIGHT(obj);
                        extract_obj(obj);
                        send_to_char(ch, "Você colocou mais %s em sua bolsa de munições.\r\n", theobj);
                    } else {
                        send_to_char(ch, "Você já tem um outro tipo de munição ai.\r\n");
                    }
                } else {
                    perform_wear(ch, obj, WEAR_QUIVER);
                }
            } else
                send_to_char(ch, "Você só pode colocar munições ai.\r\n");
        }
    }
}

static int can_take_obj(struct char_data *ch, struct obj_data *obj)
{
    if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
        act("$p: você não pode pegar isso!", FALSE, ch, obj, 0, TO_CHAR);
        return (0);
    }

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
            act("$p: você não pode carregar mais coisas.", FALSE, ch, obj, 0, TO_CHAR);
            return (0);
        } else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
            act("$p: você não pode carregar tanto peso.", FALSE, ch, obj, 0, TO_CHAR);
            return (0);
        } else if (IS_DEAD(ch) || GET_POS(ch) <= POS_DEAD) {
            act("$p: você não pode carregar nada nestas condições!", FALSE, ch, obj, 0, TO_CHAR);
            return (0);
        } else if (PLR_FLAGGED(ch, PLR_TRNS)) {
            act("$p: você não pode pegar nada!", FALSE, ch, obj, 0, TO_CHAR);
            return (0);
        }
    }

    if (OBJ_SAT_IN_BY(obj)) {
        act("Parece que tem alguem em $p..", FALSE, ch, obj, 0, TO_CHAR);
        return (0);
    }

    return (1);
}

void get_check_money(struct char_data *ch, struct obj_data *obj)
{
    int value = GET_OBJ_VAL(obj, 0);

    if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
        return;

    extract_obj(obj);

    increase_gold(ch, value);

    if (value == 1)
        send_to_char(ch, "Há uma moeda.\r\n");
    else
        send_to_char(ch, "Há %d moedas.\r\n", value);
}

static void perform_get_from_container(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode)
{
    if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
        if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
            act("$p: você não pode carregar mais coisas.", FALSE, ch, obj, 0, TO_CHAR);
        else if (get_otrigger(obj, ch)) {
            obj_from_obj(obj);
            obj_to_char(obj, ch);
            act("Você pega $p de dentro de $P.", FALSE, ch, obj, cont, TO_CHAR);
            act("$n pega $p de dentro de $P.", TRUE, ch, obj, cont, TO_ROOM);
            get_check_money(ch, obj);
        }
    }
}

void get_from_container(struct char_data *ch, struct obj_data *cont, char *arg, int mode, int howmany)
{
    struct obj_data *obj, *next_obj;
    int obj_dotmode, found = 0;

    obj_dotmode = find_all_dots(arg);

    if (OBJVAL_FLAGGED(cont, CONT_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
        act("Seria melhor abrir $p antes.", FALSE, ch, cont, 0, TO_CHAR);
    else if (obj_dotmode == FIND_INDIV) {
        if (!(obj = get_obj_in_list_vis(ch, arg, NULL, cont->contains))) {
            char buf[MAX_STRING_LENGTH];

            snprintf(buf, sizeof(buf), "Não há %s em $p.", arg);
            act(buf, FALSE, ch, cont, 0, TO_CHAR);
        } else {
            struct obj_data *obj_next;
            while (obj && howmany--) {
                obj_next = obj->next_content;
                perform_get_from_container(ch, obj, cont, mode);
                obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
            }
        }
    } else {
        if (obj_dotmode == FIND_ALLDOT && !*arg) {
            send_to_char(ch, "Pegar tudo do quê?\r\n");
            return;
        }
        for (obj = cont->contains; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (CAN_SEE_OBJ(ch, obj) && (obj_dotmode == FIND_ALL || isname(arg, obj->name))) {
                found = 1;
                perform_get_from_container(ch, obj, cont, mode);
            }
        }
        if (!found) {
            if (obj_dotmode == FIND_ALL)
                act("Parece não haver nada dentro de $p.", FALSE, ch, cont, 0, TO_CHAR);
            else {
                char buf[MAX_STRING_LENGTH];

                snprintf(buf, sizeof(buf), "Não há %s em $p..", arg);
                act(buf, FALSE, ch, cont, 0, TO_CHAR);
            }
        }
    }
}

int perform_get_from_room(struct char_data *ch, struct obj_data *obj)
{
    if (can_take_obj(ch, obj) && get_otrigger(obj, ch)) {
        obj_from_room(obj);
        obj_to_char(obj, ch);
        act("Você pega $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n pega $p.", TRUE, ch, obj, 0, TO_ROOM);
        get_check_money(ch, obj);
        return (1);
    }
    return (0);
}

static void get_from_room(struct char_data *ch, char *arg, int howmany)
{
    struct obj_data *obj, *next_obj;
    int dotmode, found = 0;

    dotmode = find_all_dots(arg);

    if (dotmode == FIND_INDIV) {
        if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
            /* Are they trying to take something in a room extra description? */
            if (find_exdesc(arg, world[IN_ROOM(ch)].ex_description) != NULL) {
                send_to_char(ch, "Você não pode pegar isto.\r\n");
                return;
            }
            send_to_char(ch, "Você parece não encontrar %s por aqui.\r\n", arg);
        } else {
            struct obj_data *obj_next;
            while (obj && howmany--) {
                obj_next = obj->next_content;
                perform_get_from_room(ch, obj);
                obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
            }
        }
    } else {
        if (dotmode == FIND_ALLDOT && !*arg) {
            send_to_char(ch, "Pegar tudo do quê?\r\n");
            return;
        }
        for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (CAN_SEE_OBJ(ch, obj) && (dotmode == FIND_ALL || isname(arg, obj->name))) {
                found = 1;
                perform_get_from_room(ch, obj);
            }
        }
        if (!found) {
            if (dotmode == FIND_ALL)
                send_to_char(ch, "Não parece haver nada aquí.\r\n");
            else
                send_to_char(ch, "Não há mais %s por aqui.\r\n", arg);
        }
    }
}

ACMD(do_get)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];

    int cont_dotmode, found = 0, mode;
    struct obj_data *cont;
    struct char_data *tmp_char;

    one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments
                                                              */

    if (!*arg1)
        send_to_char(ch, "Pegar o quê?\r\n");
    else if (!*arg2)
        get_from_room(ch, arg1, 1);
    else if (is_number(arg1) && !*arg3)
        get_from_room(ch, arg2, atoi(arg1));
    else {
        int amount = 1;
        if (is_number(arg1)) {
            amount = atoi(arg1);
            strcpy(arg1, arg2); /* strcpy: OK (sizeof: arg1 == arg2) */
            strcpy(arg2, arg3); /* strcpy: OK (sizeof: arg2 == arg3) */
        }
        cont_dotmode = find_all_dots(arg2);
        if (cont_dotmode == FIND_INDIV) {
            mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
            if (!cont)
                send_to_char(ch, "Você não tem um %s.\r\n", arg2);
            else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) && (GET_OBJ_TYPE(cont) != ITEM_CORPSE))
                act("$p não é um recipiente", FALSE, ch, cont, 0, TO_CHAR);
            else if ((IS_CORPSE(cont)) && GET_OBJ_VAL(cont, 0) != 0)
                act("Você não pode tocar em $p!", FALSE, ch, cont, 0, TO_CHAR);
            else
                get_from_container(ch, cont, arg1, mode, amount);
        } else {
            if (cont_dotmode == FIND_ALLDOT && !*arg2) {
                send_to_char(ch, "Pegar tudo o quê?\r\n");
                return;
            }
            for (cont = ch->carrying; cont; cont = cont->next_content)
                if (CAN_SEE_OBJ(ch, cont) && (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
                    if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) ||
                        ((GET_OBJ_TYPE(cont) == ITEM_CORPSE) && GET_OBJ_VAL(cont, 0) == 0)) {
                        found = 1;
                        get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
                    } else if (cont_dotmode == FIND_ALLDOT) {
                        found = 1;
                        act("$p não é recipiente.", FALSE, ch, cont, 0, TO_CHAR);
                    }
                }
            for (cont = world[IN_ROOM(ch)].contents; cont; cont = cont->next_content)
                if (CAN_SEE_OBJ(ch, cont) && (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
                    if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) ||
                        ((GET_OBJ_TYPE(cont) == ITEM_CORPSE) && GET_OBJ_VAL(cont, 0) == 0)) {
                        get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
                        found = 1;
                    } else if (cont_dotmode == FIND_ALLDOT) {
                        act("$p não é recipiente.", FALSE, ch, cont, 0, TO_CHAR);
                        found = 1;
                    }
                }
            if (!found) {
                if (cont_dotmode == FIND_ALL)
                    send_to_char(ch, "Não há nenhum recipiente por aqui.\r\n");
                else
                    send_to_char(ch, "Não há %s por aqui.\r\n", arg2);
            }
        }
    }
}

void perform_drop_gold(struct char_data *ch, int amount, byte mode, room_rnum RDR)
{
    struct obj_data *obj;

    if (PLR_FLAGGED(ch, PLR_JAILED))
        send_to_char(ch, "Melhor não tentar isso.");

    if (amount <= 0)
        send_to_char(ch, "Heh heh heh... você está bem engracad%s hoje, não?", OA(ch));
    else if (GET_GOLD(ch) < amount)
        send_to_char(ch, "Você não tem tantas moedas!");
    else {
        if (mode != SCMD_JUNK) {
            WAIT_STATE(ch, PULSE_VIOLENCE); /* to prevent coin-bombing */
            obj = create_money(amount);
            if (mode == SCMD_DONATE) {
                send_to_char(ch, "Você joga um punhado de ouro para o ar que desaparece em uma nuvem de fumaça!\r\n");
                act("$n joga um punhado de ouro para o ar que desaparece em uma nuvem de fumaça!!", FALSE, ch, 0, 0,
                    TO_ROOM);
                obj_to_room(obj, RDR);
                act("$p repentinamente aparece em uma nuvem de fumaça", 0, 0, obj, 0, TO_ROOM);
            } else {
                char buf[MAX_STRING_LENGTH];

                if (!drop_wtrigger(obj, ch)) {
                    extract_obj(obj);
                    return;
                }

                snprintf(buf, sizeof(buf), "$n solta %s.", money_desc(amount));
                act(buf, TRUE, ch, 0, 0, TO_ROOM);

                send_to_char(ch, "Você solta um punhado de ouro.\r\n");
                obj_to_room(obj, IN_ROOM(ch));
            }
        } else {
            char buf[MAX_STRING_LENGTH];

            snprintf(buf, sizeof(buf), "$n solta %s  que desaparece em uma nuvem de fumaça!", money_desc(amount));
            act(buf, FALSE, ch, 0, 0, TO_ROOM);

            send_to_char(ch, "Você solta um punhado de ouro que desaparece em uma nuvem de fumaça!\r\n");
        }
        decrease_gold(ch, amount);
    }
}

#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? ", que desaparece em uma nuvem de fumaça!" : ".")
int perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *sname, room_rnum RDR)
{
    char buf[MAX_STRING_LENGTH];
    int value;

    if (!drop_otrigger(obj, ch))
        return 0;

    if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
        return 0;

    if (OBJ_FLAGGED(obj, ITEM_NODROP) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        snprintf(buf, sizeof(buf), "Você não pode %s $p, parece uma MALDIÇÃO!", sname);
        act(buf, FALSE, ch, obj, 0, TO_CHAR);
        return (0);
    }

    snprintf(buf, sizeof(buf), "Você %s $p%s", sname, VANISH(mode));
    act(buf, FALSE, ch, obj, 0, TO_CHAR);

    snprintf(buf, sizeof(buf), "$n %s $p%s", sname, VANISH(mode));
    act(buf, TRUE, ch, obj, 0, TO_ROOM);

    obj_from_char(obj);

    if ((mode == SCMD_DONATE) && OBJ_FLAGGED(obj, ITEM_NODONATE))
        mode = SCMD_JUNK;

    switch (mode) {
        case SCMD_DROP:
            obj_to_room(obj, IN_ROOM(ch));
            return (0);
        case SCMD_DONATE:
            obj_to_room(obj, RDR);
            act("$p repentinamente aparece em uma nuvem de fumaça!", FALSE, 0, obj, 0, TO_ROOM);
            return (0);
        case SCMD_JUNK:
            value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
            extract_obj(obj);
            return (value);
        default:
            log1("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
            /* SYSERR_DESC: This error comes from perform_drop() and is output
               when perform_drop() is called with an illegal 'mode' argument. */
            break;
    }

    return (0);
}

ACMD(do_drop)
{
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *obj, *next_obj;
    room_rnum RDR = 0;
    byte mode = SCMD_DROP;
    int dotmode, amount = 0, multi, num_don_rooms;
    const char *sname;

    switch (subcmd) {
        case SCMD_JUNK:
            sname = "descarta";
            mode = SCMD_JUNK;
            break;
        case SCMD_DONATE:
            sname = "doa";
            mode = SCMD_DONATE;
            /* fail + double chance for room 1 */
            num_don_rooms = (CONFIG_DON_ROOM_1 != NOWHERE) * 2 + (CONFIG_DON_ROOM_2 != NOWHERE) +
                            (CONFIG_DON_ROOM_3 != NOWHERE) + 1;
            switch (rand_number(0, num_don_rooms)) {
                case 0:
                    mode = SCMD_JUNK;
                    break;
                case 1:
                case 2:
                    RDR = real_room(CONFIG_DON_ROOM_1);
                    break;
                case 3:
                    RDR = real_room(CONFIG_DON_ROOM_2);
                    break;
                case 4:
                    RDR = real_room(CONFIG_DON_ROOM_3);
                    break;
            }
            if (RDR == NOWHERE) {
                send_to_char(ch, "Você não pode doar agora.\r\n");
                return;
            }
            break;
        default:
            sname = "solta";
            break;
    }

    argument = one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "O que você quer %sr?\r\n", sname);
        return;
    } else if (is_number(arg)) {
        multi = atoi(arg);
        one_argument(argument, arg);
        if (!str_cmp("moedas", arg) || !str_cmp("moeda", arg) || !str_cmp("ouro", arg))
            perform_drop_gold(ch, multi, mode, RDR);
        else if (multi <= 0)
            send_to_char(ch, "Hum... isso faz muito sentido...\r\n");
        else if (!*arg)
            send_to_char(ch, "O que você deseja %s %d?\r\n", sname, multi);
        else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
            send_to_char(ch, "Não há %s em seu inventário.\r\n", arg);
        else {
            do {
                next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
                amount += perform_drop(ch, obj, mode, sname, RDR);
                obj = next_obj;
            } while (obj && --multi);
        }
    } else {
        dotmode = find_all_dots(arg);

        /* Can't junk or donate all */
        if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
            if (subcmd == SCMD_JUNK)
                send_to_char(ch, "Vá até o Depósito de Lixo se você deseja descartar TUDO!\r\n");
            else
                send_to_char(ch, "Vá até a Sala de Doações para doar TUDO!\r\n");
            return;
        }
        if (dotmode == FIND_ALL) {
            if (!ch->carrying)
                send_to_char(ch, "Você não parece estar carregando nada.\r\n");
            else
                for (obj = ch->carrying; obj; obj = next_obj) {
                    next_obj = obj->next_content;
                    amount += perform_drop(ch, obj, mode, sname, RDR);
                }
        } else if (dotmode == FIND_ALLDOT) {
            if (!*arg) {
                send_to_char(ch, "Você deseja %sr tudo de que?\r\n", sname);
                return;
            }
            if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
                send_to_char(ch, "Você não parece ter %s.\r\n", arg);

            while (obj) {
                next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
                amount += perform_drop(ch, obj, mode, sname, RDR);
                obj = next_obj;
            }
        } else {
            if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
                send_to_char(ch, "Você não parece ter %s.\r\n", arg);
            else
                amount += perform_drop(ch, obj, mode, sname, RDR);
        }
    }

    if (amount && (subcmd == SCMD_JUNK)) {
        send_to_char(ch, "Você foi recompensad%s pelos deuses!", OA(ch));
        act("$n foi recompensad$r pelos deuses!", TRUE, ch, 0, 0, TO_ROOM);
        GET_GOLD(ch) += amount;
    }
}

void perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj)
{
    if (!give_otrigger(obj, ch, vict))
        return;
    if (!receive_mtrigger(vict, ch, obj))
        return;

    if (OBJ_FLAGGED(obj, ITEM_NODROP) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        act("Você não pode se livrar de $p!!  Eca!", FALSE, ch, obj, 0, TO_CHAR);
        return;
    } else if (PLR_FLAGGED(ch, PLR_TRNS) && GET_LEVEL(vict) < LVL_DEMIGOD) {
        act("$p: você não pode dar nada!", FALSE, ch, obj, vict, TO_CHAR);
        return;
    } else if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict) && GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) < LVL_IMMORT) {
        act("$p: $N não pode carregar mais coisas.", FALSE, ch, obj, vict, TO_CHAR);
        return;
    } else if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict) && GET_LEVEL(ch) < LVL_IMMORT &&
               GET_LEVEL(vict) < LVL_IMMORT) {
        act("$p: $N não pode carregar mais peso.", FALSE, ch, obj, vict, TO_CHAR);
        return;
    } else if (IS_DEAD(vict)) {
        act("$p: $N não pode carregar nada estando mort$R!", FALSE, ch, obj, vict, TO_CHAR);
        return;
    } else if (GET_POS(vict) <= POS_DEAD) {
        act("$p: $N não pode carregar nada nestas condições!", FALSE, ch, obj, vict, TO_CHAR);
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, vict);
    act("Você entrega $p para $N.", FALSE, ch, obj, vict, TO_CHAR);
    act("$n entrega $p para você.", FALSE, ch, obj, vict, TO_VICT);
    act("$n entrega $p para $N.", TRUE, ch, obj, vict, TO_NOTVICT);

    autoquest_trigger_check(ch, vict, obj, AQ_OBJ_RETURN);
}

/* utility function for give */
static struct char_data *give_find_vict(struct char_data *ch, char *arg)
{
    struct char_data *vict;

    skip_spaces(&arg);
    if (!*arg)
        send_to_char(ch, "Para quem?\r\n");
    else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
        send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (vict == ch)
        send_to_char(ch, "Esta pessoa não merece receber nada... :-P\r\n?\r\n");
    else
        return (vict);

    return (NULL);
}

void perform_give_gold(struct char_data *ch, struct char_data *vict, int amount)
{
    char buf[MAX_STRING_LENGTH];

    if (amount <= 0) {
        send_to_char(ch, "Heh heh heh... você está bem engracad%s hoje, não?", OA(ch));
        return;
    }
    if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))) {
        send_to_char(ch, "Você não tem tantas moedas!\r\n");
        return;
    } else if (IS_DEAD(vict)) {
        act("$N não pode carregar nada estando mort$R!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    } else if (GET_POS(vict) <= POS_DEAD) {
        act("$N não pode carregar nada nestas condições!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    } else if (PLR_FLAGGED(ch, PLR_TRNS) && GET_LEVEL(vict) < LVL_DEMIGOD) {
        act("Você não pode dar nada!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    } else if (PLR_FLAGGED(vict, PLR_TRNS)) {
        act("$N não pode carregar nada!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }
    send_to_char(ch, "%s", CONFIG_OK);

    snprintf(buf, sizeof(buf), "$n entrega para você %d moeda%s de ouro.", amount, amount == 1 ? "" : "s");
    act(buf, FALSE, ch, 0, vict, TO_VICT);

    snprintf(buf, sizeof(buf), "$n entrega %s para $N.", money_desc(amount));
    act(buf, TRUE, ch, 0, vict, TO_NOTVICT);

    if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
        decrease_gold(ch, amount);

    increase_gold(vict, amount);
    bribe_mtrigger(vict, ch, amount);
}

ACMD(do_give)
{
    char arg[MAX_STRING_LENGTH];
    int amount, dotmode;
    struct char_data *vict;
    struct obj_data *obj, *next_obj;

    argument = one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Dar o que a quem?\r\n");
    else if (is_number(arg)) {
        amount = atoi(arg);
        argument = one_argument(argument, arg);
        if (!str_cmp("moedas", arg) || !str_cmp("moeda", arg)) {
            one_argument(argument, arg);
            if ((vict = give_find_vict(ch, arg)) != NULL)
                perform_give_gold(ch, vict, amount);
            return;
        } else if (!*arg) /* Give multiple code. */
            send_to_char(ch, "Você deseja dar %d o quê?\r\n?\r\n", amount);
        else if (!(vict = give_find_vict(ch, argument)))
            return;
        else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
            send_to_char(ch, "Não há %s em seu inventário.\r\n", arg);
        else {
            while (obj && amount--) {
                next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
                perform_give(ch, vict, obj);
                obj = next_obj;
            }
        }
    } else {
        char buf1[MAX_INPUT_LENGTH];

        one_argument(argument, buf1);
        if (!(vict = give_find_vict(ch, buf1)))
            return;
        dotmode = find_all_dots(arg);
        if (dotmode == FIND_INDIV) {
            if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
                send_to_char(ch, "Não há %s em seu inventário.\r\n", arg);
            else
                perform_give(ch, vict, obj);
        } else {
            if (dotmode == FIND_ALLDOT && !*arg) {
                send_to_char(ch, "Tudo de quê?\r\n");
                return;
            }
            if (!ch->carrying)
                send_to_char(ch, "Você não parece estar carregando nada.\r\n");
            else
                for (obj = ch->carrying; obj; obj = next_obj) {
                    next_obj = obj->next_content;
                    if (CAN_SEE_OBJ(ch, obj) && ((dotmode == FIND_ALL || isname(arg, obj->name))))
                        perform_give(ch, vict, obj);
                }
        }
    }
}

void weight_change_object(struct obj_data *obj, int weight)
{
    struct obj_data *tmp_obj;
    struct char_data *tmp_ch;

    if (IN_ROOM(obj) != NOWHERE) {
        GET_OBJ_WEIGHT(obj) += weight;
    } else if ((tmp_ch = obj->carried_by)) {
        obj_from_char(obj);
        GET_OBJ_WEIGHT(obj) += weight;
        obj_to_char(obj, tmp_ch);
    } else if ((tmp_obj = obj->in_obj)) {
        obj_from_obj(obj);
        GET_OBJ_WEIGHT(obj) += weight;
        obj_to_obj(obj, tmp_obj);
    } else {
        log1("SYSERR: Unknown attempt to subtract weight from an object.");
        /* SYSERR_DESC: weight_change_object() outputs this error when weight
           is attempted to be removed from an object that is not carried or in
           another object. */
    }
}

void name_from_drinkcon(struct obj_data *obj)
{
    char *new_name;
    const char *liqname;

    if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
        return;

    if (obj->name == obj_proto[GET_OBJ_RNUM(obj)].name)
        obj->name = strdup(obj_proto[GET_OBJ_RNUM(obj)].name);

    liqname = drinknames[GET_OBJ_VAL(obj, 2)];

    remove_from_string(obj->name, liqname);
    new_name = right_trim_whitespace(obj->name);
    free(obj->name);
    obj->name = new_name;
}

void name_to_drinkcon(struct obj_data *obj, int type)
{
    char *new_name;

    if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
        return;

    CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
    sprintf(new_name, "%s %s", obj->name, drinknames[type]); /* sprintf: OK
                                                              */

    if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
        free(obj->name);

    obj->name = new_name;
}

ACMD(do_drink)
{
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *temp;
    struct affected_type af;
    int amount, weight;
    int on_ground = 0;

    one_argument(argument, arg);

    if (IS_NPC(ch)) /* Cannot use GET_COND() on mobs. */
        return;

    if (!*arg) {
        char buf[MAX_STRING_LENGTH];
        switch (SECT(IN_ROOM(ch))) {
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
            case SECT_UNDERWATER:
                if ((GET_COND(ch, HUNGER) > 20) && (GET_COND(ch, THIRST) > 0)) {
                    send_to_char(ch, "Não cabe mais nada no seu estômago!\r\n");
                }
                snprintf(buf, sizeof(buf), "$n toma um gole refrescante.");
                act(buf, TRUE, ch, 0, 0, TO_ROOM);
                send_to_char(ch, "Você toma um gole refrescante.\r\n");
                gain_condition(ch, THIRST, 1);
                if (GET_COND(ch, THIRST) > 20)
                    send_to_char(ch, "Você não sente mais sede.\r\n");
                return;
            default:
                send_to_char(ch, "Beber daonde?\r\n");
                return;
        }
    }
    if (!(temp = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
        if (!(temp = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
            send_to_char(ch, "Você não pode encontrar isso!\r\n");
            return;
        } else
            on_ground = 1;
    }
    if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) && (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
        send_to_char(ch, "Você não pode beber daí!\r\n");
        return;
    }
    if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
        send_to_char(ch, "Você precisa estar segurando isso para beber dele.\r\n");
        return;
    }
    if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
        /* The pig is drunk */
        send_to_char(ch, "Você não parece saber exatamente onde está sua boca.\r\n");
        act("$n tenta beber algo, mas erra a boca!", TRUE, ch, 0, 0, TO_ROOM);
        return;
    }
    if ((GET_COND(ch, HUNGER) > 20) && (GET_COND(ch, THIRST) > 0)) {
        send_to_char(ch, "Não cabe mais nada no seu estômago!\r\n");
        return;
    }
    if (GET_OBJ_VAL(temp, 1) < 1) {
        act("Não há mais nada dentro de $p.", FALSE, ch, temp, 0, TO_CHAR);
        return;
    }

    if (!consume_otrigger(temp, ch, OCMD_DRINK)) /* check trigger */
        return;

    if (subcmd == SCMD_DRINK) {
        char buf[MAX_STRING_LENGTH];

        snprintf(buf, sizeof(buf), "$n bebe %s de $p.", drinks[GET_OBJ_VAL(temp, 2)]);
        act(buf, TRUE, ch, temp, 0, TO_ROOM);

        send_to_char(ch, "Você bebe %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);

        if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
            amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
        else
            amount = rand_number(3, 10);

    } else {
        act("$n bebe um gole do conteúdo de $p.", TRUE, ch, temp, 0, TO_ROOM);
        send_to_char(ch, "Tem sabor de %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
        amount = 1;
    }

    amount = MIN(amount, GET_OBJ_VAL(temp, 1));

    /* You can't subtract more than the object weighs, unless its unlimited. */
    if (GET_OBJ_VAL(temp, 0) > 0) {
        weight = MIN(amount, GET_OBJ_WEIGHT(temp));
        weight_change_object(temp, -weight); /* Subtract amount */
    }

    gain_condition(ch, DRUNK, drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount / 4);
    gain_condition(ch, HUNGER, drink_aff[GET_OBJ_VAL(temp, 2)][HUNGER] * amount / 4);
    gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount / 4);

    if (GET_COND(ch, DRUNK) > 10)
        send_to_char(ch, "Você se sente bêbad%s.\r\n", OA(ch));

    if (GET_COND(ch, THIRST) > 20)
        send_to_char(ch, "Você não sente mais sede.\r\n");

    if (GET_COND(ch, HUNGER) > 20)
        send_to_char(ch, "Você está satisfeit%s.\r\n", OA(ch));

    if (GET_OBJ_VAL(temp, 3) && GET_LEVEL(ch) < LVL_IMMORT) { /* The crap was poisoned ! */
        send_to_char(ch, "Opa, isso estava com um gosto estranho!\r\n");
        act("$n tosse e faz alguns sons estranhos.", TRUE, ch, 0, 0, TO_ROOM);

        new_affect(&af);
        af.spell = SPELL_POISON;
        af.duration = amount * 3;
        SET_BIT_AR(af.bitvector, AFF_POISON);
        affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
    }
    /* Empty the container (unless unlimited), and no longer poison. */
    if (GET_OBJ_VAL(temp, 0) > 0) {
        GET_OBJ_VAL(temp, 1) -= amount;
        if (!GET_OBJ_VAL(temp, 1)) { /* The last bit */
            name_from_drinkcon(temp);
            GET_OBJ_VAL(temp, 2) = 0;
            GET_OBJ_VAL(temp, 3) = 0;
        }
    }
    return;
}

ACMD(do_eat)
{
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *food;
    struct affected_type af;
    int amount;

    one_argument(argument, arg);

    if (IS_NPC(ch)) /* Cannot use GET_COND() on mobs. */
        return;

    if (!*arg) {
        send_to_char(ch, "Comer o quê?\r\n");
        return;
    }
    if (!(food = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
        send_to_char(ch, "Você não parece ter um(a) %s.", arg);
        return;
    }
    if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) || (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
        do_drink(ch, argument, 0, SCMD_SIP);
        return;
    }
    if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_IMMORT)) {
        send_to_char(ch, "Você não pode comer isso!\r\n");
        return;
    }
    if (GET_COND(ch, HUNGER) > 20) { /* Stomach full */
        send_to_char(ch, "Você está muito chei%s para comer mais!", OA(ch));
        return;
    }

    if (!consume_otrigger(food, ch, OCMD_EAT)) /* check trigger */
        return;

    if (subcmd == SCMD_EAT) {
        act("Você come $p.", FALSE, ch, food, 0, TO_CHAR);
        act("$n come $p.", TRUE, ch, food, 0, TO_ROOM);
    } else {
        act("Você mordisca um pequeno pedaço de $p.", FALSE, ch, food, 0, TO_CHAR);
        act("$n prova um pedaço de $p.", TRUE, ch, food, 0, TO_ROOM);
    }

    amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

    gain_condition(ch, HUNGER, amount);

    if (GET_COND(ch, HUNGER) > 20)
        send_to_char(ch, "Você está satisfeit%s.\r\n", OA(ch));

    if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_IMMORT)) {
        /* The crap was poisoned ! */
        send_to_char(ch, "Opa, isto estava com um gosto muito estranho!\r\n");
        act("$n tosse e solta alguns sons estranhos.", FALSE, ch, 0, 0, TO_ROOM);

        new_affect(&af);
        af.spell = SPELL_POISON;
        af.duration = amount * 2;
        SET_BIT_AR(af.bitvector, AFF_POISON);
        affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
    }
    if (subcmd == SCMD_EAT)
        extract_obj(food);
    else {
        if (!(--GET_OBJ_VAL(food, 0))) {
            send_to_char(ch, "Não sobrou nada agora.\r\n");
            extract_obj(food);
        }
    }
}

ACMD(do_pour)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct obj_data *from_obj = NULL, *to_obj = NULL;
    int amount = 0;

    two_arguments(argument, arg1, arg2);

    if (subcmd == SCMD_POUR) {
        if (!*arg1) { /* No arguments */
            send_to_char(ch, "O conteúdo do quê você deseja derramar?\r\n");
            return;
        }
        if (!(from_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
            send_to_char(ch, "Você não tem %s!\r\n", arg1);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
            send_to_char(ch, "Você não pode derramar o conteúdo disso!\r\n");
            return;
        }
    }
    if (subcmd == SCMD_FILL) {
        if (!*arg1) { /* no arguments */
            send_to_char(ch, "O que você deseja encher?  E aonde?\r\n");
            return;
        }
        if (!(to_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
            send_to_char(ch, "Você não tem %s!\r\n", arg1);
            return;
        }
        if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
            act("Você não pode encher $p!", FALSE, ch, to_obj, 0, TO_CHAR);
            return;
        }
        if (!*arg2) { /* no 2nd argument */
            act("Você deseja $p aonde?", FALSE, ch, to_obj, 0, TO_CHAR);
            return;
        }
        if (!(from_obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {
            send_to_char(ch, "Não parece ter %s aqui.\r\n", arg2);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
            act("Você não pode encher nada em $p.", FALSE, ch, from_obj, 0, TO_CHAR);
            return;
        }
    }
    if (GET_OBJ_VAL(from_obj, 1) == 0) {
        act("Não há nada dentro de $p.", FALSE, ch, from_obj, 0, TO_CHAR);
        return;
    }
    if (subcmd == SCMD_POUR) { /* pour */
        if (!*arg2) {
            send_to_char(ch, "Derramar aonde?\r\n");
            return;
        }
        if (!str_cmp(arg2, "fora") || !str_cmp(arg2, "out")) {
            if (GET_OBJ_VAL(from_obj, 0) > 0) {
                act("$n esvazia $p.", TRUE, ch, from_obj, 0, TO_ROOM);
                act("Você esvazia $p.", FALSE, ch, from_obj, 0, TO_CHAR);

                weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty
                                                                            */

                name_from_drinkcon(from_obj);
                GET_OBJ_VAL(from_obj, 1) = 0;
                GET_OBJ_VAL(from_obj, 2) = 0;
                GET_OBJ_VAL(from_obj, 3) = 0;
            } else
                send_to_char(ch, "Você não pode esvaziar isto!\r\n");

            return;
        }
        if (!(to_obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
            send_to_char(ch, "Não parece ter %s aqui.\r\n", arg2);
            return;
        }
        if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) && (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
            send_to_char(ch, "Você não pode derramar nada dentro disso.\r\n");
            return;
        }
    }
    if (to_obj == from_obj) {
        send_to_char(ch, "Algo realmente improdutivo...\r\n");
        return;
    }
    if ((GET_OBJ_VAL(to_obj, 0) < 0) || (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)))) {
        send_to_char(ch, "Já existe outro líquido dentro de %s!", to_obj->short_description);
        return;
    }
    if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0))) {
        send_to_char(ch, "Não há espaço para mais líquido dentro de %s.", to_obj->short_description);
        return;
    }
    if (subcmd == SCMD_POUR)
        send_to_char(ch, "Você derrama %s de %s.\r\n", drinks[GET_OBJ_VAL(from_obj, 2)], from_obj->short_description);

    if (subcmd == SCMD_FILL) {
        act("Você gentilmente enche $p em $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
        act("$n gentilmente enche $p em $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
    }
    /* New alias */
    if (GET_OBJ_VAL(to_obj, 1) == 0)
        name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

    /* First same type liq. */
    GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

    /* Then how much to pour */
    if (GET_OBJ_VAL(from_obj, 0) > 0) {
        GET_OBJ_VAL(from_obj, 1) -= (amount = (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

        GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

        if (GET_OBJ_VAL(from_obj, 1) < 0) { /* There was too little */
            GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
            amount += GET_OBJ_VAL(from_obj, 1);
            name_from_drinkcon(from_obj);
            GET_OBJ_VAL(from_obj, 1) = 0;
            GET_OBJ_VAL(from_obj, 2) = 0;
            GET_OBJ_VAL(from_obj, 3) = 0;
        }
    } else {
        GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);
        amount = GET_OBJ_VAL(to_obj, 0);
    }
    /* Poisoned? */
    GET_OBJ_VAL(to_obj, 3) = (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));
    /* Weight change, except for unlimited. */
    if (GET_OBJ_VAL(from_obj, 0) > 0) {
        weight_change_object(from_obj, -amount);
    }
    weight_change_object(to_obj, amount); /* Add weight */
}

static void wear_message(struct char_data *ch, struct obj_data *obj, int where)
{
    const char *wear_messages[][2] = {
        {"$n acende $p.", "Você acende $p."},

        {"$n desliza $p em seu dedo anelar direito.", "Você desliza $p em seu dedo anelar direito."},

        {"$n desliza $p em seu dedo anelar esquerdo.", "Você desliza $p em seu dedo anelar esquerdo."},

        {"$n veste $p em volta do pescoço.", "Você veste $p em volta do pescoço."},

        {"$n veste $p em volta do pescoço.", "Você veste $p em volta do pescoço."},

        {"$n veste $p no corpo.", "Você veste $p no corpo."},

        {"$n veste $p na cabeça.", "Você veste $p na cabeça."},

        {"$n coloca $p nas pernas.", "Você coloca $p nas pernas."},

        {"$n veste $p nos pés.", "Você veste $p nos pés."},

        {"$n coloca $p nas mãos.", "Você coloca $p nas mãos."},

        {"$n veste $p nos braços.", "Você veste $p nos braços."},

        {"$n prende $p em volta do braço como um escudo.", "Você começa a usar $p como um escudo."},

        {"$n veste $p sobre o corpo.", "Você veste $p sobre o corpo."},

        {"$n veste $p em volta da cintura.", "Você veste $p em volta da cintura."},

        {"$n coloca $p em volta do pulso direito.", "Você coloca $p em volta de seu pulso direito."},

        {"$n coloca $p em volta do pulso esquerdo.", "Você coloca $p em volta de seu pulso esquerdo."},

        {"$n empunha $p.", "Você empunha $p."},

        {"$n segura $p.", "Você segura $p."},

        {"$n prende $p às costas, como asas.", "Você começa a usar $p como asas."},

        {"$n veste $p em sua orelha direita.", "Você veste $p em sua orelha direita."},

        {"$n veste $p em sua orelha esquerda.", "Você veste $p em sua orelha esquerda."},

        {"$n coloca $p em seu rosto.", "Você coloca $p em seu rosto."},

        {"$n coloca $p em seu nariz.", "Você coloca $p em seu nariz."},

        {"$n começa a usar $p.", "Você começa a usar $p."},

        {"$n coloca $p em sua bolsa de munições.", "Você coloca $p em sua bolsa de munições."},
    };

    act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
    act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}

void perform_wear(struct char_data *ch, struct obj_data *obj, int where)
{
    /*
     * ITEM_WEAR_TAKE is used for objects that do not require special bits
     * to be put into that position (e.g. you can hold any object, not just
     * an object with a HOLD bit.)
     */

    int wear_bitvectors[] = {ITEM_WEAR_TAKE,  ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,    ITEM_WEAR_NECK,
                             ITEM_WEAR_BODY,  ITEM_WEAR_HEAD,   ITEM_WEAR_LEGS,   ITEM_WEAR_FEET,    ITEM_WEAR_HANDS,
                             ITEM_WEAR_ARMS,  ITEM_WEAR_SHIELD, ITEM_WEAR_ABOUT,  ITEM_WEAR_WAIST,   ITEM_WEAR_WRIST,
                             ITEM_WEAR_WRIST, ITEM_WEAR_WIELD,  ITEM_WEAR_TAKE,   ITEM_WEAR_WINGS,   ITEM_WEAR_EAR,
                             ITEM_WEAR_EAR,   ITEM_WEAR_FACE,   ITEM_WEAR_NOSE,   ITEM_WEAR_INSIGNE, ITEM_WEAR_QUIVER};

    const char *already_wearing[] = {"Você já está usando uma luz.\r\n",
                                     "VOCE NUNCA DEVERIA ESTAR VENDO ESTA MENSAGEM.  POR FAVOR, REPORTE.\r\n",
                                     "Você já está usando algo em ambos seus dedos anelares.\r\n",
                                     "VOCE NUNCA DEVERIA ESTAR VENDO ESTA MENSAGEM.  POR FAVOR, REPORTE.\r\n",
                                     "Você não pode colocar mais nada em volta de seu pescoço.\r\n",
                                     "Você já está vestindo algo em seu corpo.\r\n",
                                     "Você já está vestindo algo em sua cabeça.\r\n",
                                     "Você já está vestindo algo em suas pernas.\r\n",
                                     "Você já está vestindo algo em seus pés.\r\n",
                                     "Você já está vestindo algo em suas mãos.\r\n",
                                     "Você já está vestindo algo em seus braços.\r\n",
                                     "Você já está usando um escudo.\r\n",
                                     "Você já está vestindo algo sobre seu corpo.\r\n",
                                     "Você já está vestindo algo em volta de sua cintura.\r\n",
                                     "VOCE NUNCA DEVERIA ESTAR VENDO ESTA MENSAGEM.  POR FAVOR, REPORTE.\r\n",
                                     "Você já está vestindo algo em ambos os pulsos.\r\n",
                                     "Você já tem uma arma empunhada.\r\n",
                                     "Você já está segurando algo.\r\n",
                                     "Você já está usando asas.\r\n",
                                     "VOCE NUNCA DEVERIA ESTAR VENDO ESTA MENSAGEM.  POR FAVOR, REPORTE.\r\n",
                                     "Você já está usando algo em ambas as orelhas.\r\n",
                                     "Você já está usando algo em seu rosto.\r\n",
                                     "Você já está usando algo em seu nariz.\r\n",
                                     "Você já está usando uma insígnia.\r\n",
                                     "Você já está usando algo na bolsa de munições.\r\n"};

    /* first, make sure that the wear position is valid. */
    if (!CAN_WEAR(obj, wear_bitvectors[where])) {
        act("Você não pode vestir $p desta forma.", FALSE, ch, obj, 0, TO_CHAR);
        return;
    }
    /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
    if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R) || (where == WEAR_EAR_R))
        if (GET_EQ(ch, where))
            where++;

    if (GET_EQ(ch, where)) {
        send_to_char(ch, "%s", already_wearing[where]);
        return;
    }

    /* Armas de 2 maos -- jr - 23/01/99 */
    if ((where == WEAR_WIELD) && GET_EQ(ch, WEAR_HOLD) && OBJ_FLAGGED(obj, ITEM_TWO_HANDS)) {
        send_to_char(ch, "Você precisa das duas mãos para usar esta arma.\r\n");
        return;
    } else if ((where == WEAR_HOLD) && GET_EQ(ch, WEAR_WIELD) && OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD), ITEM_TWO_HANDS)) {
        send_to_char(ch, "%s", already_wearing[where]);
        return;
    }

    /* See if a trigger disallows it */
    if (!wear_otrigger(obj, ch, where) || (obj->carried_by != ch))
        return;

    wear_message(ch, obj, where);
    obj_from_char(obj);
    equip_char(ch, obj, where);
}

int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg)
{
    int where = -1;

    const char *keywords[] = {"*RESERVED*", "dedo",    "*RESERVED*", "pescoco",    "*RESERVED*", "corpo",
                              "cabeca",     "pernas",  "pes",        "maos",       "bracos",     "escudo",
                              "sobre",      "cintura", "pulso",      "*RESERVED*", "*RESERVED*", "*RESERVED*",
                              "asas",       "orelha",  "*RESERVED*", "rosto",      "nariz",      "insignia",
                              "municao",    "\n"};

    if (!arg || !*arg) {
        if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
            where = WEAR_FINGER_R;
        if (CAN_WEAR(obj, ITEM_WEAR_NECK))
            where = WEAR_NECK_1;
        if (CAN_WEAR(obj, ITEM_WEAR_BODY))
            where = WEAR_BODY;
        if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
            where = WEAR_HEAD;
        if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
            where = WEAR_LEGS;
        if (CAN_WEAR(obj, ITEM_WEAR_FEET))
            where = WEAR_FEET;
        if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
            where = WEAR_HANDS;
        if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
            where = WEAR_ARMS;
        if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
            where = WEAR_SHIELD;
        if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
            where = WEAR_ABOUT;
        if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
            where = WEAR_WAIST;
        if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
            where = WEAR_WRIST_R;
        if (CAN_WEAR(obj, ITEM_WEAR_WINGS))
            where = WEAR_WINGS;
        if (CAN_WEAR(obj, ITEM_WEAR_EAR))
            where = WEAR_EAR_R;
        if (CAN_WEAR(obj, ITEM_WEAR_FACE))
            where = WEAR_FACE;
        if (CAN_WEAR(obj, ITEM_WEAR_NOSE))
            where = WEAR_NOSE;
        if (CAN_WEAR(obj, ITEM_WEAR_INSIGNE))
            where = WEAR_INSIGNE;
        if (CAN_WEAR(obj, ITEM_WEAR_QUIVER))
            where = WEAR_QUIVER;
    } else if ((where = search_block(arg, keywords, FALSE)) < 0)
        send_to_char(ch, "'%s'? Que parte do corpo é esta?\r\n", arg);

    return (where);
}

ACMD(do_wear)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct obj_data *obj, *next_obj;
    int where, dotmode, items_worn = 0;

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Vestir o quê?\r\n");
        return;
    }
    dotmode = find_all_dots(arg1);

    if (*arg2 && (dotmode != FIND_INDIV)) {
        send_to_char(ch, "Você não pode especificar o mesmo lugar do corpo para mais do que uma coisa!\r\n");
        return;
    }
    if (dotmode == FIND_ALL) {
        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
                if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
                    send_to_char(ch, "Você não tem experiência suficiente para usar isto!\r\n");
                else {
                    items_worn++;
                    perform_wear(ch, obj, where);
                }
            }
        }
        if (!items_worn)
            send_to_char(ch, "Você parece não ter nada que possa ser vestido.\r\n");
    } else if (dotmode == FIND_ALLDOT) {
        if (!*arg1) {
            send_to_char(ch, "Vestir tudo do quê?\r\n");
            return;
        }
        if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
            send_to_char(ch, "Você não parece ter nenhum %s.\r\n", arg1);
        else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
            send_to_char(ch, "Você não tem experiência suficiente para usar isto!\r\n");
        else
            while (obj) {
                next_obj = get_obj_in_list_vis(ch, arg1, NULL, obj->next_content);
                if ((where = find_eq_pos(ch, obj, 0)) >= 0)
                    perform_wear(ch, obj, where);
                else
                    act("Você não pode vestir $p.", FALSE, ch, obj, 0, TO_CHAR);
                obj = next_obj;
            }
    } else {
        if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
            send_to_char(ch, "Você não parece ter nenhum %s.\r\n", arg1);
        else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
            send_to_char(ch, "Você não tem experiência suficiente para usar isto!\r\n");
        else {
            if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
                perform_wear(ch, obj, where);
            else if (!*arg2)
                act("Você não pode vestir $p.", FALSE, ch, obj, 0, TO_CHAR);
        }
    }
}

ACMD(do_wield)
{
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *obj;

    one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Empunhar o quê?\r\n");
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
        send_to_char(ch, "Você não parece ter %s.\r\n", arg);
    else {
        if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
            send_to_char(ch, "Você não pode empunhar isso.\r\n");
        else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
            send_to_char(ch, "É muito pesado para você usar.\r\n");
        else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
            send_to_char(ch, "Você não tem experiência suficiente para usar isto.\r\n");
        else
            perform_wear(ch, obj, WEAR_WIELD);
    }
}

ACMD(do_grab)
{
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *obj;

    one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Segurar o quê?\r\n");
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
        send_to_char(ch, "Você não parece ter nenhum %s.\r\n", arg);
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
        send_to_char(ch, "Você não tem experiência suficiente para usar isto.\r\n");
    else {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
            perform_wear(ch, obj, WEAR_LIGHT);
        else if (GET_OBJ_TYPE(obj) == ITEM_AMMO)
            perform_wear(ch, obj, WEAR_QUIVER);
        else {
            if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND && GET_OBJ_TYPE(obj) != ITEM_STAFF &&
                GET_OBJ_TYPE(obj) != ITEM_SCROLL && GET_OBJ_TYPE(obj) != ITEM_POTION)
                send_to_char(ch, "Você não pode segurar isso.\r\n");
            else
                perform_wear(ch, obj, WEAR_HOLD);
        }
    }
}

void perform_remove(struct char_data *ch, int pos)
{
    struct obj_data *obj;

    if (!(obj = GET_EQ(ch, pos)))
        log1("SYSERR: perform_remove: bad pos %d passed.", pos);
    /* This error occurs when perform_remove() is passed a bad 'pos'
       (location) to remove an object from. */
    else if (OBJ_FLAGGED(obj, ITEM_NODROP) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
        act("Você não pode remover $p, parece uma MALDIÇÃO!", FALSE, ch, obj, 0, TO_CHAR);
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
        act("$p: você não pode carregar mais coisas!", FALSE, ch, obj, 0, TO_CHAR);
    else {
        if (!remove_otrigger(obj, ch))
            return;

        obj_to_char(unequip_char(ch, pos), ch);
        act("Você para de usar $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n parou de usar $p.", TRUE, ch, obj, 0, TO_ROOM);
    }
}

ACMD(do_remove)
{
    char arg[MAX_INPUT_LENGTH];
    int i, dotmode, found;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Remover o quê?\r\n");
        return;
    }
    dotmode = find_all_dots(arg);

    if (dotmode == FIND_ALL) {
        found = 0;
        for (i = 0; i < NUM_WEARS; i++)
            if (GET_EQ(ch, i)) {
                perform_remove(ch, i);
                found = 1;
            }
        if (!found)
            send_to_char(ch, "Você não está usando nada.\r\n");
    } else if (dotmode == FIND_ALLDOT) {
        if (!*arg)
            send_to_char(ch, "Remover tudo do quê?\r\n");
        else {
            found = 0;
            for (i = 0; i < NUM_WEARS; i++)
                if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) && isname(arg, GET_EQ(ch, i)->name)) {
                    perform_remove(ch, i);
                    found = 1;
                }
            if (!found)
                send_to_char(ch, "Você não parece estar usando nenhum %s.\r\n", arg);
        }
    } else {
        if ((i = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) < 0)
            send_to_char(ch, "Você não parece estar usando %s.\r\n", arg);
        else
            perform_remove(ch, i);
    }
}

ACMD(do_sac)
{
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *j, *jj, *next_thing2;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Sacrificar oquê?\n\r");
        return;
    }

    if (!(j = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)) &&
        (!(j = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))) {
        send_to_char(ch, "Não parece ter nenhum %s aqui.\r\n", arg);
        return;
    }

    if (!CAN_WEAR(j, ITEM_WEAR_TAKE)) {
        send_to_char(ch, "Você não pode sacrificar isto.\r\n");
        return;
    }

    act("$n sacrifica $p.", FALSE, ch, j, 0, TO_ROOM);

    switch (rand_number(0, 5)) {
        case 0:
            send_to_char(
                ch,
                "Você sacrifica %s para os deuses.\r\nVocê recebeu uma mísera moeda de ouro pela sua humildade.\r\n",
                GET_OBJ_SHORT(j));
            increase_gold(ch, 1);
            break;
        case 1:
            send_to_char(ch, "Você sacrifica %s para os deuses.\r\nOs deuses ignoram o seu sacrificio.\r\n",
                         GET_OBJ_SHORT(j));
            break;
        case 2:
            send_to_char(ch, "Você sacrifica %s para os deuses.\r\nOs deuses te dão %d pontos de experiência.\r\n",
                         GET_OBJ_SHORT(j), 1 + 2 * GET_OBJ_LEVEL(j));
            GET_EXP(ch) += (1 + 2 * GET_OBJ_LEVEL(j));
            break;
        case 3:
            send_to_char(ch, "Você sacrifica %s para os deuses.\r\nVocê recebeu %d pontos de experiência.\r\n",
                         GET_OBJ_SHORT(j), 1 + GET_OBJ_LEVEL(j));
            GET_EXP(ch) += (1 + GET_OBJ_LEVEL(j));
            break;
        case 4:
            send_to_char(ch, "O seu sacrificio para os deuses é recompensado com %d moedas de ouro.\r\n",
                         1 + GET_OBJ_LEVEL(j));
            increase_gold(ch, (1 + GET_OBJ_LEVEL(j)));
            break;
        case 5:
            send_to_char(ch, "O seu sacrificio para os deuses é recompensado com %d moedas de ouro.\r\n",
                         (1 + 2 * GET_OBJ_LEVEL(j)));
            increase_gold(ch, (1 + 2 * GET_OBJ_LEVEL(j)));
            break;
        default:
            send_to_char(
                ch,
                "Você sacrifica %s para os deuses.\r\nVocê recebeu uma mísera moeda de ouro pela sua humildade.\r\n",
                GET_OBJ_SHORT(j));
            increase_gold(ch, 1);
            break;
    }
    for (jj = j->contains; jj; jj = next_thing2) {
        next_thing2 = jj->next_content; /* Next in inventory */
        obj_from_obj(jj);

        if (j->carried_by)
            obj_to_room(jj, IN_ROOM(j));
        else if (IN_ROOM(j) != NOWHERE)
            obj_to_room(jj, IN_ROOM(j));
        else
            assert(FALSE);
    }
    extract_obj(j);
}

ACMD(do_envenom)
{
    const char *usage = "Uso: envenom <arma> <recipiente>\r\n";

    struct obj_data *weapon, *liqcon;
    int prob, percent, num, liqneeded, weight;
    char *name;
    char arg[MAX_INPUT_LENGTH];

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_ENVENOM)) {
        send_to_char(ch, "Você não faz idéia de como fazer isso.\r\n");
        return;
    }

    argument = one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "%s", usage);
        return;
    }

    name = arg;
    if (!(num = get_number(&name))) {
        send_to_char(ch, "Envenenar o quê??\r\n");
        return;
    }

    if ((weapon = get_obj_in_equip_vis(ch, name, &num, ch->equipment)) == NULL)
        if ((weapon = get_obj_in_list_vis(ch, name, &num, ch->carrying)) == NULL) {
            send_to_char(ch, "Você não parece ter %s.\r\n", name);
            return;
        }

    if (GET_OBJ_TYPE(weapon) != ITEM_WEAPON && GET_OBJ_TYPE(weapon) != ITEM_AMMO) {
        send_to_char(ch, "Somente armas ou munições podem ser envenenadas!\r\n");
        return;
    }

    if (weapon->worn_on != WEAR_WIELD && weapon->worn_on != WEAR_QUIVER) {
        send_to_char(ch, "Você só pode envenenar armas empunhadas ou na bolsa de munições.\r\n");
        return;
    }

    /* We have the weapon at this point. Let's find the poison. */

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "%s", usage);
        return;
    }

    name = arg;
    if (!(num = get_number(&name))) {
        send_to_char(ch, "Como? Daonde você pretende extrair o veneno?\r\n");
        return;
    }

    if ((liqcon = get_obj_in_list_vis(ch, name, &num, ch->carrying)) == NULL)
        if ((liqcon = get_obj_in_list_vis(ch, name, &num, world[IN_ROOM(ch)].contents)) == NULL) {
            send_to_char(ch, "Você não parece ter %s.\r\n", name);
            return;
        }

    if (GET_OBJ_TYPE(liqcon) != ITEM_DRINKCON && GET_OBJ_TYPE(liqcon) != ITEM_FOUNTAIN) {
        send_to_char(ch, "Isso faz sentido para você?\r\n");
        return;
    }

    if (GET_OBJ_VAL(liqcon, 1) <= 0) {
        act("Mas não há nada em $p!", FALSE, ch, liqcon, NULL, TO_CHAR);
        return;
    }

    /*
     * It's OK to try to poison a weapon with a non-poisoned liquid, but,
     * the player will not be aware if it was really poisoned.
     *
     * It shouldn't also know if it's skills had been successfully poisoned
     * the weapon (much like sneak and hide skills).
     */

    percent = rand_number(1, 101); /* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_ENVENOM);
    liqneeded = rand_number(5, 10);

    liqneeded = MIN(liqneeded, GET_OBJ_VAL(liqcon, 1));
    weight = MIN(liqneeded, GET_OBJ_WEIGHT(liqcon));

    weight_change_object(liqcon, -weight);
    GET_OBJ_VAL(liqcon, 1) -= liqneeded;

    if (liqneeded < 5) {
        act("Não havia líquido suficiente em $p para envenenar $P.", FALSE, ch, liqcon, weapon, TO_CHAR);
        return;
    }

    act("Você tenta envenenar $p com o conteúdo de $P.", FALSE, ch, weapon, liqcon, TO_CHAR);
    act("$n parece se divertir com $p e $P.", TRUE, ch, weapon, liqcon, TO_ROOM);

    /*
     * The effective poison will only occour if his/her skills matched, and
     * if the liquid on liqcon was poisoned.
     */

    if (percent < prob && GET_OBJ_VAL(liqcon, 3))
        SET_BIT_AR(GET_OBJ_EXTRA(weapon), ITEM_POISONED);
}
