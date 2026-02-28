# Auction House System

## Status

**Status**: ⚠️ Experimental — Immortal-only access (February 2026)  
**Source**: `src/auction.c`, `src/act.auction.c`  
**NPC**: Belchior (auction house keeper) — `SPECIAL(belchior_auctioneer)`  
**CEDIT toggle**: `CONFIG_NEW_AUCTION_SYSTEM` (`new_auction_system` in `lib/etc/config`)  
**Command**: `leilao` (subcommands listed below)

---

## Overview

The Auction House is an experimental player-driven item marketplace where sellers can create timed auctions for items, and buyers can submit bids.  The system supports open (anyone can bid) and closed (invite-only with auction passes) auction types, ascending and Dutch (descending) price mechanisms, and both first-price and second-price (Vickrey) settlement rules.

Access is currently **restricted to immortals** for stability testing.  Full player access is enabled by toggling `CONFIG_NEW_AUCTION_SYSTEM` via CEDIT (see below).

---

## Accessing the Auction House

The auction house is accessible through **Belchior's shop**.  Players descend from Belchior's location into the auction house room:

```
→ Go to Belchior (shopkeeper NPC)
→ Down from Belchior's shop leads to the auction house
```

Belchior serves as both shopkeeper and auction house gatekeeper.  He issues **auction passes** for closed (invite-only) auctions.

---

## Player Commands

All commands use the `leilao` prefix:

| Command | Description |
|---------|-------------|
| `leilao listar` | List all active auctions |
| `leilao criar [item] [preço]` | Create a new auction for an item with a starting price |
| `leilao configurar [id] [opção] [valor]` | Configure an active auction (seller only) |
| `leilao dar [id] [valor]` | Place a bid on an auction |
| `leilao info [id]` | View full details of a specific auction |
| `leilao convidar [id] [jogador]` | Invite a player to a closed auction (seller only) |
| `leilao desconvidar [id] [jogador]` | Remove a player's invitation (seller only) |
| `leilao convidados [id]` | View the guest list for a closed auction (seller only) |
| `leilao passe` | Request an auction pass from Belchior for a closed auction |
| `leilao ajuda` | Display detailed help on auction types and configuration options |
| `leilao estatisticas` | View auction system statistics |

### Belchior's pass command

To enter a closed auction, a player must first obtain a pass from Belchior:

```
> passe [numero_do_leilao]
Belchior sorri: 'Aqui está seu passe para o leilão. Seja bem-vindo!'
```

Passes are granted only to players who have been invited by the seller (`leilao convidar`).

---

## Auction Types

### Direction

| Type | Constant | Behaviour |
|------|----------|-----------|
| Ascending | `AUCTION_ASCENDING` (0) | Starting price rises as bids come in (traditional) |
| Descending (Dutch) | `AUCTION_DESCENDING` (1) | Price starts high and drops until someone bids |

### Price Mechanism

| Type | Constant | Behaviour |
|------|----------|-----------|
| First-price | `AUCTION_FIRST_PRICE` (0) | Winner pays their own bid |
| Second-price (Vickrey) | `AUCTION_SECOND_PRICE` (1) | Winner pays the second-highest bid |

### Access Mode

| Type | Constant | Behaviour |
|------|----------|-----------|
| Open | `AUCTION_OPEN` (0) | Any player can see and bid |
| Closed | `AUCTION_CLOSED` (1) | Only invited players with a valid auction pass can bid |

---

## Auction Parameters

| Parameter | Description |
|-----------|-------------|
| Starting price | Minimum opening bid |
| Reserve price | Minimum acceptable price; if not met, auction fails |
| Buyout price | Optional instant-win price (0 = no buyout) |
| Duration | 60–3600 seconds (`MIN_AUCTION_TIME` to `MAX_AUCTION_TIME`) |

---

## Settlement

When an auction ends:

- **Online winner**: Item is transferred directly to the winning bidder; gold is transferred to the seller.
- **Offline winner**: Item and gold are queued for delivery when the player next logs in (via `increase_gold` and `obj_to_char` credit on login).
- If the reserve price is not met, the auction ends without a sale and the item is returned to the seller.

---

## CEDIT Toggle

The system is gated by a single configuration flag:

```
cedit → [toggle new_auction_system]
```

| State | Effect |
|-------|--------|
| `OFF` (default) | Only immortals can use auction commands; Belchior declines passes |
| `ON` | All players can create and bid on auctions |

The flag is stored as `new_auction_system` in `lib/etc/config`.

**Path to full player access:**
1. Complete stability testing with immortal accounts.
2. Verify gold and item delivery for both online and offline cases.
3. Set `CONFIG_NEW_AUCTION_SYSTEM = YES` via CEDIT and save.

---

## Data Structures

```c
struct auction_data {
    int auction_id;            /* Unique ID */
    char seller_name[];        /* Seller's name */
    char item_name[];          /* Item description */
    obj_vnum item_vnum;        /* Item prototype vnum */
    int direction;             /* AUCTION_ASCENDING or AUCTION_DESCENDING */
    int price_mechanism;       /* AUCTION_FIRST_PRICE or AUCTION_SECOND_PRICE */
    int access_mode;           /* AUCTION_OPEN or AUCTION_CLOSED */
    int state;                 /* AUCTION_INACTIVE / ACTIVE / FINISHED */
    long starting_price;
    long current_price;
    long reserve_price;
    long buyout_price;
    time_t start_time;
    time_t end_time;
    int duration;              /* Seconds */
    struct auction_bid *bids;
    struct auction_bid *winning_bid;
    char *invited_players;     /* For closed auctions */
};
```

Auction state is persisted via `save_auctions()` / `load_auctions()` using `lib/etc/auctions.dat`.  When the new auction system is enabled, active auctions saved to this file are reloaded at boot by `src/db.c`, so most auctions survive a normal reboot.  Periodic saves occur in `src/comm.c`; any state written since the last save can still be lost on a crash or abrupt shutdown.

---

## Known Limitations (Experimental)

- Auction persistence is best-effort: auctions are periodically saved to `lib/etc/auctions.dat` and reloaded at boot, but crashes between saves can lose recent bids or configuration changes.  Invitation and pass bookkeeping is not separately persisted and resets on reboot.
- Closed auctions require Belchior to be reachable; if Belchior is extracted (e.g., zone reset), pass requests silently fail.
- Descending-price (Dutch) auctions use an automatic price-drop mechanism in `update_auctions()`: price drops by **5% of starting price every 30 seconds** while no bids exist, down to the reserve price (or 1 gold).  Buyers must monitor `leilao info` actively to catch a specific price point; the drop cadence is system-defined and not configurable per auction.
- No in-game announcement channel integration; buyers must poll with `leilao listar`.

---

## Related Files

| File | Role |
|------|------|
| `src/auction.c` | Core data management: create, bid, pass, settlement, Belchior special |
| `src/act.auction.c` | Player command handler (`do_auction`) |
| `src/auction.h` | Data structures, constants, and function prototypes |
| `src/db.c` | Calls `load_auctions()` at boot |
| `src/oasis.h` | `CEDIT_NEW_AUCTION_SYSTEM` menu constant |
| `lib/etc/config` | Stores `new_auction_system` flag |
