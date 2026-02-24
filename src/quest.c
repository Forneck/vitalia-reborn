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
#include "spells.h"          /* for calculate_mana_density */
#include "act.h"             /* for do_tell */
#include "dg_scripts.h"      /* for char_script_id */
#include "modify.h"          /* for page_string */
#include "shadow_timeline.h" /* for cognitive capacity constants */
#include "sec.h"             /* for sec_init */

/*--------------------------------------------------------------------------
 * Exported global variables
 *--------------------------------------------------------------------------*/
const char *quest_types[] = {"Object",          "Room",         "Find mob",    "Kill mob",          "Save mob",
                             "Return object",   "Clear room",   "Kill player", "Kill mob (bounty)", "Escort mob",
                             "Improve emotion", "Magic gather", "Delivery",    "Resource gather",   "Reputation build",
                             "Shop buy",        "Shop sell",    "\n"};
const char *aq_flags[] = {"REPEATABLE", "MOB_POSTED", "\n"};

/*--------------------------------------------------------------------------
 * Local (file scope) global variables
 *--------------------------------------------------------------------------*/
static int cmd_tell;

/* Quest parsing constants */
#define MAX_QUEST_PARSE_LINES 100 /* Max lines to read before 'S' terminator */
#define QUEST_COMMENT_CHAR '*'    /* Character used for comments in quest files */

static const char *quest_cmd[] = {"list", "history", "join", "leave", "progress", "status", "remove", "clear", "\n"};

static const char *quest_mort_usage = "Uso: quest list | history | progress | join <nn> | leave";

static const char *quest_imm_usage =
    "Uso: quest list | history | progress | join <nn> | leave | status <vnum> | remove <vnum> confirm";
static const char *quest_god_usage =
    "Uso: quest list | history | progress | join <nn> | leave | status <vnum> | remove <vnum> confirm | clear <player>";

/*--------------------------------------------------------------------------*/
/* Utility Functions                                                        */
/*--------------------------------------------------------------------------*/

qst_rnum real_quest(qst_vnum vnum)
{
    qst_rnum bot, top, mid;

    if (total_quests == 0)
        return (NOTHING);

    bot = 0;
    top = total_quests - 1;

    /* Quickly reject out-of-range vnums */
    if (QST_NUM(bot) > vnum || QST_NUM(top) < vnum)
        return (NOTHING);

    /* Perform binary search on quest table (sorted by vnum) */
    while (bot <= top) {
        mid = bot + (top - bot) / 2;

        if (QST_NUM(mid) == vnum)
            return (mid);
        if (QST_NUM(mid) > vnum)
            top = mid - 1;
        else
            bot = mid + 1;
    }
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

/* Find the first available quest for a mob that matches level requirements.
 * Unlike find_available_quest_by_qmnum, this checks level range compatibility.
 * Returns quest vnum if found, NOTHING otherwise. */
qst_vnum find_mob_available_quest_by_qmnum(struct char_data *mob, mob_vnum qm)
{
    qst_rnum rnum;
    int quest_completed, quest_repeatable;

    if (!IS_NPC(mob))
        return NOTHING;

    for (rnum = 0; rnum < total_quests; rnum++) {
        if (qm == QST_MASTER(rnum)) {
            quest_completed = is_complete(mob, QST_NUM(rnum));
            quest_repeatable = IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE);

            /* Only consider quest if it's available (not completed or repeatable) */
            if (!quest_completed || quest_repeatable) {
                /* Check if mob's level is within the quest's level range */
                if (GET_LEVEL(mob) >= QST_MINLEVEL(rnum) && GET_LEVEL(mob) <= QST_MAXLEVEL(rnum)) {
                    return QST_NUM(rnum);
                }
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
    int retval, t[7], safety_counter, line_len;
    char f1[128], buf2[MAX_STRING_LENGTH];

    retval = 0;
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

    /* Read remaining lines until we find the 'S' terminator.
     * Add safeguard to prevent infinite loops from malformed quest files. */
    safety_counter = 0;

    for (;;) {
        if (!get_line(quest_f, line)) {
            log1("Format error in quest #%d: unexpected end of file, expected 'S' terminator\n", nr);
            exit(1);
        }

        /* Safeguard: prevent infinite loops from malformed quest files */
        safety_counter++;
        if (safety_counter > MAX_QUEST_PARSE_LINES) {
            line_len = strlen(line);
            log1("SYSERR: Quest #%d has too many lines without 'S' terminator (possible infinite loop)\n", nr);
            log1("SYSERR: Last line read: '%.80s%s'\n", line, (line_len > 80) ? "..." : "");
            exit(1);
        }

        switch (*line) {
            case 'S':
                total_quests = ++i;
                return;
            default:
                /* Ignore comment lines and empty lines, but log unexpected content */
                if (*line != '\0' && *line != QUEST_COMMENT_CHAR) {
                    line_len = strlen(line);
                    log1("Warning: Quest #%d has unexpected line before 'S' terminator: '%.80s%s'\n", nr, line,
                         (line_len > 80) ? "..." : "");
                }
                break;
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

    /* For AQ_ROOM_FIND and AQ_ROOM_CLEAR quests, replace room number with room name and add zone name */
    if ((QST_TYPE(rnum) == AQ_ROOM_FIND || QST_TYPE(rnum) == AQ_ROOM_CLEAR) && QST_TARGET(rnum) != NOTHING) {
        room_rnum room_rnum_val = real_room(QST_TARGET(rnum));

        if (room_rnum_val != NOWHERE) {
            const char *room_name = world[room_rnum_val].name;
            zone_rnum zone = world[room_rnum_val].zone;
            /* Note: top_of_zone_table is the last valid zone index (not count), so <= is correct */
            const char *zone_name = (zone != NOWHERE && zone >= 0 && zone <= top_of_zone_table && zone_table)
                                        ? zone_table[zone].name
                                        : "Desconhecida";
            char room_with_zone[512];
            char room_num_str[20];
            const char *pos;
            size_t room_num_len;

            /* Create combined string with room name and zone name */
            snprintf(room_with_zone, sizeof(room_with_zone), "%s em %s", room_name, zone_name);

            /* Create string representation of room number to search for */
            snprintf(room_num_str, sizeof(room_num_str), "%d", QST_TARGET(rnum));
            room_num_len = strlen(room_num_str);

            /* Search for the room number in the info message
             * We search for it as a complete number by checking boundaries */
            pos = strstr(info, room_num_str);

            if (pos != NULL) {
                /* Check if this is a complete number match (not part of a larger number) */
                bool is_start_boundary = (pos == info || !isdigit((unsigned char)*(pos - 1)));
                bool is_end_boundary = !isdigit((unsigned char)*(pos + room_num_len));

                if (is_start_boundary && is_end_boundary) {
                    size_t prefix_len = pos - info;
                    size_t suffix_start = prefix_len + room_num_len;
                    size_t room_with_zone_len = strlen(room_with_zone);
                    size_t suffix_len = strlen(info + suffix_start);
                    size_t total_len = prefix_len + room_with_zone_len + suffix_len;

                    /* Check if the formatted string will fit in the buffer (including null terminator) */
                    if (total_len + 1 <= bufsize) {
                        /* Build new message: prefix + room_name_with_zone + suffix */
                        snprintf(buf, bufsize, "%.*s%s%s", (int)prefix_len, info, room_with_zone, info + suffix_start);
                        /* Successfully formatted, return immediately to avoid further processing */
                        return buf;
                    }
                }
            }
        }
    }

    /* For AQ_OBJ_FIND and AQ_OBJ_RETURN quests, add zone name to quest description */
    if ((QST_TYPE(rnum) == AQ_OBJ_FIND || QST_TYPE(rnum) == AQ_OBJ_RETURN) && QST_TARGET(rnum) != NOTHING) {
        obj_rnum obj_rnum_val = real_object(QST_TARGET(rnum));

        if (obj_rnum_val != NOTHING) {
            zone_rnum found_zone = NOWHERE;
            struct obj_data *obj_instance;

            /* First, try to find an actual instance of this object in the world */
            for (obj_instance = object_list; obj_instance; obj_instance = obj_instance->next) {
                if (GET_OBJ_RNUM(obj_instance) == obj_rnum_val) {
                    /* Check if object is in a room */
                    if (obj_instance->in_room != NOWHERE) {
                        found_zone = world[obj_instance->in_room].zone;
                        break;
                    }
                    /* Check if carried by a mob in a valid room */
                    if (obj_instance->carried_by && IS_NPC(obj_instance->carried_by) &&
                        IN_ROOM(obj_instance->carried_by) != NOWHERE) {
                        found_zone = world[IN_ROOM(obj_instance->carried_by)].zone;
                        break;
                    }
                }
            }

            /* Fallback to questmaster's zone if no instance found */
            if (found_zone == NOWHERE) {
                mob_rnum qm_rnum = real_mobile(QST_MASTER(rnum));
                if (qm_rnum != NOBODY) {
                    struct char_data *qm_mob;
                    for (qm_mob = character_list; qm_mob; qm_mob = qm_mob->next) {
                        if (IS_NPC(qm_mob) && GET_MOB_RNUM(qm_mob) == qm_rnum && IN_ROOM(qm_mob) != NOWHERE) {
                            found_zone = world[IN_ROOM(qm_mob)].zone;
                            break;
                        }
                    }
                }
            }

            if (found_zone != NOWHERE && found_zone >= 0 && found_zone <= top_of_zone_table && zone_table) {
                const char *zone_name = zone_table[found_zone].name;
                const char *obj_name = obj_proto[obj_rnum_val].short_description;
                snprintf(temp_buf, sizeof(temp_buf), "%s\r\n\ty(%s em %s)\tn", info, obj_name, zone_name);
                snprintf(buf, bufsize, "%s", temp_buf);
                return buf;
            }
        }
    }

    /* For AQ_MOB_FIND quests, add zone name to quest description */
    if (QST_TYPE(rnum) == AQ_MOB_FIND && QST_TARGET(rnum) != NOTHING) {
        mob_rnum target_mob_rnum = real_mobile(QST_TARGET(rnum));

        if (target_mob_rnum != NOBODY) {
            /* Find the zone where this mob type can be found */
            struct char_data *target_mob;
            for (target_mob = character_list; target_mob; target_mob = target_mob->next) {
                if (IS_NPC(target_mob) && GET_MOB_RNUM(target_mob) == target_mob_rnum &&
                    IN_ROOM(target_mob) != NOWHERE) {
                    zone_rnum zone = world[IN_ROOM(target_mob)].zone;
                    if (zone != NOWHERE && zone >= 0 && zone <= top_of_zone_table && zone_table) {
                        const char *zone_name = zone_table[zone].name;
                        const char *mob_name = GET_NAME(&mob_proto[target_mob_rnum]);
                        snprintf(temp_buf, sizeof(temp_buf), "%s\r\n\ty(%s em %s)\tn", info, mob_name, zone_name);
                        snprintf(buf, bufsize, "%s", temp_buf);
                        return buf;
                    }
                }
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

    /* For AQ_EMOTION_IMPROVE quests, add emotion type and target level info */
    if (QST_TYPE(rnum) == AQ_EMOTION_IMPROVE) {
        int emotion_type = QST_RETURNMOB(rnum);
        int target_level = QST_QUANTITY(rnum);
        const char *emotion_name = "emoção";

        switch (emotion_type) {
            case EMOTION_TYPE_FRIENDSHIP:
                emotion_name = "amizade";
                break;
            case EMOTION_TYPE_TRUST:
                emotion_name = "confiança";
                break;
            case EMOTION_TYPE_LOYALTY:
                emotion_name = "lealdade";
                break;
            case EMOTION_TYPE_HAPPINESS:
                emotion_name = "felicidade";
                break;
            case EMOTION_TYPE_COMPASSION:
                emotion_name = "compaixão";
                break;
            case EMOTION_TYPE_LOVE:
                emotion_name = "afeto";
                break;
        }

        snprintf(temp_buf, sizeof(temp_buf),
                 "%s\r\n\tyDICA: Interaja positivamente através de presentes, socials amigáveis\tn\r\n"
                 "\tyou ajuda em combate para alcançar nível %d de %s.\tn",
                 info, target_level, emotion_name);
        snprintf(buf, bufsize, "%s", temp_buf);
        return buf;
    }

    /* For AQ_MAGIC_GATHER quests, add density and location count info */
    if (QST_TYPE(rnum) == AQ_MAGIC_GATHER && ch && !IS_NPC(ch)) {
        float target_density = QST_QUANTITY(rnum) / 100.0;
        int locations_remaining = GET_QUEST_COUNTER(ch);

        snprintf(temp_buf, sizeof(temp_buf),
                 "%s\r\n\tyDICA: Visite locais com densidade mágica >= %.2f.\tn\r\n"
                 "\tyLocais restantes: %d\tn",
                 info, target_density, locations_remaining);
        snprintf(buf, bufsize, "%s", temp_buf);
        return buf;
    }

    /* For AQ_RESOURCE_GATHER quests, add quantity info */
    if (QST_TYPE(rnum) == AQ_RESOURCE_GATHER && ch && !IS_NPC(ch)) {
        int items_remaining = GET_QUEST_COUNTER(ch);

        snprintf(temp_buf, sizeof(temp_buf),
                 "%s\r\n\tyIMPORTANTE: Entregue os itens ao questmaster ou requisitante.\tn\r\n"
                 "\tyItens restantes: %d\tn",
                 info, items_remaining);
        snprintf(buf, bufsize, "%s", temp_buf);
        return buf;
    }

    /* For AQ_REPUTATION_BUILD quests, add reputation target info */
    if (QST_TYPE(rnum) == AQ_REPUTATION_BUILD) {
        int target_reputation = QST_QUANTITY(rnum);

        snprintf(temp_buf, sizeof(temp_buf),
                 "%s\r\n\tyDICA: Interaja positivamente, complete tarefas e faça trades\tn\r\n"
                 "\typara alcançar nível %d de confiança.\tn",
                 info, target_reputation);
        snprintf(buf, bufsize, "%s", temp_buf);
        return buf;
    }

    /* For AQ_SHOP_BUY quests, add item type and quantity info */
    if (QST_TYPE(rnum) == AQ_SHOP_BUY && ch && !IS_NPC(ch)) {
        int items_remaining = GET_QUEST_COUNTER(ch);
        obj_vnum item_vnum = QST_RETURNMOB(rnum);
        const char *item_name = "item desconhecido";
        obj_rnum item_rnum;

        /* SIGSEGV protection: validate item_rnum before accessing obj_proto array */
        item_rnum = real_object(item_vnum);
        if (item_rnum != NOTHING && item_rnum >= 0 && obj_proto) {
            item_name = obj_proto[item_rnum].short_description;
        }

        snprintf(temp_buf, sizeof(temp_buf), "%s\r\n\tyDICA: Compre %s em lojas. Itens restantes: %d\tn", info,
                 item_name, items_remaining);
        snprintf(buf, bufsize, "%s", temp_buf);
        return buf;
    }

    /* For AQ_SHOP_SELL quests, add item type and quantity info */
    if (QST_TYPE(rnum) == AQ_SHOP_SELL && ch && !IS_NPC(ch)) {
        int items_remaining = GET_QUEST_COUNTER(ch);
        obj_vnum item_vnum = QST_RETURNMOB(rnum);
        const char *item_name = "item desconhecido";
        obj_rnum item_rnum;

        /* SIGSEGV protection: validate item_rnum before accessing obj_proto array */
        item_rnum = real_object(item_vnum);
        if (item_rnum != NOTHING && item_rnum >= 0 && obj_proto) {
            item_name = obj_proto[item_rnum].short_description;
        }

        snprintf(temp_buf, sizeof(temp_buf), "%s\r\n\tyDICA: Venda %s em lojas. Itens restantes: %d\tn", info,
                 item_name, items_remaining);
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

/** Helper to find the questmaster mob present in the same room as the entity.
 *  This centralizes the lookup logic so it can be reused by multiple quest paths.
 *
 * @param entity The character whose room we search (player or mob)
 * @param rnum   The quest rnum whose questmaster we are looking for
 * @return Pointer to the questmaster in the room, or NULL if not found/invalid
 */
static struct char_data *find_questmaster_in_room(struct char_data *entity, qst_rnum rnum)
{
    mob_rnum questmaster_rnum;
    struct char_data *temp_char;

    if (!entity || IN_ROOM(entity) == NOWHERE || rnum == NOTHING)
        return NULL;

    questmaster_rnum = real_mobile(QST_MASTER(rnum));
    if (questmaster_rnum == NOBODY)
        return NULL;

    /* Look for questmaster in current room */
    for (temp_char = world[IN_ROOM(entity)].people; temp_char; temp_char = temp_char->next_in_room) {
        if (IS_NPC(temp_char) && GET_MOB_RNUM(temp_char) == questmaster_rnum) {
            /* For mob quests, skip if questmaster is the mob itself */
            if (IS_NPC(entity) && temp_char == entity)
                continue;
            return temp_char;
        }
    }

    return NULL;
}

/** Trigger quest failure emotion memory for questmaster
 * @param entity The character who failed the quest (player or mob)
 * @param rnum The quest rnum that was failed
 */
static void trigger_quest_failure_emotion(struct char_data *entity, qst_rnum rnum)
{
    struct char_data *questmaster;

    if (!entity || !CONFIG_MOB_CONTEXTUAL_SOCIALS || IN_ROOM(entity) == NOWHERE || rnum == NOTHING)
        return;

    /* Find the questmaster mob and update their emotions */
    questmaster = find_questmaster_in_room(entity, rnum);
    if (questmaster && questmaster->ai_data) {
        update_mob_emotion_quest_failed(questmaster, entity);
    }
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

            /* Emotion trigger: Quest failure (Quest-Related 2.4) */
            trigger_quest_failure_emotion(ch, rnum);

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

            /* Emotion trigger: Quest failure (Quest-Related 2.4) */
            trigger_quest_failure_emotion(ch, rnum);

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
            int gold_reward = QST_GOLD(rnum);

            /* Apply emotion-based gold reward bonus for high trust questmasters */
            if (CONFIG_MOB_CONTEXTUAL_SOCIALS) {
                mob_rnum questmaster_rnum = real_mobile(QST_MASTER(rnum));
                if (questmaster_rnum != NOBODY && IN_ROOM(ch) != NOWHERE) {
                    struct char_data *questmaster = NULL;
                    struct char_data *temp_char;
                    for (temp_char = world[IN_ROOM(ch)].people; temp_char; temp_char = temp_char->next_in_room) {
                        if (IS_NPC(temp_char) && GET_MOB_RNUM(temp_char) == questmaster_rnum && temp_char->ai_data) {
                            questmaster = temp_char;
                            break;
                        }
                    }
                    /* High trust gives 15% better rewards - use hybrid emotion system */
                    if (questmaster && questmaster->ai_data) {
                        int effective_trust = get_effective_emotion_toward(questmaster, ch, EMOTION_TYPE_TRUST);
                        if (effective_trust >= CONFIG_EMOTION_QUEST_TRUST_HIGH_THRESHOLD) {
                            gold_reward = (int)(gold_reward * 1.15);
                        }
                    }
                }
            }

            if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD)) {
                happy_gold = (int)(gold_reward * (((float)(100 + HAPPY_GOLD)) / (float)100));
                happy_gold = MAX(happy_gold, 0);
                increase_gold(ch, happy_gold);
                send_to_char(ch, "Você recebeu %d moedas pelos seus serviços.\r\n", happy_gold);
            } else {
                increase_gold(ch, gold_reward);
                send_to_char(ch, "Você recebeu %d moedas pelos seus serviços.\r\n", gold_reward);
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
                    /* Remove NOLOCATE flag from reward item when giving it to player */
                    REMOVE_BIT_AR(GET_OBJ_EXTRA(new_obj), ITEM_NOLOCATE);
                    obj_to_char(new_obj, ch);
                    send_to_char(ch, "Você foi presentead%s com %s%s pelos seus serviços.\r\n", OA(ch),
                                 GET_OBJ_SHORT(new_obj), CCNRM(ch, C_NRM));
                }
            }
        }
        /* Only add to completed quests history if the quest is not repeatable.
         * For non-repeatable quests (both regular and mob-posted), add them to history.
         * Note: Non-repeatable mob-posted quests are deleted from the system via
         * cleanup_completed_wishlist_quest(), so storing them in history won't cause
         * vnum conflicts for future quests. */
        if (!IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE))
            add_completed_quest(ch, vnum);

        /* Notify the room that the character completed a quest */
        act("$n completou uma busca.", TRUE, ch, NULL, NULL, TO_ROOM);

        /* Emotion trigger: Quest completion (Quest-Related 2.4) */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && IN_ROOM(ch) != NOWHERE && rnum != NOTHING) {
            /* Find the questmaster mob and update their emotions */
            struct char_data *questmaster = find_questmaster_in_room(ch, rnum);
            if (questmaster && questmaster->ai_data) {
                update_mob_emotion_quest_completed(questmaster, ch);
            }
        }

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
                    /* Mark item with NOLOCATE to prevent locate object exploit */
                    SET_BIT_AR(GET_OBJ_EXTRA(object), ITEM_NOLOCATE);
                    /* Set timer to 28 ticks (1 MUD day) - negative value means "remove flag, don't extract" */
                    GET_OBJ_TIMER(object) = -28;
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

                            /* Mark item with NOLOCATE to prevent locate object exploit */
                            SET_BIT_AR(GET_OBJ_EXTRA(object), ITEM_NOLOCATE);
                            /* Set timer to 28 ticks (1 MUD day) - negative value means "remove flag, don't extract" */
                            GET_OBJ_TIMER(object) = -28;

                            obj_to_char(object, original_requester);
                            act("$n entrega $p para quem solicitou.", FALSE, vict, object, NULL, TO_ROOM);
                            /* Safety check: act() can trigger DG scripts which may extract object or characters */
                            if (MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET))
                                break;
                            if (MOB_FLAGGED(original_requester, MOB_NOTDEADYET) ||
                                PLR_FLAGGED(original_requester, PLR_NOTDEADYET))
                                break;
                            /* Safety check: object may have been extracted by DG scripts */
                            if (object->carried_by != original_requester)
                                break;
                            act("$n recebe $p de $N.", FALSE, original_requester, object, vict, TO_ROOM);
                            /* Safety check after second act() call */
                            if (MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET))
                                break;
                            if (MOB_FLAGGED(original_requester, MOB_NOTDEADYET) ||
                                PLR_FLAGGED(original_requester, PLR_NOTDEADYET))
                                break;
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
        case AQ_EMOTION_IMPROVE:
            /* Check if player improved specific emotion with target mob */
            if (!IS_NPC(ch) && vict && IS_NPC(vict) && (ch != vict)) {
                /* Check if this is the target mob */
                if (QST_TARGET(rnum) == GET_MOB_VNUM(vict)) {
                    /* Get the emotion type and target level from quest values */
                    int emotion_type = QST_RETURNMOB(rnum); /* Reusing RETURNMOB for emotion type */
                    int target_level = QST_QUANTITY(rnum);  /* Target emotion level */

                    /* Get current emotion level toward player */
                    int current_level = get_effective_emotion_toward(vict, ch, emotion_type);

                    /* Check if target level reached */
                    if (current_level >= target_level) {
                        generic_complete_quest(ch);
                    }
                }
            }
            break;
        case AQ_MAGIC_GATHER:
            /* Check if player visited location with sufficient magical density */
            if (!IS_NPC(ch) && type == AQ_ROOM_FIND) {
                /* Get current magical density */
                float current_density = calculate_mana_density(ch);
                /* Target density threshold stored in value[6] as int (multiplied by 100) */
                float target_density = QST_QUANTITY(rnum) / 100.0;

                if (current_density >= target_density) {
                    /* Validate counter before decrement */
                    if (GET_QUEST_COUNTER(ch) > 0) {
                        /* Decrement counter for number of locations to visit */
                        if (--GET_QUEST_COUNTER(ch) <= 0) {
                            generic_complete_quest(ch);
                        } else {
                            send_to_char(ch, "Você coletou energia mágica. Ainda precisa visitar %d locais.\r\n",
                                         GET_QUEST_COUNTER(ch));
                        }
                    } else {
                        log1("EXPLOIT WARNING: AQ_MAGIC_GATHER counter already at 0 for %s", GET_NAME(ch));
                    }
                }
            }
            break;
        case AQ_DELIVERY:
            /* Check if player traded required item with target mob */
            if (!IS_NPC(ch) && vict && IS_NPC(vict) && object && (ch != vict)) {
                /* Check if this is the target mob and correct item */
                if (GET_MOB_VNUM(vict) == QST_TARGET(rnum) && GET_OBJ_VNUM(object) == QST_RETURNMOB(rnum)) {
                    /* Verify the object is in the NPC's inventory (was traded/given) */
                    struct obj_data *obj_check;
                    bool has_object = false;

                    if (vict->carrying) {
                        for (obj_check = vict->carrying; obj_check; obj_check = obj_check->next_content) {
                            if (obj_check == object) {
                                has_object = true;
                                break;
                            }
                        }
                    }

                    if (has_object) {
                        /* Mark item with NOLOCATE to prevent locate object exploit */
                        SET_BIT_AR(GET_OBJ_EXTRA(object), ITEM_NOLOCATE);
                        /* Set timer to 28 ticks (1 MUD day) - negative value means "remove flag, don't extract" */
                        GET_OBJ_TIMER(object) = -28;
                        generic_complete_quest(ch);
                    }
                }
            }
            break;
        case AQ_RESOURCE_GATHER:
            /* Player must deliver multiple items to questmaster or requester */
            if (!IS_NPC(ch) && vict && IS_NPC(vict) && object && GET_OBJ_VNUM(object) == QST_TARGET(rnum)) {
                /* Check if the object is in the NPC's inventory (was given/traded) */
                struct obj_data *obj_check;
                bool has_object = false;

                if (vict->carrying) {
                    for (obj_check = vict->carrying; obj_check; obj_check = obj_check->next_content) {
                        if (obj_check == object) {
                            has_object = true;
                            break;
                        }
                    }
                }

                if (!has_object) {
                    /* Object not in NPC's inventory - don't count it */
                    break;
                }

                /* Check if giving to questmaster or original requester */
                if (GET_MOB_VNUM(vict) == QST_MASTER(rnum)) {
                    /* Given to questmaster - transfer to original requester if different */
                    mob_rnum original_requester_rnum = real_mobile(QST_RETURNMOB(rnum));

                    /* Mark item with NOLOCATE to prevent locate object exploit */
                    SET_BIT_AR(GET_OBJ_EXTRA(object), ITEM_NOLOCATE);
                    /* Set timer to 28 ticks (1 MUD day) - negative value means "remove flag, don't extract" */
                    GET_OBJ_TIMER(object) = -28;

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

                            /* Safety check: act() can trigger DG scripts which may extract object or characters */
                            if (!MOB_FLAGGED(vict, MOB_NOTDEADYET) && !PLR_FLAGGED(vict, PLR_NOTDEADYET) &&
                                !MOB_FLAGGED(original_requester, MOB_NOTDEADYET) &&
                                !PLR_FLAGGED(original_requester, PLR_NOTDEADYET) &&
                                object->carried_by == original_requester) {
                                act("$n transfere $p para o requisitante.", FALSE, vict, object, NULL, TO_ROOM);
                            }
                        }
                    }

                    /* Count this delivery - validate counter first */
                    if (GET_QUEST_COUNTER(ch) > 0) {
                        if (--GET_QUEST_COUNTER(ch) <= 0) {
                            /* All resources delivered, complete quest */
                            generic_complete_quest(ch);
                        } else {
                            /* Still need more items */
                            send_to_char(ch, "Você entregou 1 item. Ainda precisa de %d mais.\r\n",
                                         GET_QUEST_COUNTER(ch));
                        }
                    } else {
                        log1("EXPLOIT WARNING: AQ_RESOURCE_GATHER counter already at 0 for %s", GET_NAME(ch));
                    }
                } else if (GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum)) {
                    /* Given directly to original requester */
                    /* Mark item with NOLOCATE to prevent locate object exploit */
                    SET_BIT_AR(GET_OBJ_EXTRA(object), ITEM_NOLOCATE);
                    /* Set timer to 28 ticks (1 MUD day) - negative value means "remove flag, don't extract" */
                    GET_OBJ_TIMER(object) = -28;

                    /* Count this delivery - validate counter first */
                    if (GET_QUEST_COUNTER(ch) > 0) {
                        if (--GET_QUEST_COUNTER(ch) <= 0) {
                            /* All resources delivered, complete quest */
                            generic_complete_quest(ch);
                        } else {
                            /* Still need more items */
                            send_to_char(ch, "Você entregou 1 item. Ainda precisa de %d mais.\r\n",
                                         GET_QUEST_COUNTER(ch));
                        }
                    } else {
                        log1("EXPLOIT WARNING: AQ_RESOURCE_GATHER counter already at 0 for %s", GET_NAME(ch));
                    }
                }
            }
            break;
        case AQ_REPUTATION_BUILD:
            /* Reputation build quests check reputation level with target mob or zone */
            if (!IS_NPC(ch) && vict && IS_NPC(vict)) {
                /* Check if this is the target mob */
                if (GET_MOB_VNUM(vict) == QST_TARGET(rnum)) {
                    /* For now, use trust emotion as reputation proxy */
                    int current_trust = get_effective_emotion_toward(vict, ch, EMOTION_TYPE_TRUST);
                    int target_reputation = QST_QUANTITY(rnum);

                    if (current_trust >= target_reputation) {
                        generic_complete_quest(ch);
                    }
                }
            }
            break;
        case AQ_SHOP_BUY:
            /* Shop buy quests are triggered from shopping_buy() in shop.c */
            /* Counter decrements there when buying matching items */
            break;
        case AQ_SHOP_SELL:
            /* Shop sell quests are triggered from shopping_sell() in shop.c */
            /* Counter decrements there when selling matching items */
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
    char buf[MAX_STRING_LENGTH];
    size_t len = 0;

    /* Check if player has a descriptor for pagination */
    if (!ch->desc) {
        send_to_char(ch, "Você não pode ver o histórico de buscas no momento.\r\n");
        return;
    }

    len += snprintf(buf + len, sizeof(buf) - len,
                    "Você completou as seguintes buscas:\r\n"
                    "Num.  Descrição                                            Responsável\r\n"
                    "----- ---------------------------------------------------- -----------\r\n");
    for (i = 0; i < GET_NUM_QUESTS(ch) && len < sizeof(buf) - 1; i++) {
        if ((rnum = real_quest(ch->player_specials->saved.completed_quests[i])) != NOTHING)
            len += snprintf(
                buf + len, sizeof(buf) - len, "\tg%4d\tn) \tc%-52.52s\tn \ty%s\tn\r\n", ++counter, QST_DESC(rnum),
                (real_mobile(QST_MASTER(rnum)) == NOBODY) ? "Desconhecido"
                                                          : GET_NAME(&mob_proto[(real_mobile(QST_MASTER(rnum)))]));
        else
            len += snprintf(buf + len, sizeof(buf) - len, "\tg%4d\tn) \tcBusca desconhecida! Não existe mais!\tn\r\n",
                            ++counter);
    }
    if (!counter && len < sizeof(buf) - 1)
        len += snprintf(buf + len, sizeof(buf) - len, "Você não completou nenhuma busca ainda.\r\n");

    /* Use page_string for paginated output */
    page_string(ch->desc, buf, TRUE);
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
        if (QST_PREV(rnum) != NOTHING) {
            qst_rnum prev_rnum = real_quest(QST_PREV(rnum));
            if (prev_rnum != NOTHING)
                send_to_char(ch, "Você precisa completar a busca %s primeiro.\r\n", QST_NAME(prev_rnum));
        }
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
        /* Emotion trigger: Quest failure (Quest-Related 2.4) - abandonment */
        trigger_quest_failure_emotion(ch, rnum);

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
    char buf[MAX_STRING_LENGTH];
    size_t len = 0;

    /* Check if player has a descriptor for pagination */
    if (!ch->desc) {
        send_to_char(ch, "Você não pode ver a lista de buscas no momento.\r\n");
        return;
    }

    len += snprintf(buf + len, sizeof(buf) - len,
                    "A lista de buscas disponiveis:\r\n"
                    "Num.  Descrição                   Dificuldade Níveis    Feita?\r\n"
                    "----- ---------------------------- ----------- --------- ------\r\n");

    /* First, show regular quests assigned to this questmaster */
    for (rnum = 0; rnum < total_quests && len < sizeof(buf) - 1; rnum++) {
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

                len += snprintf(buf + len, sizeof(buf) - len,
                                "\tg%4d\tn) \tc%-28.28s\tn \ty%-11s\tn \tw%3d-%-3d\tn   \ty(%s)\tn\r\n", ++counter,
                                QST_NAME(rnum), get_quest_difficulty_string(rnum), QST_MINLEVEL(rnum),
                                QST_MAXLEVEL(rnum), (quest_completed ? "Sim" : "Não "));
            }
        }
    }

    /* Then, show temporary quests if this mob is a temporary questmaster */
    if (IS_TEMP_QUESTMASTER(qm) && GET_NUM_TEMP_QUESTS(qm) > 0 && len < sizeof(buf) - 1) {
        for (i = 0; i < GET_NUM_TEMP_QUESTS(qm) && len < sizeof(buf) - 1; i++) {
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

                len += snprintf(buf + len, sizeof(buf) - len,
                                "\tg%4d\tn) \tc%-28.28s\tn \ty%-11s\tn \tw%3d-%-3d\tn   \ty(%s)\tn\r\n", ++counter,
                                QST_NAME(rnum), get_quest_difficulty_string(rnum), QST_MINLEVEL(rnum),
                                QST_MAXLEVEL(rnum), (quest_completed ? "Sim" : "Não "));
            }
        }
    }

    if (!counter && len < sizeof(buf) - 1) {
        len += snprintf(buf + len, sizeof(buf) - len, "Não temos buscas disponiveis no momento, %s!\r\n", GET_NAME(ch));

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

            if (len < sizeof(buf) - 1)
                len += snprintf(buf + len, sizeof(buf) - len,
                                "\tc[DEBUG: QM %d has %d regular quests, %d temp quests, is_temp_qm=%s]\tn\r\n",
                                qm_vnum, total_regular, total_temp, IS_TEMP_QUESTMASTER(qm) ? "YES" : "NO");
        }
    }

    /* Use page_string for paginated output */
    page_string(ch->desc, buf, TRUE);
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

    /* Check emotion-based quest restrictions for mobs with AI - use hybrid emotion system */
    if (CONFIG_MOB_CONTEXTUAL_SOCIALS && IS_NPC(qm) && qm->ai_data && !IS_NPC(ch)) {
        /* Low trust makes questmaster refuse to give quests */
        int effective_trust = get_effective_emotion_toward(qm, ch, EMOTION_TYPE_TRUST);
        if (effective_trust < CONFIG_EMOTION_QUEST_TRUST_LOW_THRESHOLD) {
            snprintf(buf, sizeof(buf), "%s diz, 'Eu não confio em você o suficiente para lhe dar uma busca, %s.'",
                     GET_NAME(qm), GET_NAME(ch));
            send_to_char(ch, "%s\r\n", buf);
            save_char(ch);
            return;
        }
    }

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
    else {
        qst_rnum prev_rnum = real_quest(QST_PREV(rnum));
        if (prev_rnum != NOTHING)
            send_to_char(ch, " [\ty%5d\tn] \tc%s\tn\r\n", QST_PREV(rnum), QST_DESC(prev_rnum));
        else
            send_to_char(ch, " [\ty%5d\tn] \tc(unknown quest)\tn\r\n", QST_PREV(rnum));
    }
    send_to_char(ch, "Next  :");
    if (QST_NEXT(rnum) == NOTHING)
        send_to_char(ch, " \tyNone.\tn\r\n");
    else {
        qst_rnum next_rnum = real_quest(QST_NEXT(rnum));
        if (next_rnum != NOTHING)
            send_to_char(ch, " [\ty%5d\tn] \tc%s\tn\r\n", QST_NEXT(rnum), QST_DESC(next_rnum));
        else
            send_to_char(ch, " [\ty%5d\tn] \tc(unknown quest)\tn\r\n", QST_NEXT(rnum));
    }
}

/* Clear a player's current quest (GOD+ command) */
static void quest_clear(struct char_data *ch, char *argument)
{
    struct char_data *vict;
    qst_rnum rnum;
    char arg[MAX_INPUT_LENGTH];

    if (GET_LEVEL(ch) < LVL_GOD) {
        send_to_char(ch, "You must be at least level GOD to clear player quests.\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Uso: quest clear <player>\r\n");
        return;
    }

    /* Find the player */
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
        send_to_char(ch, "Não há ninguém com esse nome.\r\n");
        return;
    }

    /* Safety check: validate vict pointer after getting it */
    if (!vict) {
        send_to_char(ch, "Erro ao encontrar o jogador.\r\n");
        return;
    }

    /* Don't allow clearing NPCs' quests */
    if (IS_NPC(vict)) {
        send_to_char(ch, "Você não pode limpar a busca de um mob.\r\n");
        return;
    }

    /* Check if player has an active quest */
    if (GET_QUEST(vict) == NOTHING) {
        send_to_char(ch, "%s não tem nenhuma busca ativa.\r\n", GET_NAME(vict) ? GET_NAME(vict) : "Jogador");
        return;
    }

    /* Get quest info before clearing */
    rnum = real_quest(GET_QUEST(vict));
    if (rnum != NOTHING && rnum >= 0 && rnum < total_quests) {
        /* Safety check: validate quest name exists */
        const char *quest_name = QST_NAME(rnum) ? QST_NAME(rnum) : "Quest desconhecida";
        const char *vict_name = GET_NAME(vict) ? GET_NAME(vict) : "Jogador";
        send_to_char(ch, "Limpando busca de %s: [\ty%d\tn] \tc%s\tn\r\n", vict_name, GET_QUEST(vict), quest_name);
        send_to_char(vict, "Sua busca foi cancelada por um imortal.\r\n");
    } else {
        const char *vict_name = GET_NAME(vict) ? GET_NAME(vict) : "Jogador";
        send_to_char(ch, "Limpando busca de %s (busca inválida vnum %d).\r\n", vict_name, GET_QUEST(vict));
        send_to_char(vict, "Sua busca foi cancelada por um imortal.\r\n");
    }

    /* Clear the quest without penalty */
    clear_quest(vict);
    save_char(vict);

    send_to_char(ch, "\tGBusca limpa com sucesso.\tn\r\n");
    /* Safety check: validate names before mudlog */
    mudlog(NRM, LVL_GOD, TRUE, "(GC) %s cleared %s's quest.", GET_NAME(ch) ? GET_NAME(ch) : "Unknown",
           GET_NAME(vict) ? GET_NAME(vict) : "Unknown");
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
            case SCMD_QUEST_CLEAR:
                if (GET_LEVEL(ch) < LVL_GOD)
                    send_to_char(ch, "%s\r\n", usage_msg);
                else
                    quest_clear(ch, arg2);
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
    int genetic_component = 0;
    int emotion_modifier = 0;
    int level_diff;

    if (!IS_NPC(mob) || !mob->ai_data || rnum == NOTHING)
        return 0;

    /* Base capability from quest genetics (scaled to 0-60 to make room for emotions and other bonuses) */
    genetic_component = ((GET_GENQUEST(mob) + GET_GENADVENTURER(mob)) * 60) / 200;
    capability = genetic_component;

    /* EMOTION SYSTEM: Adjust capability based on curiosity */
    if (CONFIG_MOB_CONTEXTUAL_SOCIALS) {
        /* High curiosity increases quest acceptance significantly */
        if (mob->ai_data->emotion_curiosity >= 70) {
            emotion_modifier += 20; /* High curiosity: +20% chance */
        } else if (mob->ai_data->emotion_curiosity >= 50) {
            emotion_modifier += 10; /* Moderate curiosity: +10% chance */
        } else if (mob->ai_data->emotion_curiosity >= 30) {
            emotion_modifier += 5; /* Medium-low curiosity: +5% chance */
        }
        /* Low curiosity slightly reduces quest acceptance */
        else if (mob->ai_data->emotion_curiosity <= 20) {
            emotion_modifier -= 10; /* Very low curiosity: -10% chance */
        }
        /* curiosity 21-29 has no modifier (neutral range) */

        capability += emotion_modifier;
    }

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
            capability += GET_GENBRAVE(mob); /* Combat quests need bravery */
            break;
        case AQ_PLAYER_KILL:
            /* Player kill quests have increased priority for capable mobs */
            capability += GET_GENBRAVE(mob);     /* Combat quests need bravery */
            capability += GET_GENQUEST(mob) / 2; /* Additional boost from quest tendency */
            break;
        case AQ_OBJ_FIND:
        case AQ_ROOM_FIND:
        case AQ_MOB_FIND:
            capability += GET_GENROAM(mob); /* Exploration quests need roaming */
            break;
        case AQ_MOB_SAVE:
            /* Saving mobs requires bravery (to fight off threats) and compassion */
            capability += (GET_GENBRAVE(mob) + GET_GENHEALING(mob)) / 2;
            break;
        case AQ_OBJ_RETURN:
        case AQ_DELIVERY:
            /* Delivery quests need roaming and quest tendency */
            capability += (GET_GENROAM(mob) + GET_GENQUEST(mob)) / 2;
            break;
        case AQ_ROOM_CLEAR:
            capability += (GET_GENBRAVE(mob) + GET_GENGROUP(mob)) / 2; /* Need both */
            break;
        case AQ_MOB_ESCORT:
            /* Escort quests require bravery (to protect) and group tendency */
            capability += (GET_GENBRAVE(mob) + GET_GENGROUP(mob)) / 2;
            break;
        case AQ_EMOTION_IMPROVE:
            /* Emotion quests benefit from healing tendency (social skills) */
            capability += GET_GENHEALING(mob);
            break;
        case AQ_MAGIC_GATHER:
            /* Magic gathering requires adventurer and roaming tendencies */
            capability += (GET_GENADVENTURER(mob) + GET_GENROAM(mob)) / 2;
            break;
        case AQ_RESOURCE_GATHER:
            /* Resource gathering needs loot and quest tendencies */
            capability += (GET_GENLOOT(mob) + GET_GENQUEST(mob)) / 2;
            break;
        case AQ_REPUTATION_BUILD:
            /* Reputation building benefits from trade and healing (social) tendencies */
            capability += (GET_GENTRADE(mob) + GET_GENHEALING(mob)) / 2;
            break;
        case AQ_SHOP_BUY:
        case AQ_SHOP_SELL:
            /* Shop quests require trade tendency */
            capability += GET_GENTRADE(mob);
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

/* Check if a mob can accept a quest when forced by builder/goal setting
 * This is less restrictive than mob_should_accept_quest - allows any quest
 * from questmasters, not just mob-posted quests */
bool mob_can_accept_quest_forced(struct char_data *mob, qst_rnum rnum)
{
    if (!IS_NPC(mob) || !mob->ai_data || rnum == NOTHING)
        return FALSE;

    /* Don't accept if already on a quest */
    if (GET_MOB_QUEST(mob) != NOTHING)
        return FALSE;

    /* Check if mob's level is appropriate for the quest */
    if (GET_LEVEL(mob) < QST_MINLEVEL(rnum) || GET_LEVEL(mob) > QST_MAXLEVEL(rnum))
        return FALSE;

    /* MOB_NOKILL mobs cannot request escort quests (they can't die, so quest can't fail properly) */
    if (QST_TYPE(rnum) == AQ_MOB_ESCORT && MOB_FLAGGED(mob, MOB_NOKILL))
        return FALSE;

    return TRUE;
}

/* Set a quest for a mob */
void set_mob_quest(struct char_data *mob, qst_rnum rnum)
{
    if (!IS_NPC(mob) || !mob->ai_data || rnum == NOTHING)
        return;

    mob->ai_data->current_quest = QST_NUM(rnum);
    mob->ai_data->quest_timer = QST_TIME(rnum);
    mob->ai_data->quest_counter = QST_QUANTITY(rnum) > 0 ? QST_QUANTITY(rnum) : 1;

    /* Automatically set the goal to GOAL_COMPLETE_QUEST so the mob actively pursues the quest.
     * This ensures that when a mob accepts a quest, it will immediately start working on it
     * rather than waiting for a random chance to trigger quest processing. */
    mob->ai_data->current_goal = GOAL_COMPLETE_QUEST;
    mob->ai_data->goal_timer = 0;

    /* Initialize goal fields based on quest type */
    switch (QST_TYPE(rnum)) {
        case AQ_OBJ_FIND:
        case AQ_OBJ_RETURN:
        case AQ_DELIVERY:
        case AQ_RESOURCE_GATHER:
            /* For object quests, set the target item vnum */
            mob->ai_data->goal_item_vnum = QST_TARGET(rnum);
            mob->ai_data->goal_destination = NOWHERE;
            mob->ai_data->goal_target_mob_rnum = NOBODY;
            break;
        case AQ_ROOM_FIND:
        case AQ_ROOM_CLEAR:
        case AQ_MAGIC_GATHER: {
            /* For room quests, set the destination room with validation */
            room_rnum dest = real_room(QST_TARGET(rnum));
            mob->ai_data->goal_destination = (dest != NOWHERE) ? dest : NOWHERE;
            mob->ai_data->goal_item_vnum = NOTHING;
            mob->ai_data->goal_target_mob_rnum = NOBODY;
            break;
        }
        case AQ_MOB_FIND:
        case AQ_MOB_KILL:
        case AQ_MOB_KILL_BOUNTY:
        case AQ_MOB_SAVE:
        case AQ_MOB_ESCORT:
        case AQ_EMOTION_IMPROVE: {
            /* For mob quests, set the target mob with validation */
            mob_rnum target = real_mobile(QST_TARGET(rnum));
            mob->ai_data->goal_target_mob_rnum = (target != NOBODY) ? target : NOBODY;
            mob->ai_data->goal_item_vnum = NOTHING;
            mob->ai_data->goal_destination = NOWHERE;
            break;
        }
        case AQ_SHOP_BUY:
        case AQ_SHOP_SELL:
            /* For shop quests, set the target item vnum - mob will find shop via AI */
            mob->ai_data->goal_item_vnum = QST_TARGET(rnum);
            mob->ai_data->goal_destination = NOWHERE;
            mob->ai_data->goal_target_mob_rnum = NOBODY;
            break;
        default:
            /* For other quest types (AQ_PLAYER_KILL, AQ_REPUTATION_BUILD, etc.)
             * initialize to safe defaults - these may not be suitable for mobs */
            mob->ai_data->goal_destination = NOWHERE;
            mob->ai_data->goal_item_vnum = NOTHING;
            mob->ai_data->goal_target_mob_rnum = NOBODY;
            break;
    }
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

    /* Emotion trigger: Quest failure (Quest-Related 2.4) - mob failing their quest */
    trigger_quest_failure_emotion(mob, rnum);

    /* Clear the quest from the mob's state */
    clear_mob_quest(mob);

    act("$n parece desapontado.", TRUE, mob, NULL, NULL, TO_ROOM);

    /* Note: Failed quests remain in the queue so another player or mob can try them */
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
    if (rnum == NOTHING) {
        /* Quest no longer exists, just clear mob's quest state */
        log1("SYSERR: mob_complete_quest: Mob %s tried to complete non-existent quest vnum %d", GET_NAME(mob), vnum);
        clear_mob_quest(mob);
        return;
    }

    /* Give gold reward */
    if (QST_GOLD(rnum)) {
        increase_gold(mob, QST_GOLD(rnum));
    }

    /* Give experience reward (mobs can gain exp too) */
    if (QST_EXP(rnum)) {
        gain_exp(mob, QST_EXP(rnum));
    }

    /* Give object reward */
    if (QST_OBJ(rnum) != NOTHING) {
        if (real_object(QST_OBJ(rnum)) != NOTHING) {
            new_obj = read_object(QST_OBJ(rnum), VIRTUAL);
            if (new_obj) {
                obj_to_char(new_obj, mob);
                /* Remove reward item from mob's wishlist - mob got what it wanted */
                if (mob->ai_data) {
                    remove_item_from_wishlist(mob, GET_OBJ_VNUM(new_obj));
                }
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

    /* Notify the room that the mob completed a quest (similar to player flow) */
    act("$n completou uma busca.", TRUE, mob, NULL, NULL, TO_ROOM);
    act("$n parece satisfeito com sua tarefa concluída.", TRUE, mob, NULL, NULL, TO_ROOM);

    /* Emotion trigger: Quest completion - update questmaster emotions if nearby */
    if (CONFIG_MOB_CONTEXTUAL_SOCIALS && IN_ROOM(mob) != NOWHERE) {
        struct char_data *questmaster = find_questmaster_in_room(mob, rnum);
        if (questmaster && questmaster->ai_data) {
            update_mob_emotion_quest_completed(questmaster, mob);
        }
    }

    /* Clear the quest from the mob's state */
    clear_mob_quest(mob);

    /* Cleanup: If this is a mob-posted quest and not repeatable, delete it from the system
     * to free the queue slot for another quest. This is similar to wishlist quest cleanup
     * for players (cleanup_completed_wishlist_quest). */
    {
        long quest_flags = QST_FLAGS(rnum);
        if (IS_SET(quest_flags, AQ_MOB_POSTED) && !IS_SET(quest_flags, AQ_REPEATABLE)) {
            log1("MOB QUEST: Mob %s completed quest %d, removing from queue", GET_NAME(mob), vnum);
            if (!delete_quest(rnum)) {
                log1("SYSERR: MOB QUEST: Failed to delete quest %d from queue", vnum);
            }
        }
    }

    /* Note: Unlike players, mobs don't have quest history (add_completed_quest) or chain quests (next_quest) */
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

    /* Validate ch's room before any world array access */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return;

    vnum = GET_MOB_QUEST(ch);
    if (vnum == NOTHING)
        return;

    rnum = real_quest(vnum);
    if (rnum == NOTHING) {
        /* Quest no longer exists - was deleted. Clear the mob's quest reference. */
        clear_mob_quest(ch);
        return;
    }

    switch (QST_TYPE(rnum)) {
        case AQ_MOB_KILL:
        case AQ_MOB_KILL_BOUNTY:
            if (vict && IS_NPC(vict) && (ch != vict))
                if (QST_TARGET(rnum) == GET_MOB_VNUM(vict)) {
                    if (--ch->ai_data->quest_counter <= 0)
                        mob_complete_quest(ch);
                }
            break;
        case AQ_PLAYER_KILL:
            if (vict && !IS_NPC(vict) && (ch != vict)) {
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
        case AQ_SHOP_BUY:
            /* Check if mob has purchased the required item */
            if (object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum))) {
                if (--ch->ai_data->quest_counter <= 0)
                    mob_complete_quest(ch);
            }
            break;
        case AQ_SHOP_SELL:
            /* Selling quests complete when item is no longer in inventory */
            {
                struct obj_data *obj;
                bool has_item = FALSE;
                for (obj = ch->carrying; obj; obj = obj->next_content) {
                    if (GET_OBJ_VNUM(obj) == QST_TARGET(rnum)) {
                        has_item = TRUE;
                        break;
                    }
                }
                if (!has_item) {
                    if (--ch->ai_data->quest_counter <= 0)
                        mob_complete_quest(ch);
                }
            }
            break;
        case AQ_DELIVERY:
            /* Similar to OBJ_RETURN but tracks delivery */
            if (vict && IS_NPC(vict) && QST_RETURNMOB(rnum) == GET_MOB_VNUM(vict)) {
                if (--ch->ai_data->quest_counter <= 0)
                    mob_complete_quest(ch);
            }
            break;
        case AQ_RESOURCE_GATHER:
            /* Check if gathered enough of the resource */
            if (object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum))) {
                if (--ch->ai_data->quest_counter <= 0)
                    mob_complete_quest(ch);
            }
            break;
        case AQ_MOB_ESCORT:
            /* Check if escort target has reached destination */
            if (vict && IS_NPC(vict) && QST_TARGET(rnum) == GET_MOB_VNUM(vict)) {
                room_rnum dest = real_room(QST_RETURNMOB(rnum));
                /* Validate vict's room before comparing */
                if (dest != NOWHERE && IN_ROOM(vict) != NOWHERE && IN_ROOM(vict) >= 0 &&
                    IN_ROOM(vict) <= top_of_world && IN_ROOM(vict) == dest) {
                    if (--ch->ai_data->quest_counter <= 0)
                        mob_complete_quest(ch);
                }
            }
            break;
        case AQ_MAGIC_GATHER:
            /* Room-based: check if mob is at magical location */
            /* IN_ROOM(ch) already validated at function start */
            if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number) {
                if (--ch->ai_data->quest_counter <= 0)
                    mob_complete_quest(ch);
            }
            break;
        case AQ_OBJ_RETURN:
            /* Mob gives object to target mob - check if mob gave target item to the return mob */
            if (vict && IS_NPC(vict) && object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum))) {
                /* Check if the object is now in the target mob's inventory */
                struct obj_data *obj_check;
                bool has_object = FALSE;

                for (obj_check = vict->carrying; obj_check; obj_check = obj_check->next_content) {
                    if (obj_check == object) {
                        has_object = TRUE;
                        break;
                    }
                }

                if (has_object &&
                    (GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum) || GET_MOB_VNUM(vict) == QST_MASTER(rnum))) {
                    if (--ch->ai_data->quest_counter <= 0)
                        mob_complete_quest(ch);
                }
            }
            break;
        case AQ_EMOTION_IMPROVE:
            /* Mob must improve specific emotion with target mob
             * Check if the target mob's emotion toward the questing mob has reached the required level */
            if (vict && IS_NPC(vict) && vict->ai_data && QST_TARGET(rnum) == GET_MOB_VNUM(vict)) {
                int emotion_type = QST_RETURNMOB(rnum); /* Emotion type stored in RETURNMOB */
                int target_level = QST_QUANTITY(rnum);  /* Target emotion level */
                int current_emotion = 0;

                /* Get the current emotion level from the target mob toward the questing mob */
                current_emotion = get_effective_emotion_toward(vict, ch, emotion_type);

                if (current_emotion >= target_level) {
                    if (--ch->ai_data->quest_counter <= 0)
                        mob_complete_quest(ch);
                }
            }
            break;
        case AQ_REPUTATION_BUILD:
            /* Mob must improve reputation to target level
             * For mobs, this checks their own reputation stat */
            {
                int target_reputation = QST_QUANTITY(rnum);

                if (ch->ai_data->reputation >= target_reputation) {
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

/**
 * Apply an emotional profile to a mob
 * This sets baseline emotions aligned with the emotion-driven behavior thresholds
 * @param mob The mob to apply the profile to
 * @param profile_type The emotional profile type (EMOTION_PROFILE_*)
 */
static void apply_emotional_profile(struct char_data *mob, int profile_type)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    /* Reset all emotions to neutral baseline first */
    mob->ai_data->emotion_fear = 25;
    mob->ai_data->emotion_anger = 25;
    mob->ai_data->emotion_happiness = 50;
    mob->ai_data->emotion_sadness = 20;
    mob->ai_data->emotion_friendship = 40;
    mob->ai_data->emotion_love = 15;
    mob->ai_data->emotion_trust = 40;
    mob->ai_data->emotion_loyalty = 50;
    mob->ai_data->emotion_curiosity = 40;
    mob->ai_data->emotion_greed = 35;
    mob->ai_data->emotion_pride = 30;
    mob->ai_data->emotion_compassion = 40;
    mob->ai_data->emotion_envy = 20;
    mob->ai_data->emotion_courage = 40;
    mob->ai_data->emotion_excitement = 35;

    /* Apply profile-specific adjustments aligned with behavior thresholds */
    switch (profile_type) {
        case EMOTION_PROFILE_AGGRESSIVE:
            /* High anger/aggression, low trust/friendship
             * Crosses anger threshold (70) for negative socials
             * Below trust threshold (30) for trade refusal */
            mob->ai_data->emotion_anger = 75;      /* HIGH - triggers negative socials */
            mob->ai_data->emotion_trust = 25;      /* LOW - refuses service */
            mob->ai_data->emotion_friendship = 25; /* LOW - less likely to group */
            mob->ai_data->emotion_loyalty = 25;    /* LOW - abandons groups easily */
            mob->ai_data->emotion_compassion = 15; /* Very low empathy */
            mob->ai_data->emotion_greed = 75;      /* HIGH - expensive trades */
            mob->ai_data->emotion_courage = 70;    /* HIGH - stands ground */
            mob->ai_data->emotion_happiness = 20;  /* Low happiness */
            break;

        case EMOTION_PROFILE_DEFENSIVE:
            /* High fear/caution, low trust
             * Below trust threshold (30) for trade refusal
             * High fear affects flee behavior */
            mob->ai_data->emotion_fear = 70;       /* HIGH - flees earlier */
            mob->ai_data->emotion_trust = 25;      /* LOW - refuses service */
            mob->ai_data->emotion_friendship = 35; /* Below high threshold */
            mob->ai_data->emotion_loyalty = 40;    /* Moderate loyalty */
            mob->ai_data->emotion_courage = 20;    /* LOW - cautious */
            mob->ai_data->emotion_curiosity = 25;  /* LOW - avoids quests */
            mob->ai_data->emotion_sadness = 50;    /* Moderate sadness */
            break;

        case EMOTION_PROFILE_BALANCED:
            /* Moderate all emotions - crosses no extreme thresholds
             * Good for general-purpose NPCs */
            mob->ai_data->emotion_anger = 40;
            mob->ai_data->emotion_happiness = 55;
            mob->ai_data->emotion_trust = 50;      /* Mid-range - normal prices */
            mob->ai_data->emotion_friendship = 55; /* Moderate - normal grouping */
            mob->ai_data->emotion_loyalty = 55;    /* Moderate - normal group behavior */
            mob->ai_data->emotion_greed = 40;      /* Moderate - fair prices */
            mob->ai_data->emotion_curiosity = 50;  /* Moderate - accepts quests */
            mob->ai_data->emotion_courage = 50;    /* Balanced */
            break;

        case EMOTION_PROFILE_SENSITIVE:
            /* High empathy/compassion, low aggression
             * High friendship/compassion, low anger
             * Good for healers, helpers, friendly NPCs */
            mob->ai_data->emotion_compassion = 75; /* HIGH - helpful */
            mob->ai_data->emotion_friendship = 75; /* HIGH - joins groups easily */
            mob->ai_data->emotion_trust = 65;      /* HIGH - good prices */
            mob->ai_data->emotion_loyalty = 75;    /* HIGH - stays in group */
            mob->ai_data->emotion_happiness = 75;  /* HIGH - positive socials */
            mob->ai_data->emotion_anger = 15;      /* LOW - not aggressive */
            mob->ai_data->emotion_greed = 20;      /* LOW - generous */
            mob->ai_data->emotion_sadness = 30;    /* Moderate */
            mob->ai_data->emotion_curiosity = 60;  /* Moderate-high */
            break;

        case EMOTION_PROFILE_CONFIDENT:
            /* High courage/pride, low fear
             * Confident leaders, warriors, guardians */
            mob->ai_data->emotion_courage = 80;    /* Very high - rarely flees */
            mob->ai_data->emotion_pride = 75;      /* HIGH - proud */
            mob->ai_data->emotion_fear = 15;       /* LOW - fearless */
            mob->ai_data->emotion_loyalty = 75;    /* HIGH - stays with group */
            mob->ai_data->emotion_trust = 65;      /* HIGH - trusting */
            mob->ai_data->emotion_happiness = 65;  /* Above average */
            mob->ai_data->emotion_curiosity = 65;  /* Adventurous */
            mob->ai_data->emotion_friendship = 60; /* Sociable */
            break;

        case EMOTION_PROFILE_GREEDY:
            /* High greed/envy, low compassion
             * Merchants, thieves, selfish characters */
            mob->ai_data->emotion_greed = 80;      /* Very high - expensive */
            mob->ai_data->emotion_envy = 75;       /* HIGH - refuses better players */
            mob->ai_data->emotion_compassion = 15; /* LOW - selfish */
            mob->ai_data->emotion_friendship = 30; /* LOW - not friendly */
            mob->ai_data->emotion_loyalty = 30;    /* LOW - self-serving */
            mob->ai_data->emotion_trust = 35;      /* LOW-moderate */
            mob->ai_data->emotion_curiosity = 70;  /* HIGH - seeking opportunities */
            break;

        case EMOTION_PROFILE_LOYAL:
            /* High loyalty/trust, high friendship
             * Companions, guards, faithful servants */
            mob->ai_data->emotion_loyalty = 85;    /* Very high - never abandons */
            mob->ai_data->emotion_trust = 75;      /* HIGH - great prices */
            mob->ai_data->emotion_friendship = 80; /* HIGH - joins groups */
            mob->ai_data->emotion_love = 50;       /* Moderate-high - may follow */
            mob->ai_data->emotion_compassion = 65; /* HIGH - helpful */
            mob->ai_data->emotion_courage = 65;    /* Brave */
            mob->ai_data->emotion_happiness = 65;  /* Content */
            mob->ai_data->emotion_greed = 20;      /* LOW - fair */
            break;

        case EMOTION_PROFILE_NEUTRAL:
        default:
            /* Already set to neutral baseline - no changes needed */
            break;
    }

    /* Store the profile type for reference */
    mob->ai_data->emotional_profile = profile_type;
}

/* Initialize mob AI data with defaults for temporary quest master fields */
void init_mob_ai_data(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return;

    mob->ai_data->is_temp_questmaster = FALSE;
    mob->ai_data->temp_quests = NULL;
    mob->ai_data->num_temp_quests = 0;
    mob->ai_data->max_temp_quests = 0;

    /* Initialize last_chosen_action_type to -1 (no prior commitment).
     * Used by Conscientiousness consistency bias in Shadow Timeline scoring. */
    mob->ai_data->last_chosen_action_type = -1;
    mob->ai_data->action_repetition_count = 0;

    /* Initialize goal fields to sentinel values to prevent SIGSEGV.
     * These fields are checked against NOWHERE/NOTHING/NOBODY throughout the codebase.
     * Setting them to 0 (from memset) would cause incorrect behavior. */
    mob->ai_data->goal_destination = NOWHERE;
    mob->ai_data->goal_item_vnum = NOTHING;
    mob->ai_data->goal_target_mob_rnum = NOBODY;

    /* Initialize quest field to NOTHING to indicate no active quest.
     * This is critical for GOAL_ACCEPT_QUEST to work correctly, as the check
     * GET_MOB_QUEST(ch) == NOTHING at mobact.c must pass for quest acceptance. */
    mob->ai_data->current_quest = NOTHING;

    /* Initialize reputation to 40 to allow mobs to participate in trading and quests.
     * This value is at the threshold where quest reward penalties no longer apply (< 40 gets penalty),
     * placing mobs in the "average" reputation tier (40-59) with no modifiers. */
    mob->ai_data->reputation = 40;

    /* Big Five Phase 2: Initialize Conscientiousness.
     * Use an explicit flag to track initialization state instead of sentinel values.
     * This prevents confusion between "uninitialized" and "explicitly set to 0.0 (very low C)".
     *
     * The flag is checked later (during emotion initialization) to determine if we need
     * to generate a random value or preserve an existing one (from file or prototype copy). */
    if (!mob->ai_data->personality.conscientiousness_initialized) {
        mob->ai_data->personality.conscientiousness_initialized = 0; /* Mark as uninitialized */
    }

    /* Set default values for genetics if not already set from mob files.
     * This ensures mobs have reasonable default behavior even without explicit genetics.
     * These defaults can be overridden by GenFollow, GenRoam, etc. in mob files. */
    if (mob->ai_data->genetics.follow_tendency == 0) {
        /* Default follow tendency: 20% chance to follow others
         * This provides some following behavior without being too aggressive */
        mob->ai_data->genetics.follow_tendency = 20;
    }

    /* Initialize emotional_profile to neutral if not set */
    if (mob->ai_data->emotional_profile < 0 || mob->ai_data->emotional_profile > 7) {
        mob->ai_data->emotional_profile = EMOTION_PROFILE_NEUTRAL;
    }

    /* Initialize emotional_intelligence if not set from file (default 0)
     * Base it on mob level with some randomness:
     * - Low level (1-10): 20-50 (developing)
     * - Mid level (11-20): 40-70 (average)
     * - High level (21+): 50-80 (experienced)
     * This can be overridden by GenEmotionalIQ in mob files */
    if (mob->ai_data->genetics.emotional_intelligence == 0) {
        int base_ei = 30 + (GET_LEVEL(mob) * 2); /* Base grows with level */
        int variation = rand_number(-10, 10);    /* Random variation */
        mob->ai_data->genetics.emotional_intelligence = URANGE(10, base_ei + variation, 90);
    }

    /* Initialize Big Five (OCEAN) Personality traits
     * Phase 1: Only Neuroticism (N) is implemented
     * Other traits (O, C, E, A) reserved for future phases
     *
     * NEUROTICISM INITIALIZATION:
     * N is derived from genetic traits that indicate emotional sensitivity:
     * - Low brave_prevalence → Higher N (less brave = more threat-sensitive)
     * - Low emotional_intelligence → Higher N (less emotional control = more reactive)
     *
     * Formula: N = weighted_inverse_bravery + weighted_low_ei
     * Components:
     * - Inverse bravery: (100 - brave_prevalence) / 100 → [0, 1]
     * - Low EI factor: (50 - min(ei, 50)) / 50 → [0, 1] when EI < 50, else 0
     *
     * Weight distribution:
     * - Bravery: 70% weight (primary determinant of threat sensitivity)
     * - EI: 30% weight (secondary determinant of emotional volatility)
     *
     * NOTE: This initialization must happen before the emotional profile early return
     * to ensure all mobs (including those with profiles) get proper personality values.
     */
    {
        int brave = mob->ai_data->genetics.brave_prevalence;
        int ei = mob->ai_data->genetics.emotional_intelligence;

        /* Calculate inverse bravery component (0.0 to 1.0) */
        float inverse_bravery = (100.0f - (float)brave) / 100.0f;

        /* Calculate low EI component (0.0 to 1.0, only if EI < 50) */
        float low_ei_factor = 0.0f;
        if (ei < 50) {
            low_ei_factor = (50.0f - (float)ei) / 50.0f;
        }

        /* Weighted combination: 70% bravery, 30% EI */
        float neuroticism = (inverse_bravery * 0.7f) + (low_ei_factor * 0.3f);

        /* Clamp to valid range [0.0, 1.0] */
        mob->ai_data->personality.neuroticism = URANGE(0.0f, neuroticism, 1.0f);

        /* Initialize other OCEAN traits:
         * Phase 2: Conscientiousness (C) - Generate if not explicitly set
         * Phase 3+: Other traits remain at neutral baseline (0.5) for now
         *
         * CONSCIENTIOUSNESS INITIALIZATION:
         * C represents self-discipline, organization, and goal-directed behavior.
         *
         * Strategy: Use explicit initialization flag instead of sentinel values.
         * This avoids confusion between "uninitialized" and "explicitly set to 0.0".
         *
         * Storage: 0-100 in files/UI, normalized to 0.0-1.0 in personality struct.
         * Note: 0.0 is a valid value (very low conscientiousness) and can now be
         * properly distinguished from "not initialized" via the flag.
         */
        if (!mob->ai_data->personality.conscientiousness_initialized) {
            /* Uninitialized: generate a new conscientiousness value.
             * Use min=1 so that C=0 remains an unambiguous "uninitialized"
             * sentinel in saved files, consistent with db.c and medit.c. */
            int c_value = rand_gaussian(50, 15, 1, 100);
            mob->ai_data->personality.conscientiousness = (float)c_value / 100.0f;
            mob->ai_data->personality.conscientiousness_initialized = 1;
        }
        /* If already initialized, it was explicitly set (file/prototype); keep it as-is. */

        /* Big Five Phase 4: Openness (O) - Gaussian Trait_base generation.
         * μ=50 (int), σ=15, clamped [1, 100]; normalized to [0.01, 1.0] float.
         * Value 0 remains "uninitialized" sentinel consistent with other traits.
         * IMPORTANT: O_base is structural (genetic); it must never be derived from
         * SEC emotional state.  O_mod is reserved for slow long-term adaptation only
         * (±SEC_O_MOD_CAP = ±0.05); it must not be recalculated per tick. */
        if (!mob->ai_data->personality.openness_initialized) {
            int o_value = rand_gaussian(50, 15, 1, 100);
            mob->ai_data->personality.openness = (float)o_value / 100.0f;
            mob->ai_data->personality.openness_initialized = 1;
        }
        /* If already initialized, it was explicitly set (file/prototype); keep it as-is. */

        /* Big Five Phase 3: Agreeableness (A) - Gaussian Trait_base generation.
         * μ=0.5, σ=0.15, clamped 1-100. Value 0 remains "uninitialized" sentinel. */
        if (!mob->ai_data->personality.agreeableness_initialized) {
            int a_value = rand_gaussian(50, 15, 1, 100);
            mob->ai_data->personality.agreeableness = (float)a_value / 100.0f;
            mob->ai_data->personality.agreeableness_initialized = 1;
        }

        /* Big Five Phase 3: Extraversion (E) - Gaussian Trait_base generation. */
        if (!mob->ai_data->personality.extraversion_initialized) {
            int e_value = rand_gaussian(50, 15, 1, 100);
            mob->ai_data->personality.extraversion = (float)e_value / 100.0f;
            mob->ai_data->personality.extraversion_initialized = 1;
        }
    }

    /* If a specific emotional profile is set, apply it first
     * This provides consistent personality archetypes aligned with behavior thresholds */
    if (mob->ai_data->emotional_profile != EMOTION_PROFILE_NEUTRAL) {
        apply_emotional_profile(mob, mob->ai_data->emotional_profile);

        /* Still apply genetics-based adjustments on top of the profile
         * This allows fine-tuning of profile-based emotions */
        if (mob->ai_data->genetics.wimpy_tendency > 0) {
            mob->ai_data->emotion_fear =
                MIN(100, mob->ai_data->emotion_fear + mob->ai_data->genetics.wimpy_tendency / 4);
        }
        if (mob->ai_data->genetics.brave_prevalence > 0) {
            mob->ai_data->emotion_courage =
                MIN(100, mob->ai_data->emotion_courage + mob->ai_data->genetics.brave_prevalence / 4);
        }
        if (mob->ai_data->genetics.group_tendency > 0) {
            mob->ai_data->emotion_friendship =
                MIN(100, mob->ai_data->emotion_friendship + mob->ai_data->genetics.group_tendency / 6);
            mob->ai_data->emotion_loyalty =
                MIN(100, mob->ai_data->emotion_loyalty + mob->ai_data->genetics.group_tendency / 4);
        }

        /* Ensure all emotions stay within bounds after adjustments */
        mob->ai_data->emotion_fear = URANGE(0, mob->ai_data->emotion_fear, 100);
        mob->ai_data->emotion_courage = URANGE(0, mob->ai_data->emotion_courage, 100);
        mob->ai_data->emotion_friendship = URANGE(0, mob->ai_data->emotion_friendship, 100);
        mob->ai_data->emotion_loyalty = URANGE(0, mob->ai_data->emotion_loyalty, 100);

        return; /* Profile applied, skip genetics-based initialization */
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

    /* Initialize overall mood from initial emotions */
    mob->ai_data->overall_mood = calculate_mob_mood(mob);
    mob->ai_data->mood_timer = 0;

    /* Initialize extreme emotional state timers */
    mob->ai_data->berserk_timer = 0;
    mob->ai_data->paralyzed_timer = 0;

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

        /* Initialize active emotion memory - zero out all slots */
        for (i = 0; i < EMOTION_MEMORY_SIZE; i++) {
            mob->ai_data->active_memories[i].timestamp = 0; /* Mark as unused */
            mob->ai_data->active_memories[i].entity_id = 0;
            mob->ai_data->active_memories[i].entity_type = 0;
            mob->ai_data->active_memories[i].interaction_type = 0;
            mob->ai_data->active_memories[i].major_event = 0;
        }
        mob->ai_data->active_memory_index = 0; /* Start at beginning of circular buffer */
    }

    /* Initialize climate preferences - these will be set to appropriate values
     * when the mob is placed in a room during zone reset. Initialize to -1 (none/unset)
     * to indicate they have not yet been set, but preserve any non-zero prototype/espec
     * values that may already be present. Also preserve -1 if explicitly set by builder.
     * Note: Value 0 is considered "uninitialized" here since the struct is zeroed on creation.
     * Builders should use -1 in espec fields to explicitly indicate "no preference". */
    if (mob->ai_data->preferred_weather_sky == 0)
        mob->ai_data->preferred_weather_sky = -1;
    if (mob->ai_data->preferred_temperature_range == 0)
        mob->ai_data->preferred_temperature_range = -1;
    if (mob->ai_data->native_climate == 0)
        mob->ai_data->native_climate = -1;
    mob->ai_data->last_weather_sky = -1;
    mob->ai_data->weather_exposure_hours = 0;
    if (mob->ai_data->seasonal_affective_trait == 0)
        mob->ai_data->seasonal_affective_trait = -1; /* Mark as uninitialized if unset */

    /* Initialize Seasonal Affective Disorder (SAD) tendency
     * If not specified in mob file (defaults to -1), use level-based randomization: random(110 - level)
     * This gives higher-level mobs lower SAD susceptibility (more experienced/resilient)
     * Note: 0 is a valid value (no SAD), so we check for -1 to detect uninitialized state */
    if (mob->ai_data->seasonal_affective_trait < 0) {
        int max_sad = MAX(0, 110 - GET_LEVEL(mob));
        mob->ai_data->seasonal_affective_trait = rand_number(0, max_sad);
    }

    /* Initialize Shadow Timeline cognitive capacity (RFC-0001)
     * Base capacity on emotional intelligence and level
     * Higher EI = more efficient cognitive processing
     * Formula: COGNITIVE_CAPACITY_BASE + (EI * COGNITIVE_CAPACITY_EI_MULT)
     * Range: ~700-1000 */
    mob->ai_data->cognitive_capacity =
        COGNITIVE_CAPACITY_BASE + (mob->ai_data->genetics.emotional_intelligence * COGNITIVE_CAPACITY_EI_MULT);
    mob->ai_data->cognitive_capacity =
        URANGE(COGNITIVE_CAPACITY_LOWER_BOUND, mob->ai_data->cognitive_capacity, COGNITIVE_CAPACITY_MAX);

    /* SEC: initialise internal emotional state and personality baseline. */
    sec_init(mob);
}

/* Initialize mob climate preferences based on spawn room conditions.
 * This should be called when a mob is placed into a room during zone reset.
 * Sets the mob's preferred weather, temperature, and native climate based on
 * the current conditions in the room where they spawn. */
void initialize_mob_climate_preferences(struct char_data *mob, room_rnum room)
{
    zone_rnum zone;
    struct weather_data *weather;

    if (!IS_NPC(mob) || !mob->ai_data || room == NOWHERE || room > top_of_world)
        return;

    /* Only initialize if not already set (check for -1 default value) */
    if (mob->ai_data->preferred_weather_sky != -1 && mob->ai_data->preferred_temperature_range != -1 &&
        mob->ai_data->native_climate != -1)
        return;

    zone = world[room].zone;
    if (zone < 0 || zone > top_of_zone_table)
        return;

    weather = zone_table[zone].weather;
    if (!weather)
        return;

    /* Set preferred weather based on current sky condition */
    if (mob->ai_data->preferred_weather_sky == -1) {
        mob->ai_data->preferred_weather_sky = weather->sky;
    }

    /* Set preferred temperature based on current temperature
     * Temperature ranges: 0=very cold (<0°C), 1=cold (0-10°C), 2=comfortable (10-25°C),
     *                    3=hot (25-35°C), 4=very hot (>35°C) */
    if (mob->ai_data->preferred_temperature_range == -1) {
        if (weather->temperature < 0)
            mob->ai_data->preferred_temperature_range = 0;
        else if (weather->temperature < 10)
            mob->ai_data->preferred_temperature_range = 1;
        else if (weather->temperature <= 25)
            mob->ai_data->preferred_temperature_range = 2;
        else if (weather->temperature <= 35)
            mob->ai_data->preferred_temperature_range = 3;
        else
            mob->ai_data->preferred_temperature_range = 4;
    }

    /* Set native climate based on zone's climate type
     * Climate types: 0=temperate, 1=rainy, 2=tropical, 3=arctic, 4=desert */
    if (mob->ai_data->native_climate == -1) {
        mob->ai_data->native_climate = zone_table[zone].climate;
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

/* Clear a quest from all mobs and players when it is deleted from the system.
 * This prevents freezes/infinite loops when entities try to complete quests
 * that no longer exist in the quest table. Called from delete_quest() in genqst.c. */
void clear_quest_from_all_entities(qst_vnum vnum)
{
    struct char_data *ch, *next_ch;
    int mobs_cleared = 0;
    int players_cleared = 0;

    if (vnum == NOTHING)
        return;

    /* Iterate through all characters in the game */
    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;

        /* Skip characters marked for extraction */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        if (IS_NPC(ch)) {
            /* Clear quest from mob's AI data */
            if (ch->ai_data && GET_MOB_QUEST(ch) == vnum) {
                clear_mob_quest(ch);
                mobs_cleared++;
            }

            /* Also remove from temporary quest list if this mob is a temp questmaster */
            if (ch->ai_data && IS_TEMP_QUESTMASTER(ch)) {
                remove_temp_quest_from_mob(ch, vnum);
            }
        } else {
            /* Clear quest from player if they have it active */
            if (GET_QUEST(ch) == vnum) {
                send_to_char(ch,
                             "\r\n\tyAVISO: A busca que você estava fazendo foi removida do sistema.\tn\r\n"
                             "\tyEla foi completada por outra pessoa ou expirou.\tn\r\n");
                clear_quest(ch);
                save_char(ch);
                players_cleared++;
            }
        }
    }

    if (mobs_cleared > 0 || players_cleared > 0) {
        log1("QUEST CLEANUP: Quest %d deleted - cleared from %d mobs and %d players", vnum, mobs_cleared,
             players_cleared);
    }
}
