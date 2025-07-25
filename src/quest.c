/* ***********************************************************************
 *    File:   quest.c                                  Part of CircleMUD  *
 * Version:   2.1 (December 2005) Written for CircleMud CWG / Suntzu      *
 * Purpose:   To provide special quest-related code.                      *
 * Copyright: Kenneth Ray                                                 *
 * Original Version Details:                                              *
 * Morgaelin - quest.c                                                    *
 * Copyright (C) 1997 MS                                                  *
 *********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include <string.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "quest.h"
#include "act.h" /* for do_tell */

/*--------------------------------------------------------------------------
 * Exported global variables
 *--------------------------------------------------------------------------*/
const char *quest_types[] = {"Object",        "Room",       "Find mob",    "Kill mob",          "Save mob",
                             "Return object", "Clear room", "Kill player", "Kill mob (bounty)", "\n"};
const char *aq_flags[] = {"REPEATABLE", "MOB_POSTED", "\n"};

/*--------------------------------------------------------------------------
 * Local (file scope) global variables
 *--------------------------------------------------------------------------*/
static int cmd_tell;

static const char *quest_cmd[] = {"list", "history", "join", "leave", "progress", "status", "\n"};

static const char *quest_mort_usage = "Uso: quest list | history | progress | join <nn> | leave";

static const char *quest_imm_usage = "Uso: quest list | history | progress | join <nn> | leave | status <vnum>";

/*--------------------------------------------------------------------------*/
/* Utility Functions                                                        */
/*--------------------------------------------------------------------------*/

qst_rnum real_quest(qst_vnum vnum)
{
    int rnum;

    for (rnum = 0; rnum < total_quests; rnum++)
        if (QST_NUM(rnum) == vnum)
            return (rnum);
    return (NOTHING);
}

int is_complete(struct char_data *ch, qst_vnum vnum)
{
    int i;

    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
        if (ch->player_specials->saved.completed_quests[i] == vnum)
            return TRUE;
    return FALSE;
}

qst_vnum find_quest_by_qmnum(struct char_data *ch, mob_vnum qm, int num)
{
    qst_rnum rnum;
    int found = 0;
    for (rnum = 0; rnum < total_quests; rnum++) {
        if (qm == QST_MASTER(rnum))
            if (++found == num)
                return (QST_NUM(rnum));
    }
    return NOTHING;
}

/* Find quest by list position number in available quest list for a questmaster */
qst_vnum find_available_quest_by_qmnum(struct char_data *ch, mob_vnum qm, int num)
{
    qst_rnum rnum;
    int counter = 0;
    int quest_completed, quest_repeatable;

    for (rnum = 0; rnum < total_quests; rnum++) {
        if (qm == QST_MASTER(rnum)) {
            quest_completed = is_complete(ch, QST_NUM(rnum));
            quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

            /* Only count quest if it's available (not completed or repeatable) */
            if (!quest_completed || quest_repeatable) {
                if (++counter == num)
                    return (QST_NUM(rnum));
            }
        }
    }
    return NOTHING;
}

/* Find quest by list position number in the global quest list - for immortals */
qst_vnum find_quest_by_listnum(int num)
{
    if (num < 1 || num > total_quests)
        return NOTHING;
    return QST_NUM(num - 1); /* Convert from 1-based to 0-based index */
}

/*--------------------------------------------------------------------------*/
/* Quest Loading and Unloading Functions                                    */
/*--------------------------------------------------------------------------*/

void destroy_quests(void)
{
    qst_rnum rnum = 0;

    if (!aquest_table)
        return;

    for (rnum = 0; rnum < total_quests; rnum++) {
        free_quest_strings(&aquest_table[rnum]);
    }
    free(aquest_table);
    aquest_table = NULL;
    total_quests = 0;

    return;
}

int count_quests(qst_vnum low, qst_vnum high)
{
    int i, j;

    if (!aquest_table)
        return 0;

    for (i = j = 0; i < total_quests; i++)
        if (QST_NUM(i) >= low && QST_NUM(i) <= high)
            j++;

    return j;
}

void parse_quest(FILE *quest_f, int nr)
{
    static char line[256];
    static int i = 0, j;
    int retval = 0, t[7];
    char f1[128], buf2[MAX_STRING_LENGTH];
    aquest_table[i].vnum = nr;
    aquest_table[i].qm = NOBODY;
    aquest_table[i].name = NULL;
    aquest_table[i].desc = NULL;
    aquest_table[i].info = NULL;
    aquest_table[i].done = NULL;
    aquest_table[i].quit = NULL;
    aquest_table[i].flags = 0;
    aquest_table[i].type = -1;
    aquest_table[i].target = -1;
    aquest_table[i].prereq = NOTHING;
    for (j = 0; j < 7; j++)
        aquest_table[i].value[j] = 0;
    aquest_table[i].prev_quest = NOTHING;
    aquest_table[i].next_quest = NOTHING;
    aquest_table[i].func = NULL;

    aquest_table[i].gold_reward = 0;
    aquest_table[i].exp_reward = 0;
    aquest_table[i].obj_reward = NOTHING;

    /* begin to parse the data */
    aquest_table[i].name = fread_string(quest_f, buf2);
    aquest_table[i].desc = fread_string(quest_f, buf2);
    aquest_table[i].info = fread_string(quest_f, buf2);
    aquest_table[i].done = fread_string(quest_f, buf2);
    aquest_table[i].quit = fread_string(quest_f, buf2);
    if (!get_line(quest_f, line) ||
        (retval = sscanf(line, " %d %d %s %d %d %d %d", t, t + 1, f1, t + 2, t + 3, t + 4, t + 5)) != 7) {
        log1("Format error in numeric line (expected 7, got %d), %s\n", retval, line);
        exit(1);
    }
    aquest_table[i].type = t[0];
    aquest_table[i].qm = (real_mobile(t[1]) == NOBODY) ? NOBODY : t[1];
    aquest_table[i].flags = asciiflag_conv(f1);
    aquest_table[i].target = (t[2] == -1) ? NOTHING : t[2];
    aquest_table[i].prev_quest = (t[3] == -1) ? NOTHING : t[3];
    aquest_table[i].next_quest = (t[4] == -1) ? NOTHING : t[4];
    aquest_table[i].prereq = (t[5] == -1) ? NOTHING : t[5];
    if (!get_line(quest_f, line) ||
        (retval = sscanf(line, " %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6)) != 7) {
        log1("Format error in numeric line (expected 7, got %d), %s\n", retval, line);
        exit(1);
    }
    for (j = 0; j < 7; j++)
        aquest_table[i].value[j] = t[j];

    if (!get_line(quest_f, line) || (retval = sscanf(line, " %d %d %d", t, t + 1, t + 2)) != 3) {
        log1("Format error in numeric (rewards) line (expected 3, got %d), %s\n", retval, line);
        exit(1);
    }

    aquest_table[i].gold_reward = t[0];
    aquest_table[i].exp_reward = t[1];
    aquest_table[i].obj_reward = (t[2] == -1) ? NOTHING : t[2];

    for (;;) {
        if (!get_line(quest_f, line)) {
            log1("Format error in %s\n", line);
            exit(1);
        }
        switch (*line) {
            case 'S':
                total_quests = ++i;
                return;
        }
    }
} /* parse_quest */

void assign_the_quests(void)
{
    qst_rnum rnum;
    mob_rnum mrnum;

    cmd_tell = find_command("tell");

    for (rnum = 0; rnum < total_quests; rnum++) {
        if (QST_MASTER(rnum) == NOBODY) {
            log1("SYSERR: Quest #%d has no questmaster specified.", QST_NUM(rnum));
            continue;
        }
        if ((mrnum = real_mobile(QST_MASTER(rnum))) == NOBODY) {
            log1("SYSERR: Quest #%d has an invalid questmaster.", QST_NUM(rnum));
            continue;
        }
        if (mob_index[(mrnum)].func && mob_index[(mrnum)].func != questmaster)
            QST_FUNC(rnum) = mob_index[(mrnum)].func;
        mob_index[(mrnum)].func = questmaster;
    }
}

/*--------------------------------------------------------------------------*/
/* Quest Completion Functions                                               */
/*--------------------------------------------------------------------------*/
void set_quest(struct char_data *ch, qst_rnum rnum)
{
    GET_QUEST(ch) = QST_NUM(rnum);
    GET_QUEST_TIME(ch) = QST_TIME(rnum);
    GET_QUEST_COUNTER(ch) = QST_QUANTITY(rnum);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_QUEST);
    return;
}

void clear_quest(struct char_data *ch)
{
    GET_QUEST(ch) = NOTHING;
    GET_QUEST_TIME(ch) = -1;
    GET_QUEST_COUNTER(ch) = 0;
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_QUEST);
    return;
}

void add_completed_quest(struct char_data *ch, qst_vnum vnum)
{
    qst_vnum *temp;
    int i;

    CREATE(temp, qst_vnum, GET_NUM_QUESTS(ch) + 1);
    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
        temp[i] = ch->player_specials->saved.completed_quests[i];

    temp[GET_NUM_QUESTS(ch)] = vnum;
    GET_NUM_QUESTS(ch)++;

    if (ch->player_specials->saved.completed_quests)
        free(ch->player_specials->saved.completed_quests);
    ch->player_specials->saved.completed_quests = temp;
}

void remove_completed_quest(struct char_data *ch, qst_vnum vnum)
{
    qst_vnum *temp;
    int i, j = 0;

    CREATE(temp, qst_vnum, GET_NUM_QUESTS(ch));
    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
        if (ch->player_specials->saved.completed_quests[i] != vnum)
            temp[j++] = ch->player_specials->saved.completed_quests[i];

    GET_NUM_QUESTS(ch)--;

    if (ch->player_specials->saved.completed_quests)
        free(ch->player_specials->saved.completed_quests);
    ch->player_specials->saved.completed_quests = temp;
}

void generic_complete_quest(struct char_data *ch)
{
    qst_rnum rnum;
    qst_vnum vnum = GET_QUEST(ch);
    struct obj_data *new_obj;
    int happy_qp, happy_gold, happy_exp;

    if (--GET_QUEST_COUNTER(ch) <= 0) {
        rnum = real_quest(vnum);
        if (IS_HAPPYHOUR && IS_HAPPYQP) {
            happy_qp = (int)(QST_POINTS(rnum) * (((float)(100 + HAPPY_QP)) / (float)100));
            happy_qp = MAX(happy_qp, 0);
            GET_QUESTPOINTS(ch) += happy_qp;
            send_to_char(ch, "%s\r\nVocê recebeu %d pontos de busca pelos seus serviços.\r\n", QST_DONE(rnum),
                         happy_qp);
        } else {
            GET_QUESTPOINTS(ch) += QST_POINTS(rnum);
            send_to_char(ch, "%s\r\nVocê recebeu %d pontos de busca pelos seus serviços.\r\n", QST_DONE(rnum),
                         QST_POINTS(rnum));
        }
        if (QST_GOLD(rnum)) {
            if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD)) {
                happy_gold = (int)(QST_GOLD(rnum) * (((float)(100 + HAPPY_GOLD)) / (float)100));
                happy_gold = MAX(happy_gold, 0);
                increase_gold(ch, happy_gold);
                send_to_char(ch, "Você recebeu %d moedas pelos seus serviços.\r\n", happy_gold);
            } else {
                increase_gold(ch, QST_GOLD(rnum));
                send_to_char(ch, "Você recebeu %d moedas pelos seus serviços.\r\n", QST_GOLD(rnum));
            }
        }
        if (QST_EXP(rnum)) {
            gain_exp(ch, QST_EXP(rnum));
            if ((IS_HAPPYHOUR) && (IS_HAPPYEXP)) {
                happy_exp = (int)(QST_EXP(rnum) * (((float)(100 + HAPPY_EXP)) / (float)100));
                happy_exp = MAX(happy_exp, 0);
                send_to_char(ch, "Você recebeu %d pontos de experiência pelos seus serviços.\r\n", happy_exp);
            } else {
                send_to_char(ch, "Você recebeu %d pontos de experiência pelos seus serviços.\r\n", QST_EXP(rnum));
            }
        }
        if (QST_OBJ(rnum) && QST_OBJ(rnum) != NOTHING) {
            if (real_object(QST_OBJ(rnum)) != NOTHING) {
                if ((new_obj = read_object((QST_OBJ(rnum)), VIRTUAL)) != NULL) {
                    obj_to_char(new_obj, ch);
                    send_to_char(ch, "Você foi presentead%s com %s%s pelos seus serviços.\r\n", OA(ch),
                                 GET_OBJ_SHORT(new_obj), CCNRM(ch, C_NRM));
                }
            }
        }
        if (!IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE))
            add_completed_quest(ch, vnum);
        clear_quest(ch);

        /* Cleanup wishlist quests after completion */
        cleanup_completed_wishlist_quest(vnum);

        if ((real_quest(QST_NEXT(rnum)) != NOTHING) && (QST_NEXT(rnum) != vnum) && !is_complete(ch, QST_NEXT(rnum))) {
            rnum = real_quest(QST_NEXT(rnum));
            set_quest(ch, rnum);
            send_to_char(ch, "A sua busca continua:\r\n%s", QST_INFO(rnum));
        }
    }
    save_char(ch);
}

void autoquest_trigger_check(struct char_data *ch, struct char_data *vict, struct obj_data *object, int type)
{
    struct char_data *i;
    qst_rnum rnum;
    int found = TRUE;

    if (IS_NPC(ch))
        return;
    if (GET_QUEST(ch) == NOTHING) /* No current quest, skip this */
        return;
    if (GET_QUEST_TYPE(ch) != type)
        return;
    if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING)
        return;
    switch (type) {
        case AQ_OBJ_FIND:
            if (QST_TARGET(rnum) == GET_OBJ_VNUM(object))
                generic_complete_quest(ch);
            break;
        case AQ_ROOM_FIND:
            if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number)
                generic_complete_quest(ch);
            break;
        case AQ_MOB_FIND:
            for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
                if (IS_NPC(i))
                    if (QST_TARGET(rnum) == GET_MOB_VNUM(i))
                        generic_complete_quest(ch);
            break;
        case AQ_MOB_KILL:
            if (!IS_NPC(ch) && IS_NPC(vict) && (ch != vict))
                if (QST_TARGET(rnum) == GET_MOB_VNUM(vict))
                    generic_complete_quest(ch);
            break;
        case AQ_MOB_SAVE:
            if (ch == vict)
                found = FALSE;
            for (i = world[IN_ROOM(ch)].people; i && found; i = i->next_in_room)
                if (i && IS_NPC(i) && !MOB_FLAGGED(i, MOB_NOTDEADYET))
                    if ((GET_MOB_VNUM(i) != QST_TARGET(rnum)) && !AFF_FLAGGED(i, AFF_CHARM))
                        found = FALSE;
            if (found)
                generic_complete_quest(ch);
            break;
        case AQ_OBJ_RETURN:
            /* Enhanced logic: allow return to either the original requester or the questmaster */
            if (IS_NPC(vict) && object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum))) {
                if (GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum)) {
                    /* Returned directly to original requester - complete quest normally */
                    generic_complete_quest(ch);
                } else if (GET_MOB_VNUM(vict) == QST_MASTER(rnum)) {
                    /* Returned to questmaster - transfer to original requester if different */
                    mob_rnum original_requester_rnum = real_mobile(QST_RETURNMOB(rnum));
                    if (original_requester_rnum != NOBODY) {
                        struct char_data *original_requester = NULL;

                        /* Find the original requester in the world */
                        for (original_requester = character_list; original_requester;
                             original_requester = original_requester->next) {
                            if (IS_NPC(original_requester) &&
                                GET_MOB_RNUM(original_requester) == original_requester_rnum) {
                                break;
                            }
                        }

                        if (original_requester && original_requester != vict) {
                            /* Transfer item from questmaster to original requester */
                            obj_from_char(object);
                            obj_to_char(object, original_requester);
                            act("$n entrega $p para $N.", FALSE, vict, object, original_requester, TO_ROOM);
                            act("$n recebe $p de $N.", FALSE, original_requester, object, vict, TO_ROOM);
                        }

                        /* Complete the quest */
                        generic_complete_quest(ch);
                    }
                }
            }
            break;
        case AQ_ROOM_CLEAR:
            if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number) {
                for (i = world[IN_ROOM(ch)].people; i && found; i = i->next_in_room)
                    if (i && IS_NPC(i) && !MOB_FLAGGED(i, MOB_NOTDEADYET))
                        found = FALSE;
                if (found)
                    generic_complete_quest(ch);
            }
            break;
        case AQ_PLAYER_KILL:
            if (!IS_NPC(ch) && !IS_NPC(vict) && (ch != vict))
                generic_complete_quest(ch);
            break;
        case AQ_MOB_KILL_BOUNTY:
            if (!IS_NPC(ch) && IS_NPC(vict) && (ch != vict))
                if (QST_TARGET(rnum) == GET_MOB_VNUM(vict))
                    generic_complete_quest(ch);
            break;
        default:
            log1("SYSERR: Invalid quest type passed to autoquest_trigger_check");
            break;
    }
}

void quest_timeout(struct char_data *ch)
{
    if ((GET_QUEST(ch) != NOTHING) && (GET_QUEST_TIME(ch) != -1)) {
        clear_quest(ch);
        send_to_char(ch, "O tempo da sua busca expirou!\r\n");
    }
}

void check_timed_quests(void)
{
    struct char_data *ch;

    for (ch = character_list; ch; ch = ch->next)
        if (!IS_NPC(ch) && (GET_QUEST(ch) != NOTHING) && (GET_QUEST_TIME(ch) != -1))
            if (--GET_QUEST_TIME(ch) == 0)
                quest_timeout(ch);
}

/*--------------------------------------------------------------------------*/
/* Quest Command Helper Functions                                           */
/*--------------------------------------------------------------------------*/
/* Para imortais no qlist */
void list_quests(struct char_data *ch, zone_rnum zone, qst_vnum vmin, qst_vnum vmax)
{
    qst_rnum rnum;
    qst_vnum bottom, top;
    int counter = 0;

    if (zone != NOWHERE) {
        bottom = zone_table[zone].bot;
        top = zone_table[zone].top;
    } else {
        bottom = vmin;
        top = vmax;
    }
    /* Print the header for the quest listing. */
    send_to_char(ch,
                 "Index VNum    Description                                  Questmaster\r\n"
                 "----- ------- -------------------------------------------- -----------\r\n");
    for (rnum = 0; rnum < total_quests; rnum++)
        if (QST_NUM(rnum) >= bottom && QST_NUM(rnum) <= top)
            send_to_char(ch, "\tg%4d\tn) [\tg%-5d\tn] \tc%-44.44s\tn \ty[%5d]\tn\r\n", ++counter, QST_NUM(rnum),
                         QST_NAME(rnum), QST_MASTER(rnum) == NOBODY ? 0 : QST_MASTER(rnum));
    if (!counter)
        send_to_char(ch, "None found.\r\n");
}

static void quest_hist(struct char_data *ch)
{
    int i = 0, counter = 0;
    qst_rnum rnum = NOTHING;

    send_to_char(ch,
                 "Você completou as seguintes buscas:\r\n"
                 "Num.  Descrição                                            Responsável\r\n"
                 "----- ---------------------------------------------------- -----------\r\n");
    for (i = 0; i < GET_NUM_QUESTS(ch); i++) {
        if ((rnum = real_quest(ch->player_specials->saved.completed_quests[i])) != NOTHING)
            send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty%s\tn\r\n", ++counter, QST_DESC(rnum),
                         (real_mobile(QST_MASTER(rnum)) == NOBODY)
                             ? "Desconhecido"
                             : GET_NAME(&mob_proto[(real_mobile(QST_MASTER(rnum)))]));
        else
            send_to_char(ch, "\tg%4d\tn) \tcBusca desconhecida! Não existe mais!\tn\r\n", ++counter);
    }
    if (!counter)
        send_to_char(ch, "Você não completou nenhuma busca ainda.\r\n");
}

/* Unified quest finding function - searches both regular and temporary quests */
static qst_vnum find_unified_quest_by_qmnum(struct char_data *ch, struct char_data *qm, int num)
{
    qst_rnum rnum;
    int counter = 0, i;
    int quest_completed, quest_repeatable;
    mob_vnum qm_vnum = GET_MOB_VNUM(qm);

    /* First, search through regular quests assigned to this questmaster */
    for (rnum = 0; rnum < total_quests; rnum++) {
        if (qm_vnum == QST_MASTER(rnum)) {
            quest_completed = is_complete(ch, QST_NUM(rnum));
            quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

            /* Only count quest if it's available (not completed or repeatable) */
            if (!quest_completed || quest_repeatable) {
                if (++counter == num)
                    return (QST_NUM(rnum));
            }
        }
    }

    /* Then, search through temporary quests if this mob is a temporary questmaster */
    if (IS_TEMP_QUESTMASTER(qm) && GET_NUM_TEMP_QUESTS(qm) > 0) {
        for (i = 0; i < GET_NUM_TEMP_QUESTS(qm); i++) {
            rnum = real_quest(GET_TEMP_QUESTS(qm)[i]);
            if (rnum == NOTHING)
                continue;

            quest_completed = is_complete(ch, QST_NUM(rnum));
            quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

            /* Only count quest if it's available (not completed or repeatable) */
            if (!quest_completed || quest_repeatable) {
                if (++counter == num)
                    return GET_TEMP_QUESTS(qm)[i];
            }
        }
    }

    return NOTHING;
}

void quest_list(struct char_data *ch, struct char_data *qm, char argument[MAX_INPUT_LENGTH])
{
    qst_vnum vnum;
    qst_rnum rnum;

    if ((vnum = find_unified_quest_by_qmnum(ch, qm, atoi(argument))) == NOTHING)
        send_to_char(ch, "Esta não é uma busca válida!\r\n");
    else if ((rnum = real_quest(vnum)) == NOTHING)
        send_to_char(ch, "Esta não é uma busca válida!\r\n");
    else if (QST_INFO(rnum)) {
        send_to_char(ch, "Detalhes Completos da Busca \tc%s\tn:\r\n%s", QST_DESC(rnum), QST_INFO(rnum));
        if (QST_PREV(rnum) != NOTHING)
            send_to_char(ch, "Você precisa completar a busca %s primeiro.\r\n", QST_NAME(real_quest(QST_PREV(rnum))));
        if (QST_TIME(rnum) != -1)
            send_to_char(ch, "A busca tem um tempo limite de %d tick%s para ser completada.\r\n", QST_TIME(rnum),
                         QST_TIME(rnum) == 1 ? "" : "s");
    } else
        send_to_char(ch, "Não existem mais informações sobre esta busca!\r\n");
}

static void quest_quit(struct char_data *ch)
{
    qst_rnum rnum;

    if (GET_QUEST(ch) == NOTHING)
        send_to_char(ch, "Mas você não tem busca ativa no momento!\r\n");
    else if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING) {
        clear_quest(ch);
        send_to_char(ch, "Você não faz mais parte desta busca.\r\n");
        save_char(ch);
    } else {
        clear_quest(ch);
        if (QST_QUIT(rnum) && (str_cmp(QST_QUIT(rnum), "undefined") != 0))
            send_to_char(ch, "%s", QST_QUIT(rnum));
        else
            send_to_char(ch, "Você não faz mais parte desta busca.\r\n");
        if (QST_PENALTY(rnum)) {
            GET_QUESTPOINTS(ch) -= QST_PENALTY(rnum);
            send_to_char(ch, "Você paga %d pontos de busca por ter abandonando esta busca!\r\n", QST_PENALTY(rnum));
        }
        save_char(ch);
    }
}

/* Unified quest display function - shows both regular and temporary quests */
static void quest_show_unified(struct char_data *ch, struct char_data *qm)
{
    qst_rnum rnum;
    int counter = 0, i;
    int quest_completed, quest_repeatable;
    mob_vnum qm_vnum = GET_MOB_VNUM(qm);

    send_to_char(ch,
                 "A lista de buscas: disponiveis é:\r\n"
                 "Num.  Descrção                                             Feita?\r\n"
                 "----- ---------------------------------------------------- ------\r\n");

    /* First, show regular quests assigned to this questmaster */
    for (rnum = 0; rnum < total_quests; rnum++) {
        if (qm_vnum == QST_MASTER(rnum)) {
            quest_completed = is_complete(ch, QST_NUM(rnum));
            quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

            /* Only show quest if not completed or repeatable */
            if (!quest_completed || quest_repeatable) {
                send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty(%s)\tn\r\n", ++counter, QST_DESC(rnum),
                             (quest_completed ? "Sim" : "Não "));
            }
        }
    }

    /* Then, show temporary quests if this mob is a temporary questmaster */
    if (IS_TEMP_QUESTMASTER(qm) && GET_NUM_TEMP_QUESTS(qm) > 0) {
        for (i = 0; i < GET_NUM_TEMP_QUESTS(qm); i++) {
            rnum = real_quest(GET_TEMP_QUESTS(qm)[i]);
            if (rnum == NOTHING)
                continue;

            quest_completed = is_complete(ch, QST_NUM(rnum));
            quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

            /* Only show quest if not completed or repeatable */
            if (!quest_completed || quest_repeatable) {
                send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty(%s)\tn\r\n", ++counter, QST_DESC(rnum),
                             (quest_completed ? "Sim" : "Não "));
            }
        }
    }

    if (!counter) {
        send_to_char(ch, "Não temos buscas disponiveis no momento, %s!\r\n", GET_NAME(ch));

        /* Debug information for immortals */
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            int total_regular = 0, total_temp = 0;

            /* Count regular quests for this questmaster */
            for (rnum = 0; rnum < total_quests; rnum++) {
                if (qm_vnum == QST_MASTER(rnum)) {
                    total_regular++;
                }
            }

            /* Count temporary quests */
            if (IS_TEMP_QUESTMASTER(qm)) {
                total_temp = GET_NUM_TEMP_QUESTS(qm);
            }

            send_to_char(ch, "\tc[DEBUG: QM %d has %d regular quests, %d temp quests, is_temp_qm=%s]\tn\r\n", qm_vnum,
                         total_regular, total_temp, IS_TEMP_QUESTMASTER(qm) ? "YES" : "NO");
        }
    }
}

/* Unified quest join function - uses unified quest finding */
static void quest_join_unified(struct char_data *ch, struct char_data *qm, char argument[MAX_INPUT_LENGTH])
{
    qst_vnum vnum;
    qst_rnum rnum;
    char buf[MAX_INPUT_LENGTH];

    if (!*argument)
        snprintf(buf, sizeof(buf), "%s diz, 'Qual busca você quer aceitar, %s?'", GET_NAME(qm), GET_NAME(ch));
    else if (GET_QUEST(ch) != NOTHING)
        snprintf(buf, sizeof(buf), "%s diz, 'Mas você já tem uma busca ativa, %s!'", GET_NAME(qm), GET_NAME(ch));
    else if ((vnum = find_unified_quest_by_qmnum(ch, qm, atoi(argument))) == NOTHING)
        snprintf(buf, sizeof(buf), "%s diz, 'Eu não conheço nenhuma busca assim, %s!'", GET_NAME(qm), GET_NAME(ch));
    else if ((rnum = real_quest(vnum)) == NOTHING)
        snprintf(buf, sizeof(buf), "%s diz, 'Eu não conheço essa busca, %s!'", GET_NAME(qm), GET_NAME(ch));
    else if (GET_LEVEL(ch) < QST_MINLEVEL(rnum))
        snprintf(buf, sizeof(buf), "%s diz, 'Sinto muito, mas você ainda não pode participar desta busca, %s!'",
                 GET_NAME(qm), GET_NAME(ch));
    else if (GET_LEVEL(ch) > QST_MAXLEVEL(rnum))
        snprintf(buf, sizeof(buf), "%s diz, 'Sinto muito, mas você tem muita experiência para aceitar esta busca, %s!'",
                 GET_NAME(qm), GET_NAME(ch));
    else if (is_complete(ch, vnum))
        snprintf(buf, sizeof(buf), "%s diz, 'Você já completou esta busca antes, %s!'", GET_NAME(qm), GET_NAME(ch));
    else if ((QST_PREV(rnum) != NOTHING) && !is_complete(ch, QST_PREV(rnum)))
        snprintf(buf, sizeof(buf), "%s diz, 'Você precisa completar outra busca antes, %s!'", GET_NAME(qm),
                 GET_NAME(ch));
    else if ((QST_PREREQ(rnum) != NOTHING) && (real_object(QST_PREREQ(rnum)) != NOTHING) &&
             (get_obj_in_list_num(real_object(QST_PREREQ(rnum)), ch->carrying) == NULL))
        snprintf(buf, sizeof(buf), "%s diz, 'Você precisa primeiro ter %s antes!'", GET_NAME(qm),
                 obj_proto[real_object(QST_PREREQ(rnum))].short_description);
    else {
        act("Você aceitou a busca.", TRUE, ch, NULL, NULL, TO_CHAR);
        act("$n aceitou uma busca.", TRUE, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "%s diz, 'As instruções para esta busca são:'\r\n", GET_NAME(qm));
        set_quest(ch, rnum);
        send_to_char(ch, "%s", QST_INFO(rnum));
        if (QST_TIME(rnum) != -1) {
            send_to_char(ch, "%s diz, 'Você tem um tempo limite de %d tick%s para completar esta busca!'\r\n",
                         GET_NAME(qm), QST_TIME(rnum), QST_TIME(rnum) == 1 ? "" : "s");
        } else {
            send_to_char(ch, "%s diz, 'Você pode levar o tempo que precisar para completar esta busca.'\r\n",
                         GET_NAME(qm));
        }
        save_char(ch);
        return;
    }
    send_to_char(ch, "%s\r\n", buf);
    save_char(ch);
}

static void quest_progress(struct char_data *ch)
{
    qst_rnum rnum;

    if (GET_QUEST(ch) == NOTHING)
        send_to_char(ch, "Mas você não está em uma busca no momento!\r\n");
    else if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING) {
        clear_quest(ch);
        send_to_char(ch, "A busca foi cancelada e não existe mais!\r\n");
    } else {
        send_to_char(ch, "Você aceitou as seguintes buscas:\r\n%s\r\n%s", QST_DESC(rnum), QST_INFO(rnum));
        if (QST_QUANTITY(rnum) > 1)
            send_to_char(ch, "Você ainda precisa realizar %d objetivo%s da busca.\r\n", GET_QUEST_COUNTER(ch),
                         GET_QUEST_COUNTER(ch) == 1 ? "" : "s");
        if (GET_QUEST_TIME(ch) > 0)
            send_to_char(ch, "Você ainda tem %d tick%s restando para concluir a busca.\r\n", GET_QUEST_TIME(ch),
                         GET_QUEST_TIME(ch) == 1 ? "" : "s");
    }
}

/*Para imortais */
static void quest_stat(struct char_data *ch, char *argument)
{
    qst_rnum rnum;
    mob_rnum qmrnum;
    char buf[MAX_STRING_LENGTH];
    char targetname[MAX_STRING_LENGTH];
    qst_vnum quest_vnum;
    int quest_num;

    if (!*argument) {
        send_to_char(ch, "%s\r\n", quest_imm_usage);
        return;
    }

    quest_num = atoi(argument);

    /* First try to find by vnum (direct vnum lookup) */
    if ((rnum = real_quest(quest_num)) != NOTHING) {
        quest_vnum = quest_num;
    }
    /* If not found by vnum, try to find by list position */
    else if ((quest_vnum = find_quest_by_listnum(quest_num)) != NOTHING && (rnum = real_quest(quest_vnum)) != NOTHING) {
        /* Found by list position */
    } else {
        send_to_char(ch, "That quest does not exist.\r\n");
        return;
    }

    sprintbit(QST_FLAGS(rnum), aq_flags, buf, sizeof(buf));
    switch (QST_TYPE(rnum)) {
        case AQ_OBJ_FIND:
        case AQ_OBJ_RETURN:
            snprintf(targetname, sizeof(targetname), "%s",
                     real_object(QST_TARGET(rnum)) == NOTHING
                         ? "An unknown object"
                         : obj_proto[real_object(QST_TARGET(rnum))].short_description);
            break;
        case AQ_ROOM_FIND:
        case AQ_ROOM_CLEAR:
            snprintf(targetname, sizeof(targetname), "%s",
                     real_room(QST_TARGET(rnum)) == NOWHERE ? "An unknown room"
                                                            : world[real_room(QST_TARGET(rnum))].name);
            break;
        case AQ_MOB_FIND:
        case AQ_MOB_KILL:
        case AQ_MOB_SAVE:
            snprintf(targetname, sizeof(targetname), "%s",
                     real_mobile(QST_TARGET(rnum)) == NOBODY ? "An unknown mobile"
                                                             : GET_NAME(&mob_proto[real_mobile(QST_TARGET(rnum))]));
            break;
        default:
            snprintf(targetname, sizeof(targetname), "Unknown");
            break;
    }
    qmrnum = real_mobile(QST_MASTER(rnum));
    send_to_char(ch,
                 "VNum  : [\ty%5d\tn], RNum: [\ty%5d\tn] -- Questmaster: [\ty%5d\tn] \ty%s\tn\r\n"
                 "Name  : \ty%s\tn\r\n"
                 "Desc  : \ty%s\tn\r\n"
                 "Accept Message:\r\n\tc%s\tn"
                 "Completion Message:\r\n\tc%s\tn"
                 "Quit Message:\r\n\tc%s\tn"
                 "Type  : \ty%s\tn\r\n"
                 "Target: \ty%d\tn \ty%s\tn, Quantity: \ty%d\tn\r\n"
                 "Value : \ty%d\tn, Penalty: \ty%d\tn, Min Level: \ty%2d\tn, Max Level: \ty%2d\tn\r\n"
                 "Flags : \tc%s\tn\r\n",
                 QST_NUM(rnum), rnum, QST_MASTER(rnum) == NOBODY ? -1 : QST_MASTER(rnum),
                 (qmrnum == NOBODY) ? "(Invalid vnum)" : GET_NAME(&mob_proto[(qmrnum)]), QST_NAME(rnum), QST_DESC(rnum),
                 QST_INFO(rnum), QST_DONE(rnum),
                 (QST_QUIT(rnum) && (str_cmp(QST_QUIT(rnum), "undefined") != 0) ? QST_QUIT(rnum) : "Nothing\r\n"),
                 quest_types[QST_TYPE(rnum)], QST_TARGET(rnum) == NOBODY ? -1 : QST_TARGET(rnum), targetname,
                 QST_QUANTITY(rnum), QST_POINTS(rnum), QST_PENALTY(rnum), QST_MINLEVEL(rnum), QST_MAXLEVEL(rnum), buf);
    if (QST_PREREQ(rnum) != NOTHING)
        send_to_char(ch, "Preq  : [\ty%5d\tn] \ty%s\tn\r\n", QST_PREREQ(rnum) == NOTHING ? -1 : QST_PREREQ(rnum),
                     QST_PREREQ(rnum) == NOTHING ? ""
                     : real_object(QST_PREREQ(rnum)) == NOTHING
                         ? "an unknown object"
                         : obj_proto[real_object(QST_PREREQ(rnum))].short_description);
    if (QST_TYPE(rnum) == AQ_OBJ_RETURN)
        send_to_char(ch, "Mob   : [\ty%5d\tn] \ty%s\tn\r\n", QST_RETURNMOB(rnum),
                     real_mobile(QST_RETURNMOB(rnum)) == NOBODY
                         ? "an unknown mob"
                         : mob_proto[real_mobile(QST_RETURNMOB(rnum))].player.short_descr);
    if (QST_TIME(rnum) != -1)
        send_to_char(ch, "Limit : There is a time limit of %d turn%s to complete.\r\n", QST_TIME(rnum),
                     QST_TIME(rnum) == 1 ? "" : "s");
    else
        send_to_char(ch, "Limit : There is no time limit on this quest.\r\n");
    send_to_char(ch, "Prior :");
    if (QST_PREV(rnum) == NOTHING)
        send_to_char(ch, " \tyNone.\tn\r\n");
    else
        send_to_char(ch, " [\ty%5d\tn] \tc%s\tn\r\n", QST_PREV(rnum), QST_DESC(real_quest(QST_PREV(rnum))));
    send_to_char(ch, "Next  :");
    if (QST_NEXT(rnum) == NOTHING)
        send_to_char(ch, " \tyNone.\tn\r\n");
    else
        send_to_char(ch, " [\ty%5d\tn] \tc%s\tn\r\n", QST_NEXT(rnum), QST_DESC(real_quest(QST_NEXT(rnum))));
}

/*--------------------------------------------------------------------------*/
/* Quest Command Processing Function and Questmaster Special                */
/*--------------------------------------------------------------------------*/

ACMD(do_quest)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int tp;

    two_arguments(argument, arg1, arg2);
    if (!*arg1)
        send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ? quest_mort_usage : quest_imm_usage);
    else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
        send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ? quest_mort_usage : quest_imm_usage);
    else {
        switch (tp) {
            case SCMD_QUEST_LIST:
            case SCMD_QUEST_JOIN:
                /* list, join should hve been handled by questmaster spec proc */
                send_to_char(ch, "Desculpe, mas você não oode fazer isto aqui!\r\n");
                break;
            case SCMD_QUEST_HISTORY:
                quest_hist(ch);
                break;
            case SCMD_QUEST_LEAVE:
                quest_quit(ch);
                break;
            case SCMD_QUEST_PROGRESS:
                quest_progress(ch);
                break;
            case SCMD_QUEST_STATUS:
                if (GET_LEVEL(ch) < LVL_IMMORT)
                    send_to_char(ch, "%s\r\n", quest_mort_usage);
                else
                    quest_stat(ch, arg2);
                break;
            default: /* Whe should never get here, but... */
                send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ? quest_mort_usage : quest_imm_usage);
                break;
        } /* switch on subcmd number */
    }
}

SPECIAL(questmaster)
{
    qst_rnum rnum;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int tp;
    struct char_data *qm = (struct char_data *)me;
    bool has_regular_quests = FALSE;
    bool has_temp_quests = FALSE;

    /* Check if this mob has regular quests assigned */
    for (rnum = 0; rnum < total_quests; rnum++) {
        if (QST_MASTER(rnum) == GET_MOB_VNUM(qm)) {
            has_regular_quests = TRUE;
            break;
        }
    }

    /* Check if this mob has temporary quests */
    if (IS_TEMP_QUESTMASTER(qm) && GET_NUM_TEMP_QUESTS(qm) > 0) {
        has_temp_quests = TRUE;
    }

    /* If no quests at all, not a questmaster */
    if (!has_regular_quests && !has_temp_quests)
        return FALSE;

    /* Handle secondary spec procs for regular quests */
    if (has_regular_quests) {
        for (rnum = 0; rnum < total_quests; rnum++) {
            if (QST_MASTER(rnum) == GET_MOB_VNUM(qm)) {
                if (QST_FUNC(rnum) && (QST_FUNC(rnum)(ch, me, cmd, argument)))
                    return TRUE; /* The secondary spec proc handled this command */
                break;           /* Only check first matching quest for secondary spec proc */
            }
        }
    }

    if (CMD_IS("quest")) {
        two_arguments(argument, arg1, arg2);
        if (!*arg1)
            return FALSE;
        else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
            return FALSE;
        else {
            switch (tp) {
                case SCMD_QUEST_LIST:
                    if (!*arg2)
                        quest_show_unified(ch, qm);
                    else
                        quest_list(ch, qm, arg2);
                    break;
                case SCMD_QUEST_JOIN:
                    quest_join_unified(ch, qm, arg2);
                    break;
                default:
                    return FALSE; /* fall through to the do_quest command processor */
            } /* switch on subcmd number */
            return TRUE;
        }
    } else {
        return FALSE; /* not a questmaster command */
    }
}

/*--------------------------------------------------------------------------*/
/* Mob Quest Functions                                                      */
/*--------------------------------------------------------------------------*/

/* Calculate mob's capability to handle a quest based on stats and level */
int calculate_mob_quest_capability(struct char_data *mob, qst_rnum rnum)
{
    int capability = 0;
    int level_diff;

    if (!IS_NPC(mob) || !mob->ai_data || rnum == NOTHING)
        return 0;

    /* Base capability from quest genetics */
    capability = GET_GENQUEST(mob) + GET_GENADVENTURER(mob);

    /* Level consideration */
    level_diff = GET_LEVEL(mob) - QST_MINLEVEL(rnum);
    if (level_diff < 0)
        capability -= (level_diff * -10); /* Penalty for being too low level */
    else if (level_diff > 10)
        capability += 20; /* Bonus for being high level */

    /* Reputation consideration */
    capability += GET_MOB_REPUTATION(mob);

    /* Adjust based on quest difficulty */
    switch (QST_TYPE(rnum)) {
        case AQ_MOB_KILL:
        case AQ_MOB_KILL_BOUNTY:
        case AQ_PLAYER_KILL:
            capability += GET_GENBRAVE(mob); /* Combat quests need bravery */
            break;
        case AQ_OBJ_FIND:
        case AQ_ROOM_FIND:
            capability += GET_GENROAM(mob); /* Exploration quests need roaming */
            break;
        case AQ_ROOM_CLEAR:
            capability += (GET_GENBRAVE(mob) + GET_GENGROUP(mob)) / 2; /* Need both */
            break;
    }

    return MAX(0, MIN(100, capability));
}

/* Check if a mob should accept a quest */
bool mob_should_accept_quest(struct char_data *mob, qst_rnum rnum)
{
    int capability, chance;

    if (!IS_NPC(mob) || !mob->ai_data || rnum == NOTHING)
        return FALSE;

    /* Don't accept if already on a quest */
    if (GET_MOB_QUEST(mob) != NOTHING)
        return FALSE;

    /* Don't accept immortal quests (only mob-posted quests) */
    if (!IS_SET(QST_FLAGS(rnum), AQ_MOB_POSTED))
        return FALSE;

    /* Calculate capability */
    capability = calculate_mob_quest_capability(mob, rnum);

    /* Random chance based on capability */
    chance = rand() % 100;

    return (chance < capability);
}

/* Set a quest for a mob */
void set_mob_quest(struct char_data *mob, qst_rnum rnum)
{
    if (!IS_NPC(mob) || !mob->ai_data || rnum == NOTHING)
        return;

    mob->ai_data->current_quest = QST_NUM(rnum);
    mob->ai_data->quest_timer = QST_TIME(rnum);
    mob->ai_data->quest_counter = QST_QUANTITY(rnum) > 0 ? QST_QUANTITY(rnum) : 1;
}

/* Clear a quest from a mob */
void clear_mob_quest(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    mob->ai_data->current_quest = NOTHING;
    mob->ai_data->quest_timer = 0;
    mob->ai_data->quest_counter = 0;
}

/* Clear a quest from a mob with failure penalty */
void fail_mob_quest(struct char_data *mob, const char *reason)
{
    qst_rnum rnum;
    qst_vnum vnum;

    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    vnum = GET_MOB_QUEST(mob);
    if (vnum == NOTHING) {
        clear_mob_quest(mob);
        return;
    }

    rnum = real_quest(vnum);
    if (rnum == NOTHING) {
        clear_mob_quest(mob);
        return;
    }

    /* Apply genetics penalties based on quest type */
    switch (QST_TYPE(rnum)) {
        case AQ_OBJ_FIND:
        case AQ_ROOM_FIND:
        case AQ_MOB_FIND:
            /* Exploration quest failures decrease adventurer_tendency */
            if (mob->ai_data->genetics.adventurer_tendency > 0) {
                mob->ai_data->genetics.adventurer_tendency =
                    MAX(0, mob->ai_data->genetics.adventurer_tendency - rand() % 3 - 1);
            }
            break;
        case AQ_MOB_KILL:
        case AQ_MOB_KILL_BOUNTY:
        case AQ_PLAYER_KILL:
        case AQ_MOB_SAVE:
        case AQ_ROOM_CLEAR:
        case AQ_OBJ_RETURN:
            /* All other quest failures decrease quest_tendency */
            if (mob->ai_data->genetics.quest_tendency > 0) {
                mob->ai_data->genetics.quest_tendency = MAX(0, mob->ai_data->genetics.quest_tendency - rand() % 2 - 1);
            }
            break;
    }

    /* Decrease reputation slightly for failing */
    if (mob->ai_data->reputation > 0) {
        mob->ai_data->reputation = MAX(0, mob->ai_data->reputation - rand() % 5 - 1);
    }

    /* Log the failure */
    log1("QUEST FAILURE: %s failed quest %d (%s)", GET_NAME(mob), vnum, reason);

    /* Clear the quest */
    clear_mob_quest(mob);

    act("$n parece desapontado.", TRUE, mob, 0, 0, TO_ROOM);
}

/* Mob completes a quest and gets rewards */
void mob_complete_quest(struct char_data *mob)
{
    qst_rnum rnum;
    qst_vnum vnum;
    struct obj_data *new_obj;

    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    vnum = GET_MOB_QUEST(mob);
    if (vnum == NOTHING)
        return;

    rnum = real_quest(vnum);
    if (rnum == NOTHING)
        return;

    /* Give gold reward */
    if (QST_GOLD(rnum)) {
        increase_gold(mob, QST_GOLD(rnum));
    }

    /* Give object reward */
    if (QST_OBJ(rnum) != NOTHING) {
        if (real_object(QST_OBJ(rnum)) != NOTHING) {
            new_obj = read_object(QST_OBJ(rnum), VIRTUAL);
            if (new_obj) {
                obj_to_char(new_obj, mob);
            }
        }
    }

    /* Increase reputation */
    if (mob->ai_data->reputation < 100) {
        mob->ai_data->reputation = MIN(100, mob->ai_data->reputation + rand() % 10 + 1);
    }

    /* Increase genetics based on quest type */
    switch (QST_TYPE(rnum)) {
        case AQ_OBJ_FIND:
        case AQ_ROOM_FIND:
        case AQ_MOB_FIND:
            /* Exploration quests increase adventurer_tendency */
            if (mob->ai_data->genetics.adventurer_tendency < 100) {
                mob->ai_data->genetics.adventurer_tendency =
                    MIN(100, mob->ai_data->genetics.adventurer_tendency + rand() % 3 + 1);
            }
            break;
        case AQ_MOB_KILL:
        case AQ_MOB_KILL_BOUNTY:
        case AQ_PLAYER_KILL:
        case AQ_MOB_SAVE:
        case AQ_ROOM_CLEAR:
            /* Combat/protection quests increase quest_tendency moderately */
            if (mob->ai_data->genetics.quest_tendency < 100) {
                mob->ai_data->genetics.quest_tendency =
                    MIN(100, mob->ai_data->genetics.quest_tendency + rand() % 2 + 1);
            }
            break;
        case AQ_OBJ_RETURN:
            /* Return quests increase both genetics slightly */
            if (mob->ai_data->genetics.quest_tendency < 100) {
                mob->ai_data->genetics.quest_tendency = MIN(100, mob->ai_data->genetics.quest_tendency + 1);
            }
            if (mob->ai_data->genetics.adventurer_tendency < 100) {
                mob->ai_data->genetics.adventurer_tendency = MIN(100, mob->ai_data->genetics.adventurer_tendency + 1);
            }
            break;
    }

    /* Clear the quest */
    clear_mob_quest(mob);

    act("$n parece satisfeito com sua tarefa concluída.", TRUE, mob, 0, 0, TO_ROOM);
}

/* Check mob quest completion triggers */
void mob_autoquest_trigger_check(struct char_data *ch, struct char_data *vict, struct obj_data *object, int type)
{
    qst_rnum rnum;
    qst_vnum vnum;
    struct char_data *i;
    bool found = TRUE;

    if (!IS_NPC(ch) || !ch->ai_data)
        return;

    vnum = GET_MOB_QUEST(ch);
    if (vnum == NOTHING)
        return;

    rnum = real_quest(vnum);
    if (rnum == NOTHING)
        return;

    switch (QST_TYPE(rnum)) {
        case AQ_MOB_KILL:
        case AQ_MOB_KILL_BOUNTY:
            if (IS_NPC(vict) && (ch != vict))
                if (QST_TARGET(rnum) == GET_MOB_VNUM(vict)) {
                    if (--ch->ai_data->quest_counter <= 0)
                        mob_complete_quest(ch);
                }
            break;
        case AQ_PLAYER_KILL:
            if (!IS_NPC(vict) && (ch != vict)) {
                if (--ch->ai_data->quest_counter <= 0)
                    mob_complete_quest(ch);
            }
            break;
        case AQ_OBJ_FIND:
            if (object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum))) {
                if (--ch->ai_data->quest_counter <= 0)
                    mob_complete_quest(ch);
            }
            break;
        case AQ_ROOM_FIND:
            if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number) {
                if (--ch->ai_data->quest_counter <= 0)
                    mob_complete_quest(ch);
            }
            break;
        case AQ_MOB_FIND:
            for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
                if (IS_NPC(i))
                    if (QST_TARGET(rnum) == GET_MOB_VNUM(i)) {
                        if (--ch->ai_data->quest_counter <= 0)
                            mob_complete_quest(ch);
                    }
            break;
        case AQ_MOB_SAVE:
            /* Mob save quests complete when the target mob is in the same room and healthy */
            for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
                if (IS_NPC(i))
                    if (QST_TARGET(rnum) == GET_MOB_VNUM(i)) {
                        /* Check if the mob is "saved" (healthy and not in combat) */
                        if (GET_HIT(i) > GET_MAX_HIT(i) * 0.8 && !FIGHTING(i)) {
                            if (--ch->ai_data->quest_counter <= 0)
                                mob_complete_quest(ch);
                        }
                    }
            break;
        case AQ_ROOM_CLEAR:
            if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number) {
                for (i = world[IN_ROOM(ch)].people; i && found; i = i->next_in_room)
                    if (i && IS_NPC(i) && !MOB_FLAGGED(i, MOB_NOTDEADYET) && i != ch)
                        found = FALSE;
                if (found) {
                    if (--ch->ai_data->quest_counter <= 0)
                        mob_complete_quest(ch);
                }
            }
            break;
    }
}

/*--------------------------------------------------------------------------*/
/* Temporary Quest Master Functions                                        */
/*--------------------------------------------------------------------------*/

/* Initialize a mob as a temporary quest master */
void init_temp_questmaster(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    if (!IS_TEMP_QUESTMASTER(mob)) {
        mob->ai_data->is_temp_questmaster = TRUE;
        mob->ai_data->temp_quests = NULL;
        mob->ai_data->num_temp_quests = 0;
        mob->ai_data->max_temp_quests = 10; /* Maximum 10 temporary quests per mob */
    }
}

/* Clear temporary quest master status and free associated memory */
void clear_temp_questmaster(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    if (IS_TEMP_QUESTMASTER(mob)) {
        if (mob->ai_data->temp_quests) {
            free(mob->ai_data->temp_quests);
            mob->ai_data->temp_quests = NULL;
        }
        mob->ai_data->is_temp_questmaster = FALSE;
        mob->ai_data->num_temp_quests = 0;
        mob->ai_data->max_temp_quests = 0;
    }
}

/* Add a quest to a mob's temporary quest list */
bool add_temp_quest_to_mob(struct char_data *mob, qst_vnum quest_vnum)
{
    qst_vnum *new_temp_quests;
    int i;

    if (!IS_NPC(mob) || !mob->ai_data || quest_vnum == NOTHING)
        return FALSE;

    /* Initialize as temp quest master if not already */
    if (!IS_TEMP_QUESTMASTER(mob))
        init_temp_questmaster(mob);

    /* Check if quest is already in the list */
    for (i = 0; i < GET_NUM_TEMP_QUESTS(mob); i++) {
        if (GET_TEMP_QUESTS(mob)[i] == quest_vnum)
            return TRUE; /* Already have this quest */
    }

    /* Check if we have space */
    if (GET_NUM_TEMP_QUESTS(mob) >= GET_MAX_TEMP_QUESTS(mob)) {
        log1("QUEST: Mob %s cannot accept more temporary quests (max %d)", GET_NAME(mob), GET_MAX_TEMP_QUESTS(mob));
        return FALSE;
    }

    /* Allocate or reallocate memory */
    if (GET_TEMP_QUESTS(mob) == NULL) {
        new_temp_quests = malloc(sizeof(qst_vnum) * GET_MAX_TEMP_QUESTS(mob));
    } else {
        new_temp_quests = realloc(GET_TEMP_QUESTS(mob), sizeof(qst_vnum) * GET_MAX_TEMP_QUESTS(mob));
    }

    if (!new_temp_quests) {
        log1("SYSERR: add_temp_quest_to_mob: Memory allocation failed");
        return FALSE;
    }

    mob->ai_data->temp_quests = new_temp_quests;
    mob->ai_data->temp_quests[mob->ai_data->num_temp_quests] = quest_vnum;
    mob->ai_data->num_temp_quests++;

    log1("QUEST: Added temporary quest %d to mob %s", quest_vnum, GET_NAME(mob));
    return TRUE;
}

/* Remove a quest from a mob's temporary quest list */
bool remove_temp_quest_from_mob(struct char_data *mob, qst_vnum quest_vnum)
{
    int i, j;

    if (!IS_NPC(mob) || !mob->ai_data || !IS_TEMP_QUESTMASTER(mob) || quest_vnum == NOTHING)
        return FALSE;

    /* Find and remove the quest */
    for (i = 0; i < GET_NUM_TEMP_QUESTS(mob); i++) {
        if (GET_TEMP_QUESTS(mob)[i] == quest_vnum) {
            /* Shift remaining quests down */
            for (j = i; j < GET_NUM_TEMP_QUESTS(mob) - 1; j++) {
                GET_TEMP_QUESTS(mob)[j] = GET_TEMP_QUESTS(mob)[j + 1];
            }
            mob->ai_data->num_temp_quests--;

            /* If no more temporary quests, clear temp quest master status */
            if (GET_NUM_TEMP_QUESTS(mob) == 0) {
                clear_temp_questmaster(mob);
            }

            log1("QUEST: Removed temporary quest %d from mob %s", quest_vnum, GET_NAME(mob));
            return TRUE;
        }
    }

    return FALSE; /* Quest not found */
}

/* Find quest by list position number for a temporary quest master */
qst_vnum find_temp_quest_by_qmnum(struct char_data *ch, struct char_data *qm, int num)
{
    int counter = 0, i;
    qst_rnum rnum;
    int quest_completed, quest_repeatable;

    if (!IS_NPC(qm) || !IS_TEMP_QUESTMASTER(qm))
        return NOTHING;

    /* Iterate through the mob's temporary quests */
    for (i = 0; i < GET_NUM_TEMP_QUESTS(qm); i++) {
        rnum = real_quest(GET_TEMP_QUESTS(qm)[i]);
        if (rnum == NOTHING)
            continue;

        quest_completed = is_complete(ch, QST_NUM(rnum));
        quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

        /* Only count quest if it's available (not completed or repeatable) */
        if (!quest_completed || quest_repeatable) {
            if (++counter == num)
                return GET_TEMP_QUESTS(qm)[i];
        }
    }

    return NOTHING;
}

/* Check if a mob can reach the questmaster for a specific quest */
bool mob_can_reach_questmaster(struct char_data *mob, mob_vnum qm_vnum)
{
    struct char_data *qm;
    room_rnum qm_room, mob_room;

    if (!IS_NPC(mob) || qm_vnum == NOTHING)
        return FALSE;

    /* Find the questmaster in the world */
    for (qm = character_list; qm; qm = qm->next) {
        if (IS_NPC(qm) && GET_MOB_VNUM(qm) == qm_vnum)
            break;
    }

    if (!qm)
        return FALSE; /* Questmaster not found in world */

    qm_room = IN_ROOM(qm);
    mob_room = IN_ROOM(mob);

    if (qm_room == NOWHERE || mob_room == NOWHERE)
        return FALSE;

    /* For simplicity, check if they're in the same zone */
    /* In a more sophisticated implementation, we could use pathfinding */
    if (world[qm_room].zone == world[mob_room].zone)
        return TRUE;

    return FALSE; /* Different zones, assume unreachable */
}

/* Make a mob a temporary questmaster if it can't reach the real questmaster */
void make_mob_temp_questmaster_if_needed(struct char_data *mob, qst_vnum quest_vnum)
{
    qst_rnum rnum;
    mob_vnum qm_vnum;

    if (!IS_NPC(mob) || !mob->ai_data || quest_vnum == NOTHING)
        return;

    rnum = real_quest(quest_vnum);
    if (rnum == NOTHING)
        return;

    qm_vnum = QST_MASTER(rnum);

    /*
     * If the mob was actively trying to reach a questmaster (GOAL_GOTO_QUESTMASTER),
     * it means it successfully reached one and posted the quest there.
     * In this case, we should NOT make it a temporary questmaster.
     */
    if (mob->ai_data->current_goal == GOAL_GOTO_QUESTMASTER) {
        log1("QUEST: Mob %s successfully posted quest %d with questmaster via GOAL system", GET_NAME(mob), quest_vnum);
        return;
    }

    /*
     * Check if mob can reach the questmaster at all.
     * This function is called when the mob posted a quest locally (not via GOAL movement),
     * so we need to determine if it should become a temporary questmaster.
     */
    if (!mob_can_reach_questmaster(mob, qm_vnum)) {
        /* Mob can't reach questmaster, make it a temporary questmaster */
        add_temp_quest_to_mob(mob, quest_vnum);

        act("$n parece ter algo importante para dizer.", TRUE, mob, 0, 0, TO_ROOM);
        log1("QUEST: Mob %s became temporary questmaster for quest %d (can't reach QM %d)", GET_NAME(mob), quest_vnum,
             qm_vnum);
    } else {
        /*
         * Mob can reach questmaster - the quest should be available through the permanent questmaster.
         * However, if the permanent questmaster is in the same room and the mob successfully
         * delivered the quest, we should ensure the quest is visible immediately.
         *
         * Find the questmaster and ensure they have the questmaster special procedure assigned.
         */
        struct char_data *qm = NULL;
        mob_rnum qm_rnum = real_mobile(qm_vnum);

        if (qm_rnum != NOBODY) {
            /* Ensure the questmaster prototype has the correct special procedure */
            if (mob_index[qm_rnum].func != questmaster) {
                mob_index[qm_rnum].func = questmaster;
                log1("QUEST: Fixed questmaster special procedure for mob prototype %d", qm_vnum);
            }

            /* Find the questmaster in the world and ensure instance also has correct procedure */
            for (qm = character_list; qm; qm = qm->next) {
                if (IS_NPC(qm) && GET_MOB_VNUM(qm) == qm_vnum) {
                    /* Double-check that this specific instance has the questmaster function */
                    if (mob_index[GET_MOB_RNUM(qm)].func != questmaster) {
                        mob_index[GET_MOB_RNUM(qm)].func = questmaster;
                        log1("QUEST: Fixed questmaster special procedure for mob instance %s (%d)", GET_NAME(qm),
                             qm_vnum);
                    }
                    break;
                }
            }

            if (qm) {
                log1("QUEST: Mob %s posted quest %d to questmaster %s (%d) - quest should be immediately available",
                     GET_NAME(mob), quest_vnum, GET_NAME(qm), qm_vnum);
            } else {
                log1(
                    "QUEST: Mob %s posted quest %d to questmaster %d (not found in world) - quest assigned to "
                    "prototype",
                    GET_NAME(mob), quest_vnum, qm_vnum);
            }
        } else {
            log1("QUEST: WARNING - Mob %s posted quest %d to invalid questmaster %d", GET_NAME(mob), quest_vnum,
                 qm_vnum);
        }
    }
}

/* Temporary questmaster special function - handles quest interactions for temporary quest masters */
SPECIAL(temp_questmaster)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int tp;
    struct char_data *qm = (struct char_data *)me;

    /* Only handle if this mob is a temporary questmaster */
    if (!IS_TEMP_QUESTMASTER(qm) || GET_NUM_TEMP_QUESTS(qm) == 0)
        return FALSE;

    if (CMD_IS("quest")) {
        two_arguments(argument, arg1, arg2);
        if (!*arg1)
            return FALSE;
        else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
            return FALSE;
        else {
            switch (tp) {
                case SCMD_QUEST_LIST:
                    quest_show_temp(ch, qm);
                    break;
                case SCMD_QUEST_JOIN:
                    quest_join_temp(ch, qm, arg2);
                    break;
                default:
                    return FALSE; /* fall through to the do_quest command processor */
            } /* switch on subcmd number */
            return TRUE;
        }
    } else {
        return FALSE; /* not a questmaster command */
    }
}

/* Quest display function for temporary quest masters */
void quest_show_temp(struct char_data *ch, struct char_data *qm)
{
    qst_rnum rnum;
    int counter = 0, i;
    int quest_completed, quest_repeatable;

    if (!IS_NPC(qm) || !IS_TEMP_QUESTMASTER(qm)) {
        send_to_char(ch, "Este mob não é um mestre de buscas temporário.\r\n");
        return;
    }

    send_to_char(ch,
                 "Buscas temporárias disponíveis:\r\n"
                 "Num.  Descrição                                            Feita?\r\n"
                 "----- ---------------------------------------------------- ------\r\n");

    /* Iterate through the mob's temporary quests */
    for (i = 0; i < GET_NUM_TEMP_QUESTS(qm); i++) {
        rnum = real_quest(GET_TEMP_QUESTS(qm)[i]);
        if (rnum == NOTHING)
            continue;

        quest_completed = is_complete(ch, QST_NUM(rnum));
        quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

        /* Only show quest if not completed or repeatable */
        if (!quest_completed || quest_repeatable) {
            send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty(%s)\tn\r\n", ++counter, QST_DESC(rnum),
                         (quest_completed ? "Sim" : "Não "));
        }
    }

    if (!counter)
        send_to_char(ch, "Não tenho buscas disponíveis no momento, %s!\r\n", GET_NAME(ch));
}

/* Quest join function for temporary quest masters */
void quest_join_temp(struct char_data *ch, struct char_data *qm, char *arg)
{
    qst_vnum vnum;
    qst_rnum rnum;
    char buf[MAX_INPUT_LENGTH];

    if (!IS_NPC(qm) || !IS_TEMP_QUESTMASTER(qm)) {
        send_to_char(ch, "Este mob não é um mestre de buscas temporário.\r\n");
        return;
    }

    if (!*arg)
        snprintf(buf, sizeof(buf), "%s diz, 'Qual busca você quer aceitar, %s?'", GET_NAME(qm), GET_NAME(ch));
    else if (GET_QUEST(ch) != NOTHING)
        snprintf(buf, sizeof(buf), "%s diz, 'Mas você já tem uma busca ativa, %s!'", GET_NAME(qm), GET_NAME(ch));
    else if ((vnum = find_temp_quest_by_qmnum(ch, qm, atoi(arg))) == NOTHING)
        snprintf(buf, sizeof(buf), "%s diz, 'Eu não conheço nenhuma busca assim, %s!'", GET_NAME(qm), GET_NAME(ch));
    else if ((rnum = real_quest(vnum)) == NOTHING)
        snprintf(buf, sizeof(buf), "%s diz, 'Eu não conheço essa busca, %s!'", GET_NAME(qm), GET_NAME(ch));
    else if (GET_LEVEL(ch) < QST_MINLEVEL(rnum))
        snprintf(buf, sizeof(buf), "%s diz, 'Sinto muito, mas você ainda não pode participar desta busca, %s!'",
                 GET_NAME(qm), GET_NAME(ch));
    else if (GET_LEVEL(ch) > QST_MAXLEVEL(rnum))
        snprintf(buf, sizeof(buf), "%s diz, 'Sinto muito, mas você tem muita experiência para aceitar esta busca, %s!'",
                 GET_NAME(qm), GET_NAME(ch));
    else if (is_complete(ch, vnum))
        snprintf(buf, sizeof(buf), "%s diz, 'Você já completou esta busca antes, %s!'", GET_NAME(qm), GET_NAME(ch));
    else if ((QST_PREV(rnum) != NOTHING) && !is_complete(ch, QST_PREV(rnum)))
        snprintf(buf, sizeof(buf), "%s diz, 'Você precisa completar outra busca antes, %s!'", GET_NAME(qm),
                 GET_NAME(ch));
    else if ((QST_PREREQ(rnum) != NOTHING) && (real_object(QST_PREREQ(rnum)) != NOTHING) &&
             (get_obj_in_list_num(real_object(QST_PREREQ(rnum)), ch->carrying) == NULL))
        snprintf(buf, sizeof(buf), "%s diz, 'Você precisa primeiro ter %s antes!'", GET_NAME(qm),
                 obj_proto[real_object(QST_PREREQ(rnum))].short_description);
    else {
        act("Você aceitou a busca.", TRUE, ch, NULL, NULL, TO_CHAR);
        act("$n aceitou uma busca.", TRUE, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "%s diz, 'As instruções para esta busca são:'\r\n", GET_NAME(qm));
        set_quest(ch, rnum);
        send_to_char(ch, "%s", QST_INFO(rnum));
        if (QST_TIME(rnum) != -1) {
            send_to_char(ch, "%s diz, 'Você tem um tempo limite de %d tick%s para completar esta busca!'\r\n",
                         GET_NAME(qm), QST_TIME(rnum), QST_TIME(rnum) == 1 ? "" : "s");
        } else {
            send_to_char(ch, "%s diz, 'Você pode levar o tempo que precisar para completar esta busca.'\r\n",
                         GET_NAME(qm));
        }
        save_char(ch);
        return;
    }
    send_to_char(ch, "%s\r\n", buf);
    save_char(ch);
}

/*--------------------------------------------------------------------------*/
/* Save/Load Functions for Temporary Quest Assignments                     */
/*--------------------------------------------------------------------------*/

/* Initialize mob AI data with defaults for temporary quest master fields */
void init_mob_ai_data(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    mob->ai_data->is_temp_questmaster = FALSE;
    mob->ai_data->temp_quests = NULL;
    mob->ai_data->num_temp_quests = 0;
    mob->ai_data->max_temp_quests = 0;
}

/* Save temporary quest assignments to file */
void save_temp_quest_assignments(void)
{
    FILE *fp;
    struct char_data *mob;
    int i;

    if (!(fp = fopen("lib/etc/tempquests.dat", "w"))) {
        log1("SYSERR: Unable to open tempquests.dat for writing");
        return;
    }

    fprintf(fp, "# Temporary Quest Assignments Save File\n");
    fprintf(fp, "# Format: MOB_VNUM ROOM_VNUM NUM_QUESTS QUEST_VNUM1 QUEST_VNUM2 ...\n");

    /* Iterate through all mobs in the world */
    for (mob = character_list; mob; mob = mob->next) {
        if (IS_NPC(mob) && IS_TEMP_QUESTMASTER(mob) && GET_NUM_TEMP_QUESTS(mob) > 0) {
            fprintf(fp, "%d %d %d", GET_MOB_VNUM(mob), IN_ROOM(mob) != NOWHERE ? world[IN_ROOM(mob)].number : -1,
                    GET_NUM_TEMP_QUESTS(mob));

            for (i = 0; i < GET_NUM_TEMP_QUESTS(mob); i++) {
                fprintf(fp, " %d", GET_TEMP_QUESTS(mob)[i]);
            }
            fprintf(fp, "\n");
        }
    }

    fclose(fp);
    log1("Temporary quest assignments saved.");
}

/* Load temporary quest assignments from file */
void load_temp_quest_assignments(void)
{
    FILE *fp;
    char line[256];
    int mob_vnum, room_vnum, num_quests, quest_vnum;
    struct char_data *mob;
    room_rnum room_rnum;
    int i;

    if (!(fp = fopen("lib/etc/tempquests.dat", "r"))) {
        log1("No tempquests.dat file found - creating new file");
        /* Create a new empty tempquests.dat file */
        if ((fp = fopen("lib/etc/tempquests.dat", "w"))) {
            fprintf(fp, "# Temporary Quest Assignments Save File\n");
            fprintf(fp, "# Format: MOB_VNUM ROOM_VNUM NUM_QUESTS QUEST_VNUM1 QUEST_VNUM2 ...\n");
            fprintf(fp, "# This file is automatically generated and maintained by the game\n");
            fclose(fp);
            log1("Created empty tempquests.dat file");
        } else {
            log1("SYSERR: Unable to create tempquests.dat file");
        }
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        if (sscanf(line, "%d %d %d", &mob_vnum, &room_vnum, &num_quests) < 3)
            continue;

        /* Find the mob in the world */
        mob = NULL;
        for (mob = character_list; mob; mob = mob->next) {
            if (IS_NPC(mob) && GET_MOB_VNUM(mob) == mob_vnum) {
                /* If room_vnum is specified, check if mob is in the right room */
                if (room_vnum != -1) {
                    room_rnum = real_room(room_vnum);
                    if (room_rnum == NOWHERE || IN_ROOM(mob) != room_rnum)
                        continue; /* Wrong room, keep looking */
                }
                break; /* Found the right mob */
            }
        }

        if (!mob) {
            log1("TEMPQUEST: Mob %d not found for temporary quest assignment", mob_vnum);
            continue;
        }

        /* Read quest vnums and assign them to the mob */
        char *ptr = line;
        int scanned = 0;

        /* Skip the first three numbers we already read */
        sscanf(ptr, "%d %d %d%n", &mob_vnum, &room_vnum, &num_quests, &scanned);
        ptr += scanned;

        for (i = 0; i < num_quests; i++) {
            if (sscanf(ptr, "%d%n", &quest_vnum, &scanned) == 1) {
                add_temp_quest_to_mob(mob, quest_vnum);
                ptr += scanned;
            }
        }

        log1("TEMPQUEST: Loaded %d temporary quests for mob %s (%d)", GET_NUM_TEMP_QUESTS(mob), GET_NAME(mob),
             mob_vnum);
    }

    fclose(fp);
    log1("Temporary quest assignments loaded.");
}
