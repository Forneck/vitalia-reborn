/* Copyright (c) 2018 castillo7@hotmail.com

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h" 
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "act.h"
#include "formula.h"
#include "spells_script.h"
#include "spells.h"
#include "string.h"

int call_ASCRIPT (int (*function)(), char *str, struct char_data *self,
                                                struct char_data *vict,
                                                struct obj_data  *ovict,
                                                int    from, 
                                                int    param) 
{
 return ((*function) (str, self, vict, ovict, from, param));
}  

/* let's declare our scripts functions */
ASCRIPT (scr_act_to);
ASCRIPT (scr_teleport);
ASCRIPT (scr_load_mobile);

struct str_script list_script[] = {
  {"SAY_TO_ROOM", scr_act_to,      TO_ROOM},
  {"SAY_TO_SELF", scr_act_to,      TO_CHAR},
  {"SAY_TO_CHAR", scr_act_to,      TO_CHAR},
  {"SAY_TO_VICT", scr_act_to,      TO_VICT},
  {"TELEPORT",    scr_teleport,    0},
  {"LOAD_MOBILE", scr_load_mobile, 0},
  {"\n", NULL, 0}
};

/* this functions is similar to strstr, but will ignore space in str1 and
   it will move the pointer of str1 */
char *strlstr (char **str1, char *str2)
{
 int cpt = 0;
 int len = strlen(str2);
 char *p = *str1;

 if ((!*str1) || (!str2))
   return NULL;

 do {
   if (*p == ' ') continue;
   if ((*p == '\r') || (*p == '\n')) return NULL;
   if (*p == str2[cpt]) {
     if (++cpt >= len) {
       p++;
       *str1 = p;
       return p;
     }
   }
   else
     cpt = 0; 
 } while (*++p);
 return NULL;
}

/* This function extract from *str into the buffer buf, until it found the 
   character c and it will move the pointer of *str */
char *strext (char *buf, char **str, char c)
{
 int i, len;
 char *p = *str;

 len = MIN(strlen(*str), MAX_STRING_LENGTH-1);

 if (len <= 0)
   return NULL;

 for (i=0; i<len; i++) {
   if ((*p == '\r') || (*p == '\n')) return NULL;
   if (*p == c)
     break;
   else {
     buf[i] = *p++;
   }
 }
 *str = ++p;
 buf[i] = '\0';
 return p;
}

// SAY_TO_ROOM {"message"};
// SAY_TO_SELF {"message"};
// SAY_TO_CHAR {"message"};
// SAY_TO_VICT {"message"};
ASCRIPT(scr_act_to)
{
 char message[MAX_STRING_LENGTH] = "";

 strlstr (str, "{\"");
 if (!strext ((char *)&message, str, '\"')) return FALSE;
 strlstr (str, "};");

 act (message, TRUE, self, NULL, vict, param); 
 return TRUE;
}  

// TELEPORT {"where"};
// where = vnum or mobile name
ASCRIPT(scr_teleport)
{
 int effect = TRUE;
 char where[MAX_STRING_LENGTH] = "";
 room_vnum location;

 strlstr (str, "{\"");
 if (!strext ((char *)&where, str, '\"')) return FALSE;
 strlstr (str, "};");

 if (((location = find_target_room (vict, where)) != NOWHERE) && vict) {
   char_from_room (vict);
   char_to_room (vict, location);
   if (!IS_NPC (vict))
     look_at_room (vict, 1);
 } else
     effect = FALSE;
 return (effect);
}

// LOAD_MOBILE {"mob, where"};
// mob = mobile r_num
// where = vnum or mobile name
ASCRIPT(scr_load_mobile)
{
 struct char_data *mob;
 int value, effect = TRUE, rts_code;
 char who[MAX_STRING_LENGTH] = "";
 char where[MAX_STRING_LENGTH] = "";

 room_vnum location;
 mob_rnum r_num;

 strlstr (str, "{\"");
 if (!strext ((char *)&who, str, ',')) return FALSE;
 if (!strext ((char *)&where, str, '\"')) return FALSE;
 strlstr (str, "};");

 value = formula_interpreter (self, vict, from, TRUE, who, GET_LEVEL(self), &rts_code);
 if (((r_num = real_mobile (value)) != NOBODY) &&
     ((location = find_target_room (vict, where)) != NOWHERE)) {
   mob = read_mobile (r_num, REAL);
   char_to_room (mob, location);
 } else
     effect = FALSE;

 return (effect);
}

int perform_script (char *str, struct char_data *self, 
                               struct char_data *vict, 
                               struct obj_data  *ovict, 
                    int   from,
                    int   param)
{
 int str_len, fct_len, cpt = 0, ptr = 0, effect = FALSE;
 char *p, *b;

 p = str;
 b = (char *) &p;

 if (!vict)
   vict = self;

 str_len = strlen(p);
 for (ptr=0; ptr<str_len; ptr++) {
   cpt = 0;
   while (list_script[cpt].function) {
     fct_len = strlen(list_script[cpt].name);
     if (!(strncmp (list_script[cpt].name, p, fct_len))) {
       p += fct_len;
       ptr += fct_len;
       if (call_ASCRIPT (list_script[cpt].function, b, self, vict, ovict, from, list_script[cpt].param))
         effect = TRUE;
       break;
     }
     else
       cpt++;
   }
   while (*++p && ((*p == ' ') || (*p == '\r') || (*p == '\n'))) 
     ptr++;
 }
 return effect ? MAGIC_SUCCESS : MAGIC_NOEFFECT;
}
