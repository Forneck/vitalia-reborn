# CEDIT Emotion Configuration Guide

**For Game Administrators**  
**Version:** 1.0  
**Last Updated:** 2026-02-14

---

## Quick Start

To access emotion configuration in-game:

```
> cedit
[Main cedit menu appears]
> [Select emotion menu option]
```

The emotion configuration system is accessible through **cedit** (Config Editor), allowing real-time adjustment of 120+ emotion parameters without server restart.

---

## Menu Structure

### Main Emotion Menu

```
Emotion Configuration Menu:
-- Emotion System Configuration --
1) Decay System Configuration
2) Display Thresholds (Visual Indicators)
3) Combat Flee Behavior
4) Pain System Configuration
5) Memory System Configuration
6) Trade & Quest Configuration
7) Social Behavior Configuration
8) Group Behavior Configuration
9) Combat Effects Configuration
A) Weather & Climate Configuration
P) Load Preset Configuration
Q) Return to main menu

Enter choice:
```

---

## Configuration Categories

### 1. Decay System Configuration

**Purpose:** Controls how emotions naturally decrease over time toward baselines

**Menu Options:**
```
Decay System Configuration:
1) Global Decay Rate Multiplier: [100] (50-200%)
2) Extreme Emotion Threshold: [80] (0-100)
3) Extreme Decay Multiplier: [150] (100-300%)
4) Decay Rate - Fear: [2] (0-10 pts/tick)
5) Decay Rate - Anger: [2] (0-10 pts/tick)
6) Decay Rate - Happiness: [2] (0-10 pts/tick)
7) Decay Rate - Sadness: [2] (0-10 pts/tick)
8) Decay Rate - Pain: [4] (0-10 pts/tick)
9) Decay Rate - Horror: [3] (0-10 pts/tick)
A) Decay Rate - Disgust: [2] (0-10 pts/tick)
B) Decay Rate - Shame: [1] (0-10 pts/tick)
C) Decay Rate - Humiliation: [1] (0-10 pts/tick)
Q) Return to emotion menu
```

**Key Parameters:**

| Parameter | Range | Default | Effect |
|-----------|-------|---------|--------|
| Global Decay Rate Multiplier | 50-200% | 100 | Speeds up or slows all decay rates |
| Extreme Emotion Threshold | 0-100 | 80 | Point at which faster decay kicks in |
| Extreme Decay Multiplier | 100-300% | 150 | How much faster extreme emotions decay |
| Individual Decay Rates | 0-10 | varies | Points per tick (4 seconds) |

**Examples:**

**Faster Overall Decay (Casual Server):**
- Global Decay Rate Multiplier: 150%
- Result: All emotions return to baseline 50% faster

**Slower Overall Decay (Roleplay Server):**
- Global Decay Rate Multiplier: 75%
- Result: Emotions persist longer for dramatic effect

**Prevent Extreme Emotional States:**
- Extreme Emotion Threshold: 70
- Extreme Decay Multiplier: 200%
- Result: High emotions decay much faster, preventing saturation

**Pain Heals Quickly (Combat-Heavy Server):**
- Decay Rate - Pain: 6
- Result: Pain drops 1-6 points every 4 seconds

---

### 2. Display Thresholds (Visual Indicators)

**Purpose:** Controls when emotional indicators appear in room descriptions

**Menu Options:**
```
Display Thresholds Configuration:
1) Fear Display Threshold: [70] (0-100)
2) Anger Display Threshold: [70] (0-100)
3) Happiness Display Threshold: [80] (0-100)
4) Sadness Display Threshold: [70] (0-100)
5) Horror Display Threshold: [80] (0-100)
6) Pain Display Threshold: [70] (0-100)
7) Compassion Display Threshold: [70] (0-100)
8) Courage Display Threshold: [75] (0-100)
9) Curiosity Display Threshold: [70] (0-100)
A) Disgust Display Threshold: [70] (0-100)
B) Envy Display Threshold: [70] (0-100)
C) Excitement Display Threshold: [70] (0-100)
D) Friendship Display Threshold: [80] (0-100)
E) Greed Display Threshold: [70] (0-100)
F) Humiliation Display Threshold: [70] (0-100)
G) Love Display Threshold: [80] (0-100)
H) Loyalty Display Threshold: [75] (0-100)
I) Pride Display Threshold: [70] (0-100)
J) Shame Display Threshold: [70] (0-100)
K) Trust Display Threshold: [70] (0-100)
Q) Return to emotion menu
```

**Visual Indicators by Emotion:**

| Emotion | Display Text | Color |
|---------|-------------|-------|
| Fear | "(amedrontado)" | Magenta |
| Anger | "(furioso)" | Red |
| Happiness | "(feliz)" | Yellow |
| Sadness | "(triste)" | Blue |
| Horror | "(aterrorizado)" | Bright Magenta |
| Pain | "(sofrendo)" | Red |

**Examples:**

**Show More Emotional States (Roleplay):**
- All thresholds: 50
- Result: Emotions visible to players earlier

**Show Only Extreme States (Subtle):**
- All thresholds: 85
- Result: Only very strong emotions displayed

**Hide Specific Emotions:**
- Set threshold to 101 (impossible to reach)
- Result: That emotion never displays

---

### 3. Combat Flee Behavior

**Purpose:** Controls when mobs flee based on fear/courage/horror

**Menu Options:**
```
Combat Flee Behavior Configuration:
1) Fear Low Threshold: [50] (0-100)
2) Fear High Threshold: [70] (0-100)
3) Fear Low Modifier: [10] (-50 to +50 HP%)
4) Fear High Modifier: [15] (-50 to +50 HP%)
5) Courage Low Threshold: [50] (0-100)
6) Courage High Threshold: [70] (0-100)
7) Courage Low Modifier: [-10] (-50 to +50 HP%)
8) Courage High Modifier: [-15] (-50 to +50 HP%)
9) Horror Threshold: [80] (0-100)
A) Horror Modifier: [25] (-50 to +50 HP%)
Q) Return to emotion menu
```

**How Flee Modifiers Work:**

Base flee point: Mob flees at 30% HP (example)

**With Fear 60 (between low/high):**
- Modifier: +10% HP
- Flee point: 40% HP (flees earlier)

**With Courage 75 (high):**
- Modifier: -15% HP
- Flee point: 15% HP (fights longer)

**With Horror 85 (extreme):**
- Modifier: +25% HP
- Flee point: 55% HP (panic flee)

**Combined Example (Fear 60 + Courage 75):**
- Fear modifier: +10%
- Courage modifier: -15%
- Net: -5%
- Flee point: 25% HP (courage overcomes fear)

**Examples:**

**Brave Mobs (Combat-Heavy):**
- Courage Low Threshold: 40
- Courage High Modifier: -20
- Result: Mobs with even moderate courage fight much longer

**Cowardly Mobs (Realistic):**
- Fear Low Threshold: 40
- Fear Low Modifier: 15
- Result: Mobs flee earlier when scared

**Panic Flee Rare:**
- Horror Threshold: 90
- Result: Only extreme horror triggers panic

---

### 4. Pain System Configuration

**Purpose:** Controls pain emotion based on damage received

**Menu Options:**
```
Pain System Configuration:
1) Minor Damage Threshold: [5] (% of max HP)
2) Moderate Damage Threshold: [10] (% of max HP)
3) Heavy Damage Threshold: [25] (% of max HP)
4) Massive Damage Threshold: [50] (% of max HP)
5) Minor Pain Min: [1] (0-100 points)
6) Minor Pain Max: [5] (0-100 points)
7) Moderate Pain Min: [5] (0-100 points)
8) Moderate Pain Max: [15] (0-100 points)
9) Heavy Pain Min: [15] (0-100 points)
A) Heavy Pain Max: [30] (0-100 points)
B) Massive Pain Min: [30] (0-100 points)
C) Massive Pain Max: [50] (0-100 points)
Q) Return to emotion menu
```

**How Pain Works:**

When mob takes damage:
1. Calculate damage as % of max HP
2. Check thresholds (highest matching tier applies)
3. Add random pain within that tier's min/max range

**Example:**
- Mob has 100 max HP
- Takes 30 damage (30% of max HP)
- Matches "Heavy" threshold (25%)
- Pain added: rand(15, 30) = 22 points
- Mob's pain emotion: 0 + 22 = 22

**Examples:**

**High Pain Sensitivity (Roleplay):**
- All thresholds: 3%, 7%, 15%, 30%
- All max values: +10 higher
- Result: Mobs feel more pain from less damage

**Low Pain Sensitivity (Combat-Heavy):**
- All thresholds: 10%, 20%, 40%, 60%
- All max values: -5 lower
- Result: Mobs shrug off minor wounds

**Realistic Pain:**
- Keep defaults
- Result: Balanced pain response

---

### 5. Memory System Configuration

**Purpose:** Controls how mobs remember past interactions with players

**Menu Options:**
```
Memory System Configuration:
1) Recent Memory Weight: [10] (1-10 scale)
2) Fresh Memory Weight: [7] (1-10 scale)
3) Moderate Memory Weight: [5] (1-10 scale)
4) Old Memory Weight: [3] (1-10 scale)
5) Ancient Memory Weight: [1] (1-10 scale)
6) Recent Age Threshold: [300] (seconds)
7) Fresh Age Threshold: [600] (seconds)
8) Moderate Age Threshold: [1800] (seconds)
9) Old Age Threshold: [3600] (seconds)
A) Baseline Offset: [50] (0-100, neutral point)
Q) Return to emotion menu
```

**Memory Age Categories:**

| Category | Age | Default Weight | Influence |
|----------|-----|----------------|-----------|
| Recent | < 5 min | 10 | Full impact |
| Fresh | 5-10 min | 7 | Strong impact |
| Moderate | 10-30 min | 5 | Moderate impact |
| Old | 30-60 min | 3 | Weak impact |
| Ancient | > 60 min | 1 | Minimal impact |

**How Memories Work:**

1. Mob remembers last 10 interactions with each player
2. Each memory stores emotion snapshot + timestamp
3. Memories are weighted by age (recent = more important)
4. Major events (rescue, theft) get 2x weight
5. Weighted average creates "relationship emotion"
6. Relationship modifies base mood toward that player

**Examples:**

**Long Memories (Roleplay/Grudge-Holding):**
- Ancient Memory Weight: 5
- Old Memory Weight: 7
- Result: Mobs remember past actions longer

**Short Memories (Forgiving/Simple):**
- Ancient Memory Weight: 0
- Old Memory Weight: 1
- Moderate Memory Weight: 3
- Result: Only recent actions matter

**Quick Forgiveness:**
- Recent Age Threshold: 180 (3 min)
- Fresh Age Threshold: 300 (5 min)
- Result: Memories age faster, influence drops quicker

---

### 6. Trade & Quest Configuration

**Purpose:** Controls emotion thresholds for trading and questing

**Menu Options:**
```
Trade & Quest Configuration:
1) Trade Trust Threshold (High): [60] (0-100)
2) Trade Trust Threshold (Low): [30] (0-100)
3) Trade Greed High Threshold: [70] (0-100)
4) Trade Friendship High Threshold: [60] (0-100)
5) Quest Trust Threshold (Accept): [40] (0-100)
6) Quest Trust Threshold (Reward): [70] (0-100)
Q) Return to emotion menu
```

**Effects:**

**Trading:**
- Trust >= 60: 10% discount
- Trust < 30: Refuse service
- Greed >= 70: 20% markup
- Friendship >= 60: 5% additional discount

**Quests:**
- Trust < 40: Refuse to give quest
- Trust >= 70: Better quest rewards (20% bonus)

**Examples:**

**Generous Merchants:**
- Trust High: 50
- Greed High: 80
- Result: Easier to get discounts

**Suspicious Merchants:**
- Trust Low: 50
- Trust High: 80
- Result: Must build strong trust for discounts

**Easy Quests:**
- Quest Trust Accept: 20
- Result: Most players can get quests

---

### 7. Social Behavior Configuration

**Purpose:** Controls emotion-driven social interactions

**Menu Options:**
```
Social Behavior Configuration:
1) Social Initiation Happiness Threshold: [60] (0-100)
2) Social Initiation Anger Threshold: [60] (0-100)
3) Social Initiation Sadness Threshold: [50] (0-100)
4) Positive Social Happiness Boost: [5] (0-20 points)
5) Negative Social Anger Increase: [3] (0-20 points)
Q) Return to emotion menu
```

**How Social Behavior Works:**

Mobs with high emotions initiate social actions:
- Happiness >= 60: More likely to smile, laugh, hug
- Anger >= 60: More likely to glare, frown, scowl
- Sadness >= 50: Less likely to socialize (withdraw)

**Examples:**

**Very Social Mobs:**
- Happiness Threshold: 40
- Result: Happy mobs socialize more often

**Reserved Mobs:**
- All thresholds: 75
- Result: Only strong emotions trigger socials

---

### 8. Group Behavior Configuration

**Purpose:** Controls emotion effects on group dynamics

**Menu Options:**
```
Group Behavior Configuration:
1) Loyalty High Threshold: [60] (0-100)
2) Loyalty Low Threshold: [30] (0-100)
3) Friendship High Threshold: [60] (0-100)
4) Envy High Threshold: [70] (0-100)
Q) Return to emotion menu
```

**Effects:**

- **Loyalty >= 60:** Stay in group even when hurt
- **Loyalty < 30:** Abandon group when scared
- **Friendship >= 60:** More likely to join groups
- **Envy >= 70:** Refuse to group with better-equipped players

**Examples:**

**Loyal Groups (Military/Guard Mobs):**
- Loyalty High: 50
- Loyalty Low: 20
- Result: Groups stick together better

**Independent Mobs (Mercenaries):**
- Loyalty High: 80
- Loyalty Low: 50
- Result: Less group cohesion, self-preservation

---

### 9. Combat Effects Configuration

**Purpose:** Controls emotion effects during combat

**Menu Options:**
```
Combat Effects Configuration:
1) Anger High Threshold (Combat): [70] (0-100)
2) Anger Damage Bonus: [15] (0-50%)
3) Anger Attack Frequency Bonus: [25] (0-100%)
4) Pain Low Threshold: [30] (0-100)
5) Pain Moderate Threshold: [50] (0-100)
6) Pain High Threshold: [70] (0-100)
7) Pain Accuracy Penalty (Low): [1] (0-10 THAC0)
8) Pain Accuracy Penalty (Moderate): [2] (0-10 THAC0)
9) Pain Accuracy Penalty (High): [4] (0-10 THAC0)
A) Pain Damage Penalty (Low): [5] (0-50%)
B) Pain Damage Penalty (Moderate): [10] (0-50%)
C) Pain Damage Penalty (High): [20] (0-50%)
Q) Return to emotion menu
```

**How Combat Effects Work:**

**Anger Bonuses:**
- Anger >= 70: 25% chance of extra attack per round
- Anger >= 70: +15% damage on all attacks

**Pain Penalties:**
- Pain 30-49: +1 THAC0 (worse accuracy), -5% damage
- Pain 50-69: +2 THAC0, -10% damage
- Pain 70-100: +4 THAC0, -20% damage

**Examples:**

**Rage Warriors (High Damage):**
- Anger Threshold: 60
- Anger Damage Bonus: 25%
- Result: Angry mobs hit much harder

**Pain Resistance (Tough Mobs):**
- All Pain Thresholds: +20 higher
- All Pain Penalties: -50%
- Result: Pain has less combat impact

**Realistic Combat:**
- Keep defaults
- Result: Balanced anger/pain effects

---

### 10. Weather & Climate Configuration

**Purpose:** Controls how weather affects mob emotions

**Menu Options:**
```
Weather & Climate Configuration:
1) Enable Weather Affects Emotions: [YES/NO]
2) Weather Effect Multiplier: [100] (0-200%)
3) SAD (Seasonal Affective) Enable: [YES/NO]
Q) Return to emotion menu
```

**Effects:**

- Bad weather: Increases fear, sadness; decreases happiness
- Good weather: Decreases fear; increases happiness
- Preferred weather: Boosts happiness
- Indoor mobs: 50% reduced weather effects
- Adaptation: Effects reduce over time (up to 50% over 1 week)

**Examples:**

**Strong Weather Influence:**
- Weather Effect Multiplier: 150%
- Result: Weather has 50% more emotional impact

**Weak Weather Influence:**
- Weather Effect Multiplier: 50%
- Result: Weather has half the emotional impact

**No Weather Effects:**
- Enable Weather Affects Emotions: NO
- Result: Weather doesn't affect emotions at all

---

### P. Load Preset Configuration

**Purpose:** Quick-load optimized configurations for different server styles

**Menu Options:**
```
Emotion Configuration Presets:
1) Balanced (Recommended)
2) Roleplay (Slow decay, strong emotions)
3) Combat-Heavy (Fast decay, combat focus)
Q) Cancel

Enter choice:
```

**Preset Descriptions:**

**Balanced (Default):**
- Moderate decay rates
- Standard thresholds
- Balanced combat effects
- Use for: General-purpose servers

**Roleplay:**
- Slower decay (75% global multiplier)
- Lower display thresholds (60)
- Longer memories (higher ancient weight)
- Higher social activity thresholds
- Use for: RP-focused, dramatic servers

**Combat-Heavy:**
- Faster decay (125% global multiplier)
- Higher display thresholds (80)
- Shorter memories (lower ancient weight)
- Stronger combat bonuses
- Use for: PK, arena, combat servers

---

## Step-by-Step Examples

### Example 1: Making Mobs Braver

**Goal:** Reduce fleeing in combat

**Steps:**
1. Enter cedit
2. Select "Emotion Configuration Menu"
3. Select "3) Combat Flee Behavior"
4. Option "5) Courage Low Threshold" → Enter: 40
5. Option "8) Courage High Modifier" → Enter: -20
6. Option "Q" to save and return
7. Test in-game

**Result:** Mobs with courage 40+ fight longer; courage 70+ fight much longer

---

### Example 2: Making Pain Matter More

**Goal:** Increase pain's impact in combat

**Steps:**
1. Enter cedit
2. Select "Emotion Configuration Menu"
3. Select "9) Combat Effects Configuration"
4. Option "4) Pain Low Threshold" → Enter: 20
5. Option "7) Pain Accuracy Penalty (Low)" → Enter: 2
6. Option "A) Pain Damage Penalty (Low)" → Enter: 10
7. Option "Q" to save and return

**Result:** Pain affects combat earlier and more severely

---

### Example 3: Creating Grudge-Holding Mobs

**Goal:** Make mobs remember past attacks longer

**Steps:**
1. Enter cedit
2. Select "Emotion Configuration Menu"
3. Select "5) Memory System Configuration"
4. Option "4) Old Memory Weight" → Enter: 6
5. Option "5) Ancient Memory Weight" → Enter: 4
6. Option "9) Old Age Threshold" → Enter: 7200 (2 hours)
7. Option "Q" to save and return

**Result:** Mobs remember attacks for hours with strong influence

---

## Tips for Administrators

### Testing Changes
1. **Make small adjustments** - Change one parameter at a time
2. **Test with stat command** - Use `stat mob <name>` to see emotion values
3. **Observe behavior** - Watch how mobs react to events
4. **Keep notes** - Document what works well
5. **Ask for feedback** - Get player input on emotion behaviors

### Common Configurations

**For Newbie Areas:**
- Lower fear thresholds (40)
- Higher courage modifiers (-20)
- Lower pain penalties
- Result: Mobs less intimidating

**For Dangerous Areas:**
- Higher anger combat bonuses
- Lower flee thresholds
- Higher pain from damage
- Result: More challenging combat

**For Social/Town Areas:**
- Higher display thresholds (hide emotions)
- Lower social thresholds (more activity)
- Higher friendship/trust for trades
- Result: Friendly, interactive atmosphere

---

## Troubleshooting

### Problem: Mobs flee too easily
**Solution:**
- Increase flee thresholds (60 → 75)
- Increase courage modifiers (-15 → -25)
- Decrease fear modifiers (15 → 10)

### Problem: Emotions too extreme
**Solution:**
- Decrease extreme threshold (80 → 70)
- Increase extreme decay multiplier (150 → 200)
- Increase global decay multiplier (100 → 125)

### Problem: Mobs don't remember players
**Solution:**
- Increase memory weights for older memories
- Increase age thresholds (longer time before aging)
- Check zone reset frequency (resets clear memories)

### Problem: Too many emotion indicators displayed
**Solution:**
- Increase display thresholds (70 → 85)
- Or disable specific emotions (set threshold to 101)

### Problem: Combat too easy/hard
**Solution:**
- Adjust pain thresholds and penalties
- Adjust anger bonuses
- Adjust flee behavior

---

## Reference: Valid Value Ranges

| Parameter Type | Valid Range | Notes |
|---------------|-------------|-------|
| Thresholds (emotions) | 0-100 | Emotion level required |
| Decay rates | 0-10 | Points per tick (4 seconds) |
| Multipliers (%) | 50-200 or 100-300 | Percentage values |
| HP modifiers | -50 to +50 | Percentage adjustment |
| Memory weights | 1-10 | Importance scaling |
| Age thresholds | 0-3600+ | Seconds |
| Damage thresholds | 1-100 | Percentage of max HP |
| Pain amounts | 0-100 | Emotion points added |
| Combat bonuses | 0-50 | Percentage bonuses |
| Combat penalties | 0-50 | Percentage penalties |
| THAC0 penalties | 0-10 | To-hit penalty points |

---

## Related Documentation

- **RFC-1000_EMOTION_SYSTEM_ARCHITECTURE_RESPONSE.md** - Complete technical specification
- **EMOTION_SYSTEM_TECHNICAL_SUMMARY.md** - Developer quick reference
- **EMOTION_CONFIG_SYSTEM.md** - Configuration system details
- **HYBRID_EMOTION_SYSTEM.md** - Hybrid architecture guide

---

**Questions?** Contact the development team or consult the technical documentation files.
