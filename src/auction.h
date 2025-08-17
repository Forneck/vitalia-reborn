/**
 * @file auction.h
 * Auction system definitions, structures, constants.
 *
 * Part of the Vitália Reborn MUD experimental features.
 * Provides advanced auction functionality including various auction types.
 */
#ifndef _AUCTION_H_
#define _AUCTION_H_

#include "structs.h"

/* Auction types */
#define AUCTION_TYPE_ENGLISH 0 /* Ascendente, Primeiro Preço */
#define AUCTION_TYPE_DUTCH 1   /* Descendente, Segundo Preço */

/* Auction access modes */
#define AUCTION_OPEN 0   /* Anyone can bid */
#define AUCTION_CLOSED 1 /* Only invited players with auction pass */

/* Auction states */
#define AUCTION_INACTIVE 0
#define AUCTION_ACTIVE 1
#define AUCTION_FINISHED 2

/* Maximum auction duration in seconds */
#define MAX_AUCTION_TIME 3600 /* 1 hour */
#define MIN_AUCTION_TIME 60   /* 1 minute */

/* Auction bid structure */
struct auction_bid {
    char bidder_name[MAX_NAME_LENGTH]; /* Name of the bidder */
    long amount;                       /* Bid amount */
    time_t timestamp;                  /* When the bid was made */
    struct auction_bid *next;          /* Next bid in the list */
};

/* Main auction structure */
struct auction_data {
    int auction_id;                    /* Unique auction ID */
    char seller_name[MAX_NAME_LENGTH]; /* Name of the seller */
    char item_name[MAX_INPUT_LENGTH];  /* Description of item being sold */
    obj_vnum item_vnum;                /* Virtual number of the item */
    int quantity;                      /* Number of items (for bulk auctions) */

    int auction_type; /* AUCTION_TYPE_* */
    int access_mode;  /* AUCTION_OPEN or AUCTION_CLOSED */
    int state;        /* AUCTION_* state */

    long starting_price; /* Starting bid amount */
    long current_price;  /* Current highest bid */
    long reserve_price;  /* Minimum acceptable price */
    long buyout_price;   /* Instant buy price (0 = no buyout) */

    time_t start_time; /* When auction started */
    time_t end_time;   /* When auction ends */
    int duration;      /* Auction duration in seconds */

    struct auction_bid *bids;        /* Linked list of bids */
    struct auction_bid *winning_bid; /* Current winning bid */

    char *invited_players; /* List of invited players (for closed auctions) */

    struct auction_data *next; /* Next auction in the global list */
};

/* Auction invitation structure */
struct auction_invitation {
    char invited_player[MAX_NAME_LENGTH]; /* Name of invited player */
    int auction_id;                       /* Auction they're invited to */
    time_t invitation_time;               /* When they were invited */
    struct auction_invitation *next;      /* Next invitation */
};

/* Auction pass object for closed auctions */
struct auction_pass {
    char holder_name[MAX_NAME_LENGTH]; /* Player who owns this pass */
    int auction_id;                    /* Specific auction ID (0 = any auction) */
    time_t expires;                    /* When this pass expires */
    time_t issued_time;                /* When this pass was issued */
    struct auction_pass *next;         /* Next pass in the list */
};

/* Global auction list */
extern struct auction_data *auction_list;
extern struct auction_pass *auction_pass_list;
extern struct auction_invitation *auction_invitation_list;
extern int next_auction_id;

/* Function prototypes */
struct auction_data *create_auction(struct char_data *seller, struct obj_data *item, int type, int access_mode,
                                    long starting_price, long reserve_price, int duration);
int place_bid(struct char_data *bidder, int auction_id, long amount);
int has_auction_pass(struct char_data *ch, int auction_id);
int is_invited_to_auction(struct char_data *ch, int auction_id);
int invite_player_to_auction(struct char_data *seller, int auction_id, const char *player_name);
int uninvite_player_from_auction(struct char_data *seller, int auction_id, const char *player_name);
int request_auction_pass(struct char_data *ch, int auction_id);
void give_auction_pass(struct char_data *ch, int auction_id, int duration);
void update_auctions(void);
void show_auction_list(struct char_data *ch);
void show_auction_details(struct char_data *ch, int auction_id);
void show_auction_invitations(struct char_data *ch, int auction_id);
struct auction_data *find_auction(int auction_id);
void end_auction(struct auction_data *auction);
void save_auctions(void);
void load_auctions(void);

/* Special function prototype for Belchior */
SPECIAL(belchior_auctioneer);

#endif /* _AUCTION_H_ */