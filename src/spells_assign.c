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
ACMD(do_peek);
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
ACMD(do_mine);
ACMD(do_fishing);
ACMD(do_forage);
ACMD(do_eavesdrop);
ACMD(do_taint);

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

    if ((spell = get_spell_by_vnum(SPELL_BLESS_OBJECT)))
        spell->function = spell_bless_object;

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

    if ((spell = get_spell_by_vnum(SPELL_STONESKIN)))
        spell->function = spell_stoneskin;

    if ((spell = get_spell_by_vnum(SPELL_VENTRILOQUATE)))
        spell->function = spell_ventriloquate;

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

    if ((spell = get_spell_by_vnum(SKILL_PEEK)))
        spell->function = do_peek;

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

    if ((spell = get_spell_by_vnum(SKILL_MINE)))
        spell->function = do_mine;

    if ((spell = get_spell_by_vnum(SKILL_FISHING)))
        spell->function = do_fishing;

    if ((spell = get_spell_by_vnum(SKILL_FORAGE)))
        spell->function = do_forage;

    if ((spell = get_spell_by_vnum(SKILL_EAVESDROP)))
        spell->function = do_eavesdrop;

    if ((spell = get_spell_by_vnum(SKILL_TAINT_FLUID)))
        spell->function = do_taint;
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
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ABJURATION;  /* Protective spell */
    new_spell->element = ELEMENT_UNDEFINED; /* No elemental nature */
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-20");
    new_spell->applies[0].duration = strdup("24");
    new_spell->messages.to_vict = strdup("Você sente alguém $r protegendo.");
    new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");

    spedit_save_internally(new_spell);

    // SPELL_TELEPORT #2
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Transportation magic */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure teleportation */

    spedit_save_internally(new_spell);

    // SPELL_BLESS_PERSON #3
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_BLESS_PERSON;
    new_spell->status = available;
    new_spell->name = strdup("bless person");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(35 - (3 * self.level)) > 5 ? (35 - (3 * self.level)) : 5");
    new_spell->assign[0].class_num = CLASS_CLERIC;
    new_spell->assign[0].level = 6;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->applies[0].appl_num = APPLY_HITROLL;
    sprintf(buf, "1 + (1 + (self.level / 30))");
    new_spell->applies[0].modifier = strdup(buf);
    new_spell->applies[0].duration = strdup("6");
    new_spell->applies[1].appl_num = APPLY_SAVING_SPELL;
    sprintf(buf, "-(1 + (self.level / 30))");
    new_spell->applies[1].modifier = strdup(buf);
    new_spell->applies[1].duration = strdup("6");
    new_spell->messages.to_self = strdup("$N brilha por alguns instantes.");
    new_spell->messages.to_vict = strdup("Você se sente virtuos$r.");
    new_spell->messages.wear_off = strdup("Você se sente menos abençoad$r.");
    new_spell->school = SCHOOL_ABJURATION; /* Protective blessing */
    new_spell->element = ELEMENT_HOLY;     /* Divine blessing */

    spedit_save_internally(new_spell);

    // SPELL_BLINDNESS #4
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->dispel[0] = strdup("162");   // Nao pode usar junto com a chanson
    new_spell->messages.to_vict = strdup("Você está ceg$r!");
    new_spell->messages.to_room = strdup("$N parece estar ceg$r!");
    new_spell->messages.wear_off = strdup("Você volta a enxergar.");
    new_spell->school = SCHOOL_NECROMANCY;  /* Debilitating curse */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical effect */

    spedit_save_internally(new_spell);

    // SPELL_BURNING_HANDS #5
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->damages = strdup("dice(3, self.class == 0 ? 8 : 6) + 3");
    new_spell->max_dam = 100;
    new_spell->school = SCHOOL_EVOCATION; /* Damage spell */
    new_spell->element = ELEMENT_FIRE;    /* Fire damage */

    spedit_save_internally(new_spell);

    // SPELL_CALL_LIGHTNING #6
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_EVOCATION;   /* Damage spell */
    new_spell->element = ELEMENT_LIGHTNING; /* Lightning damage */

    spedit_save_internally(new_spell);

    // SPELL_CHARM_PERSON #7
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ENCHANTMENT; /* Mind-affecting spell */
    new_spell->element = ELEMENT_MENTAL;    /* Mental manipulation */

    spedit_save_internally(new_spell);

    // SPELL_CHILL_TOUCH #8
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_NECROMANCY; /* Death/negative energy */
    new_spell->element = ELEMENT_ICE;      /* Cold damage */

    spedit_save_internally(new_spell);

    // SPELL_CLONE #9
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Creates duplicate */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical creation */

    spedit_save_internally(new_spell);

    // SPELL_COLOR_SPRAY #10
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ILLUSION;    /* Visual/light-based attack */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical effect */

    spedit_save_internally(new_spell);

    // SPELL_CONTROL_WEATHER #11
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ALTERATION; /* Controls weather patterns */
    new_spell->element = ELEMENT_AIR;      /* Weather/air manipulation */

    spedit_save_internally(new_spell);

    // SPELL_CREATE_FOOD #12
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->objects[0] = strdup("10");   // object VNUM 10 = waybread
    new_spell->school = SCHOOL_CONJURATION; /* Creates physical matter */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure creation */

    spedit_save_internally(new_spell);

    // SPELL_CREATE_BERRIES#95
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_CREATE_BERRIES;
    new_spell->status = available;
    new_spell->name = strdup("create berries");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_CREATIONS;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
    new_spell->assign[0].class_num = CLASS_DRUID;
    new_spell->assign[0].level = 7;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->assign[1].class_num = CLASS_RANGER;
    new_spell->assign[1].level = 12;
    new_spell->assign[1].num_mana = strdup(buf);
    new_spell->objects[0] = strdup("11");   // object VNUM 11 = morango

    spedit_save_internally(new_spell);

    // SPELL_CREATE_WATER #13
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Creates something */
    new_spell->element = ELEMENT_WATER;     /* Water-based */

    spedit_save_internally(new_spell);

    // SPELL_CREATE_NECTAR #88
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Creation magic */
    new_spell->element = ELEMENT_WATER;     /* Liquid creation */
    spedit_save_internally(new_spell);

    // SPELL_CURE_BLIND # 14
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->dispel[0] = strdup("4");   // spell VNUM 4 = Blindness
    new_spell->dispel[1] = strdup("162");
    new_spell->school = SCHOOL_ABJURATION; /* Removes affliction */
    new_spell->element = ELEMENT_HOLY;     /* Divine healing */
    spedit_save_internally(new_spell);

    // SPELL_CURE_CRITIC # 15
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Healing magic */
    new_spell->element = ELEMENT_HOLY;      /* Divine healing */

    spedit_save_internally(new_spell);

    // SPELL_CURE_LIGHT # 16
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Healing magic */
    new_spell->element = ELEMENT_HOLY;      /* Divine healing */

    spedit_save_internally(new_spell);

    // SPELL_CURSE # 17
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ENCHANTMENT; /* Mind-affecting malevolent */
    new_spell->element = ELEMENT_UNHOLY;    /* Dark magic */

    spedit_save_internally(new_spell);

    // SPELL_DETECT_ALIGNMENT # 18
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_DIVINATION; /* Reveals information */
    new_spell->element = ELEMENT_HOLY;     /* Divine insight */

    spedit_save_internally(new_spell);

    // SPELL_DETECT_INVIS # 19
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_DIVINATION;  /* Reveals hidden things */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical sight */

    spedit_save_internally(new_spell);

    // SPELL_DETECT_MAGIC # 20
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_DIVINATION;  /* Detection magic */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure divination */

    spedit_save_internally(new_spell);

    // SPELL_DETECT_POISON # 21
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_DIVINATION; /* Reveals information */
    new_spell->element = ELEMENT_POISON;   /* Poison detection */

    spedit_save_internally(new_spell);

    // SPELL_DISPEL_EVIL # 22
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ABJURATION; /* Protective/banishing magic */
    new_spell->element = ELEMENT_HOLY;     /* Divine power against evil */

    spedit_save_internally(new_spell);

    // SPELL_EARTHQUAKE # 23
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EARTHQUAKE;
    new_spell->status = available;
    new_spell->name = strdup("earthquake");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_DAMAGE | MAG_AREAS | MAG_VIOLENT;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(40 - (3 * self.level)) > 25 ? (40 - (3 * self.level)) : 25");
    new_spell->assign[0].class_num = CLASS_CLERIC;
    new_spell->assign[0].level = 18;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->assign[1].class_num = CLASS_DRUID;
    new_spell->assign[1].level = 70;
    new_spell->assign[1].num_mana = strdup(buf);
    new_spell->damages = strdup("dice(2, 10) + param");
    new_spell->max_dam = 300;
    new_spell->messages.to_self = strdup("Você gesticula e a terra toda começa a tremer em a sua volta!");
    new_spell->messages.to_room = strdup("$n faz alguns gestos graciosos e a terra começa a tremer violentamente!");
    new_spell->school = SCHOOL_EVOCATION; /* Area damage spell */
    new_spell->element = ELEMENT_EARTH;   /* Earth-based damage */

    spedit_save_internally(new_spell);

    // SPELL_ENCHANT_WEAPON # 24
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ENCHANTMENT; /* Magical enhancement */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical effect */

    spedit_save_internally(new_spell);

    // SPELL_ENERGY_DRAIN # 25
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_EVOCATION; /* Damage spell */
    new_spell->element = ELEMENT_UNHOLY;  /* Negative energy */

    spedit_save_internally(new_spell);

    // SPELL_FIREBALL # 26
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_EVOCATION; /* Damage spell */
    new_spell->element = ELEMENT_FIRE;    /* Fire damage */

    spedit_save_internally(new_spell);

    // SPELL_HARM # 27
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_NECROMANCY; /* Negative energy */
    new_spell->element = ELEMENT_UNHOLY;   /* Dark divine power */

    spedit_save_internally(new_spell);

    // SPELL_HEAL # 28
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Healing magic */
    new_spell->element = ELEMENT_HOLY;      /* Divine healing */

    spedit_save_internally(new_spell);

    // SPELL_INVISIBILITY # 29
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ILLUSION;    /* Concealment magic */
    new_spell->element = ELEMENT_UNDEFINED; /* No elemental nature */

    spedit_save_internally(new_spell);

    // SPELL_LIGHTNING_BOLT # 30
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_EVOCATION;   /* Damage spell */
    new_spell->element = ELEMENT_LIGHTNING; /* Lightning damage */

    spedit_save_internally(new_spell);

    // SPELL_LOCATE_OBJECT # 31
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_DIVINATION;  /* Detection/location spell */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure divination magic */

    spedit_save_internally(new_spell);

    // SPELL_MAGIC_MISSILE # 32
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_EVOCATION;   /* Pure magical damage */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure arcane force */

    spedit_save_internally(new_spell);

    // SPELL_POISON # 33
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_NECROMANCY; /* Death/poison magic */
    new_spell->element = ELEMENT_POISON;   /* Poison damage */

    spedit_save_internally(new_spell);

    // SPELL_PROTECTION_FROM_EVIL # 34
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ABJURATION; /* Protection spell */
    new_spell->element = ELEMENT_HOLY;     /* Divine protection */

    spedit_save_internally(new_spell);

    // SPELL_REMOVE_CURSE # 35
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->dispel[0] = strdup("17");   // dispel curse
    new_spell->messages.to_self = strdup("$b brilha azul por alguns instantes.");
    new_spell->messages.to_vict = strdup("Você não se sente mais tão azarad$r.");
    new_spell->school = SCHOOL_ABJURATION; /* Removal/cleansing spell */
    new_spell->element = ELEMENT_HOLY;     /* Divine restoration */

    spedit_save_internally(new_spell);

    // SPELL_SANCTUARY # 36
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SPELL_SANCTUARY;
    new_spell->status = available;
    new_spell->name = strdup("sanctuary");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_UNAFFECTS | MAG_AURA;
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
    new_spell->school = SCHOOL_ABJURATION; /* Protective spell */
    new_spell->element = ELEMENT_HOLY;     /* Divine protection */

    spedit_save_internally(new_spell);

    // SPELL_SHOCKING_GRASP # 37
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_EVOCATION;   /* Damage spell */
    new_spell->element = ELEMENT_LIGHTNING; /* Electric damage */

    spedit_save_internally(new_spell);

    // SPELL_SLEEP # 38
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ENCHANTMENT; /* Mind-affecting spell */
    new_spell->element = ELEMENT_MENTAL;    /* Mental effect */

    spedit_save_internally(new_spell);

    // SPELL_STRENGTH # 39
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ALTERATION; /* Stat enhancement */
    new_spell->element = ELEMENT_PHYSICAL; /* Physical enhancement */

    spedit_save_internally(new_spell);

    // SPELL_SUMMON # 40
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_SUMMON;
    new_spell->status = available;
    new_spell->name = strdup("summon");
    new_spell->function = spell_summon;
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_WORLD | TAR_NOT_SELF;
    new_spell->mag_flags = MAG_MANUAL;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(75 - (3 * self.level)) > 50 ? (75 - (3 * self.level)) : 50");
    new_spell->assign[0].class_num = CLASS_MAGIC_USER;
    new_spell->assign[0].level = 11;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->assign[1].class_num = CLASS_CLERIC;
    new_spell->assign[1].level = 15;
    new_spell->assign[1].num_mana = strdup(buf);
    new_spell->school = SCHOOL_CONJURATION; /* Transportation/summoning spell */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical transportation */

    spedit_save_internally(new_spell);

    // SPELL_VENTRILOQUATE # 41
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_VENTRILOQUATE;
    new_spell->status = available;
    new_spell->name = strdup("ventriloquate");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
    new_spell->mag_flags = MAG_MANUAL;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(30 - (3 * self.level)) > 15 ? (30 - (3 * self.level)) : 15");
    new_spell->assign[0].class_num = CLASS_MAGIC_USER;
    new_spell->assign[0].level = 15;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->assign[1].class_num = CLASS_CLERIC;
    new_spell->assign[1].level = 15;
    new_spell->assign[1].num_mana = strdup(buf);
    new_spell->assign[2].class_num = CLASS_DRUID;
    new_spell->assign[2].level = 15;
    new_spell->assign[2].num_mana = strdup(buf);
    new_spell->assign[3].class_num = CLASS_BARD;
    new_spell->assign[3].level = 15;
    new_spell->assign[3].num_mana = strdup(buf);
    new_spell->messages.to_self = strdup("Você manipula as palavras através da magia.");
    new_spell->messages.to_room = strdup("$n murmura palavras arcanas.");
    new_spell->school = SCHOOL_ILLUSION;    /* Voice illusion */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical deception */
    spedit_save_internally(new_spell);

    // SPELL_WORD_OF_RECALL # 42
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Transportation magic */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical transport */
    spedit_save_internally(new_spell);

    // SPELL_REMOVE_POISON # 43
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->dispel[0] = strdup("33");   // remove poison
    new_spell->messages.to_self = strdup("$p emite uma fumaça por alguns instantes.");
    new_spell->messages.to_vict = strdup("Uma calorosa sensação percorre o seu corpo!");
    new_spell->messages.to_room = strdup("$N aparenta estar melhor.");
    new_spell->school = SCHOOL_ABJURATION; /* Removes affliction */
    new_spell->element = ELEMENT_HOLY;     /* Divine cleansing */

    spedit_save_internally(new_spell);

    // SPELL_SENSE_LIFE # 44
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_DIVINATION;  /* Detects living beings */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical sense */

    spedit_save_internally(new_spell);

    // SPELL_ANIMATE_DEAD # 45
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->messages.to_room = strdup("$n anima um corpo!!");
    new_spell->school = SCHOOL_NECROMANCY; /* Raises undead */
    new_spell->element = ELEMENT_UNHOLY;   /* Death magic */

    spedit_save_internally(new_spell);

    // SPELL_DISPEL_GOOD # 46
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ABJURATION; /* Banishing magic */
    new_spell->element = ELEMENT_UNHOLY;   /* Evil divine power */

    spedit_save_internally(new_spell);

    // SPELL_GROUP_ARMOR # 47
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Group healing */
    new_spell->element = ELEMENT_HOLY;      /* Divine healing power */

    spedit_save_internally(new_spell);

    // SPELL_GROUP_RECALL # 49
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ALTERATION;  /* Enhances sight */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical effect */

    spedit_save_internally(new_spell);

    // SPELL_WATERWALK # 51
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ALTERATION; /* Movement enhancement */
    new_spell->element = ELEMENT_WATER;    /* Water-based effect */

    spedit_save_internally(new_spell);

    // SPELL_IDENTIFY # 52
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_DIVINATION;  /* Reveals information */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical analysis */

    spedit_save_internally(new_spell);

    // SPELL_FLY #63
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->applies[0].duration = strdup("self.level / 4");
    new_spell->applies[1].appl_num = AFF_FLYING + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level / 4");
    new_spell->messages.to_vict = strdup("Você se sente muito leve...");
    new_spell->messages.wear_off = strdup("Você se sente pesad$r novamente.");
    new_spell->school = SCHOOL_ALTERATION; /* Movement enhancement */
    new_spell->element = ELEMENT_AIR;      /* Air-based flight */

    spedit_save_internally(new_spell);

    // SPELL_DARKNESS # 92
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_DARKNESS;
    new_spell->status = available;
    new_spell->name = strdup("darkness");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_ROOMS;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
    new_spell->assign[1].class_num = CLASS_CLERIC;
    new_spell->assign[1].level = 40;
    new_spell->assign[1].num_mana = strdup(buf);
    new_spell->assign[2].class_num = CLASS_DRUID;
    new_spell->assign[2].level = 48;

    new_spell->messages.to_self = strdup("Você lança um manto de escuridão sobre a área.");
    new_spell->messages.to_room = strdup("$N lança um manto de escuridão sobre esta área.");

    new_spell->school = SCHOOL_NECROMANCY; /* Necromancy: darkness and shadow magic */
    new_spell->element = ELEMENT_UNHOLY;   /* Unholy element, same as animate dead */

    spedit_save_internally(new_spell);

    // SPELL_TRANSPORT_VIA_PLANTS
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->assign[0].class_num = CLASS_RANGER;
    new_spell->assign[0].level = 7;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->school = SCHOOL_CONJURATION; /* Transportation magic */
    new_spell->element = ELEMENT_EARTH;     /* Plant/nature-based transport */

    spedit_save_internally(new_spell);

    // SPELL_PROTECTION_FROM_GOOD # 94
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ABJURATION; /* Protection spell */
    new_spell->element = ELEMENT_UNHOLY;   /* Unholy protection */

    spedit_save_internally(new_spell);

    // stoneskin #53
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_STONESKIN;
    new_spell->status = available;
    new_spell->name = strdup("stoneskin");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_MANUAL;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(200 - (10 * self.level)) > 100 ? (200 - (10 * self.level)) : 100");
    new_spell->assign[0].class_num = CLASS_MAGIC_USER;
    new_spell->assign[0].level = 34;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->assign[1].class_num = CLASS_CLERIC;
    new_spell->assign[1].level = 90;
    new_spell->assign[1].num_mana = strdup(buf);
    new_spell->messages.to_vict = strdup("Você sente sua pele se tornando dura como rocha.");
    new_spell->messages.to_room = strdup("A pele de $N se torna mais dura.");
    new_spell->messages.wear_off = strdup("Sua pele perde a dureza.");
    new_spell->school = SCHOOL_ABJURATION; /* Protective skin enhancement */
    new_spell->element = ELEMENT_EARTH;    /* Stone/earth-based */

    spedit_save_internally(new_spell);

    // SPELL_fireshield #54
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_FIRESHIELD;
    new_spell->status = available;
    new_spell->name = strdup("fireshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
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
    new_spell->school = SCHOOL_ABJURATION; /* Protective shield */
    new_spell->element = ELEMENT_FIRE;     /* Fire-based protection */
    spedit_save_internally(new_spell);

    // spell portal 65
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_CONJURATION; /* Creates portal */
    new_spell->element = ELEMENT_UNDEFINED; /* Pure magical transportation */

    spedit_save_internally(new_spell);

    // spell vamp_touch
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_VAMP_TOUCH;
    new_spell->status = available;
    new_spell->name = strdup("vampiric touch");
    new_spell->function = spell_vamp_touch;
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
    new_spell->mag_flags = MAG_MANUAL | MAG_VIOLENT;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(30 - (1 * self.level)) > 15 ? (30 - (1 * self.level)) : 30");
    new_spell->assign[0].class_num = CLASS_MAGIC_USER;
    new_spell->assign[0].level = 15;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->school = SCHOOL_NECROMANCY; /* Life drain magic */
    new_spell->element = ELEMENT_UNHOLY;   /* Dark life energy */

    spedit_save_internally(new_spell);

    // spell FURY_OF_AIR
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_FURY_OF_AIR;
    new_spell->status = available;
    new_spell->name = strdup("fury of air");
    new_spell->function = spell_fury_air;
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
    new_spell->mag_flags = MAG_MANUAL | MAG_VIOLENT;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(200 - (10 * self.level)) > 100 ? (200 - (10 * self.level)) : 100");
    new_spell->assign[0].class_num = CLASS_DRUID;
    new_spell->assign[0].level = 50;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->school = SCHOOL_EVOCATION; /* Air-based damage spell */
    new_spell->element = ELEMENT_AIR;     /* Wind and air magic */

    spedit_save_internally(new_spell);

    // spell_improved_armor 55
    // AC modifier formula scales from -30 to -80 based on caster level
    // Formula: -(30 + MIN(50, self.level))
    // At level 1: -30 - 1 = -31 AC
    // At level 50+: -30 - 50 = -80 AC (capped)
    // With "plus" voice modifier: doubled and clamped to sbyte range (-128 to 127)
    // With "minus" voice modifier: halved
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->applies[0].modifier = strdup("-(30 + (self.level < 50 ? self.level : 50))");
    new_spell->applies[0].duration = strdup("24");
    new_spell->messages.to_vict = strdup("Você sente alguém $r protegendo.");
    new_spell->messages.wear_off = strdup("Você se sente menos protegid$r.");
    spedit_save_internally(new_spell);

    // spell_disintegrate 56
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->damages = strdup("dice(3, self.class == 0 ? 8 : 6) + (self.class == 0 ? 195 : 180)");
    new_spell->school = SCHOOL_EVOCATION;   /* Pure destruction */
    new_spell->element = ELEMENT_UNDEFINED; /* Disintegration force */
    spedit_save_internally(new_spell);

    // SPELL_EVOKE_AIR_SERVANT # 57
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->messages.to_room = strdup("$n invoca um servo aéreo!!");
    new_spell->school = SCHOOL_CONJURATION; /* Summoning spell */
    new_spell->element = ELEMENT_AIR;       /* Air elemental */

    spedit_save_internally(new_spell);

    // spell_talkdead #58
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SPELL_TALKDEAD;
    new_spell->status = available;
    new_spell->name = strdup("talk with dead");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
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
    new_spell->school = SCHOOL_NECROMANCY; /* Communication with dead */
    new_spell->element = ELEMENT_UNHOLY;   /* Death-realm communication */
    spedit_save_internally(new_spell);

    // breath
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SPELL_BREATH;
    new_spell->status = available;
    new_spell->name = strdup("breath");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
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
    new_spell->school = SCHOOL_ALTERATION; /* Body modification */
    new_spell->element = ELEMENT_AIR;      /* Air-breathing enhancement */
    spedit_save_internally(new_spell);

    // SPELL_Paralyze #64
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_ENCHANTMENT; /* Mind-affecting/body control */
    new_spell->element = ELEMENT_MENTAL;    /* Mental paralysis */
    spedit_save_internally(new_spell);

    // SPELL_aid #65
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_AID;
    new_spell->status = available;
    new_spell->name = strdup("aid");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM | TAR_SELF_ONLY;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_POINTS;
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
    new_spell->school = SCHOOL_CONJURATION; /* Divine aid/healing enhancement */
    new_spell->element = ELEMENT_HOLY;      /* Divine blessing */
    spedit_save_internally(new_spell);

    // spell_skin_like_wood 67
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ALTERATION; /* Body transformation */
    new_spell->element = ELEMENT_EARTH;    /* Wood/nature element */
    spedit_save_internally(new_spell);

    // spell_skin_like_rock 68
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ALTERATION; /* Body transformation */
    new_spell->element = ELEMENT_EARTH;    /* Rock/stone element */
    spedit_save_internally(new_spell);

    // spell_skin_like_steel 69
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ALTERATION; /* Body transformation */
    new_spell->element = ELEMENT_PHYSICAL; /* Metal/steel element */
    spedit_save_internally(new_spell);

    // spell_skin_like_diamond 70
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ALTERATION; /* Body transformation */
    new_spell->element = ELEMENT_EARTH;    /* Crystal/diamond element */
    spedit_save_internally(new_spell);

    // spell_BURST_OF_FLAME 71
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->damages = strdup("dice(3, self.class == 4 ? 5 : 3) + (self.class == 4 ? 6 : 4)");
    new_spell->max_dam = 100;
    new_spell->school = SCHOOL_EVOCATION; /* Damage spell */
    new_spell->element = ELEMENT_FIRE;    /* Fire damage */
    spedit_save_internally(new_spell);

    // spell_BURST_OF_FIRE 72
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->damages = strdup("dice(3, self.class == 4 ? 7 : 5) + (self.class == 4 ? 9 : 6)");
    new_spell->max_dam = 100;
    new_spell->school = SCHOOL_EVOCATION; /* Damage spell */
    new_spell->element = ELEMENT_FIRE;    /* Fire damage */
    spedit_save_internally(new_spell);

    // spell_IGNITE 73
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->damages = strdup("dice(3, self.class == 4 ? 9 : 3) + (self.class == 4 ? 13 : 9)");
    new_spell->max_dam = 100;
    new_spell->school = SCHOOL_EVOCATION; /* Damage spell */
    new_spell->element = ELEMENT_FIRE;    /* Fire damage */
    spedit_save_internally(new_spell);

    // spell_INVIGOR 74
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SPELL_INVIGOR;
    new_spell->status = available;
    new_spell->name = strdup("invigor");
    new_spell->type = SPELL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
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
    new_spell->school = SCHOOL_CONJURATION; /* Restoration spell */
    new_spell->element = ELEMENT_EARTH;     /* Nature-based invigoration */
    spedit_save_internally(new_spell);

    // SPELL_MINOR_HEALING	     75
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_CONJURATION; /* Healing spell */
    new_spell->element = ELEMENT_EARTH;     /* Nature-based healing */
    spedit_save_internally(new_spell);

    // SPELL_LIGHTNING_BLAST	     76
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_CONJURATION; /* Healing/restoration magic */
    new_spell->element = ELEMENT_EARTH;     /* Nature-based healing */
    spedit_save_internally(new_spell);

    // SPELL_THISTLECOAT #78
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_THISTLECOAT;
    new_spell->status = available;
    new_spell->name = strdup("thistlecoat");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_AURA;
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
    new_spell->school = SCHOOL_ALTERATION; /* Body modification protection */
    new_spell->element = ELEMENT_EARTH;    /* Natural thorny protection */
    spedit_save_internally(new_spell);

    // SPELL_CASCADE_HAIL	     79
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_CASCADE_HAIL;
    new_spell->status = available;
    new_spell->name = strdup("cascade of hail");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_DAMAGE | MAG_AREAS | MAG_VIOLENT;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(62 - (4 * self.level)) > 57 ? (62 - (4 * self.level)) : 57");
    new_spell->assign[0].class_num = CLASS_DRUID;
    new_spell->assign[0].level = 17;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->damages = strdup("12+rand(8)");
    new_spell->max_dam = 100;
    new_spell->school = SCHOOL_EVOCATION; /* Area damage spell */
    new_spell->element = ELEMENT_ICE;     /* Ice/hail damage */

    spedit_save_internally(new_spell);

    // SPELL_DANCE_FIREFLIES	     80
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->messages.to_room =
        strdup("Você se surpreende com a nuvem de pequenos vaga-lumes que apareceram para iluminar $n");
    new_spell->messages.wear_off = strdup("Você observa os vaga-lumes indo embora, um a um.");
    new_spell->school = SCHOOL_CONJURATION; /* Summoning creatures/light */
    new_spell->element = ELEMENT_UNDEFINED; /* Light-providing magic */
    spedit_save_internally(new_spell);

    // SPELL_STINGING_SWARM	     80
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);

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
    new_spell->messages.to_room = strdup("$n invoca um corvo para ajudar!");
    new_spell->school = SCHOOL_CONJURATION; /* Animal summoning */
    new_spell->element = ELEMENT_AIR;       /* Flying creature */
    spedit_save_internally(new_spell);

    // SPELL_EVOKE_WOLF
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->messages.to_room = strdup("$n invoca um lobo para ajudar!");
    new_spell->school = SCHOOL_CONJURATION; /* Animal summoning */
    new_spell->element = ELEMENT_EARTH;     /* Land creature */
    spedit_save_internally(new_spell);

    // SPELL_EVOKE_BEAR
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->messages.to_room = strdup("$n invoca um urso para ajudar!");
    new_spell->school = SCHOOL_CONJURATION; /* Animal summoning */
    new_spell->element = ELEMENT_EARTH;     /* Powerful land creature */
    spedit_save_internally(new_spell);

    // SPELL_EVOKE_LION
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->messages.to_room = strdup("$n invoca um leão para ajudar!");
    new_spell->school = SCHOOL_CONJURATION; /* Animal summoning */
    new_spell->element = ELEMENT_FIRE;      /* Fierce predator */
    spedit_save_internally(new_spell);

    // SPELL_VOICE_EXPLOSION # 89
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_VOICE_EXPLOSION;
    new_spell->status = available;
    new_spell->name = strdup("voice explosion");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_DAMAGE | MAG_AREAS | MAG_VIOLENT;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(70 - (3 * self.level)) > 20 ? (70 - (3 * self.level)) : 20");
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 23;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->damages = strdup("dice(3,6) + param");
    new_spell->max_dam = 100;
    new_spell->messages.to_self =
        strdup("Com sua voz você emite um som de extrema violência, ferindo tudo que $r cerca!");
    new_spell->messages.to_room = strdup("$n emite com sua voz um som de violência extrema, ferindo a todos!");
    new_spell->school = SCHOOL_EVOCATION;   /* Sound-based damage spell */
    new_spell->element = ELEMENT_UNDEFINED; /* Sonic magic */

    spedit_save_internally(new_spell);
    // SPELL_SOUNDBARRIER #90

    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_SOUNDBARRIER;
    new_spell->status = available;
    new_spell->name = strdup("soundbarrier");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_AURA;
    new_spell->effectiveness = strdup("100");

    /* Custo de mana baseado no nível do conjurador */
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 15;
    new_spell->assign[0].num_mana = strdup("(100 - (5 * self.level)) > 20 ? (100 - (5 * self.level)) : 20");

    /* Aplicações do feitiço */
    new_spell->applies[0].appl_num = APPLY_SAVING_SPELL;
    new_spell->applies[0].modifier = strdup("-(1 + (param / 40))");
    new_spell->applies[0].duration = strdup("24");

    new_spell->applies[1].appl_num = APPLY_SAVING_BREATH;
    new_spell->applies[1].modifier = strdup("-(2 + (param / 40))");
    new_spell->applies[1].duration = strdup("24");

    new_spell->applies[2].appl_num = APPLY_SAVING_ROD;
    new_spell->applies[2].modifier = strdup("-(1 + (param / 40))");
    new_spell->applies[2].duration = strdup("24");

    new_spell->applies[3].appl_num = APPLY_AC;
    new_spell->applies[3].modifier =
        strdup("((self.level > (vict.level + 10)) * -70) + ((self.level <= (vict.level + 10)) * (-(vict.level - 10)))");
    new_spell->applies[3].duration = strdup("24");

    new_spell->applies[4].appl_num = AFF_SOUNDBARRIER + NUM_APPLIES;
    new_spell->applies[4].duration = strdup("24");

    /* Mensagens de efeito */
    new_spell->messages.to_vict = strdup("Voce e envolvid$r por uma protetora barreira de som.");
    new_spell->messages.to_room = strdup("$N e envolvid$r por uma protetora barreira de som.");
    new_spell->messages.wear_off = strdup("Voce percebe a barreira de som se dissipar.");

    /* Escola e elemento */
    new_spell->school = SCHOOL_ABJURATION;  /* Protection spell */
    new_spell->element = ELEMENT_UNDEFINED; /* Sound-based protection */

    /* Salva internamente */
    spedit_save_internally(new_spell);

    // SPELL_GLOOMSHIELD # 91
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SPELL_GLOOMSHIELD;
    new_spell->status = available;
    new_spell->name = strdup("gloomshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_UNAFFECTS | MAG_AURA;
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
    new_spell->school = SCHOOL_ABJURATION; /* Protective spell */
    new_spell->element = ELEMENT_UNHOLY;   /* Dark protection */
    spedit_save_internally(new_spell);

    // spell raise_dead
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_NECROMANCY; /* Death/life restoration */
    new_spell->element = ELEMENT_HOLY;     /* Divine restoration of life */

    spedit_save_internally(new_spell);

    // spell ressurect
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    new_spell->school = SCHOOL_NECROMANCY; /* Greater life restoration */
    new_spell->element = ELEMENT_HOLY;     /* Divine resurrection power */

    spedit_save_internally(new_spell);

    // SPELL_WINDWALL #96
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_WINDWALL;
    new_spell->status = available;
    new_spell->name = strdup("windwall");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
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
    new_spell->school = SCHOOL_ABJURATION; /* Protection spell */
    new_spell->element = ELEMENT_AIR;      /* Wind-based protection */
    spedit_save_internally(new_spell);

    // SPELL_BLESS_OBJECT # 97
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SPELL_BLESS_OBJECT;
    new_spell->status = available;
    new_spell->name = strdup("bless object");
    new_spell->function = spell_enchant_weapon;
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_OBJ_INV;
    new_spell->mag_flags = MAG_MANUAL;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(35 - (3 * self.level)) > 5 ? (35 - (3 * self.level)) : 5");
    new_spell->assign[0].class_num = CLASS_CLERIC;
    new_spell->assign[0].level = 6;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->school = SCHOOL_ENCHANTMENT; /* Object enhancement */
    new_spell->element = ELEMENT_HOLY;      /* Divine blessing */
    spedit_save_internally(new_spell);

    // SKILL_BACKSTAB # 131
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_BACKSTAB;
    new_spell->status = available;
    new_spell->name = strdup("backstab");
    new_spell->function = do_backstab;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_THIEF;
    new_spell->assign[0].level = 3;
    spedit_save_internally(new_spell);

    // SKILL_BASH # 132
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_BASH;
    new_spell->status = available;
    new_spell->name = strdup("bash");
    new_spell->function = do_bash;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_WARRIOR;
    new_spell->assign[0].level = 12;

    spedit_save_internally(new_spell);

    // SKILL_HIDE # 133
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_KICK;
    new_spell->status = available;
    new_spell->name = strdup("kick");
    new_spell->function = do_kick;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_WARRIOR;
    new_spell->assign[0].level = 1;
    new_spell->assign[1].class_num = CLASS_RANGER;
    new_spell->assign[1].level = 1;

    spedit_save_internally(new_spell);

    // SKILL_PICK_LOCK # 135
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_WHIRLWIND;
    new_spell->status = available;
    new_spell->name = strdup("whirlwind");
    new_spell->function = do_whirlwind;
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(20 - (1 * self.level)) > 10 ? (20 - (1 * self.level)) : 10");
    new_spell->assign[0].class_num = CLASS_WARRIOR;
    new_spell->assign[0].level = 16;
    new_spell->assign[0].num_mana = strdup(buf);

    spedit_save_internally(new_spell);

    // SKILL_RESCUE # 137
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_RESCUE;
    new_spell->status = available;
    new_spell->name = strdup("rescue");
    new_spell->function = do_rescue;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_WARRIOR;
    new_spell->assign[0].level = 3;
    new_spell->assign[1].class_num = CLASS_RANGER;
    new_spell->assign[1].level = 5;

    spedit_save_internally(new_spell);

    // SKILL_SNEAK # 138
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_STEAL;
    new_spell->status = available;
    new_spell->name = strdup("steal");
    new_spell->function = do_steal;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_THIEF;
    new_spell->assign[0].level = 4;

    spedit_save_internally(new_spell);

    // SKILL_PEEK # 258
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_PEEK;
    new_spell->status = available;
    new_spell->name = strdup("peek");
    new_spell->function = do_peek;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_THIEF;
    new_spell->assign[0].level = 5;

    spedit_save_internally(new_spell);

    // SKILL_TRACK # 140
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_BANDAGE;
    new_spell->status = available;
    new_spell->name = strdup("bandage");
    new_spell->function = do_bandage;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_WARRIOR;
    new_spell->assign[0].level = 7;

    spedit_save_internally(new_spell);

    /*SCAN*/
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);
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
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SKILL_TRIP;
    new_spell->status = available;
    new_spell->name = strdup("trip");
    new_spell->function = do_trip;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 35;

    spedit_save_internally(new_spell);

    /*BACKFLIP*/
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SKILL_COMBO_ATTACK;
    new_spell->status = available;
    new_spell->name = strdup("combo attack");
    new_spell->function = do_combo;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_WARRIOR;
    new_spell->assign[0].level = 30;

    spedit_save_internally(new_spell);

    // SKILL_SEIZE # 238
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_SEIZE;
    new_spell->status = available;
    new_spell->name = strdup("seize");
    new_spell->function = do_seize;
    new_spell->type = SKILL;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_THIEF;
    new_spell->assign[0].level = 20;

    spedit_save_internally(new_spell);

    // SKILL_SPY # 237
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);

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
    spedit_init_new_spell(new_spell);
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
    spedit_init_new_spell(new_spell);
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
    spedit_init_new_spell(new_spell);
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
    spedit_init_new_spell(new_spell);
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
    spedit_init_new_spell(new_spell);
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
    spedit_init_new_spell(new_spell);
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
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_BOWS;
    new_spell->status = available;
    new_spell->name = strdup("bows");
    new_spell->function = do_shoot;
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(15 - (1 * self.level)) > 2 ? (15 - (1 * self.level)) : 2");
    new_spell->assign[0].class_num = CLASS_RANGER;
    new_spell->assign[0].level = 30;
    new_spell->assign[0].num_mana = strdup(buf);

    spedit_save_internally(new_spell);

    // SKILL_MINE # 253
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_MINE;
    new_spell->status = available;
    new_spell->name = strdup("mine");
    new_spell->function = do_mine;
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_WARRIOR;
    new_spell->assign[0].level = 15;
    new_spell->assign[1].class_num = CLASS_RANGER;
    new_spell->assign[1].level = 12;

    spedit_save_internally(new_spell);

    // SKILL_FISHING # 254
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_FISHING;
    new_spell->status = available;
    new_spell->name = strdup("fishing");
    new_spell->function = do_fishing;
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_RANGER;
    new_spell->assign[0].level = 8;
    new_spell->assign[1].class_num = CLASS_CLERIC;
    new_spell->assign[1].level = 20;

    spedit_save_internally(new_spell);

    // SKILL_FORAGE # 255
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_FORAGE;
    new_spell->status = available;
    new_spell->name = strdup("forage");
    new_spell->function = do_forage;
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_RANGER;
    new_spell->assign[0].level = 5;
    new_spell->assign[1].class_num = CLASS_THIEF;
    new_spell->assign[1].level = 15;

    spedit_save_internally(new_spell);

    // SKILL_EAVESDROP # 256
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_EAVESDROP;
    new_spell->status = available;
    new_spell->name = strdup("eavesdrop");
    new_spell->function = do_eavesdrop;
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_THIEF;
    new_spell->assign[0].level = 10;
    new_spell->assign[1].class_num = CLASS_BARD;
    new_spell->assign[1].level = 12;

    spedit_save_internally(new_spell);

    // SKILL_TAINT_FLUID # 257
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SKILL_TAINT_FLUID;
    new_spell->status = available;
    new_spell->name = strdup("taint fluid");
    new_spell->function = do_taint;
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_THIEF;
    new_spell->assign[0].level = 30;

    spedit_save_internally(new_spell);

    // SKILL_ADAGIO # 247
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SKILL_ADAGIO;
    new_spell->status = available;
    new_spell->name = strdup("adagio");
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 45;
    new_spell->dispel[0] = strdup("248");   // Dispel Allegro
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
    spedit_init_new_spell(new_spell);
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
    new_spell->dispel[0] = strdup("247");   // Dispel Adagio
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
    spedit_init_new_spell(new_spell);
    new_spell->vnum = SKILL_NIGHTHAMMER;
    new_spell->status = available;
    new_spell->name = strdup("nighthammer");
    new_spell->type = SKILL;
    new_spell->effectiveness = strdup("100");
    new_spell->assign[0].class_num = CLASS_THIEF;
    new_spell->assign[0].level = 50;

    spedit_save_internally(new_spell);

    /*CHANSON_ARDOR #161 */
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = CHANSON_ARDOR;
    new_spell->status = available;
    new_spell->name = strdup("ardor das chamas");
    new_spell->type = CHANSON;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_CREATIONS;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(20 - (2 * self.level)) > 5 ? (20 - (2 * self.level)) : 5");
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 1;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->objects[0] = strdup("34");   // object VNUM 34
    new_spell->school = SCHOOL_CONJURATION; /* Creates flame object */
    new_spell->element = ELEMENT_FIRE;      /* Fire-based song */
    spedit_save_internally(new_spell);

    /*CHANSON_OFUSCAR #162 */
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ILLUSION;    /* Light/visual effect */
    new_spell->element = ELEMENT_UNDEFINED; /* Light-based blinding */
    spedit_save_internally(new_spell);

    /*CHANSON_NINAR #163 */
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ENCHANTMENT; /* Mind-affecting sleep */
    new_spell->element = ELEMENT_MENTAL;    /* Mental effect */
    spedit_save_internally(new_spell);

    // CHANSON_TREMOR # 164
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = CHANSON_TREMOR;
    new_spell->status = available;
    new_spell->name = strdup("tremor");
    new_spell->type = CHANSON;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_DAMAGE | MAG_AREAS | MAG_VIOLENT;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(60 - (4 * self.level)) > 20 ? (60 - (4 * self.level)) : 20");
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 72;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->damages = strdup("dice(3, 7) + param");
    new_spell->max_dam = 300;
    new_spell->messages.to_self = strdup("Com sua cancao voce faz com que o chao a redor comece a tremer e fender-se!");
    new_spell->messages.to_room = strdup("Ao cantar sua cancao, $n faz com que o chao comece a tremer e fender-se!");
    new_spell->school = SCHOOL_EVOCATION; /* Area damage spell */
    new_spell->element = ELEMENT_EARTH;   /* Earth tremor effect */
    spedit_save_internally(new_spell);

    // CHANSON_ANDARILHO             165
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = CHANSON_ANDARILHO;
    new_spell->status = available;
    new_spell->name = strdup("a balada do andarilho");
    new_spell->type = CHANSON;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_POINTS;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(50 - (4 * self.level)) > 13 ? (50 - (4 * self.level)) : 13");
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 28;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->messages.to_vict = strdup("Você sente que a Balada do Andarilho $r faz recuperar forças.");
    new_spell->points.hp = strdup("4 + dice(2, 6)");
    new_spell->points.move = strdup("12 + dice(4, 2)");
    new_spell->school = SCHOOL_CONJURATION; /* Healing/restoration */
    new_spell->element = ELEMENT_UNDEFINED; /* Restoration effect */
    spedit_save_internally(new_spell);

    // CHANSON_MORTE # 166
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->damages = strdup("dice(9,9) + 20");
    new_spell->max_dam = 100;
    new_spell->messages.to_self = strdup("Morte, o Ceifeiro Cruel responde ao seu chamado causando dano a $N!");
    new_spell->messages.to_room = strdup("Morte, o Ceifeiro Cruel responde ao chamado de $n e causa dano a $N!");
    new_spell->messages.to_vict = strdup("A propria Morte aparece e causa dano a voce!");
    new_spell->school = SCHOOL_NECROMANCY; /* Death-based damage */
    new_spell->element = ELEMENT_UNHOLY;   /* Death/negative energy */
    spedit_save_internally(new_spell);

    // CHANSON_LEVEZA # 167
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ALTERATION; /* Body modification */
    new_spell->element = ELEMENT_WATER;    /* Water-walking ability */
    spedit_save_internally(new_spell);

    // chanson_ardor #168 rever tabela

    // CHANSON_DEUSES  169
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = CHANSON_DEUSES;
    new_spell->status = available;
    new_spell->name = strdup("invocacao aos deuses");
    new_spell->type = CHANSON;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_POINTS;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(90 - (5 * self.level)) > 50 ? (90 - (5 * self.level)) : 50");
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 95;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->messages.to_vict = strdup("Os ecos de uma canção divina preenchem sua cabeça e curam seu corpo.\r\n");
    sprintf(buf, "(dice(3,6) + (param - 15)) > 1 ? (dice(3,6) + (param - 15) : 1");
    new_spell->points.hp = strdup(buf);
    new_spell->school = SCHOOL_CONJURATION; /* Divine healing */
    new_spell->element = ELEMENT_HOLY;      /* Divine intervention */
    spedit_save_internally(new_spell);

    // CHANSON_VULNERAVEL 170
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ENCHANTMENT; /* Debuff/weakness */
    new_spell->element = ELEMENT_MENTAL;    /* Mental vulnerability */
    spedit_save_internally(new_spell);

    // CHANSON_ECOS

    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_NECROMANCY; /* Cursing debuff */
    new_spell->element = ELEMENT_UNHOLY;   /* Dark curse magic */
    spedit_save_internally(new_spell);

    // CHANSON_CLAMOR 174
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ABJURATION;  /* Protection from harm */
    new_spell->element = ELEMENT_UNDEFINED; /* General protection */
    spedit_save_internally(new_spell);

    // CHANSON_CESSAR #176
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_CONJURATION; /* Healing song */
    new_spell->element = ELEMENT_UNDEFINED; /* Musical healing */
    spedit_save_internally(new_spell);

    // CHANSON_VISAO 175
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->applies[0].duration = strdup("12 + param");
    new_spell->messages.to_vict = strdup("Você se sente mais sensível ao mundo exterior.");
    new_spell->messages.wear_off = strdup("Você se sente menos sensível.");
    new_spell->school = SCHOOL_DIVINATION; /* Enhanced perception */
    new_spell->element = ELEMENT_MENTAL;   /* Mental enhancement */
    spedit_save_internally(new_spell);

    // CHANSON_ALENTO
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->applies[3].duration = strdup("param/4");
    new_spell->applies[4].appl_num = APPLY_CON;
    new_spell->applies[4].modifier = strdup("1");
    new_spell->applies[4].duration = strdup("param/4");
    new_spell->messages.to_vict = strdup("Uma grande força parece tormar conta de seu coração.");
    new_spell->messages.to_room = strdup("$N!");
    new_spell->messages.wear_off = strdup("Você volta a enxergar.");
    new_spell->school = SCHOOL_ENCHANTMENT; /* Combat enhancement */
    new_spell->element = ELEMENT_UNDEFINED; /* Battle inspiration */
    spedit_save_internally(new_spell);

    /*CHANSONS ESPECIAIS: BRINDE, VOLTAR, ENCANTO, ALENTO*/
    // CHANSON_BRINDE #88
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
    new_spell->vnum = CHANSON_BRINDE;
    new_spell->status = available;
    new_spell->name = strdup("vinho para brindar");
    new_spell->function = chanson_brinde;
    new_spell->type = CHANSON;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_OBJ_INV | TAR_OBJ_EQUIP;
    new_spell->mag_flags = MAG_MANUAL;
    new_spell->effectiveness = strdup("100");
    sprintf(buf, "(30 - (4 * self.level)) > 5 ? (30 - (4 * self.level)) : 5");
    new_spell->assign[0].class_num = CLASS_BARD;
    new_spell->assign[0].level = 6;
    new_spell->assign[0].num_mana = strdup(buf);
    new_spell->school = SCHOOL_CONJURATION; /* Object creation/transformation */
    new_spell->element = ELEMENT_UNDEFINED; /* Liquid transformation */
    spedit_save_internally(new_spell);

    // CHANSON_ENCANTO #7
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_ENCHANTMENT; /* Mind control */
    new_spell->element = ELEMENT_MENTAL;    /* Mental charm */

    spedit_save_internally(new_spell);

    // CHANSON_VOLTAR
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);
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
    new_spell->school = SCHOOL_CONJURATION; /* Transportation */
    new_spell->element = ELEMENT_UNDEFINED; /* Recall magic */
    spedit_save_internally(new_spell);

    // spellnum, spellname, maxmana, minmana, manachng, minpos, targets, viol   ent?, routines.

    // SPELL_SCROLL_IDENTIFY # 52
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_SCROLL_IDENTIFY;
    new_spell->status = available;
    new_spell->name = strdup("scroll identify");
    new_spell->function = spell_identify;
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
    new_spell->mag_flags = MAG_MANUAL;
    new_spell->effectiveness = strdup("100");
    new_spell->school = SCHOOL_DIVINATION; /* Information gathering */
    new_spell->element = ELEMENT_MENTAL;   /* Mental analysis */
    spedit_save_internally(new_spell);

    // SPELL_YOUTH
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_YOUTH;
    new_spell->status = available;
    new_spell->name = strdup("youth");
    new_spell->function = spell_youth;
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_OBJ_ROOM;
    new_spell->mag_flags = MAG_MANUAL;
    new_spell->effectiveness = strdup("100");
    new_spell->school = SCHOOL_ALTERATION;  /* Time/age manipulation */
    new_spell->element = ELEMENT_UNDEFINED; /* Temporal magic */
    spedit_save_internally(new_spell);

    // SPELL_DG_AFFECT # 298
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_DG_AFFECT;
    new_spell->status = available;
    new_spell->name = strdup("script-inflicted");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_SITTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->effectiveness = strdup("100");
    new_spell->school = SCHOOL_UNDEFINED;   /* Script-triggered effect */
    new_spell->element = ELEMENT_UNDEFINED; /* No specific element */

    spedit_save_internally(new_spell);

    // SPELL_SHIELD # 308
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_SHIELD;
    new_spell->status = available;
    new_spell->name = strdup("shield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_CHAR_ROOM | TAR_FIGHT_SELF | TAR_FIGHT_VICT;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-10");
    new_spell->applies[0].duration = strdup("12");
    new_spell->messages.wear_off = strdup("Um enorme escudo se desfaz.");
    new_spell->messages.to_self = strdup("Voce consegue fazer aparecer um enorme escudo.");
    new_spell->messages.to_vict = strdup("Um enorme escudo aparece para te proteger.");
    new_spell->messages.to_room = strdup("Um enorme escudo aparece do nada.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_PHYSICAL;
    new_spell->prerequisite_spell = SPELL_ARMOR;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_MANA_CONTROL # 309
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_MANA_CONTROL;
    new_spell->status = available;
    new_spell->name = strdup("mana control");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("20");   // Mob vnum 20 - base mana control creature
    new_spell->assign[0].class_num = CLASS_MAGIC_USER;
    new_spell->assign[0].level = 5;
    new_spell->assign[0].prac_gain = strdup("0");
    new_spell->assign[0].num_mana = strdup("0");
    new_spell->assign[1].class_num = CLASS_CLERIC;
    new_spell->assign[1].level = 5;
    new_spell->assign[1].prac_gain = strdup("0");
    new_spell->assign[1].num_mana = strdup("0");
    new_spell->assign[2].class_num = CLASS_DRUID;
    new_spell->assign[2].level = 5;
    new_spell->assign[2].prac_gain = strdup("0");
    new_spell->assign[2].num_mana = strdup("0");
    new_spell->assign[3].class_num = CLASS_RANGER;
    new_spell->assign[3].level = 5;
    new_spell->assign[3].prac_gain = strdup("0");
    new_spell->assign[3].num_mana = strdup("0");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_HOLY;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_FIDO # 310
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_FIDO;
    new_spell->status = available;
    new_spell->name = strdup("evoke fido");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS | MAG_AREAS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("3062");   // Mob vnum 3062 - fido creature
    new_spell->summon_req = strdup("3833");   // Required object vnum 3833 - summoning reagent
    new_spell->messages.to_room = strdup("$n invoca um fido para atrapalhar!");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_PHYSICAL;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_ODIF_EKOVE # 311
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_ODIF_EKOVE;
    new_spell->status = available;
    new_spell->name = strdup("odif ekove");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("3066");   // Mob vnum 3066 - reverse fido creature
    new_spell->summon_req = strdup("3833");   // Required object vnum 3833 - summoning reagent
    new_spell->messages.to_room = strdup("Um odif laitseb foi atrapalhado por $n!");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_HOLY;
    new_spell->prerequisite_spell = SPELL_EVOKE_FIDO;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_AWAKEN # 312
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_AWAKEN;
    new_spell->status = available;
    new_spell->name = strdup("awaken");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM | TAR_NOT_SELF;
    new_spell->mag_flags = MAG_UNAFFECTS;
    new_spell->effectiveness = strdup("100");
    new_spell->dispel[0] = strdup("38");   // Dispels SPELL_SLEEP (vnum 38)
    new_spell->messages.to_self = strdup("Hora de acordar!");
    new_spell->messages.to_vict = strdup("$n te chacoalha para acordar.");
    new_spell->messages.to_room = strdup("$n chacoalha $N para acordar.");
    new_spell->school = SCHOOL_ILLUSION;
    new_spell->element = ELEMENT_MENTAL;
    new_spell->prerequisite_spell = SPELL_SLEEP;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_TIAMAT # 313
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_TIAMAT;
    new_spell->status = available;
    new_spell->name = strdup("evoke tiamat");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("9303");   // Mob vnum 9303 - tiamat creature
    new_spell->summon_req = strdup("3862");   // Required object vnum 3862 - summoning reagent
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_UNHOLY;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_ORC # 314
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_ORC;
    new_spell->status = available;
    new_spell->name = strdup("evoke orc");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("19700");
    new_spell->summon_req = strdup("3848");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_EARTH;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_HOBGOBLIN # 315
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_HOBGOBLIN;
    new_spell->status = available;
    new_spell->name = strdup("evoke hobgoblin");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("4055");
    new_spell->summon_req = strdup("3837");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_PHYSICAL;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_GOBLIN # 316
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_GOBLIN;
    new_spell->status = available;
    new_spell->name = strdup("evoke goblin");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("3501");
    new_spell->summon_req = strdup("3846");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_PHYSICAL;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_HOBBIT # 317
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_HOBBIT;
    new_spell->status = available;
    new_spell->name = strdup("evoke hobbit");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("14500");
    new_spell->summon_req = strdup("3845");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_EARTH;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_ROMANO # 318
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_ROMANO;
    new_spell->status = available;
    new_spell->name = strdup("evoke romano");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("12017");
    new_spell->summon_req = strdup("3844");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_PHYSICAL;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_ANAO # 319
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_ANAO;
    new_spell->status = available;
    new_spell->name = strdup("evoke anao");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_RESTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("6509");
    new_spell->summon_req = strdup("3843");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_HOLY;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_GNOMO # 320
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_GNOMO;
    new_spell->status = available;
    new_spell->name = strdup("evoke gnomo");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("1601");
    new_spell->summon_req = strdup("3842");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_EARTH;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_ELFO # 321
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_ELFO;
    new_spell->status = available;
    new_spell->name = strdup("evoke elfo");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_RESTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("19017");
    new_spell->summon_req = strdup("3841");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_HOLY;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_BARALHO # 322
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_BARALHO;
    new_spell->status = available;
    new_spell->name = strdup("evoke baralho");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("9230");
    new_spell->summon_req = strdup("3840");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_MENTAL;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_XADREZ # 323
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_XADREZ;
    new_spell->status = available;
    new_spell->name = strdup("evoke xadrez");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("3609");
    new_spell->summon_req = strdup("3839");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_MENTAL;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_LAMIA # 324
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_LAMIA;
    new_spell->status = available;
    new_spell->name = strdup("evoke lamia");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS | MAG_AREAS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("5201");
    new_spell->summon_req = strdup("3838");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_AIR;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_MICONOIDE # 325
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_MICONOIDE;
    new_spell->status = available;
    new_spell->name = strdup("evoke miconoide");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS | MAG_AREAS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("5014");
    new_spell->summon_req = strdup("3837");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_WATER;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_LAGARTO # 326
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_LAGARTO;
    new_spell->status = available;
    new_spell->name = strdup("evoke lagarto");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("2204");
    new_spell->summon_req = strdup("3836");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_WATER;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_BOLHA # 327
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_BOLHA;
    new_spell->status = available;
    new_spell->name = strdup("evoke bolha");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS | MAG_AREAS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("3068");
    new_spell->summon_req = strdup("3835");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_ACID;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_ATOM # 328
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_ATOM;
    new_spell->status = available;
    new_spell->name = strdup("evoke atom");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("4909");
    new_spell->summon_req = strdup("3834");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_EARTH;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_URUBU # 329
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_URUBU;
    new_spell->status = available;
    new_spell->name = strdup("evoke urubu");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_FIGHTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("4252");
    new_spell->summon_req = strdup("3832");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_UNHOLY;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_EVOKE_NOVATO # 330
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_EVOKE_NOVATO;
    new_spell->status = available;
    new_spell->name = strdup("evoke novato");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_RESTING;
    new_spell->targ_flags = TAR_IGNORE;
    new_spell->mag_flags = MAG_SUMMONS;
    new_spell->effectiveness = strdup("100");
    new_spell->summon_mob = strdup("18615");
    new_spell->summon_req = strdup("3831");
    new_spell->school = SCHOOL_EVOCATION;
    new_spell->element = ELEMENT_MENTAL;
    new_spell->prerequisite_spell = SPELL_MANA_CONTROL;
    new_spell->discoverable = 1;

    spedit_save_internally(new_spell);

    // SPELL_WATERSHIELD #99 - Water/Aqua aegis (discoverable variant of fireshield)
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_WATERSHIELD;
    new_spell->status = available;
    new_spell->name = strdup("watershield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-4");
    new_spell->applies[0].duration = strdup("self.level");
    new_spell->applies[1].appl_num = AFF_WATERSHIELD + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level");
    new_spell->messages.to_vict = strdup("Uma fluida aura azul envolve você.");
    new_spell->messages.to_room = strdup("Uma fluida aura azul envolve $N.");
    new_spell->messages.wear_off = strdup("A aura de água ao redor de seu corpo desaparece.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_WATER;
    new_spell->prerequisite_spell = SPELL_FIRESHIELD;
    new_spell->discoverable = 1;
    spedit_save_internally(new_spell);

    // SPELL_ROCKSHIELD #100 - Earth/Petra aegis (discoverable variant of fireshield)
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_ROCKSHIELD;
    new_spell->status = available;
    new_spell->name = strdup("rockshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-6");
    new_spell->applies[0].duration = strdup("self.level");
    new_spell->applies[1].appl_num = AFF_ROCKSHIELD + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level");
    new_spell->messages.to_vict = strdup("Uma sólida aura marrom envolve você.");
    new_spell->messages.to_room = strdup("Uma sólida aura marrom envolve $N.");
    new_spell->messages.wear_off = strdup("A aura de pedra ao redor de seu corpo desaparece.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_EARTH;
    new_spell->prerequisite_spell = SPELL_FIRESHIELD;
    new_spell->discoverable = 1;
    spedit_save_internally(new_spell);

    // SPELL_POISONSHIELD #101 - Poison/Venenum aegis (discoverable variant of fireshield)
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_POISONSHIELD;
    new_spell->status = available;
    new_spell->name = strdup("poisonshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-3");
    new_spell->applies[0].duration = strdup("self.level");
    new_spell->applies[1].appl_num = AFF_POISONSHIELD + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level");
    new_spell->messages.to_vict = strdup("Uma tóxica aura verde envolve você.");
    new_spell->messages.to_room = strdup("Uma tóxica aura verde envolve $N.");
    new_spell->messages.wear_off = strdup("A aura venenosa ao redor de seu corpo desaparece.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_POISON;
    new_spell->prerequisite_spell = SPELL_FIRESHIELD;
    new_spell->discoverable = 1;
    spedit_save_internally(new_spell);

    // SPELL_LIGHTNINGSHIELD #102 - Lightning/Diesilla aegis (discoverable variant of fireshield)
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_LIGHTNINGSHIELD;
    new_spell->status = available;
    new_spell->name = strdup("lightningshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-4");
    new_spell->applies[0].duration = strdup("self.level");
    new_spell->applies[1].appl_num = AFF_LIGHTNINGSHIELD + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level");
    new_spell->messages.to_vict = strdup("Uma elétrica aura amarela envolve você.");
    new_spell->messages.to_room = strdup("Uma elétrica aura amarela envolve $N.");
    new_spell->messages.wear_off = strdup("A aura de raios ao redor de seu corpo desaparece.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_LIGHTNING;
    new_spell->prerequisite_spell = SPELL_FIRESHIELD;
    new_spell->discoverable = 1;
    spedit_save_internally(new_spell);

    // SPELL_ICESHIELD #103 - Ice/Cold aegis (discoverable variant of fireshield)
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_ICESHIELD;
    new_spell->status = available;
    new_spell->name = strdup("iceshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-4");
    new_spell->applies[0].duration = strdup("self.level");
    new_spell->applies[1].appl_num = AFF_ICESHIELD + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level");
    new_spell->messages.to_vict = strdup("Uma gélida aura branca envolve você.");
    new_spell->messages.to_room = strdup("Uma gélida aura branca envolve $N.");
    new_spell->messages.wear_off = strdup("A aura de gelo ao redor de seu corpo desaparece.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_ICE;
    new_spell->prerequisite_spell = SPELL_FIRESHIELD;
    new_spell->discoverable = 1;
    spedit_save_internally(new_spell);

    // SPELL_ACIDSHIELD #104 - Acid aegis (discoverable variant of fireshield)
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_ACIDSHIELD;
    new_spell->status = available;
    new_spell->name = strdup("acidshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-4");
    new_spell->applies[0].duration = strdup("self.level");
    new_spell->applies[1].appl_num = AFF_ACIDSHIELD + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level");
    new_spell->messages.to_vict = strdup("Uma corrosiva aura esverdeada envolve você.");
    new_spell->messages.to_room = strdup("Uma corrosiva aura esverdeada envolve $N.");
    new_spell->messages.wear_off = strdup("A aura ácida ao redor de seu corpo desaparece.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_ACID;
    new_spell->prerequisite_spell = SPELL_FIRESHIELD;
    new_spell->discoverable = 1;
    spedit_save_internally(new_spell);

    // SPELL_MINDSHIELD #105 - Mental aegis (discoverable variant of fireshield)
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_MINDSHIELD;
    new_spell->status = available;
    new_spell->name = strdup("mindshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-3");
    new_spell->applies[0].duration = strdup("self.level");
    new_spell->applies[1].appl_num = AFF_MINDSHIELD + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level");
    new_spell->messages.to_vict = strdup("Uma etérea aura violeta envolve você.");
    new_spell->messages.to_room = strdup("Uma etérea aura violeta envolve $N.");
    new_spell->messages.wear_off = strdup("A aura mental ao redor de seu corpo desaparece.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_MENTAL;
    new_spell->prerequisite_spell = SPELL_FIRESHIELD;
    new_spell->discoverable = 1;
    spedit_save_internally(new_spell);

    // SPELL_FORCESHIELD #106 - Physical/Force aegis (discoverable variant of fireshield)
    CREATE(new_spell, struct str_spells, 1);
    spedit_init_new_spell(new_spell);

    new_spell->vnum = SPELL_FORCESHIELD;
    new_spell->status = available;
    new_spell->name = strdup("forceshield");
    new_spell->type = SPELL;
    new_spell->min_pos = POS_STANDING;
    new_spell->targ_flags = TAR_CHAR_ROOM;
    new_spell->mag_flags = MAG_AFFECTS | MAG_ACCDUR | MAG_AURA;
    new_spell->effectiveness = strdup("100");
    new_spell->applies[0].appl_num = APPLY_AC;
    new_spell->applies[0].modifier = strdup("-5");
    new_spell->applies[0].duration = strdup("self.level");
    new_spell->applies[1].appl_num = AFF_FORCESHIELD + NUM_APPLIES;
    new_spell->applies[1].duration = strdup("self.level");
    new_spell->messages.to_vict = strdup("Uma translúcida aura de força envolve você.");
    new_spell->messages.to_room = strdup("Uma translúcida aura de força envolve $N.");
    new_spell->messages.wear_off = strdup("A aura de força ao redor de seu corpo desaparece.");
    new_spell->school = SCHOOL_ABJURATION;
    new_spell->element = ELEMENT_PHYSICAL;
    new_spell->prerequisite_spell = SPELL_FIRESHIELD;
    new_spell->discoverable = 1;
    spedit_save_internally(new_spell);

    spedit_save_to_disk();
}
