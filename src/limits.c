/**************************************************************************
 *  File: limits.c                                          Part of tbaMUD *
 *  Usage: Limits & gain funcs for HMV, exp, hunger/thirst, idle time.     *
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
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "class.h"
#include "fight.h"
#include "screen.h"
#include "spirits.h"
#include "mud_event.h"
#include "act.h"

/* local file scope function prototypes */
static int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
static void check_idling(struct char_data *ch);

/* When age < 15 return the value p0 When age is 15..29 calculate the line
   between p1 & p2 When age is 30..44 calculate the line between p2 & p3 When
   age is 45..59 calculate the line between p3 & p4 When age is 60..79
   calyiculate the line between p4 & p5 When age >= 80 return the value p6 */
static int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

    if (grafage < 15)
        return (p0); /* < 15 */
    else if (grafage <= 29)
        return (p1 + (((grafage - 15) * (p2 - p1)) / 15)); /* 15..29 */
    else if (grafage <= 44)
        return (p2 + (((grafage - 30) * (p3 - p2)) / 15)); /* 30..44 */
    else if (grafage <= 59)
        return (p3 + (((grafage - 45) * (p4 - p3)) / 15)); /* 45..59 */
    else if (grafage <= 79)
        return (p4 + (((grafage - 60) * (p5 - p4)) / 20)); /* 60..79 */
    else
        return (p6); /* >= 80 */
}

/* The hit_limit, mana_limit, and move_limit functions are gone.  They added
   an unnecessary level of complexity to the internal structure, weren't
   particularly useful, and led to some annoying bugs.  From the players'
   point of view, the only difference the removal of these functions will make
   is that a character's age will now only affect the HMV gain per tick, and
   _not_ the HMV maximums. */
/* manapoint gain pr. game hour */
int mana_gain(struct char_data *ch)
{
    int gain;

    if (IS_NPC(ch)) {
        /* Neat and fast */
        gain = GET_LEVEL(ch);
    } else {
        if (IS_DEAD(ch) || PLR_FLAGGED(ch, PLR_FROZEN))
            return (0);

        gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);

        /* Class/Level calculations */
        if (IS_MAGIC_USER(ch) || WAS_MAGIC_USER(ch))
            gain *= 3;
        else if (IS_DRUID(ch) || WAS_DRUID(ch))
            gain *= 2.5;
        else if (IS_CLERIC(ch) || WAS_CLERIC(ch))
            gain *= 2;

        /* Skill/Spell calculations */

        /* Position calculations */
        switch (GET_POS(ch)) {
            case POS_SLEEPING:
                gain *= 2;
                break;
            case POS_MEDITING:
                gain *= 3;
                break;
            case POS_RESTING:
                gain += (gain / 2); /* Divide by 2 */
                break;
            case POS_SITTING:
                gain += (gain / 4); /* Divide by 4 */
                break;
        }
        if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
            gain /= 4;

        if (IN_ROOM(ch))
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FROZEN))
                gain = 0;
    }

    if (AFF_FLAGGED(ch, AFF_POISON))
        gain /= 4;

    return (gain);
}

/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data *ch)
{
    int gain;

    if (IS_NPC(ch)) {
        /* Neat and fast */
        gain = GET_LEVEL(ch);
    } else {

        if (IS_DEAD(ch) || PLR_FLAGGED(ch, PLR_FROZEN))
            return (0);

        if (AFF_FLAGGED(ch, AFF_POISON))
            return (0);

        gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);

        if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
            gain /= 4;

        /* Class/Level calculations */
        if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
            gain /= 2; /* Ouch. */

        gain += (gain * (GET_MAX_HIT(ch) / 250) / 3);
        gain += MAX(0, (GET_CON(ch) - 10) / 2);
        /* Skill/Spell calculations */
        /* Position calculations */

        switch (GET_POS(ch)) {
            case POS_MEDITING:
                gain = 0;
                break;
            case POS_SLEEPING:
                gain += (gain / 2); /* Divide by 2 */
                break;
            case POS_RESTING:
                gain += (gain / 4); /* Divide by 4 */
                break;
            case POS_SITTING:
                gain += (gain / 8); /* Divide by 8 */
                break;
        }
        /* Room calculations */
        if (IN_ROOM(ch)) {
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FROZEN) && GET_HIT(ch) > 0) {
                gain = 0;
            } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HEAL)) {
                if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
                    gain += (gain * 0.75);
                else
                    gain += (gain * 0.5);
            }
        }
    }

    return (gain);
}

/* move gain pr. game hour */
int move_gain(struct char_data *ch)
{
    int gain;

    if (IS_NPC(ch)) {
        /* Neat and fast */
        gain = GET_LEVEL(ch);
    } else {
        if (IS_DEAD(ch) || PLR_FLAGGED(ch, PLR_FROZEN))
            return (0);

        gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);

        if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
            gain /= 4;

        /* Class/Level calculations */
        gain += (GET_CON(ch) - 10) / 2;
        /* Position calculations */
        switch (GET_POS(ch)) {
            case POS_MEDITING:
                gain = 0;
            case POS_SLEEPING:
                gain += (gain / 2); /* Divide by 2 */
                break;
            case POS_RESTING:
                gain += (gain / 4); /* Divide by 4 */
                break;
            case POS_SITTING:
                gain += (gain / 8); /* Divide by 8 */
                break;
        }

        /* Skill/Spell calculations */

        /* Class calculatuions */

        if (IS_RANGER(ch) || WAS_RANGER(ch))
            gain += (gain / 4);

        /* Room calculations */
        if (IN_ROOM(ch))
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FROZEN) && GET_MOVE(ch) > 0)
                gain = 0;
    }

    if (AFF_FLAGGED(ch, AFF_POISON))
        gain /= 4;

    return (gain);
}

/* Breath point gain pr. game hour */
int breath_gain(struct char_data *ch)
{
    int gain;

    if (IS_NPC(ch)) {
        /* NPCs don't need breath management */
        return GET_MAX_BREATH(ch);
    } else {
        if (IS_DEAD(ch) || PLR_FLAGGED(ch, PLR_FROZEN))
            return (0);

        /* Base breath gain - recover breath quickly in normal conditions */
        gain = graf(age(ch)->year, 8, 10, 12, 10, 8, 6, 4);

        /* Constitution affects breath recovery */
        gain += MAX(0, (GET_CON(ch) - 10) / 2);

        /* Position calculations - resting helps breath recovery */
        switch (GET_POS(ch)) {
            case POS_SLEEPING:
                gain += (gain / 2); /* 150% of base - same as hit/move */
                break;
            case POS_RESTING:
                gain += (gain / 4); /* 125% of base - same as hit/move */
                break;
            case POS_SITTING:
                gain += (gain / 8); /* 112.5% of base - same as hit/move */
                break;
        }

        /* Hunger and thirst affect breath recovery */
        if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
            gain /= 2;

        /* Room calculations */
        if (IN_ROOM(ch)) {
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FROZEN))
                gain = 0;
        }
    }

    return (gain);
}

void set_title(struct char_data *ch, char *title)
{

    if (GET_TITLE(ch) != NULL)
        free(GET_TITLE(ch));

    if (title == NULL) {
        GET_TITLE(ch) = strdup(GET_SEX(ch) == SEX_FEMALE ? title_female(GET_CLASS(ch), GET_LEVEL(ch))
                                                         : title_male(GET_CLASS(ch), GET_LEVEL(ch)));
    } else {
        if (strlen(title) > MAX_TITLE_LENGTH)
            title[MAX_TITLE_LENGTH] = '\0';

        GET_TITLE(ch) = strdup(title);
    }
}

void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
    if (CONFIG_USE_AUTOWIZ) {
        size_t res;
        char buf[256];

#    if defined(CIRCLE_UNIX)
        res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &", CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE,
                       LVL_IMMORT, IMMLIST_FILE, (int)getpid());
#    elif defined(CIRCLE_WINDOWS)
        res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s", CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT,
                       IMMLIST_FILE);
#    endif /* CIRCLE_WINDOWS */

        /* Abusing signed -> unsigned conversion to avoid '-1' check. */
        if (res < sizeof(buf)) {
            mudlog(CMP, LVL_IMMORT, FALSE, "Initiating autowiz.");
            reboot_wizlists();
            int rval = system(buf);
            if (rval != 0)
                mudlog(BRF, LVL_IMMORT, TRUE, "Warning: autowiz failed with return value %d", rval);
        } else
            log1("Cannot run autowiz: command-line doesn't fit in buffer.");
    }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}

void gain_exp(struct char_data *ch, int gain)
{
    int is_altered = FALSE;
    int top_level;
    int num_levels = 0;

    if (!IS_NPC(ch) && (GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_GRIMM || PLR_FLAGGED(ch, PLR_TRNS)))
        return;
    if (IS_NPC(ch)) {
        GET_EXP(ch) += gain;
        return;
    }
    if (gain > 0) {
        if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
            gain += (int)((float)gain * ((float)HAPPY_EXP / (float)(100)));
        gain = MIN(CONFIG_MAX_EXP_GAIN, gain); /* put a cap on the max gain per kill */

        GET_EXP(ch) += gain;
        if (GET_LEVEL(ch) < LVL_IMMORT)
            top_level = LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT;
        else
            top_level = LVL_GRIMM;

        while (GET_LEVEL(ch) < top_level && GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
            GET_LEVEL(ch) += 1;
            num_levels++;
            advance_level(ch);
            is_altered = TRUE;
        }

        /* Adicionar Transcend */

        if (GET_LEVEL(ch) == top_level && GET_LEVEL(ch) != LVL_GRIMM &&
            GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1))
            transcend(ch);

        if (is_altered) {
            mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced %d level%s to level %d.", GET_NAME(ch),
                   num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
            if (num_levels == 1)
                send_to_char(ch, "\a\a\tWVocê aumentou um nível!\tn\r\n");
            else
                send_to_char(ch, "\a\a\tWVocê aumentou %d níveis!\tn\r\n", num_levels);
            if (PRF_FLAGGED(ch, PRF_AUTOTITLE))
                set_title(ch, NULL);
            if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
                run_autowiz();
        }
    } else if (gain < 0) {
        gain = MAX(-CONFIG_MAX_EXP_LOSS, gain); /* Cap max exp lost per death */
        GET_EXP(ch) += gain;
        if (GET_EXP(ch) < 0)
            GET_EXP(ch) = 0;
    }
    if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
        run_autowiz();
}

void gain_exp_regardless(struct char_data *ch, int gain)
{
    int is_altered = FALSE;
    int num_levels = 0;
    if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
        gain += (int)((float)gain * ((float)HAPPY_EXP / (float)(100)));
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
        GET_EXP(ch) = 0;
    if (!IS_NPC(ch)) {
        while (GET_LEVEL(ch) < LVL_IMPL && GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
            GET_LEVEL(ch) += 1;
            num_levels++;
            advance_level(ch);
            is_altered = TRUE;
        }

        if (is_altered) {
            mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced %d level%s to level %d.", GET_NAME(ch),
                   num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
            if (num_levels == 1)
                send_to_char(ch, "\a\a\tWVocê aumentou um nível!\tn\r\n");
            else
                send_to_char(ch, "\a\a\tWVocê aumentou %d níveis!\tn\r\n", num_levels);
            if (PRF_FLAGGED(ch, PRF_AUTOTITLE))
                set_title(ch, NULL);
        }
    }
    if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
        run_autowiz();
}

void gain_condition(struct char_data *ch, int condition, int value)
{
    bool intoxicated;
    if (IS_NPC(ch) || GET_COND(ch, condition) == -1) /* No change */
        return;
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        GET_COND(ch, condition) = -1;
        return;
    }

    if (IN_ROOM(ch) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_FROZEN))
        return;
    if (PLR_FLAGGED(ch, PLR_FROZEN))
        return;
    intoxicated = (GET_COND(ch, DRUNK) > 0);
    GET_COND(ch, condition) += value;
    GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
    GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

    /* Don't send messages if player is writing or condition is above threshold */
    if (GET_COND(ch, condition) > 1 || PLR_FLAGGED(ch, PLR_WRITING))
        return;

    /* Warning message at condition == 1 (one tick before damage starts) */
    if (GET_COND(ch, condition) == 1) {
        switch (condition) {
            case HUNGER:
                send_to_char(ch, "Você está com muita fome e começará a sofrer dano em breve!\r\n");
                break;
            case THIRST:
                send_to_char(ch, "Você está com muita sede e começará a sofrer dano em breve!\r\n");
                break;
            default:
                break;
        }
    }
    /* Message at condition == 0 (damage is being applied) */
    else if (GET_COND(ch, condition) == 0) {
        switch (condition) {
            case HUNGER:
                send_to_char(ch, "Você está morrendo de fome!\r\n");
                break;
            case THIRST:
                send_to_char(ch, "Você está morrendo de sede!\r\n");
                break;
            case DRUNK:
                if (intoxicated)
                    send_to_char(ch, "Agora você está sóbri%s.\r\n", OA(ch));
                break;
            default:
                break;
        }
    }
}

static void check_idling(struct char_data *ch)
{
    if (ch->char_specials.timer > CONFIG_IDLE_VOID) {
        if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE &&
            !PLR_FLAGGED(ch, PLR_GHOST | PLR_FROZEN | PLR_JAILED)) {
            GET_WAS_IN(ch) = IN_ROOM(ch);
            if (FIGHTING(ch)) {
                stop_fighting(FIGHTING(ch));
                stop_fighting(ch);
            }
            act("$n desaparece no vazio.", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char(ch, "Você esteve ocios%s e caiu em um vazio.\r\n", OA(ch));
            save_char(ch);
            Crash_crashsave(ch);
            char_from_room(ch);
            char_to_room(ch, 1);
        } else if (ch->char_specials.timer > CONFIG_IDLE_RENT_TIME) {
            if (IN_ROOM(ch) != NOWHERE)
                char_from_room(ch);
            char_to_room(ch, 3);
            if (ch->desc) {
                STATE(ch->desc) = CON_DISCONNECT;
                /*
                 * For the 'if (d->character)' test in close_socket().
                 * -gg 3/1/98 (Happy anniversary.)
                 */
                ch->desc->character = NULL;
                ch->desc = NULL;
            }
            if (CONFIG_FREE_RENT)
                Crash_rentsave(ch, 0);
            else
                Crash_idlesave(ch);
            mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s force-rented and extracted (idle).", GET_NAME(ch));
            add_llog_entry(ch, LAST_IDLEOUT);
            extract_char(ch);
        }
    }
}

/* Update PCs, NPCs, and objects */
void point_update(void)
{
    struct char_data *i, *next_char;
    struct obj_data *j, *next_thing, *jj, *next_thing2;
    zone_rnum zona;
    int pressure, temperature, humidity, winds;
    /* characters */
    for (i = character_list; i; i = next_char) {
        next_char = i->next;
        gain_condition(i, HUNGER, -1);
        gain_condition(i, DRUNK, -1);
        zona = world[IN_ROOM(i)].zone;
        pressure = zone_table[zona].weather->pressure;
        temperature = zone_table[zona].weather->temperature;
        humidity = zone_table[zona].weather->humidity;
        winds = zone_table[zona].weather->winds;
        if ((temperature > 37.5) || (humidity <= 0.30 && temperature > 20) || (humidity >= 0.80 && temperature > 25) ||
            (pressure < 600) || (winds > 5.56 && humidity < 0.40) || (winds > 2.78 && temperature > 35)) {
            gain_condition(i, THIRST, -2);
        } else
            gain_condition(i, THIRST, -1);
        if (GET_POS(i) >= POS_STUNNED) {
            GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
            GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
            GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));

            /* Breath update - handle underwater and high altitude rooms */
            if (!IS_NPC(i) && !PLR_FLAGGED(i, PLR_FROZEN)) {
                if (SECT(IN_ROOM(i)) == SECT_UNDERWATER || ROOM_FLAGGED(IN_ROOM(i), ROOM_HIGH)) {
                    /* Lose breath when underwater or in high altitude */
                    if (!has_scuba(i)) {
                        /* Lose breath - faster in underwater than high altitude */
                        int breath_loss = (SECT(IN_ROOM(i)) == SECT_UNDERWATER) ? 2 : 1;
                        GET_BREATH(i) = MAX(0, GET_BREATH(i) - breath_loss);

                        /* Check if character is suffocating */
                        if (GET_BREATH(i) == 0) {
                            /* Suffocation damage */
                            if (SECT(IN_ROOM(i)) == SECT_UNDERWATER) {
                                send_to_char(i, "\tRVocê está se afogando!\tn\r\n");
                                if (damage(i, i, rand_number(5, 15), TYPE_SUFFERING) < 0)
                                    continue;
                            } else {
                                send_to_char(i, "\tRVocê está sufocando na altitude!\tn\r\n");
                                if (damage(i, i, rand_number(3, 10), TYPE_SUFFERING) < 0)
                                    continue;
                            }
                        } else if (GET_BREATH(i) < 5) {
                            /* Warning messages for low breath */
                            if (SECT(IN_ROOM(i)) == SECT_UNDERWATER)
                                send_to_char(i, "\tYVocê precisa urgentemente de ar!\tn\r\n");
                            else
                                send_to_char(i, "\tYVocê está com dificuldade para respirar!\tn\r\n");
                        }
                    }
                } else {
                    /* Recover breath in normal rooms */
                    GET_BREATH(i) = MIN(GET_BREATH(i) + breath_gain(i), GET_MAX_BREATH(i));
                }
            }

            if (AFF_FLAGGED(i, AFF_POISON))
                if (damage(i, i, 2, SPELL_POISON) == -1)
                    continue;                 /* Oops, they died 6/24/98 */
            if (AFF_FLAGGED(i, AFF_STINGING)) /* -- mp */
                if (damage(i, i, 5, SPELL_STINGING_SWARM) < 0)
                    continue;
            if (!IS_NPC(i) && !ROOM_FLAGGED(IN_ROOM(i), ROOM_FROZEN) && !PLR_FLAGGED(i, PLR_FROZEN)) {
                if (GET_COND(i, HUNGER) == 0)
                    if (damage(i, i, rand_number(2, 25), TYPE_HUNGRY) < 0)
                        continue;
                if (GET_COND(i, THIRST) == 0)
                    if (damage(i, i, rand_number(2, 18), TYPE_THIRSTY) < 0)
                        continue;
            }

            if (GET_POS(i) <= POS_STUNNED)
                update_pos(i);
        } else if (GET_POS(i) == POS_INCAP) {
            if (damage(i, i, 1, TYPE_SUFFERING) == -1)
                continue;

            if (!IS_NPC(i) && !ROOM_FLAGGED(IN_ROOM(i), ROOM_FROZEN) && !PLR_FLAGGED(i, PLR_FROZEN)) {
                if (GET_COND(i, HUNGER) == 0)
                    if (damage(i, i, 1, TYPE_HUNGRY) < 0)
                        continue;
                if (GET_COND(i, THIRST) == 0)
                    if (damage(i, i, 1, TYPE_THIRSTY) < 0)
                        continue;
            }
        } else if (GET_POS(i) == POS_MORTALLYW) {
            if (damage(i, i, 2, TYPE_SUFFERING) == -1)
                continue;
        }
        if (!IS_NPC(i)) {
            update_char_objects(i);
            (i->char_specials.timer)++;
            if (GET_LEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
                check_idling(i);
            /* -- mp - Jan 27, 2002 */
            if (GET_POS(i) == POS_SLEEPING) {
                if (GET_HIT(i) >= GET_MAX_HIT(i))
                    send_to_char(i, "Você sonha com as maravilhas de Vitália.\r\n");
                else if (GET_HIT(i) >= GET_MAX_HIT(i) / 2)
                    send_to_char(i, "Você sonha com incríveis batalhas.\r\n");
                else if (GET_HIT(i) >= GET_MAX_HIT(i) / 5)
                    send_to_char(i, "Você sonha com terríveis criaturas.\r\n");
                else
                    send_to_char(i, "Um antigo pesadelo atormenta seu sono.\r\n");
            }
        }
    }
    /* objects */
    for (j = object_list; j; j = next_thing) {
        next_thing = j->next; /* Next in object list */
        /* If this is a corpse */
        if (IS_CORPSE(j)) {
            /* timer count down */
            if (GET_OBJ_TIMER(j) > 0)
                GET_OBJ_TIMER(j)--;
            if (!GET_OBJ_TIMER(j)) {
                switch (autoraise_corpse(j)) {
                    case ar_skip:
                        /* Don't touch the corpse, leave it in place */
                        break;
                    case ar_ok:
                        /* Successful resurrection - corpse is handled by raise functions */
                        break;
                    case ar_dropobjs:
                        if (j->carried_by)
                            act("$p cai de suas mãos.", FALSE, j->carried_by, j, 0, TO_CHAR);
                        else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
                            act("Uma multidão de vermes consomem $p.", TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
                            act("Uma multidão de vermes consomem $p.", TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
                        }
                        for (jj = j->contains; jj; jj = next_thing2) {
                            next_thing2 = jj->next_content; /* Next in inventory */
                            obj_from_obj(jj);
                            if (j->in_obj)
                                obj_to_obj(jj, j->in_obj);
                            else if (j->carried_by)
                                obj_to_room(jj, IN_ROOM(j->carried_by));
                            else if (IN_ROOM(j) != NOWHERE)
                                obj_to_room(jj, IN_ROOM(j));
                            else
                                core_dump();
                        }
                        extract_obj(j);
                        break;
                    case ar_extract: {
                        /*
                         * Extract entire corpse.
                         * Let's extract all it's contents. After this, next_thing may
                         * point to an unexistent object. We need to re-set it to j->next
                         * before proceding.
                         */
                        for (jj = j->contains; jj; jj = next_thing2) {
                            next_thing2 = jj->next_content;
                            obj_from_obj(jj);
                            obj_to_room(jj, IN_ROOM(j));
                        }
                        extract_obj(j);
                    }
                }
            }
        }

        else if (GET_OBJ_TYPE(j) == ITEM_PORTAL) {
            GET_OBJ_TIMER(j)--;
            if (GET_OBJ_TIMER(j) == 1) {
                if (j->in_room)
                    act("$p começa a desaparecer!", FALSE, 0, j, 0, TO_ROOM);
            } else if (GET_OBJ_TIMER(j) == 0) {
                if (j->in_room)
                    act("$p desaparece em uma nuvem de fumaça!", FALSE, 0, j, 0, TO_ROOM);
                extract_obj(j);
            }
        }
        /* Handle negative timers (NOLOCATE flag removal for quest items) */
        else if (GET_OBJ_TIMER(j) < 0) {
            GET_OBJ_TIMER(j)++;
            if (GET_OBJ_TIMER(j) == 0) {
                /* Timer expired - remove NOLOCATE flag but keep the object */
                if (OBJ_FLAGGED(j, ITEM_NOLOCATE)) {
                    REMOVE_BIT_AR(GET_OBJ_EXTRA(j), ITEM_NOLOCATE);
                }
            }
        }
        /* If the timer is set, count it down and at 0, try the trigger note
           to .rej hand-patchers: make this last in your point-update() */
        else if (GET_OBJ_TIMER(j) > 0) {
            GET_OBJ_TIMER(j)--;
            timer_otrigger(j);
            if (!GET_OBJ_TIMER(j)) {
                if (j->carried_by)
                    act("$p misteriosamente desaparece.", FALSE, j->carried_by, j, 0, TO_CHAR);
                else if (j->in_room)
                    act("$p misteriosamente desaparece.", TRUE, 0, j, 0, TO_ROOM);
                extract_obj(j);
            }
        }
    }

    /* Take 1 from the happy-hour tick counter, and end happy-hour if zero */
    if (HAPPY_TIME > 1)
        HAPPY_TIME--;
    else if (HAPPY_TIME == 1) /* Last tick - set everything back to zero */
    {
        HAPPY_QP = 0;
        HAPPY_EXP = 0;
        HAPPY_GOLD = 0;
        HAPPY_TIME = 0;
        game_info("Happy hour acabou!");
    }
}

/* Note: amt may be negative */
int increase_gold(struct char_data *ch, int amt)
{
    int curr_gold;
    curr_gold = GET_GOLD(ch);
    if (amt < 0) {
        GET_GOLD(ch) = MAX(0, curr_gold + amt);
        /* Validate to prevent overflow */
        if (GET_GOLD(ch) > curr_gold)
            GET_GOLD(ch) = 0;
    } else {
        GET_GOLD(ch) = MIN(MAX_GOLD, curr_gold + amt);
        /* Validate to prevent overflow */
        if (GET_GOLD(ch) < curr_gold)
            GET_GOLD(ch) = MAX_GOLD;
    }
    if (GET_GOLD(ch) == MAX_GOLD)
        send_to_char(ch,
                     "%sVocê atingiu o limite de ouro!\r\n%sVocê deve gastar ou depositar antes de ganhar mais.\r\n",
                     QBRED, QNRM);
    return (GET_GOLD(ch));
}

int decrease_gold(struct char_data *ch, int deduction)
{
    int amt;
    amt = (deduction * -1);
    increase_gold(ch, amt);
    return (GET_GOLD(ch));
}

int increase_bank(struct char_data *ch, int amt)
{
    int curr_bank;
    if (IS_NPC(ch))
        return 0;
    curr_bank = GET_BANK_GOLD(ch);
    if (amt < 0) {
        GET_BANK_GOLD(ch) = MAX(0, curr_bank + amt);
        /* Validate to prevent overflow */
        if (GET_BANK_GOLD(ch) > curr_bank)
            GET_BANK_GOLD(ch) = 0;
    } else {
        GET_BANK_GOLD(ch) = MIN(MAX_BANK, curr_bank + amt);
        /* Validate to prevent overflow */
        if (GET_BANK_GOLD(ch) < curr_bank)
            GET_BANK_GOLD(ch) = MAX_BANK;
    }
    if (GET_BANK_GOLD(ch) == MAX_BANK)
        send_to_char(ch, "%sVocê atingiu o limite no banco!\r\n%sVocê não pode depositar mais até sacar.\r\n", QBRED,
                     QNRM);
    return (GET_BANK_GOLD(ch));
}

int decrease_bank(struct char_data *ch, int deduction)
{
    int amt;
    amt = (deduction * -1);
    increase_bank(ch, amt);
    return (GET_BANK_GOLD(ch));
}
