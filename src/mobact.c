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
#include "shop.h"
#include "graph.h"

/* local file scope only function prototypes */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack);

struct char_data *find_best_median_leader(struct char_data *ch);
bool mob_handle_grouping(struct char_data *ch);
bool mob_leader_evaluates_gear(struct char_data *leader);
bool perform_move_IA(struct char_data *ch, int dir, bool should_close_behind, int was_in);
bool mob_goal_oriented_roam(struct char_data *ch);
bool handle_duty_routine(struct char_data *ch);
bool mob_follow_leader(struct char_data *ch);
bool mob_assist_allies(struct char_data *ch);
bool mob_try_and_loot(struct char_data *ch);
bool mob_try_and_upgrade(struct char_data *ch);
void mobile_activity(void)
{
  struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  int  found;
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
    
    if (FIGHTING(ch) || !AWAKE(ch))
      continue;
    
    if (ch->ai_data && ch->ai_data->duty_frustration_timer > 0) {
        ch->ai_data->duty_frustration_timer--;
    }
    
    if (handle_duty_routine(ch))
	    continue;

    if (mob_index[GET_MOB_RNUM(ch)].func == shop_keeper) {
    	int shop_nr = find_shop_by_keeper(GET_MOB_RNUM(ch));
	    if (shop_nr != -1 && !is_shop_open(shop_nr)) {
        /* --- IA ECONÓMICA: GESTÃO DE STOCK --- */
        
        /* O lojista verifica o seu inventário à procura de lixo para destruir. */
        	struct obj_data *current_obj, *next_obj;
	        for (current_obj = ch->carrying; current_obj; current_obj = next_obj) {
	            next_obj = current_obj->next_content;
	            if (OBJ_FLAGGED(current_obj, ITEM_TRASH)) {
	                act("$n joga $p fora.", FALSE, ch, current_obj, 0, TO_ROOM);
	                extract_obj(current_obj);
	            }
	        }
	        continue;
	    }
    }
    if GROUP(ch){
	    mob_follow_leader(ch);
    }
    

    mob_assist_allies(ch);
    
    mob_try_and_loot(ch);

    /* hunt a victim, if applicable */
    hunt_victim(ch);
    

    if (mob_handle_grouping(ch)) {
        continue; /* Ação do pulso foi usada para agrupar, fim do turno. */
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
            
            //if (IS_NPC(vict))
            //continue;

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
    
    mob_try_and_upgrade(ch);
    
    /*Depois de equipar, lider avalia equipamento sobrando e doa para o grupo*/
    if (mob_leader_evaluates_gear(ch)) {
        continue;
    }
	  
    /* Prioridade de Vaguear (Roam) */
    if (mob_goal_oriented_roam(ch)) {
        continue;
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

/**
 * Verifica se um mob possui qualquer item do tipo munição.
 * @param ch O mob a ser verificado.
 * @return TRUE se tiver munição, FALSE caso contrário.
 */
bool mob_has_ammo(struct char_data *ch)
{
    struct obj_data *obj;

    /* Primeiro, verifica o item mais óbvio: a aljava (quiver) equipada. */
    if (GET_EQ(ch, WEAR_QUIVER) != NULL) {
        return TRUE;
    }

    /* Se não, percorre o inventário. */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_AMMO) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Verifica se dois mobs são compatíveis para se agruparem, baseado
 * no nível e no alinhamento.
 */
bool are_groupable(struct char_data *ch, struct char_data *target)
{
    /* Regras básicas de compatibilidade (não agrupa com jogadores, etc.) */
    if (!target || ch == target || !IS_NPC(target) || target->master != NULL)
        return FALSE;

    /* REGRA DE ALINHAMENTO */
    bool align_ok = FALSE;
    if (IS_NEUTRAL(ch) || IS_NEUTRAL(target))
        align_ok = TRUE; /* Neutros podem com todos. */
    else if ( (IS_GOOD(ch) && IS_GOOD(target)) || (IS_EVIL(ch) && IS_EVIL(target)) )
        align_ok = TRUE; /* Bem com Bem, Mal com Mal. */

    if (!align_ok)
        return FALSE;
    
    /* REGRA DE NÍVEL (simples, para verificações 1 a 1) */
    if (abs(GET_LEVEL(ch) - GET_LEVEL(target)) > 15)
        return FALSE;

    return TRUE;
}

/**
 * Verifica se um novo membro ('prospect') é compatível em nível com um
 * grupo já existente, respeitando a regra de 15 níveis de diferença
 * entre o novo mínimo e o novo máximo do grupo potencial.
 */
bool is_level_compatible_with_group(struct char_data *prospect, struct group_data *group)
{
    if (!prospect || !group || !group->members || group->members->iSize == 0)
        return FALSE;

    struct char_data *member;
    struct iterator_data iterator;
    
    int min_level = GET_LEVEL(prospect);
    int max_level = GET_LEVEL(prospect);

    /* Itera pelos membros existentes para encontrar o min/max atual do grupo */
    member = (struct char_data *)merge_iterator(&iterator, group->members);
    while(member) {
        if (GET_LEVEL(member) < min_level) min_level = GET_LEVEL(member);
        if (GET_LEVEL(member) > max_level) max_level = GET_LEVEL(member);
        member = (struct char_data *)next_in_list(&iterator);
    }
    
    /* Retorna TRUE se a nova diferença total do grupo for 15 ou menos. */
    return ((max_level - min_level) <= 15);
}


/**
 * Avalia todos os mobs solitários numa sala para encontrar o melhor candidato
 * a líder para um novo grupo, com base na regra do "líder mediano".
 * @param ch O mob que está a iniciar a verificação.
 * @return Um ponteiro para o melhor candidato a líder, ou NULL se nenhum grupo for viável.
 */
struct char_data *find_best_leader_for_new_group(struct char_data *ch)
{
    struct char_data *vict, *leader_candidate;
    int min_level = -1, max_level = -1;
    int count = 0;
    struct char_data *potential_members[51]; /* Buffer para potenciais membros */

    /* 1. Reúne todos os candidatos (mobs solitários e compatíveis) na sala. */
    for (vict = world[IN_ROOM(ch)].people; vict && count < 50; vict = vict->next_in_room) {
        if (!IS_NPC(vict) || vict->master != NULL || GROUP(vict))
            continue;
            
        /* Usa a nossa função 'are_groupable' para verificar alinhamento e nível 1-a-1 */
        if (!are_groupable(ch, vict))
            continue;
            
        potential_members[count++] = vict;
        if (min_level == -1 || GET_LEVEL(vict) < min_level) min_level = GET_LEVEL(vict);
        if (max_level == -1 || GET_LEVEL(vict) > max_level) max_level = GET_LEVEL(vict);
    }
    potential_members[count] = NULL;
    
    if (count <= 1) return ch; /* Se estiver sozinho, ele pode liderar. */

    /* 2. Se a diferença de nível já for válida, o de nível mais alto lidera. */
    if ((max_level - min_level) <= 15) {
        struct char_data *best_leader = NULL;
        for (int i = 0; i < count; i++) {
            if (best_leader == NULL || GET_LEVEL(potential_members[i]) > GET_LEVEL(best_leader)) {
                best_leader = potential_members[i];
            }
        }
        return best_leader;
    }
    
    /* 3. Se a diferença for grande, procura o "líder mediano" de nível mais alto. */
    struct char_data *best_median = NULL;
    for (int i = 0; i < count; i++) {
        leader_candidate = potential_members[i];
        if ((max_level - GET_LEVEL(leader_candidate)) <= 15 && (GET_LEVEL(leader_candidate) - min_level) <= 15) {
            if (best_median == NULL || GET_LEVEL(leader_candidate) > GET_LEVEL(best_median)) {
                best_median = leader_candidate;
            }
        }
    }
    
    return best_median; /* Pode retornar NULL se nenhum grupo for viável */
}


/**
 * A IA principal para um mob tentar formar ou juntar-se a um grupo.
 * Retorna TRUE se uma ação de grupo foi tentada/realizada.
 */
bool mob_handle_grouping(struct char_data *ch)
{
    if (MOB_FLAGGED(ch, MOB_SENTINEL))
        return FALSE;

    if (GROUP(ch) || !ch->ai_data)
        return FALSE;

    /* Verifica a chance de tentar agrupar-se. */
    const int CURIOSIDADE_MINIMA_GRUPO = 5;
    if (rand_number(1, 100) > MAX(GET_GENGROUP(ch), CURIOSIDADE_MINIMA_GRUPO))
        return FALSE;

    struct char_data *vict, *best_target_leader = NULL;
    bool best_is_local = FALSE;
    int max_group_size = 6;

    /* 1. Procura pelo MELHOR grupo existente para se juntar. */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
        if (GROUP(vict) && GROUP_LEADER(GROUP(vict)) == vict &&
            IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN) &&
            GROUP(vict)->members->iSize < max_group_size &&
            is_level_compatible_with_group(ch, GROUP(vict)) &&
            are_groupable(ch, vict)) {

            bool is_local = (world[IN_ROOM(ch)].zone == world[IN_ROOM(vict)].zone);

            if (best_target_leader == NULL) {
                best_target_leader = vict;
                best_is_local = is_local;
            } else if (is_local && !best_is_local) {
                best_target_leader = vict;
                best_is_local = TRUE;
            } else if (is_local == best_is_local && GET_LEVEL(vict) > GET_LEVEL(best_target_leader)) {
                best_target_leader = vict;
            }
        }
    }
    
    if (best_target_leader) {
        /* Lógica de Aceitação do Líder */
        int chance_aceitar = 100 - (GROUP(best_target_leader)->members->iSize * 15) + GET_GENGROUP(best_target_leader);
        if (rand_number(1, 120) <= chance_aceitar) {
            join_group(ch, GROUP(best_target_leader));
            act("$n junta-se ao grupo de $N.", TRUE, ch, 0, best_target_leader, TO_ROOM);
            return TRUE;
        }
    } else {
        /* 2. Não encontrou grupo. Verifica se é possível formar um novo. */
        struct char_data *leader_to_be = find_best_leader_for_new_group(ch);
        
        if (leader_to_be != NULL && leader_to_be == ch) {
            /* O próprio mob 'ch' é o melhor candidato a líder, então ele cria o grupo. */
            struct group_data *new_group;
            CREATE(new_group, struct group_data, 1);
	    new_group->leader = NULL;
            new_group->members = create_list();
            SET_BIT(new_group->group_flags, GROUP_ANON);
            SET_BIT(new_group->group_flags, GROUP_OPEN);
            SET_BIT(new_group->group_flags, GROUP_NPC); /* Marca como um grupo de mobs */

            join_group(ch, new_group);
            act("$n parece estar a formar um grupo e à procura de companheiros.", TRUE, ch, 0, 0, TO_ROOM);
            return TRUE;
        }
    }
    return FALSE; 
}

/**
 * O líder do grupo avalia o seu inventário e distribui upgrades
 * para os membros da sua equipa.
 * Retorna TRUE se uma ação de partilha foi realizada.
 */
bool mob_leader_evaluates_gear(struct char_data *leader)
{
    /* Apenas líderes de grupo com inventário e genética executam esta lógica. */
    if (!GROUP(leader) || GROUP_LEADER(GROUP(leader)) != leader || !leader->carrying || !leader->ai_data) {
        return FALSE;
    }

    /* Chance de executar a avaliação (para não sobrecarregar) */
    if (rand_number(1, 100) > 25) {
        return FALSE;
    }

    struct obj_data *inv_item, *item_to_give = NULL;
    struct char_data *member, *receiver = NULL;
    struct iterator_data iterator;
    int max_improvement_for_group = 10; /* Só partilha se for uma melhoria significativa */

    /* Para cada item no inventário do líder... */
    for (inv_item = leader->carrying; inv_item; inv_item = inv_item->next_content) {
        
        /* ...avalia cada membro do grupo. */
        member = (struct char_data *)merge_iterator(&iterator, GROUP(leader)->members);
        while(member) {
            if (leader == member) { /* Não avalia a si mesmo */
                member = (struct char_data *)next_in_list(&iterator);
                continue;
            }

            int wear_pos = find_eq_pos(member, inv_item, NULL);
            if (wear_pos != -1) {
                struct obj_data *member_eq = GET_EQ(member, wear_pos);
                
                /* Usamos a nossa função de avaliação, mas do ponto de vista do membro! */
                int score_improvement = evaluate_item_for_mob(member, inv_item) - evaluate_item_for_mob(member, member_eq);
                
                if (score_improvement > max_improvement_for_group) {
                    max_improvement_for_group = score_improvement;
                    item_to_give = inv_item;
                    receiver = member;
                }
            }
            member = (struct char_data *)next_in_list(&iterator);
        }
    }

    /* Se, depois de analisar tudo, encontrou uma boa partilha, executa-a. */
    if (item_to_give && receiver) {
        act("$n dá $p para $N.", FALSE, leader, item_to_give, receiver, TO_NOTVICT);
        act("$n dá-lhe $p.", FALSE, leader, item_to_give, receiver, TO_VICT);
        act("Você dá $p para $N.", FALSE, leader, item_to_give, receiver, TO_CHAR);

        obj_from_char(item_to_give);
        obj_to_char(item_to_give, receiver);
        
        /* APRENDIZAGEM: Comportamento de liderança é recompensado. */
        leader->ai_data->genetics.group_tendency += 5;
        leader->ai_data->genetics.group_tendency = MIN(GET_GENGROUP(leader), 100);
        
        return TRUE; /* Ação de partilha foi realizada. */
    }

    return FALSE;
}

/**
 * Função de apoio que executa o movimento, táticas pós-movimento, e aprendizagem.
 * VERSÃO FINAL COM APRENDIZAGEM AMBIENTAL.
 * @param ch O mob que se move.
 * @param dir A direção do movimento.
 * @param should_close_behind TRUE se a IA decidiu que deve fechar a porta.
 * @param was_in A sala de onde o mob veio.
 * @return TRUE se o mob se moveu, FALSE caso contrário.
 */
bool perform_move_IA(struct char_data *ch, int dir, bool should_close_behind, int was_in)
{
    int old_pos = GET_POS(ch);

    if (perform_move(ch, dir, 1)) {
        /* O movimento foi bem-sucedido. */

        /* TÁTICA PÓS-MOVIMENTO: fechar a porta atrás de si (já implementado). */
        if (should_close_behind) {
            int back_door = rev_dir[dir];
            if (EXIT(ch, back_door) && EXIT(ch, back_door)->to_room == was_in &&
                EXIT(ch, back_door)->keyword != NULL && !IS_SET(EXIT(ch, back_door)->exit_info, EX_DNCLOSE)) {
                do_doorcmd(ch, NULL, back_door, SCMD_CLOSE);
            }
        }

        /******************************************************************
         * APRENDIZAGEM PÓS-MOVIMENTO (VERSÃO FINAL E REFINADA)
         ******************************************************************/
        if (ch->ai_data) {
            int roam_change = 0;
            int current_sect = world[IN_ROOM(ch)].sector_type;

            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) || current_sect == SECT_LAVA || current_sect == SECT_QUICKSAND) {
                roam_change = -20; /* Penalidade severa por perigo mortal. */
                act("$n grita de dor ao entrar na armadilha!", FALSE, ch, 0, 0, TO_ROOM);
            } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FROZEN)) {
                roam_change = -10; /* Penalidade média por desconforto. */
                act("$n treme de frio ao entrar na sala gelada.", FALSE, ch, 0, 0, TO_ROOM);
            } else if (current_sect == SECT_ICE && GET_POS(ch) < old_pos) {
                roam_change = -5;  /* Penalidade leve por escorregar. */
            } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HEAL)) {
                roam_change = 10;   /* Recompensa média por encontrar um santuário. */
                act("$n parece revigorado ao entrar nesta sala.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                roam_change = 1;   /* Recompensa normal por exploração segura. */
            }
            
            ch->ai_data->genetics.roam_tendency += roam_change;
            ch->ai_data->genetics.roam_tendency = MIN(MAX(GET_GENROAM(ch), 0), 100);
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * IA de exploração orientada a objetivos. O mob agora vagueia com um propósito.
 * VERSÃO FINAL E DEFINITIVA: Segura, tática e com fluxo de ações corrigido.
 * Retorna TRUE se uma ação de roam foi executada.
 */
bool mob_goal_oriented_roam(struct char_data *ch)
{
    if (ch->master != NULL || FIGHTING(ch) || GET_POS(ch) < POS_STANDING)
        return FALSE;

    int direction = -1;
    bool has_goal = FALSE;

    /* 1. Define um objetivo (voltar ao posto ou explorar). */
    if (MOB_FLAGGED(ch, MOB_SENTINEL) && IN_ROOM(ch) != real_room(GET_LOADROOM(ch))) {
        direction = find_first_step(IN_ROOM(ch), real_room(GET_LOADROOM(ch)));
        has_goal = TRUE;
    } else {
        int base_roam = MOB_FLAGGED(ch, MOB_SENTINEL) ? 1 : 25;
        int curiosidade_minima = 10;
        int chance_roll = 100;
        int need_bonus = (GET_EQ(ch, WEAR_WIELD) == NULL ? 20 : 0) + (!GROUP(ch) ? 10 : 0);
        
        /************************************************************/
        /* REFINAMENTO: Sentinelas são muito mais relutantes.       */
        if (MOB_FLAGGED(ch, MOB_SENTINEL)) {
            curiosidade_minima = 0;
            chance_roll = 1000; /* Mais difícil de ser motivado. */
        }
        /************************************************************/

        int final_chance = MIN(MAX(base_roam + GET_GENROAM(ch) + need_bonus, curiosidade_minima), 90);
        
        if (rand_number(1, chance_roll) <= final_chance) {
            direction = rand_number(0, DIR_COUNT - 1);
            has_goal = TRUE;
        }
    }


    /* 2. Se tiver um objetivo, tenta agir. */
    if (has_goal && direction >= 0 && direction < DIR_COUNT) { /* Verificação de segurança para direction */
        struct room_direction_data *exit;
        room_rnum to_room;

        if ((exit = EXIT(ch, direction)) && (to_room = exit->to_room) <= top_of_world) {

            /* GESTÃO DE VOO (Ação que consome o turno) */
            if (AFF_FLAGGED(ch, AFF_FLYING) && ROOM_FLAGGED(to_room, ROOM_NO_FLY)) { stop_flying(ch); return TRUE; }
            if (!AFF_FLAGGED(ch, AFF_FLYING) && world[to_room].sector_type == SECT_CLIMBING) { if(start_flying(ch)) return TRUE; }

            /* RESOLUÇÃO DE PORTAS (Ação encadeada e segura) */
            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED)) {
                if (!IS_SET(exit->exit_info, EX_DNOPEN)) {
                    if (IS_SET(exit->exit_info, EX_LOCKED) && has_key(ch, exit->key)) {
                        REMOVE_BIT(exit->exit_info, EX_LOCKED);
                        act("$n usa uma chave e destranca $t.", FALSE, ch, 0, exit->keyword, TO_ROOM);
                    }
                    if (!IS_SET(exit->exit_info, EX_LOCKED)) {
                        REMOVE_BIT(exit->exit_info, EX_CLOSED);
                        act("$n abre $t.", FALSE, ch, 0, exit->keyword, TO_ROOM);
                    }
                }
            }

            /* Se, depois de tudo, a porta ainda estiver fechada, a IA não pode passar. */
            if (IS_SET(exit->exit_info, EX_CLOSED)) {
                return FALSE;
            }

            /* Verificações Finais de Caminho */
            if ((IS_SET(exit->exit_info, EX_HIDDEN) && rand_number(1, 20) > GET_WIS(ch)) ||
                (world[to_room].sector_type == SECT_UNDERWATER && !has_scuba(ch))) {
                 return FALSE;
            }
            if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && (world[to_room].zone != world[IN_ROOM(ch)].zone) && (rand_number(1, 100) > 1)) {
                return TRUE; /* Hesitou e gastou o turno. */
            }

            /* Movimento Final */
            int was_in = IN_ROOM(ch);
            bool should_close = (GET_INT(ch) > 12 && IS_SET(exit->exit_info, EX_ISDOOR) && !IS_SET(exit->exit_info, EX_DNCLOSE) && rand_number(1, 100) <= 25);
            return perform_move_IA(ch, direction, should_close, was_in);
        }
    }
    return FALSE;
}

/**
 * Gere a rotina de dever para lojistas e sentinelas, agora com um
 * "temporizador de frustração" para evitar que fiquem presos.
 * Retorna TRUE se uma ação de dever foi executada ou se o mob deve ficar parado.
 */
bool handle_duty_routine(struct char_data *ch)
{
    bool is_shopkeeper = (mob_index[GET_MOB_RNUM(ch)].func == shop_keeper);
    bool is_sentinel = MOB_FLAGGED(ch, MOB_SENTINEL);

    if (!is_shopkeeper && !is_sentinel)
        return FALSE; /* Não tem dever, a IA prossegue. */

    /******************************************************************
    * LÓGICA DE FRUSTRAÇÃO
    ******************************************************************/
    /* Se o mob está frustrado, ele "desiste" de voltar para casa por agora. */
    if (ch->ai_data && ch->ai_data->duty_frustration_timer > 0) {
        return FALSE; /* Permite que o mob execute outras IAs (loot, roam, etc.). */
    }

    bool is_on_duty = FALSE;
    room_rnum home_room = NOWHERE;

    if (is_shopkeeper) {
        int shop_nr = find_shop_by_keeper(GET_MOB_RNUM(ch));
        if (shop_nr != -1 && is_shop_open(shop_nr)) {
            is_on_duty = TRUE;
            home_room = real_room(SHOP_ROOM(shop_nr, 0));
        }
    } else { /* É um Sentinela */
        is_on_duty = TRUE;
        if (ch->ai_data) home_room = real_room(ch->ai_data->guard_post);
    }

    if (is_on_duty) {
        /* Verifica se o mob já está no seu posto. */
        if ((is_shopkeeper && ok_shop_room(find_shop_by_keeper(GET_MOB_RNUM(ch)), GET_ROOM_VNUM(IN_ROOM(ch)))) ||
            (is_sentinel && IN_ROOM(ch) == home_room)) {
            return TRUE; /* Está no posto, não faz mais nada. Fim do turno. */
        }

        /* Se não está no posto, tenta voltar. */
        if (home_room != NOWHERE) {
            int direction = find_first_step(IN_ROOM(ch), home_room);

            if (direction >= 0) {
                perform_move(ch, direction, 1);
            } else {
                /******************************************************************
                * O CAMINHO FALHOU! O MOB FICA FRUSTRADO.
                *******************************************************************/
                if (ch->ai_data) {
                    ch->ai_data->duty_frustration_timer = 6; /* Fica frustrado por 6 pulsos de IA. */
                }
                return FALSE; /* Permite que outras IAs sejam executadas. */
            }
            return TRUE; /* Tentou voltar, consome o turno. */
        }
    }

    /* Se não está de serviço, está de "folga". */
    return FALSE;
}

/**
 * Lógica para um membro de grupo seguir o seu líder se estiverem em salas diferentes.
 * VERSÃO REFINADA: Implementa a "Hierarquia de Comando".
 * @param ch O mob a executar a ação.
 * @return TRUE se o mob tentou mover-se, consumindo o seu turno de IA.
 */
bool mob_follow_leader(struct char_data *ch)
{
    /* A função só se aplica a membros de um grupo (que não são o líder). */
    if (!GROUP(ch) || GROUP_LEADER(GROUP(ch)) == ch)
        return FALSE;

    struct char_data *leader = GROUP_LEADER(GROUP(ch));

    /* Verifica se o líder é válido e se está numa sala diferente. */
    if (leader != NULL && IN_ROOM(ch) != IN_ROOM(leader)) {

        /******************************************************************/
        /* REFINAMENTO: A IA agora entende a "Hierarquia de Comando".     */
        /******************************************************************/
        bool duty_is_overridden_by_player = FALSE;

        /* Se o mob está encantado, o seu dever é seguir o mestre. */
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master == leader) {
            duty_is_overridden_by_player = TRUE;
        }
        /* Se o líder é um jogador (o grupo não tem a flag NPC), o dever é seguir o líder. */
        if (!IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_NPC)) {
            duty_is_overridden_by_player = TRUE;
        }

        /* Um Sentinela só pode ser "teimoso" se não estiver sob ordens diretas de um jogador. */
        if (MOB_FLAGGED(ch, MOB_SENTINEL) && !duty_is_overridden_by_player && rand_number(1, 100) > 2) {
            return FALSE; /* Ficou no posto, leal ao seu dever original. */
        }

        /* Tenta encontrar o caminho até ao líder. */
        int direction = find_first_step(IN_ROOM(ch), IN_ROOM(leader));

        if (direction >= 0) {
            room_rnum to_room;
            if (EXIT(ch, direction) && (to_room = EXIT(ch, direction)->to_room) <= top_of_world) {

                /* Regra especial para salas NOMOB. */
                if (ROOM_FLAGGED(to_room, ROOM_NOMOB)) {
                    /* Só pode entrar se o líder for um jogador (o que já é verificado acima). */
                    if (IS_NPC(leader)) {
                        return FALSE;
                    }
                }

                perform_move(ch, direction, 1);
                return TRUE; /* Ação de seguir foi executada. */
            }
        }
    }

    return FALSE; /* Nenhuma ação de seguir foi executada. */
}

/**
 * A IA principal para um mob ocioso decidir ajudar aliados em combate.
 * Consolida as lógicas de grupo, charmed e helper.
 * Retorna TRUE se o mob entrou em combate.
 */
bool mob_assist_allies(struct char_data *ch)
{
    struct char_data *ally_in_trouble = NULL;
    struct char_data *target_to_attack = NULL;
    int max_threat_level = 0;

    /* A IA só age se o mob estiver ocioso e puder ver. */
    if (FIGHTING(ch) || AFF_FLAGGED(ch, AFF_BLIND)) {
        return FALSE;
    }

    /* PRIORIDADE 1: Ajudar o Mestre (se estiver encantado) */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && FIGHTING(ch->master)) {
        if (IN_ROOM(ch) == IN_ROOM(ch->master)) {
            ally_in_trouble = ch->master;
            target_to_attack = FIGHTING(ch->master);
        }
    }

    /* PRIORIDADE 2: Ajudar o Grupo (se não tiver um mestre para ajudar) */
    else if (GROUP(ch)) {
        struct char_data *member;
        struct iterator_data iterator;

        member = (struct char_data *)merge_iterator(&iterator, GROUP(ch)->members);
        while(member) {
            if (ch != member && IN_ROOM(ch) == IN_ROOM(member) && FIGHTING(member)) {
                /* Lógica de "priorizar a maior ameaça" */
                if (GET_LEVEL(FIGHTING(member)) > max_threat_level) {
                    max_threat_level = GET_LEVEL(FIGHTING(member));
                    ally_in_trouble = member;
                    target_to_attack = FIGHTING(member);
                }
            }
            member = (struct char_data *)next_in_list(&iterator);
        }
    }

    /* PRIORIDADE 3: Ajudar outros NPCs (se tiver a flag MOB_HELPER) */
    else if (MOB_FLAGGED(ch, MOB_HELPER)) {
        struct char_data *vict;
        for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
            if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
                continue;
            if (IS_NPC(FIGHTING(vict))) /* Não ajuda mobs que lutam contra outros mobs */
                continue;

            ally_in_trouble = vict;
            target_to_attack = FIGHTING(vict);
            break; /* Ajuda o primeiro que encontrar. */
        }
    }

    /* Se encontrou alguém para ajudar, entra em combate. */
    if (ally_in_trouble && target_to_attack) {
        act("$n vê que $N está em apuros e corre para ajudar!", FALSE, ch, 0, ally_in_trouble, TO_NOTVICT);
        hit(ch, target_to_attack, TYPE_UNDEFINED);
        return TRUE;
    }

    return FALSE;
}

/**
 * A IA tenta saquear o melhor item da sala, com base nas suas necessidades
 * e na sua genética. A sua "vontade" de procurar é aumentada se tiver
 * uma necessidade urgente (ex: sem arma, com vida baixa).
 * Retorna TRUE se uma ação de saque foi bem-sucedida.
 */
bool mob_try_and_loot(struct char_data *ch)
{
    /* A IA só age se houver itens na sala, se o mob tiver genética e não estiver em combate. */
    if (!world[IN_ROOM(ch)].contents || !ch->ai_data || FIGHTING(ch))
        return FALSE;

    /* 1. Calcula a tendência base do mob (genética + instinto). */
    int base_tendency = MOB_FLAGGED(ch, MOB_SCAVENGER) ? 25 : 0;
    int genetic_tendency = GET_GENLOOT(ch);
    int effective_tendency = base_tendency + genetic_tendency;

    /* 2. Calcula o bónus com base nas necessidades urgentes. */
    int need_bonus = 0;
    if (GET_HIT(ch) < GET_MAX_HIT(ch) * 0.8)
        need_bonus += 15; /* Precisa de itens de cura. */

    if (GET_EQ(ch, WEAR_WIELD) == NULL)
        need_bonus += 25; /* Precisa desesperadamente de uma arma. */

    if (GET_EQ(ch, WEAR_BODY) == NULL)
        need_bonus += 15; /* Precisa de armadura. */

    if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_FIREWEAPON && !mob_has_ammo(ch))
        need_bonus += 50; /* Necessidade MUITO ALTA por munição! */

    /* 3. A chance final é a combinação de tudo, com um mínimo de curiosidade. */
    int final_chance = MAX(effective_tendency + need_bonus, 10);
    final_chance = MIN(final_chance, 95); /* Limita a 95% para não ser garantido. */

    if (rand_number(1, 100) <= final_chance) {
        int max_score = 0;
        struct obj_data *obj, *best_obj = NULL;

        /* A IA agora procura ativamente pelo item que melhor satisfaz a sua necessidade. */
        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
            int current_score = evaluate_item_for_mob(ch, obj);
            if (current_score > max_score) {
                best_obj = obj;
                max_score = current_score;
            }
        }

        if (best_obj != NULL) {
            /* Chama a função do jogo para pegar o item, garantindo todas as verificações. */
            if (perform_get_from_room(ch, best_obj)) {
                /* Aprendizagem Positiva: A decisão foi boa e bem-sucedida. */
                ch->ai_data->genetics.loot_tendency += 2;
                ch->ai_data->genetics.loot_tendency = MIN(ch->ai_data->genetics.loot_tendency, 100);
                return TRUE; /* Ação bem-sucedida, consome o turno. */
            }
        } else {
            /* Aprendizagem Negativa: A necessidade não foi satisfeita. */
            ch->ai_data->genetics.loot_tendency -= 1;
            ch->ai_data->genetics.loot_tendency = MAX(ch->ai_data->genetics.loot_tendency, 0);
        }
    }

    return FALSE; /* Nenhuma ação de saque foi executada. */
}

/**
 * A IA entra numa "sessão de equipamento", onde percorre o seu inventário
 * várias vezes para encontrar e vestir todos os upgrades possíveis.
 * Retorna TRUE se conseguiu equipar pelo menos um item.
 */
bool mob_try_and_upgrade(struct char_data *ch)
{
    /* A IA só age se tiver itens, genética e não estiver sob o controlo de um jogador. */
    if (!ch->carrying || !ch->ai_data || AFF_FLAGGED(ch, AFF_CHARM))
        return FALSE;

    const int CURIOSIDADE_MINIMA_EQUIP = 5;
    int final_chance = MAX(GET_GENEQUIP(ch), CURIOSIDADE_MINIMA_EQUIP);
    final_chance = MIN(final_chance, 90);

    /* Verifica se o mob tem a "vontade" de otimizar o seu equipamento neste pulso. */
    if (rand_number(1, 100) <= final_chance) {
        bool performed_an_upgrade_this_pulse = FALSE;
        bool keep_trying = TRUE;

        /* O loop 'while' garante que o mob continua a tentar equipar até estar otimizado. */
        while (keep_trying) {
            struct obj_data *obj, *best_possible_upgrade = NULL;
            int max_score_improvement = 0;
            int best_pos_to_equip = -1; /* Guarda a posição do melhor upgrade */

            /* Procura pelo melhor upgrade POSSÍVEL no estado atual. */
            for (obj = ch->carrying; obj; obj = obj->next_content) {
                int wear_pos = find_eq_pos(ch, obj, NULL);
                if (wear_pos == -1) continue;

                struct obj_data *equipped_item = GET_EQ(ch, wear_pos);
                bool can_perform_swap = TRUE;
                if (equipped_item != NULL) {
                    if (OBJ_FLAGGED(equipped_item, ITEM_NODROP) || IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
                        can_perform_swap = FALSE;
                    }
                }
                if (can_perform_swap) {
                    int score_improvement = evaluate_item_for_mob(ch, obj) - evaluate_item_for_mob(ch, equipped_item);
                    if (score_improvement > max_score_improvement) {
                        max_score_improvement = score_improvement;
                        best_possible_upgrade = obj;
                        best_pos_to_equip = wear_pos;
                    }
                }
            }

            /* Se encontrou um upgrade viável, executa-o e tenta de novo. */
            if (best_possible_upgrade != NULL) {
                struct obj_data *equipped_item = GET_EQ(ch, best_pos_to_equip);
                if (equipped_item != NULL) {
                    perform_remove(ch, best_pos_to_equip);
                }
                perform_wear(ch, best_possible_upgrade, best_pos_to_equip);
                performed_an_upgrade_this_pulse = TRUE;
                /* keep_trying continua TRUE para que ele re-avalie o inventário com o novo item equipado. */
            } else {
                /* Se não encontrou mais upgrades, para a sessão. */
                keep_trying = FALSE;
            }
        } /* Fim do loop 'while' */

        /* A aprendizagem acontece uma vez no final da sessão. */
        if (performed_an_upgrade_this_pulse) {
            ch->ai_data->genetics.equip_tendency += 2;
            ch->ai_data->genetics.equip_tendency = MIN(ch->ai_data->genetics.equip_tendency, 100);
        } else {
            ch->ai_data->genetics.equip_tendency -= 1;
            ch->ai_data->genetics.equip_tendency = MAX(ch->ai_data->genetics.equip_tendency, 0);
        }

        return TRUE; /* Retorna TRUE porque a IA "pensou" em se equipar, mesmo que não tenha feito upgrades. */
    }

    return FALSE; /* Nenhuma ação de equipamento foi executada. */
}
