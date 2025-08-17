/**
 * @file auction.c
 * Implementation of the auction system for Vitália Reborn.
 *
 * This file contains the core auction system functionality including
 * auction creation, bidding, pass management, and Belchior's special functions.
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
#include "shop.h"

/* External function declarations */
extern SPECIAL(shop_keeper);
extern SPECIAL(cryogenicist);
extern shop_rnum find_shop_by_keeper(mob_rnum rnum);

/* Global auction variables */
struct auction_data *auction_list = NULL;
struct auction_pass *auction_pass_list = NULL;
int next_auction_id = 1;

/**
 * Create a new auction
 */
struct auction_data *create_auction(struct char_data *seller, struct obj_data *item, 
                                   int type, int access_mode, long starting_price, 
                                   long reserve_price, int duration)
{
    struct auction_data *auction;
    
    if (!seller || !item) {
        return NULL;
    }
    
    /* Validate auction parameters */
    if (duration < MIN_AUCTION_TIME || duration > MAX_AUCTION_TIME) {
        duration = 1800; /* Default 30 minutes */
    }
    
    if (starting_price < 1) {
        starting_price = 1;
    }
    
    CREATE(auction, struct auction_data, 1);
    
    auction->auction_id = next_auction_id++;
    strlcpy(auction->seller_name, GET_NAME(seller), sizeof(auction->seller_name));
    strlcpy(auction->item_name, item->short_description, sizeof(auction->item_name));
    auction->item_vnum = GET_OBJ_VNUM(item);
    auction->quantity = 1; /* TODO: Handle bulk auctions */
    
    auction->auction_type = type;
    auction->access_mode = access_mode;
    auction->state = AUCTION_INACTIVE;
    
    auction->starting_price = starting_price;
    auction->current_price = starting_price;
    auction->reserve_price = reserve_price;
    auction->buyout_price = 0; /* TODO: Implement buyout */
    
    auction->start_time = time(0);
    auction->end_time = auction->start_time + duration;
    auction->duration = duration;
    
    auction->bids = NULL;
    auction->winning_bid = NULL;
    auction->invited_players = NULL;
    
    /* Add to global auction list */
    auction->next = auction_list;
    auction_list = auction;
    
    log1("AUCTION: Created auction #%d for %s by %s", 
        auction->auction_id, auction->item_name, auction->seller_name);
    
    return auction;
}

/**
 * Check if a character has an auction pass
 */
int has_auction_pass(struct char_data *ch, int auction_id)
{
    struct auction_pass *pass;
    
    if (!ch) {
        return 0;
    }
    
    for (pass = auction_pass_list; pass; pass = pass->next) {
        if (str_cmp(pass->holder_name, GET_NAME(ch)) == 0) {
            /* Check if pass is valid for this auction */
            if (pass->auction_id == 0 || pass->auction_id == auction_id) {
                /* Check if pass hasn't expired */
                if (pass->expires == 0 || pass->expires > time(0)) {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

/**
 * Give an auction pass to a character
 */
void give_auction_pass(struct char_data *ch, int auction_id, int duration)
{
    struct auction_pass *pass;
    
    if (!ch) {
        return;
    }
    
    CREATE(pass, struct auction_pass, 1);
    strlcpy(pass->holder_name, GET_NAME(ch), sizeof(pass->holder_name));
    pass->auction_id = auction_id;
    pass->expires = (duration > 0) ? time(0) + duration : 0;
    
    pass->next = auction_pass_list;
    auction_pass_list = pass;
    
    send_to_char(ch, "Belchior entrega a você um passe de leilão especial.\r\n");
    act("Belchior entrega um passe de leilão para $N.", FALSE, NULL, 0, ch, TO_ROOM);
}

/**
 * Find an auction by ID
 */
struct auction_data *find_auction(int auction_id)
{
    struct auction_data *auction;
    
    for (auction = auction_list; auction; auction = auction->next) {
        if (auction->auction_id == auction_id) {
            return auction;
        }
    }
    
    return NULL;
}

/**
 * Place a bid on an auction
 */
int place_bid(struct char_data *bidder, int auction_id, long amount)
{
    struct auction_data *auction;
    struct auction_bid *bid;
    
    if (!bidder) {
        return 0;
    }
    
    auction = find_auction(auction_id);
    if (!auction || auction->state != AUCTION_ACTIVE) {
        return 0;
    }
    
    /* Check if bid is higher than current price */
    if (amount <= auction->current_price) {
        return 0;
    }
    
    /* Create new bid */
    CREATE(bid, struct auction_bid, 1);
    strlcpy(bid->bidder_name, GET_NAME(bidder), sizeof(bid->bidder_name));
    bid->amount = amount;
    bid->timestamp = time(0);
    
    /* Add to bid list */
    bid->next = auction->bids;
    auction->bids = bid;
    
    /* Update auction */
    auction->current_price = amount;
    auction->winning_bid = bid;
    
    /* TODO: Reserve money from bidder */
    
    return 1;
}

/**
 * Show detailed auction information
 */
void show_auction_details(struct char_data *ch, int auction_id)
{
    struct auction_data *auction;
    struct auction_bid *bid;
    time_t remaining;
    int bid_count = 0;
    
    auction = find_auction(auction_id);
    if (!auction) {
        send_to_char(ch, "Leilão #%d não encontrado.\r\n", auction_id);
        return;
    }
    
    send_to_char(ch, "Detalhes do Leilão #%d:\r\n", auction->auction_id);
    send_to_char(ch, "========================\r\n");
    send_to_char(ch, "Item: %s\r\n", auction->item_name);
    send_to_char(ch, "Vendedor: %s\r\n", auction->seller_name);
    send_to_char(ch, "Tipo: %s\r\n", 
                (auction->auction_type == AUCTION_TYPE_ENGLISH) ? "Leilão Inglês" : "Leilão Holandês");
    send_to_char(ch, "Acesso: %s\r\n",
                (auction->access_mode == AUCTION_OPEN) ? "Aberto" : "Fechado");
    send_to_char(ch, "Preço Inicial: %ld moedas\r\n", auction->starting_price);
    send_to_char(ch, "Preço Atual: %ld moedas\r\n", auction->current_price);
    
    if (auction->reserve_price > auction->starting_price) {
        send_to_char(ch, "Preço Reserva: %ld moedas\r\n", auction->reserve_price);
    }
    
    /* Show time remaining */
    if (auction->state == AUCTION_ACTIVE) {
        remaining = auction->end_time - time(0);
        if (remaining > 0) {
            send_to_char(ch, "Tempo Restante: %ldh %ldm %lds\r\n",
                        remaining / 3600, (remaining % 3600) / 60, remaining % 60);
        } else {
            send_to_char(ch, "Este leilão já expirou.\r\n");
        }
    } else {
        send_to_char(ch, "Estado: %s\r\n",
                    (auction->state == AUCTION_INACTIVE) ? "Inativo" : "Finalizado");
    }
    
    /* Count bids */
    for (bid = auction->bids; bid; bid = bid->next) {
        bid_count++;
    }
    
    send_to_char(ch, "Total de Lances: %d\r\n", bid_count);
    
    if (auction->winning_bid) {
        send_to_char(ch, "Lance Vencedor: %ld moedas por %s\r\n",
                    auction->winning_bid->amount, auction->winning_bid->bidder_name);
    }
}
void show_auction_list(struct char_data *ch)
{
    struct auction_data *auction;
    int count = 0;
    
    send_to_char(ch, "Lista de Leilões Ativos:\r\n");
    send_to_char(ch, "========================\r\n");
    
    for (auction = auction_list; auction; auction = auction->next) {
        if (auction->state == AUCTION_ACTIVE) {
            time_t remaining = auction->end_time - time(0);
            if (remaining > 0) {
                count++;
                send_to_char(ch, "ID: %d | %s | Preço: %ld | Tempo: %ldm%lds | %s\r\n",
                           auction->auction_id,
                           auction->item_name,
                           auction->current_price,
                           remaining / 60,
                           remaining % 60,
                           (auction->access_mode == AUCTION_CLOSED) ? "FECHADO" : "ABERTO");
            }
        }
    }
    
    if (count == 0) {
        send_to_char(ch, "Não há leilões ativos no momento.\r\n");
    }
}

/**
 * Update auction states and end expired auctions
 */
void update_auctions(void)
{
    struct auction_data *auction, *next_auction;
    time_t now = time(0);
    
    for (auction = auction_list; auction; auction = next_auction) {
        next_auction = auction->next;
        
        if (auction->state == AUCTION_ACTIVE && auction->end_time <= now) {
            end_auction(auction);
        }
    }
}

/**
 * End an auction and process the results
 */
void end_auction(struct auction_data *auction)
{
    if (!auction) {
        return;
    }
    
    auction->state = AUCTION_FINISHED;
    
    /* TODO: Implement auction completion logic */
    /* - Transfer item to winner */
    /* - Transfer money to seller */
    /* - Send notifications */
    
    log1("AUCTION: Ended auction #%d - %s", auction->auction_id, auction->item_name);
}

/**
 * Belchior's combined special function - auction house management, shop keeper, and cryogenicist
 */
SPECIAL(belchior_auctioneer)
{
    struct char_data *belchior = (struct char_data *)me;
    room_rnum shop_room, auction_room;
    int shop_nr;
    
    /* First check if this is a shop-related command */
    if (CMD_IS("buy") || CMD_IS("sell") || CMD_IS("list") || CMD_IS("value")) {
        /* Find the shop number for Belchior */
        if ((shop_nr = find_shop_by_keeper(GET_MOB_RNUM(belchior))) >= 0) {
            return shop_keeper(ch, me, cmd, argument);
        }
    }
    
    /* Check for cryogenicist commands */
    if (CMD_IS("offer") || CMD_IS("rent")) {
        return cryogenicist(ch, me, cmd, argument);
    }
    
    /* Check auction house access commands */
    if (!CMD_IS("down") && !CMD_IS("descer") && !CMD_IS("baixo")) {
        return FALSE;
    }
    
    /* Check if we're in Belchior's shop */
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != 3081) {
        return FALSE;
    }
    
    /* Only allow access if new auction system is enabled */
    if (!CONFIG_NEW_AUCTION_SYSTEM) {
        act("Belchior balança a cabeça: 'Desculpe, o sistema de leilões ainda não está funcionando.'",
            FALSE, ch, 0, belchior, TO_CHAR);
        return TRUE;
    }
    
    shop_room = real_room(3081);
    auction_room = real_room(3150);
    
    if (shop_room == NOWHERE || auction_room == NOWHERE) {
        send_to_char(ch, "Há um problema com as salas do leilão.\r\n");
        return TRUE;
    }
    
    /* Check for auction pass if there are any closed auctions */
    struct auction_data *auction;
    int has_closed_auctions = 0;
    
    for (auction = auction_list; auction; auction = auction->next) {
        if (auction->state == AUCTION_ACTIVE && auction->access_mode == AUCTION_CLOSED) {
            has_closed_auctions = 1;
            break;
        }
    }
    
    if (has_closed_auctions && !has_auction_pass(ch, 0)) {
        act("Belchior bloqueia seu caminho: 'Desculpe, há leilões fechados acontecendo. Você precisa de um passe especial.'",
            FALSE, ch, 0, belchior, TO_CHAR);
        act("Belchior impede $n de descer.", FALSE, ch, 0, 0, TO_ROOM);
        return TRUE;
    }
    
    /* Unlock and open the door temporarily */
    if (EXIT(ch, DOWN)) {
        REMOVE_BIT(EXIT(ch, DOWN)->exit_info, EX_LOCKED);
        REMOVE_BIT(EXIT(ch, DOWN)->exit_info, EX_HIDDEN);
        
        act("Belchior sussurra algo e uma porta secreta se abre no chão.",
            FALSE, ch, 0, belchior, TO_ROOM);
        
        send_to_char(ch, "Belchior permite que você desça para a casa de leilões.\r\n");
        
        /* Move the character down */
        char_from_room(ch);
        char_to_room(ch, auction_room);
        
        look_at_room(ch, 0);
        
        /* Lock and hide the door again after a brief delay */
        if (world[shop_room].dir_option[DOWN]) {
            SET_BIT(world[shop_room].dir_option[DOWN]->exit_info, EX_LOCKED);
            SET_BIT(world[shop_room].dir_option[DOWN]->exit_info, EX_HIDDEN);
        }
        
        return TRUE;
    }
    
    return FALSE;
}

/**
 * Save auctions to disk (placeholder)
 */
void save_auctions(void)
{
    /* TODO: Implement auction persistence */
}

/**
 * Load auctions from disk (placeholder) 
 */
void load_auctions(void)
{
    /* TODO: Implement auction loading */
}