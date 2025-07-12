/**************************************************************************
*  File: fight.c                                           Part of tbaMUD *
*  Usage: Combat system.                                                  *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "shop.h"
#include "quest.h"
#include "spedit.h"
#include "genolc.h"
#include "genzon.h"

	/* locally defined global variables, used externally */
	/* head of l-list of fighting chars */

struct char_data *combat_list = NULL;

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"golpeia",	"golpear",	"seu",	"o",	"golpe"},	//  0: hit
  {"ferroa",	"ferroar",	"sua",	"a",	"ferroada"},	//  1: sting
  {"chicoteia",	"chicotear",	"sua",	"a",	"chicotada"},	//  2: whip
  {"retalha",	"retalhar",	"sua",	"a",	"retalhada"},	//  3: slash
  {"morde",	"morder",	"sua",	"a",	"mordida"},	//  4: bite
  {"caceteia",	"cacetear",	"sua",	"a",	"cacetada"},	//  5: bludgeon
  {"esmaga",	"esmagar",	"sua",	"a",	"esmagada"},	//  6: crush
  {"moe",	"moer",		"sua",	"a",	"moída"},	//  7: pound
  {"arranha",	"arranhar",	"sua",	"a",	"arranhada"},	//  8: claw
  {"espanca",	"espancar",	"sua",	"a",	"espancada"},	//  9: maul
  {"açoita",	"açoitar",	"sua",	"a",	"açoitada"},	// 10: thrash
  {"fura",	"furar",	"sua",	"a",	"furada"},	// 11: pierce
  {"explode",	"explodir",	"sua",	"a",	"explosão"},	// 12: blast
  {"esmurra",	"esmurrar",	"sua",	"a",	"esmurrada"},	// 13: punch
  {"esfaqueia",	"esfaquear",	"sua",	"a",	"esfaqueada"},	// 14: stab
  {"perfura",	"perfurar",	"sua",	"a",	"perfurada"},	// 15: bore
  {"espeta",	"espetar",	"sua",	"a",	"espetada"},	// 16: broach
  {"corta",	"cortar",	"sua",	"a",	"cortada"}	// 17: mow
};

/* local (file scope only) variables */
static struct char_data *next_combat_list = NULL;

/* local file scope utility functions */
static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
static void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type);
static void make_corpse(struct char_data *ch);
static void change_alignment(struct char_data *ch, struct char_data *victim);
static void group_gain(struct char_data *ch, struct char_data *victim);
static void solo_gain(struct char_data *ch, struct char_data *victim);
/** @todo refactor this function name */
static char *replace_string(const char *str, const char *weapon_singular,
							const char *weapon_plural);
int get_weapon_prof(struct char_data *ch, struct obj_data *wield);
int get_nighthammer(struct char_data *ch, bool real);
int attacks_per_round(struct char_data *ch);

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))
/* The Fight related routines */
void appear(struct char_data *ch)
{
	if (affected_by_spell(ch, SPELL_INVISIBLE))
		affect_from_char(ch, SPELL_INVISIBLE);

	REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
	REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

	if (GET_LEVEL(ch) < LVL_IMMORT)
		act("$n aparece lentamente.", FALSE, ch, 0, 0, TO_ROOM);
	else
		act("Você sente uma estranha presença a medida que $n aparece, aparentemente do nada.",
			FALSE, ch, 0, 0, TO_ROOM);
}

int compute_armor_class(struct char_data *ch)
{
	int armorclass = GET_AC(ch);
	struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);

	if (AWAKE(ch))
		armorclass += dex_app[GET_DEX(ch)].defensive * 10;

	if (!IS_NPC(ch))
	{
		armorclass += wpn_prof[get_weapon_prof(ch, wielded)].to_ac * 10;
		armorclass += nighthammer_info[get_nighthammer(ch, true)].to_ac;
		return (MAX(-200, armorclass));	/* -200 is lowest */
	}
	else
	{
		return (armorclass);
	}
}

void update_pos(struct char_data *victim)
{
	if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
		return;
	else if (GET_HIT(victim) > 0)
		GET_POS(victim) = POS_STANDING;
	else if (GET_HIT(victim) <= -11)
		GET_POS(victim) = POS_DEAD;
	else if (GET_HIT(victim) <= -6)
		GET_POS(victim) = POS_MORTALLYW;
	else if (GET_HIT(victim) <= -3)
		GET_POS(victim) = POS_INCAP;
	else
		GET_POS(victim) = POS_STUNNED;
}

void check_killer(struct char_data *ch, struct char_data *vict)
{
	if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA))
		return;
	if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
		return;
	if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
		return;

	SET_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
	send_to_char(ch, "Se você deseja ser um JOGADOR ASSASSINO, você será...\r\n");
	mudlog(BRF,
		   MAX(LVL_IMMORT, MAX(GET_INVIS_LEV(ch), GET_INVIS_LEV(vict))),
		   TRUE,
		   "PC Killer bit set on %s for initiating attack on %s at %s.",
		   GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
}

/* start one char fighting another (yes, it is horrible, I know... ) */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
	if (ch == vict)
		return;

	if (FIGHTING(ch))
	{
		core_dump();
		return;
	}

	ch->next_fighting = combat_list;
	combat_list = ch;

	if (AFF_FLAGGED(ch, AFF_SLEEP))
		affect_from_char(ch, SPELL_SLEEP);

	FIGHTING(ch) = vict;
	GET_POS(ch) = POS_FIGHTING;

	if (!CONFIG_PK_ALLOWED)
		check_killer(ch, vict);
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
	struct char_data *temp;

	if (ch == next_combat_list)
		next_combat_list = ch->next_fighting;

	REMOVE_FROM_LIST(ch, combat_list, next_fighting);
	ch->next_fighting = NULL;
	FIGHTING(ch) = NULL;
	GET_POS(ch) = POS_STANDING;
	update_pos(ch);
}

static void make_corpse(struct char_data *ch)
{
	char buf2[MAX_NAME_LENGTH + 64];
	struct obj_data *corpse, *o;
	struct obj_data *money;
	int i, x, y;

	corpse = create_obj();

	corpse->item_number = NOTHING;
	IN_ROOM(corpse) = NOWHERE;
	corpse->name = strdup("corpo");

	snprintf(buf2, sizeof(buf2), "O corpo de %s está aqui.", GET_NAME(ch));
	corpse->description = strdup(buf2);

	snprintf(buf2, sizeof(buf2), "o corpo de %s", GET_NAME(ch));
	corpse->short_description = strdup(buf2);

	GET_OBJ_TYPE(corpse) = ITEM_CORPSE;
	for (x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++)
	{
		if (x < EF_ARRAY_MAX)
			GET_OBJ_EXTRA_AR(corpse, x) = 0;
		if (y < TW_ARRAY_MAX)
			corpse->obj_flags.wear_flags[y] = 0;
	}
	SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
	GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
	GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
	GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
	GET_OBJ_RENT(corpse) = 100000;
	if (IS_NPC(ch))
	{
		SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
		GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
	}
	else
		GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;
	/* transfer character's inventory to the corpse */
	corpse->contains = ch->carrying;
	for (o = corpse->contains; o != NULL; o = o->next_content)
		o->in_obj = corpse;
	object_list_new_owner(corpse, NULL);
	/* transfer character's equipment to the corpse */
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i))
		{
			remove_otrigger(GET_EQ(ch, i), ch);
			obj_to_obj(unequip_char(ch, i), corpse);
		}

	/* transfer gold */
	if (GET_GOLD(ch) > 0)
	{
		/* following 'if' clause added to fix gold duplication loophole. The
		   above line apparently refers to the old "partially log in, kill the 
		   game character, then finish login sequence" duping bug. The
		   duplication has been fixed (knock on wood) but the test below shall 
		   live on, for a while. -gg 3/3/2002 */
		if (IS_NPC(ch) || ch->desc)
		{
			money = create_money(GET_GOLD(ch));
			obj_to_obj(money, corpse);
		}
		GET_GOLD(ch) = 0;
	}
	ch->carrying = NULL;
	IS_CARRYING_N(ch) = 0;
	IS_CARRYING_W(ch) = 0;
	/* Move o corpo para a cabana da cidade natal */
	if (!IS_NPC(ch))
	{
		GET_OBJ_VAL(corpse, 0) = GET_IDNUM(ch);
		if (GET_HOMETOWN(ch) == 1)
		{  
			obj_to_room(corpse, r_ress_room_1);
			log1("Corpo de '%s' na sala #%d tipo %d val0 '%d' val3 '%d'", GET_NAME(ch),
			 GET_ROOM_VNUM(IN_ROOM(ch)),GET_OBJ_TYPE(corpse), GET_OBJ_VAL(corpse, 0),GET_OBJ_VAL(corpse, 3));
		}
		else if (GET_HOMETOWN(ch) == 2)
	{
			obj_to_room(corpse, r_ress_room_2);
				log1("Corpo de '%s' na sala #%d tipo %d val0 '%d' val3 '%d'", GET_NAME(ch),
			 GET_ROOM_VNUM(IN_ROOM(ch)),GET_OBJ_TYPE(corpse), GET_OBJ_VAL(corpse, 0),GET_OBJ_VAL(corpse, 3));
	}
		else if (GET_HOMETOWN(ch) == 3)
        {
		obj_to_room(corpse, r_ress_room_3);
                log1("Corpo de '%s' na sala #%d tipo %d val0 '%d' val3 '%d'", GET_NAME(ch),
                GET_ROOM_VNUM(IN_ROOM(ch)),GET_OBJ_TYPE(corpse), GET_OBJ_VAL(corpse, 0),GET_OBJ_VAL(corpse, 3));
	}
		else {
			/* sem cidade natal */
			obj_to_room(corpse, IN_ROOM(ch));
	log1("Corpo de '%s' na sala #%d tipo %d val0 '%d' val3 '%d'", GET_NAME(ch),
			 GET_ROOM_VNUM(IN_ROOM(ch)),GET_OBJ_TYPE(corpse), GET_OBJ_VAL(corpse, 0),GET_OBJ_VAL(corpse, 3));
		act("Uma enorme mão aparece e pega o corpo de $n em segurança.", TRUE, ch, 0, 0,
			TO_ROOM);
		act("Uma enorme mão aparece e delicadamente deixa $p no chão.", FALSE, 0, corpse, 0,
			TO_ROOM);
	          }
	}
	else
		obj_to_room(corpse, IN_ROOM(ch));
		
}

			 /* When ch kills victim */
static void change_alignment(struct char_data *ch, struct char_data *victim)
{
	/* new alignment change algorithm: if you kill a monster with alignment A, 
	   you move 1/16th of the way to having alignment -A.  Simple and fast. */
	GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}

void death_cry(struct char_data *ch)
{
	int door;
	act("Você se arrepia ao escutar o grito de morte de $n.", FALSE, ch, 0, 0, TO_ROOM);
	for (door = 0; door < DIR_COUNT; door++)
		if (CAN_GO(ch, door))
			send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room,
						 "Você se arrepia ao escutar o grito de morte de alguém.\r\n");
}

void raw_kill(struct char_data *ch, struct char_data *killer)
{
	struct char_data *i;
	if (FIGHTING(ch))
		stop_fighting(ch);
	while (ch->affected)
		affect_remove(ch, ch->affected);
	/* To make ordinary commands work in scripts.  welcor */
	GET_POS(ch) = POS_STANDING;
	if (killer)
	{
		if (death_mtrigger(ch, killer))
			death_cry(ch);
	}
	else
		death_cry(ch);
	if (killer)
	{
		if (killer->group)
		{
			while ((i = (struct char_data *)simple_list(killer->group->members)) != NULL)
				if (IN_ROOM(i) == IN_ROOM(ch)
					|| (world[IN_ROOM(i)].zone == world[IN_ROOM(ch)].zone))
					autoquest_trigger_check(i, ch, NULL, AQ_MOB_KILL);
		}
		else
			autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);
	}

	/* Alert Group if Applicable */
	if (GROUP(ch))
		send_to_group(ch, GROUP(ch), "%s morreu.\r\n", GET_NAME(ch));
  
   
	update_pos(ch);
        
	/*************************************************************************
        * Sistema de Genética: O mob morreu. Chamamos a função de evolução.     *
        *************************************************************************/
        if (IS_NPC(ch) && ch->ai_data) {
           update_mob_prototype_genetics(ch);
        }
        /*************************************************************************
        * Fim do Bloco de Genética                                              *
        *************************************************************************/

	make_corpse(ch);
	// -- jr - Mar 17, 2000 * Enhancement of player death
	if (!IS_NPC(ch))
	{
		if (GET_LEVEL(ch) < LVL_IMMORT)
		{
			SET_BIT_AR(PLR_FLAGS(ch), PLR_GHOST);
			GET_COND(ch, HUNGER) = -1;
			GET_COND(ch, THIRST) = -1;			
		}
		GET_DEATH(ch)++;
		GET_HIT(ch) = 0;
		GET_MANA(ch) = 0;
		GET_MOVE(ch) = 0;
		GET_LOADROOM(ch) = CONFIG_DEAD_START;
	}
	extract_char(ch);
	if (killer)
	{
		autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
		autoquest_trigger_check(killer, NULL, NULL, AQ_ROOM_CLEAR);
	}
}

void die(struct char_data *ch, struct char_data *killer)
{
	gain_exp(ch, -(GET_EXP(ch) / 2));
	if (!IS_NPC(ch))
	{
		REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
		REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
	}
	raw_kill(ch, killer);
}

static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim)
{
	int share, hap_share;
	share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base));
	if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
	{
		/* This only reports the correct amount - the calc is done in gain_exp 
		 */
		hap_share = share + (int)((float)share * ((float)HAPPY_EXP / (float)(100)));
		share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, hap_share));
	}
	if (share > 1)
		send_to_char(ch, "Você recebe sua parte da experiência -- %lu pontos.\r\n", share);
	else
		send_to_char(ch, "Você recebe sua parte da experiência -- um mísero ponto!\r\n");
	gain_exp(ch, share);
	change_alignment(ch, victim);
}

static void group_gain(struct char_data *ch, struct char_data *victim)
{
	int tot_members = 0, base, tot_gain;
	struct char_data *k;
	while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
		if (IN_ROOM(ch) == IN_ROOM(k))
			tot_members++;
	/* round up to the nearest tot_members */
	tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;
	/* prevent illegal xp creation when killing players */
	if (!IS_NPC(victim))
		tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, tot_gain);
	if (tot_members >= 1)
		base = MAX(1, tot_gain / tot_members);
	else
		base = 0;
	while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL) {
		if (IN_ROOM(k) == IN_ROOM(ch)) {
			perform_group_gain(k, base, victim);
	                /******************************************************************
             * APRENDIZAGEM DE GRUPO: Reforço Positivo
             * Se o membro for um NPC com genética, a vitória em grupo
             * reforça a sua tendência de se agrupar.
             ******************************************************************/
           		 if (IS_NPC(k) && k->ai_data) {
		                k->ai_data->genetics.group_tendency += 2;
		                k->ai_data->genetics.group_tendency = MIN(GET_GENGROUP(k), 100);
		        }
		}
	}
}

static void solo_gain(struct char_data *ch, struct char_data *victim)
{
	int exp, happy_exp;
	exp = MIN(CONFIG_MAX_EXP_GAIN, GET_EXP(victim) / 3);
	/* Calculate level-difference bonus */
	if (IS_NPC(ch))
		exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
	else
		exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
	exp = MAX(exp, 1);
	if (IS_HAPPYHOUR && IS_HAPPYEXP)
	{
		happy_exp = exp + (int)((float)exp * ((float)HAPPY_EXP / (float)(100)));
		exp = MAX(happy_exp, 1);
	}

	if (exp > 1)
		send_to_char(ch, "Você recebeu %lu pontos de experiência.\r\n", exp);
	else
		send_to_char(ch, "Você recebeu um mísero ponto de experiência.\r\n");
	gain_exp(ch, exp);
	change_alignment(ch, victim);
}

static char *replace_string(const char *str,
							const char *weapon_singular, const char *weapon_plural)
{
	static char buf[256];
	char *cp = buf;
	for (; *str; str++)
	{
		if (*str == '#')
		{
			switch (*(++str))
			{
			case 'W':
				for (; *weapon_plural; *(cp++) = *(weapon_plural++));
				break;
			case 'w':
				for (; *weapon_singular; *(cp++) = *(weapon_singular++));
				break;
			default:
				*(cp++) = '#';
				break;
			}
		}
		else
			*(cp++) = *str;
		*cp = 0;
	}							/* For */

	return (buf);
}

	/* message for doing damage with a weapon */
static void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type)
{
	char *buf;
	int msgnum;
	static struct dam_weapon_type
	{
		const char *to_room;
		const char *to_char;
		const char *to_victim;
	} dam_weapons[] =
			{

				/* use #w for singular (i.e. "slash") and #W for plural (i.e.
				   "slashes") */

				{
					"$n tenta #W $N, mas erra.", /* 0: 0 */
					"\tyVocê tenta #W $N, mas erra.\tn", "\tr$n tenta #W você, mas erra.\tn"
				},
				{
					"$n faz cócegas em $N quando $e #w $M.", /* 1: 1..2 */
					"\tyVocê faz cócegas em $N quando você #w $M.\tn", "\tr$n faz cócegas em você quando $e #w você.\tn"
				},
				{
					"$n fracamente #w $N.", /* 2: 3..4 */
					"\tyVocê fracamente #w $N.\tn", "\tr$n fracamente #w você.\tn"
				},
				{
					"$n #w $N.", /* 3: 5..6 */
					"\tyVocê #w $N.\tn", "\tr$n #w você.\tn"
				},
				{
					"$n #w $N forte.", /* 4: 7..10 */
					"\tyVocê #w $N forte.\tn", "\tr$n #w você forte.\tn"
				},
				{
					"$n #w $N muito forte.", /* 5: 11..14 */
					"\tyVocê #w $N muito forte.\tn", "\tr$n #w você muito forte.\tn"
				},
				{
					"$n #w $N extremamente forte.", /* 6: 15..19 */
					"\tyVocê #w $N extremamente forte.\tn", "\tr$n #w você extremamente forte.\tn"
				},
				{
					"$n massacra $N em pequenos fragmentos com a #w $l.", /* 7: 19..23 */
					"\tyVocê massacra $N em pequenos fragmentos com a sua #w.\tn",
					"\tr$n massacra você em pequenos fragmentos com a #w $l.\tn"
				},
				{
					"$n OBLITERA $N com a #w mortal $l!!", /* 8: > 23 */
					"\tyVocê OBLITERA $N com a sua #w mortal!!\tn", "\ty$n OBLITERA você com a #w mortal $l!!\tn"
				}
			};
	w_type -= TYPE_HIT;			/* Change to base of table with text */
	if (dam == 0)
		msgnum = 0;
	else if (dam <= 2)
		msgnum = 1;
	else if (dam <= 4)
		msgnum = 2;
	else if (dam <= 6)
		msgnum = 3;
	else if (dam <= 10)
		msgnum = 4;
	else if (dam <= 14)
		msgnum = 5;
	else if (dam <= 19)
		msgnum = 6;
	else if (dam <= 23)
		msgnum = 7;
	else
		msgnum = 8;
	/* damage message to onlookers */
	buf = replace_string(dam_weapons[msgnum].to_room,
						 attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);
	/* damage message to damager */
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		send_to_char(ch, "(%d) ", dam);
	buf = replace_string(dam_weapons[msgnum].to_char,
						 attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_CHAR);
	send_to_char(ch, CCNRM(ch, C_CMP));
	/* damage message to damagee */
	if (GET_LEVEL(victim) >= LVL_IMMORT)
		send_to_char(victim, "\tR(%d)", dam);
	buf = replace_string(dam_weapons[msgnum].to_victim,
						 attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
	send_to_char(victim, CCNRM(victim, C_CMP));
}

	/* message for doing damage with a spell or skill. Also used for weapon
	   damage on miss and death blows. */
int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype)
{
	int i, j, nr;
	struct message_type *msg;
	struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);
	/* @todo restructure the messages library to a pointer based system as
	   opposed to the current cyclic location system. */
	for (i = 0; i < MAX_MESSAGES; i++)
	{
		if (fight_messages[i].a_type == attacktype)
		{
			nr = dice(1, fight_messages[i].number_of_attacks);
			for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
				msg = msg->next;
			if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMPL))
			{
				act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
				act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
				act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
			}
			else if (dam != 0)
			{
				/* 
				 * Don't send redundant color codes for TYPE_SUFFERING & other types
				 * of damage without attacker_msg.
				 */
				if (GET_POS(vict) == POS_DEAD)
				{
					if (msg->die_msg.attacker_msg)
					{
						send_to_char(ch, CCYEL(ch, C_CMP));
						act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
						send_to_char(ch, CCNRM(ch, C_CMP));
					}

					send_to_char(vict, CCRED(vict, C_CMP));
					act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
					send_to_char(vict, CCNRM(vict, C_CMP));
					act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
				}
				else
				{
					if (msg->hit_msg.attacker_msg)
					{
						send_to_char(ch, CCYEL(ch, C_CMP));
						act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
						send_to_char(ch, CCNRM(ch, C_CMP));
					}

					send_to_char(vict, CCRED(vict, C_CMP));
					act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
					send_to_char(vict, CCNRM(vict, C_CMP));
					act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
				}
			}
			else if (ch != vict)
			{					/* Dam == 0 */
				if (msg->miss_msg.attacker_msg)
				{
					send_to_char(ch, CCYEL(ch, C_CMP));
					act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
					send_to_char(ch, CCNRM(ch, C_CMP));
				}

				send_to_char(vict, CCRED(vict, C_CMP));
				act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
				send_to_char(vict, CCNRM(vict, C_CMP));
				act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
			}
			return (1);
		}
	}
	return (0);
}

	/* This function returns the following codes: < 0 Victim died.  = 0 No
	   damage. > 0 How much damage done. */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{
	long local_gold = 0, happy_gold = 0;
	char local_buf[256];
	struct char_data *tmp_char;
	struct obj_data *corpse_obj;
	if (GET_POS(victim) <= POS_DEAD)
	{
		/* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
		if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
		{
			act("$N já está mort$R! Não adianta tentar matá-l$R.",
				FALSE, ch, NULL, victim, TO_CHAR);
			return (-1);
		}
		log1("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.", GET_NAME(victim),
			 GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
		die(victim, ch);
		return (-1);			/* -je, 7/7/92 */
	}

	/* peaceful rooms */
	if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
		ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
	{
		send_to_char(ch, "Este lugar é muito calmo, sem violências...\r\n");
		return (0);
	}

	/* shopkeeper and MOB_NOKILL protection */
	if (!ok_damage_shopkeeper(ch, victim)
		|| MOB_FLAGGED(victim, MOB_NOKILL) || PLR_FLAGGED(victim, PLR_TRNS))
	{
		send_to_char(ch, "Você não pode lutar.\r\n");
		return (0);
	}

	/* You can't damage an immortal! */
	if (!IS_NPC(victim)
		&& ((GET_LEVEL(victim) >= LVL_IMMORT) && PRF_FLAGGED(victim, PRF_NOHASSLE)))
		dam = 0;
	if (victim != ch)
	{
		/* Start the attacker fighting the victim */
		if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
			set_fighting(ch, victim);
		/* Start the victim fighting the attacker */
		if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL))
		{
			set_fighting(victim, ch);
			if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
				remember(victim, ch);
		}
	}

	/* If you attack a pet, it hates your guts */
	if (victim->master == ch)
		stop_follower(victim);
	/* If the attacker is invisible, he becomes visible */
	if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_HIDE))
		appear(ch);
	if (AFF_FLAGGED(victim, AFF_STONESKIN))
	{
		dam = 0;				/* no damage when using stoneskin */
	}
	/* Cut damage in half if victim has sanct, to a minimum 1 */
	if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
		dam /= 2;
	if (AFF_FLAGGED(victim, AFF_GLOOMSHIELD) && dam >= 3)
		dam /= 3;
	/* Check for PK if this is not a PK MUD */
	if (!CONFIG_PK_ALLOWED)
	{
		check_killer(ch, victim);
		if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
			dam /= 2;
	}

	/* Set the maximum damage per round and subtract the hit points */
	dam = MAX(dam, 0);
	/* -- jr - Mar 16, 2000 * Fireshield protection */
	/* -- mp - Nov 05, 2001 * Added Thistlecoat protection */
	if ((attacktype == SPELL_FIRESHIELD ||
		 attacktype == SPELL_THISTLECOAT || attacktype == SPELL_WINDWALL) && (dam > GET_HIT(victim)))
		dam = MAX(GET_HIT(victim), 0);
	GET_HIT(victim) -= dam;
	/* Gain exp for the hit */
	if (ch != victim)
		gain_exp(ch, GET_LEVEL(victim) * dam);
	update_pos(victim);
	/* skill_message sends a message from the messages file in lib/misc.
	   dam_message just sends a generic "You hit $n extremely hard.".
	   skill_message is preferable to dam_message because it is more
	   descriptive. If we are _not_ attacking with a weapon (i.e. a spell),
	   always use skill_message. If we are attacking with a weapon: If this is 
	   a miss or a death blow, send a skill_message if one exists; if not,
	   default to a dam_message. Otherwise, always send a dam_message. */
	if (!IS_WEAPON(attacktype))
		skill_message(dam, ch, victim, attacktype);
	else
	{
		if (GET_POS(victim) == POS_DEAD || dam == 0)
		{
			if (!skill_message(dam, ch, victim, attacktype))
				dam_message(dam, ch, victim, attacktype);
		}
		else
		{
			dam_message(dam, ch, victim, attacktype);
		}
	}

	/* Use send_to_char -- act() doesn't send message if you are DEAD. */
	switch (GET_POS(victim))
	{
	case POS_MORTALLYW:
		act("$n está mortalmente ferid$r, e vai morrer logo, se não for medicad$r.", TRUE,
			victim, 0, 0, TO_ROOM);
		act("Você está mortalmente ferid$r, e vai morrer logo, se não medicad$r.", TRUE,
			victim, 0, 0, TO_CHAR | TO_SLEEP);
		break;
	case POS_INCAP:
		act("$n está incapacitad$r e vai morrer lentamente, se não for medicad$r.", TRUE,
			victim, 0, 0, TO_ROOM);
		act("Você está incapacitad$r e vai morrer lentamente, se não for medicad$r.", TRUE,
			victim, NULL, NULL, TO_CHAR | TO_SLEEP);
		break;
	case POS_STUNNED:
		act("$n está atordoad$r, mas provavelmente irá recuperar a consciência novamente.",
			TRUE, victim, 0, 0, TO_ROOM);
		act("Você está atordoad$r, mas provavelmente irá recuperar a consciência novamente.",
			TRUE, victim, 0, 0, TO_CHAR | TO_SLEEP);
		break;
	case POS_DEAD:
		act("$n está mort$r!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
		act("Você está mort$r!  Sinto muito...\r\n", TRUE, victim, 0, 0, TO_CHAR | TO_SLEEP);
		break;
	default:					/* >= POSITION SLEEPING */
		if (dam > (GET_MAX_HIT(victim) / 4))
			send_to_char(victim, "Isto realmente DOEU!\r\n");

		if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4))
		{
			send_to_char(victim, "\trVocê espera que seus ferimentos parem de %sSANGRAR%s tanto!\tn\r\n", CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
		}

		/************************************************************************************
                * Lógica de Genética REVISADA: Todos os mobs podem ser fujões.                     *                      * A flag MOB_WIMPY agora dá um bónus de "medo", em vez de ser um interruptor.     *                       ************************************************************************************/
		if (ch != victim && IS_NPC(victim) && GET_POS(victim) > POS_STUNNED) {
		    int flee_threshold = 0;
		    int base_wimpy = 0;
		                                                                                /* 1. O mob tem a flag MOB_WIMPY? Se sim, ele já tem uma base de medo. */
		    if (MOB_FLAGGED(victim, MOB_WIMPY)) {
		        base_wimpy = 20; /* Valor base de 20% de vida para fuga. */                                           }

		    /* 2. Adicionamos a tendência genética à base. */
		    if (victim->ai_data) {
		        flee_threshold = base_wimpy + victim->ai_data->genetics.wimpy_tendency;
		    } else {
		        flee_threshold = base_wimpy;
		    }                                                                                                     
		    /* 3. Garante que o limiar não passa de um valor razoável (ex: 80%) */
		    flee_threshold = MIN(flee_threshold, 80);
		    /* 4. Compara com a vida atual e tenta fugir se necessário. */
    		    if (flee_threshold > 0) {
		        /* Fuga DETERMINÍSTICA: Baseada na genética e na flag wimpy. */
		        if (GET_HIT(victim) < (GET_MAX_HIT(victim) * flee_threshold) / 100) {
		            /************************************************************
                             * LÓGICA DE DESERÇÃO DE GRUPO (IA EGOÍSTA)
                             * O medo pode levar à traição. Este é o local ideal.
                             ************************************************************/
                            if (GROUP(victim) && GROUP_LEADER(GROUP(victim)) != victim) {
                                /* A chance de abandonar é proporcional à sua tendência de fugir (Wimpy). */
                                if (rand_number(1, 150) <= GET_GENWIMPY(victim)) {
                                    act("$n entra em pânico, abandona os seus companheiros e foge!", TRUE, victim, 0, 0, TO_ROOM);
                                    send_to_char(victim, "Você não aguenta mais e abandona o seu grupo!\r\n");

                                    /* Aplica a penalidade de aprendizagem ao gene de grupo. */
                                    if (victim->ai_data) {
                                        victim->ai_data->genetics.group_tendency -= 1;
                                        if (GET_GENGROUP(victim) < 0) victim->ai_data->genetics.group_tendency = 0;
                                    }
                                    
                                    leave_group(victim); /* Sai do grupo. */
                                }
                            }
                            /************************************************************/

			    do_flee(victim, NULL, 0, 0);
		        }
		    } else {
		        /* Fuga de PÂNICO: Para mobs "corajosos" (threshold = 0) em estado crítico. */
		        /* Se a vida estiver abaixo de 10%, há uma pequena chance de pânico. */
		        if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 10)) {
		            if (rand_number(1, 100) <= 5) { /* Chance de 5% de entrar em pânico */
		                act("$n olha para os seus ferimentos, entra em pânico e tenta fugir!", TRUE, victim, 0, 0, TO_ROOM);
		                do_flee(victim, NULL, 0, 0);
		            }		
		        }
		    }
		}

		if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch)
			&& GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0)
		{
			send_to_char(victim, "Você se desespera e tenta fugir!\r\n");
			do_flee(victim, NULL, 0, 0);
		}
		break;
	}

	/* Help out poor linkless people who are attacked */
	if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED)
	{
		do_flee(victim, NULL, 0, 0);
		if (!FIGHTING(victim))
		{
			act("Você é resgatad$r por forças divinas.", FALSE, victim, 0, 0, TO_CHAR);
			act("$n é resgatad$r por forças divinas.", FALSE, victim, 0, 0, TO_ROOM);
			GET_WAS_IN(victim) = IN_ROOM(victim);
			char_from_room(victim);
			char_to_room(victim, 0);
		}
	}

	/* stop someone from fighting if they're stunned or worse */
	if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
		stop_fighting(victim);
	/* Uh oh.  Victim died. */
	if (GET_POS(victim) == POS_DEAD)
	{
		if (ch != victim && (IS_NPC(victim) || victim->desc))
		{
			if (GROUP(ch))
				group_gain(ch, victim);
			else
				solo_gain(ch, victim);
		}

		if (!IS_NPC(victim))
		{
			mudlog(BRF,
				   MAX(LVL_IMMORT,
					   MAX(GET_INVIS_LEV(ch), GET_INVIS_LEV(victim))),
				   TRUE, "%s killed by %s at %s", GET_NAME(victim),
				   GET_NAME(ch), world[IN_ROOM(victim)].name);
			if (MOB_FLAGGED(ch, MOB_MEMORY))
				forget(ch, victim);
		}
		/* Cant determine GET_GOLD on corpse, so do now and store */
		if (IS_NPC(victim))
		{
			if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD))
			{
				happy_gold = (long)(GET_GOLD(victim) * (((float)(HAPPY_GOLD)) / (float)100));
				happy_gold = MAX(0, happy_gold);
				increase_gold(victim, happy_gold);
			}
			local_gold = GET_GOLD(victim);
			sprintf(local_buf, "%ld", (long)local_gold);
		}

		die(victim, ch);
		if (GROUP(ch) && (local_gold > 0) && PRF_FLAGGED(ch, PRF_AUTOSPLIT))
		{
			generic_find("corpo", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
			if (corpse_obj)
			{
				do_get(ch, "all.moeda corpo", 0, 0);
				do_split(ch, local_buf, 0, 0);
			}
			/* need to remove the gold from the corpse */
		}
		else if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOGOLD))
		{
			do_get(ch, "all.moeda corpo", 0, 0);
		}
		if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOLOOT))
		{
			do_get(ch, "all corpo", 0, 0);
		}
		if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSAC))
		{
			do_sac(ch, "corpo", 0, 0);
		}
		return (-1);
	}
	return (dam);
}

	/* Calculate the THAC0 of the attacker. 'victim' currently isn't used but
	   you could use it for special cases like weapons that hit evil creatures 
	   easier or a weapon that always misses attacking an animal. */
int compute_thaco(struct char_data *ch, struct char_data *victim)
{
	int calc_thaco;
	int wpnprof = 0;
	int nham = 0;
	struct obj_data *wielded;

	if (!IS_NPC(ch))
	{
		calc_thaco = thaco(GET_CLASS(ch), GET_LEVEL(ch));
		wielded = GET_EQ(ch, WEAR_WIELD);
		if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
			wpnprof = get_weapon_prof(ch, wielded);
		nham = get_nighthammer(ch, true);
	}
	else						/* THAC0 for monsters is set in the HitRoll */
		calc_thaco = 20;
	calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
	calc_thaco -= GET_HITROLL(ch);
	calc_thaco -= wpn_prof[wpnprof].to_hit;
	calc_thaco -= nighthammer_info[nham].to_hit;
	calc_thaco -= (int)((GET_INT(ch) - 13) / 1.5);	/* Intelligence helps! */
	calc_thaco -= (int)((GET_WIS(ch) - 13) / 1.5);	/* So does wisdom */
	return calc_thaco;
}

void hit(struct char_data *ch, struct char_data *victim, int type)
{
	struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
	int w_type, victim_ac, calc_thaco, dam, diceroll;
	struct affected_type af;
	int percent;
	int wpnprof = 0, nham = 0;
	/* Check that the attacker and victim exist */
	if (!ch || !victim)
		return;
	/* check if the character has a fight trigger */
	fight_mtrigger(ch);
	/* Do some sanity checking, in case someone flees, etc. */
	if (IN_ROOM(ch) != IN_ROOM(victim))
	{
		if (FIGHTING(ch) && FIGHTING(ch) == victim)
			stop_fighting(ch);
		return;
	}

	/* If this is the start of combat, observe equipment (mob AI feature) */
	if (IS_NPC(ch) && ch->ai_data && !FIGHTING(ch)) {
		/* 20% chance to observe equipment during the first round of combat */
		if (rand_number(1, 100) <= 20) {
			observe_combat_equipment(ch, victim);
		}
	}

	/* Find the weapon type (for display purposes only) */
	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
		w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
	else
	{
		if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
			w_type = ch->mob_specials.attack_type + TYPE_HIT;
		else
			w_type = TYPE_HIT;
	}

	if (!IS_NPC(ch))
	{
		if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
			wpnprof = get_weapon_prof(ch, wielded);
		nham = get_nighthammer(ch, true);
	}

	/* Calculate chance of hit. Lower THAC0 is better for attacker. */
	calc_thaco = compute_thaco(ch, victim);
	// check to see if the victim has prot from evil on, and if the
	// attacker
	// is in fact evil
	// and detriment the attackers thaco roll if both are true
	if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && (GET_ALIGNMENT(ch) < -350))
	{
		calc_thaco = calc_thaco + 2;
	}

	// this is optional if you decide to add the protection from good 
	// 
	// 
	// 
	// 
	// 
	// 
	// spell
	// below

	// check to see if victim has protection from good on, and if the 
	// 
	// 
	// 
	// 
	// 
	// 
	// attacker 
	// 
	// 
	// is in fact good
	// add detriment to the attackers thaco roll if both are true
	if (AFF_FLAGGED(victim, AFF_PROTECT_GOOD) && (GET_ALIGNMENT(ch) > 350))
	{
		calc_thaco = calc_thaco + 2;
	}
	if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && (GET_ALIGNMENT(ch) < -350))
	{
		calc_thaco = calc_thaco + 2;
	}
	/* Calculate the raw armor including magic armor.  Lower AC is better for
	   defender. */
	victim_ac = compute_armor_class(victim) / 10;
	// check to see if victim is flagged with protection from evil,
	// and if the 
	// 
	// 
	// attacker is evil
	// and improve the victims armor if both are true
	if (AFF_FLAGGED(victim, AFF_PROTECT_EVIL) && (GET_ALIGNMENT(ch) < -350))
	{
		victim_ac = victim_ac - 1;
	}

	// this is optional if you decide to add the protection from good 
	// 
	// 
	// 
	// 
	// 
	// 
	// spell
	// below
	// check to see if victim is flagged with protection from good,
	// and if the 
	// 
	// 
	// attacker is good
	// and improve the victims armor if both are true
	if (AFF_FLAGGED(victim, AFF_PROTECT_GOOD) && (GET_ALIGNMENT(ch) > 350))
	{
		victim_ac = victim_ac - 1;
	}

	/* roll the die and take your chances... */
	diceroll = rand_number(1, 20);
	/* report for debugging if necessary */
	if (CONFIG_DEBUG_MODE >= NRM)
		send_to_char(ch,
					 "\t1Debug:\r\n   \t2Thaco: \t3%d\r\n   \t2AC: \t3%d\r\n   \t2Diceroll: \t3%d\tn\r\n",
					 calc_thaco, victim_ac, diceroll);
	/* Decide whether this is a hit or a miss.  Victim asleep = hit,
	   otherwise: 1 = Automatic miss.  2..19 = Checked vs. AC.  20 = Automatic 
	   hit. */
	if (diceroll == 20 || !AWAKE(victim))
		dam = TRUE;
	else if (diceroll == 1)
		dam = FALSE;
	else
		dam = (calc_thaco - diceroll <= victim_ac);
	if (!dam)
		/* the attacker missed the victim */
		damage(ch, victim, 0, type == SKILL_BACKSTAB ? SKILL_BACKSTAB : w_type);
	else
	{
		/* okay, we know the guy has been hit.  now calculate damage. Start
		   with the damage bonuses: the damroll and strength apply */
		dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
		dam += GET_DAMROLL(ch);
		dam += nighthammer_info[nham].to_dam;
		/* Maybe holding arrow? */
		if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
		{
			/* Add weapon-based damage if a weapon is being wielded */
			dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
		}
		else
		{
			/* If no weapon, add bare hand damage instead */
			if (IS_NPC(ch))
				dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
			else
				dam += rand_number(0, 2);	/* Max 2 bare hand damage for
											   players */
		}

		/* Include a damage multiplier if victim isn't ready to fight:
		   Position sitting 1.33 x normal Position resting 1.66 x normal
		   Position sleeping 2.00 x normal Position stunned 2.33 x normal
		   Position incap 2.66 x normal Position mortally 3.00 x normal Note,
		   this is a hack because it depends on the particular values of the
		   POSITION_XXX constants. */
		if (GET_POS(victim) < POS_FIGHTING)
			dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;
		/* at least 1 hp damage min per hit */
		dam = MAX(1, dam);
		if (type == SKILL_BACKSTAB)
			damage(ch, victim, dam * backstab_mult(GET_LEVEL(ch)), SKILL_BACKSTAB);
		else
			damage(ch, victim, dam, w_type);
	}
	/* 
	 * Poisoned weapons.
	 */

	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON && OBJ_FLAGGED(wielded, ITEM_POISONED)
		&& !MOB_FLAGGED(victim, MOB_NO_POISON))
	{
		new_affect(&af);
		af.spell = SPELL_POISON;
		af.modifier = -2;
		af.location = APPLY_STR;

		if (AFF_FLAGGED(victim, AFF_POISON))
			af.duration = 1;
		else
		{
			af.duration = rand_number(2, 5);
			act("Você se sente doente.", FALSE, victim, 0, ch, TO_CHAR);
			act("$n fica muito doente!", TRUE, victim, 0, ch, TO_ROOM);
		}
		SET_BIT_AR(af.bitvector, AFF_POISON);
		affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
		//remover veneno da arma
		if (rand_number(0, 3) == 0) REMOVE_BIT_AR(GET_OBJ_EXTRA(wielded),ITEM_POISONED);
	}

	/* Counter-attack spells */
	if (dam > 0 && !AFF_FLAGGED(ch, AFF_FIRESHIELD) && !AFF_FLAGGED(ch, AFF_THISTLECOAT)  && !AFF_FLAGGED(ch, AFF_WINDWALL))
	{

		/* -- jr - Mar 16, 2000 */
		if (AFF_FLAGGED(victim, AFF_FIRESHIELD))
		{
			if (mag_savingthrow(ch, SAVING_SPELL, GET_LEVEL(ch)))

				damage(victim, ch, dam / 2, SPELL_FIRESHIELD);
			else
				damage(victim, ch, dam, SPELL_FIRESHIELD);
		}

		/* -- mp - Nov 05, 2001 */
		else if (AFF_FLAGGED(victim, AFF_THISTLECOAT))
		{
			if (mag_savingthrow(ch, SAVING_SPELL, GET_LEVEL(ch)))
				damage(victim, ch, dam / 3, SPELL_THISTLECOAT);
			else
				damage(victim, ch, dam, SPELL_THISTLECOAT);
		}
			/* -- Forneck - Nov 09, 2020 */
		else if (AFF_FLAGGED(victim, AFF_WINDWALL))
		{
			if (mag_savingthrow(ch, SAVING_SPELL, GET_LEVEL(ch)))
				damage(victim, ch, dam / 2.5, SPELL_WINDWALL);
			else
				damage(victim, ch, dam, SPELL_WINDWALL);
		}
	}
	/* check if the victim has a hitprcnt trigger */
	hitprcnt_mtrigger(victim);
}

	/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
	struct char_data *ch, *tch;
	int num_of_attacks = 1, loop_attacks;
	for (ch = combat_list; ch; ch = next_combat_list)
	{
		next_combat_list = ch->next_fighting;
		if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
		{
			stop_fighting(ch);
			continue;
		}

		if (IS_NPC(ch))
		{
			if (GET_MOB_WAIT(ch) > 0)
			{
				GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
				continue;
			}
			GET_MOB_WAIT(ch) = 0;
			if (GET_POS(ch) < POS_FIGHTING)
			{
				GET_POS(ch) = POS_FIGHTING;
				act("$n se levanta!", TRUE, ch, 0, 0, TO_ROOM);
			}
		}

		if (GET_POS(ch) < POS_FIGHTING)
		{
			act("Você não pode lutar sentad$r!!", FALSE, ch, 0, 0, TO_CHAR);
			continue;
		}

		if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize)
		{
			struct iterator_data Iterator;
			tch = (struct char_data *)merge_iterator(&Iterator, GROUP(ch)->members);
			for (; tch; tch = next_in_list(&Iterator))
			{
				if (tch == ch)
					continue;
				if (!IS_NPC(tch) && !PRF_FLAGGED(tch, PRF_AUTOASSIST))
					continue;
				if (IN_ROOM(ch) != IN_ROOM(tch))
					continue;
				if (FIGHTING(tch))
					continue;
				if (GET_POS(tch) != POS_STANDING)
					continue;
				if (!CAN_SEE(tch, ch))
					continue;
				do_assist(tch, GET_NAME(ch), 0, 0);
			}
		}

		num_of_attacks = attacks_per_round(ch);

		for (loop_attacks = 0;
			 loop_attacks < num_of_attacks && FIGHTING(ch)
			 && !MOB_FLAGGED(FIGHTING(ch), MOB_NOTDEADYET); loop_attacks++)
			hit(ch, FIGHTING(ch), TYPE_UNDEFINED);

		if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET))
		{
			char actbuf[MAX_INPUT_LENGTH] = "";
			(GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);
		}

	}
}

	/* ported from Eltanin by AxL, 2feb97 */
	/* added by Miikka M. Kangas 8-14-91 (note this is called with ownpulses!)I was getting bored so I wanted to add some edge to
	   weather.  */
void beware_lightning() {
	int dam = 0;
	struct char_data *victim = NULL, *temp = NULL;
	char buf[256];
	zone_rnum zona_vitima;

    // Itera por todas as zonas
   for (int zone = 0; zone <= top_of_zone_table; zone++) {
    if (zone_table[zone].weather->sky != SKY_LIGHTNING)
        continue; // Pula para a próxima zona se não estiver relampejando

    if (rand_number(0, 9) != 0)
        continue; // Raio cai apenas 10% das vezes

    if (rand_number(0, 99) == 0) { // 99% das vezes, apenas trovão
        send_to_zone_outdoor(zone, "Voce ouve o ronco do trovao.\n\r");
        continue;
    }

    for (victim = character_list; victim; victim = temp) {
        temp = victim->next;
        zona_vitima = world[IN_ROOM(victim)].zone; //pega a zona (vnum para rnum) da sala da vitima 
	if (zona_vitima != zone)
	   continue;
        if (OUTSIDE(victim) == TRUE) { // Apenas personagens ao ar livre
            if (rand_number(0, 9) == 0) { // 1% de chance de acertar alguém
                dam = dice(1, (GET_MAX_HIT(victim) * 2));
                if (IS_AFFECTED(victim, AFF_SANCTUARY))
                    dam = MIN(dam, 18);
                if (IS_AFFECTED(victim, AFF_GLOOMSHIELD))
                    dam = MIN(dam, 33);
                dam = MIN(dam, 100);
                dam = MAX(dam, 0);
                if (GET_LEVEL(victim) >= LVL_IMMORT)
                    dam = 0; // Imortais não são afetados

                GET_HIT(victim) -= dam;

                act("KAZAK! Um raio atinge $n. Voce escuta um assobio.", TRUE, victim, 0, 0, TO_ROOM);
                act("KAZAK! Um raio atinge voce. Voce escuta um assobio.", FALSE, victim, 0, 0, TO_CHAR);

                if (dam > (GET_MAX_HIT(victim) >> 2))
                    act("Isto realmente DOEU!\r\n", FALSE, victim, 0, 0, TO_CHAR);

                send_to_zone_outdoor(zone, "*BOOM* Voce escuta o ronco do trovao.\n\r");

                update_pos(victim);

                switch (GET_POS(victim)) {
                    case POS_MORTALLYW:
                        act("$n esta mortalmente ferid$r, e vai morrer logo, se nao for medicad$r.", TRUE, victim, 0, 0, TO_ROOM);
                        act("Voce esta mortalmente ferid$r, e vai morrer logo, se nao medicad$r.", FALSE, victim, 0, 0, TO_CHAR);
                        break;
                    case POS_INCAP:
                        act("$n esta incapacitad$r e vai morrer lentamente, se nao for medicad$r.", TRUE, victim, 0, 0, TO_ROOM);
                        act("Voce esta incapacitad$r e vai morrer lentamente, se nao for medicad$r.", FALSE, victim, 0, 0, TO_CHAR);
                        break;
                    case POS_STUNNED:
                        act("$n esta atordoad$r, mas provavelmente ira recuperar a consciencia novamente.", TRUE, victim, 0, 0, TO_ROOM);
                        act("Voce esta atordoad$r, mas provavelmente ira recuperar a consciencia novamente.", FALSE, victim, 0, 0, TO_CHAR);
                        break;
                    case POS_DEAD:
                        act("$n esta mort$r! R.I.P.", TRUE, victim, 0, 0, TO_ROOM);
                        act("Voce esta mort$r! Sinto muito...\r\n", FALSE, victim, 0, 0, TO_CHAR);
                        break;
                    default:
                        if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2))
                            act("Voce espera que seus ferimentos parem de SANGRAR tanto!", FALSE, victim, 0, 0, TO_CHAR);
                        break;
                }

                if (GET_POS(victim) == POS_DEAD) {
                    sprintf(buf, "Um raio matou %s", GET_NAME(victim));
                    log1(buf);
                    gain_exp(victim, -(GET_EXP(victim) / 2));
                    if (!IS_NPC(victim)) {
                        REMOVE_BIT_AR(PLR_FLAGS(victim), PLR_KILLER);
                        REMOVE_BIT_AR(PLR_FLAGS(victim), PLR_THIEF);
			SET_BIT_AR(PLR_FLAGS(victim), PLR_GHOST);
                    }
                    if (FIGHTING(victim))
                        stop_fighting(victim);
                    while (victim->affected)
                        affect_remove(victim, victim->affected);

                    GET_POS(victim) = POS_STANDING;
                    death_cry(victim);
                    if (GROUP(victim))
                        send_to_group(victim, GROUP(victim), "%s morreu.\r\n", GET_NAME(victim));
                    update_pos(victim);
                    make_corpse(victim);
                    extract_char(victim);
                }
            } else {
                act("KAZAK! Um raio atinge perto.", FALSE, victim, 0, 0, TO_ROOM);
                act("KAZAK! Um raio atinge perto.", FALSE, victim, 0, 0, TO_CHAR);
                send_to_zone_outdoor(zone, "*BOOM* Voce escuta o ronco do trovao.\n\r");
            }
        }
    }
 }
}
/* -- jr - 24/08/99 * Weapon Proficiencies -- (C) 1999 by Fabrizio Baldi */

int get_weapon_prof(struct char_data *ch, struct obj_data *wield)
{
	int value = 0, bonus = 0, learned = 0, type = -1;
	int i = 0;
	struct str_spells *skill = NULL;
	if (IS_NPC(ch) || IS_DEAD(ch))
		return (bonus);
	if (wield != NULL)
	{
		value = GET_OBJ_VAL(wield, 3) + TYPE_HIT;
		switch (value)
		{
		case TYPE_SLASH:
			type = SKILL_WEAPON_SWORDS;
			break;
		case TYPE_STING:
		case TYPE_PIERCE:
		case TYPE_STAB:
			type = SKILL_WEAPON_DAGGERS;
			break;
		case TYPE_THRASH:
		case TYPE_WHIP:
			type = SKILL_WEAPON_WHIPS;
			break;
		case TYPE_CLAW:
			type = SKILL_WEAPON_TALONOUS_ARMS;
			break;
		case TYPE_BLUDGEON:
		case TYPE_MAUL:
		case TYPE_POUND:
		case TYPE_CRUSH:
			type = SKILL_WEAPON_BLUDGEONS;
			break;
		case TYPE_HIT:
		case TYPE_PUNCH:
		case TYPE_BITE:
		case TYPE_BLAST:
			type = SKILL_WEAPON_EXOTICS;
			break;
		case TYPE_BORE:
		case TYPE_BROACH:
		case TYPE_MOW:
			type = SKILL_WEAPON_POLEARMS;
		default:
			type = -1;
			break;
		}
	}
	 if (type != -1)
	{
		if ((learned = GET_SKILL(ch, type)) != 0)
		{
			if (learned <= 20)
				bonus = 1;
			else if (learned <= 40)
				bonus = 2;
			else if (learned <= 60)
				bonus = 3;
			else if (learned <= 80)
				bonus = 4;
			else if (learned <= 85)
				bonus = 5;
			else if (learned <= 90)
				bonus = 6;
			else if (learned <= 95)
				bonus = 7;
			else if (learned <= 99)
				bonus = 8;
			else
				bonus = 9;
		}
		else
		{
			skill = get_spell_by_vnum(type);
			for (i = 0; i < NUM_CLASSES; i++)
				if (skill->assign[i].class_num == GET_CLASS(ch))
					if (GET_LEVEL(ch) >= skill->assign[i].level)
						bonus = 10;
		}
		return (bonus);
	}
	else
	{
	   return 0;
	}
}

/* -- jr - Oct 07, 2001 */
#define NIGHTHAMMER_LVL 50
int get_nighthammer(struct char_data *ch, bool real)
{
	int mod, learned;

	if (IS_NPC(ch) || IS_DEAD(ch))
		return (0);

	if (GET_LEVEL(ch) < NIGHTHAMMER_LVL)
		return (0);

	if (!(learned = GET_SKILL(ch, SKILL_NIGHTHAMMER)))
		return (0);

	if (time_info.hours > 4 && time_info.hours < 21)
		return (0);

	mod = 1;
	mod += (time_info.hours < 4 || time_info.hours > 21);
	mod += ((GET_LEVEL(ch) - NIGHTHAMMER_LVL) / 8);
	mod = MIN(mod, 8);

	if (real && rand_number(0, 101) > learned)
		mod--;

	return MIN(MAX(mod, 0), 8);

}

/* -- jr - May 07, 2001 */
int attacks_per_round(struct char_data *ch)
{
	int n = 1;

	if (!IS_NPC(ch))
	{
		struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
		if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
			n += wpn_prof[get_weapon_prof(ch, wielded)].num_of_attacks;
	}
	else
		n += ((int)GET_LEVEL(ch) / 25);

	return (n);
}

void transcend(struct char_data *ch) {
  struct char_data *k;

  /* Set the experience */
  GET_EXP(ch) = level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1) - 1;

  /* Stop fighting, seizing, mounting, etc */
  if (FIGHTING(ch))
    stop_fighting(ch);


  	for (k = combat_list; k; k = next_combat_list)
	{
		next_combat_list = k->next_fighting;
	    if (FIGHTING(k) == ch)
      stop_fighting(k);
	}

  /* Set the transcendent flag */
  SET_BIT_AR(PLR_FLAGS(ch), PLR_TRNS);
  
  /* Restore character points */
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  /* Reset other variables */
  GET_PRACTICES(ch) = 0;

  /* Explain what happened */
  send_to_char(ch,
	    "\n@+cAo lutar, uma estranha sensaçãoo vem sobre você, e você\r\n"
	    "sente como se você não pudesse mais aprender, como se o seu\r\n"
	    "conhecimento houvesse chegado ao limite...\r\n\n"
	    "\a\a@+WVocê transcendeu!!@+n\r\n\n");

  /* Log */
  log1("(Lvl) %s (level %d) trancended.", GET_NAME(ch), GET_LEVEL(ch));

  /* Save */
  save_char(ch);
}

/**
 * Função auxiliar que calcula a nova média ponderada para um único gene
 * e atualiza o valor do protótipo.
 * @param proto_gene Um ponteiro para o campo do gene no protótipo.
 * @param instance_gene O valor final do gene da instância que morreu.
 * @param min O valor mínimo permitido para o gene.
 * @param max O valor máximo permitido para o gene.
 */
void update_single_gene(int *proto_gene, int instance_gene, int min, int max)
{
    /* Usa a nossa fórmula de média ponderada 70/30. */
    *proto_gene = (((*proto_gene) * 7) + (instance_gene * 3)) / 10;

    /* Garante que o novo valor fica dentro dos limites. */
    *proto_gene = MIN(MAX(*proto_gene, min), max);
}

/**
 * Chamada quando um mob morre para atualizar a genética do seu protótipo.
 * VERSÃO REFATORADA: Usa uma função auxiliar para maior clareza e manutenção.
 */
void update_mob_prototype_genetics(struct char_data *mob)
{
    if (!IS_NPC(mob) || GET_MOB_RNUM(mob) == NOBODY || !mob->ai_data)
        return;

    mob_rnum rnum = GET_MOB_RNUM(mob);
    struct char_data *proto = &mob_proto[rnum];

    if (!proto->ai_data)
        return;

    /* 1. Prepara os valores finais das instâncias, aplicando as penalidades de morte. */
    int final_wimpy = mob->ai_data->genetics.wimpy_tendency;
    int final_roam = mob->ai_data->genetics.roam_tendency;
    int final_group = mob->ai_data->genetics.group_tendency;
    int final_use = mob->ai_data->genetics.use_tendency;
    int final_brave = mob->ai_data->genetics.brave_prevalence;
    int final_trade = mob->ai_data->genetics.trade_tendency;

    if (MOB_FLAGGED(mob, MOB_BRAVE)) {
        final_wimpy -= 5;
        final_brave += 1; /* A morte de um bravo reforça o traço. */
    } else {
        final_wimpy += 5;
        final_brave -= 1; /* A morte de um não-bravo diminui a prevalência. */
    }

    if (IN_ROOM(mob) != real_room(GET_LOADROOM(mob))) {
        final_roam -= 10;
    }

    if (GROUP(mob)) {
        final_group -= 3;
    }

    /* 2. Chama a função auxiliar para atualizar cada gene do protótipo. */
    update_single_gene(&proto->ai_data->genetics.wimpy_tendency, final_wimpy, 0, 100);
    update_single_gene(&proto->ai_data->genetics.loot_tendency, mob->ai_data->genetics.loot_tendency, 0, 100);
    update_single_gene(&proto->ai_data->genetics.equip_tendency, mob->ai_data->genetics.equip_tendency, 0, 100);
    update_single_gene(&proto->ai_data->genetics.roam_tendency, final_roam, 0, 100);
    update_single_gene(&proto->ai_data->genetics.group_tendency, final_group, 0, 100);
    update_single_gene(&proto->ai_data->genetics.use_tendency, final_use, 0, 100);
    update_single_gene(&proto->ai_data->genetics.trade_tendency, final_trade, 0, 100);
    update_single_gene(&proto->ai_data->genetics.brave_prevalence, final_brave, 0, 75); /* Usa o limite de 75 que definimos. */

    /* 3. Marca a zona para salvar. */
    mob_vnum vnum = mob_index[rnum].vnum;
    zone_rnum rznum = real_zone_by_thing(vnum);

    if (rznum != NOWHERE) {
        add_to_save_list(zone_table[rznum].number, SL_MOB);
    }
}
