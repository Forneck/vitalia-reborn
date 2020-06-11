/**************************************************************************
*  File: act.offensive.c                                   Part of tbaMUD *
*  Usage: Player-level commands of an offensive nature.                   *
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
#include "act.h"
#include "fight.h"
#include "mud_event.h"

ACMD(do_assist)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *helpee, *opponent;

	if (FIGHTING(ch))
	{
		send_to_char(ch,
					 "Você já está lutando!  Como você pretende dar assistência a mais alguém?\r\n");
		return;
	}
	if IS_DEAD
		(ch)
	{
		send_to_char(ch, "Você não pode dar assistência a ninguém, você está mort%c!",
					 OA(ch));
		return;
	}
	one_argument(argument, arg);

	if (!*arg)
		send_to_char(ch, "Para quem você deseja dar assistência?\r\n");
	else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (helpee == ch)
		send_to_char(ch, "Você não pode ajudar a si mesm%c!\r\n", OA(ch));
	else
	{
		/* 
		 * Hit the same enemy the person you're helping is.
		 */
		if (FIGHTING(helpee))
			opponent = FIGHTING(helpee);
		else
			for (opponent = world[IN_ROOM(ch)].people;
				 opponent && (FIGHTING(opponent) != helpee); opponent = opponent->next_in_room);

		if (!opponent)
			act("Mas $L não está lutando com ninguém!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (!CAN_SEE(ch, opponent))
			act("Você não pode ver com quem $L está lutando!", FALSE, ch, 0, helpee, TO_CHAR);
		/* prevent accidental pkill */
		else if (!CONFIG_PK_ALLOWED && !IS_NPC(opponent))
			send_to_char(ch, "Use 'murder' se você deseja realmente atacar!\r\n");
		else
		{
			send_to_char(ch, "Você entra para a luta!\r\n");
			act("$N da assistência a você !", 0, helpee, 0, ch, TO_CHAR);
			act("$n da assistência a  $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
			hit(ch, opponent, TYPE_UNDEFINED);
		}
	}
}

/* -- VP -- jr - 23/06/99 * Rotina reestruturada, para eliminar bugs. -- jr -
   Apr 16, 2000 * Nova reestrutura��o da rotina. -- Cansian - Jun, 11,
   2020 * Atualizado para nova estrutura de grupo. */
ACMD(do_gassist)
{
	struct char_data *k, *helpee, *opponent;
	struct group_data *group;

	if ((group = GROUP(ch)) == NULL)
	{
		send_to_char(ch, "Mas você não é membro de um grupo!\r\n");
		return;
	}

	if (FIGHTING(ch))
	{
		send_to_char(ch,
					 "Você já está lutando!  Como você pretente dar assistência a alguém?\r\n");
		return;
	}

	if IS_DEAD
		(ch)
	{
		send_to_char(ch, "Você não pode dar assistência a ninguém, você está mort%c!",
					 OA(ch));
		return;
	}
	if (GROUP(ch))
		while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
		{
			if ((k != ch) && CAN_SEE(ch, k) && (IN_ROOM(k) == IN_ROOM(ch)) && FIGHTING(k))
			{
				if (!CAN_SEE(ch, FIGHTING(k)))
					act("Você não pode ver com quem $N está lutando.", FALSE, ch, NULL, k, TO_CHAR);
				else
				{
					helpee = k;
					break;
				}
			}
		}

	if (!helpee)
	{
		send_to_char(ch, "Você não vê ninguém lutando em seu grupo.\r\n");
		return;
	}
	else
	{
		opponent = FIGHTING(helpee);
		act("Você dá assistência a $N.", FALSE, ch, 0, helpee, TO_CHAR);

		if (!CONFIG_PK_ALLOWED && !IS_NPC(opponent))	/* prevent accidental
														   pkill */
			act("Use 'murder' se voce realmente deseja atacar $N.", FALSE, ch, 0, opponent,
				TO_CHAR);
		else
		{
			act("$N dá assistência a você!", FALSE, helpee, 0, ch, TO_CHAR);
			send_to_group(NULL, group, "%s dá assistência a um membro do grupo!\r\n",
						  GET_NAME(ch));
			act("$n dá assistência a $N.", TRUE, ch, 0, helpee, TO_NOTVICT);
			hit(ch, opponent, TYPE_UNDEFINED);
		}
	}
}

ACMD(do_hit)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;

	one_argument(argument, arg);

	if (!*arg)
		send_to_char(ch, "Bater em quem?\r\n");
	else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_to_char(ch, "Esta pessoa não parece estar aqui.\r\n");
	else if (vict == ch)
	{
		send_to_char(ch, "Você bate em si mesm%s... AI!\r\n", OA(ch));
		act("$n bate em si mesm$r, e diz AI!", FALSE, ch, 0, vict, TO_ROOM);
	}
	else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
		act("$N é amig$R e você não pode atacá-l$R.", FALSE, ch, 0, vict, TO_CHAR);
	else
	{
		if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
			return;				/* you can't order a charmed pet to attack a
								   player */

		if (!CONFIG_PK_ALLOWED && !IS_NPC(vict) && !IS_NPC(ch))
			if (!SCMD_MURDER)
				send_to_char(ch,
							 "Use 'murder' se você realmente deseja atacar outro jogador.\r\n");
			else
				check_killer(ch, vict);

		if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch)))
		{
			if (GET_DEX(ch) > GET_DEX(vict) || (GET_DEX(ch) == GET_DEX(vict) && rand_number(1, 2) == 1))	/* if 
																											   faster 
																											 */
				hit(ch, vict, TYPE_UNDEFINED);	/* first */
			else
				hit(vict, ch, TYPE_UNDEFINED);	/* or the victim is first */
			WAIT_STATE(ch, PULSE_VIOLENCE + 2);
		}
		else
			send_to_char(ch, "Você está fazendo o melhor que você pode!\r\n");
	}
}

ACMD(do_kill)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;

	if (GET_LEVEL(ch) < LVL_GRGOD || IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_NOHASSLE))
	{
		do_hit(ch, argument, cmd, subcmd);
		return;
	}
	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char(ch, "Matar quem?\r\n");
	}
	else
	{
		if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
			send_to_char(ch, "Esta pessoa não está aqui.\r\n");
		else if (ch == vict)
			send_to_char(ch, "Sua mamãe vai ficar triste... :(\r\n");
		else
		{
			act("Você esmigalha $N em pedaços!  Ah!  Sangue!", FALSE, ch, 0, vict, TO_CHAR);
			act("$N esmigalha você em pedaços!", FALSE, vict, 0, ch, TO_CHAR);
			act("$nbrutalmente destrói $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			raw_kill(vict, ch);
		}
	}
}

ACMD(do_backstab)
{
	char buf[MAX_INPUT_LENGTH];
	struct char_data *vict;
	int percent, prob;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB))
	{
		send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
		return;
	}

	one_argument(argument, buf);

	if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char(ch, "Esfaquear as costas de quem?\r\n");
		return;
	}
	if (vict == ch)
	{
		send_to_char(ch, "Como você pretende pegar você mesmo de surpresa?\r\n");
		return;
	}
	if (!CONFIG_PK_ALLOWED && !IS_NPC(vict))	/* prevent accidental
													   pkill */
		act("Use 'murder' se voce realmente deseja atacar $N.", FALSE, ch, 0, vict, TO_CHAR);
	if (!GET_EQ(ch, WEAR_WIELD))
	{
		send_to_char(ch, "Voce precisa empunhar uma arma para fazer isso.\r\n");
		return;
	}
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT)
	{
		send_to_char(ch,
					 "Somente armas perfurantes podem ser usadas para esfaquear alguém pelas costas.\r\n");
		return;
	}
	if (FIGHTING(vict))
	{
		send_to_char(ch,
					 "Você não pode pegar de surpresa uma pessoa lutando -- ela provavelmente está alerta!\r\n");
		return;
	}

	if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict))
	{
		act("Você percebe $N tentando pegar você desprevenid$r!", FALSE, vict, 0, ch, TO_CHAR);
		act("$l percebeu você tentando pegá-l$r desprevenid$r!", FALSE, vict, 0, ch, TO_VICT);
		act("$n percebeu que $N iria pegá-l$r desprevenid$r!", FALSE, vict, 0, ch, TO_NOTVICT);
		hit(vict, ch, TYPE_UNDEFINED);
		return;
	}

	percent = rand_number(1, 101);	/* 101% is a complete failure */
	prob = GET_SKILL(ch, SKILL_BACKSTAB);

	if (AWAKE(vict) && (percent > prob))
		damage(ch, vict, 0, SKILL_BACKSTAB);
	else
		hit(ch, vict, SKILL_BACKSTAB);

	if (GET_LEVEL(ch) < LVL_GOD)
	WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

/* 
 * BACKFLIP skill
 */
ACMD(do_backflip)
{
   	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;
	int percent, prob;

	if (IS_NPC(ch)
		|| !GET_SKILL(ch, SKILL_BACKFLIP)
		|| !GET_SKILL(ch, SKILL_BACKSTAB) || PLR_FLAGGED(ch, PLR_TRNS))
	{
		send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
		return;
	}
	else if (GET_POS(ch) != POS_FIGHTING)
	{
		send_to_char(ch, "Você deve estar lutando para ter sucesso.\r\n");
		return;
	}

	one_argument(argument, arg);

	if (*arg)
	{
		if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
		{
			send_to_char(ch, "Quem?\r\n");
			return;
		}
	}
	else if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
	{
		vict = FIGHTING(ch);
	}
	else
	{
		send_to_char(ch, "Quem?\r\n");
		return;
	}

	if (vict == ch)
	{
		send_to_char(ch, "Tá... entendí... faz muito sentido...\r\n");
		return;
	}
	if (FIGHTING(vict) == NULL || FIGHTING(vict) != ch)
	{
		act("$N não está lutando com você...", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}

	if (IS_DEAD(ch))
	{
		act("Como você pretende fazer isso? Você está mort$r!", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}

	percent = rand_number(1, 101);
	prob = GET_SKILL(ch, SKILL_BACKFLIP);

	percent += (25 - GET_DEX(ch));
	percent += (25 - GET_STR(ch)) / 2;

	if (percent > prob)
	{
		act("Você tenta fazer uma cambalhota por sobre $N, mas perde o equilíbrio e cai sentad$r.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n faz um lindo giro no ar e cai sentad$r no chão, que ridículo...", FALSE, ch,
			NULL, vict, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
	WAIT_STATE(ch, PULSE_VIOLENCE * 3);
	}
	else
	{
		act("Que cambalhota perfeita! $U$N não sabe onde você está e ficou perdid$R!", FALSE,
			ch, NULL, vict, TO_CHAR);
		act("$n faz uma cambalhota por sobre $N, que fica confus$R!", FALSE, ch, NULL, vict,
			TO_NOTVICT);
		act("$n faz uma cambalhota e você $r perde de vista.", FALSE, ch, NULL, vict, TO_VICT);
	WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		/* it is assured that vict will be fighting ch */
		stop_fighting(vict);
	}
}

ACMD(do_order)
{
	char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
	bool found = FALSE;
	struct char_data *vict;
	struct follow_type *k;

	half_chop(argument, name, message);

	if (!*name || !*message)
		send_to_char(ch, "Ordenar quem a fazer o quê?\r\n");
	else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))
			 && !is_abbrev(name, "seguidores") && !is_abbrev(name, "followers"))
		send_to_char(ch, "Esta pessoa não está aqui.\r\n");
	else if (ch == vict)
		send_to_char(ch, "Você evidentemente sofre de esquisofrenia.\r\n");
	else
	{
		if (AFF_FLAGGED(ch, AFF_CHARM))
		{
			send_to_char(ch, "Seu superior não irá aprovar você dando ordens.\r\n");
			return;
		}
		if (vict)
		{
			char buf[MAX_STRING_LENGTH];

			snprintf(buf, sizeof(buf), "$N ordena você a '%s'", message);
			act(buf, FALSE, vict, 0, ch, TO_CHAR);
			act("$n dá uma ordem a $N ", FALSE, ch, 0, vict, TO_ROOM);

			if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
				act("$n tem um olhar indiferente.", FALSE, vict, 0, 0, TO_ROOM);
			else
			{
				send_to_char(ch, "%s", CONFIG_OK);
				command_interpreter(vict, message);
			}
		}
		else
		{						/* This is order "followers" */
			char buf[MAX_STRING_LENGTH];

			snprintf(buf, sizeof(buf), "$ndita a ordem '%s'.", message);
			act(buf, FALSE, ch, 0, 0, TO_ROOM);

			for (k = ch->followers; k; k = k->next)
			{
				if (IN_ROOM(ch) == IN_ROOM(k->follower))
					if (AFF_FLAGGED(k->follower, AFF_CHARM))
					{
						found = TRUE;
						command_interpreter(k->follower, message);
					}
			}
			if (found)
				send_to_char(ch, "%s", CONFIG_OK);
			else
				send_to_char(ch, "Ninguém aqui é fiel a você!\r\n");
		}
	}
}

ACMD(do_flee)
{
	int i, attempt, loss;
	int perc, prob, escape = false;
	struct char_data *was_fighting;

	if (subcmd == SCMD_ESCAPE)
	{
		if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_ESCAPE))
		{
			send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
			return;
		}
		perc = rand_number(1, 101);	/* 101% is a complete failure */
		prob = GET_SKILL(ch, SKILL_ESCAPE);
		escape = prob > perc;
	}

	if (GET_POS(ch) < POS_FIGHTING)
	{
		send_to_char(ch, "Você não está em condições de fugir!\r\n");
		return;
	}

	for (i = 0; i < 6; i++)
	{
		attempt = rand_number(0, DIR_COUNT - 1);	/* Select a random
													   direction */
		if (CAN_GO(ch, attempt) && !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH))
		{
			act("$n entra em pânico e tenta fugir!", TRUE, ch, 0, 0, TO_ROOM);
			if (!escape)
				was_fighting = FIGHTING(ch);

			if (AFF_FLAGGED(ch, AFF_PARALIZE))
			{
				send_to_char(ch, "Você está paralisado! Não pode fugir! Comece a rezar...\r\n");
				act("$n não pode fugir, $l está paralisad$r!", TRUE, ch, 0, 0, TO_ROOM);
			}
			if (do_simple_move(ch, attempt, TRUE))
			{
				send_to_char(ch, "Você foge de pernas para o ar.\r\n");
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
			else
			{
				send_to_char(ch, "Você tenta fugir, mas não consegue!\r\n");
				act("$n tenta fugir mas não consegue!", TRUE, ch, 0, 0, TO_ROOM);
			}
			return;
		}
	}
	send_to_char(ch, "@WPANICO!  Você não tem escapatória!@n\r\n");
}

ACMD(do_bash)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;
	int percent, prob;


	one_argument(argument, arg);

	if (!IS_NPC(ch) && !GET_SKILL(ch, SKILL_BASH))
	{
		send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
	{
		send_to_char(ch, "Este lugar é muito calmo, sem violências...\r\n");
		return;
	}
	if (!GET_EQ(ch, WEAR_WIELD))
	{
		send_to_char(ch, "Você precisa empunhar uma arma para ter sucesso.\r\n");
		return;
	}

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
		{
			vict = FIGHTING(ch);
		}
		else
		{
			send_to_char(ch, "Derrubar quem?\r\n");
			return;
		}
	}
	if (vict == ch)
	{
		send_to_char(ch, "Sem gracinhas hoje...\r\n");
		return;
	}
	if (MOB_FLAGGED(vict, MOB_NOKILL))
	{
		send_to_char(ch, "Você não pode lutar.\r\n");
		return;
	}
	if (!CONFIG_PK_ALLOWED && !IS_NPC(vict))
	{
		/* prevent accidental pkill */
		act("Use 'murder' se voce realmente deseja atacar $N.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	percent = rand_number(1, 101);	/* 101% is a complete failure */
	if (!IS_NPC(ch))
		prob = GET_SKILL(ch, SKILL_BASH);
	else
		prob = GET_LEVEL(ch);

	if (MOB_FLAGGED(vict, MOB_NOBASH))
		percent = 101;

	if (percent > prob)
	{
		damage(ch, vict, 0, SKILL_BASH);
		if (GET_LEVEL(ch) < LVL_GOD)
		{
			GET_POS(ch) = POS_SITTING;
			WAIT_STATE(ch, PULSE_VIOLENCE * 2);
		}
	}
	else
	{
		/* 
		 * If we bash a player and they wimp out, they will move to the previous
		 * room before we set them sitting.  If we try to set the victim sitting
		 * first to make sure they don't flee, then we can't bash them!  So now
		 * we only set them sitting if they didn't flee. -gg 9/21/98
		 */
		if (damage(ch, vict, 1, SKILL_BASH) > 0)
		{						/* -1 = dead, 0 = miss */
			WAIT_STATE(vict, PULSE_VIOLENCE * 2);
			if (IN_ROOM(ch) == IN_ROOM(vict))
				GET_POS(vict) = POS_SITTING;
		}
	}

if (GET_LEVEL(ch) < LVL_GOD)
	WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

ACMD(do_rescue)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict, *tmp_ch;
	int percent, prob;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_RESCUE))
	{
		send_to_char(ch, "You have no idea how to do that.\r\n");
		return;
	}

	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char(ch, "Whom do you want to rescue?\r\n");
		return;
	}
	if (vict == ch)
	{
		send_to_char(ch, "What about fleeing instead?\r\n");
		return;
	}
	if (FIGHTING(ch) == vict)
	{
		send_to_char(ch, "How can you rescue someone you are trying to kill?\r\n");
		return;
	}
	for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch &&
		 (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

	if ((FIGHTING(vict) != NULL) && (FIGHTING(ch) == FIGHTING(vict)) && (tmp_ch == NULL))
	{
		tmp_ch = FIGHTING(vict);
		if (FIGHTING(tmp_ch) == ch)
		{
			send_to_char(ch, "You have already rescued %s from %s.\r\n", GET_NAME(vict),
						 GET_NAME(FIGHTING(ch)));
			return;
		}
	}

	if (!tmp_ch)
	{
		act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	percent = rand_number(1, 101);	/* 101% is a complete failure */
	prob = GET_SKILL(ch, SKILL_RESCUE);

	if (percent > prob)
	{
		send_to_char(ch, "You fail the rescue!\r\n");
		return;
	}
	send_to_char(ch, "Banzai!  To the rescue...\r\n");
	act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
	act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

	if (FIGHTING(vict) == tmp_ch)
		stop_fighting(vict);
	if (FIGHTING(tmp_ch))
		stop_fighting(tmp_ch);
	if (FIGHTING(ch))
		stop_fighting(ch);

	set_fighting(ch, tmp_ch);
	set_fighting(tmp_ch, ch);

	WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
}

EVENTFUNC(event_whirlwind)
{
	struct char_data *ch, *tch;
	struct mud_event_data *pMudEvent;
	struct list_data *room_list;
	int count;

	/* This is just a dummy check, but we'll do it anyway */
	if (event_obj == NULL)
		return 0;

	/* For the sake of simplicity, we will place the event data in easily
	   referenced pointers */
	pMudEvent = (struct mud_event_data *)event_obj;
	ch = (struct char_data *)pMudEvent->pStruct;

	/* When using a list, we have to make sure to allocate the list as it uses 
	   dynamic memory */
	room_list = create_list();

	/* We search through the "next_in_room", and grab all NPCs and add them to 
	   our list */
	for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
		if (IS_NPC(tch))
			add_to_list(tch, room_list);

	/* If our list is empty or has "0" entries, we free it from memory and
	   close off our event */
	if (room_list->iSize == 0)
	{
		free_list(room_list);
		send_to_char(ch, "There is no one in the room to whirlwind!\r\n");
		return 0;
	}

	/* We spit out some ugly colour, making use of the new colour options, to
	   let the player know they are performing their whirlwind strike */
	send_to_char(ch, "\t[f313]You deliver a vicious \t[f014]\t[b451]WHIRLWIND!!!\tn\r\n");

	/* Lets grab some a random NPC from the list, and hit() them up */
	for (count = dice(1, 4); count > 0; count--)
	{
		tch = random_from_list(room_list);
		hit(ch, tch, TYPE_UNDEFINED);
	}

	/* Now that our attack is done, let's free out list */
	free_list(room_list);

	/* The "return" of the event function is the time until the event is
	   called again. If we return 0, then the event is freed and removed from
	   the list, but any other numerical response will be the delay until the
	   next call */
	if (GET_SKILL(ch, SKILL_WHIRLWIND) < rand_number(1, 101))
	{
		send_to_char(ch, "You stop spinning.\r\n");
		return 0;
	}
	else
		return 1.5 * PASSES_PER_SEC;
}

/* The "Whirlwind" skill is designed to provide a basic understanding of the
   mud event and list systems. */
ACMD(do_whirlwind)
{

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_WHIRLWIND))
	{
		send_to_char(ch, "You have no idea how.\r\n");
		return;
	}

	if ROOM_FLAGGED
		(IN_ROOM(ch), ROOM_PEACEFUL)
	{
		send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
		return;
	}

	if (GET_POS(ch) < POS_FIGHTING)
	{
		send_to_char(ch, "You must be on your feet to perform a whirlwind.\r\n");
		return;
	}

	/* First thing we do is check to make sure the character is not in the
	   middle of a whirl wind attack. "char_had_mud_event() will sift through
	   the character's event list to see if an event of type "eWHIRLWIND"
	   currently exists. */
	if (char_has_mud_event(ch, eWHIRLWIND))
	{
		send_to_char(ch, "You are already attempting that!\r\n");
		return;
	}

	send_to_char(ch, "You begin to spin rapidly in circles.\r\n");
	act("$n begins to rapidly spin in a circle!", FALSE, ch, 0, 0, TO_ROOM);

	/* NEW_EVENT() will add a new mud event to the event list of the
	   character. This function below adds a new event of "eWHIRLWIND", to
	   "ch", and passes "NULL" as additional data. The event will be called in 
	   "3 * PASSES_PER_SEC" or 3 seconds */
	NEW_EVENT(eWHIRLWIND, ch, NULL, 3 * PASSES_PER_SEC);
	WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_kick)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;
	int percent, prob;


	// if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK))
	if (!IS_NPC(ch) && !GET_SKILL(ch, SKILL_KICK))
	{
		send_to_char(ch, "You have no idea how.\r\n");
		return;
	}

	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
		{
			vict = FIGHTING(ch);
		}
		else
		{
			send_to_char(ch, "Kick who?\r\n");
			return;
		}
	}
	if (vict == ch)
	{
		send_to_char(ch, "Aren't we funny today...\r\n");
		return;
	}
	/* 101% is a complete failure */
	percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + rand_number(1, 101);

	if (IS_NPC(ch))
		prob = GET_LEVEL(ch);
	else
		prob = GET_SKILL(ch, SKILL_KICK);

	if (percent > prob)
		damage(ch, vict, 0, SKILL_KICK);
	else
		damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_KICK);

	WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_bandage)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;
	int percent, prob;
	if (!GET_SKILL(ch, SKILL_BANDAGE))
	{
		send_to_char(ch, "You are unskilled in the art of bandaging.\r\n");
		return;
	}

	if (GET_POS(ch) != POS_STANDING)
	{
		send_to_char(ch, "You are not in a proper position for that!\r\n");
		return;
	}

	one_argument(argument, arg);
	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char(ch, "Who do you want to bandage?\r\n");
		return;
	}

	if (GET_HIT(vict) >= 0)
	{
		send_to_char(ch, "You can only bandage someone who is close to death.\r\n");
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE * 2);
	percent = rand_number(1, 101);	/* 101% is a complete failure */
	prob = GET_SKILL(ch, SKILL_BANDAGE);
	if (percent <= prob)
	{
		act("Your attempt to bandage fails.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n tries to bandage $N, but fails miserably.", TRUE, ch, 0, vict, TO_NOTVICT);
		damage(vict, vict, 2, TYPE_SUFFERING);
		return;
	}

	act("You successfully bandage $N.", FALSE, ch, 0, vict, TO_CHAR);
	act("$n bandages $N, who looks a bit better now.", TRUE, ch, 0, vict, TO_NOTVICT);
	act("Someone bandages you, and you feel a bit better now.", FALSE, ch, 0, vict, TO_VICT);
	GET_HIT(vict) = 0;
}

ACMD(do_trip)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;
	int percent, prob;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRIP))
	{
		send_to_char(ch, "Você não tem idéia de como fazer isso.\r\n");
		return;
	}
	else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
	{
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
		{
			vict = FIGHTING(ch);
		}
		else
		{
			send_to_char(ch, "Dar uma rasteira em quem?\r\n");
			return;
		}
	}
	else if (vict == ch)
	{
		send_to_char(ch, "Haha... Muito engraçado... Você é palhaço de circo?\r\n");
		return;
	}
	else if ((!CONFIG_PK_ALLOWED && !IS_NPC(vict)))
	{
		act("Use 'murder' se você realmente deseja atacar $N.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	percent = rand_number(1, 101);	/* 101% is a complete failure */
	prob = GET_SKILL(ch, SKILL_TRIP);

	if (MOB_FLAGGED(vict, MOB_NOBASH) || MOB_FLAGGED(vict, MOB_MOUNTABLE) ||
		MOB_FLAGGED(vict, MOB_CAN_FLY))
		percent = 101;

	if (percent > prob)
	{
		damage(ch, vict, 0, SKILL_TRIP);
		if (damage(ch, ch, 2, TYPE_UNDEFINED) > 0 && (GET_LEVEL(ch) < LVL_GOD))
		{
			GET_POS(ch) = POS_SITTING;
			GET_WAIT_STATE(ch) += 3 * PULSE_VIOLENCE;
		}
	}
	else
	{
		if (damage(ch, vict, 2, SKILL_TRIP) > 0)
		{						// -1 = dead, 0 = miss
			GET_WAIT_STATE(vict) += 2 * PULSE_VIOLENCE;
			if (IN_ROOM(ch) == IN_ROOM(vict))
				GET_POS(vict) = POS_SITTING;
		}

		if (GET_LEVEL(ch) < LVL_GOD)
			GET_WAIT_STATE(ch) += 4 * PULSE_VIOLENCE;
	}
}