/* ----------------------------=== VitaliaMUD ===----------------------------
   * File: spirits.c Usage: functions for handling spirits of dead players
   Part of VitaliaMUD source code, Copyright (C) 2000 - Juliano Ravasi Ferraz
   Copyright (C) 2000 - Juliano Ravasi Ferraz -= All rights reserved =-

    Refatorado - 19/02/2025 (Delfino e Cansian)
   *
   -------------------------------------------------------------------------- */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "spirits.h"
#include "handler.h"
#include "utils.h"

#include "comm.h"

#include "db.h"

#include "screen.h"

struct char_data *load_offline_char_by_name2(const char *name)
{
    struct char_data *victim = NULL;

    CREATE(victim, struct char_data, 1);
    clear_char(victim);
    CREATE(victim->player_specials, struct player_special_data, 1);
    new_mobile_data(victim);

    if ((load_char(name, victim)) >= 0) {
        Crash_load(victim);

        return victim;
    } else {
        free_char(victim);
        return NULL;
    }
}

struct char_data *load_offline_char_by_name(const char *name)
{
    struct char_data *victim = NULL; /*Inicializa o ponteiro victim, que será o personagem a ser carregado.*/
    int player_i;
    CREATE(victim, struct char_data, 1);
    /*Macro CREATE aloca dinamicamente memória para uma estrutura do tipo char_data e atribui o ponteiro resultante a
     * victim.*/
    clear_char(victim); /*Limpa e inicializa todos os campos da estrutura char_data, evitando lixo de memória.*/

    CREATE(victim->player_specials, struct player_special_data, 1);
    /* Aloca memória para os dados especiais do jogador (player_special_data), que contém atributos específicos como
     * preferências, configurações, etc.*/
    new_mobile_data(victim);
    /*Inicializa os dados móveis (dinâmicos) do personagem, como atributos temporários, status de combate, entre
     * outros*/

    /*Função que carrega o jogador do arquivo de disco para a memória. Se retornar >= 0, o carregamento foi
     * bem-sucedido.*/
    player_i = load_char(name, victim);
    if (player_i >= 0) /* Vai carregar o player pelo nome passado no parametro*/
    {
        // Crash_load(victim); /* vai carregar os itens do player*/
        victim->next = character_list;
        GET_PFILEPOS(victim) = player_i;
        return victim;
    } else {
        free(victim);
        return NULL;
    }
    return NULL;
}

AutoRaiseResult autoraise_corpse(struct obj_data *corpse)
{
    struct obj_data *obj = NULL, *next_obj = NULL;
    struct char_data *ch;
    int idnum, offline = FALSE;
    const char *temp;
    /*Verificação se o objeto é nulo:
    Se corpse for NULL, registra um erro e retorna ar_dropobjs (ação que indica deixar os objetos do corpo no chão).
    Verificação do tipo do objeto:
    Se o tipo do objeto não for ITEM_CORPSE (ou seja, não é um corpo), é registrado um erro e também retorna
    ar_dropobjs. Verificação do ID: Obtém o ID do personagem associado ao corpo usando GET_OBJ_VAL(corpse, 0). Se esse
    ID for menor ou igual a zero (geralmente indica que se trata de um monstro e não de um personagem jogador), retorna
    ar_dropobjs.*/
    if (!corpse) {
        return (ar_dropobjs);
    } else if (GET_OBJ_TYPE(corpse) != ITEM_CORPSE) {
        return (ar_dropobjs);
    } else if ((idnum = GET_OBJ_VAL(corpse, 0)) <= 0) {
        /* Monster */
        return (ar_dropobjs);
    }

    /*Objetivo:
    Tenta recuperar o nome do personagem associado ao idnum usando get_name_by_id(idnum).
    Se não for encontrado (temp fica nulo), isso indica que o jogador pode ter recriado seu personagem (ou seja, a
    identidade mudou). Ação: Emite uma mensagem para o ambiente (usando act) para indicar que o corpo está sendo
    descartado (seja caindo das mãos ou se desfazendo em pó). Retorna ar_extract, sinalizando que o corpo deve ser
    extraído.*/
    if ((temp = get_name_by_id(idnum)) == NULL) {
        if (corpse->carried_by && corpse->worn_on == -1) {
            act("$p cai de suas mãos.", FALSE, corpse->carried_by, corpse, NULL, TO_CHAR);
        } else if (VALID_ROOM_RNUM(corpse->in_room)) {
            struct room_data *room = &world[corpse->in_room];
            if (room->people != NULL)
                act("$p se desfaz em pó.", TRUE, NULL, corpse, NULL, TO_ROOM);
        }
        return ar_extract;
    }

    /*Objetivo:
    Percorre a lista global de personagens (character_list) procurando um personagem jogador (PC) cujo ID seja igual ao
    idnum obtido do corpo. Resultado: Se encontrado, ch apontará para esse personagem; caso contrário, ch continuará
    nulo.*/
    for (ch = character_list; ch != NULL; ch = ch->next) {
        if (!IS_NPC(ch) && (GET_IDNUM(ch) == idnum)) {
            break;
        }
    }

    /*Objetivo:
    Se não encontrou o personagem online (ch é NULL), tenta carregá-lo a partir do arquivo de jogadores usando
    load_offline_char_by_name(name). Caso falhe: Se o personagem não for encontrado no arquivo, ar_dropobjs. Flag
    Offline: Se o personagem for carregado com sucesso, define offline como verdadeiro, indicando que ele não estava
    online.*/
    if (!ch) {
        if ((ch = load_offline_char_by_name(get_name_by_id(idnum))) == NULL) {
            return (ar_dropobjs);
        }
        offline = TRUE;
    }

    /*Objetivo:
    Verifica se o personagem tem a flag PLR_DELETED, o que indica que ele se deletou (auto-deletou seu personagem).

    Ação:
    Emite uma mensagem para os presentes informando que o corpo está se desfazendo.
    Se o personagem estava offline, ele é destruído (libera recursos).
    Retorna ar_extract, indicando que o corpo deve ser removido.*/
    if (PLR_FLAGGED(ch, PLR_DELETED)) {
        if (corpse->carried_by && corpse->worn_on == -1)
            act("$p cai de suas mãos.", FALSE, corpse->carried_by, corpse, NULL, TO_CHAR);
        else if (VALID_ROOM_RNUM(corpse->in_room)) {
            struct room_data *room = &world[corpse->in_room];
            if (room->people != NULL)
                act("$p se desfaz em pó.", TRUE, NULL, corpse, NULL, TO_ROOM);
        }

        if (offline)
            free_char(ch);

        return ar_extract;
    }

    /*Objetivo:
    Se o personagem não está morto (ou seja, já foi ressuscitado ou nunca morreu), então não há razão para realizar a
    ressurreição.

    Ação:
    Se o personagem estava offline, destrói o personagem carregado.
    Retorna ar_dropobjs para indicar que os objetos do corpo serão deixados no chão.*/
    if (!IS_DEAD(ch)) {
        /* se o player nao esta morto, este obj recem criado, nao é necessário então por isso, ele é eliminado*/

        if (offline)
            free_char(ch);
        return (ar_dropobjs);
    }
    /* por enquanto este codigo vai ficar de fora.
     *if (GET_CON(ch) < 8) {

        if (offline)
            free_char(ch);
        return (ar_skip);
    }*/

    /*Objetivo:
    Se todas as condições anteriores forem atendidas (o personagem está morto, não se deletou, tem CON suficiente,
    etc.), então prossegue para a ressurreição.

    Caso Offline:
    Chama a função raise_offline(ch, corpse) para ressuscitar o personagem que estava offline.
    Em seguida, destrói o personagem carregado da playerfile (limpeza de memória, pois ele não será mais necessário).

    Caso Online:
    Chama a função raise_online(ch, NULL, corpse, corpse->in_room, FALSE) para ressuscitar o personagem que está online.
    Retorno Final:cd
    Após a ressurreição, o metodo retorna ar_extract, indicando que o corpo deve ser extraído (removido), já que a
    ressurreição ocorreu.*/
    if (offline) {
        raise_offline(ch, corpse);
        free_char(ch);
    } else if (!ch->desc) {
        /* Linkless player: in game as a ghost but has no active connection.
         * Raise them online so items are transferred to the in-memory character
         * (preserved for when they reconnect), then also save their crash file
         * so items survive a server crash before they reconnect. */
        mudlog(NRM, LVL_GOD, TRUE, "Auto-raising linkless ghost %s; saving crash file.", GET_NAME(ch));
        raise_online(ch, NULL, corpse, corpse->in_room, 2);
        Crash_crashsave(ch);
    } else {
        raise_online(ch, NULL, corpse, corpse->in_room, 2);
    }
    return (ar_extract);
}

void raise_online(struct char_data *ch, struct char_data *raiser, struct obj_data *corpse, room_rnum targ_room,
                  int restore)
{
    struct obj_data *obj = NULL, *next_obj = NULL;

    if (targ_room && targ_room != IN_ROOM(ch)) {
        act("O espírito de $n começa a brilhar e é violentamente puxado para longe.", TRUE, ch, 0, 0, TO_ROOM);
        char_from_room(ch);
        char_to_room(ch, targ_room);
    } else {
        act("O espírito de $n começa a brilhar.", TRUE, ch, 0, 0, TO_ROOM);
    }

    act("@GUma estranha sensação percorre seu espírito, que é puxado por uma força\r\n"
        "divina, carregando-$r por uma longa distância.@n",
        FALSE, ch, 0, 0, TO_CHAR);

    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_GHOST);
    if (AFF_FLAGGED(ch, AFF_FLYING))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    if (corpse) {
        act("Você perde a noção de espaço e antes que você possa gritar por socorro,\r\n"
            "você percebe que está sendo puxad$r para dentro de seu corpo!@n",
            FALSE, ch, 0, 0, TO_CHAR);
    } else {
        send_to_char(ch,
                     "Estranhamente, seu espírito começa a voltar ao formato de um corpo humano.\r\n"
                     "Você começa a sentir novamente seus braços, suas pernas, o chão, o ar\r\n"
                     "entrando pelos seus pulmões!%s\r\n",
                     CCNRM(ch, C_NRM));
    }
    send_to_char(ch, "\r\n");

    if (!restore && (GET_CON(ch) > 3) && (rand_number(1, 101) > (75 + GET_CON(ch)))) {
        // System Shock roll
        GET_CON(ch) -= 1;
        act("$n grita de dor enquanto é ressucitad$r.", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "ARRRGH!! Você sente uma dor muito forte!\r\n\r\n");
    }

    if (targ_room)
        look_at_room(ch, TRUE);

    if (!raiser) {
        act("$n foi trazid$r devolta à vida pelos Deuses!", TRUE, ch, 0, 0, TO_ROOM);
        act("\tWVocê foi trazid$r devolta à vida pelos Deuses!\tn", FALSE, ch, 0, 0, TO_CHAR);
    } else if (GET_LEVEL(raiser) >= LVL_IMMORT) {
        act("Sua força divina trouxe $N devolta à vida.", FALSE, raiser, 0, ch, TO_CHAR);
        act("$n foi trazid$r devolta à vida pela força divina de $N!", FALSE, ch, 0, raiser, TO_NOTVICT);
        act("\tWVocê foi trazid$r devolta à vida pela força divina de $N!\tn", FALSE, ch, 0, raiser, TO_CHAR);
    } else {
        act("\tWVocê sente a força de seu Deus trazendo $N devolta à vida!\tn", FALSE, raiser, 0, ch, TO_CHAR);
        act("$n foi trazid$r devolta à vida pelo Deus de $N!", FALSE, ch, 0, raiser, TO_NOTVICT);
        act("\tWVocê foi trazid$r devolta à vida pela força divina dos Deuses de $N!\tn", FALSE, ch, 0, raiser,
            TO_CHAR);
    }

    if (restore >= 2) {
        GET_HIT(ch) = GET_MAX_HIT(ch);
        GET_MANA(ch) = GET_MAX_MANA(ch);
        GET_MOVE(ch) = GET_MAX_MOVE(ch);
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            GET_COND(ch, HUNGER) = 24;
            GET_COND(ch, THIRST) = 24;
        }
    } else if (restore == 1) {
        GET_HIT(ch) = MAX(GET_MAX_HIT(ch) / 4, 1);
        GET_MANA(ch) = MAX(GET_MAX_MANA(ch) / 4, 0);
        GET_MOVE(ch) = MAX(GET_MAX_MOVE(ch) / 4, 0);
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            GET_COND(ch, HUNGER) = 12;
            GET_COND(ch, THIRST) = 12;
        }
    } else {
        GET_HIT(ch) = 1;
        GET_MANA(ch) = 0;
        GET_MOVE(ch) = 0;
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            GET_COND(ch, HUNGER) = 2;
            GET_COND(ch, THIRST) = 2;
        }
    }
    if (corpse) {
        for (obj = corpse->contains; obj != NULL; obj = next_obj) {
            next_obj = obj->next_content;
            obj_from_obj(obj);
            obj_to_char(obj, ch);
            get_check_money(ch, obj);
        }
        if (corpse->in_room)
            obj_from_room(corpse);
        else if (corpse->carried_by)
            obj_from_char(corpse);
        else if (corpse->in_obj)
            obj_from_obj(corpse);
    } else {
        act("Não tendo corpo para voltar a vida, você volta \tRpelad$r\tn!", FALSE, ch, 0, ch, TO_CHAR);
        act("Não tendo corpo para voltar a vida, $N volta \tRpelad$R\tn!", FALSE, raiser, 0, ch, TO_CHAR);
    }
    save_char(ch);
}

void raise_offline(struct char_data *ch, struct obj_data *corpse)
{
    struct obj_data *obj = NULL, *next_obj = NULL;

    room_vnum room_vnum_corpse = world[corpse->in_room].number;

    /* Raise player */
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_GHOST);
    if (!PLR_FLAGGED(ch, PLR_LOADROOM)) {
        GET_LOADROOM(ch) = room_vnum_corpse;
    }

    /* System shock roll */
    if ((GET_CON(ch) > 3) && (rand_number(1, 101) > (75 + GET_CON(ch))))
        GET_CON(ch) -= 1;

    /* Restore points */
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        GET_COND(ch, HUNGER) = 24;
        GET_COND(ch, THIRST) = 24;
    }

    /* If corpse has objects, save them to file */
    if (corpse->contains)
        raise_save_objs(ch, corpse);

    save_char(ch);
}

void raise_save_objs(struct char_data *ch, struct obj_data *corpse)
{
    struct obj_data *obj, *next_obj;
    FILE *f;
    struct rent_info rent; /* Estrutura para armazenar informações do "rent" */
    char fname[MAX_STRING_LENGTH];

    /* Procura por dinheiro no corpo e adiciona ao personagem */
    for (obj = corpse->contains; obj; obj = next_obj) {
        next_obj = obj->next_content; /* Salva próximo objeto antes de remover o atual */
        if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
            GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
            extract_obj(obj);
        }
    }

    /* Se não restar nenhum item no corpo, não há nada a salvar */
    if (!corpse->contains)
        return;

    /* Monta o nome do arquivo de crash/rent do jogador */
    if (!get_filename(fname, sizeof(fname), CRASH_FILE, GET_NAME(ch)) || !(f = fopen(fname, "wb"))) {
        return;
    }

    /* Prepara as informações de aluguel */

    rent.rentcode = RENT_RENTED;
    rent.time = time(0);

    /* Escreve as informações de aluguel no arquivo */
    if (!Crash_write_rentcode(ch, f, &rent)) {
        fclose(f);
        return;
    }

    /* Remove itens que não podem ser salvos (não rentáveis) do corpo */
    Crash_extract_norents(corpse->contains);
    /* Salva os itens restantes do corpo */
    while ((obj = corpse->contains) != NULL) {
        obj_from_obj(obj); /* Remove o objeto da lista do corpo */
        if (!Crash_save(obj, f, 0)) {

            fclose(f);
            return;
        }
        Crash_extract_objs(obj); /* Trata a remoção definitiva do objeto, se necessário */
    }

    fclose(f);
}
