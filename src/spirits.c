/* ----------------------------=== VitaliaMUD ===----------------------------
   * File: spirits.c Usage: functions for handling spirits of dead players
   Part of VitaliaMUD source code, Copyright (C) 2000 - Juliano Ravasi Ferraz
   Copyright (C) 2000 - Juliano Ravasi Ferraz -= All rights reserved =-

   *
   -------------------------------------------------------------------------- */


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "spirits.h"
#include "handler.h"
#include "utils.h"
#include "act.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "shop.h"
#include "genzon.h"				/* for real_zone_by_thing */
#include "class.h"
#include "genolc.h"
#include "genobj.h"
#include "fight.h"
#include "house.h"
#include "modify.h"
#include "quest.h"
#include "ban.h"
#include "screen.h"
#include "spedit.h"

void raise_online(struct char_data *ch, struct char_data *raiser, struct obj_data *corpse, room_rnum targ_room, int restore)
{
	struct obj_data *obj = NULL,*next_obj = NULL;
   
	if (targ_room && targ_room != IN_ROOM(ch))
	{
		act("O espírito de $n começa a brilhar e é violentamente puxado para longe.", TRUE, ch, 0, 0, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, targ_room);
	}
	else
	{
		act("O espírito de $n começa a brilhar.", TRUE, ch, 0, 0, TO_ROOM);
	}

	act("@GUma estranha sensação percorre seu espírito, que é puxado por uma força\r\n"
		"divina, carregando-$r por uma longa distância.", FALSE, ch, 0, 0, TO_CHAR);

	REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_GHOST);
	if (AFF_FLAGGED(ch, AFF_FLYING))
		REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);

		act("Você perde a noção de espaço e antes que você possa gritar por socorro,\r\n"
			"você percebe que está sendo puxad$r para dentro de seu corpo!@n",
			FALSE, ch, 0, 0, TO_CHAR);
	else
	{
		send_to_char(ch,
					 "Estranhamente, seu espírito começa a voltar ao formato de um corpo humano.\r\n"
					 "Você começa a sentir novamente seus braços, suas pernas, o chão, o ar\r\n"
					 "entrando pelos seus pulmões!%s\r\n",CCNRM(ch,C_NRM));
	}
	send_to_char(ch, "\r\n");

	if (!restore && (GET_CON(ch) > 3) && (rand_number(1, 101) > (75 + GET_CON(ch))))
	{							// System Shock roll
		GET_CON(ch) -= 1;
		act("$n grita de dor enquanto é ressucitad$r.", TRUE, ch, 0, 0, TO_ROOM);
		send_to_char(ch, "ARRRGH!! Você sente uma dor muito forte!\r\n\r\n");
	}

	if (targ_room)
		look_at_room(ch, TRUE);

	if (!raiser)
	{
		act("$n foi trazid$r devolta à vida pelos Deuses!", TRUE, ch, 0, 0, TO_ROOM);
		act("\tWVocê foi trazid$r devolta à vida pelos Deuses!\tn", FALSE, ch, 0, 0, TO_CHAR);
	}
	else if (GET_LEVEL(raiser) >= LVL_IMMORT)
	{
		act("Sua força divina trouxe $N devolta à vida.", FALSE, raiser, 0, ch, TO_CHAR);
		act("$n foi trazid$r devolta à vida pela força divina de $N!", FALSE, ch, 0, raiser,
			TO_NOTVICT);
		act("\tWVocê foi trazid$r devolta à vida pela força divina de $N!\tn", FALSE, ch, 0,
			raiser, TO_CHAR);
	}
	else
	{
		act("Você sente a força de seu Deus trazendo $N devolta à vida!@n", FALSE, raiser, 0,
			ch, TO_CHAR);
		act("$n foi trazid$r devolta à vida pelo Deus de $N!", FALSE, ch, 0, raiser, TO_NOTVICT);
		act("\tWVocê foi trazid$r devolta à vida pela força divina dos Deuses de $N!\tn", FALSE,
			ch, 0, raiser, TO_CHAR);
	}
	
	if (restore >= 2)
	{
		GET_HIT(ch) = GET_MAX_HIT(ch);
		GET_MANA(ch) = GET_MAX_MANA(ch);
		GET_MOVE(ch) = GET_MAX_MOVE(ch);
		if (GET_LEVEL(ch) < LVL_IMMORT)
		{
			GET_COND(ch, HUNGER) = 24;
			GET_COND(ch, THIRST) = 24;
		}
	}
	else if (restore == 1)
	{
		GET_HIT(ch) = MAX(GET_MAX_HIT(ch) / 4, 1);
		GET_MANA(ch) = MAX(GET_MAX_MANA(ch) / 4, 0);
		GET_MOVE(ch) = MAX(GET_MAX_MOVE(ch) / 4, 0);
		if (GET_LEVEL(ch) < LVL_IMMORT)
		{
			GET_COND(ch, HUNGER) = 12;
			GET_COND(ch, THIRST) = 12;
		}
	}
	else
	{
		GET_HIT(ch) = 1;
		GET_MANA(ch) = 0;
		GET_MOVE(ch) = 0;
		if (GET_LEVEL(ch) < LVL_IMMORT)
		{
			GET_COND(ch, HUNGER) = 2;
			GET_COND(ch, THIRST) = 2;
		}
	}
		if (corpse)
	{
	   	for (obj = corpse->contains; obj != NULL; obj = obj->next_obj)
		{
		  next_obj = obj->next_content;
			obj_from_obj(obj);
			obj_to_char(obj, ch);
			get_check_money(ch, obj);
		}
		if (corpse->in_room)
			obj_from_room(corpse);
		else if (corpse->carried_by)
			obj_from_char(corpse);
		else if (corpse->in_obj)
			obj_from_obj(corpse);
	}
	save_char(ch);
}

void raise_offline(struct char_data *ch, struct obj_data *corpse)
{
   struct obj_data *obj = NULL;
   
	/* Raise player */
	REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_GHOST);
	
	/* System shock roll */
	if ((GET_CON(ch) > 3) && (rand_number(1, 101) > (75 + GET_CON(ch))))
		GET_CON(ch) -= 1;

	/* Restore points */
	GET_HIT(ch) = GET_MAX_HIT(ch);
	GET_MANA(ch) = GET_MAX_MANA(ch);
	GET_MOVE(ch) = GET_MAX_MOVE(ch);
	if (GET_LEVEL(ch) < LVL_IMMORT)
	{
		GET_COND(ch, HUNGER) = 24;
		GET_COND(ch, THIRST) = 24;
	}

	/* If corpse has objects, save them to file */
	Crash_extract_norents(corpse);
	for (obj = corpse->contains; obj != NULL; obj = obj->next_content)
	{
	    if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
      GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
      extract_obj(obj);
	    }
      obj_from_obj(obj);
      Crash_extract_objs(obj);
	}
		

	/* Show message */
	if (corpse->carried_by)
		act("$p é envolvido por uma forte luz branca e desaparece.", FALSE, corpse->carried_by,
			corpse, 0, TO_CHAR);
	else if (corpse->in_room)
		act("$p é envolvido por uma forte luz branca e desaparece.", TRUE, NULL, corpse, 0, TO_ROOM);

	/* Store player */
	save_char(ch);
	
}
/*
* This function may return the following values:
 *   0		Leave corpse at place, ignore
 *   ar_dropobjs	Extract corpse, leave objs in room
 *   ar_extract		Extract corpse and all its contents
 */
 /*
int autoraise(struct obj_data *corpse)
{
  struct char_data *ch = NULL;
  const char *temp;
  int idnum, offline = FALSE;

  if (!corpse) {
    mudlog(CMP,LVL_GOD,"AutoRaise: received null object");
    return (AR_DROPOBJS);
  } else if (GET_OBJ_TYPE(corpse) != ITEM_CORPSE) {
    mudlog(NRM, LVL_GOD,"AutoRaise: received a non-corpse object");
    return (AR_DROPOBJS);
  } else if ((idnum = GET_OBJ_VAL(corpse, 0)) <= 0) {
    return (AR_DROPOBJS);
  }

  if (!(temp = get_name_by_id(idnum))) {
    mudlog(NRM, LVL_DEMIGOD, "AutoRaise: Player recreated its character. Extracting %s.",
	GET_OBJ_NAME(corpse));
    if (corpse->carried_by && corpse->worn_on == INVENTORY)
      act("$p cai de suas mãos.", FALSE, corpse->carried_by, corpse, 0, TO_CHAR);
    else if (corpse->in_room && !LIST_EMPTY(corpse->in_room->people))
      act("$p se desfaz em pó.", TRUE, NULL, corpse, NULL, TO_ROOM);
    return (AR_EXTRACT);
  }

	for (i = 0; i <= top_of_p_table; i++){
    if (!IS_NPC(ch) && (GET_IDNUM(ch) == idnum))
      break;
	}
	
  if (!ch) {
    if ((ch = load_offline_char_by_id(idnum)) == NULL) {
      log(CMP, LVL_GOD, "AutoRaise: Player %s was not found in the playerfile.", temp);
      return (ar_dropobjs);
    }
    offline = TRUE;
  }


  if (PLR_FLAGGED(ch, PLR_DELETED)) {
    mudlog(NRM, LVL_DEMIGOD, "AutoRaise: Player %s has self-deleted. Extracting %s.",
	GET_NAME(ch), GET_OBJ_NAME(corpse));
    if (corpse->carried_by && corpse->worn_on == INVENTORY)
      act("$p cai de suas mãos.", FALSE, corpse->carried_by, corpse, NULL, TO_CHAR);
    else if (corpse->in_room && !LIST_EMPTY(corpse->in_room->people))
      act("$p se desfaz em pó.", TRUE, NULL, corpse, NULL, TO_ROOM);
    if (offline)
      destroy_character(ch);
    return (AR_EXTRACT);
  }

  if (!IS_DEAD(ch)) {
    log(logNrm | logFile, LVL_DEMIGOD, "AutoRaise: Player %s is alive. Extracting %s.", GET_NAME(ch), GET_OBJ_NAME(corpse));
    if (offline)
      destroy_character(ch);
    return (AR_DROPOBJS);
  }

  if (GET_CON(ch) < 8) {
    log(logNrm | logFile, LVL_DEMIGOD, "AutoRaise: Player %s has insufficient constituition.", GET_NAME(ch));
    if (offline)
      destroy_character(ch);
    return (AR_SKIP);
  }


  if (offline) {
    raise_offline(ch, corpse);
    mudog(logNrm | logFile, LVL_DEMIGOD, "AutoRaise: %s (offline, lev %d) has taken back to live.", GET_NAME(ch), GET_LEVEL(ch));
    destroy_character(ch);
  } else {
    raise_online(ch, 0, corpse, corpse->in_room, 0);
    log(logNrm | logFile, LVL_DEMIGOD, "AutoRaise: %s (online, lev %d) has taken back to live.", GET_NAME(ch), GET_LEVEL(ch));
  }
  return (AR_EXTRACT);
}
*/