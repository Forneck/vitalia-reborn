# Lag Fix Summary - 30 Second Periodic Lag Issue

## Problem Identified

The 30-second periodic lag affecting a single player was caused by **mobs attempting to find questmasters** using the `find_accessible_questmaster_in_zone()` function, which is catastrophically expensive when there are many quests active.

### Root Cause Details

When a mob tries to accept or post a quest, the pathfinding operation:
1. Loops through **ALL 451 active quests**
2. For each quest, loops through **ALL characters in the game**
3. For each matching questmaster, runs **BFS pathfinding** (can scan thousands of rooms)
4. Can take **30+ milliseconds per search** - blocking the entire single-threaded server

With the server at quest capacity (451/450), mobs were still attempting these searches, causing periodic lag spikes every time a mob near the player tried to interact with the quest system.

## Fixes Implemented

### 1. Early Exit Before Pathfinding ✅
**Location:** `src/mobact.c`

Added checks to prevent expensive pathfinding when at or near quest capacity:
- `mob_process_wishlist_goals()`: Checks quest limit BEFORE searching for questmasters
- `mob_try_to_accept_quest()`: Prevents searches when at 90% capacity (405 quests)

This eliminates the lag source by stopping mobs from triggering the expensive operation.

### 2. Performance Logging ✅
**Location:** `src/utils.c`, `src/spec_procs.c`

Added detailed performance logging (>30ms threshold) to help identify expensive operations:
- `find_accessible_questmaster_in_zone()`: Logs search time, quest count, character count, pathfinding calls
- `calculate_economy_stats()`: Logs time to load all player files (>100ms threshold)
- `calculate_qp_exchange_base_rate()`: Logs time to calculate QP exchange rate

Check `syslog` for PERFORMANCE messages to monitor these operations.

### 3. Caching for Economy Stats ✅
**Location:** `src/utils.c`

Added 30-minute cache to `calculate_economy_stats()` to prevent repeated loading of 420 player files.
This prevents lag if an admin repeatedly uses the `show stats` command.

### 4. Configurable Quest Limit ✅
**Location:** `src/cedit.c`, `src/db.c`, `src/structs.h`

Made `max_mob_posted_quests` configurable via cedit:
- **Access:** `cedit → Experimental Features → Option 9`
- **Range:** 100-1000 quests  
- **Default:** 450 (original value)
- **Saved to:** `lib/etc/config`

## How to Use

### Immediate Action
The fixes are active immediately after the code is compiled and the MUD is restarted. No configuration changes are required.

### Monitoring
Monitor `syslog` for PERFORMANCE messages like:
```
PERFORMANCE: find_accessible_questmaster_in_zone() took 45ms (quests:451 chars:87 pathfinding:12) - mob <name> in zone 50
```

If you see frequent messages (more than a few per minute), consider:
1. Reducing `max_mob_posted_quests` to lower the threshold at which mobs stop trying to post quests
2. Investigating why so many quests are active (quest cleanup issues?)

### Adjusting the Quest Limit
If needed, adjust the limit via cedit:
1. Log in as an immortal with cedit access
2. Type: `cedit`
3. Select: `4) Experimental Features`
4. Select: `9) Max Mob-Posted Quests`
5. Enter new value (100-1000)
6. Type: `0` to exit and save

**Recommended values:**
- **450** (default): Good for most servers
- **300-400**: If you see frequent PERFORMANCE messages or have fewer questmasters
- **500-600**: If you have many questmasters and want more dynamic quests

### Verification
After applying the fix:
1. Ask the affected player to test in the same area where lag occurred
2. Monitor `syslog` for PERFORMANCE messages
3. Check `show stats` - if quest count is near the limit, mobs will stop searching for questmasters

## Technical Details

### Why 90% Threshold?
Mobs stop trying to accept quests at 90% capacity (e.g., 405/450) to provide a buffer:
- Allows existing quests to complete naturally
- Prevents the thundering herd problem (all mobs searching at once when limit is reached)
- Maintains quest system functionality while preventing lag

### Why Pathfinding is Expensive
The BFS pathfinding algorithm used by `find_first_step()`:
- Explores rooms breadth-first to find shortest path
- Can scan thousands of rooms for distant questmasters
- Single-threaded operation blocks all players
- Gets called for EACH questmaster in EACH quest during the search

### Performance Impact
With the fixes:
- **Before:** 30+ ms lag spikes when mobs search for questmasters
- **After:** 0 ms - mobs don't search when at capacity
- **Economy stats:** First call takes time, subsequent calls use 30-min cache
- **Overall:** Eliminates periodic lag caused by quest pathfinding

## Related Files Modified
- `src/mobact.c` - Early exits before pathfinding
- `src/utils.c` - Performance logging, caching, config usage
- `src/spec_procs.c` - Performance logging for QP exchange
- `src/cedit.c` - UI for configurable quest limit
- `src/db.c` - Load/save config, default value
- `src/structs.h` - Config struct addition
- `src/oasis.h` - Cedit mode definition
- `src/utils.h` - Config macro definition

## Next Steps for Admin

1. **Test the fix:** Ask the affected player to test in the area where lag occurred
2. **Monitor logs:** Watch `syslog` for PERFORMANCE messages over 24 hours
3. **Adjust if needed:** If you still see issues, lower `max_mob_posted_quests` via cedit
4. **Quest cleanup:** Consider implementing quest cleanup for old/completed mob-posted quests

## Additional Diagnostics

If lag persists after this fix, use the diagnostic guides:
- `PLAYER_LAG_DIAGNOSIS.md` - Technical analysis of other potential causes
- `docs/LAG_DIAGNOSTIC_COMMANDS.md` - Admin commands for systematic diagnosis

The performance logging will help identify if there are other expensive operations causing lag.
