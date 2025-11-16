/**
 * @file act.auction.c
 * Player commands for the auction system.
 *
 * This file contains all player-accessible auction commands like 'auction',
 * 'bid', 'auctions', etc.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "auction.h"

/**
 * Main auction command - allows players to start auctions, bid, and view auctions
 * Usage: auction <subcommand> [arguments]
 */
ACMD(do_auction)
{
    char subcommand[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct auction_data *auction;
    struct obj_data *obj;
    int auction_id, bid_amount;

    skip_spaces(&argument);
    half_chop(argument, subcommand, argument);
    two_arguments(argument, arg1, arg2);

    if (!*subcommand) {
        send_to_char(ch, "Comandos de leilão disponíveis:\r\n");
        send_to_char(ch, "  leilao listar            - Ver leilões ativos\r\n");
        send_to_char(ch, "  leilao criar <item> <preço> - Criar novo leilão\r\n");
        send_to_char(ch, "  leilao dar <id> <valor>  - Dar lance em leilão\r\n");
        send_to_char(ch, "  leilao info <id>         - Ver detalhes do leilão\r\n");
        send_to_char(ch, "  leilao convidar <id> <jogador> - Convidar jogador para leilão fechado\r\n");
        send_to_char(ch, "  leilao desconvidar <id> <jogador> - Remover convite de jogador\r\n");
        send_to_char(ch, "  leilao convidados <id>   - Ver lista de convidados (vendedor)\r\n");
        send_to_char(ch, "  leilao passe             - Solicitar passe para leilões fechados\r\n");
        return;
    }

    /* Check if auction system is enabled or if player is immortal */
    if (!CONFIG_NEW_AUCTION_SYSTEM && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "O sistema de leilões ainda não está ativo.\r\n");
        return;
    }

    if (is_abbrev(subcommand, "listar")) {
        show_auction_list(ch);
        return;
    }

    if (is_abbrev(subcommand, "criar")) {
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Uso: leilao criar <item> <preço_inicial>\r\n");
            return;
        }

        /* Check if we're in the auction house */
        if (GET_ROOM_VNUM(IN_ROOM(ch)) != 3092) {
            send_to_char(ch, "Você precisa estar na casa de leilões para criar um leilão.\r\n");
            return;
        }

        /* Find the item */
        if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
            send_to_char(ch, "Você não possui esse item.\r\n");
            return;
        }

        bid_amount = atoi(arg2);
        if (bid_amount < 1) {
            send_to_char(ch, "O preço inicial deve ser maior que zero.\r\n");
            return;
        }

        /* Create the auction */
        auction =
            create_auction(ch, obj, AUCTION_TYPE_ENGLISH, AUCTION_OPEN, bid_amount, bid_amount, 1800); /* 30 minutes */

        if (auction) {
            auction->state = AUCTION_ACTIVE;
            send_to_char(ch, "Leilão #%d criado para %s com preço inicial de %d moedas.\r\n", auction->auction_id,
                         obj->short_description, bid_amount);

            /* Remove item from player's inventory - it's now in the auction system */
            obj_from_char(obj);
            extract_obj(obj); /* Item will be recreated when auction ends */

            log1("AUCTION: %s created auction #%d for item vnum %d", GET_NAME(ch), auction->auction_id,
                 auction->item_vnum);
        } else {
            send_to_char(ch, "Erro ao criar o leilão.\r\n");
        }
        return;
    }

    if (is_abbrev(subcommand, "dar")) {
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Uso: leilao dar <id_leilao> <valor_lance>\r\n");
            return;
        }

        auction_id = atoi(arg1);
        bid_amount = atoi(arg2);

        if (auction_id < 1 || bid_amount < 1) {
            send_to_char(ch, "ID do leilão e valor do lance devem ser maior que zero.\r\n");
            return;
        }

        /* Check if we're in the auction house */
        if (GET_ROOM_VNUM(IN_ROOM(ch)) != 3092) {
            send_to_char(ch, "Você precisa estar na casa de leilões para dar lances.\r\n");
            return;
        }

        auction = find_auction(auction_id);
        if (!auction || auction->state != AUCTION_ACTIVE) {
            send_to_char(ch, "Leilão não encontrado ou não está ativo.\r\n");
            return;
        }

        /* Check if this is a closed auction and player has pass */
        if (auction->access_mode == AUCTION_CLOSED && !has_auction_pass(ch, auction_id)) {
            send_to_char(ch, "Este é um leilão fechado. Você precisa de um passe especial.\r\n");
            return;
        }

        if (bid_amount <= auction->current_price) {
            send_to_char(ch, "Seu lance deve ser maior que o lance atual de %ld moedas.\r\n", auction->current_price);
            return;
        }

        /* Check if player has enough money */
        if (GET_GOLD(ch) < bid_amount) {
            send_to_char(ch, "Você não tem moedas suficientes para esse lance.\r\n");
            return;
        }

        /* Place the bid */
        if (place_bid(ch, auction_id, bid_amount)) {
            send_to_char(ch, "Lance de %d moedas aceito no leilão #%d!\r\n", bid_amount, auction_id);

            /* Announce to auction house */
            char buf[MAX_STRING_LENGTH];
            snprintf(buf, sizeof(buf), "%s deu um lance de %d moedas no leilão #%d (%s).", GET_NAME(ch), bid_amount,
                     auction_id, auction->item_name);
            send_to_room(IN_ROOM(ch), "%s", buf);
        } else {
            send_to_char(ch, "Erro ao processar seu lance.\r\n");
        }
        return;
    }

    if (is_abbrev(subcommand, "info")) {
        if (!*arg1) {
            send_to_char(ch, "Uso: leilao info <id_leilao>\r\n");
            return;
        }

        auction_id = atoi(arg1);
        show_auction_details(ch, auction_id);
        return;
    }

    if (is_abbrev(subcommand, "convidar")) {
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Uso: leilao convidar <id_leilao> <nome_jogador>\r\n");
            return;
        }

        auction_id = atoi(arg1);
        if (auction_id < 1) {
            send_to_char(ch, "ID do leilão deve ser maior que zero.\r\n");
            return;
        }

        if (invite_player_to_auction(ch, auction_id, arg2)) {
            send_to_char(ch, "Jogador %s foi convidado para o leilão #%d.\r\n", arg2, auction_id);
        } else {
            auction = find_auction(auction_id);
            if (!auction) {
                send_to_char(ch, "Leilão #%d não encontrado.\r\n", auction_id);
            } else if (str_cmp(auction->seller_name, GET_NAME(ch)) != 0) {
                send_to_char(ch, "Apenas o vendedor pode convidar jogadores.\r\n");
            } else if (auction->access_mode != AUCTION_CLOSED) {
                send_to_char(ch, "Este não é um leilão fechado.\r\n");
            } else {
                send_to_char(ch, "Erro ao convidar jogador. Talvez já esteja convidado.\r\n");
            }
        }
        return;
    }

    if (is_abbrev(subcommand, "desconvidar")) {
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Uso: leilao desconvidar <id_leilao> <nome_jogador>\r\n");
            return;
        }

        auction_id = atoi(arg1);
        if (auction_id < 1) {
            send_to_char(ch, "ID do leilão deve ser maior que zero.\r\n");
            return;
        }

        if (uninvite_player_from_auction(ch, auction_id, arg2)) {
            send_to_char(ch, "Convite de %s para o leilão #%d foi removido.\r\n", arg2, auction_id);
        } else {
            auction = find_auction(auction_id);
            if (!auction) {
                send_to_char(ch, "Leilão #%d não encontrado.\r\n", auction_id);
            } else if (str_cmp(auction->seller_name, GET_NAME(ch)) != 0) {
                send_to_char(ch, "Apenas o vendedor pode remover convites.\r\n");
            } else {
                send_to_char(ch, "Jogador não estava convidado ou erro ao remover convite.\r\n");
            }
        }
        return;
    }

    if (is_abbrev(subcommand, "convidados")) {
        if (!*arg1) {
            send_to_char(ch, "Uso: leilao convidados <id_leilao>\r\n");
            return;
        }

        auction_id = atoi(arg1);
        show_auction_invitations(ch, auction_id);
        return;
    }

    if (is_abbrev(subcommand, "passe")) {
        /* Direct players to talk to Belchior */
        send_to_char(ch, "Para obter um passe de leilão, vá até Belchior e use: passe <id_leilao>\r\n");
        return;
    }

    send_to_char(ch, "Subcomando de leilão desconhecido. Digite 'leilao' para ver as opções.\r\n");
}

/**
 * Simple command to list auctions (alias for 'auction list')
 */
ACMD(do_auctions)
{
    /* Check if auction system is enabled or if player is immortal */
    if (!CONFIG_NEW_AUCTION_SYSTEM && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "O sistema de leilões ainda não está ativo.\r\n");
        return;
    }

    show_auction_list(ch);
}