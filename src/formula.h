/* Copyright (c) 1999 castillo7@hotmail.com

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#ifndef SPEDIT_FORMULA
#define SPEDIT_FORMULA

int formula_interpreter (struct char_data *self, struct char_data *vict,
                         int spell_vnum, int syserr, char *cmd, int param, int *rts_code);

#define CODE_SPACE         0     /* a space      */
#define CODE_LOG_AND       1     /* logical      */  
#define CODE_LOG_OR        2  
#define CODE_LOG_GREQ      3  
#define CODE_LOG_GR        4  
#define CODE_LOG_LTEQ      5 
#define CODE_LOG_LT        6
#define CODE_LOG_EQ        7 
#define CODE_LOG_NOTEQ     8
#define CODE_ART_NOT       9     /* arithmetic   */
#define CODE_ART_XOR       10
#define CODE_ART_AND       11
#define CODE_ART_OR        12
#define CODE_ART_SUB       13
#define CODE_ART_ADD       14
#define CODE_ART_DIV       15
#define CODE_ART_MUL       16
#define CODE_ART_OBRACE    17
#define CODE_ART_CBRACE    18
#define CODE_CON_IF        19    /* conditional  */
#define CODE_CON_ELSE      20 
#define CODE_ART_RAND      21    /* arithmetic   */
#define CODE_IDE_SELF      22    /* identifier   */
#define CODE_IDE_VICT      23
#define CODE_ART_MOD       24

#define CODE_VAR_FIRST     25    /* variable     */
#define CODE_VAR_STR       25
#define CODE_VAR_DEX       26
#define CODE_VAR_INT       27
#define CODE_VAR_WIS       28
#define CODE_VAR_CON       29
#define CODE_VAR_CHA       30
#define CODE_VAR_CLASS     31
#define CODE_VAR_LEVEL     32
#define CODE_VAR_AGE       33
#define CODE_VAR_WEIGHT    34
#define CODE_VAR_HEIGHT    35
#define CODE_VAR_MAXMANA   36
#define CODE_VAR_MAXHIT    37
#define CODE_VAR_MAXMOVE   38
#define CODE_VAR_GOLD      39
#define CODE_VAR_EXP       40
#define CODE_VAR_ARMOR     41
#define CODE_VAR_HITROLL   42
#define CODE_VAR_DAMROLL   43
#define CODE_VAR_SAVPARA   44
#define CODE_VAR_SAVROD    45
#define CODE_VAR_SAVPETRI  46
#define CODE_VAR_SAVBREATH 47
#define CODE_VAR_SAVSPELL  48
#define CODE_VAR_HIT       49
#define CODE_VAR_MANA      50
#define CODE_VAR_MOVE      51
#define CODE_VAR_IS_GOOD   52
#define CODE_VAR_IS_NEUTRAL 53
#define CODE_VAR_IS_EVIL   54
#define CODE_VAR_LAST      54    /* last variable  */ 
#define CODE_ART_DICE      55    /* this was added later */
#define CODE_COMMA         56
#define CODE_PARAM         57

#define CODE_DIGIT         99    /* digit 0 to 9 */
#define ERROR_5000         5000  /* Unknow variable or operator */
#define ERROR_5001         5001  /* formula don't have as much ( as ) */
#define ERROR_5002         5002  /* formula don't have as much ? as : */
#define ERROR_5003         5003  /* failed to resolve the formula */
#define ERROR_5004         5004  /* : expected but not found */
#define ERROR_5005         5005  /* a formula can't end by : */
#define ERROR_5006         5006  /* division by 0 */
#define ERROR_5007         5007  /* +++ Unsupported */
#define ERROR_5008         5008  /* --- Unsupported */
#define ERROR_5009         5009  /* formula don't have as much 'dice' as ',' */ 
#define ERROR_5010         5010  /* , expected but not found */
#define LAST_5K_ERROR      5010
#define ERROR_6000         6000  /* a formula can't start by */
#define ERROR_7000         7000  /* a formula can't end by */
#define ERROR_8000         8000  /* no self passed to formula interpreter */

#define CHAR_CODE(x)       ((x) == CODE_DIGIT ? 'y' : \
                            (x) == CODE_PARAM ? 'y' : \
                            (x) == CODE_COMMA ? 'B' : \
                            (x) == CODE_ART_DICE ? 'A' : \
                            (x) >= CODE_VAR_FIRST ? 'z' : \
                            (x) -  CODE_LOG_AND   + 'a') 

struct formula {
  int    command;
  int    value;
  int    sign;
  struct formula *next;
  struct formula *prev;
};

#endif
