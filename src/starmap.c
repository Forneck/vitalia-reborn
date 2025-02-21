/* Ported to Circle 3.1 by Edward Glamkowski (ejg)   05/08/03 
 *
 * I got the original code from: http://luisso.net/starmap.txt
 *
 * This is practically a drop-in snippet.  You only need make one change
 * elsewhere in the code.  Specifically, in act.informative.c, in do_look():
 *
    if (!*arg)
      look_at_room(ch, 1);
+   else if (is_abbrev(arg, "sky"))
+     look_at_sky(ch);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
 *
 * Of course, you'll need to add starmap.c to the Makefile, but I leave
 * that to you since that's a compiler specific thing.
 *
 * NOTE: I did replace the color codes in the original with the ANSI 
 *       codes from screen.h for better CircleMUD portability.  This
 *       results in a loss of information - there's only 7 colors in
 *       screen.h but 13 heavenly bodies to be displayed.  Oh well,
 *       that's as good as can be done under the circumstances.  If
 *       you use some alternate color system, be sure to do make the
 *       appropriate changes here!
 */

/************************************STARMAP*******************************
Star Map - by Nebseni of Clandestine MUD. 

Ported to Smaug Code by Desden, el Chaman Tibetano (jose@luisso.net)
Web:luisso.net/inicio.htm
- July 1999

This snippet allows players to look at the sky and see a realistic
display of stars at night, including a moon that moves across the sky
in various phases; during the day they will only see the sun (and moon
if visible).

Permission is granted to use this snippet as long as my name appears
in a help file similar to the one recommended below.

Some of these are based on actual real world (where's that?)
constellations, and some are entirely my imagination.  Feel free to
customize these to fit your own MUD.  The letters are color codes
which will be replaced by the appropriate color code when displayed,
e.g., 'W' ==> "&W ". (White)

This snippet is mostly self-contained.  The only modifications needed
to your other code are a call to this function from someplace in your
code.  My recommendation is to add an option to do_look calling this
function if the argument is "sky" with a check that the room is
outside, such as this: If it is clouded, you cannot see the stars
either ;)
 
********** The following help file is recommended *********
-1 SKY STARS STARMAP CONSTELLATION~
The night sky in the ..(put here your Mud name) is host to many stars.
The ancients recognized patterns in these stars and named many
constellations after them.  Throughout the year you may be able
to 'look sky' and see some of the most famous ones, including:

&OCygnus      &cUrsa Minor  &GOrion   &WDragon  &PLeo    &gVenus  &RMars
&CCassiopeia  &BMercurius   &bUranus  &pCrown   &rPluto  &YRaptor   

StarMap was written by Nebseni of Clandestine MUD and ported to Smaug
by Desden, el Chaman Tibetano.
~
*/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "comm.h"
#include "db.h"
#include "screen.h"


#define NUM_DAYS 35
/* Match this to the number of days per month; this is the moon cycle */
/* EJG: Should match what is in the do_time function in act.informative.c */

#define NUM_MONTHS 17
/* Match this to the number of months defined in month_name[].  */
/* EJG: Should match what is in the do_time function in act.informative.c */

#define MAP_WIDTH 72
#define MAP_HEIGHT 8
/* Should be the string length and number of the constants below.*/

/* extern vars */
extern struct time_info_data time_info;

/* locals vars */
const char * star_map[] = {
  "                                               C. C.                  g*",
  "    O:       R*        G*    G.  W* W. W.          C. C.    Y* Y. Y.    ",
  "  O*.                c.          W.W.     W.            C.       Y..Y.  ",
  "O.O. O.              c.  G..G.           W:      B*                   Y.",
  "     O.    c.     c.                     W. W.                  r*    Y.",
  "     O.c.     c.      G.             P..     W.        p.      Y.   Y:  ",
  "        c.                    G*    P.  P.           p.  p:     Y.   Y. ",
  "                 b*             P.: P*                 p.p:             "
};

/****************** CONSTELLATIONS and STARS *****************************
  Cygnus     Mars        Orion      Dragon       Cassiopeia          Venus
           Ursa Minor                           Mercurius     Pluto    
               Uranus              Leo                Crown       Raptor
*************************************************************************/

const char * sun_map[] = {
  "\\`|'/",
  "- O -",
  "/.|.\\"
};

const char * moon_map[] = {
  " @@@ ",
  "@@@@@",
  " @@@ "
};


void look_at_sky(struct char_data *ch)
{
  static char buf[MAX_STRING_LENGTH];
  int starpos, sunpos, moonpos, moonphase, i, linenum;

  if (!OUTSIDE(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "Vá para fora para olhar o ceu!\r\n");
    return;
  }

  send_to_char(ch, "Você olha para o ceu e vê:\r\n");

  if (zone_table[world[IN_ROOM(ch)].zone].weather->sky != SKY_CLOUDLESS && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "Nuvens! Tente novamente quando o ceu estiver mais limpo.\r\n");
    return;
  }

  sunpos  = (MAP_WIDTH * (24 - time_info.hours) / 24);
  moonpos = (sunpos + time_info.day * MAP_WIDTH / NUM_DAYS) % MAP_WIDTH;

  if ((moonphase = ((((MAP_WIDTH + moonpos - sunpos ) % MAP_WIDTH ) + (MAP_WIDTH/16)) * 8 ) / MAP_WIDTH) > 4) 
    moonphase -= 8;

  starpos = (sunpos + MAP_WIDTH * time_info.month / NUM_MONTHS) % MAP_WIDTH;

  /* The left end of the star_map will be straight overhead at
   * midnight during month 0. */
  for (linenum = 0; linenum < MAP_HEIGHT; linenum++) {
    if ((time_info.hours >= 6 && time_info.hours <= 18) && 
        (linenum < 3 || linenum >= 6))
      continue;

    sprintf(buf, " ");

    /* for (i = MAP_WIDTH/4; i <= 3*MAP_WIDTH/4; i++)*/
    for (i = 1; i <= MAP_WIDTH; i++) {
      /* plot moon on top of anything else...unless new moon & no eclipse */
      if ((time_info.hours >= 6 && time_info.hours <= 18) &&  /* daytime? */
          (moonpos >= MAP_WIDTH/4 - 2) && (moonpos <= 3*MAP_WIDTH/4 + 2) && /* in sky? */
          (i >= moonpos - 2) && (i <= moonpos + 2) && /* is this pixel near moon? */
          ((sunpos == moonpos && time_info.hours == 12) || moonphase != 0) && /*no eclipse*/
          (moon_map[linenum-3][i+2-moonpos] == '@')) {
        if ((moonphase < 0 && i - 2 - moonpos >= moonphase) ||
            (moonphase > 0 && i + 2 - moonpos <= moonphase)) 
          snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                   "%s@", CCWHT(ch, C_SPR));
        else
          strcat(buf, " ");

      } else if ((linenum >= 3) && (linenum < 6) && /* nighttime */
                 (moonpos >= MAP_WIDTH/4 - 2) && (moonpos <= 3*MAP_WIDTH/4 + 2) && /* in sky? */
                 (i >= moonpos - 2) && (i <= moonpos + 2) && /* is this pixel near moon? */
                 (moon_map[linenum-3][i+2-moonpos] == '@')) {
        if ((moonphase < 0 && i - 2 - moonpos >= moonphase) ||
            (moonphase > 0 && i + 2 - moonpos <= moonphase)) 
          snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                   "%s@", CCWHT(ch, C_SPR));
        else
          strcat(buf, " ");
      } else { /* plot sun or stars */
        if (time_info.hours >= 6 && time_info.hours <= 18) { /* daytime */
          if (i >= sunpos - 2 && i <= sunpos + 2) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "%s%c", CCYEL(ch, C_SPR), sun_map[linenum-3][i+2-sunpos]);
          } else 
            strcat(buf, " ");
        } else {
          switch (star_map[linenum][(MAP_WIDTH + i - starpos)%MAP_WIDTH]) {
          default     : strcat(buf," ");    break;
          case ':'    : strcat(buf,":");    break;
          case '.'    : strcat(buf,".");    break;
          case '*'    : strcat(buf,"*");    break;
          case 'G'    : strcat(buf,CCGRN(ch, C_SPR)); strcat(buf, " ");  break;
          case 'g'    : strcat(buf,CCGRN(ch, C_SPR)); strcat(buf, " ");  break;
          case 'R'    : strcat(buf,CCRED(ch, C_SPR)); strcat(buf, " ");  break;
          case 'r'    : strcat(buf,CCRED(ch, C_SPR)); strcat(buf, " ");  break;
          case 'C'    : strcat(buf,CCCYN(ch, C_SPR)); strcat(buf, " ");  break;
          case 'c'    : strcat(buf,CCCYN(ch, C_SPR)); strcat(buf, " ");  break;
          case 'B'    : strcat(buf,CCBLU(ch, C_SPR)); strcat(buf, " ");  break;
          case 'b'    : strcat(buf,CCBLU(ch, C_SPR)); strcat(buf, " ");  break;
          case 'P'    : strcat(buf,CCMAG(ch, C_SPR)); strcat(buf, " ");  break;
          case 'p'    : strcat(buf,CCMAG(ch, C_SPR)); strcat(buf, " ");  break;
          case 'O'    : strcat(buf,CCWHT(ch, C_SPR)); strcat(buf, " ");  break;
          case 'W'    : strcat(buf,CCWHT(ch, C_SPR)); strcat(buf, " ");  break;
          case 'Y'    : strcat(buf,CCYEL(ch, C_SPR)); strcat(buf, " ");  break;
          }
        }
      }
    }
    strcat(buf, "\r\n");
    send_to_char(ch,"%s", buf);
  }

  send_to_char(ch, "%s\r\n", CCNRM(ch, C_SPR));
}
