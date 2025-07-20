/**************************************************************************
 *  File: interpreter.c                                     Part of tbaMUD *
 *  Usage: Parse user commands, search for specials, call ACMD functions.  *
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
#include "db.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "constants.h"
#include "act.h" /* ACMDs located within the act*.c files */
#include "ban.h"
#include "class.h"
#include "graph.h"
#include "hedit.h"
#include "house.h"
#include "config.h"
#include "modify.h" /* for do_skillset... */
#include "quest.h"
#include "asciimap.h"
#include "prefedit.h"
#include "ibt.h"
#include "mud_event.h"
#include "spedit.h"
#include "fight.h"

ACMD(do_formula);

/* local (file scope) functions */
static int perform_dupe_check(struct descriptor_data *d);
static struct alias_data *find_alias(struct alias_data *alias_list, char *str);
static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
static int _parse_name(char *arg, char *name);
static bool perform_new_char_dupe_check(struct descriptor_data *d);
/* sort_commands utility */
static int sort_commands_helper(const void *a, const void *b);
int parse_hometown(char arg);
void hometown_menu(struct descriptor_data *d);

/* globals defined here, used here and elsewhere */
int *cmd_sort_info = NULL;

struct command_info *complete_cmd_info;

/* This is the Master Command List. You can put new commands in, take commands
   out, change the order they appear in, etc.  You can adjust the "priority" of
   commands simply by changing the order they appear in the command list. (For
   example, if you want "as" to mean "assist" instead of "ask", just put
   "assist" above "ask" in the Master Command List. In general, utility
   commands such as "at" should have high priority; infrequently used and
   dangerously destructive commands should have low priority. */

cpp_extern const struct command_info cmd_info[] = {
    {"RESERVED", "", 0, 0, 0, 0, 0}, /* this must be first -- for specprocs
                                      */

    /* directions must come before other commands but after RESERVED */
    {"north", "n", POS_STANDING, do_move, 0, SCMD_NORTH, CMD_NOARG},
    {"east", "e", POS_STANDING, do_move, 0, SCMD_EAST, CMD_NOARG},
    {"south", "s", POS_STANDING, do_move, 0, SCMD_SOUTH, CMD_NOARG},
    {"west", "w", POS_STANDING, do_move, 0, SCMD_WEST, CMD_NOARG},
    {"up", "u", POS_STANDING, do_move, 0, SCMD_UP, CMD_NOARG},
    {"down", "d", POS_STANDING, do_move, 0, SCMD_DOWN, CMD_NOARG},
    {"northwest", "northw", POS_STANDING, do_move, 0, SCMD_NW, CMD_NOARG},
    {"nw", "nw", POS_STANDING, do_move, 0, SCMD_NW, CMD_NOARG},
    {"northeast", "northe", POS_STANDING, do_move, 0, SCMD_NE, CMD_NOARG},
    {"ne", "ne", POS_STANDING, do_move, 0, SCMD_NE, CMD_NOARG},
    {"southeast", "southe", POS_STANDING, do_move, 0, SCMD_SE, CMD_NOARG},
    {"se", "se", POS_STANDING, do_move, 0, SCMD_SE, CMD_NOARG},
    {"southwest", "southw", POS_STANDING, do_move, 0, SCMD_SW, CMD_NOARG},
    {"sw", "sw", POS_STANDING, do_move, 0, SCMD_SW, CMD_NOARG},

    /* now, the main list */
    {"at", "at", POS_DEAD, do_at, LVL_IMMORT, 0, CMD_THREEARG},
    {"advance", "adv", POS_DEAD, do_advance, LVL_GRGOD, 0, CMD_TWOARG},
    {"aedit", "aed", POS_DEAD, do_oasis_aedit, LVL_GOD, 0, CMD_NOARG},
    {"alias", "ali", POS_DEAD, do_alias, 0, 0, CMD_NOARG},
    {"areas", "are", POS_DEAD, do_areas, 0, 0, CMD_NOARG},
    {"assist", "as", POS_FIGHTING, do_assist, 1, 0, CMD_ONEARG},
    {"ask", "ask", POS_RESTING, do_spec_comm, 0, SCMD_ASK, CMD_TWOARG},
    {"astat", "ast", POS_DEAD, do_astat, 0, 0, CMD_NOARG},
    {"attach", "attach", POS_DEAD, do_attach, LVL_BUILDER, 0, CMD_NOARG},
    {"auction", "auc", POS_SLEEPING, do_gen_comm, 0, SCMD_AUCTION, CMD_TWOARG},
    {"autoexits", "autoex", POS_DEAD, do_gen_tog, 0, SCMD_AUTOEXIT, CMD_NOARG},
    {"autoassist", "autoass", POS_DEAD, do_gen_tog, 0, SCMD_AUTOASSIST, CMD_NOARG},
    {"autodoor", "autodoor", POS_DEAD, do_gen_tog, 0, SCMD_AUTODOOR, CMD_NOARG},
    {"autogold", "autogold", POS_DEAD, do_gen_tog, 0, SCMD_AUTOGOLD, CMD_NOARG},
    {"autokey", "autokey", POS_DEAD, do_gen_tog, 0, SCMD_AUTOKEY, CMD_NOARG},
    {"autoloot", "autoloot", POS_DEAD, do_gen_tog, 0, SCMD_AUTOLOOT, CMD_NOARG},
    {"automap", "automap", POS_DEAD, do_gen_tog, 0, SCMD_AUTOMAP, CMD_NOARG},
    {"autosac", "autosac", POS_DEAD, do_gen_tog, 0, SCMD_AUTOSAC, CMD_NOARG},
    {"autosplit", "autospl", POS_DEAD, do_gen_tog, 0, SCMD_AUTOSPLIT, CMD_NOARG},
    {"autotitle", "autotitle", POS_DEAD, do_autotitle, 0, 0, CMD_NOARG},
    {"away", "aw", POS_DEAD, do_gen_tog, 0, SCMD_AFK, CMD_NOARG},
    {"backstab", "ba", POS_STANDING, do_cast, 1, SKILL_BACKSTAB, CMD_ONEARG},
    {"backflip", "bac", POS_FIGHTING, do_cast, 1, SKILL_BACKFLIP, CMD_ONEARG},
    {"ban", "ban", POS_DEAD, do_ban, LVL_GRGOD, 0, CMD_ONEARG},
    {"bandage", "band", POS_RESTING, do_cast, 1, SKILL_BANDAGE, CMD_ONEARG},
    {"balance", "bal", POS_STANDING, do_not_here, 1, 0, CMD_NOARG},
    {"bash", "bas", POS_FIGHTING, do_cast, 1, SKILL_BASH, CMD_ONEARG},
    {"brief", "br", POS_DEAD, do_gen_tog, 0, SCMD_BRIEF, CMD_NOARG},
    {"buildwalk", "buildwalk", POS_STANDING, do_gen_tog, LVL_BUILDER, SCMD_BUILDWALK, CMD_NOARG},
    {"buy", "bu", POS_STANDING, do_not_here, 0, 0, CMD_TWOARG},
    {"bug", "bug", POS_DEAD, do_ibt, 0, SCMD_BUG, CMD_NOARG},
    {"cast", "c", POS_SITTING, do_cast, 1, SCMD_SPELL, CMD_TWOARG},
    {"cedit", "cedit", POS_DEAD, do_oasis_cedit, LVL_IMPL, CMD_NOARG},
    {"changelog", "cha", POS_DEAD, do_changelog, LVL_IMPL, CMD_NOARG},
    {"check", "ch", POS_STANDING, do_not_here, 1, CMD_NOARG},
    {"checkload", "checkl", POS_DEAD, do_checkloadstatus, LVL_GOD, CMD_TWOARG},
    {"close", "cl", POS_SITTING, do_gen_door, 0, SCMD_CLOSE, CMD_TWOARG},
    {"clear", "cle", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, CMD_NOARG},
    {"cls", "cls", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, CMD_NOARG},
    {"consider", "con", POS_RESTING, do_consider, 0, 0, CMD_ONEARG},
    {"commands", "com", POS_DEAD, do_commands, 0, SCMD_COMMANDS, CMD_NOARG},
    {"combo", "comb", POS_FIGHTING, do_cast, 1, SKILL_COMBO_ATTACK, CMD_ONEARG},
    {"compact", "comp", POS_DEAD, do_gen_tog, 0, SCMD_COMPACT, CMD_NOARG},
    {"copyover", "copyover", POS_DEAD, do_copyover, LVL_GRGOD, 0, CMD_NOARG},
    {"credits", "cred", POS_DEAD, do_gen_ps, 0, SCMD_CREDITS, CMD_NOARG},
    {"date", "da", POS_DEAD, do_date, LVL_IMMORT, SCMD_DATE, CMD_NOARG},
    {"dc", "dc", POS_DEAD, do_dc, LVL_GOD, 0, CMD_ONEARG},
    {"deposit", "depo", POS_STANDING, do_not_here, 1, 0, CMD_ONEARG},
    {"detach", "detach", POS_DEAD, do_detach, LVL_BUILDER, 0, CMD_NOARG},
    {"diagnose", "diag", POS_RESTING, do_diagnose, 0, 0, CMD_ONEARG},
    {"dig", "dig", POS_DEAD, do_dig, LVL_BUILDER, 0, CMD_ONEARG},
    {"display", "disp", POS_DEAD, do_display, 0, 0, CMD_NOARG},
    {"donate", "don", POS_RESTING, do_drop, 0, SCMD_DONATE, CMD_ONEARG},
    {"drink", "dri", POS_RESTING, do_drink, 0, SCMD_DRINK, CMD_ONEARG},
    {"drop", "dro", POS_RESTING, do_drop, 0, SCMD_DROP, CMD_ONEARG},
    {"eat", "ea", POS_RESTING, do_eat, 0, SCMD_EAT, CMD_ONEARG},
    {"echo", "ec", POS_SLEEPING, do_echo, LVL_IMMORT, SCMD_ECHO, CMD_NOARG},
    {"emote", "em", POS_RESTING, do_echo, 0, SCMD_EMOTE, CMD_NOARG},
    {":", ":", POS_RESTING, do_echo, 1, SCMD_EMOTE, CMD_NOARG},
    {"enter", "ent", POS_STANDING, do_enter, 0, 0, CMD_ONEARG},
    {"envenom", "env", POS_FIGHTING, do_cast, 1, SKILL_ENVENOM, CMD_TWOARG},
    {"equipment", "eq", POS_SLEEPING, do_equipment, 0, 0, CMD_NOARG},
    {"evaluate", "evalu", POS_SITTING, do_evaluate, 0, 0, CMD_TWOARG},
    {"exits", "ex", POS_RESTING, do_exits, 0, 0, CMD_NOARG},
    {"examine", "exa", POS_SITTING, do_examine, 0, 0, CMD_ONEARG},
    {"export", "export", POS_DEAD, do_export_zone, LVL_IMPL, 0, CMD_NOARG},
    {"force", "force", POS_SLEEPING, do_force, LVL_GOD, 0, CMD_TWOARG},
    {"fill", "fil", POS_STANDING, do_pour, 0, SCMD_FILL, CMD_TWOARG},
    {"file", "file", POS_SLEEPING, do_file, LVL_GOD, 0, CMD_NOARG},
    {"flee", "fl", POS_FIGHTING, do_flee, 1, 0, CMD_NOARG},
    {"fly", "fly", POS_DEAD, do_fly, 1, SCMD_FLY, CMD_NOARG},
    {"follow", "fol", POS_RESTING, do_follow, 0, 0, CMD_ONEARG},
    {"formula", "form", POS_DEAD, do_formula, LVL_GOD, 0, CMD_NOARG},
    {"freeze", "freeze", POS_DEAD, do_wizutil, LVL_GRGOD, SCMD_FREEZE, CMD_ONEARG},
    {"get", "g", POS_RESTING, do_get, 0, 0, CMD_TWOARG},
    {"gassist", "gas", POS_FIGHTING, do_gassist, 1, 0, CMD_NOARG},
    {"gecho", "gecho", POS_DEAD, do_gecho, LVL_GOD, 0, CMD_NOARG},
    {"gemote", "gem", POS_SLEEPING, do_gen_comm, 0, SCMD_GEMOTE, CMD_TWOARG},
    {"give", "giv", POS_RESTING, do_give, 0, 0, CMD_TWOARG},
    {"goto", "go", POS_SLEEPING, do_goto, LVL_IMMORT, 0, CMD_ONEARG},
    {"gold", "gol", POS_RESTING, do_gold, 0, 0, CMD_NOARG},
    {"gossip", "gos", POS_SLEEPING, do_gen_comm, 0, SCMD_GOSSIP, CMD_NOARG},
    {"gstats", "gst", POS_DEAD, do_gstats, LVL_IMMORT, 0, CMD_TWOARG},
    {"group", "gr", POS_RESTING, do_group, 1, 0, CMD_TWOARG},
    {"grab", "grab", POS_RESTING, do_grab, 0, 0, CMD_ONEARG},
    {"grats", "grat", POS_SLEEPING, do_gen_comm, 0, SCMD_GRATZ, CMD_NOARG},
    {"gsay", "gsay", POS_SLEEPING, do_gsay, 0, 0, CMD_NOARG},
    {"gtell", "gt", POS_SLEEPING, do_gsay, 0, 0, CMD_NOARG},
    {"help", "h", POS_DEAD, do_help, 0, 0, CMD_NOARG},
    {"happyhour", "ha", POS_DEAD, do_happyhour, 0, 0, CMD_NOARG},
    {"hedit", "hedit", POS_DEAD, do_oasis_hedit, LVL_GOD, 0, CMD_NOARG},
    {"helpcheck", "helpch", POS_DEAD, do_helpcheck, LVL_GOD, 0, CMD_NOARG},
    {"hide", "hi", POS_RESTING, do_cast, 1, SKILL_HIDE, CMD_NOARG},
    {"hindex", "hind", POS_DEAD, do_hindex, 0, 0, CMD_NOARG},
    {"handbook", "handb", POS_DEAD, do_gen_ps, LVL_IMMORT, SCMD_HANDBOOK, CMD_NOARG},
    {"hcontrol", "hcontrol", POS_DEAD, do_hcontrol, LVL_GRGOD, 0, CMD_NOARG},
    {"history", "history", POS_DEAD, do_history, 0, 0, CMD_NOARG},
    {"hit", "hit", POS_FIGHTING, do_hit, 0, SCMD_HIT, CMD_ONEARG},
    {"hold", "hold", POS_RESTING, do_grab, 1, 0, CMD_ONEARG},
    {"holler", "holler", POS_RESTING, do_gen_comm, 1, SCMD_HOLLER, CMD_NOARG},
    {"holylight", "holy", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_HOLYLIGHT, CMD_NOARG},
    {"house", "house", POS_RESTING, do_house, 0, 0, CMD_ONEARG},
    {"inventory", "i", POS_DEAD, do_inventory, 0, 0, CMD_NOARG},
    {"identify", "id", POS_STANDING, do_not_here, 1, 0, CMD_ONEARG},
    {"idea", "ide", POS_DEAD, do_ibt, 0, SCMD_IDEA, CMD_NOARG},
    {"imotd", "imo", POS_DEAD, do_gen_ps, LVL_IMMORT, SCMD_IMOTD, CMD_NOARG},
    {"immlist", "imm", POS_DEAD, do_gen_ps, 0, SCMD_IMMLIST, CMD_NOARG},
    {"info", "info", POS_SLEEPING, do_gen_ps, 0, SCMD_INFO, CMD_NOARG},
    {"invis", "invi", POS_DEAD, do_invis, LVL_IMMORT, 0, CMD_ONEARG},
    {"junk", "j", POS_RESTING, do_drop, 0, SCMD_JUNK, CMD_ONEARG},
    {"kill", "k", POS_FIGHTING, do_kill, 0, 0, CMD_ONEARG},
    {"kick", "ki", POS_FIGHTING, do_cast, 1, SKILL_KICK, CMD_ONEARG},
    {"look", "l", POS_RESTING, do_look, 0, SCMD_LOOK, CMD_NOARG},
    {"last", "last", POS_DEAD, do_last, LVL_GOD, 0, CMD_NOARG},
    {"land", "lan", POS_DEAD, do_fly, 1, SCMD_LAND, CMD_NOARG},
    {"leave", "lea", POS_STANDING, do_leave, 0, 0, CMD_NOARG},
    {"levels", "lev", POS_DEAD, do_levels, 0, 0, CMD_NOARG},
    {"list", "lis", POS_STANDING, do_not_here, 0, 0, CMD_NOARG},
    {"links", "lin", POS_STANDING, do_links, LVL_GOD, 0, CMD_NOARG},
    {"lock", "loc", POS_SITTING, do_gen_door, 0, SCMD_LOCK, CMD_TWOARG},
    {"load", "load", POS_DEAD, do_load, LVL_BUILDER, 0, CMD_TWOARG},
    {"motd", "motd", POS_DEAD, do_gen_ps, 0, SCMD_MOTD, CMD_NOARG},
    {"mail", "mail", POS_STANDING, do_not_here, 1, 0, CMD_ONEARG},
    {"map", "map", POS_STANDING, do_map, 1, 0, CMD_NOARG},
    {"medit", "med", POS_DEAD, do_oasis_medit, LVL_BUILDER, 0, CMD_ONEARG},
    {"meditate", "medi", POS_DEAD, do_cast, 1, SKILL_MEDITATE, CMD_NOARG},
    {"mlist", "mlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_MLIST, CMD_ONEARG},
    {"mcopy", "mcopy", POS_DEAD, do_oasis_copy, LVL_GOD, CON_MEDIT, CMD_TWOARG},
    {"msgedit", "msgedit", POS_DEAD, do_msgedit, LVL_GOD, 0, CMD_NOARG},
    {"mwishlist", "mwish", POS_DEAD, do_mwishlist, LVL_IMMORT, 0, CMD_ONEARG},
    {"mwant", "mwant", POS_DEAD, do_mwant, LVL_IMMORT, 0, CMD_ONEARG},
    {"gedit", "gedit", POS_DEAD, do_oasis_gedit, LVL_BUILDER, 0, CMD_ONEARG},
    {"murder", "mur", POS_FIGHTING, do_hit, 0, SCMD_MURDER, CMD_ONEARG},
    {"mute", "mute", POS_DEAD, do_wizutil, LVL_GOD, SCMD_MUTE, CMD_ONEARG},
    {"news", "news", POS_SLEEPING, do_gen_ps, 0, SCMD_NEWS, CMD_NOARG},
    {"noauction", "noauction", POS_DEAD, do_gen_tog, 0, SCMD_NOAUCTION, CMD_NOARG},
    {"nogossip", "nogossip", POS_DEAD, do_gen_tog, 0, SCMD_NOGOSSIP, CMD_NOARG},
    {"nograts", "nograts", POS_DEAD, do_gen_tog, 0, SCMD_NOGRATZ, CMD_NOARG},
    {"nohassle", "nohassle", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NOHASSLE, CMD_NOARG},
    {"norepeat", "norepeat", POS_DEAD, do_gen_tog, 0, SCMD_NOREPEAT, CMD_NOARG},
    {"noshout", "noshout", POS_SLEEPING, do_gen_tog, 1, SCMD_NOSHOUT, CMD_NOARG},
    {"nosummon", "nosummon", POS_DEAD, do_gen_tog, 1, SCMD_NOSUMMON, CMD_NOARG},
    {"notell", "notell", POS_DEAD, do_gen_tog, 1, SCMD_NOTELL, CMD_NOARG},
    {"notitle", "notitle", POS_DEAD, do_wizutil, LVL_GOD, SCMD_NOTITLE, CMD_NOARG},
    {"nowiz", "nowiz", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NOWIZ, CMD_NOARG},
    {"open", "o", POS_SITTING, do_gen_door, 0, SCMD_OPEN, CMD_TWOARG},
    {"order", "ord", POS_RESTING, do_order, 1, 0, CMD_TWOARG},
    {"offer", "off", POS_STANDING, do_not_here, 1, 0, CMD_NOARG},
    {"olc", "olc", POS_DEAD, do_show_save_list, LVL_BUILDER, 0, CMD_NOARG},
    {"olist", "olist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_OLIST, CMD_ONEARG},
    {"oedit", "oedit", POS_DEAD, do_oasis_oedit, LVL_BUILDER, 0, CMD_ONEARG},
    {"oset", "oset", POS_DEAD, do_oset, LVL_BUILDER, 0, CMD_TWOARG},
    {"ocopy", "ocopy", POS_DEAD, do_oasis_copy, LVL_GOD, CON_OEDIT, CMD_TWOARG},
    {"put", "p", POS_RESTING, do_put, 0, 0, CMD_TWOARG},
    {"peace", "pe", POS_DEAD, do_peace, LVL_BUILDER, 0, CMD_NOARG},
    {"pick", "pi", POS_STANDING, do_gen_door, 1, SCMD_PICK, CMD_TWOARG},
    {"practice", "pr", POS_RESTING, do_practice, 1, 0, CMD_ONEARG},
    {"page", "pag", POS_DEAD, do_page, 1, 0, CMD_NOARG},
    {"pardon", "pardon", POS_DEAD, do_wizutil, LVL_GOD, SCMD_PARDON, CMD_ONEARG},
    {"plist", "plist", POS_DEAD, do_plist, LVL_GOD, 0, CMD_NOARG},
    {"plrload", "plrload", POS_DEAD, do_plrload, LVL_GRGOD, 0, CMD_ONEARG},
    {"policy", "pol", POS_DEAD, do_gen_ps, 0, SCMD_POLICIES, CMD_NOARG},
    {"pour", "pour", POS_STANDING, do_pour, 0, SCMD_POUR, CMD_TWOARG},
    {"pray", "pra", POS_RESTING, do_pray, 0, 0, CMD_NOARG},
    {"prompt", "pro", POS_DEAD, do_display, 0, 0, CMD_NOARG},
    {"prefedit", "pre", POS_DEAD, do_oasis_prefedit, 0, 0, CMD_NOARG},
    {"purge", "purge", POS_DEAD, do_purge, LVL_BUILDER, 0, CMD_NOARG},
    {"qedit", "qedit", POS_DEAD, do_oasis_qedit, LVL_BUILDER, 0, CMD_NOARG},
    {"qlist", "qlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_QLIST, CMD_ONEARG},
    {"quaff", "qua", POS_RESTING, do_use, 0, SCMD_QUAFF, CMD_ONEARG},
    {"qecho", "qec", POS_DEAD, do_qcomm, LVL_GOD, SCMD_QECHO, CMD_NOARG},
    {"quest", "que", POS_DEAD, do_quest, 0, 0, CMD_NOARG},
    {"qui", "qui", POS_DEAD, do_quit, 0, 0, CMD_NOARG},
    {"quit", "quit", POS_DEAD, do_quit, 0, SCMD_QUIT, CMD_NOARG},
    {"qsay", "qsay", POS_RESTING, do_qcomm, 0, SCMD_QSAY, CMD_NOARG},

    {"reply", "r", POS_SLEEPING, do_reply, 0, 0, CMD_NOARG},
    {"rest", "res", POS_RESTING, do_rest, 0, 0, CMD_NOARG},
    {"read", "rea", POS_RESTING, do_look, 0, SCMD_READ, CMD_ONEARG},
    {"rebegin", "rebegin", POS_STANDING, do_rebegin, 0, 0, CMD_NOARG},
    {"elevate", "elevate", POS_STANDING, do_elevate, 0, 0, CMD_NOARG},
    {"reload", "reload", POS_DEAD, do_reboot, LVL_IMPL, 0, CMD_NOARG},
    {"recall", "reca", POS_RESTING, do_recall, 0, 0, CMD_NOARG},
    {"recite", "reci", POS_RESTING, do_use, 0, SCMD_RECITE, CMD_TWOARG},
    {"receive", "rece", POS_STANDING, do_not_here, 1, 0, CMD_NOARG},
    {"recent", "recent", POS_DEAD, do_recent, LVL_IMMORT, 0, CMD_NOARG},
    {"rskill", "rskill", POS_DEAD, do_rskill, LVL_GOD, 0, CMD_TWOARG},
    {"rstats", "rstats", POS_DEAD, do_rstats, LVL_IMMORT, 0, CMD_TWOARG},
    {"remove", "rem", POS_RESTING, do_remove, 0, 0, CMD_ONEARG},
    {"rent", "rent", POS_STANDING, do_not_here, 1, 0, CMD_NOARG},
    {"report", "repo", POS_RESTING, do_report, 0, 0, CMD_NOARG},
    {"reroll", "rero", POS_DEAD, do_wizutil, LVL_GRGOD, SCMD_REROLL, CMD_ONEARG},
    {"rescue", "resc", POS_FIGHTING, do_cast, 1, SKILL_RESCUE, CMD_ONEARG},
    {"ressucite", "ress", POS_DEAD, do_ressucite, LVL_GOD, 0, CMD_ONEARG},
    {"reset", "reset", POS_RESTING, do_not_here, 0, 0, CMD_NOARG},
    {"restore", "resto", POS_DEAD, do_restore, LVL_GOD, 0, CMD_ONEARG},
    {"return", "retu", POS_DEAD, do_return, 0, 0, CMD_NOARG},
    {"redit", "redit", POS_DEAD, do_oasis_redit, LVL_BUILDER, 0, CMD_NOARG},
    {"rlist", "rlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_RLIST, CMD_ONEARG},
    {"rcopy", "rcopy", POS_DEAD, do_oasis_copy, LVL_GOD, CON_REDIT, CMD_NOARG},
    {"roomflags", "roomflags", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_SHOWVNUMS, CMD_NOARG},

    {"say", "s", POS_RESTING, do_say, 0, 0, CMD_NOARG},
    {"sacrifice", "sac", POS_RESTING, do_sac, 0, 0, CMD_ONEARG},
    {"score", "sc", POS_DEAD, do_score, 0, 0, CMD_NOARG},
    {"scan", "sca", POS_RESTING, do_cast, 1, SKILL_SCAN, CMD_NOARG},
    {"scopy", "scopy", POS_DEAD, do_oasis_copy, LVL_GOD, CON_SEDIT, CMD_NOARG},
    {"sit", "si", POS_RESTING, do_sit, 0, 0, CMD_NOARG},
    {"'", "'", POS_RESTING, do_say, 0, 0, CMD_NOARG},
    {"save", "sav", POS_SLEEPING, do_save, 0, 0, CMD_NOARG},
    {"saveall", "saveall", POS_DEAD, do_saveall, LVL_BUILDER, 0, CMD_NOARG},
    {"sell", "sell", POS_STANDING, do_not_here, 0, 0, CMD_TWOARG},
    {"seize", "sei", POS_FIGHTING, do_cast, 1, SKILL_SEIZE, CMD_ONEARG},
    {"sedit", "sedit", POS_DEAD, do_oasis_sedit, LVL_BUILDER, 0, CMD_NOARG},
    {"send", "send", POS_SLEEPING, do_send, LVL_GOD, 0, CMD_NOARG},
    {"set", "set", POS_DEAD, do_set, LVL_IMMORT, 0, CMD_NOARG},
    {"shout", "sho", POS_RESTING, do_gen_comm, 0, SCMD_SHOUT, CMD_NOARG},
    {"shoot", "shoo", POS_RESTING, do_cast, 0, SKILL_BOWS, CMD_TWOARG},
    {"show", "show", POS_DEAD, do_show, LVL_IMMORT, 0, CMD_NOARG},
    {"shutdow", "shutdow", POS_DEAD, do_shutdown, LVL_IMPL, 0, CMD_NOARG},
    {"shutdown", "shutdown", POS_DEAD, do_shutdown, LVL_IMPL, SCMD_SHUTDOWN, CMD_NOARG},
    {"sip", "sip", POS_RESTING, do_drink, 0, SCMD_SIP, CMD_ONEARG},
    {"sing", "sin", POS_SITTING, do_cast, 1, SCMD_CHANSON, CMD_TWOARG},
    {"skillset", "skillset", POS_SLEEPING, do_skillset, LVL_GRGOD, 0, CMD_NOARG},
    {"sleep", "sl", POS_SLEEPING, do_sleep, 0, 0, CMD_NOARG},
    {"slist", "slist", POS_SLEEPING, do_oasis_list, LVL_BUILDER, SCMD_OASIS_SLIST, CMD_ONEARG},
    {"sneak", "sneak", POS_STANDING, do_cast, 1, SKILL_SNEAK, CMD_NOARG},
    {"snoop", "snoop", POS_DEAD, do_snoop, LVL_GOD, 0, CMD_ONEARG},
    {"spedit", "spe", POS_DEAD, do_spedit, LVL_GRGOD, 0, CMD_NOARG},
    {"splist", "splist", POS_DEAD, do_splist, LVL_BUILDER, 0, CMD_NOARG},
    {"socials", "socials", POS_DEAD, do_commands, 0, SCMD_SOCIALS, CMD_NOARG},
    {"split", "split", POS_SITTING, do_split, 1, 0, CMD_ONEARG},
    {"spy", "spy", POS_RESTING, do_cast, 1, SKILL_SPY, CMD_ONEARG},
    {"stand", "st", POS_RESTING, do_stand, 0, 0, CMD_NOARG},
    {"stat", "stat", POS_DEAD, do_stat, LVL_IMMORT, 0, CMD_ONEARG},
    {"steal", "ste", POS_STANDING, do_cast, 1, SKILL_STEAL, CMD_TWOARG},
    {"switch", "switch", POS_DEAD, do_switch, LVL_GOD, 0, CMD_ONEARG},

    {"tell", "t", POS_DEAD, do_tell, 0, 0, CMD_NOARG},
    {"take", "ta", POS_RESTING, do_get, 0, 0, CMD_TWOARG},
    {"taste", "tas", POS_RESTING, do_eat, 0, SCMD_TASTE, CMD_ONEARG},
    {"teleport", "tele", POS_DEAD, do_teleport, LVL_BUILDER, 0, CMD_TWOARG},
    {"tedit", "tedit", POS_DEAD, do_tedit, LVL_GOD, 0, CMD_NOARG}, /* XXX: Oasisify */
    {"thaw", "thaw", POS_DEAD, do_wizutil, LVL_GRGOD, SCMD_THAW, CMD_ONEARG},
    {"title", "title", POS_DEAD, do_title, 0, 0, CMD_NOARG},
    {"time", "time", POS_DEAD, do_time, 0, 0, CMD_NOARG},
    {"toggle", "toggle", POS_DEAD, do_toggle, 0, 0, CMD_NOARG},
    {"track", "track", POS_STANDING, do_cast, 0, SKILL_TRACK, CMD_ONEARG},
    {"transfer", "transfer", POS_SLEEPING, do_trans, LVL_GOD, 0, CMD_ONEARG},
    {"trigedit", "trigedit", POS_DEAD, do_oasis_trigedit, LVL_BUILDER, 0, CMD_NOARG},
    {"trip", "tri", POS_FIGHTING, do_cast, 1, SKILL_TRIP, CMD_ONEARG},
    {"typo", "typo", POS_DEAD, do_ibt, 0, SCMD_TYPO, CMD_NOARG},
    {"tlist", "tlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_TLIST, CMD_ONEARG},
    {"tcopy", "tcopy", POS_DEAD, do_oasis_copy, LVL_GOD, CON_TRIGEDIT, CMD_NOARG},
    {"tstat", "tstat", POS_DEAD, do_tstat, LVL_BUILDER, 0, CMD_NOARG},

    {"unlock", "unlock", POS_SITTING, do_gen_door, 0, SCMD_UNLOCK, CMD_TWOARG},
    {"unban", "unban", POS_DEAD, do_unban, LVL_GRGOD, 0, CMD_ONEARG},
    {"unaffect", "unaffect", POS_DEAD, do_wizutil, LVL_GOD, SCMD_UNAFFECT, CMD_ONEARG},
    {"unfollow", "unf", POS_RESTING, do_unfollow, 0, 0, CMD_NOARG},
    {"uptime", "uptime", POS_DEAD, do_date, LVL_GOD, SCMD_UPTIME, CMD_NOARG},
    {"use", "use", POS_SITTING, do_use, 1, SCMD_USE, CMD_ONEARG},
    {"users", "users", POS_DEAD, do_users, LVL_GOD, 0, CMD_NOARG},

    {"value", "val", POS_STANDING, do_not_here, 0, 0, CMD_ONEARG},
    {"version", "ver", POS_DEAD, do_gen_ps, 0, SCMD_VERSION, CMD_NOARG},
    {"visible", "vis", POS_RESTING, do_visible, 1, 0, CMD_NOARG},
    {"vnum", "vnum", POS_DEAD, do_vnum, LVL_IMMORT, 0, CMD_NOARG},
    {"vstat", "vstat", POS_DEAD, do_vstat, LVL_IMMORT, 0, CMD_NOARG},
    {"vdelete", "vdelete", POS_DEAD, do_vdelete, LVL_BUILDER, 0, CMD_NOARG},

    {"wake", "wake", POS_MEDITING, do_wake, 0, 0, CMD_NOARG},
    {"want", "want", POS_RESTING, do_not_here, 0, 0, CMD_NOARG},
    {"wear", "wea", POS_RESTING, do_wear, 0, 0, CMD_ONEARG},
    {"weather", "weather", POS_RESTING, do_weather, 0, 0, CMD_NOARG},
    {"who", "wh", POS_DEAD, do_who, 0, 0, CMD_NOARG},
    {"whois", "whoi", POS_DEAD, do_whois, 0, 0, CMD_ONEARG},
    {"whoami", "whoami", POS_DEAD, do_gen_ps, 0, SCMD_WHOAMI, CMD_NOARG},
    {"where", "where", POS_RESTING, do_where, 1, 0, CMD_NOARG},
    {"whisper", "whisper", POS_RESTING, do_spec_comm, 0, SCMD_WHISPER, CMD_NOARG},
    {"whirlwind", "whirl", POS_FIGHTING, do_cast, 0, SKILL_WHIRLWIND, CMD_ONEARG},
    {"wield", "wie", POS_RESTING, do_wield, 0, 0, CMD_ONEARG},
    {"withdraw", "withdraw", POS_STANDING, do_not_here, 1, 0, CMD_ONEARG},
    {"wiznet", "wiz", POS_DEAD, do_wiznet, LVL_IMMORT, 0, CMD_NOARG},
    {";", ";", POS_DEAD, do_wiznet, LVL_IMMORT, 0, CMD_NOARG},
    {"wizhelp", "wizhelp", POS_DEAD, do_wizhelp, LVL_IMMORT, 0, CMD_NOARG},
    {"wizlist", "wizlist", POS_DEAD, do_gen_ps, 0, SCMD_WIZLIST, CMD_NOARG},
    {"wizupdate", "wizupde", POS_DEAD, do_wizupdate, LVL_GRGOD, 0, CMD_NOARG},
    {"wizlock", "wizlock", POS_DEAD, do_wizlock, LVL_IMPL, 0, CMD_ONEARG},
    {"write", "write", POS_STANDING, do_write, 1, 0, CMD_TWOARG},

    {"zoneresets", "zoner", POS_DEAD, do_gen_tog, LVL_IMPL, SCMD_ZONERESETS, CMD_NOARG},
    {"zreset", "zreset", POS_DEAD, do_zreset, LVL_BUILDER, 0, CMD_ONEARG},
    {"zedit", "zedit", POS_DEAD, do_oasis_zedit, LVL_BUILDER, 0, CMD_NOARG},
    {"zlist", "zlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_ZLIST, CMD_ONEARG},
    {"zlock", "zlock", POS_DEAD, do_zlock, LVL_GOD, 0, CMD_ONEARG},
    {"zunlock", "zunlock", POS_DEAD, do_zunlock, LVL_GOD, 0, CMD_ONEARG},
    {"zcheck", "zcheck", POS_DEAD, do_zcheck, LVL_BUILDER, 0, CMD_ONEARG},
    {"zpurge", "zpurge", POS_DEAD, do_zpurge, LVL_BUILDER, 0, CMD_ONEARG},

    {"\n", "zzzzzzz", 0, 0, 0, 0, CMD_NOARG}}; /* this must be last */

/* Thanks to Melzaren for this change to allow DG Scripts to be attachable
   to player's while still disallowing them to manually use the DG-Commands.
 */
static const struct mob_script_command_t mob_script_commands[] = {

    /* DG trigger commands. minimum_level should be set to -1. */
    {"masound", do_masound, 0},
    {"mkill", do_mkill, 0},
    {"mjunk", do_mjunk, 0},
    {"mdamage", do_mdamage, 0},
    {"mdoor", do_mdoor, 0},
    {"mecho", do_mecho, 0},
    {"mrecho", do_mrecho, 0},
    {"mechoaround", do_mechoaround, 0},
    {"msend", do_msend, 0},
    {"mload", do_mload, 0},
    {"mpurge", do_mpurge, 0},
    {"mgoto", do_mgoto, 0},
    {"mat", do_mat, 0},
    {"mteleport", do_mteleport, 0},
    {"mforce", do_mforce, 0},
    {"mhunt", do_mhunt, 0},
    {"mremember", do_mremember, 0},
    {"mforget", do_mforget, 0},
    {"mtransform", do_mtransform, 0},
    {"mzoneecho", do_mzoneecho, 0},
    {"mfollow", do_mfollow, 0},
    {"mlog", do_mlog, 0},
    {"\n", do_not_here, 0}};

int script_command_interpreter(struct char_data *ch, char *arg)
{
    /* DG trigger commands */

    int i;
    char first_arg[MAX_INPUT_LENGTH];
    char *line;

    skip_spaces(&arg);
    if (!*arg)
        return 0;

    line = any_one_arg(arg, first_arg);

    for (i = 0; *mob_script_commands[i].command_name != '\n'; i++)
        if (!str_cmp(first_arg, mob_script_commands[i].command_name))
            break;   // NB - only allow full matches.

    if (*mob_script_commands[i].command_name == '\n')
        return 0;   // no matching commands.

    /* Poiner to the command? */
    ((*mob_script_commands[i].command_pointer)(ch, line, 0, mob_script_commands[i].subcmd));
    return 1;   // We took care of execution. Let caller know.
}

static const char *fill[] = {"in", "from", "with", "the", "on", "at", "to",
                             /* pt */
                             "em", "de", "com", "no", "na", "para", "\n"};

static const char *reserved[] = {"a", "an", "self", "me", "all", "room", "someone", "something",
                                 /* pt */
                                 "um", "uma", "eu", "mim", "tudo", "todos", "todas", "algo", "alguém", "sala", "\n"};

static int sort_commands_helper(const void *a, const void *b)
{
    return strcmp(complete_cmd_info[*(const int *)a].sort_as, complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands(void)
{
    int a, num_of_cmds = 0;

    while (complete_cmd_info[num_of_cmds].command[0] != '\n')
        num_of_cmds++;
    num_of_cmds++; /* \n */

    CREATE(cmd_sort_info, int, num_of_cmds);

    for (a = 0; a < num_of_cmds; a++)
        cmd_sort_info[a] = a;

    /* Don't sort the RESERVED or \n entries. */
    qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}

/* This is the actual command interpreter called from game_loop() in comm.c It
   makes sure you are the proper level and position to execute the command,
   then calls the appropriate function. */
void command_interpreter(struct char_data *ch, char *argument)
{
    int cmd, length;
    char *line;
    char arg[MAX_INPUT_LENGTH];

    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct obj_data *object;
    int grupo;
    int count_obj = 0;

    /* verifica grupo e inventario */
    if (GROUP(ch) != NULL)
        grupo = 1;
    else
        grupo = 0;

    for (object = ch->carrying; object; object = object->next_content) {
        count_obj++;
    }

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

    /* just drop to next line for hitting CR */
    skip_spaces(&argument);
    if (!*argument)
        return;

    /* special case to handle one-character, non-alphanumeric commands;
       requested by many people so "'hi" or ";godnet test" is possible. Patch
       sent by Eric Green and Stefan Wasilewski. */
    if (!isalpha(*argument)) {
        arg[0] = argument[0];
        arg[1] = '\0';
        line = argument + 1;
    } else
        line = any_one_arg(argument, arg);

    /* Split argument for ann */
    two_arguments(line, arg1, arg2);

    /* Since all command triggers check for valid_dg_target before acting, the
       levelcheck here has been removed. Otherwise, find the command. */
    {
        int cont;                               /* continue the command checks */
        cont = command_wtrigger(ch, arg, line); /* any world triggers ? */
        if (!cont)
            cont = command_mtrigger(ch, arg, line); /* any mobile triggers ? */
        if (!cont)
            cont = command_otrigger(ch, arg, line); /* any object triggers ? */
        if (cont)
            return; /* yes, command trigger took over */
    }

    /* Allow IMPLs to switch into mobs to test the commands. */
    if (IS_NPC(ch) && ch->desc && GET_LEVEL(ch->desc->original) >= LVL_IMPL) {
        if (script_command_interpreter(ch, argument))
            return;
    }

    for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
        if (complete_cmd_info[cmd].command_pointer != do_action &&
            !strncmp(complete_cmd_info[cmd].command, arg, length))
            if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
                break;

    /* it's not a 'real' command, so it's a social */

    if (*complete_cmd_info[cmd].command == '\n')
        for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
            if (complete_cmd_info[cmd].command_pointer == do_action &&
                !strncmp(complete_cmd_info[cmd].command, arg, length))
                if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
                    break;

    if (*complete_cmd_info[cmd].command == '\n') {
        int found = 0;

        struct str_spells *skill = get_spell_by_name(arg, SKILL);

        if (skill) {
            do_cast(ch, line, 1, skill->vnum);
            return;
        }

        send_to_char(ch, "%s", CONFIG_HUH);
        for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
            if (*arg != *cmd_info[cmd].command || cmd_info[cmd].minimum_level > GET_LEVEL(ch))
                continue;
            /* Only apply levenshtein counts if the command is not a trigger
               command. */
            if ((levenshtein_distance(arg, cmd_info[cmd].command) <= 2) && (cmd_info[cmd].minimum_level >= 0)) {
                if (!found) {
                    send_to_char(ch, "\r\nVocê quis dizer:\r\n");
                    found = 1;
                }
                send_to_char(ch, "  %s\r\n", cmd_info[cmd].command);
            }
        }
    } else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
        send_to_char(ch, "Você tenta, mas o gelo impede...\r\n");
    else if (complete_cmd_info[cmd].command_pointer == NULL)
        send_to_char(ch, "Sinto, mas este comando ainda não foi implementado.\r\n");
    else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_level >= LVL_IMMORT)
        send_to_char(ch, "Você não pode usar comandos de imortais enquanto incorporado.\r\n");
    else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position)
        switch (GET_POS(ch)) {
            case POS_DEAD:
                act("Não dá... você está mort$r!!! :-(", FALSE, ch, 0, 0, TO_CHAR);
                break;
            case POS_INCAP:
            case POS_MORTALLYW:
                send_to_char(ch, "Você está em uma condição muito ruim, impossível fazer qualquer coisa!\r\n");
                break;
            case POS_STUNNED:
                send_to_char(ch, "Tudo o que você pode fazer agora é pensar em estrelas!\r\n");
                break;
            case POS_MEDITING:
                send_to_char(ch, "Nem pensar! Você está meditando profundamente.\r\n");
                break;
            case POS_SLEEPING:
                send_to_char(ch, "Em seus sonhos ou o que?\r\n");
                break;
            case POS_RESTING:
                send_to_char(ch, "Não... Você está em repouso agora...\r\n");
                break;
            case POS_SITTING:
                send_to_char(ch, "Você pode ficar em pé antes?\r\n");
                break;
            case POS_FIGHTING:
                send_to_char(ch, "Sem chances!  Você está lutando pela sua vida!\r\n");
                break;
        }
    else if (no_specials || !special(ch, cmd, line)) {
        ((*complete_cmd_info[cmd].command_pointer)(ch, line, cmd, complete_cmd_info[cmd].subcmd));
    }
}

/* Routines to handle aliasing. */
static struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
    while (alias_list != NULL) {
        if (*str == *alias_list->alias) /* hey, every little bit counts :-) */
            if (!strcmp(str, alias_list->alias))
                return (alias_list);
        alias_list = alias_list->next;
    }

    return (NULL);
}

void free_alias(struct alias_data *a)
{
    if (a->alias)
        free(a->alias);
    if (a->replacement)
        free(a->replacement);
    free(a);
}

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
    char arg[MAX_INPUT_LENGTH];
    char *repl;
    struct alias_data *a, *temp;
    if (IS_NPC(ch))
        return;
    repl = any_one_arg(argument, arg);
    if (!*arg) { /* no argument specified -- list currently
                    defined aliases */
        if ((a = GET_ALIASES(ch)) == NULL)
            send_to_char(ch, "Não há atalhos definidos.\r\n");
        else {
            send_to_char(ch, "\tWAtalhos definidos:\r\n");
            while (a != NULL) {
                send_to_char(ch, "\tc%-15s\tn %s\tn\r\n", a->alias, a->replacement);
                a = a->next;
            }
        }
    } else { /* otherwise, add or remove aliases */
        /* is this an alias we've already defined? */
        if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
            REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
            free_alias(a);
        }
        /* if no replacement string is specified, assume we want to delete */
        if (!*repl) {
            if (a == NULL)
                send_to_char(ch, "Não existe atalho com esse nome.\r\n");
            else
                send_to_char(ch, "Atalho apagado.\r\n");
        } else { /* otherwise, either add or redefine an alias */
            if (!str_cmp(arg, "alias")) {
                send_to_char(ch, "Você não pode criar atalho com o nome 'alias'.\r\n");
                return;
            }
            CREATE(a, struct alias_data, 1);
            a->alias = strdup(arg);
            delete_doubledollar(repl);
            a->replacement = strdup(repl);
            if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
                a->type = ALIAS_COMPLEX;
            else
                a->type = ALIAS_SIMPLE;
            a->next = GET_ALIASES(ch);
            GET_ALIASES(ch) = a;
            save_char(ch);
            send_to_char(ch, "Alias definido.\r\n");
        }
    }
}

/* Valid numeric replacements are only $1 .. $9 (makes parsing a little
   easier, and it's not that much of a limitation anyway.) Also valid is
   "$*", which stands for the entire original line after the alias. ";" is
   used to delimit commands. */
#define NUM_TOKENS 9

static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
    struct txt_q temp_queue;
    char *tokens[NUM_TOKENS], *temp, *write_point;
    char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH]; /* raw? */
    int num_of_tokens = 0, num;
    /* First, parse the original string */
    strcpy(buf2, orig); /* strcpy: OK (orig:MAX_INPUT_LENGTH <
                                           buf2:MAX_RAW_INPUT_LENGTH) */
    temp = strtok(buf2, " ");
    while (temp != NULL && num_of_tokens < NUM_TOKENS) {
        tokens[num_of_tokens++] = temp;
        temp = strtok(NULL, " ");
    }

    /* initialize */
    write_point = buf;
    temp_queue.head = temp_queue.tail = NULL;
    /* now parse the alias */
    for (temp = a->replacement; *temp; temp++) {
        if (*temp == ALIAS_SEP_CHAR) {
            *write_point = '\0';
            buf[MAX_INPUT_LENGTH - 1] = '\0';
            write_to_q(buf, &temp_queue, 1);
            write_point = buf;
        } else if (*temp == ALIAS_VAR_CHAR) {
            temp++;
            if ((num = *temp - '1') < num_of_tokens && num >= 0) {
                strcpy(write_point, tokens[num]); /* strcpy: OK */
                write_point += strlen(tokens[num]);
            } else if (*temp == ALIAS_GLOB_CHAR) {
                skip_spaces(&orig);
                strcpy(write_point, orig); /* strcpy: OK */
                write_point += strlen(orig);
            } else if ((*(write_point++) = *temp) == '$') /* redouble $ for act
                                                                                                     safety */
                *(write_point++) = '$';
        } else
            *(write_point++) = *temp;
    }

    *write_point = '\0';
    buf[MAX_INPUT_LENGTH - 1] = '\0';
    write_to_q(buf, &temp_queue, 1);
    /* push our temp_queue on to the _front_ of the input queue */
    if (input_q->head == NULL)
        *input_q = temp_queue;
    else {
        temp_queue.tail->next = input_q->head;
        input_q->head = temp_queue.head;
    }
}

/* Given a character and a string, perform alias replacement on it. Return
   values: 0: String was modified in place; call command_interpreter
   immediately.  1: String was _not_ modified in place; rather, the
   expanded aliases have been placed at the front of the character's input
   queue. */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
    char first_arg[MAX_INPUT_LENGTH], *ptr;
    struct alias_data *a, *tmp;
    /* Mobs don't have alaises. */
    if (IS_NPC(d->character))
        return (0);
    /* bail out immediately if the guy doesn't have any aliases */
    if ((tmp = GET_ALIASES(d->character)) == NULL)
        return (0);
    /* find the alias we're supposed to match */
    ptr = any_one_arg(orig, first_arg);
    /* bail out if it's null */
    if (!*first_arg)
        return (0);
    /* if the first arg is not an alias, return without doing anything */
    if ((a = find_alias(tmp, first_arg)) == NULL)
        return (0);
    if (a->type == ALIAS_SIMPLE) {
        strlcpy(orig, a->replacement, maxlen);
        return (0);
    } else {
        perform_complex_alias(&d->input, ptr, a);
        return (1);
    }
}

/* Various other parsing utilities. */

/* Searches an array of strings for a target string.  "exact" can be 0 or
   non-0, depending on whether or not the match must be exact for it to be
   returned. Returns -1 if not found; 0..n otherwise.  Array must be
   terminated with a '\n' so it knows to stop searching. */
int search_block(char *arg, const char **list, int exact)
{
    int i, l;
    /* We used to have \r as the first character on certain array items to
       prevent the explicit choice of that point.  It seems a bit silly to
       dump control characters into arrays to prevent that, so we'll just
       check in here to see if the first character of the argument is '!', and
       if so, just blindly return a '-1' for not found. - ae. */
    if (*arg == '!')
        return (-1);
    /* Make into lower case, and get length of string */
    for (l = 0; *(arg + l); l++)
        *(arg + l) = LOWER(*(arg + l));
    if (exact) {
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strcmp(arg, *(list + i)))
                return (i);
    } else {
        if (!l)
            l = 1; /* Avoid "" to match the first available *
                              string */
        for (i = 0; **(list + i) != '\n'; i++)
            if (!strncmp(arg, *(list + i), l))
                return (i);
    }

    return (-1);
}

int is_number(const char *str)
{
    if (*str == '-')
        str++;
    if (!*str)
        return (0);
    while (*str)
        if (!isdigit(*(str++)))
            return (0);
    return (1);
}

/* Function to skip over the leading spaces of a string. */
void skip_spaces(char **string)
{
    for (; **string && **string != '\t' && isspace(**string); (*string)++)
        ;
}

/* Given a string, change all instances of double dollar signs ($$) to
   single dollar signs ($).  When strings come in, all $'s are changed to
   $$'s to avoid having users be able to crash the system if the inputted
   string is eventually sent to act().  If you are using user input to
   produce screen output AND YOU ARE SURE IT WILL NOT BE SENT THROUGH THE
   act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say), you can call
   delete_doubledollar() to make the output look correct. Modifies the
   string in-place. */
char *delete_doubledollar(char *string)
{
    char *ddread, *ddwrite;
    /* If the string has no dollar signs, return immediately */
    if ((ddwrite = strchr(string, '$')) == NULL)
        return (string);
    /* Start from the location of the first dollar sign */
    ddread = ddwrite;
    while (*ddread)                              /* Until we reach the end of the string... */
        if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
            if (*ddread == '$')
                ddread++; /* skip if we saw 2 $'s in a row */
    *ddwrite = '\0';
    return (string);
}

int fill_word(char *argument) { return (search_block(argument, fill, TRUE) >= 0); }

int reserved_word(char *argument) { return (search_block(argument, reserved, TRUE) >= 0); }

/* Copy the first non-fill-word, space-delimited argument of 'argument' to
   'first_arg'; return a pointer to the remainder of the string. */
char *one_argument(char *argument, char *first_arg)
{
    char *begin = first_arg;
    if (!argument) {
        log1("SYSERR: one_argument received a NULL pointer!");
        *first_arg = '\0';
        return (NULL);
    }

    do {
        skip_spaces(&argument);
        first_arg = begin;
        while (*argument && !isspace(*argument)) {
            *(first_arg++) = LOWER(*argument);
            argument++;
        }

        *first_arg = '\0';
    } while (fill_word(begin));
    return (argument);
}

/* one_word is like any_one_arg, except that words in quotes ("") are
   considered one word. No longer ignores fill words.  -dak */
char *one_word(char *argument, char *first_arg)
{
    skip_spaces(&argument);
    if (*argument == '\"') {
        argument++;
        while (*argument && *argument != '\"') {
            *(first_arg++) = LOWER(*argument);
            argument++;
        }
        argument++;
    } else {
        while (*argument && !isspace(*argument)) {
            *(first_arg++) = LOWER(*argument);
            argument++;
        }
    }

    *first_arg = '\0';
    return (argument);
}

/* Same as one_argument except that it doesn't ignore fill words. */
char *any_one_arg(char *argument, char *first_arg)
{
    skip_spaces(&argument);
    while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
    }

    *first_arg = '\0';
    return (argument);
}

/* Same as one_argument except that it takes two args and returns the
   rest; ignores fill words */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
    return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-)
                                                                           */
}

/* Determine if a given string is an abbreviation of another. Returns 1 if
   arg1 is an abbreviation of arg2. */
int is_abbrev(const char *arg1, const char *arg2)
{
    if (!*arg1)
        return (0);
    for (; *arg1 && *arg2; arg1++, arg2++)
        if (LOWER(*arg1) != LOWER(*arg2))
            return (0);
    if (!*arg1)
        return (1);
    else
        return (0);
}

/* Return first space-delimited token in arg1; remainder of string in
   arg2. NOTE: Requires sizeof(arg2) >= sizeof(string) */
void half_chop(char *string, char *arg1, char *arg2)
{
    char *temp;
    temp = any_one_arg(string, arg1);
    skip_spaces(&temp);
    strcpy(arg2, temp); /* strcpy: OK (documentation) */
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
    int cmd;
    for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
        if (!strcmp(complete_cmd_info[cmd].command, command))
            return (cmd);
    return (-1);
}

int special(struct char_data *ch, int cmd, char *arg)
{
    struct obj_data *i;
    struct char_data *k;
    int j;
    /* special in room? */
    if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
        if (GET_ROOM_SPEC(IN_ROOM(ch))(ch, world + IN_ROOM(ch), cmd, arg))
            return (1);
    /* special in equipment list? */
    for (j = 0; j < NUM_WEARS; j++)
        if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
            if (GET_OBJ_SPEC(GET_EQ(ch, j))(ch, GET_EQ(ch, j), cmd, arg))
                return (1);
    /* special in inventory? */
    for (i = ch->carrying; i; i = i->next_content)
        if (GET_OBJ_SPEC(i) != NULL)
            if (GET_OBJ_SPEC(i)(ch, i, cmd, arg))
                return (1);
    /* special in mobile present? */
    for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
        if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
            if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k)(ch, k, cmd, arg))
                return (1);
    /* special in object present? */
    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
        if (GET_OBJ_SPEC(i) != NULL)
            if (GET_OBJ_SPEC(i)(ch, i, cmd, arg))
                return (1);
    return (0);
}

/* Stuff for controlling the non-playing sockets (get name, pwd etc). This
   function needs to die. */
static int _parse_name(char *arg, char *name)
{
    int i;
    skip_spaces(&arg);
    for (i = 0; (*name = *arg); arg++, i++, name++)
        if (!isalpha(*arg))
            return (1);
    if (!i)
        return (1);
    return (0);
}

#define RECON 1
#define USURP 2
#define UNSWITCH 3

/* This function seems a bit over-extended. */
static int perform_dupe_check(struct descriptor_data *d)
{
    struct descriptor_data *k, *next_k;
    struct char_data *target = NULL, *ch, *next_ch;
    int mode = 0;
    int pref_temp = 0; /* for "last" log */
    int id = GET_IDNUM(d->character);
    /* Now that this descriptor has successfully logged in, disconnect all
       other descriptors controlling a character with the same ID number. */
    for (k = descriptor_list; k; k = next_k) {
        next_k = k->next;
        if (k == d)
            continue;
        if (k->original && (GET_IDNUM(k->original) == id)) {
            /* Original descriptor was switched, booting it and restoring
               normal body control. */

            write_to_output(d, "\r\nMultiplo login detectado -- desconectando.\r\n");
            STATE(k) = CON_CLOSE;
            pref_temp = GET_PREF(k->character);
            if (!target) {
                target = k->original;
                mode = UNSWITCH;
            }
            if (k->character)
                k->character->desc = NULL;
            k->character = NULL;
            k->original = NULL;
        } else if (k->character && GET_IDNUM(k->character) == id && k->original) {
            /* Character taking over their own body, while an immortal was
               switched to it. */

            do_return(k->character, NULL, 0, 0);
        } else if (k->character && GET_IDNUM(k->character) == id) {
            /* Character taking over their own body. */
            pref_temp = GET_PREF(k->character);
            if (!target && STATE(k) == CON_PLAYING) {
                write_to_output(k, "\r\nEste corpo foi usurpado!\r\n");
                target = k->character;
                mode = USURP;
            }
            k->character->desc = NULL;
            k->character = NULL;
            if (k->original) {
                on_player_linkless(k->original);
                k->original = NULL;
            }

            write_to_output(k, "\r\nMultiplo login detectado -- desconectando.\r\n");
            STATE(k) = CON_CLOSE;
        }
    }

    /* Now, go through the character list, deleting all characters that are
       not already marked for deletion from the above step (i.e., in the
       CON_HANGUP state), and have not already been selected as a target for
       switching into. In addition, if we haven't already found a target,
       choose one if one is available (while still deleting the other
       duplicates, though theoretically none should be able to exist). */
    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;
        if (IS_NPC(ch))
            continue;
        if (GET_IDNUM(ch) != id)
            continue;
        /* ignore chars with descriptors (already handled by above step) */
        if (ch->desc)
            continue;
        /* don't extract the target char we've found one already */
        if (ch == target)
            continue;
        /* we don't already have a target and found a candidate for switching */
        if (!target) {
            target = ch;
            mode = RECON;
            pref_temp = GET_PREF(ch);
            continue;
        }

        /* we've found a duplicate - blow him away, dumping his eq in limbo. */
        if (IN_ROOM(ch) != NOWHERE)
            char_from_room(ch);
        char_to_room(ch, 1);
        extract_char(ch);
    }

    /* no target for switching into was found - allow login to continue */
    if (!target) {
        GET_PREF(d->character) = rand_number(1, 128000);
        if (GET_HOST(d->character))
            free(GET_HOST(d->character));
        GET_HOST(d->character) = strdup(d->host);
        return 0;
    }

    if (GET_HOST(target))
        free(GET_HOST(target));
    GET_HOST(target) = strdup(d->host);
    GET_PREF(target) = pref_temp;
    add_llog_entry(target, LAST_RECONNECT);
    /* Okay, we've found a target.  Connect d to target. */
    free_char(d->character); /* get rid of the old char */
    d->character = target;
    d->character->desc = d;
    d->original = NULL;
    d->character->char_specials.timer = 0;
    REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
    REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
    STATE(d) = CON_PLAYING;
    MXPSendTag(d, "<VERSION>");

    /* Auto-start MCCP compression if preference enabled */
    if (PRF_FLAGGED(d->character, PRF_MCCP)) {
        ProtocolMCCPStart(d);
    }

    switch (mode) {
        case RECON:
            write_to_output(d, "Reconectando.\r\n");
            act("$n se reconectou.", TRUE, d->character, 0, 0, TO_ROOM);
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.",
                   GET_NAME(d->character), d->host);
            if (has_mail(GET_IDNUM(d->character)))
                write_to_output(d, "Você tem uma nova carta!\r\n");
            break;
        case USURP:
            write_to_output(d, "Você toma o seu próprio corpo, que já estava em uso!\r\n");
            act("$n repentinamente se dobra de dor, contornad$r por uma aura branca...\r\n"
                "O corpo de $n é tomado por um novo espírito!",
                TRUE, d->character, 0, 0, TO_ROOM);
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
                   "%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
            break;
        case UNSWITCH:
            write_to_output(d, "Reconectando o personagem.\r\n");
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.",
                   GET_NAME(d->character), d->host);
            break;
    }

    return (1);
}

/* New Char dupe-check called at the start of character creation */
static bool perform_new_char_dupe_check(struct descriptor_data *d)
{
    struct descriptor_data *k, *next_k;
    bool found = FALSE;
    /* Now that this descriptor has successfully logged in, disconnect all
       other descriptors controlling a character with the same ID number. */
    for (k = descriptor_list; k; k = next_k) {
        next_k = k->next;
        if (k == d)
            continue;
        if (k->character == NULL)
            continue;
        /* Do the player names match? */
        if (!strcmp(GET_NAME(k->character), GET_NAME(d->character))) {
            /* Check the other character is still in creation? */
            if ((STATE(k) > CON_PLAYING) && (STATE(k) < CON_QCLASS)) {
                /* Boot the older one */
                k->character->desc = NULL;
                k->character = NULL;
                k->original = NULL;
                write_to_output(k, "\r\nMultiplo login detectado -- desconectando.\r\n");
                STATE(k) = CON_CLOSE;
                mudlog(NRM, LVL_GOD, TRUE, "Multiple logins detected in char creation for %s.", GET_NAME(d->character));
                found = TRUE;
            } else {
                /* Something went VERY wrong, boot both chars */
                k->character->desc = NULL;
                k->character = NULL;
                k->original = NULL;
                write_to_output(k, "\r\nMultiplo login detectado -- desconectando.\r\n");
                STATE(k) = CON_CLOSE;
                d->character->desc = NULL;
                d->character = NULL;
                d->original = NULL;
                write_to_output(
                    d, "\r\nDesculpe, devido a multiplas conexões, todas as suas conexões foram fechadas.\r\n");
                write_to_output(d, "\r\nPor favor, reconecte.\r\n");
                STATE(d) = CON_CLOSE;
                mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Multiple logins with 1st in-game and the 2nd in char creation.");
                found = TRUE;
            }
        }
    }
    return (found);
}

/* load the player, put them in the right room - used by copyover_recover
   too */
int enter_player_game(struct descriptor_data *d)
{
    int load_result;
    room_vnum load_room;
    reset_char(d->character);
    if (PLR_FLAGGED(d->character, PLR_INVSTART))
        GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);
    /* We have to place the character in a room before equipping them or
       equip_char() will gripe about the person in NOWHERE. */
    if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
        load_room = real_room(load_room);
    /* If char was saved with NOWHERE, or real_room above failed... */
    if (load_room == NOWHERE) {
        if (GET_LEVEL(d->character) >= LVL_IMMORT)
            load_room = r_immort_start_room;
        else if (GET_LEVEL(d->character) < 2)
            load_room = r_newbie_start_room;
        else if (GET_HOMETOWN(d->character) == 1)
            load_room = r_hometown_1;
        else if (GET_HOMETOWN(d->character) == 2)
            load_room = r_hometown_2;
        else if (GET_HOMETOWN(d->character) == 3)
            load_room = r_hometown_3;
        else
            load_room = r_hometown_1;
    }
    if (PLR_FLAGGED(d->character, PLR_FROZEN))
        load_room = r_frozen_start_room;
    if (PLR_FLAGGED(d->character, PLR_GHOST))
        load_room = r_dead_start_room;
    /* copyover */
    d->character->script_id = GET_IDNUM(d->character);
    /* find_char helper */
    add_to_lookup_table(d->character->script_id, (void *)d->character);
    /* After moving saving of variables to the player file, this should only
       be called in case nothing was found in the pfile. If something was
       found, SCRIPT(ch) will be set. */
    if (!SCRIPT(d->character))
        read_saved_vars(d->character);
    d->character->next = character_list;
    character_list = d->character;
    char_to_room(d->character, load_room);
    load_result = Crash_load(d->character);
    /* Save the character and their object file */
    save_char(d->character);
    Crash_crashsave(d->character);
    /* Check for a login trigger in the players' start room */
    login_wtrigger(&world[IN_ROOM(d->character)], d->character);
    return load_result;
}

EVENTFUNC(get_protocols)
{
    struct descriptor_data *d;
    struct mud_event_data *pMudEvent;
    char buf[MAX_STRING_LENGTH];
    size_t len;
    if (event_obj == NULL)
        return 0;
    pMudEvent = (struct mud_event_data *)event_obj;
    d = (struct descriptor_data *)pMudEvent->pStruct;
    /* Clear extra white space from the "protocol scroll" */
    write_to_output(d, "[H[J");
    len = snprintf(buf, MAX_STRING_LENGTH, "\tO[\toCliente\tO] \tw%s\tn | ",
                   d->pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
    if (d->pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt)
        len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toCores\tO] \tw256\tn | ");
    else if (d->pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt)
        len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toCores\tO] \twAnsi\tn | ");
    else
        len += snprintf(buf + len, MAX_STRING_LENGTH - len, "[Cores] Sem Cor | ");
    len +=
        snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toMXP\tO] \tw%s\tn | ", d->pProtocol->bMXP ? "Sim" : "Não");
    len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toMSDP\tO] \tw%s\tn | ",
                    d->pProtocol->bMSDP ? "Sim" : "Não");
    snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toATCP\tO] \tw%s\tn\r\n\r\n",
             d->pProtocol->bATCP ? "Sim" : "Não");
    write_to_output(d, buf, 0);
    write_to_output(d, GREETINGS, 0);
    STATE(d) = CON_GET_NAME;
    return 0;
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
    int load_result; /* Overloaded variable */
    int player_i;
    /* OasisOLC states */
    struct {
        int state;
        void (*func)(struct descriptor_data *, char *);
    } olc_functions[] = {{CON_OEDIT, oedit_parse},       {CON_ZEDIT, zedit_parse},
                         {CON_SEDIT, sedit_parse},       {CON_MEDIT, medit_parse},
                         {CON_REDIT, redit_parse},       {CON_CEDIT, cedit_parse},
                         {CON_TRIGEDIT, trigedit_parse}, {CON_AEDIT, aedit_parse},
                         {CON_HEDIT, hedit_parse},       {CON_QEDIT, qedit_parse},
                         {CON_PREFEDIT, prefedit_parse}, {CON_IBTEDIT, ibtedit_parse},
                         {CON_MSGEDIT, msgedit_parse},   {CON_SPEDIT, spedit_parse},
                         {CON_GEDIT, gedit_parse},       {-1, NULL}};
    skip_spaces(&arg);
    /* Quick check for the OLC states. */
    for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
        if (STATE(d) == olc_functions[player_i].state) {
            (*olc_functions[player_i].func)(d, arg);
            return;
        }

    /* Not in OLC. */
    switch (STATE(d)) {
        case CON_GET_PROTOCOL:
            write_to_output(d, "Identificando Protocolos... Por favor aguarde.\r\n");
            return;
        case CON_GET_NAME: /* wait for input of name */
            if (d->character == NULL) {
                CREATE(d->character, struct char_data, 1);
                clear_char(d->character);
                CREATE(d->character->player_specials, struct player_special_data, 1);
                new_mobile_data(d->character);
                GET_HOST(d->character) = strdup(d->host);
                d->character->desc = d;
            }
            if (!*arg)
                STATE(d) = CON_CLOSE;
            else {
                char buf[MAX_INPUT_LENGTH], tmp_name[MAX_INPUT_LENGTH];
                if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 || strlen(tmp_name) > MAX_NAME_LENGTH ||
                    !valid_name(tmp_name) || fill_word(strcpy(buf, tmp_name)) ||
                    reserved_word(buf)) { /* strcpy: OK (mutual MAX_INPUT_LENGTH) */
                    write_to_output(d, "Nome invalido.\r\nNome: ");
                    return;
                }
                if ((player_i = load_char(tmp_name, d->character)) > -1) {
                    GET_PFILEPOS(d->character) = player_i;
                    if (PLR_FLAGGED(d->character, PLR_DELETED)) {
                        /* Make sure old files are removed so the new player
                           doesn't get the deleted player's equipment. */
                        if ((player_i = get_ptable_by_name(tmp_name)) >= 0)
                            remove_player(player_i);
                        /* We get a false positive from the original deleted
                           character. */
                        free_char(d->character);
                        /* Check for multiple creations. */
                        if (!valid_name(tmp_name)) {
                            write_to_output(d, "Nome invalido.\r\nNome: ");
                            return;
                        }
                        CREATE(d->character, struct char_data, 1);
                        clear_char(d->character);
                        CREATE(d->character->player_specials, struct player_special_data, 1);
                        new_mobile_data(d->character);
                        if (GET_HOST(d->character))
                            free(GET_HOST(d->character));
                        GET_HOST(d->character) = strdup(d->host);
                        d->character->desc = d;
                        CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
                        strcpy(d->character->player.name,
                               CAP(tmp_name)); /* strcpy:
                                                                                                  OK
                                                                                                  (size
                                                                                                  checked
                                                                                                  above)
                                                                                                */
                        GET_PFILEPOS(d->character) = player_i;
                        write_to_output(d, "O nome está correto, %s (\t(S\t)/\t(N\t))? ", tmp_name);
                        STATE(d) = CON_NAME_CNFRM;
                    } else {
                        /* undo it just in case they are set */
                        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
                        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
                        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
                        d->character->player.time.logon = time(0);
                        write_to_output(d, "Senha: ");
                        echo_off(d);
                        d->idle_tics = 0;
                        STATE(d) = CON_PASSWORD;
                    }
                } else {
                    /* player unknown -- make new character */

                    /* Check for multiple creations of a character. */
                    if (!valid_name(tmp_name)) {
                        write_to_output(d, "Nome invalido.\r\nNome: ");
                        return;
                    }
                    CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
                    strcpy(d->character->player.name,
                           CAP(tmp_name)); /* strcpy:
                                                                                              OK
                                                                                              (size
                                                                                              checked
                                                                                              above) */
                    write_to_output(d, "O nome está correto, %s (\t(S\t)/\t(N\t))? ", tmp_name);
                    STATE(d) = CON_NAME_CNFRM;
                }
            }
            break;
        case CON_NAME_CNFRM: /* wait for conf. of new name */
            if (UPPER(*arg) == 'S') {
                if (isbanned(d->host) >= BAN_NEW) {
                    mudlog(NRM, LVL_GOD, TRUE, "Request for new char %s denied from [%s] (siteban)",
                           GET_PC_NAME(d->character), d->host);
                    write_to_output(d, "Desculpe-me, novos personagens nao sao permitidos de seu provedor!\r\n");
                    STATE(d) = CON_CLOSE;
                    return;
                }
                if (circle_restrict) {
                    write_to_output(
                        d, "Desculpe-me, o jogo foi temporariamente restrito.. tente novamente mais tarde.\r\n");
                    mudlog(NRM, LVL_GOD, TRUE, "Request for new char %s denied from [%s] (wizlock)",
                           GET_PC_NAME(d->character), d->host);
                    STATE(d) = CON_CLOSE;
                    return;
                }
                perform_new_char_dupe_check(d);
                write_to_output(d, "Novo personagem.\r\nDigite uma senha para %s: ", GET_PC_NAME(d->character));
                echo_off(d);
                STATE(d) = CON_NEWPASSWD;
            } else if (*arg == 'n' || *arg == 'N') {
                write_to_output(d, "Ok, qual E', entao? ");
                free(d->character->player.name);
                d->character->player.name = NULL;
                STATE(d) = CON_GET_NAME;
            } else
                write_to_output(d, "Por favor, digite Sim ou Nao.\r\n");
            break;
        case CON_PASSWORD: /* get pwd for known player */
            /* To really prevent duping correctly, the player's record should be
               reloaded from disk at this point (after the password has been
               typed). However I'm afraid that trying to load a character over an
               already loaded character is going to cause some problem down the
               road that I can't see at the moment. So to compensate, I'm going to
               (1) add a 15 or 20-second time limit for entering a password, and
               (2) re-add the code to cut off duplicates when a player quits. JE 6
               Feb 96 */

            echo_on(d); /* turn echo back on */
            /* New echo_on() eats the return on telnet. Extra space better than
               none. */
            write_to_output(d, "\r\n");
            if (!*arg)
                STATE(d) = CON_CLOSE;
            else {
                if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                    mudlog(BRF, LVL_GOD, TRUE, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
                    GET_BAD_PWS(d->character)++;
                    save_char(d->character);
                    if (++(d->bad_pws) >= CONFIG_MAX_BAD_PWS) { /* 3 strikes and you're out. */
                        write_to_output(d, "Senha incorreta... ate' mais.\r\n");
                        STATE(d) = CON_CLOSE;
                    } else {
                        write_to_output(d, "Senha incorreta.\r\nSenha: ");
                        echo_off(d);
                    }
                    return;
                }

                /* Password was correct. */
                load_result = GET_BAD_PWS(d->character);
                GET_BAD_PWS(d->character) = 0;
                d->bad_pws = 0;
                if (isbanned(d->host) == BAN_SELECT && !PLR_FLAGGED(d->character, PLR_SITEOK)) {
                    write_to_output(
                        d, "Desculpe-me, este personagem nao tem permissao para se conectar de seu provedor!\r\n");
                    STATE(d) = CON_CLOSE;
                    mudlog(NRM, LVL_GOD, TRUE, "Connection attempt for %s denied from %s", GET_NAME(d->character),
                           d->host);
                    return;
                }
                if (GET_LEVEL(d->character) < circle_restrict) {
                    write_to_output(
                        d, "Desculpe-me, o jogo foi temporariamente restrito.. tente novamente mais tarde.\r\n");
                    STATE(d) = CON_CLOSE;
                    mudlog(NRM, LVL_GOD, TRUE, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character),
                           d->host);
                    return;
                }
                /* check and make sure no other copies of this player are logged
                   in */
                if (perform_dupe_check(d))
                    return;
                if (GET_LEVEL(d->character) >= LVL_IMMORT)
                    write_to_output(d, "%s", imotd);
                else
                    write_to_output(d, "%s", motd);
                if (GET_INVIS_LEV(d->character))
                    mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s has connected. (invis %d)",
                           GET_NAME(d->character), GET_INVIS_LEV(d->character));
                else
                    mudlog(BRF, LVL_IMMORT, TRUE, "%s has connected.", GET_NAME(d->character));
                /* Add to the list of 'recent' players (since last reboot) */
                if (AddRecentPlayer(GET_NAME(d->character), d->host, FALSE, FALSE) == FALSE) {
                    mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
                           "Failure to AddRecentPlayer (returned FALSE).");
                }

                if (load_result) {
                    write_to_output(d,
                                    "\r\n\r\n\007\007\007"
                                    "%s%d FALHA%s DE LOGIN DESDE O ULTIMO LOGIN COM SUCESSO.%s\r\n",
                                    CCRED(d->character, C_SPR), load_result, (load_result > 1) ? "S" : "",
                                    CCNRM(d->character, C_SPR));
                    GET_BAD_PWS(d->character) = 0;
                }
                write_to_output(d, "\r\n*** APERTE ENTER: ");
                STATE(d) = CON_RMOTD;
            }
            break;
        case CON_NEWPASSWD:
        case CON_CHPWD_GETNEW:
            if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 || !str_cmp(arg, GET_PC_NAME(d->character))) {
                write_to_output(d, "\r\nSenha invalida.\r\nSenha: ");
                return;
            }
            strncpy(
                GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)),
                MAX_PWD_LENGTH); /* strncpy:
                                                                                                                            OK
                                                                                                                            (G_P:MAX_PWD_LENGTH+1)
                                                                                                                          */
            *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';
            write_to_output(d, "\r\nDigite novamente a senha: ");
            if (STATE(d) == CON_NEWPASSWD)
                STATE(d) = CON_CNFPASSWD;
            else
                STATE(d) = CON_CHPWD_VRFY;
            break;
        case CON_CNFPASSWD:
        case CON_CHPWD_VRFY:
            if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                write_to_output(d, "As senhas nao combinaram... vamos comecar denovo.\r\n");
                if (STATE(d) == CON_CNFPASSWD)
                    STATE(d) = CON_NEWPASSWD;
                else
                    STATE(d) = CON_CHPWD_GETNEW;
                return;
            }
            echo_on(d);
            if (STATE(d) == CON_CNFPASSWD) {
                write_to_output(d, "\r\n Qual o seu sexo (\t(M\t)/\t(F\t))? ");
                STATE(d) = CON_QSEX;
            } else {
                save_char(d->character);
                write_to_output(d, "\r\nSua senha foi alterada com sucesso.\r\n");
                show_menu_with_options(d);
                STATE(d) = CON_MENU;
            }
            break;
        case CON_QSEX: /* query sex of new user */
            switch (*arg) {
                case 'm':
                case 'M':
                    d->character->player.sex = SEX_MALE;
                    break;
                case 'f':
                case 'F':
                    d->character->player.sex = SEX_FEMALE;
                    break;
                default:
                    write_to_output(d, "Por favor, escolha um sexo valido...\r\n");
                    return;
            }

            write_to_output(d, "%s\r\nClasse: ", class_menu);
            STATE(d) = CON_QCLASS;
            break;
        case CON_QCLASS:
            load_result = parse_class(*arg);
            if (load_result == CLASS_UNDEFINED) {
                write_to_output(d, "Isso nao e' uma classe valida.\r\n Classe: ");
                return;
            } else
                GET_CLASS(d->character) = load_result;
            write_to_output(d, "\r\nCidade Natal: ");
            hometown_menu(d);
            STATE(d) = CON_QHOME;
            break;
        case CON_QHOME: /* query sex of new user */
            load_result = parse_hometown(*arg);
            if (load_result == 0) {
                write_to_output(d, "Por favor, escolha uma cidade válida...\r\n");
                return;
            } else
                GET_HOMETOWN(d->character) = (load_result);
            if (d->olc) {
                free(d->olc);
                d->olc = NULL;
            }
            if (GET_PFILEPOS(d->character) < 0)
                GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
            /* Now GET_NAME() will work properly. */
            init_char(d->character);
            save_char(d->character);
            save_player_index();
            write_to_output(d, "%s\r\n*** APERTE ENTER: ", motd);
            STATE(d) = CON_RMOTD;
            /* make sure the last log is updated correctly. */
            GET_PREF(d->character) = rand_number(1, 128000);
            GET_HOST(d->character) = strdup(d->host);
            mudlog(NRM, LVL_GOD, TRUE, "%s [%s] new player.", GET_NAME(d->character), d->host);
            /* Add to the list of 'recent' players (since last reboot) */
            if (AddRecentPlayer(GET_NAME(d->character), d->host, TRUE, FALSE) == FALSE) {
                mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
                       "Failure to AddRecentPlayer (returned FALSE).");
            }
            break;
        case CON_RMOTD: /* read CR after printing motd */
            show_menu_with_options(d);
            if (IS_HAPPYHOUR > 0) {
                write_to_output(d, "\r\n");
                write_to_output(d, "\tyEsta acontecendo um Happyhour!\tn\r\n");
                write_to_output(d, "\r\n");
            }
            add_llog_entry(d->character, LAST_CONNECT);
            STATE(d) = CON_MENU;
            break;
        case CON_MENU: { /* get selection from main menu */

            switch (*arg) {
                case '0':
                    write_to_output(d, "Adeus.\r\n");
                    // write_to_output(d, "Tenha um%s! :-)\r\n\r\n",);
                    add_llog_entry(d->character, LAST_QUIT);
                    STATE(d) = CON_CLOSE;
                    break;
                case '1':
                    load_result = enter_player_game(d);
                    send_to_char(d->character, "%s", CONFIG_WELC_MESSG);
                    /* Clear their load room if it's not persistant. */
                    if (!PLR_FLAGGED(d->character, PLR_LOADROOM))
                        GET_LOADROOM(d->character) = NOWHERE;
                    save_char(d->character);
                    greet_mtrigger(d->character, -1);
                    greet_memory_mtrigger(d->character);
                    act("$n entrou no jogo.", TRUE, d->character, 0, 0, TO_ROOM);
                    STATE(d) = CON_PLAYING;
                    MXPSendTag(d, "<VERSION>");

                    /* Auto-start MCCP compression if preference enabled */
                    if (PRF_FLAGGED(d->character, PRF_MCCP)) {
                        ProtocolMCCPStart(d);
                    }

                    if (GET_LEVEL(d->character) == 0) {
                        do_start(d->character);
                        send_to_char(d->character, "%s", CONFIG_START_MESSG);
                    }
                    look_at_room(d->character, 0);
                    if (has_mail(GET_IDNUM(d->character)))
                        send_to_char(d->character, "Você tem cartas esperando.\r\n");
                    if (load_result == 2) { /* rented items lost */
                        send_to_char(d->character,
                                     "\r\n\007Você não pode pagar seu aluguel!\r\n"
                                     "Suas posses foram doadas para o Exercito da Salvação!\r\n");
                    }
                    d->has_prompt = 0;
                    /* We've updated to 3.1 - some bits might be set wrongly: */
                    REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_BUILDWALK);
                    break;
                case '2':
                    page_string(d, background, 0);
                    STATE(d) = CON_RMOTD;
                    break;
                case '3':
                    if (d->character->player.description) {
                        write_to_output(d, "Descrição atual:\r\n%s", d->character->player.description);
                        /* Don't free this now... so that the old description gets
                           loaded as the current buffer in the editor. Do setup
                           the ABORT buffer here, however. */
                        d->backstr = strdup(d->character->player.description);
                    }
                    write_to_output(d, "Digite agora o texto que outros devem ler quando olharem para você.\r\n");
                    send_editor_help(d);
                    d->str = &d->character->player.description;
                    d->max_str = PLR_DESC_LENGTH;
                    STATE(d) = CON_PLR_DESC;
                    break;
                case '4':
                    write_to_output(d, "\r\nEntre com sua senha atual: ");
                    echo_off(d);
                    STATE(d) = CON_CHPWD_GETOLD;
                    break;
                case '5':
                    if (can_rebegin(d->character)) {
                        /* Show rebegin information */
                        send_to_char(d->character, "%s", rebegin);
                        show_class_skills(d->character, GET_CLASS(d->character));
                        STATE(d) = CON_RB_SKILL;
                    } else {
                        write_to_output(d, "\r\nVocê não pode renascer neste momento.\r\n");
                        show_menu_with_options(d);
                    }
                    break;
                case '6':
                    if (can_elevate(d->character)) {
                        write_to_output(
                            d, "Ao transcender, você se tornará um imortal e deixará para trás sua vida mortal.\r\n");
                        write_to_output(d, "Esta decisão é irreversível.\r\n\r\n");
                        write_to_output(d, "Deseja realmente transcender? (S/N): ");
                        STATE(d) = CON_ELEVATE_CONF;
                    } else {
                        write_to_output(d, "\r\nVocê não pode transcender neste momento.\r\n");
                        show_menu_with_options(d);
                    }
                    break;
                case '9':
                    write_to_output(d, "\r\nEntre com a sua senha para verificacao: ");
                    echo_off(d);
                    STATE(d) = CON_DELCNF1;
                    break;
                default:
                    write_to_output(d, "\r\nOpcao Invalida!\r\n");
                    show_menu_with_options(d);
                    break;
            }
            break;
        }

        case CON_CHPWD_GETOLD:
            if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                echo_on(d);
                write_to_output(d, "\r\nSenha incorreta.\r\n");
                show_menu_with_options(d);
                STATE(d) = CON_MENU;
            } else {
                write_to_output(d, "\r\nEntre com uma senha nova: ");
                STATE(d) = CON_CHPWD_GETNEW;
            }
            return;
        case CON_DELCNF1:
            echo_on(d);
            if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                write_to_output(d, "\r\n.Seu personagem NÃO foi apagado: senha incorreta.\r\n%s", CONFIG_MENU);
                STATE(d) = CON_MENU;
            } else {
                write_to_output(d,
                                "\r\nVOCÊ ESTÁ PRESTES A APAGAR PERMANENTEMENTE SEU PERSONAGEM.\r\n"
                                "VOCÊ TEM CERTEZA ABSOLUTA?\r\n\r\n"
                                "Por favor, digite SIM para confirmar.");
                STATE(d) = CON_DELCNF2;
            }
            break;
        case CON_DELCNF2:
            if (!strcmp(arg, "sim") || !strcmp(arg, "SIM")) {
                if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
                    write_to_output(d,
                                    "Você tenta se matar, mas o gelo te impede.\r\n"
                                    "Seu personagem não foi apagado.\r\n\r\n");
                    STATE(d) = CON_CLOSE;
                    return;
                }
                if (GET_LEVEL(d->character) < LVL_GRGOD)
                    SET_BIT_AR(PLR_FLAGS(d->character), PLR_DELETED);
                save_char(d->character);
                Crash_delete_file(GET_NAME(d->character));
                /* If the selfdelete_fastwipe flag is set (in config.c), remove
                   all the player's immediately. */
                if (selfdelete_fastwipe)
                    if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
                        SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
                        remove_player(player_i);
                    }

                delete_variables(GET_NAME(d->character));
                write_to_output(d, "Personagem '%s' apagado!\r\n Até mais.\r\n", GET_NAME(d->character));
                mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE, "%s (lev %d) has self-deleted.",
                       GET_NAME(d->character), GET_LEVEL(d->character));
                STATE(d) = CON_CLOSE;
                return;
            } else {
                write_to_output(d, "\r\nSeu personagem não foi apagado.\r\n");
                show_menu_with_options(d);
                STATE(d) = CON_MENU;
            }
            break;
            /* It is possible, if enough pulses are missed, to kick someone off
               while they are at the password prompt. We'll let the game_loop()axe
               them. */
        case CON_CLOSE:
            break;
        case CON_RB_SKILL: /* rebegin: choose skill to retain */
        {
            int skill_num = atoi(arg);
            if (skill_num < 0 || skill_num > MAX_SKILLS) {
                write_to_output(d, "Número inválido. Digite novamente: ");
                return;
            }
            if (skill_num != 0 && GET_SKILL(d->character, skill_num) <= 0) {
                write_to_output(d, "Você não tem essa habilidade. Digite novamente: ");
                return;
            }

            /* Save the selected skill to retained skills */
            if (skill_num > 0) {
                d->character->player_specials->saved.retained_skills[skill_num] = GET_SKILL(d->character, skill_num);
                write_to_output(d, "Habilidade %s será mantida.\r\n", skill_name(skill_num));
            } else {
                write_to_output(d, "Nenhuma habilidade será mantida.\r\n");
            }

            write_to_output(d, "\r\nEscolha sua nova classe:\r\n");
            write_to_output(d, "%s", class_menu);
            STATE(d) = CON_RB_NEW_CLASS;
        } break;
        case CON_RB_NEW_CLASS: /* rebegin: choose new class */
            load_result = parse_class(*arg);
            if (load_result == CLASS_UNDEFINED) {
                write_to_output(d, "Isso não é uma classe válida.\r\nClasse: ");
                return;
            }
            /* Check if class was already used */
            if (WAS_FLAGGED(d->character, load_result)) {
                write_to_output(d, "Você já viveu como essa classe. Escolha outra.\r\nClasse: ");
                return;
            }
            /* Mark current class as used and add to class history */
            SET_BIT_AR(d->character->player_specials->saved.was_class, GET_CLASS(d->character));

            /* Record in class history for statistics */
            int num_incarnations = d->character->player_specials->saved.num_incarnations;
            if (num_incarnations < 100) { /* Prevent overflow */
                d->character->player_specials->saved.class_history[num_incarnations] = GET_CLASS(d->character);
            }
            GET_CLASS(d->character) = load_result;
            write_to_output(d, "\r\nRolando novos atributos...\r\n");
            STATE(d) = CON_RB_REROLL;
            break;
        case CON_RB_REROLL: /* rebegin: reroll attributes */
            roll_real_abils(d->character);
            write_to_output(
                d, "Força: %d, Inteligência: %d, Sabedoria: %d, Destreza: %d, Constituição: %d, Carisma: %d\r\n",
                GET_STR(d->character), GET_INT(d->character), GET_WIS(d->character), GET_DEX(d->character),
                GET_CON(d->character), GET_CHA(d->character));
            write_to_output(d, "Aceitar estes atributos? (s/N): ");
            STATE(d) = CON_RB_QHOME;
            break;
        case CON_RB_QHOME: /* rebegin: reroll or accept attributes */
            if (!*arg || (*arg != 's' && *arg != 'S' && *arg != 'n' && *arg != 'N')) {
                write_to_output(d, "Por favor, responda s ou n: ");
                return;
            }
            if (*arg == 'n' || *arg == 'N') {
                roll_real_abils(d->character);
                write_to_output(
                    d, "Força: %d, Inteligência: %d, Sabedoria: %d, Destreza: %d, Constituição: %d, Carisma: %d\r\n",
                    GET_STR(d->character), GET_INT(d->character), GET_WIS(d->character), GET_DEX(d->character),
                    GET_CON(d->character), GET_CHA(d->character));
                write_to_output(d, "Aceitar estes atributos? (s/N): ");
                return;
            }

            /* Finalize rebegin process */
            GET_REMORT(d->character)++;
            GET_LEVEL(d->character) = 1;
            GET_EXP(d->character) = 1;
            GET_GOLD(d->character) = 0;
            GET_BANK_GOLD(d->character) = 0;
            GET_HIT(d->character) = GET_MAX_HIT(d->character);
            GET_MANA(d->character) = GET_MAX_MANA(d->character);
            GET_MOVE(d->character) = GET_MAX_MOVE(d->character);

            /* Initialize character for new class */
            do_start(d->character);

            /* Restore retained skills */
            {
                int i;
                for (i = 1; i <= MAX_SKILLS; i++) {
                    if (d->character->player_specials->saved.retained_skills[i] > 0) {
                        SET_SKILL(d->character, i, d->character->player_specials->saved.retained_skills[i]);
                    }
                }
            }

            save_char(d->character);

            write_to_output(d, "\r\nVocê renasceu com sucesso! Bem-vindo à sua nova vida.\r\n");
            write_to_output(d, "*** APERTE ENTER: ");
            STATE(d) = CON_RMOTD;
            break;

        case CON_ELEVATE_CONF: /* elevate: confirmation */
            if (!*arg || (*arg != 's' && *arg != 'S' && *arg != 'n' && *arg != 'N')) {
                write_to_output(d, "Por favor, responda s ou n: ");
                return;
            }
            if (*arg == 'n' || *arg == 'N') {
                write_to_output(d, "\r\nTranscendência cancelada.\r\n");
                write_to_output(d, "*** APERTE ENTER: ");
                STATE(d) = CON_RMOTD;
                return;
            }

            /* Finalize elevation process */
            GET_LEVEL(d->character) = LVL_IMMORT;
            GET_EXP(d->character) = level_exp(GET_CLASS(d->character), LVL_IMMORT);

            save_char(d->character);

            write_to_output(d, "\r\nParabéns! Você transcendeu e se tornou um imortal!\r\n");
            write_to_output(d, "Bem-vindo ao reino dos imortais.\r\n");
            write_to_output(d, "*** APERTE ENTER: ");
            STATE(d) = CON_RMOTD;
            break;

        default:
            log1("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.", STATE(d),
                 d->character ? GET_NAME(d->character) : "<unknown>");
            STATE(d) = CON_DISCONNECT; /* Safest to do. */
            break;
    }
}

void on_player_linkless(struct char_data *ch)
{
    act("$n perdeu a conexão.", TRUE, ch, 0, 0, TO_ROOM);
    save_char(ch);
}

void hometown_menu(struct descriptor_data *d)
{
    write_to_output(d, "\r\n1) Midgaard\r\n");
    write_to_output(d, "2) Nova Thalos\r\n");
    write_to_output(d, "3) Thewster\r\n");
}

int parse_hometown(char arg)
{
    arg = LOWER(arg);
    switch (arg) {
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        default:
            return 0;
    }
}
