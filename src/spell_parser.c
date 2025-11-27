/**************************************************************************
 *  File: spell_parser.c                                    Part of tbaMUD *
 *  Usage: Top-level magic routines; outside points of entry to magic sys. *
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
#include "config.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h" /* for hit() */
#include "spedit.h"
#include "spells_script.h"
#include "formula.h"
#include "utils.h"
#include "constants.h"

/* Global Variables definitions, used elsewhere */
char cast_arg2[MAX_INPUT_LENGTH];
int spell_modifier_diminish = 0; /* Set to 1 when "minus" syllable is used */
int spell_modifier_amplify = 0;  /* Set to 1 when "plus" syllable is used */

/* Local (File Scope) Function Prototypes */
static void say_spell(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj);
static void say_chanson(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj);

/* Local (File Scope) Variables */
struct syllable {
    const char *org;
    const char *news;
};
static struct syllable syls[] = {{" ", " "},
                                 {"ar", "abra"},
                                 {"ate", "i"},
                                 {"cau", "kada"},
                                 {"blind", "nose"},
                                 {"bur", "mosa"},
                                 {"cu", "judi"},
                                 {"de", "oculo"},
                                 {"dis", "mar"},
                                 {"ect", "kamina"},
                                 {"en", "uns"},
                                 {"gro", "cra"},
                                 {"light", "dies"},
                                 {"lo", "hi"},
                                 {"magi", "kari"},
                                 {"mon", "bar"},
                                 {"mor", "zak"},
                                 {"move", "sido"},
                                 {"ness", "lacri"},
                                 {"ning", "illa"},
                                 {"per", "duda"},
                                 {"ra", "gru"},
                                 {"re", "candus"},
                                 {"son", "sabru"},
                                 {"tect", "infra"},
                                 {"tri", "cula"},
                                 {"ven", "nofo"},
                                 {"word of", "inset"},
                                 {"fire", "ignis"},
                                 {"shield", "aegis"},
                                 {"stone", "petra"},
                                 {"skin", "cutis"},
                                 {"thistle", "spina"},
                                 {"coat", "tunica"},
                                 {"rock", "saxum"},
                                 {"steel", "ferrum"},
                                 {"ball", "globus"},
                                 {"heal", "sanitas"},
                                 {"aid", "auxilium"},
                                 {"harm", "nocere"},
                                 {"cure", "medere"},
                                 {"poison", "venenum"},
                                 {"charm", "fascino"},
                                 {"sleep", "somnus"},
                                 {"strength", "robur"},
                                 {"invisibility", "lateo"},
                                 {"teleport", "migro"},
                                 {"bless", "benedico"},
                                 {"curse", "maledico"},
                                 {"detect", "sentio"},
                                 {"protection", "tutela"},
                                 {"sanctuary", "asylum"},
                                 {"recall", "memini"},
                                 {"locate", "invenio"},
                                 {"missile", "telum"},
                                 {"lightning", "fulmen"},
                                 {"earthquake", "tremor"},
                                 {"enchant", "incanto"},
                                 {"energy", "vis"},
                                 {"drain", "exuo"},
                                 {"color", "color"},
                                 {"spray", "aspersio"},
                                 {"control", "impero"},
                                 {"weather", "caelum"},
                                 {"create", "creo"},
                                 {"food", "cibus"},
                                 {"water", "aqua"},
                                 {"hands", "manus"},
                                 {"touch", "tactus"},
                                 {"clone", "imago"},
                                 {"bolt", "sagitta"},
                                 {"object", "res"},
                                 {"evil", "malum"},
                                 {"good", "bonum"},
                                 {"remove", "tollo"},
                                 {"group", "turma"},
                                 {"animate", "animo"},
                                 {"dead", "mortuum"},
                                 {"sense", "sentio"},
                                 {"life", "vita"},
                                 {"weapon", "arma"},
                                 {"burning", "ardens"},
                                 {"call", "voco"},
                                 {"chill", "frigus"},
                                 {"dispel", "pello"},
                                 {"grasp", "capio"},
                                 {"infravision", "nyctalops"},
                                 {"shocking", "fulgor"},
                                 {"summon", "evoco"},
                                 {"ventriloquate", "ventriloquus"},
                                 {"alignment", "ordo"},
                                 {"critic", "gravis"},
                                 {"person", "persona"},
                                 {"a", "i"},
                                 {"b", "v"},
                                 {"c", "q"},
                                 {"d", "m"},
                                 {"e", "o"},
                                 {"f", "y"},
                                 {"g", "t"},
                                 {"h", "p"},
                                 {"i", "u"},
                                 {"j", "y"},
                                 {"k", "t"},
                                 {"l", "r"},
                                 {"m", "w"},
                                 {"n", "b"},
                                 {"o", "a"},
                                 {"p", "s"},
                                 {"q", "d"},
                                 {"r", "f"},
                                 {"s", "g"},
                                 {"t", "h"},
                                 {"u", "e"},
                                 {"v", "z"},
                                 {"w", "x"},
                                 {"x", "n"},
                                 {"y", "l"},
                                 {"z", "k"},
                                 {"", ""}};

void call_ASPELL(void (*function)(), int level, struct char_data *ch, struct char_data *vict, struct obj_data *obj)
{
    (*function)(level, ch, vict, obj);
}

void call_ACMD(void (*function)(), struct char_data *ch, char *argument, int cmd, int subcmd)
{
    (*function)(ch, argument, cmd, subcmd);
}

/* Helper function to get mana cost with recursion depth protection */
static int mag_manacost_internal(struct char_data *ch, struct char_data *tch, int spellnum, int depth)
{
    struct str_spells *spell;
    int mana, num, rts_code, i;

    /* Prevent infinite recursion - max depth of 10 should be more than enough */
    if (depth > 10) {
        log1("SYSERR: mag_manacost exceeded max recursion depth for spell %d", spellnum);
        return 100;
    }

    spell = get_spell_by_vnum(spellnum);

    if (!spell) {
        log1("SYSERR: spell not found vnum %d passed to mag_manacost.", spellnum);
        return 100;
    }

    /* For discoverable variant spells with a prerequisite, use the prerequisite spell's mana cost */
    if (spell->discoverable && spell->prerequisite_spell > 0) {
        struct str_spells *prereq_spell = get_spell_by_vnum(spell->prerequisite_spell);
        if (prereq_spell) {
            /* Recursively get the mana cost of the prerequisite spell */
            return mag_manacost_internal(ch, tch, spell->prerequisite_spell, depth + 1);
        }
    }

    num = get_spell_class(spell, GET_CLASS(ch));
    if (num == -1) {
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            log1(
                "SYSERR: spell vnum %d not assigned to class: %d"
                ", passed to mag_manacost.",
                spellnum, GET_CLASS(ch));
            /* For retained skills from other classes, find any class that has this spell
             * and use its mana formula with a 1.5x multiplier */
            mana = 0;
            int min_mana = 0;
            for (i = 0; i < NUM_CLASSES; i++) {
                if (spell->assign[i].class_num != -1 && spell->assign[i].num_mana) {
                    int class_mana = MAX(5, formula_interpreter(ch, tch, spellnum, TRUE, spell->assign[i].num_mana,
                                                                GET_LEVEL(ch), &rts_code));
                    if (min_mana == 0 || class_mana < min_mana) {
                        min_mana = class_mana;
                    }
                }
            }
            if (min_mana > 0) {
                /* Apply cross-class penalty: 1.5x mana cost */
                mana = (min_mana * 3) / 2;
            }
            /* If no formula found, use default */
            if (mana == 0) {
                mana = 100;
            }
        } else {
            return 0;
        }
    } else {
        mana =
            MAX(5, formula_interpreter(ch, tch, spellnum, TRUE, spell->assign[num].num_mana, GET_LEVEL(ch), &rts_code));
    }

    /* Apply spell modifier to mana cost */
    if (spell_modifier_diminish) {
        mana = mana / 2; /* Halve mana cost for "minus" syllable */
    } else if (spell_modifier_amplify) {
        mana = mana * 2; /* Double mana cost for "plus" syllable */
    }

    /* Apply mana density modifier to mana cost
     *
     * DESIGN NOTE: The cost reduction in low-density areas (0.9x-0.95x cost) is intentionally
     * LESS aggressive than the power reduction (0.8x-0.9x power in magic.c). This asymmetry
     * ensures that casting in low-density areas is not "free" - you pay nearly full cost but
     * get significantly reduced effect. This discourages spam-casting in poor conditions while
     * still allowing strategic spell use when necessary.
     *
     * High-density areas provide both cost savings AND power increases, rewarding tactical
     * positioning and use of Control Weather.
     */
    float mana_density = calculate_mana_density(ch);
    float density_modifier = 1.0;

    if (mana_density >= 1.2)
        density_modifier = 0.7; /* 30% less mana in exceptional density */
    else if (mana_density >= 0.9)
        density_modifier = 0.8; /* 20% less mana in very high density */
    else if (mana_density >= 0.7)
        density_modifier = 0.9; /* 10% less mana in high density */
    else if (mana_density >= 0.5)
        density_modifier = 1.0; /* Normal cost at normal density */
    else if (mana_density >= 0.3)
        density_modifier = 0.95; /* Slightly less cost in low density (but spells 10% weaker) */
    else
        density_modifier = 0.9; /* Reduced cost in very low density (but spells 20% weaker) */

    mana = (int)(mana * density_modifier);
    mana = MAX(1, mana); /* Always cost at least 1 mana */

    return mana;
}

/* Public wrapper function for mag_manacost */
int mag_manacost(struct char_data *ch, struct char_data *tch, int spellnum)
{
    return mag_manacost_internal(ch, tch, spellnum, 0);
}

static void say_spell(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj)
{
    char lbuf[256], buf[256], buf1[256], buf2[256]; /* FIXME */
    const char *format;

    struct char_data *i;
    int j, ofs = 0;

    *buf = '\0';

    strlcpy(lbuf, get_spell_name(spellnum), sizeof(lbuf));

    while (lbuf[ofs]) {
        for (j = 0; *(syls[j].org); j++) {
            if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
                strcat(buf, syls[j].news); /* strcat: BAD */
                ofs += strlen(syls[j].org);
                break;
            }
        }
        /* i.e., we didn't find a match in syls[] */
        if (!*syls[j].org) {
            log1("No entry in syllable table for substring of '%s'", lbuf);
            ofs++;
        }
    }

    if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch)) {
        if (tch == ch)
            format = "$n fecha seus olhos e expressa as palavras, '%s'.";
        else
            format = "$n olha para $N e expressa as palavras, '%s'.";
    } else if (tobj != NULL && ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
        format = "$n olha para $p e expressa as palavras, '%s'.";
    else
        format = "$n expressa as palavras, '%s'.";

    snprintf(buf1, sizeof(buf1), format, skill_name(spellnum));
    snprintf(buf2, sizeof(buf2), format, buf);

    for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
        if (i == ch || i == tch || !i->desc || !AWAKE(i))
            continue;
        if (GET_CLASS(ch) == GET_CLASS(i))
            perform_act(buf1, ch, tobj, tch, i);
        else
            perform_act(buf2, ch, tobj, tch, i);
    }

    if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch)) {
        snprintf(buf1, sizeof(buf1), "$n olha para você e expressa as palavras, '%s'.",
                 GET_CLASS(ch) == GET_CLASS(tch) ? skill_name(spellnum) : buf);
        act(buf1, FALSE, ch, NULL, tch, TO_VICT);
    }
}

static void say_chanson(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj)
{
    char buf[256], buf1[256];
    const char *format;

    struct char_data *i;

    *buf = '\0';

    if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch)) {
        if (tch == ch)
            format = "$n fecha seus olhos e começa a cantar.";
        else
            format = "$n olha para $N e começa a cantar.";
    } else if (tobj != NULL && ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
        format = "$n olha para $p e começa a cantar.";
    else
        format = "$n começa a cantar";

    snprintf(buf, sizeof(buf), format, skill_name(spellnum));

    for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
        if (i == ch || i == tch || !i->desc || !AWAKE(i))
            continue;
        perform_act(buf, ch, tobj, tch, i);
    }
    if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch)) {
        snprintf(buf1, sizeof(buf1), "$n olha para você e começa a cantar.");
        act(buf1, FALSE, ch, NULL, tch, TO_VICT);
    }
}

/* Convert spell name to mystical syllables - returns the transformed string */
static void spell_to_syllables(const char *spell_name, char *result, size_t result_size)
{
    char lbuf[256];
    int j, ofs = 0;

    *result = '\0';
    strlcpy(lbuf, spell_name, sizeof(lbuf));

    while (lbuf[ofs] && strlen(result) < result_size - 1) {
        for (j = 0; *(syls[j].org); j++) {
            if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
                strncat(result, syls[j].news, result_size - strlen(result) - 1);
                ofs += strlen(syls[j].org);
                break;
            }
        }
        /* i.e., we didn't find a match in syls[] */
        if (!*syls[j].org) {
            ofs++;
        }
    }
}

/* Public wrapper for spell_to_syllables */
void spell_to_syllables_public(const char *spell_name, char *result, size_t result_size)
{
    spell_to_syllables(spell_name, result, result_size);
}

/* Strip color codes from text for voice casting comparison
 * Color codes are @ followed by any character (e.g., @r, @n, @@ for literal @) */
static void strip_color_codes(const char *src, char *dest, size_t dest_size)
{
    size_t dest_idx = 0;
    const char *src_ptr = src;

    while (*src_ptr && dest_idx < dest_size - 1) {
        if (*src_ptr == '@' && *(src_ptr + 1)) {
            /* Skip the @ and the next character (color code) */
            /* Exception: @@ means a literal @, so keep one @ */
            if (*(src_ptr + 1) == '@') {
                dest[dest_idx++] = '@';
            }
            src_ptr += 2;
        } else {
            dest[dest_idx++] = *src_ptr;
            src_ptr++;
        }
    }
    dest[dest_idx] = '\0';
}

/* Check if spoken text matches spell syllables and attempt to cast if player knows the spell */
int check_voice_cast(struct char_data *ch, const char *spoken_text)
{
    struct str_spells *spell;
    char syllables[256];
    char spoken_clean[256];
    char spoken_lower[256];
    char target_buffer[256];
    char *parse_ptr;
    const char *target_part = NULL;
    int i, syllable_len, spoken_len;
    int has_diminish = 0, has_amplify = 0;
    const char *diminish_word = "minus"; /* Word to halve effect/cost */
    const char *amplify_word = "plus";   /* Word to double effect/cost */
    int diminish_len = strlen(diminish_word);
    int amplify_len = strlen(amplify_word);

    /* Don't trigger for NPCs or if text is too short */
    if (IS_NPC(ch) || strlen(spoken_text) < 5)
        return 0;

    /* Strip color codes from spoken text before processing */
    strip_color_codes(spoken_text, spoken_clean, sizeof(spoken_clean));

    /* Convert spoken text to lowercase for comparison */
    strlcpy(spoken_lower, spoken_clean, sizeof(spoken_lower));
    for (i = 0; spoken_lower[i]; i++)
        spoken_lower[i] = LOWER(spoken_lower[i]);

    spoken_len = strlen(spoken_lower);
    parse_ptr = spoken_lower;

    /* Check for modifier syllables at the beginning */
    if (!strncmp(parse_ptr, diminish_word, diminish_len) &&
        (parse_ptr[diminish_len] == ' ' || parse_ptr[diminish_len] == '\0')) {
        has_diminish = 1;
        parse_ptr += diminish_len;
        if (*parse_ptr == ' ')
            parse_ptr++;
    } else if (!strncmp(parse_ptr, amplify_word, amplify_len) &&
               (parse_ptr[amplify_len] == ' ' || parse_ptr[amplify_len] == '\0')) {
        has_amplify = 1;
        parse_ptr += amplify_len;
        if (*parse_ptr == ' ')
            parse_ptr++;
    }

    /* Update spoken_len to reflect remaining text after modifier */
    spoken_len = strlen(parse_ptr);

    /* Check all spells to see if the syllables match */
    for (spell = list_spells; spell; spell = spell->next) {
        /* Only check actual spells, not skills */
        if (spell->type != SPELL || spell->status != available)
            continue;

        /* Convert this spell's name to syllables */
        spell_to_syllables(spell->name, syllables, sizeof(syllables));
        syllable_len = strlen(syllables);

        /* Try exact match first (no target) */
        if (!strcmp(syllables, parse_ptr)) {
            target_part = NULL;
        }
        /* Try match with target: check if syllables match the beginning of spoken text
         * and there's a space after the syllables */
        else if (spoken_len > syllable_len && parse_ptr[syllable_len] == ' ' &&
                 !strncmp(syllables, parse_ptr, syllable_len)) {
            /* Extract everything after the syllables and space as the target */
            char *target_ptr;
            /* Calculate offset in original spoken_clean */
            int offset = parse_ptr - spoken_lower;
            strlcpy(target_buffer, spoken_clean + offset + syllable_len + 1, sizeof(target_buffer));
            target_ptr = target_buffer;
            skip_spaces(&target_ptr);
            target_part = target_ptr;
            if (!*target_part)
                target_part = NULL;
        } else {
            /* No match for this spell, try next */
            continue;
        }

        /* Found a match! Check if player knows this spell */
        if (GET_SKILL(ch, spell->vnum) > 0) {
            char cast_command[MAX_INPUT_LENGTH];
            char modifier_msg[128] = "";

            /* Build modifier message */
            if (has_diminish) {
                strlcpy(modifier_msg, " @c[Reduzido]@n", sizeof(modifier_msg));
            } else if (has_amplify) {
                strlcpy(modifier_msg, " @R[Amplificado]@n", sizeof(modifier_msg));
            }

            /* Build the cast command string with cast + spell name in quotes + optional target */
            if (target_part && *target_part)
                snprintf(cast_command, sizeof(cast_command), "cast '%s' %s", spell->name, target_part);
            else
                snprintf(cast_command, sizeof(cast_command), "cast '%s'", spell->name);

            /* Trigger the cast command */
            send_to_char(ch, "As palavras místicas ressoam com poder...%s\r\n", modifier_msg);

            /* Store modifier flags globally for the cast command to use */
            spell_modifier_diminish = has_diminish;
            spell_modifier_amplify = has_amplify;
            command_interpreter(ch, cast_command);
            /* Reset modifier flags after casting */
            spell_modifier_diminish = 0;
            spell_modifier_amplify = 0;

            return 1;
        } else {
            /* Player said the syllables but doesn't know the spell */
            send_to_char(
                ch, "Você sente uma energia estranha ao pronunciar essas palavras místicas, mas nada acontece.\r\n");
            return 1;
        }
    }

    /* No spell matched */
    return 0;
}

/* This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE. */
const char *skill_name(int num) { return get_spell_name(num); }

/* This function is the very heart of the entire magic system.  All invocations
 * of all types of magic -- objects, spoken and unspoken PC and NPC spells, the
 * works -- all come through this function eventually. This is also the entry
 * point for non-spoken or unrestricted spells. Spellnum 0 is legal but silently
 * ignored here, to make callers simpler. */
int call_magic(struct char_data *caster, struct char_data *cvict, struct obj_data *ovict, int spellnum, int level,
               int casttype)
{
    int savetype;
    int i, dur, res, rts_code;
    int damages, flags = 0;
    struct str_spells *spell;
    struct affected_type *af;

    if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
        return (0);

    if (!cast_wtrigger(caster, cvict, ovict, spellnum))
        return 0;
    if (!cast_otrigger(caster, ovict, spellnum))
        return 0;
    if (!cast_mtrigger(caster, cvict, spellnum))
        return 0;

    spell = get_spell_by_vnum(spellnum);
    if (!spell) {
        log1("SYSERR: spell not found vnum %d passed to call_magic.", spellnum);
        send_to_char(caster, "Algo deu errado  e você falha.");
        return 0;
    }

    if (GET_LEVEL(caster) < LVL_GRGOD) {
        if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC)) {
            send_to_char(caster, "A sua magia congela repentinamente e morre.\r\n");
            act("A magia de $n congela repentinamente e morre.", FALSE, caster, 0, 0, TO_ROOM);
            return (0);
        }

        if (spell->status == unavailable) {
            send_to_char(caster, "%s", CONFIG_NOEFFECT);
            return 0;
        }

        if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
            ((spell->mag_flags & MAG_DAMAGE) || (spell->mag_flags & MAG_VIOLENT))) {
            send_to_char(caster, "Um flash de luzes brancas preenchem a sala, consumindo sua magia violenta!\r\n");
            act("Luzes brancas repentinamente preenchem a sala e somem logo em seguida.", FALSE, caster, 0, 0, TO_ROOM);
            return (0);
        }

        if (cvict) {
            if (MOB_FLAGGED(cvict, MOB_NOKILL)) {
                send_to_char(caster, "%s está protegid%s por forças divinas, que te impedem de atacar %s.\r\n",
                             ELEAUpper(cvict), OA(cvict), ELEA(cvict));
                return (0);
            }

            for (af = cvict->affected; af; af = af->next)
                if (IS_SET_AR(AFF_FLAGS(cvict), AFF_PROTECT) && (af->location == spellnum)) {

                    if (af->modifier >= rand_number(0, 99)) {
                        send_to_char(caster, "%s está protegid%s e você falhou.\r\n", ELEAUpper(cvict), OA(cvict));
                        return 0;
                    }
                }
        }
    } /* fim da verificação por grgod */
    /* determine the type of saving throw */
    switch (casttype) {
        case CAST_STAFF:
        case CAST_SCROLL:
        case CAST_POTION:
        case CAST_WAND:
            savetype = SAVING_ROD;
            break;
        case CAST_SPELL:
            savetype = SAVING_SPELL;
            break;
        default:
            savetype = SAVING_BREATH;
            break;
    }

    if (spell->mag_flags & MAG_DAMAGE) {
        if ((damages = mag_damage(level, caster, cvict, spellnum, savetype)) == -1)
            return (-1); /* Successful and target died, don't cast again. */
        if (damages)
            flags = MAGIC_SUCCESS;
    }

    if (spell->mag_flags & MAG_PROTECTION) {
        for (i = 0; i < MAX_SPELL_PROTECTIONS; i++) {
            dur = MAX(
                1, formula_interpreter(caster, cvict, spellnum, TRUE, spell->protfrom[i].duration, level, &rts_code));
            res =
                MAX(0, formula_interpreter(caster, cvict, spellnum, TRUE, spell->protfrom[i].resist, level, &rts_code));
            flags |= mag_protections(level, caster, cvict, spell->vnum, spell->protfrom[i].prot_num, dur, res);
        }
    }

    if (spell->mag_flags & MAG_AFFECTS)
        flags |= mag_affects(level, caster, cvict, spellnum, savetype);

    if (spell->mag_flags & MAG_UNAFFECTS)
        flags |= mag_unaffects(level, caster, cvict, spellnum, savetype);

    if (spell->mag_flags & MAG_POINTS)
        flags |= mag_points(level, caster, cvict, spellnum, savetype);

    if (spell->mag_flags & MAG_ALTER_OBJS)
        flags |= mag_alter_objs(level, caster, ovict, spellnum, savetype);

    if (spell->mag_flags & MAG_GROUPS)
        flags |= mag_groups(level, caster, spellnum, savetype);

    if (spell->mag_flags & MAG_MASSES)
        flags |= mag_masses(level, caster, spellnum, savetype);

    if (spell->mag_flags & MAG_AREAS)
        flags |= mag_areas(level, caster, spellnum, savetype);

    if (spell->mag_flags & MAG_SUMMONS)
        flags |= mag_summons(level, caster, ovict, spellnum, savetype);

    if (spell->mag_flags & MAG_CREATIONS)
        flags |= mag_creations(level, caster, spellnum);

    if (spell->mag_flags & MAG_ROOMS)
        flags |= mag_rooms(level, caster, spellnum);

    if ((spell->mag_flags & MAG_MANUAL) && spell->function) {
        call_ASPELL(spell->function, GET_LEVEL(caster), caster, cvict, ovict);
        flags |= MAGIC_SUCCESS;
    }

    if (spell->script)
        flags |= perform_script(spell->script, caster, cvict, ovict, spell->vnum, 0);

    if (flags & MAGIC_SUCCESS) {
        if (spell->messages.to_self != NULL && (caster != cvict))
            act(spell->messages.to_self, FALSE, caster, ovict, cvict, TO_CHAR);
        if (spell->messages.to_vict != NULL && cvict)
            act(spell->messages.to_vict, FALSE, cvict, ovict, 0, TO_CHAR);
        if (spell->messages.to_room != NULL)
            act(spell->messages.to_room, TRUE, caster, ovict, cvict, TO_ROOM);

        /* Award experience for non-damage spells cast successfully */
        if (!(spell->mag_flags & MAG_DAMAGE) && caster && !IS_NPC(caster)) {
            int exp_gain = 0;

            /* Experience for casting on mobiles (not self) */
            if (cvict && cvict != caster) {
                /* Award experience equivalent to a modest hit (level-based) */
                /* Cap at caster level to prevent power-leveling on high-level NPCs */
                exp_gain = MIN(GET_LEVEL(caster), GET_LEVEL(cvict));
            }
            /* Experience for casting on objects */
            else if (ovict) {
                /* Award small fixed experience like other object interactions */
                exp_gain = rand_number(2, 10);
            }

            /* Grant the experience if any was calculated */
            if (exp_gain > 0) {
                gain_exp(caster, exp_gain);
            }
        }
    } else if (flags & MAGIC_NOEFFECT)
        send_to_char(caster, "%s", CONFIG_NOEFFECT);
    else if (flags & MAGIC_FAILED)
        send_to_char(caster, "Você falhou!\r\n");

    return (1);
}

/* mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 * Staves and wands will default to level 14 if the level is not specified; the
 * DikuMUD format did not specify staff and wand levels in the world files */
void mag_objectmagic(struct char_data *ch, struct obj_data *obj, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int i, k;
    struct char_data *tch = NULL, *next_tch;
    struct obj_data *tobj = NULL;

    one_argument(argument, arg);

    k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tch, &tobj);

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_STAFF:
            act("Você bate $p três vezes no chão.", FALSE, ch, obj, 0, TO_CHAR);
            if (obj->action_description)
                act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
            else
                act("$n bate $p três vezes no chão.", FALSE, ch, obj, 0, TO_ROOM);

            if (GET_OBJ_VAL(obj, 2) <= 0) {
                send_to_char(ch, "Parece estar sem poderes.\r\n");
                act("Aparentemente, nada aconteceu.", FALSE, ch, obj, 0, TO_ROOM);
            } else {
                GET_OBJ_VAL(obj, 2)--;
                WAIT_STATE(ch, PULSE_VIOLENCE);

                if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj)) {
                    send_to_char(ch, "$p brilha mas nada acontece.");
                    act("Aparentemente, nada aconteceu.", FALSE, ch, obj, 0, TO_ROOM);
                } else {
                    /* Level to cast spell at. */
                    k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

                    /* Area/mass spells on staves can cause crashes. So we use special cases
                     * for those spells spells here. */
                    if (IS_SET(get_spell_mag_flags(GET_OBJ_VAL(obj, 3)), MAG_MASSES | MAG_AREAS)) {
                        for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
                            i++;
                        while (i-- > 0)
                            call_magic(ch, NULL, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
                    } else {
                        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
                            next_tch = tch->next_in_room;
                            if (ch != tch)
                                call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
                        }
                    }
                }
            }
            break;
        case ITEM_WAND:
            if (k == FIND_CHAR_ROOM) {
                if (tch == ch) {
                    act("Você aponta $p para sí mesm$r.", FALSE, ch, obj, 0, TO_CHAR);
                    act("$n aponta $p para sí mesm$r.", FALSE, ch, obj, 0, TO_ROOM);
                } else {
                    act("Você aponta $p para $N.", FALSE, ch, obj, tch, TO_CHAR);
                    if (obj->action_description)
                        act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
                    else
                        act("$n aponta $p para $N.", TRUE, ch, obj, tch, TO_ROOM);
                }
            } else if (tobj != NULL) {
                act("Você encosta $p em $P.", FALSE, ch, obj, tobj, TO_CHAR);
                if (obj->action_description)
                    act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
                else
                    act("$n encosta $p em $P.", TRUE, ch, obj, tobj, TO_ROOM);
            } else if (IS_SET(get_spell_mag_flags(GET_OBJ_VAL(obj, 3)), MAG_AREAS | MAG_MASSES)) {
                /* Wands with area spells don't need to be pointed. */
                act("Você aponta $p ao redor.", FALSE, ch, obj, NULL, TO_CHAR);
                act("$n aponta $p ao redor.", TRUE, ch, obj, NULL, TO_ROOM);
            } else {
                act("Para onde $p deve ser apontado?", FALSE, ch, obj, NULL, TO_CHAR);
                return;
            }

            if (GET_OBJ_VAL(obj, 2) <= 0) {
                send_to_char(ch, "Parece estar sem poderes.\r\n");
                act("Aparentemente, nada aconteceu.", FALSE, ch, obj, 0, TO_ROOM);
                return;
            }
            GET_OBJ_VAL(obj, 2)--;
            WAIT_STATE(ch, PULSE_VIOLENCE);
            if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj)) {
                send_to_char(ch, "$p brilha mas nada acontece.");
                act("Aparentemente, nada aconteceu.", FALSE, ch, obj, 0, TO_ROOM);
            } else {
                if (GET_OBJ_VAL(obj, 0))
                    call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 0), CAST_WAND);
                else
                    call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), DEFAULT_WAND_LVL, CAST_WAND);
            }
            break;
        case ITEM_SCROLL:
            if (*arg) {
                if (!k) {
                    act("Não há nada aqui que possa ser afetado por $p.", FALSE, ch, obj, NULL, TO_CHAR);
                    return;
                }
                /* Clear the target that wasn't found */
                if (k == FIND_CHAR_ROOM) {
                    tobj = NULL;
                } else {
                    tch = NULL;
                }
            } else
                tch = ch;

            act("Você recita $p, que dissolve.", TRUE, ch, obj, 0, TO_CHAR);
            if (obj->action_description)
                act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
            else
                act("$n recita $p.", FALSE, ch, obj, NULL, TO_ROOM);

            WAIT_STATE(ch, PULSE_VIOLENCE);
            if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj)) {
                send_to_char(ch, "Estranhamente, nada aconteceu.");
                act("Aparentemente, nada aconteceu.", FALSE, ch, obj, 0, TO_ROOM);
            } else {
                for (i = 1; i <= 3; i++)
                    if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
                        break;
            }
            if (obj != NULL)
                extract_obj(obj);
            break;
        case ITEM_POTION:
            tch = ch;

            if (!consume_otrigger(obj, ch, OCMD_QUAFF)) /* check trigger */
                return;

            act("Você toma $p.", FALSE, ch, obj, NULL, TO_CHAR);
            if (obj->action_description)
                act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
            else
                act("$n toma $p.", TRUE, ch, obj, NULL, TO_ROOM);

            WAIT_STATE(ch, PULSE_VIOLENCE);
            if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj)) {
                send_to_char(ch, "Vicê sente um gosto estranhos, e nada acontece...");
                act("Aparentemente, nada aconteceu.", FALSE, ch, obj, 0, TO_ROOM);
            } else {
                for (i = 1; i <= 3; i++)
                    if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, 0), CAST_POTION) <= 0)
                        break;
            }
            if (obj != NULL)
                extract_obj(obj);
            break;
        default:
            log1("SYSERR: Unknown object_type %d in mag_objectmagic.", GET_OBJ_TYPE(obj));
            break;
    }
}

/* cast_spell is used generically to cast any spoken spell, assuming we already
 * have the target char/obj and spell number.  It checks all restrictions,
 * prints the words, etc. Entry point for NPC casts.  Recommended entry point
 * for spells cast by NPCs via specprocs. */
int cast_spell(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int spellnum)
{
    struct str_spells *spell;

    if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE) {
        log1("SYSERR: cast_spell trying to call spellnum %d/%d.", spellnum, TOP_SPELL_DEFINE);
        return (0);
    }

    spell = get_spell_by_vnum(spellnum);
    if (GET_POS(ch) < spell->min_pos) {
        switch (GET_POS(ch)) {
            case POS_SLEEPING:
                send_to_char(ch, "Você sonha com grandes poderes mágicos.\r\n");
                break;
            case POS_RESTING:
                send_to_char(ch, "Você não pode se concentrar enquanto descansa.\r\n");
                break;
            case POS_SITTING:
                send_to_char(ch, "Você não pode fazer isso sentad%s!\r\n", OA(ch));
                break;
            case POS_FIGHTING:
                send_to_char(ch, "Impossível!  Você não pode se concentrar o suficiente!\r\n");
                break;
            default:
                send_to_char(ch, "Você não pode fazer muito disso!\r\n");
                break;
        }
        return (0);
    }
    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
        send_to_char(ch, "Você tem medo de machucar $X(seu mestre,sua mestra)!\r\n");
        return (0);
    }
    if ((tch != ch) && (spell->targ_flags & TAR_SELF_ONLY)) {
        send_to_char(ch, "Você só pode %s em você mesm$r!\r\n",
                     spell->type == SPELL ? "lançar essa magia" : "fazer isto");
        return (0);
    }
    if ((tch == ch) && (spell->targ_flags & TAR_NOT_SELF)) {
        send_to_char(ch, "Você não pode %s em você mesm$r!\r\n",
                     spell->type == SPELL ? "lançar esta magia" : "fazer isto");
        return (0);
    }
    if ((spell->mag_flags & MAG_GROUPS) && !GROUP(ch)) {
        send_to_char(ch, "Você não pode %s se não estiver em um grupo!\r\n",
                     spell->type == SPELL ? "lançar esta magia" : "fazer isto");
        return (0);
    }
    send_to_char(ch, "%s", CONFIG_OK);

    if (spell->type == SPELL)
        say_spell(ch, spellnum, tch, tobj);
    else if (spell->type == CHANSON)
        say_chanson(ch, spellnum, tch, tobj);

    return (call_magic(ch, tch, tobj, spellnum, GET_LEVEL(ch), CAST_SPELL));
}

ACMD(do_cast)
{
    char *s, *targ = NULL;
    struct char_data *tch = NULL;
    struct obj_data *tobj = NULL;
    struct str_spells *spell = NULL;
    int i, delay, rts_code = TRUE;
    int effectiveness = 0;
    int number;
    int target = 0;
    int mana = 5;

    if (IS_NPC(ch))
        return;

    // for spell check that is is enclosed in ''
    if ((subcmd == SCMD_SPELL) || (subcmd == SCMD_CHANSON)) {
        s = strtok(argument, "'");
        if (s == NULL) {
            send_to_char(ch, "O que e aonde?\r\n");
            return;
        }
        if (!(s = strtok(NULL, "'"))) {
            send_to_char(ch, "Os nomes de magias devem estar entre os Sagrados Símbolos Mágicos: '\r\n");
            return;
        }
        targ = strtok(NULL, "\0");
    } else   // skill
        targ = argument;

    if (subcmd == SCMD_SPELL)
        spell = get_spell_by_name(s, SPELL);
    else if (subcmd == SCMD_CHANSON)
        spell = get_spell_by_name(s, CHANSON);
    else
        spell = get_spell_by_vnum(subcmd);

    if (!spell || (spell->status != available)) {
        if (subcmd == SCMD_SPELL)
            send_to_char(ch, "Lançar o quê?!?\r\n");
        else if (subcmd == SCMD_CHANSON)
            send_to_char(ch, "Cantar o quê?!?\r\n");
        else
            send_to_char(ch, "%s", HUH);
        return;
    }

    // for skills with a function, we only check if status == available, and
    // we return the control to it function.
    if (spell->function && (spell->type == SKILL)) {
        call_ACMD(spell->function, ch, argument, 0, 0);
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        // level = get_spell_level(spell->vnum, GET_CLASS(ch));
        // if ((level == -1) || (GET_LEVEL(ch) < level)) {
        //  send_to_char(ch, "Você não conhece essa %s!\r\n", (spell->type == SPELL) ? "magia" :(spell->type == CHANSON)
        //  ?  "canção": "habilidade");
        //   return;
        // }
        if (GET_SKILL(ch, spell->vnum) == 0) {
            send_to_char(ch, "Você não está familiarizad%s com esta %s.\r\n", OA(ch),
                         (spell->type == SPELL)     ? "magia"
                         : (spell->type == CHANSON) ? "canção"
                                                    : "habilidade");
            return;
        }
    }

    /* Find the target */
    if (targ != NULL) {
        char arg[MAX_INPUT_LENGTH];

        strlcpy(arg, targ, sizeof(arg));
        one_argument(arg, targ);
        skip_spaces(&targ);

        /* Copy target to global cast_arg2, for use in spells like locate object */
        strcpy(cast_arg2, targ);
    }

    if (spell->targ_flags & TAR_IGNORE) {
        target = TRUE;
    } else if (targ != NULL && *targ) {
        number = get_number(&targ);
        if (!target && (spell->targ_flags & TAR_CHAR_ROOM)) {
            if ((tch = get_char_vis(ch, targ, &number, FIND_CHAR_ROOM)) != NULL)
                target = TRUE;
        }
        if (!target && (spell->targ_flags & TAR_CHAR_WORLD))
            if ((tch = get_char_vis(ch, targ, &number, FIND_CHAR_WORLD)) != NULL)
                target = TRUE;

        if (!target && (spell->targ_flags & TAR_OBJ_INV))
            if ((tobj = get_obj_in_list_vis(ch, targ, &number, ch->carrying)) != NULL)
                target = TRUE;

        if (!target && (spell->targ_flags & TAR_OBJ_EQUIP)) {
            for (i = 0; !target && i < NUM_WEARS; i++)
                if (GET_EQ(ch, i) && isname(targ, GET_EQ(ch, i)->name)) {
                    tobj = GET_EQ(ch, i);
                    target = TRUE;
                }
        }
        if (!target && (spell->targ_flags & TAR_OBJ_ROOM))
            if ((tobj = get_obj_in_list_vis(ch, targ, &number, world[IN_ROOM(ch)].contents)) != NULL)
                target = TRUE;

        if (!target && (spell->targ_flags & TAR_OBJ_WORLD))
            if ((tobj = get_obj_vis(ch, targ, &number)) != NULL)
                target = TRUE;

    } else { /* if target string is empty */
        if (!target && (spell->targ_flags & TAR_FIGHT_SELF))
            if (FIGHTING(ch) != NULL) {
                tch = ch;
                target = TRUE;
            }
        if (!target && (spell->targ_flags & TAR_FIGHT_VICT))
            if (FIGHTING(ch) != NULL) {
                tch = FIGHTING(ch);
                target = TRUE;
            }
        /* if no target specified, and the spell isn't violent, default to self */
        if (!target && ((spell->targ_flags & TAR_CHAR_ROOM) || (spell->targ_flags & TAR_SELF_ONLY)) &&
            !(spell->mag_flags & MAG_VIOLENT)) {
            tch = ch;
            target = TRUE;
        }
        if (!target) {
            send_to_char(ch, "Sobre %s essa %s deve ser lançada?\r\n",
                         (spell->targ_flags & (TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)) ? "o que"
                                                                                                            : "quem",
                         (spell->type == SPELL)     ? "magia"
                         : (spell->type == CHANSON) ? "canção"
                                                    : "habilidade");
            return;
        }
    }

    if (target && (tch == ch) && (spell->damages) && (spell->mag_flags & MAG_VIOLENT)) {
        send_to_char(ch, "Você não pode lançar em você mesm$r -- pode ser ruim para sua saúde!");
        return;
    }
    if (!target) {
        send_to_char(ch, "Não foi possível encontrar o alvo de sua %s!\r\n",
                     (spell->type == SPELL) ? "magia" : "habilidade");
        return;
    }

    // spells and chansons cost mana
    if (((spell->type == SPELL) || (spell->type == CHANSON)) && (GET_LEVEL(ch) < LVL_IMMORT)) {
        mana = mag_manacost(ch, tch, spell->vnum);
        if (GET_MANA(ch) < mana) {
            send_to_char(ch, "Você não tem energia para %s!\r\n",
                         (spell->type == SPELL)     ? "lançar essa magia"
                         : (spell->type == CHANSON) ? "cantar essa canção"
                                                    : "usar esta habilidade");
            return;
        }
    }

    if (spell->effectiveness)
        effectiveness =
            GET_SKILL(ch, spell->vnum) *
            MAX(0, formula_interpreter(ch, tch, spell->vnum, TRUE, spell->effectiveness, GET_LEVEL(ch), &rts_code)) /
            100;

    if (rand_number(0, 101) > effectiveness) {
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (!tch || !skill_message(0, ch, tch, spell->vnum))
            send_to_char(ch, "Você perde a concentração!\r\n");

        if ((spell->type == SPELL) && (mana > 0))
            GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana / 2)));

        // if you lost your concentration and the spell is MAG_VIOLENT, you still hit a mobile to start the fight.
        if ((spell->mag_flags & MAG_VIOLENT) && tch && IS_NPC(tch))
            hit(tch, ch, TYPE_UNDEFINED);

        return;
    }

    if (cast_spell(ch, tch, tobj, spell->vnum)) {
        if (spell->delay) {
            delay = MAX(0, formula_interpreter(ch, tch, spell->vnum, TRUE, spell->delay, GET_LEVEL(ch), &rts_code));
            WAIT_STATE(ch, MIN(delay, MAX_SPELL_DELAY));
        } else
            WAIT_STATE(ch, PULSE_VIOLENCE);

        if ((spell->type == SPELL) || (spell->type == CHANSON))
            GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
    }
}
