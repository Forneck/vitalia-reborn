/**************************************************************************
*  File: spell_parser.c                                    Part of tbaMUD *
*  Usage: Top-level magic routines; outside points of entry to magic sys. *
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
#include "config.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h"  /* for hit() */
#include "spedit.h"
#include "spells_script.h"
#include "formula.h"
#include "utils.h"
#include "constants.h"

/* Global Variables definitions, used elsewhere */
char cast_arg2[MAX_INPUT_LENGTH];

/* Local (File Scope) Function Prototypes */
static void say_spell(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj);

/* Local (File Scope) Variables */
struct syllable {
  const char *org;
  const char *news;
};
static struct syllable syls[] = {
  {" ", " "},
  {"ar", "abra"},
  {"ate", "i"},
  {"cau", "kada"},
  {"blind", "nose"},
  {"bur", "mosa"},
  {"cu", "judi"},
  {"de", "oculo"},
  {"dis", "mar"},
  {"ect", "kamina"},
  {"en", "uns"},
  {"gro", "cra"},
  {"light", "dies"},
  {"lo", "hi"},
  {"magi", "kari"},
  {"mon", "bar"},
  {"mor", "zak"},
  {"move", "sido"},
  {"ness", "lacri"},
  {"ning", "illa"},
  {"per", "duda"},
  {"ra", "gru"},
  {"re", "candus"},
  {"son", "sabru"},
  {"tect", "infra"},
  {"tri", "cula"},
  {"ven", "nofo"},
  {"word of", "inset"},
  {"fire","hi"},
  {"shield","tate"},
  {"stone","seki"},
  {"skin","gawa"},
  {"thistle","toge"},
  {"coat","gaitou"},
  {"rock","iwa"},
  {"steel","hagane"},
  {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"},
  {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"},
  {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"},
  {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}
};


void call_ASPELL (void (*function) (), int level, struct char_data *ch,
                  struct char_data *vict, struct obj_data *obj)
{
 (*function)(level, ch, vict, obj);
}

void call_ACMD (void (*function) (), struct char_data *ch, 
                char *argument, int cmd, int subcmd)
{
 (*function)(ch, argument, cmd, subcmd);
}

int mag_manacost(struct char_data *ch, struct char_data *tch, int spellnum)
{
  struct str_spells *spell;
  int mana, num, rts_code;

  spell = get_spell_by_vnum(spellnum);

  if (!spell) {
    log1("SYSERR: spell not found vnum %d passed to mag_manacost.", spellnum);
    return 100; 
  }

  num = get_spell_class(spell, GET_CLASS(ch));
  if (num == -1) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
      log1("SYSERR: spell vnum %d not assigned to class: %d"
           ", passed to mag_manacost.", spellnum, GET_CLASS(ch));
      return 100; 
    } 
    else
      return 0;
  }
  
  mana = MAX(5, formula_interpreter (ch, tch, spellnum, TRUE, spell->assign[num].num_mana, GET_LEVEL(ch), &rts_code));

  return mana;
}

static void say_spell(struct char_data *ch, int spellnum, struct char_data *tch,
	            struct obj_data *tobj)
{
  char lbuf[256], buf[256], buf1[256], buf2[256];	/* FIXME */
  const char *format;

  struct char_data *i;
  int j, ofs = 0;

  *buf = '\0';
  
  strlcpy(lbuf, get_spell_name(spellnum), sizeof(lbuf));

  while (lbuf[ofs]) {
    for (j = 0; *(syls[j].org); j++) {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
	strcat(buf, syls[j].news);	/* strcat: BAD */
	ofs += strlen(syls[j].org);
        break;
      }
    }
    /* i.e., we didn't find a match in syls[] */
    if (!*syls[j].org) {
      log1("No entry in syllable table for substring of '%s'", lbuf);
      ofs++;
    }
  }

  if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch)) {
    if (tch == ch)
      format = "$n fecha os olhos $s e pede para '%s'.";
    else
      format = "$n olha para $N e pede para, '%s'.";
  } else if (tobj != NULL &&
	     ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
    format = "$n olha para $p e pede, '%s'.";
  else
    format = "$n murmura as palavras, '%s'.";

  snprintf(buf1, sizeof(buf1), format, skill_name(spellnum));
  snprintf(buf2, sizeof(buf2), format, buf);

  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
    if (i == ch || i == tch || !i->desc || !AWAKE(i))
      continue;
    if (GET_CLASS(ch) == GET_CLASS(i))
      perform_act(buf1, ch, tobj, tch, i);
    else
      perform_act(buf2, ch, tobj, tch, i);
  }

  if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch)) {
    snprintf(buf1, sizeof(buf1), "$n olha para você e murmura as palavras, '%s'.",
	    GET_CLASS(ch) == GET_CLASS(tch) ? skill_name(spellnum) : buf);
    act(buf1, FALSE, ch, NULL, tch, TO_VICT);
  }
}

/* This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE. */
const char *skill_name(int num)
{
  return get_spell_name(num);
}

/* This function is the very heart of the entire magic system.  All invocations
 * of all types of magic -- objects, spoken and unspoken PC and NPC spells, the
 * works -- all come through this function eventually. This is also the entry
 * point for non-spoken or unrestricted spells. Spellnum 0 is legal but silently
 * ignored here, to make callers simpler. */
int call_magic(struct char_data *caster, struct char_data *cvict,
	     struct obj_data *ovict, int spellnum, int level, int casttype)
{
  int savetype;
  int i, dur, res, rts_code; 
  int damages, flags = 0;
  struct str_spells *spell;
  struct affected_type *af;

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
    return (0);

  if (!cast_wtrigger(caster, cvict, ovict, spellnum))
    return 0;
  if (!cast_otrigger(caster, ovict, spellnum))
    return 0;
  if (!cast_mtrigger(caster, cvict, spellnum))
    return 0;

  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC)) {
    send_to_char(caster, "A sua magia enfraquece e morre.\r\n");
    act("A magia de $n enfraquece e morre.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }

  spell = get_spell_by_vnum(spellnum); 
  
  if (!spell) {
    log1("SYSERR: spell not found vnum %d passed to call_magic.", spellnum);
    return 0;
  }

  if (spell->status == unavailable) {
    send_to_char (caster, "%s", CONFIG_NOEFFECT);
    return 0;
  }

  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
     ((spell->mag_flags & MAG_DAMAGE) || (spell->mag_flags & MAG_VIOLENT))) {
    send_to_char(caster, "A flash of white light fills the room, dispelling your violent magic!\r\n");
    act("White light from no particular source suddenly fills the room, then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }

  if (cvict) {
    if (MOB_FLAGGED(cvict, MOB_NOKILL)) {
      send_to_char(caster, "Este mob está protegid%s\r\n",OA(cvict));
      return (0);
    }

    for (af = cvict->affected; af; af = af->next)
      if (IS_SET_AR(AFF_FLAGS(cvict), AFF_PROTECT) && (af->location == spellnum)) {
    
        if (af->modifier >= rand_number(0, 99)) {
          send_to_char(caster, "%s está protegid%s e você falhou.\r\n", GET_NAME(cvict),OA(cvict));
          return 0; 
        }
      }
  }

  /* determine the type of saving throw */
  switch (casttype) {
  case CAST_STAFF:
  case CAST_SCROLL:
  case CAST_POTION:
  case CAST_WAND:
    savetype = SAVING_ROD;
    break;
  case CAST_SPELL:
    savetype = SAVING_SPELL;
    break;
  default:
    savetype = SAVING_BREATH;
    break;
  }

  if (spell->mag_flags & MAG_DAMAGE) {
    if ((damages = mag_damage(level, caster, cvict, spellnum, savetype)) == -1)
      return (-1);	/* Successful and target died, don't cast again. */
    if (damages)
      flags = MAGIC_SUCCESS;
  }

  if (spell->mag_flags & MAG_PROTECTION) {
    for (i=0; i<MAX_SPELL_PROTECTIONS; i++) {
      dur = MAX(1, formula_interpreter (caster, cvict, spellnum, TRUE, spell->protfrom[i].duration, level, &rts_code));
      res = MAX(0, formula_interpreter (caster, cvict, spellnum, TRUE, spell->protfrom[i].resist, level, &rts_code));
      flags |= mag_protections(level, caster, cvict, spell->vnum, spell->protfrom[i].prot_num, dur, res);
    }
  }

  if (spell->mag_flags & MAG_AFFECTS) 
    flags |= mag_affects(level, caster, cvict, spellnum, savetype);

  if (spell->mag_flags & MAG_UNAFFECTS)
    flags |= mag_unaffects(level, caster, cvict, spellnum, savetype);

  if (spell->mag_flags & MAG_POINTS)
    flags |= mag_points(level, caster, cvict, spellnum, savetype);

  if (spell->mag_flags & MAG_ALTER_OBJS)
    flags |= mag_alter_objs(level, caster, ovict, spellnum, savetype);

  if (spell->mag_flags & MAG_GROUPS)
    flags |= mag_groups(level, caster, spellnum, savetype);

  if (spell->mag_flags & MAG_MASSES)
    flags |= mag_masses(level, caster, spellnum, savetype);

  if (spell->mag_flags & MAG_AREAS)
    flags |= mag_areas(level, caster, spellnum, savetype);

  if (spell->mag_flags & MAG_SUMMONS)
    flags |= mag_summons(level, caster, ovict, spellnum, savetype);

  if (spell->mag_flags & MAG_CREATIONS)
    flags |= mag_creations(level, caster, spellnum);

  if (spell->mag_flags & MAG_ROOMS)
    flags |= mag_rooms(level, caster, spellnum);

  if ((spell->mag_flags & MAG_MANUAL) && spell->function) 
    call_ASPELL (spell->function, GET_LEVEL(caster), caster, cvict, ovict);

  if (spell->script)
    flags |= perform_script (spell->script, caster, cvict, ovict, spell->vnum, 0);

  if (flags & MAGIC_SUCCESS) {
    if (spell->messages.to_self != NULL && (caster != cvict))
      act(spell->messages.to_self, FALSE, caster, ovict, cvict, TO_CHAR);
    if (spell->messages.to_vict != NULL && cvict)
      act(spell->messages.to_vict, FALSE, cvict, ovict, 0, TO_CHAR);
    if (spell->messages.to_room != NULL && cvict)
      act(spell->messages.to_room, TRUE, caster, ovict, cvict, TO_ROOM);
  }
  else
  if (flags & MAGIC_NOEFFECT)
    send_to_char (caster, "%s", CONFIG_NOEFFECT);
  else
  if (flags & MAGIC_FAILED)
    send_to_char (caster, "Você falhou!\r\n");

  return (1);
}

/* mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 * Staves and wands will default to level 14 if the level is not specified; the
 * DikuMUD format did not specify staff and wand levels in the world files */
void mag_objectmagic(struct char_data *ch, struct obj_data *obj,
		          char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  int i, k;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		   FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_STAFF:
    act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
    else
      act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      send_to_char(ch, "It seems powerless.\r\n");
      act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
    } else {
      GET_OBJ_VAL(obj, 2)--;
      WAIT_STATE(ch, PULSE_VIOLENCE);
      /* Level to cast spell at. */
      k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

      /* Area/mass spells on staves can cause crashes. So we use special cases
       * for those spells spells here. */
      if (IS_SET(get_spell_mag_flags(GET_OBJ_VAL(obj, 3)), MAG_MASSES | MAG_AREAS)) {
        for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
	  i++;
	while (i-- > 0)
	  call_magic(ch, NULL, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
      } else {
	for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
	  next_tch = tch->next_in_room;
	  if (ch != tch)
	    call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
	}
      }
    }
    break;
  case ITEM_WAND:
    if (k == FIND_CHAR_ROOM) {
      if (tch == ch) {
	act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
	act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
      } else {
	act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
	if (obj->action_description)
	  act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
	else
	  act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
      }
    } else if (tobj != NULL) {
      act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
      if (obj->action_description)
	act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
      else
	act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
    } else if (IS_SET(get_spell_mag_flags(GET_OBJ_VAL(obj, 3)), MAG_AREAS | MAG_MASSES)) {
      /* Wands with area spells don't need to be pointed. */
      act("You point $p outward.", FALSE, ch, obj, NULL, TO_CHAR);
      act("$n points $p outward.", TRUE, ch, obj, NULL, TO_ROOM);
    } else {
      act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
      return;
    }

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      send_to_char(ch, "It seems powerless.\r\n");
      act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
      return;
    }
    GET_OBJ_VAL(obj, 2)--;
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (GET_OBJ_VAL(obj, 0))
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 GET_OBJ_VAL(obj, 0), CAST_WAND);
    else
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 DEFAULT_WAND_LVL, CAST_WAND);
    break;
  case ITEM_SCROLL:
    if (*arg) {
      if (!k) {
	act("There is nothing to here to affect with $p.", FALSE,
	    ch, obj, NULL, TO_CHAR);
	return;
      }
    } else
      tch = ch;

    act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
    else
      act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  case ITEM_POTION:
    tch = ch;

    if (!consume_otrigger(obj, ch, OCMD_QUAFF))  /* check trigger */
      return;

    act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
    else
      act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    for (i = 1; i <= 3; i++)
      if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_POTION) <= 0)
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  default:
    log1("SYSERR: Unknown object_type %d in mag_objectmagic.",
	GET_OBJ_TYPE(obj));
    break;
  }
}

/* cast_spell is used generically to cast any spoken spell, assuming we already
 * have the target char/obj and spell number.  It checks all restrictions,
 * prints the words, etc. Entry point for NPC casts.  Recommended entry point
 * for spells cast by NPCs via specprocs. */
int cast_spell(struct char_data *ch, struct char_data *tch,
	           struct obj_data *tobj, int spellnum)
{
  struct str_spells *spell;

  if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE) {
    log1("SYSERR: cast_spell trying to call spellnum %d/%d.", spellnum,
	TOP_SPELL_DEFINE);
    return (0);
  }

  spell = get_spell_by_vnum(spellnum);
  if (GET_POS(ch) < spell->min_pos) {
    switch (GET_POS(ch)) {
      case POS_SLEEPING:
      send_to_char(ch, "You dream about great magical powers.\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "You cannot concentrate while resting.\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "You can't do this sitting!\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "Impossible!  You can't concentrate enough!\r\n");
      break;
    default:
      send_to_char(ch, "You can't do much of anything like this!\r\n");
      break;
    }
    return (0);
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
    send_to_char(ch, "You are afraid you might hurt your master!\r\n");
    return (0);
  }
  if ((tch != ch) && (spell->targ_flags & TAR_SELF_ONLY)) {
    send_to_char(ch, "You can only %s yourself!\r\n", 
                      spell->type == SPELL ? "cast this spell upon" : "do this to");
    return (0);
  }
  if ((tch == ch) && (spell->targ_flags & TAR_NOT_SELF)) {
    send_to_char(ch, "You cannot %s yourself!\r\n",
                      spell->type == SPELL ? "cast this spell upon" : "do this to");
    return (0);
  }
  if ((spell->mag_flags & MAG_GROUPS) && !GROUP(ch)) {
    send_to_char(ch, "You can't %s if you're not in a group!\r\n",
                      spell->type == SPELL ? "cast this spell" : "do this");
    return (0);
  }
  send_to_char(ch, "%s", CONFIG_OK);

  if (spell->type == SPELL)
    say_spell(ch, spellnum, tch, tobj);

  return (call_magic(ch, tch, tobj, spellnum, GET_LEVEL(ch), CAST_SPELL));
}

ACMD(do_cast)
{
 char *s, *targ = NULL;
 struct char_data *tch = NULL;
 struct obj_data *tobj = NULL;
 struct str_spells *spell = NULL;
 int i, delay, rts_code = TRUE;
 int effectiveness = 0;
 int number;
 int level, target = 0;
 int mana = 5;

 if (IS_NPC(ch))
   return;

 // for spell check that is is enclosed in ''
 if (subcmd == SCMD_SPELL) {
   s = strtok(argument, "'");
   if (s == NULL) {
     send_to_char (ch, "Cast what where?\r\n");
     return;
   }
   if (!(s = strtok(NULL, "'"))) {
     send_to_char (ch, "Spell names must be enclosed in the Holy Magic Symbols: '\r\n");
     return;
   }
   targ = strtok (NULL, "\0");
 }
 else // skill
   targ = argument;

 if (subcmd == SCMD_SPELL)
   spell = get_spell_by_name(s, SPELL); 
 else
   spell = get_spell_by_vnum(subcmd);

 if (!spell || (spell->status != available)) {
   if (subcmd == SCMD_SPELL)
     send_to_char(ch, "Cast what?!?\r\n");
   else
     send_to_char(ch, "%s", HUH);  
   return;
 }

 // for skills with a function, we only check if status == available, and
 // we return the control to it function.
 if (spell->function && (spell->type == SKILL)) {
   call_ACMD (spell->function, ch, argument, 0, 0);
   return;
 }

 if (GET_LEVEL(ch) < LVL_IMMORT) {
   level = get_spell_level(spell->vnum, GET_CLASS(ch));
   if ((level == -1) || (GET_LEVEL(ch) < level)) {
     send_to_char(ch, "You do not know that %s!\r\n", (spell->type == SPELL) ? "spell" : "skill");
     return;
   }
   if (GET_SKILL(ch, spell->vnum) == 0) {
     send_to_char (ch, "You are unfamilliar with that %s.\r\n", (spell->type == SPELL) ? "spell" : "skill");
     return;
   }
 }

/* Find the target */
  if (targ != NULL) {
    char arg[MAX_INPUT_LENGTH];

    strlcpy(arg, targ, sizeof(arg));
    one_argument(arg, targ);
    skip_spaces(&targ);

    /* Copy target to global cast_arg2, for use in spells like locate object */
    strcpy(cast_arg2, targ);
  }

  if (spell->targ_flags & TAR_IGNORE) {
    target = TRUE;
  } else if (targ != NULL && *targ) {
    number = get_number(&targ);
    if (!target && (spell->targ_flags & TAR_CHAR_ROOM)) {
      if ((tch = get_char_vis(ch, targ, &number, FIND_CHAR_ROOM)) != NULL)
        target = TRUE;
    }
    if (!target && (spell->targ_flags & TAR_CHAR_WORLD))
      if ((tch = get_char_vis(ch, targ, &number, FIND_CHAR_WORLD)) != NULL)
        target = TRUE;

    if (!target && (spell->targ_flags & TAR_OBJ_INV))
      if ((tobj = get_obj_in_list_vis(ch, targ, &number, ch->carrying)) != NULL)
        target = TRUE;

    if (!target && (spell->targ_flags & TAR_OBJ_EQUIP)) {
      for (i = 0; !target && i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && isname(targ, GET_EQ(ch, i)->name)) {
          tobj = GET_EQ(ch, i);
          target = TRUE;
        }
    }
    if (!target && (spell->targ_flags & TAR_OBJ_ROOM))
      if ((tobj = get_obj_in_list_vis(ch, targ, &number, world[IN_ROOM(ch)].contents)) != NULL)
        target = TRUE;

    if (!target && (spell->targ_flags & TAR_OBJ_WORLD))
      if ((tobj = get_obj_vis(ch, targ, &number)) != NULL)
        target = TRUE;

  } else {                      /* if target string is empty */
 if (!target && (spell->targ_flags & TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL) {
        tch = ch;
        target = TRUE;
      }
    if (!target && (spell->targ_flags & TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL) {
        tch = FIGHTING(ch);
        target = TRUE;
      }
    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && ((spell->targ_flags & TAR_CHAR_ROOM) || (spell->targ_flags & TAR_SELF_ONLY)) &&
        !(spell->mag_flags & MAG_VIOLENT)) {
      tch = ch;
      target = TRUE;
    }
    if (!target) {
      send_to_char(ch, "Upon %s should the spell be cast?\r\n",
                (spell->targ_flags & (TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)) ? "what" : "who");
      return;
    }
  }

  if (target && (tch == ch) && (spell->damages) && (spell->mag_flags & MAG_VIOLENT)) {
    send_to_char(ch, "You shouldn't cast that on yourself -- could be bad for your health!\r\n");
    return;
  }
  if (!target) {
    send_to_char(ch, "Cannot find the target of your %s!\r\n", (spell->type == SPELL) ? "spell" : "skill");
    return;
  }
  
  // only spell cost mana
  if ((spell->type == SPELL) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    mana = mag_manacost(ch, tch, spell->vnum);
    if (GET_MANA(ch) < mana) {
      send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
      return;
    }
 }

 if (spell->effectiveness)
   effectiveness = GET_SKILL(ch, spell->vnum) * 
                   MAX(0, formula_interpreter (ch, tch, spell->vnum, TRUE, spell->effectiveness, GET_LEVEL(ch), &rts_code)) / 100;

 if (rand_number (0, 101) > effectiveness) {
   WAIT_STATE(ch, PULSE_VIOLENCE);
   if (!tch || !skill_message(0, ch, tch, spell->vnum))
     send_to_char (ch, "You lost your concentration!\r\n");

   if ((spell->type == SPELL) && (mana > 0))
     GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana / 2)));

   // if you lost your concentration and the spell is MAG_VIOLENT, you still hit a mobile to start the fight.
   if ((spell->mag_flags & MAG_VIOLENT) && tch && IS_NPC(tch))
     hit(tch, ch, TYPE_UNDEFINED);

   return;
 }

 if (cast_spell(ch, tch, tobj, spell->vnum)) {
   if (spell->delay) {
     delay = MAX(0, formula_interpreter (ch, tch, spell->vnum, TRUE, spell->delay, GET_LEVEL(ch), &rts_code));
     WAIT_STATE (ch, MIN(delay, MAX_SPELL_DELAY));
   }
   else
     WAIT_STATE(ch, PULSE_VIOLENCE);

   if (spell->type == SPELL)
     GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
 }
}