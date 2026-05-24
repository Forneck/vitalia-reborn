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
extern struct char_data *character_list;

/* Global auction variables */
struct auction_data *auction_list = NULL;
struct auction_pass *auction_pass_list = NULL;
struct auction_invitation *auction_invitation_list = NULL;
int next_auction_id = 1;
struct auction_stats global_auction_stats = {0, 0, 0, 0, 0, 0};

/**
 * Find an online player by name (for auction system)
 * This searches the character list directly without visibility checks
 */
static struct char_data *find_player_online(const char *name)
{
    struct char_data *i;

    if (!name || !*name) {
        return NULL;
    }

    for (i = character_list; i; i = i->next) {
        if (!IS_NPC(i) && !str_cmp(GET_NAME(i), name)) {
            return i;
        }
    }

    return NULL;
}

/**
 * Give gold to an offline player
 * Loads their character, adds gold, saves, and frees memory
 * Returns 1 on success, 0 on failure
 */
static int give_gold_to_offline_player(const char *name, long amount)
{
    struct char_data *temp_char;
    int success = 0;

    if (!name || !*name || amount <= 0) {
        return 0;
    }

    /* Check if player is online first */
    if (find_player_online(name)) {
        log1("ERROR: Attempted to give gold to online player %s via offline method", name);
        return 0;
    }

    /* Create temporary character structure */
    CREATE(temp_char, struct char_data, 1);
    clear_char(temp_char);
    CREATE(temp_char->player_specials, struct player_special_data, 1);
    new_mobile_data(temp_char);

    /* Try to load the player */
    if (load_char(name, temp_char) >= 0) {
        /* Add gold */
        increase_gold(temp_char, amount);

        /* Save the character */
        save_char(temp_char);

        log1("AUCTION: Credited %ld gold to offline player %s", amount, name);
        success = 1;
    } else {
        log1("ERROR: Failed to load character %s for gold credit", name);
    }

    /* free_char handles player_specials and the char struct itself */
    free_char(temp_char);

    return success;
}

/**
 * Give an item to an offline player
 * Loads their character, creates item, saves to rent, and frees memory
 * Returns 1 on success, 0 on failure
 */
static int give_item_to_offline_player(const char *name, obj_vnum item_vnum)
{
    struct char_data *temp_char;
    struct obj_data *item;
    obj_rnum rnum;
    int success = 0;

    if (!name || !*name || item_vnum < 0) {
        return 0;
    }

    /* Check if player is online first */
    if (find_player_online(name)) {
        log1("ERROR: Attempted to give item to online player %s via offline method", name);
        return 0;
    }

    /* Get item real number */
    rnum = real_object(item_vnum);
    if (rnum == NOTHING) {
        log1("ERROR: Invalid item vnum %d for offline delivery", item_vnum);
        return 0;
    }

    /* Create temporary character structure */
    CREATE(temp_char, struct char_data, 1);
    clear_char(temp_char);
    CREATE(temp_char->player_specials, struct player_special_data, 1);
    new_mobile_data(temp_char);

    /* Try to load the player */
    if (load_char(name, temp_char) >= 0) {
        /* Load their rent file */
        Crash_load(temp_char);

        /* Create the item */
        item = read_object(rnum, REAL);
        if (item) {
            /* Give item to character */
            obj_to_char(item, temp_char);

            /* Save character with new item */
            Crash_crashsave(temp_char);

            log1("AUCTION: Delivered item %d to offline player %s", item_vnum, name);
            success = 1;
        } else {
            log1("ERROR: Failed to create item %d for offline delivery", item_vnum);
        }
    } else {
        log1("ERROR: Failed to load character %s for item delivery", name);
    }

    /* Extract all in-memory items from temp_char before freeing it.
     * Crash_load and obj_to_char add objects to the global object_list with
     * carried_by pointing to temp_char.  If we free temp_char without removing
     * those objects first, every remaining pointer becomes a dangling reference
     * that shows up as "carried by (null)" in the void.
     *
     * Note: temp_char is never placed in a room (IN_ROOM(temp_char) == NOWHERE),
     * so we must not call unequip_char() here — it would spam SYSERR logs and
     * attempt invalid world-array accesses.  For this temporary character we
     * detach equipment pointers directly (no AC/affect bookkeeping is needed
     * for a char that is about to be freed) and then extract_obj the object. */
    {
        int wear_pos;
        for (wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
            if (GET_EQ(temp_char, wear_pos)) {
                struct obj_data *obj = GET_EQ(temp_char, wear_pos);
                GET_EQ(temp_char, wear_pos) = NULL;
                obj->worn_by = NULL;
                obj->worn_on = NOWHERE;
                extract_obj(obj);
            }
        }
        while (temp_char->carrying) {
            struct obj_data *obj = temp_char->carrying;
            obj_from_char(obj);
            extract_obj(obj);
        }
    }

    /* free_char handles player_specials and the char struct itself */
    free_char(temp_char);

    return success;
}

/**
 * Create a new auction
 */
struct auction_data *create_auction(struct char_data *seller, struct obj_data *item, int access_mode,
                                    long starting_price, long reserve_price, int duration)
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
    /* Safely copy item name, handle NULL short_description */
    if (item->short_description) {
        strlcpy(auction->item_name, item->short_description, sizeof(auction->item_name));
    } else {
        snprintf(auction->item_name, sizeof(auction->item_name), "item #%d", GET_OBJ_VNUM(item));
    }
    auction->item_vnum = GET_OBJ_VNUM(item);
    auction->quantity = 1; /* TODO: Handle bulk auctions */

    /* Initialize with default: Ascending, First Price (English auction) */
    auction->direction = AUCTION_ASCENDING;
    auction->price_mechanism = AUCTION_FIRST_PRICE;
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

    /* Update statistics */
    global_auction_stats.total_auctions_created++;
    global_auction_stats.last_auction_time = time(0);

    log1("AUCTION: Created auction #%d for %s by %s", auction->auction_id, auction->item_name, auction->seller_name);

    return auction;
}

/**
 * Check if a character is invited to a specific auction
 */
int is_invited_to_auction(struct char_data *ch, int auction_id)
{
    struct auction_invitation *invite;

    if (!ch || auction_id < 1) {
        return 0;
    }

    for (invite = auction_invitation_list; invite; invite = invite->next) {
        if (invite->auction_id == auction_id && str_cmp(invite->invited_player, GET_NAME(ch)) == 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * Invite a player to a closed auction (seller only)
 */
int invite_player_to_auction(struct char_data *seller, int auction_id, const char *player_name)
{
    struct auction_data *auction;
    struct auction_invitation *invite;

    if (!seller || !player_name || auction_id < 1) {
        return 0;
    }

    auction = find_auction(auction_id);
    if (!auction || auction->access_mode != AUCTION_CLOSED) {
        return 0;
    }

    /* Check if seller owns this auction */
    if (str_cmp(auction->seller_name, GET_NAME(seller)) != 0) {
        return 0;
    }

    /* Check if player is already invited */
    for (struct auction_invitation *existing = auction_invitation_list; existing; existing = existing->next) {
        if (existing->auction_id == auction_id && str_cmp(existing->invited_player, player_name) == 0) {
            return 0; /* Already invited */
        }
    }

    /* Create new invitation */
    CREATE(invite, struct auction_invitation, 1);
    strlcpy(invite->invited_player, player_name, sizeof(invite->invited_player));
    invite->auction_id = auction_id;
    invite->invitation_time = time(0);

    invite->next = auction_invitation_list;
    auction_invitation_list = invite;

    log1("AUCTION: %s invited %s to auction #%d", GET_NAME(seller), player_name, auction_id);
    return 1;
}

/**
 * Remove invitation for a player from an auction (seller only)
 */
int uninvite_player_from_auction(struct char_data *seller, int auction_id, const char *player_name)
{
    struct auction_data *auction;
    struct auction_invitation *invite, *prev = NULL;

    if (!seller || !player_name || auction_id < 1) {
        return 0;
    }

    auction = find_auction(auction_id);
    if (!auction) {
        return 0;
    }

    /* Check if seller owns this auction */
    if (str_cmp(auction->seller_name, GET_NAME(seller)) != 0) {
        return 0;
    }

    /* Find and remove the invitation */
    for (invite = auction_invitation_list; invite; prev = invite, invite = invite->next) {
        if (invite->auction_id == auction_id && str_cmp(invite->invited_player, player_name) == 0) {

            if (prev) {
                prev->next = invite->next;
            } else {
                auction_invitation_list = invite->next;
            }

            free(invite);
            log1("AUCTION: %s removed invitation for %s from auction #%d", GET_NAME(seller), player_name, auction_id);
            return 1;
        }
    }

    return 0; /* Invitation not found */
}

/**
 * Player requests an auction pass from Belchior (only if invited)
 */
int request_auction_pass(struct char_data *ch, int auction_id)
{
    struct auction_data *auction;
    struct auction_pass *existing_pass;

    if (!ch || auction_id < 1) {
        return 0;
    }

    auction = find_auction(auction_id);
    if (!auction || auction->state != AUCTION_ACTIVE || auction->access_mode != AUCTION_CLOSED) {
        return 0;
    }

    /* Check if player is invited */
    if (!is_invited_to_auction(ch, auction_id)) {
        return 0;
    }

    /* Check if player already has a valid pass for this auction */
    for (existing_pass = auction_pass_list; existing_pass; existing_pass = existing_pass->next) {
        if (str_cmp(existing_pass->holder_name, GET_NAME(ch)) == 0 && existing_pass->auction_id == auction_id &&
            (existing_pass->expires == 0 || existing_pass->expires > time(0))) {
            return 0; /* Already has valid pass */
        }
    }

    /* Issue a new pass valid for 24 hours */
    give_auction_pass(ch, auction_id, 86400);
    return 1;
}

/**
 * Show who is invited to an auction (seller only)
 */
void show_auction_invitations(struct char_data *ch, int auction_id)
{
    struct auction_data *auction;
    struct auction_invitation *invite;
    int count = 0;

    auction = find_auction(auction_id);
    if (!auction) {
        send_to_char(ch, "Leilão #%d não encontrado.\r\n", auction_id);
        return;
    }

    /* Check if this is the seller */
    if (str_cmp(auction->seller_name, GET_NAME(ch)) != 0) {
        send_to_char(ch, "Apenas o vendedor pode ver a lista de convidados.\r\n");
        return;
    }

    send_to_char(ch, "Jogadores convidados para o leilão #%d:\r\n", auction_id);
    send_to_char(ch, "==========================================\r\n");

    for (invite = auction_invitation_list; invite; invite = invite->next) {
        if (invite->auction_id == auction_id) {
            count++;
            send_to_char(ch, "%d. %s (convidado há %ld minutos)\r\n", count, invite->invited_player,
                         (time(0) - invite->invitation_time) / 60);
        }
    }

    if (count == 0) {
        send_to_char(ch, "Nenhum jogador foi convidado ainda.\r\n");
    } else {
        send_to_char(ch, "\r\nTotal: %d jogador(es) convidado(s).\r\n", count);
    }
}
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
    pass->issued_time = time(0);

    pass->next = auction_pass_list;
    auction_pass_list = pass;

    send_to_char(ch, "Belchior entrega a você um passe de leilão especial para o leilão #%d.\r\n", auction_id);
    /* Only show to room if character is in a room (not in OLC/editing) */
    if (IN_ROOM(ch) != NOWHERE) {
        act("Belchior entrega um passe de leilão para $n.", FALSE, ch, 0, 0, TO_ROOM);
    }

    log1("AUCTION: Issued pass to %s for auction #%d", GET_NAME(ch), auction_id);
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
    struct auction_bid *bid, *prev_bid, **bid_ptr;

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

    /* Check if bidder has enough money */
    if (GET_GOLD(bidder) < amount) {
        return 0;
    }

    /* If this bidder already has a bid, refund and remove it */
    bid_ptr = &auction->bids;
    while (*bid_ptr) {
        prev_bid = *bid_ptr;
        if (str_cmp(prev_bid->bidder_name, GET_NAME(bidder)) == 0) {
            /* Refund previous bid */
            increase_gold(bidder, prev_bid->amount);
            /* Remove from list and free memory */
            *bid_ptr = prev_bid->next;
            free(prev_bid);
            break;
        }
        bid_ptr = &prev_bid->next;
    }

    /* Reserve money from bidder (take it now, will be used when auction ends) */
    decrease_gold(bidder, amount);

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

    /* Update statistics */
    global_auction_stats.total_bids_placed++;

    log1("AUCTION: %s bid %ld on auction #%d", GET_NAME(bidder), amount, auction_id);

    /* Check for buyout price - end auction immediately if met */
    if (auction->buyout_price > 0 && amount >= auction->buyout_price) {
        log1("AUCTION: Buyout price met on auction #%d, ending immediately", auction_id);
        end_auction(auction);
    }

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

    /* Show auction configuration */
    const char *dir_str = (auction->direction == AUCTION_ASCENDING) ? "Ascendente" : "Descendente";
    const char *mech_str = (auction->price_mechanism == AUCTION_FIRST_PRICE) ? "Primeiro Preço" : "Segundo Preço";
    send_to_char(ch, "Tipo: %s, %s\r\n", dir_str, mech_str);

    send_to_char(ch, "Acesso: %s\r\n", (auction->access_mode == AUCTION_OPEN) ? "Aberto" : "Fechado");
    send_to_char(ch, "Preço Inicial: %s moedas\r\n", format_long_br(auction->starting_price));
    send_to_char(ch, "Preço Atual: %s moedas\r\n", format_long_br(auction->current_price));

    if (auction->reserve_price > auction->starting_price) {
        send_to_char(ch, "Preço Reserva: %s moedas\r\n", format_long_br(auction->reserve_price));
    }

    if (auction->buyout_price > 0) {
        send_to_char(ch, "Preço de Compra Imediata: %s moedas\r\n", format_long_br(auction->buyout_price));
    }

    /* Show time remaining */
    if (auction->state == AUCTION_ACTIVE) {
        remaining = auction->end_time - time(0);
        if (remaining > 0) {
            send_to_char(ch, "Tempo Restante: %ldh %ldm %lds\r\n", remaining / 3600, (remaining % 3600) / 60,
                         remaining % 60);
        } else {
            send_to_char(ch, "Este leilão já expirou.\r\n");
        }
    } else {
        send_to_char(ch, "Estado: %s\r\n", (auction->state == AUCTION_INACTIVE) ? "Inativo" : "Finalizado");
    }

    /* Count bids */
    for (bid = auction->bids; bid; bid = bid->next) {
        bid_count++;
    }

    send_to_char(ch, "Total de Lances: %d\r\n", bid_count);

    if (auction->winning_bid) {
        send_to_char(ch, "Lance Vencedor: %s moedas por %s\r\n", format_long_br(auction->winning_bid->amount),
                     auction->winning_bid->bidder_name);
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
                send_to_char(ch, "ID: %d | %s | Preço: %s | Tempo: %ldm%lds | %s\r\n", auction->auction_id,
                             auction->item_name, format_long_br(auction->current_price), remaining / 60, remaining % 60,
                             (auction->access_mode == AUCTION_CLOSED) ? "FECHADO" : "ABERTO");
            }
        }
    }

    if (count == 0) {
        send_to_char(ch, "Não há leilões ativos no momento.\r\n");
    }
}

/**
 * Calculate the final price for an auction based on its mechanism
 * Returns the amount the winner should pay
 */
static long calculate_final_price(struct auction_data *auction)
{
    struct auction_bid *bid;
    long second_highest = auction->starting_price;

    if (!auction || !auction->winning_bid) {
        return 0;
    }

    /* First-price: winner pays their bid */
    if (auction->price_mechanism == AUCTION_FIRST_PRICE) {
        return auction->winning_bid->amount;
    }

    /* Second-price: winner pays second-highest bid */
    /* Find the second highest bid */
    for (bid = auction->bids; bid; bid = bid->next) {
        if (bid != auction->winning_bid && bid->amount > second_highest) {
            second_highest = bid->amount;
        }
    }

    /* If no other bids, pay the starting price */
    return second_highest;
}

/**
 * Update auction states and end expired auctions
 */
void update_auctions(void)
{
    struct auction_data *auction, *next_auction;
    time_t now = time(0);

    /* Early exit if no auctions exist - avoids unnecessary processing every 30 seconds */
    if (!auction_list) {
        return;
    }

    for (auction = auction_list; auction; auction = next_auction) {
        next_auction = auction->next;

        if (auction->state == AUCTION_ACTIVE) {
            /* Handle descending auctions - drop price automatically */
            if (auction->direction == AUCTION_DESCENDING && auction->bids == NULL) {
                /* Calculate price drop: 5% every 30 seconds, minimum 1 gold */
                time_t elapsed = now - auction->start_time;
                int drops = elapsed / 30; /* Drop every 30 seconds */

                if (drops > 0) {
                    long price_drop = (auction->starting_price * 5 * drops) / 100;
                    if (price_drop < drops) {
                        price_drop = drops; /* Minimum 1 gold per drop */
                    }

                    long new_price = auction->starting_price - price_drop;

                    /* Don't drop below reserve price or 1 gold */
                    if (new_price < auction->reserve_price && auction->reserve_price > 0) {
                        new_price = auction->reserve_price;
                    }
                    if (new_price < 1) {
                        new_price = 1;
                    }

                    /* Update current price if it changed */
                    if (new_price != auction->current_price) {
                        auction->current_price = new_price;
                    }
                }
            }

            /* Check if auction time expired */
            if (auction->end_time <= now) {
                end_auction(auction);
            }
        }
    }

    /* Clean up old finished auctions to prevent memory leaks */
    cleanup_finished_auctions();
}

/**
 * End an auction and process the results
 */
void end_auction(struct auction_data *auction)
{
    struct char_data *seller, *winner, *bidder_char;
    struct obj_data *item;
    struct auction_bid *bid;
    char buf[MAX_STRING_LENGTH];

    if (!auction) {
        return;
    }

    auction->state = AUCTION_FINISHED;

    log1("AUCTION: Ending auction #%d - %s", auction->auction_id, auction->item_name);

    /* Check if there was a winning bid */
    if (auction->winning_bid && auction->winning_bid->amount >= auction->reserve_price) {
        long final_price = calculate_final_price(auction);
        long refund_to_winner = 0;

        /* Try to find seller and winner online - using find_player_online instead of get_player_vis */
        seller = find_player_online(auction->seller_name);
        winner = find_player_online(auction->winning_bid->bidder_name);

        /* Calculate refund for winner if second-price */
        if (auction->price_mechanism == AUCTION_SECOND_PRICE && final_price < auction->winning_bid->amount) {
            refund_to_winner = auction->winning_bid->amount - final_price;
        }

        /* Refund all losing bidders */
        for (bid = auction->bids; bid; bid = bid->next) {
            /* Skip the winning bid */
            if (bid == auction->winning_bid) {
                continue;
            }

            /* Try to refund this bidder */
            bidder_char = find_player_online(bid->bidder_name);
            if (bidder_char) {
                increase_gold(bidder_char, bid->amount);
                send_to_char(bidder_char, "Seu lance de %s moedas no leilão #%d foi superado. Moedas devolvidas.\r\n",
                             format_long_br(bid->amount), auction->auction_id);
                /* Link-less player: save gold to disk in case of crash before reconnect */
                if (!bidder_char->desc)
                    save_char(bidder_char);
            } else {
                /* Bidder offline - log for manual refund */
                log1("AUCTION: Bidder %s offline, needs %ld gold refunded", bid->bidder_name, bid->amount);
            }
        }

        /* Refund winner if second-price mechanism */
        if (refund_to_winner > 0 && winner) {
            increase_gold(winner, refund_to_winner);
            /* Link-less winner: save gold refund to disk in case of crash before reconnect */
            if (!winner->desc)
                save_char(winner);
        }

        /* Create the item for the winner */
        if (auction->item_vnum > 0) {
            obj_rnum rnum = real_object(auction->item_vnum);
            if (rnum != NOTHING) {
                item = read_object(rnum, REAL);
                if (item) {
                    /* Deliver item to winner */
                    if (winner) {
                        obj_to_char(item, winner);
                        /* Link-less winner: crash-save immediately so item survives a server crash */
                        if (!winner->desc)
                            Crash_crashsave(winner);
                        /* Safely access item description */
                        const char *item_desc = item->short_description ? item->short_description : "um item";
                        if (auction->price_mechanism == AUCTION_SECOND_PRICE && refund_to_winner > 0) {
                            snprintf(buf, sizeof(buf),
                                     "Você ganhou o leilão #%d! %s foi entregue a você.\r\n"
                                     "Você ofereceu %s mas pagou apenas %s (segundo preço).\r\n",
                                     auction->auction_id, item_desc, format_long_br(auction->winning_bid->amount),
                                     format_long_br(final_price));
                        } else {
                            snprintf(buf, sizeof(buf), "Você ganhou o leilão #%d! %s foi entregue a você.\r\n",
                                     auction->auction_id, item_desc);
                        }
                        send_to_char(winner, "%s", buf);
                    } else {
                        /* Winner offline - deliver item to their rent file */
                        if (give_item_to_offline_player(auction->winning_bid->bidder_name, auction->item_vnum)) {
                            /* Item delivered successfully */
                            extract_obj(item);
                        } else {
                            /* Failed to deliver - log for manual intervention */
                            log1(
                                "AUCTION CRITICAL: Failed to deliver item %d to offline winner %s - manual "
                                "intervention required",
                                auction->item_vnum, auction->winning_bid->bidder_name);
                            extract_obj(item); /* Clean up object */
                        }
                    }
                }
            }
        }

        /* Transfer money to seller */
        if (seller) {
            increase_gold(seller, final_price);
            snprintf(buf, sizeof(buf), "Seu leilão #%d foi vendido por %s moedas!\r\n", auction->auction_id,
                     format_long_br(final_price));
            send_to_char(seller, "%s", buf);
            /* Link-less seller: save gold to disk in case of crash before reconnect */
            if (!seller->desc)
                save_char(seller);
        } else {
            /* Seller offline - credit gold to their character file */
            if (!give_gold_to_offline_player(auction->seller_name, final_price)) {
                log1("AUCTION CRITICAL: Failed to credit %ld gold to offline seller %s - manual intervention required",
                     final_price, auction->seller_name);
            }
        }

        log1("AUCTION: Auction #%d sold to %s for %ld gold (bid %ld)", auction->auction_id,
             auction->winning_bid->bidder_name, final_price, auction->winning_bid->amount);

        /* Update statistics */
        global_auction_stats.total_auctions_completed++;
        global_auction_stats.total_gold_traded += final_price;
    } else {
        /* No winning bid or reserve not met - refund all bidders and return item to seller */
        seller = find_player_online(auction->seller_name);

        /* Refund all bidders */
        for (bid = auction->bids; bid; bid = bid->next) {
            bidder_char = find_player_online(bid->bidder_name);
            if (bidder_char) {
                increase_gold(bidder_char, bid->amount);
                send_to_char(bidder_char, "O leilão #%d foi cancelado. Seu lance de %s moedas foi devolvido.\r\n",
                             auction->auction_id, format_long_br(bid->amount));
                /* Link-less player: save gold to disk in case of crash before reconnect */
                if (!bidder_char->desc)
                    save_char(bidder_char);
            } else {
                /* Refund offline bidder */
                if (!give_gold_to_offline_player(bid->bidder_name, bid->amount)) {
                    log1(
                        "AUCTION CRITICAL: Failed to refund %ld gold to offline bidder %s - manual intervention "
                        "required",
                        bid->amount, bid->bidder_name);
                }
            }
        }

        /* Return item to seller */
        if (auction->item_vnum > 0) {
            obj_rnum rnum = real_object(auction->item_vnum);
            if (rnum != NOTHING) {
                item = read_object(rnum, REAL);
                if (item) {
                    if (seller) {
                        obj_to_char(item, seller);
                        send_to_char(seller, "Seu leilão #%d não teve lances válidos. O item foi devolvido.\r\n",
                                     auction->auction_id);
                        /* Link-less seller: crash-save immediately so item survives a server crash */
                        if (!seller->desc)
                            Crash_crashsave(seller);
                    } else {
                        /* Return item to offline seller */
                        if (give_item_to_offline_player(auction->seller_name, auction->item_vnum)) {
                            extract_obj(item);
                        } else {
                            log1(
                                "AUCTION CRITICAL: Failed to return item %d to offline seller %s - manual intervention "
                                "required",
                                auction->item_vnum, auction->seller_name);
                            extract_obj(item);
                        }
                    }
                }
            }
        }

        log1("AUCTION: Auction #%d ended without valid bids", auction->auction_id);

        /* Update statistics */
        global_auction_stats.total_auctions_failed++;
    }
}

/**
 * Belchior's combined special function - auction house management, shop keeper, and cryogenicist
 */
SPECIAL(belchior_auctioneer)
{
    struct char_data *belchior = (struct char_data *)me;
    room_rnum shop_room, auction_room;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int auction_id;

    /* Validate belchior character pointer */
    if (!belchior || !ch) {
        return FALSE;
    }

    /* Validate argument pointer to prevent NULL dereference */
    if (!argument) {
        return FALSE;
    }

    /* Return FALSE for shop-related commands to let shop_keeper handle them */
    if (CMD_IS("buy") || CMD_IS("sell") || CMD_IS("list") || CMD_IS("value") || CMD_IS("identify")) {
        return FALSE;
    }

    /* Check for cryogenicist commands */
    if (CMD_IS("offer") || CMD_IS("rent")) {
        return cryogenicist(ch, me, cmd, argument);
    }

    /* Handle auction pass requests */
    if (CMD_IS("passe") || CMD_IS("pass")) {
        skip_spaces(&argument);
        two_arguments(argument, arg1, arg2);

        if (!*arg1) {
            act("Belchior diz: 'Para obter um passe de leilão, me diga: passe [numero_do_leilao]'", FALSE, ch, 0,
                belchior, TO_CHAR);
            return TRUE;
        }

        auction_id = atoi(arg1);
        if (auction_id < 1) {
            act("Belchior diz: 'Número de leilão inválido.'", FALSE, ch, 0, belchior, TO_CHAR);
            return TRUE;
        }

        if (!CONFIG_NEW_AUCTION_SYSTEM && GET_LEVEL(ch) < LVL_IMMORT) {
            act("Belchior balança a cabeça: 'Desculpe, o sistema de leilões ainda não está funcionando.'", FALSE, ch, 0,
                belchior, TO_CHAR);
            return TRUE;
        }

        if (request_auction_pass(ch, auction_id)) {
            act("Belchior sorri: 'Aqui está seu passe para o leilão. Seja bem-vindo!'", FALSE, ch, 0, belchior,
                TO_CHAR);
        } else {
            struct auction_data *auction = find_auction(auction_id);
            if (!auction) {
                act("Belchior diz: 'Este leilão não existe. Use \"leilao listar\" para ver os leilões ativos.'", FALSE,
                    ch, 0, belchior, TO_CHAR);
            } else if (auction->state != AUCTION_ACTIVE) {
                act("Belchior diz: 'Este leilão não está ativo. Use \"leilao listar\" para ver os leilões ativos.'",
                    FALSE, ch, 0, belchior, TO_CHAR);
            } else if (auction->access_mode != AUCTION_CLOSED) {
                act("Belchior sorri: 'Este é um leilão aberto! Você não precisa de passe.'", FALSE, ch, 0, belchior,
                    TO_CHAR);
                act("Belchior aponta para a escada: 'Basta descer para a casa de leilões.'", FALSE, ch, 0, belchior,
                    TO_CHAR);
            } else if (!is_invited_to_auction(ch, auction_id)) {
                act("Belchior balança a cabeça: 'Você não foi convidado para este leilão fechado.'", FALSE, ch, 0,
                    belchior, TO_CHAR);
                act("Belchior diz: 'Apenas jogadores convidados pelo vendedor podem participar.'", FALSE, ch, 0,
                    belchior, TO_CHAR);
            } else {
                act("Belchior diz: 'Você já possui um passe válido para este leilão.'", FALSE, ch, 0, belchior,
                    TO_CHAR);
                act("Belchior aponta para a escada: 'Pode descer para a casa de leilões.'", FALSE, ch, 0, belchior,
                    TO_CHAR);
            }
        }
        return TRUE;
    }

    /* Check auction house access commands */
    if (!CMD_IS("down") && !CMD_IS("descer") && !CMD_IS("baixo")) {
        return FALSE;
    }

    /* Check if we're in Belchior's shop */
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != 3081) {
        return FALSE;
    }

    /* Only allow access if new auction system is enabled or player is immortal */
    if (!CONFIG_NEW_AUCTION_SYSTEM && GET_LEVEL(ch) < LVL_IMMORT) {
        act("Belchior balança a cabeça: 'Desculpe, o sistema de leilões ainda não está funcionando.'", FALSE, ch, 0,
            belchior, TO_CHAR);
        return TRUE;
    }

    shop_room = real_room(3081);
    auction_room = real_room(3092);

    if (shop_room == NOWHERE || auction_room == NOWHERE) {
        send_to_char(ch, "Há um problema com as salas do leilão.\r\n");
        return TRUE;
    }

    /* Enhanced pass validation - check each closed auction individually */
    struct auction_data *auction;
    int needs_pass = 0;
    int has_valid_pass = 0;

    for (auction = auction_list; auction; auction = auction->next) {
        if (auction->state == AUCTION_ACTIVE && auction->access_mode == AUCTION_CLOSED) {
            needs_pass = 1;
            /* Check if player has valid pass for ANY closed auction OR is invited */
            if (has_auction_pass(ch, auction->auction_id) || is_invited_to_auction(ch, auction->auction_id)) {
                has_valid_pass = 1;
                break;
            }
        }
    }

    if (needs_pass && !has_valid_pass) {
        act("Belchior bloqueia seu caminho: 'Há leilões fechados acontecendo.'", FALSE, ch, 0, belchior, TO_CHAR);
        act("Belchior explica: 'Se você foi convidado, primeiro obtenha um passe usando: passe [numero_leilao]'", FALSE,
            ch, 0, belchior, TO_CHAR);
        act("Belchior continua: 'Use \"leilao listar\" para ver os leilões disponíveis.'", FALSE, ch, 0, belchior,
            TO_CHAR);
        act("Belchior impede $n de descer.", FALSE, ch, 0, 0, TO_ROOM);
        return TRUE;
    }

    /* Unlock and open the door temporarily */
    if (EXIT(ch, DOWN)) {
        REMOVE_BIT(EXIT(ch, DOWN)->exit_info, EX_LOCKED);
        REMOVE_BIT(EXIT(ch, DOWN)->exit_info, EX_HIDDEN);

        act("Belchior sussurra algo e uma porta secreta se abre no chão.", FALSE, ch, 0, belchior, TO_ROOM);

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
 * Clean up finished auctions and their associated data
 * Called periodically to free memory from completed auctions
 */
void cleanup_finished_auctions(void)
{
    struct auction_data *auction, *prev_auction, *next_auction;
    struct auction_bid *bid, *next_bid;
    time_t now = time(0);
    time_t cleanup_threshold = 3600; /* Clean up auctions finished more than 1 hour ago */

    prev_auction = NULL;
    for (auction = auction_list; auction; auction = next_auction) {
        next_auction = auction->next;

        /* Clean up finished auctions that are old enough */
        if (auction->state == AUCTION_FINISHED && (now - auction->end_time) > cleanup_threshold) {
            log1("AUCTION: Cleaning up finished auction #%d", auction->auction_id);

            /* Free all bids */
            for (bid = auction->bids; bid; bid = next_bid) {
                next_bid = bid->next;
                free(bid);
            }

            /* Free invited players string if allocated */
            if (auction->invited_players) {
                free(auction->invited_players);
            }

            /* Remove from list */
            if (prev_auction) {
                prev_auction->next = next_auction;
            } else {
                auction_list = next_auction;
            }

            /* Free the auction itself */
            free(auction);

            /* Don't update prev_auction since we removed this one */
            continue;
        }

        prev_auction = auction;
    }
}

/**
 * Save auctions to disk
 */
void save_auctions(void)
{
    FILE *fl;
    struct auction_data *auction;
    struct auction_bid *bid;
    char filename[256];
    int active_count = 0;

    snprintf(filename, sizeof(filename), "lib/etc/auctions.dat");

    if (!(fl = fopen(filename, "w"))) {
        log1("AUCTION: Unable to open auction file for writing: %s", filename);
        return;
    }

    /* Write header with format explanation */
    fprintf(fl, "# Auction System Save File\n");
    fprintf(fl, "# Format:\n");
    fprintf(fl, "# NEXT_ID <next_auction_id>\n");
    fprintf(fl, "# STATS <created> <completed> <failed> <gold_traded> <bids_placed> <last_time>\n");
    fprintf(fl, "# A <id> <seller> <item_vnum> <direction> <mechanism> <access>\n");
    fprintf(fl, "#   <start_price> <current_price> <reserve> <buyout>\n");
    fprintf(fl, "#   <start_time> <end_time> <duration> <item_name>\n");
    fprintf(fl, "# B <bidder_name> <amount> <timestamp>\n");
    fprintf(fl, "# E (end of auction)\n");
    fprintf(fl, "#\n");

    /* Save next auction ID and statistics */
    fprintf(fl, "NEXT_ID %d\n", next_auction_id);
    fprintf(fl, "STATS %d %d %d %ld %d %ld\n", global_auction_stats.total_auctions_created,
            global_auction_stats.total_auctions_completed, global_auction_stats.total_auctions_failed,
            global_auction_stats.total_gold_traded, global_auction_stats.total_bids_placed,
            (long)global_auction_stats.last_auction_time);

    /* Save each active auction */
    for (auction = auction_list; auction; auction = auction->next) {
        if (auction->state != AUCTION_ACTIVE) {
            continue; /* Only save active auctions */
        }

        /* Auction header: A <id> <seller> <item_vnum> <direction> <mechanism> <access> */
        fprintf(fl, "A %d %s %d %d %d %d\n", auction->auction_id, auction->seller_name, auction->item_vnum,
                auction->direction, auction->price_mechanism, auction->access_mode);

        /* Price information */
        fprintf(fl, "P %ld %ld %ld %ld\n", auction->starting_price, auction->current_price, auction->reserve_price,
                auction->buyout_price);

        /* Time information */
        fprintf(fl, "T %ld %ld %d\n", (long)auction->start_time, (long)auction->end_time, auction->duration);

        /* Item name (may contain spaces) */
        fprintf(fl, "I %s\n", auction->item_name);

        /* Save bids: B <bidder_name> <amount> <timestamp> */
        for (bid = auction->bids; bid; bid = bid->next) {
            fprintf(fl, "B %s %ld %ld\n", bid->bidder_name, bid->amount, (long)bid->timestamp);
        }

        /* End of auction marker */
        fprintf(fl, "E\n");
        active_count++;
    }

    fclose(fl);
    log1("AUCTION: Saved %d active auctions to disk", active_count);
}

/**
 * Load auctions from disk
 */
void load_auctions(void)
{
    FILE *fl;
    char filename[256];
    char line[MAX_INPUT_LENGTH];
    char keyword;
    struct auction_data *auction = NULL;
    struct auction_bid *bid;
    int loaded_count = 0;

    snprintf(filename, sizeof(filename), "lib/etc/auctions.dat");

    if (!(fl = fopen(filename, "r"))) {
        log1("AUCTION: No auction file to load (this is normal on first run)");
        return;
    }

    while (fgets(line, sizeof(line), fl)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        /* Remove newline */
        line[strcspn(line, "\n")] = 0;

        keyword = line[0];

        if (keyword == 'N') { /* NEXT_ID */
            sscanf(line, "NEXT_ID %d", &next_auction_id);
        } else if (keyword == 'S') { /* STATS */
            long temp_time;
            sscanf(line, "STATS %d %d %d %ld %d %ld", &global_auction_stats.total_auctions_created,
                   &global_auction_stats.total_auctions_completed, &global_auction_stats.total_auctions_failed,
                   &global_auction_stats.total_gold_traded, &global_auction_stats.total_bids_placed, &temp_time);
            global_auction_stats.last_auction_time = (time_t)temp_time;
        } else if (keyword == 'A') { /* Auction header */
            int temp_vnum;
            CREATE(auction, struct auction_data, 1);
            sscanf(line, "A %d %s %d %d %d %d", &auction->auction_id, auction->seller_name, &temp_vnum,
                   &auction->direction, &auction->price_mechanism, &auction->access_mode);
            auction->item_vnum = (obj_vnum)temp_vnum;
            auction->state = AUCTION_ACTIVE;
            auction->bids = NULL;
            auction->winning_bid = NULL;
            auction->invited_players = NULL;
            auction->next = auction_list;
            auction_list = auction;
            loaded_count++;
        } else if (keyword == 'P' && auction) { /* Price information */
            sscanf(line, "P %ld %ld %ld %ld", &auction->starting_price, &auction->current_price,
                   &auction->reserve_price, &auction->buyout_price);
        } else if (keyword == 'T' && auction) { /* Time information */
            long start_t, end_t;
            sscanf(line, "T %ld %ld %d", &start_t, &end_t, &auction->duration);
            auction->start_time = (time_t)start_t;
            auction->end_time = (time_t)end_t;
        } else if (keyword == 'I' && auction) { /* Item name */
            /* Read the rest of the line after "I " */
            const char *name_start = line + 2;
            strlcpy(auction->item_name, name_start, sizeof(auction->item_name));
        } else if (keyword == 'B' && auction) { /* Bid */
            long timestamp;
            CREATE(bid, struct auction_bid, 1);
            sscanf(line, "B %s %ld %ld", bid->bidder_name, &bid->amount, &timestamp);
            bid->timestamp = (time_t)timestamp;
            bid->next = auction->bids;
            auction->bids = bid;

            /* Update winning bid if this is the highest */
            if (!auction->winning_bid || bid->amount > auction->winning_bid->amount) {
                auction->winning_bid = bid;
            }
        } else if (keyword == 'E') { /* End of auction */
            auction = NULL;
        }
    }

    fclose(fl);
    log1("AUCTION: Loaded %d auctions from disk", loaded_count);
}

/**
 * Show auction statistics
 */
void show_auction_stats(struct char_data *ch)
{
    int active_count = 0;
    struct auction_data *auction;

    /* Count active auctions */
    for (auction = auction_list; auction; auction = auction->next) {
        if (auction->state == AUCTION_ACTIVE) {
            active_count++;
        }
    }

    send_to_char(ch, "\r\n&c=== ESTATÍSTICAS DO SISTEMA DE LEILÕES ===&n\r\n\r\n");
    send_to_char(ch, "Total de leilões criados: %d\r\n", global_auction_stats.total_auctions_created);
    send_to_char(ch, "Leilões completados: %d\r\n", global_auction_stats.total_auctions_completed);
    send_to_char(ch, "Leilões falhados: %d\r\n", global_auction_stats.total_auctions_failed);
    send_to_char(ch, "Leilões ativos: %d\r\n", active_count);
    send_to_char(ch, "Total de lances: %d\r\n", global_auction_stats.total_bids_placed);
    send_to_char(ch, "Ouro total negociado: %s moedas\r\n", format_long_br(global_auction_stats.total_gold_traded));

    if (global_auction_stats.last_auction_time > 0) {
        char timebuf[80];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&global_auction_stats.last_auction_time));
        send_to_char(ch, "Último leilão: %s\r\n", timebuf);
    }

    send_to_char(ch, "\r\n");
}