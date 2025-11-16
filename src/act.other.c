/**************************************************************************
 *  File: act.other.c                                       Part of tbaMUD *
 *  Usage: Miscellaneous player-level commands.                             *
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
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h" /* for has_mail() */
#include "shop.h"
#include "quest.h"
#include "spedit.h" /* for get_spell_level() */
#include "modify.h"

/* Local defined utility functions */
/* do_group utility functions */
static void print_group(struct char_data *ch);
static void display_group_list(struct char_data *ch);

int can_elevate(struct char_data *ch);
int can_rebegin(struct char_data *ch);
void show_menu_with_options(struct descriptor_data *d);
void check_thief(struct char_data *ch, struct char_data *vict);
bool are_groupable(struct char_data *ch, struct char_data *target);

ACMD(do_quit)
{
    char buf[128];
    time_t ct = time(0);
    struct tm *t = localtime(&ct);

    if (IS_NPC(ch) || !ch->desc)
        return;

    if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
        send_to_char(ch, "Você tem que escrever o comando inteiro -- não menos do que isso!\r\n");
    else if (GET_POS(ch) == POS_FIGHTING)
        send_to_char(ch, "Sem chances!  Você está lutando pela sua vida!\r\n");
    else if (GET_POS(ch) < POS_STUNNED) {
        send_to_char(ch, "Você morreu antes do tempo...\r\n");
        die(ch, NULL);
    } else {
        act("$n deixou o jogo.", TRUE, ch, 0, 0, TO_ROOM);
        mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));

        if (GET_QUEST_TIME(ch) != -1)
            quest_timeout(ch);

        sprintf(buf, "Tenha um%s! :-)\r\n\r\n",
                (t->tm_hour < 6)    ? "a boa madrugada"
                : (t->tm_hour < 12) ? " bom dia"
                : (t->tm_hour < 18) ? "a boa tarde"
                                    : "a boa noite");

        send_to_char(ch, "%s", buf);

        /* We used to check here for duping attempts, but we may as well do it
           right in extract_char(), since there is no check if a player rents
           out and it can leave them in an equally screwy situation. */

        if (CONFIG_FREE_RENT)
            Crash_rentsave(ch, 0);

        GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

        /* Stop snooping so you can't see passwords during deletion or change.
         */
        if (ch->desc->snoop_by) {
            write_to_output(ch->desc->snoop_by, "A sua vitima não está mais entre nós.\r\n");
            ch->desc->snoop_by->snooping = NULL;
            ch->desc->snoop_by = NULL;
        }

        extract_char(ch); /* Char is saved before extracting. */
    }
}

ACMD(do_save)
{
    if (IS_NPC(ch) || !ch->desc)
        return;

    send_to_char(ch, "Salvando atalhos (alias)\r\n");
    send_to_char(ch, "Salvando personagem %s.\r\n", GET_NAME(ch));
    save_char(ch);
    send_to_char(ch, "Salvando objetos.\r\n");
    Crash_crashsave(ch);
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH)) {
        send_to_char(ch, "Salvando casa.\r\n");
        House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
    }
    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
}

/* Generic function for commands which are normally overridden by special
   procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here) { send_to_char(ch, "Desculpe, mas você não pode fazer isso aqui!\r\n"); }

ACMD(do_sneak)
{
    struct affected_type af;
    byte percent;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }
    send_to_char(ch, "Ok, você vai tentar mover-se silenciosamente por um tempo.\r\n");
    if (AFF_FLAGGED(ch, AFF_SNEAK))
        affect_from_char(ch, SKILL_SNEAK);

    percent = rand_number(1, 101); /* 101% is a complete failure */

    if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
        return;

    new_affect(&af);
    af.spell = SKILL_SNEAK;
    af.duration = MIN(GET_LEVEL(ch), 24);
    SET_BIT_AR(af.bitvector, AFF_SNEAK);
    affect_to_char(ch, &af);
}

ACMD(do_hide)
{
    byte percent;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    send_to_char(ch, "Você tenta se esconder.\r\n");

    if (AFF_FLAGGED(ch, AFF_HIDE))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

    percent = rand_number(1, 101); /* 101% is a complete failure */

    if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide)
        return;

    SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
}

ACMD(do_steal)
{
    struct char_data *vict;
    struct obj_data *obj;
    char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
    int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "Você sente muita paz neste lugar para roubar alguma coisa...\r\n");
        return;
    }

    two_arguments(argument, obj_name, vict_name);

    if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Roubar o quê de quem?\r\n");
        return;
    } else if (vict == ch) {
        send_to_char(ch, "Pare, isto é muito estúpido!\r\n");
        return;
    }
    if (IS_DEAD(ch)) {
        act("Você não pode roubar d$L, você está mort$r!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    } else if (IS_DEAD(vict)) {
        act("Suas mãos passam por $N... $L é apenas um espírito...", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    /* 101% is a complete failure */
    percent = rand_number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

    if (GET_POS(vict) < POS_SLEEPING)
        percent = -1; /* ALWAYS SUCCESS, unless heavy object. */

    if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
        pcsteal = 1;

    if (!AWAKE(vict)) /* Easier to steal from sleeping people. */
        percent -= 50;

    /* No stealing if not allowed. If it is no stealing from Imm's or
       Shopkeepers. */
    if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal || GET_MOB_SPEC(vict) == shop_keeper || PLR_FLAGGED(vict, PLR_TRNS))
        percent = 101; /* Failure */

    if (str_cmp(obj_name, "moedas") && str_cmp(obj_name, "dinheiro")) {

        if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {

            for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
                if (GET_EQ(vict, eq_pos) && (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
                    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
                    obj = GET_EQ(vict, eq_pos);
                    break;
                }
            if (!obj) {
                act("$L não tem este objeto.", FALSE, ch, 0, vict, TO_CHAR);
                return;
            } else { /* It is equipment */
                if ((GET_POS(vict) > POS_STUNNED)) {
                    send_to_char(ch, "Roubar equipamento agora?  Impossível!\r\n");
                    return;
                } else {
                    if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj)) {
                        send_to_char(ch, "Impossível!\r\n");
                        return;
                    }
                    act("Você desequipa $p e rouba de $N.", FALSE, ch, obj, 0, TO_CHAR);
                    act("$n rouba $p de $N.", FALSE, ch, obj, vict, TO_NOTVICT);
                    obj_to_char(unequip_char(vict, eq_pos), ch);
                }
            }
        } else { /* obj found in inventory */

            percent += GET_OBJ_WEIGHT(obj); /* Make heavy harder */

            if (percent > GET_SKILL(ch, SKILL_STEAL)) {
                ohoh = TRUE;
                send_to_char(ch, "Opa..\r\n");
                act("$n tentou roubar algo de você!", FALSE, ch, 0, vict, TO_VICT);
                act("$n tentou roubar algo de $N.", TRUE, ch, 0, vict, TO_NOTVICT);
                /* Update mob emotions for failed steal attempt (experimental feature) */
                update_mob_emotion_stolen_from(vict, ch);
            } else { /* Steal the item */
                if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
                    if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj)) {
                        send_to_char(ch, "Impossível!\r\n");
                        return;
                    }
                    if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
                        obj_from_char(obj);
                        obj_to_char(obj, ch);
                        send_to_char(ch, "Você pegou!\r\n");
                    }
                } else
                    send_to_char(ch, "Você não pode carregar tanto peso.\r\n");
            }
        }
    } else { /* Steal some coins */
        if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
            ohoh = TRUE;
            send_to_char(ch, "Opa..\r\n");
            act("Você descobre que $n estava com as mãos em sua carteira.", FALSE, ch, 0, vict, TO_VICT);
            act("$n tentou roubar dinheiro de $N.", TRUE, ch, 0, vict, TO_NOTVICT);
            /* Update mob emotions for failed steal attempt (experimental feature) */
            update_mob_emotion_stolen_from(vict, ch);
        } else {
            /* Steal some gold coins */
            gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
            gold = MIN(1782, gold);
            if (gold > 0) {
                increase_gold(ch, gold);
                decrease_gold(vict, gold);
                if (gold > 1)
                    send_to_char(ch, "Feito!  Você conseguiu %d moedas.\r\n", gold);
                else
                    send_to_char(ch, "Você conseguiu apenas uma única moeda.\r\n");
            } else {
                send_to_char(ch, "Você não conseguiu roubar nada...\r\n");
            }
        }
    }

    if (ohoh && IS_NPC(vict) && AWAKE(vict))
        hit(vict, ch, TYPE_UNDEFINED);

    /* Reputation changes for stealing - dynamic reputation system */
    if (CONFIG_DYNAMIC_REPUTATION && !IS_NPC(ch) && !ohoh) {
        /* Successful stealing */
        int class_bonus = get_class_reputation_modifier(ch, CLASS_REP_STEALTH_ACTION, vict);
        if (IS_EVIL(ch)) {
            /* Evil characters gain reputation (infamy) for successful theft */
            if (IS_GOOD(vict)) {
                /* Stealing from good targets increases evil reputation */
                modify_player_reputation(ch, rand_number(1, 2) + class_bonus);
            } else {
                /* Any successful theft for evil characters */
                modify_player_reputation(ch, 1 + class_bonus);
            }
        } else {
            /* Good/Neutral characters LOSE reputation for stealing */
            modify_player_reputation(ch, -rand_number(2, 4));
            /* Extra penalty for stealing from good targets */
            if (IS_GOOD(vict)) {
                modify_player_reputation(ch, -rand_number(1, 3));
            }
        }
    } else if (CONFIG_DYNAMIC_REPUTATION && !IS_NPC(ch) && ohoh) {
        /* Getting caught stealing always damages reputation */
        modify_player_reputation(ch, -rand_number(3, 6));
    }

    if (!IS_NPC(ch) && !IS_NPC(vict))
        SET_BIT_AR(PLR_FLAGS(ch), PLR_HTHIEF);

    if (ohoh && CONFIG_PT_ALLOWED) /* -- jr - 03/07/99 */
        check_thief(ch, vict);
}

void check_thief(struct char_data *ch, struct char_data *vict)
{
    if (!IS_NPC(ch) && !IS_NPC(vict) && (ch != vict) && !PLR_FLAGGED(vict, PLR_KILLER) &&
        !PLR_FLAGGED(vict, PLR_THIEF)) {
        if (!PLR_FLAGGED(ch, PLR_THIEF)) {
            SET_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "PC Thief bit set on %s while trying to steal %s.",
                   GET_NAME(ch), GET_NAME(vict));
            send_to_char(ch, "Agora você é um JOGADOR LADRÃO, que pena...\r\n");
        } else {
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "PC Thief %s trying to steal %s.", GET_NAME(ch),
                   GET_NAME(vict));
        }
    }
}

ACMD(do_peek)
{
    struct char_data *vict;
    struct obj_data *obj;
    char vict_name[MAX_INPUT_LENGTH];
    int percent, success_percent, items_shown, total_items;
    int move_cost = 5; /* Movement cost for using peek */

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PEEK)) {
        send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "Você sente muita paz neste lugar para bisbilhotar...\r\n");
        return;
    }

    /* Check if enough movement points */
    if (GET_MOVE(ch) < move_cost) {
        send_to_char(ch, "Você está cansad%s demais para isso.\r\n", OA(ch));
        return;
    }

    one_argument(argument, vict_name);

    if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Bisbilhotar quem?\r\n");
        return;
    } else if (vict == ch) {
        send_to_char(ch, "Você olha para seu próprio inventário...\r\n");
        do_inventory(ch, "", 0, 0);
        return;
    }

    if (IS_DEAD(ch)) {
        act("Você não pode bisbilhotar $L, você está mort$r!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    } else if (IS_DEAD(vict)) {
        act("Suas mãos passam por $N... $L é apenas um espírito...", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    /* Consume movement points */
    GET_MOVE(ch) -= move_cost;

    /* Calculate success chance - 101% is a complete failure */
    percent = rand_number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

    if (GET_POS(vict) < POS_SLEEPING)
        percent = -1; /* ALWAYS SUCCESS */

    if (!AWAKE(vict)) /* Easier to peek at sleeping people */
        percent -= 30;

    /* Immortals and shopkeepers are harder to peek at */
    if (GET_LEVEL(vict) >= LVL_IMMORT || GET_MOB_SPEC(vict) == shop_keeper)
        percent += 50;

    /* Check if peek was successful */
    if (percent > GET_SKILL(ch, SKILL_PEEK)) {
        /* Failed - victim notices */
        send_to_char(ch, "Opa... Você foi peg%s!\r\n", OA(ch));
        act("$n tentou bisbilhotar seus pertences!", FALSE, ch, 0, vict, TO_VICT);
        act("$n tentou bisbilhotar os pertences de $N.", TRUE, ch, 0, vict, TO_NOTVICT);

        /* Reputation changes for being caught */
        if (CONFIG_DYNAMIC_REPUTATION && !IS_NPC(ch)) {
            modify_player_reputation(ch, -rand_number(2, 4));
        }

        if (IS_NPC(vict) && AWAKE(vict))
            hit(vict, ch, TYPE_UNDEFINED);

        return;
    }

    /* Success! Show items based on skill level */
    success_percent = GET_SKILL(ch, SKILL_PEEK);

    /* Count total items */
    total_items = 0;
    for (obj = vict->carrying; obj; obj = obj->next_content) {
        if (CAN_SEE_OBJ(ch, obj))
            total_items++;
    }

    if (total_items == 0) {
        act("Você consegue ver que $N não está carregando nada.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    /* Calculate how many items to show based on skill level */
    /* At 50% skill, show 50% of items. At 100% skill, show all items */
    items_shown = (total_items * success_percent) / 100;
    if (items_shown < 1)
        items_shown = 1; /* Always show at least one item */

    act("\r\nVocê consegue ver no inventário $D:", FALSE, vict, 0, ch, TO_VICT);

    /* Show items - loop through and show random items */
    if (items_shown >= total_items) {
        /* Show all items */
        for (obj = vict->carrying; obj; obj = obj->next_content) {
            if (CAN_SEE_OBJ(ch, obj)) {
                send_to_char(ch, "  ");
                show_obj_to_char(obj, ch, SHOW_OBJ_SHORT);
            }
        }
    } else {
/* Show only a portion of items */
/* Use a reasonable max size for stack allocation */
#define MAX_PEEK_ITEMS 100
        int shown_indices[MAX_PEEK_ITEMS];
        int all_indices[MAX_PEEK_ITEMS];
        int i, idx;

        /* Safety check */
        if (total_items > MAX_PEEK_ITEMS)
            total_items = MAX_PEEK_ITEMS;

        /* Create array of all indices */
        for (i = 0; i < total_items; i++)
            all_indices[i] = i;

        /* Fisher-Yates shuffle to select random items */
        for (i = 0; i < items_shown; i++) {
            idx = rand_number(i, total_items - 1);
            /* Swap */
            int temp = all_indices[i];
            all_indices[i] = all_indices[idx];
            all_indices[idx] = temp;
            shown_indices[i] = all_indices[i];
        }

        /* Display the selected items */
        for (i = 0; i < items_shown; i++) {
            int current_idx = 0;
            for (obj = vict->carrying; obj; obj = obj->next_content) {
                if (CAN_SEE_OBJ(ch, obj)) {
                    if (current_idx == shown_indices[i]) {
                        send_to_char(ch, "  ");
                        show_obj_to_char(obj, ch, SHOW_OBJ_SHORT);
                        break;
                    }
                    current_idx++;
                }
            }
        }

        if (items_shown < total_items) {
            send_to_char(ch, "\r\nVocê não consegue ver tudo que %s está carregando.\r\n", ELEA(vict));
        }
#undef MAX_PEEK_ITEMS
    }

    /* Small chance of being detected even on success (5% base) */
    if (AWAKE(vict) && rand_number(1, 100) <= 5) {
        act("$n parece estar olhando para você de forma estranha.", FALSE, ch, 0, vict, TO_VICT);
    }

    /* Reputation changes for successful peeking */
    if (CONFIG_DYNAMIC_REPUTATION && !IS_NPC(ch)) {
        int class_bonus = get_class_reputation_modifier(ch, CLASS_REP_STEALTH_ACTION, vict);
        if (IS_EVIL(ch)) {
            /* Evil characters gain small reputation for sneaky actions */
            modify_player_reputation(ch, 1 + class_bonus);
        }
    }
}

ACMD(do_practice)
{
    char arg[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return;

    one_argument(argument, arg);

    if (*arg)
        send_to_char(ch, "Você só pode praticar na presença de seu mestre.\r\n");
    else
        list_skills(ch);
}

/* Structure for sorting spells/skills by level */
struct spell_level_entry {
    char *name;
    int level;
};

/* Comparison function for qsort */
static int compare_spell_levels(const void *a, const void *b)
{
    const struct spell_level_entry *entry_a = (const struct spell_level_entry *)a;
    const struct spell_level_entry *entry_b = (const struct spell_level_entry *)b;
    return entry_a->level - entry_b->level;
}

/* Helper function to list spells/skills/chansons for a class */
static void list_spells_by_type(struct char_data *ch, int class_num, char type, bool is_current_class)
{
    struct str_spells *ptr;
    struct spell_level_entry *entries = NULL;
    int count = 0, i;
    char buf[MAX_STRING_LENGTH];
    size_t len = 0;

    /* Count how many entries we have */
    for (ptr = list_spells; ptr; ptr = ptr->next) {
        if (ptr->status == available && ptr->type == type) {
            int level = get_spell_level(ptr->vnum, class_num);
            if (level >= 0)
                count++;
        }
    }

    if (count == 0) {
        if (is_current_class) {
            send_to_char(ch, "Nenhuma %s disponível para a sua classe atual.\r\n",
                         type == SPELL   ? "magia"
                         : type == SKILL ? "habilidade"
                                         : "canção");
        } else {
            send_to_char(ch, "Nenhuma %s disponível para a classe %s.\r\n",
                         type == SPELL   ? "magia"
                         : type == SKILL ? "habilidade"
                                         : "canção",
                         pc_class_types[class_num]);
        }
        return;
    }

    /* Allocate array for sorting */
    CREATE(entries, struct spell_level_entry, count);

    /* Fill the array */
    i = 0;
    for (ptr = list_spells; ptr; ptr = ptr->next) {
        if (ptr->status == available && ptr->type == type) {
            int level = get_spell_level(ptr->vnum, class_num);
            if (level >= 0) {
                entries[i].name = ptr->name;
                entries[i].level = level;
                i++;
            }
        }
    }

    /* Sort by level */
    qsort(entries, count, sizeof(struct spell_level_entry), compare_spell_levels);

    /* Build output message */
    if (is_current_class) {
        len = snprintf(buf, sizeof(buf), "As seguintes %s estão disponíveis para a sua classe atual:\r\n",
                       type == SPELL   ? "magias"
                       : type == SKILL ? "habilidades"
                                       : "canções");
    } else {
        len = snprintf(buf, sizeof(buf), "As seguintes %s estão disponíveis para a classe %s:\r\n",
                       type == SPELL   ? "magias"
                       : type == SKILL ? "habilidades"
                                       : "canções",
                       pc_class_types[class_num]);
    }

    for (i = 0; i < count && len < sizeof(buf) - 60; i++) {
        len +=
            snprintf(buf + len, sizeof(buf) - len, "%-30s [Nível Mínimo: %3d]\r\n", entries[i].name, entries[i].level);
    }

    page_string(ch->desc, buf, TRUE);

    /* Free the array */
    free(entries);
}

/* Parse class name from argument */
static int parse_class_arg(char *arg)
{
    if (!*arg)
        return CLASS_UNDEFINED;

    if (is_abbrev(arg, "mago") || is_abbrev(arg, "mu"))
        return CLASS_MAGIC_USER;
    else if (is_abbrev(arg, "clerigo") || is_abbrev(arg, "cl"))
        return CLASS_CLERIC;
    else if (is_abbrev(arg, "ladrao") || is_abbrev(arg, "th"))
        return CLASS_THIEF;
    else if (is_abbrev(arg, "guerreiro") || is_abbrev(arg, "wa"))
        return CLASS_WARRIOR;
    else if (is_abbrev(arg, "druida") || is_abbrev(arg, "dru"))
        return CLASS_DRUID;
    else if (is_abbrev(arg, "bardo") || is_abbrev(arg, "ba"))
        return CLASS_BARD;
    else if (is_abbrev(arg, "ranger") || is_abbrev(arg, "ra"))
        return CLASS_RANGER;

    return CLASS_UNDEFINED;
}

ACMD(do_skills)
{
    char arg[MAX_INPUT_LENGTH];
    int class_num;
    bool is_current_class;

    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs não podem usar este comando.\r\n");
        return;
    }

    one_argument(argument, arg);

    if (*arg) {
        class_num = parse_class_arg(arg);
        if (class_num == CLASS_UNDEFINED) {
            send_to_char(ch, "Classe inválida. Use: mago, clerigo, ladrao, guerreiro, druida, bardo ou ranger.\r\n");
            return;
        }
        is_current_class = FALSE;
    } else {
        class_num = GET_CLASS(ch);
        is_current_class = TRUE;
    }

    list_spells_by_type(ch, class_num, SKILL, is_current_class);
}

ACMD(do_spells)
{
    char arg[MAX_INPUT_LENGTH];
    int class_num;
    bool is_current_class;

    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs não podem usar este comando.\r\n");
        return;
    }

    one_argument(argument, arg);

    if (*arg) {
        class_num = parse_class_arg(arg);
        if (class_num == CLASS_UNDEFINED) {
            send_to_char(ch, "Classe inválida. Use: mago, clerigo, ladrao, guerreiro, druida, bardo ou ranger.\r\n");
            return;
        }
        is_current_class = FALSE;
    } else {
        class_num = GET_CLASS(ch);
        is_current_class = TRUE;
    }

    list_spells_by_type(ch, class_num, SPELL, is_current_class);
}

ACMD(do_chansons)
{
    char arg[MAX_INPUT_LENGTH];
    int class_num;
    bool is_current_class;

    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs não podem usar este comando.\r\n");
        return;
    }

    one_argument(argument, arg);

    if (*arg) {
        class_num = parse_class_arg(arg);
        if (class_num == CLASS_UNDEFINED) {
            send_to_char(ch, "Classe inválida. Use: mago, clerigo, ladrao, guerreiro, druida, bardo ou ranger.\r\n");
            return;
        }
        is_current_class = FALSE;
    } else {
        class_num = GET_CLASS(ch);
        is_current_class = TRUE;
    }

    list_spells_by_type(ch, class_num, CHANSON, is_current_class);
}

ACMD(do_syllables)
{
    struct str_spells *ptr;
    char syllables[256];
    char buf[MAX_STRING_LENGTH];
    size_t len = 0;
    int count = 0;
    bool has_spells = FALSE;

    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs não podem usar este comando.\r\n");
        return;
    }

    /* Header */
    len = snprintf(buf, sizeof(buf),
                   "Sílabas Místicas das Magias que Você Conhece:\r\n"
                   "==============================================\r\n\r\n"
                   "Ao estudar suas magias, você aprende as palavras de poder que as invocam.\r\n"
                   "Você pode falar essas sílabas em voz alta para conjurar magias usando o comando 'say'.\r\n\r\n");

    /* List all spells the character knows */
    for (ptr = list_spells; ptr && len < sizeof(buf) - 200; ptr = ptr->next) {
        if (ptr->status == available && ptr->type == SPELL && GET_SKILL(ch, ptr->vnum) > 0) {
            has_spells = TRUE;
            spell_to_syllables_public(ptr->name, syllables, sizeof(syllables));

            len += snprintf(buf + len, sizeof(buf) - len, "%-30s -> %s\r\n", ptr->name, syllables);
            count++;
        }
    }

    if (!has_spells) {
        send_to_char(ch, "Você ainda não conhece nenhuma magia.\r\n");
        return;
    }

    len += snprintf(buf + len, sizeof(buf) - len,
                    "\r\n==============================================\r\n"
                    "Total de magias conhecidas: %d\r\n\r\n"
                    "Dica: Você pode dizer as sílabas místicas em voz alta para conjurar.\r\n"
                    "Exemplo: say %s\r\n"
                    "Você também pode incluir um alvo após as sílabas.\r\n"
                    "Exemplo: say %s <nome do alvo>\r\n",
                    count, count > 0 ? syllables : "ignisaegis", count > 0 ? syllables : "ignisaegis");

    page_string(ch->desc, buf, TRUE);
}

ACMD(do_experiment)
{
    struct str_spells *ptr;
    char syllables[256];
    char spoken_lower[256];
    int i;

    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs não podem experimentar com sílabas.\r\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch,
                     "Uso: experiment <silabas misticas>\r\n\r\n"
                     "Experimente combinar sílabas místicas que você aprendeu para descobrir\r\n"
                     "variantes de magias que você já conhece.\r\n\r\n"
                     "Exemplo: experiment aquaaegis\r\n"
                     "         (pode descobrir uma variante de watershield se você conhece fireshield)\r\n\r\n"
                     "Use o comando 'syllables' para ver as sílabas das magias que você conhece.\r\n");
        return;
    }

    /* Convert spoken text to lowercase for comparison */
    strlcpy(spoken_lower, argument, sizeof(spoken_lower));
    for (i = 0; spoken_lower[i]; i++)
        spoken_lower[i] = LOWER(spoken_lower[i]);

    /* Check all spells to see if the syllables match a discoverable variant */
    for (ptr = list_spells; ptr; ptr = ptr->next) {
        /* Only check discoverable spell variants */
        if (ptr->status != available || ptr->type != SPELL || !ptr->discoverable)
            continue;

        /* Validate spell vnum is within valid bounds */
        if (ptr->vnum <= 0 || ptr->vnum > MAX_SKILLS)
            continue;

        /* Validate prerequisite vnum if present */
        if (ptr->prerequisite_spell > 0 && ptr->prerequisite_spell > MAX_SKILLS)
            continue;

        /* Validate spell name exists */
        if (!ptr->name)
            continue;

        /* Check if player already knows this spell */
        if (GET_SKILL(ch, ptr->vnum) > 0)
            continue;

        /* Check if player knows the prerequisite spell */
        if (ptr->prerequisite_spell > 0 && GET_SKILL(ch, ptr->prerequisite_spell) == 0)
            continue;

        /* Convert this spell's name to syllables */
        spell_to_syllables_public(ptr->name, syllables, sizeof(syllables));

        /* Check if spoken syllables match */
        if (!strcmp(syllables, spoken_lower)) {
            /* Found a match! Teach the spell to the player at the same level as prerequisite */
            int learned_level = (ptr->prerequisite_spell > 0) ? GET_SKILL(ch, ptr->prerequisite_spell) : 15;

            SET_SKILL(ch, ptr->vnum, learned_level);

            send_to_char(ch,
                         "@GÊxito na experimentação!@n\r\n\r\n"
                         "As sílabas místicas ressoam com poder conhecido... Você sente uma conexão\r\n"
                         "com magias que já domina e percebe como adaptá-las!\r\n\r\n"
                         "Você descobriu a magia: @Y%s@n\r\n\r\n"
                         "Use 'syllables' para ver suas novas sílabas místicas.\r\n",
                         ptr->name);

            /* Log the discovery */
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
                   "%s discovered spell variant '%s' through experimentation", GET_NAME(ch), ptr->name);

            return;
        }
    }

    /* No match found */
    send_to_char(ch,
                 "Você tenta combinar as sílabas místicas, mas elas não ressoam com nenhum\r\n"
                 "conhecimento que você possui. Talvez você precise aprender magias relacionadas\r\n"
                 "primeiro, ou essas sílabas não correspondem a nenhuma variante descobrível.\r\n");
}

/**
 * Update variant skill levels when a prerequisite skill levels up.
 * When a skill is improved, all known variant skills that depend on it
 * should be updated to match the prerequisite skill level.
 * This function recursively updates chains (A -> B -> C).
 *
 * @param ch The character whose skills are being updated
 * @param prerequisite_vnum The vnum of the prerequisite skill that was leveled up
 * @param new_level The new level of the prerequisite skill
 */
void update_variant_skills(struct char_data *ch, int prerequisite_vnum, int new_level)
{
    struct str_spells *ptr;

    /* Validate ch pointer */
    if (!ch)
        return;

    if (IS_NPC(ch))
        return;

    /* Validate prerequisite_vnum is within valid bounds */
    if (prerequisite_vnum <= 0 || prerequisite_vnum > MAX_SKILLS)
        return;

    /* Iterate through all spells to find variants with this prerequisite */
    for (ptr = list_spells; ptr; ptr = ptr->next) {
        /* Check if this spell has the specified prerequisite */
        if (ptr->prerequisite_spell != prerequisite_vnum)
            continue;

        /* Validate variant vnum is within valid bounds before accessing skill array */
        if (ptr->vnum <= 0 || ptr->vnum > MAX_SKILLS)
            continue;

        /* Validate spell name exists before using it */
        if (!ptr->name)
            continue;

        /* Check if player knows this variant */
        if (GET_SKILL(ch, ptr->vnum) == 0)
            continue;

        /* Update variant skill level to match prerequisite if it's lower */
        if (GET_SKILL(ch, ptr->vnum) < new_level) {
            SET_SKILL(ch, ptr->vnum, new_level);
            send_to_char(ch,
                         "@GVariante atualizada:@n Sua proficiência em @Y%s@n aumentou para %d "
                         "para corresponder à habilidade pré-requisito.\r\n",
                         ptr->name, new_level);

            /* Recursively update any variants that depend on this variant (chain update) */
            update_variant_skills(ch, ptr->vnum, new_level);
        }
    }
}

ACMD(do_visible)
{
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        if (PLR_FLAGGED(ch, PLR_TRNS))
            send_to_char(ch, "Você não pode ficar totalmente visível.\r\n");
        else
            perform_immort_vis(ch);
        return;
    }

    if AFF_FLAGGED (ch, AFF_INVISIBLE) {
        appear(ch);
        send_to_char(ch, "Você encerra a magia da invisibilidade.\r\n");
    } else
        send_to_char(ch, "Você já está visível.\r\n");
}

ACMD(do_title)
{
    skip_spaces(&argument);
    delete_doubledollar(argument);
    parse_at(argument);

    if (IS_NPC(ch))
        send_to_char(ch, "Seu título já está ótimo... deixe assim.\r\n");
    else if (PLR_FLAGGED(ch, PLR_NOTITLE | PLR_JAILED))
        send_to_char(ch, "Você não pode mudar seu título -- você deve ter abusado!\r\n");
    else if (strstr(argument, "(") || strstr(argument, ")") || strstr(argument, "[") || strstr(argument, "]") ||
             strstr(argument, "<") || strstr(argument, ">") || strstr(argument, "{") || strstr(argument, "}"))
        send_to_char(ch, "Títulos não podem conter os caracteres (, ), [, ], <, >, { ou }.\r\n");
    else if (strlen(argument) > MAX_TITLE_LENGTH)
        send_to_char(ch, "Sinto muito, os títulos não podem ser maiores que %d letras.\r\n", MAX_TITLE_LENGTH);
    else {
        set_title(ch, argument);
        send_to_char(ch, "Pronto, você agora é %s%s%s.\r\n", GET_NAME(ch), *GET_TITLE(ch) ? " " : "", GET_TITLE(ch));
    }
}

ACMD(do_autotitle)
{
    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs não podem alterar configurações de autotitle.\r\n");
        return;
    }

    if (PRF_FLAGGED(ch, PRF_AUTOTITLE)) {
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOTITLE);
        send_to_char(ch, "Seu título não será mais alterado automaticamente.\r\n");
    } else {
        SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOTITLE);
        send_to_char(ch, "Seu título será alterado automaticamente sempre que evoluir um nível.\r\n");
    }
}

static void print_group(struct char_data *ch)
{
    struct char_data *k;

    send_to_char(ch, "\tWSeu grupo consiste de:\tn\r\n");

    while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
        send_to_char(ch, "%-*s: %s[%4d/%-4d]H [%4d/%-4d]M [%4d/%-4d]V%s\r\n", count_color_chars(GET_NAME(k)) + 22,
                     GET_NAME(k), GROUP_LEADER(GROUP(ch)) == k ? CBGRN(ch, C_NRM) : CCGRN(ch, C_NRM), GET_HIT(k),
                     GET_MAX_HIT(k), GET_MANA(k), GET_MAX_MANA(k), GET_MOVE(k), GET_MAX_MOVE(k), CCNRM(ch, C_NRM));
}

static void display_group_list(struct char_data *ch)
{
    struct group_data *group;
    int count = 0;

    if (group_list->iSize) {
        send_to_char(ch,
                     "#   Grupo Líder      # de Membros    Na Área\r\n"
                     "---------------------------------------------------\r\n");

        while ((group = (struct group_data *)simple_list(group_list)) != NULL) {
            if (IS_SET(GROUP_FLAGS(group), GROUP_NPC))
                continue;
            if (GROUP_LEADER(group) && !IS_SET(GROUP_FLAGS(group), GROUP_ANON))
                send_to_char(ch, "%-2d) %s%-12s     %-2d              %s%s\r\n", ++count,
                             IS_SET(GROUP_FLAGS(group), GROUP_OPEN) ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM),
                             GET_NAME(GROUP_LEADER(group)), group->members->iSize,
                             zone_table[world[IN_ROOM(GROUP_LEADER(group))].zone].name, CCNRM(ch, C_NRM));
            else
                send_to_char(ch, "%-2d) Oculto\r\n", ++count);
        }
    }
    if (count)
        send_to_char(ch,
                     "\r\n"
                     "%sProcurando Membros%s\r\n"
                     "%sFechado%s\r\n",
                     CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    else
        send_to_char(ch,
                     "\r\n"
                     "Atualmente sem grupos formados.\r\n");
}

/* Vatiken's Group System: Version 1.1 */
ACMD(do_group)
{
    char buf[MAX_STRING_LENGTH];
    struct char_data *vict;

    argument = one_argument(argument, buf);

    if (!*buf) {
        if (GROUP(ch))
            print_group(ch);
        else
            send_to_char(ch, "Fazer o quê com o grupo?\r\n");
        return;
    }

    if (is_abbrev(buf, "new") || is_abbrev(buf, "novo")) {
        if (GROUP(ch))
            send_to_char(ch, "Você já está em um grupo.\r\n");
        else
            create_group(ch);
    } else if (is_abbrev(buf, "list"))
        display_group_list(ch);
    else if (is_abbrev(buf, "join") || is_abbrev(buf, "entrar")) {
        skip_spaces(&argument);
        if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM))) {
            send_to_char(ch, "Entrar no grupo de quem?\r\n");
            return;
        } else if (vict == ch) {
            send_to_char(ch, "Este seria um grupo muito solitário.\r\n");
            return;
        } else if (GROUP(ch)) {
            send_to_char(ch, "Mas vocé já faz parte de um grupo.\r\n");
            return;
        } else if (!GROUP(vict)) {
            act("$N não faz parte de nenhum grupo!", FALSE, ch, 0, vict, TO_CHAR);
            return;
        }

        /******************************************************************
         * NOVA LÓGICA DE IA: Interação Jogador -> Mob Líder
         ******************************************************************/
        else if (IS_NPC(vict) && GROUP_LEADER(GROUP(vict)) == vict) {

            /* 1. Verifica a compatibilidade (nível e tamanho) */
            if (GROUP(vict)->members->iSize >= 6) {
                act("O grupo de $N já está cheio.", FALSE, ch, 0, vict, TO_CHAR);
                return;
            }
            if (!is_level_compatible_with_group(ch, GROUP(vict))) {
                act("$N parece achar que você não se encaixaria bem no grupo.", FALSE, ch, 0, vict, TO_CHAR);
                return;
            }

            if (!are_groupable(ch, vict)) { /* are_groupable verifica o alinhamento */
                act("$N olha para si com desconfiança devido ao seu alinhamento.", FALSE, ch, 0, vict, TO_CHAR);
                return;
            }

            /* 2. O líder mob toma a sua decisão. */
            int chance_aceitar = 100 - (GROUP(vict)->members->iSize * 15) + GET_GENGROUP(vict);
            if (rand_number(1, 120) <= chance_aceitar) {
                /* ACEITOU */
                act("$N acena com a cabeça, te aceitando no seu bando.", FALSE, ch, 0, vict, TO_CHAR);
                join_group(ch, GROUP(vict));
            } else {
                /* RECUSOU */
                act("$N olha para você com desconfiança e rejeita a sua companhia.", FALSE, ch, 0, vict, TO_CHAR);
                return;
            }
        }
        /******************************************************************
         * Fim da Lógica de IA. A lógica original para grupos de jogadores continua.
         ******************************************************************/
        else if (!IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN)) {
            send_to_char(ch, "Este grupo não está aceitando novos membros.\r\n");
            return;
        }
        join_group(ch, GROUP(vict));
    } else if (is_abbrev(buf, "kick") || is_abbrev(buf, "expulsar")) {
        skip_spaces(&argument);
        if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM))) {
            send_to_char(ch, "Expulsar quem do grupo?\r\n");
            return;
        } else if (vict == ch) {
            send_to_char(ch, "Existem jeitos mais fáceis de sair do grupo.\r\n");
            return;
        } else if (!GROUP(ch)) {
            send_to_char(ch, "Mas você não faz parte de nenhum grupo!\r\n");
            return;
        } else if (GROUP_LEADER(GROUP(ch)) != ch) {
            send_to_char(ch, "Você não pode expulsar membros do seu grupo sem ser o líder.\r\n");
            return;
        } else if (GROUP(vict) != GROUP(ch)) {
            act("$N não faz parte do seu grupo!", FALSE, ch, 0, vict, TO_CHAR);
            return;
        }
        send_to_char(ch, "%s não é mais membro de seu grupo.\r\n", GET_NAME(vict));
        send_to_char(vict, "Você foi chutad%s para fora do grupo.\r\n", OA(vict));
        leave_group(vict);
    } else if (is_abbrev(buf, "regroup") || is_abbrev(buf, "regrupar")) {
        if (!GROUP(ch)) {
            send_to_char(ch, "Mas você não faz parte de nenhum grupo!\r\n");
            return;
        }
        vict = GROUP_LEADER(GROUP(ch));
        if (ch == vict) {
            send_to_char(ch, "Você é o lider e não pode reagrupar.\r\n");
        } else {
            leave_group(ch);
            join_group(ch, GROUP(vict));
        }
    } else if (is_abbrev(buf, "leave") || is_abbrev(buf, "sair")) {

        if (!GROUP(ch)) {
            send_to_char(ch, "Mas você não faz parte de nenhum grupo!\r\n");
            return;
        }

        leave_group(ch);
    } else if (is_abbrev(buf, "option")) {
        skip_spaces(&argument);
        if (!GROUP(ch)) {
            send_to_char(ch, "Mas você não faz parte de nenhum grupo!\r\n");
            return;
        } else if (GROUP_LEADER(GROUP(ch)) != ch) {
            send_to_char(ch, "Somente o líder pode ajustar as opçoes do grupo.\r\n");
            return;
        }
        if (is_abbrev(argument, "open") || is_abbrev(argument, "aberto")) {
            TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN);
            send_to_char(ch, "O grupo agora está %s para novos membros.\r\n",
                         IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN) ? "aberto" : "fechado");
        } else if (is_abbrev(argument, "anonymous") || is_abbrev(argument, "escondido")) {
            TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_ANON);
            send_to_char(ch, "A localização do grupo agora está %s para os outros jogadores.\r\n",
                         IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_ANON) ? "escondida" : "disponível");
        } else
            send_to_char(ch, "As opções de grupo são: Open (Aberto), Anonymous (Escondido).\r\n");
    } else {
        send_to_char(ch, "Você precisa especificar o comando do grupo.\r\n");
    }
}

ACMD(do_report)
{
    struct group_data *group;

    if ((group = GROUP(ch)) == NULL) {
        send_to_char(ch, "Mas você não é membro de um grupo!\r\n");
        return;
    }
    send_to_group(NULL, group, "%s relata: %d/%dHp, %d/%dMn, %d/%dMv,%d/10Ac\r\n", GET_NAME(ch), GET_HIT(ch),
                  GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch), GET_MOVE(ch), GET_MAX_MOVE(ch),
                  compute_armor_class(ch));
}

ACMD(do_split)
{
    char buf[MAX_INPUT_LENGTH];
    int amount, num = 0, share, rest;
    size_t len;
    struct char_data *k;

    if (IS_NPC(ch)) {
        send_to_char(ch, "Infelizmente você não pode fazer isto\r\n");
        return;
    }

    one_argument(argument, buf);

    if (is_number(buf)) {
        amount = atoi(buf);
        if (amount <= 0) {
            send_to_char(ch, "Desculpe, mas você não pode fazer isso.\r\n");
            return;
        }
        if (amount > GET_GOLD(ch)) {
            send_to_char(ch, "Você não parece ter tanto ouro para dividir.\r\n");
            return;
        }

        if (GROUP(ch))
            while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
                if (IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k))
                    num++;

        if (num && GROUP(ch)) {
            share = amount / num;
            rest = amount % num;
        } else {
            send_to_char(ch, "Você precisa pertencer a um grupo para poder dividir seu ouro.\r\n");
            return;
        }

        decrease_gold(ch, share * (num - 1));

        /* Abusing signed/unsigned to make sizeof work. */
        len = snprintf(buf, sizeof(buf), "%s divide %d coins; você recebe %d.\r\n", GET_NAME(ch), amount, share);
        if (rest && len < sizeof(buf)) {
            snprintf(buf + len, sizeof(buf) - len, "%d moeda%s não %s, então %s ficou com o resto.\r\n", rest,
                     (rest == 1) ? "" : "s", (rest == 1) ? "pode ser dividida" : "puderam ser divididas", GET_NAME(ch));
        }

        while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
            if (k != ch && IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k)) {
                increase_gold(k, share);
                send_to_char(k, "%s", buf);
            }

        send_to_char(ch, "Voce divide %d moedas entre %d membros -- %d moedas para cada um.\r\n", amount, num, share);

        if (rest) {
            send_to_char(ch, "%d moeda%s não %s, então você ficou com o resto.\r\n", rest, (rest == 1) ? "" : "s",
                         (rest == 1) ? "pode ser dividida" : "puderam ser divididas");
            increase_gold(ch, rest);
        }
    } else {
        send_to_char(ch, "Quantas moedas voce deseja dividir com o resto do grupo?\r\n");
        return;
    }
}

ACMD(do_use)
{
    char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
    struct obj_data *mag_item;

    half_chop(argument, arg, buf);
    if (!*arg) {
        send_to_char(ch, "O quê você deseja %s?\r\n",
                     subcmd == SCMD_RECITE  ? "recitar"
                     : subcmd == SCMD_QUAFF ? "tomar"
                                            : "usar");
        return;
    }
    mag_item = GET_EQ(ch, WEAR_HOLD);

    if (!mag_item || !isname(arg, mag_item->name)) {
        switch (subcmd) {
            case SCMD_RECITE:
            case SCMD_QUAFF:
                if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
                    send_to_char(ch, "Você não parece ter %s.\r\n", arg);
                    return;
                }
                break;
            case SCMD_USE:
                send_to_char(ch, "Você não parece estar usando %s.\r\n", arg);
                return;
            default:
                log1("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
                /* SYSERR_DESC: This is the same as the unhandled case in
                   do_gen_ps(), but in the function which handles 'quaff',
                   'recite', and 'use'. */
                return;
        }
    }
    switch (subcmd) {
        case SCMD_QUAFF:
            if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
                send_to_char(ch, "Você só pode tomar poções.\r\n");
                return;
            }
            break;
        case SCMD_RECITE:
            if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
                send_to_char(ch, "Você só pode recitar pergaminhos.\r\n");
                return;
            }
            break;
        case SCMD_USE:
            if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) && (GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
                send_to_char(ch, "Você não imagina como usar isso.\r\n");
                return;
            }
            break;
    }

    mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_display)
{
    size_t i;

    if (IS_NPC(ch)) {
        send_to_char(ch, "Monstros não precisam disso.  Vá embora.\r\n");
        return;
    }
    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Uso: prompt { { H | M | V } | tudo | auto | nada }\r\n");
        return;
    }

    if (!str_cmp(argument, "auto")) {
        TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);
        send_to_char(ch, "Auto prompt %sabilitado.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "h" : "des");
        return;
    }

    if (!str_cmp(argument, "tudo") || !str_cmp(argument, "liga") ||
        (!str_cmp(argument, "on") || !str_cmp(argument, "all"))) {
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    } else if (!str_cmp(argument, "desliga") || !str_cmp(argument, "nada") || !str_cmp(argument, "off") ||
               !str_cmp(argument, "none")) {
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    } else {
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
        REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);

        for (i = 0; i < strlen(argument); i++) {
            switch (LOWER(argument[i])) {
                case 'h':
                    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
                    break;
                case 'm':
                    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
                    break;
                case 'v':
                    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
                    break;
                default:
                    send_to_char(ch, "Uso: prompt { { H | M | V } | tudo | auto | nada }\r\n");
                    return;
            }
        }
    }

    send_to_char(ch, "%s", CONFIG_OK);
}

#define TOG_OFF 0
#define TOG_ON 1
ACMD(do_gen_tog)
{
    long result;
    int i;
    char arg[MAX_INPUT_LENGTH];

    const char *tog_messages[][2] = {
        {"Você agora não poderá ser convocado por outros jogadores.\r\n",
         "Você agora pode ser convocado por outros jogadores.\r\n"},
        {"Nohassle desabilitado.\r\n", "Nohassle habilitado.\r\n"},
        {"Modo 'Breve' desligado.\r\n", "Modo 'Breve' ligado.\r\n"},
        {"Modo 'Compacto' desligado.\r\n", "Modo 'Compacto' ligado.\r\n"},
        {"Canal TELL ativado.\r\n", "Canal TELL desativado.\r\n"},
        {"Canal AUCTION ativado.\r\n", "Canal AUCTION desativado.\r\n"},
        {"Canal SHOUT ativado.\r\n", "Canal SHOUT desativado.\r\n"},
        {"Canal GOSS ativado.\r\n", "Canal GOSS desativado.\r\n"},
        {"Canal GRATZ ativado.\r\n", "Canal GRATZ desativado.\r\n"},
        {"Canal WIZ ativado.\r\n", "Canal WIZ desativado.\r\n"},
        {"Você não faz mais parte do grupo de busca (quest)!\r\n",
         "Ok, agora você faz parte do grupo de busca (quest)!\r\n"},
        {"Você não irá mais ver as flags das salas.\r\n", "Agora você irá ver as flags das salas.\r\n"},
        {"Agora você verá suas frases repetidas.\r\n", "Você não irá mais ver suas frases repetidas.\r\n"},
        {"Modo 'HolyLight' desligado.\r\n", "Modo 'HolyLight' ligado.\r\n"},
        {"Modo 'SlowNS' desligado. Endereços IP serão resolvidos.\r\n",
         "Modo 'SlowNS' ligado. Endereços IP não serão resolvidos.\r\n"},
        {"Autoexits desligado.\r\n", "Autoexits ligado.\r\n"},
        {"Não será mais possível rastrear através de portas.\r\n",
         "Agora será possível rastrear através de portas.\r\n"},
        {"A tela não vai ser limpa no OLC.\r\n", "A tela vai ser limpa no OLC.\r\n"},
        {"Modo 'Construtor' desligado.\r\n", "Modo 'Construtor' ligado.\r\n"},
        {"AWAY desligado.\r\n", "AWAY ligado.\r\n"},
        {"Autoloot desligado.\r\n", "Autoloot ligado.\r\n"},
        {"Autogold desligado.\r\n", "Autogold ligado.\r\n"},
        {"Autosplit desligado.\r\n", "Autosplit ligado.\r\n"},
        {"Autosac desligado.\r\n", "Autosac ligado.\r\n"},
        {"Autoassist desligado.\r\n", "Autoassist ligado.\r\n"},
        {"Agora, você não irá mais ver o  mini-mapa.\r\n", "Agora, você irá ver o  mini-mapa.\r\n"},
        {"Você agora precisa destrancar as portas manualmente.\r\n",
         "Você agora vai destrancar as portas automaticamente ao abrir (se tiver a chave).\r\n"},
        {"Você agora precisa especificar uma direção ao abrir, fechar e destrancar.\r\n",
         "Você agora vai encontrar a próxima porta disponível ao abrir, fechar ou destrancar.\r\n"},
        {"Você não irá mais ver os resets de áreas.\r\n", "Você irá mais ver os resets de áreas.\r\n"},
        {"Você não verá mais a saúde do oponente durante a luta.\r\n",
         "Agora você verá a saúde do oponente durante a luta.\r\n"},
        {"Seu título não será mais alterado automaticamente.\r\n",
         "Seu título será alterado automaticamente sempre que evoluir um nível.\r\n"}};

    if (IS_NPC(ch))
        return;

    switch (subcmd) {
        case SCMD_NOSUMMON:
            result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
            break;
        case SCMD_NOHASSLE:
            result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
            break;
        case SCMD_BRIEF:
            result = PRF_TOG_CHK(ch, PRF_BRIEF);
            break;
        case SCMD_COMPACT:
            result = PRF_TOG_CHK(ch, PRF_COMPACT);
            break;
        case SCMD_NOTELL:
            result = PRF_TOG_CHK(ch, PRF_NOTELL);
            break;
        case SCMD_NOAUCTION:
            result = PRF_TOG_CHK(ch, PRF_NOAUCT);
            break;
        case SCMD_NOSHOUT:
            result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
            break;
        case SCMD_NOGOSSIP:
            result = PRF_TOG_CHK(ch, PRF_NOGOSS);
            break;
        case SCMD_NOGRATZ:
            result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
            break;
        case SCMD_NOWIZ:
            result = PRF_TOG_CHK(ch, PRF_NOWIZ);
            break;
        case SCMD_QUEST:
            result = PRF_TOG_CHK(ch, PRF_QUEST);
            break;
        case SCMD_SHOWVNUMS:
            result = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
            break;
        case SCMD_NOREPEAT:
            result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
            break;
        case SCMD_HOLYLIGHT:
            result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
            break;
        case SCMD_AUTOEXIT:
            result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
            break;
        case SCMD_CLS:
            result = PRF_TOG_CHK(ch, PRF_CLS);
            break;
        case SCMD_BUILDWALK:
            if (GET_LEVEL(ch) < LVL_BUILDER) {
                send_to_char(ch, "Apenas construtores, desculpe.\r\n");
                return;
            }
            result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
            if (PRF_FLAGGED(ch, PRF_BUILDWALK)) {
                one_argument(argument, arg);
                for (i = 0; *arg && *(sector_types[i]) != '\n'; i++)
                    if (is_abbrev(arg, sector_types[i]))
                        break;
                if (*(sector_types[i]) == '\n')
                    i = 0;
                GET_BUILDWALK_SECTOR(ch) = i;
                send_to_char(ch, "Setor Padrão %s\r\n", sector_types[i]);

                mudlog(CMP, GET_LEVEL(ch), TRUE, "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch),
                       GET_OLC_ZONE(ch));
            } else
                mudlog(CMP, GET_LEVEL(ch), TRUE, "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch),
                       GET_OLC_ZONE(ch));
            break;
        case SCMD_AFK:
            result = PRF_TOG_CHK(ch, PRF_AFK);
            if (PRF_FLAGGED(ch, PRF_AFK))
                act("$n agora está longe do teclado.", TRUE, ch, 0, 0, TO_ROOM);
            else {
                act("$n retornou ao teclado.", TRUE, ch, 0, 0, TO_ROOM);
                if (has_mail(GET_IDNUM(ch)))
                    send_to_char(ch, "Você tem carta esperando.\r\n");
            }
            break;
        case SCMD_AUTOLOOT:
            result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
            break;
        case SCMD_AUTOGOLD:
            result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
            break;
        case SCMD_AUTOSPLIT:
            result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
            break;
        case SCMD_AUTOSAC:
            result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
            break;
        case SCMD_AUTOASSIST:
            result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
            break;
        case SCMD_AUTOMAP:
            result = PRF_TOG_CHK(ch, PRF_AUTOMAP);
            break;
        case SCMD_AUTOKEY:
            result = PRF_TOG_CHK(ch, PRF_AUTOKEY);
            break;
        case SCMD_AUTODOOR:
            result = PRF_TOG_CHK(ch, PRF_AUTODOOR);
            break;
        case SCMD_ZONERESETS:
            result = PRF_TOG_CHK(ch, PRF_ZONERESETS);
            break;
        case SCMD_HITBAR:
            result = PRF_TOG_CHK(ch, PRF_HITBAR);
            break;
        case SCMD_AUTOTITLE:
            result = PRF_TOG_CHK(ch, PRF_AUTOTITLE);
            break;
        default:
            log1("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
            return;
    }

    if (result)
        send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
    else
        send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

    return;
}

static void show_happyhour(struct char_data *ch)
{
    char happyexp[80], happygold[80], happyqp[80];
    int secs_left;

    if ((IS_HAPPYHOUR) || (GET_LEVEL(ch) >= LVL_GRGOD)) {
        if (HAPPY_TIME)
            secs_left = ((HAPPY_TIME - 1) * SECS_PER_MUD_HOUR) + next_tick;
        else
            secs_left = 0;

        sprintf(happyqp, "%s+%d%%%s para Questpoints por quest.\r\n", CCYEL(ch, C_NRM), HAPPY_QP, CCNRM(ch, C_NRM));
        sprintf(happygold, "%s+%d%%%s para Moedas ganhas por morte.\r\n", CCYEL(ch, C_NRM), HAPPY_GOLD,
                CCNRM(ch, C_NRM));
        sprintf(happyexp, "%s+%d%%%s para Experiencia por morte.\r\n", CCYEL(ch, C_NRM), HAPPY_EXP, CCNRM(ch, C_NRM));

        send_to_char(ch,
                     "Happy Hour de Vitália!\r\n"
                     "------------------\r\n"
                     "%s%s%sRestam: %s%d%s horas %s%d%s minutos %s%d%s segundos\r\n",
                     (IS_HAPPYEXP || (GET_LEVEL(ch) >= LVL_GOD)) ? happyexp : "",
                     (IS_HAPPYGOLD || (GET_LEVEL(ch) >= LVL_GOD)) ? happygold : "",
                     (IS_HAPPYQP || (GET_LEVEL(ch) >= LVL_GOD)) ? happyqp : "", CCYEL(ch, C_NRM), (secs_left / 3600),
                     CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), (secs_left % 3600) / 60, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
                     (secs_left % 60), CCNRM(ch, C_NRM));
    } else {
        send_to_char(ch, "Desculpa, mas não tem nenhum happy hour acontecendo!\r\n");
    }
}

ACMD(do_happyhour)
{
    char arg[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
    int num;

    if (GET_LEVEL(ch) < LVL_GOD) {
        show_happyhour(ch);
        return;
    }

    /* Only Imms get here, so check args */
    two_arguments(argument, arg, val);

    if (is_abbrev(arg, "experiencia")) {
        num = MIN(MAX((atoi(val)), 0), 1000);
        HAPPY_EXP = num;
        send_to_char(ch, "Happy Hour Exp aumentado para +%d%%\r\n", HAPPY_EXP);
    } else if ((is_abbrev(arg, "ouro")) || (is_abbrev(arg, "moedas"))) {
        num = MIN(MAX((atoi(val)), 0), 1000);
        HAPPY_GOLD = num;
        send_to_char(ch, "Happy Hour Ouro aumentado para +%d%%\r\n", HAPPY_GOLD);
    } else if ((is_abbrev(arg, "tempo")) || (is_abbrev(arg, "ticks"))) {
        num = MIN(MAX((atoi(val)), 0), 1000);
        if (HAPPY_TIME && !num)
            game_info("Happyhour chegou ao fim!");
        else if (!HAPPY_TIME && num)
            game_info("Um Happyhour começou!");

        HAPPY_TIME = num;
        send_to_char(ch, "Happy Hour configurado para %d ticks (%d:%d:%d)\r\n", HAPPY_TIME,
                     (HAPPY_TIME * SECS_PER_MUD_HOUR) / 3600, ((HAPPY_TIME * SECS_PER_MUD_HOUR) % 3600) / 60,
                     (HAPPY_TIME * SECS_PER_MUD_HOUR) % 60);
    } else if ((is_abbrev(arg, "qp")) || (is_abbrev(arg, "questpoints"))) {
        num = MIN(MAX((atoi(val)), 0), 1000);
        HAPPY_QP = num;
        send_to_char(ch, "Happy Hour Questpoints aumentados para +%d%%\r\n", HAPPY_QP);
    } else if (is_abbrev(arg, "show")) {
        show_happyhour(ch);
    } else if (is_abbrev(arg, "default") || is_abbrev(arg, "padrão")) {
        HAPPY_EXP = 100;
        HAPPY_GOLD = 50;
        HAPPY_QP = 50;
        HAPPY_TIME = 48;
        game_info("Um Happyhour foi iniciado!");
    } else {
        send_to_char(ch,
                     "Usage: %shappyhour              %s- show usage (this info)\r\n"
                     "       %shappyhour show         %s- display current settings (what mortals see)\r\n"
                     "       %shappyhour time <ticks> %s- set happyhour time and start timer\r\n"
                     "       %shappyhour qp <num>     %s- set qp percentage gain\r\n"
                     "       %shappyhour exp <num>    %s- set exp percentage gain\r\n"
                     "       %shappyhour gold <num>   %s- set gold percentage gain\r\n"
                     "       %shappyhour default      %s- sets a default setting for happyhour\r\n\r\n"
                     "Configure the happyhour settings and start a happyhour.\r\n"
                     "Currently 1 hour IRL = %d ticks\r\n"
                     "If no number is specified, 0 (off) is assumed.\r\nThe command \tyhappyhour time\tn will "
                     "therefore stop the happyhour timer.\r\n",
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
                     CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     (3600 / SECS_PER_MUD_HOUR));
    }
}

int can_rebegin(struct char_data *ch)
{
    if (!PLR_FLAGGED(ch, PLR_TRNS))
        return (0);

    return (1);
}

void show_class_skills(struct char_data *ch, int class_num)
{
    int i;
    send_to_char(ch, "\r\nHabilidades e magias disponíveis para %s:\r\n", pc_class_types[class_num]);

    /* Show skills the character currently has that are available to their class */
    for (i = 1; i <= MAX_SKILLS; i++) {
        if (GET_SKILL(ch, i) > 0 && get_spell_level(i, class_num) >= 0) {
            send_to_char(ch, "%3d. %s (%d%%)\r\n", i, skill_name(i), GET_SKILL(ch, i));
        }
    }

    /* For bards, also show chansons */
    if (class_num == CLASS_BARD) {
        for (i = CHANSON_ARDOR; i <= MAX_CHANSONS; i++) {
            if (GET_SKILL(ch, i) > 0 && get_spell_level(i, class_num) >= 0) {
                send_to_char(ch, "%3d. %s (%d%%)\r\n", i, skill_name(i), GET_SKILL(ch, i));
            }
        }
    }
    send_to_char(ch, "\r\nDigite o número da habilidade que deseja manter, ou 0 para não manter nenhuma: ");
}

void show_menu_with_options(struct descriptor_data *d)
{
    char menu_buffer[4096];

    strcpy(menu_buffer,
           "\r\n"
           "O MUNDO DE VITALIA\r\n"
           "\r\n"
           "         .-.\r\n"
           "        (0.0)\r\n"
           "      '=.|m|.='\r\n"
           "      .='/	\\`=.\r\n"
           "         	8	\r\n"
           "     _   8	8   _\r\n"
           "    (	__/	8	\\__	)\r\n"
           "     `-=:8	8:=-'\r\n"
           "         |:|\r\n"
           "         |:|   [1] Entrar no jogo.\r\n"
           "         |:|   [2]  Ler a historia deste mundo.\r\n"
           "         |:|   [3]  Ajustar a descriçao\r\n"
           "         |:|   [4]  Mudar a sua senha de acesso\r\n");

    /* Add rebegin option if eligible */
    if (d->character && can_rebegin(d->character)) {
        strcat(menu_buffer, "         |:|   [5]  Renascer (Rebegin)\r\n");
    }

    /* Add elevate option if eligible */
    if (d->character && can_elevate(d->character)) {
        strcat(menu_buffer, "         |:|   [6]  Transcender (Elevate)\r\n");
    }

    strcat(menu_buffer,
           "         |:|   [9]  Apagar o personagem\r\n"
           "         |:|   [0]  Deixar este mundo\r\n"
           "         |:|\r\n"
           "         |:|\r\n"
           "         |:|\r\n"
           "         |:|\r\n"
           "         |:|\r\n"
           "        \\:/\r\n"
           "          ^\r\n"
           "\r\n"
           "\r\n"
           "-=>  Faca sua escolha: ");

    write_to_output(d, "%s", menu_buffer);
}

int can_elevate(struct char_data *ch)
{
    int i, classes_experienced = 0;

    /* Must be transcended */
    if (!PLR_FLAGGED(ch, PLR_TRNS))
        return (0);

    /* Must be at maximum mortal level */
    if (GET_LEVEL(ch) != (LVL_IMMORT - 1))
        return (0);

    /* Must have enough experience to level to next level */
    if (GET_EXP(ch) < level_exp(GET_CLASS(ch), LVL_IMMORT))
        return (0);

    /* Must have experienced all classes at least once */
    for (i = 0; i < NUM_CLASSES; i++) {
        if (WAS_FLAGGED(ch, i) || GET_CLASS(ch) == i)
            classes_experienced++;
    }
    if (classes_experienced < NUM_CLASSES)
        return (0);

    /* Config must allow mortal to immortal advancement */
    if (CONFIG_NO_MORT_TO_IMMORT)
        return (0);

    return (1);
}

ACMD(do_recall)
{
    int loss;
    struct char_data *was_fighting = NULL;

    if (IS_NPC(ch))
        return;

    if (GET_LEVEL(ch) > 10 && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "Você não pode mais usar este comando.\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_GHOST | PLR_JAILED)) {
        send_to_char(ch, "Nah... nem pensar!\r\n");
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)) {
        send_to_char(ch, "Sem Efeito");
        return;
    }

    if (FIGHTING(ch)) {
        was_fighting = FIGHTING(ch);
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
    }

    act("$n desaparece.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    switch (GET_HOMETOWN(ch)) {
        case 1:
            char_to_room(ch, r_hometown_1);
            break;
        case 2:
            char_to_room(ch, r_hometown_2);
            break;
        case 3:
            char_to_room(ch, r_hometown_3);
            break;
        case 4:
            char_to_room(ch, r_hometown_4);
            break;
        default:
            char_to_room(ch, r_hometown_1);
            break;
    }
    act("$n aparece no meio da sala.", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
    entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);
}

ACMD(do_rebegin)
{
    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs não podem renascer.\r\n");
        return;
    }

    if (!can_rebegin(ch)) {
        send_to_char(ch, "Você não pode renascer neste momento.\r\n");
        return;
    }

    if (GET_POS(ch) == POS_FIGHTING) {
        send_to_char(ch, "Você não pode renascer durante uma luta.\r\n");
        return;
    }

    /* Show rebegin information */
    send_to_char(ch, "%s", rebegin);
    show_class_skills(ch, GET_CLASS(ch));

    STATE(ch->desc) = CON_RB_SKILL;
}

ACMD(do_elevate)
{
    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs não podem transcender.\r\n");
        return;
    }

    if (!can_elevate(ch)) {
        send_to_char(ch, "Você não pode transcender neste momento.\r\n");
        return;
    }

    if (GET_POS(ch) == POS_FIGHTING) {
        send_to_char(ch, "Você não pode transcender durante uma luta.\r\n");
        return;
    }

    /* Show elevation confirmation message */
    send_to_char(ch, "Ao transcender, você se tornará um imortal e deixará para trás sua vida mortal.\r\n");
    send_to_char(ch, "Esta decisão é irreversível.\r\n\r\n");
    send_to_char(ch, "Deseja realmente transcender? (S/N): ");

    STATE(ch->desc) = CON_ELEVATE_CONF;
}

ACMD(do_mine)
{
    int skill_num = GET_SKILL(ch, SKILL_MINE);
    int percent, prob;
    struct obj_data *obj = NULL;
    obj_vnum vnum = NOTHING;

    if (!skill_num) {
        send_to_char(ch, "Você não sabe como minerar.\r\n");
        return;
    }

    if (GET_POS(ch) < POS_STANDING) {
        send_to_char(ch, "Você precisa estar em pé para minerar.\r\n");
        return;
    }

    // Check for pickaxe requirement
    if (!IS_MOB(ch)) {   // Only check equipment for players, mobs are assumed to have tools
        struct obj_data *pickaxe = NULL;
        int i;

        // Check for pickaxe in inventory or equipment
        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(ch, i) && (GET_OBJ_VNUM(GET_EQ(ch, i)) == 10711 || GET_OBJ_VNUM(GET_EQ(ch, i)) == 6514)) {
                pickaxe = GET_EQ(ch, i);
                break;
            }
        }

        if (!pickaxe) {
            for (obj = ch->carrying; obj; obj = obj->next_content) {
                if (GET_OBJ_VNUM(obj) == 10711 || GET_OBJ_VNUM(obj) == 6514) {
                    pickaxe = obj;
                    break;
                }
            }
        }

        if (!pickaxe) {
            send_to_char(ch, "Você precisa de uma picareta para minerar.\r\n");
            return;
        }
    }

    // Check if location allows mining (could be expanded with room flags)
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "Este lugar é muito pacífico para mineração.\r\n");
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE * 2);

    percent = rand_number(1, 101);
    prob = GET_SKILL(ch, SKILL_MINE);

    send_to_char(ch, "Você procura por minerais no local...\r\n");
    act("$n procura por minerais no local.", TRUE, ch, 0, 0, TO_ROOM);

    if (percent <= prob) {
        // Determine what was found based on skill level and luck
        int roll = rand_number(1, 100);

        if (roll <= 10 + (skill_num / 10)) {
            vnum = 1041;   // Gold ring (valuable treasure)
            send_to_char(ch, "Você encontra algo valioso!\r\n");
            act("$n encontra algo valioso!", TRUE, ch, 0, 0, TO_ROOM);
        } else if (roll <= 20 + (skill_num / 8)) {
            vnum = 2918;   // Zone 29 fairy eye emerald (rare gem)
            send_to_char(ch, "Você encontra uma esmeralda rara!\r\n");
            act("$n encontra uma esmeralda rara!", TRUE, ch, 0, 0, TO_ROOM);
        } else if (roll <= 35 + (skill_num / 6)) {
            vnum = 2904;   // Zone 29 sparkling ruby
            send_to_char(ch, "Você encontra um rubi cintilante!\r\n");
            act("$n encontra um rubi cintilante!", TRUE, ch, 0, 0, TO_ROOM);
        } else if (roll <= 45 + (skill_num / 7)) {
            vnum = 2905;   // Zone 29 sparkling ruby (alternate)
            send_to_char(ch, "Você encontra outro rubi!\r\n");
            act("$n encontra um rubi!", TRUE, ch, 0, 0, TO_ROOM);
        } else if (roll <= 60 + (skill_num / 5)) {
            vnum = 10711;   // Iron pickaxe (mining tool)
            send_to_char(ch, "Você encontra uma ferramenta de mineração.\r\n");
            act("$n encontra uma ferramenta de mineração.", TRUE, ch, 0, 0, TO_ROOM);
        } else {
            send_to_char(ch, "Você encontra apenas pedras sem valor.\r\n");
            act("$n encontra apenas pedras sem valor.", TRUE, ch, 0, 0, TO_ROOM);
        }

        // If we found something, create the object
        if (vnum != NOTHING) {
            /* For mobs, check if they already have this object to prevent resource overflow */
            if (IS_MOB(ch) && char_has_obj_vnum(ch, vnum)) {
                /* Mob already has this item, skip creation to prevent overflow */
            } else {
                obj = read_object(vnum, VIRTUAL);
                if (obj) {
                    obj_to_char(obj, ch);
                    send_to_char(ch, "Você pega %s.\r\n", GET_OBJ_SHORT(obj));
                }
            }
        }

        // Give some experience for successful mining
        gain_exp(ch, rand_number(5, 15));
    } else {
        send_to_char(ch, "Você não consegue encontrar nada útil.\r\n");
        act("$n não consegue encontrar nada útil.", TRUE, ch, 0, 0, TO_ROOM);
    }
}

ACMD(do_fishing)
{
    int skill_num = GET_SKILL(ch, SKILL_FISHING);
    int percent, prob;
    struct obj_data *obj = NULL;
    obj_vnum vnum = NOTHING;

    if (!skill_num) {
        send_to_char(ch, "Você não sabe como pescar.\r\n");
        return;
    }

    if (GET_POS(ch) < POS_SITTING) {
        send_to_char(ch, "Você precisa estar pelo menos sentado para pescar.\r\n");
        return;
    }

    // Check for fishing rod/harpoon requirement
    if (!IS_MOB(ch)) {   // Only check equipment for players, mobs are assumed to have tools
        struct obj_data *fishing_tool = NULL;
        int i;

        // Check for fishing harpoon in inventory or equipment
        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(ch, i) && GET_OBJ_VNUM(GET_EQ(ch, i)) == 4458) {
                fishing_tool = GET_EQ(ch, i);
                break;
            }
        }

        if (!fishing_tool) {
            for (obj = ch->carrying; obj; obj = obj->next_content) {
                if (GET_OBJ_VNUM(obj) == 4458) {
                    fishing_tool = obj;
                    break;
                }
            }
        }

        if (!fishing_tool) {
            send_to_char(ch, "Você precisa de uma vara de pescar ou arpão para pescar.\r\n");
            return;
        }
    }

    // Check if there's water here (could be expanded with sector types)
    if (SECT(IN_ROOM(ch)) != SECT_WATER_SWIM && SECT(IN_ROOM(ch)) != SECT_WATER_NOSWIM) {
        send_to_char(ch, "Você precisa estar próximo da água para pescar.\r\n");
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE * 3);

    percent = rand_number(1, 101);
    prob = GET_SKILL(ch, SKILL_FISHING);

    send_to_char(ch, "Você lança sua linha na água e espera pacientemente...\r\n");
    act("$n lança uma linha na água.", TRUE, ch, 0, 0, TO_ROOM);

    if (percent <= prob) {
        int roll = rand_number(1, 100);

        if (roll <= 15 + (skill_num / 8)) {
            vnum = 4402;   // Large fish
            send_to_char(ch, "Você fisga um peixe grande!\r\n");
            act("$n fisga um peixe grande!", TRUE, ch, 0, 0, TO_ROOM);
        } else if (roll <= 35 + (skill_num / 4)) {
            vnum = 10922;   // Fresh fish
            send_to_char(ch, "Você fisga um pequeno peixe.\r\n");
            act("$n fisga um pequeno peixe.", TRUE, ch, 0, 0, TO_ROOM);
        } else if (roll <= 50 + (skill_num / 6)) {
            vnum = 2920;   // Zone 29 fish
            send_to_char(ch, "Você fisga um peixe comum.\r\n");
            act("$n fisga um peixe comum.", TRUE, ch, 0, 0, TO_ROOM);
        } else if (roll <= 60 + (skill_num / 8)) {
            vnum = 2972;   // Zone 29 herring
            send_to_char(ch, "Você fisga um arenque!\r\n");
            act("$n fisga um arenque!", TRUE, ch, 0, 0, TO_ROOM);
        } else {
            send_to_char(ch, "Você sente algo mordiscar, mas o peixe escapa.\r\n");
            act("$n quase fisga um peixe, mas ele escapa.", TRUE, ch, 0, 0, TO_ROOM);
        }

        if (vnum != NOTHING) {
            /* For mobs, check if they already have this object to prevent resource overflow */
            if (IS_MOB(ch) && char_has_obj_vnum(ch, vnum)) {
                /* Mob already has this item, skip creation to prevent overflow */
            } else {
                obj = read_object(vnum, VIRTUAL);
                if (obj) {
                    obj_to_char(obj, ch);
                    send_to_char(ch, "Você pega %s.\r\n", GET_OBJ_SHORT(obj));
                }
            }
        }

        gain_exp(ch, rand_number(3, 12));
    } else {
        send_to_char(ch, "Você não consegue pescar nada.\r\n");
        act("$n não consegue pescar nada.", TRUE, ch, 0, 0, TO_ROOM);
    }
}

ACMD(do_forage)
{
    int skill_num = GET_SKILL(ch, SKILL_FORAGE);
    int percent, prob;
    struct obj_data *obj = NULL;
    obj_vnum vnum = NOTHING;

    if (!skill_num) {
        send_to_char(ch, "Você não sabe como forragear.\r\n");
        return;
    }

    if (GET_POS(ch) < POS_STANDING) {
        send_to_char(ch, "Você precisa estar em pé para forragear.\r\n");
        return;
    }

    // Better foraging in forests and fields
    if (SECT(IN_ROOM(ch)) == SECT_CITY || SECT(IN_ROOM(ch)) == SECT_INSIDE) {
        send_to_char(ch, "Não há nada para forragear neste local.\r\n");
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE * 2);

    percent = rand_number(1, 101);
    prob = GET_SKILL(ch, SKILL_FORAGE) + 10;   // Slightly easier than other skills

    send_to_char(ch, "Você procura por comida e materiais úteis...\r\n");
    act("$n procura por comida e materiais no local.", TRUE, ch, 0, 0, TO_ROOM);

    if (percent <= prob) {
        int roll = rand_number(1, 100);

        if (roll <= 5 + (skill_num / 6)) {
            vnum = 12027;   // Herb bag
            send_to_char(ch, "Você encontra algumas ervas medicinais!\r\n");
            act("$n encontra algumas ervas medicinais.", TRUE, ch, 0, 0, TO_ROOM);
        } else if (roll <= 50 + (skill_num / 3)) {
            // Randomly choose between strawberries and fresh fruits
            if (rand_number(1, 2) == 1) {
                vnum = 11;   // Wild strawberries
                send_to_char(ch, "Você encontra morangos silvestres!\r\n");
                act("$n encontra morangos silvestres.", TRUE, ch, 0, 0, TO_ROOM);
            } else {
                vnum = 2328;   // Fresh fruits
                send_to_char(ch, "Você encontra alguns frutos selvagens.\r\n");
                act("$n encontra alguns frutos selvagens.", TRUE, ch, 0, 0, TO_ROOM);
            }
        } else {
            send_to_char(ch, "Você encontra apenas galhos e folhas secas.\r\n");
            act("$n encontra apenas galhos e folhas secas.", TRUE, ch, 0, 0, TO_ROOM);
        }

        if (vnum != NOTHING) {
            /* For mobs, check if they already have this object to prevent resource overflow */
            if (IS_MOB(ch) && char_has_obj_vnum(ch, vnum)) {
                /* Mob already has this item, skip creation to prevent overflow */
            } else {
                obj = read_object(vnum, VIRTUAL);
                if (obj) {
                    obj_to_char(obj, ch);
                    send_to_char(ch, "Você pega %s.\r\n", GET_OBJ_SHORT(obj));
                }
            }
        }

        gain_exp(ch, rand_number(2, 10));
    } else {
        send_to_char(ch, "Você não consegue encontrar nada comestível.\r\n");
        act("$n não consegue encontrar nada comestível.", TRUE, ch, 0, 0, TO_ROOM);
    }
}

ACMD(do_eavesdrop)
{
    int dir;
    char buf[MAX_STRING_LENGTH];
    room_rnum target_room;
    int skill_num = GET_SKILL(ch, SKILL_EAVESDROP);
    int percent, prob;
    struct char_data *temp;

    one_argument(argument, buf);

    if (!skill_num) {
        send_to_char(ch, "Você não sabe como espionar conversas.\r\n");
        return;
    }

    if (GET_POS(ch) < POS_STANDING) {
        send_to_char(ch, "Você precisa estar em pé para espionar conversas.\r\n");
        return;
    }

    // If already listening, stop listening
    if (ch->listening_to != NOWHERE) {
        REMOVE_FROM_LIST(ch, world[ch->listening_to].listeners, next_listener);
        ch->listening_to = NOWHERE;
        send_to_char(ch, "Você para de espionar conversas.\r\n");
        return;
    }

    if (!*buf) {
        send_to_char(ch, "Em qual direção você gostaria de espionar?\r\n");
        return;
    }

    // Try Portuguese directions first, then English, then abbreviations
    if ((dir = search_block(buf, dirs_pt, FALSE)) < 0) {
        if ((dir = search_block(buf, dirs, FALSE)) < 0) {
            if ((dir = search_block(buf, autoexits_pt, FALSE)) < 0) {
                if ((dir = search_block(buf, autoexits, FALSE)) < 0) {
                    send_to_char(ch, "Que direção é essa?\r\n");
                    return;
                }
            }
        }
    }

    // Check skill
    percent = rand_number(1, 101);
    prob = skill_num;

    if (percent > prob) {
        send_to_char(ch, "Você tenta espionar nessa direção, mas falha.\r\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    if (EXIT(ch, dir)) {
        if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) && EXIT(ch, dir)->keyword) {
            sprintf(buf, "A %s está fechada.\r\n", fname(EXIT(ch, dir)->keyword));
            send_to_char(ch, "%s", buf);
        } else if (EXIT(ch, dir)->to_room != NOWHERE) {
            target_room = EXIT(ch, dir)->to_room;
            ch->next_listener = world[target_room].listeners;
            world[target_room].listeners = ch;
            ch->listening_to = target_room;
            send_to_char(ch, "Você começa a espionar conversas nessa direção.\r\n");
            WAIT_STATE(ch, PULSE_VIOLENCE);
        } else {
            send_to_char(ch, "Não há uma sala nessa direção...\r\n");
        }
    } else {
        send_to_char(ch, "Não há uma sala nessa direção...\r\n");
    }
}
