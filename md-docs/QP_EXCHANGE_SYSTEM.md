# QP ↔ Gold Exchange System

## Status

**Status**: ✅ Implemented (February 2026)  
**Source**: `src/spec_procs.c` — `SPECIAL(qp_exchange)`, `calculate_qp_exchange_base_rate()`  
**NPC assignment**: `src/spec_assign.c` — mob vnum **2999** (`qp_exchange` special)  
**Commands**: `cotacao` / `rate`, `comprar` / `buy`, `vender` / `sell`

---

## Overview

The QP Exchange system allows players to convert **Quest Points (QP)** into **gold** and vice versa via a dedicated exchange NPC (mob vnum 2999).  The exchange rate is dynamic: it is recalculated once per MUD month from the current ratio of total gold to total QP held by all mortal players, so it tracks the economy over time.

---

## Player Commands

All commands are issued while standing in the same room as the exchange NPC.

### `cotacao` / `rate`

Show the current exchange rate and available commands.

```
> cotacao
Cotação de Pontos de Busca
Mês atual: Brumis
Taxa de câmbio: 10.000 moedas por 1 QP

Comandos disponíveis:
  buy <quantidade>     / comprar <quantidade>  - Compra QPs com ouro
  sell <quantidade>    / vender <quantidade>   - Vende QPs por ouro
```

### `comprar <quantidade>` / `buy <quantidade>`

Purchase QP with gold.  Cost = `amount × current_rate`.

- Player must carry enough gold on hand (`GET_GOLD`).
- QP is capped at `MAX_QP`; overflow is clamped and the player is warned when already at or when reaching the cap.
- Transaction is logged at `NRM` importance for immortals.

### `vender <quantidade>` / `sell <quantidade>`

Sell QP for gold.  Gold received = `amount × current_rate`.

- Player must hold at least `amount` QP.
- Gold added via `increase_gold()` (follows standard gold-overflow protection).
- Transaction is logged at `NRM` importance for immortals.

---

## Dynamic Exchange Rate

The system uses two related but distinct rate mechanisms:

### Runtime Rate (`get_qp_exchange_rate()`)

The rate seen by players during transactions is computed by `get_qp_exchange_rate()` in `src/utils.c`.  It calculates `total_gold / total_qp` from live economy statistics and caches the result for **30 minutes** (`QP_EXCHANGE_CACHE_TTL = 1800 s`).  On the next transaction after the cache expires, the rate is silently recalculated.

This means the displayed rate can reflect economy changes within 30 minutes without any manual action.

### Monthly Base Rate (`qp_exchange_base_rate`)

A separate monthly recalculation runs once per MUD month (17 months per year: Brumis, Kames'Hi, Teriany, …, Tellus) via `calculate_qp_exchange_base_rate()`, triggered from `weather.c` when `time_info.month` changes:

```c
update_qp_exchange_rate_on_month_change();
```

This function iterates all player files to get a precise economy snapshot and saves the result to `lib/etc/qp_exchange_rate` for persistence across reboots.  The 30-minute runtime cache seeds from this saved value at boot.

### Rate Bounds

| Constant | Value | Description |
|----------|-------|-------------|
| `QP_EXCHANGE_DEFAULT_BASE_RATE` | 10 000 | Starting / fallback rate (gold per QP) |
| `QP_EXCHANGE_MIN_BASE_RATE` | 1 000 | Floor: never cheaper than 1 000 gold/QP |
| `QP_EXCHANGE_MAX_BASE_RATE` | 100 000 000 | Ceiling: prevents int overflow |

### Persistence

The monthly base rate and the month it was last calculated are stored in:

```
lib/etc/qp_exchange_rate
```

Format: `<rate> <month>` on a single line.  The file is written after each monthly recalculation and read at boot (`load_qp_exchange_rate()`).  If the file is absent or invalid, `QP_EXCHANGE_DEFAULT_BASE_RATE` is used.

---

## Overflow Protection

Both buy and sell operations guard against integer overflow:

```c
/* After calculating cost = amount * current_rate: */
if (cost < 0 || (current_rate > 0 && cost / current_rate != amount)) {
    send_to_char(ch, "Essa quantidade é muito alta para calcular!\r\n");
    return (TRUE);
}
```

QP purchases additionally clamp to `MAX_QP` before modifying the player's QP field, following the same pattern as `increase_gold()`.

Players who are dead or have the `PLR_TRNS` (transcended) flag set cannot perform exchanges.

---

## Administration Notes

- The exchange NPC (vnum 2999) uses `SPECIAL(qp_exchange)` assigned in `spec_assign.c`.
- Immortals (level ≥ `LVL_IMMORT`) are excluded from the economy scan so they do not skew the rate.
- All transactions are logged with `mudlog(NRM, LVL_IMMORT, ...)` so they are visible in the immortal log.
- To force a rate recalculation immediately, reboot the MUD or wait for a month change.

---

## Related Files

| File | Role |
|------|------|
| `src/spec_procs.c` | Exchange NPC special proc (`qp_exchange`) and rate calculation |
| `src/spec_assign.c` | Assigns `qp_exchange` to mob vnum 2999 |
| `src/utils.c` | `get_qp_exchange_rate()` — runtime accessor used throughout the codebase |
| `src/utils.h` | Rate constants (`QP_EXCHANGE_DEFAULT_BASE_RATE`, etc.) |
| `src/armweap.c` | Uses `get_qp_exchange_rate()` to convert gold costs to QP costs in shops |
| `src/db.c` | Calls `load_qp_exchange_rate()` at boot |
| `src/weather.c` | Calls `update_qp_exchange_rate_on_month_change()` on month transitions |
| `lib/etc/qp_exchange_rate` | Persistent storage for the current rate |
| `src/interpreter.c` | Registers `cotacao` as a no-arg command (maps to `do_not_here` when away from NPC) |
