Vitalia Reborn
=============

Welcome to the Vitalia Reborn source code! 

From this repository you can build the Vitalia Reborn Mud. Modify them in any way you can imagine, and share your changes with others! 

We are trying to revive VitaliaMUD (A Brazilian mud from the earlier 2000 years based in CircleMUD) with a lot of bugfixes from tbaMUD. tbaMUD is continuing the development of CircleMUD.

We have a heap of documentation available on the web. If you're looking for the answer to something, you may want to start here: 
[TBAmud](https://www.tbamud.com/).

If you need more, just ask! A lot of developers hang out on the [forums](https://forums.tbamud.com/),
and we're proud to be part of a well-meaning, friendly and welcoming community.

Branches
--------

Our branches is where we do the bugfixes and test new resources. They may pop-up from time to time as we stabilize new releases or hotfixes.

History
---------------------------

   VitaliaMUD(originaly AbbaMUD) was created in
1995, in Universidade de Campinas (UNICAMP), by Vitor de Paula. In 2000, "O Mundo de Vitália" (VitaliaMUD). had some changes in the staff. 
   The Reborn version is trying to bring back from this earlier glory with a lot of bugfixes and new resources as they were in the devs todo notes from before.

Features
--------

### Multiline Aliases
Vitalia Reborn supports creating aliases that execute multiple commands in sequence using semicolon (`;`) separation:

```
alias chess n;n;n;n;w;w;w;w
alias prep cast 'bless';cast 'armor';cast 'shield'
alias info score;time;who;weather
```

See [docs/MULTILINE_ALIASES.md](docs/MULTILINE_ALIASES.md) for detailed documentation and examples.

### Genetic Algorithm System

Vitalia Reborn implements a sophisticated genetic trait system that defines the innate behavioral tendencies of NPCs. Each mob has 12 genetic traits (range 0-100) that influence their personality and decision-making:

- **Combat Traits**: `wimpy_tendency` (tendency to flee), `brave_prevalence` (courage in battle)
- **Social Traits**: `group_tendency` (forming groups), `follow_tendency` (following others)
- **Economic Traits**: `trade_tendency` (trading behavior), `loot_tendency` (taking items)
- **Exploration Traits**: `roam_tendency` (wandering), `adventurer_tendency` (exploration)
- **Utility Traits**: `quest_tendency` (accepting quests), `use_tendency` (using items/skills)
- **Support Traits**: `healing_tendency` (healing allies), `equip_tendency` (equipping items)

**Key Features:**
- Genetics initialize emotional baselines (e.g., high `brave_prevalence` → high courage emotion)
- Traits influence mob behavior probabilities in real-time decision-making
- Quest success/failure can evolve genetic traits over time
- Statistical analysis available via the `gstats` command for builders

**Example Usage:**
```
stat mob_name              # View mob's genetic profile
gstats all healing         # Analyze healing tendency across all mobs
gstats zone 30 adventurer  # Check adventurer distribution in zone 30
```

For implementation details, see [md-docs/GSTAT_GENETICS_UPDATE.md](md-docs/GSTAT_GENETICS_UPDATE.md) and [md-docs/GENETICS_EMOTIONS_REPUTATION_INTERACTIONS.md](md-docs/GENETICS_EMOTIONS_REPUTATION_INTERACTIONS.md).

### Emotion System

Vitalia Reborn features a comprehensive **20-emotion hybrid system** that creates realistic, dynamic NPC behavior through two layers:

**Layer 1: MOOD (Global State)**
- General emotional baseline stored per mob
- Influenced by environment, time, weather, and experiences
- Decays gradually toward genetic baselines

**Layer 2: RELATIONSHIP (Per-Entity State)**
- Specific feelings toward individual players/mobs
- Stored in circular memory buffer (10 recent interactions)
- Weighted by recency and importance

**20 Emotion Types (0-100 scale):**
- **Basic**: fear, anger, happiness, sadness, pain
- **Social**: friendship, love, trust, loyalty
- **Motivational**: curiosity, greed, pride, excitement
- **Empathic**: compassion, envy, shame
- **Complex**: disgust, horror, humiliation, courage

**Behavioral Effects:**
- **Combat**: Anger increases attack power, pain reduces effectiveness
- **Social**: Emotions determine social interaction frequency and type
- **Trading**: Trust affects shop prices and service availability
- **Decision-Making**: Emotions modify action scores in Shadow Timeline projections

**Integration:**
- Genetics initialize emotional baselines
- Events (combat, healing, socials) update emotions dynamically
- Emotions influence moral reasoning and reputation changes
- Alignment (good/evil/neutral) modifies emotional tendencies

**Example:**
```c
// Shopkeeper trusts this specific customer
int trust = get_effective_emotion_toward(keeper, buyer, EMOTION_TYPE_TRUST);
if (trust >= 60) {
    price *= 0.90;  // 10% discount for trusted customer
}
```

For technical details, see [HYBRID_EMOTION_SYSTEM.md](HYBRID_EMOTION_SYSTEM.md) and [EMOTION_SYSTEM_TODO.md](EMOTION_SYSTEM_TODO.md).

### Moral Reasoning System

The Moral Reasoning System implements **Shultz & Daley's rule-based model** for qualitative moral judgment, enabling NPCs to evaluate the ethical implications of their actions with full emotion system integration.

**Core Concepts:**
- **Responsibility**: Did the entity cause the harm? Was it voluntary and foreseeable?
- **Intent**: Was harm planned (strong intent) or reckless/negligent?
- **Justification**: Did the goal outweigh the harm? Were there less harmful alternatives?
- **Blameworthiness**: Responsible + Unjustified + (Harm > Benefit) = Guilty

**10 Moral Action Types:**
- Attack, steal, deceive, betray, abandon ally
- Help, heal, defend, trade, self-sacrifice

**Integration with Game Systems:**

1. **Shadow Timeline**: Moral costs modify action projection scores
   - Guilty actions receive negative scores (discouraging immoral behavior)
   - Helpful actions receive positive scores (encouraging altruism)
   - Alignment affects moral sensitivity (2x penalty for good-aligned, 0.5x for evil)

2. **Emotion System**: Bidirectional influence
   - **Emotions → Morality**: Compassion increases harm sensitivity, anger reduces it
   - **Morality → Emotions**: Guilty actions trigger shame/disgust, moral acts increase pride

3. **Alignment & Reputation**: Actions have consequences
   - Guilty actions shift alignment toward evil, damage reputation (for good mobs)
   - Moral actions shift toward good, build reputation
   - Evil mobs gain "infamy" reputation from guilty acts

**Example Scenario:**
```
Good-aligned guard evaluating attack on innocent:
- Base moral cost: -40 (guilty action)
- Good alignment: -40 × 2 = -80 (strong aversion)
- High compassion: -80 - 13 = -93 (even stronger aversion)
Result: Guard will not attack innocent target
```

**Validation:** Tested against 202 scenarios from original research (102 guilty, 100 innocent cases).

For complete documentation, see [docs/MORAL_REASONING.md](docs/MORAL_REASONING.md) and dataset at `lib/misc/moral_reasoning_dataset.txt`.

### Shadow Timeline API - Future Prevision System

The **Shadow Timeline** (RFC-0003 compliant) is a cognitive future simulation layer that allows autonomous NPCs to internally explore possible outcomes of actions **before** executing them, creating more intelligent and realistic behavior.

**Core Principles:**
- **Non-authoritative**: Proposes possibilities, never asserts facts
- **Observational only**: Never mutates real world state
- **Cognitively bounded**: Limited by entity capacity (500-1000 points)
- **Subjectivity**: Different mobs generate different predictions based on emotions and genetics
- **Non-persistent**: Predictions are ephemeral, not recorded

**How It Works:**

1. **Projection Generation**: Mob simulates multiple possible actions
   - Movement, combat, fleeing, social interactions, item use, spells
   - Each projection has a cognitive cost (3-20 points depending on complexity)

2. **Outcome Scoring**: Each action receives a score based on:
   - Danger level, reward potential, goal achievement
   - Emotional state (fear amplifies danger, courage reduces it)
   - Moral implications (integrated with moral reasoning)
   - Genetic traits (brave mobs prefer aggressive actions)

3. **Action Selection**: Best-scoring projection is executed in real world

4. **Capacity Regeneration**: 50 points per tick, allowing continuous operation

**12 Action Types:**
- `MOVE`, `ATTACK`, `FLEE`, `USE_ITEM`, `CAST_SPELL`
- `SOCIAL`, `TRADE`, `QUEST`, `WAIT`, `FOLLOW`, `GROUP`, `GUARD`

**Enabling Shadow Timeline:**
```
# In mob file (.mob):
#12345
mob_name~
short description~
...
ISNPC SHADOWTIMELINE    <-- Add this flag
```

**Example Use Cases:**
- Merchant evaluates whether attacking would harm trade reputation
- Guard predicts if leaving post would endanger patrol area
- Healer foresees benefit of using healing spell vs. attacking
- Thief estimates risk/reward of stealing valuable item

**Sentinel Compatibility:**
Mobs with both `MOB_SENTINEL` and `MOB_SHADOWTIMELINE` intelligently balance:
- Staying at post when no threats present (guard duty)
- Evaluating combat options when attacked (using cognitive simulation)
- Returning to post after handling threats (prioritized movement)

**Performance:**
- Memory: ~1KB per mob with Shadow Timeline active
- CPU: Lightweight heuristics, no recursive searches
- Bounded by cognitive capacity to prevent runaway computation

For architectural details and RFC-0003 specification, see [docs/SHADOW_TIMELINE.md](docs/SHADOW_TIMELINE.md) and [RFC_0003_DEFINITION.md](RFC_0003_DEFINITION.md).

Additional Notes
----------------

Your private forks of the Vitalia Reborn code are associated with your GitHub account permissions.
If you unsubscribe or switch GitHub user names, you'll need to re-fork and upload your changes from a local copy. 

