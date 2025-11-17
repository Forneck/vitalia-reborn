/**************************************************************************
 *  File: cedit.c                                           Part of tbaMUD *
 *  Usage: A graphical in-game game configuration utility for OasisOLC.    *
 *                                                                         *
 *  Copyright 2002-2003 Kip Potter                                         *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "constants.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "modify.h"

/* local scope functions, not used externally */
static void cedit_disp_menu(struct descriptor_data *d);
static void cedit_save_internally(struct descriptor_data *d);
static void cedit_disp_game_play_options(struct descriptor_data *d);
static void cedit_disp_crash_save_options(struct descriptor_data *d);
static void cedit_disp_room_numbers(struct descriptor_data *d);
static void cedit_disp_operation_options(struct descriptor_data *d);
static void cedit_disp_autowiz_options(struct descriptor_data *d);
static void cedit_disp_experimental_options(struct descriptor_data *d);
static void cedit_disp_emotion_menu(struct descriptor_data *d);
static void cedit_load_emotion_preset(struct descriptor_data *d, int preset);
static void reassign_rooms(void);
static void cedit_setup(struct descriptor_data *d);

ACMD(do_oasis_cedit)
{
    struct descriptor_data *d;
    char buf1[MAX_STRING_LENGTH];

    /* No building as a mob or while being forced. */
    if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
        return;

    /* Parse any arguments. */
    one_argument(argument, buf1);

    if (GET_LEVEL(ch) < LVL_IMPL) {
        send_to_char(ch, "You can't modify the game configuration.\r\n");
        return;
    }

    d = ch->desc;

    if (!*buf1) {
        CREATE(d->olc, struct oasis_olc_data, 1);
        OLC_ZONE(d) = 0;
        cedit_setup(d);
        STATE(d) = CON_CEDIT;
        act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
        SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

        mudlog(BRF, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s starts editing the game configuration.",
               GET_NAME(ch));
        return;
    } else if (str_cmp("save", buf1) != 0) {
        send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
        return;
    }

    send_to_char(ch, "Saving the game configuration.\r\n");
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "OLC: %s saves the game configuration.", GET_NAME(ch));

    cedit_save_to_disk();
}

static void cedit_setup(struct descriptor_data *d)
{
    /* Create the config_data struct. */
    CREATE(OLC_CONFIG(d), struct config_data, 1);

    /* Copy the current configuration from the config_info to this one and
       copy the game play options from the configuration info struct. */
    OLC_CONFIG(d)->play.pk_allowed = CONFIG_PK_ALLOWED;
    OLC_CONFIG(d)->play.pt_allowed = CONFIG_PT_ALLOWED;
    OLC_CONFIG(d)->play.fit_evolve = CONFIG_FIT_EVOLVE;
    OLC_CONFIG(d)->play.weather_affects_spells = CONFIG_WEATHER_AFFECTS_SPELLS;
    OLC_CONFIG(d)->play.school_weather_affects = CONFIG_SCHOOL_WEATHER_AFFECTS;
    OLC_CONFIG(d)->play.max_pathfind_iterations = CONFIG_MAX_PATHFIND_ITERATIONS;
    OLC_CONFIG(d)->play.max_zone_path = CONFIG_MAX_ZONE_PATH;
    OLC_CONFIG(d)->play.level_can_shout = CONFIG_LEVEL_CAN_SHOUT;
    OLC_CONFIG(d)->play.holler_move_cost = CONFIG_HOLLER_MOVE_COST;
    OLC_CONFIG(d)->play.tunnel_size = CONFIG_TUNNEL_SIZE;
    OLC_CONFIG(d)->play.max_exp_gain = CONFIG_MAX_EXP_GAIN;
    OLC_CONFIG(d)->play.max_exp_loss = CONFIG_MAX_EXP_LOSS;
    OLC_CONFIG(d)->play.max_npc_corpse_time = CONFIG_MAX_NPC_CORPSE_TIME;
    OLC_CONFIG(d)->play.max_pc_corpse_time = CONFIG_MAX_PC_CORPSE_TIME;
    OLC_CONFIG(d)->play.idle_void = CONFIG_IDLE_VOID;
    OLC_CONFIG(d)->play.idle_rent_time = CONFIG_IDLE_RENT_TIME;
    OLC_CONFIG(d)->play.idle_max_level = CONFIG_IDLE_MAX_LEVEL;
    OLC_CONFIG(d)->play.dts_are_dumps = CONFIG_DTS_ARE_DUMPS;
    OLC_CONFIG(d)->play.load_into_inventory = CONFIG_LOAD_INVENTORY;
    OLC_CONFIG(d)->play.track_through_doors = CONFIG_TRACK_T_DOORS;
    OLC_CONFIG(d)->play.no_mort_to_immort = CONFIG_NO_MORT_TO_IMMORT;
    OLC_CONFIG(d)->play.disp_closed_doors = CONFIG_DISP_CLOSED_DOORS;
    OLC_CONFIG(d)->play.diagonal_dirs = CONFIG_DIAGONAL_DIRS;
    OLC_CONFIG(d)->play.map_option = CONFIG_MAP;
    OLC_CONFIG(d)->play.map_size = CONFIG_MAP_SIZE;
    OLC_CONFIG(d)->play.minimap_size = CONFIG_MINIMAP_SIZE;
    OLC_CONFIG(d)->play.script_players = CONFIG_SCRIPT_PLAYERS;
    OLC_CONFIG(d)->play.max_house_objs = CONFIG_MAX_HOUSE_OBJS;

    /* Crash Saves */
    OLC_CONFIG(d)->csd.free_rent = CONFIG_FREE_RENT;
    OLC_CONFIG(d)->csd.max_obj_save = CONFIG_MAX_OBJ_SAVE;
    OLC_CONFIG(d)->csd.min_rent_cost = CONFIG_MIN_RENT_COST;
    OLC_CONFIG(d)->csd.auto_save = CONFIG_AUTO_SAVE;
    OLC_CONFIG(d)->csd.autosave_time = CONFIG_AUTOSAVE_TIME;
    OLC_CONFIG(d)->csd.crash_file_timeout = CONFIG_CRASH_TIMEOUT;
    OLC_CONFIG(d)->csd.rent_file_timeout = CONFIG_RENT_TIMEOUT;

    /* Room Numbers */
    OLC_CONFIG(d)->room_nums.newbie_start_room = CONFIG_NEWBIE_START;
    OLC_CONFIG(d)->room_nums.immort_start_room = CONFIG_IMMORTAL_START;
    OLC_CONFIG(d)->room_nums.frozen_start_room = CONFIG_FROZEN_START;
    OLC_CONFIG(d)->room_nums.donation_room_1 = CONFIG_DON_ROOM_1;
    OLC_CONFIG(d)->room_nums.donation_room_2 = CONFIG_DON_ROOM_2;
    OLC_CONFIG(d)->room_nums.donation_room_3 = CONFIG_DON_ROOM_3;
    OLC_CONFIG(d)->room_nums.donation_room_4 = CONFIG_DON_ROOM_4;

    OLC_CONFIG(d)->room_nums.dead_start_room = CONFIG_DEAD_START;
    OLC_CONFIG(d)->room_nums.hometown_1 = CONFIG_HOMETOWN_1;
    OLC_CONFIG(d)->room_nums.hometown_2 = CONFIG_HOMETOWN_2;
    OLC_CONFIG(d)->room_nums.hometown_3 = CONFIG_HOMETOWN_3;
    OLC_CONFIG(d)->room_nums.hometown_4 = CONFIG_HOMETOWN_4;
    OLC_CONFIG(d)->room_nums.ress_room_1 = CONFIG_RESS_ROOM_1;
    OLC_CONFIG(d)->room_nums.ress_room_2 = CONFIG_RESS_ROOM_2;
    OLC_CONFIG(d)->room_nums.ress_room_3 = CONFIG_RESS_ROOM_3;
    OLC_CONFIG(d)->room_nums.ress_room_4 = CONFIG_RESS_ROOM_4;

    /* Game Operation */
    OLC_CONFIG(d)->operation.DFLT_PORT = CONFIG_DFLT_PORT;
    OLC_CONFIG(d)->operation.max_playing = CONFIG_MAX_PLAYING;
    OLC_CONFIG(d)->operation.max_filesize = CONFIG_MAX_FILESIZE;
    OLC_CONFIG(d)->operation.max_bad_pws = CONFIG_MAX_BAD_PWS;
    OLC_CONFIG(d)->operation.siteok_everyone = CONFIG_SITEOK_ALL;
    OLC_CONFIG(d)->operation.use_new_socials = CONFIG_NEW_SOCIALS;
    OLC_CONFIG(d)->operation.auto_save_olc = CONFIG_OLC_SAVE;
    OLC_CONFIG(d)->operation.nameserver_is_slow = CONFIG_NS_IS_SLOW;
    OLC_CONFIG(d)->operation.medit_advanced = CONFIG_MEDIT_ADVANCED;
    OLC_CONFIG(d)->operation.ibt_autosave = CONFIG_IBT_AUTOSAVE;
    OLC_CONFIG(d)->operation.protocol_negotiation = CONFIG_PROTOCOL_NEGOTIATION;
    OLC_CONFIG(d)->operation.special_in_comm = CONFIG_SPECIAL_IN_COMM;
    OLC_CONFIG(d)->operation.debug_mode = CONFIG_DEBUG_MODE;

    /* Autowiz */
    OLC_CONFIG(d)->autowiz.use_autowiz = CONFIG_USE_AUTOWIZ;
    OLC_CONFIG(d)->autowiz.min_wizlist_lev = CONFIG_MIN_WIZLIST_LEV;

    /* Experimental Features */
    OLC_CONFIG(d)->experimental.new_auction_system = CONFIG_NEW_AUCTION_SYSTEM;
    OLC_CONFIG(d)->experimental.experimental_bank_system = CONFIG_EXPERIMENTAL_BANK_SYSTEM;
    OLC_CONFIG(d)->experimental.mob_contextual_socials = CONFIG_MOB_CONTEXTUAL_SOCIALS;
    OLC_CONFIG(d)->experimental.dynamic_reputation = CONFIG_DYNAMIC_REPUTATION;
    OLC_CONFIG(d)->experimental.mob_emotion_social_chance = CONFIG_MOB_EMOTION_SOCIAL_CHANCE;
    OLC_CONFIG(d)->experimental.mob_emotion_update_chance = CONFIG_MOB_EMOTION_UPDATE_CHANCE;
    OLC_CONFIG(d)->experimental.weather_affects_emotions = CONFIG_WEATHER_AFFECTS_EMOTIONS;
    OLC_CONFIG(d)->experimental.weather_effect_multiplier = CONFIG_WEATHER_EFFECT_MULTIPLIER;

    /* Emotion System Configuration */
    /* Visual indicator thresholds */
    OLC_CONFIG(d)->emotion_config.display_fear_threshold = CONFIG_EMOTION_DISPLAY_FEAR_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.display_anger_threshold = CONFIG_EMOTION_DISPLAY_ANGER_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.display_happiness_threshold = CONFIG_EMOTION_DISPLAY_HAPPINESS_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.display_sadness_threshold = CONFIG_EMOTION_DISPLAY_SADNESS_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.display_horror_threshold = CONFIG_EMOTION_DISPLAY_HORROR_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.display_pain_threshold = CONFIG_EMOTION_DISPLAY_PAIN_THRESHOLD;

    /* Combat flee behavior thresholds */
    OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = CONFIG_EMOTION_FLEE_FEAR_LOW_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = CONFIG_EMOTION_FLEE_FEAR_HIGH_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = CONFIG_EMOTION_FLEE_COURAGE_LOW_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = CONFIG_EMOTION_FLEE_COURAGE_HIGH_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.flee_horror_threshold = CONFIG_EMOTION_FLEE_HORROR_THRESHOLD;

    /* Flee modifier values */
    OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = CONFIG_EMOTION_FLEE_FEAR_LOW_MODIFIER;
    OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = CONFIG_EMOTION_FLEE_FEAR_HIGH_MODIFIER;
    OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = CONFIG_EMOTION_FLEE_COURAGE_LOW_MODIFIER;
    OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = CONFIG_EMOTION_FLEE_COURAGE_HIGH_MODIFIER;
    OLC_CONFIG(d)->emotion_config.flee_horror_modifier = CONFIG_EMOTION_FLEE_HORROR_MODIFIER;

    /* Pain system thresholds and values */
    OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = CONFIG_EMOTION_PAIN_DAMAGE_MINOR_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = CONFIG_EMOTION_PAIN_DAMAGE_MODERATE_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = CONFIG_EMOTION_PAIN_DAMAGE_HEAVY_THRESHOLD;
    OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = CONFIG_EMOTION_PAIN_DAMAGE_MASSIVE_THRESHOLD;

    OLC_CONFIG(d)->emotion_config.pain_minor_min = CONFIG_EMOTION_PAIN_MINOR_MIN;
    OLC_CONFIG(d)->emotion_config.pain_minor_max = CONFIG_EMOTION_PAIN_MINOR_MAX;
    OLC_CONFIG(d)->emotion_config.pain_moderate_min = CONFIG_EMOTION_PAIN_MODERATE_MIN;
    OLC_CONFIG(d)->emotion_config.pain_moderate_max = CONFIG_EMOTION_PAIN_MODERATE_MAX;
    OLC_CONFIG(d)->emotion_config.pain_heavy_min = CONFIG_EMOTION_PAIN_HEAVY_MIN;
    OLC_CONFIG(d)->emotion_config.pain_heavy_max = CONFIG_EMOTION_PAIN_HEAVY_MAX;
    OLC_CONFIG(d)->emotion_config.pain_massive_min = CONFIG_EMOTION_PAIN_MASSIVE_MIN;
    OLC_CONFIG(d)->emotion_config.pain_massive_max = CONFIG_EMOTION_PAIN_MASSIVE_MAX;

    /* Memory system weights and thresholds */
    OLC_CONFIG(d)->emotion_config.memory_weight_recent = CONFIG_EMOTION_MEMORY_WEIGHT_RECENT;
    OLC_CONFIG(d)->emotion_config.memory_weight_fresh = CONFIG_EMOTION_MEMORY_WEIGHT_FRESH;
    OLC_CONFIG(d)->emotion_config.memory_weight_moderate = CONFIG_EMOTION_MEMORY_WEIGHT_MODERATE;
    OLC_CONFIG(d)->emotion_config.memory_weight_old = CONFIG_EMOTION_MEMORY_WEIGHT_OLD;
    OLC_CONFIG(d)->emotion_config.memory_weight_ancient = CONFIG_EMOTION_MEMORY_WEIGHT_ANCIENT;

    OLC_CONFIG(d)->emotion_config.memory_age_recent = CONFIG_EMOTION_MEMORY_AGE_RECENT;
    OLC_CONFIG(d)->emotion_config.memory_age_fresh = CONFIG_EMOTION_MEMORY_AGE_FRESH;
    OLC_CONFIG(d)->emotion_config.memory_age_moderate = CONFIG_EMOTION_MEMORY_AGE_MODERATE;
    OLC_CONFIG(d)->emotion_config.memory_age_old = CONFIG_EMOTION_MEMORY_AGE_OLD;

    OLC_CONFIG(d)->emotion_config.memory_baseline_offset = CONFIG_EMOTION_MEMORY_BASELINE_OFFSET;

    /* Allocate space for the strings. */
    OLC_CONFIG(d)->play.OK = str_udup(CONFIG_OK);
    OLC_CONFIG(d)->play.HUH = str_udup(CONFIG_HUH);
    OLC_CONFIG(d)->play.NOPERSON = str_udup(CONFIG_NOPERSON);
    OLC_CONFIG(d)->play.NOEFFECT = str_udup(CONFIG_NOEFFECT);

    if (CONFIG_DFLT_IP)
        OLC_CONFIG(d)->operation.DFLT_IP = strdup(CONFIG_DFLT_IP);
    else
        OLC_CONFIG(d)->operation.DFLT_IP = NULL;

    if (CONFIG_DFLT_DIR)
        OLC_CONFIG(d)->operation.DFLT_DIR = strdup(CONFIG_DFLT_DIR);
    else
        OLC_CONFIG(d)->operation.DFLT_DIR = NULL;

    if (CONFIG_LOGNAME)
        OLC_CONFIG(d)->operation.LOGNAME = strdup(CONFIG_LOGNAME);
    else
        OLC_CONFIG(d)->operation.LOGNAME = NULL;

    if (CONFIG_MENU)
        OLC_CONFIG(d)->operation.MENU = strdup(CONFIG_MENU);
    else
        OLC_CONFIG(d)->operation.MENU = NULL;

    if (CONFIG_WELC_MESSG)
        OLC_CONFIG(d)->operation.WELC_MESSG = strdup(CONFIG_WELC_MESSG);
    else
        OLC_CONFIG(d)->operation.WELC_MESSG = NULL;

    if (CONFIG_START_MESSG)
        OLC_CONFIG(d)->operation.START_MESSG = strdup(CONFIG_START_MESSG);
    else
        OLC_CONFIG(d)->operation.START_MESSG = NULL;

    cedit_disp_menu(d);
}

static void cedit_save_internally(struct descriptor_data *d)
{
    /* see if we need to reassign spec procs on rooms */
    int reassign = (CONFIG_DTS_ARE_DUMPS != OLC_CONFIG(d)->play.dts_are_dumps);
    /* Copy the data back from the descriptor to the config_info structure. */
    CONFIG_PK_ALLOWED = OLC_CONFIG(d)->play.pk_allowed;
    CONFIG_PT_ALLOWED = OLC_CONFIG(d)->play.pt_allowed;
    CONFIG_FIT_EVOLVE = OLC_CONFIG(d)->play.fit_evolve;
    CONFIG_WEATHER_AFFECTS_SPELLS = OLC_CONFIG(d)->play.weather_affects_spells;
    CONFIG_SCHOOL_WEATHER_AFFECTS = OLC_CONFIG(d)->play.school_weather_affects;
    CONFIG_MAX_PATHFIND_ITERATIONS = OLC_CONFIG(d)->play.max_pathfind_iterations;
    CONFIG_MAX_ZONE_PATH = OLC_CONFIG(d)->play.max_zone_path;
    CONFIG_LEVEL_CAN_SHOUT = OLC_CONFIG(d)->play.level_can_shout;
    CONFIG_HOLLER_MOVE_COST = OLC_CONFIG(d)->play.holler_move_cost;
    CONFIG_TUNNEL_SIZE = OLC_CONFIG(d)->play.tunnel_size;
    CONFIG_MAX_EXP_GAIN = OLC_CONFIG(d)->play.max_exp_gain;
    CONFIG_MAX_EXP_LOSS = OLC_CONFIG(d)->play.max_exp_loss;
    CONFIG_MAX_NPC_CORPSE_TIME = OLC_CONFIG(d)->play.max_npc_corpse_time;
    CONFIG_MAX_PC_CORPSE_TIME = OLC_CONFIG(d)->play.max_pc_corpse_time;
    CONFIG_IDLE_VOID = OLC_CONFIG(d)->play.idle_void;
    CONFIG_IDLE_RENT_TIME = OLC_CONFIG(d)->play.idle_rent_time;
    CONFIG_IDLE_MAX_LEVEL = OLC_CONFIG(d)->play.idle_max_level;
    CONFIG_DTS_ARE_DUMPS = OLC_CONFIG(d)->play.dts_are_dumps;
    CONFIG_LOAD_INVENTORY = OLC_CONFIG(d)->play.load_into_inventory;
    CONFIG_TRACK_T_DOORS = OLC_CONFIG(d)->play.track_through_doors;
    CONFIG_NO_MORT_TO_IMMORT = OLC_CONFIG(d)->play.no_mort_to_immort;
    CONFIG_DISP_CLOSED_DOORS = OLC_CONFIG(d)->play.disp_closed_doors;
    CONFIG_DIAGONAL_DIRS = OLC_CONFIG(d)->play.diagonal_dirs;
    CONFIG_MAP = OLC_CONFIG(d)->play.map_option;
    CONFIG_MAP_SIZE = OLC_CONFIG(d)->play.map_size;
    CONFIG_MINIMAP_SIZE = OLC_CONFIG(d)->play.minimap_size;
    CONFIG_SCRIPT_PLAYERS = OLC_CONFIG(d)->play.script_players;
    CONFIG_MAX_HOUSE_OBJS = OLC_CONFIG(d)->play.max_house_objs;

    /* Crash Saves */
    CONFIG_FREE_RENT = OLC_CONFIG(d)->csd.free_rent;
    CONFIG_MAX_OBJ_SAVE = OLC_CONFIG(d)->csd.max_obj_save;
    CONFIG_MIN_RENT_COST = OLC_CONFIG(d)->csd.min_rent_cost;
    CONFIG_AUTO_SAVE = OLC_CONFIG(d)->csd.auto_save;
    CONFIG_AUTOSAVE_TIME = OLC_CONFIG(d)->csd.autosave_time;
    CONFIG_CRASH_TIMEOUT = OLC_CONFIG(d)->csd.crash_file_timeout;
    CONFIG_RENT_TIMEOUT = OLC_CONFIG(d)->csd.rent_file_timeout;

    /* Room Numbers */
    CONFIG_NEWBIE_START = OLC_CONFIG(d)->room_nums.newbie_start_room;
    CONFIG_IMMORTAL_START = OLC_CONFIG(d)->room_nums.immort_start_room;
    CONFIG_FROZEN_START = OLC_CONFIG(d)->room_nums.frozen_start_room;
    CONFIG_DON_ROOM_1 = OLC_CONFIG(d)->room_nums.donation_room_1;
    CONFIG_DON_ROOM_2 = OLC_CONFIG(d)->room_nums.donation_room_2;
    CONFIG_DON_ROOM_3 = OLC_CONFIG(d)->room_nums.donation_room_3;
    CONFIG_DON_ROOM_4 = OLC_CONFIG(d)->room_nums.donation_room_4;
    CONFIG_DEAD_START = OLC_CONFIG(d)->room_nums.dead_start_room;
    CONFIG_HOMETOWN_1 = OLC_CONFIG(d)->room_nums.hometown_1;
    CONFIG_HOMETOWN_2 = OLC_CONFIG(d)->room_nums.hometown_2;
    CONFIG_HOMETOWN_3 = OLC_CONFIG(d)->room_nums.hometown_3;
    CONFIG_HOMETOWN_4 = OLC_CONFIG(d)->room_nums.hometown_4;
    CONFIG_RESS_ROOM_1 = OLC_CONFIG(d)->room_nums.ress_room_1;
    CONFIG_RESS_ROOM_2 = OLC_CONFIG(d)->room_nums.ress_room_2;
    CONFIG_RESS_ROOM_3 = OLC_CONFIG(d)->room_nums.ress_room_3;
    CONFIG_RESS_ROOM_4 = OLC_CONFIG(d)->room_nums.ress_room_4;

    /* Game Operation */
    CONFIG_DFLT_PORT = OLC_CONFIG(d)->operation.DFLT_PORT;
    CONFIG_MAX_PLAYING = OLC_CONFIG(d)->operation.max_playing;
    CONFIG_MAX_FILESIZE = OLC_CONFIG(d)->operation.max_filesize;
    CONFIG_MAX_BAD_PWS = OLC_CONFIG(d)->operation.max_bad_pws;
    CONFIG_SITEOK_ALL = OLC_CONFIG(d)->operation.siteok_everyone;
    CONFIG_NEW_SOCIALS = OLC_CONFIG(d)->operation.use_new_socials;
    CONFIG_NS_IS_SLOW = OLC_CONFIG(d)->operation.nameserver_is_slow;
    CONFIG_OLC_SAVE = OLC_CONFIG(d)->operation.auto_save_olc;
    CONFIG_MEDIT_ADVANCED = OLC_CONFIG(d)->operation.medit_advanced;
    CONFIG_IBT_AUTOSAVE = OLC_CONFIG(d)->operation.ibt_autosave;
    CONFIG_PROTOCOL_NEGOTIATION = OLC_CONFIG(d)->operation.protocol_negotiation;
    CONFIG_SPECIAL_IN_COMM = OLC_CONFIG(d)->operation.special_in_comm;
    CONFIG_DEBUG_MODE = OLC_CONFIG(d)->operation.debug_mode;

    /* Autowiz */
    CONFIG_USE_AUTOWIZ = OLC_CONFIG(d)->autowiz.use_autowiz;
    CONFIG_MIN_WIZLIST_LEV = OLC_CONFIG(d)->autowiz.min_wizlist_lev;

    /* Experimental Features */
    CONFIG_NEW_AUCTION_SYSTEM = OLC_CONFIG(d)->experimental.new_auction_system;
    CONFIG_EXPERIMENTAL_BANK_SYSTEM = OLC_CONFIG(d)->experimental.experimental_bank_system;
    CONFIG_MOB_CONTEXTUAL_SOCIALS = OLC_CONFIG(d)->experimental.mob_contextual_socials;
    CONFIG_DYNAMIC_REPUTATION = OLC_CONFIG(d)->experimental.dynamic_reputation;
    CONFIG_MOB_EMOTION_SOCIAL_CHANCE = OLC_CONFIG(d)->experimental.mob_emotion_social_chance;
    CONFIG_MOB_EMOTION_UPDATE_CHANCE = OLC_CONFIG(d)->experimental.mob_emotion_update_chance;
    CONFIG_WEATHER_AFFECTS_EMOTIONS = OLC_CONFIG(d)->experimental.weather_affects_emotions;
    CONFIG_WEATHER_EFFECT_MULTIPLIER = OLC_CONFIG(d)->experimental.weather_effect_multiplier;

    /* Emotion System Configuration */
    /* Visual indicator thresholds */
    CONFIG_EMOTION_DISPLAY_FEAR_THRESHOLD = OLC_CONFIG(d)->emotion_config.display_fear_threshold;
    CONFIG_EMOTION_DISPLAY_ANGER_THRESHOLD = OLC_CONFIG(d)->emotion_config.display_anger_threshold;
    CONFIG_EMOTION_DISPLAY_HAPPINESS_THRESHOLD = OLC_CONFIG(d)->emotion_config.display_happiness_threshold;
    CONFIG_EMOTION_DISPLAY_SADNESS_THRESHOLD = OLC_CONFIG(d)->emotion_config.display_sadness_threshold;
    CONFIG_EMOTION_DISPLAY_HORROR_THRESHOLD = OLC_CONFIG(d)->emotion_config.display_horror_threshold;
    CONFIG_EMOTION_DISPLAY_PAIN_THRESHOLD = OLC_CONFIG(d)->emotion_config.display_pain_threshold;

    /* Combat flee behavior thresholds */
    CONFIG_EMOTION_FLEE_FEAR_LOW_THRESHOLD = OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold;
    CONFIG_EMOTION_FLEE_FEAR_HIGH_THRESHOLD = OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold;
    CONFIG_EMOTION_FLEE_COURAGE_LOW_THRESHOLD = OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold;
    CONFIG_EMOTION_FLEE_COURAGE_HIGH_THRESHOLD = OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold;
    CONFIG_EMOTION_FLEE_HORROR_THRESHOLD = OLC_CONFIG(d)->emotion_config.flee_horror_threshold;

    /* Flee modifier values */
    CONFIG_EMOTION_FLEE_FEAR_LOW_MODIFIER = OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier;
    CONFIG_EMOTION_FLEE_FEAR_HIGH_MODIFIER = OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier;
    CONFIG_EMOTION_FLEE_COURAGE_LOW_MODIFIER = OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier;
    CONFIG_EMOTION_FLEE_COURAGE_HIGH_MODIFIER = OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier;
    CONFIG_EMOTION_FLEE_HORROR_MODIFIER = OLC_CONFIG(d)->emotion_config.flee_horror_modifier;

    /* Pain system thresholds and values */
    CONFIG_EMOTION_PAIN_DAMAGE_MINOR_THRESHOLD = OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold;
    CONFIG_EMOTION_PAIN_DAMAGE_MODERATE_THRESHOLD = OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold;
    CONFIG_EMOTION_PAIN_DAMAGE_HEAVY_THRESHOLD = OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold;
    CONFIG_EMOTION_PAIN_DAMAGE_MASSIVE_THRESHOLD = OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold;

    CONFIG_EMOTION_PAIN_MINOR_MIN = OLC_CONFIG(d)->emotion_config.pain_minor_min;
    CONFIG_EMOTION_PAIN_MINOR_MAX = OLC_CONFIG(d)->emotion_config.pain_minor_max;
    CONFIG_EMOTION_PAIN_MODERATE_MIN = OLC_CONFIG(d)->emotion_config.pain_moderate_min;
    CONFIG_EMOTION_PAIN_MODERATE_MAX = OLC_CONFIG(d)->emotion_config.pain_moderate_max;
    CONFIG_EMOTION_PAIN_HEAVY_MIN = OLC_CONFIG(d)->emotion_config.pain_heavy_min;
    CONFIG_EMOTION_PAIN_HEAVY_MAX = OLC_CONFIG(d)->emotion_config.pain_heavy_max;
    CONFIG_EMOTION_PAIN_MASSIVE_MIN = OLC_CONFIG(d)->emotion_config.pain_massive_min;
    CONFIG_EMOTION_PAIN_MASSIVE_MAX = OLC_CONFIG(d)->emotion_config.pain_massive_max;

    /* Memory system weights and thresholds */
    CONFIG_EMOTION_MEMORY_WEIGHT_RECENT = OLC_CONFIG(d)->emotion_config.memory_weight_recent;
    CONFIG_EMOTION_MEMORY_WEIGHT_FRESH = OLC_CONFIG(d)->emotion_config.memory_weight_fresh;
    CONFIG_EMOTION_MEMORY_WEIGHT_MODERATE = OLC_CONFIG(d)->emotion_config.memory_weight_moderate;
    CONFIG_EMOTION_MEMORY_WEIGHT_OLD = OLC_CONFIG(d)->emotion_config.memory_weight_old;
    CONFIG_EMOTION_MEMORY_WEIGHT_ANCIENT = OLC_CONFIG(d)->emotion_config.memory_weight_ancient;

    CONFIG_EMOTION_MEMORY_AGE_RECENT = OLC_CONFIG(d)->emotion_config.memory_age_recent;
    CONFIG_EMOTION_MEMORY_AGE_FRESH = OLC_CONFIG(d)->emotion_config.memory_age_fresh;
    CONFIG_EMOTION_MEMORY_AGE_MODERATE = OLC_CONFIG(d)->emotion_config.memory_age_moderate;
    CONFIG_EMOTION_MEMORY_AGE_OLD = OLC_CONFIG(d)->emotion_config.memory_age_old;

    CONFIG_EMOTION_MEMORY_BASELINE_OFFSET = OLC_CONFIG(d)->emotion_config.memory_baseline_offset;

    /* Allocate space for the strings. */
    if (CONFIG_OK)
        free(CONFIG_OK);
    CONFIG_OK = str_udup(OLC_CONFIG(d)->play.OK);

    if (CONFIG_HUH)
        free(CONFIG_HUH);
    CONFIG_HUH = str_udup(OLC_CONFIG(d)->play.HUH);

    if (CONFIG_NOPERSON)
        free(CONFIG_NOPERSON);
    CONFIG_NOPERSON = str_udup(OLC_CONFIG(d)->play.NOPERSON);

    if (CONFIG_NOEFFECT)
        free(CONFIG_NOEFFECT);
    CONFIG_NOEFFECT = str_udup(OLC_CONFIG(d)->play.NOEFFECT);

    if (CONFIG_DFLT_IP)
        free(CONFIG_DFLT_IP);
    if (OLC_CONFIG(d)->operation.DFLT_IP)
        CONFIG_DFLT_IP = strdup(OLC_CONFIG(d)->operation.DFLT_IP);
    else
        CONFIG_DFLT_IP = NULL;

    if (CONFIG_DFLT_DIR)
        free(CONFIG_DFLT_DIR);
    if (OLC_CONFIG(d)->operation.DFLT_DIR)
        CONFIG_DFLT_DIR = strdup(OLC_CONFIG(d)->operation.DFLT_DIR);
    else
        CONFIG_DFLT_DIR = NULL;

    if (CONFIG_LOGNAME)
        free(CONFIG_LOGNAME);
    if (OLC_CONFIG(d)->operation.LOGNAME)
        CONFIG_LOGNAME = strdup(OLC_CONFIG(d)->operation.LOGNAME);
    else
        CONFIG_LOGNAME = NULL;

    if (CONFIG_MENU)
        free(CONFIG_MENU);
    if (OLC_CONFIG(d)->operation.MENU)
        CONFIG_MENU = strdup(OLC_CONFIG(d)->operation.MENU);
    else
        CONFIG_MENU = NULL;

    if (CONFIG_WELC_MESSG)
        free(CONFIG_WELC_MESSG);
    if (OLC_CONFIG(d)->operation.WELC_MESSG)
        CONFIG_WELC_MESSG = strdup(OLC_CONFIG(d)->operation.WELC_MESSG);
    else
        CONFIG_WELC_MESSG = NULL;

    if (CONFIG_START_MESSG)
        free(CONFIG_START_MESSG);
    if (OLC_CONFIG(d)->operation.START_MESSG)
        CONFIG_START_MESSG = strdup(OLC_CONFIG(d)->operation.START_MESSG);
    else
        CONFIG_START_MESSG = NULL;

    /* if we changed the dts to/from dumps, reassign - Welcor */
    if (reassign)
        reassign_rooms();

    add_to_save_list(NOWHERE, SL_CFG);
}

void cedit_save_to_disk(void)
{
    /* Just call save_config and get it over with. */
    save_config(NOWHERE);
}

int save_config(IDXTYPE nowhere)
{
    FILE *fl;
    char buf[MAX_STRING_LENGTH];

    if (!(fl = fopen(CONFIG_CONFFILE, "w"))) {
        perror("SYSERR: save_config");
        return (FALSE);
    }

    fprintf(fl,
            "* This file is autogenerated by OasisOLC (CEdit).\n"
            "* Please note the following information about this file's format.\n"
            "*\n"
            "* - If variable is a yes/no or true/false based variable, use 1's and 0's\n"
            "*   where YES or TRUE = 1 and NO or FALSE = 0.\n"
            "* - Variable names in this file are case-insensitive.  Variable values\n"
            "*   are not case-insensitive.\n"
            "* -----------------------------------------------------------------------\n"
            "* Lines starting with * are comments, and are not parsed.\n"
            "* -----------------------------------------------------------------------\n\n"
            "* [ Game Play Options ]\n");

    fprintf(fl,
            "* Is player killing allowed on the mud?\n"
            "pk_allowed = %d\n\n",
            CONFIG_PK_ALLOWED);
    fprintf(fl,
            "* Is player thieving allowed on the mud?\n"
            "pt_allowed = %d\n\n",
            CONFIG_PT_ALLOWED);
    fprintf(fl,
            "* What is the minimum level a player can shout/gossip/etc?\n"
            "level_can_shout = %d\n\n",
            CONFIG_LEVEL_CAN_SHOUT);
    fprintf(fl,
            "* How many movement points does shouting cost the player?\n"
            "holler_move_cost = %d\n\n",
            CONFIG_HOLLER_MOVE_COST);
    fprintf(fl,
            "* How many players can fit in a tunnel?\n"
            "tunnel_size = %d\n\n",
            CONFIG_TUNNEL_SIZE);
    fprintf(fl,
            "* Maximum experience gainable per kill?\n"
            "max_exp_gain = %d\n\n",
            CONFIG_MAX_EXP_GAIN);
    fprintf(fl,
            "* Maximum experience loseable per death?\n"
            "max_exp_loss = %d\n\n",
            CONFIG_MAX_EXP_LOSS);
    fprintf(fl,
            "* Number of tics before NPC corpses decompose.\n"
            "max_npc_corpse_time = %d\n\n",
            CONFIG_MAX_NPC_CORPSE_TIME);
    fprintf(fl,
            "* Number of tics before PC corpses decompose.\n"
            "max_pc_corpse_time = %d\n\n",
            CONFIG_MAX_PC_CORPSE_TIME);
    fprintf(fl,
            "* Number of tics before a PC is sent to the void.\n"
            "idle_void = %d\n\n",
            CONFIG_IDLE_VOID);
    fprintf(fl,
            "* Number of tics before a PC is autorented.\n"
            "idle_rent_time = %d\n\n",
            CONFIG_IDLE_RENT_TIME);
    fprintf(fl,
            "* Level and above of players whom are immune to idle penalties.\n"
            "idle_max_level = %d\n\n",
            CONFIG_IDLE_MAX_LEVEL);
    fprintf(fl,
            "* Should the items in death traps be junked automatically?\n"
            "dts_are_dumps = %d\n\n",
            CONFIG_DTS_ARE_DUMPS);
    fprintf(fl,
            "* When an immortal loads an object, should it load into their inventory?\n"
            "load_into_inventory = %d\n\n",
            CONFIG_LOAD_INVENTORY);
    fprintf(fl,
            "* Should PC's be able to track through hidden or closed doors?\n"
            "track_through_doors = %d\n\n",
            CONFIG_TRACK_T_DOORS);
    fprintf(fl,
            "* Should players who reach enough exp be prevented from automatically levelling to immortal?\n"
            "no_mort_to_immort = %d\n\n",
            CONFIG_NO_MORT_TO_IMMORT);
    fprintf(fl,
            "* Should closed doors be shown on autoexit / exit?\n"
            "disp_closed_doors = %d\n\n",
            CONFIG_DISP_CLOSED_DOORS);
    fprintf(fl,
            "* Are diagonal directions enabled?\n"
            "diagonal_dirs = %d\n\n",
            CONFIG_DIAGONAL_DIRS);
    fprintf(fl,
            "* Who can use the map functions? 0=off, 1=on, 2=imm_only\n"
            "map_option = %d\n\n",
            CONFIG_MAP);
    fprintf(fl,
            "* Default size of map shown by 'map' command\n"
            "default_map_size = %d\n\n",
            CONFIG_MAP_SIZE);
    fprintf(fl,
            "* Default minimap size shown to the right of room descriptions\n"
            "default_minimap_size = %d\n\n",
            CONFIG_MINIMAP_SIZE);
    fprintf(fl,
            "* Do you want scripts to be attachable to players?\n"
            "script_players = %d\n\n",
            CONFIG_SCRIPT_PLAYERS);
    fprintf(fl,
            "* Does weather affect spell effectiveness?\n"
            "weather_affects_spells = %d\n\n",
            CONFIG_WEATHER_AFFECTS_SPELLS);
    fprintf(fl,
            "* Does weather affect spells based on school?\n"
            "school_weather_affects = %d\n\n",
            CONFIG_SCHOOL_WEATHER_AFFECTS);
    fprintf(fl,
            "* Maximum iterations for advanced pathfinding (0=dynamic scaling)\n"
            "max_pathfind_iterations = %d\n\n",
            CONFIG_MAX_PATHFIND_ITERATIONS);
    fprintf(fl,
            "* Maximum zones in a pathfinding path (0=dynamic scaling)\n"
            "max_zone_path = %d\n\n",
            CONFIG_MAX_ZONE_PATH);
    fprintf(fl,
            "* Maximum objects allowed in player houses (0=unlimited)\n"
            "max_house_objs = %d\n\n",
            CONFIG_MAX_HOUSE_OBJS);

    strcpy(buf, CONFIG_OK);
    strip_cr(buf);

    fprintf(fl,
            "* Text sent to players when OK is all that is needed.\n"
            "ok = %s\n\n",
            buf);

    strcpy(buf, CONFIG_HUH);
    strip_cr(buf);

    fprintf(fl,
            "* Text sent to players for an unrecognized command.\n"
            "huh = %s\n\n",
            buf);

    strcpy(buf, CONFIG_NOPERSON);
    strip_cr(buf);

    fprintf(fl,
            "* Text sent to players when noone is available.\n"
            "noperson = %s\n\n",
            buf);

    strcpy(buf, CONFIG_NOEFFECT);
    strip_cr(buf);

    fprintf(fl,
            "* Text sent to players when an effect fails.\n"
            "noeffect = %s\n",
            buf);

    /* RENT / CRASHSAVE OPTIONS */
    fprintf(fl, "\n\n\n* [ Rent/Crashsave Options ]\n");

    fprintf(fl,
            "* Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,\n"
            "* your objects are saved at no cost, as in Merc-type MUDs.)\n"
            "free_rent = %d\n\n",
            CONFIG_FREE_RENT);

    fprintf(fl,
            "* Maximum number of items players are allowed to rent.\n"
            "max_obj_save = %d\n\n",
            CONFIG_MAX_OBJ_SAVE);

    fprintf(fl,
            "* Should the game automatically save people?\n"
            "auto_save = %d\n\n",
            CONFIG_AUTO_SAVE);

    fprintf(fl,
            "* If auto_save = 1, how often (in minutes) should the game save people's objects?\n"
            "autosave_time = %d\n\n",
            CONFIG_AUTOSAVE_TIME);

    fprintf(fl,
            "* Lifetime of crashfiles and force-rent (idlesave) files in days.\n"
            "crash_file_timeout = %d\n\n",
            CONFIG_CRASH_TIMEOUT);

    fprintf(fl,
            "* Lifetime of normal rent files in days.\n"
            "rent_file_timeout = %d\n\n",
            CONFIG_RENT_TIMEOUT);

    /* ROOM NUMBERS */
    fprintf(fl, "\n\n\n* [ Room Numbers ]\n");

    fprintf(fl,
            "* The virtual number of the room that mortals should enter at.\n"
            "newbie_start_room = %d\n\n",
            CONFIG_NEWBIE_START);

    fprintf(fl,
            "* The virtual number of the room that immorts should enter at.\n"
            "immort_start_room = %d\n\n",
            CONFIG_IMMORTAL_START);

    fprintf(fl,
            "* The virtual number of the room that frozen people should enter at.\n"
            "frozen_start_room = %d\n\n",
            CONFIG_FROZEN_START);
    fprintf(fl,
            "* The virtual number of the room that dead people should enter at.\n"
            "dead_start_room = %d\n\n",
            CONFIG_DEAD_START);

    fprintf(
        fl,
        "* The virtual numbers of the donation rooms.  Note: Add donation rooms\n"
        "* sequentially (1 & 2 before 3, 3 before 4). If you don't, you might not be able to\n"
        "* donate. Use -1 for 'no such room'.\n"
        "donation_room_1 = %d\n"
        "donation_room_2 = %d\n"
        "donation_room_3 = %d\n"
        "donation_room_4 = %d\n\n",
        CONFIG_DON_ROOM_1 != NOWHERE ? CONFIG_DON_ROOM_1 : -1, CONFIG_DON_ROOM_2 != NOWHERE ? CONFIG_DON_ROOM_2 : -1,
        CONFIG_DON_ROOM_3 != NOWHERE ? CONFIG_DON_ROOM_3 : -1, CONFIG_DON_ROOM_4 != NOWHERE ? CONFIG_DON_ROOM_4 : -1);

    fprintf(
        fl,
        "* The virtual numbers of the hometowns rooms. Use -1 for 'no such room'.\n"
        "hometown_1 = %d\n"
        "hometown_2 = %d\n"
        "hometown_3 = %d\n"
        "hometown_4 = %d\n",
        CONFIG_HOMETOWN_1 != NOWHERE ? CONFIG_HOMETOWN_1 : -1, CONFIG_HOMETOWN_2 != NOWHERE ? CONFIG_HOMETOWN_2 : -1,
        CONFIG_HOMETOWN_3 != NOWHERE ? CONFIG_HOMETOWN_3 : -1, CONFIG_HOMETOWN_4 != NOWHERE ? CONFIG_HOMETOWN_4 : -1);

    fprintf(fl,
            "* The virtual numbers of the corpses rooms. Use -1 for 'no such room'.\n"
            "ress_room_1 = %d\n"
            "ress_room_2 = %d\n"
            "ress_room_3 = %d\n"
            "ress_room_4 = %d\n",
            CONFIG_RESS_ROOM_1 != NOWHERE ? CONFIG_RESS_ROOM_1 : -1,
            CONFIG_RESS_ROOM_2 != NOWHERE ? CONFIG_RESS_ROOM_2 : -1,
            CONFIG_RESS_ROOM_3 != NOWHERE ? CONFIG_RESS_ROOM_3 : -1,
            CONFIG_RESS_ROOM_4 != NOWHERE ? CONFIG_RESS_ROOM_4 : -1);

    fprintf(fl, "\n\n\n* [ Game Operation Options ]\n");

    fprintf(fl,
            "* This is the default port on which the game should run if no port is\n"
            "* given on the command-line.  NOTE WELL: If you're using the\n"
            "* 'autorun' script, the port number there will override this setting.\n"
            "* Change the PORT= line in autorun instead of (or in addition to)\n"
            "* changing this.\n"
            "DFLT_PORT = %d\n\n",
            CONFIG_DFLT_PORT);

    if (CONFIG_DFLT_IP) {
        strcpy(buf, CONFIG_DFLT_IP);
        strip_cr(buf);

        fprintf(fl, "* IP address to which the MUD should bind.\nDFLT_IP = %s\n\n", buf);
    }

    if (CONFIG_DFLT_DIR) {
        strcpy(buf, CONFIG_DFLT_DIR);
        strip_cr(buf);

        fprintf(fl,
                "* default directory to use as data directory.\n"
                "DFLT_DIR = %s\n\n",
                buf);
    }

    if (CONFIG_LOGNAME) {
        strcpy(buf, CONFIG_LOGNAME);
        strip_cr(buf);

        fprintf(fl,
                "* What file to log messages to (ex: 'log/syslog').\n"
                "LOGNAME = %s\n\n",
                buf);
    }

    fprintf(fl,
            "* Maximum number of players allowed before game starts to turn people away.\n"
            "max_playing = %d\n\n",
            CONFIG_MAX_PLAYING);

    fprintf(fl,
            "* Maximum size of bug, typo, and idea files in bytes (to prevent bombing).\n"
            "max_filesize = %d\n\n",
            CONFIG_MAX_FILESIZE);

    fprintf(fl,
            "* Maximum number of password attempts before disconnection.\n"
            "max_bad_pws = %d\n\n",
            CONFIG_MAX_BAD_PWS);

    fprintf(fl,
            "* Is the site ok for everyone except those that are banned?\n"
            "siteok_everyone = %d\n\n",
            CONFIG_SITEOK_ALL);

    fprintf(fl,
            "* If you want to use the original social file format\n"
            "* and disable Aedit, set to 0, otherwise, 1.\n"
            "use_new_socials = %d\n\n",
            CONFIG_NEW_SOCIALS);

    fprintf(fl,
            "* If the nameserver is fast, set to 0, otherwise, 1.\n"
            "nameserver_is_slow = %d\n\n",
            CONFIG_NS_IS_SLOW);

    fprintf(fl,
            "* Should OLC autosave to disk (1) or save internally (0).\n"
            "auto_save_olc = %d\n\n",
            CONFIG_OLC_SAVE);

    if (CONFIG_MENU) {
        strcpy(buf, CONFIG_MENU);
        strip_cr(buf);

        fprintf(fl,
                "* The entrance/exit menu.\n"
                "MENU = \n%s~\n\n",
                convert_from_tabs(buf));
    }

    if (CONFIG_WELC_MESSG) {
        strcpy(buf, CONFIG_WELC_MESSG);
        strip_cr(buf);

        fprintf(fl, "* The welcome message.\nWELC_MESSG = \n%s~\n\n", convert_from_tabs(buf));
    }

    if (CONFIG_START_MESSG) {
        strcpy(buf, CONFIG_START_MESSG);
        strip_cr(buf);

        fprintf(fl,
                "* NEWBIE start message.\n"
                "START_MESSG = \n%s~\n\n",
                convert_from_tabs(buf));
    }

    fprintf(fl,
            "* Should the medit OLC show the advanced stats menu (1) or not (0).\n"
            "medit_advanced_stats = %d\n\n",
            CONFIG_MEDIT_ADVANCED);

    fprintf(fl,
            "* Should the idea, bug and typo commands autosave (1) or not (0).\n"
            "ibt_autosave = %d\n\n",
            CONFIG_IBT_AUTOSAVE);

    fprintf(fl, "\n\n\n* [ Autowiz Options ]\n");

    fprintf(fl,
            "* Should the game automatically create a new wizlist/immlist every time\n"
            "* someone immorts, or is promoted to a higher (or lower) god level?\n"
            "use_autowiz = %d\n\n",
            CONFIG_USE_AUTOWIZ);

    fprintf(fl,
            "* If yes, what is the lowest level which should be on the wizlist?\n"
            "min_wizlist_lev = %d\n\n",
            CONFIG_MIN_WIZLIST_LEV);

    fprintf(fl,
            "* If yes, enable the protocol negotiation system.\n"
            "protocol_negotiation = %d\n\n",
            CONFIG_PROTOCOL_NEGOTIATION);

    fprintf(fl,
            "* If yes, enable the special character in comm channels.\n"
            "special_in_comm = %d\n\n",
            CONFIG_SPECIAL_IN_COMM);

    fprintf(fl,
            "* If 0 then off, otherwise 1: Brief, 2: Normal, 3: Complete.\n"
            "debug_mode = %d\n\n",
            CONFIG_DEBUG_MODE);

    fprintf(fl, "\n\n\n* [ Experimental Features ]\n");

    fprintf(fl,
            "* Enable the new auction system?\n"
            "new_auction_system = %d\n\n",
            CONFIG_NEW_AUCTION_SYSTEM);

    fprintf(fl,
            "* Enable the experimental bank system?\n"
            "experimental_bank_system = %d\n\n",
            CONFIG_EXPERIMENTAL_BANK_SYSTEM);

    fprintf(fl,
            "* Enable mob contextual socials (reputation/alignment/position based)?\n"
            "mob_contextual_socials = %d\n\n",
            CONFIG_MOB_CONTEXTUAL_SOCIALS);

    fprintf(fl,
            "* Enable dynamic reputation system (combat, healing, giving, stealing, etc. - excludes quests)?\n"
            "dynamic_reputation = %d\n\n",
            CONFIG_DYNAMIC_REPUTATION);

    fprintf(fl,
            "* Probability (%%) of mob performing social per emotion tick (4 seconds)\n"
            "mob_emotion_social_chance = %d\n\n",
            CONFIG_MOB_EMOTION_SOCIAL_CHANCE);

    fprintf(fl,
            "* Probability (%%) of mob updating emotions per emotion tick (4 seconds)\n"
            "mob_emotion_update_chance = %d\n\n",
            CONFIG_MOB_EMOTION_UPDATE_CHANCE);

    fprintf(fl,
            "* Enable weather affects on mob emotions?\n"
            "weather_affects_emotions = %d\n\n",
            CONFIG_WEATHER_AFFECTS_EMOTIONS);

    fprintf(fl,
            "* Weather emotion effect multiplier (0-200%%, default 100)\n"
            "weather_effect_multiplier = %d\n\n",
            CONFIG_WEATHER_EFFECT_MULTIPLIER);

    fprintf(fl, "\n\n* [ Emotion System Configuration ]\n");

    fprintf(fl, "\n* Visual Indicator Thresholds (0-100)\n");
    fprintf(fl, "emotion_display_fear_threshold = %d\n", CONFIG_EMOTION_DISPLAY_FEAR_THRESHOLD);
    fprintf(fl, "emotion_display_anger_threshold = %d\n", CONFIG_EMOTION_DISPLAY_ANGER_THRESHOLD);
    fprintf(fl, "emotion_display_happiness_threshold = %d\n", CONFIG_EMOTION_DISPLAY_HAPPINESS_THRESHOLD);
    fprintf(fl, "emotion_display_sadness_threshold = %d\n", CONFIG_EMOTION_DISPLAY_SADNESS_THRESHOLD);
    fprintf(fl, "emotion_display_horror_threshold = %d\n", CONFIG_EMOTION_DISPLAY_HORROR_THRESHOLD);
    fprintf(fl, "emotion_display_pain_threshold = %d\n\n", CONFIG_EMOTION_DISPLAY_PAIN_THRESHOLD);

    fprintf(fl, "* Combat Flee Behavior Thresholds (0-100)\n");
    fprintf(fl, "emotion_flee_fear_low_threshold = %d\n", CONFIG_EMOTION_FLEE_FEAR_LOW_THRESHOLD);
    fprintf(fl, "emotion_flee_fear_high_threshold = %d\n", CONFIG_EMOTION_FLEE_FEAR_HIGH_THRESHOLD);
    fprintf(fl, "emotion_flee_courage_low_threshold = %d\n", CONFIG_EMOTION_FLEE_COURAGE_LOW_THRESHOLD);
    fprintf(fl, "emotion_flee_courage_high_threshold = %d\n", CONFIG_EMOTION_FLEE_COURAGE_HIGH_THRESHOLD);
    fprintf(fl, "emotion_flee_horror_threshold = %d\n\n", CONFIG_EMOTION_FLEE_HORROR_THRESHOLD);

    fprintf(fl, "* Combat Flee Behavior Modifiers (-100 to +100)\n");
    fprintf(fl, "emotion_flee_fear_low_modifier = %d\n", CONFIG_EMOTION_FLEE_FEAR_LOW_MODIFIER);
    fprintf(fl, "emotion_flee_fear_high_modifier = %d\n", CONFIG_EMOTION_FLEE_FEAR_HIGH_MODIFIER);
    fprintf(fl, "emotion_flee_courage_low_modifier = %d\n", CONFIG_EMOTION_FLEE_COURAGE_LOW_MODIFIER);
    fprintf(fl, "emotion_flee_courage_high_modifier = %d\n", CONFIG_EMOTION_FLEE_COURAGE_HIGH_MODIFIER);
    fprintf(fl, "emotion_flee_horror_modifier = %d\n\n", CONFIG_EMOTION_FLEE_HORROR_MODIFIER);

    fprintf(fl, "* Pain System Damage Thresholds (%% of max HP)\n");
    fprintf(fl, "emotion_pain_damage_minor_threshold = %d\n", CONFIG_EMOTION_PAIN_DAMAGE_MINOR_THRESHOLD);
    fprintf(fl, "emotion_pain_damage_moderate_threshold = %d\n", CONFIG_EMOTION_PAIN_DAMAGE_MODERATE_THRESHOLD);
    fprintf(fl, "emotion_pain_damage_heavy_threshold = %d\n", CONFIG_EMOTION_PAIN_DAMAGE_HEAVY_THRESHOLD);
    fprintf(fl, "emotion_pain_damage_massive_threshold = %d\n\n", CONFIG_EMOTION_PAIN_DAMAGE_MASSIVE_THRESHOLD);

    fprintf(fl, "* Pain System Pain Amounts (0-100)\n");
    fprintf(fl, "emotion_pain_minor_min = %d\n", CONFIG_EMOTION_PAIN_MINOR_MIN);
    fprintf(fl, "emotion_pain_minor_max = %d\n", CONFIG_EMOTION_PAIN_MINOR_MAX);
    fprintf(fl, "emotion_pain_moderate_min = %d\n", CONFIG_EMOTION_PAIN_MODERATE_MIN);
    fprintf(fl, "emotion_pain_moderate_max = %d\n", CONFIG_EMOTION_PAIN_MODERATE_MAX);
    fprintf(fl, "emotion_pain_heavy_min = %d\n", CONFIG_EMOTION_PAIN_HEAVY_MIN);
    fprintf(fl, "emotion_pain_heavy_max = %d\n", CONFIG_EMOTION_PAIN_HEAVY_MAX);
    fprintf(fl, "emotion_pain_massive_min = %d\n", CONFIG_EMOTION_PAIN_MASSIVE_MIN);
    fprintf(fl, "emotion_pain_massive_max = %d\n\n", CONFIG_EMOTION_PAIN_MASSIVE_MAX);

    fprintf(fl, "* Memory System Weights (1-10)\n");
    fprintf(fl, "emotion_memory_weight_recent = %d\n", CONFIG_EMOTION_MEMORY_WEIGHT_RECENT);
    fprintf(fl, "emotion_memory_weight_fresh = %d\n", CONFIG_EMOTION_MEMORY_WEIGHT_FRESH);
    fprintf(fl, "emotion_memory_weight_moderate = %d\n", CONFIG_EMOTION_MEMORY_WEIGHT_MODERATE);
    fprintf(fl, "emotion_memory_weight_old = %d\n", CONFIG_EMOTION_MEMORY_WEIGHT_OLD);
    fprintf(fl, "emotion_memory_weight_ancient = %d\n\n", CONFIG_EMOTION_MEMORY_WEIGHT_ANCIENT);

    fprintf(fl, "* Memory System Age Thresholds (seconds)\n");
    fprintf(fl, "emotion_memory_age_recent = %d\n", CONFIG_EMOTION_MEMORY_AGE_RECENT);
    fprintf(fl, "emotion_memory_age_fresh = %d\n", CONFIG_EMOTION_MEMORY_AGE_FRESH);
    fprintf(fl, "emotion_memory_age_moderate = %d\n", CONFIG_EMOTION_MEMORY_AGE_MODERATE);
    fprintf(fl, "emotion_memory_age_old = %d\n\n", CONFIG_EMOTION_MEMORY_AGE_OLD);

    fprintf(fl, "* Memory System Baseline Offset (0-100)\n");
    fprintf(fl, "emotion_memory_baseline_offset = %d\n\n", CONFIG_EMOTION_MEMORY_BASELINE_OFFSET);

    fprintf(fl, "* Trading Behavior Thresholds (0-100)\n");
    fprintf(fl, "emotion_trade_trust_high_threshold = %d\n", CONFIG_EMOTION_TRADE_TRUST_HIGH_THRESHOLD);
    fprintf(fl, "emotion_trade_trust_low_threshold = %d\n", CONFIG_EMOTION_TRADE_TRUST_LOW_THRESHOLD);
    fprintf(fl, "emotion_trade_greed_high_threshold = %d\n", CONFIG_EMOTION_TRADE_GREED_HIGH_THRESHOLD);
    fprintf(fl, "emotion_trade_friendship_high_threshold = %d\n\n", CONFIG_EMOTION_TRADE_FRIENDSHIP_HIGH_THRESHOLD);

    fprintf(fl, "* Quest Behavior Thresholds (0-100)\n");
    fprintf(fl, "emotion_quest_curiosity_high_threshold = %d\n", CONFIG_EMOTION_QUEST_CURIOSITY_HIGH_THRESHOLD);
    fprintf(fl, "emotion_quest_loyalty_high_threshold = %d\n", CONFIG_EMOTION_QUEST_LOYALTY_HIGH_THRESHOLD);
    fprintf(fl, "emotion_quest_trust_high_threshold = %d\n", CONFIG_EMOTION_QUEST_TRUST_HIGH_THRESHOLD);
    fprintf(fl, "emotion_quest_trust_low_threshold = %d\n\n", CONFIG_EMOTION_QUEST_TRUST_LOW_THRESHOLD);

    fprintf(fl, "* Social Initiation Thresholds (0-100)\n");
    fprintf(fl, "emotion_social_happiness_high_threshold = %d\n", CONFIG_EMOTION_SOCIAL_HAPPINESS_HIGH_THRESHOLD);
    fprintf(fl, "emotion_social_anger_high_threshold = %d\n", CONFIG_EMOTION_SOCIAL_ANGER_HIGH_THRESHOLD);
    fprintf(fl, "emotion_social_sadness_high_threshold = %d\n", CONFIG_EMOTION_SOCIAL_SADNESS_HIGH_THRESHOLD);
    fprintf(fl, "emotion_social_love_follow_threshold = %d\n\n", CONFIG_EMOTION_SOCIAL_LOVE_FOLLOW_THRESHOLD);

    fprintf(fl, "* Group Behavior Thresholds (0-100)\n");
    fprintf(fl, "emotion_group_loyalty_high_threshold = %d\n", CONFIG_EMOTION_GROUP_LOYALTY_HIGH_THRESHOLD);
    fprintf(fl, "emotion_group_loyalty_low_threshold = %d\n", CONFIG_EMOTION_GROUP_LOYALTY_LOW_THRESHOLD);
    fprintf(fl, "emotion_group_friendship_high_threshold = %d\n", CONFIG_EMOTION_GROUP_FRIENDSHIP_HIGH_THRESHOLD);
    fprintf(fl, "emotion_group_envy_high_threshold = %d\n\n", CONFIG_EMOTION_GROUP_ENVY_HIGH_THRESHOLD);

    fclose(fl);

    if (in_save_list(NOWHERE, SL_CFG))
        remove_from_save_list(NOWHERE, SL_CFG);

    return (TRUE);
}

/* Menu functions - The main menu. */
static void cedit_disp_menu(struct descriptor_data *d)
{

    get_char_colors(d->character);
    clear_screen(d);

    /* Menu header. */
    write_to_output(d,
                    "OasisOLC MUD Configuration Editor\r\n"
                    "%sG%s) Game Play Options\r\n"
                    "%sC%s) Crashsave/Rent Options\r\n"
                    "%sR%s) Room Numbers\r\n"
                    "%sO%s) Operation Options\r\n"
                    "%sA%s) Autowiz Options\r\n"
                    "%sX%s) Experimental Features Configuration\r\n"
                    "%sE%s) Emotion System Configuration\r\n"
                    "%sQ%s) Quit\r\n"
                    "Enter your choice : ",
                    grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm);

    OLC_MODE(d) = CEDIT_MAIN_MENU;
}

static void cedit_disp_game_play_options(struct descriptor_data *d)
{
    int m_opt;
    m_opt = OLC_CONFIG(d)->play.map_option;
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(
        d,
        "\r\n\r\n"
        "%sA%s) Player Killing Allowed  : %s%s\r\n"
        "%sB%s) Player Thieving Allowed : %s%s\r\n"
        "%sC%s) Minimum Level To Shout  : %s%d\r\n"
        "%sD%s) Holler Move Cost        : %s%d\r\n"
        "%sE%s) Tunnel Size             : %s%d\r\n"
        "%sF%s) Maximum Experience Gain : %s%d\r\n"
        "%sG%s) Maximum Experience Loss : %s%d\r\n"
        "%sH%s) Max Time for NPC Corpse : %s%d\r\n"
        "%sI%s) Max Time for PC Corpse  : %s%d\r\n"
        "%sJ%s) Tics before PC sent to void : %s%d\r\n"
        "%sK%s) Tics before PC is autosaved : %s%d\r\n"
        "%sL%s) Level Immune To IDLE        : %s%d\r\n"
        "%sM%s) Death Traps Junk Items      : %s%s\r\n"
        "%sN%s) Objects Load Into Inventory : %s%s\r\n"
        "%sO%s) Track Through Doors         : %s%s\r\n"
        "%sP%s) Display Closed Doors        : %s%s\r\n"
        "%sR%s) Diagonal Directions         : %s%s\r\n"
        "%sS%s) Prevent Mortal Level To Immortal : %s%s\r\n"
        "%s1%s) OK Message Text         : %s%s"
        "%s2%s) HUH Message Text        : %s%s"
        "%s3%s) NOPERSON Message Text   : %s%s"
        "%s4%s) NOEFFECT Message Text   : %s%s"
        "%s5%s) Map/Automap Option      : %s%s\r\n"
        "%s6%s) Default map size        : %s%d\r\n"
        "%s7%s) Default minimap size    : %s%d\r\n"
        "%s8%s) Scripts on PC's         : %s%s\r\n"
        "%s9%s) Fit Evolve             : %s%s\r\n"
        "%sZ%s) School Weather Effects  : %s%s\r\n"
        "%s0%s) Weather Affects Spells  : %s%s\r\n"
        "%sX%s) Max Pathfind Iterations : %s%d\r\n"
        "%sY%s) Max Zone Path Length    : %s%d\r\n"
        "%sW%s) Max House Objects       : %s%d\r\n"
        "%sQ%s) Exit To The Main Menu\r\n"
        "Enter your choice : ",
        grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->play.pk_allowed), grn, nrm, cyn,
        CHECK_VAR(OLC_CONFIG(d)->play.pt_allowed), grn, nrm, cyn, OLC_CONFIG(d)->play.level_can_shout, grn, nrm, cyn,
        OLC_CONFIG(d)->play.holler_move_cost, grn, nrm, cyn, OLC_CONFIG(d)->play.tunnel_size, grn, nrm, cyn,
        OLC_CONFIG(d)->play.max_exp_gain, grn, nrm, cyn, OLC_CONFIG(d)->play.max_exp_loss, grn, nrm, cyn,
        OLC_CONFIG(d)->play.max_npc_corpse_time, grn, nrm, cyn, OLC_CONFIG(d)->play.max_pc_corpse_time, grn, nrm, cyn,
        OLC_CONFIG(d)->play.idle_void, grn, nrm, cyn, OLC_CONFIG(d)->play.idle_rent_time, grn, nrm, cyn,
        OLC_CONFIG(d)->play.idle_max_level, grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->play.dts_are_dumps), grn, nrm, cyn,
        CHECK_VAR(OLC_CONFIG(d)->play.load_into_inventory), grn, nrm, cyn,
        CHECK_VAR(OLC_CONFIG(d)->play.track_through_doors), grn, nrm, cyn,
        CHECK_VAR(OLC_CONFIG(d)->play.disp_closed_doors), grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->play.diagonal_dirs),
        grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->play.no_mort_to_immort), grn, nrm, cyn, OLC_CONFIG(d)->play.OK, grn,
        nrm, cyn, OLC_CONFIG(d)->play.HUH, grn, nrm, cyn, OLC_CONFIG(d)->play.NOPERSON, grn, nrm, cyn,
        OLC_CONFIG(d)->play.NOEFFECT, grn, nrm, cyn,
        m_opt == 0 ? "Off" : (m_opt == 1 ? "On" : (m_opt == 2 ? "Imm-Only" : "Invalid!")), grn, nrm, cyn,
        OLC_CONFIG(d)->play.map_size, grn, nrm, cyn, OLC_CONFIG(d)->play.minimap_size, grn, nrm, cyn,
        CHECK_VAR(OLC_CONFIG(d)->play.script_players), grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->play.fit_evolve), grn,
        nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->play.school_weather_affects), grn, nrm, cyn,
        CHECK_VAR(OLC_CONFIG(d)->play.weather_affects_spells), grn, nrm, cyn,
        OLC_CONFIG(d)->play.max_pathfind_iterations, grn, nrm, cyn, OLC_CONFIG(d)->play.max_zone_path, grn, nrm, cyn,
        OLC_CONFIG(d)->play.max_house_objs, grn, nrm),

        OLC_MODE(d) = CEDIT_GAME_OPTIONS_MENU;
}

static void cedit_disp_crash_save_options(struct descriptor_data *d)
{
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(d,
                    "\r\n\r\n"
                    "%sA%s) Free Rent          : %s%s\r\n"
                    "%sB%s) Max Objects Saved  : %s%d\r\n"
                    "%sC%s) Minimum Rent Cost  : %s%d\r\n"
                    "%sD%s) Auto Save          : %s%s\r\n"
                    "%sE%s) Auto Save Time     : %s%d minute(s)\r\n"
                    "%sF%s) Crash File Timeout : %s%d day(s)\r\n"
                    "%sG%s) Rent File Timeout  : %s%d day(s)\r\n"
                    "%sQ%s) Exit To The Main Menu\r\n"
                    "Enter your choice : ",
                    grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->csd.free_rent), grn, nrm, cyn,
                    OLC_CONFIG(d)->csd.max_obj_save, grn, nrm, cyn, OLC_CONFIG(d)->csd.min_rent_cost, grn, nrm, cyn,
                    CHECK_VAR(OLC_CONFIG(d)->csd.auto_save), grn, nrm, cyn, OLC_CONFIG(d)->csd.autosave_time, grn, nrm,
                    cyn, OLC_CONFIG(d)->csd.crash_file_timeout, grn, nrm, cyn, OLC_CONFIG(d)->csd.rent_file_timeout,
                    grn, nrm);

    OLC_MODE(d) = CEDIT_CRASHSAVE_OPTIONS_MENU;
}

static void cedit_disp_room_numbers(struct descriptor_data *d)
{
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(
        d,
        "\r\n\r\n"
        "%sA%s) Newbie Start Room   : %s%d\r\n"
        "%sB%s) Immortal Start Room : %s%d\r\n"
        "%sC%s) Frozen Start Room   : %s%d\r\n"
        "%sD%s) Dead Start Room   : %s%d\r\n"
        "%sE%s) Hometown #1   : %s%d\r\n"
        "%sF%s) Hometown #2   : %s%d\r\n"
        "%sG%s) Hometown #3   : %s%d\r\n"
        "%sH%s) Hometown #4   : %s%d\r\n"
        "%s1%s) Donation Room #1    : %s%d\r\n"
        "%s2%s) Donation Room #2    : %s%d\r\n"
        "%s3%s) Donation Room #3    : %s%d\r\n"
        "%sI%s) Donation Room #4    : %s%d\r\n"
        "%s4%s) Ress Room #1    : %s%d\r\n"
        "%s5%s) Ress Room #2    : %s%d\r\n"
        "%s6%s) Ress Room #3    : %s%d\r\n"
        "%sJ%s) Ress Room #4    : %s%d\r\n"
        "%sQ%s) Exit To The Main Menu\r\n"
        "Enter your choice : ",
        grn, nrm, cyn, OLC_CONFIG(d)->room_nums.newbie_start_room, grn, nrm, cyn,
        OLC_CONFIG(d)->room_nums.immort_start_room, grn, nrm, cyn, OLC_CONFIG(d)->room_nums.frozen_start_room, grn, nrm,
        cyn, OLC_CONFIG(d)->room_nums.dead_start_room, grn, nrm, cyn, OLC_CONFIG(d)->room_nums.hometown_1, grn, nrm,
        cyn, OLC_CONFIG(d)->room_nums.hometown_2, grn, nrm, cyn, OLC_CONFIG(d)->room_nums.hometown_3, grn, nrm, cyn,
        OLC_CONFIG(d)->room_nums.hometown_4, grn, nrm, cyn, OLC_CONFIG(d)->room_nums.donation_room_1, grn, nrm, cyn,
        OLC_CONFIG(d)->room_nums.donation_room_2, grn, nrm, cyn, OLC_CONFIG(d)->room_nums.donation_room_3, grn, nrm,
        cyn, OLC_CONFIG(d)->room_nums.donation_room_4, grn, nrm, cyn, OLC_CONFIG(d)->room_nums.ress_room_1, grn, nrm,
        cyn, OLC_CONFIG(d)->room_nums.ress_room_2, grn, nrm, cyn, OLC_CONFIG(d)->room_nums.ress_room_3, grn, nrm, cyn,
        OLC_CONFIG(d)->room_nums.ress_room_4, grn, nrm);

    OLC_MODE(d) = CEDIT_ROOM_NUMBERS_MENU;
}

static void cedit_disp_operation_options(struct descriptor_data *d)
{
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(
        d,
        "\r\n\r\n"
        "%sA%s) Default Port : %s%d\r\n"
        "%sB%s) Default IP   : %s%s\r\n"
        "%sC%s) Default Directory   : %s%s\r\n"
        "%sD%s) Logfile Name : %s%s\r\n"
        "%sE%s) Max Players  : %s%d\r\n"
        "%sF%s) Max Filesize : %s%d\r\n"
        "%sG%s) Max Bad Pws  : %s%d\r\n"
        "%sH%s) Site Ok Everyone : %s%s\r\n"
        "%sI%s) Name Server Is Slow : %s%s\r\n"
        "%sJ%s) Use new socials file: %s%s\r\n"
        "%sK%s) OLC autosave to disk: %s%s\r\n"
        "%sL%s) Main Menu           : \r\n%s%s\r\n"
        "%sM%s) Welcome Message     : \r\n%s%s\r\n"
        "%sN%s) Start Message       : \r\n%s%s\r\n"
        "%sO%s) Medit Stats Menu    : %s%s\r\n"
        "%sP%s) Autosave bugs when resolved from commandline : %s%s\r\n"
        "%sR%s) Enable Protocol Negotiation : %s%s\r\n"
        "%sS%s) Enable Special Char in Comm : %s%s\r\n"
        "%sT%s) Current Debug Mode : %s%s\r\n"
        "%sQ%s) Exit To The Main Menu\r\n"
        "Enter your choice : ",
        grn, nrm, cyn, OLC_CONFIG(d)->operation.DFLT_PORT, grn, nrm, cyn,
        OLC_CONFIG(d)->operation.DFLT_IP ? OLC_CONFIG(d)->operation.DFLT_IP : "<None>", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.DFLT_DIR ? OLC_CONFIG(d)->operation.DFLT_DIR : "<None>", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.LOGNAME ? OLC_CONFIG(d)->operation.LOGNAME : "<None>", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.max_playing, grn, nrm, cyn, OLC_CONFIG(d)->operation.max_filesize, grn, nrm, cyn,
        OLC_CONFIG(d)->operation.max_bad_pws, grn, nrm, cyn, YESNO(OLC_CONFIG(d)->operation.siteok_everyone), grn, nrm,
        cyn, YESNO(OLC_CONFIG(d)->operation.nameserver_is_slow), grn, nrm, cyn,
        YESNO(OLC_CONFIG(d)->operation.use_new_socials), grn, nrm, cyn, YESNO(OLC_CONFIG(d)->operation.auto_save_olc),
        grn, nrm, cyn, OLC_CONFIG(d)->operation.MENU ? OLC_CONFIG(d)->operation.MENU : "<None>", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.WELC_MESSG ? OLC_CONFIG(d)->operation.WELC_MESSG : "<None>", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.START_MESSG ? OLC_CONFIG(d)->operation.START_MESSG : "<None>", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.medit_advanced ? "Advanced" : "Standard", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.ibt_autosave ? "Yes" : "No", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.protocol_negotiation ? "Yes" : "No", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.special_in_comm ? "Yes" : "No", grn, nrm, cyn,
        OLC_CONFIG(d)->operation.debug_mode == 0
            ? "OFF"
            : (OLC_CONFIG(d)->operation.debug_mode == 1
                   ? "BRIEF"
                   : (OLC_CONFIG(d)->operation.debug_mode == 2 ? "NORMAL" : "COMPLETE")),
        grn, nrm);

    OLC_MODE(d) = CEDIT_OPERATION_OPTIONS_MENU;
}

static void cedit_disp_autowiz_options(struct descriptor_data *d)
{
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(d,
                    "\r\n\r\n"
                    "%sA%s) Use the autowiz        : %s%s\r\n"
                    "%sB%s) Minimum wizlist level  : %s%d\r\n"
                    "%sQ%s) Exit To The Main Menu\r\n"
                    "Enter your choice : ",
                    grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->autowiz.use_autowiz), grn, nrm, cyn,
                    OLC_CONFIG(d)->autowiz.min_wizlist_lev, grn, nrm);

    OLC_MODE(d) = CEDIT_AUTOWIZ_OPTIONS_MENU;
}

static void cedit_disp_experimental_options(struct descriptor_data *d)
{
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(d,
                    "Configuração de Funcionalidades Experimentais:\r\n"
                    "---\r\n"
                    "Funcionalidades Experimentais:\r\n"
                    "%s1%s) Sistema de Leilão Novo : %s%s\r\n"
                    "%s2%s) Sistema Experimental de Banco : %s%s\r\n"
                    "%s3%s) Sociais Contextuais de Mobs (reputação/alinhamento/posição) : %s%s\r\n"
                    "%s4%s) Sistema de Reputação Dinâmica (combate/cura/dar/roubar - exclui quests) : %s%s\r\n"
                    "%s5%s) Chance de Social de Emoção de Mob (%%) : %s%d\r\n"
                    "%s6%s) Chance de Atualização de Emoção de Mob (%%) : %s%d\r\n",
                    grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->experimental.new_auction_system), grn, nrm, cyn,
                    CHECK_VAR(OLC_CONFIG(d)->experimental.experimental_bank_system), grn, nrm, cyn,
                    CHECK_VAR(OLC_CONFIG(d)->experimental.mob_contextual_socials), grn, nrm, cyn,
                    CHECK_VAR(OLC_CONFIG(d)->experimental.dynamic_reputation), grn, nrm, cyn,
                    OLC_CONFIG(d)->experimental.mob_emotion_social_chance, grn, nrm, cyn,
                    OLC_CONFIG(d)->experimental.mob_emotion_update_chance);

    /* Weather emotion options - only show if mob_contextual_socials is enabled */
    if (OLC_CONFIG(d)->experimental.mob_contextual_socials) {
        write_to_output(d,
                        "%s7%s) Clima Afeta Emoções de Mobs : %s%s\r\n"
                        "%s8%s) Multiplicador de Efeito do Clima (%%) : %s%d\r\n",
                        grn, nrm, cyn, CHECK_VAR(OLC_CONFIG(d)->experimental.weather_affects_emotions), grn, nrm, cyn,
                        OLC_CONFIG(d)->experimental.weather_effect_multiplier);
    }

    write_to_output(d,
                    "%s0%s) Retornar ao Menu anterior\r\n"
                    "Selecione uma opção : ",
                    grn, nrm);

    OLC_MODE(d) = CEDIT_EXPERIMENTAL_MENU;
}

static void cedit_disp_emotion_menu(struct descriptor_data *d)
{
    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(d,
                    "Emotion System Configuration\r\n"
                    "---\r\n"
                    "%sA%s) Visual Indicator Thresholds\r\n"
                    "%sB%s) Combat Flee Behavior\r\n"
                    "%sC%s) Pain System Configuration\r\n"
                    "%sD%s) Memory System Configuration\r\n"
                    "%sP%s) Load Configuration Preset\r\n"
                    "%sQ%s) Return to Main Menu\r\n"
                    "Enter your choice : ",
                    grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm);

    OLC_MODE(d) = CEDIT_EMOTION_MENU;
}

/* Load emotion configuration preset */
static void cedit_load_emotion_preset(struct descriptor_data *d, int preset)
{
    switch (preset) {
        case 1: /* Aggressive - Mobs fight harder, flee less */
            /* Display thresholds - higher to show emotions less */
            OLC_CONFIG(d)->emotion_config.display_fear_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_anger_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_happiness_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_sadness_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_horror_threshold = 90;
            OLC_CONFIG(d)->emotion_config.display_pain_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_compassion_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_courage_threshold = 60; /* Show courage more */
            OLC_CONFIG(d)->emotion_config.display_curiosity_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_disgust_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_envy_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_excitement_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_friendship_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_greed_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_humiliation_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_love_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_loyalty_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_pride_threshold = 60; /* Show pride more */
            OLC_CONFIG(d)->emotion_config.display_shame_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_trust_threshold = 80;

            /* Flee behavior - reduced fear impact, increased courage bonus */
            OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = 60;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = 80;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = 40;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = 60;
            OLC_CONFIG(d)->emotion_config.flee_horror_threshold = 90;

            OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = 5;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = 10;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = -15;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = -20;
            OLC_CONFIG(d)->emotion_config.flee_horror_modifier = 20;

            /* Pain system - less pain from damage */
            OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = 8;
            OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = 15;
            OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = 30;
            OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = 60;

            OLC_CONFIG(d)->emotion_config.pain_minor_min = 1;
            OLC_CONFIG(d)->emotion_config.pain_minor_max = 3;
            OLC_CONFIG(d)->emotion_config.pain_moderate_min = 4;
            OLC_CONFIG(d)->emotion_config.pain_moderate_max = 10;
            OLC_CONFIG(d)->emotion_config.pain_heavy_min = 11;
            OLC_CONFIG(d)->emotion_config.pain_heavy_max = 20;
            OLC_CONFIG(d)->emotion_config.pain_massive_min = 21;
            OLC_CONFIG(d)->emotion_config.pain_massive_max = 40;

            /* Memory system - shorter memory */
            OLC_CONFIG(d)->emotion_config.memory_weight_recent = 8;
            OLC_CONFIG(d)->emotion_config.memory_weight_fresh = 5;
            OLC_CONFIG(d)->emotion_config.memory_weight_moderate = 3;
            OLC_CONFIG(d)->emotion_config.memory_weight_old = 2;
            OLC_CONFIG(d)->emotion_config.memory_weight_ancient = 1;

            OLC_CONFIG(d)->emotion_config.memory_age_recent = 240;
            OLC_CONFIG(d)->emotion_config.memory_age_fresh = 480;
            OLC_CONFIG(d)->emotion_config.memory_age_moderate = 1200;
            OLC_CONFIG(d)->emotion_config.memory_age_old = 2400;

            OLC_CONFIG(d)->emotion_config.memory_baseline_offset = 50;

            /* Trading behavior thresholds - aggressive pricing */
            OLC_CONFIG(d)->emotion_config.trade_trust_high_threshold = 70;      /* Harder to gain trust bonus */
            OLC_CONFIG(d)->emotion_config.trade_trust_low_threshold = 40;       /* Less likely to refuse */
            OLC_CONFIG(d)->emotion_config.trade_greed_high_threshold = 60;      /* More greedy */
            OLC_CONFIG(d)->emotion_config.trade_friendship_high_threshold = 75; /* Harder to get discount */

            /* Quest behavior thresholds - less quest-friendly */
            OLC_CONFIG(d)->emotion_config.quest_curiosity_high_threshold = 75; /* Less curious */
            OLC_CONFIG(d)->emotion_config.quest_loyalty_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.quest_trust_high_threshold = 70; /* Harder to get bonus */
            OLC_CONFIG(d)->emotion_config.quest_trust_low_threshold = 40;  /* Less likely to refuse */

            /* Social initiation thresholds - more aggressive socials */
            OLC_CONFIG(d)->emotion_config.social_happiness_high_threshold = 80; /* Rarely happy */
            OLC_CONFIG(d)->emotion_config.social_anger_high_threshold = 60;     /* More anger */
            OLC_CONFIG(d)->emotion_config.social_sadness_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.social_love_follow_threshold = 85; /* Rarely follows */

            /* Group behavior thresholds - less loyal */
            OLC_CONFIG(d)->emotion_config.group_loyalty_high_threshold = 75;    /* Harder to stay loyal */
            OLC_CONFIG(d)->emotion_config.group_loyalty_low_threshold = 40;     /* Easier to abandon */
            OLC_CONFIG(d)->emotion_config.group_friendship_high_threshold = 75; /* Harder to join */
            OLC_CONFIG(d)->emotion_config.group_envy_high_threshold = 65;       /* More envious */

            /* Combat behavior - Aggressive preset: more anger bonus, less pain penalty */
            OLC_CONFIG(d)->emotion_config.combat_anger_high_threshold = 60; /* Easier to trigger anger bonus */
            OLC_CONFIG(d)->emotion_config.combat_anger_damage_bonus = 20;   /* Higher damage bonus */
            OLC_CONFIG(d)->emotion_config.combat_anger_attack_bonus = 30;   /* Higher attack bonus */
            OLC_CONFIG(d)->emotion_config.combat_pain_low_threshold = 40;   /* Less sensitive to pain */
            OLC_CONFIG(d)->emotion_config.combat_pain_moderate_threshold = 60;
            OLC_CONFIG(d)->emotion_config.combat_pain_high_threshold = 80;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_low = 1;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_mod = 1; /* Reduced penalties */
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_high = 3;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_low = 3;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_mod = 7;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_high = 15;
            break;

        case 2: /* Defensive - Mobs flee more easily, show fear */
            /* Display thresholds - lower to show emotions more */
            OLC_CONFIG(d)->emotion_config.display_fear_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_anger_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_happiness_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_sadness_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_horror_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_pain_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_compassion_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_courage_threshold = 80; /* Show courage less */
            OLC_CONFIG(d)->emotion_config.display_curiosity_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_disgust_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_envy_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_excitement_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_friendship_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_greed_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_humiliation_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_love_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_loyalty_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_pride_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_shame_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_trust_threshold = 70;

            /* Flee behavior - increased fear impact, reduced courage */
            OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = 40;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = 60;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = 60;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = 80;
            OLC_CONFIG(d)->emotion_config.flee_horror_threshold = 70;

            OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = 15;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = 20;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = -5;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = -10;
            OLC_CONFIG(d)->emotion_config.flee_horror_modifier = 30;

            /* Pain system - more pain from damage */
            OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = 3;
            OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = 8;
            OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = 20;
            OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = 40;

            OLC_CONFIG(d)->emotion_config.pain_minor_min = 2;
            OLC_CONFIG(d)->emotion_config.pain_minor_max = 8;
            OLC_CONFIG(d)->emotion_config.pain_moderate_min = 9;
            OLC_CONFIG(d)->emotion_config.pain_moderate_max = 20;
            OLC_CONFIG(d)->emotion_config.pain_heavy_min = 21;
            OLC_CONFIG(d)->emotion_config.pain_heavy_max = 40;
            OLC_CONFIG(d)->emotion_config.pain_massive_min = 41;
            OLC_CONFIG(d)->emotion_config.pain_massive_max = 60;

            /* Memory system - longer memory */
            OLC_CONFIG(d)->emotion_config.memory_weight_recent = 10;
            OLC_CONFIG(d)->emotion_config.memory_weight_fresh = 8;
            OLC_CONFIG(d)->emotion_config.memory_weight_moderate = 6;
            OLC_CONFIG(d)->emotion_config.memory_weight_old = 4;
            OLC_CONFIG(d)->emotion_config.memory_weight_ancient = 2;

            OLC_CONFIG(d)->emotion_config.memory_age_recent = 360;
            OLC_CONFIG(d)->emotion_config.memory_age_fresh = 720;
            OLC_CONFIG(d)->emotion_config.memory_age_moderate = 2400;
            OLC_CONFIG(d)->emotion_config.memory_age_old = 4800;

            OLC_CONFIG(d)->emotion_config.memory_baseline_offset = 50;

            /* Trading behavior thresholds - defensive pricing */
            OLC_CONFIG(d)->emotion_config.trade_trust_high_threshold = 50;      /* Easier to gain trust bonus */
            OLC_CONFIG(d)->emotion_config.trade_trust_low_threshold = 20;       /* More likely to refuse */
            OLC_CONFIG(d)->emotion_config.trade_greed_high_threshold = 80;      /* Less greedy */
            OLC_CONFIG(d)->emotion_config.trade_friendship_high_threshold = 60; /* Easier discount */

            /* Quest behavior thresholds - cautious with quests */
            OLC_CONFIG(d)->emotion_config.quest_curiosity_high_threshold = 80; /* Less curious */
            OLC_CONFIG(d)->emotion_config.quest_loyalty_high_threshold = 65;
            OLC_CONFIG(d)->emotion_config.quest_trust_high_threshold = 50; /* Easier bonus */
            OLC_CONFIG(d)->emotion_config.quest_trust_low_threshold = 20;  /* More likely to refuse */

            /* Social initiation thresholds - fearful socials */
            OLC_CONFIG(d)->emotion_config.social_happiness_high_threshold = 75; /* Less happy */
            OLC_CONFIG(d)->emotion_config.social_anger_high_threshold = 80;     /* Less angry */
            OLC_CONFIG(d)->emotion_config.social_sadness_high_threshold = 60;   /* More sad */
            OLC_CONFIG(d)->emotion_config.social_love_follow_threshold = 75;    /* Easier follow */

            /* Group behavior thresholds - less stable groups */
            OLC_CONFIG(d)->emotion_config.group_loyalty_high_threshold = 75;    /* Harder to stay */
            OLC_CONFIG(d)->emotion_config.group_loyalty_low_threshold = 35;     /* Easier to abandon */
            OLC_CONFIG(d)->emotion_config.group_friendship_high_threshold = 65; /* Easier join */
            OLC_CONFIG(d)->emotion_config.group_envy_high_threshold = 75;       /* Less envious */

            /* Combat behavior - Defensive preset: less anger bonus, more pain penalty */
            OLC_CONFIG(d)->emotion_config.combat_anger_high_threshold = 80; /* Harder to trigger anger bonus */
            OLC_CONFIG(d)->emotion_config.combat_anger_damage_bonus = 10;   /* Lower damage bonus */
            OLC_CONFIG(d)->emotion_config.combat_anger_attack_bonus = 20;   /* Lower attack bonus */
            OLC_CONFIG(d)->emotion_config.combat_pain_low_threshold = 20;   /* More sensitive to pain */
            OLC_CONFIG(d)->emotion_config.combat_pain_moderate_threshold = 40;
            OLC_CONFIG(d)->emotion_config.combat_pain_high_threshold = 60;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_low = 2;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_mod = 3; /* Higher penalties */
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_high = 5;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_low = 7;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_mod = 15;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_high = 25;
            break;

        case 3: /* Balanced - Default values (Phase 2 defaults) */
            OLC_CONFIG(d)->emotion_config.display_fear_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_anger_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_happiness_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_sadness_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_horror_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_pain_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_compassion_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_courage_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_curiosity_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_disgust_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_envy_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_excitement_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_friendship_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_greed_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_humiliation_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_love_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_loyalty_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_pride_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_shame_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_trust_threshold = 70;

            OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = 50;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = 50;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.flee_horror_threshold = 80;

            OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = 10;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = 15;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = -10;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = -15;
            OLC_CONFIG(d)->emotion_config.flee_horror_modifier = 25;

            OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = 5;
            OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = 10;
            OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = 25;
            OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = 50;

            OLC_CONFIG(d)->emotion_config.pain_minor_min = 1;
            OLC_CONFIG(d)->emotion_config.pain_minor_max = 5;
            OLC_CONFIG(d)->emotion_config.pain_moderate_min = 6;
            OLC_CONFIG(d)->emotion_config.pain_moderate_max = 15;
            OLC_CONFIG(d)->emotion_config.pain_heavy_min = 16;
            OLC_CONFIG(d)->emotion_config.pain_heavy_max = 30;
            OLC_CONFIG(d)->emotion_config.pain_massive_min = 31;
            OLC_CONFIG(d)->emotion_config.pain_massive_max = 50;

            OLC_CONFIG(d)->emotion_config.memory_weight_recent = 10;
            OLC_CONFIG(d)->emotion_config.memory_weight_fresh = 7;
            OLC_CONFIG(d)->emotion_config.memory_weight_moderate = 5;
            OLC_CONFIG(d)->emotion_config.memory_weight_old = 3;
            OLC_CONFIG(d)->emotion_config.memory_weight_ancient = 1;

            OLC_CONFIG(d)->emotion_config.memory_age_recent = 300;
            OLC_CONFIG(d)->emotion_config.memory_age_fresh = 600;
            OLC_CONFIG(d)->emotion_config.memory_age_moderate = 1800;
            OLC_CONFIG(d)->emotion_config.memory_age_old = 3600;

            OLC_CONFIG(d)->emotion_config.memory_baseline_offset = 50;

            /* Trading behavior thresholds - balanced */
            OLC_CONFIG(d)->emotion_config.trade_trust_high_threshold = 60;
            OLC_CONFIG(d)->emotion_config.trade_trust_low_threshold = 30;
            OLC_CONFIG(d)->emotion_config.trade_greed_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.trade_friendship_high_threshold = 70;

            /* Quest behavior thresholds - balanced */
            OLC_CONFIG(d)->emotion_config.quest_curiosity_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.quest_loyalty_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.quest_trust_high_threshold = 60;
            OLC_CONFIG(d)->emotion_config.quest_trust_low_threshold = 30;

            /* Social initiation thresholds - balanced */
            OLC_CONFIG(d)->emotion_config.social_happiness_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.social_anger_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.social_sadness_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.social_love_follow_threshold = 80;

            /* Group behavior thresholds - balanced */
            OLC_CONFIG(d)->emotion_config.group_loyalty_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.group_loyalty_low_threshold = 30;
            OLC_CONFIG(d)->emotion_config.group_friendship_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.group_envy_high_threshold = 70;

            /* Combat behavior thresholds and modifiers - balanced */
            OLC_CONFIG(d)->emotion_config.combat_anger_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.combat_anger_damage_bonus = 15;
            OLC_CONFIG(d)->emotion_config.combat_anger_attack_bonus = 25;
            OLC_CONFIG(d)->emotion_config.combat_pain_low_threshold = 30;
            OLC_CONFIG(d)->emotion_config.combat_pain_moderate_threshold = 50;
            OLC_CONFIG(d)->emotion_config.combat_pain_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_low = 1;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_mod = 2;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_high = 4;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_low = 5;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_mod = 10;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_high = 20;
            break;

        case 4: /* Sensitive - Emotions display more, memory lasts longer */
            /* Display thresholds - much lower to show emotions easily */
            OLC_CONFIG(d)->emotion_config.display_fear_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_anger_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_happiness_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_sadness_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_horror_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_pain_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_compassion_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_courage_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_curiosity_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_disgust_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_envy_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_excitement_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_friendship_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_greed_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_humiliation_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_love_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_loyalty_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_pride_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_shame_threshold = 50;
            OLC_CONFIG(d)->emotion_config.display_trust_threshold = 50;

            /* Flee behavior - moderate */
            OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = 45;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = 65;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = 45;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = 65;
            OLC_CONFIG(d)->emotion_config.flee_horror_threshold = 75;

            OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = 12;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = 18;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = -12;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = -18;
            OLC_CONFIG(d)->emotion_config.flee_horror_modifier = 28;

            /* Pain system - moderate pain */
            OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = 4;
            OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = 9;
            OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = 22;
            OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = 45;

            OLC_CONFIG(d)->emotion_config.pain_minor_min = 2;
            OLC_CONFIG(d)->emotion_config.pain_minor_max = 6;
            OLC_CONFIG(d)->emotion_config.pain_moderate_min = 7;
            OLC_CONFIG(d)->emotion_config.pain_moderate_max = 18;
            OLC_CONFIG(d)->emotion_config.pain_heavy_min = 19;
            OLC_CONFIG(d)->emotion_config.pain_heavy_max = 35;
            OLC_CONFIG(d)->emotion_config.pain_massive_min = 36;
            OLC_CONFIG(d)->emotion_config.pain_massive_max = 55;

            /* Memory system - very long lasting memories */
            OLC_CONFIG(d)->emotion_config.memory_weight_recent = 10;
            OLC_CONFIG(d)->emotion_config.memory_weight_fresh = 9;
            OLC_CONFIG(d)->emotion_config.memory_weight_moderate = 7;
            OLC_CONFIG(d)->emotion_config.memory_weight_old = 5;
            OLC_CONFIG(d)->emotion_config.memory_weight_ancient = 3;

            OLC_CONFIG(d)->emotion_config.memory_age_recent = 480;
            OLC_CONFIG(d)->emotion_config.memory_age_fresh = 960;
            OLC_CONFIG(d)->emotion_config.memory_age_moderate = 3600;
            OLC_CONFIG(d)->emotion_config.memory_age_old = 7200;

            OLC_CONFIG(d)->emotion_config.memory_baseline_offset = 50;

            /* Trading behavior thresholds - very responsive */
            OLC_CONFIG(d)->emotion_config.trade_trust_high_threshold = 55;      /* Easy trust bonus */
            OLC_CONFIG(d)->emotion_config.trade_trust_low_threshold = 25;       /* Easy refusal */
            OLC_CONFIG(d)->emotion_config.trade_greed_high_threshold = 75;      /* Less greedy */
            OLC_CONFIG(d)->emotion_config.trade_friendship_high_threshold = 60; /* Easy discount */

            /* Quest behavior thresholds - very responsive */
            OLC_CONFIG(d)->emotion_config.quest_curiosity_high_threshold = 65; /* More curious */
            OLC_CONFIG(d)->emotion_config.quest_loyalty_high_threshold = 65;
            OLC_CONFIG(d)->emotion_config.quest_trust_high_threshold = 55; /* Easy bonus */
            OLC_CONFIG(d)->emotion_config.quest_trust_low_threshold = 25;  /* Easy refusal */

            /* Social initiation thresholds - very emotional */
            OLC_CONFIG(d)->emotion_config.social_happiness_high_threshold = 60; /* More happy */
            OLC_CONFIG(d)->emotion_config.social_anger_high_threshold = 60;     /* More angry */
            OLC_CONFIG(d)->emotion_config.social_sadness_high_threshold = 60;   /* More sad */
            OLC_CONFIG(d)->emotion_config.social_love_follow_threshold = 70;    /* Easier follow */

            /* Group behavior thresholds - very social */
            OLC_CONFIG(d)->emotion_config.group_loyalty_high_threshold = 65;    /* Easier stay */
            OLC_CONFIG(d)->emotion_config.group_loyalty_low_threshold = 35;     /* Easier abandon */
            OLC_CONFIG(d)->emotion_config.group_friendship_high_threshold = 60; /* Easier join */
            OLC_CONFIG(d)->emotion_config.group_envy_high_threshold = 75;       /* Less envious */

            /* Combat behavior - Sensitive preset: balanced combat emotions */
            OLC_CONFIG(d)->emotion_config.combat_anger_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.combat_anger_damage_bonus = 15;
            OLC_CONFIG(d)->emotion_config.combat_anger_attack_bonus = 25;
            OLC_CONFIG(d)->emotion_config.combat_pain_low_threshold = 30;
            OLC_CONFIG(d)->emotion_config.combat_pain_moderate_threshold = 50;
            OLC_CONFIG(d)->emotion_config.combat_pain_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_low = 1;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_mod = 2;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_high = 4;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_low = 5;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_mod = 10;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_high = 20;
            break;

        case 5: /* Mercantile - Trading-focused, fair prices, trusting */
            /* Display thresholds - moderate */
            OLC_CONFIG(d)->emotion_config.display_fear_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_anger_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_happiness_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_sadness_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_horror_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_pain_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_compassion_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_courage_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_curiosity_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_disgust_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_envy_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_excitement_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_friendship_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_greed_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_humiliation_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_love_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_loyalty_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_pride_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_shame_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_trust_threshold = 60;

            /* Flee behavior - moderate courage */
            OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = 50;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = 50;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.flee_horror_threshold = 80;
            OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = 10;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = 15;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = -10;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = -15;
            OLC_CONFIG(d)->emotion_config.flee_horror_modifier = 25;

            /* Pain system - standard */
            OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = 5;
            OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = 10;
            OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = 25;
            OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = 50;
            OLC_CONFIG(d)->emotion_config.pain_minor_min = 1;
            OLC_CONFIG(d)->emotion_config.pain_minor_max = 5;
            OLC_CONFIG(d)->emotion_config.pain_moderate_min = 6;
            OLC_CONFIG(d)->emotion_config.pain_moderate_max = 15;
            OLC_CONFIG(d)->emotion_config.pain_heavy_min = 16;
            OLC_CONFIG(d)->emotion_config.pain_heavy_max = 30;
            OLC_CONFIG(d)->emotion_config.pain_massive_min = 31;
            OLC_CONFIG(d)->emotion_config.pain_massive_max = 50;

            /* Memory system - moderate */
            OLC_CONFIG(d)->emotion_config.memory_weight_recent = 10;
            OLC_CONFIG(d)->emotion_config.memory_weight_fresh = 7;
            OLC_CONFIG(d)->emotion_config.memory_weight_moderate = 5;
            OLC_CONFIG(d)->emotion_config.memory_weight_old = 3;
            OLC_CONFIG(d)->emotion_config.memory_weight_ancient = 1;
            OLC_CONFIG(d)->emotion_config.memory_age_recent = 300;
            OLC_CONFIG(d)->emotion_config.memory_age_fresh = 600;
            OLC_CONFIG(d)->emotion_config.memory_age_moderate = 1800;
            OLC_CONFIG(d)->emotion_config.memory_age_old = 3600;
            OLC_CONFIG(d)->emotion_config.memory_baseline_offset = 50;

            /* Trading behavior thresholds - VERY GENEROUS */
            OLC_CONFIG(d)->emotion_config.trade_trust_high_threshold = 50;      /* Easy trust bonus */
            OLC_CONFIG(d)->emotion_config.trade_trust_low_threshold = 20;       /* Rarely refuse */
            OLC_CONFIG(d)->emotion_config.trade_greed_high_threshold = 80;      /* Not greedy */
            OLC_CONFIG(d)->emotion_config.trade_friendship_high_threshold = 55; /* Easy discount */

            /* Quest behavior thresholds - moderate */
            OLC_CONFIG(d)->emotion_config.quest_curiosity_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.quest_loyalty_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.quest_trust_high_threshold = 55; /* Easy bonus */
            OLC_CONFIG(d)->emotion_config.quest_trust_low_threshold = 25;

            /* Social initiation thresholds - friendly */
            OLC_CONFIG(d)->emotion_config.social_happiness_high_threshold = 65;
            OLC_CONFIG(d)->emotion_config.social_anger_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.social_sadness_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.social_love_follow_threshold = 75;

            /* Group behavior thresholds - moderate */
            OLC_CONFIG(d)->emotion_config.group_loyalty_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.group_loyalty_low_threshold = 30;
            OLC_CONFIG(d)->emotion_config.group_friendship_high_threshold = 65;
            OLC_CONFIG(d)->emotion_config.group_envy_high_threshold = 75;

            /* Combat behavior - Mercantile preset: balanced combat */
            OLC_CONFIG(d)->emotion_config.combat_anger_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.combat_anger_damage_bonus = 15;
            OLC_CONFIG(d)->emotion_config.combat_anger_attack_bonus = 25;
            OLC_CONFIG(d)->emotion_config.combat_pain_low_threshold = 30;
            OLC_CONFIG(d)->emotion_config.combat_pain_moderate_threshold = 50;
            OLC_CONFIG(d)->emotion_config.combat_pain_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_low = 1;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_mod = 2;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_high = 4;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_low = 5;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_mod = 10;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_high = 20;
            break;

        case 6: /* Hermit - Antisocial, refuses interaction, distrusting */
            /* Display thresholds - show negative emotions more */
            OLC_CONFIG(d)->emotion_config.display_fear_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_anger_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_happiness_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_sadness_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_horror_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_pain_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_compassion_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_courage_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_curiosity_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_disgust_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_envy_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_excitement_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_friendship_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_greed_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_humiliation_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_love_threshold = 90;
            OLC_CONFIG(d)->emotion_config.display_loyalty_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_pride_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_shame_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_trust_threshold = 85;

            /* Flee behavior - moderate, cautious */
            OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = 45;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = 65;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = 55;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.flee_horror_threshold = 75;
            OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = 12;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = 18;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = -8;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = -12;
            OLC_CONFIG(d)->emotion_config.flee_horror_modifier = 28;

            /* Pain system - standard */
            OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = 5;
            OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = 10;
            OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = 25;
            OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = 50;
            OLC_CONFIG(d)->emotion_config.pain_minor_min = 1;
            OLC_CONFIG(d)->emotion_config.pain_minor_max = 5;
            OLC_CONFIG(d)->emotion_config.pain_moderate_min = 6;
            OLC_CONFIG(d)->emotion_config.pain_moderate_max = 15;
            OLC_CONFIG(d)->emotion_config.pain_heavy_min = 16;
            OLC_CONFIG(d)->emotion_config.pain_heavy_max = 30;
            OLC_CONFIG(d)->emotion_config.pain_massive_min = 31;
            OLC_CONFIG(d)->emotion_config.pain_massive_max = 50;

            /* Memory system - long memories */
            OLC_CONFIG(d)->emotion_config.memory_weight_recent = 10;
            OLC_CONFIG(d)->emotion_config.memory_weight_fresh = 8;
            OLC_CONFIG(d)->emotion_config.memory_weight_moderate = 6;
            OLC_CONFIG(d)->emotion_config.memory_weight_old = 4;
            OLC_CONFIG(d)->emotion_config.memory_weight_ancient = 2;
            OLC_CONFIG(d)->emotion_config.memory_age_recent = 360;
            OLC_CONFIG(d)->emotion_config.memory_age_fresh = 720;
            OLC_CONFIG(d)->emotion_config.memory_age_moderate = 2400;
            OLC_CONFIG(d)->emotion_config.memory_age_old = 4800;
            OLC_CONFIG(d)->emotion_config.memory_baseline_offset = 50;

            /* Trading behavior thresholds - VERY DISTRUSTING */
            OLC_CONFIG(d)->emotion_config.trade_trust_high_threshold = 75;      /* Hard trust bonus */
            OLC_CONFIG(d)->emotion_config.trade_trust_low_threshold = 15;       /* Often refuse */
            OLC_CONFIG(d)->emotion_config.trade_greed_high_threshold = 65;      /* More greedy */
            OLC_CONFIG(d)->emotion_config.trade_friendship_high_threshold = 80; /* Hard discount */

            /* Quest behavior thresholds - REFUSES QUESTS */
            OLC_CONFIG(d)->emotion_config.quest_curiosity_high_threshold = 85; /* Not curious */
            OLC_CONFIG(d)->emotion_config.quest_loyalty_high_threshold = 80;
            OLC_CONFIG(d)->emotion_config.quest_trust_high_threshold = 75; /* Hard bonus */
            OLC_CONFIG(d)->emotion_config.quest_trust_low_threshold = 15;  /* Often refuse */

            /* Social initiation thresholds - WITHDRAWN */
            OLC_CONFIG(d)->emotion_config.social_happiness_high_threshold = 85; /* Rarely happy */
            OLC_CONFIG(d)->emotion_config.social_anger_high_threshold = 65;     /* More angry */
            OLC_CONFIG(d)->emotion_config.social_sadness_high_threshold = 55;   /* Often sad */
            OLC_CONFIG(d)->emotion_config.social_love_follow_threshold = 90;    /* Rarely follows */

            /* Group behavior thresholds - ANTISOCIAL */
            OLC_CONFIG(d)->emotion_config.group_loyalty_high_threshold = 80;    /* Hard to stay */
            OLC_CONFIG(d)->emotion_config.group_loyalty_low_threshold = 25;     /* Easy abandon */
            OLC_CONFIG(d)->emotion_config.group_friendship_high_threshold = 80; /* Hard to join */
            OLC_CONFIG(d)->emotion_config.group_envy_high_threshold = 60;       /* Very envious */

            /* Combat behavior - Hermit preset: moderate combat emotions */
            OLC_CONFIG(d)->emotion_config.combat_anger_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.combat_anger_damage_bonus = 12;
            OLC_CONFIG(d)->emotion_config.combat_anger_attack_bonus = 20;
            OLC_CONFIG(d)->emotion_config.combat_pain_low_threshold = 35;
            OLC_CONFIG(d)->emotion_config.combat_pain_moderate_threshold = 55;
            OLC_CONFIG(d)->emotion_config.combat_pain_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_low = 1;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_mod = 2;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_high = 3;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_low = 5;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_mod = 10;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_high = 18;
            break;

        case 7: /* Loyal - Group-focused, stays with allies, faithful */
            /* Display thresholds - show positive emotions more */
            OLC_CONFIG(d)->emotion_config.display_fear_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_anger_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_happiness_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_sadness_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_horror_threshold = 85;
            OLC_CONFIG(d)->emotion_config.display_pain_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_compassion_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_courage_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_curiosity_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_disgust_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_envy_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_excitement_threshold = 70;
            OLC_CONFIG(d)->emotion_config.display_friendship_threshold = 55;
            OLC_CONFIG(d)->emotion_config.display_greed_threshold = 80;
            OLC_CONFIG(d)->emotion_config.display_humiliation_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_love_threshold = 60;
            OLC_CONFIG(d)->emotion_config.display_loyalty_threshold = 55;
            OLC_CONFIG(d)->emotion_config.display_pride_threshold = 65;
            OLC_CONFIG(d)->emotion_config.display_shame_threshold = 75;
            OLC_CONFIG(d)->emotion_config.display_trust_threshold = 60;

            /* Flee behavior - brave, stands ground */
            OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = 55;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = 45;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = 65;
            OLC_CONFIG(d)->emotion_config.flee_horror_threshold = 85;
            OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = 8;
            OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = 12;
            OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = -12;
            OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = -18;
            OLC_CONFIG(d)->emotion_config.flee_horror_modifier = 22;

            /* Pain system - standard */
            OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = 5;
            OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = 10;
            OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = 25;
            OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = 50;
            OLC_CONFIG(d)->emotion_config.pain_minor_min = 1;
            OLC_CONFIG(d)->emotion_config.pain_minor_max = 5;
            OLC_CONFIG(d)->emotion_config.pain_moderate_min = 6;
            OLC_CONFIG(d)->emotion_config.pain_moderate_max = 15;
            OLC_CONFIG(d)->emotion_config.pain_heavy_min = 16;
            OLC_CONFIG(d)->emotion_config.pain_heavy_max = 30;
            OLC_CONFIG(d)->emotion_config.pain_massive_min = 31;
            OLC_CONFIG(d)->emotion_config.pain_massive_max = 50;

            /* Memory system - very long memories */
            OLC_CONFIG(d)->emotion_config.memory_weight_recent = 10;
            OLC_CONFIG(d)->emotion_config.memory_weight_fresh = 9;
            OLC_CONFIG(d)->emotion_config.memory_weight_moderate = 7;
            OLC_CONFIG(d)->emotion_config.memory_weight_old = 5;
            OLC_CONFIG(d)->emotion_config.memory_weight_ancient = 3;
            OLC_CONFIG(d)->emotion_config.memory_age_recent = 480;
            OLC_CONFIG(d)->emotion_config.memory_age_fresh = 960;
            OLC_CONFIG(d)->emotion_config.memory_age_moderate = 3600;
            OLC_CONFIG(d)->emotion_config.memory_age_old = 7200;
            OLC_CONFIG(d)->emotion_config.memory_baseline_offset = 50;

            /* Trading behavior thresholds - TRUSTING */
            OLC_CONFIG(d)->emotion_config.trade_trust_high_threshold = 55;      /* Easy trust bonus */
            OLC_CONFIG(d)->emotion_config.trade_trust_low_threshold = 25;       /* Rarely refuse */
            OLC_CONFIG(d)->emotion_config.trade_greed_high_threshold = 75;      /* Not greedy */
            OLC_CONFIG(d)->emotion_config.trade_friendship_high_threshold = 60; /* Easy discount */

            /* Quest behavior thresholds - helpful */
            OLC_CONFIG(d)->emotion_config.quest_curiosity_high_threshold = 70;
            OLC_CONFIG(d)->emotion_config.quest_loyalty_high_threshold = 60; /* Very loyal */
            OLC_CONFIG(d)->emotion_config.quest_trust_high_threshold = 55;   /* Easy bonus */
            OLC_CONFIG(d)->emotion_config.quest_trust_low_threshold = 25;

            /* Social initiation thresholds - FRIENDLY */
            OLC_CONFIG(d)->emotion_config.social_happiness_high_threshold = 65; /* More happy */
            OLC_CONFIG(d)->emotion_config.social_anger_high_threshold = 75;     /* Less angry */
            OLC_CONFIG(d)->emotion_config.social_sadness_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.social_love_follow_threshold = 70; /* Easier follow */

            /* Group behavior thresholds - VERY LOYAL TO GROUPS */
            OLC_CONFIG(d)->emotion_config.group_loyalty_high_threshold = 60;    /* VERY easy to stay */
            OLC_CONFIG(d)->emotion_config.group_loyalty_low_threshold = 35;     /* Hard to abandon */
            OLC_CONFIG(d)->emotion_config.group_friendship_high_threshold = 55; /* VERY easy to join */
            OLC_CONFIG(d)->emotion_config.group_envy_high_threshold = 80;       /* Not envious */

            /* Combat behavior - Loyal preset: fight for allies */
            OLC_CONFIG(d)->emotion_config.combat_anger_high_threshold = 65; /* Easier to get angry defending */
            OLC_CONFIG(d)->emotion_config.combat_anger_damage_bonus = 18;   /* Higher damage for allies */
            OLC_CONFIG(d)->emotion_config.combat_anger_attack_bonus = 28;
            OLC_CONFIG(d)->emotion_config.combat_pain_low_threshold = 35;
            OLC_CONFIG(d)->emotion_config.combat_pain_moderate_threshold = 55;
            OLC_CONFIG(d)->emotion_config.combat_pain_high_threshold = 75;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_low = 1;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_mod = 2;
            OLC_CONFIG(d)->emotion_config.combat_pain_accuracy_penalty_high = 3; /* Less affected by pain */
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_low = 4;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_mod = 8;
            OLC_CONFIG(d)->emotion_config.combat_pain_damage_penalty_high = 15;
            break;
    }
}

/* The event handler. */
void cedit_parse(struct descriptor_data *d, char *arg)
{
    char *oldtext = NULL;

    switch (OLC_MODE(d)) {
        case CEDIT_CONFIRM_SAVESTRING:
            switch (*arg) {
                case 'y':
                case 'Y':
                    cedit_save_internally(d);
                    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
                           "OLC: %s modifies the game configuration.", GET_NAME(d->character));
                    cleanup_olc(d, CLEANUP_CONFIG);
                    if (CONFIG_AUTO_SAVE) {
                        cedit_save_to_disk();
                        write_to_output(d, "Game configuration saved to disk.\r\n");
                    } else
                        write_to_output(d, "Game configuration saved to memory.\r\n");
                    return;
                case 'n':
                case 'N':
                    write_to_output(d, "Game configuration not saved to memory.\r\n");
                    cleanup_olc(d, CLEANUP_CONFIG);
                    return;
                default:
                    write_to_output(d, "\r\nThat is an invalid choice!\r\n");
                    write_to_output(d, "Do you wish to save your changes? : ");
                    return;
            }

        case CEDIT_MAIN_MENU:
            switch (*arg) {
                case 'g':
                case 'G':
                    cedit_disp_game_play_options(d);
                    OLC_MODE(d) = CEDIT_GAME_OPTIONS_MENU;
                    break;

                case 'c':
                case 'C':
                    cedit_disp_crash_save_options(d);
                    OLC_MODE(d) = CEDIT_CRASHSAVE_OPTIONS_MENU;
                    break;

                case 'r':
                case 'R':
                    cedit_disp_room_numbers(d);
                    OLC_MODE(d) = CEDIT_ROOM_NUMBERS_MENU;
                    break;

                case 'o':
                case 'O':
                    cedit_disp_operation_options(d);
                    OLC_MODE(d) = CEDIT_OPERATION_OPTIONS_MENU;
                    break;

                case 'a':
                case 'A':
                    cedit_disp_autowiz_options(d);
                    OLC_MODE(d) = CEDIT_AUTOWIZ_OPTIONS_MENU;
                    break;

                case 'x':
                case 'X':
                    cedit_disp_experimental_options(d);
                    OLC_MODE(d) = CEDIT_EXPERIMENTAL_MENU;
                    break;

                case 'e':
                case 'E':
                    cedit_disp_emotion_menu(d);
                    break;

                case 'q':
                case 'Q':
                    write_to_output(d, "Do you wish to save your changes? : ");
                    OLC_MODE(d) = CEDIT_CONFIRM_SAVESTRING;
                    break;

                default:
                    write_to_output(d, "That is an invalid choice!\r\n");
                    cedit_disp_menu(d);
                    break;
            }
            break;

        case CEDIT_GAME_OPTIONS_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.pk_allowed);
                    break;

                case 'b':
                case 'B':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.pt_allowed);
                    break;

                case 'c':
                case 'C':
                    write_to_output(d, "Enter the minimum level a player must be to shout, gossip, etc : ");
                    OLC_MODE(d) = CEDIT_LEVEL_CAN_SHOUT;
                    return;

                case 'd':
                case 'D':
                    write_to_output(d, "Enter the amount it costs (in move points) to holler : ");
                    OLC_MODE(d) = CEDIT_HOLLER_MOVE_COST;
                    return;

                case 'e':
                case 'E':
                    write_to_output(d, "Enter the maximum number of people allowed in a tunnel : ");
                    OLC_MODE(d) = CEDIT_TUNNEL_SIZE;
                    return;

                case 'f':
                case 'F':
                    write_to_output(d, "Enter the maximum gain of experience per kill for players : ");
                    OLC_MODE(d) = CEDIT_MAX_EXP_GAIN;
                    return;

                case 'g':
                case 'G':
                    write_to_output(d, "Enter the maximum loss of experience per death for players : ");
                    OLC_MODE(d) = CEDIT_MAX_EXP_LOSS;
                    return;

                case 'h':
                case 'H':
                    write_to_output(d, "Enter the number of tics before NPC corpses decompose : ");
                    OLC_MODE(d) = CEDIT_MAX_NPC_CORPSE_TIME;
                    return;

                case 'i':
                case 'I':
                    write_to_output(d, "Enter the number of tics before PC corpses decompose : ");
                    OLC_MODE(d) = CEDIT_MAX_PC_CORPSE_TIME;
                    return;

                case 'j':
                case 'J':
                    write_to_output(d, "Enter the number of tics before PC's are sent to the void (idle) : ");
                    OLC_MODE(d) = CEDIT_IDLE_VOID;
                    return;

                case 'k':
                case 'K':
                    write_to_output(
                        d, "Enter the number of tics before PC's are automatically rented and forced to quit : ");
                    OLC_MODE(d) = CEDIT_IDLE_RENT_TIME;
                    return;

                case 'l':
                case 'L':
                    write_to_output(d, "Enter the level a player must be to become immune to IDLE : ");
                    OLC_MODE(d) = CEDIT_IDLE_MAX_LEVEL;
                    return;

                case 'm':
                case 'M':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.dts_are_dumps);
                    break;

                case 'n':
                case 'N':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.load_into_inventory);
                    break;

                case 'o':
                case 'O':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.track_through_doors);
                    break;

                case 'p':
                case 'P':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.disp_closed_doors);
                    break;

                case 'r':
                case 'R':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.diagonal_dirs);
                    break;

                case 's':
                case 'S':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.no_mort_to_immort);
                    break;

                case '1':
                    write_to_output(d, "Enter the OK message : ");
                    OLC_MODE(d) = CEDIT_OK;
                    return;

                case '2':
                    write_to_output(d, "Enter the HUH message : ");
                    OLC_MODE(d) = CEDIT_HUH;
                    return;

                case '3':
                    write_to_output(d, "Enter the NOPERSON message : ");
                    OLC_MODE(d) = CEDIT_NOPERSON;
                    return;

                case '4':
                    write_to_output(d, "Enter the NOEFFECT message : ");
                    OLC_MODE(d) = CEDIT_NOEFFECT;
                    return;

                case '5':
                    write_to_output(d, "1) Disable maps\r\n");
                    write_to_output(d, "2) Enable Maps\r\n");
                    write_to_output(d, "3) Maps for Immortals only\r\n");
                    write_to_output(d, "Enter choice: ");
                    OLC_MODE(d) = CEDIT_MAP_OPTION;
                    return;

                case '6':
                    write_to_output(d, "Enter default map size (1-12) : ");
                    OLC_MODE(d) = CEDIT_MAP_SIZE;
                    return;

                case '7':
                    write_to_output(d, "Enter default mini-map size (1-12) : ");
                    OLC_MODE(d) = CEDIT_MINIMAP_SIZE;
                    return;

                case '8':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.script_players);
                    break;

                case '9':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.fit_evolve);
                    break;

                case 'z':
                case 'Z':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.school_weather_affects);
                    break;

                case '0':
                    TOGGLE_VAR(OLC_CONFIG(d)->play.weather_affects_spells);
                    break;

                case 'x':
                case 'X':
                    write_to_output(d, "\r\nEnter max pathfind iterations (0=dynamic, 1-50000): ");
                    OLC_MODE(d) = CEDIT_MAX_PATHFIND_ITERATIONS;
                    return;

                case 'y':
                case 'Y':
                    write_to_output(d, "\r\nEnter max zone path length (0=dynamic, 1-500): ");
                    OLC_MODE(d) = CEDIT_MAX_ZONE_PATH;
                    return;

                case 'w':
                case 'W':
                    write_to_output(d, "\r\nEnter max objects allowed in houses (0=unlimited, 1-250): ");
                    OLC_MODE(d) = CEDIT_MAX_HOUSE_OBJS;
                    return;

                case 'q':
                case 'Q':
                    cedit_disp_menu(d);
                    return;

                default:
                    write_to_output(d, "\r\nThat is an invalid choice!\r\n");
                    cedit_disp_game_play_options(d);
            }

            cedit_disp_game_play_options(d);
            return;

        case CEDIT_CRASHSAVE_OPTIONS_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    TOGGLE_VAR(OLC_CONFIG(d)->csd.free_rent);
                    break;

                case 'b':
                case 'B':
                    write_to_output(d, "Enter the maximum number of items players can rent : ");
                    OLC_MODE(d) = CEDIT_MAX_OBJ_SAVE;
                    return;

                case 'c':
                case 'C':
                    write_to_output(d, "Enter the surcharge on top of item costs : ");
                    OLC_MODE(d) = CEDIT_MIN_RENT_COST;
                    return;

                case 'd':
                case 'D':
                    TOGGLE_VAR(OLC_CONFIG(d)->csd.auto_save);
                    break;

                case 'e':
                case 'E':
                    write_to_output(d, "Enter how often (in minutes) should the MUD save players : ");
                    OLC_MODE(d) = CEDIT_AUTOSAVE_TIME;
                    return;

                case 'f':
                case 'F':
                    write_to_output(d, "Enter the lifetime of crash and idlesave files (days) : ");
                    OLC_MODE(d) = CEDIT_CRASH_FILE_TIMEOUT;
                    return;

                case 'g':
                case 'G':
                    write_to_output(d, "Enter the lifetime of normal rent files (days) : ");
                    OLC_MODE(d) = CEDIT_RENT_FILE_TIMEOUT;
                    return;

                case 'q':
                case 'Q':
                    cedit_disp_menu(d);
                    return;

                default:
                    write_to_output(d, "\r\nThat is an invalid choice!\r\n");
            }

            cedit_disp_crash_save_options(d);
            return;

        case CEDIT_ROOM_NUMBERS_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    write_to_output(d, "Enter the room's vnum where newbies should load into : ");
                    OLC_MODE(d) = CEDIT_NEWBIE_START_ROOM;
                    return;

                case 'b':
                case 'B':
                    write_to_output(d, "Enter the room's vnum where immortals should load into : ");
                    OLC_MODE(d) = CEDIT_IMMORT_START_ROOM;
                    return;

                case 'c':
                case 'C':
                    write_to_output(d, "Enter the room's vnum where frozen people should load into : ");
                    OLC_MODE(d) = CEDIT_FROZEN_START_ROOM;
                    return;

                case 'd':
                case 'D':
                    write_to_output(d, "Enter the room's vnum where dead chars should load into : ");
                    OLC_MODE(d) = CEDIT_DEAD_START_ROOM;
                    return;

                case 'e':
                case 'E':
                    write_to_output(d, "Enter the room's vnum for hometown 1 : ");
                    OLC_MODE(d) = CEDIT_HOMETOWN_1;
                    return;

                case 'f':
                case 'F':
                    write_to_output(d, "Enter the room's vnum for hometown 2 : ");
                    OLC_MODE(d) = CEDIT_HOMETOWN_2;
                    return;
                case 'g':
                case 'G':
                    write_to_output(d, "Enter the room's vnum for hometown 3 : ");
                    OLC_MODE(d) = CEDIT_HOMETOWN_3;
                    return;

                case '1':
                    write_to_output(d, "Enter the vnum for donation room #1 : ");
                    OLC_MODE(d) = CEDIT_DONATION_ROOM_1;
                    return;

                case '2':
                    write_to_output(d, "Enter the vnum for donation room #2 : ");
                    OLC_MODE(d) = CEDIT_DONATION_ROOM_2;
                    return;

                case '3':
                    write_to_output(d, "Enter the vnum for donation room #3 : ");
                    OLC_MODE(d) = CEDIT_DONATION_ROOM_3;
                    return;

                case '4':
                    write_to_output(d, "Enter the vnum for ress room #1 : ");
                    OLC_MODE(d) = CEDIT_RESS_ROOM_1;
                    return;

                case '5':
                    write_to_output(d, "Enter the vnum for ress room #2 : ");
                    OLC_MODE(d) = CEDIT_RESS_ROOM_2;
                    return;
                case '6':
                    write_to_output(d, "Enter the vnum for ress room #3 : ");
                    OLC_MODE(d) = CEDIT_RESS_ROOM_3;
                    return;

                case 'i':
                case 'I':
                    write_to_output(d, "Enter the vnum for donation room #4 : ");
                    OLC_MODE(d) = CEDIT_DONATION_ROOM_4;
                    return;

                case 'j':
                case 'J':
                    write_to_output(d, "Enter the vnum for ress room #4 : ");
                    OLC_MODE(d) = CEDIT_RESS_ROOM_4;
                    return;

                case 'h':
                case 'H':
                    write_to_output(d, "Enter the room's vnum for hometown 4 : ");
                    OLC_MODE(d) = CEDIT_HOMETOWN_4;
                    return;

                case 'q':
                case 'Q':
                    cedit_disp_menu(d);
                    return;

                default:
                    write_to_output(d, "\r\nThat is an invalid choice!\r\n");
            }

            cedit_disp_room_numbers(d);
            return;

        case CEDIT_OPERATION_OPTIONS_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    write_to_output(d, "Enter the default port number : ");
                    OLC_MODE(d) = CEDIT_DFLT_PORT;
                    return;

                case 'b':
                case 'B':
                    write_to_output(d, "Enter the default IP Address : ");
                    OLC_MODE(d) = CEDIT_DFLT_IP;
                    return;

                case 'c':
                case 'C':
                    write_to_output(d, "Enter the default directory : ");
                    OLC_MODE(d) = CEDIT_DFLT_DIR;
                    return;

                case 'd':
                case 'D':
                    write_to_output(d, "Enter the name of the logfile : ");
                    OLC_MODE(d) = CEDIT_LOGNAME;
                    return;

                case 'e':
                case 'E':
                    write_to_output(d, "Enter the maximum number of players : ");
                    OLC_MODE(d) = CEDIT_MAX_PLAYING;
                    return;

                case 'f':
                case 'F':
                    write_to_output(d, "Enter the maximum size of the logs : ");
                    OLC_MODE(d) = CEDIT_MAX_FILESIZE;
                    return;

                case 'g':
                case 'G':
                    write_to_output(d, "Enter the maximum number of password attempts : ");
                    OLC_MODE(d) = CEDIT_MAX_BAD_PWS;
                    return;

                case 'h':
                case 'H':
                    TOGGLE_VAR(OLC_CONFIG(d)->operation.siteok_everyone);
                    break;

                case 'i':
                case 'I':
                    TOGGLE_VAR(OLC_CONFIG(d)->operation.nameserver_is_slow);
                    break;

                case 'j':
                case 'J':
                    TOGGLE_VAR(OLC_CONFIG(d)->operation.use_new_socials);
                    send_to_char(d->character, "Please note that using the stock social file will disable AEDIT.\r\n");
                    break;

                case 'k':
                case 'K':
                    TOGGLE_VAR(OLC_CONFIG(d)->operation.auto_save_olc);
                    break;

                case 'l':
                case 'L':
                    OLC_MODE(d) = CEDIT_MENU;
                    clear_screen(d);
                    send_editor_help(d);
                    write_to_output(d, "Enter the new MENU :\r\n\r\n");

                    if (OLC_CONFIG(d)->operation.MENU) {
                        write_to_output(d, "%s", OLC_CONFIG(d)->operation.MENU);
                        oldtext = strdup(OLC_CONFIG(d)->operation.MENU);
                    }

                    string_write(d, &OLC_CONFIG(d)->operation.MENU, MAX_INPUT_LENGTH, 0, oldtext);
                    return;

                case 'm':
                case 'M':
                    OLC_MODE(d) = CEDIT_WELC_MESSG;
                    clear_screen(d);
                    send_editor_help(d);
                    write_to_output(d, "Enter the new welcome message :\r\n\r\n");

                    if (OLC_CONFIG(d)->operation.WELC_MESSG) {
                        write_to_output(d, "%s", OLC_CONFIG(d)->operation.WELC_MESSG);
                        oldtext = str_udup(OLC_CONFIG(d)->operation.WELC_MESSG);
                    }

                    string_write(d, &OLC_CONFIG(d)->operation.WELC_MESSG, MAX_INPUT_LENGTH, 0, oldtext);
                    return;

                case 'n':
                case 'N':
                    OLC_MODE(d) = CEDIT_START_MESSG;
                    clear_screen(d);
                    send_editor_help(d);
                    write_to_output(d, "Enter the new newbie start message :\r\n\r\n");

                    if (OLC_CONFIG(d)->operation.START_MESSG) {
                        write_to_output(d, "%s", OLC_CONFIG(d)->operation.START_MESSG);
                        oldtext = strdup(OLC_CONFIG(d)->operation.START_MESSG);
                    }

                    string_write(d, &OLC_CONFIG(d)->operation.START_MESSG, MAX_INPUT_LENGTH, 0, oldtext);
                    return;

                case 'o':
                case 'O':
                    TOGGLE_VAR(OLC_CONFIG(d)->operation.medit_advanced);
                    break;

                case 'p':
                case 'P':
                    TOGGLE_VAR(OLC_CONFIG(d)->operation.ibt_autosave);
                    break;

                case 'r':
                case 'R':
                    TOGGLE_VAR(OLC_CONFIG(d)->operation.protocol_negotiation);
                    break;

                case 's':
                case 'S':
                    TOGGLE_VAR(OLC_CONFIG(d)->operation.special_in_comm);
                    break;

                case 't':
                case 'T':
                    write_to_output(d, "Enter the current debug level (0: Off, 1: Brief, 2: Normal, 3: Complete) : ");
                    OLC_MODE(d) = CEDIT_DEBUG_MODE;
                    return;

                case 'q':
                case 'Q':
                    cedit_disp_menu(d);
                    return;

                default:
                    write_to_output(d, "\r\nThat is an invalid choice!\r\n");
            }

            cedit_disp_operation_options(d);
            return;

        case CEDIT_AUTOWIZ_OPTIONS_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    TOGGLE_VAR(OLC_CONFIG(d)->autowiz.use_autowiz);
                    break;

                case 'b':
                case 'B':
                    write_to_output(d, "Enter the minimum level for players to appear on the wizlist : ");
                    OLC_MODE(d) = CEDIT_MIN_WIZLIST_LEV;
                    return;

                case 'q':
                case 'Q':
                    cedit_disp_menu(d);
                    return;

                default:
                    write_to_output(d, "\r\nThat is an invalid choice!\r\n");
            }

            cedit_disp_autowiz_options(d);
            return;

        case CEDIT_EXPERIMENTAL_MENU:
            switch (*arg) {
                case '1':
                    TOGGLE_VAR(OLC_CONFIG(d)->experimental.new_auction_system);
                    break;

                case '2':
                    TOGGLE_VAR(OLC_CONFIG(d)->experimental.experimental_bank_system);
                    break;

                case '3':
                    TOGGLE_VAR(OLC_CONFIG(d)->experimental.mob_contextual_socials);
                    /* If disabling mob_contextual_socials, also disable weather affects emotions */
                    if (!OLC_CONFIG(d)->experimental.mob_contextual_socials) {
                        OLC_CONFIG(d)->experimental.weather_affects_emotions = NO;
                    }
                    break;

                case '4':
                    TOGGLE_VAR(OLC_CONFIG(d)->experimental.dynamic_reputation);
                    break;

                case '5':
                    write_to_output(
                        d, "\r\nEnter the probability (%%) of mob performing social per emotion tick (0-100) : ");
                    OLC_MODE(d) = CEDIT_MOB_EMOTION_SOCIAL_CHANCE;
                    return;

                case '6':
                    write_to_output(
                        d, "\r\nEnter the probability (%%) of mob updating emotions per emotion tick (0-100) : ");
                    OLC_MODE(d) = CEDIT_MOB_EMOTION_UPDATE_CHANCE;
                    return;

                case '7':
                    /* Only allow toggling if mob_contextual_socials is enabled */
                    if (OLC_CONFIG(d)->experimental.mob_contextual_socials) {
                        TOGGLE_VAR(OLC_CONFIG(d)->experimental.weather_affects_emotions);
                    } else {
                        write_to_output(
                            d, "\r\nWeather affects emotions requires mob contextual socials to be enabled!\r\n");
                    }
                    break;

                case '8':
                    /* Only allow setting if mob_contextual_socials is enabled */
                    if (OLC_CONFIG(d)->experimental.mob_contextual_socials) {
                        write_to_output(d, "\r\nEnter weather effect multiplier (0-200%%, default 100) : ");
                        OLC_MODE(d) = CEDIT_WEATHER_EFFECT_MULTIPLIER;
                        return;
                    } else {
                        write_to_output(
                            d, "\r\nWeather effect multiplier requires mob contextual socials to be enabled!\r\n");
                    }
                    break;

                case '0':
                case 'q':
                case 'Q':
                    cedit_disp_menu(d);
                    return;

                default:
                    write_to_output(d, "\r\nEssa é uma escolha inválida!\r\n");
            }

            cedit_disp_experimental_options(d);
            return;

        case CEDIT_MOB_EMOTION_SOCIAL_CHANCE:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the probability (%%) of mob performing social per emotion tick (0-100) : ");
            } else {
                OLC_CONFIG(d)->experimental.mob_emotion_social_chance = LIMIT(atoi(arg), 0, 100);
                cedit_disp_experimental_options(d);
            }
            break;

        case CEDIT_MOB_EMOTION_UPDATE_CHANCE:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the probability (%%) of mob updating emotions per emotion tick (0-100) : ");
            } else {
                OLC_CONFIG(d)->experimental.mob_emotion_update_chance = LIMIT(atoi(arg), 0, 100);
                cedit_disp_experimental_options(d);
            }
            break;

        case CEDIT_WEATHER_EFFECT_MULTIPLIER:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter weather effect multiplier (0-200%%, default 100) : ");
            } else {
                OLC_CONFIG(d)->experimental.weather_effect_multiplier = LIMIT(atoi(arg), 0, 200);
                cedit_disp_experimental_options(d);
            }
            break;

        case CEDIT_EMOTION_MENU:
            switch (*arg) {
                case 'a':
                case 'A':
                    write_to_output(d,
                                    "\r\nVisual Indicator Thresholds (0-100):\r\n"
                                    "1) Fear Display Threshold : %d\r\n"
                                    "2) Anger Display Threshold : %d\r\n"
                                    "3) Happiness Display Threshold : %d\r\n"
                                    "4) Sadness Display Threshold : %d\r\n"
                                    "5) Horror Display Threshold : %d\r\n"
                                    "6) Pain Display Threshold : %d\r\n"
                                    "Q) Return to Emotion Menu\r\n"
                                    "Enter your choice : ",
                                    OLC_CONFIG(d)->emotion_config.display_fear_threshold,
                                    OLC_CONFIG(d)->emotion_config.display_anger_threshold,
                                    OLC_CONFIG(d)->emotion_config.display_happiness_threshold,
                                    OLC_CONFIG(d)->emotion_config.display_sadness_threshold,
                                    OLC_CONFIG(d)->emotion_config.display_horror_threshold,
                                    OLC_CONFIG(d)->emotion_config.display_pain_threshold);
                    OLC_MODE(d) = CEDIT_EMOTION_DISPLAY_SUBMENU;
                    return;

                case 'b':
                case 'B':
                    write_to_output(d,
                                    "\r\nCombat Flee Behavior:\r\n"
                                    "1) Fear Low Threshold : %d\r\n"
                                    "2) Fear High Threshold : %d\r\n"
                                    "3) Fear Low Modifier : %d\r\n"
                                    "4) Fear High Modifier : %d\r\n"
                                    "5) Courage Low Threshold : %d\r\n"
                                    "6) Courage High Threshold : %d\r\n"
                                    "7) Courage Low Modifier : %d\r\n"
                                    "8) Courage High Modifier : %d\r\n"
                                    "9) Horror Threshold : %d\r\n"
                                    "A) Horror Modifier : %d\r\n"
                                    "Q) Return to Emotion Menu\r\n"
                                    "Enter your choice : ",
                                    OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold,
                                    OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold,
                                    OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier,
                                    OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier,
                                    OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold,
                                    OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold,
                                    OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier,
                                    OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier,
                                    OLC_CONFIG(d)->emotion_config.flee_horror_threshold,
                                    OLC_CONFIG(d)->emotion_config.flee_horror_modifier);
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_SUBMENU;
                    return;

                case 'c':
                case 'C':
                    write_to_output(
                        d,
                        "\r\nPain System Configuration:\r\n"
                        "Damage Thresholds (%%): 1) Minor: %d  2) Moderate: %d  3) Heavy: %d  4) Massive: %d\r\n"
                        "Pain Amounts: 5) Minor Min: %d  6) Minor Max: %d\r\n"
                        "              7) Moderate Min: %d  8) Moderate Max: %d\r\n"
                        "              9) Heavy Min: %d  A) Heavy Max: %d\r\n"
                        "              B) Massive Min: %d  C) Massive Max: %d\r\n"
                        "Q) Return to Emotion Menu\r\n"
                        "Enter your choice : ",
                        OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold,
                        OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold,
                        OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold,
                        OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold,
                        OLC_CONFIG(d)->emotion_config.pain_minor_min, OLC_CONFIG(d)->emotion_config.pain_minor_max,
                        OLC_CONFIG(d)->emotion_config.pain_moderate_min,
                        OLC_CONFIG(d)->emotion_config.pain_moderate_max, OLC_CONFIG(d)->emotion_config.pain_heavy_min,
                        OLC_CONFIG(d)->emotion_config.pain_heavy_max, OLC_CONFIG(d)->emotion_config.pain_massive_min,
                        OLC_CONFIG(d)->emotion_config.pain_massive_max);
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_SUBMENU;
                    return;

                case 'd':
                case 'D':
                    write_to_output(
                        d,
                        "\r\nMemory System Configuration:\r\n"
                        "Weights (1-10): 1) Recent: %d  2) Fresh: %d  3) Moderate: %d  4) Old: %d  5) Ancient: %d\r\n"
                        "Age Thresholds (sec): 6) Recent: %d  7) Fresh: %d  8) Moderate: %d  9) Old: %d\r\n"
                        "A) Baseline Offset: %d\r\n"
                        "Q) Return to Emotion Menu\r\n"
                        "Enter your choice : ",
                        OLC_CONFIG(d)->emotion_config.memory_weight_recent,
                        OLC_CONFIG(d)->emotion_config.memory_weight_fresh,
                        OLC_CONFIG(d)->emotion_config.memory_weight_moderate,
                        OLC_CONFIG(d)->emotion_config.memory_weight_old,
                        OLC_CONFIG(d)->emotion_config.memory_weight_ancient,
                        OLC_CONFIG(d)->emotion_config.memory_age_recent, OLC_CONFIG(d)->emotion_config.memory_age_fresh,
                        OLC_CONFIG(d)->emotion_config.memory_age_moderate, OLC_CONFIG(d)->emotion_config.memory_age_old,
                        OLC_CONFIG(d)->emotion_config.memory_baseline_offset);
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_SUBMENU;
                    return;

                case 'p':
                case 'P':
                    write_to_output(d,
                                    "\r\nEmotion Configuration Presets:\r\n"
                                    "1) Aggressive - Mobs fight harder, flee less, greedy\r\n"
                                    "2) Defensive - Mobs flee more easily, show fear, cautious\r\n"
                                    "3) Balanced - Default balanced values for all systems\r\n"
                                    "4) Sensitive - Emotions display more, longer memory, social\r\n"
                                    "5) Mercantile - Trading-focused, fair prices, trusting\r\n"
                                    "6) Hermit - Antisocial, refuses interaction, distrusting\r\n"
                                    "7) Loyal - Group-focused, stays with allies, faithful\r\n"
                                    "Q) Return to Emotion Menu\r\n"
                                    "Enter your choice : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PRESET_MENU;
                    return;

                case 'q':
                case 'Q':
                    cedit_disp_menu(d);
                    return;

                default:
                    write_to_output(d, "\r\nThat is an invalid choice!\r\n");
            }
            cedit_disp_emotion_menu(d);
            return;

        case CEDIT_EMOTION_PRESET_MENU:
            switch (*arg) {
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    cedit_load_emotion_preset(d, *arg - '0');
                    write_to_output(d, "\r\nPreset loaded successfully!\r\n");
                    cedit_disp_emotion_menu(d);
                    return;
                case 'q':
                case 'Q':
                    cedit_disp_emotion_menu(d);
                    return;
                default:
                    write_to_output(d, "\r\nInvalid preset choice!\r\n");
                    cedit_disp_emotion_menu(d);
                    return;
            }
            return;

        case CEDIT_EMOTION_DISPLAY_SUBMENU:
            switch (*arg) {
                case '1':
                    write_to_output(d, "\r\nEnter Fear Display Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_DISPLAY_FEAR_THRESHOLD;
                    return;
                case '2':
                    write_to_output(d, "\r\nEnter Anger Display Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_DISPLAY_ANGER_THRESHOLD;
                    return;
                case '3':
                    write_to_output(d, "\r\nEnter Happiness Display Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_DISPLAY_HAPPINESS_THRESHOLD;
                    return;
                case '4':
                    write_to_output(d, "\r\nEnter Sadness Display Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_DISPLAY_SADNESS_THRESHOLD;
                    return;
                case '5':
                    write_to_output(d, "\r\nEnter Horror Display Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_DISPLAY_HORROR_THRESHOLD;
                    return;
                case '6':
                    write_to_output(d, "\r\nEnter Pain Display Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_DISPLAY_PAIN_THRESHOLD;
                    return;
                case 'q':
                case 'Q':
                    cedit_disp_emotion_menu(d);
                    return;
                default:
                    write_to_output(d, "\r\nInvalid choice!\r\n");
            }
            return;

        case CEDIT_EMOTION_FLEE_SUBMENU:
            switch (*arg) {
                case '1':
                    write_to_output(d, "\r\nEnter Fear Low Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_FEAR_LOW_THRESHOLD;
                    return;
                case '2':
                    write_to_output(d, "\r\nEnter Fear High Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_FEAR_HIGH_THRESHOLD;
                    return;
                case '3':
                    write_to_output(d, "\r\nEnter Fear Low Modifier (-100 to +100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_FEAR_LOW_MODIFIER;
                    return;
                case '4':
                    write_to_output(d, "\r\nEnter Fear High Modifier (-100 to +100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_FEAR_HIGH_MODIFIER;
                    return;
                case '5':
                    write_to_output(d, "\r\nEnter Courage Low Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_COURAGE_LOW_THRESHOLD;
                    return;
                case '6':
                    write_to_output(d, "\r\nEnter Courage High Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_COURAGE_HIGH_THRESHOLD;
                    return;
                case '7':
                    write_to_output(d, "\r\nEnter Courage Low Modifier (-100 to +100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_COURAGE_LOW_MODIFIER;
                    return;
                case '8':
                    write_to_output(d, "\r\nEnter Courage High Modifier (-100 to +100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_COURAGE_HIGH_MODIFIER;
                    return;
                case '9':
                    write_to_output(d, "\r\nEnter Horror Threshold (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_HORROR_THRESHOLD;
                    return;
                case 'a':
                case 'A':
                    write_to_output(d, "\r\nEnter Horror Modifier (-100 to +100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_FLEE_HORROR_MODIFIER;
                    return;
                case 'q':
                case 'Q':
                    cedit_disp_emotion_menu(d);
                    return;
                default:
                    write_to_output(d, "\r\nInvalid choice!\r\n");
            }
            return;

        case CEDIT_EMOTION_PAIN_SUBMENU:
            switch (*arg) {
                case '1':
                    write_to_output(d, "\r\nEnter Minor Damage Threshold (%% of HP, 0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_DAMAGE_MINOR_THRESHOLD;
                    return;
                case '2':
                    write_to_output(d, "\r\nEnter Moderate Damage Threshold (%% of HP, 0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_DAMAGE_MODERATE_THRESHOLD;
                    return;
                case '3':
                    write_to_output(d, "\r\nEnter Heavy Damage Threshold (%% of HP, 0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_DAMAGE_HEAVY_THRESHOLD;
                    return;
                case '4':
                    write_to_output(d, "\r\nEnter Massive Damage Threshold (%% of HP, 0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_DAMAGE_MASSIVE_THRESHOLD;
                    return;
                case '5':
                    write_to_output(d, "\r\nEnter Minor Pain Min (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_MINOR_MIN;
                    return;
                case '6':
                    write_to_output(d, "\r\nEnter Minor Pain Max (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_MINOR_MAX;
                    return;
                case '7':
                    write_to_output(d, "\r\nEnter Moderate Pain Min (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_MODERATE_MIN;
                    return;
                case '8':
                    write_to_output(d, "\r\nEnter Moderate Pain Max (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_MODERATE_MAX;
                    return;
                case '9':
                    write_to_output(d, "\r\nEnter Heavy Pain Min (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_HEAVY_MIN;
                    return;
                case 'a':
                case 'A':
                    write_to_output(d, "\r\nEnter Heavy Pain Max (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_HEAVY_MAX;
                    return;
                case 'b':
                case 'B':
                    write_to_output(d, "\r\nEnter Massive Pain Min (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_MASSIVE_MIN;
                    return;
                case 'c':
                case 'C':
                    write_to_output(d, "\r\nEnter Massive Pain Max (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_PAIN_MASSIVE_MAX;
                    return;
                case 'q':
                case 'Q':
                    cedit_disp_emotion_menu(d);
                    return;
                default:
                    write_to_output(d, "\r\nInvalid choice!\r\n");
            }
            return;

        case CEDIT_EMOTION_MEMORY_SUBMENU:
            switch (*arg) {
                case '1':
                    write_to_output(d, "\r\nEnter Recent Memory Weight (1-10) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_WEIGHT_RECENT;
                    return;
                case '2':
                    write_to_output(d, "\r\nEnter Fresh Memory Weight (1-10) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_WEIGHT_FRESH;
                    return;
                case '3':
                    write_to_output(d, "\r\nEnter Moderate Memory Weight (1-10) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_WEIGHT_MODERATE;
                    return;
                case '4':
                    write_to_output(d, "\r\nEnter Old Memory Weight (1-10) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_WEIGHT_OLD;
                    return;
                case '5':
                    write_to_output(d, "\r\nEnter Ancient Memory Weight (1-10) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_WEIGHT_ANCIENT;
                    return;
                case '6':
                    write_to_output(d, "\r\nEnter Recent Age Threshold (seconds) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_AGE_RECENT;
                    return;
                case '7':
                    write_to_output(d, "\r\nEnter Fresh Age Threshold (seconds) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_AGE_FRESH;
                    return;
                case '8':
                    write_to_output(d, "\r\nEnter Moderate Age Threshold (seconds) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_AGE_MODERATE;
                    return;
                case '9':
                    write_to_output(d, "\r\nEnter Old Age Threshold (seconds) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_AGE_OLD;
                    return;
                case 'a':
                case 'A':
                    write_to_output(d, "\r\nEnter Memory Baseline Offset (0-100) : ");
                    OLC_MODE(d) = CEDIT_EMOTION_MEMORY_BASELINE_OFFSET;
                    return;
                case 'q':
                case 'Q':
                    cedit_disp_emotion_menu(d);
                    return;
                default:
                    write_to_output(d, "\r\nInvalid choice!\r\n");
            }
            return;

        case CEDIT_LEVEL_CAN_SHOUT:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the minimum level a player must be to shout, gossip, etc : ");
            } else {
                OLC_CONFIG(d)->play.level_can_shout = atoi(arg);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_HOLLER_MOVE_COST:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the amount it costs (in move points) to holler : ");
            } else {
                OLC_CONFIG(d)->play.holler_move_cost = atoi(arg);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_TUNNEL_SIZE:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the maximum number of people allowed in a tunnel : ");
            } else {
                OLC_CONFIG(d)->play.tunnel_size = atoi(arg);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_MAX_EXP_GAIN:
            if (*arg)
                OLC_CONFIG(d)->play.max_exp_gain = atoi(arg);

            cedit_disp_game_play_options(d);
            break;

        case CEDIT_MAX_EXP_LOSS:
            if (*arg)
                OLC_CONFIG(d)->play.max_exp_loss = atoi(arg);

            cedit_disp_game_play_options(d);
            break;

        case CEDIT_MAX_NPC_CORPSE_TIME:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the number of tics before NPC corpses decompose : ");
            } else {
                OLC_CONFIG(d)->play.max_npc_corpse_time = atoi(arg);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_MAX_PC_CORPSE_TIME:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the number of tics before PC corpses decompose : ");
            } else {
                OLC_CONFIG(d)->play.max_pc_corpse_time = atoi(arg);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_IDLE_VOID:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the number of tics before PC's are sent to the void (idle) : ");
            } else {
                OLC_CONFIG(d)->play.idle_void = atoi(arg);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_IDLE_RENT_TIME:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the number of tics before PC's are automatically rented and forced to quit : ");
            } else {
                OLC_CONFIG(d)->play.idle_rent_time = atoi(arg);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_IDLE_MAX_LEVEL:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the level a player must be to become immune to IDLE : ");
            } else {
                OLC_CONFIG(d)->play.idle_max_level = atoi(arg);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_OK:
            if (!genolc_checkstring(d, arg))
                break;

            if (OLC_CONFIG(d)->play.OK)
                free(OLC_CONFIG(d)->play.OK);

            OLC_CONFIG(d)->play.OK = str_udupnl(arg);

            cedit_disp_game_play_options(d);
            break;

        case CEDIT_HUH:
            if (!genolc_checkstring(d, arg))
                break;

            if (OLC_CONFIG(d)->play.HUH)
                free(OLC_CONFIG(d)->play.HUH);

            OLC_CONFIG(d)->play.HUH = str_udupnl(arg);

            cedit_disp_game_play_options(d);
            break;

        case CEDIT_NOPERSON:
            if (!genolc_checkstring(d, arg))
                break;

            if (OLC_CONFIG(d)->play.NOPERSON)
                free(OLC_CONFIG(d)->play.NOPERSON);

            OLC_CONFIG(d)->play.NOPERSON = str_udupnl(arg);

            cedit_disp_game_play_options(d);
            break;

        case CEDIT_NOEFFECT:
            if (!genolc_checkstring(d, arg))
                break;

            if (OLC_CONFIG(d)->play.NOEFFECT)
                free(OLC_CONFIG(d)->play.NOEFFECT);

            OLC_CONFIG(d)->play.NOEFFECT = str_udupnl(arg);

            cedit_disp_game_play_options(d);
            break;

        case CEDIT_MAX_OBJ_SAVE:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the maximum objects a player can save : ");
            } else {
                OLC_CONFIG(d)->csd.max_obj_save = atoi(arg);
                cedit_disp_crash_save_options(d);
            }
            break;

        case CEDIT_MIN_RENT_COST:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the minimum amount it costs to rent : ");
            } else {
                OLC_CONFIG(d)->csd.min_rent_cost = atoi(arg);
                cedit_disp_crash_save_options(d);
            }
            break;

        case CEDIT_AUTOSAVE_TIME:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the interval for player's being autosaved : ");
            } else {
                OLC_CONFIG(d)->csd.autosave_time = atoi(arg);
                cedit_disp_crash_save_options(d);
            }
            break;

        case CEDIT_CRASH_FILE_TIMEOUT:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the lifetime of crash and idlesave files (days) : ");
            } else {
                OLC_CONFIG(d)->csd.crash_file_timeout = atoi(arg);
                cedit_disp_crash_save_options(d);
            }
            break;

        case CEDIT_RENT_FILE_TIMEOUT:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the lifetime of rent files (days) : ");
            } else {
                OLC_CONFIG(d)->csd.rent_file_timeout = atoi(arg);
                cedit_disp_crash_save_options(d);
            }
            break;

        case CEDIT_NEWBIE_START_ROOM:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the room's vnum where mortals should load into : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the room's vnum where mortals should load into : ");
            } else {
                OLC_CONFIG(d)->room_nums.newbie_start_room = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_IMMORT_START_ROOM:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the room's vnum where immortals should load into : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the room's vnum where immortals should load into : ");
            } else {
                OLC_CONFIG(d)->room_nums.immort_start_room = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_FROZEN_START_ROOM:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the room's vnum where frozen people should load into : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the room's vnum where frozen people should load into : ");
            } else {
                OLC_CONFIG(d)->room_nums.frozen_start_room = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_DEAD_START_ROOM:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the room's vnum where dead mortals should load into : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the room's vnum where mortals should load into : ");
            } else {
                OLC_CONFIG(d)->room_nums.dead_start_room = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_DONATION_ROOM_1:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for donation room #1 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for donation room #1 : ");
            } else {
                OLC_CONFIG(d)->room_nums.donation_room_1 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_DONATION_ROOM_2:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for donation room #2 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for donation room #2 : ");
            } else {
                OLC_CONFIG(d)->room_nums.donation_room_2 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_DONATION_ROOM_3:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for donation room #3 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for donation room #3 : ");
            } else {
                OLC_CONFIG(d)->room_nums.donation_room_3 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_DONATION_ROOM_4:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for donation room #4 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for donation room #4 : ");
            } else {
                OLC_CONFIG(d)->room_nums.donation_room_4 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_RESS_ROOM_1:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for ress room #1 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for ress room #1 : ");
            } else {
                OLC_CONFIG(d)->room_nums.ress_room_1 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_RESS_ROOM_2:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for ress room #2 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for ress room #2 : ");
            } else {
                OLC_CONFIG(d)->room_nums.ress_room_2 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;
        case CEDIT_RESS_ROOM_3:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for ress room #3 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for ress room #3 : ");
            } else {
                OLC_CONFIG(d)->room_nums.ress_room_3 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_RESS_ROOM_4:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for ress room #4 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for ress room #4 : ");
            } else {
                OLC_CONFIG(d)->room_nums.ress_room_4 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_HOMETOWN_1:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for hometown #1 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for hometown #1 : ");
            } else {
                OLC_CONFIG(d)->room_nums.hometown_1 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_HOMETOWN_2:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for hometown #2 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for hometown #2 : ");
            } else {
                OLC_CONFIG(d)->room_nums.hometown_2 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;
        case CEDIT_HOMETOWN_3:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for hometown #3 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for hometown #3 : ");
            } else {
                OLC_CONFIG(d)->room_nums.hometown_3 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_HOMETOWN_4:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the vnum for hometown #4 : ");
            } else if (real_room(atoi(arg)) == NOWHERE) {
                write_to_output(d,
                                "That room doesn't exist!\r\n"
                                "Enter the vnum for hometown #4 : ");
            } else {
                OLC_CONFIG(d)->room_nums.hometown_4 = atoi(arg);
                cedit_disp_room_numbers(d);
            }
            break;

        case CEDIT_DFLT_PORT:
            OLC_CONFIG(d)->operation.DFLT_PORT = atoi(arg);
            cedit_disp_operation_options(d);
            break;

        case CEDIT_DFLT_IP:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the default ip address : ");
            } else {
                OLC_CONFIG(d)->operation.DFLT_IP = str_udup(arg);
                cedit_disp_operation_options(d);
            }
            break;

        case CEDIT_DFLT_DIR:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the default directory : ");
            } else {
                OLC_CONFIG(d)->operation.DFLT_DIR = str_udup(arg);
                cedit_disp_operation_options(d);
            }
            break;

        case CEDIT_LOGNAME:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter the name of the logfile : ");
            } else {
                OLC_CONFIG(d)->operation.LOGNAME = str_udup(arg);
                cedit_disp_operation_options(d);
            }
            break;

        case CEDIT_MAX_PLAYING:
            OLC_CONFIG(d)->operation.max_playing = atoi(arg);
            cedit_disp_operation_options(d);
            break;

        case CEDIT_MAX_FILESIZE:
            OLC_CONFIG(d)->operation.max_filesize = atoi(arg);
            cedit_disp_operation_options(d);
            break;

        case CEDIT_MAX_BAD_PWS:
            OLC_CONFIG(d)->operation.max_bad_pws = atoi(arg);
            cedit_disp_operation_options(d);
            break;

        case CEDIT_DEBUG_MODE:
            OLC_CONFIG(d)->operation.debug_mode = LIMIT(atoi(arg), 0, 3);
            cedit_disp_operation_options(d);
            break;

        case CEDIT_MIN_WIZLIST_LEV:
            if (atoi(arg) > LVL_IMPL) {
                write_to_output(d,
                                "The minimum wizlist level can't be greater than %d.\r\n"
                                "Enter the minimum level for players to appear on the wizlist : ",
                                LVL_IMPL);
            } else {
                OLC_CONFIG(d)->autowiz.min_wizlist_lev = atoi(arg);
                cedit_disp_autowiz_options(d);
            }
            break;

        case CEDIT_MAP_OPTION:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Select 1, 2 or 3 (0 to cancel) :");
            } else {
                if ((atoi(arg) >= 1) && (atoi(arg) <= 3))
                    OLC_CONFIG(d)->play.map_option = (atoi(arg) - 1);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_MAP_SIZE:
            if (!*arg) {
                /* User just pressed return - restore to default */
                OLC_CONFIG(d)->play.map_size = 6;
                cedit_disp_game_play_options(d);
            } else {
                OLC_CONFIG(d)->play.map_size = MIN(MAX((atoi(arg)), 1), 12);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_MINIMAP_SIZE:
            if (!*arg) {
                /* User just pressed return - restore to default */
                OLC_CONFIG(d)->play.minimap_size = 2;
                cedit_disp_game_play_options(d);
            } else {
                OLC_CONFIG(d)->play.minimap_size = MIN(MAX((atoi(arg)), 1), 12);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_MAX_PATHFIND_ITERATIONS:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter max pathfind iterations (0=dynamic, 1-50000): ");
            } else {
                OLC_CONFIG(d)->play.max_pathfind_iterations = MIN(MAX(atoi(arg), 0), 50000);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_MAX_ZONE_PATH:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter max zone path length (0=dynamic, 1-500): ");
            } else {
                OLC_CONFIG(d)->play.max_zone_path = MIN(MAX(atoi(arg), 0), 500);
                cedit_disp_game_play_options(d);
            }
            break;

        case CEDIT_MAX_HOUSE_OBJS:
            if (!*arg) {
                write_to_output(d,
                                "That is an invalid choice!\r\n"
                                "Enter max objects allowed in houses (0=unlimited, 1-250): ");
            } else {
                OLC_CONFIG(d)->play.max_house_objs = MIN(MAX(atoi(arg), 0), 250);
                cedit_disp_game_play_options(d);
            }
            break;

        /* Emotion Display Thresholds */
        case CEDIT_EMOTION_DISPLAY_FEAR_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.display_fear_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_DISPLAY_ANGER_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.display_anger_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_DISPLAY_HAPPINESS_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.display_happiness_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_DISPLAY_SADNESS_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.display_sadness_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_DISPLAY_HORROR_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.display_horror_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_DISPLAY_PAIN_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.display_pain_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        /* Emotion Flee Thresholds */
        case CEDIT_EMOTION_FLEE_FEAR_LOW_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.flee_fear_low_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_FLEE_FEAR_HIGH_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.flee_fear_high_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_FLEE_COURAGE_LOW_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.flee_courage_low_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_FLEE_COURAGE_HIGH_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.flee_courage_high_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_FLEE_HORROR_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.flee_horror_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        /* Emotion Flee Modifiers */
        case CEDIT_EMOTION_FLEE_FEAR_LOW_MODIFIER:
            OLC_CONFIG(d)->emotion_config.flee_fear_low_modifier = LIMIT(atoi(arg), -100, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_FLEE_FEAR_HIGH_MODIFIER:
            OLC_CONFIG(d)->emotion_config.flee_fear_high_modifier = LIMIT(atoi(arg), -100, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_FLEE_COURAGE_LOW_MODIFIER:
            OLC_CONFIG(d)->emotion_config.flee_courage_low_modifier = LIMIT(atoi(arg), -100, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_FLEE_COURAGE_HIGH_MODIFIER:
            OLC_CONFIG(d)->emotion_config.flee_courage_high_modifier = LIMIT(atoi(arg), -100, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_FLEE_HORROR_MODIFIER:
            OLC_CONFIG(d)->emotion_config.flee_horror_modifier = LIMIT(atoi(arg), -100, 100);
            cedit_disp_emotion_menu(d);
            break;

        /* Pain Damage Thresholds */
        case CEDIT_EMOTION_PAIN_DAMAGE_MINOR_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.pain_damage_minor_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_DAMAGE_MODERATE_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.pain_damage_moderate_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_DAMAGE_HEAVY_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.pain_damage_heavy_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_DAMAGE_MASSIVE_THRESHOLD:
            OLC_CONFIG(d)->emotion_config.pain_damage_massive_threshold = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        /* Pain Amounts */
        case CEDIT_EMOTION_PAIN_MINOR_MIN:
            OLC_CONFIG(d)->emotion_config.pain_minor_min = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_MINOR_MAX:
            OLC_CONFIG(d)->emotion_config.pain_minor_max = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_MODERATE_MIN:
            OLC_CONFIG(d)->emotion_config.pain_moderate_min = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_MODERATE_MAX:
            OLC_CONFIG(d)->emotion_config.pain_moderate_max = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_HEAVY_MIN:
            OLC_CONFIG(d)->emotion_config.pain_heavy_min = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_HEAVY_MAX:
            OLC_CONFIG(d)->emotion_config.pain_heavy_max = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_MASSIVE_MIN:
            OLC_CONFIG(d)->emotion_config.pain_massive_min = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_PAIN_MASSIVE_MAX:
            OLC_CONFIG(d)->emotion_config.pain_massive_max = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        /* Memory Weights */
        case CEDIT_EMOTION_MEMORY_WEIGHT_RECENT:
            OLC_CONFIG(d)->emotion_config.memory_weight_recent = LIMIT(atoi(arg), 1, 10);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_MEMORY_WEIGHT_FRESH:
            OLC_CONFIG(d)->emotion_config.memory_weight_fresh = LIMIT(atoi(arg), 1, 10);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_MEMORY_WEIGHT_MODERATE:
            OLC_CONFIG(d)->emotion_config.memory_weight_moderate = LIMIT(atoi(arg), 1, 10);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_MEMORY_WEIGHT_OLD:
            OLC_CONFIG(d)->emotion_config.memory_weight_old = LIMIT(atoi(arg), 1, 10);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_MEMORY_WEIGHT_ANCIENT:
            OLC_CONFIG(d)->emotion_config.memory_weight_ancient = LIMIT(atoi(arg), 1, 10);
            cedit_disp_emotion_menu(d);
            break;

        /* Memory Age Thresholds */
        case CEDIT_EMOTION_MEMORY_AGE_RECENT:
            OLC_CONFIG(d)->emotion_config.memory_age_recent = MAX(atoi(arg), 1);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_MEMORY_AGE_FRESH:
            OLC_CONFIG(d)->emotion_config.memory_age_fresh = MAX(atoi(arg), 1);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_MEMORY_AGE_MODERATE:
            OLC_CONFIG(d)->emotion_config.memory_age_moderate = MAX(atoi(arg), 1);
            cedit_disp_emotion_menu(d);
            break;

        case CEDIT_EMOTION_MEMORY_AGE_OLD:
            OLC_CONFIG(d)->emotion_config.memory_age_old = MAX(atoi(arg), 1);
            cedit_disp_emotion_menu(d);
            break;

        /* Memory Baseline Offset */
        case CEDIT_EMOTION_MEMORY_BASELINE_OFFSET:
            OLC_CONFIG(d)->emotion_config.memory_baseline_offset = LIMIT(atoi(arg), 0, 100);
            cedit_disp_emotion_menu(d);
            break;

        default: /* We should never get here, but just in
                            case... */
            cleanup_olc(d, CLEANUP_CONFIG);
            mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: cedit_parse(): Reached default case!");
            write_to_output(d, "Oops...\r\n");
            break;
    }
} /* End of parse_cedit() */

static void reassign_rooms(void)
{
    void assign_rooms(void);
    int i;

    /* remove old funcs */
    for (i = 0; i < top_of_world; i++)
        world[i].func = NULL;

    /* reassign spec_procs */
    assign_rooms();
}

void cedit_string_cleanup(struct descriptor_data *d, int terminator)
{
    switch (OLC_MODE(d)) {
        case CEDIT_MENU:
        case CEDIT_WELC_MESSG:
        case CEDIT_START_MESSG:
            cedit_disp_operation_options(d);
            break;
    }
}
