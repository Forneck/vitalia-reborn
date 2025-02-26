/**
* @file constants.c
* Numeric and string contants used by the MUD.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Come up with a standard for descriptive arrays. Either all end with
* newlines or all of them don not.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"	/* alias_data */
#include "constants.h"

/** Current tbaMUD version.
 * @todo cpp_extern isn't needed here (or anywhere) as the extern reserved word
 * works correctly with C compilers (at least in my Experience)
 * Jeremy Osborne 1/28/2008 */
cpp_extern const char *tbamud_version = "Vitalia Reborn 2025 V0.3";

/* strings corresponding to ordinals/bitvectors in structs.h */
/* (Note: strings for class definitions in class.c instead of here) */

/** Description of cardinal directions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest", /* Diagonals only used if CONFIG_DIAGONAL_DIRS is set */
  "northeast",
  "southeast",
  "southwest",
  "\n"
};

const char *dirs_pt[] =
{
  "norte",
  "leste",
  "sul",
  "oeste",
  "cima",
  "baixo",
  "noroeste", /* Diagonals only used if CONFIG_DIAGONAL_DIRS is set */
  "nordeste",
  "sudeste",
  "sudoeste",
  "\n"
};

const char *autoexits[] =
{
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "nw",
  "ne",
  "se",
  "sw",
  "\n"
};
const char *autoexits_pt[] =
{
  "n",
  "l",
  "s",
  "o",
  "c",
  "b",
  "no",
  "ne",
  "se",
  "so",
  "\n"
};

/** Room flag descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "JAIL",
  "HOUSE",
  "HCRSH",
  "ARENA",
  "ATRIUM",
  "*",				/* The BFS Mark. */
   "HIGH",
  "NO_MOUNT",
  "NO_FLY",
  "HEAL",
  "FROZEN",
  "BROADCAST",
   "OLC",
  "WORLDMAP",
  "GODROOM",
  "\n"
};

/** Room flag descriptions. (ZONE_x)
 * @pre Must be in the same order as the defines in structs.h.
 * Must end array with a single newline. */
const char *zone_bits[] = {
  "CLOSED",
  "NO_ASTRAL",
  "NO_RECALL",
  "NO_SUMMON",
  "NO_LOAD",
  "SEPARATE",
  "CLIMATE",
  "NO_IMMORT",
  "QUEST",
  "GRID",
  "NOBUILD",
  "WORLDMAP",
  "\n"
};

/** Exit bits for doors.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *exit_bits[] = {
  "DOOR",
  "PICKPROOF",
  "GHOSTPROOF",
  "HIDDEN",
  "CLIMBUP",
  "CLIMBDOWN",
  "KNOCK",
  "DN_OPEN",
  "DN_CLOSE",
  "(unused)",	/* bit '9' */
  "(unused)",	/* bit '10' */
  "(unused)",	/* bit '11' */
  "(unused)",	/* bit '12' */
  "(unused)",   /* bit '13' */
  "CLOSED",
  "LOCKED",
  "\n"
};

/** Description of the room sector type.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "Underwater",
  "In Flight",
  "Climbing",
  "Air Flow",
  "Quicksand",
  "Lava",
  "Ice",
  "\n"
};

/** PC and NPC sex.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *genders[] =
{
  "neutro",
  "masculino",
  "feminino",
  "\n"
};

/** Character positions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *position_types[] = {
 "Morto",
  "Mortalmente ferido",
  "Incapacitado",
  "Atordoado",
  "Meditando",
  "Dormindo",
  "Repousando",
  "Sentado",
  "Lutando",
  "Em pé",
  "\n"
};

/** Player flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "MUTE",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this flag on a character in game. */
  "IBT_BUG",
  "IBT_IDEA",
  "IBT_TYPO",
  "HTHIEF",
  "JAIL",
  "TRNS",
  "HERETIC",
  "GHOST",
  "\n"
};

/** Mob action flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "NO_POISON",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "NO_CHARM",
  "NO_SUMMN",
  "NO_SLEEP",
  "NO_BASH",
  "NO_BLIND",
  "MOUNTABLE",
  "CAN_FLY",
  "NO_KILL",
  "ISNPC",
  "DEAD",    /* You should never see this. */
  "\n"
};

/** PC Preference flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "NO_SHOUT",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "C1",
  "C2",
  "NO_WIZ",
  "L1",
  "L2",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "RMFLG",
  "D_AUTO",
  "CLS",
  "BLDWLK",
  "AFK",
  "AUTOLOOT",
  "AUTOGOLD",
  "AUTOSPLIT",
  "AUTOSAC",
  "AUTOASSIST",
  "AUTOMAP",
  "AUTOKEY",
  "AUTODOOR",
  "ZONERESETS",
  "HITBAR",
  "AUTOTITLE",
  "NO_CLAN",
  "\n"
};

/** Affected bits.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *affected_bits[] =
{
  "\0", /* DO NOT REMOVE!! */
  "BLIND",
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "NO_TRACK",
   "STONESKIN",
   "FIRESHIELD",
  "SNEAK",
  "HIDE",
   "TALKDEAD",
  "CHARM",
  "FLYING",
  "BREATH",
  "PARALYZE",
   "LIGHT",
   "FIREFLIES",
   "STINGING",
   "THISTLECOAT",
   "SOUNDBARRIER",
   "ADAGIO",
   "ALLEGRO",
   "GLOOMSHIELD",
   "PROT-SPELL",
   "WINDWALL",
  "\n",
};

/** Connection type descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Config edit",
  "Social edit",
  "Trigger edit",
  "Help edit",
  "Quest edit",
  "Preference edit",
  "IBT edit",
  "Message edit",
  "Protocol Detection",
   "Rerolling",		// CON_REROLL
  "Select hometn",	// CON_QHOME
   "Rebegin: skill",	// CON_RB_SKILL
  "Rebegin: class",	// CON_RB_NEW_CLASS
  "Rebegin: roll",	// CON_RB_REROLL
  "Rebegin: home",	// CON_RB_QHOME
  "Immort: conf",	// CON_IMM_CONF
  "Remote control",	// CON_REMOTE
  "\n"
};

/** Describes the position in the equipment listing.
 * @pre Must be in the same order as the defines.
 * Not used in sprinttype() so no \\n. */
const char *wear_where[] = {
 "Como luz:          ",
  "No dedo:           ",
  "No dedo:           ",
  "No pescoço:        ",
  "No pescoço:        ",
  "No corpo:          ",
  "Na cabeça:         ",
  "Nas pernas:        ",
  "Nos pés:           ",
  "Nas mãos:          ",
  "Nos braços:        ",
  "Como escudo:       ",
  "Sobre o corpo:     ",
  "Sobre a cintura:   ",
  "No pulso direito:  ",
  "No pulso esquerdo: ",
  "Empunhando:        ",
  "Segurando:         ",
  "Como asas:         ",
  "Na orelha:         ",
  "Na orelha:         ",
  "No rosto:          ",
  "No nariz:          ",
  "Como insígnia:     ",
  "Na bolsa de munições: "
};

/* Describes where an item can be worn.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *equipment_types[] = {
  "Usado como luz",
  "No dedo direito",
  "No dedo esquerdo",
  "Usado no pescoço",
  "Usado no pescoço",
  "Usado no corpo",
  "Usado na cabeça",
  "Usado nas pernas",
  "Usado nos pés",
  "Usado nas mãos",
  "Usado nos braços",
  "Usado como escudo",
  "Usado sobre o corpo",
  "Usado sobre a cintura" ,
  "Usado no pulso direito",
  "Usado pulso esquerdo",
  "Usado empunhando",
  "Usado segurando",
  "Usado como asas",
  "Usado na orelha direita",
  "Usado na orelha esquerda",
  "Usado no rosto",
  "Usado no nariz",
  "Usado como insígnia",
  "Usado como munição",
  "\n"
};

/** Describes the type of object.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FURNITURE",
  "AMMO",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "WINGS",
  "CORPSE",
  "PORTAL",
  "BOOK",
  "PLANT",
  "FIRE WEAPON",
  "\n"
};

/** Describes the wear flags set on an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "WINGS",
  "EAR",
  "FACE",
  "NOSE",
  "INSIGNE",
  "QUIVER",
  "\n"
};

/** Describes the extra flags applied to an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_MAGE",
  "ANTI_CLERIC",
  "ANTI_THIEF",
  "ANTI_WARRIOR",
  "NO_SELL",
  "TWO_HANDS",
  "POISONED",
  "ANTI_DRUID",
  "ANTI_BARD",
  "QUEST_ITEM",
  "ANTI_RANGER",
  "\n"
};

/** Describes the apply types.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "\n"
};

/** Describes the closure mechanism for a container.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};

/** Describes the liquid description.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinks[] =
{
  "água",
  "cerveja",
  "vinho",
  "chopp",
  "chopp escuro",
  "uísque",
  "limonada",
  "rabo-de-galo",
  "especialidade local",
  "saliva",
  "leite",
  "chá",
  "café",
  "sangue",
  "água salgada",
  "água cristalina",
  "néctar",
  "champagne",
  "hidromel",
  "sidra",
  "rum",
  "\n"
};

/** Describes a one word alias for each type of liquid.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinknames[] =
{
  "agua",
  "cerveja",
  "vinho",
  "chopp",
  "chopp",
  "uisque",
  "limonada",
  "rabo-de-galo",
  "especialidade",
  "saliva",
  "leite",
  "cha",
  "cafe",
  "sangue",
  "agua",
  "agua",
  "nectar",
  "champagne",
  "hidromel",
  "sidra",
  "rum",
  "\n"
};

/** Define the effect of liquids on hunger, thirst, and drunkenness, in that
 * order. See values.doc for more information.
 * @pre Must be in the same order as the defines. */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13},
  {0, 2, 9},
  {5, 2, 5},
  {8, 3, 8},
  {6, 5, 3},
  {10, 1, 0}
};

/** Describes the color of the various drinks.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *color_liquid[] =
{
  "claro",
  "marrom",
  "claro",
  "marrom",
  "escuro",
  "amarelo",
  "vermelho",
  "verde",
  "claro",
  "verde claro",
  "branco",
  "marrom",
  "preto",
  "vermelho",
  "claro",
  "claro e cristalino",
  "dourado e cristalino",
  "claro",
  "castanho e cristalino",
  "rosáceo e cristalino",
  "amarelo claro",
  "\n"
};

/** Used to describe the level of fullness of a drink container. Not used in
 * sprinttype() so no \\n. */
const char *fullness[] =
{
  "menos da metade cheio",
  "cheio até a metade",
  "mais da metade cheio",
  "cheio"
};

/** Strength attribute affects.
 * The fields are hit mod, damage mod, weight carried mod, and weight wielded
 * mod. */
cpp_extern const struct str_app_type str_app[] = {
  {-5, -4, 0, 0},	/* str = 0 */
  {-5, -4, 3, 1},	/* str = 1 */
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},
  {-2, -1, 55, 5},	/* str = 5 */
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 100, 9},
  {0, 0, 115, 10},	/* str = 10 */
  {0, 0, 115, 11},
  {0, 0, 140, 12},
  {0, 0, 140, 13},
  {0, 0, 170, 14},
  {0, 0, 170, 15},	/* str = 15 */
  {0, 1, 195, 16},
  {1, 1, 220, 18},
  {1, 2, 255, 20},	/* str = 18 */
  {3, 7, 640, 40},
  {3, 8, 700, 40},	/* str = 20 */
  {4, 9, 810, 40},
  {4, 10, 970, 40},
  {5, 11, 1130, 40},
  {6, 12, 1440, 40},
  {7, 14, 1750, 40},	/* str = 25 */
  {1, 3, 280, 22},	/* str = 18/0 - 18-50 */
  {2, 3, 305, 24},	/* str = 18/51 - 18-75 */
  {2, 4, 330, 26},	/* str = 18/76 - 18-90 */
  {2, 5, 380, 28},	/* str = 18/91 - 18-99 */
  {3, 6, 480, 30}	/* str = 18/100 */
};

/** Dexterity skill modifiers for thieves.
 * The fields are for pick pockets, pick locks, find traps, sneak and hide. */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
// pkt, lck, trp, snk, hid, sze
  {-99, -99, -90, -99, -60, -99},	/* dex = 0 */
  {-90, -90, -60, -90, -50, -90},	/* dex = 1 */
  {-80, -80, -40, -80, -45, -90},
  {-70, -70, -30, -70, -40, -90},
  {-60, -60, -30, -60, -35, -90},
  {-50, -50, -20, -50, -30, -80},	/* dex = 5 */
  {-40, -40, -20, -40, -25, -80},
  {-30, -30, -15, -30, -20, -70},
  {-20, -20, -15, -20, -15, -60},
  {-15, -10, -10, -20, -10, -50},
  {-10,  -5, -10, -15,  -5, -40},	/* dex = 10 */
  { -5,   0,  -5, -10,   0, -30},
  {  0,   0,   0,  -5,   0, -20},
  {  0,   0,   0,   0,   0, -15},
  {  0,   0,   0,   0,   0, -10},
  {  0,   0,   0,   0,   0,  -5},	/* dex = 15 */
  {  0,   5,   0,   0,   0,   0},
  {  5,  10,   0,   5,   5,   0},
  { 10,  15,   5,  10,  10,   5},	/* dex = 18 */
  { 15,  20,  10,  15,  15,  10},
  { 15,  20,  10,  15,  15,  10},	/* dex = 20 */
  { 20,  25,  10,  15,  20,  15},
  { 20,  25,  15,  20,  20,  15},
  { 25,  25,  15,  20,  20,  20},
  { 25,  30,  15,  25,  25,  20},
  { 25,  30,  15,  25,  25,  25}	/* dex = 25 */
};

/** Dexterity attribute affects.
 * The fields are reaction, missile attacks, and defensive (armor class). */
cpp_extern const struct dex_app_type dex_app[] = {
  {-7, -7, 6},		/* dex = 0 */
  {-6, -6, 5},		/* dex = 1 */
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},		/* dex = 5 */
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},		/* dex = 10 */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, -1},		/* dex = 15 */
  {1, 1, -2},
  {2, 2, -3},
  {2, 2, -4},		/* dex = 18 */
  {3, 3, -4},
  {3, 3, -5},		/* dex = 20 */
  {4, 4, -5},
  {4, 4, -6},
  {4, 4, -6},
  {5, 5, -7},
  {5, 5, -7}		/* dex = 25 */
};

/** Constitution attribute affects.
 * The field referenced is for hitpoint bonus. */
cpp_extern const struct con_app_type con_app[] = {
  {-4},		/* con = 0 */
  {-3},		/* con = 1 */
  {-2},
  {-2},
  {-1},
  {-1},		/* con = 5 */
  {-1},
  {0},
  {0},
  {0},
  {0},		/* con = 10 */
  {0},
  {0},
  {0},
  {0},
  {1},		/* con = 15 */
  {2},
  {2},
  {3},		/* con = 18 */
  {3},
  {4},		/* con = 20 */
  {5},
  {5},
  {5},
  {6},
  {6}		/* con = 25 */
};

/** Intelligence attribute affects.
 * The field shows how much practicing affects a skill/spell. */
cpp_extern const struct int_app_type int_app[] = {
  {3},		/* int = 0 */
  {5},		/* int = 1 */
  {7},
  {8},
  {9},
  {10},		/* int = 5 */
  {11},
  {12},
  {13},
  {15},
  {17},		/* int = 10 */
  {19},
  {22},
  {25},
  {30},
  {35},		/* int = 15 */
  {40},
  {45},
  {50},		/* int = 18 */
  {53},
  {55},		/* int = 20 */
  {56},
  {57},
  {58},
  {59},
  {60}		/* int = 25 */
};

/** Wisdom attribute affects.
 * The field represents how many extra practice points are gained per level. */
cpp_extern const struct wis_app_type wis_app[] = {
  {0},	/* wis = 0 */
  {0},  /* wis = 1 */
  {0},
  {0},
  {0},
  {0},  /* wis = 5 */
  {0},
  {0},
  {0},
  {0},
  {0},  /* wis = 10 */
  {0},
  {2},
  {2},
  {3},
  {3},  /* wis = 15 */
  {3},
  {4},
  {5},	/* wis = 18 */
  {6},
  {6},  /* wis = 20 */
  {6},
  {6},
  {7},
  {7},
  {7}  /* wis = 25 */
};

const char *npc_class_types[] = {
  "Outro",
  "Undead",
  "Humanoide",
  "Animal",
  "Dragão",
  "Gigante",
  "\n"
};

/** Define a set of opposite directions from the cardinal directions. */
int rev_dir[] =
{
  SOUTH,
  WEST,
  NORTH,
  EAST,
  DOWN,
  UP,
  SOUTHEAST,
  SOUTHWEST,
  NORTHWEST,
  NORTHEAST
};

/** How much movement is lost moving through a particular sector type. */
int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  5,  /* Underwater */
    1,	/* Flying     */
    6,
    2,
    3,
    2,
    1
};

const char *climate_types[] = {
   "TEMPERADO",
  "CHUVOSO",
  "TROPICAL",
  "FRIO/SECO",
  "DESERTO"
};


const char *sun_states[] = {
  "DARK",
  "RISE",
  "LIGHT",
  "SET",
 "\n"
};


const char *sky_conditions[] = {
  "CLOUDLESS",
  "CLOUDY",
  "RAINING",
  "LIGHTNING",
  "SNOWING",
  "\n"
};

/** The names of the days of the mud week. Not used in sprinttype(). */
const char *weekdays[] = {
"Helis",
  "Sequni",
  "Afhis",
  "Kapiteo",
  "Perqon",
  "Libertas",
  "Veritas"
};

/** The names of the mud months. Not used in sprinttype(). */
const char *month_name[] = {
   "Brumis",		/* 0 */
  "Kames'Hi",
  "Teriany",
  "Hiro",
  "Prúdis",
  "Maqizie",
  "Kadrictes",
  "Mizu",
  "Mysoluh",
  "Karestis",
  "Neruno",
  "Latízie",
  "Aminen",
  "Autúmis",
  "V'tah",
  "Aqrien",
  "Tellus"
};

/** Names for mob trigger types. */
const char *trig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};

/** Names for object trigger types. */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED1",
  "UNUSED2",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED3",
  "Remove",
  "UNUSED4",
  "Load",
  "UNUSED5",
  "Cast",
  "Leave",
  "UNUSED6",
  "Consume",
  "Time",
  "\n"
};

/** Names for world (room) trigger types. */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED1",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "UNUSED6",
  "UNUSED7",
  "UNUSED8",
  "Cast",
  "Leave",
  "Door",
  "Login",
  "Time",
  "\n"
};

/** The names of the different channels that history is stored for.
 * @todo Only referenced by do_history at the moment. Should be moved local
 * to that function. */
const char *history_types[] = {
  "all",
  "say",
  "gossip",
  "wiznet",
  "tell",
  "shout",
  "grats",
  "pray",
  "auction",
  "holler",
  "group",
  "\n"
};

/** Flag names for Ideas, Bugs and Typos (defined in ibt.h) */
const char *ibt_bits[] = {
  "Resolved",
  "Important",
  "InProgress",
  "\n"
};

/* spells flags used by spedit OLC */
const char *targ_flags[] =
{
 "IGNORE",
 "CHAR_ROOM",
 "CHAR_WORLD",
 "FIGHT_SELF",
 "FIGHT_VICT",
"SELF_ONLY",
 "NOT_SELF",
 "OBJ_INV",
 "OBJ_ROOM",
 "OBJ_WORLD",
 "OBJ_EQUIP",
 "GROUP",
 "GROUP_VICT"
};


/* Magic aff flags used by spedit OLC system */
const char *mag_flags[] = 
{
"DAMAGE",
"AFFECTS",
"UNAFFECTS",
"POINTS",
"ALTER_OBJS",
"GROUPS",
"MASSES",
"AREAS",
"SUMMONS",
"CREATIONS",
"MANUAL",
"ROOMS",
"VIOLENT",
"ACCDUR",
"ACCMOD",
"PROTECTION"
};

cpp_extern const struct weapon_prof_data wpn_prof[] = {
  { 0,  0,  0,  0},  /* not warriors or non-learned weapons */
  { 1,  0,  0,  0},  /* prof <= 20   */
  { 2,  1,  0,  0},  /* prof <= 40   */
  { 3,  2,  0,  0},  /* prof <= 60   */
  { 4,  3,  0,  1},  /* prof <= 80   */
  { 5,  4, -1,  2},  /* prof <= 85   */
  { 6,  5, -1,  2},  /* prof <= 90   */
  { 7,  6, -2,  3},  /* prof <= 95   */
  { 8,  7, -2,  3},  /* prof <= 99   */
  { 9,  9, -3,  4},  /* prof == 100  */
  {-2, -2,  0,  0}   /* prof == 0    */
};

cpp_extern const struct nighthammer_data nighthammer_info[] = {
 /* HIT, DAM, AC  */
  {   0,   0,   0 },	/* mod = 0 */
  {   0,   0,  -1 },	/* mod = 1 */
  {   1,   0,  -2 },	/* mod = 2 */
  {   1,   1,  -3 },	/* mod = 3 */
  {   1,   1,  -4 },	/* mod = 4 */
  {   2,   1,  -6 },	/* mod = 5 */
  {   2,   2,  -8 },	/* mod = 6 */
  {   2,   2, -10 },	/* mod = 7 */
  {   2,   2, -12 }	/* mod = 8 */
};

/* --- End of constants arrays. --- */

/* Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared. */
  /** Number of defined room bit descriptions. */
  size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
  /** Number of defined action bit descriptions. */
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	/** Number of defined affected bit descriptions. */
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	/** Number of defined extra bit descriptions. */
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	/** Number of defined wear bit descriptions. */
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;
