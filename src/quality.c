/* **************************************************************************
* File: quality.c Part of tbaMUD *
* Usage: Item quality system implementation *
* Author: Generated for Vitalia Reborn MUD *
* *
* All rights reserved. See license for complete information. *
* *
* Item Quality System - Modular implementation *
* Copyright (C) 2025 by Vitalia Reborn Development Team *
************************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "quality.h"
#include "screen.h"

#ifndef Q_NO_CHAR_H
#define Q_NO_CHAR_H
#define SIMPLE_CCNRM() "\x1B[0m" /* Reset de cor ANSI */
#endif

/* Global quality system variables */
int quality_system_enabled = TRUE;

/* Quality table - defines chances, multipliers and display info */
struct quality_data quality_table[NUM_ITEM_QUALITIES] = {
    { QUALITY_COMUM, 1.0, "COMUM", "\tn" },         /* 40% chance */
    { QUALITY_INCOMUM, 1.2, "INCOMUM", "\tG" },     /* 25% chance */
    { QUALITY_RARO, 1.4, "RARO", "\tB" },           /* 20% chance */
    { QUALITY_EPICO, 1.6, "Ã‰PICO", "\tM" },         /* 10% chance */
    { QUALITY_LENDARIO, 1.8, "LENDÃRIO", "\tY" },   /* 4% chance */
    { QUALITY_OBRA_PRIMA, 2.0, "OBRA PRIMA", "\tR" }, /* 1% chance */
    { QUALITY_DIVINO, 2.5, "DIVINO", "\tW" }        /* Admin only */
};

/* Quality chance table (must sum to 100 for non-divine) */
int quality_chances[NUM_ITEM_QUALITIES-1] = {
    40,  /* COMUM */
    25,  /* INCOMUM */
    20,  /* RARO */
    10,  /* EPICO */
    4,   /* LENDARIO */
    1    /* OBRA_PRIMA */
};

/*
 * Initialize the quality system
 */
void init_quality_system(void)
{
    load_quality_config();
    log1("Quality system initialized.");
}

/*
 * Clean up quality system
 */
void cleanup_quality_system(void)
{
    save_quality_config();
    log1("Quality system cleaned up.");
}

/*
 * Load quality configuration from file
 */
void load_quality_config(void)
{
    FILE *fl;
    char line[256];
    
    if (!(fl = fopen(QUALITY_CONFIG_FILE, "r"))) {
        log1("SYSERR: Could not open quality config file, using defaults.");
        return;
    }
    
    /* Read the enabled flag */
    if (get_line(fl, line) && sscanf(line, "ENABLED: %d", &quality_system_enabled) != 1) {
        log1("SYSERR: Invalid quality config format, using defaults.");
        fclose(fl);
        return;
    }
    
    /* Read quality chances */
    for (int i = 0; i < NUM_ITEM_QUALITIES-1; i++) {
        if (get_line(fl, line) && sscanf(line, "CHANCE_%d: %d", &i, &quality_chances[i]) != 2) {
            log1("SYSERR: Invalid quality chance in config, using defaults.");
            break;
        }
    }
    
    fclose(fl);
    log1("Quality configuration loaded successfully.");
}

/*
 * Save quality configuration to file
 */
void save_quality_config(void)
{
    FILE *fl;
    
    if (!(fl = fopen(QUALITY_CONFIG_FILE, "w"))) {
        log1("SYSERR: Could not write quality config file.");
        return;
    }
    
    fprintf(fl, "# Quality System Configuration\n");
    fprintf(fl, "# Generated automatically - do not edit manually\n");
    fprintf(fl, "ENABLED: %d\n", quality_system_enabled);
    
    for (int i = 0; i < NUM_ITEM_QUALITIES-1; i++) {
        fprintf(fl, "CHANCE_%d: %d\n", i, quality_chances[i]);
    }
    
    fclose(fl);
}

/*
 * Determine quality for a new item based on random chance
 * force_divine: 1 to force divine quality (admin command)
 */
int determine_item_quality(struct obj_data *obj, int force_divine)
{
    int roll, total = 0;
    
    if (!QUALITY_ENABLED())
        return QUALITY_COMUM;
        
    if (force_divine)
        return QUALITY_DIVINO;
    
    /* Don't apply quality to certain item types */
    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_LIGHT:
        case ITEM_SCROLL:
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_NOTE:
        case ITEM_DRINKCON:
        case ITEM_KEY:
        case ITEM_FOOD:
        case ITEM_MONEY:
        case ITEM_PEN:
        case ITEM_BOAT:
        case ITEM_FOUNTAIN:
            return -1; /* No quality for these items */
    }
    
    roll = rand_number(1, 100);
    
    /* Check quality chances from highest to lowest (except divine) */
    for (int i = NUM_ITEM_QUALITIES-2; i >= 0; i--) {
        total += quality_chances[i];
        if (roll <= total) {
            return i;
        }
    }
    
    return QUALITY_COMUM; /* Fallback */
}

/*
 * Apply quality modifications to an object
 */
void apply_item_quality(struct obj_data *obj, int quality)
{
    float multiplier;
    
    if (quality < 0 || quality >= NUM_ITEM_QUALITIES)
        return;
        
    /* Verifica se o item jÃ¡ tem qualidade aplicada */
    if (HAS_QUALITY(obj)) {
        log1("SYSERR: Tentativa de aplicar qualidade a item que jÃ¡ possui qualidade");
        return;
    }
    
    multiplier = quality_table[quality].stat_multiplier;
    
    /* Apply multiplier based on item type */
    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WEAPON:
            /* Modify weapon damage */
            GET_OBJ_VAL(obj, 1) = (int)(GET_OBJ_VAL(obj, 1) * multiplier);
            GET_OBJ_VAL(obj, 2) = (int)(GET_OBJ_VAL(obj, 2) * multiplier);
            break;
        case ITEM_ARMOR:
            /* Modify armor AC */
            GET_OBJ_VAL(obj, 0) = (int)(GET_OBJ_VAL(obj, 0) * multiplier);
            break;
        case ITEM_WORN:
            /* Apply to any affects */
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                if (obj->affected[i].location != APPLY_NONE && obj->affected[i].modifier > 0) {
                    obj->affected[i].modifier = (int)(obj->affected[i].modifier * multiplier);
                }
            }
            break;
    }
    
    /* Modify item cost based on quality */
    GET_OBJ_COST(obj) = (int)(GET_OBJ_COST(obj) * multiplier);
    GET_OBJ_RENT(obj) = (int)(GET_OBJ_RENT(obj) * multiplier);
    
    /* Store quality in object spare fields */
    SET_OBJ_QUALITY(obj, quality);
    
    /* Set quality flag - CORRIGIDO para usar SET_BIT */
    SET_BIT(GET_OBJ_EXTRA(obj)[0], (1 << ITEM_QUALITY));
}

/*
 * Main function to modify object with quality
 */
void quality_modify_object(struct obj_data *obj)
{
    int quality;
    
    if (!obj || !QUALITY_ENABLED())
        return;
        
    /* Don't modify objects that already have quality */
    if (HAS_QUALITY(obj))
        return;
        
    quality = determine_item_quality(obj, FALSE);
    if (quality >= 0) {
        apply_item_quality(obj, quality);
    }
}

/*
 * Get quality name for display
 */
char *get_quality_name(int quality)
{
    if (quality < 0 || quality >= NUM_ITEM_QUALITIES)
        return "UNKNOWN";
    return quality_table[quality].name;
}

/*
 * Get formatted quality display string for an object
 */
char *get_quality_display(struct obj_data *obj)
{
    static char buf[MAX_STRING_LENGTH];
    int quality;
    
    if (!obj || !HAS_QUALITY(obj)) {
        strcpy(buf, "");
        return buf;
    }
    
    quality = get_item_quality(obj);
    if (quality < 0 || quality >= NUM_ITEM_QUALITIES) {
        strcpy(buf, "");
        return buf;
    }
    
    sprintf(buf, "%s[%s]%s ",
        quality_table[quality].color_code,
        quality_table[quality].name,
        SIMPLE_CCNRM());
    return buf;
}

/*
 * Get object name with quality prefix for display
 */
const char *get_quality_obj_name(struct obj_data *obj)
{
    static char buf[MAX_STRING_LENGTH];
    
    if (!obj) {
        return NULL;
    }
    
    // Se o objeto nÃ£o tem qualidade, retorna descriÃ§Ã£o normal
    if (!HAS_QUALITY(obj)) {
        return obj->short_description;
    }
    
    // ConstrÃ³i o nome com qualidade
    snprintf(buf, sizeof(buf), "%s%s",
        get_quality_display(obj),
        obj->short_description);
    return buf;
}

/*
 * Get item quality from object
 */
int get_item_quality(struct obj_data *obj)
{
    if (!obj || !HAS_QUALITY(obj))
        return -1;
    return GET_OBJ_QUALITY(obj);
}

/*
 * Hook function called when object is loaded from world files
 */
void quality_hook_object_load(struct obj_data *obj)
{
    if (QUALITY_ENABLED()) {
        quality_modify_object(obj);
    }
}

/*
 * Hook function called when object is created via commands
 */
void quality_hook_object_create(struct obj_data *obj, int force_divine)
{
    int quality;
    
    if (!QUALITY_ENABLED())
        return;
        
    quality = determine_item_quality(obj, force_divine);
    if (quality >= 0) {
        apply_item_quality(obj, quality);
    }
}

/*
 * Show quality system information to a player
 */
void show_quality_info(struct char_data *ch)
{
    char buf[MAX_STRING_LENGTH];
    
    sprintf(buf, "\r\n&c--- Sistema de Qualidade de Itens ---&n\r\n");
    send_to_char(ch, buf);
    
    sprintf(buf, "Status: %s\r\n", QUALITY_ENABLED() ? "&GAtivo&n" : "&RDesativo&n");
    send_to_char(ch, buf);
    
    if (!QUALITY_ENABLED()) {
        send_to_char(ch, "\r\nO sistema estÃ¡ desabilitado.\r\n");
        return;
    }
    
    sprintf(buf, "\r\n&cTipos de Qualidade e Chances:&n\r\n");
    send_to_char(ch, buf);
    
    for (int i = 0; i < NUM_ITEM_QUALITIES-1; i++) {
        sprintf(buf, "%s%-12s&n - %d%% chance - %.1fx multiplicador\r\n",
            quality_table[i].color_code,
            quality_table[i].name,
            quality_chances[i],
            quality_table[i].stat_multiplier);
        send_to_char(ch, buf);
    }
    
    sprintf(buf, "%s%-12s&n - Somente Deuses - %.1fx multiplicador\r\n",
        quality_table[QUALITY_DIVINO].color_code,
        quality_table[QUALITY_DIVINO].name,
        quality_table[QUALITY_DIVINO].stat_multiplier);
    send_to_char(ch, buf);
}

/*
 * Quality admin command
 */
ACMD(do_quality_admin)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    struct obj_data *obj;
    int quality;
    
    if (GET_LEVEL(ch) < LVL_GRGOD) {
        send_to_char(ch, "Huh?!\r\n");
        return;
    }
    
    two_arguments(argument, arg1, arg2);
    if (!*arg1) {
        send_to_char(ch, "Uso: qadmin <opÃ§Ã£o> [objeto]\r\n");
        return;
    }
    
    if (!str_cmp(arg1, "info")) {
        show_quality_info(ch);
        return;
    }
    
    if (!str_cmp(arg1, "toggle")) {
        quality_system_enabled = !quality_system_enabled;
        sprintf(buf, "Sistema de qualidade %s.\r\n",
            quality_system_enabled ? "ativado" : "desativado");
        send_to_char(ch, buf);
        save_quality_config();
        return;
    }
    
    if (!str_cmp(arg1, "setdivine")) {
        if (!*arg2) {
            send_to_char(ch, "Especifique um objeto.\r\n");
            return;
        }
        
        if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
            send_to_char(ch, "VocÃª nÃ£o possui esse objeto.\r\n");
            return;
        }
        
        /* CORREÃ‡ÃƒO: Verificar se o item jÃ¡ tem qualidade antes de aplicar */
        if (HAS_QUALITY(obj)) {
            int current_quality = get_item_quality(obj);
            if (current_quality == QUALITY_DIVINO) {
                send_to_char(ch, "Este objeto jÃ¡ possui qualidade DIVINA.\r\n");
            } else {
                send_to_char(ch, "Este objeto jÃ¡ possui qualidade %s.\r\n",
                    get_quality_name(current_quality));
                send_to_char(ch, "Use 'qadmin removequality %s' primeiro se desejar alterar.\r\n", arg2);
            }
            return;
        }
        
        quality_hook_object_create(obj, TRUE);
        send_to_char(ch, "Qualidade DIVINA aplicada ao objeto.\r\n");
        return;
    }
    
    if (!str_cmp(arg1, "removequality")) {
        if (!*arg2) {
            send_to_char(ch, "Especifique um objeto.\r\n");
            return;
        }
        
        if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
            send_to_char(ch, "VocÃª nÃ£o possui esse objeto.\r\n");
            return;
        }
        
        if (!HAS_QUALITY(obj)) {
            send_to_char(ch, "Este objeto nÃ£o possui qualidade especial.\r\n");
            return;
        }
        
        int old_quality = get_item_quality(obj);
        REMOVE_BIT(GET_OBJ_EXTRA(obj)[0], (1 << ITEM_QUALITY));
        SET_OBJ_QUALITY(obj, 0);
        send_to_char(ch, "Qualidade %s removida do objeto.\r\n",
            get_quality_name(old_quality));
        return;
    }
    
    send_to_char(ch, "OpÃ§Ãµes: info, toggle, setdivine, removequality\r\n");
}

/*
 * Quality toggle command for players
 */
ACMD(do_quality_toggle)
{
    send_to_char(ch, "Este comando Ã© reservado para administradores.\r\n");
}