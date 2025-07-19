/**************************************************************************
 *  File: castle.c                                          Part of tbaMUD *
 *  Usage: Special procedures for King's Castle area.                      *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Special procedures for Kings Castle by Pjotr. Coded by Sapowox.        *
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
#include "spec_procs.h" /**< castle.c is part of the spec_procs module */
#include "fight.h"

/* IMPORTANT! The below defined number is the zone number of the Kings Castle.
 * Change it to apply to your chosen zone number. The default zone number
 * is 80. */

#define Z_KINGS_C 150

/* local, file scope restricted functions */
static mob_vnum castle_virtual(mob_vnum offset);
static room_rnum castle_real_room(room_vnum roomoffset);
static struct char_data *find_npc_by_name(struct char_data *chAtChar, const char *pszName, int iLen);
static int block_way(struct char_data *ch, int cmd, char *arg, room_vnum iIn_room, int iProhibited_direction);
static int member_of_staff(struct char_data *chChar);
static int member_of_royal_guard(struct char_data *chChar);
static struct char_data *find_guard(struct char_data *chAtChar);
static struct char_data *get_victim(struct char_data *chAtChar);
static int banzaii(struct char_data *ch);
static int do_npc_rescue(struct char_data *ch_hero, struct char_data *ch_victim);
static int is_trash(struct obj_data *i);
static void fry_victim(struct char_data *ch);
static int castle_cleaner(struct char_data *ch, int cmd, int gripe);
static int castle_twin_proc(struct char_data *ch, int cmd, char *arg, int ctlnum, const char *twinname);
static void castle_mob_spec(mob_vnum mobnum, SPECIAL(*specproc));
/* Special procedures for Kings Castle by Pjotr. Coded by Sapowox. */
SPECIAL(CastleGuard);
SPECIAL(James);
SPECIAL(cleaning);
SPECIAL(DicknDavid);
SPECIAL(tim);
SPECIAL(tom);
SPECIAL(king_welmar);
SPECIAL(training_master);
SPECIAL(peter);
SPECIAL(jerry);

/* Assign castle special procedures. NOTE: The mobile number isn't fully
 * specified. It's only an offset from the zone's base. */
static void castle_mob_spec(mob_vnum mobnum, SPECIAL(*specproc))
{
    mob_vnum vmv = castle_virtual(mobnum);
    mob_rnum rmr = NOBODY;

    if (vmv != NOBODY)
        rmr = real_mobile(vmv);

    if (rmr == NOBODY) {
        if (!mini_mud)
            log1("SYSERR: assign_kings_castle(): can't find mob #%d.", vmv);
        /* SYSERR_DESC: When the castle_mob_spec() function is given a mobnum
         * that does not correspond to a mod loaded (when not in minimud mode),
         * this error will result. */
    } else
        mob_index[rmr].func = specproc;
}

static mob_vnum castle_virtual(mob_vnum offset)
{
    zone_rnum zon;

    if ((zon = real_zone(Z_KINGS_C)) == NOWHERE)
        return NOBODY;

    return zone_table[zon].bot + offset;
}

static room_rnum castle_real_room(room_vnum roomoffset)
{
    zone_rnum zon;

    if ((zon = real_zone(Z_KINGS_C)) == NOWHERE)
        return NOWHERE;

    return real_room(zone_table[zon].bot + roomoffset);
}

/* Routine: assign_kings_castle. Used to assign function pointers to all mobiles
 * in the Kings Castle. Called from spec_assign.c. */
void assign_kings_castle(void)
{
    castle_mob_spec(0, CastleGuard);      /* Gwydion */
    castle_mob_spec(1, king_welmar);      /* Our dear friend, the King */
    castle_mob_spec(3, CastleGuard);      /* Jim */
    castle_mob_spec(4, CastleGuard);      /* Brian */
    castle_mob_spec(5, CastleGuard);      /* Mick */
    castle_mob_spec(6, CastleGuard);      /* Matt */
    castle_mob_spec(7, CastleGuard);      /* Jochem */
    castle_mob_spec(8, CastleGuard);      /* Anne */
    castle_mob_spec(9, CastleGuard);      /* Andrew */
    castle_mob_spec(10, CastleGuard);     /* Bertram */
    castle_mob_spec(11, CastleGuard);     /* Jeanette */
    castle_mob_spec(12, peter);           /* Peter */
    castle_mob_spec(13, training_master); /* The training master */
    castle_mob_spec(16, James);           /* James the Butler */
    castle_mob_spec(17, cleaning);        /* Ze Cleaning Fomen */
    castle_mob_spec(20, tim);             /* Tim, Tom's twin */
    castle_mob_spec(21, tom);             /* Tom, Tim's twin */
    castle_mob_spec(24, DicknDavid);      /* Dick, guard of the Treasury */
    castle_mob_spec(25, DicknDavid);      /* David, Dicks brother */
    castle_mob_spec(26, jerry);           /* Jerry, the Gambler */
    castle_mob_spec(27, CastleGuard);     /* Michael */
    castle_mob_spec(28, CastleGuard);     /* Hans */
    castle_mob_spec(29, CastleGuard);     /* Boris */
}

/* Routine: member_of_staff. Used to see if a character is a member of the
 * castle staff. Used mainly by BANZAI:ng NPC:s. */
static int member_of_staff(struct char_data *chChar)
{
    int ch_num;

    if (!IS_NPC(chChar))
        return (FALSE);

    ch_num = GET_MOB_VNUM(chChar);

    if (ch_num == castle_virtual(1))
        return (TRUE);

    if (ch_num > castle_virtual(2) && ch_num < castle_virtual(15))
        return (TRUE);

    if (ch_num > castle_virtual(15) && ch_num < castle_virtual(18))
        return (TRUE);

    if (ch_num > castle_virtual(18) && ch_num < castle_virtual(30))
        return (TRUE);

    return (FALSE);
}

/* Function: member_of_royal_guard. Returns TRUE if the character is a guard on
 * duty, otherwise FALSE. Used by Peter the captain of the royal guard. */
static int member_of_royal_guard(struct char_data *chChar)
{
    int ch_num;

    if (!chChar || !IS_NPC(chChar))
        return (FALSE);

    ch_num = GET_MOB_VNUM(chChar);

    if (ch_num == castle_virtual(3) || ch_num == castle_virtual(6))
        return (TRUE);

    if (ch_num > castle_virtual(7) && ch_num < castle_virtual(12))
        return (TRUE);

    if (ch_num > castle_virtual(23) && ch_num < castle_virtual(26))
        return (TRUE);

    return (FALSE);
}

/* Function: find_npc_by_name. Returns a pointer to an npc by the given name.
 * Used by Tim and Tom. */
static struct char_data *find_npc_by_name(struct char_data *chAtChar, const char *pszName, int iLen)
{
    struct char_data *ch;

    for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
        if (IS_NPC(ch) && !strncmp(pszName, ch->player.short_descr, iLen))
            return (ch);

    return (NULL);
}

/* Function: find_guard. Returns the pointer to a guard on duty. Used by Peter
 * the Captain of the Royal Guard */
static struct char_data *find_guard(struct char_data *chAtChar)
{
    struct char_data *ch;

    for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
        if (!FIGHTING(ch) && member_of_royal_guard(ch))
            return (ch);

    return (NULL);
}

/* Function: get_victim. Returns a pointer to a randomly chosen character in
 * the same room, fighting someone in the castle staff. Used by BANZAII-ing
 * characters and King Welmar... */
static struct char_data *get_victim(struct char_data *chAtChar)
{
    struct char_data *ch;
    int iNum_bad_guys = 0, iVictim;

    for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
        if (FIGHTING(ch) && member_of_staff(FIGHTING(ch)))
            iNum_bad_guys++;

    if (!iNum_bad_guys)
        return (NULL);

    iVictim = rand_number(0, iNum_bad_guys); /* How nice, we give them a chance */
    if (!iVictim)
        return (NULL);

    iNum_bad_guys = 0;

    for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room) {
        if (FIGHTING(ch) == NULL)
            continue;

        if (!member_of_staff(FIGHTING(ch)))
            continue;

        if (++iNum_bad_guys != iVictim)
            continue;

        return (ch);
    }

    return (NULL);
}

/* Banzaii. Makes a character banzaii on attackers of the castle staff. Used
 * by Guards, Tim, Tom, Dick, David, Peter, Master, and the King. */
static int banzaii(struct char_data *ch)
{
    struct char_data *chOpponent;

    if (!AWAKE(ch) || GET_POS(ch) == POS_FIGHTING || !(chOpponent = get_victim(ch)))
        return (FALSE);

    act("$n berra: 'Protejam o Reino do Grande Rei Welmar!  BANZAIIII!!!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, chOpponent, TYPE_UNDEFINED);
    return (TRUE);
}

/* Do_npc_rescue. Makes ch_hero rescue ch_victim. Used by Tim and Tom. */
static int do_npc_rescue(struct char_data *ch_hero, struct char_data *ch_victim)
{
    struct char_data *ch_bad_guy;

    for (ch_bad_guy = world[IN_ROOM(ch_hero)].people; ch_bad_guy && (FIGHTING(ch_bad_guy) != ch_victim);
         ch_bad_guy = ch_bad_guy->next_in_room)
        ;

    /* NO WAY I'll rescue the one I'm fighting! */
    if (!ch_bad_guy || ch_bad_guy == ch_hero)
        return (FALSE);

    act("Você salva bravamente $N.", FALSE, ch_hero, 0, ch_victim, TO_CHAR);
    act("Você foi salv$r por $N, $X(seu,sua) leal amig$R!", FALSE, ch_victim, 0, ch_hero, TO_CHAR);
    act("$n heroicamente salva $N.", FALSE, ch_hero, 0, ch_victim, TO_NOTVICT);

    if (FIGHTING(ch_bad_guy))
        stop_fighting(ch_bad_guy);
    if (FIGHTING(ch_hero))
        stop_fighting(ch_hero);

    set_fighting(ch_hero, ch_bad_guy);
    set_fighting(ch_bad_guy, ch_hero);
    return (TRUE);
}

/* Procedure to block a person trying to enter a room. Used by Tim/Tom at Kings
 * bedroom and Dick/David at treasury. */
static int block_way(struct char_data *ch, int cmd, char *arg, room_vnum iIn_room, int iProhibited_direction)
{
    if (cmd != ++iProhibited_direction)
        return (FALSE);

    if (ch->player.short_descr && !strncmp(ch->player.short_descr, "Rei Welmar", 11))
        return (FALSE);

    if (IN_ROOM(ch) != real_room(iIn_room))
        return (FALSE);

    if (!member_of_staff(ch))
        act("O guarda berra para $n e empurra $l para trás.", FALSE, ch, 0, 0, TO_ROOM);

    send_to_char(ch, "O guarda berra: 'Entrada Proibida!', e empurra você para trás.\r\n");
    return (TRUE);
}

/* Routine to check if an object is trash. Used by James the Butler and the
 * Cleaning Lady. */
static int is_trash(struct obj_data *i)
{
    if (!OBJWEAR_FLAGGED(i, ITEM_WEAR_TAKE))
        return (FALSE);

    if (GET_OBJ_TYPE(i) == ITEM_DRINKCON || GET_OBJ_COST(i) <= 10 || GET_OBJ_TYPE(i) == ITEM_TRASH)
        return (TRUE);

    return (FALSE);
}

/* Fry_victim. Finds a suitabe victim, and cast some _NASTY_ spell on him. Used
 * by King Welmar. */
static void fry_victim(struct char_data *ch)
{
    struct char_data *tch;

    if (ch->points.mana < 10)
        return;

    /* Find someone suitable to fry ! */
    if (!(tch = get_victim(ch)))
        return;

    switch (rand_number(0, 8)) {
        case 1:
        case 2:
        case 3:
            send_to_char(ch, "Você ergue sua mão em um gesto teatral.\r\n");
            act("$n ergue sua mão em um gesto teatral.", 1, ch, 0, 0, TO_ROOM);
            cast_spell(ch, tch, 0, SPELL_COLOR_SPRAY);
            break;
        case 4:
        case 5:
            send_to_char(ch, "Você se concentra e resmunga consigo mesmo.\r\n");
            act("$n concentra-se, e resmunga cosigo mesm$r.", 1, ch, 0, 0, TO_ROOM);
            cast_spell(ch, tch, 0, SPELL_HARM);
            break;
        case 6:
        case 7:
            act("Você olha profundamente nos olhos de $N.", 1, ch, 0, tch, TO_CHAR);
            act("$n olha profundamente nos olhos de $N.", 1, ch, 0, tch, TO_NOTVICT);
            act("Você observa uma chama de maldade nos olhos de $n.", 1, ch, 0, tch, TO_VICT);
            cast_spell(ch, tch, 0, SPELL_FIREBALL);
            break;
        default:
            if (!rand_number(0, 1))
                cast_spell(ch, ch, 0, SPELL_HEAL);
            break;
    }

    ch->points.mana -= 10;

    return;
}

/* King_welmar. Control the actions and movements of the King. */
SPECIAL(king_welmar)
{
    char actbuf[MAX_INPUT_LENGTH];

    const char *monolog[] = {"$n proclama 'Primus in regnis Geticis coronam'.",
                             "$n proclama 'regiam gessi, subiique regis'.",
                             "$n proclama 'munus et mores colui sereno'.", "$n proclama 'principe dignos'."};

    const char bedroom_path[] = "s33004o1c1S.";
    const char throne_path[] = "W3o3cG52211rg.";
    const char monolog_path[] = "ABCDPPPP.";

    static const char *path;
    static int path_index;
    static bool move = FALSE;

    if (!move) {
        if (time_info.hours == 8 && IN_ROOM(ch) == castle_real_room(51)) {
            move = TRUE;
            path = throne_path;
            path_index = 0;
        } else if (time_info.hours == 21 && IN_ROOM(ch) == castle_real_room(17)) {
            move = TRUE;
            path = bedroom_path;
            path_index = 0;
        } else if (time_info.hours == 12 && IN_ROOM(ch) == castle_real_room(17)) {
            move = TRUE;
            path = monolog_path;
            path_index = 0;
        }
    }
    if (cmd || (GET_POS(ch) < POS_SLEEPING) || (GET_POS(ch) == POS_SLEEPING && !move))
        return (FALSE);

    if (GET_POS(ch) == POS_FIGHTING) {
        fry_victim(ch);
        return (FALSE);
    } else if (banzaii(ch))
        return (FALSE);

    if (!move)
        return (FALSE);

    switch (path[path_index]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
            perform_move(ch, path[path_index] - '0', 1);
            break;

        case 'A':
        case 'B':
        case 'C':
        case 'D':
            act(monolog[path[path_index] - 'A'], FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'P':
            break;

        case 'W':
            GET_POS(ch) = POS_STANDING;
            act("$n acorda e coloca-se de pé.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'S':
            GET_POS(ch) = POS_SLEEPING;
            act("$n se deita em sua bela cama e, instantâneamente adormece.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'r':
            GET_POS(ch) = POS_SITTING;
            act("$n senta-se em seu imponente trono.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 's':
            GET_POS(ch) = POS_STANDING;
            act("$n levanta-se.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'G':
            act("$n diz 'Bom dia, veneráveis amigos.'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'g':
            act("$n diz 'Bom dia, queridos súditos.'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'o':
            do_gen_door(ch, strcpy(actbuf, "porta"), 0, SCMD_UNLOCK); /* strcpy: OK */
            do_gen_door(ch, strcpy(actbuf, "porta"), 0, SCMD_OPEN);   /* strcpy: OK */
            break;

        case 'c':
            do_gen_door(ch, strcpy(actbuf, "porta"), 0, SCMD_CLOSE); /* strcpy: OK */
            do_gen_door(ch, strcpy(actbuf, "porta"), 0, SCMD_LOCK);  /* strcpy: OK */
            break;

        case '.':
            move = FALSE;
            break;
    }

    path_index++;
    return (FALSE);
}

/* Training_master. Acts actions to the training room, if his students are
 * present. Also allowes warrior-class to practice. Used by the Training
 * Master. */
SPECIAL(training_master)
{
    struct char_data *pupil1, *pupil2 = NULL, *tch;

    if (!AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
        return (FALSE);

    if (cmd)
        return (FALSE);

    if (banzaii(ch) || rand_number(0, 2))
        return (FALSE);

    if (!(pupil1 = find_npc_by_name(ch, "Brian", 5)))
        return (FALSE);

    if (!(pupil2 = find_npc_by_name(ch, "Mick", 4)))
        return (FALSE);

    if (FIGHTING(pupil1) || FIGHTING(pupil2))
        return (FALSE);

    if (rand_number(0, 1)) {
        tch = pupil1;
        pupil1 = pupil2;
        pupil2 = tch;
    }

    switch (rand_number(0, 7)) {
        case 0:
            act("$n atinge $N na cabeça com um poderoso golpe.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
            act("Você atinge $N na cabeça com um poderoso golpe.", FALSE, pupil1, 0, pupil2, TO_CHAR);
            act("$n atinge você na cabeça com um poderoso golpe.", FALSE, pupil1, 0, pupil2, TO_VICT);
            break;

        case 1:
            act("$n atinge $N no tórax com um ataque.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
            act("Você desfere um ataque no toráx de $N.", FALSE, pupil1, 0, pupil2, TO_CHAR);
            act("$n desfere um ataque no seu tórax.", FALSE, pupil1, 0, pupil2, TO_VICT);
            break;

        case 2:
            send_to_char(ch, "Você ordena seus alunos a se cumprimentarem.\r\n.");
            act("$n ordena seus alunos a se cumprimentarem.", FALSE, ch, 0, 0, TO_ROOM);
            act("$n se curva diante de $N.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
            act("$N se curva diante de $n.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
            act("Você se curva diante de $N, que retribui da mesma forma.", FALSE, pupil1, 0, pupil2, TO_CHAR);
            act("Você se curva diante de $n, que retribui da mesma forma.", FALSE, pupil1, 0, pupil2, TO_VICT);
            break;

        case 3:
            act("$N grita com $n, enquanto $l cai no chão e solta a sua arma.", FALSE, pupil1, 0, ch, TO_NOTVICT);
            act("$n rapidamente pega sua arma.", FALSE, pupil1, 0, 0, TO_ROOM);
            act("$N grita com você enquanto você cai, perdendo sua arma.", FALSE, pupil1, 0, ch, TO_CHAR);
            send_to_char(pupil1, "Você rapidamente pega sua arma de novo.\r\n");
            act("Você grita com $n, enquanto $l cai no chão, perdendo sua arma.", FALSE, pupil1, 0, ch, TO_VICT);
            break;

        case 4:
            act("$N trapaceia com $n, e golpeia $l pelas costas.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
            act("$N trapaceia com voc�ê e golpeia você pelas costas.", FALSE, pupil1, 0, pupil2, TO_CHAR);
            act("Você trapaceia com $n, e rapidamente golpeia $l pelas costas.", FALSE, pupil1, 0, pupil2, TO_VICT);
            break;

        case 5:
            act("$n investe um golpe contra $N mas $N desvia habilidosamente.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
            act("Você investe um golpe contra $N mas $L desvia habilidosamente.", FALSE, pupil1, 0, pupil2, TO_CHAR);
            act("$n investe um golpe contra você, mas você habilidosamente desvia dele.", FALSE, pupil1, 0, pupil2,
                TO_VICT);
            break;

        case 6:
            act("$n desajeitadamente tenta chutar $N, mas erra.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
            act("Você erra grosseiramente um chute contra $N, sem direito a desculpas.", FALSE, pupil1, 0, pupil2,
                TO_CHAR);
            act("$n falha uma anormal tentativa grosseira de chutar você.", FALSE, pupil1, 0, pupil2, TO_VICT);
            break;

        default:
            send_to_char(ch, "Você mostra para seus alunos uma avançada técnica.\r\n");
            act("$n mostra para seus alunos uma avançada técnica.", FALSE, ch, 0, 0, TO_ROOM);
            break;
    }

    return (FALSE);
}

SPECIAL(tom) { return castle_twin_proc(ch, cmd, argument, 48, "Tim"); }

SPECIAL(tim) { return castle_twin_proc(ch, cmd, argument, 49, "Tom"); }

/* Common routine for the Castle Twins. */
static int castle_twin_proc(struct char_data *ch, int cmd, char *arg, int ctlnum, const char *twinname)
{
    struct char_data *king, *twin;

    if (!AWAKE(ch))
        return (FALSE);

    if (cmd)
        return block_way(ch, cmd, arg, castle_virtual(ctlnum), 1);

    if ((king = find_npc_by_name(ch, "Rei Welmar", 11)) != NULL) {
        char actbuf[MAX_INPUT_LENGTH];

        if (!ch->master)
            do_follow(ch, strcpy(actbuf, "Rei Welmar"), 0, 0); /* strcpy: OK */
        if (FIGHTING(king))
            do_npc_rescue(ch, king);
    }

    if ((twin = find_npc_by_name(ch, twinname, strlen(twinname))) != NULL)
        if (FIGHTING(twin) && 2 * GET_HIT(twin) < GET_HIT(ch))
            do_npc_rescue(ch, twin);

    if (GET_POS(ch) != POS_FIGHTING)
        banzaii(ch);

    return (FALSE);
}

/* Routine for James the Butler. Complains if he finds any trash. This doesn't
 * make sure he _can_ carry it. */
SPECIAL(James) { return castle_cleaner(ch, cmd, TRUE); }

/* Common code for James and the Cleaning Woman. */
static int castle_cleaner(struct char_data *ch, int cmd, int gripe)
{
    struct obj_data *i;

    if (cmd || !AWAKE(ch) || GET_POS(ch) == POS_FIGHTING)
        return (FALSE);

    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
        if (!is_trash(i))
            continue;

        if (gripe) {
            act("$n diz: 'Deus, Ó Deus!  Eu tenho de demitir esta faxineira preguiçosa!'", FALSE, ch, 0, 0, TO_ROOM);
            act("$n recolhe um pouco de lixo.", FALSE, ch, 0, 0, TO_ROOM);
        }
        obj_from_room(i);
        obj_to_char(i, ch);
        return (TRUE);
    }

    return (FALSE);
}

/* Routine for the Cleaning Woman. Picks up any trash she finds. */
SPECIAL(cleaning) { return castle_cleaner(ch, cmd, FALSE); }

/* CastleGuard. Standard routine for ordinary castle guards. */
SPECIAL(CastleGuard)
{
    if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
        return (FALSE);

    return (banzaii(ch));
}

/* DicknDave. Routine for the guards Dick and David. */
SPECIAL(DicknDavid)
{
    if (!AWAKE(ch))
        return (FALSE);

    if (!cmd && GET_POS(ch) != POS_FIGHTING)
        banzaii(ch);

    return (block_way(ch, cmd, argument, castle_virtual(36), 1));
}

/*Peter. Routine for Captain of the Guards. */
SPECIAL(peter)
{
    struct char_data *ch_guard = NULL;

    if (cmd || !AWAKE(ch) || GET_POS(ch) == POS_FIGHTING)
        return (FALSE);

    if (banzaii(ch))
        return (FALSE);

    if (!(rand_number(0, 3)) && (ch_guard = find_guard(ch)))
        switch (rand_number(0, 5)) {
            case 0:
                act("$N recupera rapidamente sua atenção no momento em que $n inspeciona $L.", FALSE, ch, 0, ch_guard,
                    TO_NOTVICT);
                act("$N recupera rapidamente sua atenção no momento em que você inspeciona $L.", FALSE, ch, 0, ch_guard,
                    TO_CHAR);
                act("Você recupera rapidamente a atenção no momento em que $n inspeciona você.", FALSE, ch, 0, ch_guard,
                    TO_VICT);
                break;
            case 1:
                act("$N sente-se muito humilhad$R, no instante que $n berra com $L.", FALSE, ch, 0, ch_guard,
                    TO_NOTVICT);
                act("$N sente-se muito humilhad$R no instante que você berra com $L.", FALSE, ch, 0, ch_guard, TO_CHAR);
                act("Você se sente muito humilhado no instante que $N berra com você.", FALSE, ch, 0, ch_guard,
                    TO_VICT);
                break;
            case 2:
                act("$n fornece para $N algumas ordens reais.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
                act("Você fornece para $N algumas ordens reais.", FALSE, ch, 0, ch_guard, TO_CHAR);
                act("$n fornece a você algumas ordens reais.", FALSE, ch, 0, ch_guard, TO_VICT);
                break;
            case 3:
                act("$n olha para você.", FALSE, ch, 0, ch_guard, TO_VICT);
                act("$n olha para $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
                act("$n rosna: 'Estas botas precisam de polimento!'", FALSE, ch, 0, ch_guard, TO_ROOM);
                act("Você rosna com $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
                break;
            case 4:
                act("$n olha para você.", FALSE, ch, 0, ch_guard, TO_VICT);
                act("$n olha para $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
                act("$n rosna: 'Arrume seu colarinho!'", FALSE, ch, 0, ch_guard, TO_ROOM);
                act("Você rosna com $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
                break;
            default:
                act("$n olha para você.", FALSE, ch, 0, ch_guard, TO_VICT);
                act("$n olha para $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
                act("$n rosna: 'Esta armadura de correntes parece enferrujada!  LIMPE-A !!!'", FALSE, ch, 0, ch_guard,
                    TO_ROOM);
                act("Você rosna com $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
                break;
        }

    return (FALSE);
}

/* Procedure for Jerry and Michael in x08 of King's Castle. Code by Sapowox
 * modified by Pjotr.(Original code from Master) */
SPECIAL(jerry)
{
    struct char_data *gambler1, *gambler2 = NULL, *tch;

    if (!AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
        return (FALSE);

    if (cmd)
        return (FALSE);

    if (banzaii(ch) || rand_number(0, 2))
        return (FALSE);

    if (!(gambler1 = ch))
        return (FALSE);

    if (!(gambler2 = find_npc_by_name(ch, "Michael", 7)))
        return (FALSE);

    if (FIGHTING(gambler1) || FIGHTING(gambler2))
        return (FALSE);

    if (rand_number(0, 1)) {
        tch = gambler1;
        gambler1 = gambler2;
        gambler2 = tch;
    }

    switch (rand_number(0, 5)) {
        case 0:
            act("$n joga os dados e, alegre, grita alto com o resultado.", FALSE, gambler1, 0, gambler2, TO_NOTVICT);
            act("Você joga os dados e grita: GRANDE!", FALSE, gambler1, 0, gambler2, TO_CHAR);
            act("$n grita alto enquanto $l joga os dados.", FALSE, gambler1, 0, gambler2, TO_VICT);
            break;
        case 1:
            act("$n amaldiçoa a Deusa da Sorte veementemente quando $l vê a jogada de $N.", FALSE, gambler1, 0,
                gambler2, TO_NOTVICT);
            act("Você amaldiçoa a Deusa da Sorte quando $N joga.", FALSE, gambler1, 0, gambler2, TO_CHAR);
            act("$n pragueja irritadamente. Você está com sorte!", FALSE, gambler1, 0, gambler2, TO_VICT);
            break;
        case 2:
            act("$n suspira alto e dá para $N um pouco de ouro.", FALSE, gambler1, 0, gambler2, TO_NOTVICT);
            act("Você suspira alto pela dor de ter que dar para $N um pouco de ouro.", FALSE, gambler1, 0, gambler2,
                TO_CHAR);
            act("$n suspira alto no instante em que $l lhe dá seu justo prêmio pela vitória.", FALSE, gambler1, 0,
                gambler2, TO_VICT);
            break;
        case 3:
            act("$n sorri chei$r de remorso quando a jogada de $N supera a d$l.", FALSE, gambler1, 0, gambler2,
                TO_NOTVICT);
            act("Você sorri tristemente quando você vê que $N vence você. Novamente.", FALSE, gambler1, 0, gambler2,
                TO_CHAR);
            act("$n sorri chei$r de remorso quando sua jogada supera a d$l.", FALSE, gambler1, 0, gambler2, TO_VICT);
            break;
        case 4:
            act("$n, excitad$r, segue a jogada com seus olhos.", FALSE, gambler1, 0, gambler2, TO_NOTVICT);
            act("Você excitadamente segue a jogada com seus olhos.", FALSE, gambler1, 0, gambler2, TO_CHAR);
            act("$n, excitad$r, segue a jogada com seus olhos.", FALSE, gambler1, 0, gambler2, TO_VICT);
            break;
        default:
            act("$n diz 'Bem, minha sorte tem de mudar logo', no momento em que $l balança os dados.", FALSE, gambler1,
                0, gambler2, TO_NOTVICT);
            act("Você diz 'Bem, minha sorte tem de mudar logo' e balança os dados.", FALSE, gambler1, 0, gambler2,
                TO_CHAR);
            act("$n diz 'Bem, minha sorte tem de mudar logo', no momento em que $l balança os dados.", FALSE, gambler1,
                0, gambler2, TO_VICT);
            break;
    }
    return (FALSE);
}