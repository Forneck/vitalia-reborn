/* **************************************************************************
* File: quality.h Part of tbaMUD *
* Usage: Header file for item quality system *
* Author: Generated for Vitalia Reborn MUD *
* *
* All rights reserved. See license for complete information. *
* *
* Item Quality System - Modular implementation *
* Copyright (C) 2025 by Vitalia Reborn Development Team *
************************************************************************** */

#ifndef _QUALITY_H_
#define _QUALITY_H_

/* Quality Types */
#define QUALITY_COMUM 0
#define QUALITY_INCOMUM 1
#define QUALITY_RARO 2
#define QUALITY_EPICO 3
#define QUALITY_LENDARIO 4
#define QUALITY_OBRA_PRIMA 5
#define QUALITY_DIVINO 6

#define NUM_ITEM_QUALITIES 7

/* Quality Configuration */
struct quality_data {
    int quality_type;        /* Quality level */
    float stat_multiplier;   /* Multiplier for item stats */
    char *name;             /* Quality name display */
    char *color_code;       /* Color code for display */
};

/* External variables */
extern struct quality_data quality_table[NUM_ITEM_QUALITIES];
extern int quality_system_enabled;

/* Function prototypes */
void load_quality_config(void);
void save_quality_config(void);
int determine_item_quality(struct obj_data *obj, int force_divine);
void apply_item_quality(struct obj_data *obj, int quality);
void quality_modify_object(struct obj_data *obj);
char *get_quality_name(int quality);
char *get_quality_display(struct obj_data *obj);
void show_quality_info(struct char_data *ch);
int get_item_quality(struct obj_data *obj);

/* Quality System Commands */
ACMD(do_quality_admin);
ACMD(do_quality_toggle);

/* Hook functions */
void quality_hook_object_load(struct obj_data *obj);
void quality_hook_object_create(struct obj_data *obj, int force_divine);

/* Config functions */
void init_quality_system(void);
void cleanup_quality_system(void);

/* Quality checks - CORRIGIDO para usar IS_SET */
#define HAS_QUALITY(obj) IS_SET(GET_OBJ_EXTRA(obj)[0], (1 << ITEM_QUALITY))
#define QUALITY_ENABLED() (quality_system_enabled)

/* Configuration file path */
#define QUALITY_CONFIG_FILE "lib/etc/quality.conf"

/* Object quality macros */
#define GET_OBJ_QUALITY(obj) ((obj)->quality)
#define SET_OBJ_QUALITY(obj, val) ((obj)->quality = (val))

/* NOTA: ITEM_QUALITY Ã© definido em structs.h como 23 (posiÃ§Ã£o do bit)
   NÃ£o redefinir aqui para evitar conflitos de compilaÃ§Ã£o */

#endif /* _QUALITY_H_ */