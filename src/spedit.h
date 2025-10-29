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
#define unavailable 0
#define available 1
#define NUM_CHAR_POSITION 10
#define NUM_SPELL_FLAGS 13
#define MAX_SPELL_DELAY 50 /* this equal to 5 sec */
#define SPELL 'P'
#define SKILL 'K'
#define CHANSON 'C'
#define SPSK 'Z'
#define MAX_SPELL_AFFECTS 6     /* modifying thos MAX_SPELL would require a FULL spells database upgrade. */
#define MAX_SPELL_PROTECTIONS 6 /* for e.g: spedit_save_to_disk */
#define MAX_SPELL_OBJECTS 3
#define MAX_SPELL_DISPEL 3

#define DB_CODE_INIT_VARS 1
#define DB_CODE_NAME 2
#define DB_CODE_DAMAGES 3
#define DB_CODE_EFFECTIVENESS 4
#define DB_CODE_DELAY 5
#define DB_CODE_SCRIPT 6
#define DB_CODE_END 7
#define DB_CODE_SUMMON_MOB 8
#define DB_CODE_SUMMON_REQ 9
#define DB_CODE_MSG_WEAR_OFF 10
#define DB_CODE_MSG_TO_SELF 11
#define DB_CODE_MSG_TO_VICT 12
#define DB_CODE_MSG_TO_ROOM 13 /* 14 to 24 are free */
#define DB_CODE_DISPEL_1 25
#define DB_CODE_DISPEL_2 26
#define DB_CODE_DISPEL_3 27 /* 28 to 39 are free */
#define DB_CODE_OBJECTS_1 40
#define DB_CODE_OBJECTS_2 41
#define DB_CODE_OBJECTS_3 42 /* 43 to 49 are free */
#define DB_CODE_PTS_HP 50
#define DB_CODE_PTS_MANA 51
#define DB_CODE_PTS_MOVE 52

/* Spell Schools (Escolas) */
#define SCHOOL_UNDEFINED 0
#define SCHOOL_ABJURATION 1  /* Abjuração */
#define SCHOOL_ALTERATION 2  /* Alteração */
#define SCHOOL_CONJURATION 3 /* Conjuração */
#define SCHOOL_DIVINATION 4  /* Adivinhação */
#define SCHOOL_ENCHANTMENT 5 /* Encantamento */
#define SCHOOL_EVOCATION 6   /* Evocação */
#define SCHOOL_ILLUSION 7    /* Ilusão */
#define SCHOOL_NECROMANCY 8  /* Necromancia */
#define NUM_SCHOOLS 9

/* Skill Schools/Styles - for non-magical abilities */
#define SKILL_SCHOOL_UNDEFINED 0
#define SKILL_SCHOOL_COMBAT 1   /* Combat Style - melee combat abilities */
#define SKILL_SCHOOL_STEALTH 2  /* Stealth Style - hiding, sneaking abilities */
#define SKILL_SCHOOL_WEAPONS 3  /* Weapon Style - weapon proficiencies */
#define SKILL_SCHOOL_SURVIVAL 4 /* Survival Style - outdoor and utility skills */
#define SKILL_SCHOOL_MUSICAL 5  /* Musical Style - bard abilities */
#define SKILL_SCHOOL_SUPPORT 6  /* Support Style - defensive and healing abilities */
#define NUM_SKILL_SCHOOLS 7

/* Spell Elements */
#define ELEMENT_UNDEFINED 0
#define ELEMENT_FIRE 1      /* Fogo */
#define ELEMENT_WATER 2     /* Água */
#define ELEMENT_AIR 3       /* Ar */
#define ELEMENT_EARTH 4     /* Terra */
#define ELEMENT_LIGHTNING 5 /* Raio */
#define ELEMENT_ICE 6       /* Gelo */
#define ELEMENT_ACID 7      /* Ácido */
#define ELEMENT_POISON 8    /* Veneno */
#define ELEMENT_HOLY 9      /* Sagrado */
#define ELEMENT_UNHOLY 10   /* Profano */
#define ELEMENT_MENTAL 11   /* Mental */
#define ELEMENT_PHYSICAL 12 /* Físico */
#define NUM_ELEMENTS 13
#define DB_CODE_PTS_GOLD 53
#define DB_CODE_PTS_BREATH 54
#define DB_CODE_SCHOOL 55  /* Spell school */
#define DB_CODE_ELEMENT 56 /* Spell element */
/* 57 to 59 are free */
#define DB_CODE_PROT_1 60
#define DB_CODE_PROT_2 61
#define DB_CODE_PROT_3 62
#define DB_CODE_PROT_4 63
#define DB_CODE_PROT_5 64
#define DB_CODE_PROT_6 65 /* 66 to 79 are free */
#define DB_CODE_AFFECTS_1 80
#define DB_CODE_AFFECTS_2 81
#define DB_CODE_AFFECTS_3 82
#define DB_CODE_AFFECTS_4 83
#define DB_CODE_AFFECTS_5 84
#define DB_CODE_AFFECTS_6 85 /* 86 to 89 are free */
#define DB_CODE_CLASS_MU 90
#define DB_CODE_CLASS_CL 91
#define DB_CODE_CLASS_TH 92
#define DB_CODE_CLASS_WA 93
#define DB_CODE_CLASS_RA 94
/* 95 tp 98 are free */
#define DB_CODE_MARKER 99

extern char *UNDEF_SPELL;
extern struct str_spells *list_spells;

struct str_spells *get_spell_by_vnum(int vnum);
struct str_spells *get_spell_by_name(char *name, char type);
int get_spell_level(int vnum, int class);
int get_spell_class(struct str_spells *spell, int class);
int get_spell_mag_flags(int vnum);

void spedit_init_new_spell(struct str_spells *spell);
void spedit_save_internally(struct str_spells *spell);
void spedit_save_to_disk(void);

char *get_spell_name(int vnum);

struct str_prot {
    int prot_num;
    char *duration;
    char *resist;
};

struct str_appl {
    int appl_num;
    char *modifier;
    char *duration;
};

struct str_assign {
    int class_num;
    int level;
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
    int vnum;
    int status;
    int min_pos;
    int max_dam;
    int violent;
    char *name;
    char *delay;
    int targ_flags;
    int mag_flags;
    char *damages;
    char *effectiveness;
    char *script;
    char *objects[MAX_SPELL_OBJECTS];
    char *dispel[MAX_SPELL_DISPEL];
    char *summon_mob;
    char *summon_req;
    struct str_prot protfrom[MAX_SPELL_PROTECTIONS];
    struct str_appl applies[MAX_SPELL_AFFECTS];
    struct str_assign assign[NUM_CLASSES];
    struct str_mesg messages;
    struct str_pts points;
    int school;             /* Spell school (Escola) */
    int element;            /* Spell element (hidden) */
    int prerequisite_spell; /* Spell vnum needed to discover this variant (0 = none) */
    int discoverable;       /* Can be discovered through syllable experimentation (0 = no, 1 = yes) */
    void *function;
    struct str_spells *next;
};

/* Global list of all spells */
extern struct str_spells *list_spells;

void spedit_free_spell(struct str_spells *spell);
void spedit_main_menu(struct descriptor_data *d);
void spedit_string_cleanup(struct descriptor_data *d, int action);
#endif
