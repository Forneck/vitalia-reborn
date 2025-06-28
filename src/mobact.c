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

    /* If the mob has no specproc, do the default actions */
    if (FIGHTING(ch) || !AWAKE(ch))
      continue;

    /**********************************************************************
     * NOVA IA DE ROTINA DIÁRIA PARA LOJISTAS
     * Esta lógica tem prioridade sobre os comportamentos genéticos.
     **********************************************************************/
    if (mob_index[GET_MOB_RNUM(ch)].func == shop_keeper) {
        int shop_nr = find_shop_by_keeper(GET_MOB_RNUM(ch));
        
        if (shop_nr != -1) {
            if (is_shop_open(shop_nr)) {
                /* --- HORÁRIO DE TRABALHO --- */
                /* Objetivo: Estar em QUALQUER uma das salas da loja. */
                if (!ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch)))) {
                    /* Está fora da loja, precisa de voltar para a sala principal! */
                    room_rnum primary_shop_room = real_room(SHOP_ROOM(shop_nr, 0));
                    if (primary_shop_room != NOWHERE && IN_ROOM(ch) != primary_shop_room) {
                        int direction = find_first_step(IN_ROOM(ch), primary_shop_room);
                        if (direction >= 0) {
                            perform_move(ch, direction, 1);
                        }
                    }
                }
                /* Se já está numa sala válida da loja, não faz mais nada. */
                continue; /* O turno do lojista dedicado ao trabalho acaba aqui. */
            }
        }
    }

    /* Se o código chegou aqui, ou o mob não é um lojista, ou a sua loja está fechada. */
    /* Portanto, ele está livre para executar a sua IA Genética normal. */
    

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
    

    /**********************************************************************
     * NOVA LÓGICA DE GRUPO: Seguir o Líder (Prioridade de Movimento)
     **********************************************************************/
    if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) != ch) {
        struct char_data *leader = GROUP_LEADER(GROUP(ch));

        /* O líder está em outra sala? Se sim, a prioridade é segui-lo. */
        if (IN_ROOM(ch) != IN_ROOM(leader)) {
            int direction = find_first_step(IN_ROOM(ch), IN_ROOM(leader)); /* Pathfinding */

            if (direction >= 0) {
                /* Chama perform_move, que já tem as nossas verificações de terreno. */
                perform_move(ch, direction, 1);
                continue; /* Ação do pulso: seguiu o líder. Fim do turno. */
            }
        }
    }
    
    if (GROUP(ch) && !FIGHTING(ch)) { /* Se está num grupo e não está a lutar */
        struct char_data *member;
        struct iterator_data iterator;
        struct char_data *target_to_assist = NULL;
        int max_threat_level = 0;

        member = (struct char_data *)merge_iterator(&iterator, GROUP(ch)->members);
        while(member) {
            /* Verifica se um companheiro na mesma sala está a lutar. */
            if (ch != member && IN_ROOM(ch) == IN_ROOM(member) && FIGHTING(member)) {
                
                /* Lógica de "priorizar a maior ameaça": ataca o inimigo de nível mais alto. */
                if (GET_LEVEL(FIGHTING(member)) > max_threat_level) {
                    max_threat_level = GET_LEVEL(FIGHTING(member));
                    target_to_assist = member; /* Vamos ajudar este membro. */
                }
            }
            member = (struct char_data *)next_in_list(&iterator);
        }
        
        if (target_to_assist) {
            act("$n vê que $N está em apuros e corre para ajudar!", FALSE, ch, 0, target_to_assist, TO_NOTVICT);
            act("Você vê que $N está em apuros e corre para ajudar!", FALSE, ch, 0, target_to_assist, TO_CHAR);
            hit(ch, FIGHTING(target_to_assist), TYPE_UNDEFINED);
            continue; /* Ação do pulso: entrou em combate. Fim do turno. */
        }
    }
    
    /*************************************************************************
     * Lógica de Genética v2: SAQUEAR (Loot) com "Bónus de Necessidade"
     * PRECISA VIR ANTES DE EQUIPAR
     *************************************************************************/
    if (world[IN_ROOM(ch)].contents && ch->genetics && !FIGHTING(ch)) {

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
            best_obj = NULL;

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
                    ch->genetics->loot_tendency += 2;
                    ch->genetics->loot_tendency = MIN(ch->genetics->loot_tendency, 100);                                                             }
            } else {                                                                                                                                 /* Aprendizagem Negativa: A necessidade não foi satisfeita. */
                ch->genetics->loot_tendency -= 1;
                ch->genetics->loot_tendency = MAX(ch->genetics->loot_tendency, 0);
            }
        }
    }

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

    /**************************************************************************
     * Lógica de Genética: A "SESSÃO DE EQUIPAMENTO"                          *
     * O mob agora equipa todos os upgrades possíveis de uma só vez.          *
     **************************************************************************/
    if (ch->carrying && ch->genetics && !FIGHTING(ch) && !AFF_FLAGGED(ch, AFF_CHARM)) {
        
        const int CURIOSIDADE_MINIMA_EQUIP = 5;
        int final_chance = MAX(GET_GENEQUIP(ch), CURIOSIDADE_MINIMA_EQUIP);
        final_chance = MIN(final_chance, 90);

        if (rand_number(1, 100) <= final_chance) {
            bool performed_an_upgrade_this_pulse = FALSE;
            bool keep_trying = TRUE;

            /* O loop 'while' garante que o mob continua a tentar equipar até estar otimizado. */
            while (keep_trying) {
                struct obj_data *obj, *best_possible_upgrade = NULL;
                int max_score_improvement = 0;

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
                        }
                    }
                }

                /* Se encontrou um upgrade viável, executa-o e tenta de novo. */
                if (best_possible_upgrade != NULL) {
                    int wear_pos = find_eq_pos(ch, best_possible_upgrade, NULL);
                    if (wear_pos != -1) {
                        struct obj_data *equipped_item = GET_EQ(ch, wear_pos);
                        if (equipped_item != NULL) {
                            perform_remove(ch, wear_pos);
                        }
                        perform_wear(ch, best_possible_upgrade, wear_pos);
                        performed_an_upgrade_this_pulse = TRUE;
                        /* keep_trying continua TRUE para re-analisar o inventário. */
                    }
                } else {
                    /* Se não encontrou mais upgrades, para a sessão. */
                    keep_trying = FALSE;
                }
            } /* Fim do loop 'while' */

            /* A aprendizagem acontece uma vez no final da sessão. */
            if (performed_an_upgrade_this_pulse) {
                ch->genetics->equip_tendency += 2;
                if (ch->genetics->equip_tendency > 100) ch->genetics->equip_tendency = 100;
            } else {
                ch->genetics->equip_tendency -= 1;
                if (ch->genetics->equip_tendency < 0) ch->genetics->equip_tendency = 0;
            }
        }
    }
    
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
 * Verifica se um novo membro ('prospect') é compatível em nível com um
 * grupo já existente, respeitando a regra de 15 níveis de diferença
 * entre o novo mínimo e o novo máximo.
 */
bool is_level_compatible_with_group(struct char_data *prospect, struct group_data *group)
{
    if (!prospect || !group || !group->members) return FALSE;

    struct char_data *member;
    struct iterator_data iterator;

    /* Começa com os níveis do prospecto */
    int min_level = GET_LEVEL(prospect);
    int max_level = GET_LEVEL(prospect);

    /* Itera pelos membros existentes para encontrar o min/max atual do grupo */
    member = (struct char_data *)merge_iterator(&iterator, group->members);
    while(member) {
        if (GET_LEVEL(member) < min_level) min_level = GET_LEVEL(member);
        if (GET_LEVEL(member) > max_level) max_level = GET_LEVEL(member);
        member = (struct char_data *)next_in_list(&iterator);
    }

    /* Retorna TRUE se a nova diferença total for 15 ou menos. */
    return ((max_level - min_level) <= 15);
}


/**
 * Avalia todos os mobs solitários numa sala para encontrar o melhor candidato
 * a líder para um novo grupo, com base na regra do "líder mediano".
 * @param ch O mob que está a iniciar a verificação.
 * @return Um ponteiro para o melhor candidato a líder, ou NULL se nenhum grupo for viável.
 */
struct char_data *find_best_median_leader(struct char_data *ch)
{
    struct char_data *vict, *leader_candidate;
    int min_level = GET_LEVEL(ch), max_level = GET_LEVEL(ch);
    int count = 0;

    /* Aumentei o array para segurança, mas ajuste se necessário. */
    struct char_data *potential_members[51];

    /* 1. Reúne todos os potenciais membros (mobs solitários) na sala. */
    for (vict = world[IN_ROOM(ch)].people; vict && count < 50; vict = vict->next_in_room) {
        if (!IS_NPC(vict) || vict->master != NULL || GROUP(vict))
            continue;

        potential_members[count++] = vict;
        if (GET_LEVEL(vict) < min_level) min_level = GET_LEVEL(vict);
        if (GET_LEVEL(vict) > max_level) max_level = GET_LEVEL(vict);
    }
    potential_members[count] = NULL; /* Marca o fim da lista. */

    /* 2. Se a diferença de nível já for válida, o melhor líder é o de nível mais alto. */
    if ((max_level - min_level) <= 15) {
        struct char_data *best_leader = NULL;
        for (int i = 0; i < count; i++) {
            if (best_leader == NULL || GET_LEVEL(potential_members[i]) > GET_LEVEL(best_leader)) {
                best_leader = potential_members[i];
            }
        }
        return best_leader;
    }

    /* 3. Se a diferença for grande, procura por um "líder mediano". */
    struct char_data *best_median = NULL;
    for (int i = 0; i < count; i++) {
        leader_candidate = potential_members[i];

        if ((max_level - GET_LEVEL(leader_candidate)) <= 15 && (GET_LEVEL(leader_candidate) - min_level) <= 15) {
            /* Este candidato é um "líder mediano" válido. Ele é o melhor até agora? */
            if (best_median == NULL || GET_LEVEL(leader_candidate) > GET_LEVEL(best_median)) {
                best_median = leader_candidate;
            }
        }
    }

    /* Retorna o melhor líder mediano encontrado (pode ser NULL se nenhum for viável). */
    return best_median;
}

bool mob_handle_grouping(struct char_data *ch)
{
    if (GROUP(ch) || !ch->genetics)
        return FALSE;

    if (rand_number(1, 100) > MAX(GET_GENGROUP(ch), 5))
        return FALSE;

    struct char_data *vict, *target_leader = NULL;
    int max_group_size = 6;

    /* 1. Procura por um grupo existente para se juntar. */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
        if (GROUP(vict) && GROUP_LEADER(GROUP(vict)) == vict &&
            IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN) &&
            GROUP(vict)->members->iSize < max_group_size &&
            is_level_compatible_with_group(ch, GROUP(vict))) { /* <-- USA A NOVA FUNÇÃO */
            target_leader = vict;
            break;
        }
    }

    if (target_leader) {
        /* Lógica de Aceitação do Líder */
        int chance_aceitar = 100 - ((GROUP(target_leader)->members->iSize) * 15) + GET_GENGROUP(target_leader);
        if (rand_number(1, 120) <= chance_aceitar) {
            join_group(ch, GROUP(target_leader));
            act("$n junta-se ao grupo de $N.", TRUE, ch, 0, target_leader, TO_ROOM);
            return TRUE;
        }
    } else {
        /* 2. Não encontrou grupo. Verifica se é possível formar um novo. */
        struct char_data *leader_to_be = find_best_median_leader(ch); /* <-- USA A NOVA FUNÇÃO */

        if (leader_to_be != NULL && leader_to_be == ch) {
            /* O próprio mob 'ch' é o melhor candidato a líder, então ele cria o grupo. */
            struct group_data *new_group;
            CREATE(new_group, struct group_data, 1);
            new_group->members = create_list();
            SET_BIT(new_group->group_flags, GROUP_ANON);
            SET_BIT(new_group->group_flags, GROUP_OPEN);
            join_group(ch, new_group);
            act("$n parece estar a formar um grupo e à procura de companheiros.", TRUE, ch, 0, 0, TO_ROOM);
            return TRUE;
        }
        /* Se o melhor líder for outro, este mob irá esperar pelo turno desse outro mob. */
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
    if (!GROUP(leader) || GROUP_LEADER(GROUP(leader)) != leader || !leader->carrying || !leader->genetics) {
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
        leader->genetics->group_tendency += 5;
        leader->genetics->group_tendency = MIN(leader->genetics->group_tendency, 100);
        
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
        if (ch->genetics) {
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
            
            ch->genetics->roam_tendency += roam_change;
            ch->genetics->roam_tendency = MIN(MAX(ch->genetics->roam_tendency, 0), 100);
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * IA de exploração orientada a objetivos. O mob agora vagueia com um propósito.
 * VERSÃO FINAL COM TODAS AS NUANCES.
 * Retorna TRUE se uma ação de roam foi executada.
 */
bool mob_goal_oriented_roam(struct char_data *ch)
{
    if (ch->master != NULL || FIGHTING(ch))
        return FALSE;

    int direction = -1;
    bool has_goal = FALSE;

    /* 1. OBJETIVO PRIMÁRIO: Sentinelas e Lojistas voltam para o seu posto. */
    if (MOB_FLAGGED(ch, MOB_SENTINEL) && IN_ROOM(ch) != real_room(GET_LOADROOM(ch))) {
        direction = find_first_step(IN_ROOM(ch), real_room(GET_LOADROOM(ch)));
        has_goal = TRUE;
    }

    /* 2. CÁLCULO DA MOTIVAÇÃO PARA EXPLORAR */
    if (!has_goal) {
        int base_roam = MOB_FLAGGED(ch, MOB_SENTINEL) ? 1 : 25;
        int need_bonus = 0;
        if (GET_EQ(ch, WEAR_WIELD) == NULL) need_bonus += 20;
        if (!GROUP(ch)) need_bonus += 10;

        int final_chance = MIN(base_roam + GET_GENROAM(ch) + need_bonus, 90);

        if (rand_number(1, 100) <= final_chance) {
            direction = rand_number(0, DIR_COUNT - 1);
            has_goal = TRUE;
        }
    }

    /* 3. EXECUÇÃO DA AÇÃO SE HOUVER UM OBJETIVO */
    if (has_goal && direction != -1) {
        struct room_direction_data *exit;
        room_rnum to_room;

        if ((exit = EXIT(ch, direction)) && (to_room = exit->to_room) <= top_of_world) {
            
            /* 3a. GESTÃO DE ESTADO (Pré-requisitos da Ação) */
            if (GET_POS(ch) < POS_STANDING) {
                do_stand(ch, "", 0, 0); 
            }
            if (AFF_FLAGGED(ch, AFF_FLYING) && ROOM_FLAGGED(to_room, ROOM_NO_FLY)) {
                stop_flying(ch); 
            }
            if (!AFF_FLAGGED(ch, AFF_FLYING) && world[to_room].sector_type == SECT_CLIMBING) {
              start_flying(ch);
            }

            /* 3b. RESOLUÇÃO DE OBSTÁCULOS (Lógica Corrigida e Segura) */

            if (IS_SET(exit->exit_info, EX_ISDOOR) && IS_SET(exit->exit_info, EX_CLOSED)) {
                    if (IS_SET(exit->exit_info, EX_LOCKED) && has_key(ch, exit->key)) {
                        do_doorcmd(ch, NULL, direction, SCMD_UNLOCK);
                    }
                    /* A ação é encadeada: se agora estiver destrancada, tenta abrir. */
                    if (!IS_SET(exit->exit_info, EX_LOCKED)) {
                        do_doorcmd(ch, NULL, direction, SCMD_OPEN);
                    }
                /* Se a porta não tem nome ou está emperrada, a IA não pode passar e não faz nada. */
		else {
			return FALSE;
		}

            /* 3c. MOVIMENTO FINAL E APRENDIZAGEM */
            /* Se chegou aqui, a porta já está aberta. */
            	if ( (IS_SET(exit->exit_info, EX_HIDDEN) && rand_number(1, 20) > GET_WIS(ch)) ||
                 (world[to_room].sector_type == SECT_UNDERWATER && !has_scuba(ch)) ||
                  ROOM_FLAGGED(to_room, ROOM_NOMOB) ) {
                 return FALSE; /* Caminho é impossível. */
               }
             
            if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && (world[to_room].zone != world[IN_ROOM(ch)].zone) && (rand_number(1, 100) > 5)) {
                return TRUE; /* Hesitou e gastou o turno. */
            }
            
                /************************************************************/
                /* LÓGICA DE DECISÃO TÁTICA: Fechar a porta atrás de si?    */
                /************************************************************/
                bool should_close_behind = FALSE;
                /* Apenas mobs inteligentes consideram esta tática. */
                if (GET_INT(ch) > 12 && exit->keyword != NULL && !IS_SET(exit->exit_info, EX_DNCLOSE)) {
                    int chance = 5; /* 5% de chance base para qualquer mob inteligente. */

                    if (ch->genetics) {
                        /* Fator Medo: Um mob medroso cobre a sua retirada. */
                        /* Um mob com 100 de wimpy ganha +25% de chance. */
                        chance += (GET_GENWIMPY(ch) / 4);

                        /* Fator Territorialismo: Mobs que vagueiam pouco são mais cuidadosos. */
                        /* Um mob com 0 de roam ganha +12% de chance. */
                        chance += (100 - GET_GENROAM(ch)) / 8;
                    }

                    if (rand_number(1, 100) <= chance) {
                        should_close_behind = TRUE;
                    }
                }
               
                /* Ação de mover-se, passando a decisão tática como parâmetro. */
                return perform_move_IA(ch, direction, should_close_behind, IN_ROOM(ch));
            }
      }
    }
    return FALSE; /* Nenhuma ação de roam foi executada. */

}
