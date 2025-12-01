/**************************************************************************
 *  File: fight.c                                           Part of tbaMUD *
 *  Usage: Combat system.                                                  *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "shop.h"
#include "quest.h"
#include "spedit.h"
#include "genolc.h"
#include "genzon.h"

/* locally defined global variables, used externally */
/* head of l-list of fighting chars */

struct char_data *combat_list = NULL;

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] = {
    {"golpeia", "golpear", "seu", "o", "golpe"},            //  0: hit
    {"ferroa", "ferroar", "sua", "a", "ferroada"},          //  1: sting
    {"chicoteia", "chicotear", "sua", "a", "chicotada"},    //  2: whip
    {"retalha", "retalhar", "sua", "a", "retalhada"},       //  3: slash
    {"morde", "morder", "sua", "a", "mordida"},             //  4: bite
    {"caceteia", "cacetear", "sua", "a", "cacetada"},       //  5: bludgeon
    {"esmaga", "esmagar", "sua", "a", "esmagada"},          //  6: crush
    {"moe", "moer", "sua", "a", "moída"},                   //  7: pound
    {"arranha", "arranhar", "sua", "a", "arranhada"},       //  8: claw
    {"espanca", "espancar", "sua", "a", "espancada"},       //  9: maul
    {"açoita", "açoitar", "sua", "a", "açoitada"},          // 10: thrash
    {"fura", "furar", "sua", "a", "furada"},                // 11: pierce
    {"explode", "explodir", "sua", "a", "explosão"},        // 12: blast
    {"esmurra", "esmurrar", "sua", "a", "esmurrada"},       // 13: punch
    {"esfaqueia", "esfaquear", "sua", "a", "esfaqueada"},   // 14: stab
    {"perfura", "perfurar", "sua", "a", "perfurada"},       // 15: bore
    {"espeta", "espetar", "sua", "a", "espetada"},          // 16: broach
    {"corta", "cortar", "sua", "a", "cortada"}              // 17: mow
};

/* local (file scope only) variables */
static struct char_data *next_combat_list = NULL;

/* local file scope utility functions */
static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
static void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type);
static void make_corpse(struct char_data *ch);
static void make_magic_stone(struct char_data *ch, long target_id);
static void change_alignment(struct char_data *ch, struct char_data *victim);
static void group_gain(struct char_data *ch, struct char_data *victim);
static void solo_gain(struct char_data *ch, struct char_data *victim);
/** @todo refactor this function name */
static char *replace_string(const char *str, const struct attack_hit_type *attack_type);
int get_weapon_prof(struct char_data *ch, struct obj_data *wield);
int get_nighthammer(struct char_data *ch, bool real);
int attacks_per_round(struct char_data *ch);

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* Check if a spell has the MAG_AURA flag */
static int is_aura_spell(int spellnum)
{
    if (spellnum <= 0)
        return 0;
    return IS_SET(get_spell_mag_flags(spellnum), MAG_AURA);
}

/* Get the element of a spell from the spell database */
static int get_spell_element(int spellnum)
{
    struct str_spells *spell;

    if (spellnum <= 0)
        return ELEMENT_UNDEFINED;

    spell = get_spell_by_vnum(spellnum);
    if (spell)
        return spell->element;
    return ELEMENT_UNDEFINED;
}

/* Check if an affect is an actual aura shield (not just a side effect from aura damage).
 * Returns the AFF flag that corresponds to the aura spell, or -1 if not a shield spell. */
static int get_aura_shield_aff(int spellnum)
{
    switch (spellnum) {
        case SPELL_FIRESHIELD:
            return AFF_FIRESHIELD;
        case SPELL_THISTLECOAT:
            return AFF_THISTLECOAT;
        case SPELL_WINDWALL:
            return AFF_WINDWALL;
        case SPELL_WATERSHIELD:
            return AFF_WATERSHIELD;
        case SPELL_ROCKSHIELD:
            return AFF_ROCKSHIELD;
        case SPELL_POISONSHIELD:
            return AFF_POISONSHIELD;
        case SPELL_LIGHTNINGSHIELD:
            return AFF_LIGHTNINGSHIELD;
        case SPELL_ICESHIELD:
            return AFF_ICESHIELD;
        case SPELL_ACIDSHIELD:
            return AFF_ACIDSHIELD;
        case SPELL_MINDSHIELD:
            return AFF_MINDSHIELD;
        case SPELL_FORCESHIELD:
            return AFF_FORCESHIELD;
        case SPELL_SOUNDBARRIER:
            return AFF_SOUNDBARRIER;
        default:
            return -1;
    }
}

/* Get the first active aura shield spell affecting a character by scanning affects.
 * Only returns spells that have the actual shield bitvector set, not side effects. */
static int get_aura_shield_spell(struct char_data *ch)
{
    struct affected_type *af;
    int shield_aff;

    if (!ch)
        return 0;

    for (af = ch->affected; af; af = af->next) {
        if (af->spell > 0 && is_aura_spell(af->spell)) {
            /* Verify this is an actual shield, not a side effect (like burning/soaked) */
            shield_aff = get_aura_shield_aff(af->spell);
            if (shield_aff >= 0 && IS_SET_AR(af->bitvector, shield_aff))
                return af->spell;
        }
    }
    return 0;
}

/* Helper function to check if character has any aura shield active */
static int has_aura_shield(struct char_data *ch)
{
    if (!ch)
        return 0;
    return (get_aura_shield_spell(ch) > 0);
}

/* Get damage reflection ratio for an aura spell based on element
 * Different elements have different reflection characteristics */
static float get_aura_reflect_ratio(int spellnum, int saved)
{
    int element = get_spell_element(spellnum);

    /* Base reflection ratios by element when saving throw fails */
    if (!saved) {
        return 1.0; /* Full damage reflection on failed save */
    }

    /* Reduced reflection ratios when saving throw succeeds */
    switch (element) {
        case ELEMENT_FIRE:
            return 0.5; /* Fire - moderate reflection */
        case ELEMENT_WATER:
            return 0.4; /* Water - slightly lower reflection */
        case ELEMENT_AIR:
            return 0.4; /* Air - slightly lower reflection */
        case ELEMENT_EARTH:
            return 0.25; /* Earth/Rock - lower reflection but more defense */
        case ELEMENT_LIGHTNING:
            return 0.5; /* Lightning - moderate reflection */
        case ELEMENT_ICE:
            return 0.33; /* Ice - lower reflection */
        case ELEMENT_POISON:
            return 0.33; /* Poison - lower reflection */
        case ELEMENT_ACID:
            return 0.45; /* Acid - moderate-high reflection */
        case ELEMENT_MENTAL:
            return 0.35; /* Mental - lower reflection */
        case ELEMENT_PHYSICAL:
            return 0.4; /* Physical/Force - moderate reflection */
        case ELEMENT_HOLY:
            return 0.5; /* Holy - damage reduction (sanctuary) */
        case ELEMENT_UNHOLY:
            return 0.33; /* Unholy - damage reduction (gloomshield) */
        default:
            return 0.33; /* Default for unknown elements */
    }
}

/* Send element-specific aura reflection messages based on the aura spell type.
 * Messages are styled after the old messages from lib/misc/messages file.
 * ch = attacker (who receives reflected damage), victim = aura owner */
static void send_aura_reflect_message(struct char_data *ch, struct char_data *victim, int aura_spell, int saved)
{
    switch (aura_spell) {
        case SPELL_FIRESHIELD:
            if (saved) {
                act("A barreira de fogo de $N queima você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de fogo queima $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é queimad$r pela barreira de fogo de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira de fogo de $N cozinha você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de fogo cozinha $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é cozinhad$r pela barreira de fogo de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_THISTLECOAT:
            if (saved) {
                act("A barreira de espinhos de $N arranha você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de espinhos arranha $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é arranhad$r pela barreira de espinhos de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira de espinhos de $N rasga você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de espinhos rasga $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é rasgad$r pela barreira de espinhos de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_WINDWALL:
            if (saved) {
                act("A parede de vento de $N corta você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua parede de vento corta $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é cortad$r pela parede de vento de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A parede de vento de $N despedaça você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua parede de vento despedaça $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é despedaçad$r pela parede de vento de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_SOUNDBARRIER:
            if (saved) {
                act("A barreira sonora de $N atordoa você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira sonora atordoa $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é atordoad$r pela barreira sonora de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira sonora de $N ensurdece você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira sonora ensurdece $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é ensurdecid$r pela barreira sonora de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_WATERSHIELD:
            if (saved) {
                act("A barreira de água de $N encharca você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de água encharca $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é encharcad$r pela barreira de água de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira de água de $N afoga você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de água afoga $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é afogad$r pela barreira de água de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_ROCKSHIELD:
            if (saved) {
                act("A barreira de pedra de $N esmaga você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de pedra esmaga $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é esmagad$r pela barreira de pedra de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira de pedra de $N tritura você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de pedra tritura $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é triturad$r pela barreira de pedra de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_POISONSHIELD:
            if (saved) {
                act("A barreira venenosa de $N intoxica você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira venenosa intoxica $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é intoxicad$r pela barreira venenosa de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira venenosa de $N envenena você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira venenosa envenena $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é envenenad$r pela barreira venenosa de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_LIGHTNINGSHIELD:
            if (saved) {
                act("A barreira elétrica de $N choca você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira elétrica choca $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é chocad$r pela barreira elétrica de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira elétrica de $N eletrocuta você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira elétrica eletrocuta $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é eletrocutad$r pela barreira elétrica de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_ICESHIELD:
            if (saved) {
                act("A barreira de gelo de $N congela você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de gelo congela $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é congelad$r pela barreira de gelo de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira de gelo de $N petrifica você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de gelo petrifica $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é petrificad$r pela barreira de gelo de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_ACIDSHIELD:
            if (saved) {
                act("A barreira ácida de $N corrói você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira ácida corrói $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é corroíd$r pela barreira ácida de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira ácida de $N derrete você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira ácida derrete $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é derretid$r pela barreira ácida de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_MINDSHIELD:
            if (saved) {
                act("A barreira mental de $N confunde você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira mental confunde $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é confundid$r pela barreira mental de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira mental de $N enlouquece você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira mental enlouquece $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é enlouquecid$r pela barreira mental de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        case SPELL_FORCESHIELD:
            if (saved) {
                act("A barreira de força de $N empurra você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de força empurra $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é empurrad$r pela barreira de força de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A barreira de força de $N arremessa você!! ARRGH!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira de força arremessa $n!", FALSE, ch, 0, victim, TO_VICT);
                act("$n é arremessad$r pela barreira de força de $N.", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
        default:
            /* Fallback for any other aura spells */
            if (saved) {
                act("A aura de $N reflete parte do dano de volta para você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura reflete parte do dano de volta para $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura de $N reflete parte do dano de volta para $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            } else {
                act("A aura de $N reflete o dano de volta para você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura reflete o dano de volta para $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura de $N reflete o dano de volta para $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            }
            break;
    }
}

/* Element interaction types */
#define ELEM_NEUTRAL 0 /* No special interaction */
#define ELEM_BEATS 1   /* Attacker element destroys defender */
#define ELEM_NULLIFY 2 /* Both elements cancel each other */
#define ELEM_AMPLIFY 3 /* Attacker element is amplified against defender */

/* Check elemental interaction between two elements
 * Returns: ELEM_NEUTRAL (0), ELEM_BEATS (1), ELEM_NULLIFY (2), or ELEM_AMPLIFY (3) */
static int get_element_interaction(int attacker_element, int defender_element)
{
    /* Same element always nullifies */
    if (attacker_element == defender_element && attacker_element != ELEMENT_UNDEFINED)
        return ELEM_NULLIFY;

    /* Fire interactions */
    if (attacker_element == ELEMENT_FIRE) {
        if (defender_element == ELEMENT_ICE)
            return ELEM_AMPLIFY; /* Fire melts ice - amplified damage */
        if (defender_element == ELEMENT_WATER)
            return ELEM_NULLIFY; /* Fire and water cancel */
        if (defender_element == ELEMENT_EARTH)
            return ELEM_BEATS; /* Fire burns earth/plants */
        if (defender_element == ELEMENT_AIR)
            return ELEM_AMPLIFY; /* Air feeds fire - amplified */
    }

    /* Water interactions */
    if (attacker_element == ELEMENT_WATER) {
        if (defender_element == ELEMENT_FIRE)
            return ELEM_NULLIFY; /* Water and fire cancel */
        if (defender_element == ELEMENT_LIGHTNING)
            return ELEM_NULLIFY; /* Water conducts but disperses lightning */
        if (defender_element == ELEMENT_ACID)
            return ELEM_BEATS; /* Water dilutes acid */
    }

    /* Ice interactions */
    if (attacker_element == ELEMENT_ICE) {
        if (defender_element == ELEMENT_FIRE)
            return ELEM_BEATS; /* Ice melts against fire - ice is beaten */
        if (defender_element == ELEMENT_WATER)
            return ELEM_AMPLIFY; /* Ice freezes water - amplified */
        if (defender_element == ELEMENT_LIGHTNING)
            return ELEM_BEATS; /* Cold slows electricity */
        if (defender_element == ELEMENT_AIR)
            return ELEM_BEATS; /* Cold freezes air */
    }

    /* Lightning interactions */
    if (attacker_element == ELEMENT_LIGHTNING) {
        if (defender_element == ELEMENT_WATER)
            return ELEM_AMPLIFY; /* Lightning conducts through water - amplified */
        if (defender_element == ELEMENT_EARTH)
            return ELEM_NULLIFY; /* Earth grounds lightning */
        if (defender_element == ELEMENT_PHYSICAL)
            return ELEM_AMPLIFY; /* Lightning conducts through metal/physical */
    }

    /* Air/Wind interactions */
    if (attacker_element == ELEMENT_AIR) {
        if (defender_element == ELEMENT_FIRE)
            return ELEM_AMPLIFY; /* Wind feeds fire - amplified */
        if (defender_element == ELEMENT_POISON)
            return ELEM_BEATS; /* Wind disperses poison */
        if (defender_element == ELEMENT_EARTH)
            return ELEM_NULLIFY; /* Earth blocks wind */
    }

    /* Earth interactions */
    if (attacker_element == ELEMENT_EARTH) {
        if (defender_element == ELEMENT_AIR)
            return ELEM_BEATS; /* Earth blocks wind */
        if (defender_element == ELEMENT_LIGHTNING)
            return ELEM_BEATS; /* Earth grounds lightning */
        if (defender_element == ELEMENT_POISON)
            return ELEM_BEATS; /* Earth absorbs poison */
        if (defender_element == ELEMENT_WATER)
            return ELEM_NULLIFY; /* Water erodes earth */
    }

    /* Poison interactions */
    if (attacker_element == ELEMENT_POISON) {
        if (defender_element == ELEMENT_WATER)
            return ELEM_AMPLIFY; /* Poison spreads in water - amplified */
        if (defender_element == ELEMENT_AIR)
            return ELEM_NULLIFY; /* Air disperses poison */
    }

    /* Acid interactions */
    if (attacker_element == ELEMENT_ACID) {
        if (defender_element == ELEMENT_EARTH)
            return ELEM_AMPLIFY; /* Acid dissolves rock - amplified */
        if (defender_element == ELEMENT_PHYSICAL)
            return ELEM_AMPLIFY; /* Acid corrodes armor - amplified */
        if (defender_element == ELEMENT_WATER)
            return ELEM_NULLIFY; /* Water dilutes acid */
    }

    /* Holy interactions */
    if (attacker_element == ELEMENT_HOLY) {
        if (defender_element == ELEMENT_UNHOLY)
            return ELEM_AMPLIFY; /* Holy destroys unholy - amplified */
    }

    /* Unholy interactions */
    if (attacker_element == ELEMENT_UNHOLY) {
        if (defender_element == ELEMENT_HOLY)
            return ELEM_AMPLIFY; /* Unholy destroys holy - amplified */
    }

    /* Mental interactions */
    if (attacker_element == ELEMENT_MENTAL) {
        if (defender_element == ELEMENT_PHYSICAL)
            return ELEM_BEATS; /* Mind over matter */
    }

    /* Physical interactions */
    if (attacker_element == ELEMENT_PHYSICAL) {
        if (defender_element == ELEMENT_MENTAL)
            return ELEM_BEATS; /* Physical disrupts mental focus */
    }

    return ELEM_NEUTRAL;
}

/* Backward compatible wrapper - returns 1 if attacker beats or amplifies against defender */
static int element_beats(int attacker_element, int defender_element)
{
    int interaction = get_element_interaction(attacker_element, defender_element);
    return (interaction == ELEM_BEATS || interaction == ELEM_AMPLIFY);
}

/* Check if two elements nullify each other */
static int elements_nullify(int element1, int element2)
{
    return (get_element_interaction(element1, element2) == ELEM_NULLIFY);
}

/* Get damage multiplier based on elemental interaction */
static float get_element_damage_multiplier(int attacker_element, int defender_element)
{
    int interaction = get_element_interaction(attacker_element, defender_element);

    switch (interaction) {
        case ELEM_AMPLIFY:
            return 1.5; /* 50% more damage */
        case ELEM_BEATS:
            return 1.25; /* 25% more damage */
        case ELEM_NULLIFY:
            return 0.5; /* 50% less damage */
        default:
            return 1.0; /* Normal damage */
    }
}

/* The Fight related routines */
void appear(struct char_data *ch)
{
    if (affected_by_spell(ch, SPELL_INVISIBLE))
        affect_from_char(ch, SPELL_INVISIBLE);

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

    if (GET_LEVEL(ch) < LVL_IMMORT)
        act("$n aparece lentamente.", FALSE, ch, 0, 0, TO_ROOM);
    else
        act("Você sente uma estranha presença a medida que $n aparece, aparentemente do nada.", FALSE, ch, 0, 0,
            TO_ROOM);
}

int compute_armor_class(struct char_data *ch)
{
    int armorclass = GET_AC(ch);
    struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);

    if (AWAKE(ch))
        armorclass += dex_app[GET_DEX(ch)].defensive * 10;

    if (!IS_NPC(ch)) {
        armorclass += wpn_prof[get_weapon_prof(ch, wielded)].to_ac * 10;
        armorclass += nighthammer_info[get_nighthammer(ch, true)].to_ac;
        return (MAX(-200, armorclass)); /* -200 is lowest */
    } else {
        return (armorclass);
    }
}

void update_pos(struct char_data *victim)
{
    if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
        return;
    else if (GET_HIT(victim) > 0)
        GET_POS(victim) = POS_STANDING;
    else if (GET_HIT(victim) <= -11)
        GET_POS(victim) = POS_DEAD;
    else if (GET_HIT(victim) <= -6)
        GET_POS(victim) = POS_MORTALLYW;
    else if (GET_HIT(victim) <= -3)
        GET_POS(victim) = POS_INCAP;
    else
        GET_POS(victim) = POS_STUNNED;
}

void check_killer(struct char_data *ch, struct char_data *vict)
{
    if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA))
        return;
    if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
        return;
    if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
        return;

    SET_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
    send_to_char(ch, "Se você deseja ser um JOGADOR ASSASSINO, você será...\r\n");
    mudlog(BRF, MAX(LVL_IMMORT, MAX(GET_INVIS_LEV(ch), GET_INVIS_LEV(vict))), TRUE,
           "PC Killer bit set on %s for initiating attack on %s at %s.", GET_NAME(ch), GET_NAME(vict),
           world[IN_ROOM(vict)].name);
}

/* start one char fighting another (yes, it is horrible, I know... ) */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
    if (ch == vict)
        return;

    if (FIGHTING(ch)) {
        core_dump();
        return;
    }

    ch->next_fighting = combat_list;
    combat_list = ch;

    if (AFF_FLAGGED(ch, AFF_SLEEP))
        affect_from_char(ch, SPELL_SLEEP);

    FIGHTING(ch) = vict;
    GET_POS(ch) = POS_FIGHTING;

    if (!CONFIG_PK_ALLOWED)
        check_killer(ch, vict);
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
    struct char_data *temp;

    if (ch == next_combat_list)
        next_combat_list = ch->next_fighting;

    REMOVE_FROM_LIST(ch, combat_list, next_fighting);
    ch->next_fighting = NULL;
    FIGHTING(ch) = NULL;
    GET_POS(ch) = POS_STANDING;
    update_pos(ch);
}

static void make_corpse(struct char_data *ch)
{
    char buf2[MAX_NAME_LENGTH + 64];
    struct obj_data *corpse, *o;
    struct obj_data *money;
    int i, x, y;

    corpse = create_obj();

    corpse->item_number = NOTHING;
    IN_ROOM(corpse) = NOWHERE;
    corpse->name = strdup("corpo");

    snprintf(buf2, sizeof(buf2), "O corpo de %s está aqui.", GET_NAME(ch));
    corpse->description = strdup(buf2);

    snprintf(buf2, sizeof(buf2), "o corpo de %s", GET_NAME(ch));
    corpse->short_description = strdup(buf2);

    GET_OBJ_TYPE(corpse) = ITEM_CORPSE;
    for (x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
        if (x < EF_ARRAY_MAX)
            GET_OBJ_EXTRA_AR(corpse, x) = 0;
        if (y < TW_ARRAY_MAX)
            corpse->obj_flags.wear_flags[y] = 0;
    }
    SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
    GET_OBJ_VAL(corpse, 0) = 0; /* You can't store stuff in a corpse */
    GET_OBJ_VAL(corpse, 3) = 1; /* corpse identifier */
    GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
    GET_OBJ_RENT(corpse) = 100000;
    if (IS_NPC(ch)) {
        SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
        GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
    } else
        GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;
    /* transfer character's inventory to the corpse */
    corpse->contains = ch->carrying;
    for (o = corpse->contains; o != NULL; o = o->next_content)
        o->in_obj = corpse;
    object_list_new_owner(corpse, NULL);
    /* transfer character's equipment to the corpse */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i)) {
            remove_otrigger(GET_EQ(ch, i), ch);
            obj_to_obj(unequip_char(ch, i), corpse);
        }

    /* transfer gold */
    if (GET_GOLD(ch) > 0) {
        /* following 'if' clause added to fix gold duplication loophole. The
           above line apparently refers to the old "partially log in, kill the
           game character, then finish login sequence" duping bug. The
           duplication has been fixed (knock on wood) but the test below shall
           live on, for a while. -gg 3/3/2002 */
        if (IS_NPC(ch) || ch->desc) {
            money = create_money(GET_GOLD(ch));
            obj_to_obj(money, corpse);
        }
        GET_GOLD(ch) = 0;
    }
    ch->carrying = NULL;
    IS_CARRYING_N(ch) = 0;
    IS_CARRYING_W(ch) = 0;
    /* Move o corpo para a cabana da cidade natal */
    if (!IS_NPC(ch)) {
        GET_OBJ_VAL(corpse, 0) = GET_IDNUM(ch);
        if (GET_HOMETOWN(ch) == 1) {
            obj_to_room(corpse, r_ress_room_1);
            log1("Corpo de '%s' na sala #%d tipo %d val0 '%d' val3 '%d'", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)),
                 GET_OBJ_TYPE(corpse), GET_OBJ_VAL(corpse, 0), GET_OBJ_VAL(corpse, 3));
        } else if (GET_HOMETOWN(ch) == 2) {
            obj_to_room(corpse, r_ress_room_2);
            log1("Corpo de '%s' na sala #%d tipo %d val0 '%d' val3 '%d'", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)),
                 GET_OBJ_TYPE(corpse), GET_OBJ_VAL(corpse, 0), GET_OBJ_VAL(corpse, 3));
        } else if (GET_HOMETOWN(ch) == 3) {
            obj_to_room(corpse, r_ress_room_3);
            log1("Corpo de '%s' na sala #%d tipo %d val0 '%d' val3 '%d'", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)),
                 GET_OBJ_TYPE(corpse), GET_OBJ_VAL(corpse, 0), GET_OBJ_VAL(corpse, 3));
        } else {
            /* sem cidade natal */
            /* Safety check: validate room before placing corpse */
            if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                obj_to_room(corpse, IN_ROOM(ch));
                log1("Corpo de '%s' na sala #%d tipo %d val0 '%d' val3 '%d'", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)),
                     GET_OBJ_TYPE(corpse), GET_OBJ_VAL(corpse, 0), GET_OBJ_VAL(corpse, 3));
                act("Uma enorme mão aparece e pega o corpo de $n em segurança.", TRUE, ch, 0, 0, TO_ROOM);
                act("Uma enorme mão aparece e delicadamente deixa $p no chão.", FALSE, 0, corpse, 0, TO_ROOM);
            } else {
                /* If character is in invalid room, place corpse in first resurrection room as fallback */
                log1("AVISO: Personagem '%s' em sala inválida #%d - corpo movido para sala de ressurreição",
                     GET_NAME(ch), IN_ROOM(ch));
                obj_to_room(corpse, r_ress_room_1);
            }
        }
    } else {
        /* Safety check: validate room before placing NPC corpse */
        if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
            obj_to_room(corpse, IN_ROOM(ch));
        } else {
            log1("AVISO: NPC '%s' em sala inválida #%d - corpo descartado", GET_NAME(ch), IN_ROOM(ch));
            /* Could extract_obj(corpse) here, but safer to just not place it */
        }
    }
}

/* Create a magic stone when a quest target mob is killed */
static void make_magic_stone(struct char_data *ch, long target_id)
{
    char buf2[MAX_NAME_LENGTH + 128];
    struct obj_data *stone;
    const char *mob_name;

    /* Safety checks */
    if (!ch || !IS_NPC(ch))
        return;

    /* Validate mob has a name */
    if (!GET_NAME(ch) || !*GET_NAME(ch)) {
        log1("SYSERR: make_magic_stone called on mob without name (vnum %d)", GET_MOB_VNUM(ch));
        return;
    }

    stone = create_obj();
    if (!stone) {
        log1("SYSERR: Failed to create magic stone object for mob %s", GET_NAME(ch));
        return;
    }

    stone->item_number = NOTHING;
    IN_ROOM(stone) = NOWHERE;

    /* Allocate name with error checking */
    stone->name = strdup("pedra magica stone");
    if (!stone->name) {
        log1("SYSERR: Failed to allocate name for magic stone");
        extract_obj(stone);
        return;
    }

    /* Use mob name safely */
    mob_name = GET_NAME(ch);
    snprintf(buf2, sizeof(buf2), "Uma pedra mágica brilhante de %s está aqui.", mob_name);
    stone->description = strdup(buf2);
    if (!stone->description) {
        log1("SYSERR: Failed to allocate description for magic stone");
        extract_obj(stone);
        return;
    }

    snprintf(buf2, sizeof(buf2), "uma pedra mágica de %s", mob_name);
    stone->short_description = strdup(buf2);
    if (!stone->short_description) {
        log1("SYSERR: Failed to allocate short_description for magic stone");
        extract_obj(stone);
        return;
    }

    GET_OBJ_TYPE(stone) = ITEM_MAGIC_STONE;

    /* Clear all flags */
    for (int x = 0; x < EF_ARRAY_MAX; x++)
        GET_OBJ_EXTRA_AR(stone, x) = 0;
    for (int y = 0; y < TW_ARRAY_MAX; y++)
        stone->obj_flags.wear_flags[y] = 0;

    /* Set flags: no donate, no sell, no rent (disappears on quit) */
    SET_BIT_AR(GET_OBJ_EXTRA(stone), ITEM_NODONATE);
    SET_BIT_AR(GET_OBJ_EXTRA(stone), ITEM_NORENT);
    SET_BIT_AR(GET_OBJ_EXTRA(stone), ITEM_NOSELL);
    SET_BIT_AR(GET_OBJ_WEAR(stone), ITEM_WEAR_TAKE);

    /* Store mob species vnum in val0 and specific mob ID in val1 */
    GET_OBJ_VAL(stone, 0) = GET_MOB_VNUM(ch); /* Species vnum */
    GET_OBJ_VAL(stone, 1) = target_id;        /* Specific mob ID for bounty quests */
    GET_OBJ_VAL(stone, 2) = 0;                /* Reserved */
    GET_OBJ_VAL(stone, 3) = 1;                /* Magic stone identifier */

    GET_OBJ_WEIGHT(stone) = 1;
    GET_OBJ_RENT(stone) = 0;
    /* Set decay timer to prevent infinite accumulation - 48 ticks (~1 hour at 75 sec/tick) */
    GET_OBJ_TIMER(stone) = 48;

    /* Place stone in the room where mob died - with safety checks */
    if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
        obj_to_room(stone, IN_ROOM(ch));
        act("Uma pedra mágica brilhante cai no chão quando $n morre.", FALSE, ch, stone, 0, TO_ROOM);
    } else {
        log1("AVISO: Mob '%s' em sala inválida #%d - pedra mágica descartada", mob_name, IN_ROOM(ch));
        extract_obj(stone);
    }
}

/* When ch kills victim */
static void change_alignment(struct char_data *ch, struct char_data *victim)
{
    /* new alignment change algorithm: if you kill a monster with alignment A,
       you move 1/16th of the way to having alignment -A.  Simple and fast. */
    GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}

void death_cry(struct char_data *ch)
{
    int door;

    /* Safety check: validate room before accessing world array */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return;

    act("Você se arrepia ao escutar o grito de morte de $n.", FALSE, ch, 0, 0, TO_ROOM);
    for (door = 0; door < DIR_COUNT; door++)
        if (CAN_GO(ch, door))
            send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room,
                         "Você se arrepia ao escutar o grito de morte de alguém.\r\n");
}

void raw_kill(struct char_data *ch, struct char_data *killer)
{
    struct char_data *i;
    if (FIGHTING(ch))
        stop_fighting(ch);
    while (ch->affected)
        affect_remove(ch, ch->affected);
    /* To make ordinary commands work in scripts.  welcor */
    GET_POS(ch) = POS_STANDING;
    if (killer) {
        if (death_mtrigger(ch, killer))
            death_cry(ch);
    } else
        death_cry(ch);
    if (killer) {
        if (killer->group) {
            while ((i = (struct char_data *)simple_list(killer->group->members)) != NULL) {
                /* Safety check: validate rooms before accessing world array */
                if (IN_ROOM(i) == NOWHERE || IN_ROOM(i) < 0 || IN_ROOM(i) > top_of_world)
                    continue;
                if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                    continue;

                if (IN_ROOM(i) == IN_ROOM(ch) || (world[IN_ROOM(i)].zone == world[IN_ROOM(ch)].zone)) {
                    autoquest_trigger_check(i, ch, NULL, AQ_MOB_KILL);
                    autoquest_trigger_check(i, ch, NULL, AQ_MOB_KILL_BOUNTY);
                }
            }
        } else {
            autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);
            autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL_BOUNTY);
        }
    }

    /* Check mob quest triggers for all mobs in the area */
    if (IS_NPC(ch)) {
        struct char_data *mob;

        /* Safety check: validate ch room before accessing world array */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world) {
            /* Skip mob quest triggers if ch is not in a valid room */
        } else {
            for (mob = character_list; mob; mob = mob->next) {
                if (IS_NPC(mob) && mob != ch && GET_MOB_QUEST(mob) != NOTHING) {
                    /* Safety check: validate mob room before accessing world array */
                    if (IN_ROOM(mob) == NOWHERE || IN_ROOM(mob) < 0 || IN_ROOM(mob) > top_of_world)
                        continue;

                    if (IN_ROOM(mob) == IN_ROOM(ch) || (world[IN_ROOM(mob)].zone == world[IN_ROOM(ch)].zone)) {
                        mob_autoquest_trigger_check(mob, ch, NULL, AQ_MOB_KILL);
                        mob_autoquest_trigger_check(mob, ch, NULL, AQ_MOB_KILL_BOUNTY);
                    }
                }
            }
        }
    }

    /* Check mob quest triggers for player kills */
    if (!IS_NPC(ch) && killer && IS_NPC(killer) && GET_MOB_QUEST(killer) != NOTHING) {
        mob_autoquest_trigger_check(killer, ch, NULL, AQ_PLAYER_KILL);
    }

    /* Mob bragging when killing a high-reputation player */
    if (!IS_NPC(ch) && killer && IS_NPC(killer) && CONFIG_MOB_CONTEXTUAL_SOCIALS) {
        int victim_reputation = GET_REPUTATION(ch);

        /* High reputation victim (60+) triggers bragging */
        if (victim_reputation >= 60) {
            char brag_msg[MAX_STRING_LENGTH];

            /* Very high reputation (80+) gets more enthusiastic bragging */
            if (victim_reputation >= 80) {
                snprintf(brag_msg, sizeof(brag_msg), "ri triunfante, tendo derrotado %s, um aventureiro lendário!",
                         GET_NAME(ch));
            } else {
                snprintf(brag_msg, sizeof(brag_msg),
                         "sorri com satisfação após derrotar %s, um aventureiro respeitado.", GET_NAME(ch));
            }

            do_gmote(killer, brag_msg, 0, 0);

            /* Increase mob's own reputation for defeating a reputable opponent */
            if (killer->ai_data && killer->ai_data->reputation < 100) {
                killer->ai_data->reputation = MIN(killer->ai_data->reputation + 5, 100);
            }
        }
    }

    /* Reputation changes based on kills (players and mobs) */
    /* Dynamic reputation changes (configurable, excludes quests) */
    if (CONFIG_DYNAMIC_REPUTATION && killer && !IS_NPC(killer)) {
        /* Player killed someone */
        int class_bonus;
        if (IS_NPC(ch)) {
            /* Different reputation changes based on killer's alignment */
            if (IS_EVIL(killer)) {
                /* Evil killers gain reputation (infamy) for killing good targets */
                if (IS_GOOD(ch)) {
                    class_bonus = get_class_reputation_modifier(killer, CLASS_REP_COMBAT_KILL, ch);
                    modify_player_reputation(killer, rand_number(2, 4) + class_bonus);
                }
                /* Evil killers gain small reputation for any kill */
                else if (IS_EVIL(ch)) {
                    class_bonus = get_class_reputation_modifier(killer, CLASS_REP_COMBAT_KILL, ch);
                    modify_player_reputation(killer, rand_number(1, 2) + class_bonus);
                }
            } else {
                /* Good/Neutral killers */
                /* Killing good-aligned mobs lowers reputation */
                if (IS_GOOD(ch)) {
                    modify_player_reputation(killer, -rand_number(1, 3));
                }
                /* Killing evil-aligned mobs increases reputation slightly */
                else if (IS_EVIL(ch)) {
                    class_bonus = get_class_reputation_modifier(killer, CLASS_REP_COMBAT_KILL, ch);
                    modify_player_reputation(killer, rand_number(1, 2) + class_bonus);
                }
            }
            /* Killing high-level mobs increases reputation more (for all alignments) */
            if (GET_LEVEL(ch) >= GET_LEVEL(killer) + 5) {
                class_bonus = get_class_reputation_modifier(killer, CLASS_REP_COMBAT_KILL, ch);
                modify_player_reputation(killer, rand_number(1, 3) + class_bonus);
            }
        } else {
            /* Player killed another player - major reputation loss */
            modify_player_reputation(killer, -rand_number(5, 10));
            /* Killing high-reputation players causes even more loss */
            if (GET_REPUTATION(ch) >= 60) {
                modify_player_reputation(killer, -rand_number(5, 15));
            }
        }
    }
    /* Mob killed someone */
    else if (killer && IS_NPC(killer) && killer->ai_data) {
        if (IS_NPC(ch)) {
            /* Mob killing high-reputation mob gains reputation */
            if (GET_MOB_REPUTATION(ch) >= 50) {
                killer->ai_data->reputation = MIN(killer->ai_data->reputation + rand_number(2, 4), 100);
            }
        }
        /* Killing players handled above in bragging section */
    }

    /* Reputation loss for the victim (dynamic reputation system) */
    if (CONFIG_DYNAMIC_REPUTATION && !IS_NPC(ch)) {
        /* Dying lowers reputation slightly */
        modify_player_reputation(ch, -rand_number(1, 3));
    } else if (ch->ai_data) {
        /* Mob dying lowers its reputation */
        ch->ai_data->reputation = MAX(0, ch->ai_data->reputation - rand_number(1, 2));
    }

    /* Mob emotion updates and mourning system (experimental feature) */
    if (CONFIG_MOB_CONTEXTUAL_SOCIALS) {
        /* Notify mobs in the room about the death for mourning/emotion updates */
        struct char_data *witness, *next_witness;
        bool should_mourn;

        /* Use safe iteration - mourning can trigger extraction */
        for (witness = world[IN_ROOM(ch)].people; witness; witness = next_witness) {
            next_witness = witness->next_in_room;

            if (!IS_NPC(witness) || !witness->ai_data || witness == ch || witness == killer)
                continue;

            /* Check if witness should mourn (same group, same alignment, high friendship) */
            should_mourn = FALSE;

            /* Group members mourn */
            if (GROUP(witness) && GROUP(ch) && GROUP(witness) == GROUP(ch)) {
                should_mourn = TRUE;
            }
            /* Same alignment with decent relationship */
            else if (GET_ALIGNMENT(witness) * GET_ALIGNMENT(ch) > 0 && witness->ai_data->emotion_friendship >= 40) {
                should_mourn = TRUE;
            }
            /* High compassion mobs mourn anyone non-evil */
            else if (witness->ai_data->emotion_compassion >= 70 && !IS_EVIL(ch)) {
                should_mourn = TRUE;
            }

            if (should_mourn) {
                mob_mourn_death(witness, ch);

                /* Safety check: mourning can trigger extraction */
                if (MOB_FLAGGED(witness, MOB_NOTDEADYET) || PLR_FLAGGED(witness, PLR_NOTDEADYET))
                    continue;

                /* Revalidate ai_data */
                if (!witness->ai_data)
                    continue;
            }

            /* Update witness emotions based on the death */
            if (IS_NPC(ch) && witness->ai_data) {
                /* Witnessing mob death */
                if (GROUP(witness) && GROUP(ch) && GROUP(witness) == GROUP(ch)) {
                    /* Ally died */
                    update_mob_emotion_ally_died(witness, ch);
                } else if (IS_GOOD(witness) && IS_EVIL(ch)) {
                    /* Enemy died - good witness might feel satisfaction */
                    if (witness->ai_data) {
                        witness->ai_data->emotion_happiness =
                            URANGE(0, witness->ai_data->emotion_happiness + rand_number(5, 15), 100);
                    }
                }
            } else if (!IS_NPC(ch) && witness->ai_data) {
                /* Witnessing player death - new trigger for Environmental 2.2 */
                update_mob_emotion_witnessed_death(witness, ch, killer);
            }
        }
    }

    /* Check for bounty quest failures - if this mob was a bounty target for someone else */
    if (IS_NPC(ch)) {
        fail_bounty_quest(ch, killer);

        /* Emotion trigger: Quest betrayal if a questmaster is killed (Quest-Related 2.4) */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && killer && GET_MOB_RNUM(ch) != NOBODY && GET_MOB_RNUM(ch) >= 0 &&
            GET_MOB_RNUM(ch) <= top_of_mobt && mob_index[GET_MOB_RNUM(ch)].func == questmaster) {
            /* Notify witnesses that a questmaster was killed */
            struct char_data *witness, *next_witness;
            for (witness = world[IN_ROOM(ch)].people; witness; witness = next_witness) {
                next_witness = witness->next_in_room;
                if (IS_NPC(witness) && witness->ai_data && witness != ch && witness != killer) {
                    update_mob_emotion_quest_betrayal(witness, killer);
                }
            }
        }
    }

    /* Alert Group if Applicable */
    if (GROUP(ch))
        send_to_group(ch, GROUP(ch), "%s morreu.\r\n", GET_NAME(ch));

    update_pos(ch);

    /*************************************************************************
     * Sistema de Genética: O mob morreu. Chamamos a função de evolução.     *
     * Não atualizamos a genética de mobs encantados/evocados (AFF_CHARM),   *
     * pois são criaturas temporárias e sua morte não representa seleção     *
     * natural da espécie.                                                    *
     *************************************************************************/
    if (IS_NPC(ch) && ch->ai_data && !AFF_FLAGGED(ch, AFF_CHARM)) {
        update_mob_prototype_genetics(ch);
    }
    /*************************************************************************
     * Fim do Bloco de Genética                                              *
     *************************************************************************/

    make_corpse(ch);

    /* DISABLED: Magic stone generation temporarily disabled for testing */
    /* Create magic stone if this mob is a target for any active kill quest */
    /* if (ch && IS_NPC(ch) && has_active_kill_quest_for_mob(GET_MOB_VNUM(ch))) {
        make_magic_stone(ch, char_script_id(ch));
    } */

    // -- jr - Mar 17, 2000 * Enhancement of player death
    if (!IS_NPC(ch)) {
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            SET_BIT_AR(PLR_FLAGS(ch), PLR_GHOST);
            GET_COND(ch, HUNGER) = -1;
            GET_COND(ch, THIRST) = -1;
        }
        GET_HIT(ch) = 0;
        GET_MANA(ch) = 0;
        GET_MOVE(ch) = 0;
        GET_LOADROOM(ch) = CONFIG_DEAD_START;
    }

    /* Check if any mobs in the room are looking for keys that this mob carried */
    if (IS_NPC(ch) && ch->carrying) {
        struct char_data *room_mob;
        for (room_mob = world[IN_ROOM(ch)].people; room_mob; room_mob = room_mob->next_in_room) {
            if (IS_NPC(room_mob) && room_mob->ai_data && room_mob->ai_data->current_goal == GOAL_COLLECT_KEY &&
                room_mob->ai_data->goal_target_mob_rnum == GET_MOB_RNUM(ch)) {

                /* Check if the dying mob has the key this mob is looking for */
                struct obj_data *obj;
                obj_vnum target_key = room_mob->ai_data->goal_item_vnum;

                for (obj = ch->carrying; obj; obj = obj->next_content) {
                    if (GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == target_key) {
                        /* Found the key! The mob will pick it up when it's dropped by extract_char */
                        act("$n parece empolgado ao ver que $N tinha o que procurava.", FALSE, room_mob, 0, ch,
                            TO_ROOM);
                        break;
                    }
                }

                /* Also check equipment */
                for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
                    obj = GET_EQ(ch, wear_pos);
                    if (obj && GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == target_key) {
                        act("$n parece empolgado ao ver que $N tinha o que procurava.", FALSE, room_mob, 0, ch,
                            TO_ROOM);
                        break;
                    }
                }
            }
        }
    }

    extract_char(ch);
    if (killer) {
        autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
        autoquest_trigger_check(killer, NULL, NULL, AQ_ROOM_CLEAR);
    }
}

void die(struct char_data *ch, struct char_data *killer)
{
    gain_exp(ch, -(GET_EXP(ch) / 2));
    if (!IS_NPC(ch)) {
        REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
        REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
        GET_DEATH(ch)++;
    }
    raw_kill(ch, killer);
}

static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim)
{
    int share, hap_share;
    share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base));
    if ((IS_HAPPYHOUR) && (IS_HAPPYEXP)) {
        /* This only reports the correct amount - the calc is done in gain_exp
         */
        hap_share = share + (int)((float)share * ((float)HAPPY_EXP / (float)(100)));
        share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, hap_share));
    }
    if (share > 1)
        send_to_char(ch, "Você recebe sua parte da experiência -- %s pontos.\r\n", format_number_br(share));
    else
        send_to_char(ch, "Você recebe sua parte da experiência -- um mísero ponto!\r\n");
    gain_exp(ch, share);
    change_alignment(ch, victim);
}

static void group_gain(struct char_data *ch, struct char_data *victim)
{
    int tot_members = 0, base, tot_gain;
    struct char_data *k;

    /* Safety check: ensure group and members exist */
    if (!GROUP(ch) || !GROUP(ch)->members || !GROUP(ch)->members->iSize) {
        return;
    }

    while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
        if (IN_ROOM(ch) == IN_ROOM(k))
            tot_members++;
    /* round up to the nearest tot_members */
    tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;
    /* prevent illegal xp creation when killing players */
    if (!IS_NPC(victim))
        tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, tot_gain);
    if (tot_members >= 1)
        base = MAX(1, tot_gain / tot_members);
    else
        base = 0;
    while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL) {
        if (IN_ROOM(k) == IN_ROOM(ch)) {
            perform_group_gain(k, base, victim);
            /******************************************************************
             * APRENDIZAGEM DE GRUPO: Reforço Positivo
             * Se o membro for um NPC com genética, a vitória em grupo
             * reforça a sua tendência de se agrupar.
             ******************************************************************/
            if (IS_NPC(k) && k->ai_data) {
                k->ai_data->genetics.group_tendency += 2;
                k->ai_data->genetics.group_tendency = MIN(GET_GENGROUP(k), 100);
            }
        }
    }
}

static void solo_gain(struct char_data *ch, struct char_data *victim)
{
    int exp, happy_exp;
    exp = MIN(CONFIG_MAX_EXP_GAIN, GET_EXP(victim) / 3);
    /* Calculate level-difference bonus */
    if (IS_NPC(ch))
        exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
    else
        exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
    exp = MAX(exp, 1);
    if (IS_HAPPYHOUR && IS_HAPPYEXP) {
        happy_exp = exp + (int)((float)exp * ((float)HAPPY_EXP / (float)(100)));
        exp = MAX(happy_exp, 1);
    }

    if (exp > 1)
        send_to_char(ch, "Você recebeu %s pontos de experiência.\r\n", format_number_br(exp));
    else
        send_to_char(ch, "Você recebeu um mísero ponto de experiência.\r\n");
    gain_exp(ch, exp);
    change_alignment(ch, victim);
}

static char *replace_string(const char *str, const struct attack_hit_type *attack_type)
{
    static char buf[256];
    char *cp = buf;
    for (; *str; str++) {
        if (*str == '#') {
            switch (*(++str)) {
                case 'W':
                    for (const char *weapon = attack_type->plural; *weapon; *(cp++) = *(weapon++))
                        ;
                    break;
                case 'w':
                    for (const char *weapon = attack_type->singular; *weapon; *(cp++) = *(weapon++))
                        ;
                    break;
                case 'n':
                    for (const char *weapon = attack_type->tipo; *weapon; *(cp++) = *(weapon++))
                        ;
                    break;
                default:
                    *(cp++) = '#';
                    break;
            }
        } else
            *(cp++) = *str;
        *cp = 0;
    } /* For */

    return (buf);
}

/* message for doing damage with a weapon */
static void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type)
{
    char *buf;
    int msgnum;
    static struct dam_weapon_type {
        const char *to_room;
        const char *to_char;
        const char *to_victim;
    } dam_weapons[] = {

        {/* 0 */
         "$n tenta #W $N, mas erra.", "\tyVocê tenta #W $N, mas erra.\tn", "\tr$n tenta #W você, mas erra.\tn"},

        {/* 1 */
         "$n faz cócegas em $N tentando #W $L.", "\tyVocê faz cócegas em $N ao tentar #W $L.\tn",
         "\tr$n faz cócegas em você, tentando lhe #W.\tn"},

        {/* 2 */
         "Com muita dificuldade, $n #w $N.", "\tyCom muita dificuldade, você #w $N.\tn",
         "\trCom muita dificuldade, $n #w você.\tn"},

        {/* 3 */
         "$n #w $N.", "\tyVocê #w $N.\tn", "\tr$n #w você.\tn"},

        {/* 4 */
         "$n #w $N com força.", "\tyVocê #w $N com força.\tn", "\tr$n #w você com força.\tn"},

        {/* 5 */
         "$n #w $N com muita força.", "\tyVocê #w $N com muita força.\tn", "\tr$n #w você com muita força.\tn"},

        {/* 6 */
         "$n #w $N extremamente forte.", "\tyVocê #w $N extremamente forte.\tn",
         "\tr$n #w você extremamente forte.\tn"},

        {/* 7 */
         "$n massacra $N com #n.", "\tyVocê massacra $N com sua #n.\tn", "\tr$n massacra você com sua #n.\tn"},

        {/* 8 */
         "$n esmigalha $N com um forte #n!", "\tyVocê esmigalha $N com um forte #n!\tn",
         "\tr$n esmigalha você com um forte #n!\tn"},

        {/* 9 */
         "$n estraçalha $N com um poderoso #n!", "\tyVocê estraçalha $N com um poderoso #n!\tn",
         "\tr$n estraçalha você com um poderoso #n!\tn"},

        {/* 10 */
         "$n ANIQUILA $N com um #n mortal!!", "\tyVocê ANIQUILA $N com um #n mortal!!\tn",
         "\tr$n ANIQUILA você com um #n mortal!!\tn"},

        {/* 11 */
         "$n EXTERMINA $N com um #n devastador!!", "\tyVocê EXTERMINA $N com um #n devastador!!\tn",
         "\tr$n EXTERMINA você com um #n devastador!!\tn"}};

    w_type -= TYPE_HIT; /* Change to base of table with text */

    int vh = GET_MAX_HIT(victim);

    if (dam == 0)
        msgnum = 0;
    else if (dam <= vh * .01)
        msgnum = 1;
    else if (dam <= vh * .02)
        msgnum = 2;
    else if (dam <= vh * .05)
        msgnum = 3;
    else if (dam <= vh * .08)
        msgnum = 4;
    else if (dam <= vh * .12)
        msgnum = 5;
    else if (dam <= vh * .28)
        msgnum = 6;
    else if (dam <= vh * .35)
        msgnum = 7;
    else if (dam <= vh * .50)
        msgnum = 8;
    else if (dam <= vh * .70)
        msgnum = 9;
    else if (dam <= vh * .90)
        msgnum = 10;
    else
        msgnum = 11;

    /*if (dam == 0)
        msgnum = 0;
    else if (dam <= 2)
        msgnum = 1;
    else if (dam <= 4)
        msgnum = 2;
    else if (dam <= 6)
        msgnum = 3;
    else if (dam <= 10)
        msgnum = 4;
    else if (dam <= 14)
        msgnum = 5;
    else if (dam <= 19)
        msgnum = 6;
    else if (dam <= 23)
        msgnum = 7;
    else
        msgnum = 8;*/

    /* damage message to onlookers */
    buf = replace_string(dam_weapons[msgnum].to_room, &attack_hit_text[w_type]);
    act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);
    /* damage message to damager */
    if (GET_LEVEL(ch) >= LVL_IMMORT || PRF_FLAGGED(ch, PRF_VIEWDAMAGE))
        send_to_char(ch, "(%d) ", dam);
    buf = replace_string(dam_weapons[msgnum].to_char, &attack_hit_text[w_type]);
    act(buf, FALSE, ch, NULL, victim, TO_CHAR);
    send_to_char(ch, CCNRM(ch, C_CMP));
    /* damage message to damagee */
    if (GET_LEVEL(victim) >= LVL_IMMORT || PRF_FLAGGED(victim, PRF_VIEWDAMAGE))
        send_to_char(victim, "\tR(%d)", dam);
    buf = replace_string(dam_weapons[msgnum].to_victim, &attack_hit_text[w_type]);
    act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
    send_to_char(victim, CCNRM(victim, C_CMP));
}

/* message for doing damage with a spell or skill. Also used for weapon
   damage on miss and death blows. */
int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype)
{
    int i, j, nr;
    struct message_type *msg;
    struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);
    /* @todo restructure the messages library to a pointer based system as
       opposed to the current cyclic location system. */
    for (i = 0; i < MAX_MESSAGES; i++) {
        if (fight_messages[i].a_type == attacktype) {
            nr = dice(1, fight_messages[i].number_of_attacks);
            for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
                msg = msg->next;
            if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMPL)) {
                act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
                act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
            } else if (dam != 0) {
                /*
                 * Don't send redundant color codes for TYPE_SUFFERING & other types
                 * of damage without attacker_msg.
                 */
                if (GET_POS(vict) == POS_DEAD) {
                    if (msg->die_msg.attacker_msg) {
                        send_to_char(ch, CCYEL(ch, C_CMP));
                        act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                        send_to_char(ch, CCNRM(ch, C_CMP));
                    }

                    send_to_char(vict, CCRED(vict, C_CMP));
                    act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                    send_to_char(vict, CCNRM(vict, C_CMP));
                    act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
                } else {
                    if (msg->hit_msg.attacker_msg) {
                        /* Display damage value if PRF_VIEWDAMAGE is set */
                        if (GET_LEVEL(ch) >= LVL_IMMORT || PRF_FLAGGED(ch, PRF_VIEWDAMAGE))
                            send_to_char(ch, "(%d) ", dam);
                        send_to_char(ch, CCYEL(ch, C_CMP));
                        act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                        send_to_char(ch, CCNRM(ch, C_CMP));
                    }

                    /* Display damage value to victim if PRF_VIEWDAMAGE is set */
                    if (GET_LEVEL(vict) >= LVL_IMMORT || PRF_FLAGGED(vict, PRF_VIEWDAMAGE))
                        send_to_char(vict, "\tR(%d)", dam);
                    send_to_char(vict, CCRED(vict, C_CMP));
                    act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                    send_to_char(vict, CCNRM(vict, C_CMP));
                    act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
                }
            } else if (ch != vict) { /* Dam == 0 */
                if (msg->miss_msg.attacker_msg) {
                    send_to_char(ch, CCYEL(ch, C_CMP));
                    act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
                    send_to_char(ch, CCNRM(ch, C_CMP));
                }

                send_to_char(vict, CCRED(vict, C_CMP));
                act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
                send_to_char(vict, CCNRM(vict, C_CMP));
                act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
            }
            return (1);
        }
    }
    return (0);
}

/* This function returns the following codes: < 0 Victim died.  = 0 No
   damage. > 0 How much damage done. */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{
    long local_gold = 0, happy_gold = 0;
    char local_buf[256];
    struct char_data *tmp_char;
    struct obj_data *corpse_obj;

    /* Safety checks to prevent segfaults when ch or victim is NULL or invalid */
    if (ch == NULL || victim == NULL)
        return (0);

    /* Check if characters have been marked for extraction */
    if (PLR_FLAGGED(ch, PLR_NOTDEADYET) || MOB_FLAGGED(ch, MOB_NOTDEADYET))
        return (0);
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
        return (0);

    /* Validate rooms - prevent crashes from characters in invalid rooms */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(victim) == NOWHERE)
        return (0);

    if (GET_POS(victim) <= POS_DEAD) {
        /* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
        if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET)) {
            act("$N já está mort$R! Não adianta tentar matá-l$R.", FALSE, ch, NULL, victim, TO_CHAR);
            return (-1);
        }
        log1("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.", GET_NAME(victim),
             GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
        die(victim, ch);
        return (-1); /* -je, 7/7/92 */
    }

    /* peaceful rooms */
    if (ch->nr != real_mobile(DG_CASTER_PROXY) && ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
        send_to_char(ch, "Este lugar é muito calmo, sem violências...\r\n");
        return (0);
    }

    /* shopkeeper and MOB_NOKILL protection */
    if (!ok_damage_shopkeeper(ch, victim) || MOB_FLAGGED(victim, MOB_NOKILL) || PLR_FLAGGED(victim, PLR_TRNS)) {
        send_to_char(ch, "Você não pode lutar.\r\n");
        return (0);
    }

    /* You can't damage an immortal! */
    if (!IS_NPC(victim) && ((GET_LEVEL(victim) >= LVL_IMMORT) && PRF_FLAGGED(victim, PRF_NOHASSLE)))
        dam = 0;
    if (victim != ch) {
        /* Start the attacker fighting the victim */
        if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL)) {
            set_fighting(ch, victim);
            /* Update mob emotions when starting combat (experimental feature) */
            if (CONFIG_MOB_CONTEXTUAL_SOCIALS && IS_NPC(ch)) {
                update_mob_emotion_attacking(ch, victim);
            }
        }
        /* Start the victim fighting the attacker */
        if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
            set_fighting(victim, ch);
            if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
                remember(victim, ch);
            /* Update mob emotions when being attacked (experimental feature) */
            if (CONFIG_MOB_CONTEXTUAL_SOCIALS && IS_NPC(victim)) {
                update_mob_emotion_attacked(victim, ch);
            }
        }
    }

    /* If you attack a pet, it hates your guts */
    if (victim->master == ch)
        stop_follower(victim);
    /* If the attacker is invisible, he becomes visible */
    if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_HIDE))
        appear(ch);

    /* Check for stoneskin protection */
    apply_stoneskin_protection(victim, &dam);

    /* Cut damage in half if victim has sanct, to a minimum 1 */
    if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
        dam /= 2;
    if (AFF_FLAGGED(victim, AFF_GLOOMSHIELD) && dam >= 3)
        dam /= 3;
    /* Check for PK if this is not a PK MUD */
    if (!CONFIG_PK_ALLOWED) {
        check_killer(ch, victim);
        if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
            dam /= 2;
    }

    /* Set the maximum damage per round and subtract the hit points */
    dam = MAX(dam, 0);
    /* Aura spells (MAG_AURA) cannot deal more damage than the victim has HP
     * This prevents aura reflection from overkilling */
    if (is_aura_spell(attacktype) && (dam > GET_HIT(victim)))
        dam = MAX(GET_HIT(victim), 0);
    GET_HIT(victim) -= dam;

    /* EMOTION SYSTEM: Update pain based on damage taken (experimental feature) */
    if (CONFIG_MOB_CONTEXTUAL_SOCIALS && IS_NPC(victim) && victim->ai_data && dam > 0) {
        int pain_amount = 0;
        int damage_percent = (dam * 100) / MAX(GET_MAX_HIT(victim), 1);

        /* Pain scales with damage percentage */
        if (damage_percent >= CONFIG_EMOTION_PAIN_DAMAGE_MASSIVE_THRESHOLD) {
            /* Massive damage (50%+ of max HP) */
            pain_amount = rand_number(CONFIG_EMOTION_PAIN_MASSIVE_MIN, CONFIG_EMOTION_PAIN_MASSIVE_MAX);
        } else if (damage_percent >= CONFIG_EMOTION_PAIN_DAMAGE_HEAVY_THRESHOLD) {
            /* Heavy damage (25-49% of max HP) */
            pain_amount = rand_number(CONFIG_EMOTION_PAIN_HEAVY_MIN, CONFIG_EMOTION_PAIN_HEAVY_MAX);
        } else if (damage_percent >= CONFIG_EMOTION_PAIN_DAMAGE_MODERATE_THRESHOLD) {
            /* Moderate damage (10-24% of max HP) */
            pain_amount = rand_number(CONFIG_EMOTION_PAIN_MODERATE_MIN, CONFIG_EMOTION_PAIN_MODERATE_MAX);
        } else if (damage_percent >= CONFIG_EMOTION_PAIN_DAMAGE_MINOR_THRESHOLD) {
            /* Light damage (5-9% of max HP) */
            pain_amount = rand_number(CONFIG_EMOTION_PAIN_MINOR_MIN, CONFIG_EMOTION_PAIN_MINOR_MAX);
        } else {
            /* Minimal damage (< 5% of max HP) */
            pain_amount = rand_number(CONFIG_EMOTION_PAIN_MINOR_MIN, CONFIG_EMOTION_PAIN_MINOR_MAX);
        }

        adjust_emotion(victim, &victim->ai_data->emotion_pain, pain_amount);
    }

    /* Gain exp for the hit */
    if (ch != victim)
        gain_exp(ch, GET_LEVEL(victim) * dam);
    update_pos(victim);
    /* skill_message sends a message from the messages file in lib/misc.
       dam_message just sends a generic "You hit $n extremely hard.".
       skill_message is preferable to dam_message because it is more
       descriptive. If we are _not_ attacking with a weapon (i.e. a spell),
       always use skill_message. If we are attacking with a weapon: If this is
       a miss or a death blow, send a skill_message if one exists; if not,
       default to a dam_message. Otherwise, always send a dam_message.
       Note: Aura spells (MAG_AURA) skip skill_message since the new aura
       reflection system in this file already sends custom messages. */
    if (!IS_WEAPON(attacktype) && !is_aura_spell(attacktype))
        skill_message(dam, ch, victim, attacktype);
    else {
        if (GET_POS(victim) == POS_DEAD || dam == 0) {
            if (!skill_message(dam, ch, victim, attacktype))
                dam_message(dam, ch, victim, attacktype);
        } else {
            dam_message(dam, ch, victim, attacktype);
        }
    }

    dam = damage_mtrigger(ch, victim, dam, attacktype);
    if (dam == -1) {
        return (0);
    }
    if (victim != ch) {
        /* Start the attacker fighting the victim */
        if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
            set_fighting(ch, victim);
    }

    /* Use send_to_char -- act() doesn't send message if you are DEAD. */
    switch (GET_POS(victim)) {
        case POS_MORTALLYW:
            act("$n está mortalmente ferid$r, e vai morrer logo, se não for medicad$r.", TRUE, victim, 0, 0, TO_ROOM);
            act("Você está mortalmente ferid$r, e vai morrer logo, se não medicad$r.", TRUE, victim, 0, 0,
                TO_CHAR | TO_SLEEP);
            break;
        case POS_INCAP:
            act("$n está incapacitad$r e vai morrer lentamente, se não for medicad$r.", TRUE, victim, 0, 0, TO_ROOM);
            act("Você está incapacitad$r e vai morrer lentamente, se não for medicad$r.", TRUE, victim, NULL, NULL,
                TO_CHAR | TO_SLEEP);
            break;
        case POS_STUNNED:
            act("$n está atordoad$r, mas provavelmente irá recuperar a consciência novamente.", TRUE, victim, 0, 0,
                TO_ROOM);
            act("Você está atordoad$r, mas provavelmente irá recuperar a consciência novamente.", TRUE, victim, 0, 0,
                TO_CHAR | TO_SLEEP);
            break;
        case POS_DEAD:
            act("$n está mort$r!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
            act("Você está mort$r!  Sinto muito...\r\n", TRUE, victim, 0, 0, TO_CHAR | TO_SLEEP);
            break;
        default: /* >= POSITION SLEEPING */
            if (dam > (GET_MAX_HIT(victim) / 4))
                send_to_char(victim, "Isto realmente DOEU!\r\n");

            if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
                send_to_char(victim, "\trVocê espera que seus ferimentos parem de %sSANGRAR tanto!%s\tn\r\n",
                             CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
            }

            /************************************************************************************
             * Lógica de Genética REVISADA: Todos os mobs podem ser fujões.                     *                      *
             * A flag MOB_WIMPY agora dá um bónus de "medo", em vez de ser um interruptor.     *
             * ************************************************************************************/
            if (ch != victim && IS_NPC(victim) && GET_POS(victim) > POS_STUNNED) {
                int flee_threshold = 0;
                int base_wimpy = 0;
                int emotion_modifier = 0;

                /* 1. O mob tem a flag MOB_WIMPY? Se sim, ele já tem uma base de medo. */
                if (MOB_FLAGGED(victim, MOB_WIMPY)) {
                    base_wimpy = 20; /* Valor base de 20% de vida para fuga. */
                }

                /* 2. Adicionamos a tendência genética à base (scaled to 0-40% to make room for emotions). */
                if (victim->ai_data) {
                    /* Scale wimpy_tendency (0-100) to 0-40% range for balanced flee behavior */
                    int genetic_component = (victim->ai_data->genetics.wimpy_tendency * 40) / 100;
                    flee_threshold = base_wimpy + genetic_component;

                    /* 3. EMOTION SYSTEM: Adjust flee threshold based on emotions (hybrid system) */
                    if (CONFIG_MOB_CONTEXTUAL_SOCIALS) {
                        struct char_data *attacker = FIGHTING(victim);
                        int effective_fear, effective_courage, effective_horror;

                        /* Use hybrid emotion system: mood + relationship toward attacker */
                        if (attacker) {
                            effective_fear = get_effective_emotion_toward(victim, attacker, EMOTION_TYPE_FEAR);
                            effective_courage = get_effective_emotion_toward(victim, attacker, EMOTION_TYPE_COURAGE);
                            effective_horror = get_effective_emotion_toward(victim, attacker, EMOTION_TYPE_HORROR);
                        } else {
                            /* No specific attacker, use mood only */
                            effective_fear = victim->ai_data->emotion_fear;
                            effective_courage = victim->ai_data->emotion_courage;
                            effective_horror = victim->ai_data->emotion_horror;
                        }

                        /* High fear increases flee threshold (flee sooner) */
                        if (effective_fear >= CONFIG_EMOTION_FLEE_FEAR_HIGH_THRESHOLD) {
                            emotion_modifier += CONFIG_EMOTION_FLEE_FEAR_HIGH_MODIFIER; /* Flee at +15% HP */
                        } else if (effective_fear >= CONFIG_EMOTION_FLEE_FEAR_LOW_THRESHOLD) {
                            emotion_modifier += CONFIG_EMOTION_FLEE_FEAR_LOW_MODIFIER; /* Flee at +10% HP */
                        }

                        /* High courage reduces flee threshold (flee later) */
                        if (effective_courage >= CONFIG_EMOTION_FLEE_COURAGE_HIGH_THRESHOLD) {
                            emotion_modifier -= CONFIG_EMOTION_FLEE_COURAGE_HIGH_MODIFIER; /* Flee at -15% HP */
                        } else if (effective_courage >= CONFIG_EMOTION_FLEE_COURAGE_LOW_THRESHOLD) {
                            emotion_modifier -= CONFIG_EMOTION_FLEE_COURAGE_LOW_MODIFIER; /* Flee at -10% HP */
                        }

                        /* Horror overrides other emotions */
                        if (effective_horror >= CONFIG_EMOTION_FLEE_HORROR_THRESHOLD) {
                            emotion_modifier += CONFIG_EMOTION_FLEE_HORROR_MODIFIER; /* Panic flee at +25% HP */
                        }

                        flee_threshold += emotion_modifier;
                    }
                } else {
                    flee_threshold = base_wimpy;
                }
                /* 4. Garante que o limiar não passa de um valor razoável (10-90%) */
                flee_threshold = MIN(MAX(flee_threshold, 10), 90);
                /* 5. Compara com a vida atual e tenta fugir se necessário. */
                if (flee_threshold > 0) {
                    /* Fuga DETERMINÍSTICA: Baseada na genética e na flag wimpy. */
                    if (GET_HIT(victim) < (GET_MAX_HIT(victim) * flee_threshold) / 100) {
                        /************************************************************
                         * LÓGICA DE DESERÇÃO DE GRUPO (IA EGOÍSTA)
                         * O medo pode levar à traição. Este é o local ideal.
                         ************************************************************/
                        if (GROUP(victim) && GROUP_LEADER(GROUP(victim)) != victim) {
                            int abandon_chance = GET_GENWIMPY(victim);

                            /* Emotion modifiers for group loyalty when hurt/scared */
                            if (CONFIG_MOB_CONTEXTUAL_SOCIALS && victim->ai_data) {
                                /* High loyalty makes mobs stay in group even when hurt */
                                if (victim->ai_data->emotion_loyalty >= CONFIG_EMOTION_GROUP_LOYALTY_HIGH_THRESHOLD) {
                                    abandon_chance -= 30; /* -30 to abandon chance (more loyal) */
                                }
                                /* Low loyalty makes mobs abandon group when scared */
                                else if (victim->ai_data->emotion_loyalty <
                                         CONFIG_EMOTION_GROUP_LOYALTY_LOW_THRESHOLD) {
                                    abandon_chance += 30; /* +30 to abandon chance (less loyal) */
                                }

                                /* Ensure abandon_chance stays reasonable */
                                abandon_chance = MAX(0, MIN(abandon_chance, 150));
                            }

                            /* A chance de abandonar é proporcional à sua tendência de fugir (Wimpy). */
                            if (rand_number(1, 150) <= abandon_chance) {
                                act("$n entra em pânico, abandona os seus companheiros e foge!", TRUE, victim, 0, 0,
                                    TO_ROOM);
                                send_to_char(victim, "Você não aguenta mais e abandona o seu grupo!\r\n");

                                /* Aplica a penalidade de aprendizagem ao gene de grupo. */
                                if (victim->ai_data) {
                                    victim->ai_data->genetics.group_tendency -= 1;
                                    if (GET_GENGROUP(victim) < 0)
                                        victim->ai_data->genetics.group_tendency = 0;
                                }

                                leave_group(victim); /* Sai do grupo. */
                            }
                        }
                        /************************************************************/

                        do_flee(victim, NULL, 0, 0);

                        /* Set frustration timers after fleeing */
                        if (IS_NPC(victim) && victim->ai_data) {
                            victim->ai_data->quest_posting_frustration_timer = 6; /* 6 ticks (~1 minute) */

                            /* For sentinel mobs, also set duty frustration timer to prevent immediate return to guard
                             * post */
                            if (MOB_FLAGGED(victim, MOB_SENTINEL)) {
                                victim->ai_data->duty_frustration_timer = 6; /* 6 ticks (~1 minute) */
                            }
                        }
                    }
                } else {
                    /* Fuga de PÂNICO: Para mobs "corajosos" (threshold = 0) em estado crítico. */
                    /* Se a vida estiver abaixo de 10%, há uma pequena chance de pânico. */
                    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 10)) {
                        if (rand_number(1, 100) <= 5) { /* Chance de 5% de entrar em pânico */
                            act("$n olha para os seus ferimentos, entra em pânico e tenta fugir!", TRUE, victim, 0, 0,
                                TO_ROOM);
                            do_flee(victim, NULL, 0, 0);

                            /* Set frustration timers after panic fleeing */
                            if (IS_NPC(victim) && victim->ai_data) {
                                victim->ai_data->quest_posting_frustration_timer = 6; /* 6 ticks (~1 minute) */

                                /* For sentinel mobs, also set duty frustration timer to prevent immediate return to
                                 * guard post */
                                if (MOB_FLAGGED(victim, MOB_SENTINEL)) {
                                    victim->ai_data->duty_frustration_timer = 6; /* 6 ticks (~1 minute) */
                                }
                            }
                        }
                    }
                }
            }

            if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) && GET_HIT(victim) < GET_WIMP_LEV(victim) &&
                GET_HIT(victim) > 0) {
                send_to_char(victim, "Você se desespera e tenta fugir!\r\n");
                do_flee(victim, NULL, 0, 0);
            }
            break;
    }

    /* Help out poor linkless people who are attacked */
    if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
        do_flee(victim, NULL, 0, 0);
        if (!FIGHTING(victim)) {
            act("Você é resgatad$r por forças divinas.", FALSE, victim, 0, 0, TO_CHAR);
            act("$n é resgatad$r por forças divinas.", FALSE, victim, 0, 0, TO_ROOM);
            GET_WAS_IN(victim) = IN_ROOM(victim);
            char_from_room(victim);
            char_to_room(victim, 0);
        }
    }

    /* stop someone from fighting if they're stunned or worse */
    if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
        stop_fighting(victim);
    /* Uh oh.  Victim died. */
    if (GET_POS(victim) == POS_DEAD) {
        if (ch != victim && (IS_NPC(victim) || victim->desc)) {
            if (GROUP(ch))
                group_gain(ch, victim);
            else
                solo_gain(ch, victim);
        }

        if (!IS_NPC(victim)) {
            mudlog(BRF, MAX(LVL_IMMORT, MAX(GET_INVIS_LEV(ch), GET_INVIS_LEV(victim))), TRUE, "%s killed by %s at %s",
                   GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
            if (MOB_FLAGGED(ch, MOB_MEMORY))
                forget(ch, victim);
        }
        /* Cant determine GET_GOLD on corpse, so do now and store */
        if (IS_NPC(victim)) {
            if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD)) {
                happy_gold = (long)(GET_GOLD(victim) * (((float)(HAPPY_GOLD)) / (float)100));
                happy_gold = MAX(0, happy_gold);
                increase_gold(victim, happy_gold);
            }
            local_gold = GET_GOLD(victim);
            sprintf(local_buf, "%ld", (long)local_gold);
        }

        die(victim, ch);
        if (GROUP(ch) && (local_gold > 0) && PRF_FLAGGED(ch, PRF_AUTOSPLIT)) {
            generic_find("corpo", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
            if (corpse_obj) {
                do_get(ch, "all.moeda corpo", 0, 0);
                do_split(ch, local_buf, 0, 0);
            }
            /* need to remove the gold from the corpse */
        } else if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOGOLD)) {
            do_get(ch, "all.moeda corpo", 0, 0);
        }
        if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
            do_get(ch, "all corpo", 0, 0);
        }
        /* AUTOEXAM must run before AUTOSAC so player can see corpse contents before it's sacrificed */
        if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOEXAM)) {
            do_examine(ch, "corpo", 0, 0);
        }
        if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSAC)) {
            do_sac(ch, "corpo", 0, 0);
        }
        return (-1);
    }
    return (dam);
}

/* Calculate the THAC0 of the attacker. 'victim' currently isn't used but
   you could use it for special cases like weapons that hit evil creatures
   easier or a weapon that always misses attacking an animal. */
int compute_thaco(struct char_data *ch, struct char_data *victim)
{
    int calc_thaco;
    int wpnprof = 0;
    int nham = 0;
    struct obj_data *wielded;

    if (!IS_NPC(ch)) {
        calc_thaco = thaco(GET_CLASS(ch), GET_LEVEL(ch));
        wielded = GET_EQ(ch, WEAR_WIELD);
        if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
            wpnprof = get_weapon_prof(ch, wielded);
        nham = get_nighthammer(ch, true);
    } else /* THAC0 for monsters is set in the HitRoll */
        calc_thaco = 20;
    calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
    calc_thaco -= GET_HITROLL(ch);
    calc_thaco -= wpn_prof[wpnprof].to_hit;
    calc_thaco -= nighthammer_info[nham].to_hit;
    calc_thaco -= (int)((GET_INT(ch) - 13) / 1.5); /* Intelligence helps! */
    calc_thaco -= (int)((GET_WIS(ch) - 13) / 1.5); /* So does wisdom */
    return calc_thaco;
}

void hit(struct char_data *ch, struct char_data *victim, int type)
{
    struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
    int w_type, victim_ac, calc_thaco, dam, diceroll;
    struct affected_type af;
    int wpnprof = 0, nham = 0;
    /* Check that the attacker and victim exist */
    if (!ch || !victim)
        return;

    /* Reputation penalty for attacking allies/group members (dynamic reputation system) */
    if (CONFIG_DYNAMIC_REPUTATION && !IS_NPC(ch) && !IS_NPC(victim)) {
        /* Check if they share same master (grouped) */
        if ((ch->master && victim->master && ch->master == victim->master) || (ch->master && ch->master == victim) ||
            (victim->master && victim->master == ch)) {
            /* Attacking a group member severely damages reputation */
            modify_player_reputation(ch, -rand_number(5, 10));
        }
    }

    /* check if the character has a fight trigger */
    fight_mtrigger(ch);
    /* Do some sanity checking, in case someone flees, etc. */
    if (IN_ROOM(ch) != IN_ROOM(victim)) {
        if (FIGHTING(ch) && FIGHTING(ch) == victim)
            stop_fighting(ch);
        return;
    }

    /* If this is the start of combat, observe equipment (mob AI feature) */
    if (IS_NPC(ch) && ch->ai_data && !FIGHTING(ch)) {
        /* 20% chance to observe equipment during the first round of combat */
        if (rand_number(1, 100) <= 20) {
            observe_combat_equipment(ch, victim);
        }
    }

    /* Find the weapon type (for display purposes only) */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
        w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
    else {
        if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
            w_type = ch->mob_specials.attack_type + TYPE_HIT;
        else
            w_type = TYPE_HIT;
    }

    if (!IS_NPC(ch)) {
        if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
            wpnprof = get_weapon_prof(ch, wielded);
        nham = get_nighthammer(ch, true);
    }
    (void)wpnprof; /* Suppress unused variable warning */

    /* Calculate chance of hit. Lower THAC0 is better for attacker. */
    calc_thaco = compute_thaco(ch, victim);

    /* HYBRID EMOTION SYSTEM: Pain affects accuracy (THAC0) for NPCs */
    if (CONFIG_MOB_CONTEXTUAL_SOCIALS && IS_NPC(ch) && ch->ai_data) {
        int pain_level = ch->ai_data->emotion_pain; /* Pain is mood-based */
        if (pain_level >= CONFIG_EMOTION_COMBAT_PAIN_HIGH_THRESHOLD) {
            /* High pain: severe accuracy penalty (default +4 to THAC0 = worse) */
            calc_thaco += CONFIG_EMOTION_COMBAT_PAIN_ACCURACY_PENALTY_HIGH;
        } else if (pain_level >= CONFIG_EMOTION_COMBAT_PAIN_MODERATE_THRESHOLD) {
            /* Moderate pain: moderate accuracy penalty (default +2 to THAC0) */
            calc_thaco += CONFIG_EMOTION_COMBAT_PAIN_ACCURACY_PENALTY_MOD;
        } else if (pain_level >= CONFIG_EMOTION_COMBAT_PAIN_LOW_THRESHOLD) {
            /* Low pain: minor accuracy penalty (default +1 to THAC0) */
            calc_thaco += CONFIG_EMOTION_COMBAT_PAIN_ACCURACY_PENALTY_LOW;
        }
    }

    // check to see if the victim has prot from evil on, and if the
    // attacker
    // is in fact evil
    // and detriment the attackers thaco roll if both are true
    if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && (GET_ALIGNMENT(ch) < -350)) {
        calc_thaco = calc_thaco + 2;
    }

    // this is optional if you decide to add the protection from good
    //
    //
    //
    //
    //
    //
    // spell
    // below

    // check to see if victim has protection from good on, and if the
    //
    //
    //
    //
    //
    //
    // attacker
    //
    //
    // is in fact good
    // add detriment to the attackers thaco roll if both are true
    if (AFF_FLAGGED(victim, AFF_PROTECT_GOOD) && (GET_ALIGNMENT(ch) > 350)) {
        calc_thaco = calc_thaco + 2;
    }
    if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && (GET_ALIGNMENT(ch) < -350)) {
        calc_thaco = calc_thaco + 2;
    }
    /* Calculate the raw armor including magic armor.  Lower AC is better for
       defender. */
    victim_ac = compute_armor_class(victim) / 10;
    // check to see if victim is flagged with protection from evil,
    // and if the
    //
    //
    // attacker is evil
    // and improve the victims armor if both are true
    if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && (GET_ALIGNMENT(ch) < -350)) {
        victim_ac = victim_ac - 1;
    }

    // this is optional if you decide to add the protection from good
    //
    //
    //
    //
    //
    //
    // spell
    // below
    // check to see if victim is flagged with protection from good,
    // and if the
    //
    //
    // attacker is good
    // and improve the victims armor if both are true
    if (AFF_FLAGGED(victim, AFF_PROTECT_GOOD) && (GET_ALIGNMENT(ch) > 350)) {
        victim_ac = victim_ac - 1;
    }

    /* roll the die and take your chances... */
    diceroll = rand_number(1, 20);
    /* report for debugging if necessary */
    if (CONFIG_DEBUG_MODE >= NRM)
        send_to_char(ch, "\t1Debug:\r\n   \t2Thaco: \t3%d\r\n   \t2AC: \t3%d\r\n   \t2Diceroll: \t3%d\tn\r\n",
                     calc_thaco, victim_ac, diceroll);
    /* Decide whether this is a hit or a miss.  Victim asleep = hit,
       otherwise: 1 = Automatic miss.  2..19 = Checked vs. AC.  20 = Automatic
       hit. */
    if (diceroll == 20 || !AWAKE(victim))
        dam = TRUE;
    else if (diceroll == 1)
        dam = FALSE;
    else
        dam = (calc_thaco - diceroll <= victim_ac);
    if (!dam)
        /* the attacker missed the victim */
        damage(ch, victim, 0, type == SKILL_BACKSTAB ? SKILL_BACKSTAB : w_type);
    else {
        /* okay, we know the guy has been hit.  now calculate damage. Start
           with the damage bonuses: the damroll and strength apply */
        dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
        dam += GET_DAMROLL(ch);
        dam += nighthammer_info[nham].to_dam;
        /* Maybe holding arrow? */
        if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
            /* Add weapon-based damage if a weapon is being wielded */
            dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
        } else {
            /* If no weapon, add bare hand damage instead */
            if (IS_NPC(ch))
                dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
            else
                dam += rand_number(0, 2); /* Max 2 bare hand damage for
                                                                     players */
        }

        /* Include a damage multiplier if victim isn't ready to fight:
           Position sitting 1.33 x normal Position resting 1.66 x normal
           Position sleeping 2.00 x normal Position stunned 2.33 x normal
           Position incap 2.66 x normal Position mortally 3.00 x normal Note,
           this is a hack because it depends on the particular values of the
           POSITION_XXX constants. */
        if (GET_POS(victim) < POS_FIGHTING)
            dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

        /* HYBRID EMOTION SYSTEM: Anger and Pain affect combat effectiveness for NPCs */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && IS_NPC(ch) && ch->ai_data) {
            /* High anger increases damage */
            int effective_anger = get_effective_emotion_toward(ch, victim, EMOTION_TYPE_ANGER);
            if (effective_anger >= CONFIG_EMOTION_COMBAT_ANGER_HIGH_THRESHOLD) {
                /* Anger damage bonus (default 15%) */
                dam += (dam * CONFIG_EMOTION_COMBAT_ANGER_DAMAGE_BONUS) / 100;
            }

            /* Pain reduces damage output */
            int pain_level = ch->ai_data->emotion_pain; /* Pain is mood-based, not relational */
            if (pain_level >= CONFIG_EMOTION_COMBAT_PAIN_HIGH_THRESHOLD) {
                /* High pain: significant damage reduction (default 20%) */
                dam -= (dam * CONFIG_EMOTION_COMBAT_PAIN_DAMAGE_PENALTY_HIGH) / 100;
            } else if (pain_level >= CONFIG_EMOTION_COMBAT_PAIN_MODERATE_THRESHOLD) {
                /* Moderate pain: moderate damage reduction (default 10%) */
                dam -= (dam * CONFIG_EMOTION_COMBAT_PAIN_DAMAGE_PENALTY_MOD) / 100;
            } else if (pain_level >= CONFIG_EMOTION_COMBAT_PAIN_LOW_THRESHOLD) {
                /* Low pain: minor damage reduction (default 5%) */
                dam -= (dam * CONFIG_EMOTION_COMBAT_PAIN_DAMAGE_PENALTY_LOW) / 100;
            }
        }

        /* at least 1 hp damage min per hit */
        dam = MAX(1, dam);
        if (type == SKILL_BACKSTAB)
            damage(ch, victim, dam * backstab_mult(GET_LEVEL(ch)), SKILL_BACKSTAB);
        else
            damage(ch, victim, dam, w_type);
    }
    /*
     * Poisoned weapons - only apply poison if damage was actually dealt.
     * If stoneskin or other protection completely absorbed the attack,
     * the poison shouldn't penetrate.
     */

    if (wielded && dam > 0 && GET_OBJ_TYPE(wielded) == ITEM_WEAPON && OBJ_FLAGGED(wielded, ITEM_POISONED) &&
        !MOB_FLAGGED(victim, MOB_NO_POISON)) {
        new_affect(&af);
        af.spell = SPELL_POISON;
        af.modifier = -2;
        af.location = APPLY_STR;

        if (AFF_FLAGGED(victim, AFF_POISON))
            af.duration = 1;
        else {
            /* Use level-based duration to match MAGIA-POISON help documentation */
            af.duration = GET_LEVEL(ch);
            act("Você se sente doente.", FALSE, victim, 0, ch, TO_CHAR);
            act("$n fica muito doente!", TRUE, victim, 0, ch, TO_ROOM);
        }
        SET_BIT_AR(af.bitvector, AFF_POISON);
        affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
        // remover veneno da arma
        if (rand_number(0, 3) == 0)
            REMOVE_BIT_AR(GET_OBJ_EXTRA(wielded), ITEM_POISONED);
    }

    /* Counter-attack spells - aura shields (MAG_AURA) reflect damage to attacker */
    if (dam > 0 && !has_aura_shield(ch) && has_aura_shield(victim)) {
        /* Victim has an aura shield and attacker does not - reflect damage */
        int victim_aura = get_aura_shield_spell(victim);
        if (victim_aura > 0) {
            int saved = mag_savingthrow(ch, SAVING_SPELL, GET_LEVEL(ch));
            float ratio = get_aura_reflect_ratio(victim_aura, saved);
            int reflect_dam = (int)(dam * ratio);

            if (reflect_dam > 0) {
                /* Send reflection messages before damage is applied */
                /* Display damage value if PRF_VIEWDAMAGE is set for attacker (ch) */
                if (GET_LEVEL(ch) >= LVL_IMMORT || PRF_FLAGGED(ch, PRF_VIEWDAMAGE))
                    send_to_char(ch, "\tR(%d)\tn ", reflect_dam);
                /* Display damage value if PRF_VIEWDAMAGE is set for victim (aura owner) */
                if (GET_LEVEL(victim) >= LVL_IMMORT || PRF_FLAGGED(victim, PRF_VIEWDAMAGE))
                    send_to_char(victim, "(%d) ", reflect_dam);
                send_aura_reflect_message(ch, victim, victim_aura, saved);
                damage(victim, ch, reflect_dam, victim_aura);
            }

            /* Special effect: Fire Shield - chance to apply burning */
            if (victim_aura == SPELL_FIRESHIELD && !saved && rand_number(0, 2) == 0 && !AFF_FLAGGED(ch, AFF_BURNING)) {
                struct affected_type fire_af;
                new_affect(&fire_af);
                fire_af.spell = SPELL_FIRESHIELD;
                fire_af.duration = rand_number(1, 2);
                fire_af.modifier = -2;
                fire_af.location = APPLY_HITROLL;
                SET_BIT_AR(fire_af.bitvector, AFF_BURNING);
                affect_join(ch, &fire_af, FALSE, FALSE, FALSE, FALSE);
                act("As chamas da aura de $N queimam você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Suas chamas queimam $n!", FALSE, ch, 0, victim, TO_VICT);
                act("As chamas de $N queimam $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            }

            /* Special effect: Water Shield - chance to soak (dexterity penalty) */
            if (victim_aura == SPELL_WATERSHIELD && !saved && rand_number(0, 2) == 0 && !AFF_FLAGGED(ch, AFF_SOAKED)) {
                struct affected_type water_af;
                new_affect(&water_af);
                water_af.spell = SPELL_WATERSHIELD;
                water_af.duration = rand_number(1, 3);
                water_af.modifier = -2;
                water_af.location = APPLY_DEX;
                SET_BIT_AR(water_af.bitvector, AFF_SOAKED);
                affect_join(ch, &water_af, FALSE, FALSE, FALSE, FALSE);
                act("A água da aura de $N encharca você, reduzindo sua agilidade!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura de água encharca $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura de água de $N encharca $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            }

            /* Special effect: Rock Shield - chance to reduce attacker's AC (stagger) */
            if (victim_aura == SPELL_ROCKSHIELD && !saved && rand_number(0, 3) == 0 &&
                !AFF_FLAGGED(ch, AFF_STAGGERED)) {
                struct affected_type rock_af;
                new_affect(&rock_af);
                rock_af.spell = SPELL_ROCKSHIELD;
                rock_af.duration = rand_number(2, 4);
                rock_af.modifier = 10;
                rock_af.location = APPLY_AC;
                SET_BIT_AR(rock_af.bitvector, AFF_STAGGERED);
                affect_join(ch, &rock_af, FALSE, FALSE, FALSE, FALSE);
                act("O impacto contra a aura de pedra de $N danifica sua armadura!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura de pedra danifica a armadura de $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura de pedra de $N danifica a armadura de $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            }

            /* Special effect: Poison Shield applies poison to attacker */
            if (victim_aura == SPELL_POISONSHIELD && !saved && !AFF_FLAGGED(ch, AFF_POISON)) {
                struct affected_type poison_af;
                new_affect(&poison_af);
                poison_af.spell = SPELL_POISON;
                poison_af.duration = rand_number(2, 4);
                poison_af.modifier = -2;
                poison_af.location = APPLY_STR;
                SET_BIT_AR(poison_af.bitvector, AFF_POISON);
                affect_join(ch, &poison_af, FALSE, FALSE, FALSE, FALSE);
                act("O veneno da aura de $N contamina você!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura venenosa contamina $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura venenosa de $N contamina $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            }

            /* Special effect: Lightning Shield - chance to briefly stun (paralysis) */
            if (victim_aura == SPELL_LIGHTNINGSHIELD && !saved && rand_number(0, 4) == 0 &&
                !AFF_FLAGGED(ch, AFF_PARALIZE)) {
                struct affected_type light_af;
                new_affect(&light_af);
                light_af.spell = SPELL_LIGHTNINGSHIELD;
                light_af.duration = 1;
                SET_BIT_AR(light_af.bitvector, AFF_PARALIZE);
                affect_join(ch, &light_af, FALSE, FALSE, FALSE, FALSE);
                act("O choque elétrico da aura de $N paralisa você momentaneamente!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura elétrica paralisa $n momentaneamente!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura elétrica de $N paralisa $n momentaneamente!", FALSE, ch, 0, victim, TO_NOTVICT);
            }

            /* Special effect: Ice Shield - chance to chill (hitroll and dex penalty) */
            if (victim_aura == SPELL_ICESHIELD && !saved && rand_number(0, 2) == 0 && !AFF_FLAGGED(ch, AFF_CHILLED)) {
                struct affected_type ice_af;
                new_affect(&ice_af);
                ice_af.spell = SPELL_ICESHIELD;
                ice_af.duration = rand_number(2, 3);
                ice_af.modifier = -3;
                ice_af.location = APPLY_HITROLL;
                SET_BIT_AR(ice_af.bitvector, AFF_CHILLED);
                affect_join(ch, &ice_af, FALSE, FALSE, FALSE, FALSE);
                act("O gelo da aura de $N enregela seus movimentos!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura gélida enregela os movimentos de $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura gélida de $N enregela os movimentos de $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            }

            /* Special effect: Acid Shield applies corrosion (AC penalty) */
            if (victim_aura == SPELL_ACIDSHIELD && !saved && !AFF_FLAGGED(ch, AFF_CORRODED)) {
                struct affected_type acid_af;
                new_affect(&acid_af);
                acid_af.spell = SPELL_ACIDSHIELD;
                acid_af.duration = rand_number(1, 3);
                acid_af.modifier = 15;
                acid_af.location = APPLY_AC;
                SET_BIT_AR(acid_af.bitvector, AFF_CORRODED);
                affect_join(ch, &acid_af, FALSE, FALSE, FALSE, FALSE);
                act("O ácido da aura de $N corrói sua armadura!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura ácida corrói a armadura de $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura ácida de $N corrói a armadura de $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            }

            /* Special effect: Mind Shield - chance to confuse (intelligence and mana penalty) */
            if (victim_aura == SPELL_MINDSHIELD && !saved && rand_number(0, 3) == 0 && !AFF_FLAGGED(ch, AFF_CONFUSED)) {
                struct affected_type mind_af;
                struct affected_type mana_af;
                new_affect(&mind_af);
                mind_af.spell = SPELL_MINDSHIELD;
                mind_af.duration = rand_number(1, 2);
                mind_af.modifier = -3;
                mind_af.location = APPLY_INT;
                SET_BIT_AR(mind_af.bitvector, AFF_CONFUSED);
                affect_join(ch, &mind_af, FALSE, FALSE, FALSE, FALSE);
                /* Also reduce mana */
                new_affect(&mana_af);
                mana_af.spell = SPELL_MINDSHIELD;
                mana_af.duration = rand_number(1, 2);
                mana_af.modifier = -(GET_LEVEL(victim) * 2);
                mana_af.location = APPLY_MANA;
                affect_join(ch, &mana_af, FALSE, FALSE, FALSE, FALSE);
                act("A aura mental de $N drena sua inteligência e mana!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura mental drena a inteligência e mana de $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura mental de $N drena a inteligência e mana de $n!", FALSE, ch, 0, victim, TO_NOTVICT);
            }

            /* Special effect: Force Shield - chance to stagger (movement penalty) */
            if (victim_aura == SPELL_FORCESHIELD && !saved && rand_number(0, 3) == 0 &&
                !AFF_FLAGGED(ch, AFF_STAGGERED)) {
                struct affected_type force_af;
                new_affect(&force_af);
                force_af.spell = SPELL_FORCESHIELD;
                force_af.duration = rand_number(1, 2);
                force_af.modifier = -20;
                force_af.location = APPLY_MOVE;
                SET_BIT_AR(force_af.bitvector, AFF_STAGGERED);
                affect_join(ch, &force_af, FALSE, FALSE, FALSE, FALSE);
                act("A força da aura de $N empurra você para trás!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua aura de força empurra $n para trás!", FALSE, ch, 0, victim, TO_VICT);
                act("A aura de força de $N empurra $n para trás!", FALSE, ch, 0, victim, TO_NOTVICT);
            }
        }

        /* Check if the attacker died from counter-attack damage */
        if (IS_NPC(ch)) {
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || GET_POS(ch) <= POS_DEAD)
                return;
        } else {
            if (PLR_FLAGGED(ch, PLR_NOTDEADYET) || GET_POS(ch) <= POS_DEAD)
                return;
        }
    } else if (dam > 0 && has_aura_shield(ch) && has_aura_shield(victim)) {
        /* Both attacker and victim have aura shields - element-based interactions */
        int attacker_spell = get_aura_shield_spell(ch);
        int victim_spell = get_aura_shield_spell(victim);
        int attacker_element = get_spell_element(attacker_spell);
        int victim_element = get_spell_element(victim_spell);
        int interaction = get_element_interaction(attacker_element, victim_element);

        /* Handle different element interactions */
        if (interaction == ELEM_AMPLIFY) {
            /* Attacker's element is amplified - destroys victim's shield with extra effect */
            act("Sua aura elemental \tRdestrói\tn a barreira de $N com força amplificada!", FALSE, ch, 0, victim,
                TO_CHAR);
            act("A aura elemental de $n \tRdestrói\tn sua barreira com força amplificada!", FALSE, ch, 0, victim,
                TO_VICT);
            act("A aura elemental de $n \tRdestrói\tn a barreira de $N com força amplificada!", FALSE, ch, 0, victim,
                TO_NOTVICT);
            affect_from_char(victim, victim_spell);
        } else if (interaction == ELEM_BEATS) {
            /* Attacker's element beats victim's - destroys victim's shield */
            act("Sua aura elemental destrói a barreira de $N!", FALSE, ch, 0, victim, TO_CHAR);
            act("A aura elemental de $n destrói sua barreira!", FALSE, ch, 0, victim, TO_VICT);
            act("A aura elemental de $n destrói a barreira de $N!", FALSE, ch, 0, victim, TO_NOTVICT);
            affect_from_char(victim, victim_spell);
        } else if (interaction == ELEM_NULLIFY) {
            /* Elements nullify each other - both shields are removed */
            act("Sua aura e a barreira de $N se \tYanulam\tn mutuamente!", FALSE, ch, 0, victim, TO_CHAR);
            act("Sua barreira e a aura de $n se \tYanulam\tn mutuamente!", FALSE, ch, 0, victim, TO_VICT);
            act("As auras de $n e $N se \tYanulam\tn mutuamente!", FALSE, ch, 0, victim, TO_NOTVICT);
            affect_from_char(ch, attacker_spell);
            affect_from_char(victim, victim_spell);
        } else {
            /* Check reverse interaction (victim vs attacker) */
            int reverse_interaction = get_element_interaction(victim_element, attacker_element);
            if (reverse_interaction == ELEM_AMPLIFY || reverse_interaction == ELEM_BEATS) {
                act("A barreira de $N destrói sua aura elemental!", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira destrói a aura elemental de $n!", FALSE, ch, 0, victim, TO_VICT);
                act("A barreira de $N destrói a aura elemental de $n!", FALSE, ch, 0, victim, TO_NOTVICT);
                affect_from_char(ch, attacker_spell);
            }
            /* True neutral interaction: shields do not interact */
            else {
                act("Sua aura elemental e a barreira de $N não interagem.", FALSE, ch, 0, victim, TO_CHAR);
                act("Sua barreira e a aura elemental de $n não interagem.", FALSE, ch, 0, victim, TO_VICT);
                act("A aura elemental de $n e a barreira de $N não interagem.", FALSE, ch, 0, victim, TO_NOTVICT);
                /* Nenhuma barreira é removida */
            }
        }
    }
    /* check if the victim has a hitprcnt trigger */
    hitprcnt_mtrigger(victim);
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
    struct char_data *ch, *tch;
    int num_of_attacks = 1, loop_attacks;
    for (ch = combat_list; ch; ch = next_combat_list) {
        next_combat_list = ch->next_fighting;
        if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
            stop_fighting(ch);
            continue;
        }

        if (IS_NPC(ch)) {
            if (GET_MOB_WAIT(ch) > 0) {
                GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
                continue;
            }
            GET_MOB_WAIT(ch) = 0;
            if (GET_POS(ch) < POS_FIGHTING) {
                GET_POS(ch) = POS_FIGHTING;
                act("$n se levanta!", TRUE, ch, 0, 0, TO_ROOM);
            }
        }

        if (GET_POS(ch) < POS_FIGHTING) {
            act("Você não pode lutar sentad$r!!", FALSE, ch, 0, 0, TO_CHAR);
            continue;
        }

        if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize) {
            struct iterator_data Iterator;
            tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
            for (; tch; tch = next_in_list(&Iterator)) {
                if (tch == ch)
                    continue;
                if (!IS_NPC(tch) && !PRF_FLAGGED(tch, PRF_AUTOASSIST))
                    continue;
                if (IN_ROOM(ch) != IN_ROOM(tch))
                    continue;
                if (FIGHTING(tch))
                    continue;
                if (GET_POS(tch) != POS_STANDING)
                    continue;
                if (!CAN_SEE(tch, ch))
                    continue;
                do_assist(tch, GET_NAME(ch), 0, 0);
            }
            remove_iterator(&Iterator);
        }

        num_of_attacks = attacks_per_round(ch);

        for (loop_attacks = 0;
             loop_attacks < num_of_attacks && FIGHTING(ch) && !MOB_FLAGGED(FIGHTING(ch), MOB_NOTDEADYET);
             loop_attacks++) {
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);

            /* Check if the attacker died from counter-attack (e.g., FIRESHIELD) */
            if (IS_NPC(ch)) {
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || GET_POS(ch) <= POS_DEAD)
                    break;
            } else {
                if (PLR_FLAGGED(ch, PLR_NOTDEADYET) || GET_POS(ch) <= POS_DEAD)
                    break;
            }
        }

        /* Check if ch is still valid before calling mob special */
        if (IS_NPC(ch) && (MOB_FLAGGED(ch, MOB_NOTDEADYET) || GET_POS(ch) <= POS_DEAD))
            continue;
        if (!IS_NPC(ch) && (PLR_FLAGGED(ch, PLR_NOTDEADYET) || GET_POS(ch) <= POS_DEAD))
            continue;

        if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
            char actbuf[MAX_INPUT_LENGTH] = "";
            (GET_MOB_SPEC(ch))(ch, ch, 0, actbuf);
        }
    }
}

/* ported from Eltanin by AxL, 2feb97 */
/* added by Miikka M. Kangas 8-14-91 (note this is called with ownpulses!)I was getting bored so I wanted to add some
   edge to weather.  */
void beware_lightning()
{
    int dam = 0;
    struct char_data *victim = NULL, *temp = NULL;
    char buf[256];
    zone_rnum zona_vitima;

    // Itera por todas as zonas
    for (int zone = 0; zone <= top_of_zone_table; zone++) {
        if (zone_table[zone].weather->sky != SKY_LIGHTNING)
            continue;   // Pula para a próxima zona se não estiver relampejando

        if (rand_number(0, 9) != 0)
            continue;   // Raio cai apenas 10% das vezes

        if (rand_number(0, 99) == 0) {   // 99% das vezes, apenas trovão
            send_to_zone_outdoor(zone, "Voce ouve o ronco do trovao.\n\r");
            continue;
        }

        for (victim = character_list; victim; victim = temp) {
            temp = victim->next;

            /* Skip characters that are already marked for extraction */
            if (MOB_FLAGGED(victim, MOB_NOTDEADYET) || PLR_FLAGGED(victim, PLR_NOTDEADYET))
                continue;

            /* Safety check: validate room before accessing zone */
            if (IN_ROOM(victim) == NOWHERE || IN_ROOM(victim) < 0 || IN_ROOM(victim) > top_of_world)
                continue;

            zona_vitima = world[IN_ROOM(victim)].zone;   // pega a zona (vnum para rnum) da sala da vitima
            if (zona_vitima != zone)
                continue;
            if (OUTSIDE(victim) == TRUE) {      // Apenas personagens ao ar livre
                if (rand_number(0, 9) == 0) {   // 1% de chance de acertar alguém
                    dam = dice(1, (GET_MAX_HIT(victim) * 2));

                    /* Check for stoneskin protection first */
                    if (!apply_stoneskin_protection(victim, &dam)) {
                        /* Stoneskin was not active, check other protections */
                        if (IS_AFFECTED(victim, AFF_SANCTUARY))
                            dam = MIN(dam, 18);
                        if (IS_AFFECTED(victim, AFF_GLOOMSHIELD))
                            dam = MIN(dam, 33);
                    }

                    dam = MIN(dam, 100);
                    dam = MAX(dam, 0);
                    if (GET_LEVEL(victim) >= LVL_IMMORT)
                        dam = 0;   // Imortais não são afetados

                    GET_HIT(victim) -= dam;

                    if (dam > 0) {
                        act("KAZAK! Um raio atinge $n. Voce escuta um assobio.", TRUE, victim, 0, 0, TO_ROOM);
                        act("KAZAK! Um raio atinge voce. Voce escuta um assobio.", FALSE, victim, 0, 0, TO_CHAR);
                    }

                    if (dam > (GET_MAX_HIT(victim) >> 2))
                        act("Isto realmente DOEU!\r\n", FALSE, victim, 0, 0, TO_CHAR);

                    send_to_zone_outdoor(zone, "*BOOM* Voce escuta o ronco do trovao.\n\r");

                    update_pos(victim);

                    switch (GET_POS(victim)) {
                        case POS_MORTALLYW:
                            act("$n esta mortalmente ferid$r, e vai morrer logo, se nao for medicad$r.", TRUE, victim,
                                0, 0, TO_ROOM);
                            act("Voce esta mortalmente ferid$r, e vai morrer logo, se nao medicad$r.", FALSE, victim, 0,
                                0, TO_CHAR);
                            break;
                        case POS_INCAP:
                            act("$n esta incapacitad$r e vai morrer lentamente, se nao for medicad$r.", TRUE, victim, 0,
                                0, TO_ROOM);
                            act("Voce esta incapacitad$r e vai morrer lentamente, se nao for medicad$r.", FALSE, victim,
                                0, 0, TO_CHAR);
                            break;
                        case POS_STUNNED:
                            act("$n esta atordoad$r, mas provavelmente ira recuperar a consciencia novamente.", TRUE,
                                victim, 0, 0, TO_ROOM);
                            act("Voce esta atordoad$r, mas provavelmente ira recuperar a consciencia novamente.", FALSE,
                                victim, 0, 0, TO_CHAR);
                            break;
                        case POS_DEAD:
                            act("$n esta mort$r! R.I.P.", TRUE, victim, 0, 0, TO_ROOM);
                            act("Voce esta mort$r! Sinto muito...\r\n", FALSE, victim, 0, 0, TO_CHAR);
                            break;
                        default:
                            if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2))
                                act("Voce espera que seus ferimentos parem de SANGRAR tanto!", FALSE, victim, 0, 0,
                                    TO_CHAR);
                            break;
                    }

                    if (GET_POS(victim) == POS_DEAD) {
                        sprintf(buf, "Um raio matou %s", GET_NAME(victim));
                        log1("%s", buf);
                        gain_exp(victim, -(GET_EXP(victim) / 2));
                        if (!IS_NPC(victim)) {
                            REMOVE_BIT_AR(PLR_FLAGS(victim), PLR_KILLER);
                            REMOVE_BIT_AR(PLR_FLAGS(victim), PLR_THIEF);
                            SET_BIT_AR(PLR_FLAGS(victim), PLR_GHOST);
                        }
                        if (FIGHTING(victim))
                            stop_fighting(victim);
                        while (victim->affected)
                            affect_remove(victim, victim->affected);

                        GET_POS(victim) = POS_STANDING;
                        death_cry(victim);
                        if (GROUP(victim))
                            send_to_group(victim, GROUP(victim), "%s morreu.\r\n", GET_NAME(victim));
                        update_pos(victim);
                        make_corpse(victim);
                        extract_char(victim);
                    }
                } else {
                    act("KAZAK! Um raio atinge perto.", FALSE, victim, 0, 0, TO_ROOM);
                    act("KAZAK! Um raio atinge perto.", FALSE, victim, 0, 0, TO_CHAR);
                    send_to_zone_outdoor(zone, "*BOOM* Voce escuta o ronco do trovao.\n\r");
                }
            }
        }
    }
}
/* -- jr - 24/08/99 * Weapon Proficiencies -- (C) 1999 by Fabrizio Baldi */

int get_weapon_prof(struct char_data *ch, struct obj_data *wield)
{
    int value = 0, bonus = 0, learned = 0, type = -1;
    int i = 0;
    struct str_spells *skill = NULL;
    if (IS_NPC(ch) || IS_DEAD(ch))
        return (bonus);
    if (wield != NULL) {
        value = GET_OBJ_VAL(wield, 3) + TYPE_HIT;
        switch (value) {
            case TYPE_SLASH:
                type = SKILL_WEAPON_SWORDS;
                break;
            case TYPE_STING:
            case TYPE_PIERCE:
            case TYPE_STAB:
                type = SKILL_WEAPON_DAGGERS;
                break;
            case TYPE_THRASH:
            case TYPE_WHIP:
                type = SKILL_WEAPON_WHIPS;
                break;
            case TYPE_CLAW:
                type = SKILL_WEAPON_TALONOUS_ARMS;
                break;
            case TYPE_BLUDGEON:
            case TYPE_MAUL:
            case TYPE_POUND:
            case TYPE_CRUSH:
                type = SKILL_WEAPON_BLUDGEONS;
                break;
            case TYPE_HIT:
            case TYPE_PUNCH:
            case TYPE_BITE:
            case TYPE_BLAST:
                type = SKILL_WEAPON_EXOTICS;
                break;
            case TYPE_BORE:
            case TYPE_BROACH:
            case TYPE_MOW:
                type = SKILL_WEAPON_POLEARMS;
            default:
                type = -1;
                break;
        }
    }
    if (type != -1) {
        if ((learned = GET_SKILL(ch, type)) != 0) {
            if (learned <= 20)
                bonus = 1;
            else if (learned <= 40)
                bonus = 2;
            else if (learned <= 60)
                bonus = 3;
            else if (learned <= 80)
                bonus = 4;
            else if (learned <= 85)
                bonus = 5;
            else if (learned <= 90)
                bonus = 6;
            else if (learned <= 95)
                bonus = 7;
            else if (learned <= 99)
                bonus = 8;
            else
                bonus = 9;
        } else {
            skill = get_spell_by_vnum(type);
            for (i = 0; i < NUM_CLASSES; i++)
                if (skill->assign[i].class_num == GET_CLASS(ch))
                    if (GET_LEVEL(ch) >= skill->assign[i].level)
                        bonus = 10;
        }
        return (bonus);
    } else {
        return 0;
    }
}

/* -- jr - Oct 07, 2001 */
#define NIGHTHAMMER_LVL 50
int get_nighthammer(struct char_data *ch, bool real)
{
    int mod, learned;

    if (IS_NPC(ch) || IS_DEAD(ch))
        return (0);

    if (GET_LEVEL(ch) < NIGHTHAMMER_LVL)
        return (0);

    if (!(learned = GET_SKILL(ch, SKILL_NIGHTHAMMER)))
        return (0);

    /* Check if it's actually night time based on sunlight */
    if (weather_info.sunlight == SUN_LIGHT || weather_info.sunlight == SUN_RISE)
        return (0);

    mod = 1;
    /* Extra bonus during deep night (SUN_DARK) */
    if (weather_info.sunlight == SUN_DARK)
        mod++;
    /* Weather conditions that obscure moonlight provide additional bonus */
    if (weather_info.sky == SKY_CLOUDY || weather_info.sky == SKY_RAINING || weather_info.sky == SKY_LIGHTNING ||
        weather_info.sky == SKY_SNOWING)
        mod++;

    mod += ((GET_LEVEL(ch) - NIGHTHAMMER_LVL) / 8);
    mod = MIN(mod, 8);

    if (real && rand_number(0, 101) > learned)
        mod--;

    return MIN(MAX(mod, 0), 8);
}

/* -- jr - May 07, 2001 */
int attacks_per_round(struct char_data *ch)
{
    int n = 1;

    if (!IS_NPC(ch)) {
        struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
        if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
            n += wpn_prof[get_weapon_prof(ch, wielded)].num_of_attacks;
    } else {
        n += ((int)GET_LEVEL(ch) / 25);

        /* HYBRID EMOTION SYSTEM: High anger increases attack frequency for NPCs */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && ch->ai_data && FIGHTING(ch)) {
            /* Use hybrid system: check effective anger toward opponent */
            int effective_anger = get_effective_emotion_toward(ch, FIGHTING(ch), EMOTION_TYPE_ANGER);

            /* High anger (>= threshold) gives chance for extra attack */
            if (effective_anger >= CONFIG_EMOTION_COMBAT_ANGER_HIGH_THRESHOLD) {
                /* Random chance based on config (default 25%) */
                if (rand_number(1, 100) <= CONFIG_EMOTION_COMBAT_ANGER_ATTACK_BONUS) {
                    n++; /* Furious attack - one extra attack */
                }
            }
        }
    }

    return (n);
}

void transcend(struct char_data *ch)
{
    struct char_data *k;

    /* Set the experience */
    GET_EXP(ch) = level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1) - 1;

    /* Stop fighting, seizing, mounting, etc */
    if (FIGHTING(ch))
        stop_fighting(ch);

    for (k = combat_list; k; k = next_combat_list) {
        next_combat_list = k->next_fighting;
        if (FIGHTING(k) == ch)
            stop_fighting(k);
    }

    /* Set the transcendent flag */
    SET_BIT_AR(PLR_FLAGS(ch), PLR_TRNS);

    /* Restore character points */
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);

    /* Reset other variables */
    GET_PRACTICES(ch) = 0;

    /* Explain what happened */
    send_to_char(ch,
                 "\n@+cAo lutar, uma estranha sensaçãoo vem sobre você, e você\r\n"
                 "sente como se você não pudesse mais aprender, como se o seu\r\n"
                 "conhecimento houvesse chegado ao limite...\r\n\n"
                 "\a\a@+WVocê transcendeu!!@+n\r\n\n");

    /* Log */
    log1("(Lvl) %s (level %d) trancended.", GET_NAME(ch), GET_LEVEL(ch));

    /* Save */
    save_char(ch);
}

/**
 * Check if a mob is a shopkeeper by looking through the shop array
 */
bool is_shopkeeper(struct char_data *mob)
{
    if (!IS_NPC(mob))
        return FALSE;

    int shop_nr;
    for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
        if (SHOP_KEEPER(shop_nr) == mob->nr)
            return TRUE;

    return FALSE;
}

/**
 * Calculate the fitness of a mob based on its type and performance.
 * Higher fitness means the mob was more successful and should contribute
 * more to the genetic pool. Returns a value between 0 and 100.
 */
int calculate_mob_fitness(struct char_data *mob)
{
    if (!IS_NPC(mob) || !mob->ai_data)
        return 0;

    int base_fitness = 0;

    /* Base fitness from experience gained (survival and combat effectiveness) */
    int exp_gained = GET_EXP(mob);
    if (exp_gained > 0) {
        /* Scale XP to fitness: 0-2500 XP maps to 0-40 fitness points */
        base_fitness += MIN(exp_gained / 62, 40); /* 2500 / 62 ≈ 40 */
    }

    /* Base fitness from gold accumulated (economic success) */
    int gold = GET_GOLD(mob);
    if (gold > 0) {
        /* Scale gold to fitness: 0-1000 gold maps to 0-20 fitness points */
        base_fitness += MIN(gold / 50, 20); /* 1000 / 50 = 20 */
    }

    /* Bonus fitness based on mob type and special behaviors */

    /* Shopkeeper fitness: based on economic activity */
    if (is_shopkeeper(mob)) {
        /* Shopkeepers get bonus for having more money */
        if (gold > 500)
            base_fitness += 10;
        if (gold > 1000)
            base_fitness += 10;
        /* Future: could add shop transaction count, inventory turnover, etc. */
    }

    /* Sentinel fitness: based on combat effectiveness and area defense */
    if (MOB_FLAGGED(mob, MOB_SENTINEL)) {
        /* Sentinels get bonus for higher level (more dangerous defender) */
        base_fitness += MIN(GET_LEVEL(mob) / 5, 15); /* Level 75 gives 15 points */
        /* Sentinels get bonus for being brave */
        if (MOB_FLAGGED(mob, MOB_BRAVE))
            base_fitness += 5;
    }

    /* Group member fitness: based on group cooperation */
    if (GROUP(mob)) {
        /* Mobs that died while in a group get a small cooperation bonus */
        base_fitness += 5;
        /* Larger groups indicate better cooperation */
        if (GROUP(mob)->members && GROUP(mob)->members->iSize > 2) {
            base_fitness += 5;
        }
    }

    /* Aggressive mobs get bonus for combat engagement */
    if (MOB_FLAGGED(mob, MOB_AGGRESSIVE)) {
        base_fitness += 5;
    }

    /* Penalty for dying too close to starting location (didn't explore/adapt) */
    if (IN_ROOM(mob) == real_room(GET_LOADROOM(mob))) {
        base_fitness -= 10;
    }

    /* Bonus for dying far from starting location (exploration/adaptation) */
    room_vnum load_room = GET_LOADROOM(mob);
    if (load_room != NOWHERE && IN_ROOM(mob) != NOWHERE) {
        /* This is a simple distance check - could be improved with pathfinding */
        int room_diff = abs(GET_ROOM_VNUM(IN_ROOM(mob)) - load_room);
        if (room_diff > 50)
            base_fitness += 5; /* Explored different areas */
        if (room_diff > 100)
            base_fitness += 5; /* Explored far areas */
    }

    /* Ensure fitness is within bounds */
    return MAX(0, MIN(base_fitness, 100));
}

/**
 * Improved version of update_single_gene that uses fitness as weight.
 * Instead of a fixed 70/30 split, this uses fitness to determine
 * how much the dying mob should influence the prototype.
 */
void update_single_gene_with_fitness(int *proto_gene, int instance_gene, int fitness, int min, int max)
{
    /* Convert fitness (0-100) to influence weight (10-90) */
    /* Even low-fitness mobs get some influence (minimum 10%) */
    /* High-fitness mobs get major influence (up to 90%) */
    int influence_weight = 10 + ((fitness * 80) / 100); /* Maps 0-100 fitness to 10-90 weight */
    int prototype_weight = 100 - influence_weight;

    /* Apply the weighted formula */
    *proto_gene = ((*proto_gene * prototype_weight) + (instance_gene * influence_weight)) / 100;

    /* Ensure the new value stays within bounds */
    *proto_gene = MAX(min, MIN(*proto_gene, max));
}

/**
 * Função auxiliar que calcula a nova média ponderada para um único gene
 * e atualiza o valor do protótipo.
 * @param proto_gene Um ponteiro para o campo do gene no protótipo.
 * @param instance_gene O valor final do gene da instância que morreu.
 * @param min O valor mínimo permitido para o gene.
 * @param max O valor máximo permitido para o gene.
 */
void update_single_gene(int *proto_gene, int instance_gene, int min, int max)
{
    /* Usa a nossa fórmula de média ponderada 70/30. */
    *proto_gene = (((*proto_gene) * 7) + (instance_gene * 3)) / 10;

    /* Garante que o novo valor fica dentro dos limites. */
    *proto_gene = MIN(MAX(*proto_gene, min), max);
}

/**
 * Chamada quando um mob morre para atualizar a genética do seu protótipo.
 * VERSÃO MELHORADA: Usa fitness para determinar a influência genética.
 */
void update_mob_prototype_genetics(struct char_data *mob)
{
    if (!IS_NPC(mob) || GET_MOB_RNUM(mob) == NOBODY || !mob->ai_data)
        return;

    mob_rnum rnum = GET_MOB_RNUM(mob);
    struct char_data *proto = &mob_proto[rnum];

    if (!proto->ai_data)
        return;

    /* 1. Calculate the mob's fitness based on its performance */
    int fitness = calculate_mob_fitness(mob);

    /* Store fitness in the mob for potential future use/debugging */
    GET_FIT(mob) = fitness;

    /* 2. Prepare final gene values, applying death-related adjustments */
    int final_wimpy = mob->ai_data->genetics.wimpy_tendency;
    int final_roam = mob->ai_data->genetics.roam_tendency;
    int final_group = mob->ai_data->genetics.group_tendency;
    int final_use = mob->ai_data->genetics.use_tendency;
    int final_brave = mob->ai_data->genetics.brave_prevalence;
    int final_trade = mob->ai_data->genetics.trade_tendency;
    int final_quest = mob->ai_data->genetics.quest_tendency;
    int final_adventurer = mob->ai_data->genetics.adventurer_tendency;
    int final_follow = mob->ai_data->genetics.follow_tendency;

    if (MOB_FLAGGED(mob, MOB_BRAVE)) {
        final_wimpy -= 5;
        final_brave += 3; /* A morte de um bravo reforça o traço. */
    } else {
        final_wimpy += 5;
        final_brave -= 1; /* A morte de um não-bravo diminui a prevalência. */
    }

    if (IN_ROOM(mob) != real_room(GET_LOADROOM(mob))) {
        final_roam -= 10;
    }

    if (GROUP(mob)) {
        final_group -= 3;
    }

    /* 3. Use fitness-based genetic updates instead of fixed 70/30 split */
    update_single_gene_with_fitness(&proto->ai_data->genetics.wimpy_tendency, final_wimpy, fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.loot_tendency, mob->ai_data->genetics.loot_tendency,
                                    fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.equip_tendency, mob->ai_data->genetics.equip_tendency,
                                    fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.roam_tendency, final_roam, fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.group_tendency, final_group, fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.use_tendency, final_use, fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.trade_tendency, final_trade, fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.brave_prevalence, final_brave, fitness, 0, 75);
    update_single_gene_with_fitness(&proto->ai_data->genetics.quest_tendency, final_quest, fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.adventurer_tendency, final_adventurer, fitness, 0, 100);
    update_single_gene_with_fitness(&proto->ai_data->genetics.follow_tendency, final_follow, fitness, 0, 100);

    /* 4. Marca a zona para salvar. */
    mob_vnum vnum = mob_index[rnum].vnum;
    zone_rnum rznum = real_zone_by_thing(vnum);

    if (rznum != NOWHERE) {
        add_to_save_list(zone_table[rznum].number, SL_MOB);
    }
}
