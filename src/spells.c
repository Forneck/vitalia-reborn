/**************************************************************************
*  File: spells.c                                          Part of tbaMUD *
*  Usage: Implementation of "manual spells."                              *
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
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "act.h"
#include "fight.h"
#include "spirits.h"

/* Special spells appear below. */
ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
      act("Hum... alguma coisa deu errado em $p.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("$p foi enchido.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}

ASPELL(spell_create_nectar)
{
  int liq;

  if (ch == NULL || obj == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_NECTAR) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
      act("Hum... alguma coisa deu errado em $p.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      liq = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (liq > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_NECTAR;
	GET_OBJ_VAL(obj, 1) += liq;
	name_to_drinkcon(obj, LIQ_NECTAR);
	weight_change_object(obj, liq);
	act("As forças da natureza encheram $p.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}

ASPELL(chanson_brinde)
{
  int liq;
  if (ch == NULL || obj == NULL)
     return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);       - not used */
  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WINE) && (GET_OBJ_VAL(obj, 1) != 0)) {
       name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_BLOOD;
      name_to_drinkcon(obj, LIQ_BLOOD);
      act("Hum... alguma coisa deu errado em $p.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      liq = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (liq > 0) { 
	if (GET_OBJ_VAL(obj, 1) >= 0)
	   name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WINE;
	GET_OBJ_VAL(obj, 1) += liq;
	name_to_drinkcon(obj, LIQ_WINE); 
	weight_change_object(obj, liq);
	act("A canção faz com que $p se encha.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}

ASPELL(spell_recall)
{  
  if (victim == NULL || IS_NPC(victim))
    return;
  
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL)) {
    send_to_char(ch, "Uma estranha força não lhe deixa sair daqui.\r\n");
    return;
  }

  act("$n desaparece.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  switch (GET_HOMETOWN(victim)) {
	  case 1:
                char_to_room(victim, r_hometown_1);
		break;
        case 2:
                char_to_room(victim, r_hometown_2);
                break;
	case 3:
		char_to_room(victim, r_hometown_3);
        break;
        default:
                char_to_room(victim, r_hometown_1);
		break;
        }
  act("$n aparece no meio da sala.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_teleport)
{
  room_rnum to_room;

  if (victim == NULL || IS_NPC(victim))
    return;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL)) {
    send_to_char(ch, "Uma estranha força não lhe deixa sair daqui.\r\n");
    return;
  }

  do {
    to_room = rand_number(0, top_of_world);
  } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH) ||
           ROOM_FLAGGED(to_room, ROOM_GODROOM) || ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_CLOSED) ||
           ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_NOASTRAL));

  act("$n lentamente some de existencia até desaparecer completamente.",
      FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n lentamente entra em existencia até aparecer completamente.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

#define SUMMON_FAIL "Voce falhou.\r\n"
ASPELL(spell_summon)
{
  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_NOASTRAL)) {
  send_to_char(ch, "Uma estranha força impede de trazer %s até você!", ELEA(victim));
    return;
  }

  if (!CONFIG_PK_ALLOWED) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
     act("Assim que as palavras saem de seus lábios e $N viaja\r\n"
           "através do tempo e do espaço para você, você percebe\r\n"
           "que $L é agressiv$R e irá lhe machucar, e sabiamente\r\n"
           "$R envia devolta.",
	   FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      send_to_char(victim, "%s acabou de tentar te invocar para: %s.\r\n"
	     "%s falhou porquê você está com a proteção ligada.\r\n"
                 "Digite NOSUMMON para permitir que outros jogadores lhe\r\n"
                 "convoquem para outros lugares.\r\n",
	      GET_NAME(ch), world[IN_ROOM(ch)].name, ELEAUpper(ch));

      send_to_char(ch, "Voce falhou porquê %s está com a protecao ligada.\r\n", GET_NAME(victim));
      mudlog(BRF, MAX(LVL_IMMORT, MAX(GET_INVIS_LEV(ch), GET_INVIS_LEV(victim))), TRUE, 
        "%s falhou invocar %s para %s.", GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
      return;
    }
  }

  if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
      (IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL, 0))) {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return;
  }

  act("$n desaparece repentinamente.", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, IN_ROOM(ch));

  act("$n chega repentinamente.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n lhe convocou!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

/* Used by the locate object spell to check the alias list on objects */
static int isname_obj(char *search, char *list)
{
  char *found_in_list; /* But could be something like 'ring' in 'shimmering.' */
  char searchname[128];
  char namelist[MAX_STRING_LENGTH];
  int found_pos = -1;
  int found_name=0; /* found the name we're looking for */
  int match = 1;
  int i;

  /* Force to lowercase for string comparisons */
  sprintf(searchname, "%s", search);
  for (i = 0; searchname[i]; i++)
    searchname[i] = LOWER(searchname[i]);

  sprintf(namelist, "%s", list);
  for (i = 0; namelist[i]; i++)
    namelist[i] = LOWER(namelist[i]);

  /* see if searchname exists any place within namelist */
  found_in_list = strstr(namelist, searchname);
  if (!found_in_list) {
    return 0;
  }

  /* Found the name in the list, now see if it's a valid hit. The following
   * avoids substrings (like ring in shimmering) is it at beginning of
   * namelist? */
  for (i = 0; searchname[i]; i++)
    if (searchname[i] != namelist[i])
      match = 0;

  if (match) /* It was found at the start of the namelist string. */
    found_name = 1;
  else { /* It is embedded inside namelist. Is it preceded by a space? */
    found_pos = found_in_list - namelist;
    if (namelist[found_pos-1] == ' ')
      found_name = 1;
  }

  if (found_name)
    return 1;
  else
    return 0;
}

ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;

  if (!obj) {
    send_to_char(ch, "Você não comsegue sentir isto.\r\n");
    return;
  }

  /*  added a global var to catch 2nd arg. */
  sprintf(name, "%s", cast_arg2);

  j = GET_LEVEL(ch) / 2;  /* # items to show = twice char's level */

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname_obj(name, i->name))
      continue;

  send_to_char(ch, "%c%s está", UPPER(*i->short_description), i->short_description + 1);

    if (i->carried_by)
      send_to_char(ch, " sendo carregado por %s.\r\n", PERS(i->carried_by, ch));
    else if (IN_ROOM(i) != NOWHERE)
      send_to_char(ch, " em %s.\r\n", world[IN_ROOM(i)].name);
    else if (i->in_obj)
      send_to_char(ch, " dentro de %s.\r\n", i->in_obj->short_description);
    else if (i->worn_by)
      send_to_char(ch, " sendo vestido por %s.\r\n", PERS(i->worn_by, ch));
    else
      send_to_char(ch, " num lugar incerto.\r\n");

    j--;
  }

}

ASPELL(spell_charm)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)
    return;

  if (victim == ch)
    send_to_char(ch, "Voce gosta cada vez mais de si!\r\n");
  else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))
    send_to_char(ch, "Voce falhou porque %s está com o NOSUMMON ligado!\r\n", GET_NAME(victim));
  else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
    send_to_char(ch, "A sua vitima esta protegida pelo santuario!\r\n");
  else if (AFF_FLAGGED(victim, AFF_GLOOMSHIELD))
    send_to_char(ch, "A sua vitima esta protegida por um escudo de trevas!\r\n");
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char(ch, "A sua vitima resiste!\r\n");
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "Voce não pode ter seguidores!\r\n");
  else if (AFF_FLAGGED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
    send_to_char(ch, "Voce falhou.\r\n");
  /* player charming another player - no legal reason for this */
  else if (!CONFIG_PK_ALLOWED && !IS_NPC(victim))
    send_to_char(ch, "Voce falhou! Mas não deveria ter feito isto.\r\n");
  else if (circle_follow(victim, ch))
    send_to_char(ch, "Desculpe, mas não é permitido seguir em circulos.\r\n");
  else if (mag_savingthrow(victim, SAVING_PARA, 0))
    send_to_char(ch, "A sua vitima resiste!\r\n");
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    new_affect(&af);
    af.spell = SPELL_CHARM;
    af.duration = 24 * 2;
    if (GET_CHA(ch))
      af.duration *= GET_CHA(ch);
    if (GET_INT(victim))
      af.duration /= GET_INT(victim);
    SET_BIT_AR(af.bitvector, AFF_CHARM);
    affect_to_char(victim, &af);

    act("$n não é uma boa companhia?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim))
      //remover agro
      REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_AGGRESSIVE);
      REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_SPEC);
  }
}

ASPELL(chanson_encanto)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)   
     return;   

  if (victim == ch) 
    send_to_char(ch, "Voce gosta cada vez mais de si!\r\n");
  else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))
    send_to_char(ch, "Voce falhou porque %s esta com o NOSUMMON ligado!\r\n", GET_NAME(victim));
  else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
    send_to_char(ch, "A sua vitima esta protegida pelo santuario!\r\n");
  else if (AFF_FLAGGED(victim, AFF_GLOOMSHIELD))
    send_to_char(ch, "A sua vitima esta protegida por um escudo de trevas!\r\n");
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char(ch, "A sua vitima resiste!\r\n");
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "Voce não pode ter seguidores!\r\n");
  else if (AFF_FLAGGED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
    send_to_char(ch, "Voce falhou.\r\n");
  /* player charming another player - no legal reason for this */
  else if (!CONFIG_PK_ALLOWED && !IS_NPC(victim))
    send_to_char(ch, "Voce falhou! Mas nao deveria ter feito isto.\r\n");
  else if (circle_follow(victim, ch))
    send_to_char(ch, "Desculpe, mas não é  permitido seguir em circulos.\r\n");
  else if (mag_savingthrow(victim, SAVING_PARA, 0))
    send_to_char(ch, "A sua vitima resiste!\r\n");
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    new_affect(&af);
    af.spell = CHANSON_ENCANTO;
    af.duration = 24 * 3;
    if (GET_CHA(ch))
      af.duration *= GET_CHA(ch);
    if (GET_INT(victim))
      af.duration /= GET_INT(victim);
    SET_BIT_AR(af.bitvector, AFF_CHARM);
    affect_to_char(victim, &af);

    act("$n não é uma boa companhia?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim))
      //remover agro
      REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_AGGRESSIVE);
      REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_SPEC);
  }
}

ASPELL(spell_identify)
{
  int i, found;
  size_t len;

  if (obj) {
    char bitbuf[MAX_STRING_LENGTH];

    sprinttype(GET_OBJ_TYPE(obj), item_types, bitbuf, sizeof(bitbuf));
    send_to_char(ch, "Voce consegue sentir:\r\nObjeto '%s' do tipo %s\r\n", obj->short_description, bitbuf);

    if (GET_OBJ_AFFECT(obj)) {
      sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, bitbuf);
      send_to_char(ch, "O item vai dar as seguintes habilidades:  %s\r\n", bitbuf);
    }

    sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, bitbuf);
    send_to_char(ch, "Item e': %s\r\n", bitbuf);

    send_to_char(ch, "Peso: %d, Valor: %d, Aluguel: %d, Nivel: %d\r\n",
                     GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj));

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      len = i = 0;

      if (GET_OBJ_VAL(obj, 1) >= 1) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 1)));
        if (i >= 0)
          len += i;
      }

      if (GET_OBJ_VAL(obj, 2) >= 1 && len < sizeof(bitbuf)) {
	i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 2)));
        if (i >= 0)
          len += i;
      }

      if (GET_OBJ_VAL(obj, 3) >= 1 && len < sizeof(bitbuf)) {
	snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", skill_name(GET_OBJ_VAL(obj, 3)));
      }

      send_to_char(ch, "%s de: %s\r\n", item_types[(int) GET_OBJ_TYPE(obj)], bitbuf);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      send_to_char(ch, "%s de %s\r\nCom %d carga%s no total e %d restando.\r\n",
		item_types[(int) GET_OBJ_TYPE(obj)], skill_name(GET_OBJ_VAL(obj, 3)),
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s", GET_OBJ_VAL(obj, 2));
      break;
    case ITEM_WEAPON:
    case ITEM_AMMO:
      send_to_char(ch, "Dado de Dano e' '%dD%d' com uma media de %.1f por rodada.\r\n",
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), ((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1));
      break;
    case ITEM_ARMOR:
      send_to_char(ch, "AC-aplicado e' %d\r\n", GET_OBJ_VAL(obj, 0));
      break;
    }
    if (GET_OBJ_TYPE(obj) == ITEM_AMMO)
     send_to_char(ch, "Restam ainda %d %s.\r\n", GET_OBJ_VAL(obj, 0),GET_OBJ_VAL(obj, 0) == 1 ? "unidade" : "unidades");
     
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
	  (obj->affected[i].modifier != 0)) {
	if (!found) {
	  send_to_char(ch, "Pode te afetar com :\r\n");
	  found = TRUE;
	}
	sprinttype(obj->affected[i].location, apply_types, bitbuf, sizeof(bitbuf));
	send_to_char(ch, "   Afeta: %s em %d\r\n", bitbuf, obj->affected[i].modifier);
      }
    }
  } else if (victim) {		/* victim */
    send_to_char(ch, "Nome: %s\r\n", GET_NAME(victim));
    if (!IS_NPC(victim))
      send_to_char(ch, "%s tem %d anoss, %d meses, %d dias e %d horas de idade.\r\n",
	      GET_NAME(victim), age(victim)->year, age(victim)->month,
	      age(victim)->day, age(victim)->hours);
    send_to_char(ch, "Altura %d cm, Peso %d libras.\r\n", GET_HEIGHT(victim), GET_WEIGHT(victim));
    send_to_char(ch, "Nivel: %d, HP: %d, Mana: %d\r\n", GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
    send_to_char(ch, "AC: %d, Hitroll: %d, Damroll: %d\r\n", compute_armor_class(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
    send_to_char(ch, "Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
	GET_STR(victim), GET_ADD(victim), GET_INT(victim),
	GET_WIS(victim), GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
  }
}

/* Cannot use this spell on an equipped object or it will mess up the wielding
 * character's hit/dam totals. */
ASPELL(spell_enchant_weapon)
{
  int i;
  int hit_aff = 0, dam_aff = 0, lvl = 0;

  if (ch == NULL || obj == NULL)
    return;

  /* Either already enchanted or not a weapon. */
  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
    return;

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].location != APPLY_NONE)
      return;

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
  if (level < 36)       { hit_aff = 1;  dam_aff = 1;  lvl = 0;  }
  else if (level < 38)  { hit_aff = 2;  dam_aff = 1;  lvl = 5;  }
  else if (level < 41)  { hit_aff = 2;  dam_aff = 2;  lvl = 10; }
  else if (level < 45)  { hit_aff = 3;  dam_aff = 2;  lvl = 10; }
  else if (level < 50)  { hit_aff = 3;  dam_aff = 3;  lvl = 15; }
  else if (level < 55)  { hit_aff = 4;  dam_aff = 3;  lvl = 20; }
  else if (level < 60)  { hit_aff = 4;  dam_aff = 4;  lvl = 30; }
  else if (level < 65)  { hit_aff = 5;  dam_aff = 4;  lvl = 40; }
  else                  { hit_aff = 5;  dam_aff = 5;  lvl = 50; }
  
  obj->affected[0].location = APPLY_HITROLL;
  obj->affected[0].modifier = hit_aff;

  obj->affected[1].location = APPLY_DAMROLL;
  obj->affected[1].modifier = dam_aff;

  GET_OBJ_LEVEL(obj) = MAX(lvl, GET_OBJ_LEVEL(obj));

  if (IS_GOOD(ch)) {
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
    act("$p brilha azul.", FALSE, ch, obj, 0, TO_CHAR);
  } else if (IS_EVIL(ch)) {
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
    act("$p brilha vermelho.", FALSE, ch, obj, 0, TO_CHAR);
  } else
    act("$p brilha amarelo.", FALSE, ch, obj, 0, TO_CHAR);
}

/* Cannot use this spell on an equipped object or it will mess up the wielding
 * character's hit/dam totals. */
ASPELL(spell_bless_object)
{
  int i, pos_hit = -1, novo = 0;
  int hit_aff = 0;

  /* int lvl = 0; 
   * DEPENDE DA TABELA SE VAI USAR */

  if (ch == NULL || obj == NULL)
    return;

  /* Either already blessed or not a weapon or armoor. */
  if ((GET_OBJ_TYPE(obj) != ITEM_WEAPON) && (GET_OBJ_TYPE(obj) != ITEM_ARMOR) && (GET_OBJ_TYPE(obj) != ITEM_FIREWEAPON) && (GET_OBJ_TYPE(obj) != ITEM_AMMO)) {
     act("Você não pode abençoar $p.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if (OBJ_FLAGGED(obj, ITEM_BLESS)){
    send_to_char(ch, "Alguem já abençoou isto antes!\r\n");
    return;
  }

  if (!IS_GOOD(ch)) {
    send_to_char(ch,"Você não é virtuos%s o suficiente!\r\n", OA(ch));
    return;
  }
  
  /* Primeiro, procurar APPLY_HITROLL */
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj->affected[i].location == APPLY_HITROLL) {
        pos_hit = i;
        break;  // Já achamos, não precisa continuar
    }
  }

  /* Se não encontrou APPLY_HITROLL, procurar um slot livre */
  if (pos_hit == -1) {
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location == APPLY_NONE) {
            pos_hit = i;
            novo = 1; // Indica que está aplicando um novo efeito
            break; // Achou um livre, pode parar
        }
    }
  }

  /* Se não encontrou nenhum dos dois, falha */
  if (pos_hit == -1) {
    act("Algo deu errado ao abençoar $p.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  
  /* VER TABELA PARA BALANCEAMENTO */
  /*
  if (level < 36)       { hit_aff = 2; lvl = 0;  }
  else if (level < 38)  { hit_aff = 2; lvl = 5;  }
  else if (level < 41)  { hit_aff = 2; lvl = 10; }
  else if (level < 45)  { hit_aff = 2; lvl = 10; }
  else if (level < 50)  { hit_aff = 2; lvl = 15; }
  else if (level < 55)  { hit_aff = 2; lvl = 20; }
  else if (level < 60)  { hit_aff = 2; lvl = 30; }
  else if (level < 65)  { hit_aff = 2; lvl = 40; }
  else                  { hit_aff = 2; lvl = 50; }
  GET_OBJ_LEVEL(obj) = MAX(lvl, GET_OBJ_LEVEL(obj));
  */
  hit_aff = 2;

  obj->affected[pos_hit].location = APPLY_HITROLL;
  if (novo)
    obj->affected[pos_hit].modifier = hit_aff;
   else
    obj->affected[pos_hit].modifier += hit_aff;
  
  if (IS_GOOD(ch)) {
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BLESS);
    act("Você abençoa $p.", FALSE, ch, obj, 0, TO_CHAR);
  }

   if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
       GET_OBJ_VAL(obj, 2)++;
}

ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (AFF_FLAGGED(victim, AFF_POISON))
        send_to_char(ch, "Voce sente veneno em suas veias.\r\n");
      else
        send_to_char(ch, "Voce se sente saudavel.\r\n");
    } else {
      if (AFF_FLAGGED(victim, AFF_POISON))
        act("Voce sente que $E tem veneno no sangue.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("Voce sente que $E esta saudavel.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("Voce sente que nao deveria consumir o conteudo de $p.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("Voce sente que pode consumir  conteudo de $p .", FALSE, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char(ch, "Voce sente que nao deveria consumir isto.\r\n");
    }
  }
}

ASPELL(spell_control_weather)
{
  int change_val;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char property[MAX_INPUT_LENGTH] = {'\0'};
  char direction[MAX_INPUT_LENGTH] = {'\0'};
  char *eq_ptr;
  zone_rnum zone;
  /* Verifica se o caster é um NPC ou não possui descriptor */
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Control Weather falha se o personagem estiver dentro */
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS)) {
    send_to_char(ch, "Por mais que tente, você não consegue controlar o clima estando dentro.\r\n");
    act("Um brilho de luz aparece brevemente com a magia de $n antes de desaparecer.", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  /* Pega o argumento passado com a magia; usamos cast_arg2 (ou outro buffer local) */
  one_argument(cast_arg2, arg);
  if (!*arg) {
    send_to_char(ch, "Você deve especificar uma propriedade e uma direção (ex.: pressao=aumentar).\r\n");
    return;
  }

  /* Primeiro, tenta encontrar o caractere '=' para separar propriedade e direção */
  eq_ptr = strchr(arg, '=');
  if (eq_ptr) {
    *eq_ptr = '\0';  /* separa a string */
    strcpy(property, arg);
    strcpy(direction, eq_ptr + 1);
  } else {
    /* Se não houver '=', tenta separar a propriedade da direção usando o tamanho da palavra-chave */
    if (is_abbrev(arg, "pressao")) {
      strcpy(property, "pressao");
      strcpy(direction, arg + strlen("pressao"));
    } else if (is_abbrev(arg, "temperatura")) {
      strcpy(property, "temperatura");
      strcpy(direction, arg + strlen("temperatura"));
    } else if (is_abbrev(arg, "vento")) {
      strcpy(property, "vento");
      strcpy(direction, arg + strlen("vento"));
    } else if (is_abbrev(arg, "umidade")) {
      strcpy(property, "umidade");
      strcpy(direction, arg + strlen("umidade"));
    } else {
      send_to_char(ch, "Propriedade inválida. Use: pressao, temperatura, vento ou umidade.\r\n");
      return;
    }
  }

  /* Verifica se a direção é válida */
  if (!is_abbrev(direction, "aumentar") && !is_abbrev(direction, "diminuir")) {
    send_to_char(ch, "Você deve especificar 'aumentar' ou 'diminuir' para a direção.\r\n");
    return;
  }

  /* Define um valor de mudança aleatório */
  change_val = dice(1, 6);

  /* Aplica a mudança na propriedade desejada na zona atual */
  struct weather_data *weather = zone_table[world[IN_ROOM(ch)].zone].weather;
  zone = world[IN_ROOM(ch)].zone; /* Pega o vnum da zona e converte pra rnum via zone_rnum */
  if (!weather) {
    send_to_char(ch, "O clima local está com muita instabilidade. Tente novamente mais tarde.\r\n");
    return;
  }

  if (is_abbrev(property, "pressao")) {
    if (is_abbrev(direction, "aumentar"))
      weather->press_diff += change_val;
    else  /* diminuir */
      weather->press_diff -= change_val;

    if (weather->press_diff < -12) weather->press_diff = -12;
    if (weather->humidity > 12) weather->press_diff = 12;
    send_to_char(ch, "Você canaliza sua magia e altera a pressão da area.\r\n");
  }
  else if (is_abbrev(property, "temperatura")) {
    if (is_abbrev(direction, "aumentar"))
      weather->temp_diff += change_val;
    else
      weather->temp_diff -= change_val;
    send_to_char(ch, "Você canaliza sua magia e altera a temperatura da area.\r\n");
  }
  else if (is_abbrev(property, "vento")) {
    if (is_abbrev(direction, "aumentar")) weather->winds += (float) change_val / 100.0;  /* ajuste conforme necessário */
    else weather->winds -= (float) change_val / 100.0;
    if (weather->winds < 0) weather->winds = 0;
      send_to_char(ch, "Você canaliza sua magia e altera o vento na area.\r\n");
  }
  else if (is_abbrev(property, "umidade")) {
    if (is_abbrev(direction, "aumentar"))
      weather->humidity += (float) change_val / 100.0;
    else
      weather->humidity -= (float) change_val / 100.0;
    if (weather->humidity < 0) weather->humidity = 0;
    if (weather->humidity > 1) weather->humidity = 1;
    send_to_char(ch, "Você canaliza sua magia e altera a umidade na area.\r\n");
  }
  else {
    send_to_char(ch, "Propriedade inválida. Use: pressao, temperatura, vento ou umidade.\r\n");
    return;
  }

  act("$n canaliza a energia dos elementos, alterando o clima local!", TRUE, ch, 0, 0, TO_ROOM);
  send_to_zone_outdoor(zone,"O clima local parece estar mudando...\r\n");
}

ASPELL(spell_transport_via_plants) {
  obj_vnum obj_num = NOTHING;
  room_rnum to_room = NOWHERE;
  struct obj_data *dest_obj = NULL, *tmp_obj = NULL;

  if (ch == NULL)
    return;

  if (!obj) {
    send_to_char(ch, "O seu alvo não existe!\r\n");
    return;
  } else if (GET_OBJ_TYPE(obj) != ITEM_PLANT) {
    send_to_char(ch, "Isto nao é uma planta!\r\n");
    return;
  } 
  obj_num = GET_OBJ_VNUM(obj);

  // find another of that plant in the world
  for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next) {
    if (tmp_obj == obj)
      continue;

    // we don't want to transport to a plant in someone's inventory
    if (GET_OBJ_VNUM(tmp_obj) == obj_num && !tmp_obj->carried_by) {
      dest_obj = tmp_obj;

      // 5% chance we will just stop at this obj
      if (!rand_number(0, 10))
        break;
    }
  }

  act("$n consegue pisar dentro de $p.", FALSE, ch, obj, 0, TO_ROOM);
  act("Voce consegue pisar dentro de $p.", FALSE, ch, obj, 0, TO_CHAR);

  if (dest_obj != NULL) {
    to_room = dest_obj->in_room;
  }

  if (to_room == NOWHERE) {
    send_to_char(ch, "Voce nao consegue encontrar outra saida a nao ser voltar!\r\n");
    act("$n sai cambaleando de $p.", FALSE, ch, obj, 0, TO_ROOM);
    GET_POS(ch) = POS_STUNNED;
    return;
  } else {
    if (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH) ||
           ROOM_FLAGGED(to_room, ROOM_GODROOM) || ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_CLOSED) ||
           ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_NOASTRAL))   {
      send_to_char(ch, "Uma luz brilhante impede sua magia de funcionar!\r\n");
      act("$n sai cambaleando de $p.", FALSE, ch, obj, 0, TO_ROOM);
      GET_POS(ch) = POS_STUNNED;
      return;
    }

    // transport player to new location
    char_from_room(ch);
    char_to_room(ch, to_room);
    look_at_room(ch, 0);
    act("Voce encontra seu destino e sai de $p.", FALSE, ch, dest_obj, 0, TO_CHAR);
    act("$n pisa fora de $p!", FALSE, ch, dest_obj, 0, TO_ROOM);
    // TODO: make this an event, so player enters into the plant, and sees a couple messages, then comes out the other side
  }
}

ASPELL(spell_portal) {
	/* create a magic portal */
	struct obj_data *portal_obj;
	struct extra_descr_data *extra_desc;
	char buf[512];
	
	/*
	 check target room for legality.
	 */
	portal_obj = read_object(PORTAL_VNUM, VIRTUAL);
	if (IN_ROOM(ch) == NOWHERE || !portal_obj) {
		send_to_char(ch, "A magia falhou.\r\n");
		extract_obj(portal_obj);
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_TUNNEL)) {
		send_to_char(ch, "Não tem espaço aqui!\r\n");
		extract_obj(portal_obj);
		return;
	}

	if (IN_ROOM(victim) == NOWHERE) {
		send_to_char(ch, "Seu destino está protegido contra a sua magia.\r\n");
		extract_obj(portal_obj);
		return;
	}

if (IN_ROOM(victim) == IN_ROOM(ch)) {
		send_to_char(ch, "A magia falhou! Você já está aqui...\r\n");
		extract_obj(portal_obj);
		return;
	}
	
	if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOMAGIC | ROOM_ATRIUM | ROOM_HOUSE )) {
		send_to_char(ch, "Seu destino está protegido contra a sua magia.\r\n");
		extract_obj(portal_obj);
		return;
	}
   
   if (MOB_FLAGGED(victim, MOB_NOSUMMON)
	   || (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))) {
    send_to_char(ch, "Seu destino está protegido contra a sua magia.\r\n");
    	extract_obj(portal_obj);
		return;
	   }

	sprintf(buf, "Através das névoas do portal, você vê %s",
			world[IN_ROOM(victim)].name);

	CREATE(extra_desc, struct extra_descr_data, 1);
	extra_desc->next = portal_obj->ex_description;
	portal_obj->ex_description = extra_desc;
	CREATE(extra_desc->keyword, char, strlen(portal_obj->name) + 1);
	strcpy(extra_desc->keyword, portal_obj->name);
	extra_desc->description = strdup(buf);

	portal_obj->obj_flags.timer = level / 20;
	portal_obj->obj_flags.value[0] = world[IN_ROOM(victim)].number;
	portal_obj->obj_flags.level = level-5;
	obj_to_room(portal_obj, IN_ROOM(ch));

	act("$p  aparece do nada!", TRUE, ch, portal_obj, 0, TO_ROOM);
	act("$p aparece diante de você!", TRUE, ch, portal_obj, 0, TO_CHAR);

	/* Portal at other side */
	portal_obj = read_object(PORTAL_VNUM, VIRTUAL);
	sprintf(buf, "Através das névoas do portal, você vê %s",
			world[IN_ROOM(ch)].name);

	CREATE(extra_desc, struct extra_descr_data, 1);
	extra_desc->next = portal_obj->ex_description;
	portal_obj->ex_description = extra_desc;
	CREATE(extra_desc->keyword, char, strlen(portal_obj->name) + 1);
 strcpy(extra_desc->keyword, portal_obj->name);
	extra_desc->description = strdup(buf);

	portal_obj->obj_flags.timer = level/20;
	portal_obj->obj_flags.value[0] = world[IN_ROOM(ch)].number;
		portal_obj->obj_flags.level = level-5;
	obj_to_room(portal_obj, IN_ROOM(victim));

	act("$p aparece do nada!", TRUE, victim, portal_obj, 0, TO_ROOM);
	act("$p aparece diante de você!", TRUE, victim, portal_obj, 0, TO_CHAR);
}

ASPELL(spell_youth)
{
  if (victim == NULL)
    return;

  if (IS_NPC(victim) || GET_AGE(victim) < 25)
    send_to_char(ch,"Você não sente nada especial...\r\n");
  else {
    victim->player.time.birth += 5 * SECS_PER_MUD_YEAR;
    send_to_char(victim,"Você se sente mais jovem!\r\n");
    act("$n parece mais jovem agora.", TRUE, victim, 0, 0, TO_ROOM);
  }
}

ASPELL(spell_vamp_touch)
{
  int victim_ac, calc_thaco, diceroll, dam, rdam;

  if (ch == NULL || victim == NULL)
    return;

  calc_thaco = compute_thaco(ch,victim);
  victim_ac = compute_armor_class(victim) / 10;
  diceroll = rand_number(1, 20);

  /* decide whether this is a hit or a miss */
  if ((((diceroll < 20) && AWAKE(victim)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
    damage(ch, victim, 0, SPELL_VAMP_TOUCH);
    return;
  }

  /* okay, we know the guy has been hit.  now calculate damage. */
  dam = dice(8, 8) + ((GET_LEVEL(ch) - 22) / 4);
  rdam = damage(ch, victim, dam, SPELL_VAMP_TOUCH);

  if (rdam > 0)
    GET_HIT(ch) += rdam;
  else if (rdam < 0)
    GET_HIT(ch) += (dam / 2);

  GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));
}

/* -- mp - Oct 30, 2001 */
ASPELL(spell_fury_air)
{
  if (ch == NULL || victim == NULL)
    return;

  if (MOB_FLAGGED(victim, MOB_NOBASH)) {
    act("O forte vento parece não abalar $N.", FALSE, ch, NULL, victim, TO_CHAR);
    return;
  }

  if (GET_POS(victim) < POS_SITTING) {
    act("A magia parece ser inútil nessas condições.", FALSE, ch, NULL, victim, TO_CHAR);
    return;
  }

  if (GET_POS(victim) == POS_SITTING) {
    act("Um vento forte levanta $N.", FALSE, ch, NULL, victim, TO_CHAR);
    GET_POS(victim) = POS_STANDING;
    return;
  }

  if (damage(ch, victim, 2, SPELL_FURY_OF_AIR) > 0) {
    if (GET_LEVEL(victim) < LVL_GOD)
      GET_WAIT_STATE(victim) += 2 * PULSE_VIOLENCE;
    if (IN_ROOM(ch) == IN_ROOM(victim))
      GET_POS(victim) = POS_SITTING;
  }

  if (GET_LEVEL(ch) < LVL_GOD)
    GET_WAIT_STATE(ch) += 2 * PULSE_VIOLENCE;
}

ASPELL(spell_raise_dead)
{
  if (ch == NULL)
    return;

  if (obj && (GET_OBJ_TYPE(obj) == ITEM_CORPSE)) {
    if (GET_OBJ_VAL(obj, 0) == 0) {			/* -- jr - 26/06/99 */
      send_to_char(ch,"Não é uma boa idéia trazer monstros devolta à vida.\r\n");
      return;
    } else if (GET_OBJ_VAL(obj, 0) < 0) {		/* -- jr - 15/12/99 */
      send_to_char(ch,"Infelizmente, esta pessoa não pode mais ser ressucitada.\r\n");
      return;
    }
        if (!victim)
      send_to_char(ch,"Você sente que esta pessoa não está mais entre nós...\r\n");
      else if ALIVE(victim) {
      send_to_char(ch,"Você sente que esta pessoa já está viva em algum lugar...\r\n");
      act("$p foi destruído pela magia!", TRUE, NULL, obj, NULL, TO_ROOM);
      extract_obj(obj);
    }
    else if (GET_CON(victim) < 4)
      send_to_char(ch,"Você sente que esta pessoa não pode mais ser ressucitada.\r\n");
      else if (!IS_NPC(victim) && (GET_IDNUM(victim) == GET_OBJ_VAL(obj, 0))) {
      raise_online(victim, ch, obj, obj->in_room, 1);
      extract_obj(obj);

      /*
       * We need this because the spell can be cast by an NPC.
       */
      if (!IS_NPC(ch))
      log1("%s has taken back to live by %s (raise dead spell).",
	  GET_NAME(victim), GET_NAME(ch));
    }
  }
  else
    send_to_char(ch,"Esta magia deve ser lançada em um corpo.\r\n");
}

ASPELL(spell_ressurect)
{
  struct char_data *dead;
  struct obj_data *corpse;

  if (ch == NULL)
    return;

  if (obj && (GET_OBJ_TYPE(obj) == ITEM_CORPSE)) {
    if (GET_OBJ_VAL(obj, 0) == 0) {			/* -- jr - 26/06/99 */
      send_to_char(ch,"Não é uma boa idéia trazer monstros devolta a vida.\r\n");
      return;
    } else if (GET_OBJ_VAL(obj, 0) < 0) {		/* -- jr - 15/12/99 */
      send_to_char(ch,"Infelizmente, esta pessoa não pode mais ser ressucitada.\r\n");
      return;
    }
    
    if (!dead)
      send_to_char(ch,"Você sente que esta pessoa não está mais entre nós...\r\n");
    else if ALIVE(dead) {
      send_to_char(ch,"Você sente que esta pessoa já está viva em algum lugar...\r\n");
      act("$p foi destruído pela magia!", TRUE, NULL, obj, NULL, TO_ROOM);
      extract_obj(obj);
    }
    else {
         if (!IS_NPC(victim) && (GET_IDNUM(victim) == GET_OBJ_VAL(obj, 0))){
      raise_online(dead, ch, obj, obj->in_room, 1);
      extract_obj(obj);
         }

      /*
       * We need this because the spell can be cast by an NPC.
       */
      if (!IS_NPC(ch))
      log1("%s has taken back to live by %s (ressurect spell).", GET_NAME(dead), GET_NAME(ch));
    }
  }
  else if (victim) {
    if (IS_NPC(victim))
      send_to_char(ch,"Somente jogadores podem ser ressucitados.\r\n");
    else if (ALIVE(victim)) {
      send_to_char(ch,"Você sente que esta pessoa já está viva...\r\n");
    } else {
      	for (corpse = object_list; corpse; corpse = corpse->next)
			if (GET_OBJ_TYPE(corpse) == ITEM_CORPSE && GET_OBJ_VAL(corpse, 0) == GET_IDNUM(victim))
				break;

 	if (GET_HOMETOWN(ch) == r_hometown_1)
      raise_online(victim, ch, corpse, r_ress_room_1, 1);
  else if (GET_HOMETOWN(ch) == r_hometown_2)
      raise_online(victim, ch, corpse, r_ress_room_2, 1);
  else if (GET_HOMETOWN(ch) == r_hometown_3)
      raise_online(victim, ch, corpse, r_ress_room_3, 1);
  else
   raise_online(victim, ch, corpse,IN_ROOM(victim), 1);
   
      if (corpse)
	extract_obj(corpse);
      else {
	log1("%s used ressurect on %s, but I couldn't find the corpse.",
	    GET_NAME(ch), GET_NAME(victim));
      }

      if (!IS_NPC(ch))
      log1("%s has taken back to live by %s (ressurect spell).",GET_NAME(victim), GET_NAME(ch));
    }
  }
  else
    send_to_char(ch,"Esta magia deve ser lançada em um espírito ou em um corpo.\r\n");
}
