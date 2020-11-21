/**************************************************************************
*  File: act.other.c                                       Part of tbaMUD *
*  Usage: Miscellaneous player-level commands.                             *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h"				/* for has_mail() */
#include "shop.h"
#include "quest.h"
#include "modify.h"
#include "./include/floatfann.h"

/* Local defined utility functions */
/* do_group utility functions */
static void print_group(struct char_data *ch);
static void display_group_list(struct char_data *ch);

static int can_elevate(struct char_data *ch);
static int can_rebegin(struct char_data *ch);
void check_thief(struct char_data *ch, struct char_data *vict);


ACMD(do_quit)
{
  char buf[128];
  time_t ct = time(0);
  struct tm *t = localtime(&ct);
  
	if (IS_NPC(ch) || !ch->desc)
		return;

	if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
		send_to_char(ch, "Você tem que escrever o comando inteiro -- não menos do que isso!\r\n");
	else if (GET_POS(ch) == POS_FIGHTING)
		send_to_char(ch, "Sem chances!  Você está lutando pela sua vida!\r\n");
	else if (GET_POS(ch) < POS_STUNNED)
	{
		send_to_char(ch, "Você morreu antes do tempo...\r\n");
		die(ch, NULL);
	}
	else
	{
		act("$n deixou o jogo.", TRUE, ch, 0, 0, TO_ROOM);
		mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.",
			   GET_NAME(ch));

		if (GET_QUEST_TIME(ch) != -1)
			quest_timeout(ch);


sprintf(buf, "Tenha um%s! :-)\r\n\r\n",
	  (t->tm_hour < 6) ? "a boa madrugada" :
	  (t->tm_hour < 12) ? " bom dia" :
	  (t->tm_hour < 18) ? "a boa tarde" : "a boa noite");

		send_to_char(ch, "%s", buf);

		/* We used to check here for duping attempts, but we may as well do it 
		   right in extract_char(), since there is no check if a player rents
		   out and it can leave them in an equally screwy situation. */

		if (CONFIG_FREE_RENT)
			Crash_rentsave(ch, 0);

		GET_LOADROOM(ch) = GET_ROOM_VNUM(GET_HOMETOWN(ch));

		/* Stop snooping so you can't see passwords during deletion or change. 
		 */
		if (ch->desc->snoop_by)
		{
			write_to_output(ch->desc->snoop_by, "A sua vitima não está mais entre nós.\r\n");
			ch->desc->snoop_by->snooping = NULL;
			ch->desc->snoop_by = NULL;
		}

		extract_char(ch);		/* Char is saved before extracting. */
	}
}

ACMD(do_save)
{
	if (IS_NPC(ch) || !ch->desc)
		return;

	send_to_char(ch, "Salvando atalhos (alias)\r\n");
	send_to_char(ch, "Salvando personagem %s.\r\n", GET_NAME(ch));
	save_char(ch);
	 send_to_char(ch, "Salvando objetos.\r\n");
	Crash_crashsave(ch);
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH)){
	send_to_char(ch, "Salvando casa.\r\n");	House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
		
}
	GET_LOADROOM(ch) =  GET_ROOM_VNUM(IN_ROOM(ch));
}

/* Generic function for commands which are normally overridden by special
   procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
	send_to_char(ch, "Desculpe, mas você não pode fazer isso aqui!\r\n");
}

ACMD(do_sneak)
{
	struct affected_type af;
	byte percent;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK))
	{
		send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
		return;
	}
	send_to_char(ch, "Ok, você vai tentar mover-se silenciosamente por um tempo.\r\n");
	if (AFF_FLAGGED(ch, AFF_SNEAK))
		affect_from_char(ch, SKILL_SNEAK);

	percent = rand_number(1, 101);	/* 101% is a complete failure */

	if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
		return;

	new_affect(&af);
	af.spell = SKILL_SNEAK;
	af.duration = MIN(GET_LEVEL(ch), 24);
	SET_BIT_AR(af.bitvector, AFF_SNEAK);
	affect_to_char(ch, &af);
}

ACMD(do_hide)
{
	byte percent;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE))
	{
		send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
		return;
	}

	send_to_char(ch, "Você tenta se esconder.\r\n");

	if (AFF_FLAGGED(ch, AFF_HIDE))
		REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

	percent = rand_number(1, 101);	/* 101% is a complete failure */

	if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide)
		return;

	SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
}

ACMD(do_steal)
{
	struct char_data *vict;
	struct obj_data *obj;
	char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
	int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL))
	{
		send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
	{
		send_to_char(ch, "Você sente muita paz neste lugar para roubar alguma coisa...\r\n");
		return;
	}

	two_arguments(argument, obj_name, vict_name);

	if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char(ch, "Roubar o quê de quem?\r\n");
		return;
	}
	else if (vict == ch)
	{
		send_to_char(ch, "Pare, isto é muito estúpido!\r\n");
		return;
	}
	 if (IS_DEAD(ch)) {
    act("Você não pode roubar d$L, você está mort$r!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  } else if (IS_DEAD(vict)) {
    act("Suas mãos passam por $N... $L é apenas um espírito...", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

	/* 101% is a complete failure */
	percent = rand_number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

	if (GET_POS(vict) < POS_SLEEPING)
		percent = -1;			/* ALWAYS SUCCESS, unless heavy object. */

	if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
		pcsteal = 1;

	if (!AWAKE(vict))			/* Easier to steal from sleeping people. */
		percent -= 50;

	/* No stealing if not allowed. If it is no stealing from Imm's or
	   Shopkeepers. */
	if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal || GET_MOB_SPEC(vict) == shop_keeper || PLR_FLAGGED(vict, PLR_TRNS))
		percent = 101;			/* Failure */

	if (str_cmp(obj_name, "moedas") && str_cmp(obj_name, "dinheiro"))
	{

		if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying)))
		{

			for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
				if (GET_EQ(vict, eq_pos) &&
					(isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
					CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos)))
				{
					obj = GET_EQ(vict, eq_pos);
					break;
				}
			if (!obj)
			{
				act("$L não tem este objeto.", FALSE, ch, 0, vict, TO_CHAR);
				return;
			}
			else
			{					/* It is equipment */
				if ((GET_POS(vict) > POS_STUNNED))
				{
					send_to_char(ch, "Roubar equipamento agora?  Impossível!\r\n");
					return;
				}
				else
				{
					if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj))
					{
						send_to_char(ch, "Impossível!\r\n");
						return;
					}
					act("Você desequipa $p e rouba de $N.", FALSE, ch, obj, 0, TO_CHAR);
					act("$n rouba $p de $N.", FALSE, ch, obj, vict, TO_NOTVICT);
					obj_to_char(unequip_char(vict, eq_pos), ch);
				}
			}
		}
		else
		{						/* obj found in inventory */

			percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

			if (percent > GET_SKILL(ch, SKILL_STEAL))
			{
				ohoh = TRUE;
				send_to_char(ch, "Opa..\r\n");
				act("$n tentou roubar algo de você!", FALSE, ch, 0, vict, TO_VICT);
				act("$n tentou roubar algo de $N.", TRUE, ch, 0, vict, TO_NOTVICT);
			}
			else
			{					/* Steal the item */
				if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))
				{
					if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj))
					{
						send_to_char(ch, "Impossível!\r\n");
						return;
					}
					if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch))
					{
						obj_from_char(obj);
						obj_to_char(obj, ch);
						send_to_char(ch, "Você pegou!\r\n");
					}
				}
				else
					send_to_char(ch, "Você não pode carregar tanto peso.\r\n");
			}
		}
	}
	else
	{							/* Steal some coins */
		if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL)))
		{
			ohoh = TRUE;
			send_to_char(ch, "Opa..\r\n");
			act("Você descobre que $n estava com as mãos em sua carteira.", FALSE, ch, 0, vict, TO_VICT);
			act("$n tentou roubar dinheiro de $N.", TRUE, ch, 0, vict, TO_NOTVICT);
		}
		else
		{
			/* Steal some gold coins */
			gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
			gold = MIN(1782, gold);
			if (gold > 0)
			{
				increase_gold(ch, gold);
				decrease_gold(vict, gold);
				if (gold > 1)
					send_to_char(ch, "Feito!  Você conseguiu %d moedas.\r\n", gold);
				else
					send_to_char(ch, "Você conseguiu apenas uma única moeda.\r\n");
			}
			else
			{
				send_to_char(ch, "Você não conseguiu roubar nada...\r\n");
			}
		}
	}

	if (ohoh && IS_NPC(vict) && AWAKE(vict))
		hit(vict, ch, TYPE_UNDEFINED);
		
		 if (!IS_NPC(ch) && !IS_NPC(vict))
    SET_BIT_AR(PLR_FLAGS(ch), PLR_HTHIEF);
    
     if (ohoh && CONFIG_PT_ALLOWED)		/* -- jr - 03/07/99 */
    check_thief(ch, vict);
}

void check_thief(struct char_data *ch, struct char_data *vict)
{
  if (!IS_NPC(ch) && !IS_NPC(vict) && (ch != vict) &&
      !PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF)) {
    if (!PLR_FLAGGED(ch, PLR_THIEF)) {
      SET_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
      log1( MAX(LVL_IMMORT, GET_INVIS_LEV(ch)),
	  "PC Thief bit set on %s while trying to steal %s.",
	  GET_NAME(ch), GET_NAME(vict));
      send_to_char(ch,"Agora você é um JOGADOR LADRÃO, que pena...\r\n");
    } else {
      log1( MAX(LVL_IMMORT, GET_INVIS_LEV(ch)),
	  "PC Thief %s trying to steal %s.",
	  GET_NAME(ch), GET_NAME(vict));
    }
  }
}

ACMD(do_practice)
{
	char arg[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (*arg)
		send_to_char(ch, "Você só pode praticar na presença de seu mestre.\r\n");
	else
		list_skills(ch);
}

ACMD(do_visible)
{
	if (GET_LEVEL(ch) >= LVL_IMMORT)
	{
	  if (PLR_FLAGGED(ch, PLR_TRNS))
      send_to_char(ch, "Você não pode ficar totalmente visível.\r\n");
    else
		perform_immort_vis(ch);
		return;
	}

	if AFF_FLAGGED
		(ch, AFF_INVISIBLE)
	{
		appear(ch);
		send_to_char(ch, "Você encerra a magia da invisibilidade.\r\n");
	}
	else
		send_to_char(ch, "Você já está visível.\r\n");
}

ACMD(do_title)
{
	skip_spaces(&argument);
	delete_doubledollar(argument);
	parse_at(argument);

	if (IS_NPC(ch))
		send_to_char(ch, "Seu título já está ótimo... deixe assim.\r\n");
	else if (PLR_FLAGGED(ch, PLR_NOTITLE | PLR_JAILED))
		send_to_char(ch, "Você não pode mudar seu título -- você deve ter abusado!\r\n");
	else if (strstr(argument, "(") || strstr(argument, ")")  || strstr(argument, "[") || strstr(argument, "]") || strstr(argument, "<") || strstr(argument, ">") || strstr(argument, "{") || strstr(argument, "}"))
		send_to_char(ch, "Títulos não podem conter os caracteres (, ), [, ], <, >, { ou }.\r\n");
	else if (strlen(argument) > MAX_TITLE_LENGTH)
		send_to_char(ch, "Sinto muito, os títulos não podem ser maiores que %d letras.\r\n",
					 MAX_TITLE_LENGTH);
	else
	{
		set_title(ch, argument);
		send_to_char(ch, "Pronto, você agora é %s%s%s.\r\n", GET_NAME(ch), *GET_TITLE(ch) ? " " : "",
					 GET_TITLE(ch));
	}
}

static void print_group(struct char_data *ch)
{
	struct char_data *k;

	send_to_char(ch, "\tWSeu grupo consiste de:\tn\r\n");

	while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
		send_to_char(ch, "%-*s: %s[%4d/%-4d]H [%4d/%-4d]M [%4d/%-4d]V%s\r\n",
					 count_color_chars(GET_NAME(k)) + 22, GET_NAME(k),
					 GROUP_LEADER(GROUP(ch)) == k ? CBGRN(ch, C_NRM) : CCGRN(ch, C_NRM),
					 GET_HIT(k), GET_MAX_HIT(k),
					 GET_MANA(k), GET_MAX_MANA(k), GET_MOVE(k), GET_MAX_MOVE(k), CCNRM(ch, C_NRM));
}

static void display_group_list(struct char_data *ch)
{
	struct group_data *group;
	int count = 0;

	if (group_list->iSize)
	{
		send_to_char(ch, "#   Grupo Líder      # de Membros    Na Área\r\n"
					 "---------------------------------------------------\r\n");

		while ((group = (struct group_data *)simple_list(group_list)) != NULL)
		{
			if (IS_SET(GROUP_FLAGS(group), GROUP_NPC))
				continue;
			if (GROUP_LEADER(group) && !IS_SET(GROUP_FLAGS(group), GROUP_ANON))
				send_to_char(ch, "%-2d) %s%-12s     %-2d              %s%s\r\n",
							 ++count,
							 IS_SET(GROUP_FLAGS(group), GROUP_OPEN) ? CCGRN(ch, C_NRM) : CCRED(ch,
																							   C_NRM),
							 GET_NAME(GROUP_LEADER(group)), group->members->iSize,
							 zone_table[world[IN_ROOM(GROUP_LEADER(group))].zone].name, CCNRM(ch,
																							  C_NRM));
			else
				send_to_char(ch, "%-2d) Oculto\r\n", ++count);

		}
	}
	if (count)
		send_to_char(ch, "\r\n"
					 "%sProcurando Membros%s\r\n"
					 "%sFechado%s\r\n",
					 CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	else
		send_to_char(ch, "\r\n" "Atualmente sem grupos formados.\r\n");
}

/* Vatiken's Group System: Version 1.1 */
ACMD(do_group)
{
	char buf[MAX_STRING_LENGTH];
	struct char_data *vict;

	argument = one_argument(argument, buf);

	if (!*buf)
	{
		if (GROUP(ch))
			print_group(ch);
		else
			send_to_char(ch,
						 "Fazer o quê com o grupo?\r\n");
		return;
	}

	if (is_abbrev(buf, "new") || is_abbrev(buf, "novo"))
	{
		if (GROUP(ch))
			send_to_char(ch, "Você já está em um grupo.\r\n");
		else
			create_group(ch);
	}
	else if (is_abbrev(buf, "list"))
		display_group_list(ch);
	else if (is_abbrev(buf, "join") ||is_abbrev(buf, "entrar"))
	{
		skip_spaces(&argument);
		if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
		{
			send_to_char(ch, "Entrar no grupo de quem?\r\n");
			return;
		}
		else if (vict == ch)
		{
			send_to_char(ch, "Este seria um grupo muito solitário.\r\n");
			return;
		}
		else if (GROUP(ch))
		{
			send_to_char(ch, "Mas vocé já faz parte de um grupo.\r\n");
			return;
		}
		else if (!GROUP(vict))
		{
			act("$N não faz parte de nenhum grupo!", FALSE, ch, 0, vict, TO_CHAR);
			return;
		}
		else if (!IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN))
		{
			send_to_char(ch, "Este grupo não está aceitando novos membros.\r\n");
			return;
		}
		join_group(ch, GROUP(vict));
	}
	else if (is_abbrev(buf, "kick") || is_abbrev(buf, "expulsar") )
	{
		skip_spaces(&argument);
		if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
		{
			send_to_char(ch, "Expulsar quem do grupo?\r\n");
			return;
		}
		else if (vict == ch)
		{
			send_to_char(ch, "Existem jeitos mais fáceis de sair do grupo.\r\n");
			return;
		}
		else if (!GROUP(ch))
		{
			send_to_char(ch, "Mas você não faz parte de nenhum grupo!\r\n");
			return;
		}
		else if (GROUP_LEADER(GROUP(ch)) != ch)
		{
			send_to_char(ch, "Você não pode expulsar membros do seu grupo sem ser o líder.\r\n");
			return;
		}
		else if (GROUP(vict) != GROUP(ch))
		{
			act("$N não faz parte do seu grupo!", FALSE, ch, 0, vict, TO_CHAR);
			return;
		}
		send_to_char(ch, "%s não é mais membro de seu grupo.\r\n", GET_NAME(vict));
		send_to_char(vict, "Você foi chutad%s para fora do grupo.\r\n",OA(vict));
		leave_group(vict);
	}
	else if (is_abbrev(buf, "regroup") || is_abbrev(buf, "regrupar"))
	{
		if (!GROUP(ch))
		{
			send_to_char(ch, "Mas você não faz parte de nenhum grupo!\r\n");
			return;
		}
		vict = GROUP_LEADER(GROUP(ch));
		if (ch == vict)
		{
			send_to_char(ch, "Você é o lider e não pode reagrupar.\r\n");
		}
		else
		{
			leave_group(ch);
			join_group(ch, GROUP(vict));
		}
	}
	else if (is_abbrev(buf, "leave") || is_abbrev(buf, "sair"))
	{

		if (!GROUP(ch))
		{
			send_to_char(ch, "Mas você não faz parte de nenhum grupo!\r\n");
			return;
		}

		leave_group(ch);
	}
	else if (is_abbrev(buf, "option"))
	{
		skip_spaces(&argument);
		if (!GROUP(ch))
		{
			send_to_char(ch, "Mas você não faz parte de nenhum grupo!\r\n");
			return;
		}
		else if (GROUP_LEADER(GROUP(ch)) != ch)
		{
			send_to_char(ch, "Somente o líder pode ajustar as opçoes do grupo.\r\n");
			return;
		}
		if (is_abbrev(argument, "open") || is_abbrev(argument, "aberto"))
		{
			TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN);
			send_to_char(ch, "O grupo agora está %s para novos membros.\r\n",
						 IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN) ? "aberto" : "fechado");
		}
		else if (is_abbrev(argument, "anonymous") || is_abbrev(argument, "escondido"))
		{
			TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_ANON);
			send_to_char(ch, "A localização do grupo agora está %s para os outros jogadores.\r\n",
						 IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_ANON) ? "escondida" : "disponível");
		}
		else
			send_to_char(ch, "As opções de grupo são: Open (Aberto), Anonymous (Escondido).\r\n");
	}
	else
	{
		send_to_char(ch, "Você precisa especificar a opção do grupo.\r\n");
	}

}

ACMD(do_report)
{
	struct group_data *group;

	if ((group = GROUP(ch)) == NULL)
	{
		send_to_char(ch, "Mas você não é membro de um grupo!\r\n");
		return;
	}

	send_to_group(NULL, group, "%s relata: %d/%dHp, %d/%dMn, %d/%dMv,%d/10Ac\r\n",
				  GET_NAME(ch),
				  GET_HIT(ch), GET_MAX_HIT(ch),
				  GET_MANA(ch), GET_MAX_MANA(ch), GET_MOVE(ch), GET_MAX_MOVE(ch),compute_armor_class(ch));
}

ACMD(do_split)
{
	char buf[MAX_INPUT_LENGTH];
	int amount, num = 0, share, rest;
	size_t len;
	struct char_data *k;

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (is_number(buf))
	{
		amount = atoi(buf);
		if (amount <= 0)
		{
			send_to_char(ch, "Desculpe, mas você não pode fazer isso.\r\n");
			return;
		}
		if (amount > GET_GOLD(ch))
		{
			send_to_char(ch, "Você não parece ter tanto ouro para dividir.\r\n");
			return;
		}

		if (GROUP(ch))
			while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
				if (IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k))
					num++;

		if (num && GROUP(ch))
		{
			share = amount / num;
			rest = amount % num;
		}
		else
		{
			send_to_char(ch, "Você precisa pertencer a um grupo para poder dividir seu ouro.\r\n");
			return;
		}

		decrease_gold(ch, share * (num - 1));

		/* Abusing signed/unsigned to make sizeof work. */
		len = snprintf(buf, sizeof(buf), "%s divide %d coins; você recebe %d.\r\n",
					   GET_NAME(ch), amount, share);
		if (rest && len < sizeof(buf))
		{
			snprintf(buf + len, sizeof(buf) - len,
					 "%d moeda%s não %s, então %s ficou com o resto.\r\n", rest,
					 (rest == 1) ? "" : "s", (rest == 1) ? "pode ser dividida" : "puderam ser divididas", GET_NAME(ch));
		}

		while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
			if (k != ch && IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k))
			{
				increase_gold(k, share);
				send_to_char(k, "%s", buf);
			}

		send_to_char(ch, "Voce divide %d moedas entre %d membros -- %d moedas para cada um.\r\n",
					 amount, num, share);

		if (rest)
		{
			send_to_char(ch, "%d moeda%s não %s, então você ficou com o resto.\r\n",
						 rest, (rest == 1) ? "" : "s", (rest == 1) ? "pode ser dividida" : "puderam ser divididas");
			increase_gold(ch, rest);
		}
	}
	else
	{
		send_to_char(ch, "Quantas moedas voce deseja dividir com o resto do grupo?\r\n");
		return;
	}
}

ACMD(do_use)
{
	char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
	struct obj_data *mag_item;

	half_chop(argument, arg, buf);
	if (!*arg)
	{
		send_to_char(ch, "O quê você deseja %s?\r\n",  subcmd == SCMD_RECITE ? "recitar" :
	      subcmd == SCMD_QUAFF ? "tomar" : "usar");
		return;
	}
	mag_item = GET_EQ(ch, WEAR_HOLD);

	if (!mag_item || !isname(arg, mag_item->name))
	{
		switch (subcmd)
		{
		case SCMD_RECITE:
		case SCMD_QUAFF:
			if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
			{
				send_to_char(ch, "Você não parece ter %s.\r\n", arg);
				return;
			}
			break;
		case SCMD_USE:
			send_to_char(ch, "Você não parece estar usando %s.\r\n", arg);
			return;
		default:
			log1("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
			/* SYSERR_DESC: This is the same as the unhandled case in
			   do_gen_ps(), but in the function which handles 'quaff',
			   'recite', and 'use'. */
			return;
		}
	}
	switch (subcmd)
	{
	case SCMD_QUAFF:
		if (GET_OBJ_TYPE(mag_item) != ITEM_POTION)
		{
			send_to_char(ch, "Você só pode tomar poções.\r\n");
			return;
		}
		break;
	case SCMD_RECITE:
		if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL)
		{
			send_to_char(ch, "Você só pode recitar pergaminhos.\r\n");
			return;
		}
		break;
	case SCMD_USE:
		if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) && (GET_OBJ_TYPE(mag_item) != ITEM_STAFF))
		{
			send_to_char(ch, "Você não imagina como usar isso.\r\n");
			return;
		}
		break;
	}

	mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_display)
{
	size_t i;

	if (IS_NPC(ch))
	{
		send_to_char(ch, "Monstros não precisam disso.  Vá embora.\r\n");
		return;
	}
	skip_spaces(&argument);

	if (!*argument)
	{
		send_to_char(ch, "Uso: prompt { { H | M | V } | tudo | auto | nada }\r\n");
		return;
	}

	if (!str_cmp(argument, "auto"))
	{
		TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);
		send_to_char(ch, "Auto prompt %sabilitado.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "h" : "des");
		return;
	}

	if (!str_cmp(argument, "tudo") || !str_cmp(argument, "liga") || (!str_cmp(argument, "on") || !str_cmp(argument, "all")))
	{
		SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
		SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
		SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	}
	else if (!str_cmp(argument, "desliga") || !str_cmp(argument, "nada")|| !str_cmp(argument, "off") || !str_cmp(argument, "none"))
	{
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	}
	else
	{
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);

		for (i = 0; i < strlen(argument); i++)
		{
			switch (LOWER(argument[i]))
			{
			case 'h':
				SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
				break;
			case 'm':
				SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
				break;
			case 'v':
				SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
				break;
			default:
				send_to_char(ch, "Uso: prompt { { H | M | V } | tudo | auto | nada }\r\n");
				return;
			}
		}
	}

	send_to_char(ch, "%s", CONFIG_OK);
}

#define TOG_OFF 0
#define TOG_ON  1
ACMD(do_gen_tog)
{
	long result;
	int i;
	char arg[MAX_INPUT_LENGTH];

	const char *tog_messages[][2] = {
		{"Você agora não poderá ser convocado por outros jogadores.\r\n",
		 "Você agora pode ser convocado por outros jogadores.\r\n"},
		{"Nohassle desabilitado.\r\n",
		 "Nohassle habilitado.\r\n"},
		{"Modo 'Breve' desligado.\r\n",
      "Modo 'Breve' ligado.\r\n"},
		{"Modo 'Compacto' desligado.\r\n",
      "Modo 'Compacto' ligado.\r\n"},
		{"Canal TELL ativado.\r\n",
		 "Canal TELL desativado.\r\n"},
		{"Canal AUCTION ativado.\r\n",
		 "Canal AUCTION desativado.\r\n"},
		{"Canal SHOUT ativado.\r\n",
		 "Canal SHOUT desativado.\r\n"},
		{"Canal GOSS ativado.\r\n",
		 "Canal GOSS desativado.\r\n"},
		{"Canal GRATZ ativado.\r\n",
		 "Canal GRATZ desativado.\r\n"},
			{"Canal WIZ ativado.\r\n",
		 "Canal WIZ desativado.\r\n"},
		{"Você não faz mais parte do grupo de busca (quest)!\r\n",
		 "Ok, agora você faz parte do grupo de busca (quest)!\r\n"},
		{"Você não irá mais ver as flags das salas.\r\n",
		 "Agora você irá ver as flags das salas.\r\n"},
		{"Agora você verá suas frases repetidas.\r\n",
		 "Você não irá mais ver suas frases repetidas.\r\n"},
		{"Modo 'HolyLight' desligado.\r\n", "Modo 'HolyLight' ligado.\r\n"},
		{"Modo 'SlowNS' desligado. Endereços IP serão resolvidos.\r\n",
				"Modo 'SlowNS' ligado. Endereços IP não serão resolvidos.\r\n"},
		{"Autoexits desligado.\r\n",
		 "Autoexits ligado.\r\n"},
		{"Não será mais possível rastrear através de portas.\r\n",
				"Agora será possível rastrear através de portas.\r\n"},
		{"A tela não vai ser limpa no OLC.\r\n", "A tela vai ser limpa no OLC.\r\n"},
		{"Modo 'Construtor' desligado.\r\n", "Modo 'Construtor' ligado.\r\n"},
		{"AWAY desligado.\r\n", "AWAY ligado"},
		{"Autoloot desligado.\r\n",
		 "Autoloot ligado.\r\n"},
		{"Autogold desligado.\r\n",
		 "Autogold ligado.\r\n"},
		{"Autosplit desligado.\r\n",
		 "Autosplit ligado.\r\n"},
		{"Autosac desligado.\r\n",
		 "Autosac ligado.\r\n"},
		{"Autoassist desligado.\r\n",
		 "Autoassist ligado.\r\n"},
		{"Agora, você não irá mais ver o  mini-mapa.\r\n",
		 "Agora, você irá ver o  mini-mapa.\r\n"},
		{"Você agora precisa destrancar as portas manualmente.\r\n",
		 "Você agora vai destrancar as portas automaticamente ao abrir (se tiver a chave).\r\n"},
		{"Você agora precisa especificar uma direção ao abrir, fechar e destrancar.\r\n",
		 "Você agora vai encontrar a próxima porta disponível ao abrir, fechar ou destrancar.\r\n"},
		{"Você não irá mais ver os resets de áreas.\r\n",
		 "Você irá mais ver os resets de áreas.\r\n"},
		 {"Você não verá mais a saúde do oponente durante a luta.\r\n",
				"Agora você verá a saúde do oponente durante a luta.\r\n"},
		{"Seu título não será mais alterado automaticamente.\r\n",
				"Seu título será alterado automaticamente sempre que evoluir um nível.\r\n"}
	};

	if (IS_NPC(ch))
		return;

	switch (subcmd)
	{
	case SCMD_NOSUMMON:
		result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
		break;
	case SCMD_NOHASSLE:
		result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
		break;
	case SCMD_BRIEF:
		result = PRF_TOG_CHK(ch, PRF_BRIEF);
		break;
	case SCMD_COMPACT:
		result = PRF_TOG_CHK(ch, PRF_COMPACT);
		break;
	case SCMD_NOTELL:
		result = PRF_TOG_CHK(ch, PRF_NOTELL);
		break;
	case SCMD_NOAUCTION:
		result = PRF_TOG_CHK(ch, PRF_NOAUCT);
		break;
	case SCMD_NOSHOUT:
		result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
		break;
	case SCMD_NOGOSSIP:
		result = PRF_TOG_CHK(ch, PRF_NOGOSS);
		break;
	case SCMD_NOGRATZ:
		result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
		break;
	case SCMD_NOWIZ:
		result = PRF_TOG_CHK(ch, PRF_NOWIZ);
		break;
	case SCMD_QUEST:
		result = PRF_TOG_CHK(ch, PRF_QUEST);
		break;
	case SCMD_SHOWVNUMS:
		result = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
		break;
	case SCMD_NOREPEAT:
		result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
		break;
	case SCMD_HOLYLIGHT:
		result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
		break;
	case SCMD_AUTOEXIT:
		result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
		break;
	case SCMD_CLS:
		result = PRF_TOG_CHK(ch, PRF_CLS);
		break;
	case SCMD_BUILDWALK:
		if (GET_LEVEL(ch) < LVL_BUILDER)
		{
			send_to_char(ch, "Apenas construtores, desculpe.\r\n");
			return;
		}
		result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
		if (PRF_FLAGGED(ch, PRF_BUILDWALK))
		{
			one_argument(argument, arg);
			for (i = 0; *arg && *(sector_types[i]) != '\n'; i++)
				if (is_abbrev(arg, sector_types[i]))
					break;
			if (*(sector_types[i]) == '\n')
				i = 0;
			GET_BUILDWALK_SECTOR(ch) = i;
			send_to_char(ch, "Setor Padrão %s\r\n", sector_types[i]);

			mudlog(CMP, GET_LEVEL(ch), TRUE,
				   "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
		}
		else
			mudlog(CMP, GET_LEVEL(ch), TRUE,
				   "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch),
				   GET_OLC_ZONE(ch));
		break;
	case SCMD_AFK:
		result = PRF_TOG_CHK(ch, PRF_AFK);
		if (PRF_FLAGGED(ch, PRF_AFK))
			act("$n agora está longe do teclado.", TRUE, ch, 0, 0, TO_ROOM);
		else
		{
			act("$n retornou ao teclado.", TRUE, ch, 0, 0, TO_ROOM);
			if (has_mail(GET_IDNUM(ch)))
				send_to_char(ch, "Você tem carta esperando.\r\n");
		}
		break;
	case SCMD_AUTOLOOT:
		result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
		break;
	case SCMD_AUTOGOLD:
		result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
		break;
	case SCMD_AUTOSPLIT:
		result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
		break;
	case SCMD_AUTOSAC:
		result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
		break;
	case SCMD_AUTOASSIST:
		result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
		break;
	case SCMD_AUTOMAP:
		result = PRF_TOG_CHK(ch, PRF_AUTOMAP);
		break;
	case SCMD_AUTOKEY:
		result = PRF_TOG_CHK(ch, PRF_AUTOKEY);
		break;
	case SCMD_AUTODOOR:
		result = PRF_TOG_CHK(ch, PRF_AUTODOOR);
		break;
	case SCMD_ZONERESETS:
		result = PRF_TOG_CHK(ch, PRF_ZONERESETS);
		break;
			case SCMD_HITBAR:
		result = PRF_TOG_CHK(ch, PRF_HITBAR);
		break;
	case SCMD_AUTOTITLE:
		result = PRF_TOG_CHK(ch, PRF_AUTOTITLE);
		break;
	default:
		log1("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
		return;
	}

	if (result)
		send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
	else
		send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

	return;
}

static void show_happyhour(struct char_data *ch)
{
	char happyexp[80], happygold[80], happyqp[80];
	int secs_left;

	if ((IS_HAPPYHOUR) || (GET_LEVEL(ch) >= LVL_GRGOD))
	{
		if (HAPPY_TIME)
			secs_left = ((HAPPY_TIME - 1) * SECS_PER_MUD_HOUR) + next_tick;
		else
			secs_left = 0;

		sprintf(happyqp, "%s+%d%%%s para Questpoints por quest.\r\n", CCYEL(ch, C_NRM), HAPPY_QP,
				CCNRM(ch, C_NRM));
		sprintf(happygold, "%s+%d%%%s para Moedas ganhas por morte.\r\n", CCYEL(ch, C_NRM), HAPPY_GOLD,
				CCNRM(ch, C_NRM));
		sprintf(happyexp, "%s+%d%%%s para Experiencia por morte.\r\n", CCYEL(ch, C_NRM), HAPPY_EXP,
				CCNRM(ch, C_NRM));

		send_to_char(ch, "Happy Hour de Vitália!\r\n"
					 "------------------\r\n"
					 "%s%s%sRestam: %s%d%s horas %s%d%s minutos %s%d%s segundos\r\n",
					 (IS_HAPPYEXP || (GET_LEVEL(ch) >= LVL_GOD)) ? happyexp : "",
					 (IS_HAPPYGOLD || (GET_LEVEL(ch) >= LVL_GOD)) ? happygold : "",
					 (IS_HAPPYQP || (GET_LEVEL(ch) >= LVL_GOD)) ? happyqp : "",
					 CCYEL(ch, C_NRM), (secs_left / 3600), CCNRM(ch, C_NRM),
					 CCYEL(ch, C_NRM), (secs_left % 3600) / 60, CCNRM(ch, C_NRM),
					 CCYEL(ch, C_NRM), (secs_left % 60), CCNRM(ch, C_NRM));
	}
	else
	{
		send_to_char(ch, "Desculpa, mas não tem nenhum happy hour acontecendo!\r\n");
	}
}

ACMD(do_happyhour)
{
	char arg[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
	int num;

	if (GET_LEVEL(ch) < LVL_GOD)
	{
		show_happyhour(ch);
		return;
	}

	/* Only Imms get here, so check args */
	two_arguments(argument, arg, val);

	if (is_abbrev(arg, "experiencia"))
	{
		num = MIN(MAX((atoi(val)), 0), 1000);
		HAPPY_EXP = num;
		send_to_char(ch, "Happy Hour Exp aumentado para +%d%%\r\n", HAPPY_EXP);
	}
	else if ((is_abbrev(arg, "ouro")) || (is_abbrev(arg, "moedas")))
	{
		num = MIN(MAX((atoi(val)), 0), 1000);
		HAPPY_GOLD = num;
		send_to_char(ch, "Happy Hour Ouro aumentado para +%d%%\r\n", HAPPY_GOLD);
	}
	else if ((is_abbrev(arg, "tempo")) || (is_abbrev(arg, "ticks")))
	{
		num = MIN(MAX((atoi(val)), 0), 1000);
		if (HAPPY_TIME && !num)
			game_info("Happyhour chegou ao fim!");
		else if (!HAPPY_TIME && num)
			game_info("Um Happyhour começou!");

		HAPPY_TIME = num;
		send_to_char(ch, "Happy Hour configurado para %d ticks (%d:%d:%d)\r\n",
					 HAPPY_TIME,
					 (HAPPY_TIME * SECS_PER_MUD_HOUR) / 3600,
					 ((HAPPY_TIME * SECS_PER_MUD_HOUR) % 3600) / 60,
					 (HAPPY_TIME * SECS_PER_MUD_HOUR) % 60);
	}
	else if ((is_abbrev(arg, "qp")) || (is_abbrev(arg, "questpoints")))
	{
		num = MIN(MAX((atoi(val)), 0), 1000);
		HAPPY_QP = num;
		send_to_char(ch, "Happy Hour Questpoints aumentados para +%d%%\r\n", HAPPY_QP);
	}
	else if (is_abbrev(arg, "show"))
	{
		show_happyhour(ch);
	}
	else if (is_abbrev(arg, "default")|| is_abbrev(arg, "padrão"))
	{
		HAPPY_EXP = 100;
		HAPPY_GOLD = 50;
		HAPPY_QP = 50;
		HAPPY_TIME = 48;
		game_info("Um Happyhour foi iniciado!");
	}
	else
	{
		send_to_char(ch, "Usage: %shappyhour              %s- show usage (this info)\r\n"
					 "       %shappyhour show         %s- display current settings (what mortals see)\r\n"
					 "       %shappyhour time <ticks> %s- set happyhour time and start timer\r\n"
					 "       %shappyhour qp <num>     %s- set qp percentage gain\r\n"
					 "       %shappyhour exp <num>    %s- set exp percentage gain\r\n"
					 "       %shappyhour gold <num>   %s- set gold percentage gain\r\n"
					 "       \tyhappyhour default      \tw- sets a default setting for happyhour\r\n\r\n"
					 "Configure the happyhour settings and start a happyhour.\r\n"
					 "Currently 1 hour IRL = %d ticks\r\n"
					 "If no number is specified, 0 (off) is assumed.\r\nThe command \tyhappyhour time\tn will therefore stop the happyhour timer.\r\n",
					 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
					 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
					 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
					 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
					 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
					 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), (3600 / SECS_PER_MUD_HOUR));
	}
}

static int can_rebegin(struct char_data *ch)
{
	if (!PLR_FLAGGED(ch, PLR_TRNS))
		return (0);

	return (1);
}


static int can_elevate(struct char_data *ch)
{
	if (!PLR_FLAGGED(ch, PLR_TRNS))
		return (0);
	if (GET_REMORT(ch) < 4)
		return (0);

	return (1);
}

ACMD(do_recall)
{
   int loss;
	struct char_data *was_fighting = NULL;

	if (IS_NPC(ch))
		return;

	if (GET_LEVEL(ch) > 10 && GET_LEVEL(ch) < LVL_IMMORT)
	{
		send_to_char(ch, "Você não pode mais usar este comando.\r\n");
		return;
	}

	if (PLR_FLAGGED(ch, PLR_GHOST | PLR_JAILED))
	{
		send_to_char(ch, "Nah... nem pensar!\r\n");
		return;
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC))	{
		send_to_char(ch, "Sem Efeito");
		return;
	}

	if (FIGHTING(ch))
	{
		was_fighting = FIGHTING(ch);
		 if (was_fighting && !IS_NPC(ch))
		{
			loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
			loss *= GET_LEVEL(was_fighting);
			send_to_char(ch, "Você perdeu %ld pontos de experiência.\r\n", (long)loss);
			gain_exp(ch, -loss);
		}
		if (FIGHTING(ch))
			stop_fighting(ch);
		if (was_fighting && ch == FIGHTING(was_fighting))
			stop_fighting(was_fighting);
	}

	act("$n desaparece.", TRUE, ch, 0, 0, TO_ROOM);
	char_from_room(ch);
	char_to_room(ch, GET_HOMETOWN(ch));
	act("$n aparece no meio da sala.", TRUE, ch, 0, 0, TO_ROOM);
	look_at_room(ch, 0);
}


ACMD(do_suggestion)
{
   struct char_data *mob;
   int num;
   struct obj_data *object;
   struct fann *ann;
   struct fann *ann_move;
	fann_type *calc_output;
	fann_type *move_output;
	
	ann = fann_create_from_file("etc/aventureiro.fann");
	ann_move = fann_create_from_file("etc/move.fann");
	
	fann_type input[29];
	int grupo;
	int count_obj = 0;
	int comando = 0;
	int run_move = 0;
   int tries;
    if ((GET_LEVEL(ch) >= LVL_GOD) || IS_NPC(ch))
    {
       send_to_char(ch,"Você já deveria ter experiência suficiente para dar as  sugestões.\r\n");
       return;
    }
    
    /* verifica grupo e inventario */
	if (GROUP(ch) != NULL)
		grupo = 1;
	else
		grupo = 0;

	for (object= ch->carrying; object; object = object->next_content)
	{
			count_obj++;
	}

	   /*pega dados pro aventureiro */
	input[0] = (float)GET_HIT(ch) / 5000;
	input[1] = (float) GET_MAX_HIT(ch) / 5000;

	input[2] = (float)GET_MANA(ch) / 5000;
	input[3] = (float) GET_MAX_MANA(ch) / 5000;
	input[4] = (float)GET_MOVE(ch) / 5000;
	input[5] = (float) GET_MAX_MOVE(ch) / 5000;
	input[6] = (float) GET_EXP(ch) / 500000000;
	input[7] = (float)  GET_ROOM_VNUM(IN_ROOM(ch)) / 100000;
	input[8] = 1/(1+exp(-GET_CLASS(ch)));
	input[9] = 1/(1+exp(-GET_POS(ch)));
	input[10] = (float) GET_ALIGNMENT(ch) / 1000;
	input[11] = (float) compute_armor_class(ch) / 200;
	input[12] = (float) GET_STR(ch) / 25;
	input[13] = (float) GET_ADD(ch) / 25;
	input[14] = (float) GET_INT(ch) / 25;
	input[15] = (float) GET_WIS(ch) / 25;
	input[16] = (float) GET_CON(ch) / 25;
	input[17] = (float) GET_DEX(ch) / 25;
	input[18] = (float) GET_GOLD(ch) / 100000000;
	input[19] = (float) GET_BANK_GOLD(ch) / 100000000;
	input[20] = 1/(1+exp(-GET_COND(ch, HUNGER)));
	input[21] = 1/(1+exp(-GET_COND(ch, THIRST)));
	input[22] = (float) GET_PRACTICES(ch) / 100;
	input[23] = grupo;
	//input 24 eh o clan em vez de mudhora
	input[24] = 1/(1+exp(-0));
	input[25] = (float) GET_BREATH(ch) / 15;
	input[26] = (float) GET_HITROLL(ch) / 100;
	input[27] = (float) GET_DAMROLL(ch) / 100;
	input[28] = (float) count_obj / 100;

	calc_output = fann_run(ann, input);
   calc_output[0] = calc_output[0] * (float) MAX_COMMAND;
   comando = fabs(calc_output[0]);
   
   
   if (comando < 432){
    run_move = 1;
   move_output = fann_run(ann_move, input);
   calc_output[0] = (move_output[0] * (float) NUM_OF_DIRS);
   calc_output[0] = move_output[0] + 1;
   comando = fabs(calc_output[0]);
   
   }
   
   if (GET_IDNUM(ch) == 20)
   {
      if (run_move) {
      send_to_char(ch,"move output: %f %f %d\r\n",move_output[0],move_output[1],comando);
      }
      else {
      send_to_char(ch,"output: %f %f %f %f %f %f\r\n",calc_output[0],calc_output[1],calc_output[2],calc_output[3],calc_output[4],calc_output[5]);
      }
   }
   if ((comando == MAX_COMMAND) || (comando == 0))
   {
      send_to_char(ch,"Nenhum comando sugerido para você.\r\n");
   }
 
    if (!run_move) {
    for (tries = 0; tries < 10; tries++)
    {
     calc_output = fann_run(ann, input);
   calc_output[0] = calc_output[0] * (float) MAX_COMMAND;
   comando = fabs(calc_output[0]);
    if (complete_cmd_info[comando].minimum_level  <= GET_LEVEL(ch))
      continue;
    }
    }
      if (complete_cmd_info[comando].minimum_level  > GET_LEVEL(ch))
  send_to_char(ch, "Comando sugerido acima do teu nível.\r\n");
  
  else if (calc_output[3] > 0 && calc_output[5] > 0)
   send_to_char(ch,"Sugestão de comando: %s\r\n",complete_cmd_info[comando].command);
   else  if (calc_output[3] > 0){
      if (calc_output[3] == 0.5) {
         if  ((num = real_mobile(calc_output[4] * 10000)) != NOBODY){
         mob = read_mobile(num, REAL);
         char_to_room(mob, 0);
          send_to_char(ch,"Sugestão de comando: %s %s\r\n",complete_cmd_info[comando].command, mob->player.name);
          extract_char(mob);
         }
           send_to_char(ch,"Sugestão de comando: %s\r\n",complete_cmd_info[comando].command);
      }
          else if (calc_output[3] == 0.880797) {
         	if ((num = real_object(calc_output[4] * 10000)) != NOTHING) {
         	object = read_object(num, REAL);
           send_to_char(ch,"Sugestão de comando: %s %s\r\n",complete_cmd_info[comando].command, object->name);
         extract_obj(object);
         	}
         	  send_to_char(ch,"Sugestão de comando: %s\r\n",complete_cmd_info[comando].command);
      }
      else if (calc_output[3] == 0.731059)
         send_to_char(ch,"Sugestão de comando: %s all\r\n",complete_cmd_info[comando].command);
      else 
   send_to_char(ch,"Sugestão de comando: %s\r\n",complete_cmd_info[comando].command);
 }
   else
   send_to_char(ch,"Sugestão de comando: %s\r\n",complete_cmd_info[comando].command);
   
   fann_destroy(ann);
   fann_destroy(ann_move);
}