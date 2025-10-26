# Escort Quests Feature

## Overview

Escort quests (AQ_MOB_ESCORT) are a new type of quest where players must safely escort an NPC from the questmaster's location to a destination room. The escort NPC follows the player without joining their group, and the quest fails if the escort dies.

## Quest Type

- **Type ID**: 9 (AQ_MOB_ESCORT)
- **Type Name**: "Escort mob"

## How It Works

### 1. Quest Acceptance

When a player accepts an escort quest at a questmaster:
1. The quest is added to the player's active quest
2. An escort NPC (specified by the quest's target field) is spawned in the same room
3. The escort NPC automatically begins following the player
4. The escort NPC is marked with `MOB_NOKILL` flag to prevent attacks
5. All aggressive flags are removed from the escort NPC

### 2. During Escort

- The escort NPC follows the player as they move through the world
- The escort does NOT join the player's group
- The escort cannot be attacked due to the `MOB_NOKILL` flag
- The player must guide the escort to the destination room

### 3. Quest Completion

The quest completes when:
- The player enters the destination room (specified in quest's `value[5]` field)
- The escort NPC is alive and in the same room
- The escort NPC thanks the player with a Portuguese message
- The player receives quest rewards (quest points, gold, exp, items)

### 4. Quest Failure

The quest fails if:
- The escort NPC dies (by any means)
- The player receives a failure notification
- The quest is removed from the player's active quests
- No rewards are given

## Quest File Format

### Field Assignments

When creating an escort quest in a `.qst` file:

```
#<quest_vnum>
<name>~
<description>~
<info_message>~
<completion_message>~
<quit_message>~
<type> <questmaster_vnum> <flags> <target_mob_vnum> <prev_quest> <next_quest> <prereq_obj>
<points> <penalty> <min_level> <max_level> <time_limit> <destination_room_vnum> <quantity>
<gold_reward> <exp_reward> <obj_reward>
S
```

**Important Fields for Escort Quests:**

- `type`: Must be **9** (AQ_MOB_ESCORT)
- `target_mob_vnum`: The vnum of the NPC to escort (field 4 in line 2)
- `destination_room_vnum`: The vnum of the destination room (field 6 in line 3, or `value[5]`)

### Example Quest File

```
#5001
Escolta para a Cidade~
Um mercador precisa de escolta até a cidade~
Um mercador nervoso se aproxima:
'Por favor, me escolte até a cidade! Tenho medo de viajar sozinho.
Os bandidos têm atacado viajantes na estrada.'~
Muito obrigado por me escoltar com segurança! Aqui está sua recompensa.~
Sem escolta, não posso viajar. A busca foi abandonada.~
9 3010 a 3050 -1 -1 -1
100 0 5 15 30 3100 1
500 1000 -1
S
```

In this example:
- Quest type is 9 (escort)
- Questmaster is mob 3010
- Escort mob to spawn is mob 3050 (merchant)
- Destination room is 3100 (city)
- Player gets 100 quest points, 500 gold, 1000 exp
- Time limit is 30 ticks
- For levels 5-15

## Quest Value Fields

The `value[]` array in the quest structure is used as follows:

- `value[0]` (QST_POINTS): Quest points reward
- `value[1]` (QST_PENALTY): Quest points penalty for abandoning
- `value[2]` (QST_MINLEVEL): Minimum player level
- `value[3]` (QST_MAXLEVEL): Maximum player level
- `value[4]` (QST_TIME): Time limit in ticks (-1 for unlimited)
- **`value[5]` (QST_RETURNMOB)**: **Destination room vnum for escort quests**
- `value[6]` (QST_QUANTITY): Not used for escort quests (set to 1)

## Technical Details

### Data Structures

**Player Data:**
- `escort_mob_id` (long): Stores the IDNUM of the escort mob being tracked
- Persisted in player file with tag "Qesc:"

**Mob Flags Set:**
- `MOB_NOKILL`: Prevents the escort from being attacked
- Aggressive flags removed: `MOB_AGGRESSIVE`, `MOB_AGGR_EVIL`, `MOB_AGGR_GOOD`, `MOB_AGGR_NEUTRAL`

### Key Functions

**`spawn_escort_mob(ch, rnum)`**
- Creates the escort NPC instance
- Configures it to follow the player
- Sets protective flags
- Returns TRUE on success

**`check_escort_quest_completion(ch, rnum)`**
- Called when player enters a room
- Checks if destination reached with escort alive
- Triggers completion message and rewards
- Returns TRUE if quest completed

**`fail_escort_quest(escort_mob, killer)`**
- Called when escort mob dies
- Finds the player escorting the mob
- Notifies player of failure
- Clears the quest without rewards

### Integration Points

1. **Quest Joining** (`quest_join_unified`, `quest_join_temp`)
   - Spawns escort mob after quest acceptance

2. **Room Entry** (`char_to_room` in `handler.c`)
   - Checks for escort quest completion

3. **Mob Death** (`extract_char_final` in `handler.c`)
   - Fails escort quest if escort dies

4. **Player Save/Load** (`players.c`)
   - Persists escort_mob_id across logins

## Design Decisions

### Why Not Use Groups?

The escort NPC follows but doesn't join the group because:
- Allows for different game mechanics (escort can't fight)
- Prevents exploit of using escort in combat
- Makes the quest more challenging (protecting a non-combatant)

### Why MOB_NOKILL Flag?

The escort is protected with `MOB_NOKILL` to:
- Prevent accidental quest failure from monster attacks
- Focus the challenge on navigation, not combat
- Match the narrative (escorting someone who can't defend themselves)

### Why Track by IDNUM?

Using `GET_IDNUM(mob)` instead of a pointer because:
- Survives across zone resets
- Prevents dangling pointer issues
- Allows finding the mob anywhere in the world

## Future Enhancements

Possible improvements for future versions:

1. **Multiple Checkpoints**: Support multiple waypoints along the route
2. **Time-based Events**: Random encounters during escort
3. **Escort Dialogue**: Periodic comments from the escort NPC
4. **Variable Difficulty**: Different escort behaviors (fast/slow, scared/brave)
5. **Group Escort**: Allow multiple players to share the same escort quest
6. **Escort AI**: Make escorts avoid dangerous areas or react to threats

## Troubleshooting

**Quest not spawning escort mob:**
- Check that target mob vnum exists in mob files
- Verify quest type is set to 9
- Check server logs for spawn errors

**Quest not completing at destination:**
- Verify destination room vnum in value[5] is correct
- Ensure escort mob is following and alive
- Check that player and escort are in the same room

**Escort died unexpectedly:**
- Check if MOB_NOKILL was somehow removed
- Verify no script or trigger is killing the mob
- Check for death traps in the escort path

## Related Files

- `src/quest.h` - Quest type definitions
- `src/quest.c` - Quest logic implementation
- `src/handler.c` - Room entry and death detection
- `src/players.c` - Player data persistence
- `src/structs.h` - Data structure definitions
- `src/utils.h` - Macro definitions

## Version History

- **v1.0** (2025-10-26): Initial implementation
  - Basic escort quest functionality
  - Mob following without grouping
  - Death detection and quest failure
  - Destination detection and completion
  - Player data persistence
