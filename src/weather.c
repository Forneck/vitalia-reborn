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

static void another_hour(int mode);
void weather_change(zone_rnum zone);
extern struct weather_data climates[];

/** Call this function every mud hour to increment the gametime (by one hour)
 * and the weather patterns.
 * @param mode Really, this parameter has the effect of a boolean. In the
 * current incarnation of the function and utility functions, as long as mode
 * is non-zero, the gametime will increment one hour and the weather will be
 * changed.
 */
void weather_and_time(int mode)
{
  int i;
  another_hour(mode);
  if (mode)
    for (i=0;i<top_of_zone_table;i++){
       weather_change(zone_table[i].number);
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

  if (mode) {
    switch (time_info.hours) {
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
  if (time_info.hours > 23) {	/* Changed by HHS due to bug ??? */
    time_info.hours -= 24;
    time_info.day++;

    if (time_info.day > 34) {
      time_info.day = 0;
      time_info.month++;

      if (time_info.month > 16) {
	time_info.month = 0;
	time_info.year++;
      }
    }
  }
 }
}
/** Controls the in game weather system. If the weather changes, an information
 * update is sent via send_to_outdoors().
 * @todo There are some hard coded values that could be extracted to make
 * customizing the weather patterns easier.
 */
/* 
void weather_change(zone_rnum zone) {
    if (zone < 0 || zone > top_of_zone_table) {
        fprintf(stderr, "Erro: Zona inválida #%d\n", zone);
        return;
    }

    struct weather_data *clima = zone_table[zone].weather;
    int diff, meant, meanp, sim_pressure;

    if (!clima) {
        fprintf(stderr, "Erro: Clima não inicializado para a zona #%d\n", zone);
        return;
    }
}
*/

void weather_change(zone_rnum zone) {
    int diff, meant, meanp, sim_pressure;
    struct weather_data *weather;
    struct weather_data *climate;

    if (zone < 0 || zone > top_of_zone_table) return;

    weather = zone_table[zone].weather;
    climate = &climates[zone_table[zone].climate];

    if (!weather || !climate) return;

    // Definição das médias baseadas na estação do ano
    meant = climate->temperature;
    meanp = climate->pressure;

    if (time_info.month >= 9 && time_info.month <= 16) { // Outono/Inverno
        meant -= climate->temp_diff;
        meanp -= climate->press_diff;
    } else { // Primavera/Verão
        meant += climate->temp_diff;
        meanp += climate->press_diff;
    }

    // Mudança de pressão atmosférica
    diff = (weather->pressure > meanp ? -2 : 2);
    weather->press_diff += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));
    weather->press_diff = URANGE(weather->press_diff, -12, 12);

    weather->pressure += weather->press_diff;
    weather->pressure = URANGE(weather->pressure, 950, 1050);

    // Mudança de temperatura
    diff = (weather->temperature > meant ? -1 : 1);
    weather->temperature += (weather->press_diff / 6 + diff * dice(1, 2));

    // Simulação da pressão considerando umidade
    sim_pressure = weather->pressure - (40 * climate->humidity) + 20;
    weather->before = weather->sky;

    // Lógica de mudança do clima baseada na pressão e temperatura
    switch (weather->sky) {
        case SKY_CLOUDLESS:
            if (sim_pressure < 990) {
                weather->sky = SKY_CLOUDY;
            } else if (sim_pressure < 1010 && dice(1, 4) == 1) {
                weather->sky = SKY_CLOUDY;
            }
            break;

        case SKY_CLOUDY:
            if (sim_pressure < 970) {
                weather->sky = SKY_RAINING;
            } else if (sim_pressure < 990 && dice(1, 4) == 1) {
                weather->sky = (weather->temperature <= 0) ? SKY_SNOWING : SKY_RAINING;
            } else if (sim_pressure > 1030 && weather->temperature > 0 && dice(1, 4) == 1) {
                weather->sky = SKY_CLOUDLESS;
            }
            break;

        case SKY_RAINING:
            if (sim_pressure < 970) {
                if (weather->temperature >= 15 && dice(1, 4) == 1) {
                    weather->sky = SKY_LIGHTNING;
                } else if (weather->temperature <= 0 && dice(1, 2) == 1) {
                    weather->sky = SKY_SNOWING;
                }
            } else if (sim_pressure > 1030) {
                weather->sky = SKY_CLOUDY;
            } else if (sim_pressure > 1010 && dice(1, 4) == 1) {
                weather->sky = SKY_CLOUDY;
            } else if (weather->temperature <= 0 && dice(1, 4) == 1) {
                weather->sky = SKY_SNOWING;
            }
            break;

        case SKY_LIGHTNING:
            if (sim_pressure > 1010) {
                weather->sky = SKY_RAINING;
            } else if (sim_pressure > 990 && dice(1, 4) == 1) {
                weather->sky = SKY_RAINING;
            }
            break;

        case SKY_SNOWING:
            if (sim_pressure > 1030) {
                weather->sky = SKY_CLOUDY;
            } else if (sim_pressure > 1010 && dice(1, 4) == 1) {
                weather->sky = SKY_CLOUDY;
            } else if (weather->temperature > 0 && dice(1, 4) == 1) {
                weather->sky = SKY_CLOUDY;
            }
            break;
    }
}

