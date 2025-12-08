/**************************************************************************
 *  File: spec_procs.c                                      Part of tbaMUD *
 *  Usage: Implementation of special procedures for mobiles/objects/rooms. *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

/* For more examples:
   ftp://ftp.circlemud.org/pub/CircleMUD/contrib/snippets/specials */

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
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "modify.h"
#include "spedit.h"
#include "formula.h"

/* locally defined functions of local (file) scope */

static const char *how_good(int percent);
static void npc_steal(struct char_data *ch, struct char_data *victim);

static const char *how_good(int percent)
{
    if (percent < 0)
        return " (erro)";
    if (percent == 0)
        return " (desconhecida)";
    if (percent <= 10)
        return " (péssima)";
    if (percent <= 20)
        return " (ruim)";
    if (percent <= 40)
        return " (fraca)";
    if (percent <= 55)
        return " (média)";
    if (percent <= 70)
        return " (razoavel)";
    if (percent <= 80)
        return " (bom)";
    if (percent <= 85)
        return " (muito bom)";
    if (percent <= 90)
        return " (otimo)";
    if (percent <= 99)
        return " (excelente)";

    return " (mestre)";
}

#define LEARNED_LEVEL 0 /* % known which is considered "learned" */
#define MAX_PER_PRAC 1  /* max percent gain in skill per practice */
#define MIN_PER_PRAC 2  /* min percent gain in skill per practice */
#define PRAC_TYPE 3     /* should it say 'spell' or 'skill'? */

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])

void list_skills(struct char_data *ch)
{
    const char *overflow = "\r\n**OVERFLOW**\r\n";

    size_t len = 0;

    int vnum = 1;
    int ret;
    struct str_spells *spell;

    char buf[MAX_STRING_LENGTH];

    len = snprintf(buf, sizeof(buf),
                   "Você possui %d %s para praticar.\r\n"
                   "\tWVocê conhece as seguintes:\r\n\tn",
                   GET_PRACTICES(ch), GET_PRACTICES(ch) == 1 ? "crédito" : "créditos");

    spell = get_spell_by_vnum(vnum);
    while (spell) {
        int can_practice = 0, i;

        if (spell->status == available) {
            // if LVL_IMMORT bypass class assignement check.
            if (GET_LEVEL(ch) >= LVL_IMMORT)
                can_practice = 1;
            else
                for (i = 0; i < NUM_CLASSES; i++)
                    if ((spell->assign[i].class_num == GET_CLASS(ch) && (GET_LEVEL(ch) >= spell->assign[i].level)) ||
                        (GET_SKILL(ch, spell->vnum) > 0)) {
                        can_practice = 1;
                        break;
                    }
            if (can_practice) {
                ret = snprintf(buf + len, sizeof(buf) - len, "%-20s %s\r\n", spell->name,
                               how_good(GET_SKILL(ch, spell->vnum)));
                if (ret < 0 || len + ret >= sizeof(buf))
                    break;
                len += ret;
            }
        }
        spell = spell->next;
    }
    if (len >= sizeof(buf))
        strcpy(buf + sizeof(buf) - strlen(overflow) - 1, overflow);   // strcpy: OK

    page_string(ch->desc, buf, TRUE);
}

SPECIAL(guild)
{
    struct str_spells *spell = NULL;

    int class, skill_num, percent, level, rts_code;
    struct obj_data *object;
    int count_obj;
    int grupo;

    if (IS_NPC(ch) || !CMD_IS("practice"))
        return (FALSE);

    /*grupo e inventario */
    if (GROUP(ch) != NULL)
        grupo = 1;
    else
        grupo = 0;

    for (object = ch->carrying; object; object = object->next_content) {
        count_obj++;
    }
    skip_spaces(&argument);

    if (!*argument) {
        list_skills(ch);
        return (TRUE);
    }
    if (GET_PRACTICES(ch) <= 0) {
        send_to_char(ch, "Você não possui créditos, e não pode mais praticar.\r\n");
        return (TRUE);
    }
    spell = get_spell_by_name(argument, SPSK);
    if (!spell) {
        log1("SYSERR: spell not found '%s' at the guild.", argument);
        send_to_char(ch, "'%s não existe.\r\n", argument);
        return (TRUE);
    }

    skill_num = spell->vnum;
    level = get_spell_level(skill_num, GET_CLASS(ch));

    if ((level == -1) || (GET_LEVEL(ch) < level)) {
        send_to_char(ch, "Você não pode praticar essa %s.\r\n",
                     spell->type == SPELL   ? "magia"
                     : spell->type == SKILL ? "habilidade"
                                            : "canção");
        return (TRUE);
    }
    if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
        send_to_char(ch, "Voce ja conhece o suficiente.\r\n");
        return (TRUE);
    }
    send_to_char(ch, "Voce pratica por um tempo...\r\n");
    GET_PRACTICES(ch)--;

    class = get_spell_class(spell, GET_CLASS(ch));
    percent = GET_SKILL(ch, skill_num);
    if ((class == -1) || !spell->assign[class].prac_gain)
        percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));
    else
        percent += MAX(
            5, formula_interpreter(ch, ch, skill_num, TRUE, spell->assign[class].prac_gain, GET_LEVEL(ch), &rts_code));

    SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

    /* Update any variant skills that depend on this skill as a prerequisite */
    update_variant_skills(ch, skill_num, GET_SKILL(ch, skill_num));

    if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
        send_to_char(ch, "Voce agora conhece o suficiente!\r\n");

    return (TRUE);
}

SPECIAL(dump)
{
    struct obj_data *k;
    int value = 0;

    for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
        act("$p desaparece em uma nuvem de fumaca!", FALSE, 0, k, 0, TO_ROOM);
        extract_obj(k);
    }

    if (!CMD_IS("drop"))
        return (FALSE);

    do_drop(ch, argument, cmd, SCMD_DROP);

    for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
        act("$p desaparece em uma nuvem de fumaca!", FALSE, 0, k, 0, TO_ROOM);
        value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
        extract_obj(k);
    }

    if (value) {
        send_to_char(ch, "Voce é recompensad%s por considerável atuação.\r\n", OA(ch));
        act("$n foi recompensad$r por sua considerável atuação.", TRUE, ch, 0, 0, TO_ROOM);

        if (GET_LEVEL(ch) < 3)
            gain_exp(ch, value);
        else
            increase_gold(ch, value);
    }
    return (TRUE);
}

SPECIAL(mayor)
{
    char actbuf[MAX_INPUT_LENGTH];

    const char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
    const char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char *path = NULL;
    static int path_index;
    static bool move = FALSE;

    if (!move) {
        if (time_info.hours == 6) {
            move = TRUE;
            path = open_path;
            path_index = 0;
        } else if (time_info.hours == 20) {
            move = TRUE;
            path = close_path;
            path_index = 0;
        }
    }
    if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) || (GET_POS(ch) == POS_FIGHTING))
        return (FALSE);

    switch (path[path_index]) {
        case '0':
        case '1':
        case '2':
        case '3':
            perform_move(ch, path[path_index] - '0', 1);
            break;

        case 'W':
            GET_POS(ch) = POS_STANDING;
            act("$n acorda e boceja.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'S':
            GET_POS(ch) = POS_SLEEPING;
            act("$n senta-se e imediatamente começa a dormir.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'a':
            act("$n fala 'Ola Docinho!'", FALSE, ch, 0, 0, TO_ROOM);
            act("$n lambe os labios.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'b':
            act("$n fala 'Que visão! Preciso fazer algo sobre o lixão!'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'c':
            act("$n fala 'Vandalos!  Os adolescentes de hoje em dia não respeitam maisnada!'", FALSE, ch, 0, 0,
                TO_ROOM);
            break;

        case 'd':
            act("$n fala 'Bom dia, cidadãos!'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'e':
            act("$n fala 'Eu declaro esta cidade aberta!'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'E':
            act("$n fala 'Eu declaro Midgaard fechada!'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'O':
            do_gen_door(ch, strcpy(actbuf, "portao"), 0,
                        SCMD_UNLOCK);                                /* strcpy:
                                                                                                                                OK */
            do_gen_door(ch, strcpy(actbuf, "portao"), 0, SCMD_OPEN); /* strcpy: OK */
            break;

        case 'C':
            do_gen_door(ch, strcpy(actbuf, "portao"), 0, SCMD_CLOSE); /* strcpy: OK */
            do_gen_door(ch, strcpy(actbuf, "portao"), 0, SCMD_LOCK);  /* strcpy: OK */
            break;

        case '.':
            move = FALSE;
            break;
    }

    path_index++;
    return (FALSE);
}

/* General special procedures for mobiles. */

static void npc_steal(struct char_data *ch, struct char_data *victim)
{
    int gold;

    if (IS_NPC(victim))
        return;
    if (GET_LEVEL(victim) >= LVL_IMMORT)
        return;
    if (!CAN_SEE(ch, victim))
        return;

    if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
        act("Voce descobre que $n está com suas mãos em swu bolso.", FALSE, ch, 0, victim, TO_VICT);
        act("$n tenta roubar dinheiro  de $N.", TRUE, ch, 0, victim, TO_NOTVICT);
    } else {
        /* Steal some gold coins */
        gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
        if (gold > 0) {
            increase_gold(ch, gold);
            decrease_gold(victim, gold);
        }
    }
}

/* Quite lethal to low-level characters. */
SPECIAL(snake)
{
    if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
        return (FALSE);

    if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
        return (FALSE);

    act("$n morde $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n morde voce!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
    return (TRUE);
}

SPECIAL(thief)
{
    struct char_data *cons;

    if (cmd || GET_POS(ch) != POS_STANDING)
        return (FALSE);

    for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
        if (!IS_NPC(cons) && GET_LEVEL(cons) < LVL_IMMORT && !rand_number(0, 4)) {
            npc_steal(ch, cons);
            return (TRUE);
        }

    return (FALSE);
}

SPECIAL(magic_user)
{
    struct char_data *vict;

    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return (FALSE);

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
        vict = FIGHTING(ch);

    /* Hm...didn't pick anyone...I'll wait a round. */
    if (vict == NULL)
        return (TRUE);

    if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
        cast_spell(ch, vict, NULL, SPELL_POISON);

    if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
        cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

    if (GET_LEVEL(ch) > 12 && rand_number(0, 12) == 0) {
        if (IS_EVIL(ch))
            cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
        else if (IS_GOOD(ch))
            cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
    }

    if (rand_number(0, 4))
        return (TRUE);

    switch (GET_LEVEL(ch)) {
        case 4:
        case 5:
            cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
            break;
        case 6:
        case 7:
            cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
            break;
        case 8:
        case 9:
            cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
            break;
        case 10:
        case 11:
            cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
            break;
        case 12:
        case 13:
            cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
            break;
        case 14:
        case 15:
        case 16:
        case 17:
            cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
            break;
        default:
            cast_spell(ch, vict, NULL, SPELL_FIREBALL);
            break;
    }
    return (TRUE);
}

/* Special procedures for mobiles. */
SPECIAL(guild_guard)
{
    int i, direction;
    struct char_data *guard = (struct char_data *)me;
    const char *buf = "O guarda humilia voce e bloqueia sua passagem.\r\n";
    const char *buf2 = "O guarda bloqueia a passagem de $n.";

    if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
        return (FALSE);

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return (FALSE);

    /* find out what direction they are trying to go */
    for (direction = 0; direction < NUM_OF_DIRS; direction++)
        if (!strcmp(cmd_info[cmd].command, dirs[direction]))
            for (direction = 0; direction < DIR_COUNT; direction++)
                if (!strcmp(cmd_info[cmd].command, dirs[direction]) ||
                    !strcmp(cmd_info[cmd].command, autoexits[direction]))
                    break;

    for (i = 0; guild_info[i].guild_room != NOWHERE; i++) {
        /* Wrong guild. */
        if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room)
            continue;

        /* Wrong direction. */
        if (direction != guild_info[i].direction)
            continue;

        /* Allow the people of the guild through. */
        if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i].pc_class)
            continue;

        send_to_char(ch, "%s", buf);
        act(buf2, FALSE, ch, 0, 0, TO_ROOM);
        return (TRUE);
    }
    return (FALSE);
}

SPECIAL(puff)
{
    char actbuf[MAX_INPUT_LENGTH];

    if (cmd)
        return (FALSE);

    switch (rand_number(0, 60)) {
        case 0:
            do_say(ch, strcpy(actbuf, "Meu Deus! Quantas estrelas!"), 0,
                   0); /* strcpy:
                                                                                          OK
                                                                                        */
            return (TRUE);
        case 1:
            do_say(ch, strcpy(actbuf, "Como estes peixes vieram para aqui?"), 0,
                   0); /* strcpy:
                                                                                                  OK
                                                                                                */
            return (TRUE);
        case 2:
            do_say(ch, strcpy(actbuf, "Eu sou muito feminina."), 0,
                   0); /* strcpy:
                                                                                  OK */
            return (TRUE);
        case 3:
            do_say(ch, strcpy(actbuf, "Eu tenho este sentimento muito pacifico."), 0,
                   0); /* strcpy:
                                                                                                          OK
                                                                                                        */
            return (TRUE);
        default:
            return (FALSE);
    }
}

SPECIAL(fido)
{
    struct obj_data *i, *temp, *next_obj;

    if (cmd || !AWAKE(ch))
        return (FALSE);

    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
        if (!IS_CORPSE(i))
            continue;

        act("$n brutalmente devora um corpo.", FALSE, ch, 0, 0, TO_ROOM);
        for (temp = i->contains; temp; temp = next_obj) {
            next_obj = temp->next_content;
            obj_from_obj(temp);
            obj_to_room(temp, IN_ROOM(ch));
        }
        extract_obj(i);
        return (TRUE);
    }
    return (FALSE);
}

SPECIAL(janitor)
{
    struct obj_data *i;

    if (cmd || !AWAKE(ch))
        return (FALSE);

    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
        if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
            continue;
        if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 50)
            continue;
        act("$n recolhe um pouco de lixo.", FALSE, ch, 0, 0, TO_ROOM);
        obj_from_room(i);
        obj_to_char(i, ch);
        return (TRUE);
    }
    return (FALSE);
}

SPECIAL(cityguard)
{
    struct char_data *tch, *evil, *spittle;
    int max_evil, min_cha;

    if (cmd || !AWAKE(ch) || FIGHTING(ch))
        return (FALSE);

    max_evil = 1000;
    min_cha = 6;
    spittle = evil = NULL;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
        if (!CAN_SEE(ch, tch))
            continue;
        if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
            act("$n grita 'HEY!!!  Voce e' um daqueles malditos PKS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
            hit(ch, tch, TYPE_UNDEFINED);
            return (TRUE);
        }

        if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
            act("$n grita 'HEY!!!  Voce e' um daqueles malditos LADROES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
            hit(ch, tch, TYPE_UNDEFINED);
            return (TRUE);
        }

        if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
            max_evil = GET_ALIGNMENT(tch);
            evil = tch;
        }

        if (GET_CHA(tch) < min_cha) {
            spittle = tch;
            min_cha = GET_CHA(tch);
        }
    }

    if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
        act("$n grita 'PROTEJAM OS INOCENTES!  BANZAI!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
        hit(ch, evil, TYPE_UNDEFINED);
        return (TRUE);
    }

    /* Reward the socially inept. */
    if (spittle && !rand_number(0, 9)) {
        static int spit_social;

        if (!spit_social)
            spit_social = find_command("spit");

        if (spit_social > 0) {
            char spitbuf[MAX_NAME_LENGTH + 1];
            strncpy(spitbuf, GET_NAME(spittle),
                    sizeof(spitbuf)); /* strncpy:
                                                                                                 OK */
            spitbuf[sizeof(spitbuf) - 1] = '\0';
            do_action(ch, spitbuf, spit_social, 0);
            return (TRUE);
        }
    }
    return (FALSE);
}

#define PET_PRICE(pet) ((GET_LEVEL(pet) * GET_LEVEL(pet) * 9 + 21) * 10)
SPECIAL(pet_shops)
{
    char buf[MAX_STRING_LENGTH], pet_name[256];
    room_rnum pet_room;
    struct char_data *pet;

    /* Gross. */
    pet_room = IN_ROOM(ch) + 1;

    if (CMD_IS("list")) {
        send_to_char(ch, "Os animais de estimação são:\r\n");
        for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
            /* No, you can't have the Implementor as a pet if he's in there. */
            if (!IS_NPC(pet))
                continue;
            send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
        }
        return (TRUE);
    } else if (CMD_IS("buy")) {

        two_arguments(argument, buf, pet_name);

        if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet)) {
            send_to_char(ch, "Nao vamos ter este pet!\r\n");
            return (TRUE);
        }
        if (GET_GOLD(ch) < PET_PRICE(pet)) {
            send_to_char(ch, "Voce nao tem tanto dinheiro!\r\n");
            return (TRUE);
        }
        decrease_gold(ch, PET_PRICE(pet));

        pet = read_mobile(GET_MOB_RNUM(pet), REAL);
        GET_EXP(pet) = 0;
        SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

        if (*pet_name) {
            snprintf(buf, sizeof(buf), "%s %s", pet->player.name, pet_name);
            /* free(pet->player.name); don't free the prototype! */
            pet->player.name = strdup(buf);

            snprintf(buf, sizeof(buf),
                     "%s Uma pequena placa em uma coleira ao redor de seu pescoço diz: 'Meu nome é %s'\r\n",
                     pet->player.description, pet_name);
            /* free(pet->player.description); don't free the prototype! */
            pet->player.description = strdup(buf);
        }
        char_to_room(pet, IN_ROOM(ch));
        add_follower(pet, ch);

        /* Be certain that pets can't get/carry/use/wield/wear items */
        IS_CARRYING_W(pet) = 1000;
        IS_CARRYING_N(pet) = 100;

        send_to_char(ch, "Divirta-se com o seu animal de estimação.\r\n");
        act("$n compra $N.", FALSE, ch, 0, pet, TO_ROOM);

        return (TRUE);
    }

    /* All commands except list and buy */
    return (FALSE);
}

/* Special procedures for objects. */
/* SPECIAL(bank) { int amount, amount2; char arg[MAX_INPUT_LENGTH]; amount2 =
   atoi(argument); half_chop(argument, arg, argument);

   if (CMD_IS("balance")) { if (GET_BANK_GOLD(ch) > 0) send_to_char(ch, "O seu
   saldo atual e' de %d moedas.\r\n", GET_BANK_GOLD(ch)); else
   send_to_char(ch, "O seu saldo esta zerado.\r\n"); return (TRUE); } else if
   (CMD_IS("deposit")) { if (!strcmp(arg, "all")) amount = GET_GOLD(ch); else
   amount = amount2; if (amount <= 0) { send_to_char(ch, "Quanto voce quer
   depositar?\r\n"); return (TRUE); } if (GET_GOLD(ch) < amount) {
   send_to_char(ch, "Voce nao tem tantas moedas!\r\n"); return (TRUE); }
   decrease_gold(ch, amount); increase_bank(ch, amount); send_to_char(ch,
   "Voce deposita %d moedas.\r\n", amount); act("$n fez uma transacao
   bancaria.", TRUE, ch, 0, FALSE, TO_ROOM); return (TRUE); } else if
   (CMD_IS("withdraw")) { if (!strcmp(arg, "all")) amount = GET_BANK_GOLD(ch);
   else amount = amount2; if (amount <= 0) { send_to_char(ch, "Quanto voce quer
   sacar?\r\n"); return (TRUE); } if (GET_BANK_GOLD(ch) < amount) {
   send_to_char(ch, "Voce nao tem tanto dinheiro no banco!\r\n"); return
   (TRUE); } increase_gold(ch, amount); decrease_bank(ch, amount);
   send_to_char(ch, "Voce saca %d moedas.\r\n", amount); act("$n fez uma
   transacao bancaria.", TRUE, ch, 0, FALSE, TO_ROOM); return (TRUE); } else
   return (FALSE); } */
/* Special procedures for objects. */
SPECIAL(bank)
{
    int amount;

    if (CMD_IS("balance")) {
        if (GET_BANK_GOLD(ch) > 0)
            send_to_char(ch, "Seu saldo atual é de %s moedas.\r\n", format_number_br(GET_BANK_GOLD(ch)));
        else
            send_to_char(ch, "Você não possui dinheiro depositado.\r\n");
        return (TRUE);
    } else if (CMD_IS("deposit")) {
        if (IS_DEAD(ch) || PLR_FLAGGED(ch, PLR_TRNS)) {
            send_to_char(ch, "Como você pretende depositar algo?\r\n");
            return (TRUE);
        }
        if ((amount = atoi(argument)) <= 0) {
            send_to_char(ch, "Quanto você deseja depositar?\r\n");
            return (TRUE);
        }
        if (GET_GOLD(ch) < amount) {
            send_to_char(ch, "Você não tem tantas moedas!\r\n");
            return (TRUE);
        }
        decrease_gold(ch, amount);
        increase_bank(ch, amount);
        send_to_char(ch, "Você deposita %s moedas.\r\n", format_number_br(amount));
        act("$n faz uma transação bancária.", TRUE, ch, 0, FALSE, TO_ROOM);
        return (TRUE);
    } else if (CMD_IS("withdraw")) {

        if (IS_DEAD(ch) || PLR_FLAGGED(ch, PLR_TRNS)) {
            send_to_char(ch, "Como você pretende sacar algo?\r\n");
            return (TRUE);
        }
        if ((amount = atoi(argument)) <= 0) {
            send_to_char(ch, "Quanto você deseja sacar?\r\n");
            return (TRUE);
        }
        if (GET_BANK_GOLD(ch) < amount) {
            send_to_char(ch, "Você não possui tantas moedas depositadas!\r\n");
            return (TRUE);
        }
        increase_gold(ch, amount);
        decrease_bank(ch, amount);
        send_to_char(ch, "Você saca %s moedas.\r\n", format_number_br(amount));
        act("$n faz uma transação bancária.", TRUE, ch, 0, FALSE, TO_ROOM);
        return (TRUE);
    } else
        return (FALSE);
}

/* QP Exchange special procedure for mob 2999
 * Allows players to exchange gold for QP and QP for gold using
 * a dynamic exchange rate calculated as total_money / total_qp.
 * The rate is recalculated at the start of each MUD month.
 * The 17 MUD months are: Brumis, Kames'Hi, Teriany, Hiro, Prúdis,
 * Maqizie, Kadrictes, Mizu, Mysoluh, Karestis, Neruno, Latízie,
 * Aminen, Autúmis, V'tah, Aqrien, Tellus */

/* Number of MUD months */
#define NUM_MUD_MONTHS 17

/* Default base exchange rate: how many gold coins for 1 QP
 * This is used as a fallback if no calculated rate is available */
#define QP_EXCHANGE_DEFAULT_BASE_RATE 10000

/* Minimum and maximum allowed base rates to prevent extreme values */
#define QP_EXCHANGE_MIN_BASE_RATE 1000
#define QP_EXCHANGE_MAX_BASE_RATE 100000000

/* File to store the monthly QP exchange base rate */
#define QP_EXCHANGE_RATE_FILE "lib/etc/qp_exchange_rate"

/* Global variables for dynamic QP exchange rate */
static int qp_exchange_base_rate = QP_EXCHANGE_DEFAULT_BASE_RATE;
static int qp_exchange_rate_month = -1; /* Last month when rate was calculated */

/* Calculate the base QP exchange rate from total gold and QP in the economy.
 * This iterates through all registered mortal players (level <= 100) and
 * calculates: rate = total_money / total_qp.
 * Should be called at the start of each MUD month.
 *
 * Note: This operation loads all player files which can be slow on servers
 * with many players. However, it only runs once per MUD month (35 MUD days)
 * which is infrequent enough to not cause noticeable performance impact. */
void calculate_qp_exchange_base_rate(void)
{
    int i;
    struct char_data *temp_ch = NULL;
    long long total_money = 0;
    long long total_qp = 0;
    int player_count = 0;
    long long calculated_rate;

    for (i = 0; i <= top_of_p_table; i++) {
        /* Skip immortals (level >= LVL_IMMORT) based on cached player_table index */
        if (player_table[i].level >= LVL_IMMORT)
            continue;

        /* Create temporary character to load player data */
        CREATE(temp_ch, struct char_data, 1);
        clear_char(temp_ch);
        CREATE(temp_ch->player_specials, struct player_special_data, 1);
        new_mobile_data(temp_ch);

        if (load_char(player_table[i].name, temp_ch) >= 0) {
            total_money += GET_GOLD(temp_ch) + GET_BANK_GOLD(temp_ch);
            total_qp += GET_QUESTPOINTS(temp_ch);
            player_count++;
        }

        free_char(temp_ch);
        temp_ch = NULL;
    }

    /* Calculate the new base rate using long long to avoid overflow,
     * then clamp to int-safe bounds */
    if (total_qp > 0) {
        calculated_rate = total_money / total_qp;
        /* Clamp the rate to reasonable bounds (also ensures int-safe values) */
        if (calculated_rate < QP_EXCHANGE_MIN_BASE_RATE)
            calculated_rate = QP_EXCHANGE_MIN_BASE_RATE;
        else if (calculated_rate > QP_EXCHANGE_MAX_BASE_RATE)
            calculated_rate = QP_EXCHANGE_MAX_BASE_RATE;
        qp_exchange_base_rate = (int)calculated_rate;
    } else {
        /* No QP in economy, use default rate */
        qp_exchange_base_rate = QP_EXCHANGE_DEFAULT_BASE_RATE;
    }

    qp_exchange_rate_month = time_info.month;

    log1(
        "QP Exchange: Calculated new base rate %d gold/QP from %d players "
        "(total money: %lld, total QP: %lld)",
        qp_exchange_base_rate, player_count, total_money, total_qp);

    /* Save the rate to file for persistence */
    save_qp_exchange_rate();
}

/* Save the QP exchange base rate to a file for persistence across reboots */
void save_qp_exchange_rate(void)
{
    FILE *fp;

    if ((fp = fopen(QP_EXCHANGE_RATE_FILE, "w")) == NULL) {
        log1("SYSERR: Cannot save QP exchange rate to %s", QP_EXCHANGE_RATE_FILE);
        return;
    }

    fprintf(fp, "%d %d\n", qp_exchange_base_rate, qp_exchange_rate_month);
    fclose(fp);
}

/* Load the QP exchange base rate from file (called at boot) */
void load_qp_exchange_rate(void)
{
    FILE *fp;
    int rate, month;

    if ((fp = fopen(QP_EXCHANGE_RATE_FILE, "r")) == NULL) {
        log1("QP Exchange: No saved rate file, using default rate %d", QP_EXCHANGE_DEFAULT_BASE_RATE);
        qp_exchange_base_rate = QP_EXCHANGE_DEFAULT_BASE_RATE;
        qp_exchange_rate_month = -1;
        return;
    }

    if (fscanf(fp, "%d %d", &rate, &month) == 2) {
        /* Validate loaded values including month range */
        if (rate >= QP_EXCHANGE_MIN_BASE_RATE && rate <= QP_EXCHANGE_MAX_BASE_RATE && month >= 0 &&
            month < NUM_MUD_MONTHS) {
            qp_exchange_base_rate = rate;
            qp_exchange_rate_month = month;
            log1("QP Exchange: Loaded base rate %d gold/QP (calculated in month %d)", rate, month);
        } else {
            log1("QP Exchange: Invalid rate or month in file, using default");
            qp_exchange_base_rate = QP_EXCHANGE_DEFAULT_BASE_RATE;
            qp_exchange_rate_month = -1;
        }
    } else {
        log1("QP Exchange: Error reading rate file, using default");
        qp_exchange_base_rate = QP_EXCHANGE_DEFAULT_BASE_RATE;
        qp_exchange_rate_month = -1;
    }

    fclose(fp);
}

/* Called when a new MUD month begins to update the exchange rate */
void update_qp_exchange_rate_on_month_change(void)
{
    /* Only recalculate if the month has actually changed */
    if (qp_exchange_rate_month != time_info.month) {
        calculate_qp_exchange_base_rate();
    }
}

SPECIAL(qp_exchange)
{
    int amount, cost, current_rate;
    char arg[MAX_INPUT_LENGTH];

    /* Only handle specific commands */
    if (!CMD_IS("cotacao") && !CMD_IS("comprar") && !CMD_IS("vender") && !CMD_IS("rate") && !CMD_IS("buy") &&
        !CMD_IS("sell"))
        return (FALSE);

    /* Get current exchange rate */
    current_rate = get_qp_exchange_rate();

    /* Show exchange rate */
    if (CMD_IS("cotacao") || CMD_IS("rate")) {
        send_to_char(ch,
                     "\tCCotação de Pontos de Busca\tn\r\n"
                     "Mês atual: \tY%s\tn\r\n"
                     "Taxa de câmbio: \tG%s\tn moedas por 1 QP\r\n"
                     "\r\n"
                     "Comandos disponíveis:\r\n"
                     "  \tWbuy <quantidade>\tn / \tWcomprar <quantidade>\tn - Compra QPs com ouro\r\n"
                     "  \tWsell <quantidade>\tn / \tWvender <quantidade>\tn  - Vende QPs por ouro\r\n",
                     month_name[time_info.month], format_number_br(current_rate));
        return (TRUE);
    }

    /* Check if player is dead or transcended */
    if (IS_DEAD(ch) || PLR_FLAGGED(ch, PLR_TRNS)) {
        send_to_char(ch, "Você não pode realizar transações neste estado.\r\n");
        return (TRUE);
    }

    one_argument(argument, arg);

    /* Buy QP with gold */
    if (CMD_IS("comprar") || CMD_IS("buy")) {
        int curr_qp, new_qp;

        if (!*arg || (amount = atoi(arg)) <= 0) {
            send_to_char(ch, "Quantos pontos de busca você deseja comprar?\r\n");
            send_to_char(ch, "Taxa atual: %s moedas por 1 QP\r\n", format_number_br(current_rate));
            return (TRUE);
        }

        /* Calculate total cost */
        cost = amount * current_rate;

        /* Check for overflow (also protect against division by zero) */
        if (cost < 0 || (current_rate > 0 && cost / current_rate != amount)) {
            send_to_char(ch, "Essa quantidade é muito alta para calcular!\r\n");
            return (TRUE);
        }

        /* Check if player has enough gold */
        if (GET_GOLD(ch) < cost) {
            send_to_char(ch, "Você não tem ouro suficiente!\r\n");
            send_to_char(ch, "Você precisa de %s moedas para comprar %d QP.\r\n", format_number_br(cost), amount);
            send_to_char(ch, "Você possui: %s moedas.\r\n", format_number_br(GET_GOLD(ch)));
            return (TRUE);
        }

        /* Check for QP overflow and limit before adding (follows increase_gold pattern) */
        curr_qp = GET_QUESTPOINTS(ch);
        new_qp = MIN(MAX_QP, curr_qp + amount);

        /* Validate to prevent overflow: if new_qp < curr_qp, overflow occurred */
        if (new_qp < curr_qp)
            new_qp = MAX_QP;

        if (new_qp == MAX_QP && curr_qp == MAX_QP) {
            send_to_char(ch, "Você já possui o máximo de pontos de busca!\r\n");
            return (TRUE);
        }

        /* Perform the exchange */
        decrease_gold(ch, cost);
        GET_QUESTPOINTS(ch) = new_qp;

        if (new_qp == MAX_QP) {
            send_to_char(ch, "Você atingiu o limite máximo de pontos de busca!\r\n");
        }

        send_to_char(ch, "Você troca %s moedas por %d pontos de busca.\r\n", format_number_br(cost), amount);
        act("$n realiza uma transação de pontos de busca.", TRUE, ch, 0, FALSE, TO_ROOM);

        /* Log the transaction */
        mudlog(NRM, LVL_IMMORT, TRUE, "QP EXCHANGE: %s bought %d QP for %d gold (rate: %d)", GET_NAME(ch), amount, cost,
               current_rate);

        return (TRUE);
    }

    /* Sell QP for gold */
    if (CMD_IS("vender") || CMD_IS("sell")) {
        if (!*arg || (amount = atoi(arg)) <= 0) {
            send_to_char(ch, "Quantos pontos de busca você deseja vender?\r\n");
            send_to_char(ch, "Taxa atual: %s moedas por 1 QP\r\n", format_number_br(current_rate));
            return (TRUE);
        }

        /* Check if player has enough QP */
        if (GET_QUESTPOINTS(ch) < amount) {
            send_to_char(ch, "Você não tem pontos de busca suficientes!\r\n");
            send_to_char(ch, "Você possui: %d QP.\r\n", GET_QUESTPOINTS(ch));
            return (TRUE);
        }

        /* Calculate gold received */
        cost = amount * current_rate;

        /* Check for overflow (also protect against division by zero) */
        if (cost < 0 || (current_rate > 0 && cost / current_rate != amount)) {
            send_to_char(ch, "Essa quantidade é muito alta para calcular!\r\n");
            return (TRUE);
        }

        /* Perform the exchange */
        GET_QUESTPOINTS(ch) -= amount;
        increase_gold(ch, cost);

        send_to_char(ch, "Você troca %d pontos de busca por %s moedas.\r\n", amount, format_number_br(cost));
        act("$n realiza uma transação de pontos de busca.", TRUE, ch, 0, FALSE, TO_ROOM);

        /* Log the transaction */
        mudlog(NRM, LVL_IMMORT, TRUE, "QP EXCHANGE: %s sold %d QP for %d gold (rate: %d)", GET_NAME(ch), amount, cost,
               current_rate);

        return (TRUE);
    }

    return (FALSE);
}

/* Old VitaliaMUD SpecProcs - Complex ones that need to remain as SpecProcs */

SPECIAL(autodestruct)
{
    struct obj_data *obj = (struct obj_data *)me;
    struct char_data *owner;

    if (cmd || !FIGHTING(ch))
        return (FALSE);

    /* Only trigger on pulse */
    if (GET_OBJ_TIMER(obj) > 0) {
        GET_OBJ_TIMER(obj)--;
        if (GET_OBJ_TIMER(obj) == 0) {
            /* Object auto-destructs */
            if ((owner = obj->carried_by)) {
                act("$p suddenly explodes in your hands!", FALSE, owner, obj, 0, TO_CHAR);
                act("$p in $n's hands explodes!", FALSE, owner, obj, 0, TO_ROOM);
                GET_HIT(owner) = MAX(1, GET_HIT(owner) - dice(3, 8));
            } else if (obj->in_room != NOWHERE) {
                send_to_room(obj->in_room, "An object here suddenly explodes!");
            }
            extract_obj(obj);
            return (TRUE);
        }
    }
    return (FALSE);
}

SPECIAL(death_90)
{
    struct char_data *vict = (struct char_data *)me;

    if (cmd || !FIGHTING(vict))
        return (FALSE);

    /* Special death sequence at 90% health loss */
    if (GET_HIT(vict) <= (GET_MAX_HIT(vict) / 10)) {
        act("$n lets out a death scream that chills you to the bone!", FALSE, vict, 0, 0, TO_ROOM);
        act("$n's body begins to glow with an unholy light!", FALSE, vict, 0, 0, TO_ROOM);

        /* Cast a powerful spell before dying */
        if (GET_SKILL(vict, SPELL_FIREBALL) > 0) {
            cast_spell(vict, FIGHTING(vict), NULL, SPELL_FIREBALL);
        }

        return (TRUE);
    }
    return (FALSE);
}

SPECIAL(magik)
{
    struct char_data *magician = (struct char_data *)me;
    struct char_data *vict;
    int spell_choice;

    if (cmd || !FIGHTING(magician))
        return (FALSE);

    vict = FIGHTING(magician);
    if (!vict)
        return (FALSE);

    /* Complex magic system - chooses spells based on situation */
    if (GET_MANA(magician) < 20)
        return (FALSE);

    spell_choice = rand_number(1, 8);

    switch (spell_choice) {
        case 1:
            if (GET_SKILL(magician, SPELL_MAGIC_MISSILE) > 0) {
                cast_spell(magician, vict, NULL, SPELL_MAGIC_MISSILE);
            }
            break;
        case 2:
            if (GET_SKILL(magician, SPELL_FIREBALL) > 0) {
                cast_spell(magician, vict, NULL, SPELL_FIREBALL);
            }
            break;
        case 3:
            if (GET_SKILL(magician, SPELL_LIGHTNING_BOLT) > 0) {
                cast_spell(magician, vict, NULL, SPELL_LIGHTNING_BOLT);
            }
            break;
        case 4:
            if (GET_SKILL(magician, SPELL_COLOR_SPRAY) > 0) {
                cast_spell(magician, vict, NULL, SPELL_COLOR_SPRAY);
            }
            break;
        case 5:
            if (GET_SKILL(magician, SPELL_BLINDNESS) > 0) {
                cast_spell(magician, vict, NULL, SPELL_BLINDNESS);
            }
            break;
        case 6:
            if (GET_SKILL(magician, SPELL_CURSE) > 0) {
                cast_spell(magician, vict, NULL, SPELL_CURSE);
            }
            break;
        case 7:
            if (GET_SKILL(magician, SPELL_POISON) > 0) {
                cast_spell(magician, vict, NULL, SPELL_POISON);
            }
            break;
        case 8:
            if (GET_SKILL(magician, SPELL_SLEEP) > 0) {
                cast_spell(magician, vict, NULL, SPELL_SLEEP);
            }
            break;
    }

    return (TRUE);
}

SPECIAL(blug_staff_f)
{
    struct obj_data *obj = (struct obj_data *)me;

    if (cmd || !ch)
        return (FALSE);

    /* Full staff - more powerful version */
    if (rand_number(1, 100) <= 5) {
        act("$p pulses with a powerful magical energy!", FALSE, ch, obj, 0, TO_CHAR);
        act("$n's $p glows brightly with magic!", FALSE, ch, obj, 0, TO_ROOM);

        /* Give the wielder a temporary bonus */
        if (ch && !FIGHTING(ch)) {
            GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 10);
        }

        return (TRUE);
    }

    return (FALSE);
}

SPECIAL(blug_staff_s)
{
    struct obj_data *obj = (struct obj_data *)me;

    if (cmd || !ch)
        return (FALSE);

    /* Small staff - weaker version */
    if (rand_number(1, 100) <= 3) {
        act("$p glows softly with magical energy.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n's $p shimmers with a faint light.", FALSE, ch, obj, 0, TO_ROOM);

        /* Give the wielder a small bonus */
        if (ch && !FIGHTING(ch)) {
            GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + 5);
        }

        return (TRUE);
    }

    return (FALSE);
}
