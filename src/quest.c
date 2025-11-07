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
#include <ctype.h>
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
#include "act.h"        /* for do_tell */
#include "dg_scripts.h" /* for char_script_id */

/*--------------------------------------------------------------------------
 * Exported global variables
 *--------------------------------------------------------------------------*/
const char *quest_types[] = {
    "Object",     "Room",        "Find mob",          "Kill mob",   "Save mob", "Return object",
    "Clear room", "Kill player", "Kill mob (bounty)", "Escort mob", "\n"};
const char *aq_flags[] = {"REPEATABLE", "MOB_POSTED", "\n"};

/*--------------------------------------------------------------------------
 * Local (file scope) global variables
 *--------------------------------------------------------------------------*/
static int cmd_tell;

static const char *quest_cmd[] = {"list", "history", "join", "leave", "progress", "status", "remove", "\n"};

static const char *quest_mort_usage = "Uso: quest list | history | progress | join <nn> | leave";

static const char *quest_imm_usage =
    "Uso: quest list | history | progress | join <nn> | leave | status <vnum> | remove <vnum> confirm";
static const char *quest_god_usage =
    "Uso: quest list | history | progress | join <nn> | leave | status <vnum> | remove <vnum> confirm";

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

/* Check if a quest's level requirements are appropriate for the character */
static int is_quest_level_available(struct char_data *ch, qst_rnum rnum)
{
    /* Safety check: invalid quest rnum */
    if (rnum == NOTHING || rnum < 0 || rnum >= total_quests)
        return FALSE;

    /* Immortals can see and join all quests regardless of level */
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return TRUE;

    /* For mortals, check if their level is within the quest's level range */
    if (GET_LEVEL(ch) < QST_MINLEVEL(rnum) || GET_LEVEL(ch) > QST_MAXLEVEL(rnum))
        return FALSE;

    return TRUE;
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
/* Quest Info Formatting Functions                                          */
/*--------------------------------------------------------------------------*/

/** Format quest info message, replacing room numbers with room names for room-related quests.
 * @param rnum The real quest number.
 * @param buf Output buffer to store the formatted message.
 * @param bufsize Size of the output buffer.
 * @return Pointer to the formatted message (buf). */
static char *format_quest_info(qst_rnum rnum, struct char_data *ch, char *buf, size_t bufsize)
{
    const char *info = QST_INFO(rnum);
    char temp_buf[MAX_QUEST_MSG];

    /* For AQ_ROOM_FIND and AQ_ROOM_CLEAR quests, replace room number with room name */
    if ((QST_TYPE(rnum) == AQ_ROOM_FIND || QST_TYPE(rnum) == AQ_ROOM_CLEAR) && QST_TARGET(rnum) != NOTHING) {
        room_rnum room_rnum_val = real_room(QST_TARGET(rnum));

        if (room_rnum_val != NOWHERE) {
            const char *room_name = world[room_rnum_val].name;
            char room_num_str[20];
            const char *pos;
            size_t room_num_len;

            /* Create string representation of room number to search for */
            snprintf(room_num_str, sizeof(room_num_str), "%d", QST_TARGET(rnum));
            room_num_len = strlen(room_num_str);

            /* Search for the room number in the info message
             * We search for it as a complete number by checking boundaries */
            pos = strstr(info, room_num_str);

            while (pos != NULL) {
                /* Check if this is a complete number match (not part of a larger number) */
                bool is_start_boundary = (pos == info || !isdigit((unsigned char)*(pos - 1)));
                bool is_end_boundary = !isdigit((unsigned char)*(pos + room_num_len));

                if (is_start_boundary && is_end_boundary) {
                    size_t prefix_len = pos - info;
                    size_t suffix_start = prefix_len + room_num_len;
                    size_t room_name_len = strlen(room_name);
                    size_t suffix_len = strlen(info + suffix_start);
                    size_t total_len = prefix_len + room_name_len + suffix_len;

                    /* Check if the formatted string will fit in the buffer (including null terminator) */
                    if (total_len + 1 <= bufsize) {
                        /* Build new message: prefix + room_name + suffix */
                        snprintf(buf, bufsize, "%.*s%s%s", (int)prefix_len, info, room_name, info + suffix_start);
                        info = buf;
                        break;
                    }
                    /* Buffer too small, fall through to use original */
                    break;
                }
                /* Not a complete match, continue searching */
                pos = strstr(pos + 1, room_num_str);
            }
        }
    }

    /* For bounty quests, add explicit information about the specific target */
    if (QST_TYPE(rnum) == AQ_MOB_KILL_BOUNTY && ch && !IS_NPC(ch)) {
        if (GET_BOUNTY_TARGET_ID(ch) != NOBODY) {
            /* Player has a specific bounty target assigned - find the mob and get its name */
            struct char_data *target_mob = NULL;
            const char *target_name = NULL;

            /* Search for the mob with the matching script_id */
            for (target_mob = character_list; target_mob; target_mob = target_mob->next) {
                if (IS_NPC(target_mob) && char_script_id(target_mob) == GET_BOUNTY_TARGET_ID(ch)) {
                    target_name = GET_NAME(target_mob);
                    break;
                }
            }

            /* If the specific mob is found in the world, show its name */
            if (target_name) {
                snprintf(temp_buf, sizeof(temp_buf),
                         "%s\r\n\tyIMPORTANTE: Esta busca requer a eliminação de um alvo ESPECÍFICO: %s.\tn\r\n"
                         "\tyVocê verá '(Bounty)' marcado em vermelho ao encontrar seu alvo.\tn",
                         info, target_name);
            } else {
                /* Target mob no longer exists - it may have been killed or despawned */
                snprintf(temp_buf, sizeof(temp_buf),
                         "%s\r\n\tyAVISO: O alvo específico não está mais disponível no mundo.\tn\r\n"
                         "\tyProcure pela pedra mágica que ele pode ter deixado e a entregue ao responsável pela "
                         "busca.\tn",
                         info);
            }
        } else {
            /* No specific target assigned yet */
            snprintf(temp_buf, sizeof(temp_buf),
                     "%s\r\n\tyIMPORTANTE: Esta busca requer a eliminação de um alvo ESPECÍFICO.\tn\r\n"
                     "\tyO alvo será identificado quando você aceitar a busca.\tn",
                     info);
        }
        snprintf(buf, bufsize, "%s", temp_buf);
        return buf;
    }

    /* For AQ_MOB_KILL quests, add information about magic stones */
    if (QST_TYPE(rnum) == AQ_MOB_KILL) {
        snprintf(temp_buf, sizeof(temp_buf),
                 "%s\r\n\tyDICA: Se o alvo já foi eliminado, procure por uma pedra mágica\tn\r\n"
                 "\tyque pode ter sido deixada e a entregue ao responsável pela busca.\tn",
                 info);
        snprintf(buf, bufsize, "%s", temp_buf);
        return buf;
    }

    /* For all other quest types or if no special formatting needed, return info */
    if (info != buf) {
        snprintf(buf, bufsize, "%s", info);
    }
    return buf;
}

/*--------------------------------------------------------------------------*/
/* Quest Completion Functions                                               */
/*--------------------------------------------------------------------------*/

/* Flag to prevent recursive quest completion during reward distribution */
static bool completing_quest = FALSE;

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
    GET_ESCORT_MOB_ID(ch) = NOBODY;
    GET_BOUNTY_TARGET_ID(ch) = NOBODY;
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

/* Escort Quest Helper Functions */
/** Spawns an escort mob for the player's quest at the questmaster's location.
 * @param ch The player who accepted the escort quest.
 * @param rnum The real quest number.
 * @return TRUE if mob spawned successfully, FALSE otherwise. */
bool spawn_escort_mob(struct char_data *ch, qst_rnum rnum)
{
    struct char_data *mob;
    mob_rnum mob_rnum;

    /* Validate target mob vnum */
    if (QST_TARGET(rnum) == NOTHING || QST_TARGET(rnum) <= 0) {
        send_to_char(ch, "ERRO: Quest mal configurada (alvo inválido).\r\n");
        return FALSE;
    }

    /* Get the real mob number */
    mob_rnum = real_mobile(QST_TARGET(rnum));
    if (mob_rnum == NOBODY) {
        send_to_char(ch, "ERRO: Mob de escolta não existe (vnum %d).\r\n", QST_TARGET(rnum));
        return FALSE;
    }

    /* Create the mob */
    mob = read_mobile(mob_rnum, REAL);
    if (!mob) {
        send_to_char(ch, "ERRO: Falha ao criar mob de escolta.\r\n");
        return FALSE;
    }

    /* Place mob in the same room as the player */
    char_to_room(mob, IN_ROOM(ch));

    /* Store the mob's ID for tracking */
    GET_ESCORT_MOB_ID(ch) = GET_IDNUM(mob);

    /* Make escort mob non-aggressive (but not invulnerable) */
    REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_AGGRESSIVE);
    REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_AGGR_EVIL);
    REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_AGGR_GOOD);
    REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_AGGR_NEUTRAL);

    /* Make the mob follow the player (without charming) */
    if (mob->master)
        stop_follower(mob);

    add_follower(mob, ch);

    /* Notify player */
    act("$N se aproxima e se prepara para seguir você.", FALSE, ch, 0, mob, TO_CHAR);
    act("$N se aproxima de $n.", TRUE, ch, 0, mob, TO_ROOM);

    return TRUE;
}

/** Assigns a bounty target for the player's quest by finding a specific mob instance.
 * Sets the player's bounty target ID and sends feedback message to the player.
 * @param ch The player who accepted the bounty quest.
 * @param qm The questmaster who assigned the quest.
 * @param rnum The real quest number.
 * @return TRUE if a target was found and assigned, FALSE otherwise. */
bool assign_bounty_target(struct char_data *ch, struct char_data *qm, qst_rnum rnum)
{
    struct char_data *target_mob = NULL;
    mob_rnum target_rnum = real_mobile(QST_TARGET(rnum));

    /* Find a living mob with the target vnum in the world */
    if (target_rnum != NOBODY) {
        for (target_mob = character_list; target_mob; target_mob = target_mob->next) {
            if (IS_NPC(target_mob) && GET_MOB_RNUM(target_mob) == target_rnum &&
                !MOB_FLAGGED(target_mob, MOB_NOTDEADYET)) {
                /* Found a target - store its unique ID */
                GET_BOUNTY_TARGET_ID(ch) = char_script_id(target_mob);
                send_to_char(ch, "%s diz, 'O alvo foi localizado. Boa caçada!'\r\n", GET_NAME(qm));
                return TRUE;
            }
        }
    }

    /* No target found - the specific instance doesn't exist in the world */
    GET_BOUNTY_TARGET_ID(ch) = NOBODY;
    send_to_char(ch, "%s diz, 'O alvo não está disponível no momento. Esta busca não pode ser aceita.'\r\n",
                 GET_NAME(qm));
    return FALSE;
}

/** Checks if an escort quest has been completed (mob reached destination).
 * @param ch The player on the escort quest.
 * @param rnum The real quest number.
 * @return TRUE if quest completed, FALSE otherwise. */
bool check_escort_quest_completion(struct char_data *ch, qst_rnum rnum)
{
    struct char_data *escort_mob = NULL;
    room_vnum dest_vnum;
    const char *escort_thanks_msg = "$n diz, 'Muito obrigado por me escoltar até aqui! Você foi muito corajoso!'";

    /* Check if we're in an escort quest */
    if (GET_QUEST_TYPE(ch) != AQ_MOB_ESCORT)
        return FALSE;

    /* Get the destination room vnum (stored in value[5]) */
    dest_vnum = QST_RETURNMOB(rnum); /* Reusing RETURNMOB for destination room */

    /* Check if player is in destination room */
    if (world[IN_ROOM(ch)].number != dest_vnum)
        return FALSE;

    /* Find the escort mob by ID */
    for (escort_mob = character_list; escort_mob; escort_mob = escort_mob->next) {
        if (IS_NPC(escort_mob) && GET_IDNUM(escort_mob) == GET_ESCORT_MOB_ID(ch))
            break;
    }

    /* If escort mob not found or not in same room, quest not complete */
    if (!escort_mob || IN_ROOM(escort_mob) != IN_ROOM(ch))
        return FALSE;

    /* Escort mob thanks the player */
    act(escort_thanks_msg, FALSE, escort_mob, 0, ch, TO_ROOM);
    act(escort_thanks_msg, FALSE, escort_mob, 0, ch, TO_VICT);

    /* Complete the quest */
    generic_complete_quest(ch);

    /* Remove the escort mob from the world */
    extract_char(escort_mob);

    return TRUE;
}

/** Called when an escort mob dies - fails the escort quest.
 * @param escort_mob The escort mob that died.
 * @param killer The character who killed the mob (can be NULL). */
void fail_escort_quest(struct char_data *escort_mob, struct char_data *killer)
{
    struct char_data *ch;
    qst_rnum rnum;

    if (!IS_NPC(escort_mob))
        return;

    /* Find the player escorting this mob */
    for (ch = character_list; ch; ch = ch->next) {
        if (IS_NPC(ch))
            continue;

        if (GET_QUEST_TYPE(ch) == AQ_MOB_ESCORT && GET_ESCORT_MOB_ID(ch) == GET_IDNUM(escort_mob)) {
            /* Get quest info for penalty */
            rnum = real_quest(GET_QUEST(ch));

            /* Notify player of quest failure */
            send_to_char(ch, "\r\n\ty** SUA QUEST DE ESCOLTA FALHOU! **\tn\r\n");
            send_to_char(ch, "A pessoa que você estava escoltando morreu!\r\n");

            /* Apply penalty - escort death is worse than abandoning */
            if (rnum != NOTHING && QST_PENALTY(rnum)) {
                int death_penalty = QST_PENALTY(rnum) * 2; /* Double penalty for escort death */
                GET_QUESTPOINTS(ch) -= death_penalty;
                send_to_char(ch, "Você perde %d pontos de busca por falhar em proteger o escoltado!\r\n\r\n",
                             death_penalty);
            } else {
                send_to_char(ch, "\r\n");
            }

            /* Clear the quest */
            clear_quest(ch);
            save_char(ch);
            break;
        }
    }
}

/** Called when a bounty target mob is killed by someone other than the quest holder.
 * Fails the bounty quest for the player who had the quest.
 * @param target_mob The bounty target mob that died.
 * @param killer The character who killed the mob (can be NULL for non-combat deaths). */
void fail_bounty_quest(struct char_data *target_mob, struct char_data *killer)
{
    struct char_data *ch;
    qst_rnum rnum;
    long target_id;

    if (!IS_NPC(target_mob))
        return;

    target_id = char_script_id(target_mob);

    /* Find any player with a bounty quest targeting this specific mob */
    for (ch = character_list; ch; ch = ch->next) {
        if (IS_NPC(ch))
            continue;

        /* Check if this player has a bounty quest for this specific mob */
        if (GET_QUEST_TYPE(ch) == AQ_MOB_KILL_BOUNTY && GET_BOUNTY_TARGET_ID(ch) == target_id) {
            /* Skip if the player is the one who killed it (quest will complete normally) */
            if (killer && killer == ch)
                continue;

            /* Get quest info for penalty */
            rnum = real_quest(GET_QUEST(ch));

            /* Notify player of quest failure */
            send_to_char(ch, "\r\n\ty** SUA QUEST DE CAÇA FALHOU! **\tn\r\n");
            if (killer && !IS_NPC(killer)) {
                send_to_char(ch, "Seu alvo foi eliminado por %s antes que você pudesse completar a caçada!\r\n",
                             GET_NAME(killer));
            } else if (killer && IS_NPC(killer)) {
                send_to_char(ch, "Seu alvo foi eliminado por %s antes que você pudesse completar a caçada!\r\n",
                             GET_NAME(killer));
            } else {
                send_to_char(ch, "Seu alvo morreu antes que você pudesse completar a caçada!\r\n");
            }

            /* Apply penalty - bounty target lost is similar to abandoning */
            if (rnum != NOTHING && QST_PENALTY(rnum)) {
                GET_QUESTPOINTS(ch) -= QST_PENALTY(rnum);
                send_to_char(ch, "Você perde %d pontos de busca por falhar em capturar o alvo!\r\n\r\n",
                             QST_PENALTY(rnum));
            } else {
                send_to_char(ch, "\r\n");
            }

            /* Clear the quest */
            clear_quest(ch);
            save_char(ch);
        }
    }
}

void generic_complete_quest(struct char_data *ch)
{
    qst_rnum rnum;
    qst_vnum vnum = GET_QUEST(ch);
    struct obj_data *new_obj;
    int happy_qp, happy_gold, happy_exp;
    char formatted_info[MAX_QUEST_MSG];

    if (--GET_QUEST_COUNTER(ch) <= 0) {
        rnum = real_quest(vnum);

        /* Safety check: quest must exist */
        if (rnum == NOTHING) {
            send_to_char(ch, "ERRO: A busca que você estava fazendo não existe mais!\r\n");
            log1(
                "SYSERR: generic_complete_quest: Player %s tried to complete non-existent quest vnum %d. "
                "Quest may have been removed from game or quest table corrupted. Clearing player's quest state.",
                GET_NAME(ch), vnum);
            clear_quest(ch);
            save_char(ch);
            return;
        }

        /* Set flag to prevent recursive quest completion during reward distribution */
        completing_quest = TRUE;

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

        /* Notify the room that the character completed a quest */
        act("$n completou uma busca.", TRUE, ch, NULL, NULL, TO_ROOM);

        clear_quest(ch);

        /* Cleanup wishlist quests after completion */
        cleanup_completed_wishlist_quest(vnum);

        if ((real_quest(QST_NEXT(rnum)) != NOTHING) && (QST_NEXT(rnum) != vnum) && !is_complete(ch, QST_NEXT(rnum))) {
            rnum = real_quest(QST_NEXT(rnum));
            set_quest(ch, rnum);
            send_to_char(ch, "A sua busca continua:\r\n%s",
                         format_quest_info(rnum, ch, formatted_info, sizeof(formatted_info)));
        }

        /* Clear flag after quest completion is done */
        completing_quest = FALSE;
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

    /* Prevent recursive quest completion during reward distribution */
    if (completing_quest)
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
            /* Check for direct kill */
            if (!IS_NPC(ch) && vict && IS_NPC(vict) && (ch != vict)) {
                if (QST_TARGET(rnum) == GET_MOB_VNUM(vict))
                    generic_complete_quest(ch);
            }
            /* Check for magic stone return to questmaster or requester */
            else if (!IS_NPC(ch) && vict && IS_NPC(vict) && object && GET_OBJ_TYPE(object) == ITEM_MAGIC_STONE) {
                /* Check if this magic stone is from the target mob */
                if (GET_OBJ_VAL(object, 0) == QST_TARGET(rnum)) {
                    /* Verify the object is actually in the NPC's inventory */
                    struct obj_data *obj_check;
                    bool has_object = false;

                    /* Safety check: vict must have valid carrying list */
                    if (!vict->carrying) {
                        break;
                    }

                    for (obj_check = vict->carrying; obj_check; obj_check = obj_check->next_content) {
                        if (obj_check == object) {
                            has_object = true;
                            break;
                        }
                    }

                    if (has_object) {
                        /* Check if returning to questmaster or original requester */
                        if (GET_MOB_VNUM(vict) == QST_MASTER(rnum) || GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum)) {
                            /* Extract the magic stone - it's been used */
                            extract_obj(object);
                            generic_complete_quest(ch);
                        }
                    }
                }
            }
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
            /* Fixed logic: verify that the NPC actually has the item in inventory before completing */
            if (IS_NPC(vict) && object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum))) {
                /* Check if the object is in the NPC's inventory */
                struct obj_data *obj_check;
                bool has_object = false;

                for (obj_check = vict->carrying; obj_check; obj_check = obj_check->next_content) {
                    if (obj_check == object) {
                        has_object = true;
                        break;
                    }
                }

                if (!has_object) {
                    /* Object not in NPC's inventory - don't complete quest */
                    break;
                }

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
                            act("$n entrega $p para quem solicitou.", FALSE, vict, object, NULL, TO_ROOM);
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
            /* Check for direct kill */
            if (!IS_NPC(ch) && vict && IS_NPC(vict) && (ch != vict)) {
                /* Check if this is the specific bounty target */
                if (GET_BOUNTY_TARGET_ID(ch) != NOBODY) {
                    /* We have a specific target ID - check against it */
                    if (char_script_id(vict) == GET_BOUNTY_TARGET_ID(ch))
                        generic_complete_quest(ch);
                } else {
                    /* No specific ID set - fallback to vnum matching (for old quests or if target respawned) */
                    if (QST_TARGET(rnum) == GET_MOB_VNUM(vict))
                        generic_complete_quest(ch);
                }
            }
            /* Check for magic stone return to questmaster or requester */
            else if (!IS_NPC(ch) && vict && IS_NPC(vict) && object && GET_OBJ_TYPE(object) == ITEM_MAGIC_STONE) {
                /* For bounty quests, check both vnum and specific ID */
                bool matches = false;

                /* Check if vnum matches */
                if (GET_OBJ_VAL(object, 0) == QST_TARGET(rnum)) {
                    /* If we have a specific bounty target ID, also check that */
                    if (GET_BOUNTY_TARGET_ID(ch) != NOBODY) {
                        if (GET_OBJ_VAL(object, 1) == GET_BOUNTY_TARGET_ID(ch))
                            matches = true;
                    } else {
                        /* No specific ID required, vnum match is enough */
                        matches = true;
                    }
                }

                if (matches) {
                    /* Verify the object is actually in the NPC's inventory */
                    struct obj_data *obj_check;
                    bool has_object = false;

                    /* Safety check: vict must have valid carrying list */
                    if (!vict->carrying) {
                        break;
                    }

                    for (obj_check = vict->carrying; obj_check; obj_check = obj_check->next_content) {
                        if (obj_check == object) {
                            has_object = true;
                            break;
                        }
                    }

                    if (has_object) {
                        /* Check if returning to questmaster or original requester */
                        if (GET_MOB_VNUM(vict) == QST_MASTER(rnum) || GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum)) {
                            /* Extract the magic stone - it's been used */
                            extract_obj(object);
                            generic_complete_quest(ch);
                        }
                    }
                }
            }
            break;
        case AQ_MOB_ESCORT:
            /* Escort quest completion is handled separately in check_escort_quest_completion() */
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
                /* For mortals, check level restrictions */
                if (!is_quest_level_available(ch, rnum))
                    continue; /* Skip quests outside player's level range */
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
                /* For mortals, check level restrictions */
                if (!is_quest_level_available(ch, rnum))
                    continue; /* Skip quests outside player's level range */
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
    char formatted_info[MAX_QUEST_MSG];

    if ((vnum = find_unified_quest_by_qmnum(ch, qm, atoi(argument))) == NOTHING)
        send_to_char(ch, "Esta não é uma busca válida!\r\n");
    else if ((rnum = real_quest(vnum)) == NOTHING)
        send_to_char(ch, "Esta não é uma busca válida!\r\n");
    else if (QST_INFO(rnum)) {
        send_to_char(ch, "Detalhes Completos da Busca \tc%s\tn:\r\n%s", QST_DESC(rnum),
                     format_quest_info(rnum, ch, formatted_info, sizeof(formatted_info)));
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

/* Calculate quest difficulty based on quest data */
static const char *get_quest_difficulty_string(qst_rnum rnum)
{
    int min_level = QST_MINLEVEL(rnum);
    int max_level = QST_MAXLEVEL(rnum);
    int avg_level = (min_level + max_level) / 2;
    int reward = QST_GOLD(rnum);

    /* Difficulty based on level range and rewards */
    if (avg_level >= 50 || reward >= 500 || max_level >= 60) {
        return "Extrema";
    } else if (avg_level >= 30 || reward >= 250 || max_level >= 40) {
        return "Alta";
    } else if (avg_level >= 15 || reward >= 100 || max_level >= 25) {
        return "Média";
    } else {
        return "Baixa";
    }
}

/* Forward declaration */
static bool is_bounty_target_available(qst_rnum rnum, struct char_data *ch);

/* Unified quest display function - shows both regular and temporary quests */
static void quest_show_unified(struct char_data *ch, struct char_data *qm)
{
    qst_rnum rnum;
    int counter = 0, i;
    int quest_completed, quest_repeatable;
    mob_vnum qm_vnum = GET_MOB_VNUM(qm);

    send_to_char(ch,
                 "A lista de buscas disponiveis:\r\n"
                 "Num.  Descrição                   Dificuldade Níveis    Feita?\r\n"
                 "----- ---------------------------- ----------- --------- ------\r\n");

    /* First, show regular quests assigned to this questmaster */
    for (rnum = 0; rnum < total_quests; rnum++) {
        if (qm_vnum == QST_MASTER(rnum)) {
            quest_completed = is_complete(ch, QST_NUM(rnum));
            quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

            /* Only show quest if not completed or repeatable */
            if (!quest_completed || quest_repeatable) {
                /* For mortals, check level restrictions */
                if (!is_quest_level_available(ch, rnum))
                    continue; /* Skip quests outside player's level range */

                /* Skip bounty quests if the specific target is unavailable */
                if (!is_bounty_target_available(rnum, ch))
                    continue;

                send_to_char(ch, "\tg%4d\tn) \tc%-28.28s\tn \ty%-11s\tn \tw%3d-%-3d\tn   \ty(%s)\tn\r\n", ++counter,
                             QST_NAME(rnum), get_quest_difficulty_string(rnum), QST_MINLEVEL(rnum), QST_MAXLEVEL(rnum),
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
                /* For mortals, check level restrictions */
                if (!is_quest_level_available(ch, rnum))
                    continue; /* Skip quests outside player's level range */

                /* Skip bounty quests if the specific target is unavailable */
                if (!is_bounty_target_available(rnum, ch))
                    continue;

                send_to_char(ch, "\tg%4d\tn) \tc%-28.28s\tn \ty%-11s\tn \tw%3d-%-3d\tn   \ty(%s)\tn\r\n", ++counter,
                             QST_NAME(rnum), get_quest_difficulty_string(rnum), QST_MINLEVEL(rnum), QST_MAXLEVEL(rnum),
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
    char formatted_info[MAX_QUEST_MSG];
    int quest_num;

    /* Early check for descriptor - needed for confirmation system */
    if (!ch->desc) {
        log1("SYSERR: quest_join_unified called for character without descriptor: %s", GET_NAME(ch));
        return;
    }

    if (!*argument) {
        snprintf(buf, sizeof(buf), "%s diz, 'Qual busca você quer aceitar, %s?'", GET_NAME(qm), GET_NAME(ch));
        send_to_char(ch, "%s\r\n", buf);
        save_char(ch);
        return;
    }

    quest_num = atoi(argument);

    if (GET_QUEST(ch) != NOTHING) {
        snprintf(buf, sizeof(buf), "%s diz, 'Mas você já tem uma busca ativa, %s!'", GET_NAME(qm), GET_NAME(ch));
    } else if ((vnum = find_unified_quest_by_qmnum(ch, qm, quest_num)) == NOTHING) {
        snprintf(buf, sizeof(buf), "%s diz, 'Eu não conheço nenhuma busca assim, %s!'", GET_NAME(qm), GET_NAME(ch));
    } else if ((rnum = real_quest(vnum)) == NOTHING) {
        snprintf(buf, sizeof(buf), "%s diz, 'Eu não conheço essa busca, %s!'", GET_NAME(qm), GET_NAME(ch));
    } else if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(ch) < QST_MINLEVEL(rnum)) {
        snprintf(buf, sizeof(buf), "%s diz, 'Sinto muito, mas você ainda não pode participar desta busca, %s!'",
                 GET_NAME(qm), GET_NAME(ch));
    } else if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(ch) > QST_MAXLEVEL(rnum)) {
        snprintf(buf, sizeof(buf), "%s diz, 'Sinto muito, mas você tem muita experiência para aceitar esta busca, %s!'",
                 GET_NAME(qm), GET_NAME(ch));
    } else if (is_complete(ch, vnum)) {
        snprintf(buf, sizeof(buf), "%s diz, 'Você já completou esta busca antes, %s!'", GET_NAME(qm), GET_NAME(ch));
    } else if ((QST_PREV(rnum) != NOTHING) && !is_complete(ch, QST_PREV(rnum))) {
        snprintf(buf, sizeof(buf), "%s diz, 'Você precisa completar outra busca antes, %s!'", GET_NAME(qm),
                 GET_NAME(ch));
    } else if ((QST_PREREQ(rnum) != NOTHING) && (real_object(QST_PREREQ(rnum)) != NOTHING) &&
               (get_obj_in_list_num(real_object(QST_PREREQ(rnum)), ch->carrying) == NULL)) {
        snprintf(buf, sizeof(buf), "%s diz, 'Você precisa primeiro ter %s antes!'", GET_NAME(qm),
                 obj_proto[real_object(QST_PREREQ(rnum))].short_description);
    } else {
        /* Show quest details and ask for confirmation */
        send_to_char(ch, "%s diz, 'Você deseja aceitar esta busca, %s?'\r\n\r\n", GET_NAME(qm), GET_NAME(ch));
        send_to_char(ch, "\tc┌────────────────────────────────────────────────────────────────┐\tn\r\n");
        send_to_char(ch, "\tc│                      DETALHES DA BUSCA                        │\tn\r\n");
        send_to_char(ch, "\tc└────────────────────────────────────────────────────────────────┘\tn\r\n\r\n");
        send_to_char(ch, "\tcNome:\tn         %s\r\n", QST_NAME(rnum));
        send_to_char(ch, "\tcNíveis:\tn       %d-%d\r\n", QST_MINLEVEL(rnum), QST_MAXLEVEL(rnum));
        send_to_char(ch, "\tcDificuldade:\tn  %s\r\n", get_quest_difficulty_string(rnum));
        if (QST_TIME(rnum) != -1)
            send_to_char(ch, "\tcTempo Limite:\tn %d tick%s\r\n", QST_TIME(rnum), QST_TIME(rnum) == 1 ? "" : "s");
        else
            send_to_char(ch, "\tcTempo Limite:\tn Sem limite\r\n");
        send_to_char(ch, "\r\n%s", format_quest_info(rnum, ch, formatted_info, sizeof(formatted_info)));
        send_to_char(ch, "\r\n\tyAceitar esta busca? (S/N):\tn ");

        /* Store quest info and switch to confirmation state */
        ch->desc->pending_quest_vnum = vnum;
        ch->desc->pending_questmaster = qm;
        STATE(ch->desc) = CON_QACCEPT;
        return;
    }
    send_to_char(ch, "%s\r\n", buf);
    save_char(ch);
}

static void quest_progress(struct char_data *ch)
{
    qst_rnum rnum;
    char formatted_info[MAX_QUEST_MSG];

    if (GET_QUEST(ch) == NOTHING)
        send_to_char(ch, "Mas você não está em uma busca no momento!\r\n");
    else if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING) {
        clear_quest(ch);
        send_to_char(ch, "A busca foi cancelada e não existe mais!\r\n");
    } else {
        send_to_char(ch, "Você aceitou as seguintes buscas:\r\n%s\r\n%s", QST_DESC(rnum),
                     format_quest_info(rnum, ch, formatted_info, sizeof(formatted_info)));
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

static void quest_remove(struct char_data *ch, char *argument)
{
    qst_rnum rnum;
    qst_vnum quest_vnum;
    int quest_num;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    if (GET_LEVEL(ch) < LVL_GOD) {
        send_to_char(ch, "You must be at least level GOD to remove quests.\r\n");
        return;
    }

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "%s\r\n", quest_god_usage);
        return;
    }

    quest_num = atoi(arg1);

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

    /* Check if quest is mob-posted */
    if (!IS_SET(QST_FLAGS(rnum), AQ_MOB_POSTED)) {
        send_to_char(ch, "Only mob-posted quests can be removed. Quest %d is not mob-posted.\r\n", quest_vnum);
        return;
    }

    /* Show quest info and require confirmation */
    if (!*arg2 || str_cmp(arg2, "confirm") != 0) {
        send_to_char(ch, "Quest to be removed:\r\n");
        send_to_char(ch, "  VNum: \ty%d\tn, Name: \tc%s\tn\r\n", quest_vnum, QST_NAME(rnum));
        send_to_char(ch, "  Desc: \tc%s\tn\r\n", QST_DESC(rnum));
        send_to_char(ch, "  Type: \ty%s\tn\r\n", quest_types[QST_TYPE(rnum)]);
        send_to_char(ch, "\r\nTo confirm deletion, type: \tYquest remove %d confirm\tn\r\n", quest_num);
        return;
    }

    /* Delete the quest with confirmation */
    char quest_name_copy[MAX_QUEST_NAME];
    snprintf(quest_name_copy, sizeof(quest_name_copy), "%s", QST_NAME(rnum));

    send_to_char(ch, "Removing quest %d...\r\n", quest_vnum);
    if (delete_quest(rnum)) {
        send_to_char(ch, "\tGQuest successfully removed.\tn\r\n");
        mudlog(NRM, LVL_GOD, TRUE, "(GC) %s removed mob-posted quest %d (%s).", GET_NAME(ch), quest_vnum,
               quest_name_copy);
    } else {
        send_to_char(ch, "\tRFailed to remove quest.\tn\r\n");
    }
}

/*--------------------------------------------------------------------------*/
/* Quest Command Processing Function and Questmaster Special                */
/*--------------------------------------------------------------------------*/

ACMD(do_quest)
{
    char arg1[MAX_INPUT_LENGTH];
    char *arg2;
    int tp;
    const char *usage_msg;

    arg2 = one_argument(argument, arg1);

    /* Determine which usage message to show */
    if (GET_LEVEL(ch) >= LVL_GOD)
        usage_msg = quest_god_usage;
    else if (GET_LEVEL(ch) >= LVL_IMMORT)
        usage_msg = quest_imm_usage;
    else
        usage_msg = quest_mort_usage;

    if (!*arg1)
        send_to_char(ch, "%s\r\n", usage_msg);
    else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
        send_to_char(ch, "%s\r\n", usage_msg);
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
            case SCMD_QUEST_REMOVE:
                if (GET_LEVEL(ch) < LVL_GOD)
                    send_to_char(ch, "%s\r\n", usage_msg);
                else
                    quest_remove(ch, arg2);
                break;
            default: /* Whe should never get here, but... */
                send_to_char(ch, "%s\r\n", usage_msg);
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

    /* MOB_NOKILL mobs cannot request escort quests (they can't die, so quest can't fail properly) */
    if (QST_TYPE(rnum) == AQ_MOB_ESCORT && MOB_FLAGGED(mob, MOB_NOKILL))
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

    /* If mob was working on quest completion, clear that goal */
    if (mob->ai_data->current_goal == GOAL_COMPLETE_QUEST) {
        mob->ai_data->current_goal = GOAL_NONE;
        mob->ai_data->goal_destination = NOWHERE;
        mob->ai_data->goal_target_mob_rnum = NOBODY;
        mob->ai_data->goal_item_vnum = NOTHING;
        mob->ai_data->goal_timer = 0;
    }
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
    char formatted_info[MAX_QUEST_MSG];

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
    else if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(ch) < QST_MINLEVEL(rnum))
        snprintf(buf, sizeof(buf), "%s diz, 'Sinto muito, mas você ainda não pode participar desta busca, %s!'",
                 GET_NAME(qm), GET_NAME(ch));
    else if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(ch) > QST_MAXLEVEL(rnum))
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
        send_to_char(ch, "%s", format_quest_info(rnum, ch, formatted_info, sizeof(formatted_info)));
        if (QST_TIME(rnum) != -1) {
            send_to_char(ch, "%s diz, 'Você tem um tempo limite de %d tick%s para completar esta busca!'\r\n",
                         GET_NAME(qm), QST_TIME(rnum), QST_TIME(rnum) == 1 ? "" : "s");
        } else {
            send_to_char(ch, "%s diz, 'Você pode levar o tempo que precisar para completar esta busca.'\r\n",
                         GET_NAME(qm));
        }

        /* For escort quests, spawn the escort mob */
        if (QST_TYPE(rnum) == AQ_MOB_ESCORT) {
            if (!spawn_escort_mob(ch, rnum)) {
                /* If spawning fails, cancel the quest */
                send_to_char(ch, "%s diz, 'Desculpe, parece que houve um problema. A busca foi cancelada.'\r\n",
                             GET_NAME(qm));
                clear_quest(ch);
            }
        }
        /* For bounty quests, find and mark the target mob */
        else if (QST_TYPE(rnum) == AQ_MOB_KILL_BOUNTY) {
            assign_bounty_target(ch, qm, rnum);
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

    /* Set default values for genetics if not already set from mob files.
     * This ensures mobs have reasonable default behavior even without explicit genetics.
     * These defaults can be overridden by GenFollow, GenRoam, etc. in mob files. */
    if (mob->ai_data->genetics.follow_tendency == 0) {
        /* Default follow tendency: 20% chance to follow others
         * This provides some following behavior without being too aggressive */
        mob->ai_data->genetics.follow_tendency = 20;
    }

    /* Initialize emotions based on genetics and alignment
     * This gives mobs starting emotional states that reflect their nature */

    /* Fear and courage are inversely related to brave_prevalence and wimpy_tendency */
    mob->ai_data->emotion_fear = mob->ai_data->genetics.wimpy_tendency / 2;      /* 0-50 based on wimpy */
    mob->ai_data->emotion_courage = mob->ai_data->genetics.brave_prevalence / 2; /* 0-50 based on bravery */

    /* Friendship and loyalty based on group_tendency */
    mob->ai_data->emotion_friendship = mob->ai_data->genetics.group_tendency / 3; /* 0-33 based on grouping */
    mob->ai_data->emotion_loyalty = mob->ai_data->genetics.group_tendency / 2;    /* 0-50 based on grouping */

    /* Trust based on trade_tendency */
    mob->ai_data->emotion_trust = mob->ai_data->genetics.trade_tendency / 2; /* 0-50 based on trading */

    /* Curiosity based on quest_tendency and adventurer_tendency */
    mob->ai_data->emotion_curiosity =
        (mob->ai_data->genetics.quest_tendency + mob->ai_data->genetics.adventurer_tendency) / 4; /* 0-50 */

    /* Greed based on loot_tendency */
    mob->ai_data->emotion_greed = mob->ai_data->genetics.loot_tendency / 2; /* 0-50 based on looting */

    /* Compassion based on healing_tendency */
    mob->ai_data->emotion_compassion = mob->ai_data->genetics.healing_tendency / 2; /* 0-50 based on healing */

    /* Excitement based on roam_tendency */
    mob->ai_data->emotion_excitement = mob->ai_data->genetics.roam_tendency / 2; /* 0-50 based on roaming */

    /* Alignment-based emotions */
    if (IS_GOOD(mob)) {
        mob->ai_data->emotion_happiness = 30 + rand_number(0, 20); /* Good mobs start happier: 30-50 */
        mob->ai_data->emotion_anger = rand_number(0, 20);          /* Lower anger: 0-20 */
        mob->ai_data->emotion_compassion += 20;                    /* More compassionate */
    } else if (IS_EVIL(mob)) {
        mob->ai_data->emotion_anger = 30 + rand_number(0, 20); /* Evil mobs start angrier: 30-50 */
        mob->ai_data->emotion_happiness = rand_number(0, 20);  /* Lower happiness: 0-20 */
        mob->ai_data->emotion_greed += 20;                     /* More greedy */
    } else {
        /* Neutral mobs have balanced emotions */
        mob->ai_data->emotion_happiness = rand_number(10, 30);
        mob->ai_data->emotion_anger = rand_number(10, 30);
    }

    /* Random variation in other emotions (0-20) */
    mob->ai_data->emotion_sadness = rand_number(0, 20);
    mob->ai_data->emotion_love = rand_number(0, 15);
    mob->ai_data->emotion_pride = rand_number(10, 40);
    mob->ai_data->emotion_envy = rand_number(0, 25);

    /* Ensure all emotions stay within 0-100 bounds (defensive programming for future modifications) */
    mob->ai_data->emotion_fear = URANGE(0, mob->ai_data->emotion_fear, 100);
    mob->ai_data->emotion_courage = URANGE(0, mob->ai_data->emotion_courage, 100);
    mob->ai_data->emotion_happiness = URANGE(0, mob->ai_data->emotion_happiness, 100);
    mob->ai_data->emotion_anger = URANGE(0, mob->ai_data->emotion_anger, 100);
    mob->ai_data->emotion_friendship = URANGE(0, mob->ai_data->emotion_friendship, 100);
    mob->ai_data->emotion_loyalty = URANGE(0, mob->ai_data->emotion_loyalty, 100);
    mob->ai_data->emotion_trust = URANGE(0, mob->ai_data->emotion_trust, 100);
    mob->ai_data->emotion_curiosity = URANGE(0, mob->ai_data->emotion_curiosity, 100);
    mob->ai_data->emotion_greed = URANGE(0, mob->ai_data->emotion_greed, 100);
    mob->ai_data->emotion_compassion = URANGE(0, mob->ai_data->emotion_compassion, 100);
    mob->ai_data->emotion_excitement = URANGE(0, mob->ai_data->emotion_excitement, 100);
    mob->ai_data->emotion_sadness = URANGE(0, mob->ai_data->emotion_sadness, 100);
    mob->ai_data->emotion_love = URANGE(0, mob->ai_data->emotion_love, 100);
    mob->ai_data->emotion_pride = URANGE(0, mob->ai_data->emotion_pride, 100);
    mob->ai_data->emotion_envy = URANGE(0, mob->ai_data->emotion_envy, 100);

    /* Initialize emotion memory system - zero out all memory slots */
    {
        int i;
        for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
            mob->ai_data->memories[i].timestamp = 0; /* Mark as unused */
            mob->ai_data->memories[i].entity_id = 0;
            mob->ai_data->memories[i].entity_type = 0;
            mob->ai_data->memories[i].interaction_type = 0;
            mob->ai_data->memories[i].major_event = 0;
            mob->ai_data->memories[i].trust_level = 0;
            mob->ai_data->memories[i].friendship_level = 0;
            mob->ai_data->memories[i].fear_level = 0;
            mob->ai_data->memories[i].anger_level = 0;
        }
        mob->ai_data->memory_index = 0; /* Start at beginning of circular buffer */
    }
}

/* Save temporary quest assignments to file */
void save_temp_quest_assignments(void)
{
    FILE *fp;
    struct char_data *mob;
    int i;
    char temp_file[256];

    /* Use a temporary file for atomic writes */
    snprintf(temp_file, sizeof(temp_file), TEMP_QUEST_FILE);

    if (!(fp = fopen(temp_file, "w"))) {
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

    if (!(fp = fopen(TEMP_QUEST_FILE, "r"))) {
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

/* Check if a bounty quest's target is currently available in the world
 * Returns TRUE if the target is available or quest doesn't have a specific target yet
 * Returns FALSE if the quest has a specific target that is no longer in the world */
static bool is_bounty_target_available(qst_rnum rnum, struct char_data *ch)
{
    struct char_data *target_mob;

    /* Safety check: validate quest rnum */
    if (rnum == NOTHING || rnum < 0 || rnum >= total_quests)
        return TRUE;

    /* Only check for bounty quests */
    if (QST_TYPE(rnum) != AQ_MOB_KILL_BOUNTY)
        return TRUE;

    /* If player hasn't accepted the quest yet (no bounty target ID), it's available */
    if (!ch || IS_NPC(ch) || GET_BOUNTY_TARGET_ID(ch) == NOBODY)
        return TRUE;

    /* Check if this is the player's active quest */
    if (GET_QUEST(ch) != QST_NUM(rnum))
        return TRUE;

    /* Search for the specific target mob in the world - with safety checks */
    for (target_mob = character_list; target_mob; target_mob = target_mob->next) {
        /* Safety check: ensure target_mob is valid before accessing */
        if (!target_mob)
            break;

        if (IS_NPC(target_mob) && char_script_id(target_mob) == GET_BOUNTY_TARGET_ID(ch)) {
            return TRUE; /* Target is still in the world */
        }
    }

    /* Target not found - it's been killed or despawned */
    return FALSE;
}

/* Called from interpreter.c when player confirms quest acceptance */
void accept_pending_quest(struct descriptor_data *d)
{
    struct char_data *ch = d->character;
    struct char_data *qm = d->pending_questmaster;
    qst_vnum vnum = d->pending_quest_vnum;
    qst_rnum rnum;
    char formatted_info[MAX_QUEST_MSG];
    struct char_data *tmp;
    bool qm_valid = FALSE;

    /* Validate questmaster is still in the game and not being extracted */
    if (qm) {
        for (tmp = character_list; tmp; tmp = tmp->next) {
            if (tmp == qm && !MOB_FLAGGED(qm, MOB_NOTDEADYET)) {
                qm_valid = TRUE;
                break;
            }
        }
    }

    if (!ch || !qm || vnum == NOTHING || !qm_valid) {
        write_to_output(d, "\r\nErro ao aceitar busca. O questmaster não está mais disponível. Tente novamente.\r\n");
        /* Clear pending quest data */
        d->pending_quest_vnum = NOTHING;
        d->pending_questmaster = NULL;
        return;
    }

    rnum = real_quest(vnum);
    if (rnum == NOTHING) {
        write_to_output(d, "\r\nEsta busca não existe mais.\r\n");
        /* Clear pending quest data */
        d->pending_quest_vnum = NOTHING;
        d->pending_questmaster = NULL;
        return;
    }

    /* Final checks before accepting */
    if (GET_QUEST(ch) != NOTHING) {
        write_to_output(d, "\r\nVocê já tem uma busca ativa!\r\n");
        /* Clear pending quest data */
        d->pending_quest_vnum = NOTHING;
        d->pending_questmaster = NULL;
        return;
    }

    /* Accept the quest */
    act("Você aceitou a busca.", TRUE, ch, NULL, NULL, TO_CHAR);
    act("$n aceitou uma busca.", TRUE, ch, NULL, NULL, TO_ROOM);
    write_to_output(d, "%s diz, 'Ótimo! As instruções para esta busca são:'\r\n", GET_NAME(qm));
    set_quest(ch, rnum);
    write_to_output(d, "%s", format_quest_info(rnum, ch, formatted_info, sizeof(formatted_info)));

    if (QST_TIME(rnum) != -1) {
        write_to_output(d, "%s diz, 'Você tem um tempo limite de %d tick%s para completar esta busca!'\r\n",
                        GET_NAME(qm), QST_TIME(rnum), QST_TIME(rnum) == 1 ? "" : "s");
    } else {
        write_to_output(d, "%s diz, 'Você pode levar o tempo que precisar para completar esta busca.'\r\n",
                        GET_NAME(qm));
    }

    /* For escort quests, spawn the escort mob */
    if (QST_TYPE(rnum) == AQ_MOB_ESCORT) {
        if (!spawn_escort_mob(ch, rnum)) {
            write_to_output(d, "%s diz, 'Desculpe, parece que houve um problema. A busca foi cancelada.'\r\n",
                            GET_NAME(qm));
            clear_quest(ch);
        }
    }
    /* For bounty quests, find and mark the target mob */
    else if (QST_TYPE(rnum) == AQ_MOB_KILL_BOUNTY) {
        if (!assign_bounty_target(ch, qm, rnum)) {
            clear_quest(ch);
        }
    }

    save_char(ch);

    /* Clear pending quest data */
    d->pending_quest_vnum = NOTHING;
    d->pending_questmaster = NULL;
}

/* Check if there are any active kill quests for a specific mob vnum
 * Returns true if there are active AQ_MOB_KILL or AQ_MOB_KILL_BOUNTY quests for this mob */
bool has_active_kill_quest_for_mob(mob_vnum target_vnum)
{
    qst_rnum rnum;

    /* Safety check: validate target_vnum */
    if (target_vnum == NOTHING || target_vnum < 0)
        return false;

    for (rnum = 0; rnum < total_quests; rnum++) {
        /* Safety check: ensure rnum is valid */
        if (rnum < 0 || rnum >= total_quests)
            break;

        /* Only check kill-type quests */
        if (QST_TYPE(rnum) != AQ_MOB_KILL && QST_TYPE(rnum) != AQ_MOB_KILL_BOUNTY)
            continue;

        /* Check if this quest targets our mob */
        if (QST_TARGET(rnum) == target_vnum) {
            /* Check if quest is mob-posted (those are the ones that can have the issue) */
            if (IS_SET(QST_FLAGS(rnum), AQ_MOB_POSTED))
                return true;
        }
    }

    return false;
}

/* Check if player is carrying a magic stone for their active quest and fail the quest if so
 * This is called before extracting NORENT items on logout/quit */
void check_and_fail_quest_with_magic_stone(struct char_data *ch)
{
    qst_rnum rnum;
    struct obj_data *obj;
    bool has_magic_stone = false;

    /* Safety checks */
    if (!ch || IS_NPC(ch))
        return;

    /* Check if player has an active quest */
    if (GET_QUEST(ch) == NOTHING)
        return;

    rnum = real_quest(GET_QUEST(ch));
    if (rnum == NOTHING)
        return;

    /* Only check for kill-type quests */
    if (QST_TYPE(rnum) != AQ_MOB_KILL && QST_TYPE(rnum) != AQ_MOB_KILL_BOUNTY)
        return;

    /* Check if player is carrying a magic stone that matches their quest */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_MAGIC_STONE) {
            /* Check if this stone matches the quest target */
            if (GET_OBJ_VAL(obj, 0) == QST_TARGET(rnum)) {
                /* For bounty quests, also check the specific mob ID if set */
                if (QST_TYPE(rnum) == AQ_MOB_KILL_BOUNTY && GET_BOUNTY_TARGET_ID(ch) != NOBODY) {
                    if (GET_OBJ_VAL(obj, 1) == GET_BOUNTY_TARGET_ID(ch)) {
                        has_magic_stone = true;
                        break;
                    }
                } else {
                    /* For regular kill quests, vnum match is sufficient */
                    has_magic_stone = true;
                    break;
                }
            }
        }
    }

    /* If player has the magic stone, fail the quest with penalty */
    if (has_magic_stone) {
        send_to_char(
            ch, "\r\n\tyAVISO: Você estava carregando uma pedra mágica de busca que não pode ser preservada.\tn\r\n");
        send_to_char(ch, "\tyA busca '%s' foi cancelada.\tn\r\n", QST_NAME(rnum));

        /* Apply penalty if quest has one */
        if (QST_PENALTY(rnum)) {
            GET_QUESTPOINTS(ch) -= QST_PENALTY(rnum);
            send_to_char(ch, "\tyVocê paga %d pontos de busca por ter perdido a pedra mágica.\tn\r\n",
                         QST_PENALTY(rnum));
        }

        /* Clear the quest */
        clear_quest(ch);
        save_char(ch);
    }
}
