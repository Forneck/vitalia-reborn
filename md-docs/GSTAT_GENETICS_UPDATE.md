# GSTAT and Genetics Update

## Summary
This update adds support for the `healing` (also known as `bandage`) gene tendency to the `gstats` command and verifies that the `adventurer` gene tendency is working correctly for quest acceptance.

## Changes Made

### 1. Added Healing/Bandage Tendency to GSTATS Command

**Files Modified:**
- `src/act.wizard.c` - Added healing gene support to gstats command
- `lib/text/help/help.hlp` - Updated help documentation

**Changes in act.wizard.c:**
- Added "healing" and "bandage" as valid gene name options in usage messages (lines ~5537, 5547, 5557, 5587)
- Added healing tendency name mapping: `else if (!str_cmp(gene_arg, "healing") || !str_cmp(gene_arg, "bandage"))` (line ~5585)
- Added healing_tendency extraction in data collection loop (line ~5667)

### 2. Verified Adventurer Tendency Functionality

The adventurer tendency gene is **already working correctly** for quest acceptance:

**Quest Capability Calculation (`src/quest.c:1349`):**
```c
/* Base capability from quest genetics */
capability = GET_GENQUEST(mob) + GET_GENADVENTURER(mob);
```

**Quest Seeking Behavior (`src/mobact.c`):**
- Mobs with `GET_GENQUEST(ch) > 30 && GET_GENADVENTURER(ch) > 20` will actively look for quests
- Mobs with `GET_GENADVENTURER(ch) > 50` are more likely to post exploration quests (30% chance)

**Quest Success/Failure (`src/quest.c`):**
- Exploration quests (AQ_OBJ_FIND, AQ_ROOM_FIND, AQ_MOB_FIND) increase adventurer_tendency on success
- Exploration quest failures decrease adventurer_tendency
- This creates a feedback loop that evolves mob behavior over time

## Genetic System Overview

The genetics system includes these tendencies (all range 0-100):

1. **wimpy_tendency** - Tendency to flee from combat
2. **loot_tendency** - Tendency to loot items
3. **equip_tendency** - Tendency to equip items
4. **roam_tendency** - Tendency to move around/explore
5. **brave_prevalence** - Tendency to be brave in combat
6. **group_tendency** - Tendency to form/join groups
7. **use_tendency** - Tendency to use items/skills
8. **trade_tendency** - Tendency to engage in trade
9. **quest_tendency** - Tendency to accept quests
10. **adventurer_tendency** - Tendency to explore and go on adventures
11. **follow_tendency** - Tendency to follow other characters
12. **healing_tendency** - **[NEW IN GSTATS]** Tendency to heal allies with bandage skill

All genes are displayed in the `stat` command for NPCs, but only now is `healing_tendency` included in the statistical analysis available through `gstats`.

## How to Use GSTATS with Healing Gene

### Command Syntax:
```
gstats <target> healing
gstats <target> bandage    (alias for healing)
```

### Examples:
```
gstats all healing           - Analyze healing tendency across all mobs
gstats zone 30 healing       - Analyze healing tendency in zone 30
gstats cleric healing        - Analyze healing tendency for specific mob
gstats 3001 bandage          - Same as healing, using mob vnum
```

### Output Format:
The command displays:
- Mean, median, and standard deviation
- Minimum and maximum values
- Distribution histogram (0-100 range in 10-point bins)
- Sample size

## Testing Instructions

### Test 1: Verify Healing Gene in GSTATS
1. Start the MUD server
2. Log in with an immortal character (level >= LVL_IMMORT)
3. Run: `gstats all healing`
4. Verify the command executes without errors
5. Check that statistics are displayed for mobs with healing_tendency > 0

### Test 2: Verify Adventurer Gene Still Works
1. Run: `gstats all adventurer`
2. Verify statistics are displayed correctly
3. Check a specific mob: `stat <mob_name>`
4. Verify "Tendência Aventureiro (Genética)" is displayed with a value

### Test 3: Verify Help Documentation
1. Run: `help gstats`
2. Verify "healing" is listed in the available genes
3. Verify the example includes healing gene usage

### Test 4: Verify Quest Acceptance (Adventurer Gene)
To verify adventurer tendency affects quest acceptance:
1. Create or find a mob with high adventurer_tendency (>50) and quest_tendency (>30)
2. Observe the mob's behavior over time (it should actively seek quests)
3. Check with `stat <mob>` to see if it has accepted a quest
4. Mobs with low adventurer_tendency (<20) should rarely seek quests

## Implementation Notes

### Why Both "healing" and "bandage"?
The gene is internally called `healing_tendency` but represents the tendency to use the bandage skill. Both command names are accepted for user convenience:
- "healing" - matches the internal field name
- "bandage" - matches the actual skill name users are familiar with

### Backward Compatibility
- All existing genes continue to work exactly as before
- The adventurer gene was already functional and required no code changes
- No changes to mob file format or data structures were needed

### Code Quality
- All changes follow existing code patterns in act.wizard.c
- Code formatted with clang-format according to project standards
- Build completes successfully with no new warnings or errors

## Related Code Files

- `src/act.wizard.c` - GSTATS command implementation
- `src/quest.c` - Quest capability and adventurer tendency usage
- `src/mobact.c` - Mob AI including quest seeking and healing behavior
- `src/structs.h` - mob_genetics structure definition (line ~struct mob_genetics)
- `src/utils.h` - GET_GENHEALING and GET_GENADVENTURER macros
- `src/genmob.c` - Mob file I/O for genetics data
- `lib/text/help/help.hlp` - Help documentation

## Verification Checklist

- [x] Code builds successfully without errors
- [x] Code formatted with clang-format
- [x] Help documentation updated
- [x] All gene options listed consistently in all usage messages
- [x] Healing gene extraction implemented correctly
- [x] Adventurer gene verified as working correctly
- [x] No breaking changes to existing functionality
