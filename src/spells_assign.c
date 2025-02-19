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
ACMD(do_seize);
ACMD(do_spy);
ACMD(do_envenom);
ACMD(do_shoot);

void set_spells_function()
{
 struct str_spells *spell;

 log1("Assigning spells's function.");

 if ((spell = get_spell_by_vnum(SPELL_TELEPORT)))
   spell->function = spell_teleport;

 if ((spell = get_spell_by_vnum(SPELL_CHARM)))
   spell->function = spell_charm;
 
 if ((spell = get_spell_by_vnum(CHANSON_ENCANTO)))
    spell->function = chanson_encanto;

 if ((spell = get_spell_by_vnum(SPELL_CONTROL_WEATHER)))
   spell->function = spell_control_weather;

 if ((spell = get_spell_by_vnum(SPELL_CREATE_WATER)))
   spell->function = spell_create_water;
   
    if ((spell = get_spell_by_vnum(SPELL_CREATE_NECTAR)))
   spell->function = spell_create_nectar;

    if ((spell = get_spell_by_vnum(CHANSON_BRINDE)))
   spell->function = chanson_brinde;

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

 if ((spell = get_spell_by_vnum(CHANSON_VOLTAR)))
   spell->function = spell_recall;

 if ((spell = get_spell_by_vnum(SPELL_IDENTIFY)))
   spell->function = spell_identify;

 if ((spell = get_spell_by_vnum(SPELL_SCROLL_IDENTIFY)))
   spell->function = spell_identify;

 if ((spell = get_spell_by_vnum(SPELL_TRANSPORT_VIA_PLANTS)))
   spell->function = spell_transport_via_plants;

 if ((spell = get_spell_by_vnum(SPELL_PORTAL)))
   spell->function = spell_portal;
   
    if ((spell = get_spell_by_vnum(SPELL_RAISE_DEAD)))
   spell->function = spell_raise_dead;
   
     if ((spell = get_spell_by_vnum(SPELL_RESSURECT)))
   spell->function = spell_ressurect;
   
    if ((spell = get_spell_by_vnum(SPELL_YOUTH)))
   spell->function = spell_youth;
   
   if ((spell = get_spell_by_vnum(SPELL_VAMP_TOUCH)))
   spell->function = spell_vamp_touch;

   if ((spell = get_spell_by_vnum(SPELL_FURY_OF_AIR)))
   spell->function = spell_fury_air;

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
   
    if ((spell = get_spell_by_vnum(SKILL_SEIZE)))
   spell->function = do_seize;
   
   if ((spell = get_spell_by_vnum(SKILL_ENVENOM)))
   spell->function = do_envenom;
   
    if ((spell = get_spell_by_vnum(SKILL_SPY)))
   spell->function = do_spy;
   
   if ((spell = get_spell_by_vnum(SKILL_BOWS)))
   spell->function = do_shoot;
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
 new_spell->assign[0].class_num = CLASS_RANGER;
 new_spell->assign[0].level = 100;
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
 new_spell->messages.to_vict = strdup("Você se sente virtuos$r.");
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
 new_spell->mag_flags = MAG_AFFECTS | MAG_UNAFFECTS;
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
 new_spell->dispel[0] = strdup("162"); //Nao pode usar junto com a chanson
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
 new_spell->messages.wear_off = strdup("Você se sente mais auto-confiante.");

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
 new_spell->messages.to_vict = strdup("Você sente sua força indo embora!");
 new_spell->messages.wear_off = strdup("Você se sente retomando suas forças.");

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
 new_spell->effectiveness = strdup("50");
 new_spell->summon_mob = strdup("10");
// new_spell->summon_req = strdup("10221");
 sprintf(buf, "(130 - (3 * self.level)) > 120 ? (130 - (3 * self.level)) : 120 ");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 14;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N magicamente se divide!");

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
 new_spell->objects[0] = strdup("10");  // object VNUM 10 = waybread
 
 spedit_save_internally(new_spell);

// SPELL_CREATE_BERRIES#95
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CREATE_BERRIES;
 new_spell->status = available;
 new_spell->name = strdup("create berries");
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


 // SPELL_CREATE_NECTAR #88
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CREATE_NECTAR;
 new_spell->status = available;
 new_spell->name = strdup("create nectar");
 new_spell->function = spell_create_nectar;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_INV | TAR_OBJ_EQUIP;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 7;
 new_spell->assign[0].num_mana = strdup(buf);
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
 new_spell->messages.to_vict = strdup("Sua visão retorna!");
 new_spell->messages.to_room = strdup("Os olhos de $n brilham momentaneamente.");
 new_spell->dispel[0] = strdup("4");  // spell VNUM 4 = Blindness
 new_spell->dispel[1] = strdup("162");
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
 new_spell->messages.to_vict = strdup("Você se sente muito melhor!");
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
 new_spell->messages.to_vict = strdup("Você se sente melhor.");
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
 new_spell->applies[0].modifier = strdup("-1 * (1 + (self.level / 50))");
 new_spell->applies[0].duration = strdup("1 + (self.level / 10)");
 new_spell->applies[1].appl_num = APPLY_DAMROLL;
 new_spell->applies[1].modifier = strdup("-1 * (1 + (self.level / 50))");
 new_spell->applies[1].duration = strdup("1 + (self.level / 10)");
 new_spell->applies[2].appl_num = AFF_CURSE + NUM_APPLIES;
 new_spell->applies[2].duration = strdup("1 + (self.level / 10)");
 new_spell->messages.to_self = strdup("Você brilha avermelhado!");
 new_spell->messages.to_vict = strdup("Você sente um leve desconforto.");
 new_spell->messages.to_room = strdup("$N brilha avermelhado!");
 new_spell->messages.wear_off = strdup("Você se sente mais otimista.");

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
 new_spell->messages.to_vict = strdup("Você sente suas pupilas se dilatando.");
 new_spell->messages.wear_off = strdup("Você perde sua sensibilidade.");

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
  new_spell->assign[2].class_num = CLASS_RANGER;
 new_spell->assign[2].level = 20;
 new_spell->assign[2].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_DETECT_INVIS + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("12 + param");
 new_spell->messages.to_vict = strdup("Você sente suas pupilas se dilatando.");
 new_spell->messages.wear_off = strdup("Suas pupilas voltam ao normal.");

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
 new_spell->messages.to_vict = strdup("Você sente suas pupilas se dilatando.");
 new_spell->messages.wear_off = strdup("Suas pupilas voltam ao normal.");

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
 new_spell->messages.wear_off = strdup("A detecção de veneno se encerra.");	

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
 //new_spell->max_dam = 100;
 new_spell->messages.to_self = strdup("Você gesticula e a terra toda começa a tremer em a sua volta!");
 new_spell->messages.to_room = strdup("$n faz alguns gestos graciosos e a terra começa a tremer violentamente!");

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
 new_spell->assign[0].level = 65;
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
 new_spell->assign[1].level = 90;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->dispel[0] = strdup("4");
 new_spell->messages.to_vict = strdup("Uma calorosa sensação percorre o seu corpo.\r\n");

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
 new_spell->messages.to_self = strdup("$n desaparece.");
 new_spell->messages.to_vict = strdup("Você desaparece.");
 new_spell->messages.to_room = strdup("$n começa a ficar mais clar$r e acaba por desaparecer.");
 new_spell->messages.wear_off = strdup("Você se sente expost$r.");

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
  new_spell->assign[1].class_num = CLASS_RANGER;
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
 new_spell->messages.to_self = strdup("$b emite uma fumaça por alguns instantes.");
 new_spell->messages.to_vict = strdup("Você se sente doente.");
 new_spell->messages.to_room = strdup("$N fica muito doente!");
 new_spell->messages.wear_off = strdup("Você se sente menos doente.");

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
 new_spell->messages.to_vict = strdup("Você se sente protegid$r!");
 new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");

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
 new_spell->messages.to_self = strdup("$b brilha azul por alguns instantes.");
 new_spell->messages.to_vict = strdup("Você não se sente mais tão azarad$r.");

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
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_UNAFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(110 - (5 * self.level)) > 85 ? (110 - (5 * self.level)) : 85");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 21;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_SANCTUARY + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("4");
 new_spell->dispel[0] = strdup("91");
 new_spell->messages.to_vict = strdup("Uma aura branca $r envolve por instantes.");
 new_spell->messages.to_room = strdup("$N é envolvid$R por uma aura branca.");
 new_spell->messages.wear_off = strdup("A aura branca ao redor de seu corpo desaparece.");

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
 new_spell->messages.wear_off = strdup("Você se sente menos cansad$r.");
 new_spell->messages.to_vict = strdup("Você sente muito sono...  Zzzz......\r\n");
 new_spell->messages.to_room = strdup("$N dorme.");

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
 new_spell->messages.to_vict = strdup("Você se sente mais forte!");
 new_spell->messages.wear_off = strdup("Você se sente mais frac$r.");

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
  new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 15;
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
 sprintf(buf, "(30 - (3 * self.level)) > 15 ? (30 - (3 * self.level)) : 15");
/*new_spell->assign[0].class_num = CLASS_MAGIC_USER;
new_spell->assign[0].level = 109;
 new_spell->assign[0].num_mana = strdup(buf);
 */
 spedit_save_internally(new_spell) ;

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
 new_spell->messages.to_self = strdup("$p emite uma fumaça por alguns instantes.");
 new_spell->messages.to_vict = strdup("Uma calorosa sensação percorre o seu corpo!");
 new_spell->messages.to_room = strdup("$N aparenta estar melhor."); 

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
 new_spell->assign[3].level = 20;
 new_spell->assign[3].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_SENSE_LIFE + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("self.level");
 new_spell->messages.to_vict = strdup("Você sente sua sensibilidade aumentar.");
 new_spell->messages.wear_off = strdup("Você se sente menos sensível às coisas ao seu redor.");

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
 new_spell->effectiveness = strdup("50");
 new_spell->summon_mob = strdup("11");
 sprintf(buf, "(130 - (3 * self.level)) > 120 ? (130 - (3 * self.level)) : 120 ");
   new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 14;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N anima um corpo!!");

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
 new_spell->messages.to_vict = strdup("Sua visão fica avermelhada.");
 new_spell->messages.to_room = strdup("Os olhos de $N ficam vermelhos.");
 new_spell->messages.wear_off = strdup("Sua visão noturna parece desaparecer.");

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
 new_spell->applies[0].appl_num = AFF_WATERWALK + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("24");
 sprintf(buf, "(80 - (5 * self.level)) > 40 ? (80 - (5 * self.level)) : 40");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 70;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 40;
 new_spell->assign[1].num_mana = strdup(buf);
   new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 43;
 new_spell->assign[2].num_mana = strdup(buf);
   new_spell->assign[3].class_num = CLASS_RANGER;
 new_spell->assign[3].level = 32;
 new_spell->assign[3].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("Você sente um tipo de rede surgir sob seus pés.");
 new_spell->messages.wear_off = strdup("Seus pés parecem menos boiantes.");

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
 sprintf(buf, "(40 - (5 * self.level)) > 20 ? (50 - (5 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 11;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_BARD;
 new_spell->assign[1].level = 20;
 new_spell->assign[1].num_mana = strdup(buf);

 spedit_save_internally(new_spell);

 // SPELL_FLY #63
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
 sprintf(buf, "(80 - (3 * self.level)) > 40 ? (80 - (3 * self.level)) : 40");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 18;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 65;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 75;
 new_spell->assign[2].num_mana = strdup(buf);
 new_spell->assign[3].class_num = CLASS_RANGER;
 new_spell->assign[3].level = 42;
 new_spell->assign[3].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_DEX;
 new_spell->applies[0].modifier = strdup("1 + (param > 18)");
  new_spell->applies[0].duration = strdup("1");
   new_spell->applies[1].appl_num = AFF_LIGHT + NUM_APPLIES;
 new_spell->applies[1].duration = strdup("4");
 new_spell->messages.to_vict = strdup("Você se sente muito leve...");
 new_spell->messages.wear_off = strdup("Você se sente pesad$r novamente.");

 spedit_save_internally(new_spell);

 // SPELL_DARKNESS # 92
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
new_spell->assign[1].class_num = CLASS_CLERIC;                             new_spell->assign[1].level = 40;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 48;

 new_spell->messages.to_self = strdup("Você lança um manto de escuridão sobre a área.");
 new_spell->messages.to_room = strdup("$N lança um manto de escuridão sobre esta área.");

 spedit_save_internally(new_spell);
 
 
 //SPELL_TRANSPORT_VIA_PLANTS
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
 
 // SPELL_PROTECTION_FROM_GOOD # 94
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
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(200 - (10 * self.level)) > 100 ? (200 - (10 * self.level)) : 100");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 90;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_STONESKIN + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você sente sua pele se tornando dura como rocha.");
 new_spell->messages.to_room = strdup("A pele de $N se torna mais dura.");
 new_spell->messages.wear_off = strdup("Sua pele perde a dureza.");

 spedit_save_internally(new_spell);

// SPELL_fireshield #54
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_FIRESHIELD;
 new_spell->status = available;
 new_spell->name = strdup("fireshield");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(150 - (10 * self.level)) > 100 ? (150 - (10 * self.level)) : 100");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 31;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-5");
 new_spell->applies[0].duration = strdup("self.level");
new_spell->applies[1].appl_num = AFF_FIRESHIELD + NUM_APPLIES;
 new_spell->applies[1].duration = strdup("self.level"); 
 new_spell->messages.to_vict = strdup("Uma ardente aura vermelha envolve você.");
  new_spell->messages.to_room = strdup("Uma ardente aura vermelha envolve $N.");
 new_spell->messages.wear_off = strdup("A aura de fogo ao redor de seu corpo desaparece.");
 spedit_save_internally(new_spell);

  //spell portal 65
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
 sprintf(buf, "(160 - (5 * self.level)) > 90 ? (160 - (5 * self.level)) : 90");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 90;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);
 
   //spell vamp_touch
  CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_VAMP_TOUCH;
 new_spell->status = available;
 new_spell->name = strdup("vampiric touch");
 new_spell->function = spell_vamp_touch;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT ;
 new_spell->mag_flags = MAG_MANUAL | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (1 * self.level)) > 15 ? (30 - (1 * self.level)) : 30");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 15;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);
 
    //spell FURY_OF_AIR
  CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_FURY_OF_AIR;
 new_spell->status = available;
 new_spell->name = strdup("fury of air");
 new_spell->function = spell_fury_air;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT ;
 new_spell->mag_flags = MAG_MANUAL | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(200 - (10 * self.level)) > 100 ? (200 - (10 * self.level)) : 100");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 50;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);
 
 //spell_improved_armor 55
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_IMPROVED_ARMOR;
 new_spell->status = available;
 new_spell->name = strdup("improved armor");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(80 - (5 * self.level)) > 30 ? (80 - (5 * self.level)) : 30");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 18;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 80;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-80");
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você sente alguém $r protegendo.");
 new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");
 spedit_save_internally(new_spell);
 
 //spell_disintegrate 56
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_DISINTEGRATE;
 new_spell->status = available;
 new_spell->name = strdup("disintegrate");
 new_spell->type = SPELL;
  new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(90 - (10 * self.level)) > 50 ? (90 - (10 * self.level)) : 5");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 85;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup ("dice(3, self.class == 0 ? 8 : 6) + self.class == 0 ? 195 : 180");
 spedit_save_internally(new_spell);
 
  // SPELL_EVOKE_AIR_SERVANT # 57
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_EVOKE_AIR_SERVANT;
 new_spell->status = available;
 new_spell->name = strdup("evoke air servant");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_SUMMONS;
 new_spell->effectiveness = strdup("50");
 new_spell->summon_mob = strdup("19");
sprintf(buf, "(130 - (3 * self.level)) > 120 ? (130 - (3 * self.level)) : 120 ");
   new_spell->assign[0].class_num = CLASS_RANGER;
 new_spell->assign[0].level = 14;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N invoca um servo aéreo!!");

 spedit_save_internally(new_spell);
 
 // spell_talkdead #58
  CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SPELL_TALKDEAD;
 new_spell->status = available;
 new_spell->name = strdup("talk with dead");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags =
  TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(50 - (5 * self.level)) > 20 ? (50 - (5 * self.level)) : 20");
  new_spell->applies[0].appl_num = AFF_TALKDEAD + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("self.level");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 10;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("Agora você pode sentir o reino dos mortos.\r\n");
 new_spell->messages.wear_off = strdup("Você não consegue mais sentir o mundo dos mortos.");
 spedit_save_internally(new_spell);
 
  //breath
  CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SPELL_BREATH;
 new_spell->status = available;
 new_spell->name = strdup("breath");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags =
  TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 15 ? (30 - (3 * self.level)) : 15");
  new_spell->applies[0].appl_num = AFF_BREATH + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("2");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 7;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 6;
 new_spell->assign[1].num_mana = strdup(buf);
   new_spell->assign[2].class_num = CLASS_DRUID;
 new_spell->assign[2].level = 6;
 new_spell->assign[2].num_mana = strdup(buf);
 new_spell->assign[3].class_num = CLASS_RANGER;
 new_spell->assign[3].level = 5;
 new_spell->assign[3].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("Você sente que não precisa mais respirar.\r\n");
 new_spell->messages.wear_off = strdup("Você sente necessidade de respirar novamente.");
 spedit_save_internally(new_spell);
 
  // SPELL_Paralyze #64
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_PARALYSE;
 new_spell->status = available;
 new_spell->name = strdup("paralyze");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(50 - (3 * self.level)) > 20 ? (50 - (3 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_MAGIC_USER;
 new_spell->assign[0].level = 22;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_CLERIC;
 new_spell->assign[1].level = 26;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_PARALIZE + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("1");
 new_spell->messages.to_vict = strdup("Você foi paralisad$r!");
  new_spell->messages.to_room = strdup("$N foi paralisad$r!");
 new_spell->messages.wear_off = strdup("Você pode andar novamente.");
 spedit_save_internally(new_spell);
 
 // SPELL_aid #65
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_AID;
  new_spell->status = available;
 new_spell->name = strdup("aid");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR |  MAG_POINTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(180 - (5 * self.level)) > 120 ? (180 - (5 * self.level)) : 120");
  new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 45;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].duration = strdup("self.level/5");
 new_spell->applies[0].appl_num = APPLY_HIT;
 new_spell->applies[0].modifier = strdup("victim.maxhit/6<120?victim.maxhit/6 : 120");
  new_spell->messages.wear_off = strdup("Você sente seu corpo menos resistente.");
 new_spell->messages.to_vict = strdup("Você sente seu corpo mais resistente.");
  spedit_save_internally(new_spell);
 
 //spell_skin_like_wood 67
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_SKIN_LIKE_WOOD;
 new_spell->status = available;
 new_spell->name = strdup("skin like wood");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (5 * self.level)) > 10 ? (30 - (5 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 3;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-15");
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você se sente protegid$r pelo GRANDE CARVALHO.");
 new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");
 spedit_save_internally(new_spell);
 
  //spell_skin_like_rock 68
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_SKIN_LIKE_ROCK;
 new_spell->status = available;
 new_spell->name = strdup("skin like rock");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(70 - (5 * self.level)) > 15 ? (70 - (5 * self.level)) : 15");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 35;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-35");
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você se sente protegid$ pelas MONTANHAS DRAGONHELM.");
 new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");
 spedit_save_internally(new_spell);
 
   //spell_skin_like_steel 69
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_SKIN_LIKE_STEEL;
 new_spell->status = available;
 new_spell->name = strdup("skin like steel");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(100 - (5 * self.level)) > 20 ? (100 - (5 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 65;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-60");
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você se sente protegid$r pelo aço.");
 new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");
 spedit_save_internally(new_spell);
 
   //spell_skin_like_diamond 70
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_SKIN_LIKE_DIAMOND;
 new_spell->status = available;
 new_spell->name = strdup("skin like diamond");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(110 - (5 * self.level)) > 35 ? (110 - (5 * self.level)) : 35");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 90;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-90");
 new_spell->applies[0].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você se sente protegid$r como uma pedra preciosa.");
 new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");
 spedit_save_internally(new_spell);
 
    //spell_BURST_OF_FLAME 71
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_BURST_OF_FLAME;
 new_spell->status = available;
 new_spell->name = strdup("burst of flame");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(20 - (3 * self.level)) > 10 ? (20 - (3 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 1;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->damages = strdup ("dice(3, self.class == 4 ? 5 : 3) + self.class == 4 ? 6 : 4");
 new_spell->max_dam = 100;
 spedit_save_internally(new_spell);
 
     //spell_BURST_OF_FIRE 72
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_BURST_OF_FIRE;
 new_spell->status = available;
 new_spell->name = strdup("burst of fire");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(25 - (3 * self.level)) > 15 ? (25 - (3 * self.level)) : 25");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 5;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->damages = strdup ("dice(3, self.class == 4 ? 7 : 5) + self.class == 4 ? 9 : 6");
 new_spell->max_dam = 100;
 spedit_save_internally(new_spell);
 
      //spell_IGNITE 73
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_IGNITE;
 new_spell->status = available;
 new_spell->name = strdup("ignite");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 20 ? (30 - (3 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 9;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->damages = strdup ("dice(3, self.class == 4 ? 9 : 3) + self.class == 4 ? 13 : 9");
 new_spell->max_dam = 100;
 spedit_save_internally(new_spell);
 
      //spell_INVIGOR 74
CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
new_spell->vnum = SPELL_INVIGOR;
 new_spell->status = available;
 new_spell->name = strdup("invigor");
 new_spell->type = SPELL;
 TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_POINTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(40 - (3 * self.level)) > 15 ? (40 - (3 * self.level)) : 15");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 13;
 new_spell->assign[0].num_mana = strdup(buf);
   new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 15;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("Você se sente mais dispost$r.");
 new_spell->points.move = strdup("dice(1, 8) + 1 + (param / 4)");
 spedit_save_internally(new_spell);
 
  // SPELL_MINOR_HEALING	     75
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SPELL_MINOR_HEALING;
 new_spell->status = available;
 new_spell->name = strdup("minor healing");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_POINTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (2 * self.level)) > 10 ? (30 - (2 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 15;
 new_spell->assign[0].num_mana = strdup(buf);
  new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 18;
 new_spell->assign[1].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("Uma onda de calor atravessa seu corpo, fazendo você se sentir melhor.\r\n");
 new_spell->points.hp = strdup("dice(2, 8) + 2 + (param / 4)");
 spedit_save_internally(new_spell);
 
  //SPELL_LIGHTNING_BLAST	     76 
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_LIGHTNING_BLAST;
 new_spell->status = available;
 new_spell->name = strdup("lightning blast");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
 new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(150 - (40 * self.level)) > 75 ? (150 - (40 * self.level)) : 75");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 80;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("self.class == 4 ? dice(4, 7) + 195 : dice(4,4) + 180");
 new_spell->max_dam = 300;

 spedit_save_internally(new_spell);
 
   // SPELL_REGENERATION	     77
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SPELL_REGENERATION;
 new_spell->status = available;
 new_spell->name = strdup("regeneration");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_POINTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(60 - (3 * self.level)) > 15 ? (60 - (3 * self.level)) : 15");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 40;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_vict = strdup("Você sente o poder da natureza curando seu corpo.\r\n");
 new_spell->points.hp = strdup("dice(6, 8)");
  new_spell->points.move = strdup("10 + dice(2, 6)");
 spedit_save_internally(new_spell);
 
 // SPELL_THISTLECOAT #78
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_THISTLECOAT;
 new_spell->status = available;
 new_spell->name = strdup("thistlecoat");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 20 ? (30 - (3* self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 8;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-3");
 new_spell->applies[0].duration = strdup("13");
 new_spell->applies[1].appl_num = AFF_THISTLECOAT + NUM_APPLIES;
  new_spell->applies[1].duration = strdup("13");
 new_spell->messages.to_vict = strdup("Você é recobert$r por um casaco de espinhos!");
  new_spell->messages.to_room = strdup("Um casaco de espinhos recobre $N.");
 new_spell->messages.wear_off = strdup("Você percebe que seu casaco de espinhos se desfez.");
 spedit_save_internally(new_spell);
 
 // SPELL_CASCADE_HAIL	     79
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_CASCADE_HAIL;
 new_spell->status = available;
 new_spell->name = strdup("cascade of hail");
 new_spell->type = SPELL;
  new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_AREAS | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(62 - (4 * self.level)) > 57 ? (62 - (4 * self.level)) : 57");
  new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 17;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("12+rand(8)");
 new_spell->max_dam = 100;
 
 spedit_save_internally(new_spell);
 
 //SPELL_DANCE_FIREFLIES	     80
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_DANCE_FIREFLIES;
 new_spell->status = available;
 new_spell->name = strdup("dance of fireflies");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(70 - (4 * self.level)) > 65 ? (70 - (4 * self.level)) : 65");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 19;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_FIREFLIES + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("15 + (self.level / 2)");
 new_spell->applies[1].appl_num = AFF_INFRAVISION + NUM_APPLIES;
  new_spell->applies[1].duration = strdup("15 + (self.level / 2)");
 new_spell->messages.to_vict = strdup("Você percebe que pequenos vaga-lumes surgem e iluminam seu caminho!");
  new_spell->messages.to_room = strdup("Você se surpreende com a nuvem de pequenos vaga-lumes que apareceram para iluminar $n");
 new_spell->messages.wear_off = strdup("Você observa os vaga-lumes indo embora, um a um.");
 spedit_save_internally(new_spell);
 
 //SPELL_STINGING_SWARM	     80
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_STINGING_SWARM;
 new_spell->status = available;
 new_spell->name = strdup("stinging swarm");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(60 - (4 * self.level)) > 60? (65 - (4 * self.level)) : 60");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 25;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_STINGING + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("rand(3) + 3 + (self.level / 4);");
 new_spell->messages.to_vict = strdup("Um enxame de abelhas rodeia sua cabeça, cuidado!");
  new_spell->messages.to_room = strdup("Um enxame de abelhas rodeia a cabeça de $n!");
 new_spell->messages.wear_off = strdup("Você nota que o enxame já não é tão grande e que está se desfazendo.");
 spedit_save_internally(new_spell);
 
  // SPELL_EVOKE_CROW
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_EVOKE_CROW;
 new_spell->status = available;
 new_spell->name = strdup("evoke crow");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_SUMMONS;
 new_spell->effectiveness = strdup("50");
 new_spell->summon_mob = strdup("12");
// new_spell->summon_req = strdup("10221");
 sprintf(buf, "(130 - (3 * self.level)) > 120 ? (130 - (3 * self.level)) : 120 ");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 14;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N invoca um corvo para ajudar!");
 spedit_save_internally(new_spell);
 
 // SPELL_EVOKE_WOLF
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_EVOKE_WOLF;
 new_spell->status = available;
 new_spell->name = strdup("evoke wolf");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_SUMMONS;
 new_spell->effectiveness = strdup("50");
 new_spell->summon_mob = strdup("13");
// new_spell->summon_req = strdup("10221");
 sprintf(buf, "(190 - (3 * self.level)) > 150 ? (190 - (3 * self.level)) : 150 ");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 30;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N invoca um lobo para ajudar!");
 spedit_save_internally(new_spell);
 
 // SPELL_EVOKE_BEAR
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_EVOKE_BEAR;
 new_spell->status = available;
 new_spell->name = strdup("evoke bear");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_SUMMONS;
 new_spell->effectiveness = strdup("50");
 new_spell->summon_mob = strdup("14");
// new_spell->summon_req = strdup("10221");
 sprintf(buf, "(220 - (3 * self.level)) > 170 ? (220 - (3 * self.level)) : 170 ");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 60;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N invoca um urso para ajudar!");
 spedit_save_internally(new_spell);
 
  // SPELL_EVOKE_LION
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_EVOKE_LION;
 new_spell->status = available;
 new_spell->name = strdup("evoke lion");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_SUMMONS;
 new_spell->effectiveness = strdup("50");
 new_spell->summon_mob = strdup("15");
// new_spell->summon_req = strdup("10221");
 sprintf(buf, "(280 - (3 * self.level)) > 200 ? (280 - (3 * self.level)) : 200 ");
 new_spell->assign[0].class_num = CLASS_DRUID;
 new_spell->assign[0].level = 85;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->messages.to_room = strdup("$N invoca um leão para ajudar!");
 spedit_save_internally(new_spell);
 
 // SPELL_VOICE_EXPLOSION # 89
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_VOICE_EXPLOSION;
 new_spell->status = available;
 new_spell->name = strdup("voice explosion");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_AREAS | MAG_VIOLENT;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(70 - (3 * self.level)) > 20 ? (70 - (3 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 23;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->damages = strdup("dice(3,6) + param");
 new_spell->max_dam = 100;
 new_spell->messages.to_self = strdup("Com sua voz você emite um som de extrema violência, ferindo tudo que $r cerca!");
 new_spell->messages.to_room = strdup("$n emite com sua voz um som de violência extrema, ferindo a todos!");

 spedit_save_internally(new_spell);
  // SPELL_SOUNDBARRIER #90
   CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SPELL_SOUNDBARRIER;
 new_spell->status = available;
 new_spell->name = strdup("soundbarrier");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(100 - (5 * self.level)) > 20 ? (100 - (5 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 15;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_SAVING_SPELL;
  new_spell->applies[0].modifier = strdup("-(1 + (param / 40))");
 new_spell->applies[0].duration = strdup("24");
  new_spell->applies[1].appl_num = APPLY_SAVING_BREATH;
 new_spell->applies[1].modifier = strdup("-(2 + (param / 40))");
 new_spell->applies[1].duration = strdup("24");
 new_spell->applies[2].appl_num = APPLY_SAVING_ROD;
 new_spell->applies[2].modifier = strdup("-(1+ (param / 40))");
 new_spell->applies[2].duration = strdup("24");
 new_spell->applies[3].appl_num = APPLY_AC;
 new_spell->applies[3].modifier = strdup("self. level > (vict.level +10) ? -70 : -(vict.level -10)");
 new_spell->applies[3].duration = strdup("24");
 new_spell->applies[4].appl_num = AFF_SOUNDBARRIER + NUM_APPLIES;
 new_spell->applies[4].duration = strdup("24");
 new_spell->messages.to_vict = strdup("Você é envolvid$r por uma protetora barreira de som.");
 new_spell->messages.to_room = strdup("$N é envolvid$r por uma protetora barreira de som.");
 new_spell->messages.wear_off = strdup("Você percebe a barreira de som se dissipar.");
 spedit_save_internally(new_spell);
  
  // SPELL_GLOOMSHIELD # 91
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SPELL_GLOOMSHIELD;
 new_spell->status = available;
 new_spell->name = strdup("gloomshield");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_UNAFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(110 - (5 * self.level)) > 85 ? (110 - (5 * self.level)) : 85");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 21;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_GLOOMSHIELD + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("4");
  new_spell->applies[1].appl_num = APPLY_AC;
 new_spell->applies[1].modifier = strdup("-15");
 new_spell->applies[1].duration = strdup("4");
 new_spell->dispel[0] = strdup("36");
 new_spell->messages.to_vict = strdup("Um escudo de trevas resguarda o seu corpo.");
 new_spell->messages.to_room = strdup("Um escudo de trevas resguarda o corpo de $N.");
 new_spell->messages.wear_off = strdup("O escudo de trevas que $r resguardava se desvanece lentamente.");
 spedit_save_internally(new_spell);
 
  //spell raise_dead
  CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_RAISE_DEAD;
 new_spell->status = available;
 new_spell->name = strdup("raise dead");
 new_spell->function = spell_raise_dead;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(100 - (8 * self.level)) > 60 ? (100 - (8 * self.level)) : 60");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 16;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);
 
  //spell ressurect
  CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_RESSURECT;
 new_spell->status = available;
 new_spell->name = strdup("ressurect");
 new_spell->function = spell_ressurect;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_ROOM | TAR_CHAR_WORLD | TAR_NOT_SELF;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(180 - (10 * self.level)) > 80 ? (180 - (10 * self.level)) : 80");
 new_spell->assign[0].class_num = CLASS_CLERIC;
 new_spell->assign[0].level = 32;
 new_spell->assign[0].num_mana = strdup(buf);

 spedit_save_internally(new_spell);
 
// SPELL_WINDWALL #96
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_WINDWALL;
 new_spell->status = available;
 new_spell->name = strdup("windwall");
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
 new_spell->effectiveness = strdup("100");
sprintf(buf, "(80 - (4* self.level)) > 40 ? (80 - ( 4 * self.level)) : 40");
 new_spell->assign[0].class_num = CLASS_RANGER;
 new_spell->assign[0].level = 15;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_AC;
 new_spell->applies[0].modifier = strdup("-4");
 new_spell->applies[0].duration = strdup("self.level");
new_spell->applies[1].appl_num = AFF_WINDWALL + NUM_APPLIES;
 new_spell->applies[1].duration = strdup("self.level"); 
 new_spell->messages.to_vict = strdup("Uma parede de vento envolve você.");
  new_spell->messages.to_room = strdup("Uma parede de vento envolve $N.");
 new_spell->messages.wear_off = strdup("A parede de vento ao redor de seu corpo desaparece.");
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

 // SKILL_SEIZE # 238
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_SEIZE;
 new_spell->status = available;
 new_spell->name = strdup("seize");
 new_spell->function = do_seize;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 20;

 spedit_save_internally(new_spell);

 // SKILL_SPY # 237
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_SPY;
 new_spell->status = available;
 new_spell->name = strdup("spy");
 new_spell->function = do_spy;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 15;

 spedit_save_internally(new_spell);

 // SKILL_ENVENOM # 241
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_ENVENOM;
 new_spell->status = available;
 new_spell->name = strdup("envenom");
 new_spell->function = do_envenom;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 8;

 spedit_save_internally(new_spell);


/*SWORDS*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_WEAPON_SWORDS;
 new_spell->status = available;
 new_spell->name = strdup("swords");
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 3;
 spedit_save_internally(new_spell);
 
 /*Daggers*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_WEAPON_DAGGERS;
 new_spell->status = available;
 new_spell->name = strdup("daggers");
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 3;
new_spell->assign[1].class_num = CLASS_THIEF;
 new_spell->assign[1].level = 25;
new_spell->assign[2].class_num = CLASS_BARD;
 new_spell->assign[2].level = 4;
 new_spell->assign[3].class_num = CLASS_RANGER;
 new_spell->assign[3].level = 8;

 spedit_save_internally(new_spell);
 
 /*Whips*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_WEAPON_WHIPS;
 new_spell->status = available;
 new_spell->name = strdup("whips");
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 8;

 spedit_save_internally(new_spell);
 
 /*TALONOUS ARMS*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_WEAPON_TALONOUS_ARMS;
 new_spell->status = available;
 new_spell->name = strdup("talonous arms");
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 13;

 spedit_save_internally(new_spell);
 
 /*bludgeons*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_WEAPON_BLUDGEONS;
 new_spell->status = available;
 new_spell->name = strdup("bludgeons");
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 15;

 spedit_save_internally(new_spell);
 
 /*EXOTICS*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_WEAPON_EXOTICS;
 new_spell->status = available;
 new_spell->name = strdup("exotics");
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_WARRIOR;
 new_spell->assign[0].level = 20;
  new_spell->assign[1].class_num = CLASS_RANGER;
 new_spell->assign[1].level = 10;

 spedit_save_internally(new_spell);
 
  // SKILL_BOWS # 252
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SKILL_BOWS;
 new_spell->status = available;
 new_spell->name = strdup("bows");
 new_spell->function = do_shoot;
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
 new_spell->assign[0].class_num = CLASS_RANGER;
 new_spell->assign[0].level = 30;

 spedit_save_internally(new_spell);

 
// SKILL_ADAGIO # 247
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell); 
new_spell->vnum = SKILL_ADAGIO;
new_spell->status = available; 
new_spell->name = strdup("adagio");
new_spell->type = SKILL;
new_spell->effectiveness = strdup("100"); 
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 45;
new_spell->dispel[0] = strdup("248"); //Dispel Allegro
new_spell->min_pos = POS_FIGHTING;
new_spell->targ_flags = TAR_SELF_ONLY;
new_spell->mag_flags = MAG_AFFECTS | MAG_UNAFFECTS; 
new_spell->effectiveness = strdup("100");
new_spell->applies[0].appl_num = APPLY_STR;
new_spell->applies[0].modifier = strdup("(self.level/45)");
new_spell->applies[0].duration = strdup("6");
new_spell->applies[1].appl_num = APPLY_DAMROLL;
new_spell->applies[1].modifier = strdup("(self.level/45)");
new_spell->applies[1].duration = strdup("6");
new_spell->applies[2].appl_num = APPLY_HITROLL;
new_spell->applies[2].modifier = strdup("-(self.level/45)");
new_spell->applies[2].duration = strdup("6");
new_spell->applies[3].appl_num = APPLY_DEX;   
new_spell->applies[3].modifier = strdup("-(self.level/45)"); 
new_spell->applies[3].duration = strdup("6");
new_spell->applies[4].appl_num = AFF_ADAGIO + NUM_APPLIES;     
new_spell->applies[4].duration = strdup("6");
new_spell->messages.to_vict = strdup("Você tenta fazer com que seu corpo vibre num ritmo de adagio."); 
new_spell->messages.wear_off = strdup("Você para de vibrar em adagio.");
spedit_save_internally(new_spell);

// SKILL_ALLEGRO # 248
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell); 
new_spell->vnum = SKILL_ALLEGRO;
new_spell->status = available;   
new_spell->name = strdup("allegro");
new_spell->type = SKILL;  
new_spell->effectiveness = strdup("100");      
new_spell->min_pos = POS_FIGHTING;   
new_spell->targ_flags = TAR_SELF_ONLY;      
new_spell->mag_flags = MAG_AFFECTS | MAG_UNAFFECTS;    
new_spell->effectiveness = strdup("100");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 50;  
new_spell->dispel[0] = strdup("247"); //Dispel Adagio
new_spell->applies[0].appl_num = APPLY_STR;   
new_spell->applies[0].modifier = strdup("-(self.level/45)");
new_spell->applies[0].duration = strdup("6"); 
new_spell->applies[1].appl_num = APPLY_DAMROLL;
new_spell->applies[1].modifier = strdup("-(self.level/45)");  
new_spell->applies[1].duration = strdup("6");   
new_spell->applies[2].appl_num = APPLY_HITROLL; 
new_spell->applies[2].modifier = strdup("(self.level/45)");
new_spell->applies[2].duration = strdup("6"); 
new_spell->applies[3].appl_num = APPLY_DEX;  
new_spell->applies[3].modifier = strdup("(self.level/45)"); 
new_spell->applies[3].duration = strdup("6");
new_spell->applies[4].appl_num = AFF_ALLEGRO + NUM_APPLIES;
new_spell->applies[4].duration = strdup("6");  
new_spell->messages.to_vict = strdup("Você tenta fazer com que seu corpo vibre num ritmo de allegro.");    
new_spell->messages.wear_off = strdup("Você para de vibrar em allegro.");
spedit_save_internally(new_spell);


 /*NIGHTHAMMER*/
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = SKILL_NIGHTHAMMER;
 new_spell->status = available;
 new_spell->name = strdup("nighthammer");
 new_spell->type = SKILL;
 new_spell->effectiveness = strdup("100");
  new_spell->assign[0].class_num = CLASS_THIEF;
 new_spell->assign[0].level = 50;

 spedit_save_internally(new_spell);
 
 /*CHANSON_ARDOR #161 */
 CREATE(new_spell, struct str_spells, 1);                                    spedit_init_new_spell (new_spell);
 new_spell->vnum = CHANSON_ARDOR;
 new_spell->status = available;
 new_spell->name = strdup("ardor das chamas");
 new_spell->type = CHANSON;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_IGNORE; 
 new_spell->mag_flags = MAG_CREATIONS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(20 - (2 * self.level)) > 5 ? (20 - (2 * self.level)) : 5")
;
 new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 1;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->objects[0] = strdup("34");  // object VNUM 34
 spedit_save_internally(new_spell);

 /*CHANSON_OFUSCAR #162 */
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = CHANSON_OFUSCAR;
 new_spell->status = available;
 new_spell->name = strdup("luz que ofusca");
 new_spell->type = CHANSON;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
 new_spell->mag_flags = MAG_AFFECTS | MAG_UNAFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(30 - (3 * self.level)) > 10 ? (30 - (3 * self.level)) : 10");
 new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 9;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_HITROLL;
 new_spell->applies[0].modifier = strdup("-4");
 new_spell->applies[0].duration = strdup("3");
 new_spell->applies[1].appl_num = APPLY_DEX;
 new_spell->applies[1].modifier = strdup("-4");
 new_spell->applies[1].duration = strdup("3");
 new_spell->applies[2].appl_num = AFF_BLIND + NUM_APPLIES;
 new_spell->applies[2].duration = strdup("3");
 new_spell->dispel[0] = strdup("4");
 new_spell->messages.to_vict = strdup("Você está ceg$r!");
 new_spell->messages.to_room = strdup("$N parece estar ceg$r!");
 new_spell->messages.wear_off = strdup("Você volta a enxergar.");
 spedit_save_internally(new_spell);
 
/*CHANSON_NINAR #163 */
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = CHANSON_NINAR;
 new_spell->status = available;
 new_spell->name = strdup("cancao de ninar");
 new_spell->type = CHANSON;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(42 - (4 * self.level)) > 6 ? (42 - (4 * self.level)) : 6");
 new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 12;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = AFF_SLEEP + NUM_APPLIES;
 new_spell->applies[0].duration = strdup("5 + (self.level / 5)");
 new_spell->messages.to_vict = strdup("A cancao parece fazer todo seu corpo adormecer e...     Zzzz......");
 new_spell->messages.to_room = strdup("$N parece ficar sonolento!");
 new_spell->messages.wear_off = strdup("Você se sente menos sonolent$r.");
 spedit_save_internally(new_spell);
 
 // CHANSON_TREMOR # 164
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);
 new_spell->vnum = CHANSON_TREMOR;
 new_spell->status = available;
 new_spell->name = strdup("tremor");
 new_spell->type = CHANSON;
 new_spell->min_pos = POS_FIGHTING;
 new_spell->targ_flags = TAR_IGNORE;
 new_spell->mag_flags = MAG_AREAS | MAG_VIOLENT;  
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(60 - (4 * self.level)) > 20 ? (60 - (4 * self.level)) : 20");
 new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 72;
 new_spell->assign[0].num_mana = strdup(buf); 
 new_spell->damages = strdup("dice(3, 6) + param");
 //new_spell->max_dam = 100;
 new_spell->messages.to_self = strdup("Com sua cancao voce faz com que o chao a redor comece a tremer e fender-se!");
 new_spell->messages.to_room = strdup("Ao cantar sua cancao, $n faz com que o chao comece a tremer e fender-se!");
 spedit_save_internally(new_spell);

// CHANSON_ANDARILHO             165
CREATE(new_spell, struct str_spells, 1);                                    spedit_init_new_spell (new_spell);                                          new_spell->vnum = CHANSON_ANDARILHO;
new_spell->status = available;
new_spell->name = strdup("a balada do andarilho");
new_spell->type = CHANSON;
new_spell->min_pos = POS_FIGHTING;
new_spell->targ_flags = TAR_CHAR_ROOM;                                      new_spell->mag_flags = MAG_POINTS;
new_spell->effectiveness = strdup("100"); 
sprintf(buf, "(50 - (4 * self.level)) > 13 ? (50 - (4 * self.level)) : 13");                                                                            new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 28;
new_spell->assign[0].num_mana = strdup(buf);
new_spell->messages.to_vict = strdup("Você sente que a Balada do Andarilho $r faz recuperar forças.");
 new_spell->points.hp = strdup("4 + dice(2, 6)");
 new_spell->points.move = strdup("12 + dice(4, 2)");
 spedit_save_internally(new_spell);

// CHANSON_MORTE # 166
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = CHANSON_MORTE;
new_spell->status = available;
new_spell->name = strdup("o grito da morte");
new_spell->type = CHANSON;
new_spell->min_pos = POS_FIGHTING;
new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
new_spell->mag_flags = MAG_DAMAGE | MAG_VIOLENT;
new_spell->effectiveness = strdup("100");
sprintf(buf, "(100 - (4 * self.level)) > 45 ? (100 - (4 * self.level)) : 45");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 40;
new_spell->assign[0].num_mana = strdup(buf);                                new_spell->damages = strdup("dice(9,9) + 20");
//new_spell->max_dam = 100;
new_spell->messages.to_self = strdup("Morte, o Ceifeiro Cruel responde ao seu chamado causando dano a $N!");
new_spell->messages.to_room = strdup("Morte, o Ceifeiro Cruel responde ao chamado de $n e causa dano a $N!");
new_spell->messages.to_vict = strdup("A propria Morte aparece e causa dano a voce!");
spedit_save_internally(new_spell);

// CHANSON_LEVEZA # 167 
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell); 
new_spell->vnum = CHANSON_LEVEZA;
new_spell->status = available;
new_spell->name = strdup("leveza das aguas");
new_spell->type = CHANSON;
new_spell->min_pos = POS_STANDING;
new_spell->targ_flags = TAR_CHAR_ROOM;
new_spell->mag_flags = MAG_AFFECTS;
new_spell->effectiveness = strdup("100");
sprintf(buf, "(80 - (5 * self.level)) > 40 ? (80 - (5 * self.level)) : 40");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 18;
new_spell->assign[0].num_mana = strdup(buf);
new_spell->applies[0].appl_num = AFF_WATERWALK + NUM_APPLIES;
new_spell->applies[0].duration = strdup("24");
new_spell->messages.to_vict = strdup("Você sente que seus pés podem flutuar sobre a água.");
new_spell->messages.wear_off = strdup("Seus pés parecem mais pesados.");
spedit_save_internally(new_spell);

// chanson_ardor #168 rever tabela


// CHANSON_DEUSES  169
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = CHANSON_DEUSES;
new_spell->status = available;
new_spell->name = strdup("invocacao aos deuses");
new_spell->type = CHANSON;
new_spell->min_pos = POS_FIGHTING;
new_spell->targ_flags = TAR_CHAR_ROOM;                                      new_spell->mag_flags = MAG_POINTS;                                          new_spell->effectiveness = strdup("100");
sprintf(buf, "(90 - (5 * self.level)) > 50 ? (90 - (5 * self.level)) : 50");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 95;
new_spell->assign[0].num_mana = strdup(buf);
new_spell->messages.to_vict = strdup("Os ecos de uma canção divina preenchem sua cabeça e curam seu corpo.\r\n");
sprintf(buf, "(dice(3,6) + (param - 15)) > 1 ? (dice(3,6) + (param - 15) : 1");
new_spell->points.hp = strdup(buf);
spedit_save_internally(new_spell);

//CHANSON_VULNERAVEL 170
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = CHANSON_VULNERAVEL;
new_spell->status = available;
new_spell->name = strdup("vulnerabilidade");
new_spell->type = CHANSON;
new_spell->min_pos = POS_STANDING;
new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
new_spell->mag_flags = MAG_AFFECTS | MAG_UNAFFECTS;
new_spell->effectiveness = strdup("100");
sprintf(buf, "(50 - (3 * self.level)) > 20 ? (50 - (3 * self.level)) : 20");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 68;
new_spell->assign[0].num_mana = strdup(buf);
new_spell->applies[0].appl_num = APPLY_SAVING_PARA + NUM_APPLIES;
new_spell->applies[0].duration = strdup("24");
new_spell->applies[0].modifier = strdup("3");
new_spell->applies[1].appl_num = APPLY_SAVING_ROD + NUM_APPLIES;
new_spell->applies[1].duration = strdup("24");
new_spell->applies[1].modifier = strdup("2");
new_spell->applies[2].appl_num = APPLY_SAVING_PETRI + NUM_APPLIES;
new_spell->applies[2].duration = strdup("24"); 
new_spell->applies[2].modifier = strdup("2");
new_spell->applies[3].appl_num = APPLY_SAVING_BREATH + NUM_APPLIES;
new_spell->applies[3].duration = strdup("24"); 
new_spell->applies[3].modifier = strdup("2");
new_spell->applies[4].appl_num = APPLY_SAVING_SPELL + NUM_APPLIES;
new_spell->applies[4].duration = strdup("24");     
new_spell->applies[4].modifier = strdup("3");
new_spell->dispel[0] = strdup("174");
new_spell->messages.to_vict = strdup("Você se sente muito vulnerável.");
new_spell->messages.wear_off = strdup("Você se sente menos vulnerável.");
spedit_save_internally(new_spell);

//CHANSON_ECOS

CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = CHANSON_ECOS;
new_spell->status = available; 
new_spell->name = strdup("ecos da maldicao");
new_spell->type = CHANSON;
new_spell->min_pos = POS_STANDING;
new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
new_spell->mag_flags = MAG_AFFECTS | MAG_POINTS | MAG_ACCDUR;
new_spell->effectiveness = strdup("100");
sprintf(buf, "(70 - (4 * self.level)) > 30 ? (70 - (4 * self.level)) : 40");
 new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 82;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_DEX;
 new_spell->applies[0].modifier = strdup("-(1+self.level/25)");
 new_spell->applies[0].duration = strdup("1+(self.level/10)");
 new_spell->applies[1].appl_num = APPLY_HITROLL;
 new_spell->applies[1].modifier = strdup("-(2+self.level/25)");
 new_spell->applies[1].duration = strdup("1+(self.level/10)");
 new_spell->points.move = strdup("-(50 + (2*self.level))");
 spedit_save_internally(new_spell);

//CHANSON_CLAMOR 174
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell); 
new_spell->vnum = CHANSON_CLAMOR;  
new_spell->status = available;    
new_spell->name = strdup("clamor");
new_spell->type = CHANSON;  
new_spell->min_pos = POS_STANDING;    
new_spell->targ_flags = TAR_CHAR_ROOM;
new_spell->mag_flags = MAG_AFFECTS | MAG_UNAFFECTS;   
new_spell->effectiveness = strdup("100");    
sprintf(buf, "(40 - (3 * self.level)) > 10 ? (40 - (3 * self.level)) : 10");
new_spell->assign[0].class_num = CLASS_BARD;   
new_spell->assign[0].level = 52;    
new_spell->assign[0].num_mana = strdup(buf);    
new_spell->applies[0].appl_num = APPLY_SAVING_PARA + NUM_APPLIES;  
new_spell->applies[0].duration = strdup("24");           
new_spell->applies[0].modifier = strdup("-3");
new_spell->applies[1].appl_num = APPLY_SAVING_ROD + NUM_APPLIES;  
new_spell->applies[1].duration = strdup("24");     
new_spell->applies[1].modifier = strdup("-2");
new_spell->applies[2].appl_num = APPLY_SAVING_PETRI + NUM_APPLIES;
new_spell->applies[2].duration = strdup("24");   
new_spell->applies[2].modifier = strdup("-2");   
new_spell->applies[3].appl_num = APPLY_SAVING_BREATH + NUM_APPLIES;  
new_spell->applies[3].duration = strdup("24");    
new_spell->applies[3].modifier = strdup("+2");   
new_spell->applies[4].appl_num = APPLY_SAVING_SPELL + NUM_APPLIES;   
new_spell->applies[4].duration = strdup("24");   
new_spell->applies[4].modifier = strdup("-3");    
new_spell->dispel[0] = strdup("170");
new_spell->messages.to_vict = strdup("Você se sente menos vulnerável.");
new_spell->messages.wear_off = strdup("Você se sente mais vulnerável.");
spedit_save_internally(new_spell);

//CHANSON_CESSAR #176
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = CHANSON_CESSAR;
new_spell->status = available;
new_spell->name = strdup("cancao do bardo amigo");
new_spell->type = CHANSON;
new_spell->min_pos = POS_FIGHTING;
new_spell->targ_flags = TAR_CHAR_ROOM;
new_spell->mag_flags = MAG_POINTS;
new_spell->effectiveness = strdup("100");
sprintf(buf, "(30 - (2 * self.level)) > 10 ? (30 - (2 * self.level)) : 10");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 42;
new_spell->assign[0].num_mana = strdup(buf);
new_spell->messages.to_vict = strdup("A canção parece curar suas feridas!.");
new_spell->points.hp = strdup("dice(5, 3) + (param / 4)");
spedit_save_internally(new_spell);

//CHANSON_VISAO 175
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = CHANSON_VISAO;
new_spell->status = available;
new_spell->name = strdup("visao de artista");
new_spell->type = CHANSON;
new_spell->min_pos = POS_STANDING;
new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
new_spell->effectiveness = strdup("100");
sprintf(buf, "(20 - (2 * self.level)) > 10 ? (20 - (2 * self.level)) : 10");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 59;   
new_spell->assign[0].num_mana = strdup(buf);
new_spell->applies[0].appl_num = AFF_DETECT_INVIS + NUM_APPLIES;
new_spell->applies[0].duration = strdup("12 + param");                      new_spell->messages.to_vict = strdup("Você se sente mais sensível ao mundo exterior.");
new_spell->messages.wear_off = strdup("Você se sente menos sensível.");
spedit_save_internally(new_spell);

//CHANSON_ALENTO
CREATE(new_spell, struct str_spells, 1);                        
spedit_init_new_spell (new_spell); 
new_spell->vnum = CHANSON_ALENTO;
new_spell->status = available;
 new_spell->name = strdup("alento de guerra");
 new_spell->type = CHANSON;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM;
 new_spell->mag_flags = MAG_AFFECTS;
 new_spell->effectiveness = strdup("100");
 sprintf(buf, "(270 - (15 * self.level)) > 115 ? (270 - (15 * self.level)) : 115");
 new_spell->assign[0].class_num = CLASS_BARD;
 new_spell->assign[0].level = 77;
 new_spell->assign[0].num_mana = strdup(buf);
 new_spell->applies[0].appl_num = APPLY_HITROLL;
 new_spell->applies[0].modifier = strdup("2");
 new_spell->applies[0].duration = strdup("param/4");
 new_spell->applies[1].appl_num = APPLY_DAMROLL;
 new_spell->applies[1].modifier = strdup("2");
 new_spell->applies[1].duration = strdup("param/4");
 new_spell->applies[2].appl_num = APPLY_STR;
 new_spell->applies[2].modifier = strdup("2");
 new_spell->applies[2].duration = strdup("param/4");
 new_spell->applies[3].appl_num = APPLY_DEX;
 new_spell->applies[3].modifier = strdup("1");
 new_spell->applies[3].duration = strdup("param/");
new_spell->applies[4].appl_num = APPLY_CON;
new_spell->applies[4].modifier = strdup("1");
new_spell->applies[4].duration = strdup("param/4");
 new_spell->messages.to_vict = strdup("Uma grande força parece tormar conta de seu coração.");
 new_spell->messages.to_room = strdup("$N!");
 new_spell->messages.wear_off = strdup("Você volta a enxergar.");


/*CHANSONS ESPECIAIS: BRINDE, VOLTAR, ENCANTO, ALENTO*/
// CHANSON_BRINDE #88
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell); 
new_spell->vnum = CHANSON_BRINDE;
new_spell->status = available;
new_spell->name = strdup("vinho para brindar");
new_spell->function = chanson_brinde;
new_spell->type = CHANSON;
new_spell->min_pos = POS_STANDING;   
new_spell->targ_flags = TAR_OBJ_INV | TAR_OBJ_EQUIP; 
new_spell->mag_flags = MAG_MANUAL;
new_spell->effectiveness = strdup("100");     
sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");  new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 6;
new_spell->assign[0].num_mana = strdup(buf);
spedit_save_internally(new_spell);

// CHANSON_ENCANTO #7
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = CHANSON_ENCANTO;
new_spell->status = available;
new_spell->name = strdup("voz do encanto");
new_spell->function = chanson_encanto;
new_spell->type = CHANSON;
new_spell->min_pos = POS_FIGHTING;
new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
new_spell->mag_flags = MAG_MANUAL | MAG_VIOLENT;
new_spell->effectiveness = strdup("100");
sprintf(buf, "(80 - (5 * self.level)) > 50 ? (80 - (5 * self.level)) : 50");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 62;
new_spell->assign[0].num_mana = strdup(buf);
new_spell->messages.wear_off = strdup("Você se sente mais auto-confiante.");

spedit_save_internally(new_spell);

// CHANSON_VOLTAR 
CREATE(new_spell, struct str_spells, 1);
spedit_init_new_spell (new_spell);
new_spell->vnum = CHANSON_VOLTAR;
new_spell->status = available;
new_spell->name = strdup("o bardo quer voltar");
new_spell->function = spell_recall;
new_spell->type = CHANSON;
new_spell->min_pos = POS_FIGHTING;
new_spell->targ_flags = TAR_CHAR_ROOM;
new_spell->mag_flags = MAG_MANUAL;
new_spell->effectiveness = strdup("100"); 
sprintf(buf, "(30 - (3 * self.level)) > 10 ? (30 - (3 * self.level)) : 10");
new_spell->assign[0].class_num = CLASS_BARD;
new_spell->assign[0].level = 31;
new_spell->assign[0].num_mana = strdup(buf);
spedit_save_internally(new_spell);

//spellnum, spellname, maxmana, minmana, manachng, minpos, targets, viol   ent?, routines.

 // SPELL_SCROLL_IDENTIFY # 52
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_SCROLL_IDENTIFY;
 new_spell->status = available;
 new_spell->name = strdup("scroll identify");
 new_spell->function = spell_identify;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
 spedit_save_internally(new_spell);

  //SPELL_YOUTH
 CREATE(new_spell, struct str_spells, 1);
 spedit_init_new_spell (new_spell);

 new_spell->vnum = SPELL_YOUTH;
 new_spell->status = available;
 new_spell->name = strdup("youth");
 new_spell->function = spell_youth;
 new_spell->type = SPELL;
 new_spell->min_pos = POS_STANDING;
 new_spell->targ_flags = TAR_OBJ_ROOM;
 new_spell->mag_flags = MAG_MANUAL;
 new_spell->effectiveness = strdup("100");
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
