/**************************************************************************
 *  File: mobact.c                                          Part of tbaMUD *
 *  Usage: Functions for generating intelligent (?) behavior in mobiles.   *
 *                                                                         *
 *  Shadow Timeline Integration: RFC-0003 COMPLIANT                        *
 *  This file consumes Shadow Timeline projections for mob AI decisions.   *
 *  See shadow_timeline.h for core RFC-0003 implementation.                *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "graph.h"
#include "fight.h"
#include "shop.h"
#include "graph.h"
#include "spedit.h"
#include "shop.h"
#include "quest.h"
#include "spec_procs.h"
#include "shadow_timeline.h"
#include "emotion_projection.h"
#include "dg_scripts.h"
#include "sec.h"

/* local file scope only function prototypes */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack);
static bool can_heal_based_on_alignment(struct char_data *healer, struct char_data *target);

/* External function prototypes */
void call_ACMD(void (*function)(), struct char_data *ch, char *argument, int cmd, int subcmd);

struct mob_upgrade_plan find_best_upgrade_for_mob(struct char_data *ch);
struct char_data *find_best_median_leader(struct char_data *ch);
bool mob_handle_grouping(struct char_data *ch);
bool mob_share_gear_with_group(struct char_data *ch);
bool perform_move_IA(struct char_data *ch, int dir, bool should_close_behind, int was_in);
bool mob_goal_oriented_roam(struct char_data *ch, room_rnum target_room);
bool handle_duty_routine(struct char_data *ch);
bool mob_follow_leader(struct char_data *ch);

/**
 * Big Five Phase 2B: Conscientiousness Executive Control Helper
 *
 * Apply conscientiousness modulation to a deliberative decision.
 * This should be called ONCE per decision cycle, not for reflexive actions.
 *
 * @param ch The mob making a decision
 * @param base_impulse_prob Base probability of impulsive action [0.0, 1.0]
 * @param needs_delay Whether this decision should have reaction delay
 * @param delay_ptr Pointer to delay value to modify (can be NULL if !needs_delay)
 * @return Modified impulse probability after conscientiousness filtering
 */
static float apply_executive_control(struct char_data *ch, float base_impulse_prob, bool needs_delay, float *delay_ptr)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data)
        return base_impulse_prob;

    /* Calculate emotional arousal for this decision cycle */
    float arousal = calculate_emotional_arousal(ch);

    /* Apply impulse modulation (high C reduces impulsive actions) */
    float modulated_impulse = apply_conscientiousness_impulse_modulation(ch, base_impulse_prob);

    /* Apply reaction delay if needed (high C increases deliberation under arousal)
     * Remove the > 0.0f check to allow delay calculation even when starting from 0 */
    if (needs_delay && delay_ptr) {
        *delay_ptr = apply_conscientiousness_reaction_delay(ch, *delay_ptr, arousal);
    }

    return modulated_impulse;
}

bool mob_try_stealth_follow(struct char_data *ch);
bool mob_assist_allies(struct char_data *ch);
bool mob_try_heal_ally(struct char_data *ch);
bool mob_try_and_loot(struct char_data *ch);
bool mob_try_and_upgrade(struct char_data *ch);
bool mob_manage_inventory(struct char_data *ch);
bool mob_handle_item_usage(struct char_data *ch);
bool mob_try_to_sell_junk(struct char_data *ch);
struct obj_data *find_unblessed_weapon_or_armor(struct char_data *ch);
struct obj_data *find_cursed_item_in_inventory(struct char_data *ch);
struct obj_data *find_bank_nearby(struct char_data *ch);
bool mob_use_bank(struct char_data *ch);
struct char_data *get_mob_in_room_by_rnum(room_rnum room, mob_rnum rnum);
struct char_data *get_mob_in_room_by_vnum(room_rnum room, mob_vnum vnum);
struct char_data *find_questmaster_by_vnum(mob_vnum vnum);
struct char_data *find_mob_by_vnum(mob_vnum vnum);
room_rnum find_object_location(obj_vnum obj_vnum);
void mob_process_wishlist_goals(struct char_data *ch);
bool mob_try_to_accept_quest(struct char_data *ch);
bool mob_process_quest_completion(struct char_data *ch, qst_rnum quest_rnum);
bool mob_try_donate(struct char_data *ch, struct obj_data *obj);
bool mob_try_sacrifice(struct char_data *ch, struct obj_data *corpse);
bool mob_try_junk(struct char_data *ch, struct obj_data *obj);
bool mob_try_drop(struct char_data *ch, struct obj_data *obj);

/* Function to find where a key can be obtained */
room_rnum find_key_location(obj_vnum key_vnum, int *source_type, mob_vnum *carrying_mob);
bool mob_set_key_collection_goal(struct char_data *ch, obj_vnum key_vnum, int original_goal, room_rnum original_dest);
bool validate_goal_obj(struct char_data *ch);
bool shadow_should_activate(struct char_data *ch);

/** Function to handle mob leveling when they gain enough experience.
 * Mobs automatically distribute improvements to stats and abilities
 * since they don't have practice points like players.
 * @param ch The mob character to potentially level up. */
void check_mob_level_up(struct char_data *ch)
{
    if (!IS_MOB(ch) || !ch->ai_data)
        return;

    /* Calculate experience needed for next level (simplified formula) */
    int exp_needed = GET_LEVEL(ch) * 1000;

    if (GET_EXP(ch) >= exp_needed && GET_LEVEL(ch) < LVL_IMMORT - 1) {
        GET_LEVEL(ch)++;
        GET_EXP(ch) -= exp_needed;

        /* Automatically improve stats based on genetics tendencies */
        if (ch->ai_data->genetics.brave_prevalence > 50) {
            GET_STR(ch) = MIN(GET_STR(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.quest_tendency > 50) {
            GET_INT(ch) = MIN(GET_INT(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.use_tendency > 50) {
            GET_WIS(ch) = MIN(GET_WIS(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.roam_tendency > 50) {
            GET_DEX(ch) = MIN(GET_DEX(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.equip_tendency > 50) {
            GET_CON(ch) = MIN(GET_CON(ch) + 1, 25);
        }
        if (ch->ai_data->genetics.trade_tendency > 50) {
            GET_CHA(ch) = MIN(GET_CHA(ch) + 1, 25);
        }

        /* Improve hit points based on constitution */
        int hp_gain = rand_number(1, 10) + con_app[GET_CON(ch)].hitp;
        GET_MAX_HIT(ch) += MAX(hp_gain, 1);
        GET_HIT(ch) = GET_MAX_HIT(ch); /* Full heal on level up */

        /* Improve mana for spellcasting mobs */
        if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC) {
            int mana_gain = rand_number(1, 8) + int_app[GET_INT(ch)].learn;
            GET_MAX_MANA(ch) += MAX(mana_gain, 1);
            GET_MANA(ch) = GET_MAX_MANA(ch);
        }

        /* Skills automatically improve with level - handled in get_mob_skill() */

        /* Message for nearby players to see the mob improving */
        if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world && world[IN_ROOM(ch)].people) {
            struct char_data *viewer, *next_viewer;
            for (viewer = world[IN_ROOM(ch)].people; viewer; viewer = next_viewer) {
                next_viewer = viewer->next_in_room;
                if (IS_MOB(viewer) || viewer == ch)
                    continue;
                act("$n parece ter ficado mais experiente!", TRUE, ch, 0, viewer, TO_VICT);
            }
        }
    }
}

/**
 * Validates that goal_obj is still valid (in mob's inventory).
 * Objects can be extracted at any time through various means (sold, dropped, junked, etc.)
 * so we need to verify the goal_obj pointer still points to a valid object.
 * @param ch The mob character to validate goal_obj for.
 * @return TRUE if goal_obj is valid and in mob's inventory, FALSE otherwise.
 */
bool validate_goal_obj(struct char_data *ch)
{
    if (!ch || !ch->ai_data || !ch->ai_data->goal_obj)
        return FALSE;

    /* Check if the goal_obj is still in the mob's inventory */
    struct obj_data *obj;
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (obj == ch->ai_data->goal_obj) {
            return TRUE; /* Found it - it's still valid */
        }
    }

    /* Object not found in inventory - it was extracted or moved */
    ch->ai_data->goal_obj = NULL;
    return FALSE;
}

/**
 * Makes a mob perform a contextual social based on target's reputation, alignment, gender, position, and mob's emotions
 * @param ch The mob performing the social
 * @param target The target of the social (can be player or mob)
 */
static void mob_contextual_social(struct char_data *ch, struct char_data *target)
{
    /* Social lists for mob autonomous behaviour.
     * Every social in lib/misc/socials.new (except extreme-violence ones that are
     * only appropriate as targeted reactions, not as autonomous expressions) is
     * reachable from at least one list below.
     *
     * Intentionally excluded (never used autonomously – always hostile):
     *   choke, strangle, smite, sword, despine, shiskabob, vice
     */

    /* ── Positive / warm ─────────────────────────────────────────────── */
    const char *positive_socials[] = {
        "bow",        "smile",     "nods",     "nod",     "waves",  "wink",     "applaud", "agree",     "beam",
        "clap",       "grin",      "greet",    "thanks",  "thank",  "thumbsup", "welcome", "winks",     "backclap",
        "sweetsmile", "happy",     "admire",   "adoring", "cackle", "chitter",  "croon",   "enthuse",   "gleam",
        "halo",       "handshake", "highfive", "hum",     "nuzzle", "snuggle",  "squeeze", "stroke",    "tango",
        "whistle",    "worship",   "yodel",    "lol",     "yes",    "ok",       "ack",     "apologize", "forgive",
        "handraise",  "lau",       "nudge",    "beckon",  "sorry",  "touch",    "rainbow", NULL};

    /* ── Negative / hostile expressions ──────────────────────────────── */
    const char *negative_socials[] = {"frown",      "glare",   "spit",   "sneer",     "accuse", "growl", "snarl",
                                      "curse",      "mock",    "hate",   "steam",     "swear",  "blame", "criticize",
                                      "disapprove", "evileye", "fume",   "grimace",   "ignore", "scold", "snap",
                                      "rebuke",     "wrong",   "nono",   "raspberry", "pooh",   "stomp", "swat",
                                      "sue",        "lame",    "tongue", "smir",      NULL};

    /* ── Neutral / observational ──────────────────────────────────────── */
    const char *neutral_socials[] = {"ponder",  "shrugs",    "peer",    "blink",  "wonder",      "think",    "wait",
                                     "scratch", "yawn",      "stretch", "comb",   "contemplate", "cough",    "daydream",
                                     "hiccup",  "hush",      "listen",  "mumble", "mutter",      "nailfile", "pace",
                                     "point",   "sage",      "shh",     "sneeze", "snore",       "type",     "wipe",
                                     "slump",   "slouch",    "lean",    "roll",   "jog",         "sidle",    "scout",
                                     "sheathe", "unsheathe", "modest",  "raise",  "tip",         "revolve",  "dark",
                                     "bored",   "flip",      "pity",    "furrow", "fade",        "nodrugs",  "night",
                                     "run",     "tap",       "curl",    "pin",    "job",         "mooch",    NULL};

    /* ── Resting / caring / comforting ───────────────────────────────── */
    const char *resting_socials[] = {"comfort", "pat",     "calm",       "console", "cradle",
                                     "tuck",    "massage", "haircut",    "relax",   "breathe",
                                     "relief",  "phew",    "spongebath", "conso",   NULL};

    /* ── Fearful / submissive / sad ───────────────────────────────────── */
    const char *fearful_socials[] = {"cower",  "whimper", "cringe",   "flinch", "gasp",    "panic",    "shake",
                                     "worry",  "shivers", "beg",      "blue",   "crushed", "crylaugh", "dread",
                                     "eek",    "eep",     "quiver",   "shy",    "sigh",    "whine",    "puppyeyes",
                                     "squeak", "meep",    "insomnia", NULL};

    /* ── Loving / romantic ────────────────────────────────────────────── */
    const char *loving_socials[] = {"hug",    "cuddle", "kiss",   "bearhug", "blush",  "caress", "embrace",
                                    "flirt",  "love",   "swoon",  "charm",   "huggle", "ghug",   "rose",
                                    "honey",  "lust",   "nibble", "ogle",    "peck",   "pet",    "propose",
                                    "smooch", "snog",   "sways",  "chu",     NULL};

    /* ── Very intimate (sexual) – requires very high relationship ─────── */
    const char *very_intimate_socials[] = {"sex", "french", "fondle", "grope", "seduce", "makeout", NULL};

    /* ── Proud / boastful ─────────────────────────────────────────────── */
    const char *proud_socials[] = {"strut", "flex",  "boast",    "brag", "pose",   "ego",  "juggle",
                                   "model", "pride", "drumroll", "mic",  "pushup", "smug", NULL};

    /* ── Envious / greedy ─────────────────────────────────────────────── */
    const char *envious_socials[] = {"envy", "eye", "greed", NULL};

    /* ── Playful / teasing ────────────────────────────────────────────── */
    const char *playful_socials[] = {
        "tickle",  "poke",     "tease",  "bounce",    "giggle",  "dance",    "skip",   "joke",   "sing",
        "knuckle", "snowball", "tackle", "spank",     "vampire", "boink",    "bonk",   "bop",    "cartwheel",
        "hop",     "hustle",   "jest",   "madgiggle", "mosh",    "nyuk",     "pillow", "pop",    "pounce",
        "pout",    "pur",      "rofl",   "squeal",    "tag",     "tapdance", "tug",    "tummy",  "tweak",
        "twiddle", "waggle",   "waltz",  "wiggle",    "goose",   "noogie",   "pinch",  "ruffle", "suckit-up",
        "wedgie",  "zaps",     "spin",   "hotfoot",   "hula",    "splash",   "flail",  NULL};

    /* ── Aggressive / combat-flavoured ───────────────────────────────── */
    const char *aggressive_socials[] = {"threaten",  "challenge", "growl",  "snarl",   "bite",  "slap",     "battlecry",
                                        "warscream", "smash",     "smack",  "charge",  "burn",  "pound",    "whack",
                                        "whip",      "shock",     "needle", "clobber", "thwap", "shootout", "mace",
                                        "flame",     "fwap",      "arrest", NULL};

    /* ── Disgusting ───────────────────────────────────────────────────── */
    const char *disgusting_socials[] = {"belch",  "booger",   "burp", "drool", "earlick", "fart", "gag",  "moan",
                                        "phlegm", "picknose", "puke", "snort", "spew",    "moon", "pant", NULL};

    /* ── Sad / mourning ───────────────────────────────────────────────── */
    const char *sad_socials[] = {"cry", "sob", "weep", "sulk", "sad", NULL};

    /* ── Confused / puzzled ───────────────────────────────────────────── */
    const char *confused_socials[] = {"boggle",    "blink",     "puzzle",  "wonder", "scratch", "think",  "confuse",
                                      "discombob", "disturbed", "doh",     "duh",    "eww",     "gibber", "hmmmmm",
                                      "hrmph",     "lost",      "newidea", "jaw",    "...",     NULL};

    /* ── Excited / celebratory ────────────────────────────────────────── */
    const char *excited_socials[] = {"bounce", "whoo", "cheers",   "huzzah", "tada",
                                     "yayfor", "romp", "sundance", "toast",  NULL};

    /* ── Respectful ───────────────────────────────────────────────────── */
    const char *respectful_socials[] = {"salute", "curtsey", "kneel", NULL};

    /* ── Grateful ─────────────────────────────────────────────────────── */
    const char *grateful_socials[] = {"thanks", "bow", "applaud", "backclap", "beam", "salute", NULL};

    /* ── Mocking ──────────────────────────────────────────────────────── */
    const char *mocking_socials[] = {"mock", "sneer", "snicker", "jeer", "taunt", NULL};

    /* ── Submissive ───────────────────────────────────────────────────── */
    const char *submissive_socials[] = {"cower",  "grovel", "bow",       "kneel", "whimper",
                                        "cringe", "flinch", "puppyeyes", "shy",   NULL};

    /* ── Curious ──────────────────────────────────────────────────────── */
    const char *curious_socials[] = {"peer", "ponder",  "wonder", "sniff", "gaze", "stare",
                                     "lean", "curious", "creep",  "scout", NULL};

    /* ── Triumphant ───────────────────────────────────────────────────── */
    const char *triumphant_socials[] = {"cheers", "flex", "roar", "battlecry", "strut", NULL};

    /* ── Protective / comforting ──────────────────────────────────────── */
    const char *protective_socials[] = {"embrace", "pat", "comfort", "console", NULL};

    /* ── Mourning ─────────────────────────────────────────────────────── */
    const char *mourning_socials[] = {"cry", "sob", "weep", "sulk", "despair", NULL};

    /* ── Angry expression (frustration/outrage) ──────────────────────── */
    const char *angry_expression_socials[] = {"argh", "grumbles", "grunt", "hysterical", "postal", "tantrum",
                                              "fuss", "stomp",    "fume",  "snap",       "steam",  NULL};

    /* ── Amused ───────────────────────────────────────────────────────── */
    const char *amused_socials[] = {"amused", "chortle", "egrin", "snigger", "titter", "cackle", "lol", "rofl", NULL};

    /* ── Animal sounds ────────────────────────────────────────────────── */
    const char *animal_socials[] = {"bark", "hiss", "howl", "meow", "moo", "pur", NULL};

    /* ── Silly / absurd ───────────────────────────────────────────────── */
    const char *silly_socials[] = {
        "abc",        "abrac",   "babble",  "batman", "bloob",  "christmas", "crazed", "crazy",      "defib",  "doodle",
        "elephantma", "fish",    "insane",  "meds",   "prozac", "snoopy",    "spork",  "testsocial", "wakka",  "zip",
        "lala",       "muahaha", "lofr",    "dbc",    "bo",     "pa",        "sal",    "sm",         "ki",     "ren",
        "orcs",       "humans",  "mortals", "joint",  "lz",     "com",       "dead",   "deaf",       "gibber", NULL};

    /* ── Communication / meta ─────────────────────────────────────────── */
    const char *communication_socials[] = {"adieu", "brb", "channel", "goodbye", "reconnect", "wb", "night", NULL};

    /* ── Food / drink ─────────────────────────────────────────────────── */
    const char *food_drink_socials[] = {"beer", "cake", "carrot", "coffee", "custard", "pie", NULL};

    /* ── Gesture ──────────────────────────────────────────────────────── */
    const char *gesture_socials[] = {"arch",     "armcross", "behind", "crossfinger", "eyebrow", "eyer",
                                     "facegrab", "facepalm", "fan",    "foot",        NULL};

    /* ── Exclamation ──────────────────────────────────────────────────── */
    const char *exclamation_socials[] = {"ahem", "aww", "blah", "boo", "heh", "oh", "ouch", "tsk", NULL};

    /* ── Self-directed / physical expression ─────────────────────────── */
    const char *self_directed_socials[] = {"bleed", "collapse", "faint",  "fall",    "groan", "headache", "perspire",
                                           "pray",  "scream",   "shiver", "shudder", "sweat", "twitch",   "wince",
                                           "shame", "froth",    "foam",   "curl",    "flail", "blush",    NULL};

    /* ── Miscellaneous neutral (catch-all for remaining socials) ──────── */
    const char *misc_socials[] = {
        "aim",        "avsalute", "bat",    "box",      "buzz",  "cold",    "conga",    "conga2",  "creep", "curious",
        "scab",       "scuf",     "secret", "slippers", "stone", "sunset",  "sunshade", "target",  "tie",   "trance",
        "understand", "wipe",     "women",  "pull",     "pulse", "lic",     "mooch",    "muffle",  "muss",  "knight",
        "flare",      "flash",    "flick",  "floor",    "flop",  "flutter", "innocent", "smoke",   "dive",  "duck",
        "excuse",     "sidle",    "differ", "rainbow",  "pin",   "lame",    "pity",     "hotfoot", NULL};

    const char **social_list = NULL;
    int target_reputation;
    int mob_alignment;
    int target_pos;
    int social_index;
    int cmd_num;

    /* Core emotions */
    int mob_anger, mob_happiness, mob_fear, mob_love, mob_friendship, mob_sadness;
    /* Extended emotions */
    int mob_trust, mob_loyalty, mob_curiosity, mob_greed, mob_pride, mob_compassion, mob_envy, mob_courage,
        mob_excitement;

    /* Validate parameters */
    if (!ch || !target)
        return;

    /* Only mobs with ai_data can use emotions */
    if (!IS_NPC(ch) || !ch->ai_data)
        return;

    target_reputation = GET_REPUTATION(target);
    mob_alignment = GET_ALIGNMENT(ch);
    target_pos = GET_POS(target);

    /* Get mob core emotions */
    mob_anger = GET_MOB_ANGER(ch);
    mob_happiness = GET_MOB_HAPPINESS(ch);
    mob_fear = GET_MOB_FEAR(ch);
    mob_love = GET_MOB_LOVE(ch);
    mob_friendship = GET_MOB_FRIENDSHIP(ch);
    mob_sadness = GET_MOB_SADNESS(ch);

    /* Get extended emotions */
    mob_trust = GET_MOB_TRUST(ch);
    mob_loyalty = GET_MOB_LOYALTY(ch);
    mob_curiosity = GET_MOB_CURIOSITY(ch);
    mob_greed = GET_MOB_GREED(ch);
    mob_pride = GET_MOB_PRIDE(ch);
    mob_compassion = GET_MOB_COMPASSION(ch);
    mob_envy = GET_MOB_ENVY(ch);
    mob_courage = GET_MOB_COURAGE(ch);
    mob_excitement = GET_MOB_EXCITEMENT(ch);

    /* Don't perform socials if mob is busy or target is fighting/dead */
    if (FIGHTING(ch) || FIGHTING(target) || target_pos <= POS_STUNNED)
        return;

    /* ── Emotion-based social selection (priority order) ─────────────────
     * Extreme states are checked first; moderate states and context-based
     * selections follow.  The default is misc_socials (catch-all). */

    /* Very high fear (80+) and very low courage (20-) - submissive behaviour */
    if (mob_fear >= 80 && mob_courage <= 20) {
        social_list = submissive_socials;
    }
    /* High fear (70+) and low courage - fearful behaviour */
    else if (mob_fear >= 70 && mob_courage < 40) {
        social_list = fearful_socials;
    }
    /* Very high pride (85+) with high courage after victory - triumphant */
    else if (mob_pride >= 85 && mob_courage >= 70 && mob_excitement >= 60) {
        social_list = triumphant_socials;
    }
    /* High pride (75+) - proud / boastful */
    else if (mob_pride >= 75) {
        social_list = proud_socials;
    }
    /* High compassion (70+) with loyalty towards injured/weak target - protective */
    else if (mob_compassion >= 70 && mob_loyalty >= 60 &&
             (target_pos <= POS_RESTING || GET_HIT(target) < GET_MAX_HIT(target) / 2)) {
        social_list = protective_socials;
    }
    /* High sadness (75+) with low excitement - mourning */
    else if (mob_sadness >= 75 && mob_excitement < 30) {
        social_list = mourning_socials;
    }
    /* High happiness (60+) with very high friendship (70+) and trust (60+) - grateful */
    else if (mob_happiness >= 60 && mob_friendship >= 70 && mob_trust >= 60) {
        social_list = grateful_socials;
    }
    /* High anger (60+) with high pride (60+) but not fully aggressive - mocking */
    else if (mob_anger >= 60 && mob_pride >= 60 && mob_courage >= 50 && !FIGHTING(ch)) {
        social_list = mocking_socials;
    }
    /* Very high love (85+) AND trust (75+) AND friendship (75+) - very intimate.
     * Placed before envy/curiosity so deep intimacy takes priority. */
    else if (mob_love >= 85 && mob_trust >= 75 && mob_friendship >= 75) {
        social_list = very_intimate_socials;
    }
    /* High curiosity (75+) with moderate excitement - curious/investigating */
    else if (mob_curiosity >= 75 && mob_excitement >= 40 && mob_excitement < 70) {
        social_list = curious_socials;
    }
    /* High envy (70+) and target has good equipment/reputation */
    else if (mob_envy >= 70 && (target_reputation >= 50 || mob_greed >= 60)) {
        social_list = envious_socials;
    }
    /* High love (70+) or very high friendship with high trust */
    else if (mob_love >= 70 || (mob_friendship >= 80 && mob_trust >= 60)) {
        social_list = loving_socials;
    }
    /* High anger (70+) or high anger with low loyalty to good targets - aggressive */
    else if (mob_anger >= 70 || (mob_anger >= 50 && mob_loyalty < 30 && IS_GOOD(target))) {
        social_list = aggressive_socials;
    }
    /* Moderate anger (40-69) expressing frustration - angry expression */
    else if (mob_anger >= 40 && mob_anger < 70 && mob_courage >= 30) {
        social_list = angry_expression_socials;
    }
    /* Moderate anger with low courage - negative behaviour */
    else if (mob_anger >= 50 && mob_courage < 50) {
        social_list = negative_socials;
    }
    /* Low happiness (<40) with moderate anger (30+) and low pride (<50) – mob
     * performs gross or distasteful acts out of apathy or resentment */
    else if (mob_happiness < 40 && mob_anger >= 30 && mob_pride < 50) {
        social_list = disgusting_socials;
    }
    /* High sadness (70+) - sad behaviour */
    else if (mob_sadness >= 70) {
        social_list = sad_socials;
    }
    /* High happiness (70+) with very high excitement (75+) - excited */
    else if (mob_happiness >= 70 && mob_excitement >= 75) {
        social_list = excited_socials;
    }
    /* High happiness (70+) with high excitement (60-74) and high curiosity - amused */
    else if (mob_happiness >= 70 && mob_excitement >= 60 && mob_curiosity >= 60) {
        social_list = amused_socials;
    }
    /* High happiness (70+) with high excitement (60-74) - playful */
    else if (mob_happiness >= 70 && mob_excitement >= 60 && mob_excitement < 75) {
        social_list = playful_socials;
    }
    /* High happiness (70+) with high curiosity (70+) but moderate excitement - silly */
    else if (mob_happiness >= 70 && mob_curiosity >= 70 && mob_excitement < 60) {
        social_list = silly_socials;
    }
    /* High curiosity (70+) with moderate happiness (30-69) - confused/wondering */
    else if (mob_curiosity >= 70 && mob_happiness >= 30 && mob_happiness < 70) {
        social_list = confused_socials;
    }
    /* High happiness (70+) shows positive behaviour */
    else if (mob_happiness >= 70) {
        social_list = positive_socials;
    }
    /* High reputation target (60+) - positive from happy/friendly mobs */
    else if (target_reputation >= 60 && (mob_happiness >= 40 || mob_friendship >= 50 || mob_alignment >= -350)) {
        social_list = positive_socials;
    }
    /* Low reputation target (<20) - negative response */
    else if (target_reputation < 20 || mob_anger >= 40) {
        social_list = negative_socials;
    }
    /* Alignment-based selection */
    else if (IS_GOOD(ch) && IS_EVIL(target) && mob_anger >= 20) {
        social_list = negative_socials;
    } else if (IS_EVIL(ch) && IS_GOOD(target) && mob_sadness < 50) {
        social_list = negative_socials;
    } else if (IS_GOOD(ch) && IS_GOOD(target) && mob_friendship >= 30) {
        social_list = positive_socials;
    } else if (IS_EVIL(ch) && IS_EVIL(target) && target_reputation >= 40) {
        social_list = respectful_socials;
    } else if (IS_GOOD(ch) && target_reputation >= 80) {
        social_list = respectful_socials;
    }
    /* Target is resting/sitting - comforting socials (low anger, high compassion/friendship) */
    else if ((target_pos == POS_RESTING || target_pos == POS_SITTING) && mob_anger < 30 &&
             (mob_compassion >= 50 || mob_friendship >= 40)) {
        social_list = resting_socials;
    }
    /* High sadness with low excitement - withdrawn/neutral */
    else if (mob_sadness >= 60 && mob_excitement < 40) {
        social_list = neutral_socials;
    }
    /* Moderate curiosity (40+) with moderate but not high excitement (30-49) –
     * mob makes instinctive animal sounds while alert/observing but not fully engaged */
    else if (mob_curiosity >= 40 && mob_excitement >= 30 && mob_excitement < 50) {
        social_list = animal_socials;
    }
    /* Moderate happiness + any state - food/drink sharing */
    else if (mob_happiness >= 50 && mob_friendship >= 40 && mob_curiosity >= 30) {
        social_list = food_drink_socials;
    }
    /* Moderate happiness + moderate curiosity - gesture/communication */
    else if (mob_happiness >= 40 && mob_curiosity >= 50) {
        social_list = gesture_socials;
    }
    /* Low excitement with low curiosity - exclamation */
    else if (mob_excitement < 30 && mob_curiosity < 40 && mob_happiness >= 30) {
        social_list = exclamation_socials;
    }
    /* High curiosity with high excitement - show interest */
    else if (mob_curiosity >= 60 && mob_excitement >= 50) {
        social_list = neutral_socials;
    }
    /* Low emotional arousal - self-directed physical expression */
    else if (mob_happiness < 40 && mob_anger < 30 && mob_fear >= 20) {
        social_list = self_directed_socials;
    }
    /* Communication meta (very low arousal, no strong emotion) */
    else if (mob_happiness < 50 && mob_anger < 30 && mob_excitement < 30) {
        social_list = communication_socials;
    }
    /* Default - miscellaneous / catch-all */
    else {
        social_list = misc_socials;
    }

    /* ── Winner-Takes-All: SEC dominant-emotion filter ────────────────────
     * Prevents contradictory actions when opposing emotions are simultaneously
     * high by suppressing socials whose driving emotion is weaker than
     * SEC_WTA_THRESHOLD % of the dominant SEC axis weight. */
    if (ch->ai_data) {
        const struct sec_state *sec_s = &ch->ai_data->sec;

        /* Map chosen social category to its primary SEC emotion.
         *
         * HAPPINESS : positive/grateful/loving/very_intimate/playful/excited/
         *             triumphant/proud/protective/amused/food_drink
         * ANGER     : aggressive/negative/mocking/envious/angry_expression/disgusting
         * FEAR      : fearful/submissive
         * SADNESS   : sad/mourning
         * NONE      : neutral/mixed, resting, respectful, curious, confused,
         *             silly, animal, gesture, exclamation, self_directed,
         *             communication, misc — contextual/cognitive, not restricted
         */
        int social_emotion;
        if (social_list == positive_socials || social_list == grateful_socials || social_list == loving_socials ||
            social_list == very_intimate_socials || social_list == playful_socials || social_list == excited_socials ||
            social_list == triumphant_socials || social_list == proud_socials || social_list == protective_socials ||
            social_list == amused_socials || social_list == food_drink_socials) {
            social_emotion = SEC_DOMINANT_HAPPINESS;
        } else if (social_list == aggressive_socials || social_list == negative_socials ||
                   social_list == mocking_socials || social_list == envious_socials ||
                   social_list == angry_expression_socials || social_list == disgusting_socials) {
            social_emotion = SEC_DOMINANT_ANGER;
        } else if (social_list == fearful_socials || social_list == submissive_socials) {
            social_emotion = SEC_DOMINANT_FEAR;
        } else if (social_list == sad_socials || social_list == mourning_socials) {
            social_emotion = SEC_DOMINANT_SADNESS;
        } else {
            social_emotion = SEC_DOMINANT_NONE;
        }

        if (social_emotion != SEC_DOMINANT_NONE) {
            int dom_type = sec_get_dominant_emotion(ch);

            float dom_val;
            switch (dom_type) {
                case SEC_DOMINANT_FEAR:
                    dom_val = sec_s->fear;
                    break;
                case SEC_DOMINANT_SADNESS:
                    dom_val = sec_s->sadness;
                    break;
                case SEC_DOMINANT_ANGER:
                    dom_val = sec_s->anger;
                    break;
                case SEC_DOMINANT_HAPPINESS:
                    dom_val = sec_s->happiness;
                    break;
                default:
                    dom_val = 0.0f;
                    break;
            }

            float chosen_val;
            if (social_emotion == SEC_DOMINANT_HAPPINESS)
                chosen_val = sec_s->happiness;
            else if (social_emotion == SEC_DOMINANT_ANGER)
                chosen_val = sec_s->anger;
            else if (social_emotion == SEC_DOMINANT_SADNESS)
                chosen_val = sec_s->sadness;
            else
                chosen_val = sec_s->fear;

            if (dom_val > SEC_AROUSAL_EPSILON && chosen_val < dom_val * ((float)CONFIG_SEC_WTA_THRESHOLD / 100.0f))
                social_list = neutral_socials;
        }
    }

    /* Select a random social from the chosen category */
    for (social_index = 0; social_list[social_index] != NULL; social_index++)
        ;

    if (social_index == 0)
        return;

    social_index = rand_number(0, social_index - 1);

    /* Find the social command number */
    for (cmd_num = 0; *complete_cmd_info[cmd_num].command != '\n'; cmd_num++) {
        if (!strcmp(complete_cmd_info[cmd_num].command, social_list[social_index]))
            break;
    }

    if (*complete_cmd_info[cmd_num].command != '\n') {
        /* Execute the social */
        do_action(ch, GET_NAME(target), cmd_num, 0);
    }
}

/**
 * Separate heartbeat pass for mob emotion and social behavior.
 * Called more frequently than mobile_activity() to make mobs feel more alive.
 * Runs at PULSE_MOB_EMOTION (every 4 seconds) vs PULSE_MOBILE (every 10 seconds).
 */
void mob_emotion_activity(void)
{
    struct char_data *ch, *next_ch;

    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;

        /* Skip if we've reached the end of the list or if this is not a mob */
        if (!ch || !IS_MOB(ch))
            continue;

        /* Skip mobs that have been marked for extraction */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Safety check: Skip mobs that are not in a valid room */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
            continue;

        /* Skip if mob is fighting or not awake */
        if (FIGHTING(ch) || !AWAKE(ch))
            continue;

        /* Mobs perform contextual socials based on reputation, alignment, gender, and position */
        /* Only perform if experimental feature is enabled */
        /* Probability controlled by CONFIG_MOB_EMOTION_SOCIAL_CHANCE (configurable in cedit) */
        /* Emotion modifiers: high happiness/anger increases chance, high sadness decreases */
        int social_chance = CONFIG_MOB_EMOTION_SOCIAL_CHANCE;

        if (ch->ai_data) {
            /* Lethargy Buffer: high SEC sadness strongly suppresses all social activity.
             * A lethargic mob (sadness-dominant) withdraws from interaction — it will
             * not perform socials unless the lethargy bias is low enough to allow it.
             * Skip directly when lethargy_bias >= SEC_LETHARGY_SUPPRESS_THRESHOLD (strongly lethargic). */
            float lethargy = sec_get_lethargy_bias(ch);
            if (lethargy >= SEC_LETHARGY_SUPPRESS_THRESHOLD)
                continue;
            /* Moderate lethargy scales down the social chance proportionally. */
            if (lethargy > 0.0f)
                social_chance = (int)(social_chance * (1.0f - lethargy));

            /* High happiness increases social probability */
            if (ch->ai_data->emotion_happiness >= CONFIG_EMOTION_SOCIAL_HAPPINESS_HIGH_THRESHOLD) {
                social_chance += 15; /* +15% for high happiness */
            }
            /* High anger increases negative social probability */
            if (ch->ai_data->emotion_anger >= CONFIG_EMOTION_SOCIAL_ANGER_HIGH_THRESHOLD) {
                social_chance += 10; /* +10% for high anger */
            }
            /* High sadness decreases social probability (withdrawal) */
            if (ch->ai_data->emotion_sadness >= CONFIG_EMOTION_SOCIAL_SADNESS_HIGH_THRESHOLD) {
                social_chance -= 15; /* -15% for high sadness (withdrawn) */
            }
            /* Extraversion (E) modulates social action probability.
             * High E (sociable) increases the chance; low E (introverted) reduces it.
             * Formula: E_final ∈ [0,1] → modifier = (E_final - 0.5) * 20 ∈ [-10, +10].
             * This is a gain-rate modulation, not an emotion injection. */
            float E_final = sec_get_extraversion_final(ch);
            int e_mod = (int)((E_final - SEC_E_SOCIAL_CENTER) * SEC_E_SOCIAL_SCALE);
            social_chance += e_mod;
            /* Ensure social_chance stays within reasonable bounds */
            social_chance = MAX(1, MIN(social_chance, 95));
        }

        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && rand_number(1, 100) <= social_chance) {
            struct char_data *potential_target;

            /* Look for a suitable target in the room */
            for (potential_target = world[IN_ROOM(ch)].people; potential_target;
                 potential_target = potential_target->next_in_room) {
                if (potential_target == ch)
                    continue;

                /* Skip if target is fighting, sleeping, or dead */
                if (FIGHTING(potential_target) || GET_POS(potential_target) <= POS_SLEEPING)
                    continue;

                /* Can the mob see the target? */
                if (!CAN_SEE(ch, potential_target))
                    continue;

                /* Perform contextual social */
                mob_contextual_social(ch, potential_target);

                /* Safety check: do_action can trigger DG scripts which may cause extraction */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    break; /* Exit the for loop and continue to next mob */

                /* Only target one character per social action */
                break;
            }

            /* If mob was extracted during social, continue to next mob in main loop */
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                continue;
        }

        /* Love-based following: Mobs with high love (>80) toward specific players should follow them */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && ch->ai_data) {
            /* Look for a player to follow if not already following someone */
            if (!ch->master && !MOB_FLAGGED(ch, MOB_SENTINEL) && !MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
                struct char_data *potential_love_target;
                struct char_data *best_love_target = NULL;
                int highest_love = 0;

                /* Find the player the mob loves most using hybrid emotion system */
                for (potential_love_target = world[IN_ROOM(ch)].people; potential_love_target;
                     potential_love_target = potential_love_target->next_in_room) {
                    /* Skip self and other mobs */
                    if (potential_love_target == ch || IS_NPC(potential_love_target))
                        continue;

                    /* Must be able to see the target */
                    if (!CAN_SEE(ch, potential_love_target))
                        continue;

                    /* Check effective love toward this specific player */
                    int effective_love = get_effective_emotion_toward(ch, potential_love_target, EMOTION_TYPE_LOVE);
                    if (effective_love >= CONFIG_EMOTION_SOCIAL_LOVE_FOLLOW_THRESHOLD &&
                        effective_love > highest_love) {
                        highest_love = effective_love;
                        best_love_target = potential_love_target;
                    }
                }

                /* Follow the most loved player if found */
                if (best_love_target) {
                    add_follower(ch, best_love_target);

                    /* Announce the following with a loving social or message */
                    act("$n olha para $N com adoração e começa a seguir $M.", FALSE, ch, 0, best_love_target,
                        TO_NOTVICT);
                    act("$n olha para você com adoração e começa a seguir você.", FALSE, ch, 0, best_love_target,
                        TO_VICT);

                    /* Safety check for extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        break;

                    /* Only follow one player per tick */
                    break;
                }

                /* If mob was extracted, continue to next mob */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    continue;
            }
        }

        /* Passive emotion regulation - emotions gradually return to baseline (experimental feature) */
        /* Probability controlled by CONFIG_MOB_EMOTION_UPDATE_CHANCE (configurable in cedit) */
        if (CONFIG_MOB_CONTEXTUAL_SOCIALS && rand_number(1, 100) <= CONFIG_MOB_EMOTION_UPDATE_CHANCE) {
            update_mob_emotion_passive(ch);

            /* Emotion contagion - mobs influence each other's emotions */
            /* Run less frequently than passive updates (50% chance when passive runs) */
            if (rand_number(1, 100) <= 50) {
                update_mob_emotion_contagion(ch);
            }

            /* Mood system - update overall mood less frequently (25% chance when passive runs) */
            /* This calculates mood from emotions and applies weather/time effects */
            if (rand_number(1, 100) <= 25) {
                update_mob_mood(ch);
            }

            /* Extreme emotional states - check for maxed/minimized emotions and breakdowns */
            /* Run less frequently (20% chance when passive runs) */
            if (rand_number(1, 100) <= 20) {
                check_extreme_emotional_states(ch);
            }
        }

    } /* end for() */
}

void mobile_activity(void)
{
    struct char_data *ch, *next_ch, *vict;
    int found;
    memory_rec *names;

    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;

        if (!ch || !IS_MOB(ch))
            continue;

        /* Skip mobs that have been marked for extraction (e.g., from death traps) */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Safety check: Skip mobs that are not in a valid room */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world) {
            log1("SYSERR: Mobile %s (#%d) is in NOWHERE or invalid room in mobile_activity().",
                 ch ? GET_NAME(ch) : "NULL", ch ? GET_MOB_VNUM(ch) : -1);
            continue;
        }

        /* Examine call for special procedure */
        if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
            if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
                log1("SYSERR: %s (#%d): Attempting to call non-existing mob function.", GET_NAME(ch), GET_MOB_VNUM(ch));
                REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
            } else {
                char actbuf[MAX_INPUT_LENGTH] = "";
                if ((mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, actbuf))
                    continue; /* go to next char */
            }
        }

        // *** ADDED SAFETY CHECK ***
        // A spec proc might have extracted ch and returned FALSE.
        // The check at the top of the loop already caught other flags,
        // but we must re-check here before proceeding.
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* 4D Relational Decision Space: compute projection state once per AI tick.
         * This runs for both fighting and non-fighting mobs so the 4D state is
         * always current when downstream systems (Shadow Timeline, combat, social)
         * consume it.
         *
         * Target priority:
         *  1. Current fight target (always authoritative; overrides hysteresis).
         *  2. Previous idle-fallback target if still valid in this room (hysteresis:
         *     prevents oscillation when multiple candidates share the same room).
         *  3. First visible, awake, non-extracting character in the room.
         *  4. NULL.
         *
         * Hysteresis strategy: the entity ID/type of the last idle target is stored
         * in mob_ai_data and reused as long as the character remains a valid target.
         * A new scan is only run when the stored target is gone or no longer eligible.
         *
         * We allow NPC targets because:
         *  - get_relationship_emotion() fully supports mob-to-mob memories.
         *  - FIGHTING(ch) can already be a mob; the idle fallback should be consistent.
         *  - Mob-to-mob Affiliation/Dominance drives group dynamics and loyalty. */
        if (ch->ai_data && CONFIG_MOB_CONTEXTUAL_SOCIALS) {
            struct char_data *target_4d = FIGHTING(ch);

            if (target_4d) {
                /* Combat target always wins; clear stored idle target to avoid stale
                 * hysteresis resuming after combat ends. */
                ch->ai_data->last_4d_target_id = 0;
            } else if (IN_ROOM(ch) != NOWHERE) {
                /* Check whether the stored hysteresis target is still valid. */
                struct char_data *sticky = NULL;
                if (ch->ai_data->last_4d_target_id != 0) {
                    struct char_data *scan;
                    long want_id = ch->ai_data->last_4d_target_id;
                    int want_type = ch->ai_data->last_4d_target_type;
                    for (scan = world[IN_ROOM(ch)].people; scan; scan = scan->next_in_room) {
                        if (scan == ch)
                            continue;
                        bool type_match =
                            IS_NPC(scan) ? (want_type == ENTITY_TYPE_MOB) : (want_type == ENTITY_TYPE_PLAYER);
                        if (!type_match)
                            continue;
                        long scan_id = IS_NPC(scan) ? char_script_id(scan) : GET_IDNUM(scan);
                        if (scan_id != want_id)
                            continue;
                        /* Found – validate eligibility */
                        if (CAN_SEE(ch, scan) && AWAKE(scan) && (!IS_NPC(scan) || !MOB_FLAGGED(scan, MOB_NOTDEADYET))) {
                            sticky = scan;
                        }
                        break; /* ID is unique in room */
                    }
                }

                if (sticky) {
                    target_4d = sticky;
                } else {
                    /* Scan for a new target and store it for hysteresis. */
                    struct char_data *nearby;
                    for (nearby = world[IN_ROOM(ch)].people; nearby; nearby = nearby->next_in_room) {
                        if (nearby != ch && CAN_SEE(ch, nearby) && AWAKE(nearby) &&
                            (!IS_NPC(nearby) || !MOB_FLAGGED(nearby, MOB_NOTDEADYET))) {
                            target_4d = nearby;
                            ch->ai_data->last_4d_target_id =
                                IS_NPC(nearby) ? char_script_id(nearby) : GET_IDNUM(nearby);
                            ch->ai_data->last_4d_target_type = IS_NPC(nearby) ? ENTITY_TYPE_MOB : ENTITY_TYPE_PLAYER;
                            break;
                        }
                    }
                    if (!target_4d)
                        ch->ai_data->last_4d_target_id = 0;
                }
            }

            ch->ai_data->last_4d_state = compute_emotion_4d_state(ch, target_4d);

            /* HELPLESSNESS: post-4D deformation of Dominance and Arousal axes.
             * D_final = D_base * (1 - H/100)  → Helplessness erodes perceived control.
             * A_final = A_base * (1 - H/150)  → Helplessness slightly dampens urgency.
             * Behavior naturally shifts toward disengagement as the 4D region changes.
             * Only affects the effective axes; raw values are preserved for auditing. */
            if (ch->ai_data->helplessness > 0.0f && ch->ai_data->last_4d_state.valid) {
                float h_factor_d = MAX(0.0f, 1.0f - (ch->ai_data->helplessness / 100.0f));
                float h_factor_a = MAX(0.0f, 1.0f - (ch->ai_data->helplessness / 150.0f));
                ch->ai_data->last_4d_state.dominance *= h_factor_d;
                ch->ai_data->last_4d_state.arousal *= h_factor_a;
                if (CONFIG_MOB_4D_DEBUG)
                    log1("4D-HELPLESSNESS: mob=%s(#%d) H=%.1f D_deformed=%.1f A_deformed=%.1f", GET_NAME(ch),
                         GET_MOB_VNUM(ch), ch->ai_data->helplessness, ch->ai_data->last_4d_state.dominance,
                         ch->ai_data->last_4d_state.arousal);
            }

            if (CONFIG_MOB_4D_DEBUG)
                log_4d_state(ch, target_4d, &ch->ai_data->last_4d_state);

            /* SEC: update internal emotional projections from post-hysteresis 4D result. */
            sec_update(ch, &ch->ai_data->last_4d_state);
        }

        if (ch->ai_data && ch->ai_data->duty_frustration_timer > 0) {
            ch->ai_data->duty_frustration_timer--;
        }

        if (ch->ai_data && ch->ai_data->quest_posting_frustration_timer > 0) {
            ch->ai_data->quest_posting_frustration_timer--;
        }

        /* HELPLESSNESS: decay faster out of combat, slower in combat.
         * Placed before the FIGHTING/AWAKE continue so fighting mobs also decay. */
        if (ch->ai_data && ch->ai_data->helplessness > 0.0f) {
            ch->ai_data->helplessness -= FIGHTING(ch) ? 1.0f : 5.0f;
            if (ch->ai_data->helplessness < 0.0f)
                ch->ai_data->helplessness = 0.0f;
        }

        /* SEC: passive decay toward emotional baseline when arousal is low. */
        if (ch->ai_data)
            sec_passive_decay(ch);

        if (FIGHTING(ch) || !AWAKE(ch))
            continue;

        /* Skip paralyzed mobs - they cannot perform actions */
        if (AFF_FLAGGED(ch, AFF_PARALIZE))
            continue;

        /* Check if mob can level up from gained experience */
        check_mob_level_up(ch);

        /* RFC-0003 §7.2: Regenerate Shadow Timeline cognitive capacity */
        /* Cognitive cost regenerates naturally over time */
        if (ch->ai_data) {
            shadow_regenerate_capacity(ch);
        }

        /* RFC-0003 COMPLIANT: Shadow Timeline decision-making */
        /* RFC-0003 §6.1: Only autonomous decision-making entities may consult */
        /* RFC-0003 §6.2: Entity must have internal decision logic and action selection */
        /* Only for mobs with SHADOWTIMELINE flag and sufficient cognitive capacity */
        if (MOB_FLAGGED(ch, MOB_SHADOWTIMELINE) && ch->ai_data &&
            ch->ai_data->cognitive_capacity >= COGNITIVE_CAPACITY_MIN && shadow_should_activate(ch)) {
            struct shadow_action action;
            bool shadow_action_executed = FALSE;

            /* RFC-0003 §5.1: Shadow Timeline proposes possibilities, never asserts facts */
            /* Use Shadow Timeline to choose next action based on non-authoritative projection */
            if (mob_shadow_choose_action(ch, &action)) {
                /* Capture pre-execution HP snapshot for feedback system */
                ch->ai_data->last_hp_snapshot = GET_HIT(ch);
                /* Record chosen action type for consistency and novelty tracking.
                 * action_repetition_count builds when the same type is chosen consecutively,
                 * giving the depth-aware O novelty bonus time to grow before a switch occurs. */
                if (ch->ai_data->last_chosen_action_type == (int)action.type) {
                    ch->ai_data->action_repetition_count++;
                } else {
                    ch->ai_data->action_repetition_count = 1;
                }
                ch->ai_data->last_chosen_action_type = (int)action.type;

                /* RFC-0003 §4.2: Execute action in live world (Shadow Timeline never mutates) */
                /* RFC-0003 §5.2: Action chosen from hypothetical, probabilistic projections */

                /* Execute the chosen action based on type */
                switch (action.type) {
                    case SHADOW_ACTION_MOVE:
                        /* Attempt movement in projected direction */
                        if (action.direction >= 0 && action.direction < NUM_OF_DIRS) {
                            perform_move(ch, action.direction, 1);
                            shadow_action_executed = TRUE;
                            goto shadow_feedback_and_continue;
                        }
                        break;

                    case SHADOW_ACTION_ATTACK:
                        /* Attack projected target if still valid and not already fighting */
                        if (action.target && !FIGHTING(ch)) {
                            struct char_data *target = (struct char_data *)action.target;
                            /* Verify target still exists and is in same room */
                            if (IN_ROOM(target) == IN_ROOM(ch)) {
                                /* Big Five Phase 2B: Apply Conscientiousness to Shadow Timeline Attack Decision
                                 * This is a DELIBERATIVE choice from projected futures.
                                 * High C reduces impulsive attack execution, adds reaction delay under arousal.
                                 * Base impulse = 0.8 (shadow projections are inherently somewhat impulsive)
                                 * Base delay = 1.0 tick (unit baseline for executive modulation) */
                                float base_impulse = 0.8f;
                                float base_delay = 1.0f; /* Unit baseline for delay modulation */
                                float modulated_impulse = apply_executive_control(ch, base_impulse, TRUE, &base_delay);

                                /* Check if impulse succeeds (probabilistic execution) */
                                if (rand_number(0, 100) <= (int)(modulated_impulse * 100.0f)) {
                                    /* Apply reaction delay if high C under arousal */
                                    if (base_delay > 0.5f) {
                                        /* Delay implemented as probability of postponing action
                                         * Higher delay = higher chance to skip this tick */
                                        int delay_chance = (int)(base_delay * 20.0f); /* Scale to percentage */
                                        if (rand_number(0, 100) < delay_chance) {
                                            /* Deliberation causes hesitation - skip action this tick */
                                            shadow_action_executed = FALSE;
                                            break;
                                        }
                                    }

                                    /* Execute attack */
                                    hit(ch, target, TYPE_UNDEFINED);
                                    shadow_action_executed = TRUE;
                                    goto shadow_feedback_and_continue;
                                } else {
                                    /* Impulse suppressed by high C - don't attack */
                                    shadow_action_executed = FALSE;
                                }
                            }
                        }
                        break;

                    case SHADOW_ACTION_FLEE:
                        /* Flee if we're actually in combat */
                        if (FIGHTING(ch)) {
                            /* If mob has a master or followers it is abandoning them */
                            if (IS_NPC(ch) && ch->ai_data && (ch->master || ch->followers))
                                add_active_emotion_memory(ch, FIGHTING(ch),
                                    INTERACT_ABANDON_ALLY, 1, "flee");
                            do_flee(ch, "", 0, 0);
                            shadow_action_executed = TRUE;
                            goto shadow_feedback_and_continue;
                        }
                        break;

                    case SHADOW_ACTION_SOCIAL:
                        /* Perform social action with target */
                        if (action.target) {
                            struct char_data *target = (struct char_data *)action.target;
                            if (IN_ROOM(target) == IN_ROOM(ch)) {
                                /* Simple friendly social - could be expanded */
                                act("$n parece interessado em $N.", FALSE, ch, 0, target, TO_NOTVICT);
                                act("$n olha para você de forma interessada.", FALSE, ch, 0, target, TO_VICT);
                            }
                        }
                        break;

                    case SHADOW_ACTION_USE_ITEM:
                        /* Use item from inventory (e.g., consume potion, use key) */
                        if (action.target) {
                            struct obj_data *obj = (struct obj_data *)action.target;
                            /* Verify object still exists and is carried by mob */
                            if (obj->carried_by == ch) {
                                /* For now, focus on consumables (food, drink, potions) */
                                if (GET_OBJ_TYPE(obj) == ITEM_POTION) {
                                    /* Quaff potion */
                                    act("$n bebe $p.", TRUE, ch, obj, 0, TO_ROOM);
                                    /* Apply potion effects would go here */
                                    extract_obj(obj);
                                    shadow_action_executed = TRUE;
                                    goto shadow_feedback_and_continue;
                                } else if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
                                    /* Eat food */
                                    act("$n come $p.", TRUE, ch, obj, 0, TO_ROOM);
                                    extract_obj(obj);
                                    shadow_action_executed = TRUE;
                                    goto shadow_feedback_and_continue;
                                }
                            }
                        }
                        break;

                    case SHADOW_ACTION_CAST_SPELL:
                        /* Delegate to the real mob spell/item system.
                         * The old placeholder emitted "conjura uma magia" messages
                         * without casting a real spell.  mob_handle_item_usage()
                         * finds the best item+spell combination in the mob's
                         * inventory, calls cast_spell(), and handles all messages
                         * through the normal spell system. */
                        if (mob_handle_item_usage(ch)) {
                            shadow_action_executed = TRUE;
                            goto shadow_feedback_and_continue;
                        }
                        break;

                    case SHADOW_ACTION_TRADE:
                        /* Trading/shopping action - visit shop or trade with other mobs */
                        /* This would typically involve pathfinding to shop keeper */
                        /* For now, just acknowledge the intention */
                        if (action.target) {
                            struct char_data *shopkeeper = (struct char_data *)action.target;
                            if (IN_ROOM(shopkeeper) == IN_ROOM(ch) && IS_NPC(shopkeeper)) {
                                /* Simple trade interaction */
                                act("$n examina os itens de $N.", TRUE, ch, 0, shopkeeper, TO_NOTVICT);
                                act("$n examina seus itens.", TRUE, ch, 0, shopkeeper, TO_VICT);
                                /* Actual buy/sell would be implemented here */
                            }
                        }
                        break;

                    case SHADOW_ACTION_QUEST:
                        /* Quest-related action (accept or complete quest) */
                        if (ch->ai_data) {
                            /* Try to complete quest if mob has active quest */
                            if (ch->ai_data->current_quest != NOTHING) {
                                mob_complete_quest(ch);
                                shadow_action_executed = TRUE;
                                goto shadow_feedback_and_continue;
                            }
                            /* Try to accept quest if target is a questmaster */
                            else if (action.target) {
                                struct char_data *questmaster = (struct char_data *)action.target;
                                /* Verify questmaster still exists and is in same room */
                                if (IN_ROOM(questmaster) == IN_ROOM(ch) && IS_NPC(questmaster)) {
                                    /* Find available quest from this questmaster */
                                    qst_vnum available_quest =
                                        find_mob_available_quest_by_qmnum(ch, GET_MOB_VNUM(questmaster));
                                    if (available_quest != NOTHING) {
                                        qst_rnum quest_rnum = real_quest(available_quest);
                                        if (quest_rnum != NOTHING && mob_can_accept_quest_forced(ch, quest_rnum)) {
                                            set_mob_quest(ch, quest_rnum);
                                            act("$n fala com $N e aceita uma tarefa.", FALSE, ch, 0, questmaster,
                                                TO_ROOM);
                                            shadow_action_executed = TRUE;
                                            /* Note: Don't goto feedback here, let it fall through to execute feedback
                                             */
                                            /* Safety check: act() can trigger DG scripts which may cause extraction */
                                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                                goto shadow_feedback_and_continue;
                                        }
                                    }
                                }
                            }
                        }
                        break;

                    case SHADOW_ACTION_FOLLOW:
                        /* Follow another entity */
                        if (action.target && !ch->master) {
                            struct char_data *leader = (struct char_data *)action.target;
                            /* Verify leader in same room and not already following */
                            if (IN_ROOM(leader) == IN_ROOM(ch) && leader != ch) {
                                /* add_follower sends "começa a seguir" messages itself */
                                add_follower(ch, leader);
                                shadow_action_executed = TRUE;
                                goto shadow_feedback_and_continue;
                            }
                        }
                        break;

                    case SHADOW_ACTION_GROUP:
                        /* Group formation action */
                        if (action.target) {
                            struct char_data *potential_member = (struct char_data *)action.target;
                            /* Verify target in same room */
                            if (IN_ROOM(potential_member) == IN_ROOM(ch)) {
                                /* If mob is following someone, express desire to group */
                                if (ch->master == potential_member) {
                                    /* Request to join group or show loyalty */
                                    act("$n se aproxima de $N em um gesto amigável.", FALSE, ch, 0, potential_member,
                                        TO_NOTVICT);
                                    act("$n se aproxima de você.", FALSE, ch, 0, potential_member, TO_VICT);
                                    /* For NPC-to-NPC, this strengthens the follow bond */
                                    if (IS_NPC(potential_member) && ch->ai_data) {
                                        ch->ai_data->emotion_loyalty = MIN(100, ch->ai_data->emotion_loyalty + 5);
                                    }
                                    shadow_action_executed = TRUE;
                                    goto shadow_feedback_and_continue;
                                }
                            }
                        }
                        break;

                    case SHADOW_ACTION_WAIT:
                        /* Intentionally wait/do nothing this tick */
                        shadow_action_executed = TRUE;
                        goto shadow_feedback_and_continue;

                    case SHADOW_ACTION_GUARD:
                        /* Stand guard at post - sentinel duty fulfilled */
                        /* No action needed, just maintain vigilance */
                        shadow_action_executed = TRUE;
                        goto shadow_feedback_and_continue;

                    default:
                        /* Other action types not yet implemented in mob_activity */
                        break;
                }

                /* For actions that didn't execute via goto, evaluate feedback if needed */
                if (shadow_action_executed) {
                    int real_score = shadow_evaluate_real_outcome(ch);
                    shadow_update_feedback(ch, real_score, ch->ai_data->last_outcome_obvious);
                }
            }

            /* RFC-0003 §10.1: Shadow Timeline projections are ephemeral and non-persistent */
            /* Projection context has been freed by mob_shadow_choose_action */

            /* Only skip normal goal processing if Shadow Timeline successfully executed an action.
             * If ST didn't handle the action (shadow_action_executed == FALSE), fall through to
             * normal goal processing. This allows mobs with active goals (like GOAL_COMPLETE_QUEST)
             * to continue processing their goals even when Shadow Timeline is enabled but doesn't
             * provide an appropriate action. This prevents mobs from getting stuck when they accept
             * a quest but Shadow Timeline doesn't handle quest processing. */
            if (shadow_action_executed) {
                continue; /* Skip rest of mob_activity for this mob */
            }
            /* Otherwise, fall through to normal goal processing below */

        shadow_feedback_and_continue:
            /* Evaluate feedback for actions that successfully executed */
            {
                int real_score = shadow_evaluate_real_outcome(ch);
                shadow_update_feedback(ch, real_score, ch->ai_data->last_outcome_obvious);
            }
            continue; /* Skip rest of mob_activity for this mob */
        }

        if (ch->ai_data && ch->ai_data->current_goal != GOAL_NONE) {
            /* Re-verify room validity before complex AI operations */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world) {
                ch->ai_data->current_goal = GOAL_NONE;
                continue;
            }

            /* Increment goal timer and check for timeout */
            ch->ai_data->goal_timer++;

            /* Categorize goals for frustration timeout handling:
             * - Quest-related goals get higher threshold (150 ticks = ~15 minutes) to allow
             *   reaching distant rooms for room-finding quests
             * - Shopping/key collection goals use lower threshold (50 ticks = ~5 minutes)
             */
            bool is_quest_goal =
                (ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER || ch->ai_data->current_goal == GOAL_ACCEPT_QUEST ||
                 ch->ai_data->current_goal == GOAL_COMPLETE_QUEST);
            bool is_shopping_goal =
                (ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_SELL ||
                 ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_BUY || ch->ai_data->current_goal == GOAL_COLLECT_KEY);

            /* If stuck on goal for too long, abandon it */
            if ((is_quest_goal || is_shopping_goal) && ch->ai_data->goal_timer > (is_quest_goal ? 150 : 50)) {
                act("$n parece frustrado e desiste da viagem.", FALSE, ch, 0, 0, TO_ROOM);
                /* Safety check: act() can trigger DG scripts which may cause extraction */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    continue;

                /* If abandoning quest completion, fail the quest */
                if (ch->ai_data->current_goal == GOAL_COMPLETE_QUEST) {
                    fail_mob_quest(ch, "timeout");
                }

                /* If abandoning key collection, restore original goal if it exists */
                if (ch->ai_data->current_goal == GOAL_COLLECT_KEY && ch->ai_data->original_goal != GOAL_NONE) {
                    ch->ai_data->current_goal = ch->ai_data->original_goal;
                    ch->ai_data->goal_destination = ch->ai_data->original_destination;
                    ch->ai_data->goal_obj = ch->ai_data->original_obj;
                    ch->ai_data->goal_target_mob_rnum = ch->ai_data->original_target_mob;
                    ch->ai_data->goal_item_vnum = ch->ai_data->original_item_vnum;
                    ch->ai_data->goal_timer = 0;
                    /* Clear original goal data */
                    ch->ai_data->original_goal = GOAL_NONE;
                    ch->ai_data->original_destination = NOWHERE;
                    ch->ai_data->original_obj = NULL;
                    ch->ai_data->original_target_mob = NOBODY;
                    ch->ai_data->original_item_vnum = NOTHING;
                    continue; /* Continue with restored goal */
                } else {
                    ch->ai_data->current_goal = GOAL_NONE;
                    ch->ai_data->goal_destination = NOWHERE;
                    ch->ai_data->goal_obj = NULL;
                    ch->ai_data->goal_target_mob_rnum = NOBODY;
                    ch->ai_data->goal_item_vnum = NOTHING;
                    ch->ai_data->goal_timer = 0;
                    continue; /* Allow other priorities this turn */
                }
            }

            /* Handle goals that don't require movement */
            if (ch->ai_data->current_goal == GOAL_HUNT_TARGET || ch->ai_data->current_goal == GOAL_GET_GOLD) {

                /* For hunting, implement basic hunting behavior */
                if (ch->ai_data->current_goal == GOAL_HUNT_TARGET) {
                    /* Look for the target mob in the current room */
                    struct char_data *target = NULL;
                    struct char_data *temp_char;

                    /* Verify room validity before accessing people list */
                    if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                        for (temp_char = world[IN_ROOM(ch)].people; temp_char; temp_char = temp_char->next_in_room) {
                            if (!IS_NPC(temp_char))
                                continue;
                            if (GET_MOB_RNUM(temp_char) == ch->ai_data->goal_target_mob_rnum) {
                                target = temp_char;
                                break;
                            }
                        }
                    }

                    if (target && !FIGHTING(ch)) {
                        /* Attack the target */
                        act("$n se concentra em $N com olhos determinados.", FALSE, ch, 0, target, TO_ROOM);
                        /* Safety check: act() can trigger DG scripts which may cause extraction */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                        if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET)) {
                            /* Target was extracted, give up hunting */
                            ch->ai_data->current_goal = GOAL_NONE;
                            ch->ai_data->goal_target_mob_rnum = NOBODY;
                            ch->ai_data->goal_item_vnum = NOTHING;
                            ch->ai_data->goal_timer = 0;
                            continue;
                        }
                        hit(ch, target, TYPE_UNDEFINED);
                        /* Safety check: hit() can indirectly cause extract_char */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    } else if (ch->ai_data->goal_timer > 100) {
                        /* Give up hunting after too long */
                        ch->ai_data->current_goal = GOAL_NONE;
                        ch->ai_data->goal_target_mob_rnum = NOBODY;
                        ch->ai_data->goal_item_vnum = NOTHING;
                        ch->ai_data->goal_timer = 0;
                    }
                    continue;
                }

                /* For getting gold, prioritize normal looting behavior */
                if (ch->ai_data->current_goal == GOAL_GET_GOLD) {
                    /* The mob's normal looting behavior will be more active */
                    /* This is handled elsewhere in the code */
                    if (ch->ai_data->goal_timer > 200) { /* Give up after longer time */
                        ch->ai_data->current_goal = GOAL_NONE;
                        ch->ai_data->goal_item_vnum = NOTHING;
                        ch->ai_data->goal_timer = 0;
                    }
                    continue;
                }
            }

            /* Handle resource gathering goals */
            if (ch->ai_data->current_goal == GOAL_MINE || ch->ai_data->current_goal == GOAL_FISH ||
                ch->ai_data->current_goal == GOAL_FORAGE || ch->ai_data->current_goal == GOAL_EAVESDROP) {

                /* Check if mob can perform the resource action */
                bool can_perform = FALSE;

                if (ch->ai_data->current_goal == GOAL_MINE) {
                    /* Check if location is suitable for mining */
                    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && GET_SKILL(ch, SKILL_MINE) > 0) {
                        can_perform = TRUE;
                    }
                } else if (ch->ai_data->current_goal == GOAL_FISH) {
                    /* Check if there's water nearby */
                    if ((SECT(IN_ROOM(ch)) == SECT_WATER_SWIM || SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) &&
                        GET_SKILL(ch, SKILL_FISHING) > 0) {
                        can_perform = TRUE;
                    }
                } else if (ch->ai_data->current_goal == GOAL_FORAGE) {
                    /* Check if location has vegetation */
                    if (SECT(IN_ROOM(ch)) != SECT_CITY && SECT(IN_ROOM(ch)) != SECT_INSIDE &&
                        GET_SKILL(ch, SKILL_FORAGE) > 0) {
                        can_perform = TRUE;
                    }
                } else if (ch->ai_data->current_goal == GOAL_EAVESDROP) {
                    /* Check if there are other people to eavesdrop on */
                    struct char_data *temp_char;
                    /* Verify room validity before accessing people list */
                    if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                        for (temp_char = world[IN_ROOM(ch)].people; temp_char; temp_char = temp_char->next_in_room) {
                            if (temp_char == ch)
                                continue;
                            if (GET_POS(temp_char) >= POS_RESTING && GET_SKILL(ch, SKILL_EAVESDROP) > 0) {
                                can_perform = TRUE;
                                break;
                            }
                        }
                    }
                }

                if (can_perform && rand_number(1, 100) <= 5) { /* 30% chance to perform action each round */
                    /* Perform the appropriate resource action - disabled for reduce resources usage until fix*/
                    switch (ch->ai_data->current_goal) {
                        case GOAL_MINE:
                            // call_ACMD(do_mine, ch, "", 0, 0);
                            break;
                        case GOAL_FISH:
                            // call_ACMD(do_fishing, ch, "", 0, 0);
                            break;
                        case GOAL_FORAGE:
                            // call_ACMD(do_forage, ch, "", 0, 0);
                            break;
                        case GOAL_EAVESDROP:
                            call_ACMD(do_eavesdrop, ch, "", 0, 0);
                            break;
                    }

                    /* Safety check: call_ACMD functions might indirectly trigger extract_char
                     * through complex chains (e.g., triggering scripts, special procedures, etc.) */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;

                    /* After performing action, continue with this goal for a while longer */
                    if (ch->ai_data->goal_timer > rand_number(30, 60)) {
                        /* Sometimes switch to a different goal or give up */
                        ch->ai_data->current_goal = GOAL_NONE;
                        ch->ai_data->goal_timer = 0;
                    }
                } else if (ch->ai_data->goal_timer > 100) {
                    /* Give up if can't perform action or been trying too long */
                    ch->ai_data->current_goal = GOAL_NONE;
                    ch->ai_data->goal_timer = 0;
                }
                continue;
            }

            room_rnum dest = ch->ai_data->goal_destination;

            /* Validate destination room */
            if (dest != NOWHERE && (dest < 0 || dest > top_of_world)) {
                ch->ai_data->current_goal = GOAL_NONE;
                ch->ai_data->goal_destination = NOWHERE;
                continue;
            }

            /* Já chegou ao destino? */
            if (dest != NOWHERE && IN_ROOM(ch) == dest) {
                if (ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_SELL) {
                    /* Usa a memória para encontrar o lojista correto. */
                    struct char_data *keeper = get_mob_in_room_by_rnum(IN_ROOM(ch), ch->ai_data->goal_target_mob_rnum);

                    /* Validate keeper exists and is not marked for extraction */
                    if (!keeper || MOB_FLAGGED(keeper, MOB_NOTDEADYET) || PLR_FLAGGED(keeper, PLR_NOTDEADYET)) {
                        /* Keeper not available, abandon this goal */
                        ch->ai_data->current_goal = GOAL_NONE;
                        ch->ai_data->goal_destination = NOWHERE;
                        ch->ai_data->goal_obj = NULL;
                        ch->ai_data->goal_target_mob_rnum = NOBODY;
                        ch->ai_data->goal_item_vnum = NOTHING;
                        ch->ai_data->goal_timer = 0;
                        continue;
                    }

                    if (ch->ai_data->goal_obj) {
                        /* Validate goal_obj is still in inventory before accessing it */
                        if (!validate_goal_obj(ch)) {
                            /* Object was extracted, clear goal and continue */
                            ch->ai_data->current_goal = GOAL_NONE;
                            ch->ai_data->goal_destination = NOWHERE;
                            ch->ai_data->goal_target_mob_rnum = NOBODY;
                            ch->ai_data->goal_item_vnum = NOTHING;
                            ch->ai_data->goal_timer = 0;
                            continue;
                        }

                        int shop_rnum = find_shop_by_keeper(keeper->nr);

                        /* Check if this shop actually buys this type of item */
                        bool shop_buys_this_item = FALSE;
                        if (shop_rnum != -1 && shop_rnum <= top_shop && shop_index[shop_rnum].type) {
                            for (int i = 0; i < MAX_TRADE && SHOP_BUYTYPE(shop_rnum, i) != NOTHING; i++) {
                                if (SHOP_BUYTYPE(shop_rnum, i) == GET_OBJ_TYPE(ch->ai_data->goal_obj)) {
                                    shop_buys_this_item = TRUE;
                                    break;
                                }
                            }
                        }

                        if (shop_buys_this_item) {
                            /* Shop buys this item, proceed with sale */
                            shopping_sell(ch->ai_data->goal_obj->name, ch, keeper, shop_rnum);
                            /* Clear goal_obj pointer as the object has been extracted by shopping_sell */
                            ch->ai_data->goal_obj = NULL;

                            /* Safety check: shopping_sell can trigger DG Scripts via act(), do_tell(), etc.
                             * which could indirectly cause extract_char for ch or keeper */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;

                            /* Check if keeper was extracted during the transaction */
                            if (MOB_FLAGGED(keeper, MOB_NOTDEADYET) || PLR_FLAGGED(keeper, PLR_NOTDEADYET)) {
                                /* Keeper was extracted, clear goal and continue */
                                ch->ai_data->current_goal = GOAL_NONE;
                                ch->ai_data->goal_destination = NOWHERE;
                                ch->ai_data->goal_target_mob_rnum = NOBODY;
                                ch->ai_data->goal_item_vnum = NOTHING;
                                ch->ai_data->goal_timer = 0;
                                continue;
                            }

                            ch->ai_data->genetics.trade_tendency += 1;
                            ch->ai_data->genetics.trade_tendency = MIN(ch->ai_data->genetics.trade_tendency, 100);
                        } else {
                            /* Shop doesn't buy this item, find a new shop */
                            /* Revalidate goal_obj before using it (could have been extracted during shop processing) */
                            if (!validate_goal_obj(ch)) {
                                ch->ai_data->current_goal = GOAL_NONE;
                                ch->ai_data->goal_destination = NOWHERE;
                                ch->ai_data->goal_target_mob_rnum = NOBODY;
                                ch->ai_data->goal_item_vnum = NOTHING;
                                ch->ai_data->goal_timer = 0;
                                continue;
                            }

                            int new_shop_rnum = find_best_shop_to_sell(ch, ch->ai_data->goal_obj);
                            if (new_shop_rnum != -1 && new_shop_rnum <= top_shop && shop_index[new_shop_rnum].in_room) {
                                room_rnum new_target_room = real_room(SHOP_ROOM(new_shop_rnum, 0));
                                if (new_target_room != NOWHERE && find_first_step(IN_ROOM(ch), new_target_room) != -1) {
                                    /* Found a new shop, update goal */
                                    ch->ai_data->goal_destination = new_target_room;
                                    ch->ai_data->goal_target_mob_rnum = SHOP_KEEPER(new_shop_rnum);
                                    act("$n percebe que esta loja não compra o que tem e decide procurar outra.", FALSE,
                                        ch, 0, 0, TO_ROOM);
                                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                        continue;
                                    /* Continue with updated goal - don't process other activities this tick */
                                    continue;
                                }
                            }
                            /* No suitable shop found, abandon goal */
                            act("$n parece frustrado por não conseguir vender os seus itens.", FALSE, ch, 0, 0,
                                TO_ROOM);
                            /* Safety check: act() can trigger DG scripts which may cause extraction */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;
                            ch->ai_data->current_goal = GOAL_NONE;
                            ch->ai_data->goal_destination = NOWHERE;
                            ch->ai_data->goal_obj = NULL;
                            ch->ai_data->goal_target_mob_rnum = NOBODY;
                            ch->ai_data->goal_item_vnum = NOTHING;
                            ch->ai_data->goal_timer = 0;
                            continue;
                        }
                        /* After selling, check if there are more items to sell to this same shop */
                        struct obj_data *next_item_to_sell = NULL;
                        int min_score = 10;

                        if (shop_rnum != -1 && shop_index[shop_rnum].type) {
                            /* Look for more junk to sell to this same shop */
                            struct obj_data *obj;
                            for (obj = ch->carrying; obj; obj = obj->next_content) {
                                if (OBJ_FLAGGED(obj, ITEM_NODROP) ||
                                    (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains))
                                    continue;

                                /* Check if this shop buys this type of item */
                                bool shop_buys_this = FALSE;
                                for (int i = 0; SHOP_BUYTYPE(shop_rnum, i) != NOTHING; i++) {
                                    if (SHOP_BUYTYPE(shop_rnum, i) == GET_OBJ_TYPE(obj)) {
                                        shop_buys_this = TRUE;
                                        break;
                                    }
                                }

                                if (shop_buys_this) {
                                    int score = evaluate_item_for_mob(ch, obj);
                                    if (score < min_score) {
                                        min_score = score;
                                        next_item_to_sell = obj;
                                    }
                                }
                            }

                            /* If we found another item to sell to this shop, continue selling */
                            if (next_item_to_sell) {
                                ch->ai_data->goal_obj = next_item_to_sell;
                                continue; /* Continue with current goal, don't clear it */
                            }
                        }
                    }
                } else if (ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_BUY) {
                    /* Chegou à loja para comprar um item da wishlist */
                    struct char_data *keeper = get_mob_in_room_by_rnum(IN_ROOM(ch), ch->ai_data->goal_target_mob_rnum);

                    /* Validate keeper exists and is not marked for extraction */
                    if (!keeper || MOB_FLAGGED(keeper, MOB_NOTDEADYET) || PLR_FLAGGED(keeper, PLR_NOTDEADYET)) {
                        /* Keeper not available, abandon this goal */
                        ch->ai_data->current_goal = GOAL_NONE;
                        ch->ai_data->goal_destination = NOWHERE;
                        ch->ai_data->goal_target_mob_rnum = NOBODY;
                        ch->ai_data->goal_item_vnum = NOTHING;
                        ch->ai_data->goal_timer = 0;
                        continue;
                    }

                    if (ch->ai_data->goal_item_vnum != NOTHING) {
                        /* Tenta comprar o item */
                        char buy_command[MAX_INPUT_LENGTH];
                        act("$n olha os produtos da loja.", FALSE, ch, 0, 0, TO_ROOM);
                        /* Safety check: act() can trigger DG scripts which may cause extraction */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                        sprintf(buy_command, "%d", ch->ai_data->goal_item_vnum);
                        shopping_buy(buy_command, ch, keeper, find_shop_by_keeper(keeper->nr));

                        /* Safety check: shopping operations could indirectly cause extract_char
                         * through scripts, triggers, or special procedures */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;

                        /* Check if keeper was extracted during the transaction */
                        if (MOB_FLAGGED(keeper, MOB_NOTDEADYET) || PLR_FLAGGED(keeper, PLR_NOTDEADYET)) {
                            /* Keeper was extracted, clear goal and continue */
                            ch->ai_data->current_goal = GOAL_NONE;
                            ch->ai_data->goal_destination = NOWHERE;
                            ch->ai_data->goal_target_mob_rnum = NOBODY;
                            ch->ai_data->goal_item_vnum = NOTHING;
                            ch->ai_data->goal_timer = 0;
                            continue;
                        }

                        /* Remove o item da wishlist se a compra foi bem sucedida */
                        remove_item_from_wishlist(ch, ch->ai_data->goal_item_vnum);
                        act("$n parece satisfeito com a sua compra.", FALSE, ch, 0, 0, TO_ROOM);
                        /* Safety check: act() can trigger DG scripts which may cause extraction */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    }
                } else if (ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER) {
                    /* Chegou ao questmaster para postar uma quest */
                    if (ch->ai_data->goal_item_vnum != NOTHING) {
                        /* Encontra o item na wishlist para obter a prioridade correta */
                        struct mob_wishlist_item *wishlist_item =
                            find_item_in_wishlist(ch, ch->ai_data->goal_item_vnum);
                        int reward = wishlist_item ? wishlist_item->priority * 2 : ch->ai_data->goal_item_vnum;
                        obj_vnum item_vnum = ch->ai_data->goal_item_vnum;

                        /* Posta a quest no questmaster */
                        mob_posts_quest(ch, item_vnum, reward);
                        /* Safety check: mob_posts_quest() calls act() internally which can trigger DG scripts */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;

                        /* Only show message if quest was successfully posted (item removed from wishlist) */
                        if (!find_item_in_wishlist(ch, item_vnum)) {
                            act("$n fala com o questmaster e entrega um pergaminho.", FALSE, ch, 0, 0, TO_ROOM);
                            /* Safety check: act() can trigger DG scripts which may cause extraction */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;
                        }
                    }
                    /* Clear goal after posting quest or if no item to post */
                    ch->ai_data->current_goal = GOAL_NONE;
                    ch->ai_data->goal_destination = NOWHERE;
                    ch->ai_data->goal_item_vnum = NOTHING;
                    ch->ai_data->goal_timer = 0;
                } else if (ch->ai_data->current_goal == GOAL_ACCEPT_QUEST) {
                    /* Chegou ao questmaster para aceitar uma quest */
                    struct char_data *questmaster =
                        get_mob_in_room_by_rnum(IN_ROOM(ch), ch->ai_data->goal_target_mob_rnum);
                    if (questmaster && GET_MOB_QUEST(ch) == NOTHING) {
                        /* Find first quest available for mob's level range */
                        qst_vnum available_quest = find_mob_available_quest_by_qmnum(ch, GET_MOB_VNUM(questmaster));
                        if (available_quest != NOTHING) {
                            qst_rnum quest_rnum = real_quest(available_quest);
                            /* mob_can_accept_quest_forced provides additional validation (existing quest, escort quest
                             * restrictions) */
                            if (quest_rnum != NOTHING && mob_can_accept_quest_forced(ch, quest_rnum)) {
                                set_mob_quest(ch, quest_rnum);
                                act("$n fala com $N e aceita uma tarefa.", FALSE, ch, 0, questmaster, TO_ROOM);
                                /* Safety check: act() can trigger DG scripts which may cause extraction */
                                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                    continue;

                                /* Removed manual goal setting - set_mob_quest() now handles this automatically,
                                 * setting GOAL_COMPLETE_QUEST and initializing goal fields based on quest type */
                                continue; /* Process quest completion immediately */
                            }
                        } else {
                            act("$n fala com $N mas parece não haver tarefas disponíveis.", FALSE, ch, 0, questmaster,
                                TO_ROOM);
                            /* Safety check: act() can trigger DG scripts which may cause extraction */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;
                        }
                    }
                    /* Clear goal if quest was not accepted (no questmaster, already has quest, no quests available, or
                     * didn't want to accept) */
                    ch->ai_data->current_goal = GOAL_NONE;
                    ch->ai_data->goal_destination = NOWHERE;
                    ch->ai_data->goal_target_mob_rnum = NOBODY;
                    ch->ai_data->goal_timer = 0;
                } else if (ch->ai_data->current_goal == GOAL_COMPLETE_QUEST) {
                    /* Process quest completion based on quest type */
                    if (GET_MOB_QUEST(ch) != NOTHING) {
                        qst_rnum quest_rnum = real_quest(GET_MOB_QUEST(ch));
                        if (quest_rnum != NOTHING) {
                            if (mob_process_quest_completion(ch, quest_rnum)) {
                                /* Quest completed, goal will be cleared in mob_complete_quest */
                                continue;
                            }
                        } else {
                            /* Invalid quest, clear it */
                            clear_mob_quest(ch);
                        }
                    } else {
                        /* No quest, clear goal */
                        ch->ai_data->current_goal = GOAL_NONE;
                    }
                } else if (ch->ai_data->current_goal == GOAL_COLLECT_KEY) {
                    /* Arrived at key location - try to collect the key */
                    struct obj_data *key_obj = NULL, *next_obj = NULL;
                    obj_vnum target_key = ch->ai_data->goal_item_vnum;
                    bool key_collected = FALSE;

                    /* Look for the key on the ground */
                    for (key_obj = world[IN_ROOM(ch)].contents; key_obj; key_obj = next_obj) {
                        next_obj = key_obj->next_content; /* Save next pointer before obj_from_room */
                        if (GET_OBJ_TYPE(key_obj) == ITEM_KEY && GET_OBJ_VNUM(key_obj) == target_key) {
                            obj_from_room(key_obj);
                            obj_to_char(key_obj, ch);
                            act("$n pega $p do chão.", FALSE, ch, key_obj, 0, TO_ROOM);
                            /* Safety check: act() can trigger DG scripts which may cause extraction */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;
                            key_collected = TRUE;
                            break;
                        }
                    }

                    /* If not found on ground, look in containers */
                    if (!key_collected) {
                        struct obj_data *container, *next_container;
                        for (container = world[IN_ROOM(ch)].contents; container; container = next_container) {
                            next_container = container->next_content;
                            if (GET_OBJ_TYPE(container) == ITEM_CONTAINER && !OBJVAL_FLAGGED(container, CONT_CLOSED)) {
                                for (key_obj = container->contains; key_obj; key_obj = next_obj) {
                                    next_obj = key_obj->next_content; /* Save next pointer before obj_from_obj */
                                    if (GET_OBJ_TYPE(key_obj) == ITEM_KEY && GET_OBJ_VNUM(key_obj) == target_key) {
                                        obj_from_obj(key_obj);
                                        obj_to_char(key_obj, ch);
                                        act("$n pega $p de $P.", FALSE, ch, key_obj, container, TO_ROOM);
                                        /* Safety check: act() can trigger DG scripts which may cause extraction */
                                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                            continue;
                                        key_collected = TRUE;
                                        break;
                                    }
                                }
                                if (key_collected)
                                    break;
                            }
                        }
                    }

                    /* If not found in containers, look for mob carrying it */
                    if (!key_collected && ch->ai_data->goal_target_mob_rnum != NOBODY) {
                        struct char_data *target_mob =
                            get_mob_in_room_by_rnum(IN_ROOM(ch), ch->ai_data->goal_target_mob_rnum);
                        if (target_mob && !FIGHTING(ch)) {
                            /* Attack the mob to get the key */
                            act("$n ataca $N para obter algo que precisa.", FALSE, ch, 0, target_mob, TO_ROOM);
                            /* Safety check: act() can trigger DG scripts which may cause extraction */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;
                            if (MOB_FLAGGED(target_mob, MOB_NOTDEADYET) || PLR_FLAGGED(target_mob, PLR_NOTDEADYET))
                                continue;
                            hit(ch, target_mob, TYPE_UNDEFINED);
                            /* Safety check: hit() can indirectly cause extract_char */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;
                            /* Don't clear goal yet - continue fighting until key is obtained */
                            continue;
                        }
                    }

                    /* If key was collected, restore original goal */
                    if (key_collected) {
                        if (ch->ai_data->original_goal != GOAL_NONE) {
                            ch->ai_data->current_goal = ch->ai_data->original_goal;
                            ch->ai_data->goal_destination = ch->ai_data->original_destination;
                            ch->ai_data->goal_obj = ch->ai_data->original_obj;
                            ch->ai_data->goal_target_mob_rnum = ch->ai_data->original_target_mob;
                            ch->ai_data->goal_item_vnum = ch->ai_data->original_item_vnum;
                            ch->ai_data->goal_timer = 0;

                            /* Clear original goal data */
                            ch->ai_data->original_goal = GOAL_NONE;
                            ch->ai_data->original_destination = NOWHERE;
                            ch->ai_data->original_obj = NULL;
                            ch->ai_data->original_target_mob = NOBODY;
                            ch->ai_data->original_item_vnum = NOTHING;

                            act("$n parece satisfeito por ter encontrado o que procurava.", FALSE, ch, 0, 0, TO_ROOM);
                            /* Safety check: act() can trigger DG scripts which may cause extraction */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                continue;
                            /* Continue with restored goal - don't clear it below */
                            continue;
                        }
                    }
                    /* If key not found, goal will be cleared below */
                }
                /* Limpa o objetivo, pois foi concluído. */
                ch->ai_data->current_goal = GOAL_NONE;
                ch->ai_data->goal_destination = NOWHERE;
                ch->ai_data->goal_obj = NULL;
                ch->ai_data->goal_target_mob_rnum = NOBODY;
                ch->ai_data->goal_item_vnum = NOTHING;
                ch->ai_data->goal_timer = 0;
            } else {
                /* Ainda não chegou. Continua a vaguear em direção ao objetivo. */
                mob_goal_oriented_roam(ch, dest);
                /* Safety check: mob_goal_oriented_roam uses perform_move which can trigger death traps */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    continue;
            }

            continue; /* O turno do mob foi gasto a trabalhar no seu objetivo. */
        }

        if (mob_index[GET_MOB_RNUM(ch)].func == shop_keeper) {
            int shop_nr = find_shop_by_keeper(GET_MOB_RNUM(ch));
            if (shop_nr != -1 && !is_shop_open(shop_nr)) {
                /* --- IA ECONÓMICA: GESTÃO DE STOCK --- */

                /* O lojista verifica o seu inventário à procura de lixo para destruir. */
                struct obj_data *current_obj, *next_obj;
                for (current_obj = ch->carrying; current_obj; current_obj = next_obj) {
                    next_obj = current_obj->next_content;
                    if (OBJ_FLAGGED(current_obj, ITEM_TRASH)) {
                        act("$n joga $p fora.", FALSE, ch, current_obj, 0, TO_ROOM);
                        /* Safety check: act() can trigger DG scripts which may cause extraction */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                        extract_obj(current_obj);
                        /* Safety check: extract_obj could potentially trigger DG Scripts
                         * on the object being extracted, which might indirectly affect ch */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    }
                }
                continue;
            }
        }

        if GROUP (ch) {
            mob_follow_leader(ch);
            /* Safety check: mob_follow_leader calls perform_move which can trigger death traps */
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                continue;
        }

        /* Charmed mobs that are not in a group should follow their master */
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && !GROUP(ch)) {
            /* Safety check: Validate master pointer and room */
            if (IN_ROOM(ch->master) != NOWHERE && IN_ROOM(ch->master) >= 0 && IN_ROOM(ch->master) <= top_of_world) {
                /* Only follow if in different room from master */
                if (IN_ROOM(ch) != IN_ROOM(ch->master)) {
                    int direction = find_first_step(IN_ROOM(ch), IN_ROOM(ch->master));
                    if (direction >= 0 && direction < DIR_COUNT) {
                        /* Try to move toward master - perform_move will handle doors/obstacles */
                        perform_move(ch, direction, 1);
                        /* Safety check: perform_move can trigger death traps */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    }
                }
            }
        }

        /* Try stealth following (for non-grouped following behavior) */
        mob_try_stealth_follow(ch);
        /* Safety check: mob_try_stealth_follow calls perform_move which can trigger death traps */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        mob_assist_allies(ch);
        /* Safety check: mob_assist_allies calls hit() which can cause extract_char */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Try to heal allies in critical condition */
        mob_try_heal_ally(ch);
        /* Safety check: mob_try_heal_ally calls do_bandage which can trigger scripts */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        mob_try_and_loot(ch);

        /* hunt a victim, if applicable */
        hunt_victim(ch);

        // *** ADDED SAFETY CHECK ***
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Wishlist-based goal planning - not for charmed mobs */
        if (ch->ai_data && !AFF_FLAGGED(ch, AFF_CHARM) && rand_number(1, 100) <= 10) { /* 10% chance per tick */
            mob_process_wishlist_goals(ch);
            /* Safety check: mob_process_wishlist_goals can call act() which may trigger DG scripts */
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                continue;
        }

        /* Quest acceptance - try to find and accept quests occasionally (not for charmed mobs)
         * Performance optimization: Use mob's rnum to stagger quest checks across ticks
         * This prevents all mobs from checking on the same tick */
        if (ch->ai_data && !AFF_FLAGGED(ch, AFF_CHARM) && GET_MOB_QUEST(ch) == NOTHING &&
            ch->ai_data->current_goal == GOAL_NONE &&
            ((GET_MOB_RNUM(ch) + pulse) % 20 == 0) && /* Stagger: each mob checks every 20 ticks */
            rand_number(1, 100) <= 10) {              /* 10% chance when it's their turn */
            mob_try_to_accept_quest(ch);
        }

        /* Mob quest processing - not for charmed mobs
         * Only check timer for mobs that actually have quests */
        if (ch->ai_data && !AFF_FLAGGED(ch, AFF_CHARM) && GET_MOB_QUEST(ch) != NOTHING) {
            /* Decrement quest timer deterministically (every tick) */
            if (ch->ai_data->quest_timer > 0) {
                ch->ai_data->quest_timer--;
                if (ch->ai_data->quest_timer <= 0) {
                    /* Quest timeout */
                    act("$n parece desapontado por não completar uma tarefa a tempo.", TRUE, ch, 0, 0, TO_ROOM);
                    fail_mob_quest(ch, "timeout");
                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                }
            }
        }

        /* Mob combat quest posting - chance to post bounty/revenge quests (not for charmed mobs) */
        if (ch->ai_data && !AFF_FLAGGED(ch, AFF_CHARM)) {
            /* Dynamic probability based on quest tendency and bravery for player kill quests */
            int combat_quest_chance = 2; /* Base 2% chance per tick */

            /* Increase chance for player kill quests if mob is being hunted and capable */
            if (GET_GENBRAVE(ch) > 50 && GET_GENQUEST(ch) >= 5) {
                struct char_data *attacker = HUNTING(ch);
                if (attacker && !IS_NPC(attacker) && GET_GOLD(ch) > 200) {
                    /* Increase probability significantly for player kill quests when mob is actively hunted */
                    combat_quest_chance = 5 + (GET_GENQUEST(ch) / 20); /* 5-10% chance based on quest tendency */
                }
            }

            if (rand_number(1, 100) <= combat_quest_chance) {
                /* Check if mob should post a player kill quest (revenge for being attacked) */
                if (GET_GENBRAVE(ch) > 50 && GET_GENQUEST(ch) >= 5) {
                    /* Check if mob was recently attacked by a player (has hostile memory) */
                    struct char_data *attacker = HUNTING(ch);
                    if (attacker && !IS_NPC(attacker) && GET_GOLD(ch) > 200) {
                        /* Post a player kill quest for revenge */
                        int reward = MIN(GET_GOLD(ch) / 3, 500 + rand_number(0, 300));
                        mob_posts_combat_quest(ch, AQ_PLAYER_KILL, NOTHING, reward);
                        /* Safety check: mob_posts_combat_quest calls act() which may trigger DG scripts */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                        HUNTING(ch) = NULL; /* Clear hunting after posting quest */
                    }
                }

                /* Check if mob should post a bounty quest against hostile mobs in area */
                if (GET_GENQUEST(ch) >= 10 && GET_GOLD(ch) > 300) {
                    /* Safety check: Validate ch's room before accessing world array */
                    if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                        struct char_data *target, *next_target;
                        /* Look for aggressive mobs in the same zone */
                        for (target = character_list; target; target = next_target) {
                            next_target = target->next;

                            /* Safety check: Skip characters marked for extraction */
                            if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                                continue;

                            /* Safety check: Validate room before accessing world array */
                            if (!IS_NPC(target) || target == ch || IN_ROOM(target) == NOWHERE || IN_ROOM(target) < 0 ||
                                IN_ROOM(target) > top_of_world)
                                continue;

                            /* Skip mobs that should be excluded from quests (summoned, skill-spawned) */
                            if (is_mob_excluded_from_quests(target))
                                continue;

                            if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone &&
                                MOB_FLAGGED(target, MOB_AGGRESSIVE) && GET_ALIGNMENT(target) < -200 &&
                                GET_LEVEL(target) >= GET_LEVEL(ch) - 5) {

                                /* Post bounty quest against this aggressive mob */
                                int reward = MIN(GET_GOLD(ch) / 4, 400 + GET_LEVEL(target) * 10);
                                mob_posts_combat_quest(ch, AQ_MOB_KILL_BOUNTY, GET_MOB_VNUM(target), reward);
                                /* Safety check: mob_posts_combat_quest calls act() which may trigger DG scripts */
                                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                    break; /* Exit the loop, then continue to next mob in main loop */
                                break;     /* Only post one bounty quest per tick */
                            }
                        }
                        /* Safety check after the loop in case ch was extracted */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    }
                }
            }
        }

        /* Additional quest posting - exploration, protection, and general kill quests (not for charmed mobs) */
        if (ch->ai_data && !AFF_FLAGGED(ch, AFF_CHARM) &&
            rand_number(1, 100) <= 3) { /* 3% chance per tick for other quest types */
            /* Safety check: Validate ch's room before accessing world array */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                continue;

            struct char_data *target;
            int reward;

            /* Check genetics and decide what type of quest to post */
            if (GET_GENADVENTURER(ch) >= 5 && rand_number(1, 100) <= 30) {
                /* Post exploration quests */
                if (rand_number(1, 100) <= 40) {
                    /* AQ_OBJ_FIND quest - find a random object in the zone */
                    zone_rnum mob_zone = world[IN_ROOM(ch)].zone;
                    obj_vnum obj_target = NOTHING;

                    /* Find a suitable object in the zone to search for */
                    for (int i = zone_table[mob_zone].bot; i <= zone_table[mob_zone].top; i++) {
                        if (real_object(i) != NOTHING) {
                            obj_target = i;
                            break;
                        }
                    }

                    if (obj_target != NOTHING && GET_GOLD(ch) > 100) {
                        reward = MIN(GET_GOLD(ch) / 6, 200 + GET_LEVEL(ch) * 5);
                        mob_posts_exploration_quest(ch, AQ_OBJ_FIND, obj_target, reward);
                        /* Safety check: mob_posts_exploration_quest calls act() which may trigger DG scripts */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    }
                } else if (rand_number(1, 100) <= 50) {
                    /* AQ_ROOM_FIND quest - explore a room in the zone */
                    zone_rnum mob_zone = world[IN_ROOM(ch)].zone;
                    room_rnum room_target_rnum = NOWHERE;
                    int attempts = 0;
                    const int max_attempts = 20;

                    /* Find a suitable room: not GODROOM, not HOUSE, but DEATH is OK */
                    while (attempts < max_attempts && room_target_rnum == NOWHERE) {
                        room_vnum candidate_vnum = zone_table[mob_zone].bot +
                                                   rand_number(0, zone_table[mob_zone].top - zone_table[mob_zone].bot);
                        room_rnum candidate_rnum = real_room(candidate_vnum);

                        if (candidate_rnum != NOWHERE && !ROOM_FLAGGED(candidate_rnum, ROOM_GODROOM) &&
                            !ROOM_FLAGGED(candidate_rnum, ROOM_HOUSE)) {
                            room_target_rnum = candidate_rnum;
                        }
                        attempts++;
                    }

                    if (room_target_rnum != NOWHERE && GET_GOLD(ch) > 75) {
                        reward = MIN(GET_GOLD(ch) / 8, 150 + GET_LEVEL(ch) * 3);
                        mob_posts_exploration_quest(ch, AQ_ROOM_FIND, world[room_target_rnum].number, reward);
                        /* Safety check: mob_posts_exploration_quest calls act() which may trigger DG scripts */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    }
                } else {
                    /* AQ_MOB_FIND quest - find a friendly mob */
                    struct char_data *next_target;
                    for (target = character_list; target; target = next_target) {
                        next_target = target->next;

                        /* Safety check: Skip characters marked for extraction */
                        if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                            continue;

                        /* Safety check: Validate room before accessing world array */
                        if (!IS_NPC(target) || target == ch || IN_ROOM(target) == NOWHERE || IN_ROOM(target) < 0 ||
                            IN_ROOM(target) > top_of_world)
                            continue;

                        /* Safety check: Validate ch's room before zone comparison */
                        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                            break;

                        if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone && GET_ALIGNMENT(target) > 0 &&
                            !MOB_FLAGGED(target, MOB_AGGRESSIVE)) {

                            if (GET_GOLD(ch) > 80) {
                                reward = MIN(GET_GOLD(ch) / 7, 120 + GET_LEVEL(target) * 4);
                                mob_posts_exploration_quest(ch, AQ_MOB_FIND, GET_MOB_VNUM(target), reward);
                                /* Safety check: mob_posts_exploration_quest calls act() which may trigger DG scripts */
                                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                    break; /* Exit loop, then continue to next mob in main loop */
                            }
                            break; /* Only post one find quest per tick */
                        }
                    }
                    /* Safety check after the loop in case ch was extracted */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                }
            } else if (GET_GENBRAVE(ch) > 60 && rand_number(1, 100) <= 25) {
                /* Post protection quests */
                if (rand_number(1, 100) <= 60) {
                    /* AQ_MOB_SAVE quest - protect a weak mob */
                    struct char_data *next_target;
                    for (target = character_list; target; target = next_target) {
                        next_target = target->next;

                        /* Safety check: Skip characters marked for extraction */
                        if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                            continue;

                        /* Safety check: Validate room before accessing world array */
                        if (!IS_NPC(target) || target == ch || IN_ROOM(target) == NOWHERE || IN_ROOM(target) < 0 ||
                            IN_ROOM(target) > top_of_world)
                            continue;

                        /* Safety check: Validate ch's room before zone comparison */
                        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                            break;

                        if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone &&
                            GET_LEVEL(target) < GET_LEVEL(ch) && GET_ALIGNMENT(target) > 200) {

                            if (GET_GOLD(ch) > 120) {
                                reward = MIN(GET_GOLD(ch) / 5, 250 + GET_LEVEL(target) * 6);
                                mob_posts_protection_quest(ch, AQ_MOB_SAVE, GET_MOB_VNUM(target), reward);
                                /* Safety check: mob_posts_protection_quest calls act() which may trigger DG scripts */
                                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                    break; /* Exit loop, then continue to next mob in main loop */
                            }
                            break; /* Only post one save quest per tick */
                        }
                    }
                    /* Safety check after the loop in case ch was extracted */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                } else {
                    /* AQ_ROOM_CLEAR quest - clear a dangerous room */
                    zone_rnum mob_zone = world[IN_ROOM(ch)].zone;
                    room_rnum dangerous_room = NOWHERE;

                    /* Find a room with aggressive mobs that is not GODROOM, HOUSE, or PEACEFUL */
                    for (int r = zone_table[mob_zone].bot; r <= zone_table[mob_zone].top; r++) {
                        room_rnum real_r = real_room(r);
                        if (real_r != NOWHERE && !ROOM_FLAGGED(real_r, ROOM_GODROOM) &&
                            !ROOM_FLAGGED(real_r, ROOM_HOUSE) && !ROOM_FLAGGED(real_r, ROOM_PEACEFUL)) {
                            for (target = world[real_r].people; target; target = target->next_in_room) {
                                if (IS_NPC(target) && MOB_FLAGGED(target, MOB_AGGRESSIVE)) {
                                    dangerous_room = r;
                                    break;
                                }
                            }
                            if (dangerous_room != NOWHERE)
                                break;
                        }
                    }

                    if (dangerous_room != NOWHERE && GET_GOLD(ch) > 150) {
                        reward = MIN(GET_GOLD(ch) / 4, 300 + GET_LEVEL(ch) * 8);
                        mob_posts_protection_quest(ch, AQ_ROOM_CLEAR, dangerous_room, reward);
                        /* Safety check: mob_posts_protection_quest calls act() which may trigger DG scripts */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                    }
                }
            } else if (GET_GENQUEST(ch) >= 5 && rand_number(1, 100) <= 20) {
                /* Post general kill quests */
                struct char_data *next_target;
                for (target = character_list; target; target = next_target) {
                    next_target = target->next;

                    /* Safety check: Skip characters marked for extraction */
                    if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                        continue;

                    /* Safety check: Validate room before accessing world array */
                    if (!IS_NPC(target) || target == ch || IN_ROOM(target) == NOWHERE || IN_ROOM(target) < 0 ||
                        IN_ROOM(target) > top_of_world)
                        continue;

                    /* Safety check: Validate ch's room before zone comparison */
                    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                        break;

                    if (world[IN_ROOM(target)].zone == world[IN_ROOM(ch)].zone && GET_ALIGNMENT(target) < -100 &&
                        GET_LEVEL(target) >= GET_LEVEL(ch) - 10) {

                        if (GET_GOLD(ch) > 100) {
                            reward = MIN(GET_GOLD(ch) / 5, 200 + GET_LEVEL(target) * 8);
                            mob_posts_general_kill_quest(ch, GET_MOB_VNUM(target), reward);
                            /* Safety check: mob_posts_general_kill_quest calls act() which may trigger DG scripts */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                break; /* Exit loop, then continue to next mob in main loop */
                        }
                        break; /* Only post one kill quest per tick */
                    }
                }
                /* Safety check after the loop in case ch was extracted */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    continue;
            }
        }

        mob_handle_grouping(ch);

        /* Aggressive Mobs - skip if helper, blind, or charmed */
        if (!MOB_FLAGGED(ch, MOB_HELPER) && !AFF_FLAGGED(ch, AFF_BLIND) && !AFF_FLAGGED(ch, AFF_CHARM)) {
            found = FALSE;
            /* Re-verify room validity before accessing room data */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                continue;

            for (vict = world[IN_ROOM(ch)].people; vict && !found;) {
                /* Check vict validity before dereferencing */
                if (!vict) {
                    break;
                }

                struct char_data *next_vict = vict->next_in_room; /* Save next pointer before any actions */

                //	if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
                if (!CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE)) {
                    vict = next_vict;
                    continue;
                }

                /* Safety check: Validate vict is still valid before checking AWAKE(vict)
                 * This is critical when vict may be sleeping/meditating and position is changing.
                 * Without this check, AWAKE(vict) can cause SIGSEGV if vict was extracted or
                 * became invalid between iterations or during CAN_SEE/PRF_FLAGGED checks.
                 * Note: vict should not be NULL here since we're iterating from room people list,
                 * but we check NOTDEADYET flags to catch pending extractions. */
                if (!vict || MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET)) {
                    vict = next_vict;
                    continue;
                }

                if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict)) {
                    vict = next_vict;
                    continue;
                }

                if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) || (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
                    (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
                    (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict))) {

                    /* Can a master successfully control the charmed monster? */
                    if (aggressive_mob_on_a_leash(ch, ch->master, vict)) {
                        vict = next_vict;
                        continue;
                    }

                    if (vict == ch) {
                        vict = next_vict;
                        continue;
                    }

                    // if (IS_NPC(vict))
                    // continue;

                    /* Big Five Phase 2B: Apply Conscientiousness to Aggressive Targeting
                     * This is a DELIBERATIVE decision point: mob chooses to attack or not.
                     * High C reduces impulsive aggression, low C increases it.
                     * Charisma provides base resistance, C modulates the impulse. */

                    /* Base impulse probability: 1.0 - (charisma_resistance / 20.0)
                     * CHA 0  = 100% impulse, CHA 20 = 0% impulse
                     * This represents victim's ability to diffuse aggression */
                    float base_impulse = 1.0f - (MIN(GET_CHA(vict), 20) / 20.0f);

                    /* Apply executive control (C modulation of impulse) */
                    float modulated_impulse = apply_executive_control(ch, base_impulse, FALSE, NULL);

                    /* Convert back to threshold for rand_number check
                     * Original: if (rand_number(0, 20) <= GET_CHA(vict))
                     * New: if (rand_number(0, 100) > modulated_impulse * 100) */
                    int impulse_threshold = (int)(modulated_impulse * 100.0f);

                    if (rand_number(0, 100) > impulse_threshold) {
                        /* Resisted impulse - intimidate instead of attack */
                        act("$n olha para $N com indiferença.", FALSE, ch, 0, vict, TO_NOTVICT);
                        /* Safety check: act() can trigger DG scripts which may extract ch or vict */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                        if (MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET)) {
                            vict = next_vict;
                            continue;
                        }
                        act("$N olha para você com indiferença.", FALSE, vict, 0, ch, TO_CHAR);
                        /* Safety check: second act() may also trigger extraction */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                        if (MOB_FLAGGED(vict, MOB_NOTDEADYET) || PLR_FLAGGED(vict, PLR_NOTDEADYET)) {
                            vict = next_vict;
                            continue;
                        }
                    } else {
                        /* Impulse succeeded - attack */
                        hit(ch, vict, TYPE_UNDEFINED);
                        /* Safety check: hit() can indirectly cause extract_char */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            continue;
                        found = TRUE;
                    }
                }

                vict = next_vict; /* Move to next victim safely */
            }
        }

        // *** ADDED SAFETY CHECK ***
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        mob_try_and_upgrade(ch);

        mob_share_gear_with_group(ch);

        /* Bank usage for mobs with high trade genetics */
        mob_use_bank(ch);
        /* Safety check: mob_use_bank() calls act() which can trigger DG scripts */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        if (handle_duty_routine(ch)) {
            /* Safety check: handle_duty_routine calls perform_move which can trigger death traps */
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                continue;
            continue;
        }

        /* Prioridade de Vaguear (Roam) */
        if (!mob_try_to_sell_junk(ch)) {
            mob_goal_oriented_roam(ch, NOWHERE);
            /* Safety check: mob_goal_oriented_roam uses perform_move which can trigger death traps */
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                continue;
        } else {
            /* Safety check: mob_try_to_sell_junk can call mob_goal_oriented_roam internally */
            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                continue;
        }

        /* Mob Memory */
        if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
            found = FALSE;
            /* Re-verify room validity before accessing room data */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                continue;

            for (vict = world[IN_ROOM(ch)].people; vict && !found;) {
                /* Check vict validity before dereferencing */
                if (!vict) {
                    break;
                }

                struct char_data *next_vict = vict->next_in_room; /* Save next pointer before any actions */

                if (!CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE)) {
                    vict = next_vict;
                    continue;
                }

                for (names = MEMORY(ch); names && !found; names = names->next) {
                    if (!names)
                        break; /* Safety check - names became NULL */

                    if (names->id != GET_IDNUM(vict))
                        continue;

                    /* Can a master successfully control the charmed monster? */
                    if (aggressive_mob_on_a_leash(ch, ch->master, vict))
                        continue;

                    found = TRUE;
                    act("''Ei!  Você é o demônio que me atacou!!!', exclama $n.", FALSE, ch, 0, 0, TO_ROOM);
                    hit(ch, vict, TYPE_UNDEFINED);
                    /* Safety check: hit() can indirectly cause extract_char */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                }

                vict = next_vict; /* Move to next victim safely */
            }
        }

        // *** ADDED SAFETY CHECK ***
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Charmed Mob Rebellion: In order to rebel, there need to be more charmed
         * monsters than the person can feasibly control at a time.  Then the
         * mobiles have a chance based on the charisma of their leader.
         * 1-4 = 0, 5-7 = 1, 8-10 = 2, 11-13 = 3, 14-16 = 4, 17-19 = 5, etc. */
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
            num_followers_charmed(ch->master) > (GET_CHA(ch->master) - 2) / 3) {
            if (!aggressive_mob_on_a_leash(ch, ch->master, ch->master)) {
                if (CAN_SEE(ch, ch->master) && !PRF_FLAGGED(ch->master, PRF_NOHASSLE))
                    hit(ch, ch->master, TYPE_UNDEFINED);
                /* Safety check: hit() can indirectly cause extract_char */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    continue;
                stop_follower(ch);
                /* Safety check: stop_follower() calls act() which can trigger DG scripts */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    continue;
            }
        }

        // *** ADDED SAFETY CHECK ***
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            continue;

        /* Resource gathering goal assignment for idle mobs */
        if (ch->ai_data && ch->ai_data->current_goal == GOAL_NONE && rand_number(1, 1000) <= 10) {
            /* 1% chance per tick for mob to start resource gathering if they have no other goals */

            /* Check genetics to determine preferred resource activity */
            int activity_choice = rand_number(1, 100);

            if (ch->ai_data->genetics.loot_tendency > 60 && activity_choice <= 30) {
                /* High loot tendency mobs prefer mining for valuable materials */
                if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && GET_SKILL(ch, SKILL_MINE) > 0) {
                    ch->ai_data->current_goal = GOAL_MINE;
                    ch->ai_data->goal_timer = 0;
                    act("$n olha ao redor procurando minerais.", FALSE, ch, 0, 0, TO_ROOM);
                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                }
            } else if (SECT(IN_ROOM(ch)) == SECT_WATER_SWIM || SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) {
                /* Near water, try fishing */
                if (GET_SKILL(ch, SKILL_FISHING) > 0 && activity_choice <= 40) {
                    ch->ai_data->current_goal = GOAL_FISH;
                    ch->ai_data->goal_timer = 0;
                    act("$n olha para a água pensativamente.", FALSE, ch, 0, 0, TO_ROOM);
                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                }
            } else if (SECT(IN_ROOM(ch)) != SECT_CITY && SECT(IN_ROOM(ch)) != SECT_INSIDE) {
                /* In wilderness, try foraging */
                if (GET_SKILL(ch, SKILL_FORAGE) > 0 && activity_choice <= 50) {
                    ch->ai_data->current_goal = GOAL_FORAGE;
                    ch->ai_data->goal_timer = 0;
                    act("$n examina a vegetação local.", FALSE, ch, 0, 0, TO_ROOM);
                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                }
            } else if (ch->ai_data->genetics.loot_tendency > 40) {
                /* In social areas, try eavesdropping for information */
                struct char_data *temp_char;
                bool has_targets = FALSE;
                /* Verify room validity before accessing people list */
                if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
                    for (temp_char = world[IN_ROOM(ch)].people; temp_char; temp_char = temp_char->next_in_room) {
                        if (temp_char == ch)
                            continue;
                        if (GET_POS(temp_char) >= POS_RESTING) {
                            has_targets = TRUE;
                            break;
                        }
                    }
                }

                if (has_targets && GET_SKILL(ch, SKILL_EAVESDROP) > 0 && activity_choice <= 25) {
                    ch->ai_data->current_goal = GOAL_EAVESDROP;
                    ch->ai_data->goal_timer = 0;
                    act("$n discretamente presta atenção ao que acontece ao redor.", FALSE, ch, 0, 0, TO_ROOM);
                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        continue;
                }
            }
        }

        /* Evil mobs (alignment -350) try to poison drink containers */
        if (GET_ALIGNMENT(ch) <= -350 && rand_number(1, 100) <= 10) {
            struct char_data *victim;
            struct obj_data *container;

            /* Verify room validity before accessing people list */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                continue;

            /* Look for potential victims in the room */
            for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room) {
                if (victim == ch || IS_NPC(victim))
                    continue;

                /* Look for drink containers in victim's inventory */
                for (container = victim->carrying; container; container = container->next_content) {
                    if (GET_OBJ_TYPE(container) == ITEM_DRINKCON && GET_OBJ_VAL(container, 1) > 0 && /* Has liquid */
                        GET_OBJ_VAL(container, 3) == 0) { /* Not already poisoned */

                        /* 50% chance to successfully poison if evil enough */
                        if (rand_number(1, 100) <= 50) {
                            GET_OBJ_VAL(container, 3) = 1; /* Poison it */

                            /* Subtle message - victim might not notice */
                            if (rand_number(1, 100) <= 50) {
                                act("$n parece sussurrar algo enquanto se aproxima de você.", FALSE, ch, 0, victim,
                                    TO_VICT);
                                /* Safety check: act() can trigger DG scripts which may cause extraction */
                                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                    return;
                                if (MOB_FLAGGED(victim, MOB_NOTDEADYET) || PLR_FLAGGED(victim, PLR_NOTDEADYET))
                                    return;
                            }

                            /* Observer message */
                            act("$n se aproxima discretamente de $N por um momento.", FALSE, ch, 0, victim, TO_NOTVICT);
                            /* Safety check: act() can trigger DG scripts which may cause extraction */
                            if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                return;
                            if (MOB_FLAGGED(victim, MOB_NOTDEADYET) || PLR_FLAGGED(victim, PLR_NOTDEADYET))
                                return;
                            return; /* Only poison one container per round */
                        }
                    }
                }
            }
        }

        /* Note: Mob emotion and social behavior has been moved to mob_emotion_activity()
         * which runs at PULSE_MOB_EMOTION (every 4 seconds) for better responsiveness. */

        /* Add new mobile actions here */

    } /* end for() */
}

/* Mob Memory Routines */
/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
    memory_rec *tmp;
    bool present = FALSE;

    if (IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
        return;

    for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
        if (tmp->id == GET_IDNUM(victim))
            present = TRUE;

    if (!present) {
        CREATE(tmp, memory_rec, 1);
        tmp->next = MEMORY(ch);
        tmp->id = GET_IDNUM(victim);
        MEMORY(ch) = tmp;
    }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
    memory_rec *curr, *prev = NULL;

    if (!(curr = MEMORY(ch)))
        return;

    while (curr && curr->id != GET_IDNUM(victim)) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
        return; /* person wasn't there at all. */

    if (curr == MEMORY(ch))
        MEMORY(ch) = curr->next;
    else
        prev->next = curr->next;

    free(curr);
}

/* erase ch's memory */
void clearMemory(struct char_data *ch)
{
    memory_rec *curr, *next;

    curr = MEMORY(ch);

    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    MEMORY(ch) = NULL;
}

/* An aggressive mobile wants to attack something.  If they're under the
 * influence of mind altering PC, then see if their master can talk them out
 * of it, eye them down, or otherwise intimidate the slave. */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack)
{
    static int snarl_cmd;
    int dieroll;

    if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
        return (FALSE);

    if (!snarl_cmd)
        snarl_cmd = find_command("snarl");

    /* Sit. Down boy! HEEEEeeeel! */
    dieroll = rand_number(1, 20);
    if (dieroll != 1 && (dieroll == 20 || dieroll > 10 - GET_CHA(master) + GET_INT(slave))) {
        if (snarl_cmd > 0 && attack && !rand_number(0, 3)) {
            char victbuf[MAX_NAME_LENGTH + 1];

            strncpy(victbuf, GET_NAME(attack), sizeof(victbuf)); /* strncpy: OK */
            victbuf[sizeof(victbuf) - 1] = '\0';

            do_action(slave, victbuf, snarl_cmd, 0);
        }

        /* Success! But for how long? Hehe. */
        return (TRUE);
    }

    /* So sorry, now you're a player killer... Tsk tsk. */
    return (FALSE);
}

/**
 * Verifica se um mob possui qualquer item do tipo munição.
 * @param ch O mob a ser verificado.
 * @return TRUE se tiver munição, FALSE caso contrário.
 */
bool mob_has_ammo(struct char_data *ch)
{
    struct obj_data *obj;

    /* Primeiro, verifica o item mais óbvio: a aljava (quiver) equipada. */
    if (GET_EQ(ch, WEAR_QUIVER) != NULL) {
        return TRUE;
    }

    /* Se não, percorre o inventário. */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_AMMO) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Verifica se dois mobs são compatíveis para se agruparem, baseado
 * no nível e no alinhamento.
 */
bool are_groupable(struct char_data *ch, struct char_data *target)
{
    /* Regras básicas de compatibilidade (não agrupa com jogadores, etc.) */
    if (!target || ch == target || !IS_NPC(target) || target->master != NULL)
        return FALSE;

    /* REGRA DE ALINHAMENTO */
    bool align_ok = FALSE;
    if (IS_NEUTRAL(ch) || IS_NEUTRAL(target))
        align_ok = TRUE; /* Neutros podem com todos. */
    else if ((IS_GOOD(ch) && IS_GOOD(target)) || (IS_EVIL(ch) && IS_EVIL(target)))
        align_ok = TRUE; /* Bem com Bem, Mal com Mal. */

    if (!align_ok)
        return FALSE;

    /* REGRA DE NÍVEL (simples, para verificações 1 a 1) */
    if (abs(GET_LEVEL(ch) - GET_LEVEL(target)) > 15)
        return FALSE;

    return TRUE;
}

/**
 * Verifica se um novo membro ('prospect') é compatível em nível com um
 * grupo já existente, respeitando a regra de 15 níveis de diferença
 * entre o novo mínimo e o novo máximo do grupo potencial.
 */
bool is_level_compatible_with_group(struct char_data *prospect, struct group_data *group)
{
    if (!prospect || !group || !group->members || group->members->iSize == 0)
        return FALSE;

    struct char_data *member;
    struct iterator_data iterator;

    int min_level = GET_LEVEL(prospect);
    int max_level = GET_LEVEL(prospect);

    /* Itera pelos membros existentes para encontrar o min/max atual do grupo */
    member = (struct char_data *)merge_iterator(&iterator, group->members);
    while (member) {
        if (GET_LEVEL(member) < min_level)
            min_level = GET_LEVEL(member);
        if (GET_LEVEL(member) > max_level)
            max_level = GET_LEVEL(member);
        member = (struct char_data *)next_in_list(&iterator);
    }

    /* Retorna TRUE se a nova diferença total do grupo for 15 ou menos. */
    return ((max_level - min_level) <= 15);
}

/**
 * Avalia todos os mobs solitários numa sala para encontrar o melhor candidato
 * a líder para um novo grupo, com base na regra do "líder mediano".
 * @param ch O mob que está a iniciar a verificação.
 * @return Um ponteiro para o melhor candidato a líder, ou NULL se nenhum grupo for viável.
 */
struct char_data *find_best_leader_for_new_group(struct char_data *ch)
{
    struct char_data *vict, *leader_candidate;
    int min_level = -1, max_level = -1;
    int count = 0;
    struct char_data *potential_members[51]; /* Buffer para potenciais membros */

    /* Safety check: Validate room before accessing people list */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return NULL;

    /* 1. Reúne todos os candidatos (mobs solitários e compatíveis) na sala. */
    for (vict = world[IN_ROOM(ch)].people; vict && count < 50; vict = vict->next_in_room) {
        if (!IS_NPC(vict) || vict->master != NULL || GROUP(vict))
            continue;

        /* Usa a nossa função 'are_groupable' para verificar alinhamento e nível 1-a-1 */
        if (!are_groupable(ch, vict))
            continue;

        potential_members[count++] = vict;
        if (min_level == -1 || GET_LEVEL(vict) < min_level)
            min_level = GET_LEVEL(vict);
        if (max_level == -1 || GET_LEVEL(vict) > max_level)
            max_level = GET_LEVEL(vict);
    }
    potential_members[count] = NULL;

    if (count <= 1)
        return ch; /* Se estiver sozinho, ele pode liderar. */

    /* 2. Se a diferença de nível já for válida, o de nível mais alto lidera. */
    if ((max_level - min_level) <= 15) {
        struct char_data *best_leader = NULL;
        for (int i = 0; i < count; i++) {
            if (best_leader == NULL || GET_LEVEL(potential_members[i]) > GET_LEVEL(best_leader)) {
                best_leader = potential_members[i];
            }
        }
        return best_leader;
    }

    /* 3. Se a diferença for grande, procura o "líder mediano" de nível mais alto. */
    struct char_data *best_median = NULL;
    for (int i = 0; i < count; i++) {
        leader_candidate = potential_members[i];
        if ((max_level - GET_LEVEL(leader_candidate)) <= 15 && (GET_LEVEL(leader_candidate) - min_level) <= 15) {
            if (best_median == NULL || GET_LEVEL(leader_candidate) > GET_LEVEL(best_median)) {
                best_median = leader_candidate;
            }
        }
    }

    return best_median; /* Pode retornar NULL se nenhum grupo for viável */
}

/**
 * A IA principal para um mob tentar formar ou juntar-se a um grupo.
 * Retorna TRUE se uma ação de grupo foi tentada/realizada.
 */
bool mob_handle_grouping(struct char_data *ch)
{
    if (MOB_FLAGGED(ch, MOB_SENTINEL))
        return FALSE;

    if (!ch->ai_data)
        return FALSE;

    /* Don't form groups in death traps or near dangerous areas to reduce cleanup overhead */
    if (IN_ROOM(ch) != NOWHERE && ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH))
        return FALSE;

    /* Check adjacent rooms for death traps - avoid grouping near danger */
    int i;
    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (EXIT(ch, i) && VALID_ROOM_RNUM(EXIT(ch, i)->to_room) && ROOM_FLAGGED(EXIT(ch, i)->to_room, ROOM_DEATH)) {
            /* Only skip grouping 50% of the time near death traps to avoid completely killing group formation */
            if (rand_number(1, 100) <= 50)
                return FALSE;
            break;
        }
    }

    /* Verifica a chance de tentar agrupar-se. */
    const int CURIOSIDADE_MINIMA_GRUPO = 5;
    if (rand_number(1, 100) > MAX(GET_GENGROUP(ch), CURIOSIDADE_MINIMA_GRUPO))
        return FALSE;

    struct char_data *vict, *best_target_leader = NULL;
    bool best_is_local = FALSE;
    int max_group_size = 6;

    /* CENÁRIO 1: O mob está num grupo. */
    if (GROUP(ch)) {
        /* Se ele é um líder de um grupo muito pequeno (só ele), ele pode tentar uma fusão. */
        if (GROUP_LEADER(GROUP(ch)) == ch && GROUP(ch)->members->iSize <= 1) {
            struct char_data *vict, *best_target_leader = NULL;
            /* Safety check: Validate room before accessing people list */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return FALSE;

            /* Procura por outros grupos maiores na sala. */
            for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
                if (GROUP(vict) && GROUP_LEADER(GROUP(vict)) == vict && vict != ch) {
                    if (is_level_compatible_with_group(ch, GROUP(vict)) && are_groupable(ch, vict)) {
                        /* Encontrou um grupo maior e compatível. É uma boa opção. */
                        if (best_target_leader == NULL ||
                            GROUP(vict)->members->iSize > GROUP(best_target_leader)->members->iSize) {
                            best_target_leader = vict;
                        }
                    }
                }
            }
            if (best_target_leader) {
                /* Decisão tática de abandonar a própria liderança para se juntar a um grupo mais forte. */
                act("$n avalia o grupo de $N e decide que é mais forte juntar-se a eles.", TRUE, ch, 0,
                    best_target_leader, TO_ROOM);
                leave_group(ch); /* Abandona o seu próprio grupo solitário. */
                join_group(ch, GROUP(best_target_leader));
                return TRUE;
            }
        }
    }
    /* CENÁRIO 2: O mob está sozinho. */
    else {

        /* Safety check: Validate room before accessing people list */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
            return FALSE;

        /* 1. Procura pelo MELHOR grupo existente para se juntar. */
        for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
            if (GROUP(vict) && GROUP_LEADER(GROUP(vict)) == vict && IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN) &&
                GROUP(vict)->members->iSize < max_group_size && is_level_compatible_with_group(ch, GROUP(vict)) &&
                are_groupable(ch, vict)) {

                /* Safety check: Validate vict's room before zone comparison */
                if (IN_ROOM(vict) == NOWHERE || IN_ROOM(vict) < 0 || IN_ROOM(vict) > top_of_world)
                    continue;

                bool is_local = (world[IN_ROOM(ch)].zone == world[IN_ROOM(vict)].zone);

                if (best_target_leader == NULL) {
                    best_target_leader = vict;
                    best_is_local = is_local;
                } else if (is_local && !best_is_local) {
                    best_target_leader = vict;
                    best_is_local = TRUE;
                } else if (is_local == best_is_local && GET_LEVEL(vict) > GET_LEVEL(best_target_leader)) {
                    best_target_leader = vict;
                }
            }
        }

        if (best_target_leader) {
            /* Lógica de Aceitação do Líder */
            int chance_aceitar =
                100 - (GROUP(best_target_leader)->members->iSize * 15) + GET_GENGROUP(best_target_leader);

            /* Emotion modifiers for group acceptance */
            if (CONFIG_MOB_CONTEXTUAL_SOCIALS && best_target_leader->ai_data && !IS_NPC(ch)) {
                /* High friendship makes mobs more likely to accept into group */
                if (best_target_leader->ai_data->emotion_friendship >= CONFIG_EMOTION_GROUP_FRIENDSHIP_HIGH_THRESHOLD) {
                    chance_aceitar += 20; /* +20% chance to accept */
                }

                /* High envy refuses to group with better-equipped players */
                if (best_target_leader->ai_data->emotion_envy >= CONFIG_EMOTION_GROUP_ENVY_HIGH_THRESHOLD) {
                    /* Check if the player is better equipped (simple heuristic: higher level or more gold) */
                    if (GET_LEVEL(ch) > GET_LEVEL(best_target_leader) ||
                        GET_GOLD(ch) > GET_GOLD(best_target_leader) * 2) {
                        act("$n olha invejosamente para $N e recusa-se a formar um grupo.", TRUE, best_target_leader, 0,
                            ch, TO_ROOM);
                        return FALSE;
                    }
                }
            }

            if (CONFIG_MOB_CONTEXTUAL_SOCIALS && ch->ai_data) {
                /* High friendship makes mobs more likely to join groups */
                if (ch->ai_data->emotion_friendship >= CONFIG_EMOTION_GROUP_FRIENDSHIP_HIGH_THRESHOLD) {
                    chance_aceitar += 15; /* +15% chance to join */
                }

                /* High envy refuses to group with better-equipped players */
                if (ch->ai_data->emotion_envy >= CONFIG_EMOTION_GROUP_ENVY_HIGH_THRESHOLD) {
                    /* Check if the leader is better equipped */
                    if (!IS_NPC(best_target_leader) && (GET_LEVEL(best_target_leader) > GET_LEVEL(ch) ||
                                                        GET_GOLD(best_target_leader) > GET_GOLD(ch) * 2)) {
                        act("$n olha invejosamente para $N e recusa-se a juntar ao grupo.", TRUE, ch, 0,
                            best_target_leader, TO_ROOM);
                        return FALSE;
                    }
                }
            }

            if (rand_number(1, 120) <= chance_aceitar) {
                join_group(ch, GROUP(best_target_leader));
                act("$n junta-se ao grupo de $N.", TRUE, ch, 0, best_target_leader, TO_ROOM);
                /* Safety check: act() can trigger DG scripts which may cause extraction */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    return FALSE;
                if (MOB_FLAGGED(best_target_leader, MOB_NOTDEADYET) || PLR_FLAGGED(best_target_leader, PLR_NOTDEADYET))
                    return FALSE;
                return TRUE;
            }
        } else {
            /* 2. Não encontrou grupo. Verifica se é possível formar um novo. */
            struct char_data *leader_to_be = find_best_leader_for_new_group(ch);

            if (leader_to_be != NULL && leader_to_be == ch) {
                /* O próprio mob 'ch' é o melhor candidato a líder, então ele cria o grupo. */
                struct group_data *new_group = create_group(ch);
                if (new_group) {
                    SET_BIT(new_group->group_flags, GROUP_ANON);
                    act("$n parece estar a formar um grupo e à procura de companheiros.", TRUE, ch, 0, 0, TO_ROOM);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/**
 * Um membro do grupo avalia o seu inventário completo (incluindo contentores) e
 * partilha o melhor upgrade possível com o companheiro de equipa que mais
 * beneficiaria dele.
 * VERSÃO FINAL COM GESTÃO DE CONTENTORES.
 * Retorna TRUE se uma ação de partilha foi realizada.
 */
bool mob_share_gear_with_group(struct char_data *ch)
{
    /* A IA só age se o mob estiver num grupo, tiver itens e não for encantado. */
    if (!GROUP(ch) || !GROUP(ch)->members || GROUP(ch)->members->iSize == 0 || ch->carrying == NULL || !ch->ai_data ||
        AFF_FLAGGED(ch, AFF_CHARM)) {
        return FALSE;
    }

    /* Chance de executar a avaliação para não sobrecarregar. */
    if (rand_number(1, 100) > 25) {
        return FALSE;
    }

    struct obj_data *item_to_give = NULL;
    struct char_data *receiver = NULL;
    struct obj_data *container_source = NULL; /* De onde o item será tirado */
    int max_improvement_for_group = 10;       /* Só partilha se for uma melhoria significativa */

    struct obj_data *item;
    struct char_data *member;
    struct iterator_data iterator, inner_iterator;

    /* 1. O mob avalia todo o seu inventário para encontrar a melhor oportunidade de partilha. */
    for (item = ch->carrying; item; item = item->next_content) {
        /* Avalia o item atual do inventário principal. */
        member = (struct char_data *)merge_iterator(&iterator, GROUP(ch)->members);
        while (member) {
            if (ch != member) {
                int wear_pos = find_eq_pos(member, item, NULL);
                if (wear_pos != -1) {
                    int improvement =
                        evaluate_item_for_mob(member, item) - evaluate_item_for_mob(member, GET_EQ(member, wear_pos));
                    if (improvement > max_improvement_for_group) {
                        max_improvement_for_group = improvement;
                        item_to_give = item;
                        receiver = member;
                        container_source = NULL;
                    }
                }
            }
            member = (struct char_data *)next_in_list(&iterator);
        }
        remove_iterator(&iterator);

        /* Se o item for um contentor, avalia os itens lá dentro. */
        if (GET_OBJ_TYPE(item) == ITEM_CONTAINER && !OBJVAL_FLAGGED(item, CONT_CLOSED)) {
            struct obj_data *contained_item;
            for (contained_item = item->contains; contained_item; contained_item = contained_item->next_content) {
                member = (struct char_data *)merge_iterator(&inner_iterator, GROUP(ch)->members);
                while (member) {
                    if (ch == member || IN_ROOM(ch) != IN_ROOM(member)) {
                        member = (struct char_data *)next_in_list(&inner_iterator);
                        continue;
                    }
                    int wear_pos = find_eq_pos(member, contained_item, NULL);
                    if (wear_pos != -1) {
                        int improvement = evaluate_item_for_mob(member, contained_item) -
                                          evaluate_item_for_mob(member, GET_EQ(member, wear_pos));
                        if (improvement > max_improvement_for_group) {
                            max_improvement_for_group = improvement;
                            item_to_give = contained_item;
                            receiver = member;
                            container_source = item; /* Lembra-se de que o item está neste contentor. */
                        }
                    }
                    member = (struct char_data *)next_in_list(&inner_iterator);
                }
                remove_iterator(&inner_iterator);
            }
        }
    }

    /* 2. Se, depois de analisar tudo, encontrou uma boa partilha, executa-a. */
    if (item_to_give && receiver) {
        /* Se o item estiver num contentor, tira-o primeiro. */
        if (container_source) {
            obj_from_obj(item_to_give);
            obj_to_char(item_to_give, ch);
            act("$n tira $p de $P.", TRUE, ch, item_to_give, container_source, TO_ROOM);
        }

        /* Agora, o item está no inventário principal. Executa a partilha. */
        perform_give(ch, receiver, item_to_give);

        /* APRENDIZAGEM: Comportamento cooperativo é recompensado. */
        ch->ai_data->genetics.group_tendency += 3;
        ch->ai_data->genetics.group_tendency = MIN(ch->ai_data->genetics.group_tendency, 100);

        return TRUE; /* Ação de partilha foi realizada. */
    }

    return FALSE;
}

/**
 * Função de apoio que executa o movimento, táticas pós-movimento, e aprendizagem.
 * VERSÃO FINAL COM APRENDIZAGEM AMBIENTAL.
 * @param ch O mob que se move.
 * @param dir A direção do movimento.
 * @param should_close_behind TRUE se a IA decidiu que deve fechar a porta.
 * @param was_in A sala de onde o mob veio.
 * @return TRUE se o mob se moveu, FALSE caso contrário.
 */
bool perform_move_IA(struct char_data *ch, int dir, bool should_close_behind, int was_in)
{
    int old_pos = GET_POS(ch);

    if (perform_move(ch, dir, 1)) {
        /* O movimento foi bem-sucedido. */

        /* TÁTICA PÓS-MOVIMENTO: fechar a porta atrás de si (já implementado). */
        if (should_close_behind) {
            int back_door = rev_dir[dir];
            if (EXIT(ch, back_door) && EXIT(ch, back_door)->to_room == was_in && EXIT(ch, back_door)->keyword != NULL &&
                !IS_SET(EXIT(ch, back_door)->exit_info, EX_DNCLOSE)) {
                do_doorcmd(ch, NULL, back_door, SCMD_CLOSE);
            }
        }

        /******************************************************************
         * APRENDIZAGEM PÓS-MOVIMENTO (VERSÃO FINAL E REFINADA)
         ******************************************************************/
        if (ch->ai_data) {
            /* Safety check: Validate room before accessing world array */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return TRUE; /* Movement succeeded but skip learning */

            int roam_change = 0;
            int current_sect = world[IN_ROOM(ch)].sector_type;

            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) || current_sect == SECT_LAVA || current_sect == SECT_QUICKSAND) {
                roam_change = -20; /* Penalidade severa por perigo mortal. */
                act("$n grita de dor ao entrar na armadilha!", FALSE, ch, 0, 0, TO_ROOM);
            } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FROZEN)) {
                roam_change = -10; /* Penalidade média por desconforto. */
                act("$n treme de frio ao entrar na sala gelada.", FALSE, ch, 0, 0, TO_ROOM);
            } else if (current_sect == SECT_ICE && GET_POS(ch) < old_pos) {
                roam_change = -5; /* Penalidade leve por escorregar. */
            } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HEAL)) {
                roam_change = 10; /* Recompensa média por encontrar um santuário. */
                act("$n parece revigorado ao entrar nesta sala.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                roam_change = 1; /* Recompensa normal por exploração segura. */
            }

            ch->ai_data->genetics.roam_tendency += roam_change;
            ch->ai_data->genetics.roam_tendency = MIN(MAX(GET_GENROAM(ch), 0), 100);
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * IA de exploração orientada a objetivos. O mob agora vagueia com um propósito.
 * Se um 'target_room' for fornecido, ele tentará navegar até lá.
 * Se não, ele usa a sua lógica de exploração padrão.
 * VERSÃO FINAL COM NAVEGAÇÃO POR OBJETIVO E PATHFINDING INTELIGENTE.
 * Retorna TRUE se uma ação de roam foi executada.
 */
bool mob_goal_oriented_roam(struct char_data *ch, room_rnum target_room)
{
    static int roam_throttle = 0;

    if (ch->master != NULL || FIGHTING(ch) || GET_POS(ch) < POS_STANDING)
        return FALSE;

    /* Throttle pathfinding calls to prevent resource exhaustion */
    roam_throttle++;
    if (target_room != NOWHERE && roam_throttle % 4 != 0) {
        /* Only do complex pathfinding every 4th call for goal-oriented movement */
        return FALSE;
    }

    int direction = -1;
    bool has_goal = FALSE;

    /* --- FASE 1: DEFINIÇÃO DO OBJETIVO --- */

    /* Se um destino específico foi dado, essa é a prioridade máxima. */
    if (target_room != NOWHERE && IN_ROOM(ch) != target_room) {
        /* Use intelligent pathfinding that compares basic vs advanced methods */
        direction = mob_smart_pathfind(ch, target_room);
        if (direction == -1) {
            /* Fall back to simple pathfinding if smart pathfinding fails */
            direction = find_first_step(IN_ROOM(ch), target_room);

            /* If both pathfinding methods fail, check if it's due to missing keys */
            if (direction == -1 && ch->ai_data && ch->ai_data->current_goal != GOAL_COLLECT_KEY) {
                /* Check if we need a key to reach the target */
                obj_vnum blocking_key = find_blocking_key(ch, IN_ROOM(ch), target_room);
                if (blocking_key != NOTHING) {
                    /* Set key collection goal with current goal as original */
                    if (mob_set_key_collection_goal(ch, blocking_key, ch->ai_data->current_goal, target_room)) {
                        return TRUE; /* Goal changed to key collection */
                    }
                }
            }
        }
        has_goal = TRUE;
    } else {
        /* Se nenhum destino foi dado, usa a lógica de exploração padrão. */
        if (MOB_FLAGGED(ch, MOB_SENTINEL) && ch->ai_data) {
            /* Check if sentinel has an active quest goal - quest goals take precedence over guard duty */
            bool sentinel_has_quest_goal = (ch->ai_data->current_goal == GOAL_COMPLETE_QUEST);
            if (!sentinel_has_quest_goal && IN_ROOM(ch) != real_room(ch->ai_data->guard_post)) {
                /* Use specialized duty pathfinding for sentinels returning to post */
                direction = mob_duty_pathfind(ch, real_room(ch->ai_data->guard_post));
                if (direction == -1) {
                    direction = find_first_step(IN_ROOM(ch), real_room(ch->ai_data->guard_post));
                }
                has_goal = TRUE;
            } else {
                /* Sentinels with quest goals should roam normally to seek quest objectives
                 * base_roam: 25% = normal mob roam chance, 1% = sentinel standing guard */
                int base_roam = sentinel_has_quest_goal ? 25 : 1;
                int need_bonus = (GET_EQ(ch, WEAR_WIELD) == NULL ? 20 : 0) + (!GROUP(ch) ? 10 : 0);
                int final_chance = MIN(base_roam + GET_GENROAM(ch) + need_bonus, 90);

                if (rand_number(1, 100) <= final_chance) {
                    direction = rand_number(0, DIR_COUNT - 1);
                    has_goal = TRUE;
                }
            }
        } else {
            /* base_roam: 25% = normal mob roam chance, 1% = sentinel standing guard */
            int base_roam = MOB_FLAGGED(ch, MOB_SENTINEL) ? 1 : 25;
            int need_bonus = (GET_EQ(ch, WEAR_WIELD) == NULL ? 20 : 0) + (!GROUP(ch) ? 10 : 0);
            int final_chance = MIN(base_roam + GET_GENROAM(ch) + need_bonus, 90);

            if (rand_number(1, 100) <= final_chance) {
                direction = rand_number(0, DIR_COUNT - 1);
                has_goal = TRUE;
            }
        }
    }

    /* --- FASE 2: EXECUÇÃO DA AÇÃO SE HOUVER UM OBJETIVO --- */
    if (has_goal && direction >= 0 && direction < DIR_COUNT) { /* Verificação de segurança para direction */
        struct room_direction_data *exit;
        room_rnum to_room;

        if ((exit = EXIT(ch, direction)) && (to_room = exit->to_room) != NOWHERE && to_room >= 0 &&
            to_room <= top_of_world) {

            /* GESTÃO DE VOO (Ação que consome o turno) */
            if (AFF_FLAGGED(ch, AFF_FLYING) && ROOM_FLAGGED(to_room, ROOM_NO_FLY))
                stop_flying(ch);
            if (!AFF_FLAGGED(ch, AFF_FLYING) && world[to_room].sector_type == SECT_CLIMBING)
                start_flying(ch);

            /* RESOLUÇÃO DE PORTAS (UMA AÇÃO DE CADA VEZ, E APENAS EM PORTAS REAIS) */
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED)) {
                if (!IS_SET(exit->exit_info, EX_DNOPEN)) {
                    if (IS_SET(exit->exit_info, EX_LOCKED) && has_key(ch, exit->key)) {
                        do_doorcmd(ch, NULL, direction, SCMD_UNLOCK);
                    }
                    if (!IS_SET(exit->exit_info, EX_LOCKED)) {
                        do_doorcmd(ch, NULL, direction, SCMD_OPEN);
                    }
                }
            }

            /* Se, depois de tudo, a porta ainda estiver fechada, a IA não pode passar. */
            if (IS_SET(exit->exit_info, EX_CLOSED)) {
                return FALSE;
            }

            /* Verificações Finais de Caminho */
            if ((IS_SET(exit->exit_info, EX_HIDDEN) && rand_number(1, 20) > GET_WIS(ch)) ||
                (world[to_room].sector_type == SECT_UNDERWATER && !has_scuba(ch))) {
                return FALSE;
            }

            /* Prevent mobs from entering death traps during normal wandering */
            if (IS_NPC(ch) && ROOM_FLAGGED(to_room, ROOM_DEATH)) {
                return FALSE;
            }

            /* Safety check: Validate ch's room before zone comparison */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return FALSE;

            if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && (world[to_room].zone != world[IN_ROOM(ch)].zone)) {
                /* For shopping goals, be more willing to cross zones */
                int zone_cross_chance = (ch->ai_data && ch->ai_data->current_goal == GOAL_GOTO_SHOP_TO_SELL) ? 25 : 1;
                if (rand_number(1, 100) > zone_cross_chance) {
                    return TRUE; /* Hesitou e gastou o turno. */
                }
            }

            /* Movimento Final */
            int was_in = IN_ROOM(ch);
            bool should_close = (GET_INT(ch) > 12 && IS_SET(exit->exit_info, EX_ISDOOR) &&
                                 !IS_SET(exit->exit_info, EX_DNCLOSE) && rand_number(1, 100) <= 25);
            return perform_move_IA(ch, direction, should_close, was_in);
        }
    }

    return FALSE;
}

/**
 * Gere a rotina de dever para lojistas e sentinelas, agora com um
 * "temporizador de frustração" para evitar que fiquem presos.
 * Retorna TRUE se uma ação de dever foi executada ou se o mob deve ficar parado.
 */
bool handle_duty_routine(struct char_data *ch)
{
    bool is_shopkeeper = (mob_index[GET_MOB_RNUM(ch)].func == shop_keeper);
    bool is_sentinel = MOB_FLAGGED(ch, MOB_SENTINEL);

    if (!is_shopkeeper && !is_sentinel)
        return FALSE; /* Não tem dever, a IA prossegue. */

    /******************************************************************
     * LÓGICA DE FRUSTRAÇÃO
     ******************************************************************/
    /* Se o mob está frustrado, ele "desiste" de voltar para casa por agora. */
    if (ch->ai_data && ch->ai_data->duty_frustration_timer > 0) {
        return FALSE; /* Permite que o mob execute outras IAs (loot, roam, etc.). */
    }

    /******************************************************************
     * SENTINELS COM GOALS DE QUEST TÊM PRIORIDADE TEMPORÁRIA
     ******************************************************************/
    /* Allow sentinels to temporarily abandon their post for quest activities */
    if (is_sentinel && ch->ai_data &&
        (ch->ai_data->current_goal == GOAL_POST_QUEST || ch->ai_data->current_goal == GOAL_GOTO_QUESTMASTER ||
         ch->ai_data->current_goal == GOAL_ACCEPT_QUEST || ch->ai_data->current_goal == GOAL_COMPLETE_QUEST)) {
        return FALSE; /* Let quest-related AI take priority over guard duty */
    }

    bool is_on_duty = FALSE;
    room_rnum home_room = NOWHERE;

    if (is_shopkeeper) {
        int shop_nr = find_shop_by_keeper(GET_MOB_RNUM(ch));
        if (shop_nr != -1 && shop_nr <= top_shop && is_shop_open(shop_nr) && shop_index[shop_nr].in_room) {
            is_on_duty = TRUE;
            home_room = real_room(SHOP_ROOM(shop_nr, 0));
        }
    } else { /* É um Sentinela */
        is_on_duty = TRUE;
        if (ch->ai_data)
            home_room = real_room(ch->ai_data->guard_post);
    }

    if (is_on_duty) {
        /* Verifica se o mob já está no seu posto. */
        if ((is_shopkeeper && ok_shop_room(find_shop_by_keeper(GET_MOB_RNUM(ch)), GET_ROOM_VNUM(IN_ROOM(ch)))) ||
            (is_sentinel && IN_ROOM(ch) == home_room)) {
            return TRUE; /* Está no posto, não faz mais nada. Fim do turno. */
        }

        /* Se não está no posto, tenta voltar usando pathfinding inteligente. */
        if (home_room != NOWHERE) {
            static int duty_pathfind_calls = 0;
            int direction = -1;

            /* Use specialized duty pathfinding with priority caching */
            direction = mob_duty_pathfind(ch, home_room);

            /* Fallback to throttled pathfinding if duty pathfinding fails */
            if (direction == -1) {
                duty_pathfind_calls++;
                if (duty_pathfind_calls % 5 == 0) {
                    /* Only do complex pathfinding every 5th call for duty movement */
                    direction = mob_smart_pathfind(ch, home_room);
                } else {
                    /* Use simple pathfinding most of the time */
                    direction = find_first_step(IN_ROOM(ch), home_room);
                }
            }

            if (direction == -1) {
                /* Fall back to basic pathfinding if duty pathfinding fails */
                direction = find_first_step(IN_ROOM(ch), home_room);

                /* If both pathfinding methods fail, check if it's due to missing keys */
                if (direction == -1 && ch->ai_data && ch->ai_data->current_goal != GOAL_COLLECT_KEY) {
                    obj_vnum blocking_key = find_blocking_key(ch, IN_ROOM(ch), home_room);
                    if (blocking_key != NOTHING) {
                        /* Set key collection goal with return to post as original */
                        if (mob_set_key_collection_goal(ch, blocking_key, GOAL_RETURN_TO_POST, home_room)) {
                            return TRUE; /* Goal changed to key collection */
                        }
                    }
                }
            }

            if (direction >= 0) {
                perform_move(ch, direction, 1);
            } else {
                /******************************************************************
                 * O CAMINHO FALHOU! O MOB FICA FRUSTRADO.
                 *******************************************************************/
                if (ch->ai_data) {
                    ch->ai_data->duty_frustration_timer = 6; /* Fica frustrado por 6 pulsos de IA. */
                }
                return FALSE; /* Permite que outras IAs sejam executadas. */
            }
            return TRUE; /* Tentou voltar, consome o turno. */
        }
    }

    /* Se não está de serviço, está de "folga". */
    return FALSE;
}

/**
 * Lógica para um membro de grupo seguir o seu líder se estiverem em salas diferentes.
 * VERSÃO REFINADA: Implementa a "Hierarquia de Comando".
 * @param ch O mob a executar a ação.
 * @return TRUE se o mob tentou mover-se, consumindo o seu turno de IA.
 */
bool mob_follow_leader(struct char_data *ch)
{
    /* A função só se aplica a membros de um grupo (que não são o líder). */
    if (!GROUP(ch) || GROUP_LEADER(GROUP(ch)) == ch)
        return FALSE;

    struct char_data *leader = GROUP_LEADER(GROUP(ch));

    /* Verifica se o líder é válido e se está numa sala diferente. */
    if (leader != NULL && IN_ROOM(ch) != IN_ROOM(leader)) {

        /******************************************************************/
        /* REFINAMENTO: A IA agora entende a "Hierarquia de Comando".     */
        /******************************************************************/
        bool duty_is_overridden_by_player = FALSE;

        /* Se o mob está encantado, o seu dever é seguir o mestre. */
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master == leader) {
            duty_is_overridden_by_player = TRUE;
        }
        /* Se o líder é um jogador (o grupo não tem a flag NPC), o dever é seguir o líder. */
        if (!IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_NPC)) {
            duty_is_overridden_by_player = TRUE;
        }

        /* Um Sentinela só pode ser "teimoso" se não estiver sob ordens diretas de um jogador. */
        if (MOB_FLAGGED(ch, MOB_SENTINEL) && !duty_is_overridden_by_player && rand_number(1, 100) > 2) {
            return FALSE; /* Ficou no posto, leal ao seu dever original. */
        }

        /* Tenta encontrar o caminho até ao líder usando pathfinding inteligente. */
        static int follow_pathfind_calls = 0;
        int direction = -1;

        /* Throttle follow pathfinding to reduce resource usage */
        follow_pathfind_calls++;
        if (follow_pathfind_calls % 6 == 0) {
            /* Only do complex pathfinding every 6th call for following */
            direction = mob_smart_pathfind(ch, IN_ROOM(leader));
        } else {
            /* Use simple pathfinding most of the time for following */
            direction = find_first_step(IN_ROOM(ch), IN_ROOM(leader));
        }

        if (direction == -1) {
            /* Fall back to basic pathfinding if smart pathfinding fails */
            direction = find_first_step(IN_ROOM(ch), IN_ROOM(leader));

            /* If both pathfinding methods fail, check if it's due to missing keys */
            if (direction == -1 && ch->ai_data && ch->ai_data->current_goal != GOAL_COLLECT_KEY) {
                obj_vnum blocking_key = find_blocking_key(ch, IN_ROOM(ch), IN_ROOM(leader));
                if (blocking_key != NOTHING) {
                    /* Don't set a formal goal for following, just fail gracefully */
                    /* Following is a lower priority activity */
                    return FALSE;
                }
            }
        }

        if (direction >= 0) {
            room_rnum to_room;
            if (EXIT(ch, direction) && (to_room = EXIT(ch, direction)->to_room) <= top_of_world) {

                /* Regra especial para salas NOMOB. */
                if (ROOM_FLAGGED(to_room, ROOM_NOMOB)) {
                    /* Só pode entrar se o líder for um jogador (o que já é verificado acima). */
                    if (IS_NPC(leader)) {
                        return FALSE;
                    }
                }

                /* Try to move - perform_move will handle doors/obstacles */
                perform_move(ch, direction, 1);
                return TRUE; /* Ação de seguir foi executada. */
            }
        }
    }

    return FALSE; /* Nenhuma ação de seguir foi executada. */
}

/**
 * Tenta fazer o mob seguir outro personagem (player ou mob) sem formar grupo.
 * Usado para comportamento furtivo/evil (espionagem, roubo, etc).
 * Retorna TRUE se o mob começou ou continuou a seguir alguém.
 */
bool mob_try_stealth_follow(struct char_data *ch)
{
    /* Safety check: Validate room */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    /* Só funciona para mobs com ai_data e que não estão em combate */
    if (!ch->ai_data || FIGHTING(ch))
        return FALSE;

    /* Não segue se já está em grupo ou encantado */
    if (GROUP(ch) || AFF_FLAGGED(ch, AFF_CHARM))
        return FALSE;

    /* Verifica a tendência de seguir */
    int follow_tendency = GET_GENFOLLOW(ch);

    /* Mobs sentinelas não devem seguir (ficam no posto) */
    if (MOB_FLAGGED(ch, MOB_SENTINEL) && rand_number(1, 100) > 2)
        return FALSE;

    /* Se já está seguindo alguém, verifica se deve continuar */
    if (ch->master) {
        /* Safety check: Validate master pointer and room */
        if (!ch->master || IN_ROOM(ch->master) == NOWHERE || IN_ROOM(ch->master) < 0 ||
            IN_ROOM(ch->master) > top_of_world) {
            /* Master is invalid or in invalid room, stop following */
            stop_follower(ch);
            return FALSE;
        }

        /* Check if mob can still see the target */
        if (!CAN_SEE(ch, ch->master)) {
            /* Lost visibility of target, stop following so we can try a new target later */
            /* Small decrease - visibility loss can be temporary/environmental */
            ch->ai_data->genetics.follow_tendency -= 1;
            ch->ai_data->genetics.follow_tendency = MAX(ch->ai_data->genetics.follow_tendency, 0);
            stop_follower(ch);
            return FALSE;
        }

        /* Se o alvo saiu da sala, segue ele */
        if (IN_ROOM(ch) != IN_ROOM(ch->master)) {
            int direction = find_first_step(IN_ROOM(ch), IN_ROOM(ch->master));
            if (direction >= 0 && direction < DIR_COUNT) {
                room_rnum to_room;
                /* Safety check: Validate exit before accessing */
                if (EXIT(ch, direction) && (to_room = EXIT(ch, direction)->to_room) != NOWHERE && to_room >= 0 &&
                    to_room <= top_of_world) {
                    /* Resolve portas se necessário */
                    if (IS_SET(EXIT(ch, direction)->exit_info, EX_ISDOOR) &&
                        IS_SET(EXIT(ch, direction)->exit_info, EX_CLOSED)) {
                        if (!IS_SET(EXIT(ch, direction)->exit_info, EX_DNOPEN)) {
                            if (IS_SET(EXIT(ch, direction)->exit_info, EX_LOCKED) &&
                                has_key(ch, EXIT(ch, direction)->key)) {
                                do_doorcmd(ch, NULL, direction, SCMD_UNLOCK);
                            }
                            if (!IS_SET(EXIT(ch, direction)->exit_info, EX_LOCKED)) {
                                do_doorcmd(ch, NULL, direction, SCMD_OPEN);
                            }
                        }
                    }

                    /* Tenta mover-se discretamente */
                    if (!IS_SET(EXIT(ch, direction)->exit_info, EX_CLOSED)) {
                        perform_move(ch, direction, 1);
                        /* Reward for successfully following/moving with target */
                        ch->ai_data->genetics.follow_tendency += 1;
                        ch->ai_data->genetics.follow_tendency = MIN(ch->ai_data->genetics.follow_tendency, 100);
                        return TRUE;
                    }
                } else {
                    /* Cannot find valid path, stop following */
                    /* Moderate decrease for path failure */
                    ch->ai_data->genetics.follow_tendency -= 2;
                    ch->ai_data->genetics.follow_tendency = MAX(ch->ai_data->genetics.follow_tendency, 0);
                    stop_follower(ch);
                    return FALSE;
                }
            } else {
                /* Cannot find path to target, stop following */
                /* Moderate decrease for path failure */
                ch->ai_data->genetics.follow_tendency -= 2;
                ch->ai_data->genetics.follow_tendency = MAX(ch->ai_data->genetics.follow_tendency, 0);
                stop_follower(ch);
                return FALSE;
            }
        }
        /* Já está na mesma sala que o alvo, mantém a observação */
        return TRUE;
    }

    /* Não está seguindo ninguém, decide se deve começar */
    /* Chance base ajustada pela tendência genética */
    if (rand_number(1, 100) > follow_tendency)
        return FALSE;

    /* Procura um alvo adequado para seguir na sala */
    struct char_data *target = NULL;
    struct char_data *vict;
    int best_score = 0;

    /* Safety check: Validate room before iterating people list */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
        /* Safety check: Validate vict pointer */
        if (!vict || vict == ch)
            continue;

        /* Only follow targets that are visible to the mob */
        if (!CAN_SEE(ch, vict))
            continue;

        /* Safety check: Validate vict's room */
        if (IN_ROOM(vict) == NOWHERE || IN_ROOM(vict) < 0 || IN_ROOM(vict) > top_of_world)
            continue;

        /* Don't follow immortals (level LVL_IMMORT or above) */
        if (GET_LEVEL(vict) >= LVL_IMMORT)
            continue;

        /* Preferência por players, especialmente se o mob for evil */
        int score = 0;

        if (!IS_NPC(vict)) {
            score += 50; /* Players são alvos mais interessantes */

            /* Mobs evil têm maior interesse em seguir players good */
            if (GET_ALIGNMENT(ch) < -200 && IS_GOOD(vict))
                score += 30;

            /* Mobs com alto nível de roaming também seguem mais */
            score += GET_GENROAM(ch) / 5;
        } else {
            /* Outros mobs podem ser seguidos também */
            score += 20;

            /* Prefere mobs com itens valiosos ou de nível maior */
            if (GET_LEVEL(vict) > GET_LEVEL(ch))
                score += 10;
        }

        /* Evita seguir mobs agressivos ou em combate */
        if (FIGHTING(vict) || (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_AGGRESSIVE)))
            score -= 50;

        if (score > best_score) {
            best_score = score;
            target = vict;
        }
    }

    /* Se encontrou um bom alvo, começa a seguir */
    if (target && best_score > 30) {
        /* Verifica se não criaria um loop circular de seguimento */
        if (circle_follow(ch, target)) {
            return FALSE;
        }

        /* Stealth-aware following: Check if mob has sneak/hide affects */
        bool is_stealthy = AFF_FLAGGED(ch, AFF_SNEAK) || AFF_FLAGGED(ch, AFF_HIDE);

        /* Manually set up following relationship (like add_follower but with conditional messages) */
        if (!ch->master) {
            struct follow_type *k;
            ch->master = target;
            CREATE(k, struct follow_type, 1);
            k->follower = ch;
            k->next = target->followers;
            target->followers = k;

            /* Messages are conditional based on stealth */
            if (!is_stealthy) {
                /* Normal visible following */
                if (IS_NPC(ch)) {
                    /* NPCs don't get the "You will now follow" message */
                } else {
                    act("Você agora irá seguir $N.", FALSE, ch, 0, target, TO_CHAR);
                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return FALSE;
                    if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                        return FALSE;
                }
                if (CAN_SEE(target, ch)) {
                    act("$n começa a seguir você.", TRUE, ch, 0, target, TO_VICT);
                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return FALSE;
                    if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                        return FALSE;
                }
                act("$n começa a seguir $N.", TRUE, ch, 0, target, TO_NOTVICT);
                /* Safety check: act() can trigger DG scripts which may cause extraction */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    return FALSE;
                if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                    return FALSE;
            } else {
                /* Stealthy following - only notify if target can see through stealth */
                if (CAN_SEE(target, ch)) {
                    /* Target can see through the stealth */
                    act("$n começa a seguir você silenciosamente.", TRUE, ch, 0, target, TO_VICT);
                    /* Safety check: act() can trigger DG scripts which may cause extraction */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return FALSE;
                    if (MOB_FLAGGED(target, MOB_NOTDEADYET) || PLR_FLAGGED(target, PLR_NOTDEADYET))
                        return FALSE;
                }
                /* No message to room when sneaking/hiding */
            }
        }

        /* Aumenta levemente a tendência de seguir se teve sucesso (menor peso que a frustração) */
        ch->ai_data->genetics.follow_tendency += 1;
        ch->ai_data->genetics.follow_tendency = MIN(ch->ai_data->genetics.follow_tendency, 100);
        return TRUE;
    }

    /* Se não encontrou ninguém para seguir, pequena chance de esquecer/reduzir interesse */
    if (rand_number(1, 100) <= 2) { /* 2% chance por tick de reduzir naturalmente */
        ch->ai_data->genetics.follow_tendency -= 1;
        ch->ai_data->genetics.follow_tendency = MAX(ch->ai_data->genetics.follow_tendency, 0);
    }

    return FALSE;
}

/**
 * A IA principal para um mob ocioso decidir ajudar aliados em combate.
 * Consolida as lógicas de grupo, charmed e helper.
 * Retorna TRUE se o mob entrou em combate.
 */
bool mob_assist_allies(struct char_data *ch)
{
    struct char_data *ally_in_trouble = NULL;
    struct char_data *target_to_attack = NULL;
    int max_threat_level = 0;

    /* A IA só age se o mob estiver ocioso e puder ver. */
    if (FIGHTING(ch) || AFF_FLAGGED(ch, AFF_BLIND)) {
        return FALSE;
    }

    /* PRIORIDADE 1: Ajudar o Mestre (se estiver encantado) */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && FIGHTING(ch->master)) {
        if (IN_ROOM(ch) == IN_ROOM(ch->master)) {
            ally_in_trouble = ch->master;
            target_to_attack = FIGHTING(ch->master);
        }
    }

    /* PRIORIDADE 2: Ajudar o Grupo (se não tiver um mestre para ajudar) */
    else if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize) {
        struct char_data *member;
        struct iterator_data iterator;

        member = (struct char_data *)merge_iterator(&iterator, GROUP(ch)->members);
        while (member) {
            if (ch != member && IN_ROOM(ch) == IN_ROOM(member) && FIGHTING(member)) {
                /* Lógica de "priorizar a maior ameaça" */
                if (GET_LEVEL(FIGHTING(member)) > max_threat_level) {
                    max_threat_level = GET_LEVEL(FIGHTING(member));
                    ally_in_trouble = member;
                    target_to_attack = FIGHTING(member);
                }
            }
            member = (struct char_data *)next_in_list(&iterator);
        }
        remove_iterator(&iterator);
    }

    /* PRIORIDADE 3: Ajudar outros NPCs (se tiver a flag MOB_HELPER) */
    else if (MOB_FLAGGED(ch, MOB_HELPER)) {
        /* Safety check: Validate room before accessing people list */
        if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) >= 0 && IN_ROOM(ch) <= top_of_world) {
            struct char_data *vict;
            for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
                if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
                    continue;
                if (IS_NPC(FIGHTING(vict))) /* Não ajuda mobs que lutam contra outros mobs */
                    continue;

                ally_in_trouble = vict;
                target_to_attack = FIGHTING(vict);
                break; /* Ajuda o primeiro que encontrar. */
            }
        }
    }

    /* Se encontrou alguém para ajudar, entra em combate. */
    if (ally_in_trouble && target_to_attack) {
        act("$n vê que $N está em apuros e corre para ajudar!", FALSE, ch, 0, ally_in_trouble, TO_NOTVICT);
        hit(ch, target_to_attack, TYPE_UNDEFINED);
        /* Safety check: hit() can indirectly cause extract_char */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            return TRUE;
        return TRUE;
    }

    return FALSE;
}

/**
 * Helper function to check if a healer can heal a target based on alignment.
 * Good heals good, evil heals evil, neutral can heal anyone (they're flexible).
 * @param healer The character attempting to heal.
 * @param target The character to be healed.
 * @return TRUE if healing is allowed based on alignment, FALSE otherwise.
 */
static bool can_heal_based_on_alignment(struct char_data *healer, struct char_data *target)
{
    /* Neutral characters are flexible and can heal anyone */
    if (IS_NEUTRAL(healer))
        return TRUE;

    /* Good characters heal good characters */
    if (IS_GOOD(healer) && IS_GOOD(target))
        return TRUE;

    /* Evil characters heal evil characters */
    if (IS_EVIL(healer) && IS_EVIL(target))
        return TRUE;

    return FALSE;
}

/**
 * Mob AI tries to heal an ally that is near death using bandage skill.
 * Checks alignment compatibility and healing tendency genetics.
 * @param ch The mob attempting to heal.
 * @return TRUE if healing was attempted, FALSE otherwise.
 */
bool mob_try_heal_ally(struct char_data *ch)
{
    struct char_data *ally_to_heal = NULL;
    int lowest_hp_percent = 100;

    /* Only mobs with AI data and healing tendency can heal */
    if (!ch->ai_data || ch->ai_data->genetics.healing_tendency <= 0)
        return FALSE;

    /* Must have the bandage skill */
    if (GET_SKILL(ch, SKILL_BANDAGE) <= 0)
        return FALSE;

    /* Can't heal if fighting, blind, or not standing */
    if (FIGHTING(ch) || AFF_FLAGGED(ch, AFF_BLIND) || GET_POS(ch) != POS_STANDING)
        return FALSE;

    /* Safety check: Validate room before accessing people list */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    /* Look for an ally in critical condition (negative HP) who needs healing */
    struct char_data *vict;

    /* Priority 1: Check master first (if charmed) */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        if (GET_HIT(ch->master) < 0 && !IS_DEAD(ch->master) && can_heal_based_on_alignment(ch, ch->master)) {
            ally_to_heal = ch->master;
        }
    }

    /* Priority 2 & 3: Check room if no master needs healing */
    if (!ally_to_heal) {
        for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
            /* Skip self, dead characters, and characters not in critical condition */
            if (ch == vict || IS_DEAD(vict) || GET_HIT(vict) >= 0)
                continue;

            /* Check alignment compatibility */
            if (!can_heal_based_on_alignment(ch, vict))
                continue;

            /* Priority 2: Heal group members */
            if (GROUP(ch) && GROUP(vict) && GROUP(ch) == GROUP(vict)) {
                ally_to_heal = vict;
                break;
            }

            /* Priority 3: Heal other NPCs with same alignment (find most critical) */
            if (IS_NPC(vict)) {
                int hp_percent = (GET_HIT(vict) * 100) / MAX(GET_MAX_HIT(vict), 1);
                if (hp_percent < lowest_hp_percent) {
                    lowest_hp_percent = hp_percent;
                    ally_to_heal = vict;
                }
            }
        }
    }

    /* If found someone to heal, check healing tendency and attempt healing */
    if (ally_to_heal) {
        /* Check if mob decides to heal based on genetics */
        if (rand_number(1, 100) > ch->ai_data->genetics.healing_tendency)
            return FALSE;

        /* Attempt to bandage the ally */
        char heal_arg[MAX_INPUT_LENGTH];
        snprintf(heal_arg, sizeof(heal_arg), "%s", GET_NAME(ally_to_heal));
        call_ACMD(do_bandage, ch, heal_arg, 0, 0);

        /* Safety check: call_ACMD might indirectly trigger extract_char */
        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
            return TRUE;

        return TRUE;
    }

    return FALSE;
}

/**
 * A IA tenta saquear o melhor item da sala, com base nas suas necessidades
 * e na sua genética. A sua "vontade" de procurar é aumentada se tiver
 * uma necessidade urgente (ex: sem arma, com vida baixa).
 * Retorna TRUE se uma ação de saque foi bem-sucedida.
 */
bool mob_try_and_loot(struct char_data *ch)
{
    /* Safety check: Validate room before accessing world array */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    /* A IA só age se houver itens na sala, se o mob tiver genética e não estiver em combate. */
    if (!world[IN_ROOM(ch)].contents || !ch->ai_data || FIGHTING(ch))
        return FALSE;

    /* 1. Calcula a tendência base do mob (genética + instinto). */
    int base_tendency = MOB_FLAGGED(ch, MOB_SCAVENGER) ? 25 : 0;
    int genetic_tendency = GET_GENLOOT(ch);
    int effective_tendency = base_tendency + genetic_tendency;

    /* 2. Calcula o bónus com base nas necessidades urgentes. */
    int need_bonus = 0;
    if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.8)
        need_bonus += 15; /* Precisa de itens de cura. */

    if (GET_EQ(ch, WEAR_WIELD) == NULL)
        need_bonus += 25; /* Precisa desesperadamente de uma arma. */

    if (GET_EQ(ch, WEAR_BODY) == NULL)
        need_bonus += 15; /* Precisa de armadura. */

    if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_FIREWEAPON && !mob_has_ammo(ch))
        need_bonus += 50; /* Necessidade MUITO ALTA por munição! */

    /* 3. A chance final é a combinação de tudo, com um mínimo de curiosidade. */
    int final_chance = MAX(effective_tendency + need_bonus, 10);
    final_chance = MIN(final_chance, 95); /* Limita a 95% para não ser garantido. */

    if (rand_number(1, 100) <= final_chance) {
        int max_score = 0;
        struct obj_data *obj, *best_obj = NULL;

        /* A IA agora procura ativamente pelo item que melhor satisfaz a sua necessidade. */
        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
            int current_score = evaluate_item_for_mob(ch, obj);
            if (current_score > max_score) {
                best_obj = obj;
                max_score = current_score;
            }
        }

        if (best_obj != NULL) {
            /* Safety check: Re-validate that best_obj is still in the room.
             * Another player/mob might have picked it up during the evaluation loop.
             * This prevents crashes from accessing freed memory. */
            struct obj_data *obj_check;
            bool obj_still_exists = FALSE;
            for (obj_check = world[IN_ROOM(ch)].contents; obj_check; obj_check = obj_check->next_content) {
                if (obj_check == best_obj) {
                    obj_still_exists = TRUE;
                    break;
                }
            }

            if (!obj_still_exists) {
                /* Object was taken by someone else, abort this loot attempt */
                return FALSE;
            }

            /* Chama a função do jogo para pegar o item, garantindo todas as verificações. */
            if (perform_get_from_room(ch, best_obj)) {
                /* Aprendizagem Positiva: A decisão foi boa e bem-sucedida. */
                ch->ai_data->genetics.loot_tendency += 2;
                ch->ai_data->genetics.loot_tendency = MIN(ch->ai_data->genetics.loot_tendency, 100);
                return TRUE; /* Ação bem-sucedida, consome o turno. */
            }
            //} else {
            /* Aprendizagem Negativa: A necessidade não foi satisfeita. */
            //    ch->ai_data->genetics.loot_tendency -= 1;
            //    ch->ai_data->genetics.loot_tendency = MAX(ch->ai_data->genetics.loot_tendency, 0);
        }
    }

    return FALSE; /* Nenhuma ação de saque foi executada. */
}

/**
 * A IA entra numa "sessão de equipamento", onde percorre o seu inventário
 * (incluindo contentores) várias vezes para encontrar e vestir todos os upgrades possíveis.
 * VERSÃO FINAL COMPLETA.
 * Retorna TRUE se conseguiu equipar pelo menos um item.
 */
bool mob_try_and_upgrade(struct char_data *ch)
{
    if (!ch->ai_data || AFF_FLAGGED(ch, AFF_CHARM))
        return FALSE;

    /* A chance de sequer pensar em se equipar é baseada na genética. */
    if (rand_number(1, 100) > MIN(MAX(GET_GENEQUIP(ch), 5), 90))
        return FALSE;

    bool performed_an_upgrade_this_pulse = FALSE;
    bool keep_trying = TRUE;
    int max_iterations = 10; /* Prevent infinite loops by limiting iterations */
    int iteration_count = 0;

    /* O loop 'while' garante que o mob continua a tentar equipar até estar otimizado. */
    while (keep_trying && iteration_count < max_iterations) {
        iteration_count++;

        /* Pede à nossa função de busca para encontrar o melhor plano de upgrade. */
        struct mob_upgrade_plan plan = find_best_upgrade_for_mob(ch);

        /* Se encontrou um upgrade viável (melhoria > 0), executa-o. */
        if (plan.item_to_equip && plan.improvement_score > 0) {

            /* PASSO 1: Se o item estiver num contentor, tira-o primeiro. */
            if (plan.container) {
                obj_from_obj(plan.item_to_equip);
                obj_to_char(plan.item_to_equip, ch);
            }

            /* PASSO 2: Remove o item antigo que está no slot. */
            struct obj_data *equipped_item = GET_EQ(ch, plan.wear_pos);
            if (equipped_item) {
                perform_remove(ch, plan.wear_pos);
            }

            /* PASSO 3: Equipa o novo item no slot agora vazio. */
            perform_wear(ch, plan.item_to_equip, plan.wear_pos);

            performed_an_upgrade_this_pulse = TRUE;
            /* keep_trying continua TRUE para que ele re-avalie o inventário. */

        } else {
            /* Se não encontrou mais upgrades viáveis, para a sessão. */
            keep_trying = FALSE;
        }
    } /* Fim do loop 'while' */

    /* Log if we hit the iteration limit to help debug infinite loops */
    if (iteration_count >= max_iterations) {
        log1("SYSERR: mob_try_and_upgrade hit iteration limit for mob %s (vnum %d)", GET_NAME(ch), GET_MOB_VNUM(ch));
    }

    /* A aprendizagem acontece uma vez no final da sessão. */
    if (performed_an_upgrade_this_pulse) {
        ch->ai_data->genetics.equip_tendency = MIN(ch->ai_data->genetics.equip_tendency + 2, 100);
        /* Se mob não se equipar, fica frustrado demais e a tendencia cai a 0. Devemos evitar isso.
         }
         else {
            ch->ai_data->genetics.equip_tendency = MAX(ch->ai_data->genetics.equip_tendency - 1, 0);
        */
    }

    /* Retorna TRUE se a IA "pensou" em se equipar, para consumir o seu foco neste pulso. */
    return TRUE;
}

/**
 * A IA de gestão de inventário. O mob tenta organizar os seus itens,
 * guardando-os no melhor contentoir que possui.
 * Retorna TRUE se uma ação de organização foi realizada.
 */
bool mob_manage_inventory(struct char_data *ch)
{
    /* A IA só age se tiver genética e se o inventário estiver a ficar cheio. */
    if (!ch->ai_data || !ch->carrying || IS_CARRYING_N(ch) < (CAN_CARRY_N(ch) * 0.8))
        return FALSE;

    /* Check if mob should sacrifice corpses in the room first */
    if (rand_number(1, 100) <= 15) { /* 15% chance to check for corpses */
        if (mob_try_sacrifice(ch, NULL)) {
            return TRUE; /* Mob sacrificed something, that's enough for this pulse */
        }
    }

    /* Check if inventory is extremely full - may need to donate or junk items */
    bool inventory_overfull = (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) * 0.95);
    bool inventory_heavy = (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch) * 0.95);

    if (inventory_overfull || inventory_heavy) {
        /* Try to donate valuable items if mob has good alignment */
        if (GET_ALIGNMENT(ch) > 0 && rand_number(1, 100) <= 20) {
            if (mob_try_donate(ch, NULL)) {
                return TRUE;
            }
        }

        /* Try to junk worthless items if desperate */
        if (rand_number(1, 100) <= 30) {
            if (mob_try_junk(ch, NULL)) {
                return TRUE;
            }
        }

        /* Last resort - drop something */
        if (rand_number(1, 100) <= 40) {
            if (mob_try_drop(ch, NULL)) {
                return TRUE;
            }
        }
    }

    /* A "vontade" de se organizar é baseada no use_tendency. */
    if (rand_number(1, 100) > GET_GENUSE(ch)) /* GET_GENUSE será a nova macro */
        return FALSE;

    struct obj_data *obj, *container = NULL, *best_container = NULL;
    int max_capacity = 0;

    /* 1. Encontra o melhor contentor aberto no inventário. */
    for (container = ch->carrying; container; container = container->next_content) {
        if (GET_OBJ_TYPE(container) == ITEM_CONTAINER && !OBJVAL_FLAGGED(container, CONT_CLOSED)) {
            if (GET_OBJ_VAL(container, 0) > max_capacity) {
                max_capacity = GET_OBJ_VAL(container, 0);
                best_container = container;
            }
        }
    }

    /* Se não encontrou um contentor utilizável, não pode fazer nada. */
    if (!best_container)
        return FALSE;

    bool item_stored = FALSE;

    /* 2. Percorre o inventário novamente para guardar os itens. */
    struct obj_data *next_obj_to_store;
    for (obj = ch->carrying; obj; obj = next_obj_to_store) {
        next_obj_to_store = obj->next_content;

        /* Não guarda o próprio contentor, nem itens amaldiçoados, nem outros contentores. */
        if (obj == best_container || OBJ_FLAGGED(obj, ITEM_NODROP) || GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
            continue;

        /* Verifica se o item cabe no contentor (lógica de perform_put). */
        if ((GET_OBJ_WEIGHT(best_container) + GET_OBJ_WEIGHT(obj)) <= GET_OBJ_VAL(best_container, 0)) {
            perform_put(ch, obj, best_container);
            item_stored = TRUE;
        }
    }

    /* A IA aprende que organizar é útil. */
    if (item_stored && ch->ai_data) {
        ch->ai_data->genetics.use_tendency += 3;
        ch->ai_data->genetics.use_tendency = MIN(ch->ai_data->genetics.use_tendency, 100);
    }

    return item_stored;
}

/**
 * Procura em todo o inventário de um mob (incluindo contentores) pelo
 * melhor upgrade de equipamento possível.
 * @param ch O mob que está a procurar.
 * @return Uma estrutura mob_upgrade_plan com o plano para o melhor upgrade.
 */
struct mob_upgrade_plan find_best_upgrade_for_mob(struct char_data *ch)
{
    struct mob_upgrade_plan best_plan = {NULL, NULL, -1, 0};
    struct obj_data *item, *contained_item, *equipped_item;
    int wear_pos, score;

    /* Loop através de todos os itens (no inventário principal e dentro de contentores) */
    for (item = ch->carrying; item; item = item->next_content) {
        /* Primeiro, avalia o item atual do inventário principal. */
        wear_pos = find_eq_pos(ch, item, NULL);
        if (wear_pos != -1) {
            equipped_item = GET_EQ(ch, wear_pos);
            /* Verifica se a troca é possível antes de avaliar. */
            if (equipped_item == NULL ||
                (!OBJ_FLAGGED(equipped_item, ITEM_NODROP) && IS_CARRYING_N(ch) < CAN_CARRY_N(ch))) {
                score = evaluate_item_for_mob(ch, item) - evaluate_item_for_mob(ch, equipped_item);
                if (score > best_plan.improvement_score) {
                    best_plan.improvement_score = score;
                    best_plan.item_to_equip = item;
                    best_plan.wear_pos = wear_pos;
                    best_plan.container = NULL; /* Item está no inventário principal. */
                }
            }
        }

        /* Se o item for um contentor aberto, procura dentro dele. */
        if (GET_OBJ_TYPE(item) == ITEM_CONTAINER && !OBJVAL_FLAGGED(item, CONT_CLOSED)) {
            for (contained_item = item->contains; contained_item; contained_item = contained_item->next_content) {
                wear_pos = find_eq_pos(ch, contained_item, NULL);
                if (wear_pos != -1) {
                    equipped_item = GET_EQ(ch, wear_pos);
                    /* Ao tirar um item, o número de itens não aumenta, por isso a verificação de CAN_CARRY_N não é
                     * necessária aqui. */
                    if (equipped_item == NULL || !OBJ_FLAGGED(equipped_item, ITEM_NODROP)) {
                        score = evaluate_item_for_mob(ch, contained_item) - evaluate_item_for_mob(ch, equipped_item);
                        if (score > best_plan.improvement_score) {
                            best_plan.improvement_score = score;
                            best_plan.item_to_equip = contained_item;
                            best_plan.wear_pos = wear_pos;
                            best_plan.container = item; /* Lembra-se de onde o item está! */
                        }
                    }
                }
            }
        }
    }
    return best_plan;
}

/**
 * Procura no inventário de um personagem por um item amaldiçoado
 * (que tenha a flag ITEM_NODROP).
 * @param ch O personagem cujo inventário será verificado.
 * @return Um ponteiro para o primeiro item amaldiçoado encontrado, ou NULL se não houver nenhum.
 */
struct obj_data *find_cursed_item_in_inventory(struct char_data *ch)
{
    struct obj_data *obj;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
            return obj;
        }
    }
    return NULL;
}

/**
 * Procura no inventário de um personagem por uma arma ou armadura que ainda
 * não tenha sido abençoada (sem a flag ITEM_BLESS).
 * @param ch O personagem cujo inventário será verificado.
 * @return Um ponteiro para o primeiro item válido encontrado, ou NULL se não houver.
 */
struct obj_data *find_unblessed_weapon_or_armor(struct char_data *ch)
{
    struct obj_data *obj;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON || GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
            if (!OBJ_FLAGGED(obj, ITEM_BLESS)) {
                return obj;
            }
        }
    }
    return NULL;
}

/*
 * staff  - [0] level   [1] max charges [2] num charges [3] spell num
 * wand   - [0] level   [1] max charges [2] num charges [3] spell num        * scroll - [0] level   [1] spell num   [2]
 * spell num   [3] spell num        * potion - [0] level   [1] spell num   [2] spell num   [3] spell num        * Staves
 * and wands will default to level 14 if the level is not specified;*/

/**
 * Retorna todos os números de magia contidos em um item mágico.
 * @param obj O objeto a ser verificado.
 * @param spells Array para armazenar os números das magias (deve ter pelo menos 3 elementos).
 * @return O número de magias válidas encontradas, ou 0 se o item não for mágico.
 */
int get_all_spells_from_item(struct obj_data *obj, int *spells)
{
    if (!obj || !spells)
        return 0;

    int count = 0;

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WAND:
        case ITEM_STAFF:
            // Wands and staves only have one spell in slot 3
            if (GET_OBJ_VAL(obj, 3) > 0) {
                spells[count++] = GET_OBJ_VAL(obj, 3);
            }
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
            // Scrolls and potions can have spells in slots 1, 2, and 3
            for (int slot = 1; slot <= 3; slot++) {
                if (GET_OBJ_VAL(obj, slot) > 0) {
                    spells[count++] = GET_OBJ_VAL(obj, slot);
                }
            }
            break;

        default:
            break;
    }

    return count;
}

/**
 * Retorna o número da magia contida em um item mágico (wand, staff, scroll, potion).
 * Para scrolls e potions, retorna a primeira magia válida encontrada.
 * @param obj O objeto a ser verificado.
 * @return O número da magia, ou -1 se o item não for mágico.
 */
int get_spell_from_item(struct obj_data *obj)
{
    if (!obj)
        return -1;

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WAND:
        case ITEM_STAFF:
            return GET_OBJ_VAL(obj, 3);

        case ITEM_SCROLL:
        case ITEM_POTION:
            // For scrolls and potions, return the first valid spell found
            for (int slot = 1; slot <= 3; slot++) {
                if (GET_OBJ_VAL(obj, slot) > 0) {
                    return GET_OBJ_VAL(obj, slot);
                }
            }
            return -1;

        default:
            return -1;
    }
}

/**
 * A IA principal para um mob decidir, usar e aprender com itens.
 * VERSÃO FINAL COM LÓGICA DE SUPORTE E PREPARAÇÃO DE ITENS.
 * Retorna TRUE se uma ação foi executada.
 */
bool mob_handle_item_usage(struct char_data *ch)
{
    if (!ch->carrying || !ch->ai_data)
        return FALSE;
    if (rand_number(1, 100) > MAX(GET_GENUSE(ch), 15))
        return FALSE;

    struct obj_data *obj, *item_to_use = NULL, *target_obj = NULL, *best_target_obj = NULL;
    struct char_data *target_char = NULL, *best_target_char = NULL;
    int best_score = 0;
    int spellnum_to_cast = -1;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        int spells[3];   // Maximum 3 spells for scrolls/potions
        int spell_count = get_all_spells_from_item(obj, spells);

        if (spell_count <= 0)
            continue;

        // Evaluate each spell in the item
        for (int spell_idx = 0; spell_idx < spell_count; spell_idx++) {
            int current_score = 0;
            target_char = NULL;   // Reset targets for each iteration
            target_obj = NULL;
            int skillnum = spells[spell_idx];

            struct str_spells *spell = get_spell_by_vnum(skillnum);
            if (!spell)
                continue;

            /* --- Início da Árvore de Decisão Tática Expandida --- */
            if (FIGHTING(ch)) {
                /* ** MODO DE COMBATE ** */

                // PRIORIDADE 1: EMERGÊNCIA - Cura crítica
                if (IS_SET(spell->mag_flags, MAG_POINTS) && GET_HIT(ch) < (GET_MAX_HIT(ch) * 0.3)) {
                    current_score = 200;
                    target_char = ch;
                }
                // PRIORIDADE 2: Cura moderada
                else if (IS_SET(spell->mag_flags, MAG_POINTS) && GET_HIT(ch) < (GET_MAX_HIT(ch) * 0.7)) {
                    current_score = 150;
                    target_char = ch;
                }

                // PRIORIDADE 3: Proteções específicas de alinhamento
                else if (skillnum == SPELL_PROT_FROM_EVIL && IS_GOOD(ch) && IS_EVIL(FIGHTING(ch)) &&
                         !IS_AFFECTED(ch, AFF_PROTECT_EVIL)) {
                    current_score = 140;
                    target_char = ch;
                } else if (skillnum == SPELL_PROT_FROM_GOOD && IS_EVIL(ch) && IS_GOOD(FIGHTING(ch)) &&
                           !IS_AFFECTED(ch, AFF_PROTECT_GOOD)) {
                    current_score = 140;
                    target_char = ch;
                }

                // PRIORIDADE 4: Proteções defensivas supremas
                else if (skillnum == SPELL_SANCTUARY && !IS_AFFECTED(ch, AFF_SANCTUARY)) {
                    current_score = 130;
                    target_char = ch;
                } else if (skillnum == SPELL_GLOOMSHIELD && !IS_AFFECTED(ch, AFF_GLOOMSHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_STONESKIN && !IS_AFFECTED(ch, AFF_STONESKIN)) {
                    current_score = 120;
                    target_char = ch;
                }

                // PRIORIDADE 5: Escudos de dano
                else if (skillnum == SPELL_FIRESHIELD && !IS_AFFECTED(ch, AFF_FIRESHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_THISTLECOAT && !IS_AFFECTED(ch, AFF_THISTLECOAT)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_WINDWALL && !IS_AFFECTED(ch, AFF_WINDWALL)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_SOUNDBARRIER && !IS_AFFECTED(ch, AFF_SOUNDBARRIER)) {
                    current_score = 110;
                    target_char = ch;
                } else if (skillnum == SPELL_WATERSHIELD && !IS_AFFECTED(ch, AFF_WATERSHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_ROCKSHIELD && !IS_AFFECTED(ch, AFF_ROCKSHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_POISONSHIELD && !IS_AFFECTED(ch, AFF_POISONSHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_LIGHTNINGSHIELD && !IS_AFFECTED(ch, AFF_LIGHTNINGSHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_ICESHIELD && !IS_AFFECTED(ch, AFF_ICESHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_ACIDSHIELD && !IS_AFFECTED(ch, AFF_ACIDSHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_MINDSHIELD && !IS_AFFECTED(ch, AFF_MINDSHIELD)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_FORCESHIELD && !IS_AFFECTED(ch, AFF_FORCESHIELD)) {
                    current_score = 115;
                    target_char = ch;
                }

                // PRIORIDADE 6: Buffs de combate
                else if (skillnum == SPELL_STRENGTH && !IS_AFFECTED(ch, skillnum)) {
                    current_score = 105;
                    target_char = ch;
                } else if (skillnum == SPELL_ARMOR && !IS_AFFECTED(ch, skillnum)) {
                    current_score = 100;
                    target_char = ch;
                }

                // PRIORIDADE 7: Debuffs táticos inimigos
                else if (skillnum == SPELL_BLINDNESS && !IS_AFFECTED(FIGHTING(ch), AFF_BLIND)) {
                    current_score = 95;
                    target_char = FIGHTING(ch);
                } else if (skillnum == SPELL_SLEEP && !IS_AFFECTED(FIGHTING(ch), AFF_SLEEP)) {
                    current_score = 95;
                    target_char = FIGHTING(ch);
                } else if (skillnum == SPELL_PARALYSE && !IS_AFFECTED(FIGHTING(ch), AFF_PARALIZE)) {
                    current_score = 95;
                    target_char = FIGHTING(ch);
                } else if (skillnum == SPELL_CURSE && !IS_AFFECTED(FIGHTING(ch), AFF_CURSE)) {
                    current_score = 90;
                    target_char = FIGHTING(ch);
                } else if (skillnum == SPELL_POISON && !IS_AFFECTED(FIGHTING(ch), AFF_POISON)) {
                    current_score = 85;
                    target_char = FIGHTING(ch);
                }

                // PRIORIDADE 8: Dano direto
                else if (spell->mag_flags & MAG_DAMAGE) {
                    // Danos mais altos para magias mais poderosas
                    if (skillnum == SPELL_DISINTEGRATE || skillnum == SPELL_FIREBALL ||
                        skillnum == SPELL_LIGHTNING_BOLT) {
                        current_score = 80;
                        target_char = FIGHTING(ch);
                    } else if (skillnum == SPELL_DISPEL_EVIL || skillnum == SPELL_DISPEL_GOOD ||
                               skillnum == SPELL_HARM) {
                        current_score = 75;
                        target_char = FIGHTING(ch);
                    } else {
                        current_score = 70;
                        target_char = FIGHTING(ch);
                    }
                }

            } else {
                /* ** MODO DE PREPARAÇÃO (Fora de Combate) ** */

                // PRIORIDADE 1: Remover problemas sérios
                if (IS_AFFECTED(ch, AFF_CURSE) && skillnum == SPELL_REMOVE_CURSE) {
                    current_score = 180;
                    target_char = ch;
                } else if (IS_AFFECTED(ch, AFF_POISON) && skillnum == SPELL_REMOVE_POISON) {
                    current_score = 170;
                    target_char = ch;
                } else if (IS_AFFECTED(ch, AFF_BLIND) && skillnum == SPELL_CURE_BLIND) {
                    current_score = 160;
                    target_char = ch;
                }

                // PRIORIDADE 2: Limpar itens amaldiçoados
                else if (IS_SET(spell->mag_flags, MAG_UNAFFECTS) && skillnum == SPELL_REMOVE_CURSE) {
                    struct obj_data *cursed_item = find_cursed_item_in_inventory(ch);
                    if (cursed_item) {
                        current_score = 150;
                        target_obj = cursed_item;
                    }
                }

                // PRIORIDADE 3: Proteções defensivas supremas
                else if (skillnum == SPELL_SANCTUARY && !IS_AFFECTED(ch, AFF_SANCTUARY)) {
                    current_score = 140;
                    target_char = ch;
                } else if (skillnum == SPELL_STONESKIN && !IS_AFFECTED(ch, AFF_STONESKIN)) {
                    current_score = 135;
                    target_char = ch;
                } else if (skillnum == SPELL_GLOOMSHIELD && !IS_AFFECTED(ch, AFF_GLOOMSHIELD)) {
                    current_score = 130;
                    target_char = ch;
                }

                // PRIORIDADE 4: Escudos de dano e proteções
                else if (skillnum == SPELL_FIRESHIELD && !IS_AFFECTED(ch, AFF_FIRESHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_THISTLECOAT && !IS_AFFECTED(ch, AFF_THISTLECOAT)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_WINDWALL && !IS_AFFECTED(ch, AFF_WINDWALL)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_SOUNDBARRIER && !IS_AFFECTED(ch, AFF_SOUNDBARRIER)) {
                    current_score = 120;
                    target_char = ch;
                } else if (skillnum == SPELL_WATERSHIELD && !IS_AFFECTED(ch, AFF_WATERSHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_ROCKSHIELD && !IS_AFFECTED(ch, AFF_ROCKSHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_POISONSHIELD && !IS_AFFECTED(ch, AFF_POISONSHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_LIGHTNINGSHIELD && !IS_AFFECTED(ch, AFF_LIGHTNINGSHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_ICESHIELD && !IS_AFFECTED(ch, AFF_ICESHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_ACIDSHIELD && !IS_AFFECTED(ch, AFF_ACIDSHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_MINDSHIELD && !IS_AFFECTED(ch, AFF_MINDSHIELD)) {
                    current_score = 125;
                    target_char = ch;
                } else if (skillnum == SPELL_FORCESHIELD && !IS_AFFECTED(ch, AFF_FORCESHIELD)) {
                    current_score = 125;
                    target_char = ch;
                }

                // PRIORIDADE 5: Proteções de alinhamento
                else if (skillnum == SPELL_PROT_FROM_EVIL && IS_GOOD(ch) && !IS_AFFECTED(ch, AFF_PROTECT_EVIL)) {
                    current_score = 115;
                    target_char = ch;
                } else if (skillnum == SPELL_PROT_FROM_GOOD && IS_EVIL(ch) && !IS_AFFECTED(ch, AFF_PROTECT_GOOD)) {
                    current_score = 115;
                    target_char = ch;
                }

                // PRIORIDADE 6: Melhoramento de itens
                else if (IS_SET(spell->mag_flags, MAG_MANUAL) && (skillnum == SPELL_BLESS_OBJECT) && IS_GOOD(ch)) {
                    struct obj_data *item_to_buff = find_unblessed_weapon_or_armor(ch);
                    if (item_to_buff) {
                        current_score = 110;
                        target_obj = item_to_buff;
                    }
                } else if (skillnum == SPELL_ENCHANT_WEAPON) {
                    // Procura arma não encantada
                    struct obj_data *weapon = GET_EQ(ch, WEAR_WIELD);
                    if (weapon && GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {
                        current_score = 105;
                        target_obj = weapon;
                    }
                }

                // PRIORIDADE 7: Buffs físicos
                else if (skillnum == SPELL_STRENGTH && !IS_AFFECTED(ch, skillnum)) {
                    current_score = 100;
                    target_char = ch;
                } else if (skillnum == SPELL_ARMOR && !IS_AFFECTED(ch, skillnum)) {
                    current_score = 95;
                    target_char = ch;
                }

                // PRIORIDADE 8: Habilidades de detecção
                else if (skillnum == SPELL_DETECT_INVIS && !IS_AFFECTED(ch, AFF_DETECT_INVIS)) {
                    current_score = 90;
                    target_char = ch;
                } else if (skillnum == SPELL_DETECT_MAGIC && !IS_AFFECTED(ch, AFF_DETECT_MAGIC)) {
                    current_score = 85;
                    target_char = ch;
                } else if (skillnum == SPELL_DETECT_ALIGN && !IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
                    current_score = 85;
                    target_char = ch;
                } else if (skillnum == SPELL_SENSE_LIFE && !IS_AFFECTED(ch, AFF_SENSE_LIFE)) {
                    current_score = 80;
                    target_char = ch;
                } else if (skillnum == SPELL_INFRAVISION && !IS_AFFECTED(ch, AFF_INFRAVISION)) {
                    current_score = 75;
                    target_char = ch;
                }

                // PRIORIDADE 9: Habilidades de movimento/utilidade
                else if (skillnum == SPELL_FLY && !IS_AFFECTED(ch, AFF_FLYING)) {
                    current_score = 70;
                    target_char = ch;
                } else if (skillnum == SPELL_WATERWALK && !IS_AFFECTED(ch, AFF_WATERWALK)) {
                    current_score = 65;
                    target_char = ch;
                } else if (skillnum == SPELL_BREATH && !IS_AFFECTED(ch, AFF_BREATH)) {
                    current_score = 60;
                    target_char = ch;
                }

                // PRIORIDADE 10: Habilidades furtivas/táticas
                else if (skillnum == SPELL_INVISIBLE && !IS_AFFECTED(ch, AFF_INVISIBLE)) {
                    current_score = 55;
                    target_char = ch;
                }

                // PRIORIDADE 11: Buffs genéricos restantes
                else if (IS_SET(spell->mag_flags, MAG_AFFECTS) && IS_SET(spell->targ_flags, TAR_SELF_ONLY)) {
                    // Verifica qualquer magia de buff defensivo genérico
                    if (!IS_AFFECTED(ch, skillnum)) {
                        current_score = 50;
                        target_char = ch;
                    }
                }
            }

            if (current_score > best_score) {
                best_score = current_score;
                item_to_use = obj;
                spellnum_to_cast = skillnum;
                // Store the correct target
                best_target_char = target_char;
                best_target_obj = target_obj;
            }
        }   // End of spell evaluation loop
    }

    if (item_to_use) {
        if (is_last_consumable(ch, item_to_use)) {
            /* Bloco comentado: Se fosse o ultimo e a tendencia caisse, tendencia cai pra 0
            ch->ai_data->genetics.use_tendency = MAX(ch->ai_data->genetics.use_tendency - 1, 0);
            */
            return FALSE;
        }

        if (cast_spell(ch, best_target_char, best_target_obj, spellnum_to_cast)) {
            ch->ai_data->genetics.use_tendency += 2;
        } else {
            ch->ai_data->genetics.use_tendency -= 2;
        }
        ch->ai_data->genetics.use_tendency = MIN(MAX(ch->ai_data->genetics.use_tendency, 0), 100);
        return TRUE;
    }
    return FALSE;
}

/**
 * Encontra um mob específico em uma sala pelo seu número real (rnum).
 * @param room The room number to search in
 * @param rnum The mob rnum to find
 * @return Pointer to the mob if found, NULL otherwise
 */
struct char_data *get_mob_in_room_by_rnum(room_rnum room, mob_rnum rnum)
{
    struct char_data *i;

    /* Safety check: Validate room before accessing world array */
    if (room == NOWHERE || room < 0 || room > top_of_world) {
        return NULL;
    }

    for (i = world[room].people; i; i = i->next_in_room) {
        if (IS_NPC(i) && GET_MOB_RNUM(i) == rnum) {
            return i;
        }
    }
    return NULL;
}

/**
 * Find a mob in a room by its virtual number (vnum).
 * @param room The room number to search in
 * @param vnum The virtual number of the mob to find
 * @return Pointer to the mob if found, NULL otherwise
 */
struct char_data *get_mob_in_room_by_vnum(room_rnum room, mob_vnum vnum)
{
    struct char_data *i;

    /* Safety check: Validate room before accessing world array */
    if (room == NOWHERE || room < 0 || room > top_of_world) {
        return NULL;
    }

    for (i = world[room].people; i; i = i->next_in_room) {
        if (IS_NPC(i) && GET_MOB_VNUM(i) == vnum) {
            return i;
        }
    }
    return NULL;
}

/**
 * Find a questmaster mob anywhere in the world by its virtual number.
 * @param vnum The virtual number of the questmaster to find
 * @return Pointer to the questmaster if found, NULL otherwise
 */
struct char_data *find_questmaster_by_vnum(mob_vnum vnum)
{
    struct char_data *i, *next_i;

    /* Search through all characters in the world */
    for (i = character_list; i; i = next_i) {
        next_i = i->next;

        /* Safety check: Skip characters marked for extraction */
        if (MOB_FLAGGED(i, MOB_NOTDEADYET) || PLR_FLAGGED(i, PLR_NOTDEADYET))
            continue;

        /* Safety check: Validate room before using the character */
        if (!IS_NPC(i) || IN_ROOM(i) == NOWHERE || IN_ROOM(i) < 0 || IN_ROOM(i) > top_of_world)
            continue;

        if (GET_MOB_VNUM(i) == vnum) {
            /* Check if this mob is a questmaster (has quest special procedure) */
            if (mob_index[GET_MOB_RNUM(i)].func == questmaster || mob_index[GET_MOB_RNUM(i)].func == temp_questmaster) {
                return i;
            }
        }
    }
    return NULL;
}

/**
 * Find any mob in the world by vnum (not just questmasters).
 * Note: This performs O(n) search through character_list. Called during quest
 * processing which is throttled, so performance impact is acceptable.
 * @param vnum The mob vnum to find.
 * @return Pointer to the mob if found, NULL otherwise.
 */
struct char_data *find_mob_by_vnum(mob_vnum vnum)
{
    struct char_data *i, *next_i;

    /* Search through all characters in the world */
    for (i = character_list; i; i = next_i) {
        next_i = i->next;

        /* Safety check: Skip characters marked for extraction */
        if (MOB_FLAGGED(i, MOB_NOTDEADYET) || PLR_FLAGGED(i, PLR_NOTDEADYET))
            continue;

        /* Safety check: Validate room before using the character */
        if (!IS_NPC(i) || IN_ROOM(i) == NOWHERE || IN_ROOM(i) < 0 || IN_ROOM(i) > top_of_world)
            continue;

        if (GET_MOB_VNUM(i) == vnum) {
            return i;
        }
    }
    return NULL;
}

/**
 * Find the location of an object in the world (on ground, in open container, or carried by a mob).
 * Note: This performs O(rooms + mobs) search through the world. Called during quest
 * processing which is throttled, so performance impact is acceptable.
 * Only searches open containers - mobs cannot see into closed containers.
 * @param obj_vnum The object vnum to find.
 * @return Room number where the object can be found, or NOWHERE if not found.
 */
room_rnum find_object_location(obj_vnum obj_vnum)
{
    struct obj_data *obj;
    struct char_data *mob;
    room_rnum room;

    /* First, check if object is on the ground in any room */
    for (room = 0; room <= top_of_world; room++) {
        for (obj = world[room].contents; obj; obj = obj->next_content) {
            if (GET_OBJ_VNUM(obj) == obj_vnum) {
                return room;
            }
            /* Check inside open containers on the ground (mobs can't see inside closed ones) */
            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
                struct obj_data *contained;
                for (contained = obj->contains; contained; contained = contained->next_content) {
                    if (GET_OBJ_VNUM(contained) == obj_vnum) {
                        return room;
                    }
                }
            }
        }
    }

    /* Then, check if any mob is carrying the object */
    for (mob = character_list; mob; mob = mob->next) {
        if (!IS_NPC(mob))
            continue;
        if (MOB_FLAGGED(mob, MOB_NOTDEADYET) || PLR_FLAGGED(mob, PLR_NOTDEADYET))
            continue;
        if (IN_ROOM(mob) == NOWHERE || IN_ROOM(mob) < 0 || IN_ROOM(mob) > top_of_world)
            continue;

        for (obj = mob->carrying; obj; obj = obj->next_content) {
            if (GET_OBJ_VNUM(obj) == obj_vnum) {
                return IN_ROOM(mob);
            }
        }
    }

    return NOWHERE; /* Object not found */
}

/**
 * A IA de economia. O mob tenta vender os seus itens de menor valor.
 * VERSÃO FINAL COM CORREÇÃO DE BUGS.
 * Retorna TRUE se o mob definiu um objetivo ou executou uma venda.
 */
bool mob_try_to_sell_junk(struct char_data *ch)
{

    /* 1. GATILHO: A IA só age se tiver genética e se o inventário estiver > 80% cheio. */
    /* Charmed mobs should not autonomously sell items. */
    bool inventory_full = (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) * 0.8);
    bool inventory_heavy = (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch) * 0.8);

    if (!ch->ai_data || AFF_FLAGGED(ch, AFF_CHARM) || !ch->carrying || (!inventory_full && !inventory_heavy))
        return FALSE;

    if (rand_number(1, 100) > MAX(GET_GENTRADE(ch), 5))
        return FALSE;

    struct obj_data *obj, *item_to_sell = NULL;
    int min_score = 10;

    /* 2. ANÁLISE DE INVENTÁRIO: Encontra o pior item para vender. */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (OBJ_FLAGGED(obj, ITEM_NODROP) || (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains))
            continue;

        int score = evaluate_item_for_mob(ch, obj);
        if (score < min_score) {
            min_score = score;
            item_to_sell = obj;
        }
    }

    if (!item_to_sell)
        return FALSE; /* Não tem "lixo" para vender. */

    /* 3. ANÁLISE DE MERCADO: Encontra a melhor loja para vender o item. */
    /* CORREÇÃO: Usamos 'int' para podermos verificar o -1. */
    int best_shop_rnum = find_best_shop_to_sell(ch, item_to_sell);

    /* If the best shop is not reachable, try to find an alternative */
    if (best_shop_rnum == -1 || best_shop_rnum < 0 || best_shop_rnum > top_shop ||
        !shop_index[best_shop_rnum].in_room || SHOP_ROOM(best_shop_rnum, 0) == NOWHERE) {
        return FALSE; /* No shops available */
    }

    room_rnum target_shop_room = real_room(SHOP_ROOM(best_shop_rnum, 0));

    /* Check if we can reach the shop, if not try to find an alternative */
    if (target_shop_room == NOWHERE || find_first_step(IN_ROOM(ch), target_shop_room) == -1) {
        /* Try to find a second-best shop that is reachable */
        best_shop_rnum = -1;
        float best_profit = 0.0;

        for (int snum = 0; snum <= top_shop; snum++) {
            if (!is_shop_open(snum))
                continue;

            /* Validate shop index entry is properly initialized */
            if (!shop_index[snum].type || !shop_index[snum].in_room) {
                continue;
            }

            /* Check if shop buys this type of item */
            bool buys_this_type = FALSE;
            for (int i = 0; i < MAX_TRADE && SHOP_BUYTYPE(snum, i) != NOTHING; i++) {
                if (SHOP_BUYTYPE(snum, i) == GET_OBJ_TYPE(item_to_sell)) {
                    buys_this_type = TRUE;
                    break;
                }
            }
            if (!buys_this_type)
                continue;

            room_rnum shop_location = real_room(SHOP_ROOM(snum, 0));
            if (shop_location == NOWHERE)
                continue;

            /* This time, prioritize reachability over profit */
            if (find_first_step(IN_ROOM(ch), shop_location) != -1) {
                float current_profit = GET_OBJ_COST(item_to_sell) * SHOP_BUYPROFIT(snum);
                if (current_profit > best_profit) {
                    best_profit = current_profit;
                    best_shop_rnum = snum;
                }
            }
        }

        if (best_shop_rnum == -1) {
            return FALSE; /* No reachable shops found */
        }

        /* Validate shop before accessing its data */
        if (!shop_index[best_shop_rnum].in_room) {
            return FALSE; /* Shop data is corrupted */
        }

        target_shop_room = real_room(SHOP_ROOM(best_shop_rnum, 0));
    }

    if (best_shop_rnum >= 0 && best_shop_rnum <= top_shop && target_shop_room != NOWHERE) {
        /* Se ainda não está na loja, o objetivo é ir para lá. */
        act("$n olha para a sua mochila e parece estar a planejar uma viagem.", FALSE, ch, 0, 0, TO_ROOM);
        ch->ai_data->current_goal = GOAL_GOTO_SHOP_TO_SELL;
        ch->ai_data->goal_destination = target_shop_room;
        ch->ai_data->goal_obj = item_to_sell;
        ch->ai_data->goal_target_mob_rnum = SHOP_KEEPER(best_shop_rnum);
        ch->ai_data->goal_timer = 0; /* Reset timer for new goal */

        mob_goal_oriented_roam(ch, target_shop_room);
        return TRUE;
    }

    return FALSE;
}

/*
 * ===============================================
 * WISHLIST GOAL PROCESSING FOR MOB AI
 * ===============================================
 */

/**
 * Makes a mob occasionally decide to seek out and accept quests.
 * Called when a mob has no current goal and not already on a quest.
 * Performance optimizations:
 * - Early exits for common rejection cases
 * - Frustration timer check before expensive calculations
 * - Room validation before zone lookups
 * @param ch The mob that might accept a quest
 * @return TRUE if the mob decides to pursue quest acceptance, FALSE otherwise
 */
bool mob_try_to_accept_quest(struct char_data *ch)
{
    struct char_data *questmaster;
    zone_rnum mob_zone;
    int acceptance_threshold;

    /* Quick early exits - cheapest checks first */
    if (!IS_NPC(ch) || !ch->ai_data)
        return FALSE;

    /* Already has a goal or quest */
    if (ch->ai_data->current_goal != GOAL_NONE || GET_MOB_QUEST(ch) != NOTHING)
        return FALSE;

    /* Check if frustrated from recent quest activities (cheap check) */
    if (ch->ai_data->quest_posting_frustration_timer > 0)
        return FALSE;

    /* Safety check: Validate room before any world array access */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return FALSE;

    /* Calculate acceptance threshold based on mob's genetics and emotions */
    acceptance_threshold = 20;                    /* Base 20% chance */
    acceptance_threshold += GET_GENQUEST(ch) / 5; /* Up to +20% from quest_tendency */

    /* Further increase if mob has high curiosity (emotion system) */
    if (CONFIG_MOB_CONTEXTUAL_SOCIALS && ch->ai_data->emotion_curiosity >= 50) {
        acceptance_threshold += 10; /* +10% for curious mobs */
    }

    /* Cap at reasonable maximum to avoid too aggressive quest-taking */
    acceptance_threshold = MIN(acceptance_threshold, 60);

    /* Random check - do this BEFORE expensive zone/questmaster lookups */
    if (rand() % 100 > acceptance_threshold)
        return FALSE;

    /* Early exit: Don't accept new quests when near limit to reduce server load
     * When close to max quests, the expensive pathfinding in
     * find_accessible_questmaster_in_zone() causes severe lag.
     * Accepting quests at 90% capacity still allows new quests while preventing
     * the catastrophic performance hit of too many concurrent searches. */
    if (count_mob_posted_quests() >= (CONFIG_MAX_MOB_POSTED_QUESTS * 9 / 10)) { /* 90% of max */
        return FALSE;
    }

    /* Now do the expensive operations - look for accessible questmasters */
    mob_zone = world[IN_ROOM(ch)].zone;
    questmaster = find_accessible_questmaster_in_zone(ch, mob_zone);

    if (questmaster && questmaster != ch) {
        /* Find first quest available for mob's level range */
        qst_vnum available_quest = find_mob_available_quest_by_qmnum(ch, GET_MOB_VNUM(questmaster));
        if (available_quest != NOTHING) {
            qst_rnum quest_rnum = real_quest(available_quest);
            if (quest_rnum != NOTHING && mob_should_accept_quest(ch, quest_rnum)) {
                /* Set goal to go to questmaster and accept quest */
                ch->ai_data->current_goal = GOAL_ACCEPT_QUEST;
                ch->ai_data->goal_destination = IN_ROOM(questmaster);
                ch->ai_data->goal_target_mob_rnum = GET_MOB_RNUM(questmaster);
                ch->ai_data->goal_timer = 0;
                act("$n parece estar à procura de trabalho.", FALSE, ch, 0, 0, TO_ROOM);
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 * Process quest completion for a mob with an active quest.
 * Handles different quest types and implements quest completion logic.
 * @param ch The mob attempting to complete a quest
 * @param quest_rnum The real number of the quest being completed
 * @return TRUE if quest completion was attempted (even if not successful), FALSE if quest could not be processed
 */
bool mob_process_quest_completion(struct char_data *ch, qst_rnum quest_rnum)
{
    struct char_data *target_mob, *questmaster;
    struct obj_data *target_obj;
    room_rnum target_room;
    int quest_type = QST_TYPE(quest_rnum);

    if (!IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* Handle quest completion based on quest type */
    switch (quest_type) {
        case AQ_OBJ_FIND:
            /* Check if mob has the required object */
            target_obj = get_obj_in_list_num(QST_TARGET(quest_rnum), ch->carrying);
            if (target_obj) {
                /* Object found, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else {
                /* Object not found, try to find where it is and go there */
                target_room = find_object_location(QST_TARGET(quest_rnum));
                if (target_room != NOWHERE && target_room != IN_ROOM(ch)) {
                    /* Found object, move towards it */
                    ch->ai_data->goal_destination = target_room;
                    mob_goal_oriented_roam(ch, target_room);
                    /* Safety check: mob_goal_oriented_roam can trigger death traps */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                } else {
                    /* Object not found in world, add to wishlist and let wishlist system handle it */
                    add_item_to_wishlist(ch, QST_TARGET(quest_rnum), 100); /* High priority */
                    /* Continue with GOAL_COMPLETE_QUEST but roam to search */
                    mob_goal_oriented_roam(ch, NOWHERE);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                }
                return TRUE;
            }
            break;

        case AQ_ROOM_FIND:
            /* Check if mob is in the target room */
            target_room = real_room(QST_TARGET(quest_rnum));
            if (target_room != NOWHERE && IN_ROOM(ch) == target_room) {
                /* Target room reached, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else if (target_room != NOWHERE) {
                /* Move toward target room */
                ch->ai_data->goal_destination = target_room;
                mob_goal_oriented_roam(ch, target_room);
                return TRUE;
            }
            break;

        case AQ_MOB_FIND:
            /* Check if target mob is in current room */
            target_mob = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_TARGET(quest_rnum));
            if (target_mob) {
                /* Target mob found, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else {
                /* Target mob not found in current room, try to find it in the world */
                target_mob = find_mob_by_vnum(QST_TARGET(quest_rnum));
                if (target_mob && IN_ROOM(target_mob) != NOWHERE) {
                    /* Found target mob, move towards its location */
                    ch->ai_data->goal_destination = IN_ROOM(target_mob);
                    mob_goal_oriented_roam(ch, IN_ROOM(target_mob));
                    /* Safety check: mob_goal_oriented_roam can trigger death traps */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                } else {
                    /* Target mob not found in world, roam randomly to search */
                    mob_goal_oriented_roam(ch, NOWHERE);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                }
            }
            return TRUE;
            break;

        case AQ_MOB_KILL:
        case AQ_MOB_KILL_BOUNTY:
            /* Check if target mob is in current room and attack it */
            target_mob = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_TARGET(quest_rnum));
            if (target_mob && !FIGHTING(ch)) {
                /* Attack the target mob */
                act("$n olha para $N com determinação.", FALSE, ch, 0, target_mob, TO_ROOM);
                /* Safety check: act() can trigger DG scripts which may cause extraction */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    return TRUE;
                if (MOB_FLAGGED(target_mob, MOB_NOTDEADYET) || PLR_FLAGGED(target_mob, PLR_NOTDEADYET))
                    return TRUE;
                hit(ch, target_mob, TYPE_UNDEFINED);
                /* Safety check: hit() can indirectly cause extract_char */
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    return TRUE;
                return TRUE;
            } else if (!target_mob) {
                /* Target mob not found in current room, try to find it in the world */
                target_mob = find_mob_by_vnum(QST_TARGET(quest_rnum));
                if (target_mob && IN_ROOM(target_mob) != NOWHERE) {
                    /* Found target mob, move towards its location */
                    ch->ai_data->goal_destination = IN_ROOM(target_mob);
                    mob_goal_oriented_roam(ch, IN_ROOM(target_mob));
                    /* Safety check: mob_goal_oriented_roam can trigger death traps */
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                } else {
                    /* Target mob not found in world, roam randomly to search */
                    mob_goal_oriented_roam(ch, NOWHERE);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                }
            }
            return TRUE;
            break;

        case AQ_OBJ_RETURN:
            /* Check if mob has the required object to return */
            target_obj = get_obj_in_list_num(QST_TARGET(quest_rnum), ch->carrying);
            if (target_obj) {
                /* Has object, find questmaster to return it to */
                questmaster = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_RETURNMOB(quest_rnum));
                if (!questmaster) {
                    questmaster = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_MASTER(quest_rnum));
                }

                if (questmaster) {
                    /* Return object to questmaster */
                    act("$n entrega $p para quem solicitou.", FALSE, ch, target_obj, NULL, TO_ROOM);
                    obj_from_char(target_obj);
                    extract_obj(target_obj);
                    mob_complete_quest(ch);
                    ch->ai_data->current_goal = GOAL_NONE;
                    return TRUE;
                } else {
                    /* Need to find questmaster - go to questmaster location */
                    questmaster = find_questmaster_by_vnum(QST_RETURNMOB(quest_rnum));
                    if (!questmaster) {
                        questmaster = find_questmaster_by_vnum(QST_MASTER(quest_rnum));
                    }

                    if (questmaster) {
                        ch->ai_data->goal_destination = IN_ROOM(questmaster);
                        mob_goal_oriented_roam(ch, IN_ROOM(questmaster));
                        return TRUE;
                    }
                }
            } else {
                /* Don't have object, try to find where it is and go there */
                target_room = find_object_location(QST_TARGET(quest_rnum));
                if (target_room != NOWHERE && target_room != IN_ROOM(ch)) {
                    /* Found object, move towards it */
                    ch->ai_data->goal_destination = target_room;
                    mob_goal_oriented_roam(ch, target_room);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                } else {
                    /* Object not found in world, add to wishlist and roam to search */
                    add_item_to_wishlist(ch, QST_TARGET(quest_rnum), 100);
                    mob_goal_oriented_roam(ch, NOWHERE);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                }
                return TRUE;
            }
            break;

        case AQ_ROOM_CLEAR:
            /* Kill all mobs in target room */
            target_room = real_room(QST_TARGET(quest_rnum));
            if (target_room != NOWHERE) {
                if (IN_ROOM(ch) == target_room) {
                    /* In target room, check for hostile mobs */
                    struct char_data *temp_mob;
                    bool found_hostile = FALSE;

                    for (temp_mob = world[target_room].people; temp_mob; temp_mob = temp_mob->next_in_room) {
                        if (IS_NPC(temp_mob) && temp_mob != ch && !AFF_FLAGGED(temp_mob, AFF_CHARM)) {
                            /* Found a mob to kill */
                            if (!FIGHTING(ch)) {
                                act("$n ataca $N para limpar a área.", FALSE, ch, 0, temp_mob, TO_ROOM);
                                /* Safety check: act() can trigger DG scripts which may cause extraction */
                                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                    return TRUE;
                                if (MOB_FLAGGED(temp_mob, MOB_NOTDEADYET) || PLR_FLAGGED(temp_mob, PLR_NOTDEADYET)) {
                                    found_hostile = FALSE; /* Target was extracted, continue searching */
                                    continue;
                                }
                                hit(ch, temp_mob, TYPE_UNDEFINED);
                                /* Safety check: hit() can indirectly cause extract_char */
                                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                                    return TRUE;
                            }
                            found_hostile = TRUE;
                            break;
                        }
                    }

                    if (!found_hostile) {
                        /* Room cleared, complete quest */
                        mob_complete_quest(ch);
                        ch->ai_data->current_goal = GOAL_NONE;
                        return TRUE;
                    }
                } else {
                    /* Move toward target room */
                    ch->ai_data->goal_destination = target_room;
                    mob_goal_oriented_roam(ch, target_room);
                    return TRUE;
                }
            }
            break;

        case AQ_SHOP_BUY:
            /* Mob needs to buy specific item from a shop */
            target_obj = get_obj_in_list_num(QST_TARGET(quest_rnum), ch->carrying);
            if (target_obj) {
                /* Already has the item, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else {
                /* Need to buy - add to wishlist and set goal to find shop */
                add_item_to_wishlist(ch, QST_TARGET(quest_rnum), 100); /* High priority */
                ch->ai_data->current_goal = GOAL_GOTO_SHOP_TO_BUY;
                ch->ai_data->goal_item_vnum = QST_TARGET(quest_rnum);
                /* Roam to find a shop - the goal system will handle proper navigation */
                mob_goal_oriented_roam(ch, NOWHERE);
                if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                    return TRUE;
                return TRUE;
            }
            break;

        case AQ_SHOP_SELL:
            /* Mob needs to sell specific item to a shop */
            target_obj = get_obj_in_list_num(QST_TARGET(quest_rnum), ch->carrying);
            if (!target_obj) {
                /* Item not in inventory - might have already sold it, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else {
                /* Has the item, set goal to sell it */
                ch->ai_data->current_goal = GOAL_GOTO_SHOP_TO_SELL;
                ch->ai_data->goal_obj = target_obj;
                ch->ai_data->goal_item_vnum = GET_OBJ_VNUM(target_obj);
                return TRUE;
            }
            break;

        case AQ_DELIVERY:
            /* Similar to OBJ_RETURN - deliver item to specific mob */
            target_obj = get_obj_in_list_num(QST_TARGET(quest_rnum), ch->carrying);
            if (target_obj) {
                /* Has item, find delivery target mob */
                /* Validate room before accessing */
                if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                    return FALSE;
                target_mob = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_RETURNMOB(quest_rnum));
                if (target_mob) {
                    /* Deliver item */
                    act("$n entrega $p para $N.", FALSE, ch, target_obj, target_mob, TO_ROOM);
                    obj_from_char(target_obj);
                    obj_to_char(target_obj, target_mob);
                    mob_complete_quest(ch);
                    ch->ai_data->current_goal = GOAL_NONE;
                    return TRUE;
                } else {
                    /* Delivery target not found in room, seek them in the world */
                    target_mob = find_mob_by_vnum(QST_RETURNMOB(quest_rnum));
                    if (target_mob && IN_ROOM(target_mob) != NOWHERE) {
                        ch->ai_data->goal_destination = IN_ROOM(target_mob);
                        mob_goal_oriented_roam(ch, IN_ROOM(target_mob));
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            return TRUE;
                    } else {
                        /* Target not found in world, roam to search */
                        mob_goal_oriented_roam(ch, NOWHERE);
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            return TRUE;
                    }
                    return TRUE;
                }
            } else {
                /* Don't have item, try to find where it is and go there */
                target_room = find_object_location(QST_TARGET(quest_rnum));
                if (target_room != NOWHERE && target_room != IN_ROOM(ch)) {
                    ch->ai_data->goal_destination = target_room;
                    mob_goal_oriented_roam(ch, target_room);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                } else {
                    /* Object not found, add to wishlist and roam to search */
                    add_item_to_wishlist(ch, QST_TARGET(quest_rnum), 100);
                    mob_goal_oriented_roam(ch, NOWHERE);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                }
                return TRUE;
            }
            break;

        case AQ_RESOURCE_GATHER:
            /* Mob must gather X quantity of specific item - check if enough collected */
            {
                int count = 0;
                struct obj_data *obj;
                for (obj = ch->carrying; obj; obj = obj->next_content) {
                    if (GET_OBJ_VNUM(obj) == QST_TARGET(quest_rnum))
                        count++;
                }
                if (count >= ch->ai_data->quest_counter) {
                    mob_complete_quest(ch);
                    ch->ai_data->current_goal = GOAL_NONE;
                    return TRUE;
                } else {
                    /* Need more - try to find where the resource is and go there */
                    target_room = find_object_location(QST_TARGET(quest_rnum));
                    if (target_room != NOWHERE && target_room != IN_ROOM(ch)) {
                        ch->ai_data->goal_destination = target_room;
                        mob_goal_oriented_roam(ch, target_room);
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            return TRUE;
                    } else {
                        /* Resource not found, add to wishlist and roam to search */
                        add_item_to_wishlist(ch, QST_TARGET(quest_rnum), 100);
                        mob_goal_oriented_roam(ch, NOWHERE);
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            return TRUE;
                    }
                    return TRUE;
                }
            }
            break;

        case AQ_MOB_ESCORT:
            /* Escort mob to destination - simplified: follow target mob */
            /* Validate room before accessing */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return FALSE;
            target_mob = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_TARGET(quest_rnum));
            if (target_mob) {
                /* Found escort target, check if at destination */
                target_room = real_room(QST_RETURNMOB(quest_rnum)); /* Using RETURNMOB as destination for escort */
                /* Validate target_mob's room before accessing */
                if (target_room != NOWHERE && IN_ROOM(target_mob) != NOWHERE && IN_ROOM(target_mob) == target_room) {
                    mob_complete_quest(ch);
                    ch->ai_data->current_goal = GOAL_NONE;
                    return TRUE;
                }
                /* Stay near escort target - don't move away */
                return TRUE;
            } else {
                /* Lost escort target, search for them in the world */
                target_mob = find_mob_by_vnum(QST_TARGET(quest_rnum));
                if (target_mob && IN_ROOM(target_mob) != NOWHERE) {
                    ch->ai_data->goal_destination = IN_ROOM(target_mob);
                    mob_goal_oriented_roam(ch, IN_ROOM(target_mob));
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                } else {
                    /* Escort target not found, roam to search */
                    mob_goal_oriented_roam(ch, NOWHERE);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                }
                return TRUE;
            }
            break;

        case AQ_MAGIC_GATHER:
            /* Mob needs to visit locations with high magical density */
            /* Validate room before accessing */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return FALSE;
            target_room = real_room(QST_TARGET(quest_rnum));
            if (target_room != NOWHERE && IN_ROOM(ch) == target_room) {
                /* At magical location, complete quest */
                mob_complete_quest(ch);
                ch->ai_data->current_goal = GOAL_NONE;
                return TRUE;
            } else if (target_room != NOWHERE) {
                /* Move toward magical location */
                ch->ai_data->goal_destination = target_room;
                mob_goal_oriented_roam(ch, target_room);
                return TRUE;
            }
            break;

        case AQ_EMOTION_IMPROVE:
            /* Mob needs to improve emotion with target mob - interact with target */
            /* Validate room before accessing */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return FALSE;
            target_mob = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_TARGET(quest_rnum));
            if (target_mob) {
                /* Found target, perform positive social interaction
                 * We only decrement quest_counter if target is visible and interaction is possible */
                if (CAN_SEE(ch, target_mob) && GET_POS(ch) >= POS_RESTING) {
                    do_action(ch, GET_NAME(target_mob), find_command("nod"), 0);
                    /* Decrement counter only after successful interaction attempt */
                    if (--ch->ai_data->quest_counter <= 0) {
                        mob_complete_quest(ch);
                        ch->ai_data->current_goal = GOAL_NONE;
                    }
                }
                return TRUE;
            } else {
                /* Target not in room, seek them in the world */
                target_mob = find_mob_by_vnum(QST_TARGET(quest_rnum));
                if (target_mob && IN_ROOM(target_mob) != NOWHERE) {
                    ch->ai_data->goal_destination = IN_ROOM(target_mob);
                    mob_goal_oriented_roam(ch, IN_ROOM(target_mob));
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                } else {
                    /* Target not found, roam to search */
                    mob_goal_oriented_roam(ch, NOWHERE);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                }
                return TRUE;
            }
            break;

        case AQ_MOB_SAVE:
            /* Mob needs to save/protect target mob - stay near them and guard for a duration.
             * The quest_counter tracks how many ticks of successful protection are needed.
             * The mob must stay near the target and keep them safe until counter reaches 0. */
            /* Validate room before accessing */
            if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
                return FALSE;
            target_mob = get_mob_in_room_by_vnum(IN_ROOM(ch), QST_TARGET(quest_rnum));
            if (target_mob) {
                /* If target is fighting, help them by attacking their opponent first */
                if (FIGHTING(target_mob) && !FIGHTING(ch)) {
                    struct char_data *opponent = FIGHTING(target_mob);
                    /* Validate opponent before attacking */
                    if (opponent && !MOB_FLAGGED(opponent, MOB_NOTDEADYET) && !PLR_FLAGGED(opponent, PLR_NOTDEADYET) &&
                        IN_ROOM(opponent) != NOWHERE && IN_ROOM(opponent) == IN_ROOM(ch)) {
                        act("$n se move para proteger $N!", FALSE, ch, 0, target_mob, TO_ROOM);
                        /* Safety check: act() can trigger DG scripts which may cause extraction */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            return TRUE;
                        if (MOB_FLAGGED(target_mob, MOB_NOTDEADYET) || PLR_FLAGGED(target_mob, PLR_NOTDEADYET))
                            return TRUE;
                        if (MOB_FLAGGED(opponent, MOB_NOTDEADYET) || PLR_FLAGGED(opponent, PLR_NOTDEADYET))
                            return TRUE;
                        hit(ch, opponent, TYPE_UNDEFINED);
                        /* Safety check: hit() can indirectly cause extract_char */
                        if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                            return TRUE;
                        /* Additional safety check: target_mob may have been extracted by combat or scripts */
                        if (MOB_FLAGGED(target_mob, MOB_NOTDEADYET) || PLR_FLAGGED(target_mob, PLR_NOTDEADYET))
                            return TRUE;
                    }
                    return TRUE;
                }

                /* Target is not fighting (handled above). Check if target is healthy and decrement protection counter.
                 * Using 80% health as the threshold for "healthy" condition */
                if (GET_HIT(target_mob) > GET_MAX_HIT(target_mob) * 0.8 && !FIGHTING(target_mob)) {
                    /* Target is healthy - count this as a successful protection tick */
                    if (--ch->ai_data->quest_counter <= 0) {
                        /* Protected long enough - quest complete! */
                        mob_complete_quest(ch);
                        ch->ai_data->current_goal = GOAL_NONE;
                    }
                }
                /* If target is injured or fighting, stay and guard (don't decrement counter) */
                return TRUE;
            } else {
                /* Target not in room, seek them in the world */
                target_mob = find_mob_by_vnum(QST_TARGET(quest_rnum));
                if (target_mob && IN_ROOM(target_mob) != NOWHERE) {
                    ch->ai_data->goal_destination = IN_ROOM(target_mob);
                    mob_goal_oriented_roam(ch, IN_ROOM(target_mob));
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                } else {
                    /* Target not found, roam to search */
                    mob_goal_oriented_roam(ch, NOWHERE);
                    if (MOB_FLAGGED(ch, MOB_NOTDEADYET) || PLR_FLAGGED(ch, PLR_NOTDEADYET))
                        return TRUE;
                }
                return TRUE;
            }
            break;

        default:
            /* Unknown quest type, abandon quest */
            fail_mob_quest(ch, "unknown quest type");
            ch->ai_data->current_goal = GOAL_NONE;
            return TRUE;
    }

    return FALSE;
}

/**
 * Processa a wishlist de um mob e define um objetivo baseado no item de maior prioridade.
 * Implementa a árvore de decisão tática descrita no issue.
 * @param ch O mob cujo wishlist será processado
 */
void mob_process_wishlist_goals(struct char_data *ch)
{
    struct mob_wishlist_item *desired_item;
    mob_rnum target_mob;
    shop_rnum target_shop;
    room_rnum shop_room;
    obj_rnum obj_rnum;
    int item_cost, required_gold;
    bool has_active_quest;
    int i;

    if (!IS_NPC(ch) || !ch->ai_data || ch->ai_data->current_goal != GOAL_NONE) {
        return; /* Já tem um objetivo ou não é um mob com AI */
    }

    /* Check quest posting frustration timer - prevent quest posting after fleeing */
    if (ch->ai_data->quest_posting_frustration_timer > 0) {
        return; /* Still frustrated from fleeing, cannot post quests */
    }

    /* Priority 1: If mob has an active quest, prioritize quest completion */
    if (GET_MOB_QUEST(ch) != NOTHING) {
        qst_vnum mob_quest_vnum = GET_MOB_QUEST(ch);
        qst_rnum quest_rnum = real_quest(mob_quest_vnum);
        if (quest_rnum != NOTHING) {
            int quest_type = QST_TYPE(quest_rnum);

            /* Handle object-based quest types - check if mob already has the required object(s) */
            switch (quest_type) {
                case AQ_OBJ_FIND:
                case AQ_OBJ_RETURN:
                case AQ_DELIVERY:
                case AQ_SHOP_BUY: {
                    /* These quests require the mob to obtain a specific object */
                    obj_vnum quest_obj_vnum = QST_TARGET(quest_rnum);
                    struct obj_data *quest_obj = get_obj_in_list_num(quest_obj_vnum, ch->carrying);

                    if (!quest_obj) {
                        /* Mob doesn't have the object - add to wishlist with high priority */
                        add_item_to_wishlist(ch, quest_obj_vnum, 200);
                        desired_item = get_top_wishlist_item(ch);
                        /* Continue with normal wishlist processing for quest object */
                    } else {
                        /* Mob already has the quest object - transition to quest completion goal.
                         * mob_process_quest_completion will handle the specific completion logic. */
                        ch->ai_data->current_goal = GOAL_COMPLETE_QUEST;
                        ch->ai_data->goal_timer = 0;
                        return;
                    }
                    break;
                }

                case AQ_SHOP_SELL: {
                    /* This quest requires the mob to sell a specific object.
                     * Transition to completion - mob_process_quest_completion will check if item
                     * is still in inventory (needs to sell) or not (already sold, complete). */
                    ch->ai_data->current_goal = GOAL_COMPLETE_QUEST;
                    ch->ai_data->goal_timer = 0;
                    return;
                }

                case AQ_RESOURCE_GATHER: {
                    /* This quest requires gathering X quantity of a specific object.
                     * Check if mob already has enough. */
                    obj_vnum quest_obj_vnum = QST_TARGET(quest_rnum);
                    int required_count = ch->ai_data->quest_counter;
                    int current_count = 0;
                    struct obj_data *obj;

                    for (obj = ch->carrying; obj; obj = obj->next_content) {
                        if (GET_OBJ_VNUM(obj) == quest_obj_vnum)
                            current_count++;
                    }

                    if (current_count < required_count) {
                        /* Mob doesn't have enough - add to wishlist with high priority */
                        add_item_to_wishlist(ch, quest_obj_vnum, 200);
                        desired_item = get_top_wishlist_item(ch);
                        /* Continue with normal wishlist processing */
                    } else {
                        /* Mob already has enough - transition to quest completion */
                        ch->ai_data->current_goal = GOAL_COMPLETE_QUEST;
                        ch->ai_data->goal_timer = 0;
                        return;
                    }
                    break;
                }

                default:
                    /* For non-object quests (mob kill, room find, mob find, room clear, escort,
                     * magic gather, emotion improve, mob save, reputation build, player kill, bounty),
                     * transition directly to quest completion goal. */
                    ch->ai_data->current_goal = GOAL_COMPLETE_QUEST;
                    ch->ai_data->goal_timer = 0;
                    return;
            }
        } else {
            /* Quest no longer exists in quest table - was deleted.
             * Clear the mob's quest reference to prevent freeze/lag. */
            log1("QUEST FIX: Cleared invalid quest vnum %d from mob %s", mob_quest_vnum, GET_NAME(ch));
            clear_mob_quest(ch);
        }
    }

    desired_item = get_top_wishlist_item(ch);
    if (!desired_item) {
        return; /* Wishlist vazia */
    }

    /* Passo 1: Identificar o objetivo */
    /* Passo 2: Encontrar fontes para o item */

    /* Tenta encontrar um mob que dropa o item */
    target_mob = find_mob_with_item(desired_item->vnum);

    /* Tenta encontrar uma loja que vende o item */
    target_shop = find_shop_selling_item(desired_item->vnum);

    /* Passo 3: Avaliar as opções e escolher o melhor plano */

    /* Opção 1: Caçar um mob */
    if (target_mob != NOBODY && target_mob >= 0 && target_mob < top_of_mobt) {
        /* Verifica se consegue matar o alvo (simplificado) */
        if (GET_LEVEL(ch) >= mob_proto[target_mob].player.level - 5) {
            /* Pode caçar este mob */
            ch->ai_data->current_goal = GOAL_HUNT_TARGET;
            ch->ai_data->goal_target_mob_rnum = target_mob;
            ch->ai_data->goal_item_vnum = desired_item->vnum;
            ch->ai_data->goal_timer = 0;
            act("$n parece estar a planear uma caça.", FALSE, ch, 0, 0, TO_ROOM);
            return;
        }
    }

    /* Opção 2: Comprar numa loja */
    if (target_shop != NOTHING && target_shop <= top_shop && shop_index[target_shop].in_room) {
        /* Verifica se pode chegar à loja e tem ouro suficiente */
        shop_room = real_room(SHOP_ROOM(target_shop, 0));

        if (shop_room != NOWHERE && !ROOM_FLAGGED(shop_room, ROOM_NOTRACK)) {
            /* Calcula o custo do item */
            obj_rnum = real_object(desired_item->vnum);
            if (obj_rnum != NOTHING && obj_rnum >= 0 && obj_rnum < top_of_objt) {
                item_cost = GET_OBJ_COST(&obj_proto[obj_rnum]);
                if (item_cost <= 0)
                    item_cost = 1;
                item_cost = (int)(item_cost * shop_index[target_shop].profit_buy);

                if (GET_GOLD(ch) >= item_cost) {
                    /* Tem ouro suficiente, vai comprar */
                    ch->ai_data->current_goal = GOAL_GOTO_SHOP_TO_BUY;
                    ch->ai_data->goal_destination = shop_room;
                    ch->ai_data->goal_target_mob_rnum = SHOP_KEEPER(target_shop);
                    ch->ai_data->goal_item_vnum = desired_item->vnum;
                    ch->ai_data->goal_timer = 0;
                    act("$n examina a sua bolsa e sorri.", FALSE, ch, 0, 0, TO_ROOM);
                    return;
                } else {
                    /* Não tem ouro suficiente, precisa conseguir mais */
                    required_gold = item_cost - GET_GOLD(ch);
                    if (required_gold <= GET_LEVEL(ch) * 100) { /* Meta razoável */
                        ch->ai_data->current_goal = GOAL_GET_GOLD;
                        ch->ai_data->goal_item_vnum = desired_item->vnum;
                        ch->ai_data->goal_timer = 0;
                        act("$n conta as suas moedas e franze o sobrolho.", FALSE, ch, 0, 0, TO_ROOM);
                        return;
                    }
                }
            }
        }
    }

    /* Opção 3: Postar uma quest (implementação aprimorada) */
    /* Skip if there's already an active quest for this item - try other methods instead */
    has_active_quest = FALSE;
    for (i = 0; i < total_quests; i++) {
        if (QST_RETURNMOB(i) == GET_MOB_VNUM(ch) && QST_TARGET(i) == desired_item->vnum &&
            QST_TYPE(i) == AQ_OBJ_RETURN) {
            has_active_quest = TRUE;
            break;
        }
    }

    if (!has_active_quest && GET_GOLD(ch) >= desired_item->priority * 2) {
        /* Early exit: Check quest limit BEFORE expensive pathfinding
         * With 451 autoquests, pathfinding for questmasters is catastrophically expensive:
         * - Loops through 451 quests × ALL characters × BFS pathfinding
         * - Can cause multi-second lag spikes when many mobs attempt this
         * - At quest limit, posting will fail anyway, so skip the expensive search */
        if (!can_add_mob_posted_quest()) {
            /* At quest limit - don't waste CPU searching for questmasters */
            return;
        }

        /* Safety check: Validate room before accessing world array */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
            return;

        /* Tem ouro suficiente para oferecer uma recompensa */
        zone_rnum mob_zone = world[IN_ROOM(ch)].zone;
        struct char_data *accessible_qm = find_accessible_questmaster_in_zone(ch, mob_zone);

        if (accessible_qm && accessible_qm != ch) {
            /* Há um questmaster acessível, vai até ele para postar */
            ch->ai_data->current_goal = GOAL_GOTO_QUESTMASTER;
            ch->ai_data->goal_destination = IN_ROOM(accessible_qm);
            ch->ai_data->goal_item_vnum = desired_item->vnum;
            ch->ai_data->goal_timer = 0;
            act("$n parece estar a considerar contratar aventureiros.", FALSE, ch, 0, 0, TO_ROOM);
            return; /* Vai tentar chegar ao questmaster */
        } else {
            /* Não há questmaster acessível, torna-se questmaster próprio */
            ch->ai_data->current_goal = GOAL_POST_QUEST;
            ch->ai_data->goal_item_vnum = desired_item->vnum;
            ch->ai_data->goal_timer = 0;
            act("$n parece estar a considerar contratar aventureiros.", FALSE, ch, 0, 0, TO_ROOM);

            /* Simula a postagem da quest */
            mob_posts_quest(ch, desired_item->vnum, desired_item->priority * 2);

            /* Reseta o objetivo após "postar" a quest */
            ch->ai_data->current_goal = GOAL_NONE;
            return;
        }
    }

    /* Se chegou aqui, não consegue obter o item por agora */
    /* Remove da wishlist itens muito antigos (>1 hora) */
    if (time(0) - desired_item->added_time > 3600) {
        remove_item_from_wishlist(ch, desired_item->vnum);
    }
}

/*
 * ===============================================
 * MOB COMMAND USAGE FUNCTIONS
 * ===============================================
 */

/**
 * Makes a mob try to donate an item to one of the donation rooms
 * @param ch The mob
 * @param obj The object to donate (NULL to find one automatically)
 * @return TRUE if donation was attempted
 */
bool mob_try_donate(struct char_data *ch, struct obj_data *obj)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* If no object specified, find a suitable item to donate */
    if (!obj) {
        struct obj_data *item;
        int min_score = 10000;

        /* Find the least valuable item to donate */
        for (item = ch->carrying; item; item = item->next_content) {
            if (!CAN_WEAR(item, ITEM_WEAR_TAKE) || OBJ_FLAGGED(item, ITEM_NODROP)) {
                continue;
            }

            int score = evaluate_item_for_mob(ch, item);
            if (score < min_score) {
                min_score = score;
                obj = item;
            }
        }

        if (!obj) {
            return FALSE; /* No suitable item found */
        }
    }

    /* Use the donate command */
    char cmd_buf[MAX_INPUT_LENGTH];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", obj->name);
    do_drop(ch, cmd_buf, 0, SCMD_DONATE);

    return TRUE;
}

/**
 * Makes a mob try to sacrifice a corpse
 * @param ch The mob
 * @param corpse The corpse to sacrifice (NULL to find one automatically)
 * @return TRUE if sacrifice was attempted
 */
bool mob_try_sacrifice(struct char_data *ch, struct obj_data *corpse)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* If no corpse specified, find one in the room */
    if (!corpse) {
        struct obj_data *obj;

        /* Safety check: Validate room before accessing world array */
        if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
            return FALSE;

        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
            if (IS_CORPSE(obj)) {
                corpse = obj;
                break;
            }
        }

        if (!corpse) {
            return FALSE; /* No corpse found */
        }
    }

    /* Use the sacrifice command */
    char cmd_buf[MAX_INPUT_LENGTH];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", corpse->name);
    do_sac(ch, cmd_buf, 0, 0);

    return TRUE;
}

/**
 * Makes a mob try to junk (destroy) an item
 * @param ch The mob
 * @param obj The object to junk (NULL to find one automatically)
 * @return TRUE if junk was attempted
 */
bool mob_try_junk(struct char_data *ch, struct obj_data *obj)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* If no object specified, find a suitable item to junk */
    if (!obj) {
        struct obj_data *item;
        int min_score = 10000;

        /* Find the least valuable item to junk */
        for (item = ch->carrying; item; item = item->next_content) {
            if (!CAN_WEAR(item, ITEM_WEAR_TAKE) || OBJ_FLAGGED(item, ITEM_NODROP)) {
                continue;
            }

            int score = evaluate_item_for_mob(ch, item);
            if (score < min_score && GET_OBJ_COST(item) < 50) { /* Only junk cheap items */
                min_score = score;
                obj = item;
            }
        }

        if (!obj) {
            return FALSE; /* No suitable item found */
        }
    }

    /* Use the junk command */
    char cmd_buf[MAX_INPUT_LENGTH];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", obj->name);
    do_drop(ch, cmd_buf, 0, SCMD_JUNK);

    return TRUE;
}

/**
 * Makes a mob try to drop an item
 * @param ch The mob
 * @param obj The object to drop (NULL to find one automatically)
 * @return TRUE if drop was attempted
 */
bool mob_try_drop(struct char_data *ch, struct obj_data *obj)
{
    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* If no object specified, find a suitable item to drop */
    if (!obj) {
        struct obj_data *item;

        /* Drop a random item from inventory */
        int count = 0;
        for (item = ch->carrying; item; item = item->next_content) {
            if (CAN_WEAR(item, ITEM_WEAR_TAKE) && !OBJ_FLAGGED(item, ITEM_NODROP)) {
                count++;
            }
        }

        if (count == 0) {
            return FALSE; /* No droppable items */
        }

        int target = rand_number(1, count);
        count = 0;
        for (item = ch->carrying; item; item = item->next_content) {
            if (CAN_WEAR(item, ITEM_WEAR_TAKE) && !OBJ_FLAGGED(item, ITEM_NODROP)) {
                count++;
                if (count == target) {
                    obj = item;
                    break;
                }
            }
        }

        if (!obj) {
            return FALSE;
        }
    }

    /* Use the drop command */
    char cmd_buf[MAX_INPUT_LENGTH];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", obj->name);
    do_drop(ch, cmd_buf, 0, SCMD_DROP);

    return TRUE;
}

/* Source types for key location */
#define KEY_SOURCE_ROOM 1      /* Key is on the ground in a room */
#define KEY_SOURCE_CONTAINER 2 /* Key is inside a container */
#define KEY_SOURCE_MOB 3       /* Key is carried by a mob */

/**
 * Find where a specific key can be obtained
 * @param key_vnum The vnum of the key to find
 * @param source_type Pointer to store the source type (KEY_SOURCE_*)
 * @param carrying_mob Pointer to store the mob vnum if carried by a mob
 * @return room number where the key can be found, or NOWHERE if not found
 */
room_rnum find_key_location(obj_vnum key_vnum, int *source_type, mob_vnum *carrying_mob)
{
    room_rnum room;
    struct obj_data *obj;
    struct char_data *mob;

    if (source_type)
        *source_type = 0;
    if (carrying_mob)
        *carrying_mob = NOBODY;

    /* Search all rooms for the key */
    for (room = 0; room <= top_of_world; room++) {
        /* Check objects on room floor */
        for (obj = world[room].contents; obj; obj = obj->next_content) {
            if (GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == key_vnum) {
                if (source_type)
                    *source_type = KEY_SOURCE_ROOM;
                return room;
            }

            /* Check inside containers */
            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
                struct obj_data *contained;
                for (contained = obj->contains; contained; contained = contained->next_content) {
                    if (GET_OBJ_TYPE(contained) == ITEM_KEY && GET_OBJ_VNUM(contained) == key_vnum) {
                        if (source_type)
                            *source_type = KEY_SOURCE_CONTAINER;
                        return room;
                    }
                }
            }
        }

        /* Check mobs in this room */
        for (mob = world[room].people; mob; mob = mob->next_in_room) {
            if (!IS_NPC(mob))
                continue;

            /* Check mob inventory */
            for (obj = mob->carrying; obj; obj = obj->next_content) {
                if (GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == key_vnum) {
                    if (source_type)
                        *source_type = KEY_SOURCE_MOB;
                    if (carrying_mob)
                        *carrying_mob = GET_MOB_VNUM(mob);
                    return room;
                }
            }

            /* Check mob equipment */
            for (int wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
                obj = GET_EQ(mob, wear_pos);
                if (obj && GET_OBJ_TYPE(obj) == ITEM_KEY && GET_OBJ_VNUM(obj) == key_vnum) {
                    if (source_type)
                        *source_type = KEY_SOURCE_MOB;
                    if (carrying_mob)
                        *carrying_mob = GET_MOB_VNUM(mob);
                    return room;
                }
            }
        }
    }

    return NOWHERE; /* Key not found anywhere */
}

/**
 * Set a key collection goal for a mob, storing the original goal to return to
 * @param ch The mob
 * @param key_vnum The key to collect
 * @param original_goal The goal to return to after collecting the key
 * @param original_dest The original destination
 * @return TRUE if goal was set successfully
 */
bool mob_set_key_collection_goal(struct char_data *ch, obj_vnum key_vnum, int original_goal, room_rnum original_dest)
{
    int source_type;
    mob_vnum carrying_mob;
    room_rnum key_location;

    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* Find where the key is located */
    key_location = find_key_location(key_vnum, &source_type, &carrying_mob);

    if (key_location == NOWHERE) {
        return FALSE; /* Key not found anywhere */
    }

    /* Store the original goal information */
    ch->ai_data->original_goal = original_goal;
    ch->ai_data->original_destination = original_dest;
    ch->ai_data->original_obj = ch->ai_data->goal_obj;
    ch->ai_data->original_target_mob = ch->ai_data->goal_target_mob_rnum;
    ch->ai_data->original_item_vnum = ch->ai_data->goal_item_vnum;

    /* Set new key collection goal */
    ch->ai_data->current_goal = GOAL_COLLECT_KEY;
    ch->ai_data->goal_destination = key_location;
    ch->ai_data->goal_item_vnum = key_vnum;
    ch->ai_data->goal_timer = 0;

    /* Set appropriate target based on source type */
    switch (source_type) {
        case KEY_SOURCE_ROOM:
        case KEY_SOURCE_CONTAINER:
            ch->ai_data->goal_target_mob_rnum = NOBODY;
            ch->ai_data->goal_obj = NULL;
            break;
        case KEY_SOURCE_MOB:
            ch->ai_data->goal_target_mob_rnum = real_mobile(carrying_mob);
            ch->ai_data->goal_obj = NULL;
            break;
    }

    act("$n parece estar procurando por algo específico.", FALSE, ch, 0, 0, TO_ROOM);

    return TRUE;
}

/**
 * Find a bank object (ATM/cashcard) in the same room as the mob or in the mob's inventory
 * @param ch The mob looking for a bank
 * @return Pointer to a bank object, or NULL if none found
 */
struct obj_data *find_bank_nearby(struct char_data *ch)
{
    struct obj_data *obj;

    if (!ch || !IS_NPC(ch)) {
        return NULL;
    }

    /* Safety check: Validate room before accessing world array */
    if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) < 0 || IN_ROOM(ch) > top_of_world)
        return NULL;

    /* First check objects in the same room as the mob */
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
        /* Check if this object has the bank special procedure */
        if (GET_OBJ_SPEC(obj) == bank) {
            return obj;
        }
    }

    /* Then check objects in the mob's inventory */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        /* Check if this object has the bank special procedure */
        if (GET_OBJ_SPEC(obj) == bank) {
            return obj;
        }
    }

    return NULL; /* No bank found in room or inventory */
}

/**
 * Makes a mob use banking services if available and appropriate
 * @param ch The mob that might use the bank
 * @return TRUE if bank was used, FALSE otherwise
 */
bool mob_use_bank(struct char_data *ch)
{
    struct obj_data *bank_obj;
    int current_gold, bank_gold;
    int deposit_amount, withdraw_amount;

    if (!ch || !IS_NPC(ch) || !ch->ai_data) {
        return FALSE;
    }

    /* Only mobs with high trade genetics use banks */
    if (GET_GENTRADE(ch) <= 50) {
        return FALSE;
    }

    /* Only mobs with very high trade genetics should use banks */
    if (GET_GENTRADE(ch) <= 60) {
        return FALSE;
    }

    /* Only intelligent mobs should use banks (Intelligence > 10) */
    if (GET_INT(ch) <= 10) {
        return FALSE;
    }

    /* Don't use bank if fighting or not awake */
    if (FIGHTING(ch) || !AWAKE(ch)) {
        return FALSE;
    }

    /* Find a bank in the same room or inventory */
    bank_obj = find_bank_nearby(ch);
    if (!bank_obj) {
        return FALSE; /* No bank available */
    }

    current_gold = GET_GOLD(ch);
    bank_gold = GET_BANK_GOLD(ch);

    /* Decision logic: deposit if carrying too much, withdraw if too little */
    if (current_gold > 5000 && rand_number(1, 100) <= 30) {
        /* Deposit excess gold */
        deposit_amount = current_gold / 2; /* Deposit half of current gold */

        /* Simulate the deposit command */
        decrease_gold(ch, deposit_amount);
        increase_bank(ch, deposit_amount);

        act("$n faz uma transação bancária.", TRUE, ch, 0, FALSE, TO_ROOM);

        return TRUE;

    } else if (current_gold < 100 && bank_gold > 500 && rand_number(1, 100) <= 20) {
        /* Withdraw gold when running low */
        withdraw_amount = MIN(bank_gold / 3, 1000); /* Withdraw 1/3 of bank balance, max 1000 */

        /* Simulate the withdraw command */
        increase_gold(ch, withdraw_amount);
        decrease_bank(ch, withdraw_amount);

        act("$n faz uma transação bancária.", TRUE, ch, 0, FALSE, TO_ROOM);

        return TRUE;
    }

    return FALSE; /* No banking action taken */
}

bool shadow_should_activate(struct char_data *ch)
{
    if (!ch || !ch->ai_data)
        return FALSE;

    int cap = ch->ai_data->cognitive_capacity;
    if (cap < 50)
        return FALSE;

    int curiosity = ch->ai_data->emotion_curiosity;
    int fear = ch->ai_data->emotion_fear;
    int greed = ch->ai_data->emotion_greed;

    /* --- Base motivational drive --- */
    int base_drive = (curiosity * 5) / 10 + /* 0.5 */
                     (fear * 7) / 10 +      /* 0.7 */
                     (greed * 4) / 10;      /* 0.4 */

    /* --- Context multiplier --- */
    int multiplier = 100; /* 1.0 */

    if (FIGHTING(ch))
        multiplier += 60;

    if (GET_HIT(ch) < GET_MAX_HIT(ch) / 3)
        multiplier += 30;

    int interest = (base_drive * multiplier) / 100;

    /* --- Fatigue scaling --- */
    interest = (interest * cap) / 1000;

    /* --- Novelty immediate boost --- */
    int novelty = ch->ai_data->recent_prediction_error;
    interest += novelty / 3;

    /* --- Attention bias (long-term adaptation) --- */
    interest += ch->ai_data->attention_bias;

    /* --- Dynamic threshold --- */
    int threshold = rand_number(60, 120);

    return interest > threshold;
}
