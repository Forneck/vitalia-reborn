/**
* @file spells.h
* Constants and function prototypes for the spell system.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _SPELLS_H_
#define _SPELLS_H_

#define MAGIC_NOEFFECT  (1 << 0)
#define MAGIC_FAILED    (1 << 1)
#define MAGIC_SUCCESS   (1 << 2)

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12

#define CAST_UNDEFINED	(-1)
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)
#define MAG_ROOMS   (1 << 11)
#define MAG_VIOLENT     (1 << 12)
#define MAG_ACCDUR      (1 << 13)
#define MAG_ACCMOD      (1 << 14)
#define MAG_PROTECTION  (1 << 15)
#define NUM_MAG         16

#define TYPE_UNDEFINED               (-1)
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */
#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD           45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD            46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_ARMOR            47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL             48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL           49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION            50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK              51 /* Reserved Skill[] DO NOT CHANGE */ 
#define SPELL_IDENTIFY               52 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STONESKIN		     53 /* -- vp -			 */
#define SPELL_FIRESHIELD	     54 /* -- vp -			 */
#define SPELL_IMPROVED_ARMOR	     55 /* -- vp -			 */
#define SPELL_DISINTEGRATE	     56 /* -- vp -			 */
#define SPELL_EVOKE_AIR_SERVANT    57 /* -- vp -			 */
#define SPELL_TALKDEAD		     58 /* -- vp -			 */
#define SPELL_RAISE_DEAD	     59 /* -- vp -			 */
#define SPELL_RESSURECT		     60 /* -- vp -			 */
#define SPELL_VAMP_TOUCH	     61 /* -- vp -			 */
#define SPELL_BREATH		     62 /* -- vp -			 */
#define SPELL_FLY		     63 /* -- vp -			 */
#define SPELL_PARALYSE		     64 /* -- vp -			 */
#define SPELL_AID		     65 /* -- jr - Set 07, 1999		 */
#define SPELL_PORTAL		     66 /* -- jr - Set 24, 1999		 */
#define SPELL_SKIN_LIKE_WOOD	     67 /* -- mp - Oct 13, 2001		 */
#define SPELL_SKIN_LIKE_ROCK	     68 /* -- mp - Oct 13, 2001		 */
#define SPELL_SKIN_LIKE_STEEL	     69 /* -- mp - Oct 13, 2001		 */
#define SPELL_SKIN_LIKE_DIAMOND	     70 /* -- mp - Oct 13, 2001		 */
#define SPELL_BURST_OF_FLAME	     71 /* -- ac - Oct 13, 2001		 */
#define SPELL_BURST_OF_FIRE	     72 /* -- ac - Oct 13, 2001		 */
#define SPELL_IGNITE		     73 /* -- ac - Oct 13, 2001		 */
#define SPELL_INVIGOR		     74 /* -- mp - Oct 13, 2001		 */
#define SPELL_MINOR_HEALING	     75 /* -- ac - Oct 14, 2001		 */
#define SPELL_LIGHTNING_BLAST	     76 /* -- ac - Oct 14, 2001		 */
#define SPELL_REGENERATION	     77 /* -- mp - Oct 13, 2001		 */
#define SPELL_THISTLECOAT	     78 /* -- mp -			 */
#define SPELL_CASCADE_HAIL	     79 /* -- mp -			 */
#define SPELL_DANCE_FIREFLIES	     80 /* -- mp -			 */
#define SPELL_STINGING_SWARM	     81 /* -- mp -			 */
#define SPELL_FURY_OF_AIR	     82 /* -- mp - Nov 04, 2001		 */
#define SPELL_EVOKE_CROW	     83 /* -- mp -			 */
#define SPELL_EVOKE_WOLF	     84 /* -- mp -			 */
#define SPELL_EVOKE_BEAR	     85 /* -- mp -			 */
#define SPELL_EVOKE_LION	     87 /* -- mp -			 */
#define SPELL_CREATE_NECTAR	     88 /* -- mp -			 */
#define SPELL_VOICE_EXPLOSION	     89 /* -- ac - March, 2003		 */
#define SPELL_SOUNDBARRIER	     90 /* -- ac - March, 2003		 */
#define SPELL_GLOOMSHIELD	     91 /* -- ac - September, 2003	 */

#define SPELL_DARKNESS               92
#define SPELL_TRANSPORT_VIA_PLANTS      93
#define SPELL_PROT_FROM_GOOD         94
#define SPELL_CREATE_BERRIES 95
#define SPELL_WINDWALL 96
/* To make an affect induced by dg_affect look correct on 'stat' we need to define it with a 'spellname'. */
#define SPELL_DG_AFFECT              97

/** Total Number of defined spells */
#define NUM_SPELLS                   97

/* Insert new spells here, up to MAX_SPELLS */
#define MAX_SPELLS		    160

/* PLAYER CHANSONS - Numbered from MAX_SPELLS+1 to MAX_CHANSON */
#define CHANSON_HEAD		    161 /* First chanson, just for ctrl	 */
#define CHANSON_ARDOR		    161 /* -- AC - JUNE, 2003		 */
#define CHANSON_OFUSCAR		    162 /* -- AC - JUNE, 2003		 */
#define CHANSON_NINAR		    163 /* -- AC - JUNE, 2003		 */
#define CHANSON_TREMOR		    164 /* -- AC - JUNE, 2003		 */
#define CHANSON_ANDARILHO	    165 /* -- AC - JUNE, 2003		 */
#define CHANSON_MORTE		    166 /* -- AC - JUNE, 2003		 */
#define CHANSON_LEVEZA		    167 /* -- AC - JUNE, 2003		 */
#define CHANSON_ALENTO		    168 /* -- AC - JUNE, 2003 * tabela		 */
#define CHANSON_DEUSES		    169 /* -- AC - JUNE, 2003		 */
#define CHANSON_VULNERAVEL	    170 /* -- AC - JUNE, 2003		 */
#define CHANSON_VOLTAR		    171 /* -- AC - JUNE, 2003 manual	 */
#define CHANSON_BRINDE		    172 /* -- AC - JUNE, 2003 manual-ok	 */
#define CHANSON_ECOS		    173 /* -- AC - JUNE, 2003		 */
#define CHANSON_CLAMOR		    174 /* -- AC - JUNE, 2003		 */
#define CHANSON_ENCANTO             175 /* -- AC - SEPT, 2003 manual-ok  */
#define CHANSON_CESSAR              176 /* -- AC - SEPT, 2003            */
#define CHANSON_VISAO               177 /* -- AC - SEPT, 2003            */

/* Insert new songs here, up to MAX_CHANSONS */
#define MAX_CHANSONS		    220


/* PLAYER SKILLS - Numbered from MAX_SONGS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB              221 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                  222 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                  223 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                  224 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK             225 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PUNCH                 226 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE                227 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                 228 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                 229 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK		    230 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_WEAPON_SWORDS	    231 /* -- jr - 24/08/99 * Wpn-prof	 */
#define SKILL_WEAPON_DAGGERS	    232 /* -- jr - 24/08/99 * Wpn-prof	 */
#define SKILL_WEAPON_WHIPS	    233 /* -- jr - 24/08/99 * Wpn-prof	 */
#define SKILL_WEAPON_TALONOUS_ARMS  234 /* -- jr - 24/08/99 * Wpn-prof	 */
#define SKILL_WEAPON_BLUDGEONS	    235 /* -- jr - 24/08/99 * Wpn-prof	 */
#define SKILL_WEAPON_EXOTICS	    236 /* -- jr - 24/08/99 * Wpn-prof	 */
#define SKILL_SPY		    237 /* -- jr - May 15, 2000		 */
#define SKILL_SEIZE		    238 /* -- jr - Sep 15, 2000		 */
#define SKILL_BACKFLIP		    239	/* -- jr - Apr 07, 2001		 */
#define SKILL_BANDAGE		    240 /* -- jr - Apr 07, 2001		 */
#define SKILL_ENVENOM		    241	/* -- jr - Jul 19, 2001		 */
#define SKILL_NIGHTHAMMER	    242	/* -- jr - Oct 07, 2001		 */
#define SKILL_MEDITATE		    243 /* -- jr - Dec 29, 2001		 */
#define SKILL_ESCAPE		    244 /* -- jr - Dec 29, 2001		 */
#define SKILL_COMBO_ATTACK	    245	/* -- jr - Dec 29, 2001		 */
#define SKILL_WEAPON_POLEARMS	    246	/* -- AC - Apr, 2003		 */
#define SKILL_ADAGIO		    247	/* -- AC - Apr, 2003		 */
#define SKILL_ALLEGRO		    248	/* -- AC - Apr, 2003		 */
#define SKILL_TRIP		    249	/* -- AC - JUNE, 2003		 */
#define SKILL_WHIRLWIND             250
#define SKILL_SCAN 251
#define SKILL_BOWS 252

/* New skills may be added here up to MAX_SKILLS (297) */

/* NON-PLAYER AND OBJECT SPELLS AND SKILLS: The practice levels for the spells
 * and skills below are _not_ recorded in the players file; therefore, the
 * intended use is for spells and skills associated with objects (such as
 * SPELL_IDENTIFY used with scrolls of identify) or non-players (such as NPC
 * only spells). */
 #define SPELL_SCROLL_IDENTIFY		    301
#define SPELL_FIRE_BREATH	    302
#define SPELL_GAS_BREATH	    303
#define SPELL_FROST_BREATH	    304
#define SPELL_ACID_BREATH	    305
#define SPELL_LIGHTNING_BREATH	    306
#define SPELL_YOUTH		    307
#define SPELL_FIRESTORM		    308	/* -- jr - Apr 08, 2001	 */

#define TOP_SPELL_DEFINE	    399
/* NEW NPC/OBJECT SPELLS can be inserted here up to 399 */


/* WEAPON ATTACK TYPES */
#define TYPE_HIT		    400
#define TYPE_STING		    401
#define TYPE_WHIP		    402
#define TYPE_SLASH		    403
#define TYPE_BITE		    404
#define TYPE_BLUDGEON		    405
#define TYPE_CRUSH		    406
#define TYPE_POUND		    407
#define TYPE_CLAW		    408
#define TYPE_MAUL		    409
#define TYPE_THRASH		    410
#define TYPE_PIERCE		    411
#define TYPE_BLAST		    412
#define TYPE_PUNCH		    413
#define TYPE_STAB		    414
#define TYPE_BORE		    415
#define TYPE_BROACH		    416
#define TYPE_MOW		    417
/** The total number of attack types */
#define NUM_ATTACK_TYPES  18

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING		    499

/* special attack types */
#define TYPE_FIRING		    500 /* For use of FireShield Spell */
#define TYPE_HUNGRY		    501
#define TYPE_THIRSTY		    502
#define TYPE_FALLING		    503
#define TYPE_BURNING		    504
#define TYPE_COLD		    505
#define TYPE_DROWNING		    506

#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4

/***
 **Possible Targets:
 **  TAR_IGNORE    : IGNORE TARGET.
 **  TAR_CHAR_ROOM : PC/NPC in room.
 **  TAR_CHAR_WORLD: PC/NPC in world.
 **  TAR_FIGHT_SELF: If fighting, and no argument, select tar_char as self.
 **  TAR_FIGHT_VICT: If fighting, and no argument, select tar_char as victim (fighting).
 **  TAR_SELF_ONLY : If no argument, select self, if argument check that it IS self.
 **  TAR_NOT_SELF  : Target is anyone else besides self.
 **  TAR_OBJ_INV   : Object in inventory.
 **  TAR_OBJ_ROOM  : Object in room.
 **  TAR_OBJ_WORLD : Object in world.
 **  TAR_OBJ_EQUIP : Object held.
 **  TAR_GROUP     : PC in group.
 **  TAR_GROUP_VICT: Pc in victim's group.
 ***/
#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF   	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP	  (1 << 10)
#define TAR_GROUP       (1 << 11)
#define TAR_GROUP_VICT  (1 << 12)

#define PORTAL_VNUM 32
/* Possible Targets:
   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self. */
#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4

#define ASPELL(spellname) \
void	spellname(int level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict);

ASPELL(spell_create_water);
ASPELL(spell_create_nectar);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_ventriloquate); /* NEW */
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_control_weather);
ASPELL(spell_transport_via_plants);
ASPELL(spell_raise_dead); /* NEW */
ASPELL(spell_ressurect); /* NEW */
ASPELL(spell_vamp_touch); /* NEW */
ASPELL(spell_youth); /* NEW */
ASPELL(spell_portal);		/* -- jr - 24/09/99 */
ASPELL(spell_fury_air);		/* -- mp */
ASPELL(spell_skinrock);		/* -- mp */
ASPELL(spell_skinsteel);	/* -- mp */
ASPELL(spell_skindiamond);	/* -- mp */
ASPELL(spell_create_nectar);	/* -- mp */
ASPELL(spell_soundbarrier);	/* -- ac */
ASPELL(chanson_voltar);
ASPELL(chanson_brinde);
ASPELL(chanson_encanto);

/* basic magic calling functions */

int mag_protections(int level, struct char_data *ch, struct char_data *tch,
                 int spellnum, int spellprot, int dur, int res);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

int mag_affects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

int mag_groups(int level, struct char_data *ch, int spellnum, int savetype);

int mag_masses(int level, struct char_data *ch, int spellnum, int savetype);

int mag_areas(int level, struct char_data *ch, int spellnum, int savetype);

int mag_rooms(int level, struct char_data *ch, int spellnum);

int mag_summons(int level, struct char_data *ch, struct obj_data *obj,
 int spellnum, int savetype);

int mag_points(int level, struct char_data *ch, struct char_data *victim,
 int spellnum, int savetype);

int mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int type);

int mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
  int spellnum, int type);

int mag_creations(int level, struct char_data *ch, int spellnum);

int	call_magic(struct char_data *caster, struct char_data *cvict,
  struct obj_data *ovict, int spellnum, int level, int casttype);

void	mag_objectmagic(struct char_data *ch, struct obj_data *obj,
			char *argument);

int	cast_spell(struct char_data *ch, struct char_data *tch,
  struct obj_data *tobj, int spellnum);

/* other prototypes */
void spell_level(int spell, int chclass, int level);
const char *skill_name(int num);

/* From magic.c */
int mag_savingthrow(struct char_data *ch, int type, int modifier);
void affect_update(void);

/* from spell_parser.c */
ACMD(do_cast);

/* Global variables */
extern char cast_arg2[];

#endif /* _SPELLS_H_ */
