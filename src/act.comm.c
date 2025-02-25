/**************************************************************************
*  File: act.comm.c                                        Part of tbaMUD *
*  Usage: Player-level communication commands.                            *
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
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "act.h"
#include "modify.h"

bool legal_communication(char *arg);

bool legal_communication(char *arg)
{
	while (*arg)
	{
		if (*arg == '@')
		{
			arg++;
			if (*arg == '(' || *arg == ')' || *arg == '<' || *arg == '>')
				return FALSE;
		}
		arg++;
	}
	return TRUE;
}

ACMD(do_say)
{

	skip_spaces(&argument);

	if (!*argument)
		send_to_char(ch, "Sim, mas O QUÊ você deseja dizer?\r\n");
	else
	{
		char buf[MAX_INPUT_LENGTH + 14], *msg;
		struct char_data *vict;

		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
			parse_at(argument);

		snprintf(buf, sizeof(buf), "$n\tn  disse, '%s'", argument);
		if (IS_DEAD(ch))		/* IMPLEMENTED */
			msg = act(buf, FALSE, ch, 0, 0, TO_DEAD);
		else
			msg = act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);

		for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
			if (vict != ch && GET_POS(vict) > POS_SLEEPING)
				add_history(vict, msg, HIST_SAY);

		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(ch, "%s", CONFIG_OK);
		else
		{
			sprintf(buf, "Você disse, '%s'", argument);
			msg = act(buf, FALSE, ch, 0, 0, TO_CHAR | DG_NO_TRIG);
			add_history(ch, msg, HIST_SAY);
		}

		/* Trigger check. */
		speech_mtrigger(ch, argument);
		speech_wtrigger(ch, argument);
	}
}

ACMD(do_gsay)
{
	skip_spaces(&argument);

	if (IS_DEAD(ch))
	{
		act("Sinto muito, mas você está mort$r.", TRUE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!GROUP(ch))
	{
		send_to_char(ch, "Mas você não é membro de um grupo!\r\n");
		return;
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && GET_LEVEL(ch) < LVL_DEMIGOD)
	{
		send_to_char(ch, "As paredes parecem absorver seus pensamentos.\r\n");
		return;
	}

	if (!*argument)
		send_to_char(ch, "Sim, mas O QUÊ você quer falar para o grupo?\r\n");
	else
	{

		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
			parse_at(argument);

		send_to_group(ch, ch->group, "%s%s%s falou por telepatia com o grupo,'%s'%s\r\n",
					  CCGRN(ch, C_NRM), CCGRN(ch, C_NRM), GET_NAME(ch), argument, CCNRM(ch,
																						C_NRM));

		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(ch, "%s", CONFIG_OK);
		else
			send_to_char(ch, "%sVocê fala com o grupo, '%s'%s\r\n", CCGRN(ch, C_NRM), argument,
						 CCNRM(ch, C_NRM));
	}
}

static void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
	char buf[MAX_STRING_LENGTH], *msg;

	snprintf(buf, sizeof(buf), "%s$n falou telepaticamente com você, '%s'%s", CCBLU(vict, C_NRM),
			 arg, CCNRM(vict, C_NRM));
	msg = act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
	add_history(vict, msg, HIST_TELL);

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(ch, "%s", CONFIG_OK);
	else
	{
		snprintf(buf, sizeof(buf), "%sVocê fala telepaticamente com $N, '%s'%s",
				 CCBLU(ch, C_NRM), arg, CCNRM(ch, C_NRM));
		msg = act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		add_history(ch, msg, HIST_TELL);
	}

	if (!IS_NPC(vict) && !IS_NPC(ch))
		GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

static int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
	if (!ch)
		log1("SYSERR: is_tell_ok called with no characters");
	else if (!vict)
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (ch == vict)
		act("Você tentou dizer algo para si mesm$r.", FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
	else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
		send_to_char(ch, "Você precisa desligar o notell primeiro.\r\n");
	else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
		send_to_char(ch, "Este lugar parece absorver seus pensamentos.");
	else if (!IS_NPC(vict) && !vict->desc)	/* linkless */
		act("$N está sem conexão no momento.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (PLR_FLAGGED(vict, PLR_WRITING))
		act("$L está escrevendo uma mensagem no momento. Tente novamente mais tarde.", FALSE, ch,
			0, vict, TO_CHAR | TO_SLEEP);
	else if (PRF_FLAGGED(vict, PRF_AFK))
		act("$N parece estar em outra realidade...", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL))
			 || (ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD)))
		act("$N não consegue lhe ouvir.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else
		return (TRUE);

	return (FALSE);
}

/* Yes, do_tell probably could be combined with whisper and ask, but it is
   called frequently, and should IMHO be kept as tight as possible. */
ACMD(do_tell)
{
	struct char_data *vict = NULL;
	char buf[MAX_INPUT_LENGTH + 25], buf2[MAX_INPUT_LENGTH];
	// +25 to make 
	// room for
	// constants

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
		send_to_char(ch, "Com quem você deseja falar? E o quê?\r\n");
	else if (!str_cmp(buf, "m-w"))
	{
#ifdef CIRCLE_WINDOWS
		/* getpid() is not portable */
		send_to_char(ch, "Desculpe, mas não está disponivel.\r\n");
#else /* all other configurations */
		char word[MAX_INPUT_LENGTH], *p, *q;

		if (last_webster_teller != -1L)
		{
			if (GET_IDNUM(ch) == last_webster_teller)
			{
				send_to_char(ch, "Você ainda está esperando uma resposta.\r\n");
				return;
			}
			else
			{
				send_to_char(ch, "Espere, m-w está ocupado! Tente novamente depois.\r\n");
				return;
			}
		}

		/* Only a-z and +/- allowed. */
		for (p = buf2, q = word; *p; p++)
			if ((LOWER(*p) <= 'z' && LOWER(*p) >= 'a') || (*p == '+') || (*p == '-'))
				*q++ = *p;

		*q = '\0';

		if (!*word)
		{
			send_to_char(ch, "Desculpe, mas apenas letras são permitidas.\r\n");
			return;
		}
		snprintf(buf, sizeof(buf), "../lib/bin/webster %s %d &", word, (int)getpid());
		last_webster_teller = GET_IDNUM(ch);
		send_to_char(ch, "Você procura '%s' no dicionario.\r\n", word);
#endif /* platform specific part */
	}
	else if (GET_LEVEL(ch) < LVL_IMMORT
			 && !(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (GET_LEVEL(ch) >= LVL_IMMORT && !(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (is_tell_ok(ch, vict))
	{
		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
			parse_at(buf2);

		perform_tell(ch, vict, buf2);
	}
}

ACMD(do_reply)
{
	struct char_data *tch = character_list;

	if (IS_NPC(ch))
		return;

	skip_spaces(&argument);

	if (GET_LAST_TELL(ch) == NOBODY)
		send_to_char(ch, "Você não tem ninguém para responder!\r\n");
	else if (!*argument)
		send_to_char(ch, "O que você deseja responder?\r\n");
	else
	{
		/* Make sure the person you're replying to is still playing by
		   searching for them.  Note, now last tell is stored as player IDnum
		   instead of a pointer, which is much better because it's safer, plus 
		   will still work if someone logs out and back in again. A descriptor 
		   list based search would be faster although we could not find link
		   dead people. Not that they can hear tells anyway. :) -gg 2/24/98 */
		while (tch && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
			tch = tch->next;

		if (!tch)
			send_to_char(ch, "Esta pessoa já não está mais jogando.\r\n");
		else if (is_tell_ok(ch, tch))
		{
			if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
				parse_at(argument);
			perform_tell(ch, tch, argument);
		}
	}
}

ACMD(do_spec_comm)
{
	char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
	struct char_data *vict;
	const char *action_sing, *action_plur, *action_others, *action_verb;

	switch (subcmd)
	{
	case SCMD_WHISPER:
		action_sing = "sussurra";
		action_plur = "sussurra";
		action_others = "$n sussurra algo para $N.";
		action_verb = "sussurrar";
		break;

	case SCMD_ASK:
		action_sing = "pergunta";
		action_plur = "pergunta";
		action_others = "$n pergunta algo para $N.";
		action_verb = "perguntar";
		break;

	default:
		action_sing = "trava a lingua";
		action_plur = "trava a lingua";
		action_others = "$n se atrapalha tentando falar algo para $N.";
		action_verb = "atrapalhar";
		break;
	}

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
		send_to_char(ch, "Para quem você deseja %s.. e oquê??\r\n", action_verb);
	else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (IS_DEAD(ch))
		send_to_char(ch, "É dificil %s quando você está morto...\r\n", action_verb);
	else if (vict == ch)
		send_to_char(ch, "Você não consegue esticar a sua boca até sua propria orelha...\r\n");
	else
	{
		char buf1[MAX_STRING_LENGTH];

		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
			parse_at(buf2);

		snprintf(buf1, sizeof(buf1), "$n %s para você, '%s'", action_plur, buf2);
		act(buf1, FALSE, ch, 0, vict, TO_VICT);

		if ((!IS_NPC(ch)) && (PRF_FLAGGED(ch, PRF_NOREPEAT)))
			send_to_char(ch, "%s", CONFIG_OK);
		else
			send_to_char(ch, "Você %s para %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);

		act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);

	}
}

ACMD(do_write)
{
	struct obj_data *paper, *pen = NULL;
	char *papername, *penname;
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

	papername = buf1;
	penname = buf2;

	two_arguments(argument, papername, penname);

	if (!ch->desc)
		return;

	if (!*papername)
	{
		/* Nothing was delivered. */
		send_to_char(ch,
					 "Escrever? Com o quê?  Aonde?  O quê você está tentando fazer?!?\r\n");
		return;
	}

	if IS_DEAD
		(ch)
	{
		act("Você não pode escrever enquanto estiver mort$r, certo?", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (*penname)
	{
		/* Nothing was delivered. */
		if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying)))
		{
			send_to_char(ch, "Você não tem %s.\r\n", papername);
			return;
		}
		if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying)))
		{
			send_to_char(ch, "Você não tem %s.\r\n", penname);
			return;
		}
	}
	else
	{							/* There was one arg.. let's see what we can
								   find. */
		if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying)))
		{
			send_to_char(ch, "Não há nenhum %s no seu inventario.\r\n", papername);
			return;
		}
		if (GET_OBJ_TYPE(paper) == ITEM_PEN)
		{						/* Oops, a pen. */
			pen = paper;
			paper = NULL;
		}
		else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
		{
			send_to_char(ch, "Isso não foi feito para ter algo escrito.\r\n");
			return;
		}

		/* One object was found.. now for the other one. */
		if (!GET_EQ(ch, WEAR_HOLD))
		{
			send_to_char(ch, "Você não pode escrever apenas com um(a) %s.\r\n", papername);
			return;
		}
		if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD)))
		{
			send_to_char(ch, "A coisa que você está segurando está invisível!  Eca!!\r\n");
			return;
		}
		if (pen)
			paper = GET_EQ(ch, WEAR_HOLD);
		else
			pen = GET_EQ(ch, WEAR_HOLD);
	}

	/* Now let's see what kind of stuff we've found. */
	if (GET_OBJ_TYPE(pen) != ITEM_PEN)
		act("$p não serve para escrever.", FALSE, ch, pen, 0, TO_CHAR);
	else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
		act("$p não foi feito para ter algo escrito.", FALSE, ch, paper, 0, TO_CHAR);
	else
	{
		char *backstr = NULL;

		/* Something on it, display it as that's in input buffer. */
		if (paper->action_description)
		{
			backstr = strdup(paper->action_description);
			send_to_char(ch, "Já tem algo escrito ai:\r\n");
			send_to_char(ch, "%s", paper->action_description);
		}

		/* We can write. */
		act("$n começa a escrever uma anotação.", TRUE, ch, 0, 0, TO_ROOM);
		send_editor_help(ch->desc);
		string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
	}
}

ACMD(do_page)
{
	struct descriptor_data *d;
	struct char_data *vict;
	char buf2[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

	half_chop(argument, arg, buf2);

	if (IS_NPC(ch))
		send_to_char(ch, "Monstros não podem paginar... vá embora.\r\n");
	else if (!*arg)
		send_to_char(ch, "Quem você gostaria de paginar?\r\n");
	else
	{
		char buf[MAX_STRING_LENGTH];

		snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
		if (!str_cmp(arg, "all") || !str_cmp(arg, "todos"))
		{
			if (GET_LEVEL(ch) > LVL_GOD)
			{
				for (d = descriptor_list; d; d = d->next)
					if (STATE(d) == CON_PLAYING && d->character)
						act(buf, FALSE, ch, 0, d->character, TO_VICT);
			}
			else
				send_to_char(ch, "Você não é onipotente suficiente para fazer isso!\r\n");
			return;
		}
		if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
		{
			act(buf, FALSE, ch, 0, vict, TO_VICT);
			if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
				send_to_char(ch, "%s", CONFIG_OK);
			else
				act(buf, FALSE, ch, 0, vict, TO_CHAR);
		}
		else
			send_to_char(ch, "Esta pessoa não está jogando!\r\n");
	}
}

	/* Generalized communication function by Fred C. Merkel (Torg). */
ACMD(do_gen_comm)
{
	struct descriptor_data *i;
	char color_on[24];
	char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH + 50], *msg;	// +
	// 50
	// to 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// make
	// room
	// for
	// color
	// codes
	bool emoting = FALSE;

	/* Array of flags which must _not_ be set in order for comm to be heard. */
	int channels[] = {
	   0,
		PRF_NOSHOUT,
		PRF_NOGOSS,
		PRF_NOAUCT,
		PRF_NOGRATZ,
		PRF_NOGOSS,
		0
	};

	int hist_type[] = {
		HIST_SHOUT,
		HIST_GOSSIP,
		HIST_AUCTION,
		HIST_GRATS,
		HIST_PRAY,
	};

	/* com_msgs: [0] Message if you can't perform the action because of
	   noshout [1] name of the action [2] message if you're not on the channel 
	   [3] a color string. */
	const char *com_msgs[][4] = {
		{"Você não pode gritar!\r\n",
		 "grita",
		 "Desligue o noshout antes!\r\n",
		 KYEL},

		{"Você não pode tagarelar!!\r\n",
		 "tagarela",
		 "Você não está neste canal!\r\n",
		 KYEL},

		{"Você não pode leiloar!!\r\n",
		 "leiloa",
		 "Você não está neste canal!\r\n",
		 KMAG},

		{"Você não pode parabenizar!\r\n",
		 "parabeniza",
		 "Você nem está neste canal!\r\n",
		 KGRN},

		{"Você não pode tagarelar as tuas emocoes!\r\n",
		 "tagarela",
		 "Você nem está neste canal!\r\n",
		 KYEL}
	};

	if (PLR_FLAGGED(ch, PLR_NOSHOUT))
	{
		send_to_char(ch, "%s", com_msgs[subcmd][0]);
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
	{
		send_to_char(ch, "As paredes parecem absorver suas palavras\r\n");
		return;
	}


	if (subcmd == SCMD_GEMOTE)
	{
		if (!*argument)
			send_to_char(ch, "Gemote? Sim? mas gemote oquê?\r\n");
		else
			do_gmote(ch, argument, 0, 1);
		return;
	}

	/* Level_can_shout defined in config.c. */
	if (GET_LEVEL(ch) < CONFIG_LEVEL_CAN_SHOUT)
	{
		send_to_char(ch, "Você precisa estar no nivel %d para poder %sr.\r\n",
					 CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
		return;
	}
	/* Make sure the char is on the channel. */
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, channels[subcmd]))
	{
		send_to_char(ch, "%s", com_msgs[subcmd][2]);
		return;
	}

	/* skip leading spaces */
	skip_spaces(&argument);

	/* Make sure that there is something there to say! */
	if (!*argument)
	{
		send_to_char(ch, "Sim, %sr, nós devemos %sr mas OQUE???\r\n", com_msgs[subcmd][1],
					 com_msgs[subcmd][1]);
		return;
	}

	/* Set up the color on code. */
	strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

	/* First, set up strings to be given to the communicator. */
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(ch, "%s", CONFIG_OK);
	else
	{
		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
			parse_at(argument);

		snprintf(buf1, sizeof(buf1), "%sVocê %s, '%s%s'%s",
				 COLOR_LEV(ch) >= C_CMP ? color_on : "", com_msgs[subcmd][1], argument,
				 COLOR_LEV(ch) >= C_CMP ? color_on : "", CCNRM(ch, C_CMP));

		msg = act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
		add_history(ch, msg, hist_type[subcmd]);
	}
	if (!emoting)
		snprintf(buf1, sizeof(buf1), "$n %s, '%s'", com_msgs[subcmd][1], argument);

	/* Now send all the strings out. */
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING || i == ch->desc || !i->character)
			continue;
		if (!IS_NPC(ch)
			&& (PRF_FLAGGED(i->character, channels[subcmd])
				|| PLR_FLAGGED(i->character, PLR_WRITING)))
			continue;

		if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
			continue;

		if (subcmd == SCMD_SHOUT
			&& ((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone)
				|| !AWAKE(i->character)))
			continue;

		snprintf(buf2, sizeof(buf2), "%s%s%s",
				 (COLOR_LEV(i->character) >= C_NRM) ? color_on : "", buf1, KNRM);
		msg = act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
		add_history(i->character, msg, hist_type[subcmd]);
	}
}

ACMD(do_qcomm)
{
	if (!PRF_FLAGGED(ch, PRF_QUEST))
	{
		send_to_char(ch, "Você não faz parte do grupo de busca (quest)!\r\n");
		return;
	}
	skip_spaces(&argument);

	if (!*argument)
		send_to_char(ch, "Sim, legal, mas O QUÊ??\r\n");
	else
	{
		char buf[MAX_STRING_LENGTH];
		struct descriptor_data *i;

		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
			parse_at(argument);

		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(ch, "%s", CONFIG_OK);
		else if (subcmd == SCMD_QSAY)
		{
			snprintf(buf, sizeof(buf), "Você disse para o grupo de \"quest\", '%s'", argument);
			act(buf, FALSE, ch, 0, argument, TO_CHAR);
		}
		else
			act(argument, FALSE, ch, 0, argument, TO_CHAR);

		if (subcmd == SCMD_QSAY)
			snprintf(buf, sizeof(buf), "$n disse para o grupo de \"quest\", '%s'", argument);
		else
		{
			strlcpy(buf, argument, sizeof(buf));
			mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s qechoed: %s",
				   GET_NAME(ch), argument);
		}
		for (i = descriptor_list; i; i = i->next)
			if (STATE(i) == CON_PLAYING && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
				act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
	}
}

ACMD(do_pray)
{

	skip_spaces(&argument);

	if (!*argument)
		send_to_char(ch, "Rezar? O quê?\r\n");
	else
	{
		char buf[MAX_INPUT_LENGTH + 14], buf1[MAX_INPUT_LENGTH + 14], *msg;
     	struct descriptor_data *d;
     
     
		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
			parse_at(argument);

		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(ch, "%s", CONFIG_OK);
		else
		{
			sprintf(buf, "Você reza, '%s'", argument);
			msg = act(buf, FALSE, ch, 0, 0, TO_CHAR | DG_NO_TRIG);

			add_history(ch, msg, HIST_PRAY);

			act("$n se ajoelha e conversa com os Deuses.", FALSE, ch, NULL, NULL, TO_ROOM);

			if (PLR_FLAGGED(ch, PLR_HERETIC))
			{
				send_to_char(ch,
							 "Você tem a nítida impressão de que suas preces não serão ouvidas...\r\n");
				return;
			}
			snprintf(buf1, sizeof(buf1), "%s: <%d> reza, '%s'\r\n", 			GET_NAME(ch), GET_LEVEL(ch), argument);

			for (d = descriptor_list; d; d = d->next)
			{
				if (IS_PLAYING(d) && (GET_LEVEL(d->character) >= LVL_GOD) &&
					(!PRF_FLAGGED(d->character, PRF_NOWIZ)) && (d != ch->desc))
				{
					parse_at(buf1);
					send_to_char(d->character, "%s%s%s",buf1,CBGRN(d->character,C_SPR),CCNRM(d->character,C_SPR));
					add_history(d->character, buf1, HIST_PRAY);
				}
			}
		}
	}
}
