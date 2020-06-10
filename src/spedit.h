/* Copyright (c) 2020 castillo7@hotmail.com

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#ifndef SPEDIT
#define SPEDIT

/* all by castillo */
#define unavailable             0
#define available               1
#define NUM_CHAR_POSITION       9
#define NUM_SPELL_FLAGS         13
#define MAX_SPELL_DELAY         50   /* this equal to 5 sec */
#define SPELL                   'P'
#define SKILL                   'K'
#define SPSK                    'Z'
#define MAX_SPELL_AFFECTS       6    /* modifying thos MAX_SPELL would require a FULL spells database upgrade. */
#define MAX_SPELL_PROTECTIONS   6    /* for e.g: spedit_save_to_disk */
#define MAX_SPELL_OBJECTS       3
#define MAX_SPELL_DISPEL        3

#define DB_CODE_INIT_VARS       1
#define DB_CODE_NAME            2
#define DB_CODE_DAMAGES         3 
#define DB_CODE_EFFECTIVENESS   4
#define DB_CODE_DELAY           5
#define DB_CODE_SCRIPT          6
#define DB_CODE_END             7
#define DB_CODE_SUMMON_MOB      8
#define DB_CODE_SUMMON_REQ      9
#define DB_CODE_MSG_WEAR_OFF    10 
#define DB_CODE_MSG_TO_SELF     11
#define DB_CODE_MSG_TO_VICT     12
#define DB_CODE_MSG_TO_ROOM     13 /* 14 to 24 are free */
#define DB_CODE_DISPEL_1        25
#define DB_CODE_DISPEL_2        26
#define DB_CODE_DISPEL_3        27 /* 28 to 39 are free */
#define DB_CODE_OBJECTS_1       40
#define DB_CODE_OBJECTS_2       41
#define DB_CODE_OBJECTS_3       42 /* 43 to 49 are free */
#define DB_CODE_PTS_HP          50
#define DB_CODE_PTS_MANA        51
#define DB_CODE_PTS_MOVE        52
#define DB_CODE_PTS_GOLD        53
#define DB_CODE_PTS_BREATH        54
  /* 55 to 59 are free */
#define DB_CODE_PROT_1          60
#define DB_CODE_PROT_2          61
#define DB_CODE_PROT_3          62
#define DB_CODE_PROT_4          63
#define DB_CODE_PROT_5          64
#define DB_CODE_PROT_6          65 /* 66 to 79 are free */
#define DB_CODE_AFFECTS_1       80
#define DB_CODE_AFFECTS_2       81
#define DB_CODE_AFFECTS_3       82
#define DB_CODE_AFFECTS_4       83
#define DB_CODE_AFFECTS_5       84
#define DB_CODE_AFFECTS_6       85 /* 86 to 89 are free */
#define DB_CODE_CLASS_MU        90
#define DB_CODE_CLASS_CL        91
#define DB_CODE_CLASS_TH        92
#define DB_CODE_CLASS_WA        93
#define DB_CODE_CLASS_RA        94
 /* 95 tp 98 are free */
#define DB_CODE_MARKER          99

extern char *UNDEF_SPELL;

struct str_spells *get_spell_by_vnum(int vnum);
struct str_spells *get_spell_by_name (char *name, char type);
int get_spell_level(int vnum, int class);
int get_spell_class(struct str_spells *spell, int class);
int get_spell_mag_flags(int vnum);

void spedit_init_new_spell (struct str_spells *spell);
void spedit_save_internally (struct str_spells *spell);
void spedit_save_to_disk(void);

char *get_spell_name(int vnum);

struct str_prot {
   int  prot_num;
   char *duration;
   char *resist;
};

struct str_appl {
   int  appl_num;
   char *modifier;
   char *duration;  
};

struct str_assign {
   int  class_num;
   int  level;
   char *prac_gain;
   char *num_mana;
};

struct str_mesg {
   char *wear_off;
   char *to_self;
   char *to_vict;
   char *to_room;
};

struct str_pts {
   char *hp;
   char *mana;
   char *move;
   char *gold;
   char *breath;
};

struct str_spells {
   char type;
   int  vnum;
   int  status;
   int  min_pos;
   int  max_dam;
   int  violent;
   char *name;
   char *delay;
   int  targ_flags;
   int  mag_flags;
   char *damages;
   char *effectiveness;
   char *script;
   char *objects[MAX_SPELL_OBJECTS];
   char *dispel[MAX_SPELL_DISPEL];
   char *summon_mob;
   char *summon_req;
   struct str_prot protfrom [MAX_SPELL_PROTECTIONS];
   struct str_appl applies [MAX_SPELL_AFFECTS];
   struct str_assign assign [NUM_CLASSES]; 
   struct str_mesg messages;
   struct str_pts points;
   void *function;
   struct str_spells *next;
};

void spedit_free_spell (struct str_spells *spell);
void spedit_main_menu (struct descriptor_data *d);
void spedit_string_cleanup (struct descriptor_data *d, int action);
#endif