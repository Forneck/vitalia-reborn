/**
 * @file weather.c
 *
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
#include "spec_procs.h"

static void another_hour(int mode);
void weather_change(int zone);
extern struct weather_data climates[];
void update_weather(void);

/** Call this function every mud hour to increment the gametime (by one hour)
 * and the weather patterns.
 * @param mode Really, this parameter has the effect of a boolean. In the
 * current incarnation of the function and utility functions, as long as mode
 * is non-zero, the gametime will increment one hour and the weather will be
 * changed.
 */

void update_weather(void)
{
    int i;
    for (i = 0; i <= top_of_zone_table; i++) {
        weather_change(i);
    }
}

void weather_and_time(int mode)
{
    another_hour(mode);
    update_weather();
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
        if (time_info.month >= 1 && time_info.month <= 4) {
            // Inverno (Kames'Hi, Teriany, Hiro, Prudis) - dias curtos
            switch (time_info.hours) {
                case 8:
                    weather_info.sunlight = SUN_RISE;
                    send_to_outdoor("O sol nasceu no leste.\r\n");
                    break;
                case 9:
                    weather_info.sunlight = SUN_LIGHT;
                    send_to_outdoor("O dia comecou.\r\n");
                    break;
                case 16:
                    weather_info.sunlight = SUN_SET;
                    send_to_outdoor("O sol lentamente desaparece no oeste.\r\n");
                    break;
                case 17:
                    weather_info.sunlight = SUN_DARK;
                    send_to_outdoor("A noite comecou.\r\n");
                    break;
                default:
                    break;
            }
        } else if (time_info.month >= 5 && time_info.month <= 8) {
            // Primavera (Maqizie, Kadrictes, Mizu, Mysoluh) - dias aumentando
            switch (time_info.hours) {
                case 6:
                    weather_info.sunlight = SUN_RISE;
                    send_to_outdoor("O sol nasceu no leste.\r\n");
                    break;
                case 7:
                    weather_info.sunlight = SUN_LIGHT;
                    send_to_outdoor("O dia comecou.\r\n");
                    break;
                case 18:
                    weather_info.sunlight = SUN_SET;
                    send_to_outdoor("O sol lentamente desaparece no oeste.\r\n");
                    break;
                case 19:
                    weather_info.sunlight = SUN_DARK;
                    send_to_outdoor("A noite comecou.\r\n");
                    break;
                default:
                    break;
            }
        } else if (time_info.month >= 9 && time_info.month <= 12) {
            // Verão (Karestis, Neruno, Latizie, Aminen) - dias longos
            switch (time_info.hours) {
                case 5:
                    weather_info.sunlight = SUN_RISE;
                    send_to_outdoor("O sol nasceu no leste.\r\n");
                    break;
                case 6:
                    weather_info.sunlight = SUN_LIGHT;
                    send_to_outdoor("O dia comecou.\r\n");
                    break;
                case 20:
                    weather_info.sunlight = SUN_SET;
                    send_to_outdoor("O sol lentamente desaparece no oeste.\r\n");
                    break;
                case 21:
                    weather_info.sunlight = SUN_DARK;
                    send_to_outdoor("A noite comecou.\r\n");
                    break;
                default:
                    break;
            }
        } else {
            // Outono (Autumis, V'tah, Aqrien, Tellus, Brumis) - dias encurtando
            switch (time_info.hours) {
                case 7:
                    weather_info.sunlight = SUN_RISE;
                    send_to_outdoor("O sol nasceu no leste.\r\n");
                    break;
                case 8:
                    weather_info.sunlight = SUN_LIGHT;
                    send_to_outdoor("O dia comecou.\r\n");
                    break;
                case 17:
                    weather_info.sunlight = SUN_SET;
                    send_to_outdoor("O sol lentamente desaparece no oeste.\r\n");
                    break;
                case 18:
                    weather_info.sunlight = SUN_DARK;
                    send_to_outdoor("A noite comecou.\r\n");
                    break;
                default:
                    break;
            }
        }
    }

    if (time_info.hours > 23) { /* Changed by HHS due to bug ??? */
        time_info.hours -= 24;
        time_info.day++;

        if (time_info.day > 34) {
            time_info.day = 0;
            time_info.month++;

            if (time_info.month > 16) {
                time_info.month = 0;
                time_info.year++;
            }

            /* Update QP exchange rate at the start of each new month */
            update_qp_exchange_rate_on_month_change();
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

void weather_change(int zone)
{
    int diff, meant, meanp, sim_pressure, p;
    struct weather_data *climate;

    if (zone < 0 || zone > top_of_zone_table)
        return;
    struct weather_data *weather = zone_table[zone].weather;
    climate = &climates[zone_table[zone].climate];

    if (!weather || !climate) {
        log1("Weather com erro");
        return;
    }

    /* 1. Determina as médias sazonais (per 'help seasons') */
    meant = climate->temperature;
    meanp = climate->pressure;

    if (time_info.month >= 1 && time_info.month <= 4) {           // Inverno (Kames'Hi, Teriany, Hiro, Prudis)
        meant -= climate->temp_diff * 1.5;                        // Redução mais intensa
        meanp += climate->press_diff * 1.2;                       // Aumento da pressão
    } else if (time_info.month >= 5 && time_info.month <= 8) {    // Primavera (Maqizie, Kadrictes, Mizu, Mysoluh)
        meant -= climate->temp_diff * 0.5;                        // Aumento leve
        meanp += climate->press_diff * 0.5;                       // Aumento moderado
    } else if (time_info.month >= 9 && time_info.month <= 12) {   // Verão (Karestis, Neruno, Latizie, Aminen)
        meant += climate->temp_diff * 1.5;                        // Aumento intenso
        meanp -= climate->press_diff * 1.2;                       // Queda na pressão
    } else {                                                      // Outono (Autumis, V'tah, Aqrien, Tellus, Brumis)
        meant += climate->temp_diff * 0.5;                        // Resfriamento moderado
        meanp -= climate->press_diff * 0.5;                       // Queda moderada
    }

    /* 2. Atualiza a pressão atmosférica */
    diff = (weather->pressure > meanp ? -2 : 2);
    weather->press_diff += (dice(1, 3) * diff + dice(1, 6) - dice(1, 6));
    weather->press_diff = URANGE(weather->press_diff, -10, 10);
    weather->pressure += weather->press_diff;
    weather->pressure = URANGE(weather->pressure, 960, 1040);

    /* 3. Atualiza a temperatura */
    diff = (weather->temperature > meant ? -1 : 1);
    int season_modifier = (time_info.month >= 1 && time_info.month <= 4) ? 1 : 2;
    // No inverno, menos variação
    weather->temperature += (weather->press_diff / (5 + season_modifier) + diff * dice(1, 2));
    weather->temperature =
        URANGE(climate->temperature - 15, weather->temperature, climate->temperature + 15);   // Limita variação

    /* 4. Calcula a pressão efetiva simulada (considerando umidade) */
    sim_pressure = weather->pressure - (40 * weather->humidity) + 20;
    weather->before = weather->sky;

    /* 5. Atualiza a umidade */
    diff = (weather->pressure < meanp ? 1 : -1);
    int humidity_factor =
        (time_info.month >= 5 && time_info.month <= 8) || (time_info.month >= 9 && time_info.month <= 12) ? 2 : 1;
    weather->humidity += ((diff * dice(1, 3) + dice(1, 6) - dice(1, 6)) / 100.0) * humidity_factor;
    /* Garante que a umidade nunca seja zero ou negativa - mínimo de 1% */
    weather->humidity =
        URANGE(FMAX(0.01, climate->humidity - 0.2), weather->humidity, climate->humidity + 0.3);   // Maior estabilidade

    /* 6. Atualiza o vento */
    diff = (weather->press_diff > 6 ? 1 : (weather->press_diff < -6 ? -1 : 0));
    int wind_factor =
        (time_info.month >= 9 && time_info.month <= 12) || (time_info.month >= 1 && time_info.month <= 4) ? 2 : 1;
    weather->winds += (diff * dice(1, 2) + dice(1, 3) - dice(1, 3)) * climate->winds * wind_factor;
    weather->winds = URANGE(climate->winds * 3, weather->winds, climate->winds * 25);   // Limita oscilações

    /* 7. Transição entre estados do céu com base em sim_pressure e temperatura */
    switch (weather->sky) {
        case SKY_CLOUDLESS:
            if (weather->humidity > 0.60 && sim_pressure < 1005) {
                weather->sky = SKY_CLOUDY;
                weather->before = SKY_CLOUDLESS;
            }
            break;

        case SKY_CLOUDY:
            if (weather->humidity > 0.80 && sim_pressure < 990) {
                weather->sky = (weather->temperature <= 0 ? SKY_SNOWING : SKY_RAINING);
                weather->before = SKY_CLOUDY;
            } else if (sim_pressure > 1020 && weather->humidity < 0.50 && weather->winds > 6.0) {
                weather->sky = SKY_CLOUDLESS;
                weather->before = SKY_CLOUDY;
            }
            break;

        case SKY_RAINING:
            if (weather->humidity > 0.90 && sim_pressure < 970 && weather->winds > 8.33) {
                weather->sky = SKY_LIGHTNING;
                weather->before = SKY_RAINING;
            } else if (weather->temperature <= 0) {
                weather->sky = SKY_SNOWING;
                weather->before = SKY_RAINING;
            } else if (sim_pressure > 1020 && weather->humidity < 0.50 && weather->winds > 5.0) {
                weather->sky = SKY_CLOUDY;
                weather->before = SKY_RAINING;
            }
            break;

        case SKY_LIGHTNING:
            if (sim_pressure > 990 || weather->humidity < 0.85) {
                weather->sky = SKY_RAINING;
                weather->before = SKY_LIGHTNING;
            }
            break;

        case SKY_SNOWING:
            if (weather->temperature > 0) {
                weather->sky = SKY_CLOUDY;
                weather->before = SKY_SNOWING;
            } else if (sim_pressure > 1020) {
                weather->sky = SKY_CLOUDLESS;
                weather->before = SKY_SNOWING;
            }
            break;
    }

    /* 8. Envio de mensagens ambientais se houver mudança no estado do céu */
    if (weather->sky != weather->before) {
        switch (weather->sky) {
            case SKY_CLOUDLESS:
                send_to_zone_outdoor(zone, "As nuvens se dissipam, revelando um céu limpo.\r\n");
                break;

            case SKY_CLOUDY:
                if (weather->before == SKY_CLOUDLESS)
                    send_to_zone_outdoor(zone, "Nuvens começam a se formar no céu.\r\n");
                else if (weather->before == SKY_SNOWING)
                    send_to_zone_outdoor(zone, "A neve para de cair e as nuvens permanecem.\r\n");
                else
                    send_to_zone_outdoor(zone, "A chuva cessa, mas as nuvens ainda cobrem o céu.\r\n");
                break;

            case SKY_RAINING:
                if (weather->before == SKY_LIGHTNING)
                    send_to_zone_outdoor(zone, "Os relâmpagos cessam, mas a chuva continua.\r\n");
                else if (weather->before == SKY_SNOWING)
                    send_to_zone_outdoor(zone, "A neve se transforma em chuva.\r\n");
                else
                    send_to_zone_outdoor(zone, "A chuva começa a cair suavemente.\r\n");
                break;

            case SKY_LIGHTNING:
                send_to_zone_outdoor(zone, "Relâmpagos iluminam o céu tempestuoso.\r\n");
                break;

            case SKY_SNOWING:
                if (weather->before == SKY_RAINING)
                    send_to_zone_outdoor(zone, "A chuva se transforma em flocos de neve.\r\n");
                else
                    send_to_zone_outdoor(zone, "A neve começa a cair suavemente.\r\n");
                break;
        }

    } else {
        /* 9. Mensagens ambientais ocasionais, baseadas no vento e na temperatura */
        p = rand_number(0, 99);
        switch (weather->sky) {
            case SKY_CLOUDLESS:
            case SKY_CLOUDY:
                /* Ajustamos para que a probabilidade dependa do vento (m/s * 10) */
                if (p < (weather->winds * 10)) {
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
                if (p < (weather->winds * 10)) {
                    if (p < 25)
                        send_to_zone_outdoor(zone, "Uma brisa passa por você, jogando a chuva contra o seu rosto.\r\n");
                    else
                        send_to_zone_outdoor(zone, "A chuva e os fortes ventos dificultam os seus passos.\r\n");
                }
                if (weather->sky == SKY_RAINING) {
                    if (weather->humidity >= 0.75 && p >= 50)
                        send_to_zone_outdoor(zone, "Pesadas gotas de chuva caem com violência.\r\n");
                } else if (weather->sky == SKY_LIGHTNING) {
                    if (weather->humidity >= 0.60 && p < 50)
                        send_to_zone_outdoor(zone, "O som dos trovões preenche o ar.\r\n");
                    else if (weather->humidity >= 0.70 && p < 60)
                        send_to_zone_outdoor(zone, "Um claro relâmpago rasga os céus.\r\n");
                }
                break;

            case SKY_SNOWING:
                if (p < (weather->winds * 10))
                    send_to_zone_outdoor(zone, "O frio vento do inverno parece congelar seus ossos.\r\n");
                else
                    send_to_zone_outdoor(zone, "Pequenos flocos de neve se dispersam pelo ar.\r\n");
                break;
        }
    }
    zone_table[zone].weather = weather;

    /* Apply weather effects to mob emotions in this zone
     * Performance: Only iterate character_list once per zone update
     * Only affect NPCs that are in this zone (both indoor and outdoor)
     * Indoor mobs get reduced effects (50%) handled in apply_weather_to_mood()
     */
    struct char_data *ch;
    for (ch = character_list; ch; ch = ch->next) {
        /* Only affect NPCs that are in this zone */
        if (IS_NPC(ch) && world[IN_ROOM(ch)].zone == zone) {
            apply_weather_to_mood(ch, weather, weather_info.sunlight);
        }
    }
}
