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

int get_item_apply_score(struct char_data *ch, struct obj_data *obj);
int evaluate_item_for_mob(struct char_data *ch, struct obj_data *obj);
bool mob_has_ammo(struct char_data *ch);
struct char_data *find_best_median_leader(struct char_data *ch);
bool mob_handle_grouping(struct char_data *ch);
bool mob_leader_evaluates_gear(struct char_data *leader);

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
    
    /* hunt a victim, if applicable */
    hunt_victim(ch);
    

    if (mob_handle_grouping(ch)) {
        continue; /* Ação do pulso foi usada para agrupar, fim do turno. */
    }

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
	  
    /****************************************************************************
     * Lógica de Genética: VAGUEAR (Roam) - VERSÃO FINAL E CORRIGIDA
     * Substitui o bloco "Mob Movement" original.
     ****************************************************************************/
    if (ch->master == NULL) {
        int CURIOSIDADE_EXPLORAR = 10;
	int effective_roam_tendency = 0;
	int base_roam_instinct = 25;
        int final_chance = 0;
        if (MOB_FLAGGED(ch, MOB_SENTINEL)) {
            base_roam_instinct = 1;
            CURIOSIDADE_EXPLORAR = 0;
        }
        
	effective_roam_tendency = base_roam_instinct + GET_GENROAM(ch);
        final_chance = MAX(effective_roam_tendency, CURIOSIDADE_EXPLORAR);
        final_chance = MIN(final_chance, 90);
        if (rand_number(1, 100) <= final_chance) {
                /* Prossegue com a lógica de movimento. */
                int was_in = IN_ROOM(ch);
                door = rand_number(0, DIR_COUNT - 1);
                struct room_direction_data *exit;
                room_rnum to_room;

                if ((exit = EXIT(ch, door)) != NULL && (to_room = exit->to_room) != NOWHERE) {
                    	
                    /* 2. Lógica de Voo: Aterra/Descola conforme a necessidade antes de tentar andar no mesmo pulso. */
                    if (AFF_FLAGGED(ch, AFF_FLYING) && ROOM_FLAGGED(to_room, ROOM_NO_FLY)) {
                        stop_flying(ch);
                    } else if (!AFF_FLAGGED(ch, AFF_FLYING) && world[to_room].sector_type == SECT_CLIMBING) {
                        start_flying(ch);
                    }
		 /* 3. PRÉ-REQUISITO DE OBSTÁCULOS: A IA "olha" antes de ir. */
                if ( (IS_SET(exit->exit_info, EX_HIDDEN) && rand_number(1, 20) > GET_WIS(ch)) ||
                     (IS_SET(exit->exit_info, EX_DNOPEN)) ||
                     (world[to_room].sector_type == SECT_UNDERWATER && !has_scuba(ch)) ||
                      ROOM_FLAGGED(to_room, ROOM_NOMOB) ) {
                     /* O caminho é impossível, a IA desiste por este pulso. */
                } else {
                    /* Se o caminho é possível, lida com portas. */
                 if (IS_SET(exit->exit_info, EX_CLOSED)) {
                    /* Se a porta estiver trancada e o mob tiver a chave... */
                    if (IS_SET(exit->exit_info, EX_LOCKED) && has_key(ch, exit->key)) {
                        /* ...ele usa o comando para destrancar. */
                        do_doorcmd(ch, NULL, door, SCMD_UNLOCK);
                    }
                    
                    /* Se a porta agora estiver destrancada (ou nunca esteve), ele a abre. */
                    if (!IS_SET(exit->exit_info, EX_LOCKED) && !IS_SET(exit->exit_info, EX_DNOPEN)) {
                        do_doorcmd(ch, NULL, door, SCMD_OPEN);
                    }
                }
                
                /* A IA só prossegue se, depois das suas ações, a porta estiver aberta. */
                if (!IS_SET(exit->exit_info, EX_CLOSED)) {
                    
                    /* --- LÓGICA DE FECHAR A PORTA ATRÁS DE SI --- */
                    bool should_close_behind = (GET_INT(ch) > 12 && rand_number(1,100) <= 25); /* Ex: 25% de chance para mobs inteligentes */

                    /* Guarda a porta de retorno. */
                    int back_door = rev_dir[door];
                    
                    /* Ação de mover e aprender (já existente) */
                    if (perform_move(ch, door, 1)) {
                        /* Se o mob se moveu e decidiu fechar a porta, ele o faz. */
                        if (should_close_behind && EXIT(ch, back_door) && EXIT(ch, back_door)->to_room == was_in) {
                             /* Verifica se a porta pode ser fechada. */
                             if (!IS_SET(EXIT(ch, back_door)->exit_info, EX_DNCLOSE)) {
                                do_doorcmd(ch, NULL, back_door, SCMD_CLOSE);
                             }
                        }
                        /* 5. PRÉ-REQUISITO PSICOLÓGICO: Medo de sair da zona. */
                        if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && (world[to_room].zone != world[IN_ROOM(ch)].zone) && (rand_number(1, 100) > 5)) {
                            /* Hesitou. Fim do turno. */
                        } else {
                            /* 6. AÇÃO FINAL E APRENDIZAGEM */
                               if (perform_move(ch, door, 1)) {
                                   /* 4. Lógica de Aprendizagem Pós-Movimento */
                                   if (ch->genetics) {
                                       int roam_change = 0;
                                       int current_sect = world[IN_ROOM(ch)].sector_type;
                                       if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) || current_sect == SECT_LAVA || current_sect == SECT_QUICKSAND) {
                                           roam_change = -20;
                                       } else if (current_sect == SECT_ICE && GET_POS(ch) < POS_STANDING) {
                                           roam_change = -5;
					   /* Mob caiu ao andar, tem que andar com mais cuidado. Também precisa levantar para a proxima ação */
					   do_stand(ch, "", 0, 0);
                                       } else {
                                           roam_change = 1;
                                       }
                                       ch->genetics->roam_tendency += roam_change;
                                       if (ch->genetics->roam_tendency < 0) ch->genetics->roam_tendency = 0;
                                       if (ch->genetics->roam_tendency > 100) ch->genetics->roam_tendency = 100;
                                   }
                               }
                           }
                        }
                    }
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
    if (obj == NULL) {
	    return 0;
    }

    int score = 0;

    /* --- FILTROS INICIAIS --- */
    if (!CAN_GET_OBJ(ch, obj) || !CAN_WEAR(obj, ITEM_WEAR_TAKE)) return 0;
    if ((IS_EVIL(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_EVIL)) ||
        (IS_GOOD(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_GOOD)) ||
        (IS_NEUTRAL(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL))) return 0;

    /* --- AVALIAÇÃO POR TIPO DE ITEM --- */
    switch (GET_OBJ_TYPE(obj)) {

        case ITEM_WEAPON:
        case ITEM_FIREWEAPON: {
            struct obj_data *current_weapon = GET_EQ(ch, WEAR_WIELD);
            int new_w_stats_score = get_item_apply_score(ch, obj);

            if (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON) {
                if (mob_has_ammo(ch)) {
                    score = 150 + new_w_stats_score;
                } else {
                    score = 50 + new_w_stats_score;
                }
            } else {
                float new_w_dam = (float)(GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1)) / 2.0;
                if (current_weapon == NULL || GET_OBJ_TYPE(current_weapon) == ITEM_FIREWEAPON) {
                    score = 100 + new_w_stats_score;
                } else {
                    float old_w_dam = (float)(GET_OBJ_VAL(current_weapon, 1) * (GET_OBJ_VAL(current_weapon, 2) + 1)) / 2.0;
                    int old_w_stats_score = get_item_apply_score(ch, current_weapon);
                    int total_improvement = ((int)(new_w_dam - old_w_dam) * 10) + (new_w_stats_score - old_w_stats_score);
                    if (total_improvement > 0) score = total_improvement;
                }
            }
            break;
        }

        case ITEM_ARMOR:
        case ITEM_WINGS:{
            int wear_pos = find_eq_pos(ch, obj, NULL);
            if (wear_pos != -1) {
                struct obj_data *current_armor = GET_EQ(ch, wear_pos);
                int new_armor_stats_score = get_item_apply_score(ch, obj) - (GET_OBJ_VAL(obj, 0) * 5);

                if (current_armor == NULL) {
                    score = 50 + new_armor_stats_score;
                } else {
                    int old_armor_stats_score = get_item_apply_score(ch, current_armor) - (GET_OBJ_VAL(current_armor, 0) * 5);
                    if (new_armor_stats_score > old_armor_stats_score) {
                        score = new_armor_stats_score - old_armor_stats_score;
                    }
                }
            }
            break;
        }

        case ITEM_LIGHT: {
            bool has_light = FALSE;
            if (IS_DARK(IN_ROOM(ch)) && !IS_AFFECTED(ch, AFF_INFRAVISION)) {
                int i;
                for (i = 0; i < NUM_WEARS; i++) {
                    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT) {
                        has_light = TRUE;
                        break;
                    }
                }
                if (!has_light) score = 75;
            }
            break;
        }

        case ITEM_POTION:
        case ITEM_SCROLL:
        case ITEM_WAND:
        case ITEM_STAFF:
            score = GET_OBJ_VAL(obj, 0) * 2;
            break;

        case ITEM_KEY:
            score = 30;
            break;

        case ITEM_BOAT: {
            bool near_water = FALSE;
            int door;
            room_rnum adjacent_room;

            for (door = 0; door < DIR_COUNT; door++) {
                if (world[IN_ROOM(ch)].dir_option[door] &&
                    (adjacent_room = world[IN_ROOM(ch)].dir_option[door]->to_room) != NOWHERE) {

                    if (world[adjacent_room].sector_type == SECT_WATER_SWIM ||
                        world[adjacent_room].sector_type == SECT_WATER_NOSWIM) {
                        near_water = TRUE;
                        break;
                    }
                }
            }
            if (near_water) {
                score = 200;
            } else {
                score = 5;
            }
            break;
        }

        case ITEM_CONTAINER:
            score = 20;
            break;

        case ITEM_FOOD:
            if (GET_COND(ch, HUNGER) >= 0 && GET_COND(ch, HUNGER) < 10) {
                 score = 40;
            }
            break;
	
	case ITEM_DRINKCON:
	    if (GET_COND(ch, THIRST) >= 0 && GET_COND(ch, THIRST) < 10) {
		    score = 40;
	    }
            break;

        default:
            score = GET_OBJ_COST(obj) / 100;
            break;
    }
    return score;
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
