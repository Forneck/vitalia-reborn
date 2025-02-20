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
static void weather_change(zone_rnum zone);

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

/** Controls the in game weather system. If the weather changes, an information
 * update is sent via send_to_outdoors().
 * @todo There are some hard coded values that could be extracted to make
 * customizing the weather patterns easier.
 */  
static void weather_change(zone_rnum zone)
{
  int diff, meant, meanp, sim_pressure,press_diff;
  zone_table[zone].weather->before = zone_table[zone].weather->sky;
    
  if ((time_info.month >= 9) && (time_info.month <= 16))
    diff = (zone_table[zone].weather->pressure > 985 ? -2 : 2);
  else
    diff = (zone_table[zone].weather->pressure > 1015 ? -2 : 2);

  zone_table[zone].weather->press_diff += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

  zone_table[zone].weather->press_diff = MIN(zone_table[zone].weather->press_diff, 12);
  zone_table[zone].weather->press_diff = MAX(zone_table[zone].weather->press_diff, -12);

  zone_table[zone].weather->pressure += zone_table[zone].weather->press_diff;

  zone_table[zone].weather->pressure = MIN(zone_table[zone].weather->pressure, 1040);
  zone_table[zone].weather->pressure = MAX(zone_table[zone].weather->pressure, 960);

  press_diff = 0;

  switch (zone_table[zone].weather->sky) {
  case SKY_CLOUDLESS:
    if (zone_table[zone].weather->pressure < 990)
      press_diff = 1;
    else if (zone_table[zone].weather->pressure < 1010)
      if (dice(1, 4) == 1)
	press_diff = 1;
    break;
  case SKY_CLOUDY:
    if (zone_table[zone].weather->pressure < 970)
      press_diff = 2;
    else if (zone_table[zone].weather->pressure < 990) {
      if (dice(1, 4) == 1)
	press_diff = 2;
      else
	press_diff = 0;
    } else if (zone_table[zone].weather->pressure > 1030)
      if (dice(1, 4) == 1)
	press_diff = 3;

    break;
  case SKY_RAINING:
    if (zone_table[zone].weather->pressure < 970) {
      if (dice(1, 4) == 1)
	press_diff = 4;
      else
	press_diff = 0;
    } else if (zone_table[zone].weather->pressure > 1030)
      press_diff = 5;
    else if (zone_table[zone].weather->pressure > 1010)
      if (dice(1, 4) == 1)
	press_diff = 5;

    break;
  case SKY_LIGHTNING:
    if (zone_table[zone].weather->pressure > 1010)
      press_diff = 6;
    else if (zone_table[zone].weather->pressure > 990)
      if (dice(1, 4) == 1)
	press_diff = 6;

    break;
  default:
    press_diff = 0;
    zone_table[zone].weather->sky = SKY_CLOUDLESS;
    break;
  }

  switch (press_diff) {
  case 0:
    break;
  case 1:
    send_to_outdoor("O ceu comeca a ficar nublado.\r\n");
    zone_table[zone].weather->sky = SKY_CLOUDY;
    break;
  case 2:
    send_to_outdoor("A chuva comeca a cair.\r\n");
    zone_table[zone].weather->sky = SKY_RAINING;
    break;
  case 3:
    send_to_outdoor("As nuvens desaparecem.\r\n");
    zone_table[zone].weather->sky = SKY_CLOUDLESS;
    break;
  case 4:
  send_to_outdoor("Relampagos comecam a iluminar o ceu.\r\n");
    zone_table[zone].weather->sky = SKY_LIGHTNING;
    break;
  case 5:
    send_to_outdoor("A chuva parou.\r\n");
    zone_table[zone].weather->sky = SKY_CLOUDY;
    break;
  case 6:
    send_to_outdoor("Os relampagos param.\r\n");
    zone_table[zone].weather->sky = SKY_RAINING;
    break;
  default:
    break;
  }
}



