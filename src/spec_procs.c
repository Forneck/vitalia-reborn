/**************************************************************************
*  File: spec_procs.c                                      Part of tbaMUD *
*  Usage: Implementation of special procedures for mobiles/objects/rooms. *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* For more examples:
   ftp://ftp.circlemud.org/pub/CircleMUD/contrib/snippets/specials */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "modify.h"
#include "spedit.h"
#include "formula.h"
#include "./include/floatfann.h"

/* locally defined functions of local (file) scope */

static const char *how_good(int percent);
static void npc_steal(struct char_data *ch, struct char_data *victim);


static const char *how_good(int percent)
{
	if (percent < 0)
		return " (erro)";
	if (percent == 0)
		return " (nao aprendeu)";
	if (percent <= 10)
		return " (horrivel)";
	if (percent <= 20)
		return " (ruim)";
	if (percent <= 40)
		return " (pobre)";
	if (percent <= 55)
		return " (mediana)";
	if (percent <= 70)
		return " (razoavel)";
	if (percent <= 80)
		return " (bom)";
	if (percent <= 85)
		return " (muito bom)";
	if (percent <= 90)
		return " (otimo)";
	if (percent <= 99)
		return " (excelente)";

	return " (perfeitamente)";
}



#define LEARNED_LEVEL	0		/* % known which is considered "learned" */
#define MAX_PER_PRAC	1		/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2		/* min percent gain in skill per practice */
#define PRAC_TYPE	3			/* should it say 'spell' or 'skill'? */

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])


void list_skills(struct char_data *ch)
{
	const char *overflow = "\r\n**OVERFLOW**\r\n";

	size_t len = 0;

	int vnum = 1;
	int ret;
	struct str_spells *spell;

	char buf[MAX_STRING_LENGTH];

	len = snprintf(buf, sizeof(buf), "You have %d practice session%s remaining.\r\n"
				   "You know the following:\r\n",
				   GET_PRACTICES(ch), GET_PRACTICES(ch) == 1 ? "" : "s");

	spell = get_spell_by_vnum(vnum);
	while (spell)
	{
		int from_myclass = 0, i;

		if (spell->status == available)
		{
			// if LVL_IMMORT bypass class assignement check.
			if (GET_LEVEL(ch) >= LVL_IMMORT)
				from_myclass = 1;
			else
				for (i = 0; i < NUM_CLASSES; i++)
					if ((spell->assign[i].class_num == GET_CLASS(ch) &&
						 (GET_LEVEL(ch) >= spell->assign[i].level)))
					{
						from_myclass = 1;
						break;
					}
			if (from_myclass)
			{
				ret =
					snprintf(buf + len, sizeof(buf) - len, "%-20s %s\r\n", spell->name,
							 how_good(GET_SKILL(ch, spell->vnum)));
				if (ret < 0 || len + ret >= sizeof(buf))
					break;
				len += ret;
			}
		}
		spell = spell->next;
	}
	if (len >= sizeof(buf))
		strcpy(buf + sizeof(buf) - strlen(overflow) - 1, overflow);	// strcpy: 
																	// 
	// OK 

	page_string(ch->desc, buf, TRUE);
}

SPECIAL(guild)
{
	struct str_spells *spell = NULL;

	int class, skill_num, percent, level, rts_code;
	struct obj_data *object;
   struct fann *ann;
   int count_obj;
   int grupo;

	fann_type input[29];
	fann_type output[6];

	if (IS_NPC(ch) || !CMD_IS("practice"))
		return (FALSE);
    
    /*grupo e inventario */
    if (GROUP(ch) != NULL)
		grupo = 1;
	else
		grupo = 0;
	
	for (object= ch->carrying; object; object = object->next_content)
	{
			count_obj++;
	}
	 /*pega dados pro aventureiro */
	input[0] = 1/(1+exp(-GET_HIT(ch)));
	input[1] = 1/(1+exp(-GET_MAX_HIT(ch)));

	input[2] = 1/(1+exp(-GET_MANA(ch)));
	input[3] = 1/(1+exp(-GET_MAX_MANA(ch)));
	input[4] = 1/(1+exp(-GET_MOVE(ch)));
	input[5] = 1/(1+exp(-GET_MAX_MOVE(ch)));
	input[6] = 1/(1+exp(-GET_EXP(ch)));
	input[7] = 1/(1+exp(-GET_ROOM_VNUM(IN_ROOM(ch))));
	input[8] = 1/(1+exp(-GET_CLASS(ch)));
	input[9] = 1/(1+exp(-GET_POS(ch)));
	input[10] = 1/(1+exp(-GET_ALIGNMENT(ch)));
	input[11] = 1/(1+exp(-compute_armor_class(ch)));
	input[12] = 1/(1+exp(-GET_STR(ch)));
	input[13] = 1/(1+exp(-GET_ADD(ch)));
	input[14] = 1/(1+exp(-GET_INT(ch)));
	input[15] = 1/(1+exp(-GET_WIS(ch)));
	input[16] = 1/(1+exp(-GET_CON(ch)));
	input[17] = 1/(1+exp(-GET_DEX(ch)));
	input[18] = 1/(1+exp(-GET_GOLD(ch)));
	input[19] = 1/(1+exp(-GET_BANK_GOLD(ch)));
	input[20] = 1/(1+exp(-GET_COND(ch, HUNGER)));
	input[21] = 1/(1+exp(-GET_COND(ch, THIRST)));
	input[22] = 1/(1+exp(-GET_PRACTICES(ch)));
	input[23] = 1/(1+exp(-grupo));
	//input 24 eh o clan em vez de mudhora
	input[24] = 1/(1+exp(-0));
	input[25] = 1/(1+exp(-GET_BREATH(ch)));
	input[26] = 1/(1+exp(-GET_HITROLL(ch)));
	input[27] = 1/(1+exp(-GET_DAMROLL(ch)));
	input[28] = 1/(1+exp(-count_obj));

	
	output[0] = 646/780;
	output[1] = 1/(1+exp(-CMD_MISC));
	output[2] = 1/(1+exp(-CMD_ARG_SKILL));
	skip_spaces(&argument);

	if (!*argument)
	{
		list_skills(ch);
		return (TRUE);
	}
	if (GET_PRACTICES(ch) <= 0)
	{
		send_to_char(ch, "Voce não pode praticar agora.\r\n");
		return (TRUE);
	}
	spell = get_spell_by_name(argument, SPSK);
	if (!spell)
	{
		log1("SYSERR: spell not found '%s' at the guild.", argument);
		send_to_char(ch, "'%s não existe.\r\n", argument);
		return (TRUE);
	}

	skill_num = spell->vnum;
	level = get_spell_level(skill_num, GET_CLASS(ch));
   output[3] = 1/(1+exp(-skill_num));
   output[4] = output[5] = -1;
   
	if ((level == -1) || (GET_LEVEL(ch) < level))
	{
		send_to_char(ch, "Você não conhece essa %s.\r\n",
					 spell->type == SPELL ? "magia" : "habilidade");
		return (TRUE);
	}
	if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
	{
		send_to_char(ch, "Voce ja conhece o suficiente.\r\n");
		return (TRUE);
	}
	send_to_char(ch, "Voce pratica por um tempo...\r\n");
	GET_PRACTICES(ch)--;

	class = get_spell_class(spell, GET_CLASS(ch));
	percent = GET_SKILL(ch, skill_num);
	if ((class == -1) || !spell->assign[class].prac_gain)
		percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));
	else
		 percent +=
			MAX(5,
				formula_interpreter(ch, ch, skill_num, TRUE, spell->assign[class].prac_gain,
									GET_LEVEL(ch), &rts_code));

	SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

	if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
		send_to_char(ch, "Voce agora conhece o suficiente!\r\n");

 	ann = fann_create_from_file("etc/aventureiro.fann");
 	
  	if (GET_LEVEL(ch) < LVL_GOD)
			fann_train(ann, input, output);

		fann_save(ann, "etc/aventureiro.fann");
		fann_destroy(ann);
	return (TRUE);
}

SPECIAL(dump)
{
	struct obj_data *k;
	int value = 0;

	for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents)
	{
		act("$p desaparece em uma nuvem de fumaca!", FALSE, 0, k, 0, TO_ROOM);
		extract_obj(k);
	}

	if (!CMD_IS("drop"))
		return (FALSE);

	do_drop(ch, argument, cmd, SCMD_DROP);

	for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents)
	{
		act("$p desaparece em uma nuvem de fumaca!", FALSE, 0, k, 0, TO_ROOM);
		value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
		extract_obj(k);
	}

	if (value)
	{
		send_to_char(ch, "Voce merece uma recompensa.\r\n");
		act("$n ganhou uma recompensa pela sua cidadania.", TRUE, ch, 0, 0, TO_ROOM);

		if (GET_LEVEL(ch) < 3)
			gain_exp(ch, value);
		else
			increase_gold(ch, value);
	}
	return (TRUE);
}

SPECIAL(mayor)
{
	char actbuf[MAX_INPUT_LENGTH];

	const char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
	const char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

	static const char *path = NULL;
	static int path_index;
	static bool move = FALSE;

	if (!move)
	{
		if (time_info.hours == 6)
		{
			move = TRUE;
			path = open_path;
			path_index = 0;
		}
		else if (time_info.hours == 20)
		{
			move = TRUE;
			path = close_path;
			path_index = 0;
		}
	}
	if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) || (GET_POS(ch) == POS_FIGHTING))
		return (FALSE);

	switch (path[path_index])
	{
	case '0':
	case '1':
	case '2':
	case '3':
		perform_move(ch, path[path_index] - '0', 1);
		break;

	case 'W':
		GET_POS(ch) = POS_STANDING;
		act("$n acorda e resmunga alto.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'S':
		GET_POS(ch) = POS_SLEEPING;
		act("$n deita e comeca a dormir.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'a':
		act("$n fala 'Ola Docinho!'", FALSE, ch, 0, 0, TO_ROOM);
		act("$n lambe os labios.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'b':
		act("$n fala 'Que vista! Preciso fazer algo sobre o lixao!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'c':
		act("$n fala 'Vandalos!  Os jovens hoje nao tem respeito por nada!'",
			FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'd':
		act("$n fala 'Bom dia, amados cidadoes!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'e':
		act("$n fala 'Eu declaro o bazar aberto!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'E':
		act("$n fala 'Eu declaro Midgaard fechada!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'O':
		do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK);	/* strcpy: 
																	   OK */
		do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);	/* strcpy: OK */
		break;

	case 'C':
		do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE);	/* strcpy: OK */
		do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);	/* strcpy: OK */
		break;

	case '.':
		move = FALSE;
		break;

	}

	path_index++;
	return (FALSE);
}

/* General special procedures for mobiles. */

static void npc_steal(struct char_data *ch, struct char_data *victim)
{
	int gold;

	if (IS_NPC(victim))
		return;
	if (GET_LEVEL(victim) >= LVL_IMMORT)
		return;
	if (!CAN_SEE(ch, victim))
		return;

	if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0))
	{
		act("Voce descobre que $n tinha as maos na sua carteira.", FALSE, ch, 0, victim, TO_VICT);
		act("$n tenta roubar ouro de $N.", TRUE, ch, 0, victim, TO_NOTVICT);
	}
	else
	{
		/* Steal some gold coins */
		gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
		if (gold > 0)
		{
			increase_gold(ch, gold);
			decrease_gold(victim, gold);
		}
	}
}

/* Quite lethal to low-level characters. */
SPECIAL(snake)
{
	if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
		return (FALSE);

	if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
		return (FALSE);

	act("$n morde $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
	act("$n morde voce!", 1, ch, 0, FIGHTING(ch), TO_VICT);
	call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
	return (TRUE);
}

SPECIAL(thief)
{
	struct char_data *cons;

	if (cmd || GET_POS(ch) != POS_STANDING)
		return (FALSE);

	for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
		if (!IS_NPC(cons) && GET_LEVEL(cons) < LVL_IMMORT && !rand_number(0, 4))
		{
			npc_steal(ch, cons);
			return (TRUE);
		}

	return (FALSE);
}

SPECIAL(magic_user)
{
	struct char_data *vict;

	if (cmd || GET_POS(ch) != POS_FIGHTING)
		return (FALSE);

	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
		if (FIGHTING(vict) == ch && !rand_number(0, 4))
			break;

	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
		vict = FIGHTING(ch);

	/* Hm...didn't pick anyone...I'll wait a round. */
	if (vict == NULL)
		return (TRUE);

	if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
		cast_spell(ch, vict, NULL, SPELL_POISON);

	if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
		cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

	if (GET_LEVEL(ch) > 12 && rand_number(0, 12) == 0)
	{
		if (IS_EVIL(ch))
			cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
		else if (IS_GOOD(ch))
			cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
	}

	if (rand_number(0, 4))
		return (TRUE);

	switch (GET_LEVEL(ch))
	{
	case 4:
	case 5:
		cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
		break;
	case 6:
	case 7:
		cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
		break;
	case 8:
	case 9:
		cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
		break;
	case 10:
	case 11:
		cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
		break;
	case 12:
	case 13:
		cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
		break;
	default:
		cast_spell(ch, vict, NULL, SPELL_FIREBALL);
		break;
	}
	return (TRUE);
}

/* Special procedures for mobiles. */
SPECIAL(guild_guard)
{
	int i, direction;
	struct char_data *guard = (struct char_data *)me;
	const char *buf = "O guarda humilia voce e bloqueia sua passagem.\r\n";
	const char *buf2 = "O guarda humilia $n e bloqueia a passagem.";

	if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
		return (FALSE);

	if (GET_LEVEL(ch) >= LVL_IMMORT)
		return (FALSE);

	/* find out what direction they are trying to go */
	for (direction = 0; direction < NUM_OF_DIRS; direction++)
		if (!strcmp(cmd_info[cmd].command, dirs[direction]))
			for (direction = 0; direction < DIR_COUNT; direction++)
				if (!strcmp(cmd_info[cmd].command, dirs[direction]) ||
					!strcmp(cmd_info[cmd].command, autoexits[direction]))
					break;

	for (i = 0; guild_info[i].guild_room != NOWHERE; i++)
	{
		/* Wrong guild. */
		if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room)
			continue;

		/* Wrong direction. */
		if (direction != guild_info[i].direction)
			continue;

		/* Allow the people of the guild through. */
		if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i].pc_class)
			continue;

		send_to_char(ch, "%s", buf);
		act(buf2, FALSE, ch, 0, 0, TO_ROOM);
		return (TRUE);
	}
	return (FALSE);
}

SPECIAL(puff)
{
	char actbuf[MAX_INPUT_LENGTH];

	if (cmd)
		return (FALSE);

	switch (rand_number(0, 60))
	{
	case 0:
		do_say(ch, strcpy(actbuf, "Meu Deus! Quantas estrelas!"), 0, 0);	/* strcpy: 
																			   OK 
																			 */
		return (TRUE);
	case 1:
		do_say(ch, strcpy(actbuf, "Como estes peixes vieram para aqui?"), 0, 0);	/* strcpy: 
																					   OK 
																					 */
		return (TRUE);
	case 2:
		do_say(ch, strcpy(actbuf, "Eu sou muito feminina."), 0, 0);	/* strcpy: 
																	   OK */
		return (TRUE);
	case 3:
		do_say(ch, strcpy(actbuf, "Eu tenho este sentimento muito pacifico."), 0, 0);	/* strcpy: 
																						   OK 
																						 */
		return (TRUE);
	default:
		return (FALSE);
	}
}

SPECIAL(fido)
{
	struct obj_data *i, *temp, *next_obj;

	if (cmd || !AWAKE(ch))
		return (FALSE);

	for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
	{
		if (!IS_CORPSE(i))
			continue;

		act("$n devora um corpo selvagemente.", FALSE, ch, 0, 0, TO_ROOM);
		for (temp = i->contains; temp; temp = next_obj)
		{
			next_obj = temp->next_content;
			obj_from_obj(temp);
			obj_to_room(temp, IN_ROOM(ch));
		}
		extract_obj(i);
		return (TRUE);
	}
	return (FALSE);
}

SPECIAL(janitor)
{
	struct obj_data *i;

	if (cmd || !AWAKE(ch))
		return (FALSE);

	for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
	{
		if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
			continue;
		if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
			continue;
		act("$n recolhe um pouco de lixo.", FALSE, ch, 0, 0, TO_ROOM);
		obj_from_room(i);
		obj_to_char(i, ch);
		return (TRUE);
	}
	return (FALSE);
}

SPECIAL(cityguard)
{
	struct char_data *tch, *evil, *spittle;
	int max_evil, min_cha;

	if (cmd || !AWAKE(ch) || FIGHTING(ch))
		return (FALSE);

	max_evil = 1000;
	min_cha = 6;
	spittle = evil = NULL;

	for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
	{
		if (!CAN_SEE(ch, tch))
			continue;
		if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER))
		{
			act("$n grita 'HEY!!!  Voce e' um daqueles malditos PKS!!!!!!'", FALSE, ch, 0, 0,
				TO_ROOM);
			hit(ch, tch, TYPE_UNDEFINED);
			return (TRUE);
		}

		if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF))
		{
			act("$n grita 'HEY!!!  Voce e' um daqueles malditos LADROES!!!!!!'", FALSE, ch, 0, 0,
				TO_ROOM);
			hit(ch, tch, TYPE_UNDEFINED);
			return (TRUE);
		}

		if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil
			&& (IS_NPC(tch) || IS_NPC(FIGHTING(tch))))
		{
			max_evil = GET_ALIGNMENT(tch);
			evil = tch;
		}

		if (GET_CHA(tch) < min_cha)
		{
			spittle = tch;
			min_cha = GET_CHA(tch);
		}
	}

	if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0)
	{
		act("$n grita 'PROTEJAM OS INOCENTES!!  BANZAI!  AVANTE!  ARARARAGGGHH!'", FALSE, ch, 0, 0,
			TO_ROOM);
		hit(ch, evil, TYPE_UNDEFINED);
		return (TRUE);
	}

	/* Reward the socially inept. */
	if (spittle && !rand_number(0, 9))
	{
		static int spit_social;

		if (!spit_social)
			spit_social = find_command("spit");

		if (spit_social > 0)
		{
			char spitbuf[MAX_NAME_LENGTH + 1];
			strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf));	/* strncpy: 
																	   OK */
			spitbuf[sizeof(spitbuf) - 1] = '\0';
			do_action(ch, spitbuf, spit_social, 0);
			return (TRUE);
		}
	}
	return (FALSE);
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)
SPECIAL(pet_shops)
{
	char buf[MAX_STRING_LENGTH], pet_name[256];
	room_rnum pet_room;
	struct char_data *pet;

	/* Gross. */
	pet_room = IN_ROOM(ch) + 1;

	if (CMD_IS("list"))
	{
		send_to_char(ch, "Pets disponiveis:\r\n");
		for (pet = world[pet_room].people; pet; pet = pet->next_in_room)
		{
			/* No, you can't have the Implementor as a pet if he's in there. */
			if (!IS_NPC(pet))
				continue;
			send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
		}
		return (TRUE);
	}
	else if (CMD_IS("buy"))
	{

		two_arguments(argument, buf, pet_name);

		if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet))
		{
			send_to_char(ch, "Nao vamos ter este pet!\r\n");
			return (TRUE);
		}
		if (GET_GOLD(ch) < PET_PRICE(pet))
		{
			send_to_char(ch, "Voce nao tem tanto dinheiro!\r\n");
			return (TRUE);
		}
		decrease_gold(ch, PET_PRICE(pet));

		pet = read_mobile(GET_MOB_RNUM(pet), REAL);
		GET_EXP(pet) = 0;
		SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

		if (*pet_name)
		{
			snprintf(buf, sizeof(buf), "%s %s", pet->player.name, pet_name);
			/* free(pet->player.name); don't free the prototype! */
			pet->player.name = strdup(buf);

			snprintf(buf, sizeof(buf),
					 "%s Uma pequena placa com corrente no pescoco com 'Me chamam de %s'\r\n",
					 pet->player.description, pet_name);
			/* free(pet->player.description); don't free the prototype! */
			pet->player.description = strdup(buf);
		}
		char_to_room(pet, IN_ROOM(ch));
		add_follower(pet, ch);

		/* Be certain that pets can't get/carry/use/wield/wear items */
		IS_CARRYING_W(pet) = 1000;
		IS_CARRYING_N(pet) = 100;

		send_to_char(ch, "Obrigado por comprar conosco.\r\n");
		act("$n comprou $N como pet.", FALSE, ch, 0, pet, TO_ROOM);

		return (TRUE);
	}

	/* All commands except list and buy */
	return (FALSE);
}

/* Special procedures for objects. */
/* SPECIAL(bank) { int amount, amount2; char arg[MAX_INPUT_LENGTH]; amount2 =
   atoi(argument); half_chop(argument, arg, argument);

   if (CMD_IS("balance")) { if (GET_BANK_GOLD(ch) > 0) send_to_char(ch, "O seu 
   saldo atual e' de %d moedas.\r\n", GET_BANK_GOLD(ch)); else
   send_to_char(ch, "O seu saldo esta zerado.\r\n"); return (TRUE); } else if
   (CMD_IS("deposit")) { if (!strcmp(arg, "all")) amount = GET_GOLD(ch); else
   amount = amount2; if (amount <= 0) { send_to_char(ch, "Quanto voce quer
   depositar?\r\n"); return (TRUE); } if (GET_GOLD(ch) < amount) {
   send_to_char(ch, "Voce nao tem tantas moedas!\r\n"); return (TRUE); }
   decrease_gold(ch, amount); increase_bank(ch, amount); send_to_char(ch,
   "Voce deposita %d moedas.\r\n", amount); act("$n fez uma transacao
   bancaria.", TRUE, ch, 0, FALSE, TO_ROOM); return (TRUE); } else if
   (CMD_IS("withdraw")) { if (!strcmp(arg, "all")) amount = GET_BANK_GOLD(ch);
   else amount = amount2; if (amount <= 0) { send_to_char(ch, "Quanto voce quer 
   sacar?\r\n"); return (TRUE); } if (GET_BANK_GOLD(ch) < amount) {
   send_to_char(ch, "Voce nao tem tanto dinheiro no banco!\r\n"); return
   (TRUE); } increase_gold(ch, amount); decrease_bank(ch, amount);
   send_to_char(ch, "Voce saca %d moedas.\r\n", amount); act("$n fez uma
   transacao bancaria.", TRUE, ch, 0, FALSE, TO_ROOM); return (TRUE); } else
   return (FALSE); } */
/* Special procedures for objects. */
SPECIAL(bank)
{
	int amount;

	if (CMD_IS("balance"))
	{
		if (GET_BANK_GOLD(ch) > 0)
			send_to_char(ch, "Your current balance is %d coins.\r\n", GET_BANK_GOLD(ch));
		else
			send_to_char(ch, "You currently have no money deposited.\r\n");
		return (TRUE);
	}
	else if (CMD_IS("deposit"))
	{
		if ((amount = atoi(argument)) <= 0)
		{
			send_to_char(ch, "How much do you want to deposit?\r\n");
			return (TRUE);
		}
		if (GET_GOLD(ch) < amount)
		{
			send_to_char(ch, "You don't have that many coins!\r\n");
			return (TRUE);
		}
		decrease_gold(ch, amount);
		increase_bank(ch, amount);
		send_to_char(ch, "You deposit %d coins.\r\n", amount);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (TRUE);
	}
	else if (CMD_IS("withdraw"))
	{
		if ((amount = atoi(argument)) <= 0)
		{
			send_to_char(ch, "How much do you want to withdraw?\r\n");
			return (TRUE);
		}
		if (GET_BANK_GOLD(ch) < amount)
		{
			send_to_char(ch, "You don't have that many coins deposited!\r\n");
			return (TRUE);
		}
		increase_gold(ch, amount);
		decrease_bank(ch, amount);
		send_to_char(ch, "You withdraw %d coins.\r\n", amount);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (TRUE);
	}
	else
		return (FALSE);
}