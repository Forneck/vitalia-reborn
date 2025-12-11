# Player Lag Diagnosis Guide

This guide helps diagnose periodic lag issues affecting individual players.

## Issue Description
Player reports constant lag approximately every 30 seconds, persisting across:
- Different characters
- Different clients
- Different network connections (including mobile data)
- Combat and non-combat situations

## Potential Causes and Diagnostics

### 1. Server-Wide Issues (affects all players)
- **Auction System**: Currently disabled (`CONFIG_NEW_AUCTION_SYSTEM = 0`), but if enabled, check for:
  - Active auctions ending frequently
  - File I/O operations for offline player gold/item delivery
  - **Fix**: Early exit added in `update_auctions()` when no auctions exist

### 2. Player-Specific Issues (affects only one player)

#### A. DG Script Triggers
Triggers can run periodically and may cause lag if they perform expensive operations.

**Check for:**
- Mob triggers with random or greet types in the player's area
- Object triggers on items the player is carrying/wearing
- Room triggers in the player's location with periodic execution

**Diagnostic Commands:**
```
stat <player>           # Check player flags and stats
stat room               # Check room triggers
stat zone               # Check zone settings
```

**Files to Check:**
- `lib/world/trg/*.trg` - All trigger scripts
- Look for triggers with timing or random activation

#### B. Affects and Timers
Check for affects with ~30 second durations that might be reapplying or causing issues.

**Diagnostic Commands:**
```
stat <player>           # Shows all affects on the player
set file <player>       # View player file directly
```

#### C. Quest Timers
Check if the player has any quests with periodic checks or timers.

**Files to Check:**
- `lib/world/qst/*.qst` - Quest definitions
- Player quest state in player file

#### D. Auto-Quests
The server has 451 autoquests. Check if any are causing periodic checks.

**Check:**
- Quest trigger conditions
- Periodic quest state updates

#### E. Special Procedures
Check for special procedures (spec_procs) attached to:
- Mobs in the player's area
- Objects the player is carrying
- The room the player is in

**Files to Check:**
- `src/spec_procs.c` - All special procedures
- `src/spec_assign.c` - Special procedure assignments

#### F. Player Location/Room
Check if the issue is location-specific:
1. Ask player to move to different zones
2. Check if lag persists or stops in different locations
3. Check room triggers and special features in affected rooms

**Diagnostic Commands:**
```
goto <player>           # Teleport to player's location
stat room               # Check room properties
```

### 3. Network/Connection Issues

#### A. MSDP/GMCP/MXP Protocol
Check if the player's client is using MSDP or other protocols that send periodic updates.

**Location in code:**
- `src/comm.c:1045` - `msdp_update()` runs every second
- Check if MSDP updates are causing issues

#### B. Client Output Buffer
Check if the player's output buffer is filling up.

**Check:**
- Large output from commands
- Spam from combat or triggers
- Buffer overflow indicated in show stats: "11 overflows"

### 4. Recommended Diagnostic Steps

1. **Identify Location**: Ask player to note exact room VNUM when lag occurs
2. **Check Triggers**: Use `stat room` to check for triggers in that location
3. **Check Inventory**: Ask player to drop all items and test if lag persists
4. **Check Affects**: Use `stat <player>` to see all active affects
5. **Monitor Logs**: Check `syslog` and `log/syslog.*` during lag incidents
6. **Test Different Zones**: Have player move to starter zones to isolate issue
7. **Check Mob Special Procs**: Look for mobs with special procedures in player's area

### 5. Code Additions for Future Diagnosis

Consider adding:
- Performance timing logs for expensive operations
- Player-specific lag tracking
- Trigger execution time monitoring
- Option to disable specific systems per-player for testing

### 6. Emergency Mitigation

If lag is severe and affecting gameplay:
1. Disable problematic triggers temporarily
2. Move player to a "safe" room with minimal triggers/mobs
3. Check for stuck events: `ch->events` list
4. Consider temporary ban of specific game features for that player

## Performance Monitoring

Check `show stats` output for:
- Overflow count (currently 11) - indicates output buffer issues
- Large number of active triggers (currently 205)
- Number of groups pending cleanup (currently 0)
- Buffer switches (37377) - might indicate excessive output

## Resolution

Once cause is identified:
1. Fix the underlying trigger/script/procedure
2. Add safeguards to prevent similar issues
3. Test thoroughly in the problematic location
4. Monitor logs after fix is deployed
