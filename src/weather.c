/**
* @file weather.c                                          
* Functions that handle the in game progress of time and weather changes.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "fight.h"
#include "constants.h"

static void another_hour(int mode);
void weather_change(zone_rnum zone);
void setup_weather(struct zone_data *zone);
//int get_extra_movement(struct char_data *ch);
extern struct zone_data *zone_table;
/** Call this function every mud hour to increment the gametime (by one hour)
 * and the weather patterns.
 * @param mode Really, this parameter has the effect of a boolean. In the
 * current incarnation of the function and utility functions, as long as mode
 * is non-zero, the gametime will increment one hour and the weather will be
 * changed.
 */
void weather_and_time(int mode)
{
	struct zone_data *zone;
	struct descriptor_data *d;
	struct weather_data *weather;
	const struct climate_data *climate;
	int p;
	zone_rnum i;

	another_hour(mode);
	if (mode)
		weather_change(-1);

	for (i = 0; i <= top_of_zone_table; i++)
		weather_change(i);

	for (d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) != CON_PLAYING || d->character == NULL)
			continue;
		if (!AWAKE(d->character) || !OUTSIDE(d->character))
			continue;

		if (ZONE_CLIMATE(IN_ROOM(d->character)) == CLIM_DEFAULT)
		{
			climate = &climates[CLIM_NEUTRAL];
			weather = &weather_info;
		}
		else
		{
			climate = &climates[ZONE_CLIMATE(IN_ROOM(d->character))];
			weather = zone_table[GET_ROOM_ZONE(IN_ROOM(d->character))].weather;
		}
		if (weather->sky != weather->before)
		{
			switch (weather->sky)
			{
			case SKY_CLOUDLESS:
				write_to_output(d->character, "As nuvens desapareceram do céu.\r\n");
				break;
			case SKY_CLOUDY:
				if (weather->before == SKY_CLOUDLESS)
					write_to_output(d->character, "Nuvens misteriosas aparecem no céu.\r\n");
				else if (weather->before == SKY_SNOWING)
					write_to_output(d->character, "A neve parou de cair.\r\n");
				else
					write_to_output(d->character, "A chuva parou.\r\n");
				break;
			case SKY_RAINING:
				if (weather->before == SKY_LIGHTNING)
					write_to_output(d->character, "Os relâmpagos param.\r\n");
				else if (weather->before == SKY_SNOWING)
					write_to_output(d->character,
								 "A neve para de cair e dá lugar para a chuva.\r\n");
				else
					write_to_output(d->character, "A chuva começa a cair.\r\n");
				break;
			case SKY_LIGHTNING:
				write_to_output(d->character, "Relâmpagos começam a aparecer no céu.\r\n");
				break;
			case SKY_SNOWING:
				if (weather->before == SKY_RAINING)
					write_to_output(d->character, "A chuva para de cair e começa a nevar.\r\n");
				else
					write_to_output(d->character, "A neve começa a cair.\r\n");
				break;
			}
		}
		else
		{
			p = rand_number(0, 99);
			switch (weather->sky)
			{
			case SKY_CLOUDLESS:
			case SKY_CLOUDY:
				if (p < climate->winds * 100)
				{
					if (p < 25)
						write_to_output(d->character,
									 "Você sente uma brisa passando por você.\r\n");
					else if (p < 50)
						write_to_output(d->character, "Um forte vento açoita o seu rosto.\r\n");
					else if (p < 75)
						write_to_output(d->character, "A ventania dificulta os seus movimentos.\r\n");
					else
						write_to_output(d->character, "Os ventos parecem o castigar.\r\n");
				}
				else if (weather->temperature < 5)
					write_to_output(d->character,
								 "Uma nevasca parece prestes a cair sobre a região.\r\n");
				break;
			case SKY_RAINING:
			case SKY_LIGHTNING:
				if (p < climate->winds * 100)
				{
					if (p < 25)
						write_to_output(d->character,
									 "Uma brisa passa por você, jogando a chuva contra o seu rosto.\r\n");
					else
						write_to_output(d->character,
									 "A chuva e os fortes ventos dificultam os seus passos.\r\n");
				}

				switch (weather->sky)
				{
				case SKY_RAINING:
					if (climate->humidity >= 0.75 && p >= 50)
						write_to_output(d->character,
									 "Pesadas gotas de chuva caem com violência.\r\n");
					break;
				case SKY_LIGHTNING:
					if (climate->humidity >= 0.60 && p < 50)
						write_to_output(d->character, "O som dos trovões preenchem o ar.\r\n");
					else if (climate->humidity >= 0.70 && p < 60)
						write_to_output(d->character, "Um claro relâmpago rasga os céus.\r\n");
					break;
				default:		/* stupid gcc */
					break;
				}
				break;
			case SKY_SNOWING:
				if (p < climate->winds * 100)
					write_to_output(d->character,
								 "O frio vento do inverno parece congelar seus ossos.\r\n");
				else
					write_to_output(d->character,
								 "Pequenos flocos de neve se dispersam pelo ar.\r\n");
				break;
			}
		}
	}
}

/** Increment the game time by one hour (no matter what) and display any time 
 * dependent messages via send_to_outdoors() (if parameter is non-zero).
 * @param mode Really, this parameter has the effect of a boolean. If non-zero,
 * display day/night messages to all eligible players.
 */
static void another_hour(int mode)
{
	time_info.hours++;
	if (mode)
	{
		switch (time_info.hours)
		{
		case 5:
			weather_info.sunlight = SUN_RISE;
			send_to_outdoor("O sol nasceu no leste.\r\n");
			break;
		case 6:
			weather_info.sunlight = SUN_LIGHT;
			send_to_outdoor("O dia comecou.\r\n");
			break;
		case 18:
			weather_info.sunlight = SUN_SET;
			send_to_outdoor("O sol lentamente desaparece no oeste.\r\n");
			break;
		case 20:
			weather_info.sunlight = SUN_DARK;
			send_to_outdoor("A noite comecou.\r\n");
			break;
		default:
			break;
		}
	}
	if (time_info.hours > 23)
	{							/* Changed by HHS due to bug ??? */
		time_info.hours -= 24;
		time_info.day++;
		if (time_info.day > 34)
		{
			time_info.day = 0;
			time_info.month++;
			if (time_info.month > 16)
			{
				time_info.month = 0;
				time_info.year++;
			}
		}
	}
}

/** Controls the in game weather system. If the weather changes, an information
 * update is sent via send_to_outdoors().
 * @todo There are some hard coded values that could be extracted to make
 * customizing the weather patterns easier.
 */
void weather_change(zone_rnum zone)
{
	int diff, meant, meanp, sim_pressure;
	const struct climate_data *climate;
	struct weather_data *weather;
	if (zone)
	{
		weather = zone_table[zone].weather;
		climate = &climates[ZONE_CLIMATE(zone)];
	}
	if (zone == -1)
	{
		weather = &weather_info;
		climate = &climates[CLIM_NEUTRAL];
	}

	meant = climate->temperature;
	meanp = climate->pressure;
	if ((time_info.month >= 9) && (time_info.month <= 16))
	{
		meant -= climate->temp_diff;
		meanp -= climate->press_diff;
	}
	else
	{
		meant += climate->temp_diff;
		meanp += climate->press_diff;
	}


	diff = (weather->pressure > meanp ? -2 : 2);
	weather->change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));
	weather->change = MAX((-12), MIN((12), (weather->change)));
	weather->pressure += weather->change;
	weather->pressure = MAX((950), MIN((1050), (weather->pressure)));
	diff = (weather->temperature > meant ? -1 : 1);
	weather->temperature += (weather->change / 6 + diff * dice(1, 2));
	/* 
	 *   960       980      1000      1020      1040
	 *  ��|----+----|----+----|----+----|----+----|��
	 *     LIGHTING  RAINING   CLOUDY    CLOUDLESS    temp >= 15�C
	 *     RAINING   RAINING   CLOUDY    CLOUDLESS    0�C < temp < 15�C
	 *     SNOWING   SNOWING   CLOUDY    CLOUDY       temp <= 0�C
	 */
	sim_pressure = weather->pressure - (40 * climate->humidity) + 20;
	weather->before = weather->sky;
	switch (weather->sky)
	{
	case SKY_CLOUDLESS:
		if (sim_pressure < 990)
		{
			weather->sky = SKY_CLOUDY;
		}
		else if (sim_pressure < 1010)
		{
			if (dice(1, 4) == 1)
				weather->sky = SKY_CLOUDY;
		}
		break;
	case SKY_CLOUDY:
		if (sim_pressure < 970)
		{
			weather->sky = SKY_RAINING;
		}
		else if (sim_pressure < 990)
		{
			if (dice(1, 4) == 1)
			{
				if (weather->temperature <= 0)
					weather->sky = SKY_SNOWING;
				else
					weather->sky = SKY_RAINING;
			}
		}
		else if (sim_pressure > 1030)
		{
			if (weather->temperature > 0 && dice(1, 4) == 1)
				weather->sky = SKY_CLOUDLESS;
		}
		break;
	case SKY_RAINING:
		if (sim_pressure < 970)
		{
			if (weather->temperature >= 15)
			{
				if (dice(1, 4) == 1)
					weather->sky = SKY_LIGHTNING;
			}
			else if (weather->temperature <= 0)
			{
				if (dice(1, 2) == 1)
					weather->sky = SKY_SNOWING;
			}
		}
		else if (sim_pressure > 1030)
		{
			weather->sky = SKY_CLOUDY;
		}
		else if (sim_pressure > 1010)
		{
			if (dice(1, 4) == 1)
				weather->sky = SKY_CLOUDY;
		}
		else if (weather->temperature <= 0)
		{
			if (dice(1, 4) == 1)
				weather->sky = SKY_SNOWING;
		}
		break;
	case SKY_LIGHTNING:
		if (sim_pressure > 1010)
		{
			weather->sky = SKY_RAINING;
		}
		else if (sim_pressure > 990)
		{
			if (dice(1, 4) == 1)
				weather->sky = SKY_RAINING;
		}
		break;
	case SKY_SNOWING:
		if (sim_pressure > 1030)
		{
			weather->sky = SKY_CLOUDY;
		}
		else if (sim_pressure > 1010)
		{
			if (dice(1, 4) == 1)
				weather->sky = SKY_CLOUDY;
		}
		else if (weather->temperature > 0)
		{
			if (dice(1, 4) == 1)
				weather->sky = SKY_CLOUDY;
		}
		break;
	}

}


void setup_weather(struct zone_data *zone)
{
	struct weather_data *weather;
	const struct climate_data *climate;
	int sim_pressure;
	if (zone)
	{
		weather = zone->weather;
		climate = &climates[zone->climate];
	}
	else
	{
		weather = &weather_info;
		climate = &climates[CLIM_NEUTRAL];
	}

	if ((time_info.month >= 9) && (time_info.month <= 16))
	{
		weather->temperature = climate->temperature - climate->temp_diff;
		weather->pressure = climate->pressure - climate->press_diff;
	}
	else
	{
		weather->temperature = climate->temperature + climate->temp_diff;
		weather->pressure = climate->pressure + climate->press_diff;
	}

	weather->temperature += rand_number(-3, +3);
	weather->pressure += rand_number(-18, +18);
	weather->change = 0;
	sim_pressure = weather->pressure - (40 * climate->humidity) + 20;
	if (sim_pressure <= 980)
	{
		if (weather->temperature >= 15)
			weather->sky = SKY_LIGHTNING;
		else if (weather->temperature <= 0)
			weather->sky = SKY_SNOWING;
		else
			weather->sky = SKY_RAINING;
	}
	else if (sim_pressure <= 1000)
	{
		if (weather->temperature <= 0)
			weather->sky = SKY_SNOWING;
		else
			weather->sky = SKY_RAINING;
	}
	else if (sim_pressure <= 1020)
	{
		weather->sky = SKY_CLOUDY;
	}
	else
	{
		if (weather->temperature <= 0)
			weather->sky = SKY_CLOUDY;
		else
			weather->sky = SKY_CLOUDLESS;
	}
}

/*
int get_extra_movement(struct char_data *ch)
{
	const struct climate_data *climate;
	struct weather_data *weather;
	int r = 0;
	if (zone_table[GET_ROOM_ZONE(IN_ROOM(ch))].climate == CLIM_DEFAULT)
	{
		climate = &climates[CLIM_NEUTRAL];
		weather = &weather_info;
	}
	else
	{
		climate = &climates[zone_table[GET_ROOM_ZONE(IN_ROOM(ch))].climate];
		weather = &zone_table[GET_ROOM_ZONE(IN_ROOM(ch))].weather;
	}

	if (!OUTSIDE(ch))
		return 0;
	r += climate->winds * 4;
	if (weather->sky == SKY_RAINING || weather->sky == SKY_LIGHTNING)
		r += 1;
	else if (weather->sky == SKY_SNOWING)
		r += 2;
	return (r);
}
*/