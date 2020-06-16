/* Copyright (c) 2020 castillo7@hotmail.com

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "spedit.h"

ACMD(do_backstab);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick);
ACMD(do_sneak);
ACMD(do_track);
ACMD(do_steal);
ACMD(do_hide);
ACMD(do_pick_lock);
ACMD(do_whirlwind);
ACMD(do_bandage);
ACMD(do_scan);
ACMD(do_meditate);
ACMD(do_trip);
ACMD(do_backflip);
ACMD(do_combo);

void set_spells_function()
{
 struct str_spells *spell;

 log1("Assigning spells's function.");

 if ((spell = get_spell_by_vnum(SPELL_TELEPORT)))
   spell->function = spell_teleport;

 if ((spell = get_spell_by_vnum(SPELL_CHARM)))
   spell->function = spell_charm;

 if ((spell = get_spell_by_vnum(SPELL_CONTROL_WEATHER)))
   spell->function = spell_control_weather;

 if ((spell = get_spell_by_vnum(SPELL_CREATE_WATER)))
   spell->function = spell_create_water;
   
    if ((spell = get_spell_by_vnum(SPELL_CREATE_NECTAR)))
   spell->function = spell_create_nectar;

 if ((spell = get_spell_by_vnum(SPELL_DETECT_POISON)))
   spell->function = spell_detect_poison;

 if ((spell = get_spell_by_vnum(SPELL_ENCHANT_WEAPON)))
   spell->function = spell_enchant_weapon;

 if ((spell = get_spell_by_vnum(SPELL_LOCATE_OBJECT)))
   spell->function = spell_locate_object;

 if ((spell = get_spell_by_vnum(SPELL_SUMMON)))
   spell->function = spell_summon;

 if ((spell = get_spell_by_vnum(SPELL_WORD_OF_RECALL)))
   spell->function = spell_recall;

 if ((spell = get_spell_by_vnum(SPELL_IDENTIFY)))
   spell->function = spell_identify;

 if ((spell = get_spell_by_vnum(SPELL_TRANSPORT_VIA_PLANTS)))
   spell->function = spell_transport_via_plants;

 if ((spell = get_spell_by_vnum(SPELL_PORTAL)))
   spell->function = spell_portal;

 if ((spell = get_spell_by_vnum(SKILL_BACKSTAB)))
   spell->function = do_backstab;

 if ((spell = get_spell_by_vnum(SKILL_BASH)))
   spell->function = do_bash;

 if ((spell = get_spell_by_vnum(SKILL_HIDE)))
   spell->function = do_hide;

 if ((spell = get_spell_by_vnum(SKILL_KICK)))
   spell->function = do_kick;

 if ((spell = get_spell_by_vnum(SKILL_WHIRLWIND)))
   spell->function = do_whirlwind;

 if ((spell = get_spell_by_vnum(SKILL_RESCUE)))
   spell->function = do_rescue;

 if ((spell = get_spell_by_vnum(SKILL_SNEAK)))
   spell->function = do_sneak;

 if ((spell = get_spell_by_vnum(SKILL_STEAL)))
   spell->function = do_steal;

 if ((spell = get_spell_by_vnum(SKILL_TRACK)))
   spell->function = do_track;

 if ((spell = get_spell_by_vnum(SKILL_BANDAGE)))
   spell->function = do_bandage;
   
    if ((spell = get_spell_by_vnum(SKILL_SCAN)))
   spell->function = do_scan;
   
       if ((spell = get_spell_by_vnum(SKILL_TRIP)))
   spell->function = do_trip;
   
      if ((spell = get_spell_by_vnum(SKILL_BACKFLIP)))
   spell->function = do_backflip;
   
       if ((spell = get_spell_by_vnum(SKILL_COMBO_ATTACK)))
   spell->function = do_combo;
    
       if ((spell = get_spell_by_vnum(SKILL_MEDITATE)))
   spell->function = do_meditate;
}

// This function create the database of all the spells and skills,
// that exist in TBA MUD and respect the original VNUMs.
// for compatibility reasons.
//
// This function could be remove eventually. ?!
// It's there to create the original spells/skills DB, or recreate it.
// If a spells/skills DB exists, set_spells_function() will be called instead.
void create_spells_db() 
{
 struct str_spells *new_spell = NULL;
 char buf[MAX_STRING_LENGTH];

 log1("Creating spells Database.");

 // SPELL_ARMOR #1
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_ARMOR;
 new_spell->status = available;
 new_spell->name = strdup("armor");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 15 ? (30 - (3 * self.level)) : 15");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 3;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 1;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-20");
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você sente alguém $r protegendo.");
 new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");

 spedit_save_internally(new_spell);

 // SPELL_TELEPORT #2
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_TELEPORT;
 new_spell->status = available;
 new_spell->name = strdup("teleport");
 new_spell->function = spell_teleport;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(75 - (3 * self.level)) > 50 ? (75 - (3 * self.level)) : 50");
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_BLESS #3
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_BLESS;
 new_spell->status = available;
 new_spell->name = strdup("bless");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ALTER_OBJS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(35 - (3 * self.level)) > 5 ? (35 - (3 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 6;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_HITROLL;
 sprintf(buf,"1 + (1 + (self.level / 30))");
 new_spell->applies[0].modifier = strdup(buf);
 new_spell->applies[0].duration = strdup("6");
 new_spell->applies[1].appl_num = APPLY_SAVING_SPELL;
 sprintf(buf,"-(1 + (self.level / 30))");
 new_spell->applies[1].modifier = strdup(buf);
 new_spell->applies[1].duration = strdup("6");
 new_spell->messages.to_self = strdup("$p brilha por alguns instantes.");
 new_spell->messages.to_vict = strdup("Você se sente $x(virtuoso,virtuosa).");
 new_spell->messages.wear_off = strdup("Você se sente menos abençoad$r.");

 spedit_save_internally(new_spell);

 // SPELL_BLINDNESS #4
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_BLINDNESS;
 new_spell->status = available;
 new_spell->name = strdup("blindness");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(35 - (1 * self.level)) > 25 ? (35 - (1 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 14;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 8;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_HITROLL;
 new_spell->applies[0].modifier = strdup("-4");
 new_spell->applies[0].duration = strdup("2");
 new_spell->applies[1].appl_num = APPLY_AC;
 new_spell->applies[1].modifier = strdup("40");
 new_spell->applies[1].duration = strdup("2");
 new_spell->applies[2].appl_num = AFF_BLIND + NUM_APPLIES;
 new_spell->applies[2].duration = strdup("2");
 new_spell->messages.to_vict = strdup("Você está ceg$r!");
 new_spell->messages.to_room = strdup("$N parece estar ceg$r!");
 new_spell->messages.wear_off = strdup("Você volta a enxergar.");

 spedit_save_internally(new_spell);

 // SPELL_BURNING_HANDS #5
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_BURNING_HANDS;
 new_spell->status = available;
 new_spell->name = strdup("burning hands");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 10 ? (30 - (3 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 6;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup ("dice(3, self.class == 0 ? 8 : 6) + 3");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_CALL_LIGHTNING #6
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CALL_LIGHTNING;
 new_spell->status = available;
 new_spell->name = strdup("call lightning");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (3 * self.level)) > 25 ? (40 - (3 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 22;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(7, 8) + 7");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_CHARM_PERSON #7
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CHARM;
 new_spell->status = available;
 new_spell->name = strdup("charm person");
 new_spell->function = spell_charm;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
 new_spell->mag_flags = MAG_MANUAL | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(75 - (2 * self.level)) > 50 ? (75 - (2 * self.level)) : 50");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 25;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 28;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 24;
 new_spell->assign[2].num_mana = strdup(buf);
 new_spell->messages.wear_off = strdup("You feel more self-confident.");

 spedit_save_internally(new_spell);

 // SPELL_CHILL_TOUCH #8
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CHILL_TOUCH;
 new_spell->status = available;
 new_spell->name = strdup("chill touch");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT | MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 10 ? (30 - (3 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 3;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_STR;
 new_spell->applies[0].modifier = strdup("-1");
 new_spell->applies[0].duration = strdup("4");
 new_spell->damages = strdup("dice(1, self.class == 0 ? 8 : 6) + 1");
 new_spell->max_dam = 100;
 new_spell->messages.to_vict = strdup("You feel your strength wither!");
 new_spell->messages.wear_off = strdup("You feel your strength return.");

 spedit_save_internally(new_spell);

 // SPELL_CLONE #9
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CLONE;
 new_spell->status = available;
 new_spell->name = strdup("clone");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_SUMMONS;
 new_spell->effectiveness = strdup("100");
 new_spell->summon_mob = strdup("10");
 new_spell->summon_req = strdup("161");
 sprintf(buf, "(80 - (5 * self.level)) > 65 ? (80 - (5 * self.level)) : 65");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 100;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N magically divides!");

 spedit_save_internally(new_spell);

 // SPELL_COLOR_SPRAY #10
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_COLOR_SPRAY;
 new_spell->status = available;
 new_spell->name = strdup("color spray");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 15 ? (30 - (3 * self.level)) : 15");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 17;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(9, self.class == 0 ? 8 : 6) + 9");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_CONTROL_WEATHER #11
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CONTROL_WEATHER;
 new_spell->status = available;
 new_spell->name = strdup("control weather");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(75 - (5 * self.level)) > 25 ? (75 - (5 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 26;
 new_spell->assign[0].num_mana = strdup(buf);
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 25;
 new_spell->assign[1].num_mana = strdup(buf);
    new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 55;
 new_spell->assign[2].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_CREATE_FOOD #12
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CREATE_FOOD;
 new_spell->status = available;
 new_spell->name = strdup("create food");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_CREATIONS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 2;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_DRUID;
 new_spell->assign[1].level = 7;
 new_spell->assign[1].num_mana = strdup(buf);
   new_spell->assign[2].class_num = CLASS_RANGER;
 new_spell->assign[2].level = 12;
 new_spell->assign[2].num_mana = strdup(buf);
 new_spell->objects[0] = strdup("10");  // object VNUM 10 = waybread
 
 spedit_save_internally(new_spell);

// SPELL_CREATE_FOOD_DRUID #95
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CREATE_FOOD_DRUID;
 new_spell->status = available;
 new_spell->name = strdup("create food");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_CREATIONS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
 new_spell->assign[0].class_num =  CLASS_DRUID;
 new_spell->assign[0].level = 7;
 new_spell->assign[0].num_mana = strdup(buf);
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 12;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->objects[0] = strdup("11");  // object VNUM 11 = morango
 
 spedit_save_internally(new_spell);

 // SPELL_CREATE_WATER #13
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CREATE_WATER;
 new_spell->status = available;
 new_spell->name = strdup("create water");
 new_spell->function = spell_create_water;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_INV | TAR_OBJ_EQUIP;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 3;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 12;
 new_spell->assign[1].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_CURE_BLIND # 14
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CURE_BLIND;
 new_spell->status = available;
 new_spell->name = strdup("cure blind");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_UNAFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (2 * self.level)) > 5 ? (30 - (2 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 5;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("Your vision returns!");
 new_spell->messages.to_room = strdup("There's a momentary gleam in $N's eyes.");
 new_spell->dispel[0] = strdup("4");  // spell VNUM 4 = Blindness

 spedit_save_internally(new_spell);

 // SPELL_CURE_CRITIC # 15
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CURE_CRITIC;
 new_spell->status = available;
 new_spell->name = strdup("cure critic");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_POINTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (2 * self.level)) > 10 ? (30 - (2 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 14;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("You feel a lot better!");
 new_spell->points.hp = strdup("dice(3, 8) + 3 + (param / 4)");
 spedit_save_internally(new_spell);
 
 // SPELL_CURE_LIGHT # 16
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CURE_LIGHT;
 new_spell->status = available;
 new_spell->name = strdup("cure light");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_POINTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (2 * self.level)) > 10 ? (30 - (2 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 1;
 new_spell->assign[0].num_mana = strdup(buf);
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 3;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("You feel better.");
 new_spell->points.hp = strdup("dice(1, 8) + 1 + (param / 4)");
 spedit_save_internally(new_spell);

 // SPELL_CURSE # 17
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CURSE;
 new_spell->status = available;
 new_spell->name = strdup("curse");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV;
 new_spell->mag_flags = MAG_VIOLENT | MAG_AFFECTS | MAG_ACCDUR | MAG_ACCMOD | MAG_ALTER_OBJS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(80 - (2 * self.level)) > 50 ? (80 - (2 * self.level)) : 50");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 20;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_HITROLL;
 new_spell->applies[0].modifier = strdup("-1");
 new_spell->applies[0].duration = strdup("1 + (self.level / 2)");
 new_spell->applies[1].appl_num = APPLY_DAMROLL;
 new_spell->applies[1].modifier = strdup("-1");
 new_spell->applies[1].duration = strdup("1 + (self.level / 2)");
 new_spell->applies[2].appl_num = AFF_CURSE + NUM_APPLIES;
 new_spell->applies[2].duration = strdup("1 + (self.level / 2)");
 new_spell->messages.to_self = strdup("$b briefly glows red.");
 new_spell->messages.to_vict = strdup("You feel very uncomfortable.");
 new_spell->messages.to_room = strdup("$N briefly glows red!");
 new_spell->messages.wear_off = strdup("You feel more optimistic.");

 spedit_save_internally(new_spell);
 
 // SPELL_DETECT_ALIGNMENT # 18
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DETECT_ALIGN;
 new_spell->status = available;
 new_spell->name = strdup("detect alignment");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(20 - (2 * self.level)) > 10 ? (20 - (2 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 4;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_DETECT_ALIGN + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("12 + param");
 new_spell->messages.to_vict = strdup("Your eyes tingle.");
 new_spell->messages.wear_off = strdup("You feel less aware.");

 spedit_save_internally(new_spell);

 // SPELL_DETECT_INVIS # 19
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DETECT_INVIS;
 new_spell->status = available;
 new_spell->name = strdup("detect invisibility");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(20 - (2 * self.level)) > 10 ? (20 - (2 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 2;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 8;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_DETECT_INVIS + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("12 + param");
 new_spell->messages.to_vict = strdup("Your eyes tingle.");
 new_spell->messages.wear_off = strdup("Your eyes stop tingling.");

 spedit_save_internally(new_spell);

 // SPELL_DETECT_MAGIC # 20
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DETECT_MAGIC;
 new_spell->status = available;
 new_spell->name = strdup("detect magic");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(20 - (2 * self.level)) > 10 ? (20 - (2 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 2;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_DETECT_MAGIC + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("12 + param");
 new_spell->messages.to_vict = strdup("Your eyes tingle.");
 new_spell->messages.wear_off = strdup("The detect magic wears off.");

 spedit_save_internally(new_spell);

 // SPELL_DETECT_POISON # 21
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DETECT_POISON;
 new_spell->status = available;
 new_spell->name = strdup("detect poison");
 new_spell->function = spell_detect_poison;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(15 - (1 * self.level)) > 5 ? (15 - (1 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 16;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 4;
 new_spell->assign[1].num_mana = strdup(buf);
   new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 4;
 new_spell->assign[2].num_mana = strdup(buf);
   new_spell->assign[3].class_num = CLASS_RANGER;
 new_spell->assign[3].level = 12;
 new_spell->assign[3].num_mana = strdup(buf);
 new_spell->messages.wear_off = strdup("The detect poison wears off.");	

 spedit_save_internally(new_spell);

 // SPELL_DISPEL_EVIL # 22
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DISPEL_EVIL;
 new_spell->status = available;
 new_spell->name = strdup("dispel evil");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (3 * self.level)) > 25 ? (40 - (3 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 20;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(6, 8) + 6");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_EARTHQUAKE # 23
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_EARTHQUAKE;
 new_spell->status = available;
 new_spell->name = strdup("earthquake");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_AREAS | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (3 * self.level)) > 25 ? (40 - (3 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 18;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_DRUID;
 new_spell->assign[1].level = 70;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(2, 8) + param");
 new_spell->max_dam = 100;
 new_spell->messages.to_self = strdup("You gesture and the earth begins to shake all around you!");
 new_spell->messages.to_room = strdup("$N gracefully gestures and the earth begins to shake violently!");

 spedit_save_internally(new_spell);

 // SPELL_ENCHANT_WEAPON # 24
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_ENCHANT_WEAPON;
 new_spell->status = available;
 new_spell->name = strdup("enchant weapon");
 new_spell->function = spell_enchant_weapon;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_INV;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(150 - (10 * self.level)) > 100 ? (150 - (10 * self.level)) : 100");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 34;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_ENERGY_DRAIN # 25
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_ENERGY_DRAIN;
 new_spell->status = available;
 new_spell->name = strdup("energy drain");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT | MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (1 * self.level)) > 25 ? (40 - (1 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 18;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("vict.level <= 2 ? 100 : dice(1, 10)");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_FIREBALL # 26
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_FIREBALL;
 new_spell->status = available;
 new_spell->name = strdup("fireball");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (2 * self.level)) > 30 ? (40 - (2 * self.level)) : 30");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 22;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(11, self.class == 0 ? 8 : 6) + 11");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_HARM # 27
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_HARM;
 new_spell->status = available;
 new_spell->name = strdup("harm");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(75 - (3 * self.level)) > 45 ? (75 - (3 * self.level)) : 45");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 28;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(8, 8) + 8");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_HEAL # 28
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_HEAL;
 new_spell->status = available;
 new_spell->name = strdup("heal");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_POINTS | MAG_UNAFFECTS;
 new_spell->effectiveness = strdup("100");
 new_spell->points.hp = strdup("100 + dice(3, 8)");
 sprintf(buf, "(60 - (3 * self.level)) > 40 ? (60 - (3 * self.level)) : 40");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 24;
 new_spell->assign[0].num_mana = strdup(buf);
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 16;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->dispel[0] = strdup("4");
 new_spell->messages.to_vict = strdup("A warm feeling floods your body.");

 spedit_save_internally(new_spell);

 // SPELL_INVISIBILITY # 29
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_INVISIBLE;
 new_spell->status = available;
 new_spell->name = strdup("invisibility");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ALTER_OBJS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(35 - (1 * self.level)) > 25 ? (35 - (1 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 5;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-40");
 new_spell->applies[0].duration = strdup("12 + (self.level / 4)");
 new_spell->applies[1].appl_num = AFF_INVISIBLE + NUM_APPLIES;
 new_spell->applies[1].duration = strdup("12 + (self.level / 4)");
 new_spell->messages.to_self = strdup("$b vanishes.");
 new_spell->messages.to_vict = strdup("You vanish.");
 new_spell->messages.to_room = strdup("$N slowly fades out of existence.");
 new_spell->messages.wear_off = strdup("You feel yourself exposed.");

 spedit_save_internally(new_spell);

 // SPELL_LIGHTNING_BOLT # 30
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_LIGHTNING_BOLT;
 new_spell->status = available;
 new_spell->name = strdup("lightning bolt");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (1 * self.level)) > 15 ? (30 - (1 * self.level)) : 15");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 12;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_MAGIC_USER;
 new_spell->assign[1].level = 80;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(7, self.class == 0 ? 8 : 6) + 7");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_LOCATE_OBJECT # 31
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_LOCATE_OBJECT;
 new_spell->status = available;
 new_spell->name = strdup("locate object");
 new_spell->function = spell_locate_object;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_WORLD;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(25 - (1 * self.level)) > 20 ? (25 - (1 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 9;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_MAGIC_MISSILE # 32
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_MAGIC_MISSILE;
 new_spell->status = available;
 new_spell->name = strdup("magic missile");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(25 - (3 * self.level)) > 10 ? (25 - (3 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 1;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_BARD;
 new_spell->assign[1].level = 2;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(1, self.class == 0 ? 8 : 6) + 1");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_POISON # 33
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_POISON;
 new_spell->status = available;
 new_spell->name = strdup("poison");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV;
 new_spell->mag_flags = MAG_VIOLENT | MAG_AFFECTS | MAG_ALTER_OBJS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(50 - (3 * self.level)) > 20 ? (50 - (3 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 20;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 12;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_STR;
 new_spell->applies[0].modifier = strdup("-2");
 new_spell->applies[0].duration = strdup("self.level");
 new_spell->applies[1].appl_num = AFF_POISON + NUM_APPLIES;
 new_spell->applies[1].duration = strdup("self.level");
 new_spell->messages.to_self = strdup("$b steams briefly.");
 new_spell->messages.to_vict = strdup("You feel very sick.");
 new_spell->messages.to_room = strdup("$N gets violently ill!");
 new_spell->messages.wear_off = strdup("You feel less sick.");

 spedit_save_internally(new_spell);

 // SPELL_PROTECTION_FROM_EVIL # 34
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_PROT_FROM_EVIL;
 new_spell->status = available;
 new_spell->name = strdup("protection from evil");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (3 * self.level)) > 10 ? (40 - (3 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 11;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_PROTECT_EVIL + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("You feel invulnerable!");
 new_spell->messages.wear_off = strdup("You feel less protected.");

 spedit_save_internally(new_spell);

 // SPELL_REMOVE_CURSE # 35
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_REMOVE_CURSE;
 new_spell->status = available;
 new_spell->name = strdup("remove curse");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP;
 new_spell->mag_flags = MAG_UNAFFECTS | MAG_ALTER_OBJS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(45 - (5 * self.level)) > 25 ? (45 - (5 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 34;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->dispel[0] = strdup("17"); // dispel curse
 new_spell->messages.to_self = strdup("$b briefly glows blue.");
 new_spell->messages.to_vict = strdup("You don't feel so unlucky.");

 spedit_save_internally(new_spell);

 // SPELL_SANCTUARY # 36
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_SANCTUARY;
 new_spell->status = available;
 new_spell->name = strdup("sanctuary");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(110 - (5 * self.level)) > 85 ? (110 - (5 * self.level)) : 85");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 21;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_SANCTUARY + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("4");
 new_spell->messages.to_vict = strdup("A white aura momentarily surrounds you.");
 new_spell->messages.to_room = strdup("$N is surrounded by a white aura.");
 new_spell->messages.wear_off = strdup("The white aura around your body fades.");

 spedit_save_internally(new_spell);

 // SPELL_SHOCKING_GRASP # 37
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_SHOCKING_GRASP;
 new_spell->status = available;
 new_spell->name = strdup("shocking grasp");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 15 ? (30 - (3 * self.level)) : 15");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 10;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(5, self.class == 0 ? 8 : 6) + 5");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_SLEEP # 38
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_SLEEP;
 new_spell->status = available;
 new_spell->name = strdup("sleep");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (5 * self.level)) > 25 ? (40 - (5 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 12;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_SLEEP + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("self.level / 4 + 4");
 new_spell->messages.wear_off = strdup("You feel less tired.");
 new_spell->messages.to_vict = strdup("You feel very sleepy...  Zzzz......");
 new_spell->messages.to_room = strdup("$N goes to sleep.");

 spedit_save_internally(new_spell);

 // SPELL_STRENGTH # 39
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_STRENGTH;
 new_spell->status = available;
 new_spell->name = strdup("strength");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_ACCMOD;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(35 - (1 * self.level)) > 30 ? (35 - (1 * self.level)) : 30");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 8;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_STR;
 new_spell->applies[0].modifier = strdup("1 + (param > 18)");
 new_spell->applies[0].duration = strdup("(self.level / 2) + 4");
 new_spell->messages.to_vict = strdup("You feel stronger!");
 new_spell->messages.wear_off = strdup("You feel weaker.");

 spedit_save_internally(new_spell);

 // SPELL_SUMMON # 40
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_SUMMON;
 new_spell->status = available;
 new_spell->name = strdup("summon");
 new_spell->function = spell_summon;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(75 - (3 * self.level)) > 50 ? (75 - (3 * self.level)) : 50");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 11;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_MAGIC_USER;
 new_spell->assign[1].level = 14;
 new_spell->assign[1].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_VENTRILOQUATE # 41
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_VENTRILOQUATE;
 new_spell->status = available;
 new_spell->name = strdup("ventriloquate");
 new_spell->type = SPELL;
 new_spell->effectiveness = strdup("100");

 spedit_save_internally(new_spell);

 // SPELL_WORD_OF_RECALL # 42
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_WORD_OF_RECALL;
 new_spell->status = available;
 new_spell->name = strdup("word of recall");
 new_spell->function = spell_recall;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(20 - (2 * self.level)) > 10 ? (20 - (2 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 65;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 18;
 new_spell->assign[1].num_mana = strdup(buf);
  new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 37;
 new_spell->assign[2].num_mana = strdup(buf);
  new_spell->assign[3].class_num = CLASS_RANGER;
 new_spell->assign[3].level = 50;
 new_spell->assign[3].num_mana = strdup(buf);
 spedit_save_internally(new_spell);

 // SPELL_REMOVE_POISON # 43
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_REMOVE_POISON;
 new_spell->status = available;
 new_spell->name = strdup("remove poison");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_UNAFFECTS | MAG_ALTER_OBJS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (4 * self.level)) > 8 ? (40 - (4 * self.level)) : 8");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 18;
 new_spell->assign[0].num_mana = strdup(buf);
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 15;
 new_spell->assign[1].num_mana = strdup(buf);
    new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 18;
 new_spell->assign[2].num_mana = strdup(buf);
 new_spell->dispel[0] = strdup("33");  // remove poison
 new_spell->messages.to_self = strdup("$b steams briefly.");
 new_spell->messages.to_vict = strdup("A warm feeling runs through your body!");
 new_spell->messages.to_room = strdup("$N looks better."); 

 spedit_save_internally(new_spell);

 // SPELL_SENSE_LIFE # 44
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_SENSE_LIFE;
 new_spell->status = available;
 new_spell->name = strdup("sense life");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(20 - (2 * self.level)) > 10 ? (20 - (2 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 28;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 15;
 new_spell->assign[1].num_mana = strdup(buf);
  new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 29;
 new_spell->assign[2].num_mana = strdup(buf);
   new_spell->assign[3].class_num = CLASS_RANGER;
 new_spell->assign[3].level = 22;
 new_spell->assign[3].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_SENSE_LIFE + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("self.level");
 new_spell->messages.to_vict = strdup("Your feel your awareness improve.");
 new_spell->messages.wear_off = strdup("You feel less aware of your surroundings.");

 spedit_save_internally(new_spell);

 // SPELL_ANIMATE_DEAD # 45
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_ANIMATE_DEAD;
 new_spell->status = available;
 new_spell->name = strdup("animate dead");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_SUMMONS;
 new_spell->effectiveness = strdup("100");
 new_spell->summon_mob = strdup("11");
 sprintf(buf, "(35 - (3 * self.level)) > 10 ? (35 - (3 * self.level)) : 10");
   new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 100;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N animates a corpse!");

 spedit_save_internally(new_spell);

 // SPELL_DISPEL_GOOD # 46
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DISPEL_GOOD;
 new_spell->status = available;
 new_spell->name = strdup("dispel good");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (3 * self.level)) > 25 ? (40 - (3 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 20;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(6, 8) + 6");
 new_spell->max_dam = 100;

 spedit_save_internally(new_spell);

 // SPELL_GROUP_ARMOR # 47
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_GROUP_ARMOR;
 new_spell->status = available;
 new_spell->name = strdup("group armor");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_GROUPS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(50 - (2 * self.level)) > 30 ? (50 - (2 * self.level)) : 30");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 13;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_GROUP_HEAL # 48
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_GROUP_HEAL;
 new_spell->status = available;
 new_spell->name = strdup("group heal");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_GROUPS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(80 - (5 * self.level)) > 60 ? (80 - (5 * self.level)) : 60");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 32;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_GROUP_RECALL # 49
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_GROUP_RECALL;
 new_spell->status = available;
 new_spell->name = strdup("group recall");
 new_spell->type = SPELL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(80 - (5 * self.level)) > 60 ? (80 - (5 * self.level)) : 60");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 68;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_INFRAVISION # 50
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_INFRAVISION;
 new_spell->status = available;
 new_spell->name = strdup("infravision");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 sprintf(buf, "(25 - (1 * self.level)) > 10 ? (25 - (1 * self.level)) : 10");
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 4;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 10;
 new_spell->assign[1].num_mana = strdup(buf);
   new_spell->assign[2].class_num = CLASS_RANGER;
 new_spell->assign[2].level = 9;
 new_spell->assign[2].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_INFRAVISION + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("12 + param");
 new_spell->messages.to_vict = strdup("Your eyes glow red.");
 new_spell->messages.to_room = strdup("$N's eyes glow red.");
 new_spell->messages.wear_off = strdup("Your night vision seems to fade.");

 spedit_save_internally(new_spell);

 // SPELL_WATERWALK # 51
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_WATERWALK;
 new_spell->status = available;
 new_spell->name = strdup("waterwalk");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (2 * self.level)) > 20 ? (40 - (2 * self.level)) : 20");
 new_spell->applies[0].appl_num = AFF_WATERWALK + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("24");
 sprintf(buf, "(80 - (5 * self.level)) > 60 ? (80 - (5 * self.level)) : 60");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 70;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 40;
 new_spell->assign[1].num_mana = strdup(buf);
   new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 43;
 new_spell->assign[2].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("You feel webbing between your toes.");
 new_spell->messages.wear_off = strdup("Your feet seem less buoyant.");

 spedit_save_internally(new_spell);

 // SPELL_IDENTIFY # 52
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_IDENTIFY;
 new_spell->status = available;
 new_spell->name = strdup("identify");
 new_spell->function = spell_identify;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(50 - (5 * self.level)) > 25 ? (50 - (5 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 11;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_BARD;
 new_spell->assign[1].level = 20;
 new_spell->assign[1].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_FLY # 53
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_FLY;
 new_spell->status = available;
 new_spell->name = strdup("fly");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (2 * self.level)) > 20 ? (40 - (2 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 22;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_FLYING + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("You float above the ground.");
 new_spell->messages.wear_off = strdup("You drift slowly to the ground.");

 spedit_save_internally(new_spell);

 // SPELL_DARKNESS # 54
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DARKNESS;
 new_spell->status = available;
 new_spell->name = strdup("darkness");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_ROOMS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 12;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_self = strdup("You cast a shroud of darkness upon the area.");
 new_spell->messages.to_room = strdup("$N casts a shroud of darkness upon this area.");

 spedit_save_internally(new_spell);
 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_TRANSPORT_VIA_PLANTS;
 new_spell->status = available;
 new_spell->name = strdup("transport via plants");
 new_spell->function = spell_transport_via_plants;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 5;
 new_spell->assign[0].num_mana = strdup(buf);
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 7;
 new_spell->assign[1].num_mana = strdup(buf);

 spedit_save_internally(new_spell);
 
 // SPELL_PROTECTION_FROM_EVIL # 56
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_PROT_FROM_GOOD;
 new_spell->status = available;
 new_spell->name = strdup("protection from good");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (3 * self.level)) > 10 ? (40 - (3 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 8;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_PROTECT_GOOD + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("You feel invulnerable!");
 new_spell->messages.wear_off = strdup("You feel less protected.");

 spedit_save_internally(new_spell);

//stoneskin #53
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_STONESKIN;
 new_spell->status = available;
 new_spell->name = strdup("stoneskin");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (3 * self.level)) > 10 ? (40 - (3 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 34;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 90;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_STONESKIN + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você sente sua pele se tornando dura como rocha.");
 new_spell->messages.to_room = strdup("A pele de $n se torna mais dura.");
 new_spell->messages.wear_off = strdup("Sua pele perde a dureza.");

 spedit_save_internally(new_spell);

// SPELL_fireshield #54
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_FIRESHIELD;
 new_spell->status = available;
 new_spell->name = strdup("Fireshield");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 15 ? (30 - (3 * self.level)) : 15");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 31;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-5");
 new_spell->applies[0].duration = strdup("self.level");
 new_spell->messages.to_vict = strdup("Uma ardente aura vermelha envolve você.");
  new_spell->messages.to_room = strdup("Uma ardente aura vermelha envolve $n.");
 new_spell->messages.wear_off = strdup("A aura de fogo ao redor de seu corpo desaparece.");

 spedit_save_internally(new_spell);

 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_PORTAL;
 new_spell->status = available;
 new_spell->name = strdup("portal");
 new_spell->function = spell_portal;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_WORLD | TAR_NOT_SELF;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 90;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SKILL_BACKSTAB # 131 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_BACKSTAB;
 new_spell->status = available;
 new_spell->name = strdup("backstab");
 new_spell->function = do_backstab;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 3;


 spedit_save_internally(new_spell);

 // SKILL_BASH # 132 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_BASH;
 new_spell->status = available;
 new_spell->name = strdup("bash");
 new_spell->function = do_bash;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 12;

 spedit_save_internally(new_spell);

 // SKILL_HIDE # 133 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_HIDE;
 new_spell->status = available;
 new_spell->name = strdup("hide");
 new_spell->function = do_hide;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 5;
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 9;

 spedit_save_internally(new_spell);

 // SKILL_KICK # 224 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_KICK;
 new_spell->status = available;
 new_spell->name = strdup("kick");
 new_spell->function = do_kick;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 1;
  new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 1;

 spedit_save_internally(new_spell);

 // SKILL_PICK_LOCK # 135 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_PICK_LOCK;
 new_spell->status = available;
 new_spell->name = strdup("pick lock");
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 2;

 spedit_save_internally(new_spell);

 // SKILL_WHIRLWIND # 136 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_WHIRLWIND;
 new_spell->status = available;
 new_spell->name = strdup("whirlwind");
 new_spell->function = do_whirlwind;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 16;

 spedit_save_internally(new_spell);

 // SKILL_RESCUE # 137 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_RESCUE;
 new_spell->status = available;
 new_spell->name = strdup("rescue");
 new_spell->function = do_rescue;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 3;
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 5;

 spedit_save_internally(new_spell);

 // SKILL_SNEAK # 138 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_SNEAK;
 new_spell->status = available;
 new_spell->name = strdup("sneak");
 new_spell->function = do_sneak;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 1;

 spedit_save_internally(new_spell);

 // SKILL_STEAL # 139 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_STEAL;
 new_spell->status = available;
 new_spell->name = strdup("steal");
 new_spell->function = do_steal;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 4;

 spedit_save_internally(new_spell);

 // SKILL_TRACK # 140 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_TRACK;
 new_spell->status = available;
 new_spell->name = strdup("track");
 new_spell->function = do_track;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 6;
 new_spell->assign[1].class_num = CLASS_WARRIOR;
 new_spell->assign[1].level = 9;
   new_spell->assign[2].class_num = CLASS_RANGER;
 new_spell->assign[2].level = 5;

 spedit_save_internally(new_spell);

 // SKILL_BANDAGE # 141 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_BANDAGE;
 new_spell->status = available;
 new_spell->name = strdup("bandage");
 new_spell->function = do_bandage;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 7;

 spedit_save_internally(new_spell);

 /*SCAN*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_SCAN;
 new_spell->status = available;
 new_spell->name = strdup("scan");
 new_spell->function = do_scan;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_RANGER;
 new_spell->assign[0].level = 10;
  new_spell->assign[1].class_num = CLASS_THIEF;
 new_spell->assign[1].level = 7;

 spedit_save_internally(new_spell);

 /*MEDITATE */
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_MEDITATE;
 new_spell->status = available;
 new_spell->name = strdup("meditate");
 new_spell->function = do_meditate;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 12;
 new_spell->assign[1].class_num = CLASS_MAGIC_USER;
 new_spell->assign[1].level = 10;
 new_spell->assign[2].class_num = CLASS_CLERIC;
 new_spell->assign[2].level = 9;
 new_spell->assign[3].class_num = CLASS_RANGER;
 new_spell->assign[3].level = 8;
  new_spell->assign[4].class_num = CLASS_BARD;
 new_spell->assign[4].level = 56;

 spedit_save_internally(new_spell);
 
  /*TRIP*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_TRIP;
 new_spell->status = available;
 new_spell->name = strdup("trip");
 new_spell->function = do_trip;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 35;

 spedit_save_internally(new_spell);

  /*BACKFLIP*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_BACKFLIP;
 new_spell->status = available;
 new_spell->name = strdup("backflip");
 new_spell->function = do_backflip;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 24;

 spedit_save_internally(new_spell);

 /*COMBO*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_COMBO_ATTACK;
 new_spell->status = available;
 new_spell->name = strdup("combo attack");
 new_spell->function = do_combo;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 30;

 spedit_save_internally(new_spell);

 // SPELL_DG_AFFECT # 298 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DG_AFFECT;
 new_spell->status = available;
 new_spell->name = strdup("script-inflicted");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_SITTING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->effectiveness = strdup("100");

 spedit_save_internally(new_spell);



 spedit_save_to_disk();
}