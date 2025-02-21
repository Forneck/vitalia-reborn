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

void weather_change(zone_rnum zone) {
    if (zone < 0 || zone > top_of_zone_table) {
        fprintf(stderr, "Erro: Zona inválida #%d\n", zone);
        return;
    }

    struct weather_data *clima = zone_table[zone].weather;

    if (!clima) {
        fprintf(stderr, "Erro: Clima não inicializado para a zona #%d\n", zone);
        return;
    }

    printf("Antes: Zona #%d - Temp: %d, Pressão: %d, Céu: %d\n",
           zone, clima->temperature, clima->pressure, clima->sky);

    // Simplesmente altera alguns valores para teste
    clima->temperature += (rand() % 3) - 1; // Varia -1 a +1
    clima->pressure += (rand() % 5) - 2; // Varia -2 a +2
    clima->sky = (clima->sky + 1) % 4; // Cicla entre 0-3

    printf("Depois: Zona #%d - Temp: %d, Pressão: %d, Céu: %d\n",
           zone, clima->temperature, clima->pressure, clima->sky);


}

