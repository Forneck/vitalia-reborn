/**************************************************************************
*  File: mobact.c                                          Part of tbaMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles.   *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "graph.h"
#include "fight.h"


/* local file scope only function prototypes */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack);

int get_item_apply_score(struct char_data *ch, struct obj_data *obj);
int evaluate_item_for_mob(struct char_data *ch, struct obj_data *obj);

void mobile_activity(void)
{
  struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  int door, found;
  memory_rec *names;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (!IS_MOB(ch))
      continue;

    /* Examine call for special procedure */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
	log1("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
		GET_NAME(ch), GET_MOB_VNUM(ch));
	REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
      } else {
        char actbuf[MAX_INPUT_LENGTH] = "";
	if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, actbuf))
	  continue;		/* go to next char */
      }
    }

    /* If the mob has no specproc, do the default actions */
    if (FIGHTING(ch) || !AWAKE(ch))
      continue;

    /* hunt a victim, if applicable */
    hunt_victim(ch);

    /* Scavenger (picking up objects) */
    /*************************************************************************
     * Lógica de Genética FINAL: Comportamento de SAQUEAR (Loot)              *
     * Usa a função de avaliação e garante uma chance mínima de aprendizagem. *
     *************************************************************************/
    if (world[IN_ROOM(ch)].contents && ch->genetics && !FIGHTING(ch)) {
        
        const int CURIOSIDADE_AMBIENTE = 10;
        int effective_loot_tendency = 0;
        int base_scavenger_instinct = 0;
        int final_chance = 0;

        if (MOB_FLAGGED(ch, MOB_SCAVENGER)) {
            base_scavenger_instinct = 25;
        }

        effective_loot_tendency = base_scavenger_instinct + GET_GENLOOT(ch);
        final_chance = MAX(effective_loot_tendency, CURIOSIDADE_AMBIENTE);
        final_chance = MIN(final_chance, 90);

        if (rand_number(1, 100) <= final_chance) {
            int max_score = 0;
            best_obj = NULL;

            for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
                int current_score = evaluate_item_for_mob(ch, obj);
                if (current_score > max_score) {
                    best_obj = obj;
                    max_score = current_score;
                }
            }
            
            if (best_obj != NULL) {
                obj_from_room(best_obj);
                obj_to_char(best_obj, ch);
                act("$n pega $p.", FALSE, ch, best_obj, 0, TO_ROOM);

                ch->genetics->loot_tendency += 2;
                if (ch->genetics->loot_tendency > 100)
                    ch->genetics->loot_tendency = 100;
            } else {
                ch->genetics->loot_tendency -= 1;
                if (ch->genetics->loot_tendency < 0)
                    ch->genetics->loot_tendency = 0;
            }
        }
    }

    /* Mob Movement */
    if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
       ((door = rand_number(0, 18)) < DIR_COUNT) && CAN_GO(ch, door) &&
       !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
       !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
       (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
           (world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone))) 
    {
      /* If the mob is charmed, do not move the mob. */
      if (ch->master == NULL)
        perform_move(ch, door, 1);
    }

    /* Aggressive Mobs */
     if (!MOB_FLAGGED(ch, MOB_HELPER) && (!AFF_FLAGGED(ch, AFF_BLIND) || !AFF_FLAGGED(ch, AFF_CHARM))) {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
//	if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
	if (!CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
	  continue;

	if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
	  continue;

	if (MOB_FLAGGED(ch, MOB_AGGRESSIVE  ) ||
	   (MOB_FLAGGED(ch, MOB_AGGR_EVIL   ) && IS_EVIL(vict)) ||
	   (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
	   (MOB_FLAGGED(ch, MOB_AGGR_GOOD   ) && IS_GOOD(vict))) {

          /* Can a master successfully control the charmed monster? */
          if (aggressive_mob_on_a_leash(ch, ch->master, vict))
            continue;
            
         if (vict == ch)
            continue;
            
            if (IS_NPC(vict))
            continue;
      if (rand_number(0, 20) <= GET_CHA(vict)) {
        act("$n olha para $N com indiferença.",
            FALSE, ch, 0, vict, TO_NOTVICT);
        act("$N olha para você com indiferença.",
            FALSE, vict, 0, ch, TO_CHAR);
      } else {
	  hit(ch, vict, TYPE_UNDEFINED);
	  found = TRUE;
      }
	}
      }
    }

    /* Mob Memory */
    if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
	if (!CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
	  continue;

	for (names = MEMORY(ch); names && !found; names = names->next) {
	  if (names->id != GET_IDNUM(vict))
            continue;

          /* Can a master successfully control the charmed monster? */
          if (aggressive_mob_on_a_leash(ch, ch->master, vict))
            continue;

          found = TRUE;
          act("''Ei!  Você é o demônio que me atacou!!!', exclama $n.", FALSE, ch, 0, 0, TO_ROOM);
          hit(ch, vict, TYPE_UNDEFINED);
        }
      }
    }

    /* Charmed Mob Rebellion: In order to rebel, there need to be more charmed 
     * monsters than the person can feasibly control at a time.  Then the
     * mobiles have a chance based on the charisma of their leader.
     * 1-4 = 0, 5-7 = 1, 8-10 = 2, 11-13 = 3, 14-16 = 4, 17-19 = 5, etc. */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && num_followers_charmed(ch->master) > (GET_CHA(ch->master) - 2) / 3) {
      if (!aggressive_mob_on_a_leash(ch, ch->master, ch->master)) {
        if (CAN_SEE(ch, ch->master) && !PRF_FLAGGED(ch->master, PRF_NOHASSLE))
          hit(ch, ch->master, TYPE_UNDEFINED);
        stop_follower(ch);
      }
    }

   /* Help your master */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
     vict = FIGHTING(ch->master);
      if (vict && vict != ch) {
        act("$n se arremessa para salvar $N!", FALSE, ch, 0, ch->master, TO_ROOM);
        hit(ch, vict, TYPE_UNDEFINED);
      }
    }


    /* Helper Mobs */
   if (!FIGHTING(ch) && MOB_FLAGGED(ch, MOB_HELPER) && 
      (!AFF_FLAGGED(ch, AFF_BLIND) || !AFF_FLAGGED(ch, AFF_CHARM))) 
    {
      found = FALSE;
      for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) 
      {
	      if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
          continue; 
        if (GROUP(vict) && GROUP(vict) == GROUP(ch))
          continue;
	      if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
          continue;

	      act("$n se arremessa para salvar $N!", FALSE, ch, 0, vict, TO_ROOM);
	      hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
	      found = TRUE;
      }
    }

    /* Add new mobile actions here */

  }				/* end for() */
}

/* Mob Memory Routines */
/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
  memory_rec *tmp;
  bool present = FALSE;

  if ( IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim)) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return;			/* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}

/* erase ch's memory */
void clearMemory(struct char_data *ch)
{
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}

/* An aggressive mobile wants to attack something.  If they're under the 
 * influence of mind altering PC, then see if their master can talk them out 
 * of it, eye them down, or otherwise intimidate the slave. */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack)
{
  static int snarl_cmd;
  int dieroll;

  if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
    return (FALSE);

  if (!snarl_cmd)
    snarl_cmd = find_command("snarl");

  /* Sit. Down boy! HEEEEeeeel! */
  dieroll = rand_number(1, 20);
  if (dieroll != 1 && (dieroll == 20 || dieroll > 10 - GET_CHA(master) + GET_INT(slave))) {
    if (snarl_cmd > 0 && attack && !rand_number(0, 3)) {
      char victbuf[MAX_NAME_LENGTH + 1];

      strncpy(victbuf, GET_NAME(attack), sizeof(victbuf));	/* strncpy: OK */
      victbuf[sizeof(victbuf) - 1] = '\0';

      do_action(slave, victbuf, snarl_cmd, 0);
    }

    /* Success! But for how long? Hehe. */
    return (TRUE);
  }

  /* So sorry, now you're a player killer... Tsk tsk. */
  return (FALSE);
}

/* Em src/mobact.c */

/**
 * Calcula uma pontuação para os bónus (applies e affects) de um objeto.
 * Esta função age como um "identify" para o mob.
 */
int get_item_apply_score(struct char_data *ch, struct obj_data *obj)
{
    int i, total_score = 0;

    if (obj == NULL) {
        return 0;
    }

    /* 1. Avalia os applies numéricos (bónus de stats) */
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location != APPLY_NONE) {
            switch (obj->affected[i].location) {
                case APPLY_STR:
                case APPLY_DEX:
                case APPLY_CON:
                case APPLY_INT:
                case APPLY_WIS:
                case APPLY_CHA:
                    total_score += obj->affected[i].modifier * 5;
                    break;
                case APPLY_HIT:
                case APPLY_MANA:
                case APPLY_MOVE:
                    total_score += obj->affected[i].modifier;
                    break;
                case APPLY_AC:
                    total_score -= obj->affected[i].modifier * 5;
                    break;
                case APPLY_HITROLL:
                case APPLY_DAMROLL:
                    total_score += obj->affected[i].modifier * 10;
                    break;
                default:
                    break;
            }
        }
    }

    /* 2. Avalia os bónus de afetações (AFF_) - só dá pontos se o mob ainda não tiver o bónus */

    // --- Auras Protetoras (Muito Valiosas) ---
    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_SANCTUARY) && !AFF_FLAGGED(ch, AFF_SANCTUARY))
        total_score += 200;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_STONESKIN) && !AFF_FLAGGED(ch, AFF_STONESKIN))
        total_score += 180;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_GLOOMSHIELD) && !AFF_FLAGGED(ch, AFF_GLOOMSHIELD))
        total_score += 150;

    // --- Escudos de Dano (Damage Shields) ---
    if ((IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_FIRESHIELD) && !AFF_FLAGGED(ch, AFF_FIRESHIELD)) ||
        (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_THISTLECOAT) && !AFF_FLAGGED(ch, AFF_THISTLECOAT)) ||
        (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_WINDWALL) && !AFF_FLAGGED(ch, AFF_WINDWALL)))
        total_score += 120;

    // --- Bónus de Alinhamento ---
    if (IS_GOOD(ch) && IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_PROTECT_EVIL) && !AFF_FLAGGED(ch, AFF_PROTECT_EVIL))
        total_score += 100;

    if (IS_EVIL(ch) && IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_PROTECT_GOOD) && !AFF_FLAGGED(ch, AFF_PROTECT_GOOD))
        total_score += 100;

    // --- Bónus Táticos (Ofensivos/Furtivos) ---
    if ((IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_INVISIBLE) && !AFF_FLAGGED(ch, AFF_INVISIBLE)) ||
        (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_SNEAK) && !AFF_FLAGGED(ch, AFF_SNEAK)) ||
        (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_HIDE) && !AFF_FLAGGED(ch, AFF_HIDE)))
        total_score += 70;

    // --- Bónus de Deteção ---
    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_DETECT_INVIS) && !AFF_FLAGGED(ch, AFF_DETECT_INVIS))
        total_score += 60;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_SENSE_LIFE) && !AFF_FLAGGED(ch, AFF_SENSE_LIFE))
        total_score += 60;

    // --- Utilidades ---
    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_INFRAVISION) && !AFF_FLAGGED(ch, AFF_INFRAVISION))
        total_score += 30;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_LIGHT) && !AFF_FLAGGED(ch, AFF_LIGHT))
        total_score += 20;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_WATERWALK) && !AFF_FLAGGED(ch, AFF_WATERWALK))
        total_score += 15;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_FLYING) && !AFF_FLAGGED(ch, AFF_FLYING))
        total_score += 40;

    return total_score;
}


/**
 * Avalia o quão "desejável" um objeto é para um determinado mob.
 * Retorna uma pontuação numérica. Quanto maior a pontuação, mais o mob quer o item.
 */
int evaluate_item_for_mob(struct char_data *ch, struct obj_data *obj)
{
    int score = 0;

    if (!CAN_GET_OBJ(ch, obj) || !CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
        return 0;
    }

    /* Filtro de Alinhamento */
    if ((IS_EVIL(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_EVIL)) ||
        (IS_GOOD(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_GOOD)) ||
        (IS_NEUTRAL(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL))) {
        return 0;
    }

    /* Avaliação de Armas */
    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
        struct obj_data *current_weapon = GET_EQ(ch, WEAR_WIELD);
        float new_w_dam = (float)(GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1)) / 2.0;
        int new_w_stats_score = get_item_apply_score(ch, obj);

        if (current_weapon == NULL) {
            score += 100 + new_w_stats_score;
        } else {
            float old_w_dam = (float)(GET_OBJ_VAL(current_weapon, 1) * (GET_OBJ_VAL(current_weapon, 2) + 1)) / 2.0;
            int old_w_stats_score = get_item_apply_score(ch, current_weapon);
            int total_improvement = ((int)(new_w_dam - old_w_dam) * 10) + (new_w_stats_score - old_w_stats_score);
            if (total_improvement > 0) {
                score += total_improvement;
            }
        }
    }

    /* Avaliação de Armaduras */
    if (GET_OBJ_TYPE(obj) == ITEM_ARMOR) {
        int wear_pos = find_eq_pos(ch, obj, NULL);
        if (wear_pos != -1) {
            struct obj_data *current_armor = GET_EQ(ch, wear_pos);
            int new_armor_ac = GET_OBJ_VAL(obj, 0);
            int new_armor_stats_score = get_item_apply_score(ch, obj);

            if (current_armor == NULL) {
                score += 50 + new_armor_stats_score;
            } else {
                int old_armor_ac = GET_OBJ_VAL(current_armor, 0);
                int old_armor_stats_score = get_item_apply_score(ch, current_armor);
                int ac_improvement = (old_armor_ac - new_armor_ac) * 5;
                int stats_improvement = new_armor_stats_score - old_armor_stats_score;

                if (ac_improvement + stats_improvement > 0) {
                    score += ac_improvement + stats_improvement;
                }
            }
        }
    }

    /* Bónus base pelo valor em ouro (peso baixo) */
    score += GET_OBJ_COST(obj) / 100;

    return score;
}
