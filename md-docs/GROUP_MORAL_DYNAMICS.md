# Group Moral Dynamics

**Version:** 1.0  
**Status:** Production Ready  
**Integration Date:** 2026-02-17  
**Dependencies:** Memory-Based Moral Learning (v1.2), Shadow Timeline, Emotion System

## Overview

The Group Moral Dynamics system extends individual moral reasoning to handle collective decision-making, peer pressure, and group reputation. It enables mobs in groups to exhibit realistic social moral behavior including conformity, dissent, and collective responsibility.

## Theory Background

### Social Moral Psychology

Group moral dynamics are based on principles from social psychology:

1. **Peer Pressure**: Individuals modify behavior to align with group norms
2. **Collective Responsibility**: Groups share moral accountability for actions
3. **Moral Conformity**: Members conform to group moral standards
4. **Dissent**: Strong individual convictions can overcome group pressure
5. **Group Reputation**: Collective moral standing affects inter-group relations

## Architecture

### Core Components

#### 1. Group Moral Reputation (`group_data`)

```c
struct group_data {
    struct char_data *leader;
    struct list_data *members;
    int group_flags;

    /* Group moral dynamics */
    int moral_reputation;        /* 0-100: group's moral standing */
    int collective_guilt_count;  /* Number of guilty group actions */
    int collective_good_count;   /* Number of moral group actions */
    time_t last_moral_action;    /* Timestamp of last group action */
};
```

**Moral Reputation Scale:**
- **70-100**: Highly moral group, respected by good-aligned entities
- **50-69**: Moderately moral, neutral reputation
- **30-49**: Questionable morals, cautiously viewed
- **0-29**: Immoral group, avoided or attacked by good groups

#### 2. Peer Pressure Calculation

```c
int moral_get_peer_pressure(struct char_data *ch, int action_type);
```

**Factors Influencing Peer Pressure:**
- Group members' alignments
- Members' emotions (compassion, courage, loyalty)
- Members' learned moral biases
- Group size (larger groups = more pressure)
- Leader influence

**Pressure Direction:**
- **Positive (+)**: Group encourages action
- **Negative (-)**: Group discourages action
- **Range**: -100 to +100

#### 3. Leader Influence

```c
int moral_get_leader_influence(struct char_data *follower, struct char_data *leader, int action_type);
```

**Leadership Factors:**
- Level difference (higher level = more influence)
- Follower's loyalty emotion
- Follower's emotional intelligence (high EI = more independent)
- Leader's reputation
- Leader's alignment (affects moral direction)

**Influence Range**: -50 to +50

#### 4. Moral Dissent

```c
bool moral_would_dissent_from_group(struct char_data *ch, int group_action_cost, int action_type,
                                   struct char_data *victim);
```

**Dissent Triggers:**
- Individual strongly opposes (cost < -60) but group encourages (cost > 20)
- High compassion (>70) + good alignment (>500)
- Learned avoidance for action type
- High emotional intelligence (>80)
- Low loyalty (<30) + significant cost difference (>50)

#### 5. Collective Responsibility

```c
bool moral_evaluate_group_action(struct char_data *leader, struct char_data *victim, int action_type,
                                struct moral_judgment *judgment);

void moral_record_group_action(struct group_data *group, struct char_data *victim, int action_type,
                              struct moral_judgment *judgment);
```

**Group Action Processing:**
1. Evaluate moral judgment (same as individual)
2. Record in ALL group members' moral memories
3. Update group moral reputation
4. Adjust individual alignments (collective responsibility)
5. Update leader's reputation

## Integration with Moral Evaluation

Group dynamics are automatically integrated into `moral_evaluate_action_cost()`:

```c
/* Individual moral evaluation */
int moral_cost = [base evaluation];

/* Apply group dynamics */
if (actor->group && actor->group->members) {
    /* Peer pressure */
    int peer_pressure = moral_get_peer_pressure(actor, action_type);
    moral_cost += peer_pressure;
    
    /* Group reputation modifier */
    int reputation_mod = moral_get_group_reputation_modifier(actor->group, victim->group);
    moral_cost += reputation_mod;
    
    /* Check for dissent */
    if (moral_would_dissent_from_group(actor, group_cost, action_type, victim)) {
        /* Reduce peer pressure influence by 50% */
        moral_cost -= peer_pressure / 2;
    }
}
```

## Behavioral Scenarios

### Scenario 1: Peer Pressure Overcomes Individual Morality

**Setup:**
- Group of 5 evil-aligned bandits (reputation: 25)
- One neutral member with high compassion (75)

**Situation:** Group decides to attack innocent travelers

**Individual Evaluation:**
- Compassionate member's cost: -80 (strongly opposes)

**Group Dynamics:**
- Peer pressure: +45 (4 evil members encourage)
- Leader influence: +15 (evil leader, loyal follower)
- Final cost: -80 + 45 + 15 = -20

**Outcome:** Member still opposes but less strongly; may reluctantly participate

### Scenario 2: Strong Dissent Prevents Conformity

**Setup:**
- Same group, but compassionate member has:
  - High EI (85)
  - Learned avoidance for attacking innocents (3 previous guilty judgments)
  - Lower loyalty (25)

**Group Dynamics:**
- Peer pressure: +45
- Individual cost: -80
- Dissent detected: TRUE (learned avoidance + low loyalty)
- Peer pressure reduced: +45 → +22 (50% reduction)
- Final cost: -80 + 22 = -58

**Outcome:** Member refuses to participate, may leave group

### Scenario 3: Good Group Reputation Prevents Inter-Group Conflict

**Setup:**
- Good-aligned paladin group (reputation: 85)
- Encounters another good group (reputation: 75)

**Group Dynamics:**
- Both groups have high moral reputation
- Reputation modifier: +20 (strong positive to avoid good-vs-good)
- Individual moral cost for attack: -60
- Final cost: -60 + 20 = -40

**Outcome:** Attack strongly discouraged, groups may cooperate instead

### Scenario 4: Collective Responsibility Affects All Members

**Setup:**
- Mixed-alignment group (neutral leader, 3 good members, 1 evil)
- Leader orders attack on fleeing enemy

**Action:**
- Leader executes attack
- Moral judgment: GUILTY (attacking fleeing enemy)

**Collective Responsibility:**
1. ALL 5 members receive judgment in moral memory
2. Group reputation drops: 50 → 42 (-8)
3. All members' alignments shift toward evil slightly
4. Leader's personal reputation affected most

**Long-term Effect:**
- Good members may develop learned avoidance
- Group moral conflicts increase
- Potential group dissolution

### Scenario 5: Leader Influence Overrides Low-EI Follower

**Setup:**
- Charismatic evil leader (level 20, reputation: 70)
- Low-EI follower (EI: 25, high loyalty: 85, level 10)

**Leader Influence Calculation:**
- Level difference: +20 (10 levels)
- High loyalty: +15
- Low EI: +30% amplification = total +45
- Leader alignment: evil, action is ATTACK → +45 (encouraging)

**Outcome:**
- Follower's individual opposition (-30) completely overridden
- Final cost: -30 + 45 = +15 (now encourages action)
- Follower conforms to leader's immoral decision

## Group Reputation Effects

### Reputation Building

**Actions that Increase Reputation:**
- Helping other groups (+5 to +15 per action)
- Defending innocents (+10 to +20)
- Consistent moral behavior (bonus after 5+ good actions)

**Actions that Decrease Reputation:**
- Attacking good-aligned groups (-10 to -20)
- Betrayal (-15 to -30)
- Repeated immoral actions (penalty after 5+ guilty actions)

### Reputation Impact on Interactions

**High Reputation (70-100):**
- Good groups more likely to cooperate
- Reduced hostility from neutrals
- Easier quest acceptance from NPCs
- +10 to +20 bonus for helpful actions

**Low Reputation (0-29):**
- Good groups more likely to attack
- Evil groups may see as targets (weak)
- NPCs refuse interactions
- -10 to -20 penalty for harmful actions

## Memory System Integration

Group actions are recorded in emotion memory with moral judgment data:

```
Emotion Memory:
  [2 min ago] Player#12345 Attacked           [MAJOR]
      Emotions: Fear:30 Anger:40 Happy:20 Sad:30 Shame:60
      Moral: Attack, Guilty, Blame:75, Severity:60, Regret:70
      [GROUP ACTION - Collective Responsibility]
```

**Effect on Learning:**
- All group members learn from collective action
- Individual moral memories influenced by group outcome
- Repeated group actions strengthen learning
- Members may develop different reactions based on individual traits

## Performance Considerations

**Peer Pressure Calculation:**
- O(n) where n = group size
- Typically 2-6 members = negligible impact
- Cached during Shadow Timeline projection

**Memory Overhead:**
- +16 bytes per group (moral reputation fields)
- +4 bytes per group action recorded
- For 100 active groups: ~2 KB additional memory

**CPU Impact:**
- Peer pressure: ~0.05ms per calculation
- Group evaluation: ~0.1ms per group action
- Total impact: < 0.2ms per grouped mob decision

## Usage Examples

### Example 1: Creating a Moral Group

```c
/* Create group with good leader */
struct char_data *leader = load_mobile(PALADIN_VNUM);
struct group_data *group = create_group(leader);

/* Group reputation initialized based on leader */
// group->moral_reputation = 70 (good leader bonus)

/* Add members */
struct char_data *member1 = load_mobile(CLERIC_VNUM);
struct char_data *member2 = load_mobile(WARRIOR_VNUM);
join_group(member1, group);
join_group(member2, group);
```

### Example 2: Recording Group Action

```c
/* Group attacks enemy */
struct char_data *leader = group->leader;
struct char_data *victim = target;

/* Evaluate and record group action */
struct moral_judgment judgment;
if (moral_evaluate_group_action(leader, victim, MORAL_ACTION_ATTACK, &judgment)) {
    /* Record in all members' memories */
    moral_record_group_action(group, victim, MORAL_ACTION_ATTACK, &judgment);
}
```

### Example 3: Checking for Dissent

```c
/* Before executing group action */
for (member in group->members) {
    int group_cost = moral_get_peer_pressure(member, MORAL_ACTION_ATTACK);
    
    if (moral_would_dissent_from_group(member, group_cost, MORAL_ACTION_ATTACK, victim)) {
        /* Member refuses to participate */
        send_to_char(member, "You refuse to attack innocent civilians!\r\n");
        
        /* May leave group if loyalty is low */
        if (member->ai_data->emotion_loyalty < 20) {
            stop_follower(member);
        }
    }
}
```

### Example 4: Inter-Group Dynamics

```c
/* Two groups encounter each other */
struct group_data *good_group = paladin_group; // reputation: 80
struct group_data *evil_group = bandit_group;  // reputation: 25

/* Evaluate interaction */
int reputation_mod = moral_get_group_reputation_modifier(evil_group, good_group);
// Returns: -15 (evil group encouraged to attack good group)

reputation_mod = moral_get_group_reputation_modifier(good_group, evil_group);
// Returns: -10 (good group less opposed to attacking evil group)
```

## Debugging & Display

### Stat Command Output

```
Master is: Paladin, Followers are: Cleric, Warrior
Group Moral Reputation: 72/100 (Guilty:1 Good:8) [LEADER]
```

**Interpretation:**
- Reputation 72 = highly moral group
- 1 guilty action (minor transgression)
- 8 good actions (consistent moral behavior)
- [LEADER] indicates this character leads the group

### Debug Commands

```c
/* Check peer pressure for action */
int pressure = moral_get_peer_pressure(mob, MORAL_ACTION_ATTACK);
log("Peer pressure for %s to attack: %d", GET_NAME(mob), pressure);

/* Check leader influence */
int influence = moral_get_leader_influence(follower, leader, MORAL_ACTION_ATTACK);
log("Leader influence on %s: %d", GET_NAME(follower), influence);

/* Check dissent */
if (moral_would_dissent_from_group(mob, group_cost, MORAL_ACTION_ATTACK, victim)) {
    log("%s would dissent from group action", GET_NAME(mob));
}
```

## Advanced Scenarios

### Scenario: Group Moral Evolution

**Week 1:** New bandit group forms
- Initial reputation: 45 (neutral-ish)
- Mixed alignments

**Week 2:** Several attacks on caravans
- 5 guilty group actions recorded
- Reputation drops: 45 → 28
- Good-aligned members develop regret

**Week 3:** Internal conflict
- 2 good members dissent from raid
- Low loyalty + learned avoidance
- Members leave group

**Week 4:** Evil core remains
- Reputation: 22
- Consistent immoral actions
- Other evil groups may ally

### Scenario: Redemption Arc

**Starting Point:** Evil group (reputation: 18)

**Trigger:** New good-aligned leader joins
- Leader influence pulls group toward good

**Process:**
1. Leader refuses immoral actions
2. Members experience peer pressure reversal
3. Group performs helpful actions
4. Reputation gradually increases
5. Evil members dissent and leave

**End State:** Reformed group (reputation: 65)

## Configuration & Tuning

### Peer Pressure Strength

```c
/* In moral_get_peer_pressure() */

/* Current: Group size amplification */
if (member_count >= 5) {
    average_pressure = (average_pressure * 120) / 100; /* 20% boost */
} else if (member_count >= 3) {
    average_pressure = (average_pressure * 110) / 100; /* 10% boost */
}

/* Adjust these multipliers to tune conformity strength */
```

### Leader Influence Limits

```c
/* In moral_get_leader_influence() */

/* Current: Level difference caps at +20 */
influence += MIN(20, MAX(0, level_diff * 2));

/* Increase/decrease MIN value to change leader power */
```

### Dissent Thresholds

```c
/* In moral_would_dissent_from_group() */

/* Current: Strong opposition required */
if (individual_cost < -60 && group_action_cost > 20) {
    /* Check for dissent triggers */
}

/* Adjust -60 and +20 to make dissent easier/harder */
```

## Integration Checklist

- [x] Group reputation initialized on group creation
- [x] Peer pressure calculated during action evaluation
- [x] Leader influence applied to followers
- [x] Dissent detection functional
- [x] Collective responsibility records group actions
- [x] Group reputation updates after actions
- [x] Inter-group reputation modifiers work
- [x] Stat command displays group moral info
- [x] Memory system records group actions
- [x] Shadow Timeline integration complete

## Future Enhancements

1. **Group Moral Identity**
   - Explicit moral codes for groups
   - Custom alignment tendencies
   - Faction-based moral variations

2. **Dynamic Leadership**
   - Leadership challenges based on moral disagreements
   - Reputation-based leadership changes
   - Split groups from moral conflicts

3. **Inter-Group Diplomacy**
   - Alliances based on reputation
   - Reputation-based trade bonuses/penalties
   - Group-wide reputation with NPCs/factions

4. **Moral Momentum**
   - Streak bonuses for consistent behavior
   - Faster reputation changes after 10+ actions
   - Group "moral identity" formation

5. **Player-NPC Group Dynamics**
   - Players affected by NPC group morality
   - Player actions affect group reputation
   - Reputation-based group benefits/penalties

## References

1. Memory-Based Moral Learning (v1.2) - `md-docs/MORAL_REASONING.md`
2. Shadow Timeline System - `md-docs/SHADOW_TIMELINE.md`
3. Emotion System - `md-docs/EMOTION_SYSTEM_TECHNICAL_SUMMARY.md`
4. Group Mechanics - `src/handler.c` (group management functions)

## Files

- `src/structs.h` - Extended group_data with moral reputation
- `src/moral_reasoner.h` - Group moral dynamics API
- `src/moral_reasoner.c` - Implementation
- `src/handler.c` - Group creation initialization
- `src/act.wizard.c` - Stat command display
- `md-docs/GROUP_MORAL_DYNAMICS.md` - This documentation

## License

Part of Vitalia Reborn MUD engine.  
Copyright (C) 2026 Vitalia Reborn Design
