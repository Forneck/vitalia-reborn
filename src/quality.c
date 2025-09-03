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
#include <math.h>

/* Nível máximo para o cálculo do bônus. Nível 100 terá o bônus mínimo. */
#define QUALITY_LEVEL_CAP 100.0f
/* Fator MÁXIMO do bônus (aplicado a itens de nível 0). 0.5 = 50% do bônus base. */
#define MAX_BONUS_SCALE_FACTOR 0.5f 
/* Fator MÍNIMO do bônus (aplicado a itens de nível 100). 0.15 = 15% do bônus base. */
#define MIN_BONUS_SCALE_FACTOR 0.15f 


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
    { QUALITY_EPICO, 1.6, "ÉPICO", "\tM" },         /* 10% chance */
    { QUALITY_LENDARIO, 1.8, "LENDÁRIO", "\tY" },   /* 4% chance */
    { QUALITY_OBRA_PRIMA, 2.0, "OBRA PRIMA", "\tR" }, /* 1% chance */
    { QUALITY_DIVINO, 3.0, "DIVINO", "\tW" }        /* Admin only */
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
    int i = 0; /* Contador para as chances */
    int enabled_flag_read = 0; /* Flag para garantir que ENABLED foi lido */
    
    if (!(fl = fopen(QUALITY_CONFIG_FILE, "r"))) {
        log1("SYSERR: Could not open quality config file, using defaults.");
        return;
    }
    
    while (get_line(fl, line)) {
        /* Ignora comentários e linhas em branco */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
            continue;
            
        /* Tenta ler a flag ENABLED primeiro */
        if (!enabled_flag_read) {
            if (sscanf(line, "ENABLED: %d", &quality_system_enabled) == 1) {
                enabled_flag_read = 1; /* Marca que a flag foi lida com sucesso */
                continue; /* Vai para a próxima linha */
            } else {
                /* Se a primeira linha de dados não for ENABLED, o formato é inválido */
                log1("SYSERR: Invalid quality config format, using defaults. Expected 'ENABLED:' first.");
                fclose(fl);
                return;
            }
        }
        
        /* Se ENABLED já foi lido, tenta ler as chances */
        if (i < NUM_ITEM_QUALITIES - 1) {
            int temp_index, temp_chance;
            if (sscanf(line, "CHANCE_%d: %d", &temp_index, &temp_chance) == 2) {
                if (temp_index >= 0 && temp_index < NUM_ITEM_QUALITIES - 1) {
                    quality_chances[temp_index] = temp_chance;
                    i++; /* Incrementa o contador de chances lidas */
                }
            }
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
/*
 * Determine quality for a new item based on random chance
 * force_divine: 1 to force divine quality (admin command)
 */
int determine_item_quality(struct obj_data *obj, int force_divine)
{
    int roll, total = 0;
    
    if (force_divine)
        return QUALITY_DIVINO;
    
    /* 1. Primeiro, verifica se o tipo de item é elegível para receber qualidade. */
    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_LIGHT:
        case ITEM_WEAPON:
        case ITEM_ARMOR:
        case ITEM_WORN:
        case ITEM_WINGS:
        case ITEM_FIREWEAPON:
            /* Itens elegíveis continuam para a próxima verificação. */
            break;
        default:
            /* Qualquer outro tipo de item nunca terá qualidade. */
            return -1;
    }
    
    /* 2. Se o sistema estiver desabilitado, todos os itens ELEGÍVEIS se tornam Comuns. */
    if (!QUALITY_ENABLED()) {
        return QUALITY_COMUM;
    }
    
    /* 3. Se o sistema está habilitado, rola os dados para determinar a qualidade. */
    roll = rand_number(1, 100);
    
    /* A lógica de rolagem de dados permanece a mesma */
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
    float base_multiplier, final_multiplier;
    int i;
    
    if (quality < 0 || quality >= NUM_ITEM_QUALITIES)
        return;
        
    /* Verifica se o item já tem qualidade aplicada */
    if (HAS_QUALITY(obj)) {
        log1("SYSERR: Tentativa de aplicar qualidade a item que já possui qualidade");
        return;
    }
    
    /* Pega o multiplicador base da tabela de qualidade */
    base_multiplier = quality_table[quality].stat_multiplier;

    /* LÓGICA ESPECIAL PARA DIVINO */
    if (quality == QUALITY_DIVINO) {
        float obra_prima_bonus = quality_table[QUALITY_OBRA_PRIMA].stat_multiplier - 1.0f;
        base_multiplier = 1.0f + (obra_prima_bonus * 2.0f);
    }

    /* CALCULA O BÔNUS COM A NOVA CURVA DE PODER LINEAR */
    float bonus_only_multiplier = base_multiplier - 1.0f;
    
    if (bonus_only_multiplier <= 0) {
        final_multiplier = 1.0f;
    } else {
        float level_scaling_factor_raw;
        
        /* Calcula a posição do item na curva de nível (de 1.0 a 0.0) */
        if (GET_OBJ_LEVEL(obj) >= 0 && GET_OBJ_LEVEL(obj) < QUALITY_LEVEL_CAP) {
            level_scaling_factor_raw = 1.0f - (GET_OBJ_LEVEL(obj) / QUALITY_LEVEL_CAP);
        } else {
            level_scaling_factor_raw = 0.0f; /* Itens acima do cap recebem bônus mínimo */
        }
        
        /* Interpola o fator de bônus entre o MÁXIMO e o MÍNIMO */
        float final_bonus_scale = MIN_BONUS_SCALE_FACTOR + ((MAX_BONUS_SCALE_FACTOR - MIN_BONUS_SCALE_FACTOR) * level_scaling_factor_raw);
        
        /* Aplica a escala final ao bônus do item */
        float scaled_bonus = bonus_only_multiplier * final_bonus_scale;
        
        final_multiplier = 1.0f + scaled_bonus;
    }

    /* Aplica o multiplicador com base no tipo de item */
    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WEAPON:
        case ITEM_FIREWEAPON:
            {
                /* BALANCEAMENTO: O bônus de dano da arma é reduzido por um fator de 4. */
                float weapon_bonus = final_multiplier - 1.0f;
                float weapon_damage_multiplier = 1.0f + (weapon_bonus / 4.0f);

                GET_OBJ_VAL(obj, 1) = (int)roundf(GET_OBJ_VAL(obj, 1) * weapon_damage_multiplier);
                GET_OBJ_VAL(obj, 2) = (int)roundf(GET_OBJ_VAL(obj, 2) * weapon_damage_multiplier);
            }
            break;
        case ITEM_ARMOR:
            GET_OBJ_VAL(obj, 0) = (int)roundf(GET_OBJ_VAL(obj, 0) * final_multiplier);
            break;
    }

    /* Aplica a todos os affects com o multiplicador COMPLETO */
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location != APPLY_NONE && obj->affected[i].modifier != 0) {
            obj->affected[i].modifier = (int)roundf(obj->affected[i].modifier * final_multiplier);
        }
    }
    
    GET_OBJ_COST(obj) = (int)roundf(GET_OBJ_COST(obj) * base_multiplier);
    GET_OBJ_RENT(obj) = (int)roundf(GET_OBJ_RENT(obj) * base_multiplier);
    
    SET_OBJ_QUALITY(obj, quality);
    
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
    
    // Se o objeto não tem qualidade, retorna descrição normal
    if (!HAS_QUALITY(obj)) {
        return obj->short_description;
    }
    
    // Constrói o nome com qualidade
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
void quality_hook_object_load(struct obj_data *obj) {
  if (QUALITY_ENABLED()) {
    /* Só aplicar qualidade se o objeto não tem qualidade ainda */
    if (!HAS_QUALITY(obj)) {
      quality_modify_object(obj);
    }
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
    
    sprintf(buf, "\r\n\tc--- Sistema de Qualidade de Itens ---\tn\r\n");
    send_to_char(ch, buf);
    
    sprintf(buf, "Status: %s\r\n", QUALITY_ENABLED() ? "\tGAtivo\tn" : "\tRDesativo\tn");
    send_to_char(ch, buf);
    
    if (!QUALITY_ENABLED()) {
        send_to_char(ch, "\r\nO sistema está desabilitado.\r\n");
        return;
    }
    
    sprintf(buf, "\r\n\tcTipos de Qualidade e Chances:\tn\r\n");
    send_to_char(ch, buf);
    
    for (int i = 0; i < NUM_ITEM_QUALITIES-1; i++) {
        sprintf(buf, "%s%-12s\t - %.2f chance - %.1fx multiplicador\r\n",
            quality_table[i].color_code,
                quality_table[i].name,
                (float)quality_chances[i] / 100.0f,
                quality_table[i].stat_multiplier);
        send_to_char(ch, buf);
    }
    
    sprintf(buf, "%s%-12s\tn - Somente Deuses - %.1fx multiplicador\r\n",
        quality_table[QUALITY_DIVINO].color_code,
        quality_table[QUALITY_DIVINO].name,
        quality_table[QUALITY_DIVINO].stat_multiplier);
    send_to_char(ch, buf);

    send_to_char(ch, "\r\n\tc-------------------------------------\tn\r\n");
    send_to_char(ch, "\tcNotas para Administradores:\tn\r\n");
    send_to_char(ch, "\tC1. As chances podem ser alteradas no arquivo 'lib/etc/quality.conf'.\tn\r\n");
    send_to_char(ch, "\tC2. O MUD precisa ser reiniciado ('shutdown') para que as alterações no arquivo tenham efeito.\tn\r\n");
    send_to_char(ch, "\tC3. A soma das chances de Comum a Obra Prima deve ser sempre 100.\tn\r\n");


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
        send_to_char(ch, "Uso: qadmin <opção> [objeto]\r\n");
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
            send_to_char(ch, "Você não possui esse objeto.\r\n");
            return;
        }
        
        /* CORREÇÃO: Verificar se o item já tem qualidade antes de aplicar */
        if (HAS_QUALITY(obj)) {
            int current_quality = get_item_quality(obj);
            if (current_quality == QUALITY_DIVINO) {
                send_to_char(ch, "Este objeto já possui qualidade DIVINA.\r\n");
            } else {
                send_to_char(ch, "Este objeto já possui qualidade %s.\r\n",
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
            send_to_char(ch, "Você não possui esse objeto.\r\n");
            return;
        }
        
        if (!HAS_QUALITY(obj)) {
            send_to_char(ch, "Este objeto não possui qualidade especial.\r\n");
            return;
        }
        
        int old_quality = get_item_quality(obj);
        REMOVE_BIT(GET_OBJ_EXTRA(obj)[0], (1 << ITEM_QUALITY));
        SET_OBJ_QUALITY(obj, 0);
        send_to_char(ch, "Qualidade %s removida do objeto.\r\n",
            get_quality_name(old_quality));
        return;
    }
    
    send_to_char(ch, "Opções: info, toggle, setdivine, removequality\r\n");
}

/*
 * Quality toggle command for players
 */
ACMD(do_quality_info)
/*
 * Comando para jogadores verem as chances de qualidade
 */
{
    char buf[MAX_STRING_LENGTH];

    send_to_char(ch, "\r\n\tc--- Sistema de Qualidade de Itens ---\tn\r\n");

    if (!QUALITY_ENABLED()) {
        send_to_char(ch, "O sistema de qualidade de itens está atualmente \tRDESATIVADO\tn.\r\n");
        return;
    }

    send_to_char(ch, "O sistema de qualidade de itens está \tGATIVO\tn.\r\n");
    send_to_char(ch, "As chances de um item receber uma qualidade especial ao ser criado são (em porcentagens):\r\n");
    send_to_char(ch, "\tc-------------------------------------\tn\r\n");

    /* Loop para mostrar as chances de Comum a Obra Prima */
    for (int i = 0; i < NUM_ITEM_QUALITIES - 1; i++) {
        /* Nao mostra qualidades com 0% de chance para não poluir a tela do jogador */
        if (quality_chances[i] > 0) {
            sprintf(buf, " %s%-12s\tn: %d%%\r\n",
                quality_table[i].color_code,
                quality_table[i].name,
                quality_chances[i]);
            send_to_char(ch, buf);
        }
    }
    send_to_char(ch, "\tc-------------------------------------\tn\r\n");
    send_to_char(ch, "Itens de qualidade \tWDIVINA\tn não ocorrem naturalmente no mundo. Para conseguir um, ofereça... favores... ao seu GOD favorito\r\n");
}
