/* ----------------------------=== VitaliaMUD ===---------------------------- *
 *   File: spirits.h
 *  Usage: Header file for functions handling spirits of dead players
 *
 * Part of VitaliaMUD source code, Copyright (C) 2000 - Juliano Ravasi Ferraz
 * Copyright (C) 2000 - Juliano Ravasi Ferraz -= All rights reserved =-
 * -------------------------------------------------------------------------- */

typedef enum
{
    ar_skip = 0,       // Não fazer nada com o corpo
    ar_dropobjs = 1,   // Deixar o corpo e seus objetos caírem no chão
    ar_extract = 2,    // Extrair o corpo e seus itens
    ar_ok = 3          // Ressurreição bem-sucedida
} AutoRaiseResult;

void raise_save_objs(struct char_data *ch, struct obj_data *corpse);
void raise_online(struct char_data *ch, struct char_data *raiser, struct obj_data *corpse, room_rnum targ_room,
                  int restore);
void raise_offline(struct char_data *ch, struct obj_data *corpse);
AutoRaiseResult autoraise_corpse(struct obj_data *corpse);

extern void Crash_extract_norents(struct obj_data *obj);
extern void Crash_extract_objs(struct obj_data *obj);
extern void get_check_money(struct char_data *ch, struct obj_data *obj);

// int autoraise(struct obj_data *corpse);

#define AR_SKIP 0
#define AR_DROPOBJS 1
#define AR_EXTRACT 2