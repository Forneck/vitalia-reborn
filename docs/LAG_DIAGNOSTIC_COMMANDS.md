# Lag Diagnostic Commands for Administrators

This guide provides specific commands to help diagnose the 30-second lag issue.

## Player Information Commands

### Check Player Status
```
stat <player_name>
```
Shows: Level, flags, affects, position, fighting status

### Check Player Location
```
goto <player_name>
stat room
```
Shows: Room vnum, zone, flags, triggers attached

### Check Player Inventory
```
at <player_name> inventory
```
Check for objects with triggers

### Check Player Equipment
```
at <player_name> equipment
```
Check for worn items with triggers

## Zone and Room Diagnostics

### Check Zone Information
```
show zone <zone_number>
```
Shows: Zone commands, reset timings

### Check Room Triggers
When in the player's room:
```
stat room
```
Look for: Triggers (T ####), special procedures

### Check All Triggers in Zone
```
vstat trigger <vnum>
```
For each trigger vnum shown in the room/mobs/objects

## Mob and Object Diagnostics

### Check Mobs in Room
```
stat mob <mob_name>
```
Look for: Special procedures, triggers attached

### Check Objects in Room
```
stat obj <object_name>
```
Look for: Triggers, special flags

## Performance Monitoring

### Show Stats
```
show stats
```
Key metrics to watch:
- `buf_overflows`: Should stay low (currently 11)
- `buf_switches`: High number might indicate output issues (currently 37377)
- Connected players vs in-game players

### System Log Check
Check `syslog` for:
- Trigger errors
- Script warnings
- File I/O operations
- Character save/load messages around the time of lag

## Specific Tests for 30-Second Lag

### Test 1: Location Isolation
1. Note player's current room vnum
2. Teleport player to a known "safe" room (e.g., Temple/Starting room)
3. Ask if lag persists
4. If lag stops → Issue is location-specific (room/zone/mobs)

### Test 2: Inventory Isolation
1. Have player drop all items: `drop all`
2. Have player remove all equipment: `remove all`
3. Ask if lag persists
4. If lag stops → Issue is item/object-specific (triggers on objects)

### Test 3: Affect Isolation
1. Check all affects: `stat <player>`
2. Remove suspicious affects (especially custom/quest ones)
3. Ask if lag persists
4. If lag stops → Issue is affect-related

### Test 4: Trigger Timing
1. Enable script debug mode (if available)
2. Watch for triggers that fire every ~30 seconds
3. Check `syslog` for script execution timing

## Common Lag Causes and Checks

### 1. Random Trigger with Bad Timing
Check for triggers with:
- `w 30` or `w 300` (wait 30 seconds)
- Random triggers that might align to ~30 seconds
- Timer-based triggers

Example problem trigger:
```
Name: 'Example Periodic Check'
Trigger Type: Random or Timer
Actions that run every 30 seconds
```

### 2. Quest Timer
Check active quests on player:
```
show quest <player_name>  (if command exists)
```
Or check player file for quest data

### 3. Special Procedure with Timer
Check `src/spec_assign.c` for:
- Mobs assigned special procedures in player's zone
- Objects with special procedures in player's inventory

### 4. Mob Emotion System
Check if mob emotion system is causing issues:
```
CONFIG_MOB_EMOTION_UPDATE_CHANCE in lib/etc/config
```
Currently runs every 4 seconds, but might interact badly with other systems

### 5. Auto-Quest System
Check if any of the 451 autoquests have timers:
- Check `lib/world/qst/*.qst` files
- Look for timer-based completion checks

## Advanced Diagnostics

### Enable Verbose Logging
Temporarily add logging to track:
1. When `update_auctions()` is called (if enabled)
2. When triggers execute
3. When special procedures run
4. When file I/O occurs

### Code Modification for Testing
Add timing instrumentation:
```c
// In src/comm.c, heartbeat function
struct timeval start, end;
long elapsed_ms;

gettimeofday(&start, NULL);
// ... operation to test ...
gettimeofday(&end, NULL);
elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
if (elapsed_ms > 10) {
    log("PERFORMANCE: Operation took %ld ms", elapsed_ms);
}
```

## Resolution Steps

Once cause is identified:

### If it's a trigger:
1. Edit the trigger file (lib/world/trg/*.trg)
2. Fix the timing or logic issue
3. Reload triggers: `tedit save world`
4. Test with player

### If it's a special procedure:
1. Edit `src/spec_procs.c`
2. Fix the procedure logic
3. Recompile: `cd src && make`
4. Reboot MUD or `reload` command

### If it's a quest:
1. Edit quest file (lib/world/qst/*.qst)
2. Adjust timer or conditions
3. Reload quests
4. Test with player

### If it's configuration:
1. Edit `lib/etc/config`
2. Adjust problematic setting
3. Use `set` command or reboot
4. Test with player

## Preventive Measures

1. **Audit all triggers** for expensive operations
2. **Review special procedures** for periodic execution
3. **Monitor system performance** regularly
4. **Keep logs** of all lag incidents
5. **Test changes** in offline mode first

## Contact Points

If issue persists:
- Check GitHub issues for similar reports
- Review recent code changes (git log)
- Consider temporary workarounds (disable features)
- Document findings for future reference
