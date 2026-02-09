# Testing Guide: Shadow Timeline Sentinel Fix

This guide provides step-by-step instructions for testing the Shadow Timeline sentinel compatibility fix.

## Prerequisites

1. Build the MUD server with the latest changes:
   ```bash
   cd vitalia-reborn
   ./configure
   cd src
   make clean && make
   ```

2. Ensure you have access to the MUD as an immortal (level 34+) for testing

## Test Scenario 1: Sentinel Stays at Post

**Objective:** Verify that a sentinel with Shadow Timeline stays at their designated post.

### Setup:
1. Create or select a sentinel mob with both flags:
   ```
   MOB_SENTINEL MOB_SHADOWTIMELINE
   ```
2. Set the guard_post to a specific room VNUM
3. Load the mob at their guard post

### Test Steps:
1. Watch the mob for several game ticks (at least 20-30 ticks)
2. Note if the mob moves from their post

### Expected Results:
- ✅ Mob should stay at post >90% of the time
- ✅ When mob acts, it should primarily choose SHADOW_ACTION_GUARD
- ✅ If mob moves, it should be for compelling reasons (nearby threats, etc.)

### Failure Indicators:
- ❌ Mob wanders away without reason
- ❌ Mob leaves and doesn't return
- ❌ Mob never chooses GUARD action

## Test Scenario 2: Sentinel Returns to Post

**Objective:** Verify that a sentinel away from their post returns.

### Setup:
1. Use the same sentinel mob from Test 1
2. Manually teleport the mob away from their post:
   ```
   at <room_vnum> trans <mob_name>
   ```
3. Choose a room that is 3-5 rooms away but reachable

### Test Steps:
1. Watch the mob's movement
2. Count how many ticks it takes to return
3. Verify the path taken is reasonable

### Expected Results:
- ✅ Mob should consistently move toward their post
- ✅ Mob should reach post within reasonable time (based on distance)
- ✅ Mob should take efficient path (not random wandering)

### Failure Indicators:
- ❌ Mob wanders randomly instead of pathfinding
- ❌ Mob moves away from post
- ❌ Mob gets stuck or stops moving

## Test Scenario 3: Sentinel Combat Response

**Objective:** Verify sentinel responds to threats and returns to post after.

### Setup:
1. Use the same sentinel mob at their post
2. Prepare to attack the mob

### Test Steps:
1. Attack the sentinel
2. Observe combat behavior
3. Flee from combat (or let combat end)
4. Watch if sentinel returns to post

### Expected Results:
- ✅ Sentinel should respond to attack intelligently
- ✅ After combat, sentinel should return to post
- ✅ Combat decisions should use Shadow Timeline (intelligent choices)

### Failure Indicators:
- ❌ Sentinel flees immediately without fighting
- ❌ Sentinel pursues too far from post
- ❌ Sentinel doesn't return to post after combat

## Test Scenario 4: Quest Override

**Objective:** Verify quest goals can temporarily override guard duty.

### Setup:
1. Give sentinel a quest that requires leaving their post
2. Ensure quest completion requires travel

### Test Steps:
1. Observe if sentinel pursues quest
2. Watch for quest completion
3. Verify return to post after quest completes

### Expected Results:
- ✅ Sentinel should pursue quest when active
- ✅ After quest completion, sentinel should return to post
- ✅ Goal system should properly set GOAL_RETURN_TO_POST

### Failure Indicators:
- ❌ Sentinel refuses to pursue quest
- ❌ Sentinel completes quest but doesn't return
- ❌ System crashes or errors occur

## Test Scenario 5: Unreachable Post

**Objective:** Verify system handles unreachable posts gracefully.

### Setup:
1. Create sentinel with guard_post set to unreachable room
2. Load sentinel in a different area with no path to guard_post

### Test Steps:
1. Load sentinel and watch behavior
2. Verify no crashes or errors
3. Check that mob doesn't freeze or loop infinitely

### Expected Results:
- ✅ No crashes or errors
- ✅ Mob exhibits normal behavior (roaming, etc.)
- ✅ No infinite pathfinding loops
- ✅ System logs should be clean

### Failure Indicators:
- ❌ Server crashes
- ❌ Mob freezes (stops all activity)
- ❌ Infinite pathfinding attempts
- ❌ Error messages in logs

## Performance Test

**Objective:** Verify the changes don't cause performance issues.

### Test Steps:
1. Load multiple (10-20) sentinel mobs with Shadow Timeline
2. Monitor server CPU and memory usage
3. Check game tick timing
4. Review system logs for warnings

### Expected Results:
- ✅ CPU usage remains reasonable
- ✅ No memory leaks
- ✅ Game ticks complete in normal time
- ✅ No excessive pathfinding calls

### Failure Indicators:
- ❌ CPU usage spikes
- ❌ Memory usage grows continuously
- ❌ Game ticks become slow
- ❌ Excessive log messages

## Debug Commands

Useful immortal commands for testing:

```
stat mob <mob_name>          # Check mob stats and flags
at <room> look               # Check specific room
trans <mob> <room>           # Teleport mob to room
mload <mob_vnum>             # Load mob for testing
purge <mob_name>             # Remove mob
syslog all                   # Enable all system logs
```

## Success Criteria

The fix is successful if:
- ✅ All 5 test scenarios pass
- ✅ Performance test shows no issues
- ✅ No crashes or errors occur
- ✅ System logs are clean
- ✅ Sentinels maintain posts effectively
- ✅ Shadow Timeline cognitive features still work

## Failure Handling

If any test fails:
1. Document the exact failure scenario
2. Check system logs for errors
3. Review relevant code section
4. Create detailed bug report
5. Retest after fixes

## Reporting Results

When testing is complete, report:
1. Which scenarios were tested
2. Pass/fail status for each
3. Any unexpected behaviors
4. Performance observations
5. Suggestions for improvements

## Additional Notes

- Test with different mob levels and genetics
- Test in various zone types and configurations
- Test with mobs that have other special procedures
- Test interaction with other AI systems (grouping, quests, etc.)

---

**Testing Completed By:** _______________  
**Date:** _______________  
**Overall Result:** ⬜ Pass / ⬜ Fail  
**Notes:** _______________________________________________
