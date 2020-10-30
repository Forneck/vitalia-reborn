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
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("$p esta cheio.", FALSE, ch, obj, 0, TO_CHAR);
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

ASPELL(spell_recall)
{
   room_vnum recall_room;
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL)) {
    send_to_char(ch, "Uma luz brilhante impede a sua magia de funcionar!");
    return;
  }

  act("$n desaparece.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, recall_room);
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
    send_to_char(ch, "Uma luz brilhante impede a sua magia de funcionar!");
    return;
  }

  do {
    to_room = rand_number(0, top_of_world);
  } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH) ||
           ROOM_FLAGGED(to_room, ROOM_GODROOM) || ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_CLOSED) ||
           ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_NOASTRAL));

  act("$n lentamente some de existencia ate desaparecer completamente.",
      FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n lentamente some de existencia.", FALSE, victim, 0, 0, TO_ROOM);
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
  send_to_char(ch, "Uma luz brilhante impede a sua magia de funcionar!");
    return;
  }

  if (!CONFIG_PK_ALLOWED) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("Assim que as palavras saem da  sua boca e $N viaja\r\n"
	  "atraves do tempo e espaco ate voce, voce percebe que $E\r\n"
	  "pode te machucar, entao voce sabiamente manda $M de volta.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      send_to_char(victim, "%s acabou de tentar te invocar para: %s.\r\n"
	      "Mas falhou!\r\n"
	      "Digite NOSUMMON para permitir invocacao.\r\n",
	      GET_NAME(ch), world[IN_ROOM(ch)].name);

      send_to_char(ch, "Voce falhou porque %s esta' com a protecao ligada.\r\n", GET_NAME(victim));
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

  act("$n desaparece subitamente", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, IN_ROOM(ch));

  act("$n chega subitamente.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n invocou voce!", FALSE, ch, 0, victim, TO_VICT);
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
    send_to_char(ch, "Voce nao comsegue sentir isto.\r\n");
    return;
  }

  /*  added a global var to catch 2nd arg. */
  sprintf(name, "%s", cast_arg2);

  j = GET_LEVEL(ch) / 2;  /* # items to show = twice char's level */

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname_obj(name, i->name))
      continue;

  send_to_char(ch, "%c%s", UPPER(*i->short_description), i->short_description + 1);

    if (i->carried_by)
      send_to_char(ch, " carregado por %s.\r\n", PERS(i->carried_by, ch));
    else if (IN_ROOM(i) != NOWHERE)
      send_to_char(ch, " esta em %s.\r\n", world[IN_ROOM(i)].name);
    else if (i->in_obj)
      send_to_char(ch, " dentro de %s.\r\n", i->in_obj->short_description);
    else if (i->worn_by)
      send_to_char(ch, " no corpo de %s.\r\n", PERS(i->worn_by, ch));
    else
      send_to_char(ch, "esta' num lugar incerto.\r\n");

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
    send_to_char(ch, "Voce falhou porque %s esta com o NOSUMMON ligado!\r\n", GET_NAME(victim));
  else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
    send_to_char(ch, "A sua vitima esta protegida pelo santuario!\r\n");
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char(ch, "A sua vitima resiste!\r\n");
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "Voce nao pode ter seguidores!\r\n");
  else if (AFF_FLAGGED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
    send_to_char(ch, "Voce falhou.\r\n");
  /* player charming another player - no legal reason for this */
  else if (!CONFIG_PK_ALLOWED && !IS_NPC(victim))
    send_to_char(ch, "Voce falhou! Mas nao deveria ter feito isto.\r\n");
  else if (circle_follow(victim, ch))
    send_to_char(ch, "Desculpe, mas nao e' permitido seguir em circulos.\r\n");
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

    act("$n nao e' uma boa companhia?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim))
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
      send_to_char(ch, "Dado de Dano e' '%dD%d' com uma media de %.1f por rodada.\r\n",
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), ((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1));
      break;
    case ITEM_ARMOR:
      send_to_char(ch, "AC-aplicado e' %d\r\n", GET_OBJ_VAL(obj, 0));
      break;
    }
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
   int change = -1; 
   char arg[MAX_INPUT_LENGTH] = {'\0'};
   zone_rnum i;

  if (IS_NPC(ch) || !ch->desc)
    return;
  
   /* Control Weather fails if you aren't outside */
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
   {
        send_to_char(ch, "Por mais que tente, voce nao consegue controlar o clima.\r\n");
        act("Um brilho de luz aparece brevemente com a magia de $n antes de desaparecer.", TRUE, ch, 0, 0, TO_ROOM);
        return;
    }

  one_argument(cast_arg2, arg); /* Thanks Zusuk! */

  if (is_abbrev(arg, "pior"))
  {
 	if (weather_info.sky == SKY_LIGHTNING) {
  	send_to_char(ch, "Voce nao consegue piorar o tempo mais do que isso!");
  	return;}
  	else{
    change = 1; /* this will increase weather_info.sky by one, making it worse */
    weather_info.change -= dice(1, 6); /* weather_info.change being lower will slightly bias future natural weather shifts towards worse weather */
  	}
  } else if (is_abbrev(arg, "melhor"))
  {
  	if (weather_info.sky == SKY_CLOUDLESS){
  	send_to_char(ch, "Voce nao consegue melhorar o tempo mais do que isso!");
  	return;}
  	else{
    change = -1; /* this will decrease weather_info.sky by one, making it better */
    weather_info.change += dice(1, 6); /* weather_info.change being higher will slightly bias future natural weather shifts towards better weather */
  	}
  }
  else 
  {
    send_to_char(ch, "Voce precisa lancar esta magia com 'melhor' ou 'pior' para ter sucesso!\r\n");
    return;
  }
  
    send_to_char(ch, "Voce solta uma bola de energia azul em direcao ao ceu!\r\n");
    act("$n solta uma bola de energia azul em direcao ao ceu!", TRUE, ch, 0, 0, TO_ROOM);

    /* Send to Outdoors each cast may be too spammy? */
    if (change == -1)
        send_to_outdoor("O clima parece estar melhorando.\r\n");
    else
        send_to_outdoor("O clima parece estar piorando.\r\n");
    
    /* applies the changes to weather */
    weather_info.sky += change;
    weather_info.sky = MIN(MAX(weather_info.sky, SKY_CLOUDLESS), SKY_LIGHTNING);

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
      if (!rand_number(0, 20))
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