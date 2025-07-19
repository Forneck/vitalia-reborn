/**************************************************************************
 *  File: class.c                                           Part of tbaMUD *
 *  Usage: Source file for class-specific code.                            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

/* This file attempts to concentrate most of the code which must be changed in
   order for new classes to be added.  If you're adding a new class, you
   should go through this entire file from beginning to end and add the
   appropriate new special cases for your new class. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "class.h"

/* Names first */
const char *class_abbrevs[] = {"Mag", "Cle", "Lad", "Gue", "Dru", "Brd", "Ra", "\n"};

const char *pc_class_types[] = {"Mago", "Clerigo", "Ladrao", "Guerreiro", "Druida", "Bardo", "Ranger", "\n"};

/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
    "\r\n"
    "Escolha uma classe:\r\n"
    "  [\t(C\t)]lerigo\r\n"
    "  [\t(L\t)]adrão\r\n"
    "  [\t(G\t)]uerreiro\r\n"
    "  [\t(M\t)]ago\r\n"
    "  [\t(D\t)]ruida\r\n"
    "  [\t(B\t)]ardo\r\n"
    "  [\t(R\t)]anger\r\n";
/*
 * EXPERIENCE
 * exp_table[level][class]
 */
const long exp_table[LVL_GRIMM + 1][NUM_CLASSES] = {
    /*{ MAGIC USER , CLERIC     , THIEF      , WARRIOR    , DRUID      , BARD , RANGER       }*/
    {0, 0, 0, 0, 0, 0, 0}, /* level   0 */
    {1, 1, 1, 1, 1, 1, 1},
    {2000, 1500, 1500, 2000, 1700, 1500, 1500},
    {4000, 3000, 3000, 4000, 3400, 3100, 3000},
    {8000, 6000, 6000, 8000, 7200, 6500, 6000},
    {16000, 12000, 12000, 16000, 14400, 14500, 13000},
    {
        32000,
        24000,
        24000,
        32000,
        28800,
        29000,
        27500,
    },
    {64000, 50000, 50000, 64000, 60000, 55000, 55000},
    {125000, 100000, 100000, 125000, 110000, 110000, 110000},
    {250000, 200000, 200000, 250000, 220000, 200000, 225000},
    {500000, 400000, 400000, 500000, 440000, 480000, 450000}, /* level  10 */
    {750000, 600000, 600000, 750000, 660000, 720000, 675000},
    {1000000, 800000, 800000, 1000000, 880000, 1000000, 900000},
    {1250000, 1000000, 1000000, 1250000, 1100000, 1110000, 1125000},
    {1500000, 1200000, 1200000, 1500000, 1300000, 1400000, 1350000},
    {1750000, 1400000, 1400000, 1750000, 1500000, 1600000, 1800000},
    {2000000, 1600000, 1600000, 2000000, 1700000, 1800000, 2100000},
    {2250000, 1800000, 1800000, 2250000, 1900000, 2100000, 2400000},
    {2500000, 2000000, 2000000, 2500000, 2100000, 2400000, 2700000},
    {2800000, 2200000, 2200000, 2800000, 2400000, 2700000, 3000000},
    {3100000, 2400000, 2400000, 3100000, 2700000, 3100000, 3250000}, /* level  20 */
    {3400000, 2700000, 2700000, 3400000, 3000000, 3400000, 3500000},
    {3700000, 3000000, 3000000, 3700000, 3300000, 3700000, 3800000},
    {4000000, 3300000, 3300000, 4000000, 3600000, 4000000, 4100000},
    {4400000, 3600000, 3600000, 4400000, 3900000, 4300000, 4400000},
    {4800000, 4000000, 4000000, 4800000, 4300000, 4700000, 4800000},
    {5200000, 4400000, 4400000, 5200000, 4700000, 5000000, 5200000},
    {5600000, 4800000, 4800000, 5600000, 5100000, 5300000, 5600000},
    {6000000, 5200000, 5200000, 6000000, 5500000, 5600000, 6000000},
    {6500000, 5600000, 5600000, 6500000, 5900000, 5900000, 6400000},
    {7000000, 6000000, 6000000, 7000000, 6500000, 6300000, 7000000}, /* level  30 */
    {7500000, 6500000, 6500000, 7500000, 7000000, 6700000, 7400000},
    {8000000, 7000000, 7000000, 8000000, 7500000, 7200000, 7900000},
    {8500000, 7500000, 7500000, 8500000, 8000000, 7900000, 8400000},
    {9000000, 8000000, 8000000, 9000000, 8500000, 8400000, 8900000},
    {9500000, 8600000, 8600000, 9500000, 9100000, 8900000, 9400000},
    {10000000, 9200000, 9200000, 10000000, 9700000, 9400000, 9900000},
    {10750000, 9800000, 9800000, 10750000, 10000000, 9900000, 10600000},
    {11500000, 10400000, 10400000, 11500000, 10600000, 10400000, 11000000},
    {12250000, 11000000, 11000000, 12250000, 11200000, 11000000, 11600000},
    {13000000, 11700000, 11700000, 13000000, 11800000, 11600000, 12400000}, /* level  40 */
    {13750000, 12400000, 12400000, 13750000, 12400000, 12200000, 12800000},
    {14500000, 13100000, 13100000, 14500000, 13100000, 12800000, 13500000},
    {15250000, 13800000, 13800000, 15250000, 13800000, 13500000, 14200000},
    {16000000, 14500000, 14500000, 16000000, 14600000, 14200000, 14900000},
    {16800000, 15200000, 15200000, 16800000, 15400000, 14900000, 16300000},
    {17600000, 16000000, 16000000, 17600000, 16300000, 15600000, 17100000},
    {18400000, 16800000, 16800000, 18400000, 17100000, 16300000, 17600000},
    {19200000, 17600000, 17600000, 19200000, 17800000, 17000000, 18000000},
    {20000000, 18400000, 18400000, 20000000, 18800000, 18000000, 19200000},
    {20900000, 19300000, 19300000, 20900000, 19500000, 19200000, 20400000}, /* level  50 */
    {21800000, 20200000, 20200000, 21800000, 20600000, 20400000, 21600000},
    {22700000, 21100000, 21100000, 22700000, 21500000, 21600000, 22800000},
    {23600000, 22000000, 22000000, 23600000, 23000000, 22800000, 23600000},
    {24500000, 23000000, 23000000, 24500000, 24000000, 24000000, 24300000},
    {25400000, 24000000, 24000000, 25400000, 25000000, 25200000, 25200000},
    {26300000, 25000000, 25000000, 26300000, 26000000, 26400000, 26000000},
    {27200000, 26000000, 26000000, 27200000, 27000000, 27200000, 27200000},
    {28100000, 27000000, 27000000, 28100000, 28000000, 28000000, 28100000},
    {29000000, 28000000, 28000000, 29000000, 29000000, 29000000, 28000000},
    {30000000, 29000000, 29000000, 30000000, 30000000, 30000000, 30000000}, /* level  60 */
    {31000000, 30000000, 30000000, 31000000, 31000000, 31000000, 30000000},
    {32000000, 31000000, 31000000, 32000000, 32000000, 32000000, 32000000},
    {33000000, 32000000, 32000000, 33000000, 33000000, 33000000, 33000000},
    {34000000, 34000000, 34000000, 34000000, 34000000, 34000000, 34000000},
    {36000000, 36000000, 36000000, 36000000, 36000000, 36000000, 36000000},
    {38000000, 38000000, 38000000, 38000000, 38000000, 38000000, 38000000},
    {40000000, 40000000, 40000000, 40000000, 40000000, 40000000, 40000000},
    {42000000, 42000000, 42000000, 42000000, 42000000, 42000000, 42000000},
    {44000000, 44000000, 44000000, 44000000, 44000000, 44000000, 44000000},
    {46000000, 46000000, 46000000, 46000000, 46000000, 46000000, 46000000}, /* level  70 */
    {49000000, 49000000, 49000000, 49000000, 49000000, 49000000, 49000000},
    {52000000, 52000000, 52000000, 52000000, 52000000, 52000000, 52000000},
    {55000000, 55000000, 55000000, 55000000, 55000000, 55000000, 55000000},
    {58000000, 58000000, 58000000, 58000000, 58000000, 58000000, 58000000},
    {61000000, 61000000, 61000000, 61000000, 61000000, 61000000, 61000000},
    {64000000, 64000000, 64000000, 64000000, 64000000, 64000000, 64000000},
    {67000000, 67000000, 67000000, 67000000, 67000000, 67000000, 67000000},
    {70000000, 70000000, 70000000, 70000000, 70000000, 70000000, 70000000},
    {74000000, 74000000, 74000000, 74000000, 74000000, 74000000, 74000000},
    {78000000, 78000000, 78000000, 78000000, 78000000, 78000000, 78000000}, /* level  80 */
    {82000000, 82000000, 82000000, 82000000, 82000000, 82000000, 82000000},
    {86000000, 86000000, 86000000, 86000000, 86000000, 86000000, 86000000},
    {90000000, 90000000, 90000000, 90000000, 90000000, 90000000, 90000000},
    {95000000, 95000000, 95000000, 95000000, 95000000, 95000000, 95000000},
    {100000000, 100000000, 100000000, 100000000, 100000000, 100000000, 100000000},
    {105000000, 105000000, 105000000, 105000000, 105000000, 105000000, 105000000},
    {110000000, 110000000, 110000000, 110000000, 110000000, 110000000, 110000000},
    {115000000, 115000000, 115000000, 115000000, 115000000, 115000000, 115000000},
    {120000000, 120000000, 120000000, 120000000, 120000000, 120000000, 120000000},
    {130000000, 130000000, 130000000, 130000000, 130000000, 130000000, 130000000}, /* level  90 */
    {140000000, 140000000, 140000000, 140000000, 140000000, 140000000, 140000000},
    {150000000, 150000000, 150000000, 150000000, 150000000, 150000000, 150000000},
    {160000000, 160000000, 160000000, 160000000, 160000000, 160000000, 160000000},
    {170000000, 170000000, 170000000, 170000000, 170000000, 170000000, 170000000},
    {180000000, 180000000, 180000000, 180000000, 180000000, 180000000, 180000000},
    {190000000, 190000000, 190000000, 190000000, 190000000, 190000000, 190000000},
    {200000000, 200000000, 200000000, 200000000, 200000000, 200000000, 200000000},
    {210000000, 210000000, 210000000, 210000000, 210000000, 210000000, 210000000},
    {220000000, 220000000, 220000000, 220000000, 220000000, 220000000, 220000000},
    {230000000, 230000000, 230000000, 230000000, 230000000, 230000000, 230000000}, /* level 100 */
    {250000000, 250000000, 250000000, 250000000, 250000000, 250000000, 250000000},
    {285000000, 285000000, 285000000, 285000000, 285000000, 285000000, 285000000},
    {335000000, 335000000, 335000000, 335000000, 335000000, 335000000, 335000000},
    {410000000, 410000000, 410000000, 410000000, 410000000, 410000000, 410000000},
    {500000000, 500000000, 500000000, 500000000, 500000000, 500000000, 500000000} /* level 105 */
};
/* The code to interpret a class letter -- used in interpreter.c when a new
   character is selecting a class and by 'set class' in act.wizard.c. */
int parse_class(char arg)
{
    arg = LOWER(arg);

    switch (arg) {
        case 'm':
            return CLASS_MAGIC_USER;
        case 'c':
            return CLASS_CLERIC;
        case 'g':
            return CLASS_WARRIOR;
        case 'l':
            return CLASS_THIEF;
        case 'd':
            return CLASS_DRUID;
        case 'b':
            return CLASS_BARD;
        case 'r':
            return CLASS_RANGER;
        default:
            return CLASS_UNDEFINED;
    }
}

/* bitvectors (i.e., powers of two) for each class, mainly for use in do_who
   and do_users.  Add new classes at the end so that all classes use
   sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5,
   etc.) up to the limit of your bitvector_t, typically 0-31. */
bitvector_t find_class_bitvector(const char *arg)
{
    size_t rpos, ret = 0;

    for (rpos = 0; rpos < strlen(arg); rpos++)
        ret |= (1 << parse_class(arg[rpos]));

    return (ret);
}

/* These are definitions which control the guildmasters for each class. The
   first field (top line) controls the highest percentage skill level a
   character of the class is allowed to attain in any skill.  (After this
   level, attempts to practice will say "You are already learned in this
   area." The second line controls the maximum percent gain in learnedness a
   character is allowed per practice -- in other words, if the random die
   throw comes out higher than this number, the gain will only be this number
   instead. The third line controls the minimu percent gain in learnedness a
   character is allowed per practice -- in other words, if the random die
   throw comes out below this number, the gain will be set up to this number.
   The fourth line simply sets whether the character knows 'spells' or
   'skills'. This does not affect anything except the message given to the
   character when trying to practice (i.e. "You know of the following spells"
   vs. "You know of the following skills" */

#define SPELL 0
#define SKILL 1
#define CHANSON 2

/* #define LEARNED_LEVEL 0 % known which is considered "learned" */
/* #define MAX_PER_PRAC 1 max percent gain in skill per practice */
/* #define MIN_PER_PRAC 2 min percent gain in skill per practice */
/* #define PRAC_TYPE 3 should it say 'spell' or 'skill'? */

int prac_params[4][NUM_CLASSES] = {
    /* MAG CLE THE WAR DRU BAR RAN */
    {95, 95, 85, 80, 95, 85, 90},                        /* learned level */
    {90, 90, 12, 12, 90, 28, 30},                        /* max per practice */
    {25, 25, 0, 0, 25, 14, 10},                          /* min per practice */
    {SPELL, SPELL, SKILL, SKILL, SPELL, CHANSON, SKILL}, /* prac name */
};

/* The appropriate rooms for each guildmaster/guildguard; controls which types
   of people the various guildguards let through.  i.e., the first line shows
   that from room 3017, only MAGIC_USERS are allowed to go south. Don't forget
   to visit spec_assign.c if you create any new mobiles that should be a guild
   master or guard so they can act appropriately. If you "recycle" the existing
   mobs that are used in other guilds for your new guild, then you don't have
   to change that file, only here. Guildguards are now implemented via
   triggers. This code remains as an example. */
struct guild_info_type guild_info[] = {

    /* Newbies */
    {-999, 18116, WEST},

    /* Midgaard */
    {CLASS_MAGIC_USER, 3017, SOUTH},
    {CLASS_CLERIC, 3004, NORTH},
    {CLASS_THIEF, 3027, EAST},
    {CLASS_WARRIOR, 3021, EAST},
    {CLASS_DRUID, 3071, WEST},
    {CLASS_BARD, 3086, EAST},

    /* New Thalos */
    {CLASS_MAGIC_USER, 5525, NORTH},
    {CLASS_CLERIC, 5512, SOUTH},
    {CLASS_THIEF, 5532, SOUTH},
    {CLASS_WARRIOR, 5526, SOUTH},
    {CLASS_DRUID, 5673, DOWN},
    {CLASS_BARD, 5499, SOUTH},

    /* Bhogavati */
    {CLASS_MAGIC_USER, 11331, EAST},
    {CLASS_CLERIC, 11332, NORTH},
    {CLASS_THIEF, 11333, SOUTH},
    {CLASS_WARRIOR, 11334, WEST},
    {CLASS_DRUID, 11335, NORTH},

    /* Brass Dragon */
    {-999 /* all */, 5065, WEST},

    {-999, 11642, SOUTH},
    {CLASS_THIEF, 20401, EAST},
    {-999, 17759, NORTH},

    /* this must go last -- add new guards above! */
    {-1, NOWHERE, -1}};

/* Saving throws for : MCTW : PARA, ROD, PETRI, BREATH, SPELL. Levels 0-40. Do
   not forget to change extern declaration in magic.c if you add to this. */
byte saving_throws(int class_num, int type, int level)
{
    switch (class_num) {
        case CLASS_MAGIC_USER:
            switch (type) {
                case SAVING_PARA: /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 69;
                        case 3:
                            return 68;
                        case 4:
                            return 67;
                        case 5:
                            return 66;
                        case 6:
                            return 65;
                        case 7:
                            return 63;
                        case 8:
                            return 61;
                        case 9:
                            return 60;
                        case 10:
                            return 59;
                        case 11:
                            return 57;
                        case 12:
                            return 55;
                        case 13:
                            return 54;
                        case 14:
                            return 53;
                        case 15:
                            return 53;
                        case 16:
                            return 52;
                        case 17:
                            return 51;
                        case 18:
                            return 50;
                        case 19:
                            return 48;
                        case 20:
                            return 46;
                        case 21:
                            return 45;
                        case 22:
                            return 44;
                        case 23:
                            return 42;
                        case 24:
                            return 40;
                        case 25:
                            return 38;
                        case 26:
                            return 36;
                        case 27:
                            return 34;
                        case 28:
                            return 32;
                        case 29:
                            return 30;
                        case 30:
                            return 28;
                        case 31:
                            return 26;
                        case 32:
                            return 24;
                        case 33:
                            return 22;
                        case 34:
                            return 20;
                        case 35:
                            return 18;
                        case 36:
                            return 16;
                        case 37:
                            return 14;
                        case 38:
                            return 12;
                        case 39:
                            return 10;
                        case 40:
                            return 8;
                        case 41:
                            return 6;
                        case 42:
                            return 4;
                        case 43:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_ROD: /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 55;
                        case 2:
                            return 53;
                        case 3:
                            return 51;
                        case 4:
                            return 49;
                        case 5:
                            return 47;
                        case 6:
                            return 45;
                        case 7:
                            return 43;
                        case 8:
                            return 41;
                        case 9:
                            return 40;
                        case 10:
                            return 39;
                        case 11:
                            return 37;
                        case 12:
                            return 35;
                        case 13:
                            return 33;
                        case 14:
                            return 31;
                        case 15:
                            return 30;
                        case 16:
                            return 29;
                        case 17:
                            return 27;
                        case 18:
                            return 25;
                        case 19:
                            return 23;
                        case 20:
                            return 21;
                        case 21:
                            return 20;
                        case 22:
                            return 19;
                        case 23:
                            return 17;
                        case 24:
                            return 15;
                        case 25:
                            return 14;
                        case 26:
                            return 13;
                        case 27:
                            return 12;
                        case 28:
                            return 11;
                        case 29:
                            return 10;
                        case 30:
                            return 9;
                        case 31:
                            return 8;
                        case 32:
                            return 7;
                        case 33:
                            return 6;
                        case 34:
                            return 5;
                        case 35:
                            return 4;
                        case 36:
                            return 3;
                        case 37:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_PETRI: /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 65;
                        case 2:
                            return 63;
                        case 3:
                            return 61;
                        case 4:
                            return 59;
                        case 5:
                            return 57;
                        case 6:
                            return 55;
                        case 7:
                            return 53;
                        case 8:
                            return 51;
                        case 9:
                            return 50;
                        case 10:
                            return 49;
                        case 11:
                            return 47;
                        case 12:
                            return 45;
                        case 13:
                            return 43;
                        case 14:
                            return 41;
                        case 15:
                            return 40;
                        case 16:
                            return 39;
                        case 17:
                            return 37;
                        case 18:
                            return 35;
                        case 19:
                            return 33;
                        case 20:
                            return 31;
                        case 21:
                            return 30;
                        case 22:
                            return 29;
                        case 23:
                            return 27;
                        case 24:
                            return 25;
                        case 25:
                            return 23;
                        case 26:
                            return 21;
                        case 27:
                            return 19;
                        case 28:
                            return 17;
                        case 29:
                            return 15;
                        case 30:
                            return 13;
                        case 31:
                            return 11;
                        case 32:
                            return 10;
                        case 33:
                            return 8;
                        case 34:
                            return 6;
                        case 35:
                            return 4;
                        case 36:
                            return 3;
                        case 37:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_BREATH: /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 73;
                        case 3:
                            return 71;
                        case 4:
                            return 69;
                        case 5:
                            return 67;
                        case 6:
                            return 65;
                        case 7:
                            return 63;
                        case 8:
                            return 61;
                        case 9:
                            return 60;
                        case 10:
                            return 59;
                        case 11:
                            return 57;
                        case 12:
                            return 55;
                        case 13:
                            return 53;
                        case 14:
                            return 51;
                        case 15:
                            return 50;
                        case 16:
                            return 49;
                        case 17:
                            return 47;
                        case 18:
                            return 45;
                        case 19:
                            return 43;
                        case 20:
                            return 41;
                        case 21:
                            return 40;
                        case 22:
                            return 39;
                        case 23:
                            return 37;
                        case 24:
                            return 35;
                        case 25:
                            return 33;
                        case 26:
                            return 31;
                        case 27:
                            return 29;
                        case 28:
                            return 27;
                        case 29:
                            return 25;
                        case 30:
                            return 23;
                        case 31:
                            return 21;
                        case 32:
                            return 20;
                        case 33:
                            return 19;
                        case 34:
                            return 17;
                        case 35:
                            return 15;
                        case 36:
                            return 13;
                        case 37:
                            return 11;
                        case 38:
                            return 10;
                        case 39:
                            return 9;
                        case 40:
                            return 7;
                        case 41:
                            return 5;
                        case 42:
                            return 3;
                        case 43:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_SPELL: /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 60;
                        case 2:
                            return 58;
                        case 3:
                            return 56;
                        case 4:
                            return 54;
                        case 5:
                            return 52;
                        case 6:
                            return 50;
                        case 7:
                            return 48;
                        case 8:
                            return 46;
                        case 9:
                            return 45;
                        case 10:
                            return 44;
                        case 11:
                            return 42;
                        case 12:
                            return 40;
                        case 13:
                            return 38;
                        case 14:
                            return 36;
                        case 15:
                            return 35;
                        case 16:
                            return 34;
                        case 17:
                            return 32;
                        case 18:
                            return 30;
                        case 19:
                            return 28;
                        case 20:
                            return 26;
                        case 21:
                            return 25;
                        case 22:
                            return 24;
                        case 23:
                            return 22;
                        case 24:
                            return 20;
                        case 25:
                            return 18;
                        case 26:
                            return 16;
                        case 27:
                            return 14;
                        case 28:
                            return 12;
                        case 29:
                            return 10;
                        case 30:
                            return 8;
                        case 31:
                            return 6;
                        case 32:
                            return 5;
                        case 33:
                            return 4;
                        case 34:
                            return 3;
                        case 35:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                default:
                    log1("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
        case CLASS_CLERIC:
            switch (type) {
                case SAVING_PARA: /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 60;
                        case 2:
                            return 59;
                        case 3:
                            return 48;
                        case 4:
                            return 46;
                        case 5:
                            return 45;
                        case 6:
                            return 43;
                        case 7:
                            return 40;
                        case 8:
                            return 37;
                        case 9:
                            return 35;
                        case 10:
                            return 34;
                        case 11:
                            return 33;
                        case 12:
                            return 31;
                        case 13:
                            return 30;
                        case 14:
                            return 29;
                        case 15:
                            return 27;
                        case 16:
                            return 26;
                        case 17:
                            return 25;
                        case 18:
                            return 24;
                        case 19:
                            return 23;
                        case 20:
                            return 22;
                        case 21:
                            return 21;
                        case 22:
                            return 20;
                        case 23:
                            return 18;
                        case 24:
                            return 15;
                        case 25:
                            return 14;
                        case 26:
                            return 12;
                        case 27:
                            return 10;
                        case 28:
                            return 9;
                        case 29:
                            return 8;
                        case 30:
                            return 7;
                        case 31:
                            return 5;
                        case 32:
                            return 4;
                        case 33:
                            return 3;
                        case 34:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_ROD: /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 69;
                        case 3:
                            return 68;
                        case 4:
                            return 66;
                        case 5:
                            return 65;
                        case 6:
                            return 63;
                        case 7:
                            return 60;
                        case 8:
                            return 57;
                        case 9:
                            return 55;
                        case 10:
                            return 54;
                        case 11:
                            return 53;
                        case 12:
                            return 51;
                        case 13:
                            return 50;
                        case 14:
                            return 49;
                        case 15:
                            return 47;
                        case 16:
                            return 46;
                        case 17:
                            return 45;
                        case 18:
                            return 44;
                        case 19:
                            return 43;
                        case 20:
                            return 42;
                        case 21:
                            return 41;
                        case 22:
                            return 40;
                        case 23:
                            return 38;
                        case 24:
                            return 35;
                        case 25:
                            return 34;
                        case 26:
                            return 32;
                        case 27:
                            return 30;
                        case 28:
                            return 29;
                        case 29:
                            return 28;
                        case 30:
                            return 27;
                        case 31:
                            return 26;
                        case 32:
                            return 25;
                        case 33:
                            return 24;
                        case 34:
                            return 23;
                        case 35:
                            return 22;
                        case 36:
                            return 21;
                        case 37:
                            return 20;
                        case 38:
                            return 18;
                        case 39:
                            return 15;
                        case 40:
                            return 14;
                        case 41:
                            return 12;
                        case 42:
                            return 10;
                        case 43:
                            return 9;
                        case 44:
                            return 8;
                        case 45:
                            return 7;
                        case 46:
                            return 6;
                        case 47:
                            return 5;
                        case 48:
                            return 4;
                        case 49:
                            return 3;
                        case 50:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_PETRI: /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 65;
                        case 2:
                            return 64;
                        case 3:
                            return 63;
                        case 4:
                            return 61;
                        case 5:
                            return 60;
                        case 6:
                            return 58;
                        case 7:
                            return 55;
                        case 8:
                            return 53;
                        case 9:
                            return 50;
                        case 10:
                            return 49;
                        case 11:
                            return 48;
                        case 12:
                            return 46;
                        case 13:
                            return 45;
                        case 14:
                            return 44;
                        case 15:
                            return 43;
                        case 16:
                            return 41;
                        case 17:
                            return 40;
                        case 18:
                            return 39;
                        case 19:
                            return 38;
                        case 20:
                            return 37;
                        case 21:
                            return 36;
                        case 22:
                            return 35;
                        case 23:
                            return 33;
                        case 24:
                            return 31;
                        case 25:
                            return 29;
                        case 26:
                            return 27;
                        case 27:
                            return 25;
                        case 28:
                            return 24;
                        case 29:
                            return 23;
                        case 30:
                            return 22;
                        case 31:
                            return 20;
                        case 32:
                            return 19;
                        case 33:
                            return 18;
                        case 34:
                            return 16;
                        case 35:
                            return 15;
                        case 36:
                            return 14;
                        case 37:
                            return 13;
                        case 38:
                            return 11;
                        case 39:
                            return 10;
                        case 40:
                            return 9;
                        case 41:
                            return 8;
                        case 42:
                            return 6;
                        case 43:
                            return 5;
                        case 44:
                            return 4;
                        case 45:
                            return 3;
                        case 46:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_BREATH: /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 80;
                        case 2:
                            return 79;
                        case 3:
                            return 78;
                        case 4:
                            return 76;
                        case 5:
                            return 75;
                        case 6:
                            return 73;
                        case 7:
                            return 70;
                        case 8:
                            return 67;
                        case 9:
                            return 65;
                        case 10:
                            return 64;
                        case 11:
                            return 63;
                        case 12:
                            return 61;
                        case 13:
                            return 60;
                        case 14:
                            return 59;
                        case 15:
                            return 57;
                        case 16:
                            return 56;
                        case 17:
                            return 55;
                        case 18:
                            return 54;
                        case 19:
                            return 53;
                        case 20:
                            return 52;
                        case 21:
                            return 51;
                        case 22:
                            return 50;
                        case 23:
                            return 48;
                        case 24:
                            return 45;
                        case 25:
                            return 44;
                        case 26:
                            return 42;
                        case 27:
                            return 40;
                        case 28:
                            return 39;
                        case 29:
                            return 38;
                        case 30:
                            return 37;
                        case 31:
                            return 35;
                        case 32:
                            return 34;
                        case 33:
                            return 33;
                        case 34:
                            return 30;
                        case 35:
                            return 29;
                        case 36:
                            return 28;
                        case 37:
                            return 26;
                        case 38:
                            return 24;
                        case 39:
                            return 22;
                        case 40:
                            return 21;
                        case 41:
                            return 20;
                        case 42:
                            return 18;
                        case 43:
                            return 16;
                        case 44:
                            return 15;
                        case 45:
                            return 13;
                        case 46:
                            return 11;
                        case 47:
                            return 10;
                        case 48:
                            return 8;
                        case 49:
                            return 6;
                        case 50:
                            return 4;
                        case 51:
                            return 3;
                        case 52:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_SPELL: /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 74;
                        case 3:
                            return 73;
                        case 4:
                            return 71;
                        case 5:
                            return 70;
                        case 6:
                            return 68;
                        case 7:
                            return 65;
                        case 8:
                            return 63;
                        case 9:
                            return 60;
                        case 10:
                            return 59;
                        case 11:
                            return 58;
                        case 12:
                            return 56;
                        case 13:
                            return 55;
                        case 14:
                            return 54;
                        case 15:
                            return 53;
                        case 16:
                            return 51;
                        case 17:
                            return 50;
                        case 18:
                            return 49;
                        case 19:
                            return 48;
                        case 20:
                            return 47;
                        case 21:
                            return 46;
                        case 22:
                            return 45;
                        case 23:
                            return 43;
                        case 24:
                            return 41;
                        case 25:
                            return 39;
                        case 26:
                            return 37;
                        case 27:
                            return 35;
                        case 28:
                            return 34;
                        case 29:
                            return 33;
                        case 30:
                            return 32;
                        case 31:
                            return 30;
                        case 32:
                            return 29;
                        case 33:
                            return 28;
                        case 34:
                            return 26;
                        case 35:
                            return 24;
                        case 36:
                            return 22;
                        case 37:
                            return 21;
                        case 38:
                            return 20;
                        case 39:
                            return 19;
                        case 40:
                            return 17;
                        case 41:
                            return 15;
                        case 42:
                            return 13;
                        case 43:
                            return 12;
                        case 44:
                            return 11;
                        case 45:
                            return 10;
                        case 46:
                            return 9;
                        case 47:
                            return 7;
                        case 48:
                            return 6;
                        case 49:
                            return 5;
                        case 50:
                            return 3;
                        case 51:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                default:
                    log1("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
        case CLASS_THIEF:
            switch (type) {
                case SAVING_PARA: /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 65;
                        case 2:
                            return 64;
                        case 3:
                            return 63;
                        case 4:
                            return 62;
                        case 5:
                            return 61;
                        case 6:
                            return 60;
                        case 7:
                            return 59;
                        case 8:
                            return 58;
                        case 9:
                            return 57;
                        case 10:
                            return 56;
                        case 11:
                            return 55;
                        case 12:
                            return 54;
                        case 13:
                            return 53;
                        case 14:
                            return 52;
                        case 15:
                            return 51;
                        case 16:
                            return 50;
                        case 17:
                            return 49;
                        case 18:
                            return 48;
                        case 19:
                            return 47;
                        case 20:
                            return 46;
                        case 21:
                            return 45;
                        case 22:
                            return 44;
                        case 23:
                            return 43;
                        case 24:
                            return 42;
                        case 25:
                            return 41;
                        case 26:
                            return 40;
                        case 27:
                            return 39;
                        case 28:
                            return 38;
                        case 29:
                            return 37;
                        case 30:
                            return 36;
                        case 31:
                            return 35;
                        case 32:
                            return 34;
                        case 33:
                            return 33;
                        case 34:
                            return 32;
                        case 35:
                            return 31;
                        case 36:
                            return 30;
                        case 37:
                            return 29;
                        case 38:
                            return 28;
                        case 39:
                            return 27;
                        case 40:
                            return 26;
                        case 41:
                            return 25;
                        case 42:
                            return 24;
                        case 43:
                            return 23;
                        case 44:
                            return 22;
                        case 45:
                            return 21;
                        case 46:
                            return 20;
                        case 47:
                            return 19;
                        case 48:
                            return 18;
                        case 49:
                            return 17;
                        case 50:
                            return 16;
                        case 51:
                            return 15;
                        case 52:
                            return 14;
                        case 53:
                            return 13;
                        case 54:
                            return 12;
                        case 55:
                            return 11;
                        case 56:
                            return 10;
                        case 57:
                            return 9;
                        case 58:
                            return 8;
                        case 59:
                            return 7;
                        case 60:
                            return 6;
                        case 61:
                            return 5;
                        case 62:
                            return 4;
                        case 63:
                            return 3;
                        case 64:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_ROD: /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 68;
                        case 3:
                            return 66;
                        case 4:
                            return 64;
                        case 5:
                            return 62;
                        case 6:
                            return 60;
                        case 7:
                            return 58;
                        case 8:
                            return 56;
                        case 9:
                            return 54;
                        case 10:
                            return 52;
                        case 11:
                            return 50;
                        case 12:
                            return 48;
                        case 13:
                            return 46;
                        case 14:
                            return 44;
                        case 15:
                            return 42;
                        case 16:
                            return 40;
                        case 17:
                            return 38;
                        case 18:
                            return 36;
                        case 19:
                            return 34;
                        case 20:
                            return 32;
                        case 21:
                            return 30;
                        case 22:
                            return 28;
                        case 23:
                            return 26;
                        case 24:
                            return 24;
                        case 25:
                            return 22;
                        case 26:
                            return 20;
                        case 27:
                            return 18;
                        case 28:
                            return 16;
                        case 29:
                            return 14;
                        case 30:
                            return 12;
                        case 31:
                            return 10;
                        case 32:
                            return 8;
                        case 33:
                            return 6;
                        case 34:
                            return 4;
                        case 35:
                            return 3;
                        case 36:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_PETRI: /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 60;
                        case 2:
                            return 59;
                        case 3:
                            return 58;
                        case 4:
                            return 58;
                        case 5:
                            return 56;
                        case 6:
                            return 55;
                        case 7:
                            return 54;
                        case 8:
                            return 53;
                        case 9:
                            return 52;
                        case 10:
                            return 51;
                        case 11:
                            return 50;
                        case 12:
                            return 49;
                        case 13:
                            return 48;
                        case 14:
                            return 47;
                        case 15:
                            return 46;
                        case 16:
                            return 45;
                        case 17:
                            return 44;
                        case 18:
                            return 43;
                        case 19:
                            return 42;
                        case 20:
                            return 41;
                        case 21:
                            return 40;
                        case 22:
                            return 39;
                        case 23:
                            return 38;
                        case 24:
                            return 37;
                        case 25:
                            return 36;
                        case 26:
                            return 35;
                        case 27:
                            return 34;
                        case 28:
                            return 33;
                        case 29:
                            return 32;
                        case 30:
                            return 31;
                        case 31:
                            return 30;
                        case 32:
                            return 29;
                        case 33:
                            return 28;
                        case 34:
                            return 27;
                        case 35:
                            return 26;
                        case 36:
                            return 25;
                        case 37:
                            return 24;
                        case 38:
                            return 23;
                        case 39:
                            return 22;
                        case 40:
                            return 21;
                        case 41:
                            return 20;
                        case 42:
                            return 19;
                        case 43:
                            return 18;
                        case 44:
                            return 17;
                        case 45:
                            return 16;
                        case 46:
                            return 15;
                        case 47:
                            return 14;
                        case 48:
                            return 13;
                        case 49:
                            return 12;
                        case 50:
                            return 11;
                        case 51:
                            return 10;
                        case 52:
                            return 9;
                        case 53:
                            return 8;
                        case 54:
                            return 7;
                        case 55:
                            return 6;
                        case 56:
                            return 5;
                        case 57:
                            return 4;
                        case 58:
                            return 3;
                        case 59:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_BREATH: /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 80;
                        case 2:
                            return 79;
                        case 3:
                            return 78;
                        case 4:
                            return 77;
                        case 5:
                            return 76;
                        case 6:
                            return 75;
                        case 7:
                            return 74;
                        case 8:
                            return 73;
                        case 9:
                            return 72;
                        case 10:
                            return 71;
                        case 11:
                            return 70;
                        case 12:
                            return 69;
                        case 13:
                            return 68;
                        case 14:
                            return 67;
                        case 15:
                            return 66;
                        case 16:
                            return 65;
                        case 17:
                            return 64;
                        case 18:
                            return 63;
                        case 19:
                            return 62;
                        case 20:
                            return 61;
                        case 21:
                            return 60;
                        case 22:
                            return 59;
                        case 23:
                            return 58;
                        case 24:
                            return 57;
                        case 25:
                            return 56;
                        case 26:
                            return 55;
                        case 27:
                            return 54;
                        case 28:
                            return 53;
                        case 29:
                            return 52;
                        case 30:
                            return 51;
                        case 31:
                            return 50;
                        case 32:
                            return 49;
                        case 33:
                            return 48;
                        case 34:
                            return 47;
                        case 35:
                            return 46;
                        case 36:
                            return 45;
                        case 37:
                            return 44;
                        case 38:
                            return 43;
                        case 39:
                            return 42;
                        case 40:
                            return 41;
                        case 41:
                            return 40;
                        case 42:
                            return 39;
                        case 43:
                            return 38;
                        case 44:
                            return 37;
                        case 45:
                            return 36;
                        case 46:
                            return 35;
                        case 47:
                            return 34;
                        case 48:
                            return 33;
                        case 49:
                            return 32;
                        case 50:
                            return 31;
                        case 51:
                            return 30;
                        case 52:
                            return 29;
                        case 53:
                            return 28;
                        case 54:
                            return 27;
                        case 55:
                            return 26;
                        case 56:
                            return 25;
                        case 57:
                            return 24;
                        case 58:
                            return 23;
                        case 59:
                            return 22;
                        case 60:
                            return 21;
                        case 61:
                            return 20;
                        case 62:
                            return 19;
                        case 63:
                            return 18;
                        case 64:
                            return 17;
                        case 65:
                            return 16;
                        case 66:
                            return 15;
                        case 67:
                            return 14;
                        case 68:
                            return 13;
                        case 69:
                            return 12;
                        case 70:
                            return 11;
                        case 71:
                            return 10;
                        case 72:
                            return 9;
                        case 73:
                            return 8;
                        case 74:
                            return 7;
                        case 75:
                            return 6;
                        case 76:
                            return 5;
                        case 77:
                            return 4;
                        case 78:
                            return 3;
                        case 79:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_SPELL: /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 73;
                        case 3:
                            return 71;
                        case 4:
                            return 69;
                        case 5:
                            return 67;
                        case 6:
                            return 65;
                        case 7:
                            return 63;
                        case 8:
                            return 61;
                        case 9:
                            return 59;
                        case 10:
                            return 57;
                        case 11:
                            return 55;
                        case 12:
                            return 53;
                        case 13:
                            return 51;
                        case 14:
                            return 49;
                        case 15:
                            return 47;
                        case 16:
                            return 45;
                        case 17:
                            return 43;
                        case 18:
                            return 41;
                        case 19:
                            return 39;
                        case 20:
                            return 37;
                        case 21:
                            return 35;
                        case 22:
                            return 33;
                        case 23:
                            return 31;
                        case 24:
                            return 29;
                        case 25:
                            return 27;
                        case 26:
                            return 25;
                        case 27:
                            return 23;
                        case 28:
                            return 21;
                        case 29:
                            return 19;
                        case 30:
                            return 17;
                        case 31:
                            return 15;
                        case 32:
                            return 13;
                        case 33:
                            return 11;
                        case 34:
                            return 9;
                        case 35:
                            return 7;
                        case 36:
                            return 5;
                        case 37:
                            return 3;
                        default:
                            return 1;
                            break;
                    }
                default:
                    log1("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
        case CLASS_WARRIOR:
            switch (type) {
                case SAVING_PARA: /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 68;
                        case 3:
                            return 67;
                        case 4:
                            return 65;
                        case 5:
                            return 62;
                        case 6:
                            return 58;
                        case 7:
                            return 55;
                        case 8:
                            return 53;
                        case 9:
                            return 52;
                        case 10:
                            return 50;
                        case 11:
                            return 47;
                        case 12:
                            return 43;
                        case 13:
                            return 40;
                        case 14:
                            return 38;
                        case 15:
                            return 37;
                        case 16:
                            return 35;
                        case 17:
                            return 32;
                        case 18:
                            return 28;
                        case 19:
                            return 25;
                        case 20:
                            return 24;
                        case 21:
                            return 23;
                        case 22:
                            return 22;
                        case 23:
                            return 20;
                        case 24:
                            return 19;
                        case 25:
                            return 17;
                        case 26:
                            return 16;
                        case 27:
                            return 15;
                        case 28:
                            return 14;
                        case 29:
                            return 13;
                        case 30:
                            return 12;
                        case 31:
                            return 11;
                        case 32:
                            return 10;
                        case 33:
                            return 9;
                        case 34:
                            return 8;
                        case 35:
                            return 7;
                        case 36:
                            return 6;
                        case 37:
                            return 5;
                        case 38:
                            return 4;
                        case 39:
                            return 3;
                        case 40:
                            return 2;
                        case 41:
                            return 1; /* Some mobiles. */
                        default:
                            return 1;
                            break;
                    }
                case SAVING_ROD: /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 80;
                        case 2:
                            return 78;
                        case 3:
                            return 77;
                        case 4:
                            return 75;
                        case 5:
                            return 72;
                        case 6:
                            return 68;
                        case 7:
                            return 65;
                        case 8:
                            return 63;
                        case 9:
                            return 62;
                        case 10:
                            return 60;
                        case 11:
                            return 57;
                        case 12:
                            return 53;
                        case 13:
                            return 50;
                        case 14:
                            return 48;
                        case 15:
                            return 47;
                        case 16:
                            return 45;
                        case 17:
                            return 42;
                        case 18:
                            return 38;
                        case 19:
                            return 35;
                        case 20:
                            return 34;
                        case 21:
                            return 33;
                        case 22:
                            return 32;
                        case 23:
                            return 30;
                        case 24:
                            return 29;
                        case 25:
                            return 27;
                        case 26:
                            return 26;
                        case 27:
                            return 25;
                        case 28:
                            return 24;
                        case 29:
                            return 23;
                        case 30:
                            return 22;
                        case 31:
                            return 20;
                        case 32:
                            return 18;
                        case 33:
                            return 16;
                        case 34:
                            return 14;
                        case 35:
                            return 12;
                        case 36:
                            return 10;
                        case 37:
                            return 8;
                        case 38:
                            return 6;
                        case 39:
                            return 5;
                        case 40:
                            return 4;
                        case 41:
                            return 3;
                        case 42:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_PETRI: /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 73;
                        case 3:
                            return 72;
                        case 4:
                            return 70;
                        case 5:
                            return 67;
                        case 6:
                            return 63;
                        case 7:
                            return 60;
                        case 8:
                            return 58;
                        case 9:
                            return 57;
                        case 10:
                            return 55;
                        case 11:
                            return 52;
                        case 12:
                            return 48;
                        case 13:
                            return 45;
                        case 14:
                            return 43;
                        case 15:
                            return 42;
                        case 16:
                            return 40;
                        case 17:
                            return 37;
                        case 18:
                            return 33;
                        case 19:
                            return 30;
                        case 20:
                            return 29;
                        case 21:
                            return 28;
                        case 22:
                            return 26;
                        case 23:
                            return 25;
                        case 24:
                            return 24;
                        case 25:
                            return 23;
                        case 26:
                            return 21;
                        case 27:
                            return 20;
                        case 28:
                            return 19;
                        case 29:
                            return 18;
                        case 30:
                            return 17;
                        case 31:
                            return 16;
                        case 32:
                            return 15;
                        case 33:
                            return 14;
                        case 34:
                            return 13;
                        case 35:
                            return 12;
                        case 36:
                            return 11;
                        case 37:
                            return 10;
                        case 38:
                            return 9;
                        case 39:
                            return 8;
                        case 40:
                            return 7;
                        case 41:
                            return 6;
                        case 42:
                            return 5;
                        case 43:
                            return 4;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_BREATH: /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 85;
                        case 2:
                            return 83;
                        case 3:
                            return 82;
                        case 4:
                            return 80;
                        case 5:
                            return 75;
                        case 6:
                            return 70;
                        case 7:
                            return 65;
                        case 8:
                            return 63;
                        case 9:
                            return 62;
                        case 10:
                            return 60;
                        case 11:
                            return 55;
                        case 12:
                            return 50;
                        case 13:
                            return 45;
                        case 14:
                            return 43;
                        case 15:
                            return 42;
                        case 16:
                            return 40;
                        case 17:
                            return 37;
                        case 18:
                            return 33;
                        case 19:
                            return 30;
                        case 20:
                            return 29;
                        case 21:
                            return 28;
                        case 22:
                            return 26;
                        case 23:
                            return 25;
                        case 24:
                            return 24;
                        case 25:
                            return 23;
                        case 26:
                            return 21;
                        case 27:
                            return 20;
                        case 28:
                            return 19;
                        case 29:
                            return 18;
                        case 30:
                            return 17;
                        case 31:
                            return 16;
                        case 32:
                            return 15;
                        case 33:
                            return 14;
                        case 34:
                            return 13;
                        case 35:
                            return 12;
                        case 36:
                            return 11;
                        case 37:
                            return 10;
                        case 38:
                            return 9;
                        case 39:
                            return 8;
                        case 40:
                            return 7;
                        case 41:
                            return 6;
                        case 42:
                            return 5;
                        case 43:
                            return 4;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_SPELL: /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 85;
                        case 2:
                            return 83;
                        case 3:
                            return 82;
                        case 4:
                            return 80;
                        case 5:
                            return 77;
                        case 6:
                            return 73;
                        case 7:
                            return 70;
                        case 8:
                            return 68;
                        case 9:
                            return 67;
                        case 10:
                            return 65;
                        case 11:
                            return 62;
                        case 12:
                            return 58;
                        case 13:
                            return 55;
                        case 14:
                            return 53;
                        case 15:
                            return 52;
                        case 16:
                            return 50;
                        case 17:
                            return 47;
                        case 18:
                            return 43;
                        case 19:
                            return 40;
                        case 20:
                            return 39;
                        case 21:
                            return 38;
                        case 22:
                            return 36;
                        case 23:
                            return 35;
                        case 24:
                            return 34;
                        case 25:
                            return 33;
                        case 26:
                            return 31;
                        case 27:
                            return 30;
                        case 28:
                            return 29;
                        case 29:
                            return 28;
                        case 30:
                            return 27;
                        case 31:
                            return 25;
                        case 32:
                            return 23;
                        case 33:
                            return 21;
                        case 34:
                            return 19;
                        case 35:
                            return 17;
                        case 36:
                            return 15;
                        case 37:
                            return 13;
                        case 38:
                            return 11;
                        case 39:
                            return 9;
                        case 40:
                            return 7;
                        case 41:
                            return 6;
                        case 42:
                            return 5;
                        case 43:
                            return 4;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                default:
                    log1("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
        case CLASS_RANGER:
            switch (type) {
                case SAVING_PARA: /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 65;
                        case 2:
                            return 64;
                        case 3:
                            return 63;
                        case 4:
                            return 62;
                        case 5:
                            return 61;
                        case 6:
                            return 60;
                        case 7:
                            return 59;
                        case 8:
                            return 58;
                        case 9:
                            return 57;
                        case 10:
                            return 56;
                        case 11:
                            return 55;
                        case 12:
                            return 54;
                        case 13:
                            return 53;
                        case 14:
                            return 52;
                        case 15:
                            return 51;
                        case 16:
                            return 50;
                        case 17:
                            return 49;
                        case 18:
                            return 48;
                        case 19:
                            return 47;
                        case 20:
                            return 46;
                        case 21:
                            return 45;
                        case 22:
                            return 44;
                        case 23:
                            return 43;
                        case 24:
                            return 42;
                        case 25:
                            return 41;
                        case 26:
                            return 40;
                        case 27:
                            return 39;
                        case 28:
                            return 38;
                        case 29:
                            return 37;
                        case 30:
                            return 36;
                        case 31:
                            return 33;
                        case 32:
                            return 30;
                        case 33:
                            return 27;
                        case 34:
                            return 24;
                        case 35:
                            return 21;
                        case 36:
                            return 19;
                        case 37:
                            return 16;
                        case 38:
                            return 13;
                        case 39:
                            return 11;
                        case 40:
                            return 9;
                        case 41:
                            return 7;
                        case 42:
                            return 5;
                        case 43:
                            return 3;
                        case 44:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_ROD: /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 68;
                        case 3:
                            return 66;
                        case 4:
                            return 64;
                        case 5:
                            return 62;
                        case 6:
                            return 60;
                        case 7:
                            return 58;
                        case 8:
                            return 56;
                        case 9:
                            return 54;
                        case 10:
                            return 52;
                        case 11:
                            return 50;
                        case 12:
                            return 48;
                        case 13:
                            return 46;
                        case 14:
                            return 44;
                        case 15:
                            return 42;
                        case 16:
                            return 40;
                        case 17:
                            return 38;
                        case 18:
                            return 36;
                        case 19:
                            return 34;
                        case 20:
                            return 32;
                        case 21:
                            return 30;
                        case 22:
                            return 28;
                        case 23:
                            return 26;
                        case 24:
                            return 24;
                        case 25:
                            return 22;
                        case 26:
                            return 20;
                        case 27:
                            return 18;
                        case 28:
                            return 16;
                        case 29:
                            return 14;
                        case 30:
                            return 13;
                        case 31:
                            return 12;
                        case 32:
                            return 11;
                        case 33:
                            return 10;
                        case 34:
                            return 9;
                        case 35:
                            return 8;
                        case 36:
                            return 7;
                        case 37:
                            return 6;
                        case 38:
                            return 4;
                        case 39:
                            return 3;
                        case 40:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_PETRI: /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 60;
                        case 2:
                            return 59;
                        case 3:
                            return 58;
                        case 4:
                            return 58;
                        case 5:
                            return 56;
                        case 6:
                            return 55;
                        case 7:
                            return 54;
                        case 8:
                            return 53;
                        case 9:
                            return 52;
                        case 10:
                            return 51;
                        case 11:
                            return 50;
                        case 12:
                            return 49;
                        case 13:
                            return 48;
                        case 14:
                            return 47;
                        case 15:
                            return 46;
                        case 16:
                            return 45;
                        case 17:
                            return 44;
                        case 18:
                            return 43;
                        case 19:
                            return 42;
                        case 20:
                            return 41;
                        case 21:
                            return 40;
                        case 22:
                            return 39;
                        case 23:
                            return 38;
                        case 24:
                            return 37;
                        case 25:
                            return 36;
                        case 26:
                            return 35;
                        case 27:
                            return 34;
                        case 28:
                            return 33;
                        case 29:
                            return 32;
                        case 30:
                            return 31;
                        case 31:
                            return 30;
                        case 32:
                            return 28;
                        case 33:
                            return 26;
                        case 34:
                            return 24;
                        case 35:
                            return 22;
                        case 36:
                            return 20;
                        case 37:
                            return 17;
                        case 38:
                            return 15;
                        case 39:
                            return 13;
                        case 40:
                            return 12;
                        case 41:
                            return 10;
                        case 42:
                            return 8;
                        case 43:
                            return 5;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_BREATH: /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        default:
                            return MAX((81 - level), 1);
                            break;
                    }
                case SAVING_SPELL: /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 73;
                        case 3:
                            return 71;
                        case 4:
                            return 69;
                        case 5:
                            return 67;
                        case 6:
                            return 65;
                        case 7:
                            return 63;
                        case 8:
                            return 61;
                        case 9:
                            return 59;
                        case 10:
                            return 57;
                        case 11:
                            return 55;
                        case 12:
                            return 53;
                        case 13:
                            return 51;
                        case 14:
                            return 49;
                        case 15:
                            return 47;
                        case 16:
                            return 45;
                        case 17:
                            return 43;
                        case 18:
                            return 41;
                        case 19:
                            return 39;
                        case 20:
                            return 37;
                        case 21:
                            return 35;
                        case 22:
                            return 33;
                        case 23:
                            return 31;
                        case 24:
                            return 29;
                        case 25:
                            return 27;
                        case 26:
                            return 25;
                        case 27:
                            return 23;
                        case 28:
                            return 21;
                        case 29:
                            return 19;
                        case 30:
                            return 17;
                        case 31:
                            return 15;
                        case 32:
                            return 13;
                        case 33:
                            return 11;
                        case 34:
                            return 9;
                        case 35:
                            return 7;
                        case 36:
                            return 5;
                        case 37:
                            return 3;
                        case 38:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                default:
                    log1("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
            /* Druidas */
        case CLASS_DRUID:
            switch (type) {
                case SAVING_PARA: /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 60;
                        case 2:
                            return 55;
                        case 3:
                            return 48;
                        case 4:
                            return 46;
                        case 5:
                            return 45;
                        case 6:
                            return 43;
                        case 7:
                            return 40;
                        case 8:
                            return 37;
                        case 9:
                            return 35;
                        case 10:
                            return 34;
                        case 11:
                            return 33;
                        case 12:
                            return 31;
                        case 13:
                            return 30;
                        case 14:
                            return 29;
                        case 15:
                            return 27;
                        case 16:
                            return 26;
                        case 17:
                            return 25;
                        case 18:
                            return 24;
                        case 19:
                            return 23;
                        case 20:
                            return 22;
                        case 21:
                            return 21;
                        case 22:
                            return 20;
                        case 23:
                            return 18;
                        case 24:
                            return 15;
                        case 25:
                            return 14;
                        case 26:
                            return 12;
                        case 27:
                            return 10;
                        case 28:
                            return 9;
                        case 29:
                            return 8;
                        case 30:
                            return 7;
                        case 31:
                            return 5;
                        case 32:
                            return 4;
                        case 33:
                            return 3;
                        case 34:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_ROD: /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 69;
                        case 3:
                            return 68;
                        case 4:
                            return 66;
                        case 5:
                            return 65;
                        case 6:
                            return 63;
                        case 7:
                            return 60;
                        case 8:
                            return 57;
                        case 9:
                            return 55;
                        case 10:
                            return 54;
                        case 11:
                            return 53;
                        case 12:
                            return 51;
                        case 13:
                            return 50;
                        case 14:
                            return 49;
                        case 15:
                            return 47;
                        case 16:
                            return 46;
                        case 17:
                            return 45;
                        case 18:
                            return 44;
                        case 19:
                            return 43;
                        case 20:
                            return 42;
                        case 21:
                            return 41;
                        case 22:
                            return 40;
                        case 23:
                            return 38;
                        case 24:
                            return 35;
                        case 25:
                            return 34;
                        case 26:
                            return 32;
                        case 27:
                            return 30;
                        case 28:
                            return 29;
                        case 29:
                            return 28;
                        case 30:
                            return 27;
                        case 31:
                            return 26;
                        case 32:
                            return 25;
                        case 33:
                            return 24;
                        case 34:
                            return 23;
                        case 35:
                            return 22;
                        case 36:
                            return 21;
                        case 37:
                            return 20;
                        case 38:
                            return 18;
                        case 39:
                            return 15;
                        case 40:
                            return 14;
                        case 41:
                            return 12;
                        case 42:
                            return 10;
                        case 43:
                            return 9;
                        case 44:
                            return 8;
                        case 45:
                            return 7;
                        case 46:
                            return 6;
                        case 47:
                            return 5;
                        case 48:
                            return 4;
                        case 49:
                            return 3;
                        case 50:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_PETRI: /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 65;
                        case 2:
                            return 64;
                        case 3:
                            return 63;
                        case 4:
                            return 61;
                        case 5:
                            return 60;
                        case 6:
                            return 58;
                        case 7:
                            return 55;
                        case 8:
                            return 53;
                        case 9:
                            return 50;
                        case 10:
                            return 49;
                        case 11:
                            return 48;
                        case 12:
                            return 46;
                        case 13:
                            return 45;
                        case 14:
                            return 44;
                        case 15:
                            return 43;
                        case 16:
                            return 41;
                        case 17:
                            return 40;
                        case 18:
                            return 39;
                        case 19:
                            return 38;
                        case 20:
                            return 37;
                        case 21:
                            return 36;
                        case 22:
                            return 35;
                        case 23:
                            return 33;
                        case 24:
                            return 31;
                        case 25:
                            return 29;
                        case 26:
                            return 27;
                        case 27:
                            return 25;
                        case 28:
                            return 24;
                        case 29:
                            return 23;
                        case 30:
                            return 22;
                        case 31:
                            return 20;
                        case 32:
                            return 19;
                        case 33:
                            return 18;
                        case 34:
                            return 16;
                        case 35:
                            return 15;
                        case 36:
                            return 14;
                        case 37:
                            return 13;
                        case 38:
                            return 11;
                        case 39:
                            return 10;
                        case 40:
                            return 9;
                        case 41:
                            return 8;
                        case 42:
                            return 6;
                        case 43:
                            return 5;
                        case 44:
                            return 4;
                        case 45:
                            return 3;
                        case 46:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_BREATH: /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 80;
                        case 2:
                            return 79;
                        case 3:
                            return 78;
                        case 4:
                            return 76;
                        case 5:
                            return 75;
                        case 6:
                            return 73;
                        case 7:
                            return 70;
                        case 8:
                            return 67;
                        case 9:
                            return 65;
                        case 10:
                            return 64;
                        case 11:
                            return 63;
                        case 12:
                            return 61;
                        case 13:
                            return 60;
                        case 14:
                            return 59;
                        case 15:
                            return 57;
                        case 16:
                            return 56;
                        case 17:
                            return 55;
                        case 18:
                            return 54;
                        case 19:
                            return 53;
                        case 20:
                            return 52;
                        case 21:
                            return 51;
                        case 22:
                            return 50;
                        case 23:
                            return 48;
                        case 24:
                            return 45;
                        case 25:
                            return 44;
                        case 26:
                            return 42;
                        case 27:
                            return 40;
                        case 28:
                            return 39;
                        case 29:
                            return 38;
                        case 30:
                            return 37;
                        case 31:
                            return 35;
                        case 32:
                            return 34;
                        case 33:
                            return 33;
                        case 34:
                            return 30;
                        case 35:
                            return 29;
                        case 36:
                            return 28;
                        case 37:
                            return 26;
                        case 38:
                            return 24;
                        case 39:
                            return 22;
                        case 40:
                            return 21;
                        case 41:
                            return 20;
                        case 42:
                            return 18;
                        case 43:
                            return 16;
                        case 44:
                            return 15;
                        case 45:
                            return 13;
                        case 46:
                            return 11;
                        case 47:
                            return 10;
                        case 48:
                            return 8;
                        case 49:
                            return 6;
                        case 50:
                            return 4;
                        case 51:
                            return 3;
                        case 52:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_SPELL: /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 74;
                        case 3:
                            return 73;
                        case 4:
                            return 71;
                        case 5:
                            return 70;
                        case 6:
                            return 68;
                        case 7:
                            return 65;
                        case 8:
                            return 63;
                        case 9:
                            return 60;
                        case 10:
                            return 59;
                        case 11:
                            return 58;
                        case 12:
                            return 56;
                        case 13:
                            return 55;
                        case 14:
                            return 54;
                        case 15:
                            return 53;
                        case 16:
                            return 51;
                        case 17:
                            return 50;
                        case 18:
                            return 49;
                        case 19:
                            return 48;
                        case 20:
                            return 47;
                        case 21:
                            return 46;
                        case 22:
                            return 45;
                        case 23:
                            return 43;
                        case 24:
                            return 41;
                        case 25:
                            return 39;
                        case 26:
                            return 37;
                        case 27:
                            return 35;
                        case 28:
                            return 34;
                        case 29:
                            return 33;
                        case 30:
                            return 32;
                        case 31:
                            return 30;
                        case 32:
                            return 29;
                        case 33:
                            return 28;
                        case 34:
                            return 26;
                        case 35:
                            return 24;
                        case 36:
                            return 22;
                        case 37:
                            return 21;
                        case 38:
                            return 20;
                        case 39:
                            return 19;
                        case 40:
                            return 17;
                        case 41:
                            return 15;
                        case 42:
                            return 13;
                        case 43:
                            return 12;
                        case 44:
                            return 11;
                        case 45:
                            return 10;
                        case 46:
                            return 9;
                        case 47:
                            return 7;
                        case 48:
                            return 6;
                        case 49:
                            return 5;
                        case 50:
                            return 3;
                        case 51:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                default:
                    log1("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
            /* Bardo */
        case CLASS_BARD:
            switch (type) {
                case SAVING_PARA: /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 65;
                        case 3:
                            return 55;
                        case 4:
                            return 50;
                        case 5:
                            return 45;
                        case 6:
                            return 42;
                        case 7:
                            return 39;
                        case 8:
                            return 37;
                        case 9:
                            return 35;
                        case 10:
                            return 33;
                        case 11:
                            return 32;
                        case 12:
                            return 31;
                        case 13:
                            return 30;
                        case 14:
                            return 29;
                        case 15:
                            return 28;
                        case 16:
                            return 27;
                        case 17:
                            return 26;
                        case 18:
                            return 25;
                        case 19:
                            return 24;
                        case 20:
                            return 23;
                        case 21:
                            return 22;
                        case 22:
                            return 21;
                        case 23:
                            return 20;
                        case 24:
                            return 19;
                        case 25:
                            return 18;
                        case 26:
                            return 17;
                        case 27:
                            return 16;
                        case 28:
                            return 15;
                        case 29:
                            return 14;
                        case 30:
                            return 13;
                        case 31:
                            return 12;
                        case 32:
                            return 11;
                        case 33:
                            return 10;
                        case 34:
                            return 9;
                        case 35:
                            return 8;
                        case 36:
                            return 7;
                        case 37:
                            return 6;
                        case 38:
                            return 5;
                        case 39:
                            return 4;
                        case 40:
                            return 3;
                        case 41:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_ROD: /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 69;
                        case 3:
                            return 66;
                        case 4:
                            return 63;
                        case 5:
                            return 60;
                        case 6:
                            return 59;
                        case 7:
                            return 58;
                        case 8:
                            return 57;
                        case 9:
                            return 55;
                        case 10:
                            return 53;
                        case 11:
                            return 51;
                        case 12:
                            return 49;
                        case 13:
                            return 46;
                        case 14:
                            return 43;
                        case 15:
                            return 40;
                        case 16:
                            return 38;
                        case 17:
                            return 35;
                        case 18:
                            return 32;
                        case 19:
                            return 29;
                        case 20:
                            return 25;
                        case 21:
                            return 22;
                        case 22:
                            return 20;
                        case 23:
                            return 19;
                        case 24:
                            return 17;
                        case 25:
                            return 15;
                        case 26:
                            return 14;
                        case 27:
                            return 13;
                        case 28:
                            return 12;
                        case 29:
                            return 11;
                        case 30:
                            return 9;
                        case 31:
                            return 8;
                        case 32:
                            return 7;
                        case 33:
                            return 6;
                        case 34:
                            return 5;
                        case 35:
                            return 4;
                        case 36:
                            return 3;
                        case 37:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_PETRI: /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 66;
                        case 3:
                            return 63;
                        case 4:
                            return 61;
                        case 5:
                            return 59;
                        case 6:
                            return 58;
                        case 7:
                            return 57;
                        case 8:
                            return 55;
                        case 9:
                            return 52;
                        case 10:
                            return 49;
                        case 11:
                            return 48;
                        case 12:
                            return 47;
                        case 13:
                            return 46;
                        case 14:
                            return 45;
                        case 15:
                            return 42;
                        case 16:
                            return 39;
                        case 17:
                            return 36;
                        case 18:
                            return 33;
                        case 19:
                            return 30;
                        case 20:
                            return 29;
                        case 21:
                            return 28;
                        case 22:
                            return 25;
                        case 23:
                            return 24;
                        case 24:
                            return 23;
                        case 25:
                            return 22;
                        case 26:
                            return 20;
                        case 27:
                            return 17;
                        case 28:
                            return 14;
                        case 29:
                            return 13;
                        case 30:
                            return 11;
                        case 31:
                            return 10;
                        case 32:
                            return 9;
                        case 33:
                            return 8;
                        case 34:
                            return 6;
                        case 35:
                            return 5;
                        case 36:
                            return 4;
                        case 37:
                            return 3;
                        case 38:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_BREATH: /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 80;
                        case 2:
                            return 79;
                        case 3:
                            return 78;
                        case 4:
                            return 76;
                        case 5:
                            return 75;
                        case 6:
                            return 74;
                        case 7:
                            return 72;
                        case 8:
                            return 69;
                        case 9:
                            return 65;
                        case 10:
                            return 64;
                        case 11:
                            return 63;
                        case 12:
                            return 62;
                        case 13:
                            return 61;
                        case 14:
                            return 59;
                        case 15:
                            return 58;
                        case 16:
                            return 57;
                        case 17:
                            return 56;
                        case 18:
                            return 55;
                        case 19:
                            return 54;
                        case 20:
                            return 53;
                        case 21:
                            return 52;
                        case 22:
                            return 51;
                        case 23:
                            return 49;
                        case 24:
                            return 47;
                        case 25:
                            return 46;
                        case 26:
                            return 45;
                        case 27:
                            return 43;
                        case 28:
                            return 41;
                        case 29:
                            return 39;
                        case 30:
                            return 38;
                        case 31:
                            return 37;
                        case 32:
                            return 36;
                        case 33:
                            return 35;
                        case 34:
                            return 34;
                        case 35:
                            return 33;
                        case 36:
                            return 32;
                        case 37:
                            return 30;
                        case 38:
                            return 29;
                        case 39:
                            return 27;
                        case 40:
                            return 25;
                        case 41:
                            return 24;
                        case 42:
                            return 23;
                        case 43:
                            return 22;
                        case 44:
                            return 21;
                        case 45:
                            return 20;
                        case 46:
                            return 19;
                        case 47:
                            return 18;
                        case 48:
                            return 17;
                        case 49:
                            return 16;
                        case 50:
                            return 15;
                        case 51:
                            return 14;
                        case 52:
                            return 13;
                        case 53:
                            return 12;
                        case 54:
                            return 11;
                        case 55:
                            return 10;
                        case 56:
                            return 9;
                        case 57:
                            return 8;
                        case 58:
                            return 7;
                        case 59:
                            return 6;
                        case 60:
                            return 5;
                        case 61:
                            return 4;
                        case 62:
                            return 3;
                        case 63:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                case SAVING_SPELL: /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 77;
                        case 2:
                            return 73;
                        case 3:
                            return 72;
                        case 4:
                            return 71;
                        case 5:
                            return 70;
                        case 6:
                            return 69;
                        case 7:
                            return 67;
                        case 8:
                            return 65;
                        case 9:
                            return 63;
                        case 10:
                            return 60;
                        case 11:
                            return 58;
                        case 12:
                            return 57;
                        case 13:
                            return 56;
                        case 14:
                            return 55;
                        case 15:
                            return 54;
                        case 16:
                            return 53;
                        case 17:
                            return 52;
                        case 18:
                            return 50;
                        case 19:
                            return 49;
                        case 20:
                            return 47;
                        case 21:
                            return 46;
                        case 22:
                            return 45;
                        case 23:
                            return 43;
                        case 24:
                            return 40;
                        case 25:
                            return 37;
                        case 26:
                            return 35;
                        case 27:
                            return 33;
                        case 28:
                            return 30;
                        case 29:
                            return 27;
                        case 30:
                            return 24;
                        case 31:
                            return 22;
                        case 32:
                            return 20;
                        case 33:
                            return 19;
                        case 34:
                            return 16;
                        case 35:
                            return 13;
                        case 36:
                            return 11;
                        case 37:
                            return 10;
                        case 38:
                            return 9;
                        case 39:
                            return 8;
                        case 40:
                            return 7;
                        case 41:
                            return 6;
                        case 42:
                            return 5;
                        case 43:
                            return 4;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        default:
                            return 1;
                            break;
                    }
                default:
                    log1("SYSERR: Invalid saving throw type.");
                    break;
            }
        default:
            log1("SYSERR: Invalid class saving throw.");
            break;
    }

    /* Should not get here unless something is wrong. */
    return 100;
}

/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(int class_num, int level)
{
    switch (class_num) {
        case CLASS_MAGIC_USER:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                case 3:
                case 4:
                    return 20;
                case 5:
                case 6:
                case 7:
                case 8:
                    return 19;
                case 9:
                case 10:
                case 11:
                case 12:
                    return 18;
                case 13:
                case 14:
                case 15:
                case 16:
                    return 17;
                case 17:
                case 18:
                case 19:
                case 20:
                    return 16;
                case 21:
                case 22:
                case 23:
                case 24:
                    return 15;
                case 25:
                case 26:
                case 27:
                case 28:
                    return 14;
                case 29:
                case 30:
                case 31:
                case 32:
                    return 13;
                case 33:
                case 34:
                case 35:
                case 36:
                    return 12;
                case 37:
                case 38:
                case 39:
                case 40:
                    return 11;
                case 41:
                case 42:
                case 43:
                case 44:
                    return 10;
                case 45:
                case 46:
                case 47:
                case 48:
                    return 9;
                case 49:
                case 50:
                case 51:
                case 52:
                    return 8;
                case 53:
                case 54:
                case 55:
                case 56:
                    return 7;
                case 57:
                case 58:
                case 59:
                case 60:
                    return 6;
                case 61:
                case 62:
                case 63:
                case 64:
                    return 5;
                case 65:
                case 66:
                case 67:
                case 68:
                    return 4;
                case 69:
                case 70:
                case 71:
                case 72:
                    return 3;
                case 73:
                case 74:
                case 75:
                case 76:
                    return 2;
                default:
                    return 1;
            }
            break;
        case CLASS_CLERIC:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                case 3:
                case 4:
                    return 20;
                case 5:
                case 6:
                case 7:
                case 8:
                    return 18;
                case 9:
                case 10:
                case 11:
                case 12:
                    return 16;
                case 13:
                case 14:
                case 15:
                case 16:
                    return 14;
                case 17:
                case 18:
                case 19:
                case 20:
                    return 12;
                case 21:
                case 22:
                case 23:
                case 24:
                    return 10;
                case 25:
                case 26:
                case 27:
                case 28:
                    return 8;
                case 29:
                case 30:
                case 31:
                case 32:
                    return 6;
                case 33:
                case 34:
                case 35:
                case 36:
                    return 4;
                case 37:
                case 38:
                case 39:
                case 40:
                    return 2;
                default:
                    return 1;
            }
            break;
        case CLASS_THIEF:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                case 3:
                    return 20;
                case 4:
                case 5:
                case 6:
                    return 19;
                case 7:
                case 8:
                case 9:
                    return 18;
                case 10:
                case 11:
                case 12:
                    return 17;
                case 13:
                case 14:
                case 15:
                    return 16;
                case 16:
                case 17:
                case 18:
                    return 15;
                case 19:
                case 20:
                case 21:
                    return 14;
                case 22:
                case 23:
                case 24:
                    return 13;
                case 25:
                case 26:
                case 27:
                    return 12;
                case 28:
                case 29:
                case 30:
                    return 11;
                case 31:
                case 32:
                case 33:
                    return 10;
                case 34:
                case 35:
                case 36:
                    return 9;
                case 37:
                case 38:
                case 39:
                    return 8;
                case 40:
                case 41:
                case 42:
                    return 7;
                case 43:
                case 44:
                case 45:
                    return 6;
                case 46:
                case 47:
                case 48:
                    return 5;
                case 49:
                case 50:
                case 51:
                    return 4;
                case 52:
                case 53:
                case 54:
                    return 3;
                case 55:
                case 56:
                case 57:
                    return 2;
                default:
                    return 1;
            }
            break;
        case CLASS_WARRIOR:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                    return 20;
                case 3:
                case 4:
                    return 19;
                case 5:
                case 6:
                    return 18;
                case 7:
                case 8:
                    return 17;
                case 9:
                case 10:
                    return 16;
                case 11:
                case 12:
                    return 15;
                case 13:
                case 14:
                    return 14;
                case 15:
                case 16:
                    return 13;
                case 17:
                case 18:
                    return 12;
                case 19:
                case 20:
                    return 11;
                case 21:
                case 22:
                    return 10;
                case 23:
                case 24:
                    return 9;
                case 25:
                case 26:
                    return 8;
                case 27:
                case 28:
                    return 7;
                case 29:
                case 30:
                    return 6;
                case 31:
                case 32:
                    return 5;
                case 33:
                case 34:
                    return 4;
                case 35:
                case 36:
                    return 3;
                case 37:
                case 38:
                    return 2;
                default:
                    return 1;
            }
        case CLASS_RANGER:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                    return 20;
                case 2:
                    return 19;
                case 3:
                    return 18;
                case 4:
                    return 17;
                case 5:
                    return 16;
                case 6:
                    return 15;
                case 7:
                    return 14;
                case 8:
                    return 13;
                case 9:
                    return 12;
                case 10:
                    return 11;
                case 11:
                    return 10;
                case 12:
                    return 9;
                case 13:
                    return 9;
                case 14:
                    return 8;
                case 15:
                    return 8;
                case 16:
                    return 7;
                case 17:
                    return 7;
                case 18:
                    return 6;
                case 19:
                    return 6;
                case 20:
                    return 5;
                case 21:
                    return 5;
                case 22:
                    return 5;
                case 23:
                    return 4;
                case 24:
                    return 4;
                case 25:
                    return 4;
                case 26:
                    return 3;
                case 27:
                    return 3;
                case 28:
                    return 3;
                case 29:
                    return 2;
                case 30:
                    return 2;
                case 31:
                    return 2;
                default:
                    return 1;
            }
            break;
        case CLASS_DRUID:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                    return 20;
                case 3:
                case 4:
                    return 19;
                case 5:
                case 6:
                case 7:
                case 8:
                    return 18;
                case 9:
                case 10:
                case 11:
                case 12:
                    return 17;
                case 13:
                case 14:
                    return 16;
                case 15:
                case 16:
                case 17:
                    return 15;
                case 18:
                case 19:
                case 20:
                    return 14;
                case 21:
                    return 13;
                case 22:
                case 23:
                case 24:
                    return 12;
                case 25:
                case 26:
                case 27:
                case 28:
                    return 11;
                case 29:
                case 30:
                case 31:
                case 32:
                    return 9;
                case 33:
                case 34:
                case 35:
                case 36:
                    return 8;
                case 37:
                case 38:
                case 39:
                    return 6;
                case 40:
                case 41:
                case 42:
                case 43:
                case 44:
                case 45:
                    return 4;
                case 46:
                case 47:
                case 48:
                case 49:
                case 50:
                case 51:
                    return 3;
                case 52:
                case 53:
                case 54:
                case 55:
                case 56:
                case 57:
                case 58:
                case 59:
                case 60:
                case 61:
                case 62:
                case 63:
                case 64:
                    return 2;
                default:
                    return 1;
            }
            break;
        case CLASS_BARD:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                case 3:
                    return 20;
                case 4:
                case 5:
                    return 19;
                case 6:
                case 7:
                case 8:
                    return 18;
                case 9:
                case 10:
                case 11:
                case 12:
                    return 17;
                case 13:
                case 14:
                case 15:
                case 16:
                    return 16;
                case 17:
                case 18:
                case 19:
                    return 15;
                case 20:
                case 21:
                    return 14;
                case 22:
                case 23:
                    return 13;
                case 24:
                case 25:
                    return 12;
                case 26:
                case 27:
                case 28:
                    return 11;
                case 29:
                case 30:
                    return 10;
                case 31:
                case 32:

                case 33:
                    return 9;
                case 34:
                case 35:
                case 36:
                    return 8;
                case 37:
                case 38:
                    return 7;
                case 39:
                case 40:
                    return 6;
                case 41:
                case 42:
                    return 5;
                case 43:
                case 44:
                case 45:
                    return 4;
                case 46:
                case 47:
                case 48:
                case 49:
                case 50:
                case 51:
                case 52:
                case 53:
                case 54:
                case 55:
                    return 3;
                case 56:
                case 57:
                case 58:
                case 59:
                    return 2;
                default:
                    return 1;
            }
        default:
            log1("SYSERR: Unknown class in thac0 chart.");
    }

    /* Will not get there unless something is wrong. */
    return 100;
}

/* Roll the 6 stats for a character... each stat is made of the sum of the
   best 3 out of 4 rolls of a 6-sided die.  Each class then decides which
   priority will be given for the best to worst stats. */
void roll_real_abils(struct char_data *ch)
{
    int i, j, k, temp;
    ubyte table[6];
    ubyte rolls[4];

    for (i = 0; i < 6; i++)
        table[i] = 0;

    for (i = 0; i < 6; i++) {

        for (j = 0; j < 4; j++)
            rolls[j] = rand_number(1, 6);

        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] - MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

        for (k = 0; k < 6; k++)
            if (table[k] < temp) {
                temp ^= table[k];
                table[k] ^= temp;
                temp ^= table[k];
            }
    }

    ch->real_abils.str_add = 0;

    switch (GET_CLASS(ch)) {
        case CLASS_MAGIC_USER:
            ch->real_abils.intel = table[0];
            ch->real_abils.wis = table[1];
            ch->real_abils.dex = table[2];
            ch->real_abils.str = table[3];
            ch->real_abils.con = table[4];
            ch->real_abils.cha = table[5];
            break;
        case CLASS_CLERIC:
            ch->real_abils.wis = table[0];
            ch->real_abils.intel = table[1];
            ch->real_abils.str = table[2];
            ch->real_abils.dex = table[3];
            ch->real_abils.con = table[4];
            ch->real_abils.cha = table[5];
            break;
        case CLASS_THIEF:
            ch->real_abils.dex = table[0];
            ch->real_abils.str = table[1];
            ch->real_abils.con = table[2];
            ch->real_abils.intel = table[3];
            ch->real_abils.wis = table[4];
            ch->real_abils.cha = table[5];
            break;
        case CLASS_WARRIOR:
            ch->real_abils.str = table[0];
            ch->real_abils.dex = table[1];
            ch->real_abils.con = table[2];
            ch->real_abils.wis = table[3];
            ch->real_abils.intel = table[4];
            ch->real_abils.cha = table[5];
            if (ch->real_abils.str == 18)
                ch->real_abils.str_add = rand_number(0, 100);
            break;
        case CLASS_DRUID: /* -- mp */
            ch->real_abils.wis = table[0];
            ch->real_abils.con = table[1];
            ch->real_abils.dex = table[2];
            ch->real_abils.intel = table[3];
            ch->real_abils.str = table[4];
            ch->real_abils.cha = table[5];
            break;
        case CLASS_BARD: /* -- ac */
            ch->real_abils.dex = table[0];
            ch->real_abils.intel = table[1];
            ch->real_abils.con = table[2];
            ch->real_abils.str = table[3];
            ch->real_abils.wis = table[4];
            ch->real_abils.cha = table[5];
            break;
        case CLASS_RANGER:
            ch->real_abils.str = table[0];
            ch->real_abils.con = table[1];
            ch->real_abils.dex = table[2];
            ch->real_abils.intel = table[3];
            ch->real_abils.wis = table[4];
            ch->real_abils.cha = table[5];
            break;
    }
    ch->aff_abils = ch->real_abils;
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
    GET_LEVEL(ch) = 1;
    GET_EXP(ch) = 1;

    set_title(ch, NULL);
    roll_real_abils(ch);

    GET_MAX_HIT(ch) = 10;
    GET_MAX_MANA(ch) = 100;
    GET_MAX_MOVE(ch) = 82;
    GET_MAX_BREATH(ch) = 15;
    advance_level(ch);

    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    GET_BREATH(ch) = GET_MAX_BREATH(ch);

    update_pos(ch);

    GET_COND(ch, THIRST) = 24;
    GET_COND(ch, HUNGER) = 24;
    GET_COND(ch, DRUNK) = 0;

    if (CONFIG_SITEOK_ALL)
        SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);

    GET_FIT(ch) = 0;
}

/* This function controls the change to maxmove, maxmana, and maxhp for each
   class every time they gain a level. */
void advance_level(struct char_data *ch)
{
    int add_hp, add_mana = 0, add_move = 0, i;

    add_hp = con_app[GET_CON(ch)].hitp;

    switch (GET_CLASS(ch)) {

        case CLASS_MAGIC_USER:
            add_hp += rand_number(3, 8);
            add_mana = rand_number(GET_LEVEL(ch), (int)(2 * GET_LEVEL(ch)));
            add_move = rand_number(0, 2);
            break;

        case CLASS_CLERIC:
            add_hp += rand_number(5, 10);
            add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
            add_move = rand_number(0, 2);
            break;

        case CLASS_THIEF:
            add_hp += rand_number(7, 13);
            add_mana = 0;
            add_move = rand_number(1, 3);
            break;

        case CLASS_WARRIOR:
            add_hp += rand_number(10, 15);
            add_mana = 0;
            add_move = rand_number(1, 3);
            break;

        case CLASS_DRUID: /* -- mp */
            add_hp += rand_number(6, 11);
            add_mana = rand_number(GET_LEVEL(ch), (int)(GET_LEVEL(ch) * 1.7));
            add_move = rand_number(1, 3);
            break;

        case CLASS_BARD:
            add_hp += rand_number(7, 12);
            add_mana = rand_number(GET_LEVEL(ch), (int)(GET_LEVEL(ch) * 1.7));
            add_move = rand_number(1, 2);
            break;

        case CLASS_RANGER:
            add_hp += rand_number(7, 13);
            add_mana = rand_number((int)(0.5 * GET_LEVEL(ch)), (int)GET_LEVEL(ch));
            add_mana = MIN(add_mana, 5);
            add_move = rand_number(2, 4);
            break;
    }

    if (GET_LEVEL(ch) > 50)
        add_hp *= 1.20;
    else if (GET_LEVEL(ch) > 75)
        add_hp *= 1.40;

    ch->points.max_hit += MAX(1, add_hp);
    ch->points.max_move += MAX(1, add_move);

    if (GET_LEVEL(ch) > 1)
        ch->points.max_mana += add_mana;

    if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
        GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);
    else
        GET_PRACTICES(ch) += MIN(2, MAX(1, wis_app[GET_WIS(ch)].bonus));

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        for (i = 0; i < 3; i++)
            GET_COND(ch, i) = (char)-1;
        SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }

    snoop_check(ch);
    save_char(ch);
}

/* This simply calculates the backstab multiplier based on a character's
   level. This used to be an array, but was changed to be a function so that
   it would be easier to add more levels to your MUD.  This doesn't really
   create a big performance hit because it's not used very often. */
int backstab_mult(int level)
{
    if (level <= 10)
        return (2); /* level 1 - 10 */
    else if (level <= 20)
        return (3); /* level 11 - 20 */
    else if (level <= 28)
        return (4); /* level 21 - 28 */
    else if (level <= 36)
        return (5); /* level 29 - 36 */
    else if (level <= 45)
        return (6); /* level 37 - 45 */
    else if (level < LVL_IMMORT)
        return (7); /* all remaining mortal levels */
    else if (level <= LVL_GRIMM)
        return (10); /* immortals */
    else
        return (20); /* gods */
}

/* invalid_class is used by handler.c to determine if a piece of equipment is
   usable by a particular class, based on the ITEM_ANTI_{class} bitvectors. */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
    if (OBJ_FLAGGED(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_BARD) && IS_BARD(ch))
        return TRUE;

    return FALSE;
}

/* This is the exp given to implementors -- it must always be greater than the
   exp required for immortality, plus at least 20,000 or so. */
#define EXP_MAX 1000000000

/* Function to return the exp required for each class/level */
int level_exp(int chclass, int level)
{
    if (level > LVL_IMPL || level < 0) {
        log1("SYSERR: Requesting exp for invalid level %d!", level);
        return 0;
    }

    /* Gods have exp close to EXP_MAX.  This statement should never have to
       changed, regardless of how many mortal or immortal levels exist. */
    if (level > LVL_GRIMM)
        return (EXP_MAX - ((LVL_IMPL - level) * 50000000));

    return (exp_table[level][chclass]);

    /* This statement should never be reached if the exp tables in this
       function are set up properly.  If you see exp of 123456 then the tables
       above are incomplete. */
    log1("SYSERR: XP tables not set up correctly in class.c!");
    return 123456;
}
/*
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level)
{
    if (level <= 0 || level > LVL_IMPL)
        return "o Homem";
    if (level == LVL_IMPL)
        return "o Deus Supremo";
    if (level == LVL_CREATOR)
        return "o Deus Criador";

    switch (chclass) {

        case CLASS_MAGIC_USER:
            switch (level) {
                case 1:
                    return "o Aprendiz de Magica";
                case 2:
                    return "o Aprendiz de Magica";
                case 3:
                    return "o Aprendiz de Magica";
                case 4:
                    return "o Aprendiz de Magica";
                case 5:
                    return "o Aprendiz de Magica";
                case 6:
                    return "o Estudante de Magia";
                case 7:
                    return "o Estudante de Magia";
                case 8:
                    return "o Estudante de Magia";
                case 9:
                    return "o Cursado em Magia";
                case 10:
                    return "o Cursado em Magia";
                case 11:
                    return "o Cursado em Magia";
                case 12:
                    return "o Examinador em Magias";
                case 13:
                    return "o Examinador em Magias";
                case 14:
                    return "o Examinador em Magias";
                case 15:
                    return "o Medium em Magia";
                case 16:
                    return "o Medium em Magia";
                case 17:
                    return "o Medium em Magia";
                case 18:
                    return "o Escritor de Magias";
                case 19:
                    return "o Escritor de Magias";
                case 20:
                    return "o Escritor de Magias";
                case 21:
                    return "o Profeta";
                case 22:
                    return "o Profeta";
                case 23:
                    return "o Profeta";
                case 24:
                    return "o Sabio";
                case 25:
                    return "o Sabio";
                case 26:
                    return "o Sabio";
                case 27:
                    return "o Ilusionista";
                case 28:
                    return "o Ilusionista";
                case 29:
                    return "o Ilusionista";
                case 30:
                    return "o Abjurador";
                case 31:
                    return "o Abjurador";
                case 32:
                    return "o Abjurador";
                case 33:
                    return "o Invocador";
                case 34:
                    return "o Invocador";
                case 35:
                    return "o Invocador";
                case 36:
                    return "o Encantador";
                case 37:
                    return "o Encantador";
                case 38:
                    return "o Encantador";
                case 39:
                    return "o Praticante";
                case 40:
                    return "o Praticante";
                case 41:
                    return "o Praticante";
                case 42:
                    return "o Magico";
                case 43:
                    return "o Magico";
                case 44:
                    return "o Magico";
                case 45:
                    return "o Criador";
                case 46:
                    return "o Criador";
                case 47:
                    return "o Criador";
                case 48:
                    return "o Grande Sabio";
                case 49:
                    return "o Grande Sabio";
                case 50:
                    return "o Grande Sabio";
                case 51:
                    return "o Mago";
                case 52:
                    return "o Mago";
                case 53:
                    return "o Mago";
                case 54:
                    return "o Bruxo";
                case 55:
                    return "o Bruxo";
                case 56:
                    return "o Bruxo";
                case 57:
                    return "o Grande Bruxo";
                case 58:
                    return "o Grande Bruxo";
                case 59:
                    return "o Grande Bruxo";
                case 60:
                    return "o Feiticeiro";
                case 61:
                    return "o Feiticeiro";
                case 62:
                    return "o Feiticeiro";
                case 63:
                    return "o Grande Feiticeiro";
                case 64:
                    return "o Grande Feiticeiro";
                case 65:
                    return "o Grande Feiticeiro";
                case 66:
                    return "o Necromante";
                case 67:
                    return "o Necromante";
                case 68:
                    return "o Necromante";
                case 69:
                    return "o Estudante do Oculto";
                case 70:
                    return "o Estudante do Oculto";
                case 71:
                    return "o Estudante do Oculto";
                case 72:
                    return "o Discipulo do Misterio";
                case 73:
                    return "o Discipulo do Misterio";
                case 74:
                    return "o Discipulo do Misterio";
                case 75:
                    return "o Elemental Menor";
                case 76:
                    return "o Elemental Menor";
                case 77:
                    return "o Elemental Menor";
                case 78:
                    return "o Grande Elemental";
                case 79:
                    return "o Grande Elemental";
                case 80:
                    return "o Grande Elemental";
                case 81:
                    return "o Habil das Magicas";
                case 82:
                    return "o Habil das Magicas";
                case 83:
                    return "o Habil das Magicas";
                case 84:
                    return "o Xamã";
                case 85:
                    return "o Xamã";
                case 86:
                    return "o Xamã";
                case 87:
                    return "o Guardiao de Talismãs";
                case 88:
                    return "o Guardiao de Talismãs";
                case 89:
                    return "o Guardiao de Talismãs";
                case 90:
                    return "o Arque-magico";
                case 91:
                    return "o Arque-magico";
                case 92:
                    return "o Arque-magico";
                case 93:
                    return "o Arque-magico";
                case 94:
                    return "o Arque-magico";
                case 95:
                    return "o Arque-magico";
                case 96:
                    return "o Arque-magico";
                case 97:
                    return "o Arque-magico";
                case 98:
                    return "o Arque-magico";
                case 99:
                    return "o Arque-magico";
                case 100:
                    return "o Arque-magico";
                case LVL_IMMORT:
                    return "o Feiticeiro Imortal";
                case LVL_IMM2:
                    return "o Feiticeiro Imortal";
                case LVL_IMM3:
                    return "o Feiticeiro Imortal";
                case LVL_ANCIENT:
                    return "o Feiticeiro Imortal";
                case LVL_GRIMM:
                    return "o Feiticeiro Imortal";
                case LVL_DEMIGOD:
                    return "o Avatar da Magia";
                case LVL_GOD:
                    return "o Avatar da Magia";
                case LVL_GRGOD:
                    return "o Deus da Magia";
            }
            break;

        case CLASS_CLERIC:
            switch (level) {
                case 1:
                    return "o Crente";
                case 2:
                    return "o Crente";
                case 3:
                    return "o Crente";
                case 4:
                    return "o Crente";
                case 5:
                    return "o Crente";
                case 6:
                    return "o Crente";
                case 7:
                    return "o Crente";
                case 8:
                    return "o Crente";
                case 9:
                    return "o Servente";
                case 10:
                    return "o Servente";
                case 11:
                    return "o Servente";
                case 12:
                    return "o Servente";
                case 13:
                    return "o Servente";
                case 14:
                    return "o Assistente";
                case 15:
                    return "o Assistente";
                case 16:
                    return "o Assistente";
                case 17:
                    return "o Assistente";
                case 18:
                    return "o Assistente";
                case 19:
                    return "o Noviço";
                case 20:
                    return "o Noviço";
                case 21:
                    return "o Noviço";
                case 22:
                    return "o Noviço";
                case 23:
                    return "o Noviço";
                case 24:
                    return "o Missionario";
                case 25:
                    return "o Missionario";
                case 26:
                    return "o Missionario";
                case 27:
                    return "o Missionario";
                case 28:
                    return "o Missionario";
                case 29:
                    return "o Praticante";
                case 30:
                    return "o Praticante";
                case 31:
                    return "o Praticante";
                case 32:
                    return "o Praticante";
                case 33:
                    return "o Praticante";
                case 34:
                    return "o Diacono";
                case 35:
                    return "o Diacono";
                case 36:
                    return "o Diacono";
                case 37:
                    return "o Diacono";
                case 38:
                    return "o Diacono";
                case 39:
                    return "o Vigario";
                case 40:
                    return "o Vigario";
                case 41:
                    return "o Vigario";
                case 42:
                    return "o Vigario";
                case 43:
                    return "o Vigario";
                case 44:
                    return "o Sacerdote";
                case 45:
                    return "o Sacerdote";
                case 46:
                    return "o Sacerdote";
                case 47:
                    return "o Sacerdote";
                case 48:
                    return "o Sacerdote";
                case 49:
                    return "o Ministro";
                case 50:
                    return "o Ministro";
                case 51:
                    return "o Ministro";
                case 52:
                    return "o Ministro";
                case 53:
                    return "o Ministro";
                case 54:
                    return "o Canon";
                case 55:
                    return "o Canon";
                case 56:
                    return "o Canon";
                case 57:
                    return "o Canon";
                case 58:
                    return "o Canon";
                case 59:
                    return "o Paroco";
                case 60:
                    return "o Paroco";
                case 61:
                    return "o Paroco";
                case 62:
                    return "o Paroco";
                case 63:
                    return "o Paroco";
                case 64:
                    return "o Frade";
                case 65:
                    return "o Frade";
                case 66:
                    return "o Frade";
                case 67:
                    return "o Frade";
                case 68:
                    return "o Frade";
                case 69:
                    return "o Curador";
                case 70:
                    return "o Curador";
                case 71:
                    return "o Curador";
                case 72:
                    return "o Curador";
                case 73:
                    return "o Curador";
                case 74:
                    return "o Capelao";
                case 75:
                    return "o Capelao";
                case 76:
                    return "o Capelao";
                case 77:
                    return "o Capelao";
                case 78:
                    return "o Capelao";
                case 79:
                    return "o Expositor";
                case 80:
                    return "o Expositor";
                case 81:
                    return "o Expositor";
                case 82:
                    return "o Expositor";
                case 83:
                    return "o Expositor";
                case 84:
                    return "o Bispo";
                case 85:
                    return "o Bispo";
                case 86:
                    return "o Bispo";
                case 87:
                    return "o Bispo";
                case 88:
                    return "o Bispo";
                case 89:
                    return "o Arce-Bispo";
                case 90:
                    return "o Arce-Bispo";
                case 91:
                    return "o Arce-Bispo";
                case 92:
                    return "o Arce-Bispo";
                case 93:
                    return "o Arce-Bispo";
                case 94:
                    return "o Patriarca";
                case 95:
                    return "o Patriarca";
                case 96:
                    return "o Patriarca";
                case 97:
                    return "o Patriarca";
                case 98:
                    return "o Patriarca";
                case 99:
                    return "o Patriarca";
                case 100:
                    return "o Patriarca";
                case LVL_IMMORT:
                    return "o Cardinal Imortal";
                case LVL_IMM2:
                    return "o Cardinal Imortal";
                case LVL_IMM3:
                    return "o Cardinal Imortal";
                case LVL_ANCIENT:
                    return "o Cardinal Imortal";
                case LVL_GRIMM:
                    return "o Cardinal Imortal";
                case LVL_DEMIGOD:
                    return "o Inquisitor";
                case LVL_GOD:
                    return "o Inquisitor";
                case LVL_GRGOD:
                    return "o Deus do bom e do mau";
            }
            break;

        case CLASS_THIEF:
            switch (level) {
                case 1:
                    return "o Furtador";
                case 2:
                    return "o Furtador";
                case 3:
                    return "o Furtador";
                case 4:
                    return "o Furtador";
                case 5:
                    return "o Furtador";
                case 6:
                    return "o Pequeno Ladrao";
                case 7:
                    return "o Pequeno Ladrao";
                case 8:
                    return "o Pequeno Ladrao";
                case 9:
                    return "o Pequeno Ladrao";
                case 10:
                    return "o Pequeno Ladrao";
                case 11:
                    return "o Filador";
                case 12:
                    return "o Filador";
                case 13:
                    return "o Filador";
                case 14:
                    return "o Filador";
                case 15:
                    return "o Filador";
                case 16:
                    return "o Batedor de Carteiras";
                case 17:
                    return "o Batedor de Carteiras";
                case 18:
                    return "o Batedor de Carteiras";
                case 19:
                    return "o Batedor de Carteiras";
                case 20:
                    return "o Batedor de Carteiras";
                case 21:
                    return "o Escondido";
                case 22:
                    return "o Escondido";
                case 23:
                    return "o Escondido";
                case 24:
                    return "o Escondido";
                case 25:
                    return "o Escondido";
                case 26:
                    return "o Arrancador";
                case 27:
                    return "o Arrancador";
                case 28:
                    return "o Arrancador";
                case 29:
                    return "o Arrancador";
                case 30:
                    return "o Arrancador";
                case 31:
                    return "o Ladrao de Bolsas";
                case 32:
                    return "o Ladrao de Bolsas";
                case 33:
                    return "o Ladrao de Bolsas";
                case 34:
                    return "o Ladrao de Bolsas";
                case 35:
                    return "o Ladrao de Bolsas";
                case 36:
                    return "o Apanhador";
                case 37:
                    return "o Apanhador";
                case 38:
                    return "o Apanhador";
                case 39:
                    return "o Apanhador";
                case 40:
                    return "o Apanhador";
                case 41:
                    return "o Vigarista";
                case 42:
                    return "o Vigarista";
                case 43:
                    return "o Vigarista";
                case 44:
                    return "o Vigarista";
                case 45:
                    return "o Vigarista";
                case 46:
                    return "o Mentiroso";
                case 47:
                    return "o Mentiroso";
                case 48:
                    return "o Mentiroso";
                case 49:
                    return "o Mentiroso";
                case 50:
                    return "o Mentiroso";
                case 51:
                    return "o Roubador";
                case 52:
                    return "o Roubador";
                case 53:
                    return "o Roubador";
                case 54:
                    return "o Roubador";
                case 55:
                    return "o Roubador";
                case 56:
                    return "o Esperto";
                case 57:
                    return "o Esperto";
                case 58:
                    return "o Esperto";
                case 59:
                    return "o Esperto";
                case 60:
                    return "o Esperto";
                case 61:
                    return "o Salteador";
                case 62:
                    return "o Salteador";
                case 63:
                    return "o Salteador";
                case 64:
                    return "o Salteador";
                case 65:
                    return "o Salteador";
                case 66:
                    return "o Arrombador";
                case 67:
                    return "o Arrombador";
                case 68:
                    return "o Arrombador";
                case 69:
                    return "o Arrombador";
                case 70:
                    return "o Arrombador";
                case 71:
                    return "o Ladrao";
                case 72:
                    return "o Ladrao";
                case 73:
                    return "o Ladrao";
                case 74:
                    return "o Ladrao";
                case 75:
                    return "o Ladrao";
                case 76:
                    return "o Apunhalador";
                case 77:
                    return "o Apunhalador";
                case 78:
                    return "o Apunhalador";
                case 79:
                    return "o Apunhalador";
                case 80:
                    return "o Apunhalador";
                case 81:
                    return "o Esfaqueador";
                case 82:
                    return "o Esfaqueador";
                case 83:
                    return "o Esfaqueador";
                case 84:
                    return "o Esfaqueador";
                case 85:
                    return "o Esfaqueador";
                case 86:
                    return "o Matador";
                case 87:
                    return "o Matador";
                case 88:
                    return "o Matador";
                case 89:
                    return "o Matador";
                case 90:
                    return "o Matador";
                case 91:
                    return "o Bandido";
                case 92:
                    return "o Bandido";
                case 93:
                    return "o Bandido";
                case 94:
                    return "o Bandido";
                case 95:
                    return "o Bandido";
                case 96:
                    return "o Corta-Gargantas";
                case 97:
                    return "o Corta-Gargantas";
                case 98:
                    return "o Corta-Gargantas";
                case 99:
                    return "o Corta-Gargantas";
                case 100:
                    return "o Corta-Gargantas";
                case LVL_IMMORT:
                    return "o Assassino Imortal";
                case LVL_IMM2:
                    return "o Assassino Imortal";
                case LVL_IMM3:
                    return "o Assassino Imortal";
                case LVL_ANCIENT:
                    return "o Assassino Imortal";
                case LVL_GRIMM:
                    return "o Assassino Imortal";
                case LVL_DEMIGOD:
                    return "o Deus dos Ladroes";
                case LVL_GOD:
                    return "o Guardião dos Ladroes";
                case LVL_GRGOD:
                    return "o Legislador dos Ladroes";
            }
            break;

        case CLASS_WARRIOR:
            switch (level) {
                case 1:
                    return "o Novato em Espadas";
                case 2:
                    return "o Novato em Espadas";
                case 3:
                    return "o Novato em Espadas";
                case 4:
                    return "o Novato em Espadas";
                case 5:
                    return "o Novato em Espadas";
                case 6:
                    return "o Recruta";
                case 7:
                    return "o Recruta";
                case 8:
                    return "o Recruta";
                case 9:
                    return "o Recruta";
                case 10:
                    return "o Recruta";
                case 11:
                    return "o Sentinela";
                case 12:
                    return "o Sentinela";
                case 13:
                    return "o Sentinela";
                case 14:
                    return "o Sentinela";
                case 15:
                    return "o Sentinela";
                case 16:
                    return "o Lutador";
                case 17:
                    return "o Lutador";
                case 18:
                    return "o Lutador";
                case 19:
                    return "o Lutador";
                case 20:
                    return "o Lutador";
                case 21:
                    return "o Soldado";
                case 22:
                    return "o Soldado";
                case 23:
                    return "o Soldado";
                case 24:
                    return "o Soldado";
                case 25:
                    return "o Soldado";
                case 26:
                    return "o Guerreiro";
                case 27:
                    return "o Guerreiro";
                case 28:
                    return "o Guerreiro";
                case 29:
                    return "o Guerreiro";
                case 30:
                    return "o Guerreiro";
                case 31:
                    return "o Veterano";
                case 32:
                    return "o Veterano";
                case 33:
                    return "o Veterano";
                case 34:
                    return "o Veterano";
                case 35:
                    return "o Veterano";
                case 36:
                    return "o Esgrimista";
                case 37:
                    return "o Esgrimista";
                case 38:
                    return "o Esgrimista";
                case 39:
                    return "o Esgrimista";
                case 40:
                    return "o Esgrimista";
                case 41:
                    return "o Defendente";
                case 42:
                    return "o Defendente";
                case 43:
                    return "o Defendente";
                case 44:
                    return "o Defendente";
                case 45:
                    return "o Defendente";
                case 46:
                    return "o Combatente";
                case 47:
                    return "o Combatente";
                case 48:
                    return "o Combatente";
                case 49:
                    return "o Combatente";
                case 50:
                    return "o Combatente";
                case 51:
                    return "o Valente";
                case 52:
                    return "o Valente";
                case 53:
                    return "o Valente";
                case 54:
                    return "o Valente";
                case 55:
                    return "o Valente";
                case 56:
                    return "o Heroi";
                case 57:
                    return "o Heroi";
                case 58:
                    return "o Heroi";
                case 59:
                    return "o Heroi";
                case 60:
                    return "o Heroi";
                case 61:
                    return "o Aventureiro";
                case 62:
                    return "o Aventureiro";
                case 63:
                    return "o Aventureiro";
                case 64:
                    return "o Aventureiro";
                case 65:
                    return "o Aventureiro";
                case 66:
                    return "o Mercenario";
                case 67:
                    return "o Mercenario";
                case 68:
                    return "o Mercenario";
                case 69:
                    return "o Mercenario";
                case 70:
                    return "o Mercenario";
                case 71:
                    return "o Mestre em Espada";
                case 72:
                    return "o Mestre em Espada";
                case 73:
                    return "o Mestre em Espada";
                case 74:
                    return "o Mestre em Espada";
                case 75:
                    return "o Mestre em Espada";
                case 76:
                    return "o Tenente";
                case 77:
                    return "o Tenente";
                case 78:
                    return "o Tenente";
                case 79:
                    return "o Tenente";
                case 80:
                    return "o Tenente";
                case 81:
                    return "o Campeao";
                case 82:
                    return "o Campeao";
                case 83:
                    return "o Campeao";
                case 84:
                    return "o Campeao";
                case 85:
                    return "o Campeao";
                case 86:
                    return "o Dragoon";
                case 87:
                    return "o Dragoon";
                case 88:
                    return "o Dragoon";
                case 89:
                    return "o Dragoon";
                case 90:
                    return "o Dragoon";
                case 91:
                    return "o Cavaleiro";
                case 92:
                    return "o Cavaleiro";
                case 93:
                    return "o Cavaleiro";
                case 94:
                    return "o Cavaleiro";
                case 95:
                    return "o Cavaleiro";
                case 96:
                    return "o Grande Cavaleiro";
                case 97:
                    return "o Grande Cavaleiro";
                case 98:
                    return "o Grande Cavaleiro";
                case 99:
                    return "o Grande Cavaleiro";
                case 100:
                    return "o Grande Cavaleiro";
                case LVL_IMMORT:
                    return "o Guerreiro Imortal";
                case LVL_IMM2:
                    return "o Guerreiro Imortal";
                case LVL_IMM3:
                    return "o Guerreiro Imortal";
                case LVL_ANCIENT:
                    return "o Guerreiro Imortal";
                case LVL_GRIMM:
                    return "o Guerreiro Imortal";
                case LVL_DEMIGOD:
                    return "o Extirpador";
                case LVL_GOD:
                    return "o Extirpador";
                case LVL_GRGOD:
                    return "o Deus da Guerra";
            }
            break;

        case CLASS_DRUID: /* -- mp */
            switch (level) {
                case 1:
                    return "o Aspirante";
                case 2:
                    return "o Aspirante";
                case 3:
                    return "o Aspirante";
                case 4:
                    return "o Aspirante";
                case 5:
                    return "o Aspirante";
                case 6:
                    return "o Ovate";
                case 7:
                    return "o Ovate";
                case 8:
                    return "o Ovate";
                case 9:
                    return "o Ovate";
                case 10:
                    return "o Ovate";
                case 11:
                    return "o Iniciado no Primeiro Círculo";
                case 12:
                    return "o Iniciado no Primeiro Círculo";
                case 13:
                    return "o Iniciado no Primeiro Círculo";
                case 14:
                    return "o Iniciado no Primeiro Círculo";
                case 15:
                    return "o Iniciado no Primeiro Círculo";
                case 16:
                    return "o Iniciado no Segundo Círculo";
                case 17:
                    return "o Iniciado no Segundo Círculo";
                case 18:
                    return "o Iniciado no Segundo Círculo";
                case 19:
                    return "o Iniciado no Segundo Círculo";
                case 20:
                    return "o Iniciado no Segundo Círculo";
                case 21:
                    return "o Iniciado no Terceiro Círculo";
                case 22:
                    return "o Iniciado no Terceiro Círculo";
                case 23:
                    return "o Iniciado no Terceiro Círculo";
                case 24:
                    return "o Iniciado no Terceiro Círculo";
                case 25:
                    return "o Iniciado no Terceiro Círculo";
                case 26:
                    return "o Iniciado no Quarto Círculo";
                case 27:
                    return "o Iniciado no Quarto Círculo";
                case 28:
                    return "o Iniciado no Quarto Círculo";
                case 29:
                    return "o Iniciado no Quarto Círculo";
                case 30:
                    return "o Iniciado no Quarto Círculo";
                case 31:
                    return "o Iniciado no Quinto Círculo";
                case 32:
                    return "o Iniciado no Quinto Círculo";
                case 33:
                    return "o Iniciado no Quinto Círculo";
                case 34:
                    return "o Iniciado no Quinto Círculo";
                case 35:
                    return "o Iniciado no Quinto Círculo";
                case 36:
                    return "o Iniciado no Sexto Círculo";
                case 37:
                    return "o Iniciado no Sexto Círculo";
                case 38:
                    return "o Iniciado no Sexto Círculo";
                case 39:
                    return "o Iniciado no Sexto Círculo";
                case 40:
                    return "o Iniciado no Sexto Círculo";
                case 41:
                    return "o Iniciado no Sétimo Círculo";
                case 42:
                    return "o Iniciado no Sétimo Círculo";
                case 43:
                    return "o Iniciado no Sétimo Círculo";
                case 44:
                    return "o Iniciado no Sétimo Círculo";
                case 45:
                    return "o Iniciado no Sétimo Círculo";
                case 46:
                    return "o Iniciado no Oitavo Círculo";
                case 47:
                    return "o Iniciado no Oitavo Círculo";
                case 48:
                    return "o Iniciado no Oitavo Círculo";
                case 49:
                    return "o Iniciado no Oitavo Círculo";
                case 50:
                    return "o Iniciado no Oitavo Círculo";
                case 51:
                    return "o Iniciado no Nono Círculo";
                case 52:
                    return "o Iniciado no Nono Círculo";
                case 53:
                    return "o Iniciado no Nono Círculo";
                case 54:
                    return "o Iniciado no Nono Círculo";
                case 55:
                    return "o Iniciado no Nono Círculo";
                case 56:
                    return "o Druida";
                case 57:
                    return "o Druida";
                case 58:
                    return "o Druida";
                case 59:
                    return "o Druida";
                case 60:
                    return "o Druida";
                case 61:
                    return "o Arque-Druida";
                case 62:
                    return "o Arque-Druida";
                case 63:
                    return "o Arque-Druida";
                case 64:
                    return "o Arque-Druida";
                case 65:
                    return "o Arque-Druida";
                case 66:
                    return "o Grão-Druida";
                case 67:
                    return "o Grão-Druida";
                case 68:
                    return "o Grão-Druida";
                case 69:
                    return "o Grão-Druida";
                case 70:
                    return "o Grão-Druida";
                case 71:
                    return "o Magno Druida";
                case 72:
                    return "o Magno Druida";
                case 73:
                    return "o Magno Druida";
                case 74:
                    return "o Magno Druida";
                case 75:
                    return "o Magno Druida";
                case 76:
                    return "o Druida Hierofante";
                case 77:
                    return "o Druida Hierofante";
                case 78:
                    return "o Druida Hierofante";
                case 79:
                    return "o Druida Hierofante";
                case 80:
                    return "o Druida Hierofante";
                case 81:
                    return "o Hierofante Iniciado";
                case 82:
                    return "o Hierofante Iniciado";
                case 83:
                    return "o Hierofante Iniciado";
                case 84:
                    return "o Hierofante Iniciado";
                case 85:
                    return "o Hierofante Iniciado";
                case 86:
                    return "o Hierofante Adepto";
                case 87:
                    return "o Hierofante Adepto";
                case 88:
                    return "o Hierofante Adepto";
                case 89:
                    return "o Hierofante Adepto";
                case 90:
                    return "o Hierofante Adepto";
                case 91:
                    return "o Mestre Hierofante";
                case 92:
                    return "o Mestre Hierofante";
                case 93:
                    return "o Mestre Hierofante";
                case 94:
                    return "o Mestre Hierofante";
                case 95:
                    return "o Mestre Hierofante";
                case 96:
                    return "o Sublime Hierofante";
                case 97:
                    return "o Sublime Hierofante";
                case 98:
                    return "o Sublime Hierofante";
                case 99:
                    return "o Sublime Hierofante";
                case 100:
                    return "o Sublime Hierofante";
                case LVL_IMMORT:
                    return "o Místico Hierofante";
                case LVL_IMM2:
                    return "o Místico Hierofante";
                case LVL_IMM3:
                    return "o Místico Hierofante";
                case LVL_ANCIENT:
                    return "o Místico Hierofante";
                case LVL_GRIMM:
                    return "o Místico Hierofante";
                case LVL_DEMIGOD:
                    return "o Hierofante Arcano";
                case LVL_GOD:
                    return "o Hierofante Arcano";
                case LVL_GRGOD:
                    return "Hierofante da Cabala";
            }
            break;

        case CLASS_BARD: /* ac -- march 2003 */
            switch (level) {
                case 1:
                    return "o Aprendiz de Malabarista";
                case 2:
                    return "o Aprendiz de Malabarista";
                case 3:
                    return "o Aprendiz de Malabarista";
                case 4:
                    return "o Aprendiz de Malabarista";
                case 5:
                    return "o Aprendiz de Malabarista";
                case 6:
                    return "o Histrião";
                case 7:
                    return "o Histrião";
                case 8:
                    return "o Histrião";
                case 9:
                    return "o Histrião";
                case 10:
                    return "o Histrião";
                case 11:
                    return "o Truão";
                case 12:
                    return "o Truão";
                case 13:
                    return "o Truão";
                case 14:
                    return "o Truão";
                case 15:
                    return "o Truão";
                case 16:
                    return "o Estudante de Música";
                case 17:
                    return "o Estudante de Música";
                case 18:
                    return "o Estudante de Música";
                case 19:
                    return "o Estudante de Música";
                case 20:
                    return "o Estudante de Música";
                case 21:
                    return "o Cantor";
                case 22:
                    return "o Cantor";
                case 23:
                    return "o Cantor";
                case 24:
                    return "o Cantor";
                case 25:
                    return "o Cantor";
                case 26:
                    return "o Musicista";
                case 27:
                    return "o Musicista";
                case 28:
                    return "o Musicista";
                case 29:
                    return "o Musicista";
                case 30:
                    return "o Musicista";
                case 31:
                    return "o Literato";
                case 32:
                    return "o Literato";
                case 33:
                    return "o Literato";
                case 34:
                    return "o Literato";
                case 35:
                    return "o Literato";
                case 36:
                    return "o Escritor";
                case 37:
                    return "o Escritor";
                case 38:
                    return "o Escritor";
                case 39:
                    return "o Escritor";
                case 40:
                    return "o Escritor";
                case 41:
                    return "o Poeta";
                case 42:
                    return "o Poeta";
                case 43:
                    return "o Poeta";
                case 44:
                    return "o Poeta";
                case 45:
                    return "o Poeta";
                case 46:
                    return "o Contador de Histórias";
                case 47:
                    return "o Contador de Histórias";
                case 48:
                    return "o Contador de Histórias";
                case 49:
                    return "o Contador de Histórias";
                case 50:
                    return "o Contador de Histórias";
                case 51:
                    return "o Instrumentista";
                case 52:
                    return "o Instrumentista";
                case 53:
                    return "o Instrumentista";
                case 54:
                    return "o Instrumentista";
                case 55:
                    return "o Instrumentista";
                case 56:
                    return "o Virtuose";
                case 57:
                    return "o Virtuose";
                case 58:
                    return "o Virtuose";
                case 59:
                    return "o Virtuose";
                case 60:
                    return "o Virtuose";
                case 61:
                    return "o Jogral";
                case 62:
                    return "o Jogral";
                case 63:
                    return "o Jogral";
                case 64:
                    return "o Jogral";
                case 65:
                    return "o Jogral";
                case 66:
                    return "o Segrel";
                case 67:
                    return "o Segrel";
                case 68:
                    return "o Segrel";
                case 69:
                    return "o Segrel";
                case 70:
                    return "o Segrel";
                case 71:
                    return "o Menestrel";
                case 72:
                    return "o Menestrel";
                case 73:
                    return "o Menestrel";
                case 74:
                    return "o Menestrel";
                case 75:
                    return "o Menestrel";
                case 76:
                    return "o Bardo";
                case 77:
                    return "o Bardo";
                case 78:
                    return "o Bardo";
                case 79:
                    return "o Bardo";
                case 80:
                    return "o Bardo";
                case 81:
                    return "o Bardo Nobre";
                case 82:
                    return "o Bardo Nobre";
                case 83:
                    return "o Bardo Nobre";
                case 84:
                    return "o Bardo Nobre";
                case 85:
                    return "o Bardo Nobre";
                case 86:
                    return "o Mestre na Música";
                case 87:
                    return "o Mestre na Música";
                case 88:
                    return "o Mestre na Música";
                case 89:
                    return "o Mestre na Música";
                case 90:
                    return "o Mestre na Música";
                case 91:
                    return "o Trovador";
                case 92:
                    return "o Trovador";
                case 93:
                    return "o Trovador";
                case 94:
                    return "o Trovador";
                case 95:
                    return "o Trovador";
                case 96:
                    return "o Mestre da Trova";
                case 97:
                    return "o Mestre da Trova";
                case 98:
                    return "o Mestre da Trova";
                case 99:
                    return "o Mestre da Trova";
                case 100:
                    return "o Mestre da Trova";
                case LVL_IMMORT:
                    return "o Trovador Celestial";
                case LVL_IMM2:
                    return "o Trovador Celestial";
                case LVL_IMM3:
                    return "o Trovador Celestial";
                case LVL_ANCIENT:
                    return "o Trovador Celestial";
                case LVL_GRIMM:
                    return "o Trovador Celestial";
                case LVL_DEMIGOD:
                    return "o Deus da Música e da Poesia";
                case LVL_GOD:
                    return "a Harmonia Divina";
                case LVL_GRGOD:
                    return "o Regente da Vibração Universal";
            }
            break;

        case CLASS_RANGER:
            switch (level) {
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    return "o Pintinho";
                case 6:
                case 7:
                case 8:
                    return "o Patatenra";
                case 9:
                case 10:
                case 11:
                    return "o Filhote";
                case 12:
                case 13:
                case 14:
                    return "o Raposinha";
                case 15:
                case 16:
                case 17:
                    return "o Lobinho";
                case 18:
                case 19:
                case 20:
                    return "o Ursinho";
                case 21:
                case 22:
                case 23:
                    return "o Tigrinho";
                case 24:
                case 25:
                case 26:
                    return "o Leãozinho";
                case 27:
                case 28:
                case 29:
                    return "o Filhote de Falcão";
                case 30:
                case 31:
                case 32:
                    return "o Filhote de Águia";
                case 33:
                case 34:
                case 35:
                    return "o Furão";
                case 36:
                case 37:
                case 38:
                    return "o Raposa";
                case 39:
                case 40:
                case 41:
                    return "o Lince";
                case 42:
                case 43:
                case 44:
                    return "o Lobo";
                case 45:
                case 46:
                case 47:
                    return "o Urso";
                case 48:
                case 49:
                case 50:
                    return "o Tigre";
                case 51:
                case 52:
                case 53:
                    return "o Leão";
                case 54:
                case 55:
                case 56:
                    return "o Falcão";
                case 57:
                case 58:
                case 59:
                    return "o Gavião";
                case 60:
                case 61:
                case 62:
                    return "o Águia";
                case 63:
                case 64:
                case 65:
                    return "o Escoteiro";
                case 66:
                case 67:
                case 68:
                    return "o Lenhador";
                case 69:
                case 70:
                case 71:
                    return "o Encontrador de Trilhas";
                case 72:
                case 73:
                case 74:
                    return "o Fazedor de Trilhas";
                case 75:
                case 76:
                case 77:
                    return "o Montanhês";
                case 78:
                case 79:
                case 80:
                    return "o Ranger";
                case 81:
                case 82:
                case 83:
                    return "o Caçador";
                case 84:
                case 85:
                case 86:
                    return "o Alto Ranger";
                case 87:
                case 88:
                case 89:
                    return "o Grande Senhor Ranger";
                case 90:
                case 91:
                case 92:
                case 93:
                case 94:
                case 95:
                case 96:
                case 97:
                case 98:
                case 99:
                case 100:
                    return "o Grande Senhor Caçador";
                case LVL_IMMORT:
                    return "o Caçador Imortal ";
                case LVL_GOD:
                    return "o Patrono da Caça";
                case LVL_GRGOD:
                    return "O Deus das Florestas";
                default:
                    return "o Viajante";
            }
    }

    /* Default title for classes which do not have titles defined */
    return "o sem-classe";
}
/*
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level)
{
    if (level <= 0 || level > LVL_IMPL)
        return "a Mulher";
    if (level == LVL_IMPL)
        return "a Deusa Suprema";
    if (level == LVL_CREATOR)
        return "a Deusa Criadora";

    switch (chclass) {

        case CLASS_MAGIC_USER:
            switch (level) {
                case 1:
                    return "a Aprendiz de Magica";
                case 2:
                    return "a Aprendiz de Magica";
                case 3:
                    return "a Aprendiz de Magica";
                case 4:
                    return "a Aprendiz de Magica";
                case 5:
                    return "a Aprendiz de Magica";
                case 6:
                    return "a Estudante de Magia";
                case 7:
                    return "a Estudante de Magia";
                case 8:
                    return "a Estudante de Magia";
                case 9:
                    return "a Cursada em Magia";
                case 10:
                    return "a Cursada em Magia";
                case 11:
                    return "a Cursada em Magia";
                case 12:
                    return "a Examinadora em Magias";
                case 13:
                    return "a Examinadora em Magias";
                case 14:
                    return "a Examinadora em Magias";
                case 15:
                    return "a Medium em Magia";
                case 16:
                    return "a Medium em Magia";
                case 17:
                    return "a Medium em Magia";
                case 18:
                    return "a Escritora de Magias";
                case 19:
                    return "a Escritora de Magias";
                case 20:
                    return "a Escritora de Magias";
                case 21:
                    return "a Profeta";
                case 22:
                    return "a Profeta";
                case 23:
                    return "a Profeta";
                case 24:
                    return "a Sabia";
                case 25:
                    return "a Sabia";
                case 26:
                    return "a Sabia";
                case 27:
                    return "a Ilusionista";
                case 28:
                    return "a Ilusionista";
                case 29:
                    return "a Ilusionista";
                case 30:
                    return "a Abjuradora";
                case 31:
                    return "a Abjuradora";
                case 32:
                    return "a Abjuradora";
                case 33:
                    return "a Invocadora";
                case 34:
                    return "a Invocadora";
                case 35:
                    return "a Invocadora";
                case 36:
                    return "a Encantadora";
                case 37:
                    return "a Encantadora";
                case 38:
                    return "a Encantadora";
                case 39:
                    return "a Praticante";
                case 40:
                    return "a Praticante";
                case 41:
                    return "a Praticante";
                case 42:
                    return "a Magica";
                case 43:
                    return "a Magica";
                case 44:
                    return "a Magica";
                case 45:
                    return "a Criadora";
                case 46:
                    return "a Criadora";
                case 47:
                    return "a Criadora";
                case 48:
                    return "a Grande Sabia";
                case 49:
                    return "a Grande Sabia";
                case 50:
                    return "a Grande Sabia";
                case 51:
                    return "a Maga";
                case 52:
                    return "a Maga";
                case 53:
                    return "a Maga";
                case 54:
                    return "a Bruxa";
                case 55:
                    return "a Bruxa";
                case 56:
                    return "a Bruxa";
                case 57:
                    return "a Grande Bruxa";
                case 58:
                    return "a Grande Bruxa";
                case 59:
                    return "a Grande Bruxa";
                case 60:
                    return "a Feiticeira";
                case 61:
                    return "a Feiticeira";
                case 62:
                    return "a Feiticeira";
                case 63:
                    return "a Grande Feiticeira";
                case 64:
                    return "a Grande Feiticeira";
                case 65:
                    return "a Grande Feiticeira";
                case 66:
                    return "a Necromante";
                case 67:
                    return "a Necromante";
                case 68:
                    return "a Necromante";
                case 69:
                    return "a Estudante do Oculto";
                case 70:
                    return "a Estudante do Oculto";
                case 71:
                    return "a Estudante do Oculto";
                case 72:
                    return "a Estudante do Misterio";
                case 73:
                    return "a Estudante do Misterio";
                case 74:
                    return "a Estudante do Misterio";
                case 75:
                    return "a Elemental Menor";
                case 76:
                    return "a Elemental Menor";
                case 77:
                    return "a Elemental Menor";
                case 78:
                    return "a Grande Elemental";
                case 79:
                    return "a Grande Elemental";
                case 80:
                    return "a Grande Elemental";
                case 81:
                    return "a Habil das Magias";
                case 82:
                    return "a Habil das Magias";
                case 83:
                    return "a Habil das Magias";
                case 84:
                    return "a Xamã";
                case 85:
                    return "a Xamã";
                case 86:
                    return "a Xamã";
                case 87:
                    return "a Guardiã de Talismãs";
                case 88:
                    return "a Guardiã de Talismãs";
                case 89:
                    return "a Guardiã de Talismãs";
                case 90:
                    return "a Arque-magica";
                case 91:
                    return "a Arque-magica";
                case 92:
                    return "a Arque-magica";
                case 93:
                    return "a Arque-magica";
                case 94:
                    return "a Arque-magica";
                case 95:
                    return "a Arque-magica";
                case 96:
                    return "a Arque-magica";
                case 97:
                    return "a Arque-magica";
                case 98:
                    return "a Arque-magica";
                case 99:
                    return "a Arque-magica";
                case 100:
                    return "a Arque-magica";
                case LVL_IMMORT:
                    return "a Feiticeira Imortal";
                case LVL_IMM2:
                    return "a Feiticeira Imortal";
                case LVL_IMM3:
                    return "a Feiticeira Imortal";
                case LVL_ANCIENT:
                    return "a Feiticeira Imortal";
                case LVL_GRIMM:
                    return "a Feiticeira Imortal";
                case LVL_DEMIGOD:
                    return "a Imperadora da Magia";
                case LVL_GOD:
                    return "a Imperadora da Magia";
                case LVL_GRGOD:
                    return "a Deusa da Magia";
            }
            break;

        case CLASS_CLERIC:
            switch (level) {
                case 1:
                    return "a Crente";
                case 2:
                    return "a Crente";
                case 3:
                    return "a Crente";
                case 4:
                    return "a Crente";
                case 5:
                    return "a Crente";
                case 6:
                    return "a Crente";
                case 7:
                    return "a Crente";
                case 8:
                    return "a Crente";
                case 9:
                    return "a Servente";
                case 10:
                    return "a Servente";
                case 11:
                    return "a Servente";
                case 12:
                    return "a Servente";
                case 13:
                    return "a Servente";
                case 14:
                    return "a Assistente";
                case 15:
                    return "a Assistente";
                case 16:
                    return "a Assistente";
                case 17:
                    return "a Assistente";
                case 18:
                    return "a Assistente";
                case 19:
                    return "a Noviça";
                case 20:
                    return "a Noviça";
                case 21:
                    return "a Noviça";
                case 22:
                    return "a Noviça";
                case 23:
                    return "a Noviça";
                case 24:
                    return "a Missionaria";
                case 25:
                    return "a Missionaria";
                case 26:
                    return "a Missionaria";
                case 27:
                    return "a Missionaria";
                case 28:
                    return "a Missionaria";
                case 29:
                    return "a Praticante";
                case 30:
                    return "a Praticante";
                case 31:
                    return "a Praticante";
                case 32:
                    return "a Praticante";
                case 33:
                    return "a Praticante";
                case 34:
                    return "a Diaconisa";
                case 35:
                    return "a Diaconisa";
                case 36:
                    return "a Diaconisa";
                case 37:
                    return "a Diaconisa";
                case 38:
                    return "a Diaconisa";
                case 39:
                    return "a Vigaria";
                case 40:
                    return "a Vigaria";
                case 41:
                    return "a Vigaria";
                case 42:
                    return "a Vigaria";
                case 43:
                    return "a Vigaria";
                case 44:
                    return "a Sacerdotisa";
                case 45:
                    return "a Sacerdotisa";
                case 46:
                    return "a Sacerdotisa";
                case 47:
                    return "a Sacerdotisa";
                case 48:
                    return "a Sacerdotisa";
                case 49:
                    return "a Ministra";
                case 50:
                    return "a Ministra";
                case 51:
                    return "a Ministra";
                case 52:
                    return "a Ministra";
                case 53:
                    return "a Ministra";
                case 54:
                    return "a Canonisa";
                case 55:
                    return "a Canonisa";
                case 56:
                    return "a Canonisa";
                case 57:
                    return "a Canonisa";
                case 58:
                    return "a Canonisa";
                case 59:
                    return "a Paroca";
                case 60:
                    return "a Paroca";
                case 61:
                    return "a Paroca";
                case 62:
                    return "a Paroca";
                case 63:
                    return "a Paroca";
                case 64:
                    return "a Freira";
                case 65:
                    return "a Freira";
                case 66:
                    return "a Freira";
                case 67:
                    return "a Freira";
                case 68:
                    return "a Freira";
                case 69:
                    return "a Curadora";
                case 70:
                    return "a Curadora";
                case 71:
                    return "a Curadora";
                case 72:
                    return "a Curadora";
                case 73:
                    return "a Curadora";
                case 74:
                    return "a Capela";
                case 75:
                    return "a Capela";
                case 76:
                    return "a Capela";
                case 77:
                    return "a Capela";
                case 78:
                    return "a Capela";
                case 79:
                    return "a Expositora";
                case 80:
                    return "a Expositora";
                case 81:
                    return "a Expositora";
                case 82:
                    return "a Expositora";
                case 83:
                    return "a Expositora";
                case 84:
                    return "a Bispa";
                case 85:
                    return "a Bispa";
                case 86:
                    return "a Bispa";
                case 87:
                    return "a Bispa";
                case 88:
                    return "a Bispa";
                case 89:
                    return "a Arque-Dama da Igreja";
                case 90:
                    return "a Arque-Dama da Igreja";
                case 91:
                    return "a Arque-Dama da Igreja";
                case 92:
                    return "a Arque-Dama da Igreja";
                case 93:
                    return "a Arque-Dama da Igreja";
                case 94:
                    return "a Matriarca";
                case 95:
                    return "a Matriarca";
                case 96:
                    return "a Matriarca";
                case 97:
                    return "a Matriarca";
                case 98:
                    return "a Matriarca";
                case 99:
                    return "a Matriarca";
                case 100:
                    return "a Matriarca";
                case LVL_IMMORT:
                    return "a Sacerdotisa Imortal";
                case LVL_IMM2:
                    return "a Sacerdotisa Imortal";
                case LVL_IMM3:
                    return "a Sacerdotisa Imortal";
                case LVL_ANCIENT:
                    return "a Sacerdotisa Imortal";
                case LVL_GRIMM:
                    return "a Sacerdotisa Imortal";
                case LVL_DEMIGOD:
                    return "a Inquisitora";
                case LVL_GOD:
                    return "a Inquisitora";
                case LVL_GRGOD:
                    return "a Deusa do bom e do mau";
            }
            break;

        case CLASS_THIEF:
            switch (level) {
                case 1:
                    return "a Furtadora";
                case 2:
                    return "a Furtadora";
                case 3:
                    return "a Furtadora";
                case 4:
                    return "a Furtadora";
                case 5:
                    return "a Furtadora";
                case 6:
                    return "a Pequena Ladra";
                case 7:
                    return "a Pequena Ladra";
                case 8:
                    return "a Pequena Ladra";
                case 9:
                    return "a Pequena Ladra";
                case 10:
                    return "a Pequena Ladra";
                case 11:
                    return "a Filadora";
                case 12:
                    return "a Filadora";
                case 13:
                    return "a Filadora";
                case 14:
                    return "a Filadora";
                case 15:
                    return "a Filadora";
                case 16:
                    return "a Batedora de Carteiras";
                case 17:
                    return "a Batedora de Carteiras";
                case 18:
                    return "a Batedora de Carteiras";
                case 19:
                    return "a Batedora de Carteiras";
                case 20:
                    return "a Batedora de Carteiras";
                case 21:
                    return "a Escondida";
                case 22:
                    return "a Escondida";
                case 23:
                    return "a Escondida";
                case 24:
                    return "a Escondida";
                case 25:
                    return "a Escondida";
                case 26:
                    return "a Arrancadora";
                case 27:
                    return "a Arrancadora";
                case 28:
                    return "a Arrancadora";
                case 29:
                    return "a Arrancadora";
                case 30:
                    return "a Arrancadora";
                case 31:
                    return "a Ladra de Bolsas";
                case 32:
                    return "a Ladra de Bolsas";
                case 33:
                    return "a Ladra de Bolsas";
                case 34:
                    return "a Ladra de Bolsas";
                case 35:
                    return "a Ladra de Bolsas";
                case 36:
                    return "a Apanhadora";
                case 37:
                    return "a Apanhadora";
                case 38:
                    return "a Apanhadora";
                case 39:
                    return "a Apanhadora";
                case 40:
                    return "a Apanhadora";
                case 41:
                    return "a Vigarista";
                case 42:
                    return "a Vigarista";
                case 43:
                    return "a Vigarista";
                case 44:
                    return "a Vigarista";
                case 45:
                    return "a Vigarista";
                case 46:
                    return "a Mentirosa";
                case 47:
                    return "a Mentirosa";
                case 48:
                    return "a Mentirosa";
                case 49:
                    return "a Mentirosa";
                case 50:
                    return "a Mentirosa";
                case 51:
                    return "a Roubadora";
                case 52:
                    return "a Roubadora";
                case 53:
                    return "a Roubadora";
                case 54:
                    return "a Roubadora";
                case 55:
                    return "a Roubadora";
                case 56:
                    return "a Esperta";
                case 57:
                    return "a Esperta";
                case 58:
                    return "a Esperta";
                case 59:
                    return "a Esperta";
                case 60:
                    return "a Esperta";
                case 61:
                    return "a Salteadora";
                case 62:
                    return "a Salteadora";
                case 63:
                    return "a Salteadora";
                case 64:
                    return "a Salteadora";
                case 65:
                    return "a Salteadora";
                case 66:
                    return "a Arrombadora";
                case 67:
                    return "a Arrombadora";
                case 68:
                    return "a Arrombadora";
                case 69:
                    return "a Arrombadora";
                case 70:
                    return "a Arrombadora";
                case 71:
                    return "a Ladra";
                case 72:
                    return "a Ladra";
                case 73:
                    return "a Ladra";
                case 74:
                    return "a Ladra";
                case 75:
                    return "a Ladra";
                case 76:
                    return "a Apunhaladora";
                case 77:
                    return "a Apunhaladora";
                case 78:
                    return "a Apunhaladora";
                case 79:
                    return "a Apunhaladora";
                case 80:
                    return "a Apunhaladora";
                case 81:
                    return "a Esfaqueadora";
                case 82:
                    return "a Esfaqueadora";
                case 83:
                    return "a Esfaqueadora";
                case 84:
                    return "a Esfaqueadora";
                case 85:
                    return "a Esfaqueadora";
                case 86:
                    return "a Matadora";
                case 87:
                    return "a Matadora";
                case 88:
                    return "a Matadora";
                case 89:
                    return "a Matadora";
                case 90:
                    return "a Matadora";
                case 91:
                    return "a Bandida";
                case 92:
                    return "a Bandida";
                case 93:
                    return "a Bandida";
                case 94:
                    return "a Bandida";
                case 95:
                    return "a Bandida";
                case 96:
                    return "a Corta-Gargantas";
                case 97:
                    return "a Corta-Gargantas";
                case 98:
                    return "a Corta-Gargantas";
                case 99:
                    return "a Corta-Gargantas";
                case 100:
                    return "a Corta-Gargantas";
                case LVL_IMMORT:
                    return "a Assassina Imortal";
                case LVL_IMM2:
                    return "a Assassina Imortal";
                case LVL_IMM3:
                    return "a Assassina Imortal";
                case LVL_ANCIENT:
                    return "a Assassina Imortal";
                case LVL_GRIMM:
                    return "a Assassina Imortal";
                case LVL_DEMIGOD:
                    return "a Deusa dos Ladroes";
                case LVL_GOD:
                    return "a Guardiã dos Ladroes";
                case LVL_GRGOD:
                    return "a Legisladora dos Ladroes";
            }
            break;

        case CLASS_WARRIOR:
            switch (level) {
                case 1:
                    return "a Novata em Espadas";
                case 2:
                    return "a Novata em Espadas";
                case 3:
                    return "a Novata em Espadas";
                case 4:
                    return "a Novata em Espadas";
                case 5:
                    return "a Novata em Espadas";
                case 6:
                    return "a Recruta";
                case 7:
                    return "a Recruta";
                case 8:
                    return "a Recruta";
                case 9:
                    return "a Recruta";
                case 10:
                    return "a Recruta";
                case 11:
                    return "a Sentinela";
                case 12:
                    return "a Sentinela";
                case 13:
                    return "a Sentinela";
                case 14:
                    return "a Sentinela";
                case 15:
                    return "a Sentinela";
                case 16:
                    return "a Lutadora";
                case 17:
                    return "a Lutadora";
                case 18:
                    return "a Lutadora";
                case 19:
                    return "a Lutadora";
                case 20:
                    return "a Lutadora";
                case 21:
                    return "a Soldada";
                case 22:
                    return "a Soldada";
                case 23:
                    return "a Soldada";
                case 24:
                    return "a Soldada";
                case 25:
                    return "a Soldada";
                case 26:
                    return "a Guerreira";
                case 27:
                    return "a Guerreira";
                case 28:
                    return "a Guerreira";
                case 29:
                    return "a Guerreira";
                case 30:
                    return "a Guerreira";
                case 31:
                    return "a Veterana";
                case 32:
                    return "a Veterana";
                case 33:
                    return "a Veterana";
                case 34:
                    return "a Veterana";
                case 35:
                    return "a Veterana";
                case 36:
                    return "a Esgrimista";
                case 37:
                    return "a Esgrimista";
                case 38:
                    return "a Esgrimista";
                case 39:
                    return "a Esgrimista";
                case 40:
                    return "a Esgrimista";
                case 41:
                    return "a Defendente";
                case 42:
                    return "a Defendente";
                case 43:
                    return "a Defendente";
                case 44:
                    return "a Defendente";
                case 45:
                    return "a Defendente";
                case 46:
                    return "a Combatente";
                case 47:
                    return "a Combatente";
                case 48:
                    return "a Combatente";
                case 49:
                    return "a Combatente";
                case 50:
                    return "a Combatente";
                case 51:
                    return "a Valente";
                case 52:
                    return "a Valente";
                case 53:
                    return "a Valente";
                case 54:
                    return "a Valente";
                case 55:
                    return "a Valente";
                case 56:
                    return "a Heroina";
                case 57:
                    return "a Heroina";
                case 58:
                    return "a Heroina";
                case 59:
                    return "a Heroina";
                case 60:
                    return "a Heroina";
                case 61:
                    return "a Aventureira";
                case 62:
                    return "a Aventureira";
                case 63:
                    return "a Aventureira";
                case 64:
                    return "a Aventureira";
                case 65:
                    return "a Aventureira";
                case 66:
                    return "a Mercenaria";
                case 67:
                    return "a Mercenaria";
                case 68:
                    return "a Mercenaria";
                case 69:
                    return "a Mercenaria";
                case 70:
                    return "a Mercenaria";
                case 71:
                    return "a Mestre em Espada";
                case 72:
                    return "a Mestre em Espada";
                case 73:
                    return "a Mestre em Espada";
                case 74:
                    return "a Mestre em Espada";
                case 75:
                    return "a Mestre em Espada";
                case 76:
                    return "a Tenente";
                case 77:
                    return "a Tenente";
                case 78:
                    return "a Tenente";
                case 79:
                    return "a Tenente";
                case 80:
                    return "a Tenente";
                case 81:
                    return "a Campea";
                case 82:
                    return "a Campea";
                case 83:
                    return "a Campea";
                case 84:
                    return "a Campea";
                case 85:
                    return "a Campea";
                case 86:
                    return "a Dragoon";
                case 87:
                    return "a Dragoon";
                case 88:
                    return "a Dragoon";
                case 89:
                    return "a Dragoon";
                case 90:
                    return "a Dragoon";
                case 91:
                    return "a Cavaleira";
                case 92:
                    return "a Cavaleira";
                case 93:
                    return "a Cavaleira";
                case 94:
                    return "a Cavaleira";
                case 95:
                    return "a Cavaleira";
                case 96:
                    return "a Grande Cavaleira";
                case 97:
                    return "a Grande Cavaleira";
                case 98:
                    return "a Grande Cavaleira";
                case 99:
                    return "a Grande Cavaleira";
                case 100:
                    return "a Grande Cavaleira";
                case LVL_IMMORT:
                    return "a Guerreira Imortal";
                case LVL_IMM2:
                    return "a Guerreira Imortal";
                case LVL_IMM3:
                    return "a Guerreira Imortal";
                case LVL_ANCIENT:
                    return "a Guerreira Imortal";
                case LVL_GRIMM:
                    return "a Guerreira Imortal";
                case LVL_DEMIGOD:
                    return "a Rainha da Destruicao";
                case LVL_GOD:
                    return "a Rainha da Destruicao";
                case LVL_GRGOD:
                    return "a Deusa da Guerra";
            }
            break;

        case CLASS_DRUID: /* -- mp */
            switch (level) {
                case 1:
                    return "a Aspirante";
                case 2:
                    return "a Aspirante";
                case 3:
                    return "a Aspirante";
                case 4:
                    return "a Aspirante";
                case 5:
                    return "a Aspirante";
                case 6:
                    return "a Ovate";
                case 7:
                    return "a Ovate";
                case 8:
                    return "a Ovate";
                case 9:
                    return "a Ovate";
                case 10:
                    return "a Ovate";
                case 11:
                    return "a Iniciada no Primeiro Círculo";
                case 12:
                    return "a Iniciada no Primeiro Círculo";
                case 13:
                    return "a Iniciada no Primeiro Círculo";
                case 14:
                    return "a Iniciada no Primeiro Círculo";
                case 15:
                    return "a Iniciada no Primeiro Círculo";
                case 16:
                    return "a Iniciada no Segundo Círculo";
                case 17:
                    return "a Iniciada no Segundo Círculo";
                case 18:
                    return "a Iniciada no Segundo Círculo";
                case 19:
                    return "a Iniciada no Segundo Círculo";
                case 20:
                    return "a Iniciada no Segundo Círculo";
                case 21:
                    return "a Iniciada no Terceiro Círculo";
                case 22:
                    return "a Iniciada no Terceiro Círculo";
                case 23:
                    return "a Iniciada no Terceiro Círculo";
                case 24:
                    return "a Iniciada no Terceiro Círculo";
                case 25:
                    return "a Iniciada no Terceiro Círculo";
                case 26:
                    return "a Iniciada no Quarto Círculo";
                case 27:
                    return "a Iniciada no Quarto Círculo";
                case 28:
                    return "a Iniciada no Quarto Círculo";
                case 29:
                    return "a Iniciada no Quarto Círculo";
                case 30:
                    return "a Iniciada no Quarto Círculo";
                case 31:
                    return "a Iniciada no Quinto Círculo";
                case 32:
                    return "a Iniciada no Quinto Círculo";
                case 33:
                    return "a Iniciada no Quinto Círculo";
                case 34:
                    return "a Iniciada no Quinto Círculo";
                case 35:
                    return "a Iniciada no Quinto Círculo";
                case 36:
                    return "a Iniciada no Sexto Círculo";
                case 37:
                    return "a Iniciada no Sexto Círculo";
                case 38:
                    return "a Iniciada no Sexto Círculo";
                case 39:
                    return "a Iniciada no Sexto Círculo";
                case 40:
                    return "a Iniciada no Sexto Círculo";
                case 41:
                    return "a Iniciada no Sétimo Círculo";
                case 42:
                    return "a Iniciada no Sétimo Círculo";
                case 43:
                    return "a Iniciada no Sétimo Círculo";
                case 44:
                    return "a Iniciada no Sétimo Círculo";
                case 45:
                    return "a Iniciada no Sétimo Círculo";
                case 46:
                    return "a Iniciada no Oitavo Círculo";
                case 47:
                    return "a Iniciada no Oitavo Círculo";
                case 48:
                    return "a Iniciada no Oitavo Círculo";
                case 49:
                    return "a Iniciada no Oitavo Círculo";
                case 50:
                    return "a Iniciada no Oitavo Círculo";
                case 51:
                    return "a Iniciada no Nono Círculo";
                case 52:
                    return "a Iniciada no Nono Círculo";
                case 53:
                    return "a Iniciada no Nono Círculo";
                case 54:
                    return "a Iniciada no Nono Círculo";
                case 55:
                    return "a Iniciada no Nono Círculo";
                case 56:
                    return "a Druidesa";
                case 57:
                    return "a Druidesa";
                case 58:
                    return "a Druidesa";
                case 59:
                    return "a Druidesa";
                case 60:
                    return "a Druidesa";
                case 61:
                    return "a Arque-Druidesa";
                case 62:
                    return "a Arque-Druidesa";
                case 63:
                    return "a Arque-Druidesa";
                case 64:
                    return "a Arque-Druidesa";
                case 65:
                    return "a Arque-Druidesa";
                case 66:
                    return "a Grã-Druidesa";
                case 67:
                    return "a Grã-Druidesa";
                case 68:
                    return "a Grã-Druidesa";
                case 69:
                    return "a Grã-Druidesa";
                case 70:
                    return "a Grã-Druidesa";
                case 71:
                    return "a Magna Druidesa";
                case 72:
                    return "a Magna Druidesa";
                case 73:
                    return "a Magna Druidesa";
                case 74:
                    return "a Magna Druidesa";
                case 75:
                    return "a Magna Druidesa";
                case 76:
                    return "a Druidesa Hierofante";
                case 77:
                    return "a Druidesa Hierofante";
                case 78:
                    return "a Druidesa Hierofante";
                case 79:
                    return "a Druidesa Hierofante";
                case 80:
                    return "a Druidesa Hierofante";
                case 81:
                    return "a Hierofante Iniciada";
                case 82:
                    return "a Hierofante Iniciada";
                case 83:
                    return "a Hierofante Iniciada";
                case 84:
                    return "a Hierofante Iniciada";
                case 85:
                    return "a Hierofante Iniciada";
                case 86:
                    return "a Hierofante Adepta";
                case 87:
                    return "a Hierofante Adepta";
                case 88:
                    return "a Hierofante Adepta";
                case 89:
                    return "a Hierofante Adepta";
                case 90:
                    return "a Hierofante Adepta";
                case 91:
                    return "a Mestra Hierofante";
                case 92:
                    return "a Mestra Hierofante";
                case 93:
                    return "a Mestra Hierofante";
                case 94:
                    return "a Mestra Hierofante";
                case 95:
                    return "a Mestra Hierofante";
                case 96:
                    return "a Sublime Hierofante";
                case 97:
                    return "a Sublime Hierofante";
                case 98:
                    return "a Sublime Hierofante";
                case 99:
                    return "a Sublime Hierofante";
                case 100:
                    return "a Sublime Hierofante";
                case LVL_IMMORT:
                    return "a Mística Hierofante";
                case LVL_IMM2:
                    return "a Mística Hierofante";
                case LVL_IMM3:
                    return "a Mística Hierofante";
                case LVL_ANCIENT:
                    return "a Mística Hierofante";
                case LVL_GRIMM:
                    return "a Mística Hierofante";
                case LVL_DEMIGOD:
                    return "a Hierofante Arcana";
                case LVL_GOD:
                    return "a Hierofante Arcana";
                case LVL_GRGOD:
                    return "a Hierofante da Cabala";
            }
            break;

        case CLASS_BARD: /* ac -- march 2003 */
            switch (level) {
                case 1:
                    return "a Aprendiz de Malabarista";
                case 2:
                    return "a Aprendiz de Malabarista";
                case 3:
                    return "a Aprendiz de Malabarista";
                case 4:
                    return "a Aprendiz de Malabarista";
                case 5:
                    return "a Aprendiz de Malabarista";
                case 6:
                    return "a Histriã";
                case 7:
                    return "a Histriã";
                case 8:
                    return "a Histriã";
                case 9:
                    return "a Histriã";
                case 10:
                    return "a Histriã";
                case 11:
                    return "a Truona";
                case 12:
                    return "a Truona";
                case 13:
                    return "a Truona";
                case 14:
                    return "a Truona";
                case 15:
                    return "a Truona";
                case 16:
                    return "a Estudante de Música";
                case 17:
                    return "a Estudante de Música";
                case 18:
                    return "a Estudante de Música";
                case 19:
                    return "a Estudante de Música";
                case 20:
                    return "a Estudante de Música";
                case 21:
                    return "a Cantora";
                case 22:
                    return "a Cantora";
                case 23:
                    return "a Cantora";
                case 24:
                    return "a Cantora";
                case 25:
                    return "a Cantora";
                case 26:
                    return "a Musicista";
                case 27:
                    return "a Musicista";
                case 28:
                    return "a Musicista";
                case 29:
                    return "a Musicista";
                case 30:
                    return "a Musicista";
                case 31:
                    return "a Literata";
                case 32:
                    return "a Literata";
                case 33:
                    return "a Literata";
                case 34:
                    return "a Literata";
                case 35:
                    return "a Literata";
                case 36:
                    return "a Escritora";
                case 37:
                    return "a Escritora";
                case 38:
                    return "a Escritora";
                case 39:
                    return "a Escritora";
                case 40:
                    return "a Escritora";
                case 41:
                    return "a Poeta";
                case 42:
                    return "a Poeta";
                case 43:
                    return "a Poeta";
                case 44:
                    return "a Poeta";
                case 45:
                    return "a Poeta";
                case 46:
                    return "a Contadora de Histórias";
                case 47:
                    return "a Contadora de Histórias";
                case 48:
                    return "a Contadora de Histórias";
                case 49:
                    return "a Contadora de Histórias";
                case 50:
                    return "a Contadora de Histórias";
                case 51:
                    return "a Instrumentista";
                case 52:
                    return "a Instrumentista";
                case 53:
                    return "a Instrumentista";
                case 54:
                    return "a Instrumentista";
                case 55:
                    return "a Instrumentista";
                case 56:
                    return "a Virtuose";
                case 57:
                    return "a Virtuose";
                case 58:
                    return "a Virtuose";
                case 59:
                    return "a Virtuose";
                case 60:
                    return "a Virtuose";
                case 61:
                    return "a Jogralesa";
                case 62:
                    return "a Jogralesa";
                case 63:
                    return "a Jogralesa";
                case 64:
                    return "a Jogralesa";
                case 65:
                    return "a Jogralesa";
                case 66:
                    return "a Segrelesa";
                case 67:
                    return "a Segrelesa";
                case 68:
                    return "a Segrelesa";
                case 69:
                    return "a Segrelesa";
                case 70:
                    return "a Segrelesa";
                case 71:
                    return "a Menestrel";
                case 72:
                    return "a Menestrel";
                case 73:
                    return "a Menestrel";
                case 74:
                    return "a Menestrel";
                case 75:
                    return "a Menestrel";
                case 76:
                    return "a Barda";
                case 77:
                    return "a Barda";
                case 78:
                    return "a Barda";
                case 79:
                    return "a Barda";
                case 80:
                    return "a Barda";
                case 81:
                    return "a Barda Nobre";
                case 82:
                    return "a Barda Nobre";
                case 83:
                    return "a Barda Nobre";
                case 84:
                    return "a Barda Nobre";
                case 85:
                    return "a Barda Nobre";
                case 86:
                    return "a Mestra na Música";
                case 87:
                    return "a Mestra na Música";
                case 88:
                    return "a Mestra na Música";
                case 89:
                    return "a Mestra na Música";
                case 90:
                    return "a Mestra na Música";
                case 91:
                    return "a Trovadora";
                case 92:
                    return "a Trovadora";
                case 93:
                    return "a Trovadora";
                case 94:
                    return "a Trovadora";
                case 95:
                    return "a Trovadora";
                case 96:
                    return "a Mestra da Trova";
                case 97:
                    return "a Mestra da Trova";
                case 98:
                    return "a Mestra da Trova";
                case 99:
                    return "a Mestra da Trova";
                case 100:
                    return "a Mestra da Trova";
                case LVL_IMMORT:
                    return "a Trovadora Celestial";
                case LVL_IMM2:
                    return "a Trovadora Celestial";
                case LVL_IMM3:
                    return "a Trovadora Celestial";
                case LVL_ANCIENT:
                    return "a Trovadora Celestial";
                case LVL_GRIMM:
                    return "a Trovadora Celestial";
                case LVL_DEMIGOD:
                    return "a Deusa da Música e da Poesia";
                case LVL_GOD:
                    return "a Harmonia Divina";
                case LVL_GRGOD:
                    return "a Regente da Vibração Universal";
            }
            break;

        case CLASS_RANGER:
            switch (level) {
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    return "a Pintinho";
                case 6:
                case 7:
                case 8:
                    return "a Patatenra";
                case 9:
                case 10:
                case 11:
                    return "a Gatinha";
                case 12:
                case 13:
                case 14:
                    return "a Raposinha";
                case 15:
                case 16:
                case 17:
                    return "a Lobinha";
                case 18:
                case 19:
                case 20:
                    return "a Ursinha";
                case 21:
                case 22:
                case 23:
                    return "a Tigresinha";
                case 24:
                case 25:
                case 26:
                    return "a Leoazinha";
                case 27:
                case 28:
                case 29:
                    return "a Filhote de Falcão";
                case 30:
                case 31:
                case 32:
                    return "a Filhote de Águia";
                case 33:
                case 34:
                case 35:
                    return "a Furão";
                case 36:
                case 37:
                case 38:
                    return "a Raposa";
                case 39:
                case 40:
                case 41:
                    return "a Lince";
                case 42:
                case 43:
                case 44:
                    return "a Loba";
                case 45:
                case 46:
                case 47:
                    return "a Ursa";
                case 48:
                case 49:
                case 50:
                    return "a Tigresa";
                case 51:
                case 52:
                case 53:
                    return "a Leoa";
                case 54:
                case 55:
                case 56:
                    return "a Peregrina";
                case 57:
                case 58:
                case 59:
                    return "a Dama Falcão";
                case 60:
                case 61:
                case 62:
                    return "a Dama Águia";
                case 63:
                case 64:
                case 65:
                    return "a Escoteira";
                case 66:
                case 67:
                case 68:
                    return "a Lenhadora";
                case 69:
                case 70:
                case 71:
                    return "a Encontradora de Trilhas";
                case 72:
                case 73:
                case 74:
                    return "a Fazedora de Trilhas";
                case 75:
                case 76:
                case 77:
                    return "a Montanhesa";
                case 78:
                case 79:
                case 80:
                    return "a Dama Ranger";
                case 81:
                case 82:
                case 83:
                    return "a Caçadora";
                case 84:
                case 85:
                case 86:
                    return "a Alta Ranger";
                case 87:
                case 88:
                case 89:
                    return "a Grande Dama Ranger";
                case 90:
                case 91:
                case 92:
                case 93:
                case 94:
                case 95:
                case 96:
                case 97:
                case 98:
                case 99:
                case 100:
                    return "a Grande Dama Caçadora";
                case LVL_IMMORT:
                    return "a Caçadora Imortal";
                case LVL_GOD:
                    return "a Matrona da Caça";
                case LVL_GRGOD:
                    return "a Deusa das Florestas";
                default:
                    return "a Viajante";
            }
    }

    /* Default title for classes which do not have titles defined */
    return "a sem-classe";
}
