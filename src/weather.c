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
    int diff, meant, meanp, sim_pressure, p;
    struct weather_data *weather;
    struct weather_data *climate;

    if (zone < 0 || zone > top_of_zone_table)
        return;

    weather = zone_table[zone].weather;
    climate = &climates[zone_table[zone].climate];

    if (!weather || !climate)
        return;

    /* 1. Determina as médias sazonais */
    meant = climate->temperature;
    meanp = climate->pressure;
    if (time_info.month >= 9 && time_info.month <= 16) {  // Outono/Inverno
        meant -= climate->temp_diff;
        meanp -= climate->press_diff;
    } else {  // Primavera/Verão
        meant += climate->temp_diff;
        meanp += climate->press_diff;
    }

    /* 2. Atualiza a pressão atmosférica */
    diff = (weather->pressure > meanp ? -2 : 2);
    weather->press_diff += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));
    weather->press_diff = URANGE(weather->press_diff, -12, 12);
    weather->pressure += weather->press_diff;
    weather->pressure = URANGE(weather->pressure, 950, 1050);

    /* 3. Atualiza a temperatura */
    diff = (weather->temperature > meant ? -1 : 1);
    weather->temperature += (weather->press_diff / 6 + diff * dice(1, 2));

    /* 4. Calcula a pressão efetiva simulada (considerando umidade) */
    sim_pressure = weather->pressure - (40 * climate->humidity) + 20;
    weather->before = weather->sky;

    /* 5. Transição entre estados do céu com base em sim_pressure e temperatura */
    switch (weather->sky) {
        case SKY_CLOUDLESS:
            if (sim_pressure < 990)
                weather->sky = SKY_CLOUDY;
            else if (sim_pressure < 1010 && dice(1,4) == 1)
                weather->sky = SKY_CLOUDY;
            break;

        case SKY_CLOUDY:
            if (sim_pressure < 970)
                weather->sky = SKY_RAINING;
            else if (sim_pressure < 990 && dice(1,4) == 1)
                weather->sky = (weather->temperature <= 0 ? SKY_SNOWING : SKY_RAINING);
            else if (sim_pressure > 1030 && weather->temperature > 0 && dice(1,4) == 1)
                weather->sky = SKY_CLOUDLESS;
            break;

        case SKY_RAINING:
            if (sim_pressure < 970) {
                if (weather->temperature >= 15 && dice(1,4) == 1)
                    weather->sky = SKY_LIGHTNING;
                else if (weather->temperature <= 0 && dice(1,2) == 1)
                    weather->sky = SKY_SNOWING;
            } else if (sim_pressure > 1030)
                weather->sky = SKY_CLOUDY;
            else if (sim_pressure > 1010 && dice(1,4) == 1)
                weather->sky = SKY_CLOUDY;
            else if (weather->temperature <= 0 && dice(1,4) == 1)
                weather->sky = SKY_SNOWING;
            break;

        case SKY_LIGHTNING:
            if (sim_pressure > 1010)
                weather->sky = SKY_RAINING;
            else if (sim_pressure > 990 && dice(1,4) == 1)
                weather->sky = SKY_RAINING;
            break;

        case SKY_SNOWING:
            if (sim_pressure > 1030)
                weather->sky = SKY_CLOUDY;
            else if (sim_pressure > 1010 && dice(1,4) == 1)
                weather->sky = SKY_CLOUDY;
            else if (weather->temperature > 0 && dice(1,4) == 1)
                weather->sky = SKY_CLOUDY;
            break;
    }

    /* 6. Envio de mensagens ambientais se houver mudança no estado do céu */
    if (weather->sky != weather->before) {
        switch (weather->sky) {
            case SKY_CLOUDLESS:
                send_to_zone_outdoor(zone, "As nuvens desapareceram do céu.\r\n");
                break;
            case SKY_CLOUDY:
                if (weather->before == SKY_CLOUDLESS)
                    send_to_zone_outdoor(zone, "Nuvens misteriosas aparecem no céu.\r\n");
                else if (weather->before == SKY_SNOWING)
                    send_to_zone_outdoor(zone, "A neve parou de cair.\r\n");
                else
                    send_to_zone_outdoor(zone, "A chuva parou.\r\n");
                break;
            case SKY_RAINING:
                if (weather->before == SKY_LIGHTNING)
                    send_to_zone_outdoor(zone, "Os relâmpagos páram.\r\n");
                else if (weather->before == SKY_SNOWING)
                    send_to_zone_outdoor(zone, "A neve pára de cair e dá lugar para a chuva.\r\n");
                else
                    send_to_zone_outdoor(zone, "A chuva começa a cair.\r\n");
                break;
            case SKY_LIGHTNING:
                send_to_zone_outdoor(zone, "Relâmpagos começam a aparecer no céu.\r\n");
                break;
            case SKY_SNOWING:
                if (weather->before == SKY_RAINING)
                    send_to_zone_outdoor(zone, "A chuva pára de cair e começa a nevar.\r\n");
                else
                    send_to_zone_outdoor(zone, "A neve começa a cair.\r\n");
                break;
        }
    } else {
        /* 7. Mensagens ambientais ocasionais, baseadas no vento e na temperatura */
        p = rand_number(0, 99);
        switch (weather->sky) {
            case SKY_CLOUDLESS:
            case SKY_CLOUDY:
                if (p < climate->winds * 100) {
                    if (p < 25)
                        send_to_zone_outdoor(zone, "Você sente uma brisa passando por você.\r\n");
                    else if (p < 50)
                        send_to_zone_outdoor(zone, "Um forte vento açoita o seu rosto.\r\n");
                    else if (p < 75)
                        send_to_zone_outdoor(zone, "A ventania dificulta os seus movimentos.\r\n");
                    else
                        send_to_zone_outdoor(zone, "Os ventos parecem o castigar.\r\n");
                } else if (weather->temperature < 5) {
                    send_to_zone_outdoor(zone, "Uma nevasca parece prestes a cair sobre a região.\r\n");
                }
                break;

            case SKY_RAINING:
            case SKY_LIGHTNING:
                if (p < climate->winds * 100) {
                    if (p < 25)
                        send_to_zone_outdoor(zone, "Uma brisa passa por você, jogando a chuva contra o seu rosto.\r\n");
                    else
                        send_to_zone_outdoor(zone, "A chuva e os fortes ventos dificultam os seus passos.\r\n");
                }
                if (weather->sky == SKY_RAINING) {
                    if (climate->humidity >= 0.75 && p >= 50)
                        send_to_zone_outdoor(zone, "Pesadas gotas de chuva caem com violência.\r\n");
                } else if (weather->sky == SKY_LIGHTNING) {
                    if (climate->humidity >= 0.60 && p < 50)
                        send_to_zone_outdoor(zone, "O som dos trovões preenche o ar.\r\n");
                    else if (climate->humidity >= 0.70 && p < 60)
                        send_to_zone_outdoor(zone, "Um claro relâmpago rasga os céus.\r\n");
                }
                break;

            case SKY_SNOWING:
                if (p < climate->winds * 100)
                    send_to_zone_outdoor(zone, "O frio vento do inverno parece congelar seus ossos.\r\n");
                else
                    send_to_zone_outdoor(zone, "Pequenos flocos de neve se dispersam pelo ar.\r\n");
                break;
        }
    }
}

