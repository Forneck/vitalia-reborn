/**************************************************************************
 *  File: act.informative.c                                 Part of tbaMUD *
 *  Usage: Player-level commands of an informative nature.                 *
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
#include "constants.h"
#include "dg_scripts.h"
#include "mud_event.h"
#include "mail.h" /**< For the has_mail function */
#include "act.h"
#include "class.h"
#include "fight.h"
#include "modify.h"
#include "asciimap.h"
#include "quest.h"
#include "spedit.h"
#include "string.h"  /* garante que strcmp/strlcpy/strtok estejam disponíveis */

/* Mapeia nomes crus de AFF (affected_bits) para nomes amigáveis. */
static const char *aff_pretty_name(const char *raw)
{
    if (!raw || !*raw)
        return "";

    /* Itens sem bits de efeito (NOBITS) não devem aparecer em 'affects'. */
    if (!strcmp(raw, "NOBITS"))
        return "";

    /* ---------- DEBUFFS / STATUS NEGATIVOS ---------- */
    if (!strcmp(raw, "BLIND"))       return "blindness";
    if (!strcmp(raw, "CURSE"))       return "curse";
    if (!strcmp(raw, "POISON"))      return "poison";
    if (!strcmp(raw, "SLEEP"))       return "sleep";
    if (!strcmp(raw, "PARALYZE"))    return "paralyze";
    if (!strcmp(raw, "CHARM"))       return "charm";

    /* (Opcional) você pode considerar alguns como “controle”:
     * TALKDEAD = falar com mortos (já mapeado em buff abaixo)
     * NO_TRACK = impedir tracking (já está em buffs utilitários)
     */

    /* ---------- BUFFS / STATUS POSITIVOS E UTILITÁRIOS ---------- */
    if (!strcmp(raw, "INVIS"))        return "invisibility";
    if (!strcmp(raw, "DET-ALIGN"))    return "detect alignment";
    if (!strcmp(raw, "DET-INVIS"))    return "detect invisibility";
    if (!strcmp(raw, "DET-MAGIC"))    return "detect magic";
    if (!strcmp(raw, "SENSE-LIFE"))   return "sense life";
    if (!strcmp(raw, "WATWALK"))      return "waterwalk";
    if (!strcmp(raw, "SANCT"))        return "sanctuary";
    if (!strcmp(raw, "GROUP"))        return "group";
    if (!strcmp(raw, "INFRA"))        return "infravision";
    if (!strcmp(raw, "PROT-EVIL"))    return "protection from evil";
    if (!strcmp(raw, "PROT-GOOD"))    return "protection from good";
    if (!strcmp(raw, "NO_TRACK"))     return "no track";
    if (!strcmp(raw, "STONESKIN"))    return "stoneskin";
    if (!strcmp(raw, "FIRESHIELD"))   return "fireshield";
    if (!strcmp(raw, "TALKDEAD"))     return "talk with dead";
    if (!strcmp(raw, "FLYING"))       return "fly";
    if (!strcmp(raw, "BREATH"))       return "breath";
    if (!strcmp(raw, "LIGHT"))        return "light";
    if (!strcmp(raw, "FIREFLIES"))    return "fireflies";
    if (!strcmp(raw, "STINGING"))     return "stinging swarm";
    if (!strcmp(raw, "THISTLECOAT"))  return "thistlecoat";
    if (!strcmp(raw, "SOUNDBARRIER")) return "soundbarrier";
    if (!strcmp(raw, "ADAGIO"))       return "adagio";
    if (!strcmp(raw, "ALLEGRO"))      return "allegro";
    if (!strcmp(raw, "GLOOMSHIELD"))  return "gloomshield";
    if (!strcmp(raw, "PROT-SPELL"))   return "protection from spells";
    if (!strcmp(raw, "WINDWALL"))     return "windwall";

    return raw;
}

/* prototypes of local functions */
/* do_diagnose utility functions */
static void diag_char_to_char(struct char_data *i, struct char_data *ch);
/* do_look and do_examine utility functions */
static void do_auto_exits(struct char_data *ch);
static void list_char_to_char(struct char_data *list, struct char_data *ch);
static void list_one_char(struct char_data *i, struct char_data *ch);
static void look_at_char(struct char_data *i, struct char_data *ch);
static void look_at_target(struct char_data *ch, char *arg);
static void look_in_direction(struct char_data *ch, int dir);
static void look_in_obj(struct char_data *ch, char *arg);
/* Emotion system helper */
static void get_highest_emotion_display(struct char_data *mob, struct char_data *viewer, const char **out_text,
                                        const char **out_color);
/* do_look, do_inventory utility functions */
static void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show);
/* do_look, do_equipment, do_examine, do_inventory */
void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);
static void show_obj_modifiers(struct obj_data *obj, struct char_data *ch, int mode);
/* do_where utility functions */
static void perform_immort_where(char_data *ch, const char *arg);
static void perform_mortal_where(struct char_data *ch, char *arg);
static size_t print_object_location(int num, const obj_data *obj, const char_data *ch, char *buf, size_t len,
                                    size_t buf_size, int recur);
extern void look_at_sky(struct char_data *ch);
/* Subcommands */
/* For show_obj_to_char 'mode'. /-- arbitrary - defined in act.h */

void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode)
{
    int found = 0;
    struct char_data *temp;

    if (!obj || !ch) {
        log1("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", (void *)obj, (void *)ch);
        /* SYSERR_DESC: Somehow a NULL pointer was sent to show_obj_to_char()
           in either the 'obj' or the 'ch' variable.  The error will indicate
           which was NULL by listing both of the pointers passed to it.  This
           is often a difficult one to trace, and may require stepping through
           a debugger. */
        return;
    }

    if ((mode == 0) && obj->description) {
        if (GET_OBJ_VAL(obj, 1) != 0 || OBJ_SAT_IN_BY(obj)) {
            for (temp = OBJ_SAT_IN_BY(obj); temp; temp = NEXT_SITTING(temp)) {
                if (temp == ch)
                    found++;
            }
            if (found) {
                send_to_char(ch, "Você está %s em %s.", GET_POS(ch) == POS_SITTING ? "sentando" : "descansando",
                             obj->short_description);
                goto end;
            }
        }
    }

    switch (mode) {
        case SHOW_OBJ_LONG:
            /* Hide objects starting with . from non-holylighted people. - Elaseth
             */
            if (*obj->description == '.' && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
                return;

            if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS)) {
                send_to_char(ch, "[%d] ", GET_OBJ_VNUM(obj));
                if (SCRIPT(obj)) {
                    if (!TRIGGERS(SCRIPT(obj))->next)
                        send_to_char(ch, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(obj))));
                    else
                        send_to_char(ch, "[TRIGS] ");
                }
            }
            send_to_char(ch, "%s", CCGRN(ch, C_NRM));
            send_to_char(ch, "%s", obj->description);
            break;

        case SHOW_OBJ_SHORT:
            if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS)) {
                send_to_char(ch, "[%d] ", GET_OBJ_VNUM(obj));
                if (SCRIPT(obj)) {
                    if (!TRIGGERS(SCRIPT(obj))->next)
                        send_to_char(ch, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(obj))));
                    else
                        send_to_char(ch, "[TRIGS] ");
                }
            }
            send_to_char(ch, "%s", obj->short_description);
            break;

        case SHOW_OBJ_ACTION:
            switch (GET_OBJ_TYPE(obj)) {
                case ITEM_NOTE:
                    if (obj->action_description) {
                        char notebuf[MAX_NOTE_LENGTH + 64];

                        snprintf(notebuf, sizeof(notebuf), "Há algo escrito:\r\n\r\n%s", obj->action_description);
                        page_string(ch->desc, notebuf, TRUE);
                    } else
                        send_to_char(ch, "Está em branco.\r\n");
                    return;

                case ITEM_DRINKCON:
                    send_to_char(ch, "Isto parece um recipiente de líquido.");
                    break;

                default:
                    send_to_char(ch, "Você não vê nada de especial...");
                    break;
            }
            break;

        default:
            log1("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
            /* SYSERR_DESC: show_obj_to_char() has some predefined 'mode's
               (argument #3) to tell it what to display to the character when it
               is called.  If the mode is not one of these, it will output this
               error, and indicate what mode was passed to it.  To correct it, you
               will need to find the call with the incorrect mode and change it to
               an acceptable mode. */
            return;
    }
end:

    show_obj_modifiers(obj, ch, mode);
    send_to_char(ch, "\r\n");
}

static void show_obj_modifiers(struct obj_data *obj, struct char_data *ch, int mode)
{

    if (mode == SHOW_OBJ_SHORT) {
        bool has_aura = FALSE;

        if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
            send_to_char(ch, " (invis)\tW");
            has_aura = TRUE;
        }

        if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
            send_to_char(ch, "\tC (aura azul)\tW");
            has_aura = TRUE;
        }

        if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
            send_to_char(ch, "\tC (aura amarela)\tW");
            has_aura = TRUE;
        }

        if (OBJ_FLAGGED(obj, ITEM_GLOW)) {
            send_to_char(ch, "\tC (aura brilhante)\tW");
            has_aura = TRUE;
        }

        if (OBJ_FLAGGED(obj, ITEM_HUM)) {
            send_to_char(ch, "\tC (zunindo)\tW");
            has_aura = TRUE;
        }
    } else {
        if (OBJ_FLAGGED(obj, ITEM_INVISIBLE))
            send_to_char(ch, " \tw(invisivel)\tn");

        if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
            send_to_char(ch, " \tn..Isso brilha azul!\tn");

        if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
            send_to_char(ch, " \tn..Isso brilha amarelo!\tn");

        if (OBJ_FLAGGED(obj, ITEM_GLOW))
            send_to_char(ch, " \tn..Isso tem uma aura brilhante!\tn");

        if (OBJ_FLAGGED(obj, ITEM_HUM))
            send_to_char(ch, " \tn..Isso emite uma fraca melodia!\tn");
    }
}

static void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show)
{
    struct obj_data *i, *j, *display;
    bool found;
    int num;

    found = FALSE;

    /* Loop through the list of objects */
    for (i = list; i; i = i->next_content) {
        num = 0;

        /* Check the list to see if we've already counted this object */
        for (j = list; j != i; j = j->next_content)
            if ((j->short_description == i->short_description && j->name == i->name) ||
                (!strcmp(j->short_description, i->short_description) && !strcmp(j->name, i->name)))
                break; /* found a matching object */
        if (j != i)
            continue; /* we counted object i earlier in the list */

        /* Count matching objects, including this one */
        for (display = j = i; j; j = j->next_content)
            /* This if-clause should be exactly the same as the one in the
               loop above */
            if ((j->short_description == i->short_description && j->name == i->name) ||
                (!strcmp(j->short_description, i->short_description) && !strcmp(j->name, i->name)))
                if (CAN_SEE_OBJ(ch, j)) {
                    ++num;
                    /* If the original item can't be seen, switch it for this
                       one */
                    if (display == i && !CAN_SEE_OBJ(ch, display))
                        display = j;
                }

        /* When looking in room, hide objects starting with '.', except for
           holylight */
        if (num > 0 && (mode != SHOW_OBJ_LONG || *display->description != '.' ||
                        (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))) {
            if (mode == SHOW_OBJ_LONG)
                send_to_char(ch, "%s", CCGRN(ch, C_NRM));
            if (num != 1)
                send_to_char(ch, "(%2ix) ", num);
            show_obj_to_char(display, ch, mode);

            send_to_char(ch, "%s", CCNRM(ch, C_NRM));
            found = TRUE;
        }
    }
    if (!found && show)
        send_to_char(ch, "  Nada.\r\n");
}

static void diag_char_to_char(struct char_data *i, struct char_data *ch)
{
    struct {
        byte percent;
        const char *text;
    } diagnosis[] = {
        {100, "está em excelente condição."},
        {90, "tem alguns arranhões."},
        {75, "tem alguns pequenos machucados e dilacerações.\r\n"},
        {50, "tem vários machucados."},
        {30, "tem alguns grandes machucados e arranhões.\r\n"},
        {15, "parece bem destruido."},
        {0, "está em uma condição horrorosa."},
        {-1, "está sangrando abundantemente de seus machucados.\r\n"},
    };
    int percent, ar_index;
    const char *pers = PERS(i, ch);

    if (GET_MAX_HIT(i) > 0)
        percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
    else
        percent = -1; /* How could MAX_HIT be < 1?? */

    for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
        if (percent >= diagnosis[ar_index].percent)
            break;

    if (percent <= 15 && percent > 0 && GET_SEX(i) == SEX_FEMALE)
        send_to_char(ch, "%c%s parece bem destruida.\r\n", UPPER(*pers), pers + 1);
    else
        send_to_char(ch, "%c%s %s\r\n", UPPER(*pers), pers + 1, diagnosis[ar_index].text);
}

static void look_at_char(struct char_data *i, struct char_data *ch)
{
    int j, found;

    if (!ch->desc)
        return;

    if (IS_DEAD(i))
        act("$l está mort$r!", FALSE, i, 0, ch, TO_VICT);

    if (i->player.description)
        send_to_char(ch, "%s", i->player.description);
    else
        act("Você não vê nada de nada de especial n$l.", FALSE, i, 0, ch, TO_VICT);

    diag_char_to_char(i, ch);

    /* Check if this is a questmaster */
    if (IS_NPC(i) && mob_index[GET_MOB_RNUM(i)].func == questmaster) {
        send_to_char(ch,
                     "\r\n\tyEste personagem é um \tCQuestmaster\ty - você pode usar '\tcquest list\ty' aqui.\tn\r\n");
    }

    found = FALSE;
    for (j = 0; !found && j < NUM_WEARS; j++)
        if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
            found = TRUE;

    if (found) {
        send_to_char(ch, "\r\n"); /* act() does capitalization. */
        act("$n está usando:", FALSE, i, 0, ch, TO_VICT);
        for (j = 0; j < NUM_WEARS; j++)
            if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
                send_to_char(ch, "\tW%s\tn", wear_where[j]);
                show_obj_to_char(GET_EQ(i, j), ch, SHOW_OBJ_SHORT);
            }
    }
    if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_IMMORT)) {
        act("\r\nVocê consegue ver em seu inventário:", FALSE, i, 0, ch, TO_VICT);
        list_obj_to_char(i->carrying, ch, SHOW_OBJ_SHORT, TRUE);
    }

    /* Emotion trigger: NPCs seeing powerful equipment on players (Environmental 2.2) */
    if (IS_NPC(ch) && !IS_NPC(i) && ch->ai_data && found && CONFIG_MOB_CONTEXTUAL_SOCIALS) {
        /* Check if player has high-value equipment that triggers envy */
        int total_eq_value = 0;
        for (j = 0; j < NUM_WEARS; j++) {
            if (GET_EQ(i, j)) {
                total_eq_value += GET_OBJ_COST(GET_EQ(i, j));
            }
        }
        /* Trigger envy if equipment value is significant (threshold: 10000 gold) */
        if (total_eq_value >= 10000) {
            update_mob_emotion_saw_equipment(ch, i);
        }
    }
}

/**
 * Helper function to determine the highest emotion above threshold for display.
 * Checks all 20 emotions and returns the text and color for the dominant emotion.
 *
 * @param mob The mob whose emotions to check
 * @param viewer The character viewing the mob
 * @param out_text Output pointer for emotion text (e.g., "(furioso)")
 * @param out_color Output pointer for color code
 */
static void get_highest_emotion_display(struct char_data *mob, struct char_data *viewer, const char **out_text,
                                        const char **out_color)
{
    int highest_value = 0;
    *out_text = NULL;
    *out_color = NULL;

    /* Use hybrid emotion system to get emotions toward the viewer */
    int effective_fear = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_FEAR);
    int effective_anger = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_ANGER);
    int effective_happiness = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_HAPPINESS);
    int effective_sadness = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_SADNESS);
    int effective_trust = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_TRUST);
    int effective_friendship = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_FRIENDSHIP);
    int effective_love = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_LOVE);
    int effective_disgust = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_DISGUST);
    int effective_envy = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_ENVY);
    int effective_horror = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_HORROR);
    int effective_courage = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_COURAGE);
    int effective_pride = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_PRIDE);

    /* Check all emotions and find the highest one above threshold */
    if (effective_fear >= CONFIG_EMOTION_DISPLAY_FEAR_THRESHOLD && effective_fear > highest_value) {
        highest_value = effective_fear;
        *out_text = "(amedrontado)";
        *out_color = CCMAG(viewer, C_NRM);
    }
    if (effective_anger >= CONFIG_EMOTION_DISPLAY_ANGER_THRESHOLD && effective_anger > highest_value) {
        highest_value = effective_anger;
        *out_text = "(furioso)";
        *out_color = CCRED(viewer, C_NRM);
    }
    if (effective_happiness >= CONFIG_EMOTION_DISPLAY_HAPPINESS_THRESHOLD && effective_happiness > highest_value) {
        highest_value = effective_happiness;
        *out_text = "(feliz)";
        *out_color = CCYEL(viewer, C_NRM);
    }
    if (effective_sadness >= CONFIG_EMOTION_DISPLAY_SADNESS_THRESHOLD && effective_sadness > highest_value) {
        highest_value = effective_sadness;
        *out_text = "(triste)";
        *out_color = CCBLU(viewer, C_NRM);
    }
    if (effective_horror >= CONFIG_EMOTION_DISPLAY_HORROR_THRESHOLD && effective_horror > highest_value) {
        highest_value = effective_horror;
        *out_text = "(aterrorizado)";
        *out_color = CBMAG(viewer, C_NRM);
    }
    /* Pain and some emotions are more mood-based, use mob's direct value */
    if (mob->ai_data->emotion_pain >= CONFIG_EMOTION_DISPLAY_PAIN_THRESHOLD &&
        mob->ai_data->emotion_pain > highest_value) {
        highest_value = mob->ai_data->emotion_pain;
        *out_text = "(sofrendo)";
        *out_color = CCRED(viewer, C_NRM);
    }
    int effective_compassion = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_COMPASSION);
    if (effective_compassion >= CONFIG_EMOTION_DISPLAY_COMPASSION_THRESHOLD && effective_compassion > highest_value) {
        highest_value = effective_compassion;
        *out_text = "(compassivo)";
        *out_color = CCGRN(viewer, C_NRM);
    }
    if (effective_courage >= CONFIG_EMOTION_DISPLAY_COURAGE_THRESHOLD && effective_courage > highest_value) {
        highest_value = effective_courage;
        *out_text = "(corajoso)";
        *out_color = CCYEL(viewer, C_NRM);
    }
    int effective_curiosity = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_CURIOSITY);
    if (effective_curiosity >= CONFIG_EMOTION_DISPLAY_CURIOSITY_THRESHOLD && effective_curiosity > highest_value) {
        highest_value = effective_curiosity;
        *out_text = "(curioso)";
        *out_color = CCCYN(viewer, C_NRM);
    }
    if (effective_disgust >= CONFIG_EMOTION_DISPLAY_DISGUST_THRESHOLD && effective_disgust > highest_value) {
        highest_value = effective_disgust;
        *out_text = "(enojado)";
        *out_color = CCGRN(viewer, C_NRM);
    }
    if (effective_envy >= CONFIG_EMOTION_DISPLAY_ENVY_THRESHOLD && effective_envy > highest_value) {
        highest_value = effective_envy;
        *out_text = "(invejoso)";
        *out_color = CCMAG(viewer, C_NRM);
    }
    int effective_excitement = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_EXCITEMENT);
    if (effective_excitement >= CONFIG_EMOTION_DISPLAY_EXCITEMENT_THRESHOLD && effective_excitement > highest_value) {
        highest_value = effective_excitement;
        *out_text = "(animado)";
        *out_color = CCYEL(viewer, C_NRM);
    }
    if (effective_friendship >= CONFIG_EMOTION_DISPLAY_FRIENDSHIP_THRESHOLD && effective_friendship > highest_value) {
        highest_value = effective_friendship;
        *out_text = "(amigavel)";
        *out_color = CCGRN(viewer, C_NRM);
    }
    int effective_greed = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_GREED);
    if (effective_greed >= CONFIG_EMOTION_DISPLAY_GREED_THRESHOLD && effective_greed > highest_value) {
        highest_value = effective_greed;
        *out_text = "(ganancioso)";
        *out_color = CCYEL(viewer, C_NRM);
    }
    int effective_humiliation = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_HUMILIATION);
    if (effective_humiliation >= CONFIG_EMOTION_DISPLAY_HUMILIATION_THRESHOLD &&
        effective_humiliation > highest_value) {
        highest_value = effective_humiliation;
        *out_text = "(humilhado)";
        *out_color = CCMAG(viewer, C_NRM);
    }
    if (effective_love >= CONFIG_EMOTION_DISPLAY_LOVE_THRESHOLD && effective_love > highest_value) {
        highest_value = effective_love;
        *out_text = "(apaixonado)";
        *out_color = CCRED(viewer, C_NRM);
    }
    int effective_loyalty = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_LOYALTY);
    if (effective_loyalty >= CONFIG_EMOTION_DISPLAY_LOYALTY_THRESHOLD && effective_loyalty > highest_value) {
        highest_value = effective_loyalty;
        *out_text = "(leal)";
        *out_color = CCBLU(viewer, C_NRM);
    }
    if (effective_pride >= CONFIG_EMOTION_DISPLAY_PRIDE_THRESHOLD && effective_pride > highest_value) {
        highest_value = effective_pride;
        *out_text = "(orgulhoso)";
        *out_color = CCYEL(viewer, C_NRM);
    }
    int effective_shame = get_effective_emotion_toward(mob, viewer, EMOTION_TYPE_SHAME);
    if (effective_shame >= CONFIG_EMOTION_DISPLAY_SHAME_THRESHOLD && effective_shame > highest_value) {
        highest_value = effective_shame;
        *out_text = "(envergonhado)";
        *out_color = CCMAG(viewer, C_NRM);
    }
    if (effective_trust >= CONFIG_EMOTION_DISPLAY_TRUST_THRESHOLD && effective_trust > highest_value) {
        highest_value = effective_trust;
        *out_text = "(confiante)";
        *out_color = CCGRN(viewer, C_NRM);
    }
}

static void list_one_char(struct char_data *i, struct char_data *ch)
{
    struct obj_data *furniture;
    char art;

    art = (GET_SEX(i) == SEX_FEMALE ? 'a' : 'o');

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS)) {
        if (IS_NPC(i))
            send_to_char(ch, "[%d] ", GET_MOB_VNUM(i));
        if (SCRIPT(i) && TRIGGERS(SCRIPT(i))) {
            if (!TRIGGERS(SCRIPT(i))->next)
                send_to_char(ch, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(i))));
            else
                send_to_char(ch, "[TRIGS] ");
        }
    }

    if (GROUP(i)) {
        if (GROUP(i) == GROUP(ch))
            send_to_char(ch, "(%s%s%s) ", CBGRN(ch, C_NRM), GROUP_LEADER(GROUP(i)) == i ? "lider" : "grupo",
                         CCYEL(ch, C_NRM));
        else
            send_to_char(ch, "(%s%s%s) ", CBRED(ch, C_NRM), GROUP_LEADER(GROUP(i)) == i ? "lider" : "grupo",
                         CCYEL(ch, C_NRM));
    }

    if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
        if (AFF_FLAGGED(i, AFF_INVISIBLE))
            send_to_char(ch, "*");

        if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
            if (IS_EVIL(i))
                send_to_char(ch, "%s(Aura Vermelha) ", CBRED(ch, C_NRM));
            else if (IS_GOOD(i))
                send_to_char(ch, "%s(Aura Azul) ", CBBLU(ch, C_NRM));
        }

        /* Check if this mob is a bounty target for the viewer */
        if (!IS_NPC(ch) && GET_QUEST(ch) != NOTHING && GET_QUEST_TYPE(ch) == AQ_MOB_KILL_BOUNTY &&
            GET_BOUNTY_TARGET_ID(ch) != NOBODY && char_script_id(i) == GET_BOUNTY_TARGET_ID(ch)) {
            send_to_char(ch, "%s(Bounty) ", CBRED(ch, C_NRM));
        }

        /* EMOTION SYSTEM: Add emotion indicators based on high emotion levels */
        /* Only display if player has DISPEMOTE preference enabled */
        /* Shows the highest emotion that exceeds its threshold */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && i->ai_data && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_DISPEMOTE)) {
            const char *emotion_text = NULL;
            const char *emotion_color = NULL;

            get_highest_emotion_display(i, ch, &emotion_text, &emotion_color);

            /* Display the highest emotion if any was found */
            if (emotion_text && emotion_color) {
                send_to_char(ch, "%s%s%s ", emotion_color, emotion_text, CCYEL(ch, C_NRM));
            }
        }

        send_to_char(ch, "%s%s", CCYEL(ch, C_NRM), i->player.long_descr);

        if (AFF_FLAGGED(i, AFF_SANCTUARY))
            act("\tW...$l brilha com uma luz branca!\tn", FALSE, i, 0, ch, TO_VICT);
        else if (AFF_FLAGGED(i, AFF_GLOOMSHIELD))
            act("\tL...$l é resguardad$r por um espesso escudo de trevas!\tn", FALSE, i, 0, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_FIRESHIELD))
            act("\tR...$l está envolvid$r por uma aura de fogo!\tn", FALSE, i, 0, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_WINDWALL))
            act("\tw...$l está envolvid$r por uma parede de vento!\tn", FALSE, i, 0, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_BLIND))
            act("\tw...$l está tateando ao redor, ceg$r!\tn", FALSE, i, 0, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_FIREFLIES))
            act("\tG...$l está rodead$r por vaga-lumes!\tn", FALSE, i, 0, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_THISTLECOAT))
            act("\ty...$l está protegid$r por uma barreira de espinhos!\tn", FALSE, i, 0, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_SOUNDBARRIER))
            act("\tc...$l está envolt$r por uma protetora barreira de som!\tn", FALSE, i, 0, ch, TO_VICT);
        return;
    }

    if (IS_NPC(i)) {
        /* Check if this mob is a bounty target for the viewer */
        if (!IS_NPC(ch) && GET_QUEST(ch) != NOTHING && GET_QUEST_TYPE(ch) == AQ_MOB_KILL_BOUNTY &&
            GET_BOUNTY_TARGET_ID(ch) != NOBODY && char_script_id(i) == GET_BOUNTY_TARGET_ID(ch)) {
            send_to_char(ch, "%s(Bounty) ", CBRED(ch, C_NRM));
        }

        /* EMOTION SYSTEM: Add emotion indicators for mobs not in default position */
        /* Only display if player has DISPEMOTE preference enabled */
        /* Shows the highest emotion that exceeds its threshold */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && i->ai_data && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_DISPEMOTE)) {
            const char *emotion_text = NULL;
            const char *emotion_color = NULL;

            get_highest_emotion_display(i, ch, &emotion_text, &emotion_color);

            /* Display the highest emotion if any was found */
            if (emotion_text && emotion_color) {
                send_to_char(ch, "%s%s%s ", emotion_color, emotion_text, CCNRM(ch, C_NRM));
            }
        }

        send_to_char(ch, "%c%s", UPPER(*i->player.short_descr), i->player.short_descr + 1);
    } else {
        if (IS_DEAD(i))
            send_to_char(ch, "O espírito de ");
        send_to_char(ch, "%s %s%s", i->player.name, *GET_TITLE(i) ? "" : "", GET_TITLE(i));
    }

    if (AFF_FLAGGED(i, AFF_INVISIBLE))
        send_to_char(ch, "%s (invisível)\ty", CCWHT(ch, C_NRM));
    if (AFF_FLAGGED(i, AFF_HIDE))
        send_to_char(ch, "%s (escondid%c)%s", CCWHT(ch, C_NRM), art, CCNRM(ch, C_NRM));
    if (!IS_NPC(i) && !i->desc)
        send_to_char(ch, " %s(%slinkless%s)%s", CCGRN(ch, C_NRM), CBWHT(ch, C_CMP), CCGRN(ch, C_CMP), CCNRM(ch, C_NRM));
    if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
        send_to_char(ch, "%s (escrevendo)%s", CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
    if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK))
        send_to_char(ch, "%s (construindo)%s", CBRED(ch, C_NRM), CCNRM(ch, C_NRM));
    if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_AFK))
        send_to_char(ch, "%s (away)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

    if (GET_POS(i) != POS_FIGHTING) {

        if (SITTING(i))
            furniture = SITTING(i);

        if (SWIMMING(i))
            send_to_char(ch, " está nadando aqui.");
        else {
            switch (GET_POS(i)) {
                case (POS_DEAD):
                    if (FLYING(i))
                        send_to_char(ch, " está flutuando aqui, mort%c.", art);
                    else
                        send_to_char(ch, " está caíd%c aqui, mort%c.", art, art);
                    break;
                case POS_MORTALLYW:
                    if (FLYING(i))
                        send_to_char(ch, " está flutuando aqui, mortalmente ferid%c.", art);
                    else
                        send_to_char(ch, " está caíd%c aqui, mortalmente ferid%c.", art, art);
                    break;
                case POS_INCAP:
                    if (FLYING(i))
                        send_to_char(ch, " está flutuando aqui, incapacitad%c.", art);
                    else
                        send_to_char(ch, " está caíd%c aqui, incapacitad%c.", art, art);
                    break;
                case POS_STUNNED:
                    if (FLYING(i))
                        send_to_char(ch, " está flutuando aqui, atordoad%c.", art);
                    else
                        send_to_char(ch, " está caíd%c aqui, atordoad%c.", art, art);
                    break;
                case POS_MEDITING:
                    if (FLYING(i))
                        send_to_char(ch, " está em um transe profundo, meditando enquanto levita.");
                    else
                        send_to_char(ch, " está em um transe profundo, meditando.");
                    break;
                case POS_SLEEPING:
                    if (FLYING(i)) {
                        if (SITTING(i))
                            send_to_char(ch, " está aqui, dormindo em %s no ar.", OBJS(furniture, ch));
                        else
                            send_to_char(ch, " está aqui, dormindo no ar.");
                    } else {
                        if (SITTING(i))
                            send_to_char(ch, " está dormindo em %s.", OBJS(furniture, ch));
                        else
                            send_to_char(ch, " está dormindo aqui.");
                    }
                    break;
                case POS_RESTING:
                    if (FLYING(i)) {
                        if (SITTING(i))
                            send_to_char(ch, " está descansando em %s no ar.", OBJS(furniture, ch));
                        else
                            send_to_char(ch, " está aqui, descansando no ar.");
                    } else {
                        if (SITTING(i))
                            send_to_char(ch, " está descansando em %s.", OBJS(furniture, ch));
                        else
                            send_to_char(ch, " está descansando aqui.");
                    }
                    break;
                case POS_SITTING:
                    if (FLYING(i)) {
                        if (SITTING(i))
                            send_to_char(ch, " está aqui, sentad%c em %s no ar.", art, OBJS(furniture, ch));
                        else
                            send_to_char(ch, " está aqui, sentad%c no ar.", art);
                    } else {
                        if (SITTING(i))
                            send_to_char(ch, " está sentad%c em %s.", art, OBJS(furniture, ch));
                        else
                            send_to_char(ch, " está sentad%c aqui.", art);
                    }
                    break;
                case POS_STANDING:
                    if (FLYING(i))
                        send_to_char(ch, " está voando aqui.");
                    else
                        send_to_char(ch, " está em pé aqui.");
                    break;
                default:
                    send_to_char(ch, "!FIGHTING");
                    break;
            }
        }
    } else {
        if (FIGHTING(i)) {
            send_to_char(ch, " está aqui, lutando contra ");
            if (FIGHTING(i) == ch)
                send_to_char(ch, "VOCÊ!");
            else {
                if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
                    send_to_char(ch, "%s!", PERS(FIGHTING(i), ch));
                else
                    send_to_char(ch, "alguem que já se foi!");
            }
        } else /* NIL fighting pointer */
            send_to_char(ch, " está aqui brigando com o nada.");
    }

    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
        if (IS_EVIL(i))
            send_to_char(ch, "%s(Aura Vermelha)%s ", CBRED(ch, C_NRM), CCNRM(ch, C_NRM));
        else if (IS_GOOD(i))
            send_to_char(ch, "%s(Aura Azul)%s ", CBBLU(ch, C_NRM), CCNRM(ch, C_NRM));
    }
    send_to_char(ch, "\tn\r\n");
    if (AFF_FLAGGED(i, AFF_SANCTUARY))
        act("\tW...$l brilha com uma luz branca!\tn", FALSE, i, 0, ch, TO_VICT);
    else if (AFF_FLAGGED(i, AFF_GLOOMSHIELD)) {
        act("\tL...$l é resguardad$r por um espesso escudo de trevas!\tn", FALSE, i, 0, ch, TO_VICT);
    }
    if (AFF_FLAGGED(i, AFF_FIRESHIELD))
        act("\tR...$l está envolvid$r por uma aura de fogo!\tn", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_WINDWALL))
        act("\tw...$l está envolvid$r por uma parede de vento!\tn", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_BLIND))
        act("\tw...$l está tateando ao redor, ceg$r!\tn", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_FIREFLIES))
        act("\tG...$l está rodead$r por vaga-lumes!\tn", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_THISTLECOAT))
        act("\ty...$l está protegid$r por uma barreira espinhos!\tn", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_SOUNDBARRIER))
        act("\tc...$l está envolt$r por uma protetora barreira de som!\tn", FALSE, i, 0, ch, TO_VICT);
}

static void list_char_to_char(struct char_data *list, struct char_data *ch)
{
    struct char_data *i;
    for (i = list; i; i = i->next_in_room)
        if (ch != i) {
            /* hide npcs whose description starts with a '.' from
               non-holylighted people - Idea from Elaseth of TBA */
            if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) && IS_NPC(i) && i->player.long_descr &&
                *i->player.long_descr == '.')
                continue;
            send_to_char(ch, "%s", CCYEL(ch, C_NRM));
            if (CAN_SEE(ch, i))
                list_one_char(i, ch);
            else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) && AFF_FLAGGED(i, AFF_INFRAVISION))
                send_to_char(ch, "Você vê um par de %solhos vermelhos%s te olhando intensamente.\r\n", CCRED(ch, C_CMP),
                             CCYEL(ch, C_NRM));
            else if (IS_DEAD(i) && !rand_number(0, 5))
                send_to_char(ch, "Você sente a presença de alguém na sala...\r\n");

            send_to_char(ch, "%s", CCNRM(ch, C_NRM));
        }
}

static void do_auto_exits(struct char_data *ch)
{
    int door, slen = 0;
    send_to_char(ch, "%s[ Saídas: ", CCCYN(ch, C_NRM));
    for (door = 0; door < DIR_COUNT; door++) {
        if (!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
            continue;
        if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) && !CONFIG_DISP_CLOSED_DOORS)
            continue;
        if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
            continue;
        if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
            send_to_char(ch, "%s!%s%s ", EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) ? CCWHT(ch, C_NRM) : CCRED(ch, C_NRM),
                         autoexits[door], CCCYN(ch, C_NRM));
        else if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN))
            send_to_char(ch, "%s%s%s ", CCWHT(ch, C_NRM), autoexits[door], CCCYN(ch, C_NRM));
        else
            send_to_char(ch, "\t(%s\t) ", autoexits[door]);
        slen++;
    }

    send_to_char(ch, "%s]%s\r\n", slen ? "" : "Nenhuma!", CCNRM(ch, C_NRM));
}

ACMD(do_exits)
{
    int door, len = 0;
    if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT) {
        act("Você não pode ver nada! Você está ceg$r!", FALSE, ch, NULL, NULL, TO_CHAR);
        return;
    }

    send_to_char(ch, "Saídas:\r\n");
    for (door = 0; door < DIR_COUNT; door++) {
        if (!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
            continue;
        if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) && !CONFIG_DISP_CLOSED_DOORS)
            continue;
        if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
            continue;
        len++;
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS) && !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
            send_to_char(ch, "%-5s - [%5d]%s %s\r\n", dirs_pt[door], GET_ROOM_VNUM(EXIT(ch, door)->to_room),
                         EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) ? " [ESCONDIDA]" : "",
                         world[EXIT(ch, door)->to_room].name);
        else if (CONFIG_DISP_CLOSED_DOORS && EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
            /* But we tell them the door is closed */
            send_to_char(ch, "%-5s - %s (caminho fechado)%s\r\n", dirs[door],
                         (EXIT(ch, door)->keyword) ? fname(EXIT(ch, door)->keyword) : "(caminho aberto)",
                         EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) ? " (e escondido)" : ".");
        } else
            send_to_char(ch, "%-5s - %s\r\n", dirs[door],
                         IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch)
                             ? "(muito escuro para saber)"
                             : world[EXIT(ch, door)->to_room].name);
    }

    if (!len)
        send_to_char(ch, " Nenhuma.\r\n");
}

void look_at_room(struct char_data *ch, int ignore_brief)
{
    trig_data *t;
    struct room_data *rm = &world[IN_ROOM(ch)];
    room_vnum target_room;
    target_room = IN_ROOM(ch);
    int i, obj_list_mode;
    if (!ch->desc)
        return;
    if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char(ch, "Está tudo escuro...\r\n");
        return;
    } else if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "Você não vê nada... apenas a infinita escuridão...\r\n");
        return;
    }
    send_to_char(ch, "%s", CCCYN(ch, C_NRM));
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS)) {
        char buf[MAX_STRING_LENGTH];
        sprintbitarray(ROOM_FLAGS(IN_ROOM(ch)), room_bits, RF_ARRAY_MAX, buf);
        send_to_char(ch, "[%5d] ", GET_ROOM_VNUM(IN_ROOM(ch)));
        send_to_char(ch, "%s [ %s] [ %s ]", world[IN_ROOM(ch)].name, buf, sector_types[world[IN_ROOM(ch)].sector_type]);
        if (SCRIPT(rm)) {
            send_to_char(ch, "[T");
            for (t = TRIGGERS(SCRIPT(rm)); t; t = t->next)
                send_to_char(ch, " %d", GET_TRIG_VNUM(t));
            send_to_char(ch, "]");
        }
    } else
        send_to_char(ch, "%s", world[IN_ROOM(ch)].name);

    send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
    if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief || ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH)) {
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOMAP) && can_see_map(ch))
            str_and_map(world[target_room].description, ch, target_room);
        else
            send_to_char(ch, "%s", world[IN_ROOM(ch)].description);
    }

    if (!PLR_FLAGGED(ch, PLR_GHOST)) {
        for (i = 0; i < NUM_OF_DIRS; i++)
            if (EXIT(ch, i) && EXIT(ch, i)->to_room != NOWHERE && ROOM_FLAGGED(EXIT(ch, i)->to_room, ROOM_DEATH)) {
                send_to_char(ch, "\tWVocê sente \tRPERIGO\tW por perto.\tn\r\n");
                break;
            }
    }

    /* autoexits */
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
        do_auto_exits(ch);
    /* now list characters & objects */
        obj_list_mode = SHOW_OBJ_LONG;
        if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
             obj_list_mode = SHOW_OBJ_SHORT;

        list_obj_to_char(world[IN_ROOM(ch)].contents, ch, obj_list_mode, FALSE);
        list_char_to_char(world[IN_ROOM(ch)].people, ch);


    /* Show mana density if character has detect magic active */
    if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
        float mana_density = calculate_mana_density(ch);
        const char *density_desc;
        int color_level;
        const char *density_color;

        /* Get description and color level from helper function */
        get_mana_density_description(mana_density, &density_desc, &color_level);

        /* Map color level to color macro */
        switch (color_level) {
            case 0:
                density_color = CBCYN(ch, C_NRM);
                break;
            case 1:
                density_color = CBGRN(ch, C_NRM);
                break;
            case 2:
                density_color = CCYEL(ch, C_NRM);
                break;
            case 3:
                density_color = CCRED(ch, C_NRM);
                break;
            case 4:
            default:
                density_color = CBRED(ch, C_NRM);
                break;
        }

        send_to_char(ch, "\r\n%sA densidade mágica aqui está %s%s%s.%s\r\n", CCMAG(ch, C_NRM), density_color,
                     density_desc, CCNRM(ch, C_NRM), CCNRM(ch, C_NRM));
    }
}

static void look_in_direction(struct char_data *ch, int dir)
{
    if (EXIT(ch, dir)) {
        if (EXIT(ch, dir)->general_description)
            send_to_char(ch, "%s", EXIT(ch, dir)->general_description);
        else
            send_to_char(ch, "Você não vê nada de especial.\r\n");
        if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && EXIT(ch, dir)->keyword)
            send_to_char(ch, "%s está fechada.\r\n", fname(EXIT(ch, dir)->keyword));
        else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword)
            send_to_char(ch, "%s está aberta.\r\n", fname(EXIT(ch, dir)->keyword));
    } else
        send_to_char(ch, "Você não vê nada de especial.\r\n");
}

static void look_in_obj(struct char_data *ch, char *arg)
{
    struct obj_data *obj = NULL;
    struct char_data *dummy = NULL;
    int amt, bits;
    if (!*arg)
        send_to_char(ch, "Olhar dentro do quê?\r\n");
    else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
        send_to_char(ch, "Não parece ter %s aqui.\r\n", arg);
    } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) && (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
               (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) && (GET_OBJ_TYPE(obj) != ITEM_CORPSE))
        send_to_char(ch, "Não há nada dentro disso!\r\n");
    else {
        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER || GET_OBJ_TYPE(obj) == ITEM_CORPSE) {
            if (OBJVAL_FLAGGED(obj, CONT_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
                send_to_char(ch, "Está fechado.\r\n");
            else {
                send_to_char(ch, "%s", fname(obj->name));
                switch (bits) {
                    case FIND_OBJ_INV:
                        send_to_char(ch, " (carregando): \r\n");
                        break;
                    case FIND_OBJ_ROOM:
                        send_to_char(ch, " (aqui): \r\n");
                        break;
                    case FIND_OBJ_EQUIP:
                        send_to_char(ch, " (usando): \r\n");
                        break;
                }

                list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, TRUE);
            }
        } else { /* item must be a fountain or drink container */
            if ((GET_OBJ_VAL(obj, 1) == 0) && (GET_OBJ_VAL(obj, 0) != -1))
                send_to_char(ch, "Está vazio.\r\n");
            else {
                if (GET_OBJ_VAL(obj, 0) < 0) {
                    char buf2[MAX_STRING_LENGTH];
                    sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2, sizeof(buf2));
                    send_to_char(ch, "Isto está cheio de um liquido %s.\r\n", buf2);
                } else if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0))
                    send_to_char(ch, "O conteúdo parece um pouco estragado.\r\n"); /* BUG
                                                                                    */
                else {
                    char buf2[MAX_STRING_LENGTH];
                    amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
                    sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2, sizeof(buf2));
                    send_to_char(ch, "Está %s de um liquído %s.\r\n", fullness[amt], buf2);
                }
            }
        }
    }
}

char *find_exdesc(char *word, struct extra_descr_data *list)
{
    struct extra_descr_data *i;
    for (i = list; i; i = i->next)
        if (*i->keyword == '.' ? isname(word, i->keyword + 1) : isname(word, i->keyword))
            return (i->description);
    return (NULL);
}

/* Given the argument "look at <target>", figure out what object or char
   matches the target.  First, see if there is another char in the room
   with the name.  Then check local objs for exdescs. Thanks to Angus
   Mezick for the suggested fix to this problem. */
static void look_at_target(struct char_data *ch, char *arg)
{
    int bits, found = FALSE, j, fnum, i = 0;
    struct char_data *found_char = NULL;
    struct obj_data *obj, *found_obj = NULL;
    char *desc;
    if (!ch->desc)
        return;
    if (!*arg) {
        send_to_char(ch, "Olhar para o quê?\r\n");
        return;
    }

    bits =
        generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &found_char, &found_obj);
    /* Is the target a character? */
    if (found_char != NULL) {
        look_at_char(found_char, ch);
        if (ch != found_char) {
            if (CAN_SEE(found_char, ch))
                act("$n olha para você.", TRUE, ch, 0, found_char, TO_VICT);
            act("$n olha para $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
        }
        return;
    }

    /* Strip off "number." from 2.foo and friends. */
    if (!(fnum = get_number(&arg))) {
        send_to_char(ch, "Olhar para o quê?\r\n");
        return;
    }

    /* Does the argument match an extra desc in the room? */
    if ((desc = find_exdesc(arg, world[IN_ROOM(ch)].ex_description)) != NULL && ++i == fnum) {
        page_string(ch->desc, desc, FALSE);
        return;
    }

    /* Does the argument match an extra desc in the char's equipment? */
    for (j = 0; j < NUM_WEARS && !found; j++)
        if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
            if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum) {
                send_to_char(ch, "%s", desc);
                found = TRUE;
            }

    /* Does the argument match an extra desc in the char's inventory? */
    for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
        if (CAN_SEE_OBJ(ch, obj))
            if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
                send_to_char(ch, "%s", desc);
                found = TRUE;
            }
    }

    /* Does the argument match an extra desc of an object in the room? */
    for (obj = world[IN_ROOM(ch)].contents; obj && !found; obj = obj->next_content)
        if (CAN_SEE_OBJ(ch, obj))
            if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
                send_to_char(ch, "%s", desc);
                found = TRUE;
            }

    /* If an object was found back in generic_find */
    if (bits) {
        if (!found)
            show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
        else {
            show_obj_modifiers(found_obj, ch, 0);
            send_to_char(ch, "\r\n");
        }
    } else if (!found)
        send_to_char(ch, "Você não vê isto aqui.\r\n");
}

ACMD(do_look)
{
    int look_type;
    int found = 0;
    char tempsave[MAX_INPUT_LENGTH];
    if (!ch->desc)
        return;
    if (GET_POS(ch) < POS_SLEEPING)
        send_to_char(ch, "Você não pode ver nada além de estrelas!");
    else if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT) {
        act("Você não pode ver nada, você está ceg$r!", false, ch, NULL, NULL, TO_CHAR);
    }

    else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char(ch, "Está tudo escuro...\r\n");

        list_char_to_char(world[IN_ROOM(ch)].people, ch); /* glowing red
                                                                                                             eyes */
    } else {
        char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
        half_chop(argument, arg, arg2);
        if (subcmd == SCMD_READ) {
            if (!*arg)
                send_to_char(ch, "Ler o quê?\r\n");
            else
                look_at_target(ch, strcpy(tempsave, arg));
            return;
        }
        if (!*arg) /* "look" alone, without an argument at all */
            look_at_room(ch, 1);
        else if (is_abbrev(arg, "ceu") || is_abbrev(arg, "sky"))
            look_at_sky(ch);
        else if (is_abbrev(arg, "em") || is_abbrev(arg, "in"))
            look_in_obj(ch, arg2);
        /* did the char type 'look <direction>?' */
        else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
            look_in_direction(ch, look_type);
        else if (is_abbrev(arg, "para") || is_abbrev(arg, "at"))
            look_at_target(ch, strcpy(tempsave, arg2));
        else if (is_abbrev(arg, "around") || is_abbrev(arg, "emvolta")) {
            struct extra_descr_data *i;
            for (i = world[IN_ROOM(ch)].ex_description; i; i = i->next) {
                if (*i->keyword != '.') {
                    send_to_char(ch, "%s%s:\r\n%s", (found ? "\r\n" : ""), i->keyword, i->description);
                    found = 1;
                }
            }
            if (!found)
                send_to_char(ch, "Você não encontrou nada notavel.\r\n");
        } else
            look_at_target(ch, strcpy(tempsave, arg));
    }
}

ACMD(do_examine)
{
    struct char_data *tmp_char;
    struct obj_data *tmp_object;
    char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    if (!*arg) {
        send_to_char(ch, "Examinar o quê?\r\n");
        return;
    }

    /* look_at_target() eats the number. */
    look_at_target(ch, strcpy(tempsave, arg)); /* strcpy: OK */
    generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
    if (tmp_object) {
        if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) || (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
            (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER) || (GET_OBJ_TYPE(tmp_object) == ITEM_CORPSE)) {
            send_to_char(ch, "Quando você olha dentro, você vê:\r\n");
            look_in_obj(ch, arg);
        }
    }
}

ACMD(do_gold)
{
    if (GET_GOLD(ch) == 0)
        act("Você está quebrad$r!", FALSE, ch, 0, 0, TO_CHAR);
    else if (GET_GOLD(ch) == 1)
        send_to_char(ch, "Você tem uma miserável moedinha de ouro.\r\n");
    else
        send_to_char(ch, "Você têm %d moedas de ouro.\r\n", GET_GOLD(ch));
}

ACMD(do_score)
{
    const char *underline =
        "________________________________________________________________________________"
        "________________________________________________________________________________\tn\r\n";

    struct time_info_data playing_time;
    if (IS_NPC(ch))
        return;
    ubyte screen_width = GET_SCREEN_WIDTH(ch);

    char buffer[screen_width + 8];   // 80 caracteres + null terminator
    snprintf(buffer, sizeof(buffer), "\tB___[ \tW%s, \tn%s \tB]___%s", GET_NAME(ch), GET_TITLE(ch), underline);
    buffer[screen_width + 7] = '\0';   // Garante o truncamento exato em screen_width  caracteres
    send_to_char(ch, "%s\tn\r\n", buffer);

    send_to_char(ch, "  %s, %d anos, nível %d.\r\n", pc_class_types[GET_CLASS(ch)], GET_AGE(ch), GET_LEVEL(ch));
    send_to_char(ch, "\r\n");

    send_to_char(ch, " \tW Saúde\tb......:\tn %5d\tgHp\tn (%5d) %s \tn \tW Armadura\tb...:\tn%5d/10\tn\r\n",
                 GET_HIT(ch), GET_MAX_HIT(ch), gauge(0, 0, GET_HIT(ch), GET_MAX_HIT(ch)), compute_armor_class(ch));

    send_to_char(ch, " \tW Magia\tb......:\tn %5d\tgMn\tn (%5d) %s \tn \tW Alinhamento\tb:\tn%5d %s\tn\r\n",
                 GET_MANA(ch), GET_MAX_MANA(ch), gauge(0, 0, GET_MANA(ch), GET_MAX_MANA(ch)), GET_ALIGNMENT(ch),
                 align_gauge(GET_ALIGNMENT(ch)));

    send_to_char(ch, " \tW Movimento\tb..:\tn %5d\tgMv\tn (%5d) %s \tn \tW Fôlego\tb.....:\tn%5d (%5d) %s\tn\r\n",
                 GET_MOVE(ch), GET_MAX_MOVE(ch), gauge(0, 0, GET_MOVE(ch), GET_MAX_MOVE(ch)), GET_BREATH(ch),
                 GET_MAX_BREATH(ch), gauge(0, 0, GET_BREATH(ch), GET_MAX_BREATH(ch)));

    send_to_char(ch, "\r\n");

    send_to_char(ch, " \tW Experiência\tb:\tn %'13ld\tgxp\tn        \tn \tW Ouro\tb.......:\tn%'13d\tYg\tn\r\n",
                 GET_EXP(ch), GET_GOLD(ch));
    send_to_char(ch, " \tW Prox Nível\tb.:\tn %'13ld\tgxp\tn %s\r\n ",
                 level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1) - GET_EXP(ch),
                 gauge(0, 0, GET_EXP(ch) - level_exp(GET_CLASS(ch), GET_LEVEL(ch)),
                       level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1) - level_exp(GET_CLASS(ch), GET_LEVEL(ch))));
    send_to_char(ch, " \tW Reputação\tb.:\tn %14d         \tn %s\r\n ", GET_REPUTATION(ch),
                 reputation_gauge(GET_REPUTATION(ch)));
    send_to_char(ch, "\r\n");

    send_to_char(ch,
                 " \tW Atrib\tb:\tW \tgStr\tn [%d/%d]   \tgInt\tn [%d]   \tgWis\tn [%d]   \tgDex\tn [%d]   \tgCon\tn "
                 "[%d]   \tgChar\tn [%d] \r\n",
                 GET_STR(ch), GET_ADD(ch), GET_INT(ch), GET_WIS(ch), GET_DEX(ch), GET_CON(ch), GET_CHA(ch));

    send_to_char(ch,
                 "  \tWResis\tb: \tgPar\tn [%2d]   \tgRod\tn [%2d]   \tgPet\tn [%2d] "
                 "  \tgBre\tn [%2d]   \tgSpe\tn [%2d]\r\n",
                 GET_SAVE(ch, SAVING_PARA), GET_SAVE(ch, SAVING_ROD), GET_SAVE(ch, SAVING_PETRI),
                 GET_SAVE(ch, SAVING_BREATH), GET_SAVE(ch, SAVING_SPELL));

    send_to_char(ch, "  \tWBônus\tb: \tgHitRoll\tn [%2d]   \tgDamRoll\tn [%2d]\r\n", GET_HITROLL(ch), GET_DAMROLL(ch));
    send_to_char(ch, "\r\n");
    if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT) {

        send_to_char(ch, "  PoofIn:  &g%s&:&+n\r\n", POOFIN(ch));

        send_to_char(ch, "  PoofOut: &g%s&:&+n\r\n", POOFOUT(ch));

        if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_COMPACT))
            send_to_char(ch, "\r\n");
    }

    send_to_char(ch, "\tnVocê irá recuperar [%d\tgHp\tn %d\tgMn\tn %d\tgMv\tn] por tick.\r\n", hit_gain(ch),
                 mana_gain(ch), move_gain(ch));

    send_to_char(ch, "Você morreu \tR%d\tn vez%s e caiu em \tR%d\tn armadilhas de morte.\r\n", GET_DEATH(ch),
                 GET_DEATH(ch) == 1 ? "" : "es", GET_DTS(ch));
    playing_time = *real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
    send_to_char(ch, "Você está jogando por \tR%d\tn dia%s e \tR%d\tn hora%s \r\n", playing_time.day,
                 playing_time.day == 1 ? " " : "s", playing_time.hours, playing_time.hours == 1 ? "." : "s.");

    send_to_char(ch, "Você tem \tg%d\tn pontos de busca e ", GET_QUESTPOINTS(ch));
    send_to_char(ch, "completou \tg%d\tn busca%s.\r\n", GET_NUM_QUESTS(ch), GET_NUM_QUESTS(ch) == 1 ? " " : "s");
    if (GET_QUEST(ch) != NOTHING) {
        qst_rnum quest_rnum = real_quest(GET_QUEST(ch));
        if (quest_rnum != NOTHING) {
            send_to_char(ch, "A sua busca atual é: \tg%s\tn", QST_NAME(quest_rnum));
            if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
                send_to_char(ch, "\tn[%d] \r\n", GET_QUEST(ch));
            else
                send_to_char(ch, " \tn\r\n");
        }
    }
    send_to_char(ch, "Você tem \tg%3d Enc.\tn (Encarnações).\r\n", GET_REMORT(ch));

    if (PLR_FLAGGED(ch, PLR_TRNS))
        send_to_char(ch, "Você transcendeu.\r\n");

    switch (GET_POS(ch)) {
        case POS_DEAD:
            send_to_char(ch, "Você está mort%s!\r\n", OA(ch));
            break;
        case POS_MORTALLYW:
            send_to_char(ch, "Você está mortalmente ferid%s! Você precisa de ajuda!!\r\n", OA(ch));
            break;
        case POS_INCAP:
            send_to_char(ch, "Você está incapacitad%s, morrendo lentamente...\r\n", OA(ch));
            break;
        case POS_STUNNED:
            send_to_char(ch, "Você está atordoad%s! Não pode se mover!\r\n", OA(ch));
            break;
        case POS_MEDITING:
            send_to_char(ch, "Você está meditando profundamente.\r\n");
            break;
        case POS_SLEEPING:
            send_to_char(ch, "Você está dormindo.\r\n");
            break;
        case POS_RESTING:
            send_to_char(ch, "Você está descansando.\r\n");
            break;
        case POS_SITTING:
            if (!SITTING(ch))
                send_to_char(ch, "Você está sentad%s.\r\n", OA(ch));
            else {
                struct obj_data *furniture = SITTING(ch);
                send_to_char(ch, "Você está sentad%s em %s.\r\n", OA(ch), furniture->short_description);
            }
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Você está lutando contra %s.\r\n",
                         FIGHTING(ch) ? PERS(FIGHTING(ch), ch) : "alguem que já partiu");
            break;
        case POS_STANDING:
            send_to_char(ch, "Você está em pé.\r\n");
            break;
        default:
            send_to_char(ch, "Você está voando.\r\n");
            break;
    }
    if (SWIMMING(ch))
        send_to_char(ch, "Você está nadando\r\n");

    if (GET_COND(ch, DRUNK) > 10)
        send_to_char(ch, "Você está bebad%s.\r\n", OA(ch));
    if (GET_COND(ch, HUNGER) == 0)
        send_to_char(ch, "Você está famint%s.\r\n", OA(ch));
    if (GET_COND(ch, THIRST) == 0)
        send_to_char(ch, "Você está sedent%s.\r\n", OA(ch));
    if (AFF_FLAGGED(ch, AFF_BREATH))
        send_to_char(ch, "Você não precisa respirar.\r\n");
    else if (!IS_NPC(ch) && (SECT(IN_ROOM(ch)) == SECT_UNDERWATER || ROOM_FLAGGED(IN_ROOM(ch), ROOM_HIGH))) {
        if (GET_BREATH(ch) < 12)
            send_to_char(ch, "\tWVocê realmente precisa de AR!!!\tn\r\n");
        else if (GET_BREATH(ch) < 30)
            send_to_char(ch, "Você precisa de ar.\r\n");
    } else if (!IS_NPC(ch) && GET_BREATH(ch) < 30 && GET_BREATH(ch) < GET_MAX_BREATH(ch))
        send_to_char(ch, "Você está tomando ar.\r\n");

    if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT)
        send_to_char(ch, "Você está ceg%s!\r\n", OA(ch));
    if (AFF_FLAGGED(ch, AFF_INVISIBLE))
        send_to_char(ch, "Você está invisível.\r\n");
    if (AFF_FLAGGED(ch, AFF_POISON))
        send_to_char(ch, "Você está envenenad%s!\r\n", OA(ch));
    if (AFF_FLAGGED(ch, AFF_CHARM))
        send_to_char(ch, "Você foi encantad%s!\r\n", OA(ch));
    if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
        send_to_char(ch, "Você sente a presença de objetos e pessoas invisíveis.\r\n");
    if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
        send_to_char(ch, "Você está sensivel a presença de coisas mágicas.\r\n");
    if (AFF_FLAGGED(ch, AFF_SANCTUARY))
        send_to_char(ch, "Você está protegid%s pelo Santuario.\r\n", OA(ch));
    else if (AFF_FLAGGED(ch, AFF_GLOOMSHIELD))
        send_to_char(ch, "Você está protegid%s por um espesso escudo de trevas!.\r\n", OA(ch));
    if (AFF_FLAGGED(ch, AFF_FIRESHIELD))
        send_to_char(ch, "Você está protegid%s por uma aura de fogo!\r\n", OA(ch));
    if (AFF_FLAGGED(ch, AFF_WINDWALL))
        send_to_char(ch, "Você está protegid%s por uma parede de vento!\r\n", OA(ch));

    if (AFF_FLAGGED(ch, AFF_PROTECT_GOOD))
        send_to_char(ch, "Você se sente protegid%s contra seres bons.\r\n", OA(ch));
    if (AFF_FLAGGED(ch, AFF_PROTECT_EVIL))
        send_to_char(ch, "Você se sente protegid%s contra seres maus.\r\n", OA(ch));
    if (affected_by_spell(ch, SPELL_BLESS_PERSON))
        send_to_char(ch, "Você foi abençoad%s.\r\n", OA(ch));

    else if (affected_by_spell(ch, SPELL_SKIN_LIKE_DIAMOND))
        send_to_char(ch, "Você está protegid%s como uma pedra preciosa.\r\n", OA(ch));
    else if (affected_by_spell(ch, SPELL_IMPROVED_ARMOR))
        send_to_char(ch, "Você se sente muito protegid%s.\r\n", OA(ch));
    else if (affected_by_spell(ch, SPELL_SKIN_LIKE_STEEL))
        send_to_char(ch, "Você está protegid%s pela força do aço.\r\n", OA(ch));
    else if (affected_by_spell(ch, SPELL_SKIN_LIKE_ROCK))
        send_to_char(ch, "Você está protegid%s pelas Montanhas Dragonhelm.\r\n", OA(ch));
    else if (affected_by_spell(ch, SPELL_ARMOR))
        send_to_char(ch, "Você se sente protegid%s.\r\n", OA(ch));
    else if (affected_by_spell(ch, SPELL_SKIN_LIKE_WOOD))
        send_to_char(ch, "Você está protegid%s pelo Grande Carvalho.\r\n", OA(ch));

    if (get_nighthammer(ch, false) > 0)
        send_to_char(ch, "Você está protegid%s pelo manto da noite.\r\n", OA(ch));

    if (affected_by_spell(ch, SPELL_FLY))
        send_to_char(ch, "Você sente bem leve.\r\n");

    if (AFF_FLAGGED(ch, AFF_FIREFLIES))
        send_to_char(ch, "Você está rodead%s de vaga-lumes.\r\n", OA(ch));
    else if (AFF_FLAGGED(ch, AFF_INFRAVISION))
        send_to_char(ch, "Os seus olhos brilham vermelho!\r\n");

    if (AFF_FLAGGED(ch, AFF_STINGING))
        send_to_char(ch, "Vários insetos rodeiam você, causando muita dor.\r\n");

    if (AFF_FLAGGED(ch, AFF_STONESKIN)) {
        int points = get_stoneskin_points(ch);
        send_to_char(ch, "Sua pele está muito dura (%d pontos de proteção).\r\n", points);
    }

    if (AFF_FLAGGED(ch, AFF_THISTLECOAT))
        send_to_char(ch, "Você está protegid%s por um casaco de espinhos.\r\n", OA(ch));

    if (AFF_FLAGGED(ch, AFF_WATERWALK))
        send_to_char(ch, "Você pode andar sobre a água.\r\n");

    if (affected_by_spell(ch, SPELL_SOUNDBARRIER))
        send_to_char(ch, "Você está envolt%s por uma barreira de som.\r\n", OA(ch));

    if (AFF_FLAGGED(ch, AFF_ADAGIO))
        send_to_char(ch, "Seu corpo vibra em adagio.\r\n");

    if (AFF_FLAGGED(ch, AFF_ALLEGRO))
        send_to_char(ch, "Seu corpo vibra em allegro.\r\n");

    if (affected_by_spell(ch, CHANSON_ALENTO))
        send_to_char(ch, "Uma grande força é emanada de seu coração.\r\n");

    if (affected_by_spell(ch, CHANSON_ECOS))
        send_to_char(ch, "Ecos de enlouquecedora maldição preenchem sua cabeça.\r\n");

    if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
        send_to_char(ch, "Você pode ser convocad%s por outros jogadores.\r\n", OA(ch));
    else
        send_to_char(ch, "Você NÃO pode ser convocad%s!\r\n", OA(ch));

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        if (POOFIN(ch))
            send_to_char(ch, "%sPOOFIN: %s%s %s%s\r\n", QYEL, QCYN, GET_NAME(ch), POOFIN(ch), QNRM);
        else
            send_to_char(ch, "%sPOOFIN: %s%s appears with an ear-splitting bang.%s\r\n", QYEL, QCYN, GET_NAME(ch),
                         QNRM);
        if (POOFOUT(ch))
            send_to_char(ch, "%sPOOFOUT: %s%s %s%s\r\n", QYEL, QCYN, GET_NAME(ch), POOFOUT(ch), QNRM);
        else
            send_to_char(ch, "%sPOOFOUT: %s%s disappears in a puff of smoke.%s\r\n", QYEL, QCYN, GET_NAME(ch), QNRM);
        send_to_char(ch, "Your current zone: %s%d%s\r\n", CCCYN(ch, C_NRM), GET_OLC_ZONE(ch), CCNRM(ch, C_NRM));
    }
}

ACMD(do_affects)
{
    struct affected_type *aff;
    char duration_buf[128];
    int has_spell_affects = 0;
    int has_item_affects = 0;

    if (IS_NPC(ch))
        return;

    /* 1) Efeitos vindos de magias (lista de affects do char) */
    send_to_char(ch, "\tWEfeitos ativos:\tn\r\n");

    for (aff = ch->affected; aff; aff = aff->next) {
        if (aff->duration == -1) {
            snprintf(duration_buf, sizeof(duration_buf), "permanente");
        } else {
            snprintf(duration_buf, sizeof(duration_buf), "%d h%s",
                     aff->duration,
                     (aff->duration == 1) ? "" : "s");
        }

        /* Nome da magia em branco, duração em ciano escuro entre colchetes brancos */
        send_to_char(ch, "  \tW%s\tn \tW[\tc%s\tW]\tn\r\n",
                     skill_name(aff->spell),
                     duration_buf);
        has_spell_affects = 1;
    }

    /* 2) Efeitos vindos de itens equipados (AFF_XXX em GET_OBJ_AFFECT) */
    {
        int i;
        struct obj_data *obj = NULL;
        char bitbuf[MAX_STRING_LENGTH];
        char localbuf[MAX_STRING_LENGTH];
        char *tok;

        /* Primeiro pass: apenas verificar se existe ALGUM efeito de item. */
        for (i = 0; i < NUM_WEARS && !has_item_affects; i++) {
            if (!(obj = GET_EQ(ch, i)))
                continue;

            sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, bitbuf);
            if (!*bitbuf)
                continue;

            strlcpy(localbuf, bitbuf, sizeof(localbuf));
            tok = strtok(localbuf, " ");
            while (tok != NULL) {
                const char *pretty = aff_pretty_name(tok);

                if (*pretty && strcmp(pretty, "\n") != 0) {
                    has_item_affects = 1;
                    break;
                }

                tok = strtok(NULL, " ");
            }
        }

        /* Se houver efeitos de item, imprimimos o cabeçalho e listamos por item */
        if (has_item_affects) {
            send_to_char(ch, "\r\n\tWEfeitos de equipamentos:\tn\r\n");

            for (i = 0; i < NUM_WEARS; i++) {
                if (!(obj = GET_EQ(ch, i)))
                    continue;

                sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, bitbuf);
                if (!*bitbuf)
                    continue;

                strlcpy(localbuf, bitbuf, sizeof(localbuf));
                tok = strtok(localbuf, " ");

                /* Vamos imprimir um bloco por item assim:
                 * [nome do item em ciano escuro, colchetes brancos]
                 *     -> nome da magia em branco
                 */
                int printed_header_for_this_item = 0;

                while (tok != NULL) {
                    const char *pretty = aff_pretty_name(tok);

                    if (*pretty && strcmp(pretty, "\n") != 0) {
                        if (!printed_header_for_this_item) {
                            send_to_char(ch, "\tW[\tc%s\tW]\tn\r\n",
                                         obj->short_description ? obj->short_description : "(item sem descrição)");
                            printed_header_for_this_item = 1;
                        }
                        send_to_char(ch, "    -> \tW%s\tn\r\n", pretty);
                    }

                    tok = strtok(NULL, " ");
                }
            }
        }
    }
}

ACMD(do_inventory)
{
    send_to_char(ch, "\tWVocê está carregando:\tn\r\n");
    list_obj_to_char(ch->carrying, ch, SHOW_OBJ_SHORT, TRUE);
}

ACMD(do_equipment)
{
    int i, found = FALSE;
    struct obj_data *obj;

    send_to_char(ch, "\tWVocê está usando:\tn\r\n");

    for (i = 0; i < NUM_WEARS; i++) {
        if ((obj = GET_EQ(ch, i))) {
            found = TRUE;

            send_to_char(ch, "\tW%-12s\tn\tw", wear_where[i]);

            if (CAN_SEE_OBJ(ch, obj))
                show_obj_to_char(obj, ch, SHOW_OBJ_SHORT);
            else
                send_to_char(ch, "algo.\r\n");
        }
    }

    if (!found)
        send_to_char(ch, "  Nada.\r\n");
}

ACMD(do_time)
{
    const char *suf;
    int weekday, day;
    /* day in [1..35] */
    day = time_info.day + 1;
    /* 35 days in a month, 7 days a week */
    weekday = ((35 * time_info.month) + day) % 7;
    send_to_char(ch, "São %d %s, de %s. \r\n ", (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
                 time_info.hours >= 12 ? " pm " : " am ", weekdays[weekday]);
    /* Peter Ajamian supplied the following as a fix for a bug introduced in
       the ordinal display that caused 11, 12, and 13 to be incorrectly
       displayed as 11st, 12nd, and 13rd.  Nate Winters had already submitted
       a fix, but it hard-coded a limit on ordinal display which I want to
       avoid. -dak */
    suf = "°";
    /* if (((day % 100) / 10) != 1) { switch (day % 10) { case 1: suf = " st
       "; break; case 2: suf = " nd "; break; case 3: suf = " rd "; break; } }
     */
    send_to_char(ch, " O %d%s dia do %s, Ano %d. \r\n ", day, suf, month_name[time_info.month], time_info.year);
}

/* Helper function to get element color for weather display */
static const char *get_weather_element_color(struct char_data *ch, int element)
{
    switch (element) {
        case ELEMENT_FIRE:
            return CBRED(ch, C_NRM);
        case ELEMENT_WATER:
        case ELEMENT_ICE:
            return CBBLU(ch, C_NRM);
        case ELEMENT_AIR:
            return CBWHT(ch, C_NRM);
        case ELEMENT_EARTH:
            return CCYEL(ch, C_NRM);
        case ELEMENT_LIGHTNING:
            return CBCYN(ch, C_NRM);
        case ELEMENT_ACID:
            return CBGRN(ch, C_NRM);
        case ELEMENT_POISON:
            return CCGRN(ch, C_NRM);
        case ELEMENT_HOLY:
            return CBYEL(ch, C_NRM);
        case ELEMENT_UNHOLY:
            return CBMAG(ch, C_NRM);
        case ELEMENT_MENTAL:
            return CCMAG(ch, C_NRM);
        case ELEMENT_PHYSICAL:
            return CCWHT(ch, C_NRM);
        default:
            return CCNRM(ch, C_NRM);
    }
}

/* Helper function to get school color for weather display */
static const char *get_weather_school_color(struct char_data *ch, int school)
{
    switch (school) {
        case SCHOOL_EVOCATION:
            return CBRED(ch, C_NRM);
        case SCHOOL_CONJURATION:
            return CBGRN(ch, C_NRM);
        case SCHOOL_ILLUSION:
            return CBMAG(ch, C_NRM);
        case SCHOOL_DIVINATION:
            return CBCYN(ch, C_NRM);
        case SCHOOL_NECROMANCY:
            return CBBLK(ch, C_NRM);
        case SCHOOL_ENCHANTMENT:
            return CBYEL(ch, C_NRM);
        case SCHOOL_ABJURATION:
            return CBBLU(ch, C_NRM);
        case SCHOOL_ALTERATION:
            return CBWHT(ch, C_NRM);
        default:
            return CCNRM(ch, C_NRM);
    }
}

/**
 * Safely append formatted string to buffer with overflow protection
 *
 * This helper prevents buffer overflows that can freeze players (requiring reconnect).
 * If overflow would occur, appends "***OVERFLOW***" message and returns FALSE.
 *
 * @param buffer The destination buffer
 * @param bufsize Total size of the buffer
 * @param overflow_occurred Pointer to flag that tracks if overflow has occurred
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 * @return TRUE if append succeeded, FALSE if overflow would occur
 */
static int safe_strcat_formatted(char *buffer, size_t bufsize, int *overflow_occurred, const char *format, ...)
{
    size_t current_len;
    size_t remaining;
    int written;
    va_list args;

    /* If overflow already occurred, don't append anything more */
    if (*overflow_occurred)
        return FALSE;

    current_len = strlen(buffer);
    remaining = bufsize - current_len;

    /* Reserve space for potential overflow message */
    if (remaining < 100) {
        snprintf(buffer + current_len, remaining, "\r\n***LISTA TRUNCADA (OVERFLOW)***\r\n");
        *overflow_occurred = TRUE;
        return FALSE;
    }

    va_start(args, format);
    written = vsnprintf(buffer + current_len, remaining, format, args);
    va_end(args);

    /* Check if output was truncated */
    if (written < 0 || (size_t)written >= remaining) {
        /* Truncation occurred, add overflow message */
        snprintf(buffer + current_len, remaining, "\r\n***LISTA TRUNCADA (OVERFLOW)***\r\n");
        *overflow_occurred = TRUE;
        return FALSE;
    }

    return TRUE;
}

/* Analyze weather conditions and show magical effects */
static void show_magical_conditions(struct char_data *ch, struct weather_data *weather)
{
    /* Defensive programming: validate inputs */
    if (!ch || !weather) {
        mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: show_magical_conditions called with NULL pointer(s)");
        return;
    }

    /* Use reasonable buffer sizes for weather display (1KB each is more than enough) */
    char favored_schools[1024] = "";
    char unfavored_schools[1024] = "";
    char favored_elements[1024] = "";
    char unfavored_elements[1024] = "";
    int has_favored_schools = 0, has_unfavored_schools = 0;
    int has_favored_elements = 0, has_unfavored_elements = 0;
    size_t len; /* For safe buffer management */

    /* Check spell schools */
    if (weather->sky >= SKY_RAINING && weather->humidity > 0.6) {
        len = strlen(favored_schools);
        if (len < sizeof(favored_schools) - 100) { /* Ensure enough space */
            snprintf(favored_schools + len, sizeof(favored_schools) - len, "  %sEvocação%s (+15%%)\r\n",
                     get_weather_school_color(ch, SCHOOL_EVOCATION), CCNRM(ch, C_NRM));
        }
        has_favored_schools = 1;
    } else if (weather->sky <= SKY_CLOUDLESS && weather->humidity < 0.4) {
        len = strlen(unfavored_schools);
        if (len < sizeof(unfavored_schools) - 100) { /* Ensure enough space */
            snprintf(unfavored_schools + len, sizeof(unfavored_schools) - len, "  %sEvocação%s (-10%%)\r\n",
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        has_unfavored_schools = 1;
    }

    if (weather->humidity >= 0.4 && weather->humidity <= 0.7) {
        len = strlen(favored_schools);
        if (len < sizeof(favored_schools) - 100) { /* Ensure enough space */
            snprintf(favored_schools + len, sizeof(favored_schools) - len, "  %sConjuração%s (+10%%)\r\n",
                     get_weather_school_color(ch, SCHOOL_CONJURATION), CCNRM(ch, C_NRM));
        }
        has_favored_schools = 1;
    }

    if (weather->sky == SKY_CLOUDY && weather->humidity > 0.8) {
        len = strlen(favored_schools);
        if (len < sizeof(favored_schools) - 100) { /* Ensure enough space */
            snprintf(favored_schools + len, sizeof(favored_schools) - len, "  %sIlusão%s (+20%%)\r\n",
                     get_weather_school_color(ch, SCHOOL_ILLUSION), CCNRM(ch, C_NRM));
        }
        has_favored_schools = 1;
    } else if (weather->sky == SKY_CLOUDLESS) {
        len = strlen(unfavored_schools);
        if (len < sizeof(unfavored_schools) - 100) { /* Ensure enough space */
            snprintf(unfavored_schools + len, sizeof(unfavored_schools) - len, "  %sIlusão%s (-15%%)\r\n",
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        has_unfavored_schools = 1;
    }

    if (weather->sky == SKY_CLOUDLESS && weather->humidity < 0.5) {
        len = strlen(favored_schools);
        if (len < sizeof(favored_schools) - 100) { /* Ensure enough space */
            snprintf(favored_schools + len, sizeof(favored_schools) - len, "  %sAdivinhação%s (+15%%)\r\n",
                     get_weather_school_color(ch, SCHOOL_DIVINATION), CCNRM(ch, C_NRM));
        }
        has_favored_schools = 1;
    } else if (weather->sky >= SKY_RAINING) {
        len = strlen(unfavored_schools);
        if (len < sizeof(unfavored_schools) - 100) { /* Ensure enough space */
            snprintf(unfavored_schools + len, sizeof(unfavored_schools) - len, "  %sAdivinhação%s (-10%%)\r\n",
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        has_unfavored_schools = 1;
    }

    if (weather->sky >= SKY_RAINING) {
        len = strlen(favored_schools);
        if (len < sizeof(favored_schools) - 100) { /* Ensure enough space */
            snprintf(favored_schools + len, sizeof(favored_schools) - len, "  %sNecromancia%s (+20%%)\r\n",
                     get_weather_school_color(ch, SCHOOL_NECROMANCY), CCNRM(ch, C_NRM));
        }
        has_favored_schools = 1;
    } else if (weather->sky == SKY_CLOUDLESS) {
        len = strlen(unfavored_schools);
        if (len < sizeof(unfavored_schools) - 100) { /* Ensure enough space */
            snprintf(unfavored_schools + len, sizeof(unfavored_schools) - len, "  %sNecromancia%s (-15%%)\r\n",
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        has_unfavored_schools = 1;
    }

    if (weather->pressure > 1020) {
        len = strlen(favored_schools);
        if (len < sizeof(favored_schools) - 100) { /* Ensure enough space */
            snprintf(favored_schools + len, sizeof(favored_schools) - len, "  %sEncantamento%s (+5%%)\r\n",
                     get_weather_school_color(ch, SCHOOL_ENCHANTMENT), CCNRM(ch, C_NRM));
        }
        has_favored_schools = 1;
    } else if (weather->pressure < 980) {
        len = strlen(unfavored_schools);
        if (len < sizeof(unfavored_schools) - 100) { /* Ensure enough space */
            snprintf(unfavored_schools + len, sizeof(unfavored_schools) - len, "  %sEncantamento%s (-5%%)\r\n",
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        has_unfavored_schools = 1;
    }

    /* Check spell elements */
    if (weather->humidity < 0.3) {
        len = strlen(favored_elements);
        if (len < sizeof(favored_elements) - 100) { /* Ensure enough space */
            snprintf(favored_elements + len, sizeof(favored_elements) - len, "  %sFogo%s (+25%%)\r\n",
                     get_weather_element_color(ch, ELEMENT_FIRE), CCNRM(ch, C_NRM));
        }
        has_favored_elements = 1;
    } else if (weather->humidity > 0.7) {
        len = strlen(unfavored_elements);
        if (len < sizeof(unfavored_elements) - 100) { /* Ensure enough space */
            snprintf(unfavored_elements + len, sizeof(unfavored_elements) - len, "  %sFogo%s (-25%%)\r\n",
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        has_unfavored_elements = 1;
    }

    if (weather->humidity > 0.7) {
        len = strlen(favored_elements);
        if (len < sizeof(favored_elements) - 150) { /* Ensure enough space for longer line */
            snprintf(favored_elements + len, sizeof(favored_elements) - len, "  %sÁgua%s (+25%%), %sGelo%s (+25%%)\r\n",
                     get_weather_element_color(ch, ELEMENT_WATER), CCNRM(ch, C_NRM),
                     get_weather_element_color(ch, ELEMENT_ICE), CCNRM(ch, C_NRM));
        }
        has_favored_elements = 1;
    } else if (weather->humidity < 0.3) {
        len = strlen(unfavored_elements);
        if (len < sizeof(unfavored_elements) - 150) { /* Ensure enough space for longer line */
            snprintf(unfavored_elements + len, sizeof(unfavored_elements) - len,
                     "  %sÁgua%s (-25%%), %sGelo%s (-25%%)\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM), CCRED(ch, C_NRM),
                     CCNRM(ch, C_NRM));
        }
        has_unfavored_elements = 1;
    }

    if (weather->humidity > 0.8 && weather->sky >= SKY_RAINING) {
        len = strlen(favored_elements);
        if (len < sizeof(favored_elements) - 100) { /* Ensure enough space */
            snprintf(favored_elements + len, sizeof(favored_elements) - len, "  %sRaio%s (+30%%)\r\n",
                     get_weather_element_color(ch, ELEMENT_LIGHTNING), CCNRM(ch, C_NRM));
        }
        has_favored_elements = 1;
    } else if (weather->humidity < 0.2) {
        len = strlen(unfavored_elements);
        if (len < sizeof(unfavored_elements) - 100) { /* Ensure enough space */
            snprintf(unfavored_elements + len, sizeof(unfavored_elements) - len, "  %sRaio%s (-20%%)\r\n",
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        has_unfavored_elements = 1;
    }

    if (weather->winds > 15.0) {
        len = strlen(favored_elements);
        if (len < sizeof(favored_elements) - 100) { /* Ensure enough space */
            snprintf(favored_elements + len, sizeof(favored_elements) - len, "  %sAr%s (+20%%)\r\n",
                     get_weather_element_color(ch, ELEMENT_AIR), CCNRM(ch, C_NRM));
        }
        has_favored_elements = 1;
    } else if (weather->winds < 2.0) {
        len = strlen(unfavored_elements);
        if (len < sizeof(unfavored_elements) - 100) { /* Ensure enough space */
            snprintf(unfavored_elements + len, sizeof(unfavored_elements) - len, "  %sAr%s (-15%%)\r\n",
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
        }
        has_unfavored_elements = 1;
    }

    /* Display magical conditions if character level is high enough */
    if (GET_LEVEL(ch) >= 5 &&
        (has_favored_schools || has_unfavored_schools || has_favored_elements || has_unfavored_elements)) {
        send_to_char(ch, "\r\n%s=== Condições Mágicas ===%s\r\n", CBCYN(ch, C_NRM), CCNRM(ch, C_NRM));

        if (has_favored_schools) {
            send_to_char(ch, "%sEscolas Favorecidas:%s\r\n", CBGRN(ch, C_NRM), CCNRM(ch, C_NRM));
            send_to_char(ch, "%s", favored_schools);
        }

        if (has_unfavored_schools) {
            send_to_char(ch, "%sEscolas Desfavorecidas:%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
            send_to_char(ch, "%s", unfavored_schools);
        }

        if (has_favored_elements) {
            send_to_char(ch, "%sElementos Favorecidos:%s\r\n", CBGRN(ch, C_NRM), CCNRM(ch, C_NRM));
            send_to_char(ch, "%s", favored_elements);
        }

        if (has_unfavored_elements) {
            send_to_char(ch, "%sElementos Desfavorecidos:%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
            send_to_char(ch, "%s", unfavored_elements);
        }
    }
}

ACMD(do_weather)
{
    zone_rnum zona;
    room_rnum room_num;
    struct weather_data *weather;
    const char *sky_look[] = {"limpo", "nublado", "chuvoso", "relampejando", "nevando"};

    /* Defensive programming: validate room and zone */
    room_num = IN_ROOM(ch);
    if (room_num == NOWHERE || room_num < 0 || room_num > top_of_world) {
        send_to_char(ch, "Você não consegue sentir o clima aqui.\r\n");
        mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: do_weather - invalid room %d for %s", room_num, GET_NAME(ch));
        return;
    }

    zona = world[room_num].zone;
    if (zona < 0 || zona > top_of_zone_table) {
        send_to_char(ch, "Você não consegue sentir o clima aqui.\r\n");
        mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: do_weather - invalid zone %d for room %d", zona, room_num);
        return;
    }

    weather = zone_table[zona].weather;
    if (!weather) {
        send_to_char(ch, "O clima local está muito instável para ser percebido.\r\n");
        return;
    }

    int pressure = weather->pressure;
    int press_diff = weather->press_diff;
    int sky = weather->sky;
    int temperature = weather->temperature;
    float humidity = weather->humidity;
    float wind = weather->winds;

    const char *weather_feel;
    const char *temp_feel;

    /* Descrição baseada na pressão, umidade e vento */
    if (press_diff < 0 && humidity > 0.7 && wind > 5) {
        weather_feel = "um temporal está se formando";
    } else if (press_diff < 0) {
        weather_feel = "seu pé lhe diz que um tempo ruim se aproxima";
    } else if (wind > 8) {
        weather_feel = "há um vento forte soprando";
    } else if (humidity > 0.8) {
        weather_feel = "o ar está pesado e úmido";
    } else {
        weather_feel = "você sente uma brisa agradável";
    }

    /* Descrição baseada na temperatura */
    if (temperature < 5) {
        temp_feel = "e o frio congela seus ossos";
    } else if (temperature > 30) {
        temp_feel = "e o calor abrasador faz o suor escorrer";
    } else {
        temp_feel = "e o clima está ameno";
    }

    if (OUTSIDE(ch) || GET_LEVEL(ch) > 106) {
        /* Envia a mensagem combinando ambas as descrições */
        send_to_char(ch, " O céu está %s, %s %s.\r\n", sky_look[sky], weather_feel, temp_feel);
        /* Discutir melhor momento para exibir: Acima de level X, Remort Y ou Deuses */
        if (GET_LEVEL(ch) >= 10) {
            send_to_char(ch, " Pressão: %d hPa, Céu: %s \r\n", pressure, sky_look[sky]);
            send_to_char(ch, "Temperatura %d º.C, Umidade %.2f\r\n", temperature, humidity);
            send_to_char(ch, "Vento: %.2f m/s\r\n", wind);
        }

        /* Show mana density if character has detect magic active */
        if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
            float mana_density = calculate_mana_density(ch);
            const char *density_desc;
            int color_level;
            const char *density_color;

            /* Get description and color level from helper function */
            get_mana_density_description(mana_density, &density_desc, &color_level);

            /* Map color level to color macro */
            switch (color_level) {
                case 0:
                    density_color = CBCYN(ch, C_NRM);
                    break;
                case 1:
                    density_color = CBGRN(ch, C_NRM);
                    break;
                case 2:
                    density_color = CCYEL(ch, C_NRM);
                    break;
                case 3:
                    density_color = CCRED(ch, C_NRM);
                    break;
                case 4:
                default:
                    density_color = CBRED(ch, C_NRM);
                    break;
            }

            send_to_char(ch, "\r\n%sDensidade Mágica:%s %s%s%s (%.2f)\r\n", CBCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                         density_color, density_desc, CCNRM(ch, C_NRM), mana_density);

            /* Explain what the density means for spellcasting */
            if (mana_density >= 0.9) {
                send_to_char(ch,
                             "  %sAs energias mágicas fluem abundantemente aqui!%s\r\n"
                             "  Magias consomem menos mana e são mais poderosas.\r\n",
                             CBGRN(ch, C_NRM), CCNRM(ch, C_NRM));
            } else if (mana_density >= 0.7) {
                send_to_char(ch, "  %sAs condições são favoráveis para conjuração.%s\r\n", CCGRN(ch, C_NRM),
                             CCNRM(ch, C_NRM));
            } else if (mana_density < 0.5) {
                send_to_char(ch,
                             "  %sAs energias mágicas estão fracas neste local.%s\r\n"
                             "  Magias serão menos eficientes aqui.\r\n",
                             CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
            }
        }

        /* Show magical conditions based on weather */
        show_magical_conditions(ch, weather);
    } else {
        send_to_char(ch, " Você não tem idéia de como o tempo possa estar.\r\n");
    }
}

/* puts -'s instead of spaces */
void space_to_minus(char *str)
{
    while ((str = strchr(str, ' ')) != NULL)
        *str = '-';
}

int search_help(const char *argument, int level)
{
    int chk, bot, top, mid, minlen;
    bot = 0;
    top = top_of_helpt;
    minlen = strlen(argument);
    while (bot <= top) {
        mid = (bot + top) / 2;
        if (!(chk = strn_cmp(argument, help_table[mid].keywords, minlen))) {
            while ((mid > 0) && !strn_cmp(argument, help_table[mid - 1].keywords, minlen))
                mid--;
            while (level < help_table[mid].min_level && mid < (bot + top) / 2)
                mid++;
            if (strn_cmp(argument, help_table[mid].keywords, minlen) || level < help_table[mid].min_level)
                break;
            return (mid);
        } else if (chk > 0)
            bot = mid + 1;
        else
            top = mid - 1;
    }
    return NOWHERE;
}

ACMD(do_help)
{
    int mid = 0;
    int i, found = 0;
    if (!ch->desc)
        return;
    skip_spaces(&argument);
    if (!help_table) {
        send_to_char(ch, " Não há ajuda disponivel. \r\n ");
        return;
    }

    if (!*argument) {
        if (GET_LEVEL(ch) < LVL_IMMORT)
            page_string(ch->desc, help, 0);
        else
            page_string(ch->desc, ihelp, 0);
        return;
    }

    space_to_minus(argument);
    if ((mid = search_help(argument, GET_LEVEL(ch))) == NOWHERE) {
        send_to_char(ch, " Não encontrei ajuda para %s. \r\n", argument);
        mudlog(NRM, MIN(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "%s tried to get help on %s", GET_NAME(ch), argument);
        for (i = 0; i < top_of_helpt; i++) {
            if (help_table[i].min_level > GET_LEVEL(ch))
                continue;
            /* To help narrow down results, if they don't start with the same
               letters, move on. */
            if (*argument != *help_table[i].keywords)
                continue;
            if (levenshtein_distance(argument, help_table[i].keywords) <= 2) {
                if (!found) {
                    send_to_char(ch, " \r\nVocê queria dizer:\r\n ");
                    found = 1;
                }
                send_to_char(ch, " \t < send link = \"Help %s\">%s\t</send>\r\n", help_table[i].keywords,
                             help_table[i].keywords);
            }
        }

        return;
    }

    page_string(ch->desc, help_table[mid].entry, 0);
}

#define WHO_FORMAT "Uso: who [minlev[-maxlev]] [-n nome] [-c classe] [-k] [-l] [-n] [-q] [-r] [-s] [-z]\r\n"

/* Written by Rhade */
/* Written by Rhade */
ACMD(do_who)
{
    struct descriptor_data *d;
    struct char_data *tch;
    int i, num_can_see = 0;
    char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    char mode;
    int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0;
    int showclass = 0, short_list = 0, outlaws = 0;
    int who_room = 0, showgroup = 0, showleader = 0;

    struct {
        const char *disp;
        int min_level;
        int max_level;
        int count;
    } rank[] = {{"\tC-\tG=\tY> \tWImortais:\tn\r\n", LVL_IMMORT, LVL_IMPL, 0},
                {"\tC-\tG=\tY> \tWMortais:\tn\r\n", 11, LVL_IMMORT - 1, 0},
                {"\tC-\tG=\tY> \tWIniciantes:\tn\r\n", 1, 10, 0},
                {"\n", 0, 0, 0}};

    skip_spaces(&argument);
    strcpy(buf, argument);
    name_search[0] = '\0';

    while (*buf) {
        char arg[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];
        half_chop(buf, arg, buf1);
        if (isdigit(*arg)) {
            sscanf(arg, "%d-%d", &low, &high);
            strcpy(buf, buf1);
        } else if (*arg == '-') {
            mode = *(arg + 1);
            switch (mode) {
                case 'k':
                    outlaws = 1;
                    strcpy(buf, buf1);
                    break;
                case 'z':
                    localwho = 1;
                    strcpy(buf, buf1);
                    break;
                case 's':
                    short_list = 1;
                    strcpy(buf, buf1);
                    break;
                case 'q':
                    questwho = 1;
                    strcpy(buf, buf1);
                    break;
                case 'n':
                    half_chop(buf1, name_search, buf);
                    break;
                case 'r':
                    who_room = 1;
                    strcpy(buf, buf1);
                    break;
                case 'c':
                    half_chop(buf1, arg, buf);
                    showclass = find_class_bitvector(arg);
                    break;
                case 'l':
                    showleader = 1;
                    strcpy(buf, buf1);
                    break;
                case 'g':
                    showgroup = 1;
                    strcpy(buf, buf1);
                    break;
                default:
                    send_to_char(ch, "%s", WHO_FORMAT);
                    return;
            }
        } else {
            send_to_char(ch, "%s", WHO_FORMAT);
            return;
        }
    }

    /* Contagem dos ranks */
    for (d = descriptor_list; d && !short_list; d = d->next) {
        if (d->original)
            tch = d->original;
        else if (!(tch = d->character))
            continue;
        if (!IS_PLAYING(d))
            continue;
        if (!CAN_SEE(ch, tch))
            continue;
        if (*name_search && str_cmp(GET_NAME(tch), name_search) && !strstr(GET_TITLE(tch), name_search))
            continue;
        if (GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
            continue;
        if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
            continue;
        if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
            continue;
        if (localwho && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
            continue;
        if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
            continue;
        if (showclass && !(showclass & (1 << GET_CLASS(tch))))
            continue;
        if (showgroup && !GROUP(tch))
            continue;
        if (showleader && (!GROUP(tch) || GROUP_LEADER(GROUP(tch)) != tch))
            continue;

        for (i = 0; *rank[i].disp != '\n'; i++)
            if (GET_LEVEL(tch) >= rank[i].min_level && GET_LEVEL(tch) <= rank[i].max_level)
                rank[i].count++;
    }

    /* Impressão das listas */
    for (i = 0; *rank[i].disp != '\n'; i++) {
        if (!rank[i].count && !short_list)
            continue;

        send_to_char(ch, "%s", rank[i].disp);

        for (d = descriptor_list; d; d = d->next) {
            if (d->original)
                tch = d->original;
            else if (!(tch = d->character))
                continue;
            if (!IS_PLAYING(d))
                continue;

            if ((GET_LEVEL(tch) < rank[i].min_level || GET_LEVEL(tch) > rank[i].max_level) && !short_list)
                continue;
            if (!CAN_SEE(ch, tch))
                continue;

            num_can_see++;

            /* === BLOCO DE IMPRESSÃO COM CORES ESTILO who_tag === */

            const char *statusFlag = IS_DEAD(tch) ? "\tR+\tn" : /* morto */
                                         PLR_FLAGGED(tch, PLR_TRNS) ? "\tW*\tn"
                                                                    : /* transcendido */
                                         (GET_REMORT(tch) > 4) ? "\tG*\tn"
                                                               : " "; /* várias encarnações */

            /* define cor dos colchetes */
            const char *corNivel = (GET_LEVEL(tch) >= LVL_IMMORT) ? "\tY" : "\tW";

            /* monta o texto de encarnação */
            char enc_buf[32];
            if (GET_REMORT(tch) > 0)
                // Usando %3d para garantir 3 dígitos para o número (ex: "  1", " 10")
                snprintf(enc_buf, sizeof(enc_buf), " (%3da. Enc.)", GET_REMORT(tch));
            else
                enc_buf[0] = '\0';

            /* ----------------------------------- */
            /* --- Lógica de Alinhamento à Direita --- */
            /* ----------------------------------- */

            // Coluna fixa onde o texto da encarnação deve começar.
            // Ajuste este valor se a largura da sua tela for diferente.
            const int COLUNA_FIXA_ENC = 65;

            // 1. Calcula o comprimento do texto antes do padding.
            //    Formato inicial: cor + [LVL CLS FLG] + espaço + Nome + (espaço condicional + Título)
            //    Tamanho da parte fixa (Cor + Colchetes + LVL + CLS + Flag): ~10 caracteres visíveis.

            // **IMPORTANTE**: Este é um cálculo simplificado. Você deve contar o número de caracteres VISÍVEIS.
            // Os colchetes e as flags dão cerca de 10 caracteres fixos:
            // [ 11 CLS * ] - 10 caracteres visíveis

            int len_linha_prefixo = 10;
            len_linha_prefixo += strlen(GET_NAME(tch));

            // Adiciona o tamanho do título + o espaço condicional (se existir)
            if (*GET_TITLE(tch)) {
                len_linha_prefixo += strlen(GET_TITLE(tch));
                // len_linha_prefixo += 1; // Espaço condicional
            }

            // 2. Calcula o padding necessário.
            int padding_needed = COLUNA_FIXA_ENC - len_linha_prefixo;

            // 3. Prevenção: Se Nome+Título for muito longo, usamos 1 espaço como separador mínimo.
            if (padding_needed < 1) {
                padding_needed = 1;
            }

            // 4. Cria e preenche o buffer de padding com espaços.
            char padding_buf[COLUNA_FIXA_ENC + 1];
            // Preenche o buffer com ' ' (espaços)
            memset(padding_buf, ' ', padding_needed);
            // Garante que o buffer seja uma string terminada em NULL
            padding_buf[padding_needed] = '\0';

            /* imprime a linha */
            // CORRIGIDO: O formato agora inclui um %s extra para o padding_buf
            send_to_char(ch, "%s[%3d \tC%-3s\tn%s%s] \tB%s%s\tW%s%s%s\tn\r\n",
                         corNivel,                       // %s (1) Cor do colchete/nível
                         GET_LEVEL(tch),                 // %-3d (2) Nível
                         CLASS_ABBR(tch),                // \tC%-3s (3) Classe
                         statusFlag,                     // %s (4) Flag
                         corNivel,                       // %s (5) Cor para fechar o colchete
                         GET_NAME(tch),                  // \tB%s (6) Nome
                         (*GET_TITLE(tch) ? " " : ""),   // %s (7) ESPAÇO CONDICIONAL
                         GET_TITLE(tch),                 // \tW%s (8) Título
                         padding_buf,                    // %s (9) **PADDING DE ALINHAMENTO**
                         enc_buf                         // %s (10) Encarnação
            );
        }

        send_to_char(ch, "\r\n");
    }

    /* Rodapé */
    if (!num_can_see)
        send_to_char(ch, "\tWNinguém!\tn\r\n");
    else if (num_can_see == 1)
        send_to_char(ch, "\tGApenas um personagem solitário!\tn\r\n");
    else
        send_to_char(ch, "\tGVocê pode ver %d\tG %s.\tn\r\n", num_can_see,
                     PLURAL(num_can_see, "personagem", "personagens"));

    if (IS_HAPPYHOUR > 0)
        send_to_char(ch, "\tWÉ Happy Hour! Digite \tRhappyhour\tW para ver os bônus atuais.\tn\r\n");
}
#define USERS_FORMAT "formato: users [-l minlevel[-maxlevel]] [-n nome] [-h host] [-c classe] [-o] [-p]\r\n"

ACMD(do_users)
{
    char line[200], line2[220], idletime[10], classname[20];
    char state[30], timestr[9], mode;
    char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
    struct char_data *tch;
    struct descriptor_data *d;
    int low = 0, high = LVL_IMPL, num_can_see = 0;
    int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;
    char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
    host_search[0] = name_search[0] = '\0';
    strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
    while (*buf) {
        char buf1[MAX_INPUT_LENGTH];
        half_chop(buf, arg, buf1);
        if (*arg == '-') {
            mode = *(arg + 1); /* just in case; we destroy arg in the switch */
            switch (mode) {
                case 'o':
                case 'k':
                    outlaws = 1;
                    playing = 1;
                    strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
                    break;
                case 'p':
                    playing = 1;
                    strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
                    break;
                case 'd':
                    deadweight = 1;
                    strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
                    break;
                case 'l':
                    playing = 1;
                    half_chop(buf1, arg, buf);
                    sscanf(arg, "%d-%d", &low, &high);
                    break;
                case 'n':
                    playing = 1;
                    half_chop(buf1, name_search, buf);
                    break;
                case 'h':
                    playing = 1;
                    half_chop(buf1, host_search, buf);
                    break;
                case 'c':
                    playing = 1;
                    half_chop(buf1, arg, buf);
                    showclass = find_class_bitvector(arg);
                    break;
                default:
                    send_to_char(ch, "%s", USERS_FORMAT);
                    return;
            } /* end of switch */

        } else { /* endif */
            send_to_char(ch, "%s", USERS_FORMAT);
            return;
        }
    } /* end while (parser) */
    send_to_char(ch,
                 "Num Classe  Nome         Status         Idl   Login\t*   Site\r\n"
                 "--- ------- ------------ -------------- ----- -------- ------------------------\r\n");
    one_argument(argument, arg);
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING && playing)
            continue;
        if (STATE(d) == CON_PLAYING && deadweight)
            continue;
        if (IS_PLAYING(d)) {
            if (d->original)
                tch = d->original;
            else if (!(tch = d->character))
                continue;
            if (*host_search && !strstr(d->host, host_search))
                continue;
            if (*name_search && str_cmp(GET_NAME(tch), name_search))
                continue;
            if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
                continue;
            if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
                continue;
            if (showclass && !(showclass & (1 << GET_CLASS(tch))))
                continue;
            if (GET_INVIS_LEV(tch) > GET_LEVEL(ch))
                continue;
            if (d->original)
                sprintf(classname, "[%2d %s]", GET_LEVEL(d->original), CLASS_ABBR(d->original));
            else
                sprintf(classname, "[%2d %s]", GET_LEVEL(d->character), CLASS_ABBR(d->character));
        } else
            strcpy(classname, "   -   ");
        strftime(timestr, sizeof(timestr), "%H:%M:%S", localtime(&(d->login_time)));
        if (STATE(d) == CON_PLAYING && d->original)
            strcpy(state, "Trocado");
        else
            strcpy(state, connected_types[STATE(d)]);
        if (d->character && STATE(d) == CON_PLAYING)
            sprintf(idletime, "%5d", d->character->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
        else
            strcpy(idletime, "     ");
        sprintf(line, "%3d %-7s %-12s %-14s %-3s %-8s ", d->desc_num, classname,
                d->original && d->original->player.name     ? d->original->player.name
                : d->character && d->character->player.name ? d->character->player.name
                                                            : "INDEFINIDO",
                state, idletime, timestr);
        if (*d->host)
            sprintf(line + strlen(line), "[%s]\r\n", d->host);
        else
            strcat(line, "[Hostname desconhecido]\r\n");
        if (STATE(d) != CON_PLAYING) {
            sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
            strcpy(line, line2);
        }
        if (STATE(d) != CON_PLAYING || (STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
            send_to_char(ch, "%s", line);
            num_can_see++;
        }
    }

    send_to_char(ch, "\r\n%d conexões visiveis.\r\n", num_can_see);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
    if (IS_NPC(ch)) {
        send_to_char(ch, "Indisponivel para mobs!\r\n");
        return;
    }

    switch (subcmd) {
        case SCMD_CREDITS:
            page_string(ch->desc, credits, 0);
            break;
        case SCMD_NEWS:
            GET_LAST_NEWS(ch) = time(0);
            page_string(ch->desc, news, 0);
            break;
        case SCMD_INFO:
            page_string(ch->desc, info, 0);
            break;
        case SCMD_WIZLIST:
            page_string(ch->desc, wizlist, 0);
            break;
        case SCMD_IMMLIST:
            page_string(ch->desc, immlist, 0);
            break;
        case SCMD_HANDBOOK:
            page_string(ch->desc, handbook, 0);
            break;
        case SCMD_POLICIES:
            page_string(ch->desc, policies, 0);
            break;
        case SCMD_CLANPOLICIES:
            page_string(ch->desc, clanpolicies, 0);
            break;
        case SCMD_MOTD:
            GET_LAST_MOTD(ch) = time(0);
            page_string(ch->desc, motd, 0);
            break;
        case SCMD_IMOTD:
            page_string(ch->desc, imotd, 0);
            break;
        case SCMD_CLEAR:
            send_to_char(ch, "\033[H\033[J");
            break;
        case SCMD_VERSION:
            send_to_char(ch, "%s\r\n", tbamud_version);
            break;
        case SCMD_WHOAMI:
            send_to_char(ch, "%s\r\n", GET_NAME(ch));
            break;
        default:
            log1("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
            /* SYSERR_DESC: General page string function for such things as
               'credits', 'news', 'wizlist', 'clear', 'version'.  This occurs when
               a call is made to this routine that is not one of the predefined
               calls.  To correct it, either a case needs to be added into the
               function to account for the subcmd that is being passed to it, or
               the call to the function needs to have the correct subcmd put into
               place. */
            return;
    }
}

static void perform_mortal_where(struct char_data *ch, char *arg)
{
    struct char_data *i;
    struct descriptor_data *d;
    int j;
    if (!*arg) {
        j = world[(IN_ROOM(ch))].zone;
        send_to_char(ch, "Jogadores em %s\tn.\r\n--------------------\r\n", zone_table[j].name);
        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) != CON_PLAYING || d->character == ch)
                continue;
            if ((i = (d->original ? d->original : d->character)) == NULL)
                continue;
            if (IN_ROOM(i) == NOWHERE || !CAN_SEE(ch, i))
                continue;
            if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)
                continue;
            send_to_char(ch, "%-20s%s - %s%s\r\n", GET_NAME(i), QNRM, world[IN_ROOM(i)].name, QNRM);
        }
    } else { /* print only FIRST char, not all. */
        for (i = character_list; i; i = i->next) {
            if (IN_ROOM(i) == NOWHERE || i == ch)
                continue;
            if (!CAN_SEE(ch, i) || world[IN_ROOM(i)].zone != world[IN_ROOM(ch)].zone)
                continue;
            if (!isname(arg, i->player.name))
                continue;
            send_to_char(ch, "%-25s%s - %s%s\r\n", GET_NAME(i), QNRM, world[IN_ROOM(i)].name, QNRM);
            return;
        }
        send_to_char(ch, "Ninguem com este nome.\r\n");
    }
}

static size_t print_object_location(const int num, const obj_data *obj, const char_data *ch,   // NOLINT(*-no-recursion)
                                    char *buf, size_t len, const size_t buf_size, const int recur)
{
    size_t nlen = 0;

    if (num > 0)
        nlen = snprintf(buf + len, buf_size - len, "O%4d. %-25s%s - ", num, obj->short_description, QNRM);
    else
        nlen = snprintf(buf + len, buf_size - len, "%37s", " - ");

    len += nlen;
    nlen = 0;
    if (len > buf_size)
        return len;   // let the caller know we overflowed

    if (SCRIPT(obj)) {
        if (!TRIGGERS(SCRIPT(obj))->next)
            nlen = snprintf(buf + len, buf_size - len, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(obj))));
        else
            nlen = snprintf(buf + len, buf_size - len, "[TRIGS] ");
    }

    len += nlen;
    if (len > buf_size)
        return len;   // let the caller know we overflowed

    if (IN_ROOM(obj) != NOWHERE)
        nlen = snprintf(buf + len, buf_size - len, "[%5d] %s%s\r\n", GET_ROOM_VNUM(IN_ROOM(obj)),
                        world[IN_ROOM(obj)].name, QNRM);
    else if (obj->carried_by) {
        if (PRF_FLAGGED(ch, PRF_SHOWVNUMS))
            nlen = snprintf(buf + len, buf_size - len, "carried by [%5d] %s%s\r\n", GET_MOB_VNUM(obj->carried_by),
                            PERS(obj->carried_by, ch), QNRM);
        else
            nlen = snprintf(buf + len, buf_size - len, "carried by %s%s\r\n", PERS(obj->carried_by, ch), QNRM);
        if (PRF_FLAGGED(ch, PRF_VERBOSE) && IN_ROOM(obj->carried_by) != NOWHERE && len + nlen < buf_size)
            nlen += snprintf(buf + len + nlen, buf_size - len - nlen, "%37sin [%5d] %s%s\r\n", " - ",
                             GET_ROOM_VNUM(IN_ROOM(obj->carried_by)), world[IN_ROOM(obj->carried_by)].name, QNRM);
    } else if (obj->worn_by) {
        if (PRF_FLAGGED(ch, PRF_SHOWVNUMS))
            nlen = snprintf(buf + len, buf_size - len, "worn by [%5d] %s%s\r\n", GET_MOB_VNUM(obj->worn_by),
                            PERS(obj->worn_by, ch), QNRM);
        else
            nlen = snprintf(buf + len, buf_size - len, "worn by %s%s\r\n", PERS(obj->worn_by, ch), QNRM);
        if (PRF_FLAGGED(ch, PRF_VERBOSE) && IN_ROOM(obj->worn_by) != NOWHERE && len + nlen < buf_size)
            nlen += snprintf(buf + len + nlen, buf_size - len - nlen, "%37sin [%5d] %s%s\r\n", " - ",
                             GET_ROOM_VNUM(IN_ROOM(obj->worn_by)), world[IN_ROOM(obj->worn_by)].name, QNRM);
    } else if (obj->in_obj) {
        nlen = snprintf(buf + len, buf_size - len, "inside %s%s%s\r\n", obj->in_obj->short_description, QNRM,
                        (recur ? ", which is" : " "));
        if (recur && nlen + len < buf_size) {
            len += nlen;
            nlen = 0;
            len = print_object_location(0, obj->in_obj, ch, buf, len, buf_size, recur);
        }
    } else
        nlen = snprintf(buf + len, buf_size - len, "in an unknown location\r\n");
    len += nlen;
    return len;
}

static void perform_immort_where(char_data *ch, const char *arg)
{
    char_data *i;
    obj_data *k;
    struct descriptor_data *d;
    int num = 0,
        found = FALSE;   // "num" here needs to match the lookup in do_stat, so "stat 4.sword" finds the right one
    const char *error_message = "\r\n***OVERFLOW***\r\n";
    char buf[MAX_STRING_LENGTH];
    size_t len = 0, nlen = 0;
    const size_t buf_size = sizeof(buf) - strlen(error_message) - 1;

    if (!*arg) {
        send_to_char(ch, "Players  Room    Location                       Zone\r\n");
        send_to_char(ch, "-------- ------- ------------------------------ -------------------\r\n");
        for (d = descriptor_list; d; d = d->next)
            if (IS_PLAYING(d)) {
                i = (d->original ? d->original : d->character);
                if (i && CAN_SEE(ch, i) && (IN_ROOM(i) != NOWHERE)) {
                    if (d->original)
                        send_to_char(ch, "%-8s%s - [%5d] %s%s (in %s%s)\r\n", GET_NAME(i), QNRM,
                                     GET_ROOM_VNUM(IN_ROOM(d->character)), world[IN_ROOM(d->character)].name, QNRM,
                                     GET_NAME(d->character), QNRM);
                    else
                        send_to_char(ch, "%-8s%s %s[%s%5d%s]%s %-*s%s %s%s\r\n", GET_NAME(i), QNRM, QCYN, QYEL,
                                     GET_ROOM_VNUM(IN_ROOM(i)), QCYN, QNRM,
                                     30 + count_color_chars(world[IN_ROOM(i)].name), world[IN_ROOM(i)].name, QNRM,
                                     zone_table[(world[IN_ROOM(i)].zone)].name, QNRM);
                }
            }
    } else {
        if (PRF_FLAGGED(ch, PRF_VERBOSE))
            len = snprintf(buf, buf_size, "   ### Mob name                   - Room #  Room name\r\n");

        for (i = character_list; i; i = i->next)
            if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE && isname(arg, i->player.name)) {
                found = 1;
                nlen = snprintf(buf + len, buf_size - len, "M%4d. %-25s%s - [%5d] %-25s%s", ++num, GET_NAME(i), QNRM,
                                GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name, QNRM);
                if (len + nlen >= buf_size) {
                    len += snprintf(buf + len, buf_size - len, "%s", error_message);
                    break;
                }
                len += nlen;
                if (SCRIPT(i) && TRIGGERS(SCRIPT(i))) {
                    if (!TRIGGERS(SCRIPT(i))->next)
                        nlen = snprintf(buf + len, buf_size - len, "[T%d]", GET_TRIG_VNUM(TRIGGERS(SCRIPT(i))));
                    else
                        nlen = snprintf(buf + len, buf_size - len, "[TRIGS]");

                    if (len + nlen >= buf_size) {
                        snprintf(buf + len, buf_size - len, "%s", error_message);
                        break;
                    }
                    len += nlen;
                }
                nlen = snprintf(buf + len, buf_size - len, "%s\r\n", QNRM);
                if (len + nlen >= buf_size) {
                    snprintf(buf + len, buf_size - len, "%s", error_message);
                    break;
                }
                len += nlen;
            }

        if (PRF_FLAGGED(ch, PRF_VERBOSE) && len < buf_size) {
            nlen = snprintf(buf + len, buf_size - len, "  ###  Object name                 Location\r\n");
            if (len + nlen >= buf_size) {
                snprintf(buf + len, buf_size - len, "%s", error_message);
            }
            len += nlen;
        }

        if (len < buf_size) {
            for (k = object_list; k; k = k->next) {
                if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
                    found = 1;
                    len = print_object_location(++num, k, ch, buf, len, buf_size, TRUE);
                    if (len >= buf_size) {
                        snprintf(buf + buf_size, sizeof(buf) - buf_size, "%s", error_message);
                        break;
                    }
                }
            }
        }

        if (!found)
            send_to_char(ch, "Couldn't find any such thing.\r\n");
        else
            page_string(ch->desc, buf, TRUE);
    }
}

ACMD(do_where)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        perform_immort_where(ch, arg);
    else
        perform_mortal_where(ch, arg);
}

ACMD(do_levels)
{
    char buf[MAX_STRING_LENGTH], arg[MAX_STRING_LENGTH];
    size_t len = 0, nlen;
    int i, ret, min_lev = 1, max_lev = LVL_IMMORT, val;
    if (IS_NPC(ch)) {
        send_to_char(ch, "Você não pode fazer isto.\r\n");
        return;
    }
    one_argument(argument, arg);
    if (*arg) {
        if (isdigit(*arg)) {
            ret = sscanf(arg, "%d-%d", &min_lev, &max_lev);
            if (ret == 0) {
                /* No valid args found */
                min_lev = 1;
                max_lev = LVL_IMMORT;
            } else if (ret == 1) {
                /* One arg = range is (num) either side of current level */
                val = min_lev;
                max_lev = MIN(GET_LEVEL(ch) + val, LVL_IMMORT);
                min_lev = MAX(GET_LEVEL(ch) - val, 1);
            } else if (ret == 2) {
                /* Two args = min-max range limit - just do sanity checks */
                min_lev = MAX(min_lev, 1);
                max_lev = MIN(max_lev + 1, LVL_IMMORT);
            }
        } else {
            send_to_char(ch, "Uso: %slevels [<min>-<max> | <limite>]%s\r\n\r\n", QYEL, QNRM);
            send_to_char(ch, "Mostra a exp por nivel.\r\n");
            send_to_char(ch, "%slevels       %s- mostra para os niveis (1-%d)\r\n", QCYN, QNRM, (LVL_IMMORT - 1));
            send_to_char(ch, "%slevels 5     %s- mostra 5 niveis para cima e para baixo\r\n", QCYN, QNRM);
            send_to_char(ch, "%slevels 10-25 %s- mostra do nivel 10 ao nivel 25\r\n", QCYN, QNRM);
            return;
        }
    }

    for (i = min_lev; i < max_lev; i++) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d-%-8d : ", (int)i, level_exp(GET_CLASS(ch), i),
                        level_exp(GET_CLASS(ch), i + 1) - 1);
        if (len + nlen >= sizeof(buf))
            break;
        len += nlen;
        switch (GET_SEX(ch)) {
            case SEX_MALE:
            case SEX_NEUTRAL:
                nlen = snprintf(buf + len, sizeof(buf) - len, "%s\r\n", title_male(GET_CLASS(ch), i));
                break;
            case SEX_FEMALE:
                nlen = snprintf(buf + len, sizeof(buf) - len, "%s\r\n", title_female(GET_CLASS(ch), i));
                break;
            default:
                nlen = snprintf(buf + len, sizeof(buf) - len, "Nao sei dizer o titulo.\r\n");
                break;
        }
        if (len + nlen >= sizeof(buf))
            break;
        len += nlen;
    }

    if (len < sizeof(buf) && max_lev == LVL_IMMORT)
        snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d          : Imortalidade\r\n", LVL_IMMORT,
                 level_exp(GET_CLASS(ch), LVL_IMMORT));
    page_string(ch->desc, buf, TRUE);
}

ACMD(do_consider)
{
    char buf[MAX_INPUT_LENGTH];
    struct char_data *victim;
    int diff;
    one_argument(argument, buf);
    if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Considerar matar quem?\r\n");
        return;
    }
    if (victim == ch) {
        send_to_char(ch, "Fácil! Realmente muito fácil!\r\n");
        return;
    }
    if (!IS_NPC(victim)) {
        send_to_char(ch, "Você gostaria que eu pedisse emprestado uma cruz e uma pá?\r\n");
        return;
    }
    diff = (GET_LEVEL(victim) - GET_LEVEL(ch));
    if (diff <= -10)
        send_to_char(ch, "Mas... e agora? Para onde este franguinho vai?\r\n");
    else if (diff <= -5)
        send_to_char(ch, "Você pode matar com um dedo!\r\n");
    else if (diff <= -2)
        send_to_char(ch, "Será uma luta fácil.\r\n");
    else if (diff <= -1)
        send_to_char(ch, "Honestamente fácil.\r\n");
    else if (diff == 0)
        send_to_char(ch, "A partida perfeita!\r\n");
    else if (diff <= 1)
        send_to_char(ch, "You would need some luck!\r\n");
    else if (diff <= 2)
        send_to_char(ch, "You would need a lot of luck!\r\n");
    else if (diff <= 3)
        send_to_char(ch, "You would need a lot of luck and great equipment!\r\n");
    else if (diff <= 5)
        send_to_char(ch, "Do you feel lucky, punk?\r\n");
    else if (diff <= 10)
        send_to_char(ch, "Are you mad!?\r\n");
    else if (diff <= 100)
        send_to_char(ch, "You ARE mad!\r\n");
}

ACMD(do_diagnose)
{
    char buf[MAX_INPUT_LENGTH];
    struct char_data *vict;
    one_argument(argument, buf);
    if (*buf) {
        if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
            send_to_char(ch, "%s", CONFIG_NOPERSON);
        else
            diag_char_to_char(vict, ch);
    } else {
        if (FIGHTING(ch))
            diag_char_to_char(FIGHTING(ch), ch);
        else
            send_to_char(ch, "Diagnosticar quem?\r\n");
    }
}

ACMD(do_toggle)
{
    char buf2[4], arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int toggle, tp, wimp_lev, result = 0, len = 0, i;
    const char *types[] = {"desligado", "breve", "normal", "ligado", "\n"};
    const struct {
        char *command;
        bitvector_t toggle; /* this needs changing once hashmaps are
                                               implemented */
        char min_level;
        char *disable_msg;
        char *enable_msg;
    } tog_messages[] = {
        {"nosummon", PRF_SUMMONABLE, 0, "Você agora não poderá ser convocado por outros jogadores.\r\n",
         "Você agora pode ser convocado por outros jogadores.\r\n"},
        {"nohassle", PRF_NOHASSLE, LVL_IMMORT, "Nohassle desabilitado.\r\n", "Nohassle habilitado.\r\n"},
        {"brief", PRF_BRIEF, 0, "Modo 'Breve' desligado.\r\n", "Modo 'Breve' ligado.\r\n"},
        {"compact", PRF_COMPACT, 0, "Modo 'Compacto' desligado.\r\n", "Modo 'Compacto' ligado.\r\n"},
        {"notell", PRF_NOTELL, 0, "Canal TELL ativado.\r\n", "Canal TELL desativado.\r\n"},
        {"noauction", PRF_NOAUCT, 0, "Canal AUCTION ativado.\r\n\r\n", "Canal AUCTION desativado.\r\n"},
        {"noshout", PRF_NOSHOUT, 0, "Canal SHOUT ativado.\r\n\r\n", "Canal SHOUT desativado.\r\n"},
        {"nogossip", PRF_NOGOSS, 0, "Canal GOSS ativado.\r\n", "Canal GOSS desativado.\r\n"},
        {"nograts", PRF_NOGRATZ, 0, "Canal GRATZ ativado.\r\n", "Canal GRATZ desativado.\r\n"},
        {"nowiz", PRF_NOWIZ, LVL_IMMORT, "Canal WIZ ativado.\r\n", "Canal WIZ desativado.\r\n"},
        {"quest", PRF_QUEST, 0, "Você não faz mais parte do grupo de busca (quest)!\r\n",
         "Ok, agora você faz parte do grupo de busca (quest)!\r\n"},
        {"showvnums", PRF_SHOWVNUMS, LVL_IMMORT, "Você não irá mais ver as flags das salas.\r\n",
         "Agora você irá ver as flags das salas.\r\n"},
        {"norepeat", PRF_NOREPEAT, 0, "Agora você verá suas frases repetidas.\r\n",
         "Você não irá mais ver suas frases repetidas.\r\n"},
        {"holylight", PRF_HOLYLIGHT, LVL_IMMORT, "Modo 'HolyLight' desligado.\r\n", "Modo 'HolyLight' ligado.\r\n"},
        {"slownameserver", 0, LVL_IMPL, "Modo 'SlowNS' desligado. Endereços IP serão resolvidos.\r\n",
         "Modo 'SlowNS' ligado. Endereços IP não serão resolvidos.\r\n"},
        {"autoexits", PRF_AUTOEXIT, 0, "AutoExits desligado.\r\n", "AutoExits ligado.\r\n"},
        {"trackthru", 0, LVL_IMPL, "Não será mais possível rastrear através de portas.\r\n",
         "Agora será possível rastrear através de portas.\r\n"},
        {"clsolc", PRF_CLS, LVL_BUILDER, "A tela não vai ser limpa no OLC.\r\n", "A tela vai ser limpa no OLC.\r\n"},
        {"buildwalk", PRF_BUILDWALK, LVL_BUILDER, "Modo 'Construtor' desligado.\r\n", "Modo 'Construtor' ligado.\r\n"},
        {"away", PRF_AFK, 0, "AWAY desligado.\r\n", "AWAY ligado"},
        {"autoloot", PRF_AUTOLOOT, 0, "Autoloot desligado.\r\n", "Autoloot ligado.\r\n"},
        {"autogold", PRF_AUTOGOLD, 0, "Autogold desligado.\r\n", "Autogold ligado.\r\n"},
        {"autosplit", PRF_AUTOSPLIT, 0, "Autosplit desligado.\r\n", "Autosplit ligado.\r\n"},
        {"autosac", PRF_AUTOSAC, 0, "Autosac desligado.\r\n", "Autosac ligado.\r\n"},
        {"autoexam", PRF_AUTOEXAM, 0, "Auto examinar desligado.\r\n", "Auto examinar ligado.\r\n"},
        {"autoassist", PRF_AUTOASSIST, 0, "Autoassist desligado.\r\n", "Autoassist ligado.\r\n"},
        {"automap", PRF_AUTOMAP, 1, "Agora, você não irá mais ver o  mini-mapa.\r\n",
         "Agora, você irá ver o  mini-mapa.\r\n"},
        {"autokey", PRF_AUTOKEY, 0, "Você agora precisa destrancar as portas manualmente.\r\n",
         "Você agora vai destrancar as portas automaticamente ao abrir (se tiver a chave).\r\n"},
        {"autodoor", PRF_AUTODOOR, 0, "Você agora precisa especificar uma direção ao abrir, fechar e destrancar.\r\n",
         "Você agora vai encontrar a próxima porta disponível ao abrir, fechar ou destrancar.\r\n"},
        {"zoneresets", PRF_ZONERESETS, LVL_IMPL, "Você não irá mais ver os resets de áreas.\r\n",
         "Você irá mais ver os resets de áreas.\r\n"},
        {"syslog", 0, LVL_IMMORT, "\n", "\n"},
        {"wimpy", 0, 0, "\n", "\n"},
        {"pagelength", 0, 0, "\n", "\n"},
        {"screenwidth", 0, 0, "\n", "\n"},
        {"color", 0, 0, "\n", "\n"},
        {"hitbar", PRF_HITBAR, 0, "Você não verá mais a saúde do oponente durante a luta.\r\n",
         "Agora você verá a saúde do oponente durante a luta.\r\n"},
        {"autotitle", PRF_AUTOTITLE, 0, "Seu título não será mais alterado automaticamente.\r\n",
         "Seu título será alterado automaticamente sempre que evoluir um nível.\r\n"},
        {"\n", 0, -1, "\n", "\n"} /* must be last */
    };
    if (IS_NPC(ch))
        return;
    argument = one_argument(argument, arg);
    any_one_arg(argument, arg2); /* so that we don't skip 'on' */
    if (!*arg) {
        if (!GET_WIMP_LEV(ch))
            strcpy(buf2, "DES"); /* strcpy: OK */
        else
            snprintf(buf2, sizeof(buf2), "%-3.3d",
                     GET_WIMP_LEV(ch)); /* thanks to Ironfist for the fix for the buffer overrun here */
        if (GET_LEVEL(ch) == LVL_IMPL) {
            send_to_char(ch,
                         " SlowNameserver: %-3s   "
                         "                        "
                         " Trackthru Doors: %-3s\r\n",
                         ONOFF(CONFIG_NS_IS_SLOW), ONOFF(CONFIG_TRACK_T_DOORS));
        }

        send_to_char(ch,
                     "      Exibir Hp: %-3s    "
                     "      Modo Breve: %-3s    "
                     "     Protec. Summon: %-3s\r\n"
                     "      Exibir Mv: %-3s    "
                     "       Modo Compacto: %-3s    "
                     "          Quest: %-3s\r\n"
                     "   Exibir Mn: %-3s    "
                     "         NoTell: %-3s    "
                     "   Repetir Com.: %-3s\r\n"
                     "      Auto Exits: %-3s    "
                     "        NoShout: %-3s    "
                     "        Nível de fuga: %-3s\r\n"
                     "       NoGossip: %-3s    "
                     "      NoAuction: %-3s    "
                     "        NoGrats: %-3s\r\n"
                     "       AutoLoot: %-3s    "
                     "       AutoGold: %-3s    "
                     "      AutoSplit: %-3s\r\n"
                     "        AutoSac: %-3s    "
                     "     AutoAssist: %-3s    "
                     "        AutoMap: %-3s\r\n"
                     "     Linhas: %-3d    "
                     "    Colunas: %-3d    "
                     "           AWAY: %-3s\r\n"
                     "        Autokey: %-3s    "
                     "       Autodoor: %-3s    "
                     "   Nível de Cor: %s     \r\n "
                     "     Hitbar: %-3s   "
                     "     Autotitle: %-3s   \r\n",
                     ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)), ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
                     ONOFF(PRF_FLAGGED(ch, PRF_SUMMONABLE)), ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
                     ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)), ONOFF(PRF_FLAGGED(ch, PRF_QUEST)),
                     ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)), ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
                     ONOFF(PRF_FLAGGED(ch, PRF_NOREPEAT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
                     ONOFF(PRF_FLAGGED(ch, PRF_NOSHOUT)), buf2, ONOFF(PRF_FLAGGED(ch, PRF_NOGOSS)),
                     ONOFF(PRF_FLAGGED(ch, PRF_NOAUCT)), ONOFF(PRF_FLAGGED(ch, PRF_NOGRATZ)),
                     ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
                     ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)),
                     ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOMAP)), GET_PAGE_LENGTH(ch),
                     GET_SCREEN_WIDTH(ch), ONOFF(PRF_FLAGGED(ch, PRF_AFK)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOKEY)),
                     ONOFF(PRF_FLAGGED(ch, PRF_AUTODOOR)), types[COLOR_LEV(ch)], ONOFF(PRF_FLAGGED(ch, PRF_HITBAR)),
                     ONOFF(PRF_FLAGGED(ch, PRF_AUTOTITLE)));
        return;
    }

    len = strlen(arg);
    for (toggle = 0; *tog_messages[toggle].command != '\n'; toggle++)
        if (!strncmp(arg, tog_messages[toggle].command, len))
            break;
    if (*tog_messages[toggle].command == '\n' || tog_messages[toggle].min_level > GET_LEVEL(ch)) {
        send_to_char(ch, "Você não pode alterar isto!\r\n");
        return;
    }

    switch (toggle) {
        case SCMD_COLOR:
            if (!*arg2) {
                send_to_char(ch, "Seu nível de cores agora está %s.\r\n", types[COLOR_LEV(ch)]);
                return;
            }

            if (((tp = search_block(arg2, types, FALSE)) == -1)) {
                send_to_char(ch, "Uso: toggle color { desligado | breve | normal | ligado }\r\n");
                return;
            }
            REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
            REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
            if (tp & 1)
                SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
            if (tp & 2)
                SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);

            send_to_char(ch, "Sua %scor%s agora está %s.\r\n", CCRED(ch, C_SPR), CCNRM(ch, C_OFF), types[tp]);
            return;
        case SCMD_SYSLOG:
            if (!*arg2) {
                send_to_char(ch, "Seu syslog atualmente está %s.\r\n",
                             types[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
                return;
            }
            if (((tp = search_block(arg2, types, FALSE)) == -1)) {
                send_to_char(ch, "Uso: toggle syslog { desligado | breve | normal | ligado }\r\n");
                return;
            }
            REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
            REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
            if (tp & 1)
                SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
            if (tp & 2)
                SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);

            send_to_char(ch, "Seu syslog agora está %s.\r\n", types[tp]);
            return;
        case SCMD_SLOWNS:
            result = (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW);
            break;
        case SCMD_TRACK:
            result = (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS);
            break;
        case SCMD_BUILDWALK:
            if (GET_LEVEL(ch) < LVL_BUILDER) {
                send_to_char(ch, "Apenas para construtores, desculpe.\r\n");
                return;
            }
            result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
            if (PRF_FLAGGED(ch, PRF_BUILDWALK)) {
                for (i = 0; *arg2 && *(sector_types[i]) != '\n'; i++)
                    if (is_abbrev(arg2, sector_types[i]))
                        break;
                if (*(sector_types[i]) == '\n')
                    i = 0;
                GET_BUILDWALK_SECTOR(ch) = i;
                send_to_char(ch, "Tipo de setor padrão é %s\r\n", sector_types[i]);
                mudlog(CMP, GET_LEVEL(ch), TRUE, "OLC: %s ligou buildwalk. Zona permitida %d", GET_NAME(ch),
                       GET_OLC_ZONE(ch));
            } else
                mudlog(CMP, GET_LEVEL(ch), TRUE, "OLC: %s desligou buildwalk. Zona permitida %d", GET_NAME(ch),
                       GET_OLC_ZONE(ch));
            break;
        case SCMD_AFK:
            if ((result = PRF_TOG_CHK(ch, PRF_AFK)))
                act("$n agora está ausente do teclado.", TRUE, ch, 0, 0, TO_ROOM);
            else {
                act("$n retornou ao teclado.", TRUE, ch, 0, 0, TO_ROOM);
                if (has_mail(GET_IDNUM(ch)))
                    send_to_char(ch, "Você tem correio esperando.\r\n");
            }
            break;
        case SCMD_WIMPY:
            if (!*arg2) {
                if (GET_WIMP_LEV(ch)) {
                    send_to_char(ch, "Seu nível de fuga atual é %d pontos de vida.\r\n", GET_WIMP_LEV(ch));
                    return;
                } else {
                    send_to_char(ch, "No momento, você não é covarde. (certeza, certeza...)\r\n");
                    return;
                }
            }
            if (isdigit(*arg2)) {
                if ((wimp_lev = atoi(arg2)) != 0) {
                    if (wimp_lev < 0)
                        send_to_char(ch, "Heh, heh, heh.. estamos engraçados hoje, eh?\r\n");
                    else if (wimp_lev > GET_MAX_HIT(ch))
                        send_to_char(ch, "Isso não faz muito sentido, não é?\r\n");
                    else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
                        send_to_char(
                            ch, "Você não pode definir seu nível de fuga acima da metade dos seus pontos de vida.\r\n");
                    else {
                        send_to_char(ch, "Ok, você fugirá se cair abaixo de %d pontos de vida.", wimp_lev);
                        GET_WIMP_LEV(ch) = wimp_lev;
                    }
                } else {
                    send_to_char(ch, "Ok, agora você lutará até o fim amargo.");
                    GET_WIMP_LEV(ch) = 0;
                }
            } else
                send_to_char(ch, "Especifique com quantos pontos de vida você quer fugir. (0 para desabilitar)\r\n");
            break;
        case SCMD_PAGELENGTH:
            if (!*arg2)
                send_to_char(ch, "Seu comprimento de página atual está definido para %d linhas.", GET_PAGE_LENGTH(ch));
            else if (is_number(arg2)) {
                GET_PAGE_LENGTH(ch) = MIN(MAX(atoi(arg2), 5), 255);
                send_to_char(ch, "Ok, seu comprimento de página agora está definido para %d linhas.",
                             GET_PAGE_LENGTH(ch));
            } else
                send_to_char(ch, "Por favor, especifique um número de linhas (5 - 255).");
            break;
        case SCMD_SCREENWIDTH:
            if (!*arg2)
                send_to_char(ch, "Sua largura de tela atual está definida para %d caracteres.", GET_SCREEN_WIDTH(ch));
            else if (is_number(arg2)) {
                GET_SCREEN_WIDTH(ch) = MIN(MAX(atoi(arg2), 40), 200);
                send_to_char(ch, "Ok, sua largura de tela agora está definida para %d caracteres.",
                             GET_SCREEN_WIDTH(ch));
            } else
                send_to_char(ch, "Por favor, especifique um número de caracteres (40 - 200).");
            break;
        case SCMD_AUTOMAP:
            if (can_see_map(ch)) {
                if (!*arg2) {
                    TOGGLE_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
                    result = (PRF_FLAGGED(ch, tog_messages[toggle].toggle));
                } else if (!strcmp(arg2, "on")) {
                    SET_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
                    result = 1;
                } else if (!strcmp(arg2, "off")) {
                    REMOVE_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
                } else {
                    send_to_char(ch, "Valor para %s deve ser 'on' ou 'off'.\r\n", tog_messages[toggle].command);
                    return;
                }
            } else
                send_to_char(ch, "Desculpe, automapa está atualmente desabilitado.\r\n");
            break;
        default:
            if (!*arg2) {
                TOGGLE_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
                result = (PRF_FLAGGED(ch, tog_messages[toggle].toggle));
            } else if (!strcmp(arg2, "on")) {
                SET_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
                result = 1;
            } else if (!strcmp(arg2, "off")) {
                REMOVE_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
            } else {
                send_to_char(ch, "Valor para %s deve ser 'on' ou 'off'.\r\n", tog_messages[toggle].command);
                return;
            }
    }
    if (result)
        send_to_char(ch, "%s", tog_messages[toggle].enable_msg);
    else
        send_to_char(ch, "%s", tog_messages[toggle].disable_msg);
}

ACMD(do_commands)
{
    int no, i, cmd_num;
    int socials = 0;
    const char *commands[1000];
    int overflow = sizeof(commands) / sizeof(commands[0]);
    if (!ch->desc)
        return;
    if (subcmd == SCMD_SOCIALS)
        socials = 1;
    send_to_char(ch, "Os seguintes %s estão disponíveis para você:\r\n", socials ? "socials" : "comandos");
    /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
    for (no = 0, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; ++cmd_num) {

        i = cmd_sort_info[cmd_num];
        if (complete_cmd_info[i].minimum_level < 0 || GET_LEVEL(ch) < complete_cmd_info[i].minimum_level)
            continue;
        if (complete_cmd_info[i].minimum_level >= LVL_IMMORT)
            continue;
        if (socials != (complete_cmd_info[i].command_pointer == do_action))
            continue;
        if (--overflow < 0)
            continue;
        /* matching command: copy to commands list */
        commands[no++] = complete_cmd_info[i].command;
    }

    /* display commands list in a nice columnized format */
    column_list(ch, 0, commands, no, FALSE);
}

void free_history(struct char_data *ch, int type)
{
    struct txt_block *tmp = GET_HISTORY(ch, type), *ftmp;
    while ((ftmp = tmp)) {
        tmp = tmp->next;
        if (ftmp->text)
            free(ftmp->text);
        free(ftmp);
    }
    GET_HISTORY(ch, type) = NULL;
}

ACMD(do_history)
{
    char arg[MAX_INPUT_LENGTH];
    int type;
    one_argument(argument, arg);
    type = search_block(arg, history_types, FALSE);
    if (!*arg || type < 0) {
        int i;
        send_to_char(ch, "Uso: history <");
        for (i = 0; *history_types[i] != '\n'; i++) {
            send_to_char(ch, " %s ", history_types[i]);
            if (*history_types[i + 1] == '\n')
                send_to_char(ch, ">\r\n");
            else
                send_to_char(ch, "|");
        }
        return;
    }

    if (GET_HISTORY(ch, type) && GET_HISTORY(ch, type)->text && *GET_HISTORY(ch, type)->text) {
        struct txt_block *tmp;
        for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next)
            send_to_char(ch, "%s", tmp->text);
            /* Make this a 1 if you want history to clear after viewing */
#if 0
		free_history(ch, type);
#endif
    } else
        send_to_char(ch, "Você não possui histórico neste canal.\r\n");
}

#define HIST_LENGTH 100
void add_history(struct char_data *ch, char *str, int type)
{
    int i = 0;
    char time_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    struct txt_block *tmp;
    time_t ct;
    if (IS_NPC(ch))
        return;
    tmp = GET_HISTORY(ch, type);
    ct = time(0);
    strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct));
    sprintf(buf, "%s%s", time_str, str);
    if (!tmp) {
        CREATE(GET_HISTORY(ch, type), struct txt_block, 1);
        GET_HISTORY(ch, type)->text = strdup(buf);
    } else {
        while (tmp->next)
            tmp = tmp->next;
        CREATE(tmp->next, struct txt_block, 1);
        tmp->next->text = strdup(buf);
        for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next, i++)
            ;
        for (; i > HIST_LENGTH && GET_HISTORY(ch, type); i--) {
            tmp = GET_HISTORY(ch, type);
            GET_HISTORY(ch, type) = tmp->next;
            if (tmp->text)
                free(tmp->text);
            free(tmp);
        }
    }
    /* add this history message to ALL */
    if (type != HIST_ALL)
        add_history(ch, str, HIST_ALL);
}

ACMD(do_whois)
{
    struct char_data *victim = 0;
    int hours;
    int got_from_file = 0;
    char buf[MAX_STRING_LENGTH];
    one_argument(argument, buf);
    if (!*buf) {
        send_to_char(ch, "Whois quem?\r\n");
        return;
    }

    if (!(victim = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD))) {
        CREATE(victim, struct char_data, 1);
        clear_char(victim);
        new_mobile_data(victim);
        CREATE(victim->player_specials, struct player_special_data, 1);
        if (load_char(buf, victim) > -1)
            got_from_file = 1;
        else {
            send_to_char(ch, "Esta pessoa não está jogando!\r\n");
            free_char(victim);
            return;
        }
    }

    /* We either have our victim from file or he's playing or function has
       returned. */
    sprinttype(GET_SEX(victim), genders, buf, sizeof(buf));
    send_to_char(ch, "Nome: %s %s\r\nSexo: %s\r\n", GET_NAME(victim),
                 (victim->player.title ? victim->player.title : ""), buf);
    sprinttype(victim->player.chclass, pc_class_types, buf, sizeof(buf));
    send_to_char(ch, "Classe: %s\r\n", buf);
    send_to_char(ch, "Nível: %d\r\n", GET_LEVEL(victim));
    if (!(GET_LEVEL(victim) < LVL_IMMORT) || (GET_LEVEL(ch) >= GET_LEVEL(victim))) {
        strftime(buf, sizeof(buf), "%a %b %d %Y", localtime(&(victim->player.time.logon)));
        hours = (time(0) - victim->player.time.logon) / 3600;
        if (!got_from_file) {
            send_to_char(ch, "Ultimo Login: Jogando Agora!  (Ausente %d Minutos)",
                         victim->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
            if (!victim->desc)
                send_to_char(ch, "  (Linkless)\r\n");
            else
                send_to_char(ch, "\r\n");
            if (PRF_FLAGGED(victim, PRF_AFK))
                send_to_char(ch, "%s%s está away no momento, então %s pode não responder a sua comunicação.%s\r\n",
                             CBGRN(ch, C_NRM), GET_NAME(victim), ELEA(victim), CCNRM(ch, C_NRM));
        } else if (hours > 0)
            send_to_char(ch, "Último Login: %s (%d dias & %d horas.)\r\n", buf, hours / 24, hours % 24);
        else
            send_to_char(ch, "Último Login: %s (0 horas & %d minutos.)\r\n", buf,
                         (int)(time(0) - victim->player.time.logon) / 60);
    }

    if (has_mail(GET_IDNUM(victim)))
        act("$L$u possui carta esperando.", FALSE, ch, 0, victim, TO_CHAR);
    else
        act("$L$u não possui carta esperando.", FALSE, ch, 0, victim, TO_CHAR);
    if (PLR_FLAGGED(victim, PLR_DELETED))
        send_to_char(ch, "***DELETADO***\r\n");
    if (!got_from_file && victim->desc != NULL && GET_LEVEL(ch) >= LVL_GOD) {
        protocol_t *prot = victim->desc->pProtocol;
        send_to_char(ch, "Cliente:  %s [%s]\r\n", prot->pVariables[eMSDP_CLIENT_ID]->pValueString,
                     prot->pVariables[eMSDP_CLIENT_VERSION]->pValueString
                         ? prot->pVariables[eMSDP_CLIENT_VERSION]->pValueString
                         : "Desconhecido");
        send_to_char(ch, "Cor:   %s\r\n",
                     prot->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt
                         ? "Xterm"
                         : (prot->pVariables[eMSDP_ANSI_COLORS]->ValueInt ? "Ansi" : "Nenhum"));
        send_to_char(ch, "MXP:     %s\r\n", prot->bMXP ? "Sim" : "Não");
        send_to_char(ch, "Charset: %s\r\n", prot->bCHARSET ? "Sim" : "Não");
        send_to_char(ch, "MSP:     %s\r\n", prot->bMSP ? "Sim" : "Não");
        send_to_char(ch, "ATCP:    %s\r\n", prot->bATCP ? "Sim" : "Não");
        send_to_char(ch, "MSDP:    %s\r\n", prot->bMSDP ? "Sim" : "Não");
    }

    if (got_from_file)
        free_char(victim);
}

static bool get_zone_levels(zone_rnum znum, char *buf)
{
    /* Create a string for the level restrictions for this zone. */
    if ((zone_table[znum].min_level == -1) && (zone_table[znum].max_level == -1)) {
        sprintf(buf, "<Não configurado!>");
        return FALSE;
    }

    if (zone_table[znum].min_level == -1) {
        sprintf(buf, "Até o nível %d", zone_table[znum].max_level);
        return TRUE;
    }

    if (zone_table[znum].max_level == -1) {
        sprintf(buf, "Acima do nível %d", zone_table[znum].min_level);
        return TRUE;
    }

    sprintf(buf, "Níveis %d até %d", zone_table[znum].min_level, zone_table[znum].max_level);
    return TRUE;
}

ACMD(do_areas)
{
    int i, hilev = -1, lolev = -1, zcount = 0, lev_set, len = 0, tmp_len = 0;
    char arg[MAX_INPUT_LENGTH], *second, lev_str[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    bool show_zone = FALSE, overlap = FALSE, overlap_shown = FALSE;
    one_argument(argument, arg);
    if (*arg) {
        /* There was an arg typed - check for level range */
        second = strchr(arg, '-');
        if (second) {
            /* Check for 1st value */
            if (second == arg)
                lolev = 0;
            else
                lolev = atoi(arg);
            /* Check for 2nd value */
            if (*(second + 1) == '\0' || !isdigit(*(second + 1)))
                hilev = 100;
            else
                hilev = atoi(second + 1);
        } else {
            /* No range - single number */
            lolev = atoi(arg);
            hilev = -1; /* No high level - indicates single level */
        }
    }
    if (hilev != -1 && lolev > hilev) {
        /* Swap hi and lo lev if needed */
        i = lolev;
        lolev = hilev;
        hilev = i;
    }
    if (hilev != -1)
        len = snprintf(buf, sizeof(buf), "Checando alcance: %s%d até %d%s\r\n", QYEL, lolev, hilev, QNRM);
    else if (lolev != -1)
        len = snprintf(buf, sizeof(buf), "Checando nível: %s%d%s\r\n", QYEL, lolev, QNRM);
    else
        len = snprintf(buf, sizeof(buf), "Checando todas as áreas.\r\n");
    for (i = 0; i <= top_of_zone_table; i++) { /* Go through the whole zone table */
        show_zone = FALSE;
        overlap = FALSE;
        if (ZONE_FLAGGED(i, ZONE_GRID)) { /* Is this zone 'on the grid' ? */
            if (lolev == -1) {
                /* No range supplied, show all zones */
                show_zone = TRUE;
            } else if ((hilev == -1) && (lolev >= ZONE_MINLVL(i)) && (lolev <= ZONE_MAXLVL(i))) {
                /* Single number supplied, it's in this zone's range */
                show_zone = TRUE;
            } else if ((hilev != -1) && (lolev >= ZONE_MINLVL(i)) && (hilev <= ZONE_MAXLVL(i))) {
                /* Range supplied, it's completely within this zone's range
                   (no overlap) */
                show_zone = TRUE;
            } else if ((hilev != -1) && ((lolev >= ZONE_MINLVL(i) && lolev <= ZONE_MAXLVL(i)) ||
                                         (hilev <= ZONE_MAXLVL(i) && hilev >= ZONE_MINLVL(i)))) {
                /* Range supplied, it overlaps this zone's range */
                show_zone = TRUE;
                overlap = TRUE;
            } else if (ZONE_MAXLVL(i) < 0 && (lolev >= ZONE_MINLVL(i))) {
                /* Max level not set for this zone, but specified min in range
                 */
                show_zone = TRUE;
            } else if (ZONE_MAXLVL(i) < 0 && (hilev >= ZONE_MINLVL(i))) {
                /* Max level not set for this zone, so just display it as red */
                show_zone = TRUE;
                overlap = TRUE;
            }
        }

        if (show_zone) {
            if (overlap)
                overlap_shown = TRUE;
            lev_set = get_zone_levels(i, lev_str);
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "\tn(%3d) %s%-*s\tn %s%s\tn\r\n", ++zcount,
                               overlap ? QRED : QCYN, count_color_chars(zone_table[i].name) + 30, zone_table[i].name,
                               lev_set ? "\tc" : "\tn", lev_set ? lev_str : "Todos os níveis");
            len += tmp_len;
        }
    }
    tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%d%s area%s encontrada%s.\r\n", QYEL, zcount, QNRM,
                       zcount == 1 ? "" : "s", zcount == 1 ? "" : "s");
    len += tmp_len;
    if (overlap_shown) {
        snprintf(buf + len, sizeof(buf) - len,
                 "Áreas mostradas em \trvermelho\tn podem ter criaturas fora do alcance mostrado.\r\n");
    }

    if (zcount == 0)
        send_to_char(ch, "Nenhuma area encontrada.\r\n");
    else
        page_string(ch->desc, buf, TRUE);
}

static void list_scanned_chars(struct char_data *list, struct char_data *ch, int distance, int door)
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH - 1];
    const char *how_far[] = {"perto", "longe", "muito longe"};
    struct char_data *i;
    int count = 0;
    *buf = '\0';
    /* this loop is a quick, easy way to help make a grammatical sentence
       (i.e., "You see x, x, y, and z." with commas, "and", etc.) */
    for (i = list; i; i = i->next_in_room)
        /* put any other conditions for scanning someone in this if statement
           - i.e., if (CAN_SEE(ch, i) && condition2 && condition3) or whatever
         */
        if (CAN_SEE(ch, i))
            count++;
    if (!count)
        return;
    for (i = list; i; i = i->next_in_room) {

        /* make sure to add changes to the if statement above to this one
           also, using or's to join them.. i.e., if (!CAN_SEE(ch, i) ||
           !condition2 || !condition3) */

        if (!CAN_SEE(ch, i))
            continue;
        if (!*buf)
            snprintf(buf, sizeof(buf), "Você vê %s", GET_NAME(i));
        else
            strncat(buf, GET_NAME(i), sizeof(buf) - strlen(buf) - 1);
        if (--count > 1)
            strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
        else if (count == 1)
            strncat(buf, " e ", sizeof(buf) - strlen(buf) - 1);
        else {
            if ((door == UP) || (door == DOWN))
                snprintf(buf2, sizeof(buf2), " %s em %s.\r\n", how_far[distance], dirs_pt[door]);
            else
                snprintf(buf2, sizeof(buf2), " %s ao %s.\r\n", how_far[distance], dirs_pt[door]);

            strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
        }
    }
    send_to_char(ch, "%s", buf);
}

ACMD(do_scan)
{
    int door;
    bool found = FALSE;
    int range;
    int maxrange = 3;
    int prob, percent;
    room_rnum scanned_room = IN_ROOM(ch);

    if (!IS_NPC(ch) && !GET_SKILL(ch, SKILL_SCAN)) {
        send_to_char(ch, "You have no idea how.\r\n");
        return;
    }
    if (IS_AFFECTED(ch, AFF_BLIND)) {
        send_to_char(ch, "Você não pode ver nada! Você está ceg%s!\r\n", OA(ch));
        return;
    }
    percent = rand_number(1, 101); /* 101% is a complete failure */
    if (!IS_NPC(ch))
        prob = GET_SKILL(ch, SKILL_SCAN);
    else
        prob = GET_LEVEL(ch);

    if (percent > prob)
        maxrange = 0;

    for (door = 0; door < DIR_COUNT; door++) {
        for (range = 1; range <= maxrange; range++) {
            if (world[scanned_room].dir_option[door] && world[scanned_room].dir_option[door]->to_room != NOWHERE &&
                !IS_SET(world[scanned_room].dir_option[door]->exit_info, EX_CLOSED) &&
                !IS_SET(world[scanned_room].dir_option[door]->exit_info, EX_HIDDEN)) {
                scanned_room = world[scanned_room].dir_option[door]->to_room;
                if (IS_DARK(scanned_room) && !CAN_SEE_IN_DARK(ch)) {
                    if (world[scanned_room].people)
                        send_to_char(ch, "%s: Está tudo escuro... Mas você escuta ruídos\r\n", dirs_pt[door]);
                    else
                        send_to_char(ch, "%s: Está tudo escuro...\r\n", dirs_pt[door]);
                    found = TRUE;
                } else {
                    if (world[scanned_room].people) {
                        list_scanned_chars(world[scanned_room].people, ch, range - 1, door);
                        found = TRUE;
                    }
                }
            }   // end of if
            else
                break;
        }   // end of range
        scanned_room = IN_ROOM(ch);
    }   // end of directions
    if (!found) {
        send_to_char(ch, "Você não vê nada próximo.\r\n");
    }
}   // end of do_scan

ACMD(do_evaluate)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct char_data *target_char = NULL;
    struct obj_data *target_obj = NULL;
    int score;

    half_chop(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Avaliar que item?\r\nSintaxe: avaliar <item> [personagem]\r\n");
        return;
    }

    /* Encontra o personagem alvo. Se nenhum for especificado, o alvo é o próprio jogador. */
    if (!*arg2) {
        target_char = ch;
    } else {
        if (!(target_char = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
            send_to_char(ch, "Não vê essa pessoa aqui.\r\n");
            return;
        }
    }

    /* Encontra o objeto no inventário do personagem alvo. */
    if (!(target_obj = get_obj_in_list_vis(ch, arg1, NULL, target_char->carrying))) {
        send_to_char(ch, "%s não parece ter '%s'.\r\n", (target_char == ch) ? "Você" : GET_NAME(target_char), arg1);
        return;
    }

    /* Chama a nossa função de IA e mostra o resultado! */
    score = evaluate_item_for_mob(target_char, target_obj);

    send_to_char(ch, "A pontuação de desejo de '%s' para %s é: %s%d%s.\r\n", target_obj->short_description,
                 (target_char == ch) ? "você" : GET_NAME(target_char), CCCYN(ch, C_NRM), score, CCNRM(ch, C_NRM));

    if (IS_NPC(target_char)) {
        send_to_char(ch, "(Quanto maior a pontuação, mais o mob irá desejar este item).\r\n");
    }
}
