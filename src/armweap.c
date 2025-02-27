/* ************************************************************************* */
/* coded by castillo. email: castillo7@hotmail.com                           */
/* ------------------------------------------------------------------------- */
/* 08/01/1999 : Released ver 1.0                                             */
/* ------------------------------------------------------------------------- */
/* 10/01/1999 : ver 1.01 - fix a little bug that could mix up player's order */
/* ------------------------------------------------------------------------- */
/* 17/01/2020 : Updated to compile against TBAMUD 2020                       */
/* ************************************************************************* */

#include "conf.h" 
#include "sysdep.h" 
#include "structs.h" 
#include "utils.h" 
#include "comm.h" 
#include "fight.h"
#include "interpreter.h" 
#include "modify.h"
#include "handler.h" 
#include "db.h" 
#include "spells.h" 
#include "oasis.h"
#include "genzon.h"
#include "genobj.h"

#define ARMORER_MODE    0
#define BLACKSMITH_MODE 1

/* extern variables */
extern char *pc_class_types[]; 
extern char *apply_types[]; 
extern char *wear_bits[]; 

/* ----------------------------------------------------------------------- */
/* This give a list a zone number (vnum) the program will use to 
   store all armors and weapons created. Support zone of 100 VNUMs
   WARNING: the list must end with NOWHERE                                 */
/* ----------------------------------------------------------------------- */
const int zones_system [] = {652,653, 654, NOWHERE};
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
/* These fields define what is allowed to choose...                        
   FALSE(0): not allowed.                                                       
   TRUE(1) : allowed.                                                           
   TRUE(2) : allowed but the rest isn't. NOTE: must be used in need, to keep 
             good text alignement. but using only 0 and 1 won't hurt.      */
/* ----------------------------------------------------------------------- */
const short allow_wear [NUM_ITEM_WEARS] = 
      {0, // ITEM_WEAR_TAKE
       1, // ITEM_WEAR_FINGER
       1, // ITEM_WEAR_NECK
       1, // ITEM_WEAR_BODY
       1, // ITEM_WEAR_HEAD
       1, // ITEM_WEAR_LEGS
       1, // ITEM_WEAR_FEET
       1, // ITEM_WEAR_HANDS
       1, // ITEM_WEAR_ARMS
       1, // ITEM_WEAR_SHIELD
       1, // ITEM_WEAR_ABOUT
       1, // ITEM_WEAR_WAIST
       1, // ITEM_WEAR_WRIST
       0, // ITEM_WEAR_WIELD
       1, // ITEM_WEAR_HOLD
       0, // ITEM_WEAR_WINGS
       1, // ITEM_WEAR_WEAR
       1, // ITEM_WEAR_FACE
       1, // ITEM_WEAR_NOSE
       1, // ITEM_WEAR_INSIGNE
       0  // ITEM_WEAR_QUIVER
       }; 

const short allow_stat [NUM_APPLIES] = 
      {0, // APPLY_NONE
       1, // APPLY_STR
       1, // APPLY_DEX
       1, // APPLY_INT
       1, // APPLY_WIS
       1, // APPLY_CON
       1, // APPLY_CHA
       0, // APPLY_CLASS (Reserved)
       0, // APPLY_LEVEL (Reserved)
       1, // APPLY_AGE
       1, // APPLY_CHAR_WEIGHT
       1, // APPLY_CHAR_HEIGHT
       1, // APPLY_MANA
       1, // APPLY_HIT
       1, // APPLY_MOVE
       0, // APPLY_GOLD (Reserved)
       0, // APPLY_EXP (Reserved)
       1, // APPLY_AC
       1, // APPLY_HITROLL
       1, // APPLY_DAMROLL
       1, // APPLY_SAVING_PARA
       1, // APPLY_SAVING_ROD
       1, // APPLY_SAVING_PETRI
       1, // APPLY_SAVING_BREATH
       1  // APPLY_SAVING_SPELL
       }; 


const short allow_attack [NUM_ATTACK_TYPES] = 
      {1, // hit
       1, // sting
       1, // whip
       1, // slash
       1, // bite
       1, // bludgeon
       1, // crush
       1, // pound
       1, // claw
       1, // maul
       1, // thrash
       1, // pierce
       1, // blast
       1, // punch
       1  // stab
      };

/* ----------------------------------------------------------------------- */
/* TRUE(1) or FALSE(0). this field identify which applies are better to
   have under 0. right now, there's: ARMOR, and all SAVING_ set to 1. 
   That's needed to "calculate" the price for an armor or weapon...
   eg: hitroll by 1, or armor by 1... armor must be known as a negative
   stat, because it's better to have armor by (under 0)                    */
/* ----------------------------------------------------------------------- */
const short neg_stat [NUM_APPLIES] = 
      {0, // APPLY_NONE
       0, // APPLY_STR
       0, // APPLY_DEX
       0, // APPLY_INT
       0, // APPLY_WIS
       0, // APPLY_CON
       0, // APPLY_CHA
       0, // APPLY_CLASS (Reserved)
       0, // APPLY_LEVEL (Reserved)
       0, // APPLY_AGE
       0, // APPLY_CHAR_WEIGHT
       0, // APPLY_CHAR_HEIGHT
       0, // APPLY_MANA
       0, // APPLY_HIT
       0, // APPLY_MOVE
       0, // APPLY_GOLD (Reserved)
       0, // APPLY_EXP (Reserved)
       1, // APPLY_AC
       0, // APPLY_HITROLL
       0, // APPLY_DAMROLL
       1, // APPLY_SAVING_PARA
       1, // APPLY_SAVING_ROD
       1, // APPLY_SAVING_PETRI
       1, // APPLY_SAVING_BREATH
       1  // APPLY_SAVING_SPELL
      };
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
const signed int stat_limits [NUM_APPLIES][2] = 
      {{0,0},    // APPLY_NONE
       {-4,4},   // APPLY_STR
       {-4,4},   // APPLY_DEX
       {-4,4},   // APPLY_INT
       {-4,4},   // APPLY_WIS
       {-4,4},   // APPLY_CON
       {-4,4},   // APPLY_CHA
       {0,0},    // APPLY_CLASS (Reserved)
       {0,0},    // APPLY_LEVEL (Reserved)
       {-10,10}, // APPLY_AGE
       {-99,99}, // APPLY_CHAR_WEIGHT
       {-1,1},   // APPLY_CHAR_HEIGHT
       {0,100},  // APPLY_MANA
       {0,100},  // APPLY_HIT
       {0,100},  // APPLY_MOVE
       {0,0},    // APPLY_GOLD (Reserved)
       {0,0},    // APPLY_EXP (Reserved)
       {-30,30}, // APPLY_AC
       {-4,4},   // APPLY_HITROLL
       {-4,4},   // APPLY_DAMROLL
       {-10,10}, // APPLY_SAVING_PARA
       {-10,10}, // APPLY_SAVING_ROD
       {-10,10}, // APPLY_SAVING_PETRI
       {-10,10}, // APPLY_SAVING_BREATH
       {-10,10}  // APPLY_SAVING_SPELL
       };

/* index (0-2)... 0=HITROLL_MODIFIER, 1=NUMBER_DAMAGE, 2=SIZE_DAMAGE */
const short weapon_limits [3][2]  = {{0, 10}, {0, 10}, {0, 20}};

const int damage_price = 800000;
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
/* basic price for an armor is based on the price of the slot choosen */
const int wear_price [NUM_ITEM_WEARS] = 
      {0     , // ITEM_WEAR_TAKE
       300000, // ITEM_WEAR_FINGER
       500000, // ITEM_WEAR_NECK
       300000, // ITEM_WEAR_BODY
       300000, // ITEM_WEAR_HEAD
       300000, // ITEM_WEAR_LEGS
       300000, // ITEM_WEAR_FEET
       300000, // ITEM_WEAR_HANDS
       300000, // ITEM_WEAR_ARMS
       300000, // ITEM_WEAR_SHIELD
       300000, // ITEM_WEAR_ABOUT
       300000, // ITEM_WEAR_WAIST
       300000, // ITEM_WEAR_WRIST
       300000, // ITEM_WEAR_WIELD
       300000,  // ITEM_WEAR_HOLD
       0, // ITEM_WEAR_WINGS
       200000, // ITEM_WEAR_EAR
       200000, // ITEM_WEAR_FACE
       150000, // ITEM_WEAR_NOSE
      1000000, // ITEM_WEAR_INSIGNE
       0  // ITEM_WEAR_QUIVER
       };

const int stat_price [NUM_APPLIES] = 
      {0,      // APPLY_NONE
       80000,  // APPLY_STR
       100000, // APPLY_DEX
       100000, // APPLY_INT
       100000, // APPLY_WIS
       100000, // APPLY_CON
       100000, // APPLY_CHA
       0,      // APPLY_CLASS (Reserved)
       0,      // APPLY_LEVEL (Reserved)
       50000,  // APPLY_AGE
       20000,  // APPLY_CHAR_WEIGHT
       20000,  // APPLY_CHAR_HEIGHT
       50000,  // APPLY_MANA
       50000,  // APPLY_HIT
       50000,  // APPLY_MOVE
       0,      // APPLY_GOLD (Reserved)
       0,      // APPLY_EXP (Reserved)
       70000,  // APPLY_AC
       150000, // APPLY_HITROLL
       150000, // APPLY_DAMROLL
       20000,  // APPLY_SAVING_PARA
       20000,  // APPLY_SAVING_ROD
       20000,  // APPLY_SAVING_PETRI
       20000,  // APPLY_SAVING_BREATH
       20000   // APPLY_SAVING_SPELL
       }; 

/* basic price for a weapon is based on attack type */
const int attack_price [NUM_ATTACK_TYPES] = 
      {400000, // hit
       400000, // sting
       400000, // whip
       400000, // slash
       400000, // bite
       400000, // bludgeon
       400000, // crush
       400000, // pound
       400000, // claw
       400000, // maul
       400000, // thrash
       400000, // pierce
       400000, // blast
       400000, // punch
       400000  // stab
       };
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
const char weapon_affname [3][20] = {
           "HITROLL_MODIFIER", 
           "NUMBER_DAMAGE",  
           "SIZE_DAMAGE"};
const char name_mob_mode [2][20] = {"Armorer", "Blacksmith"};
const char name_obj_mode [2][20] = {"armor", "weapon"};

struct armory_olcs {
   char *name;
   struct oasis_olc_data *olc;
   struct armory_olcs *next;
} *armorer_list = NULL;

struct armory_olcs *blacksmith_list = NULL;
struct obj_data item;
/* ********************************************************************** */

/* validate a description, name, etc.
   We don't allow ~ or $, because it's marks in database file... 
   otherwise after a crash or reboot, your mud won't boot up.
   I don't @ as color code for object name

   NOTE: Truncate the string if the size is above max_size                */
void val_desc (char *s, bool color, int max_size) 
{
 int i;
 for (i=0; i<strlen(s); i++) {
   if (i >= max_size - 1) {
     s[i] = '\x0';
     break;
   }

   if ((s[i] == '~') || (s[i] == '$') || ((s[i] == '@') && !color))
     s[i] = ' ';
 }
} 

void list_stat (struct char_data *ch) 
{
 int i, cpt=0;
 char buf [2048] = "";
 char ind [4][10] = {"\r\n", "  *  ", "\r\n", "\r\n"};

 sprintf(buf, "There is the list of availables stats:\r\n"
              "(choose one or more with: want <name> <value>)\r\n\r\n"
              "@YName            Price   Limits         "
              "Name            Price   Limits.@n\r\n");      
 parse_at(buf);
 send_to_char(ch, "%s", buf);

 for (i=0; i<NUM_APPLIES; i++)
   if ((allow_stat [i] < 0) || (allow_stat [i] > 2)) /* sanity check */ 
     send_to_char (ch, "\r\n**** ERROR: allow_stat[] is corrupted.\r\n");
   else
     if (allow_stat [i] != 0) {
       sprintf (buf, "@R%-13s @m: @g%4d@Rk   @c[@g%3d@c, @g%3d@c]@n%s",
                     apply_types [i], stat_price [i] / 1000, stat_limits [i][0],
                     stat_limits [i][1], (i < NUM_APPLIES - 1) ?
                     ind [allow_stat [i] * 2 + (++cpt % 2) - 2] : "\r\n");
       parse_at(buf);
       send_to_char (ch, "%s", buf);
     } 
}

void list_wear (struct char_data *ch)
{
 int i, cpt=0;
 char buf [2048] = "";
 char ind [4][10] = {"\r\n", "   *   ", "\r\n", "\r\n"};

 sprintf (buf, "What kind of body armor do you want?\r\n"
               "(choose a kind with: want wear <name>)\r\n\r\n"
               "@YName            Price      Name            Price.@n\r\n");
 parse_at(buf);
 send_to_char (ch, "%s", buf);

 for (i=0; i<NUM_ITEM_WEARS; i++)
   if ((allow_wear [i] < 0) || (allow_wear [i] > 2)) /* sanity check */ 
     send_to_char (ch, "\r\n**** ERROR: allow_wear [] is corrupted.\r\n");
   else
     if (allow_wear [i] != 0) {
       sprintf (buf, "@R%-13s @m: @g%3d@Rk@n%s", wear_bits [i], 
                wear_price [i] / 1000, (i < NUM_ITEM_WEARS - 1) ? 
                ind [allow_wear [i] * 2 + (++cpt % 2) - 2] : "\r\n");
       parse_at(buf);
       send_to_char (ch, "%s", buf);
     } 
}
 
void list_attack (struct char_data *ch)
{
 int i, cpt=0;
 char buf [2048] = "";
 char ind [4][10] = {"\r\n", "   *   ", "\r\n", "\r\n"};

 sprintf (buf, "What kind of weapon do you want?\r\n"
               "(choose a kind with: want attack <name>)\r\n\r\n"
               "@YAttack name      Price       Attack name      Price.@n\r\n");
 parse_at(buf);
 send_to_char (ch, "%s", buf);

 for (i=0; i<NUM_ATTACK_TYPES; i++) 
   if ((allow_attack [i] < 0) || (allow_attack [i] > 2)) /* sanity check */ 
     send_to_char (ch, "\r\n**** ERROR: allow_attack [] is corrupted.\r\n");
   else
     if (allow_attack [i] != 0) {
       sprintf (buf, "@R%-12s @m: @g%6d@Rk@n%s", attack_hit_text[i].singular,
                attack_price[i] / 1000, (i < NUM_ATTACK_TYPES - 1) ?
                ind [allow_attack [i] * 2 + (++cpt % 2) - 2] : "\r\n");
       parse_at(buf);
       send_to_char (ch, "%s", buf);
     } 
}

void list_help (struct char_data *ch, int mode) 
{
  char buf [2048] = "";

  sprintf (buf, "@RCommands are@5:@n\r\n"
                "@YList stat   @m- @cShow you the list of add on.\r\n"
                "%s"
                "@YWant        @m- @cac <value>.\r\n"
                "@Y            @m- @clong <desc>.  (that's the description you see\r\n"
                "                             @cif your %s is lying on the floor)\r\n"
                "@Y            @m- @cname <name>.  (that's the words to handle your %s)\r\n"
                "@Y            @m- @cshort <desc>. (that's the normal description)\r\n"
                "@Y            @m- @cstat <value>. (see: 'list stat')\r\n"
                "@Y%s" 
                "@YBuy         @m- @cbuy the %s you just setted up.\r\n"
                "@YReset       @m- @cWill reset the %s to the default one.\r\n"
                "@RNote: @m- @cthere is a limits of @R%d @cstats per item.\r\n" 
                "      @m- @cYou get a pay back of half the value for a negative stat.\n\r" 
                "      @m- @cThere is a minimum fee per %s that is: the price of %s",

                (mode == ARMORER_MODE) ? "@YList wear   @m- @cGet a look to my armorer's skills.\r\n"
                                       : "@YList attack @m- @cGet a look to my blacksmith's skills.\r\n",
                name_obj_mode [mode], name_obj_mode [mode],
                (mode == ARMORER_MODE) ? "            @m- @cwear <item_wear>.\r\n"
                                       :
                                         "            @m- @cSIZE_DAMAGE <number>.\r\n"
					 "            @m- @cNUMBER_DAMAGE <number>.\r\n",
                name_obj_mode [mode], name_obj_mode [mode],
                MAX_OBJ_AFFECT, name_obj_mode [mode],
                (mode == ARMORER_MODE) ? "item wear.\r\n" : 
                "attack type.\r\n      @m- @cADD TO THE BASIC PRICE:"
                "(NUMBER_DAMAGE * SIZE_DAMAGE * 800k)@n\r\n");
  parse_at(buf);
  send_to_char (ch, "%s", buf);
  sprintf(buf, "@c-----------------------------------------------------------------@n\r\n");
  parse_at(buf);
  send_to_char (ch, "%s", buf);
}

void do_want (struct char_data *ch, char *argument, struct armory_olcs *d,
              int mode) 
{
  int i, i2, found = -1, number;
  char buf [2048] = "";

  argument = one_argument(argument, buf);
  skip_spaces (&argument);

  number = atoi (argument);
  for (i=0; i<NUM_APPLIES; i++) {
    if (allow_stat[i] && isname(buf, apply_types[i])) {
      if ((number < stat_limits[i][0]) || (number > stat_limits[i][1])) {
        sprintf (buf, "@gYour value is out of range. the limits for @m%s"
                      " @gis @c[@R%d@g, @R%d@c].@n\r\n", 
                 apply_types[i], stat_limits[i][0], stat_limits[i][1]);
        parse_at(buf);
        send_to_char (ch, "%s", buf);
        return;    
      } else {
          for (i2=0; i2<MAX_OBJ_AFFECT; i2++) 
            if (OLC_OBJ(d)->affected [i2].location == i) {
              found = i2;
              i2 = MAX_OBJ_AFFECT;
            } else if ( (found == -1) && 
                         ((OLC_OBJ(d)->affected[i2].location == 0) ||
                          (i2 == MAX_OBJ_AFFECT - 1)) )
                     found = i2; 
          if (number != 0) {
            OLC_OBJ(d)->affected[found].location = i;
            sprintf (buf, "@gYou set @m%s @gto @m: @R%d.\r\n@n",
                    apply_types[i], number);     
          } else {
              OLC_OBJ(d)->affected[found].location = 0;
              sprintf (buf, "@gYou unset @m%s.@n\r\n", apply_types[i]);
            } 
          OLC_OBJ(d)->affected[found].modifier = number;
          parse_at(buf);
          send_to_char (ch, "%s", buf);
          return;                      
        }   
    }
  }

  if (isname(buf, "name")) {
    if (OLC_OBJ(d)->name)
      free (OLC_OBJ(d)->name);
    val_desc (argument, false, MAX_OBJ_NAME);
    OLC_OBJ(d)->name = strdup(argument);
    sprintf (buf, "@gYou set @mNAME@g to @m: @R%s@n\r\n", argument);
    parse_at(buf);
    send_to_char (ch, "%s", buf);
    return;
  }
  if (isname(buf, "long description")) {
    if (OLC_OBJ(d)->description)
      free (OLC_OBJ(d)->description);
    val_desc(argument, true, MAX_OBJ_DESC);
    OLC_OBJ(d)->description = strdup(argument);
    sprintf (buf, "@mYou set @gLONG DESCRIPTION@m to @g: @R%s@n\r\n", argument);
    parse_at(buf);
    send_to_char (ch, "%s", buf); 
    return;
  }
  if (isname(buf, "short description")) {
    if (OLC_OBJ(d)->short_description)
      free (OLC_OBJ(d)->short_description);
    val_desc(argument, true, MAX_OBJ_DESC);
    OLC_OBJ(d)->short_description = strdup (argument);
    sprintf (buf, "@mYou set @gSHORT DESCRIPTION@m to @g: @R%s@n\r\n", 
             argument);
    parse_at(buf);
    send_to_char (ch, "%s", buf);
    return;
  }
  if (mode == ARMORER_MODE) {
    if (isname(buf, "ac")) {
      if ( (number < stat_limits[APPLY_AC][0]) ||
           (number > stat_limits[APPLY_AC][1]) ) {
        sprintf (buf, "@gYour value is out of range. the limits for @m%s"
                      " @gis @c[@R%d@g, @R%d@c].@n\r\n", 
                 apply_types[APPLY_AC], stat_limits[APPLY_AC][0],
                 stat_limits[APPLY_AC][1]);
        parse_at(buf);
        send_to_char (ch, "%s", buf);
        return;
      }
      GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
      sprintf (buf, "@gYou set @mAC@g to @m: @R%d@n\r\n", number);
      parse_at(buf);
      send_to_char (ch, "%s", buf);
      return;
    }
    if (isname(buf, "wear")) {
      for (i=0; i<NUM_ITEM_WEARS; i++)
        if (allow_wear [i] && isname(argument, wear_bits[i])) {
          GET_OBJ_WEAR(OLC_OBJ(d))[0] = i;
          sprintf (buf, "@gYou set @mWEAR_ITEM@g to @m: @R%s@n\r\n", 
                   wear_bits[i]);
          parse_at(buf);
          send_to_char (ch, "%s", buf);
          return;
        }
    }
  } else {          /* mode=blacksmith */
      if (isname(buf, "attack")) 
        for (i=0; i<NUM_ATTACK_TYPES; i++)
           if (allow_attack[i] && isname(argument,
               attack_hit_text[i].singular)) {
             GET_OBJ_VAL(OLC_OBJ(d), 3) = i;
             sprintf (buf, "@gYou set @mATTACK TYPE@g to @m: @R%s@n\r\n",
                      attack_hit_text[i].singular);
             parse_at(buf);
             send_to_char (ch, "%s", buf);
             return;
           }    
      for (i=0; i<3; i++)
        if (isname(buf, weapon_affname [i])) {
           if ((number < weapon_limits [i][0]) || 
               (number > weapon_limits [i][1])) {
             sprintf (buf, "@gYour value is out of range. the limits for "
                           "@m%s @gis @c[@R%d@g, @R%d@c].@n\r\n", 
                            weapon_affname [i], weapon_limits[i][0],
                            weapon_limits[i][1]);
             parse_at(buf);
             send_to_char (ch, "%s", buf);
             return;     
           } else {
               GET_OBJ_VAL(OLC_OBJ(d), i) = number;
               sprintf (buf, "@gYou set @m%s @gto @m: @R%d@n\r\n",
                        weapon_affname [i], number);
               parse_at(buf);
               send_to_char (ch, "%s", buf);
               return;
             }         
        }
    }
 sprintf (buf, "@mSorry, @gbut i don't understand what you want.@n\r\n");
 parse_at(buf);
 send_to_char (ch, "%s", buf);
}

void reset_armweap (struct char_data *ch, struct armory_olcs *q, int aff,
                    int mode)
{
 char buf [2048] = "";

 clear_object(OLC_OBJ(q));  
 if (OLC_OBJ(q)->name)
   free(OLC_OBJ(q)->name);
 if (OLC_OBJ(q)->description)
   free(OLC_OBJ(q)->description);
 if (OLC_OBJ(q)->short_description)
   free(OLC_OBJ(q)->short_description); 
 OLC_OBJ(q)->name = strdup("Undefined object");
 OLC_OBJ(q)->description = strdup ("An unfinished object is lying here.");
 OLC_OBJ(q)->short_description = strdup("An unfinished object");
 GET_OBJ_WEAR(OLC_OBJ(q))[0] = 1;
 if (mode == ARMORER_MODE) 
   GET_OBJ_TYPE(OLC_OBJ(q)) = ITEM_ARMOR;
 else 
   GET_OBJ_TYPE(OLC_OBJ(q)) = ITEM_WEAPON; 
 OLC_VAL(q) = 0;
 if (aff) {
   sprintf (buf, "@gStat for %s has been reset.@n\r\n", name_obj_mode [mode]);
   parse_at(buf);
   send_to_char (ch, "%s", buf);
 }
}

long int armory_cost ( struct armory_olcs *q, int mode )
{
 long int cost;
 long int min_cost;
 int i, ac_cost, aff_cost, aff_id, aff_mod;

/* add the base price: wear slot price */
 if (mode == ARMORER_MODE) {
   cost = min_cost = wear_price [GET_OBJ_WEAR(OLC_OBJ(q))[0]];

/* add the AC cost */
   ac_cost = stat_price [APPLY_AC];
   if (GET_OBJ_VAL(OLC_OBJ(q), 0) < 0) 
     ac_cost = ac_cost / 2;

   cost += ac_cost * GET_OBJ_VAL(OLC_OBJ(q), 0);
 }
 else {
   cost = min_cost = attack_price [GET_OBJ_VAL(OLC_OBJ(q), 3)];

   cost += GET_OBJ_VAL(OLC_OBJ(q), 0) * stat_price[APPLY_HITROLL];
   cost += GET_OBJ_VAL(OLC_OBJ(q), 1) * GET_OBJ_VAL(OLC_OBJ(q), 2) * damage_price;
 }

/* add the price of each stats (modifier) */
 for (i=0; i < MAX_OBJ_AFFECT; i++) {
    aff_id = OLC_OBJ(q)->affected[i].location;
    aff_mod = OLC_OBJ(q)->affected[i].modifier;

    aff_cost = stat_price [aff_id] * abs(aff_mod);

    if (neg_stat [aff_id] ^ (aff_mod > 0))
      cost += aff_cost;
    else
      cost -= aff_cost / 2;   
 }

/* let's make sure the eq cost somethings */
 if (cost < min_cost)
   cost = min_cost;

 return (cost);
}

void armweap_buy (struct char_data *ch, struct armory_olcs *q, int mode)
{
 long int cost;
 struct obj_data *obj;
 int i, x = -1;
 char buf [2048] = "";

 cost = armory_cost (q, mode);
 if (GET_GOLD(ch) < cost) {
   sprintf (buf, "@gSorry, you don't have enough coins!@n\r\n");
   parse_at(buf);
   send_to_char (ch, "%s", buf);
   return;
 }

 OLC_NUM(q) = NOWHERE;
 while (OLC_NUM(q) == NOWHERE) {
   while ( (zones_system[++x] != NOWHERE) &&
           ((OLC_ZNUM(q) = real_zone_by_thing(zones_system[x] * 100)) == NOWHERE));
   if (zones_system[x] == NOWHERE) {
     sprintf (buf, "Sorry, there's no zone to create armor. notify an admin.\r\n");
     send_to_char (ch, "%s", buf);

     log1("SYSERR: There's no zone allowed to created new objs.");
     return;
   }
   for (i=0; i<100; i++)
      if (real_object (zones_system[x] * 100 + i) == NOTHING) {
        OLC_NUM(q) = (zones_system[x] * 100 + i);
        break;
      }
 }

 (ch->desc)->olc = q->olc;
 if (mode == BLACKSMITH_MODE) 
   SET_BIT_AR(GET_OBJ_WEAR(OLC_OBJ(q)), ITEM_WEAR_WIELD);
 else
   GET_OBJ_WEAR(OLC_OBJ(q))[0] = (1 << GET_OBJ_WEAR(OLC_OBJ(q))[0]) + 1;

 oedit_save_internally(ch->desc);
 
 (ch->desc)->olc = NULL;

 save_objects(OLC_ZNUM(q));

 obj = read_object (real_object(OLC_NUM(q)), REAL);

 if (!obj) {
   send_to_char (ch, "Object creation failed!\r\n");
   return;
 }

 obj_to_char (obj, ch);
 GET_GOLD(ch) -= cost;

 sprintf (buf, "@gYou bought @m%s @gfor @R%lu @mcoins!@n\r\n", 
          obj->short_description, cost);
 parse_at(buf);
 send_to_char (ch, "%s", buf); 
 sprintf (buf, "NOTIFY: %s created %s at the armorer.\n\r", GET_NAME(ch), 
          obj->short_description);
 log1("%s", buf);

 reset_armweap (ch, q, FALSE, mode); 
}

int main_armweap (struct char_data *ch, int cmd, char *argument, int mode)
{
 int i;
 struct descriptor_data *d;
 struct armory_olcs *q = NULL, *prev = NULL; 
 char buf [2048] = "";

 if (!(d=ch->desc)) 
   return 1;

 skip_spaces(&argument);

 if (CMD_IS("want") || CMD_IS("reset") || CMD_IS("list") ||
     CMD_IS("buy")) {
   if (IS_NPC(ch)) {
     send_to_char (ch, "Little pet, Go Away!\r\n");
     return 1;
   }
   for (q = (mode == ARMORER_MODE) ? armorer_list : blacksmith_list; q;
        prev=q, q=q->next)
      if (isname (q->name, GET_NAME(ch))) 
        break;
   if (q == NULL) {
     CREATE(q, struct armory_olcs, 1);
     if ((mode == ARMORER_MODE) && (armorer_list == NULL))
       armorer_list = q;
     else if ((mode == BLACKSMITH_MODE) && (blacksmith_list == NULL))
            blacksmith_list = q;
          else
            prev->next=q;
     q->name = strdup(GET_NAME(ch));
     q->next=NULL;
     CREATE(q->olc, struct oasis_olc_data, 1);
     CREATE(OLC_OBJ(q), struct obj_data, 1);
     reset_armweap(ch, q, FALSE, mode);
   }
 }
 if (CMD_IS("buy")) {
   armweap_buy (ch, q, mode);
   return 1;
 }
 if (CMD_IS("want")) {
   do_want (ch, argument, q, mode);
   return 1;
 }
 if (CMD_IS("reset")) {
   reset_armweap (ch, q, TRUE, mode);
   return 1;
 } 
 if (CMD_IS("list")) {
   sprintf (buf, "@gWelcome to the %s of @w%s@g.@n\r\n", 
            name_mob_mode [mode], pc_class_types[(int)GET_CLASS(ch)]);
   parse_at(buf);
   send_to_char (ch, "%s", buf);
   sprintf(buf, "@w-----------------------------------------------------------------@n\r\n");
   parse_at(buf);
   send_to_char (ch, "%s", buf);

   if (isname(argument, "stat")) {
     list_stat(ch);
     return 1;
   }
   if (mode == ARMORER_MODE)
     if (isname(argument, "wear")) {
       list_wear(ch);
       return 1;
     } else;
   else if (isname(argument, "attack")) {
          list_attack(ch);
          return 1;
        }
   if (isname(argument, "help")) {
     list_help (ch, mode);
     return 1;
   }
   sprintf (buf, "@RNote@O: 'list help' to get a list of help.\r\n\r\n"
                     "There is your current order@m:@n\r\n"
                 "@c-----------------------------------------------------------------@n\r\n"
                 "@YName list@m          :@c %s@n\r\n" 
                 "@YShort description@m  :@c %s@n\r\n"
                 "@YLong description@m   :@c %s@n\r\n", 
            OLC_OBJ(q)->name, OLC_OBJ(q)->short_description,
            OLC_OBJ(q)->description);
   parse_at(buf);
   send_to_char (ch, "%s", buf);
   if (mode == ARMORER_MODE) 
     sprintf (buf, "@YWear@m               : @c%s\r\n"
                   "@YAC@m                 : @c%d@n\r\n",
              wear_bits[GET_OBJ_WEAR(OLC_OBJ(q))[0]], 
              GET_OBJ_VAL(OLC_OBJ(q), 0));
   else 
     sprintf (buf, "@YAttack type        @m: @c%s\r\n" 
                   "@Y%-19s@m: @c%d\r\n" 
                   "@Y%-19s@m: @c%d\r\n"
                   "@Y%-19s@m: @c%d@n\r\n",
              attack_hit_text [GET_OBJ_VAL(OLC_OBJ(q), 3)].singular,
              weapon_affname [0], GET_OBJ_VAL(OLC_OBJ(q), 0), 
              weapon_affname [1], GET_OBJ_VAL(OLC_OBJ(q), 1),
              weapon_affname [2], GET_OBJ_VAL(OLC_OBJ(q), 2));
   parse_at(buf);
   send_to_char (ch, "%s", buf);

   for (i=0; i<MAX_OBJ_AFFECT; i++) 
     if (OLC_OBJ(q)->affected[i].location) {
       sprintf (buf, "@YModifier           @m: @c%s @mby @c%d@n\r\n", 
                      apply_types [(int)OLC_OBJ(q)->affected[i].location],
                      OLC_OBJ(q)->affected[i].modifier);
       parse_at(buf);
       send_to_char (ch, "%s", buf);
     }  
   sprintf (buf, "\r\n@gThis nice %s should cost you a little @m: @R%lu"
                 " @gcoins.@n\r\n", 
            name_obj_mode [mode], armory_cost (q, mode));
   parse_at(buf);
   send_to_char (ch, "%s", buf);
   return 1;
 }
 return 0;
}

void free_armweap()
{
 struct armory_olcs *q = armorer_list, *n;
 while (q) {
   n = q->next;

   free(q->name);
   free(OLC_OBJ(q));
   free(q->olc);
   free(q);

   q = n;
 }

 q = blacksmith_list;
 while (q) {
   n = q->next;

   free(q->name);
   free(OLC_OBJ(q));
   free(q->olc);
   free(q);

   q = n;
 }
}

SPECIAL(armorer)
{
 return (main_armweap (ch, cmd, argument, ARMORER_MODE));
}

SPECIAL(blacksmith)
{
 return (main_armweap (ch, cmd, argument, BLACKSMITH_MODE));
}
/* ******************************************************************** */
