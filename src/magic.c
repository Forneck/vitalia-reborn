/**************************************************************************
*  File: magic.c                                           Part of tbaMUD *
*  Usage: Low-level functions for magic; spell template code.             *
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
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "class.h"
#include "fight.h"
#include "mud_event.h"
#include "spedit.h"
#include "formula.h"

/* local file scope function prototypes */
static int mag_materials(struct char_data *ch, IDXTYPE item0, IDXTYPE item1, IDXTYPE item2,
						 int extract, int verbose);


/* Negative apply_saving_throw[] values make saving throws better! So do
   negative modifiers.  Though people may be used to the reverse of that. It's 
   due to the code modifying the target saving throw instead of the random
   number of the character as in some other systems. */
int mag_savingthrow(struct char_data *ch, int type, int modifier)
{
	/* NPCs use warrior tables according to some book */
	int class_sav = CLASS_WARRIOR;
	int save;

	if (!IS_NPC(ch))
		class_sav = GET_CLASS(ch);

	save = saving_throws(class_sav, type, GET_LEVEL(ch));
	save += GET_SAVE(ch, type);
	save += modifier;

	/* Throwing a 0 is always a failure. */
	if (MAX(1, save) < rand_number(0, 99))
		return (TRUE);

	/* Oops, failed. Sorry. */
	return (FALSE);
}

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
	struct affected_type *af, *next;
	struct char_data *i;
	struct str_spells *spell;

	for (i = character_list; i; i = i->next)
		for (af = i->affected; af; af = next)
		{
			next = af->next;
			if (af->duration >= 1)
				af->duration--;
			else if (af->duration == -1)	/* No action */
				;
			else
			{
				if ((af->spell > 0) && (af->spell <= MAX_SKILLS))
				{
					if (!af->next || (af->next->spell != af->spell) || (af->next->duration > 0))
					{
						spell = get_spell_by_vnum(af->spell);
						if (spell && spell->messages.wear_off)
							send_to_char(i, "%s\r\n", spell->messages.wear_off);
					}
				}
				affect_remove(i, af);
			}
		}
}

/* Checks for up to 3 vnums (spell reagents) in the player's inventory. If
   multiple vnums are passed in, the function ANDs the items together as
   requirements (ie. if one or more are missing, the spell will not fail).
   @param ch The caster of the spell. @param item0 The first required item of
   the spell, NOTHING if not required. @param item1 The second required item
   of the spell, NOTHING if not required. @param item2 The third required item 
   of the spell, NOTHING if not required. @param extract TRUE if mag_materials 
   should consume (destroy) the items in the players inventory, FALSE if not.
   Items will only be removed on a successful cast. @param verbose TRUE to
   provide some generic failure or success messages, FALSE to send no in game
   messages from this function. @retval int TRUE if ch has all materials to
   cast the spell, FALSE if not. */
static int mag_materials(struct char_data *ch, IDXTYPE item0,
						 IDXTYPE item1, IDXTYPE item2, int extract, int verbose)
{
	/* Begin Local variable definitions. */
  /*------------------------------------------------------------------------*/
	/* Used for object searches. */
	struct obj_data *tobj = NULL;
	/* Points to found reagents. */
	struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;
  /*------------------------------------------------------------------------*/
	/* End Local variable definitions. */

	/* Begin success checks. Checks must pass to signal a success. */
  /*------------------------------------------------------------------------*/
	/* Check for the objects in the players inventory. */
	for (tobj = ch->carrying; tobj; tobj = tobj->next_content)
	{
		if ((item0 != NOTHING) && (GET_OBJ_VNUM(tobj) == item0))
		{
			obj0 = tobj;
			item0 = NOTHING;
		}
		else if ((item1 != NOTHING) && (GET_OBJ_VNUM(tobj) == item1))
		{
			obj1 = tobj;
			item1 = NOTHING;
		}
		else if ((item2 != NOTHING) && (GET_OBJ_VNUM(tobj) == item2))
		{
			obj2 = tobj;
			item2 = NOTHING;
		}
	}

	/* If we needed items, but didn't find all of them, then the spell is a
	   failure. */
	if ((item0 != NOTHING) || (item1 != NOTHING) || (item2 != NOTHING))
	{
		/* Generic spell failure messages. */
		if (verbose)
		{
			switch (rand_number(0, 2))
			{
			case 0:
				send_to_char(ch, "Uma verruga nasce no seu nariz.\r\n");
				break;
			case 1:
				send_to_char(ch, "Seus cabelos caem.\r\n");
				break;
			case 2:
				send_to_char(ch, "Um enorme caroço nasce no seu dedão.\r\n");
				break;
			}
		}
		/* Return fales, the material check has failed. */
		return (FALSE);
	}
  /*------------------------------------------------------------------------*/
	/* End success checks. */

	/* From here on, ch has all required materials in their inventory and the
	   material check will return a success. */

	/* Begin Material Processing. */
  /*------------------------------------------------------------------------*/
	/* Extract (destroy) the materials, if so called for. */
	if (extract)
	{
		if (obj0 != NULL)
			extract_obj(obj0);
		if (obj1 != NULL)
			extract_obj(obj1);
		if (obj2 != NULL)
			extract_obj(obj2);
		/* Generic success messages that signals extracted objects. */
		if (verbose)
		{
			send_to_char(ch, "Uma nuvem de fumaça sai do pacote.\r\n");
			act("Uma nuvem de fumaça sai do pacote de $n.", TRUE, ch, NULL, NULL, TO_ROOM);
		}
	}

	/* Don't extract the objects, but signal materials successfully found. */
	if (!extract && verbose)
	{
		send_to_char(ch, "O seu pacote começa a tremer.\r\n");
		act("Algo começa a tremer no pacote de $n.", TRUE, ch, NULL, NULL, TO_ROOM);
	}
  /*------------------------------------------------------------------------*/
	/* End Material Processing. */

	/* Signal to calling function that the materials were successfully found
	   and processed. */
	return (TRUE);
}


/* Every spell that does damage comes through here.  This calculates the
   amount of damage, adds in any modifiers, determines what the saves are,
   tests for save and calls damage(). -1 = dead, otherwise the amount of
   damage done. */
int mag_damage(int level, struct char_data *ch, struct char_data *victim,
			   int spellnum, int savetype)
{
	struct str_spells *spell;
	int dam = 0;
	int rts_code;

	if (victim == NULL || ch == NULL)
		return (0);

	spell = get_spell_by_vnum(spellnum);

	if (!spell)
	{
		log1("SYSERR: spell not found vnum %d passed to mag_damage.", spellnum);
		return dam;
	}

	if (!spell->damages)
	{
		log1("SYSERR: no damages set for spell vnum %d passed to mag_damage.", spellnum);
		return dam;
	}

	dam =
		MIN(spell->max_dam,
			MAX(0,
				formula_interpreter(ch, victim, spellnum, TRUE, spell->damages, level,
									&rts_code)));

	/* Apply weather modifier if enabled */
	if (CONFIG_WEATHER_AFFECTS_SPELLS && spell->element != ELEMENT_UNDEFINED) {
		float weather_modifier = get_weather_spell_modifier(ch, spell->element);
		dam = (int)(dam * weather_modifier);
	}

	/* Apply school-based weather modifier if enabled */
	if (CONFIG_SCHOOL_WEATHER_AFFECTS && spell->school != SCHOOL_UNDEFINED) {
		float school_modifier = get_school_weather_modifier(ch, spell->school);
		dam = (int)(dam * school_modifier);
	}

	// special spells that formula interpreter can't deal with.
	switch (spellnum)
	{
	case SPELL_DISPEL_EVIL:
		if (IS_EVIL(ch))
		{
			victim = ch;
			dam = GET_HIT(ch) - 1;
		}
		else if (IS_GOOD(victim))
		{
			act("Os deuses protegem $N.", FALSE, ch, 0, victim, TO_CHAR);
			return (0);
		}
		break;
	case SPELL_DISPEL_GOOD:
		if (IS_GOOD(ch))
		{
			victim = ch;
			dam = GET_HIT(ch) - 1;
		}
		else if (IS_EVIL(victim))
		{
			act("Os deuses protegem $N.", FALSE, ch, 0, victim, TO_CHAR);
			return (0);
		}
		break;
	}

	/* divide damage by two if victim makes his saving throw */
	if (mag_savingthrow(victim, savetype, 0))
		dam /= 2;

	/* and finally, inflict the damage */
	return (damage(ch, victim, dam, spellnum));
}


/* Every spell that does an affect comes through here.  This determines the
   effect, whether it is added or replacement, whether it is legal or not,
   etc. affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod) */
int mag_affects(int level, struct char_data *ch, struct char_data *victim,
				int spellnum, int savetype)
{
	struct affected_type af[MAX_SPELL_AFFECTS];
	bool accum_affect = FALSE, accum_duration = FALSE;
	int i, j, rts_code, affect, effect = 0;
	struct str_spells *spell;


	if (victim == NULL || ch == NULL)
		return MAGIC_FAILED;

	spell = get_spell_by_vnum(spellnum);
	if (!spell)
	{
		log1("SYSERR: unknown spellnum %d passed to mag_affects.", spellnum);
		return MAGIC_FAILED;
	}

	for (i = 0; i < MAX_SPELL_AFFECTS; i++)
	{
		new_affect(&(af[i]));
		af[i].spell = spellnum;
		int applnum = spell->applies[i].appl_num;

		if (applnum <= APPLY_NONE)
		{
			af[i].location = APPLY_NONE;
			af[i].modifier = 0;
		}
		else if (applnum < NUM_APPLIES)
		{
			af[i].location = spell->applies[i].appl_num;
			af[i].modifier =
				formula_interpreter(ch, victim, spellnum, TRUE, spell->applies[i].modifier, level,
									&rts_code);
		}
		else
		{
			af[i].location = spell->applies[i].appl_num;

			affect = applnum;
			SET_BIT_AR(af[i].bitvector, affect - NUM_APPLIES);
		}

		af[i].duration =
			MAX(1,
				formula_interpreter(ch, victim, spellnum, TRUE, spell->applies[i].duration, level,
									&rts_code));

		if (spell->mag_flags & MAG_ACCDUR)
			accum_duration = TRUE;

		if (spell->mag_flags & MAG_ACCMOD)
			accum_affect = TRUE;

		// specials spells that formula interpreter can't deal with:
		switch (spellnum)
		{
		case SPELL_CHILL_TOUCH:
			if (mag_savingthrow(victim, savetype, 0))
				af[i].duration = 1;
			break;
		case SPELL_BLINDNESS:
			if (MOB_FLAGGED(victim, MOB_NOBLIND) || GET_LEVEL(victim) >= LVL_IMMORT ||
				mag_savingthrow(victim, savetype, 0))
			{
				return MAGIC_FAILED;
			}
			break;
		case SPELL_CURSE:
			if (mag_savingthrow(victim, savetype, 0))
			{
				return MAGIC_FAILED;
			}
			break;
		case SPELL_INVISIBLE:
			if (!victim)
				victim = ch;
			break;
			case SPELL_FIRESHIELD:
			if (!victim)
				victim = ch;
			break;
			case SPELL_THISTLECOAT:
			if (!victim)
				victim = ch;
			break;
			case SPELL_WINDWALL:
			if (!victim)
				victim = ch;
			break;
		case SPELL_POISON:
			if (mag_savingthrow(victim, savetype, 0))
			{
				return MAGIC_FAILED;
			}
			break;
		case SPELL_SLEEP:
			if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
				return MAGIC_FAILED;
			if (MOB_FLAGGED(victim, MOB_NOSLEEP))
				return MAGIC_FAILED;
			if (mag_savingthrow(victim, savetype, 0))
				return MAGIC_FAILED;

			if (GET_POS(victim) > POS_SLEEPING)
			{
				effect++;
				GET_POS(victim) = POS_SLEEPING;
			}
			break;
		case SPELL_STRENGTH:
			if (GET_ADD(victim) == 100)
				return MAGIC_NOEFFECT;
		}
	}

	/* If this is a mob that has this affect set in its mob file, do not
	   perform the affect.  This prevents people from un-sancting mobs by
	   sancting them and waiting for it to fade, for example. */
	if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
	{
		for (i = 0; i < MAX_SPELL_AFFECTS; i++)
		{
			for (j = 1; j < NUM_AFF_FLAGS; j++)
			{
				if (IS_SET_AR(af[i].bitvector, j) && AFF_FLAGGED(victim, j))
					return MAGIC_NOEFFECT;
			}
		}
	}

	/* If the victim is already affected by this spell, and the spell does not
	   have an accumulative effect, then fail the spell. */
	if (affected_by_spell(victim, spellnum) && !(accum_duration || accum_affect))
		return MAGIC_NOEFFECT;

	for (i = 0; i < MAX_SPELL_AFFECTS; i++)
		if (af[i].bitvector[0] || af[i].bitvector[1] ||
			af[i].bitvector[2] || af[i].bitvector[3] || (af[i].location != APPLY_NONE))
		{
			affect_join(victim, af + i, accum_duration, FALSE, accum_affect, FALSE);
			effect++;
		}

	if (effect)
		return MAGIC_SUCCESS;
	else
		return MAGIC_NOEFFECT;
}

int mag_protections(int level, struct char_data *ch, struct char_data *tch,
					int spellnum, int spellprot, int dur, int res)
{
	struct str_spells *spell;
	struct affected_type af;
	int accum_duration = FALSE;

	if (tch == NULL || ch == NULL)
		return MAGIC_FAILED;

	spell = get_spell_by_vnum(spellnum);

	if (!spell)
	{
		log1("SYSERR: unknown spellnum %d passed to mag_affects.", spellnum);
		return MAGIC_FAILED;
	}

	if (spell->mag_flags & MAG_ACCDUR)
		accum_duration = TRUE;
	new_affect(&af);
	SET_BIT_AR(af.bitvector, AFF_PROTECT);
	af.spell = spellnum;
	af.location = spellprot;
	af.modifier = res;
	af.duration = dur;

	affect_join(tch, &af, accum_duration, FALSE, FALSE, FALSE);
	return MAGIC_SUCCESS;
}

/* This function is used to provide services to mag_groups.  This function is
   the one you should change to add new group spells. */
static int perform_mag_groups(int level, struct char_data *ch,
							  struct char_data *tch, int spellnum, int savetype)
{
	int flags = 0;

	switch (spellnum)
	{
	case SPELL_GROUP_HEAL:
		flags = mag_points(level, ch, tch, SPELL_HEAL, savetype);
		break;
	case SPELL_GROUP_ARMOR:
		flags = mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
		break;
	case SPELL_GROUP_RECALL:
		spell_recall(level, ch, tch, NULL);
		flags = MAGIC_SUCCESS;
		break;
	}

	return flags;
}

/* Every spell that affects the group should run through here
   perform_mag_groups contains the switch statement to send us to the right
   magic. Group spells affect everyone grouped with the caster who is in the
   room, caster last. To add new group spells, you shouldn't have to change
   anything in mag_groups. Just add a new case to perform_mag_groups. */
int mag_groups(int level, struct char_data *ch, int spellnum, int savetype)
{
	int ret_flags = 0;

	struct char_data *tch;

	if (ch == NULL)
		return MAGIC_FAILED;

	if (!GROUP(ch))
		return MAGIC_FAILED;

	while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
	{
		if (IN_ROOM(tch) != IN_ROOM(ch))
			continue;
		if (tch == ch)
			continue;
		ret_flags |= perform_mag_groups(level, ch, tch, spellnum, savetype);
	}
	ret_flags |= perform_mag_groups(level, ch, ch, spellnum, savetype);

	if (ret_flags & MAGIC_SUCCESS)
		return MAGIC_SUCCESS;

	if (ret_flags == MAGIC_FAILED)
		return MAGIC_FAILED;

	return MAGIC_NOEFFECT;
}


/* Mass spells affect every creature in the room except the caster. No spells
   of this class currently implemented. */
int mag_masses(int level, struct char_data *ch, int spellnum, int savetype)
{
	struct char_data *tch, *tch_next;

	for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next)
	{
		tch_next = tch->next_in_room;
		if (tch == ch)
			continue;

		switch (spellnum)
		{
		}
	}

	// return NOEFFECT for now since mag_masses doesn't support any spells
	// yet.
	return MAGIC_NOEFFECT;
}

/* Every spell that affects an area (room) runs through here.  These are
   generally offensive spells.  This calls mag_damage to do the actual damage.
   All spells listed here must also have a case in mag_damage() in order for
   them to work. Area spells have limited targets within the room. */
int mag_areas(int level, struct char_data *ch, int spellnum, int savetype)
{
	struct char_data *tch, *next_tch;
	struct str_spells *spell;
	int effect = 0;

	if (ch == NULL)
		return MAGIC_FAILED;

	spell = get_spell_by_vnum(spellnum);

	for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
	{
		next_tch = tch->next_in_room;

		/* The skips: 1: the caster 2: immortals 3: if no pk on this mud,
		   skips over all players 4: pets (charmed NPCs) 5: other players in
		   the same group (if the spell is 'violent') 6: Flying people if
		   earthquake is the spell */
		if (tch == ch)
			continue;
		if (!IS_NPC(tch) && GET_LEVEL(tch) >= LVL_IMMORT)
			continue;
		if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(tch))
			continue;
		if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
			continue;
		if (!IS_NPC(tch) && (spell->mag_flags & MAG_VIOLENT) && GROUP(tch) && GROUP(ch)
			&& GROUP(ch) == GROUP(tch))
			continue;
		if ((spellnum == SPELL_EARTHQUAKE) && AFF_FLAGGED(tch, AFF_FLYING))
			continue;
		/* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
		effect++;
		mag_damage(level, ch, tch, spellnum, 1);
	}

	if (effect)
		return MAGIC_SUCCESS;
	else
		return MAGIC_NOEFFECT;
}

/*----------------------------------------------------------------------------*/
/* Begin Magic Summoning - Generic Routines and Local Globals */
/*----------------------------------------------------------------------------*/

/* Every spell which summons/gates/conjours a mob comes through here. */

/* Keep the \r\n because these use send_to_char. */
static const char *mag_summon_fail_msgs[] = {
	"\r\n",
 "Não existe tais criaturas.\r\n",
  "Oh ou...\r\n",
  "Nada feito...\r\n",
  "Oh Deus!\r\n",
  "Os elementos se opõe!\r\n",
  "Você falhou.\r\n",
  "Não há corpo!\r\n"
};

int mag_summons(int level, struct char_data *ch, struct obj_data *obj, int spellnum, int savetype)
{
	struct char_data *mob = NULL;
	struct obj_data *tobj, *next_obj;
	int pfail = 0, fmsg = 0, handle_corpse = FALSE;
	mob_vnum mob_num;
	obj_vnum obj_num;
	struct str_spells *spell;
	int rts_code;

	if (ch == NULL)
		return MAGIC_FAILED;

	spell = get_spell_by_vnum(spellnum);

	if (!spell)
	{
		log1("SYSERR: unknown spellnum %d passed to mag_summons.", spellnum);
		return MAGIC_FAILED;
	}

	if (spellnum == SPELL_ANIMATE_DEAD)
	{
		if (obj == NULL || !IS_CORPSE(obj))
		{
			act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
			return MAGIC_FAILED;
		}
		handle_corpse = TRUE;
	}

	fmsg = rand_number(2, 6);	/* Random fail message. */
	if (spell->summon_mob)
	{
		mob_num =
			MAX(0,
				formula_interpreter(ch, ch, spellnum, TRUE, spell->summon_mob, level, &rts_code));

		// mob_num is unsigned, it won't catch negative value from
		// formula_interpreter.
		// I'm using MAX(0, formula), and consider 0 illegal.
		if (mob_num == 0)
		{
			log1("SYSERR: Illegal mobile to summon at mag_summons.");
			send_to_char(ch,"Você não se lembra muito bem como criar tal criatura.\r\n");
			return MAGIC_FAILED;
		}
	}
	else
	{
		log1("SYSERR: No mobile to summon at mag_summons.");
		send_to_char(ch,"Você não se lembra muito bem como criar tal criatura.\r\n");
		return MAGIC_FAILED;
	}

	if (spell->summon_req)
	{
		pfail = 0;
		obj_num =
			MAX(0,
				formula_interpreter(ch, ch, spellnum, TRUE, spell->summon_req, level, &rts_code));

		// same as mob_num.
		if (obj_num == 0)
		{
			log1("SYSERR: Illegal object required to summon at mag_summons.");
			send_to_char(ch,"Você não consegue lembrar qual o componente para criar tal criatura.\r\n");
			pfail = 102;
		}
		else if (!mag_materials(ch, obj_num, NOTHING, NOTHING, TRUE, TRUE)) {
			send_to_char(ch,"Você não tem o componente certo para criar tal criatura.\r\n");
			pfail = 102;
		}
	}
	else
		pfail = 10;				/* 10% failure, should vary in the future. */

	if (AFF_FLAGGED(ch, AFF_CHARM))
	{
		send_to_char(ch, "Você é muito levian$R para ter qualquer seguidor!\r\n");
		return MAGIC_NOEFFECT;
	}

	if (rand_number(0, 101) < pfail)
	{
		send_to_char(ch, "%s", mag_summon_fail_msgs[fmsg]);
		return MAGIC_NOEFFECT;	// since we send a special fail message,
								// return NOEFFECT makes more sens.
	}

	if (!(mob = read_mobile(mob_num, VIRTUAL)))
	{
		send_to_char(ch, "Você não se lembra muito bem como criar tal criatura.\r\n");
		return MAGIC_NOEFFECT;
	}
	char_to_room(mob, IN_ROOM(ch));
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;
	SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

	if (spellnum == SPELL_CLONE)
	{
		/* Don't mess up the prototype; use new string copies. */
		mob->player.name = strdup(GET_NAME(ch));
		mob->player.short_descr = strdup(GET_NAME(ch));
	}

	load_mtrigger(mob);
	add_follower(mob, ch);

	if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
		join_group(mob, GROUP(ch));

	if (handle_corpse)
	{
		for (tobj = obj->contains; tobj; tobj = next_obj)
		{
			next_obj = tobj->next_content;
			obj_from_obj(tobj);
			obj_to_char(tobj, mob);
		}
		extract_obj(obj);
	}
	return MAGIC_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/* End Magic Summoning - Generic Routines and Local Globals */
/*----------------------------------------------------------------------------*/


int mag_points(int level, struct char_data *ch, struct char_data *victim,
			   int spellnum, int savetype)
{
	struct str_spells *spell;

	int hp, mana, move, gold, rts_code, effect = 0;

	if (victim == NULL)
		return MAGIC_FAILED;

	spell = get_spell_by_vnum(spellnum);

	if (!spell)
	{
		log1("SYSERR: unknown spellnum %d passed to mag_points.", spellnum);
		return MAGIC_FAILED;
	}

	if (spell->points.hp)
	{
		hp = formula_interpreter(ch, victim, spellnum, TRUE, spell->points.hp, level, &rts_code);
		GET_HIT(victim) = MIN(GET_MAX_HIT(victim), MAX(1, GET_HIT(victim) + hp));
		effect++;
	}

	if (spell->points.mana)
	{
		mana =
			formula_interpreter(ch, victim, spellnum, TRUE, spell->points.mana, level, &rts_code);
		GET_MANA(victim) = MIN(GET_MAX_MANA(victim), MAX(0, GET_MANA(victim) + mana));
		effect++;
	}

	if (spell->points.move)
	{
		move =
			formula_interpreter(ch, victim, spellnum, TRUE, spell->points.move, level, &rts_code);
		GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), MAX(0, GET_MOVE(victim) + move));
		effect++;
	}

	if (spell->points.gold)
	{
		gold =
			formula_interpreter(ch, victim, spellnum, TRUE, spell->points.gold, level, &rts_code);
		GET_GOLD(victim) = MAX(0, GET_GOLD(victim) + gold);
		effect++;
	}

	update_pos(victim);
	if (effect)
		return MAGIC_SUCCESS;
	else
		return MAGIC_NOEFFECT;
}

int mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
				  int spellnum, int type)
{
	int i, dispel = 0, effect = 0, rts_code;
	struct str_spells *spell;

	if (victim == NULL)
		return MAGIC_FAILED;

	spell = get_spell_by_vnum(spellnum);
	if (!spell)
	{
		log1("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
		return MAGIC_FAILED;
	}

	// complementary messages for the spell HEAL.
	if (((spellnum == SPELL_HEAL) || ( spellnum = SPELL_AID)) && (affected_by_spell(victim, SPELL_BLINDNESS)))
	{
		act("Sua visão retorna!", FALSE, victim, 0, ch, TO_CHAR);
		act("Os olhos de $n brilham momentaneamente.", TRUE, victim, 0, ch, TO_ROOM);
	}

	for (i = 0; i < MAX_SPELL_DISPEL; i++)
	{
		if (spell->dispel[i])
			dispel =
				formula_interpreter(ch, victim, spellnum, TRUE, spell->dispel[i], level,
									&rts_code);
		else
			continue;

		if (dispel <= 0)
			log1("SYSERR: Invalid dispel spell vnum: %d", dispel);
		else if (affected_by_spell(victim, dispel))
		{
			effect++;
			affect_from_char(victim, dispel);
		}
	}

	if (effect)
		return MAGIC_SUCCESS;
	else
		return MAGIC_NOEFFECT;
}

int mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
				   int spellnum, int savetype)
{
	int effect = 1;
        
	if (obj == NULL)
		return MAGIC_FAILED;

	switch (spellnum)
	{
	case SPELL_CURSE:
		if (!OBJ_FLAGGED(obj, ITEM_NODROP))
		{
			SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
			if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
				GET_OBJ_VAL(obj, 2)--;
			send_to_char(ch,"Isto brilha vermelho por alguns instantes.\r\n");
		}
		break;
	case SPELL_INVISIBLE:
		if (!OBJ_FLAGGED(obj, ITEM_NOINVIS) && !OBJ_FLAGGED(obj, ITEM_INVISIBLE))
		{
			SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
			send_to_char(ch,"Isto desaparece.\r\n");
		}
		break;
	case SPELL_POISON:
		if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
			 (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
			 (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3))
		{
			GET_OBJ_VAL(obj, 3) = 1;
			send_to_char(ch,"Isto emite uma fumaça escura por alguns instantes.\r\n");
		}
		break;
	case SPELL_REMOVE_CURSE:
		if (OBJ_FLAGGED(obj, ITEM_NODROP))
		{
			REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
			if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
				GET_OBJ_VAL(obj, 2)++;
			send_to_char(ch,"Isto brilha azul por uns instantes.\r\n");
		}
		break;
	case SPELL_REMOVE_POISON:
		if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
			 (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
			 (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3))
		{
			GET_OBJ_VAL(obj, 3) = 0;
			send_to_char(ch,"Isto emite uma fumaça clara por alguns instantes.\r\n");
		}
		break;
	default:
		effect = 0;
	}

	if (effect)
		return MAGIC_SUCCESS;
	else
		return MAGIC_NOEFFECT;
}

int mag_creations(int level, struct char_data *ch, int spellnum)
{
	struct obj_data *tobj;
	struct str_spells *spell;
	obj_vnum z;
	int i, rts_code, effect = 0, goofed = 0;

	if (ch == NULL)
		return MAGIC_FAILED;

	spell = get_spell_by_vnum(spellnum);

	if (!spell)
	{
		log1("SYSERR: spell_creations, spell %d not found", spellnum);
		return MAGIC_FAILED;
	}

	for (i = 0; i < MAX_SPELL_OBJECTS; i++)
	{
		if (spell->objects[i])
		{
			z = MAX(0,
					formula_interpreter(ch, ch, spellnum, TRUE, spell->objects[i], level,
										&rts_code));

			if (!rts_code && z)
			{
				if (!(tobj = read_object(z, VIRTUAL)))
				{
					goofed = 1;
					log1("SYSERR: spell_creations, spell %d, obj %d: obj not found", spellnum, z);
				}
				else
				{
					obj_to_char(tobj, ch);
					act("$n criou $p.", FALSE, ch, tobj, 0, TO_ROOM);
					act("Você criou $p.", FALSE, ch, tobj, 0, TO_CHAR);
					load_otrigger(tobj);
					effect++;
				}
			}
			else
				log1("SYSERR: spell_creations, formula interpreter failed on spell %d", spellnum);
		}
	}

	// special message, if object(s) creation failed.
	if (goofed)
		send_to_char(ch, "Eu pareço ter me atrapalhado%s\r\n", effect ? " um pouco." : ".");

	if (effect)
		return MAGIC_SUCCESS;
	else
		return MAGIC_NOEFFECT;
}

int mag_rooms(int level, struct char_data *ch, int spellnum)
{
	room_rnum rnum;
	int duration = 0;
	bool failure = FALSE;
	event_id IdNum = eNULL;


	rnum = IN_ROOM(ch);

	if (ROOM_FLAGGED(rnum, ROOM_NOMAGIC))
		failure = TRUE;

	switch (spellnum)
	{
	case SPELL_DARKNESS:
		IdNum = eSPL_DARKNESS;
		if (ROOM_FLAGGED(rnum, ROOM_DARK))
			failure = TRUE;

		duration = 10;
		SET_BIT_AR(ROOM_FLAGS(rnum), ROOM_DARK);
		break;

	}

	if (failure || IdNum == eNULL)
		return MAGIC_FAILED;

	NEW_EVENT(eSPL_DARKNESS, &world[rnum], NULL, duration * PASSES_PER_SEC);

	return MAGIC_SUCCESS;
}
