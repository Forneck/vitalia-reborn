# Weather and Spell Climate System Enhancement

## Design Document

**Version:** 1.0  
**Date:** 2025-11-18  
**Status:** Implementation Plan

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Problem Statement](#problem-statement)
3. [Design Principles](#design-principles)
4. [Proposed Solution](#proposed-solution)
5. [Implementation Details](#implementation-details)
6. [Player Benefits](#player-benefits)
7. [Game Balance Considerations](#game-balance-considerations)
8. [Technical Architecture](#technical-architecture)
9. [Testing Strategy](#testing-strategy)
10. [Future Considerations](#future-considerations)

---

## Executive Summary

This document outlines the enhancement of Vitalia Reborn's weather and spell climate system to address player concerns about:
- Control Weather spell perceived effectiveness and mana cost
- Magic power vs physical combat balance
- Visibility of weather bonuses on spells and schools

The solution introduces a **Mana Density** system that creates a secondary atmospheric layer, making weather effects on magic more tangible and rewarding strategic spell use.

---

## Problem Statement

### Current Issues

1. **Control Weather Spell Limitations**
   - Takes too long to be effective depending on parameters
   - High mana cost with perceived minimal benefit
   - Not rewarding enough for the investment

2. **Magic vs Physical Combat Imbalance**
   - Physical attacks and skills are perceived as more powerful than magic spells
   - Weather bonuses exist but are not clearly visible to players
   - Lack of strategic depth in spell casting location choices

3. **Poor Visibility of Climate Bonuses**
   - Players cannot see which schools/elements are favored in current weather
   - No clear feedback about weather effects on spells
   - Difficult to make informed decisions about spell selection

---

## Design Principles

### Core Principles

1. **Visibility First**
   - All weather effects must be clearly communicated to players
   - Use color coding consistent with existing spell system
   - Provide actionable information when players check weather

2. **Strategic Depth**
   - Weather should create meaningful tactical choices
   - Different environments should favor different magic types
   - Indoor/outdoor distinction should matter

3. **Reward Investment**
   - Control Weather spell should feel powerful and impactful
   - Mana spent on weather control should return benefits
   - High-density areas should reward mage characters

4. **Minimal Code Changes**
   - Build on existing weather and spell systems
   - Maintain backward compatibility
   - Use established patterns and conventions

5. **Game Balance**
   - Keep magic competitive with physical combat
   - Ensure no single strategy dominates
   - Maintain class identity and roles

---

## Proposed Solution

### 1. Enhanced Weather Command

**What:** Add display of favored/unfavored schools and elements to the weather command.

**Why:** Players need immediate, clear feedback about how current weather affects their spells.

**How:**
- Color-code elements using existing element color system
- Show school names for clarity
- Display favorable conditions in bright colors (green/cyan)
- Display unfavorable conditions in dim colors (red/magenta)
- List based on current weather parameters (sky, humidity, pressure, temperature)

**When:** Display automatically when player uses "weather" command.

**Player Benefit:** 
- Make informed spell selection decisions
- Understand why spells feel more/less effective
- Plan strategy based on environmental conditions

**Example Output:**
```
O céu está limpo, você sente uma brisa agradável e o clima está ameno.
Pressão: 1015 hPa, Céu: limpo 
Temperatura 20 º.C, Umidade 0.45
Vento: 4.50 m/s

Condições Mágicas Favoráveis:
  Escolas: Adivinhação (+15%), Encantamento (+5%)
  Elementos: Fogo (+25%), Ar (+20%)

Condições Mágicas Desfavoráveis:
  Escolas: Evocação (-10%), Ilusão (-15%)
  Elementos: Água (-25%), Gelo (-25%)
```

### 2. Mana Density System

**What:** A room-based floating-point parameter representing magical energy concentration.

**Why:** 
- Creates tangible benefit from weather control
- Makes environment matter for spell casting
- Provides secondary layer above world alongside weather

**How:**

#### Calculation Factors

Mana density is calculated from multiple environmental factors:

1. **Weather Conditions (40% weight)**
   - Sky state: Storm/Lightning = high density
   - Humidity: Moderate (40-70%) = optimal
   - Pressure: Low pressure = higher density
   - Temperature: Moderate ranges preferred

2. **Time of Day (20% weight)**
   - Dawn/Dusk (twilight): Peak density
   - Night: High density
   - Day: Moderate density
   - Noon: Lower density

3. **Sun State (15% weight)**
   - SUN_RISE/SUN_SET: Highest bonus
   - SUN_DARK: High bonus
   - SUN_LIGHT: Standard

4. **Sector Type (15% weight)**
   - Water sectors: +30% for water/ice spells
   - Forest/Hills: +20% for nature spells
   - Mountains: +15% for air spells
   - Desert/Fire: +20% for fire spells
   - City/Road: Neutral

5. **Indoor/Outdoor (10% weight)**
   - Outdoor: Full weather effects
   - Indoor: 30% of weather effects, but stable
   - Special rooms (temples, libraries): Can have high base density

#### Density Scale
- **0.0 - 0.3:** Very Low (unfavorable) - 90% mana cost, 80% power
- **0.3 - 0.5:** Low - 95% mana cost, 90% power
- **0.5 - 0.7:** Normal (baseline) - 100% mana cost, 100% power
- **0.7 - 0.9:** High - 90% mana cost, 110% power
- **0.9 - 1.2:** Very High - 80% mana cost, 120% power
- **1.2+:** Exceptional - 70% mana cost, 130% power

**When:** 
- Calculate on-demand when spell is cast
- Update periodically (every game hour) for display purposes
- Can be temporarily boosted by Control Weather spell

**Player Benefit:**
- Mages become more efficient in favorable conditions
- Strategic positioning matters
- Control Weather becomes visibly rewarding
- Creates "power spots" for magical activities

**Game Balance:**
- Physical classes unaffected by mana density
- Magic users get situational advantage, not constant superiority
- Indoor safety comes at cost of reduced magical power
- Weather control has tangible, measurable benefit

### 3. Control Weather Spell Enhancement

**What:** Improve feedback and add mana density boost effect.

**Why:** Make the spell feel impactful and worth its mana cost.

**How:**
- Show before/after comparison of weather parameters
- Display expected mana density change
- Add temporary mana density boost in area (stacks with natural density)
- Boost duration based on spell power and caster level
- Show "magical energy swirls in the air" type messages

**When:** Immediately upon successful cast, with periodic reminders of active effect.

**Player Benefit:**
- Clear understanding of spell effects
- Tangible reward for mana investment
- Can create favorable conditions for extended operations

---

## Implementation Details

### File Changes Required

1. **src/act.informative.c**
   - Modify `ACMD(do_weather)` to add school/element display
   - Add helper function to determine favored/unfavored conditions

2. **src/spells.c** or **src/magic.c**
   - Add `float calculate_mana_density(struct char_data *ch)` function
   - Modify existing spell casting to use mana density
   - Add mana density display functions

3. **src/spells.c**
   - Enhance `ASPELL(spell_control_weather)` with better feedback
   - Add mana density boost effect to control weather

4. **src/structs.h**
   - Add `float mana_density` to appropriate structure (room or weather)
   - Add `time_t mana_density_boost_expire` for control weather effect

5. **src/weather.c**
   - Possibly add mana density calculation helpers
   - Update weather_change to note mana density shifts

### Color Coding Reference

Using existing element colors from act.other.c:
- Fire: CBRED (Bright Red)
- Water: CBBLU (Bright Blue)  
- Air: CBWHT (Bright White)
- Earth: CCYEL (Yellow)
- Lightning: CBCYN (Bright Cyan)
- Ice: CBBLU (Bright Blue)
- Acid: CBGRN (Bright Green)
- Poison: CCGRN (Green)
- Holy: CBYEL (Bright Yellow)
- Unholy: CBMAG (Bright Magenta)
- Mental: CCMAG (Magenta)
- Physical: CCWHT (White)

Favorable condition: CBGRN (Bright Green) or CBCYN (Bright Cyan)
Unfavorable condition: CBRED (Bright Red) or CCRED (Dark Red)

---

## Player Benefits

### Immediate Benefits

1. **Clear Feedback**
   - See exactly how weather affects your spells
   - Understand why spell power varies
   - Make informed decisions about spell selection

2. **Strategic Depth**
   - Choose casting locations strategically
   - Time spells for optimal conditions
   - Use Control Weather tactically

3. **Power Fantasy**
   - Feel the environment respond to your magic
   - Experience tangible benefits from weather control
   - See your mage character excel in favorable conditions

### Long-term Benefits

1. **Enhanced Class Identity**
   - Mages have environmental mastery
   - Weather matters to spellcasters
   - Unique gameplay considerations for magic users

2. **Exploration Incentive**
   - Seek out high mana density areas
   - Discover favorable casting locations
   - Environmental awareness matters

3. **Tactical Complexity**
   - Pre-battle weather setup becomes viable strategy
   - Group coordination around weather control
   - Environmental factors in PvP/PvE encounters

---

## Game Balance Considerations

### Balance Mechanisms

1. **Mana Density is Double-Edged**
   - High density benefits ALL casters (including enemies)
   - Must balance offensive and defensive considerations
   - Physical classes unaffected, maintaining their reliability

2. **Control Weather Limitations**
   - High mana cost remains (balanced by efficiency gains)
   - Takes time to change weather (can't instant-win)
   - Area effect benefits enemies too
   - Doesn't work indoors

3. **Situational, Not Constant**
   - Bonuses require specific conditions
   - Players must adapt to changing weather
   - No "always optimal" strategy

4. **Class Balance**
   - Physical classes: Consistent, reliable, weather-independent
   - Magic classes: Variable power, efficient in right conditions, strategic depth
   - Hybrid classes: Flexibility to use best tool for situation

### Preventing Exploitation

1. **Diminishing Returns**
   - Mana density capped at reasonable maximum
   - Multiple Control Weather casts don't stack infinitely
   - Time-limited boosts

2. **Countermeasures Available**
   - Other casters can alter weather
   - Physical classes ignore mana density
   - Indoor areas provide neutral ground

3. **Risk/Reward Balance**
   - High mana density requires outdoor exposure
   - Weather changes affect everyone
   - Setup time creates vulnerability window

---

## Technical Architecture

### Mana Density Calculation Pseudocode

```c
float calculate_mana_density(struct char_data *ch) {
    float density = 0.5; // baseline
    room_rnum room = IN_ROOM(ch);
    zone_rnum zone = world[room].zone;
    struct weather_data *weather = zone_table[zone].weather;
    
    // Indoor rooms get reduced weather effect
    float weather_weight = ROOM_FLAGGED(room, ROOM_INDOORS) ? 0.3 : 1.0;
    
    // Weather contribution (40% of total)
    float weather_factor = 0.0;
    if (weather->sky == SKY_LIGHTNING) weather_factor = 0.4;
    else if (weather->sky == SKY_RAINING) weather_factor = 0.3;
    else if (weather->sky == SKY_CLOUDY) weather_factor = 0.2;
    else if (weather->sky == SKY_SNOWING) weather_factor = 0.25;
    else weather_factor = 0.1; // cloudless
    
    // Humidity factor (optimal around 50-70%)
    if (weather->humidity >= 0.5 && weather->humidity <= 0.7)
        weather_factor += 0.2;
    else if (weather->humidity > 0.8)
        weather_factor += 0.1;
    
    // Low pressure increases density
    if (weather->pressure < 990)
        weather_factor += 0.15;
    else if (weather->pressure < 1000)
        weather_factor += 0.1;
    
    density += weather_factor * weather_weight * 0.4;
    
    // Time of day (20% of total)
    float time_factor = 0.0;
    if (time_info.hours == dawn_hour || time_info.hours == dusk_hour)
        time_factor = 0.2; // peak at twilight
    else if (weather_info.sunlight == SUN_DARK)
        time_factor = 0.15;
    else if (weather_info.sunlight == SUN_LIGHT)
        time_factor = 0.05;
    
    density += time_factor;
    
    // Sun state (15% of total)
    if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_SET)
        density += 0.15;
    else if (weather_info.sunlight == SUN_DARK)
        density += 0.10;
    
    // Sector type (15% of total)
    int sector = SECT(room);
    float sector_factor = 0.0;
    switch (sector) {
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
            sector_factor = 0.15;
            break;
        case SECT_FOREST:
        case SECT_HILLS:
        case SECT_MOUNTAIN:
            sector_factor = 0.12;
            break;
        default:
            sector_factor = 0.05;
    }
    density += sector_factor;
    
    // Check for Control Weather boost
    if (has_weather_control_boost(zone)) {
        density += 0.3; // significant boost
    }
    
    // Cap between 0.0 and 1.5
    return URANGE(0.0, density, 1.5);
}
```

### Spell Modification Integration

```c
// In mag_damage() or similar spell effect function:
float mana_density = calculate_mana_density(ch);
float density_modifier = 1.0;

// Power scaling
if (mana_density >= 1.2)
    density_modifier = 1.3;
else if (mana_density >= 0.9)
    density_modifier = 1.2;
else if (mana_density >= 0.7)
    density_modifier = 1.1;
else if (mana_density >= 0.5)
    density_modifier = 1.0;
else if (mana_density >= 0.3)
    density_modifier = 0.9;
else
    density_modifier = 0.8;

damage = (int)(damage * density_modifier);

// Mana cost modification (in cast_spell or similar)
float mana_cost_modifier = 1.0;
if (mana_density >= 1.2)
    mana_cost_modifier = 0.7;
else if (mana_density >= 0.9)
    mana_cost_modifier = 0.8;
else if (mana_density >= 0.7)
    mana_cost_modifier = 0.9;
else if (mana_density >= 0.5)
    mana_cost_modifier = 1.0;
else if (mana_density >= 0.3)
    mana_cost_modifier = 0.95;
else
    mana_cost_modifier = 0.9;

mana_cost = (int)(mana_cost * mana_cost_modifier);
```

---

## Testing Strategy

### Unit Testing

1. **Mana Density Calculation**
   - Test each factor independently
   - Verify cap limits (0.0 - 1.5)
   - Check indoor vs outdoor differences
   - Validate sector type effects

2. **Weather Display**
   - Verify color codes render correctly
   - Check all weather condition combinations
   - Ensure proper Portuguese language display

3. **Spell Power Scaling**
   - Test damage at various density levels
   - Verify mana cost adjustments
   - Check healing spell modifications

### Integration Testing

1. **Control Weather Interaction**
   - Cast Control Weather and verify density boost
   - Check boost expiration
   - Test multiple casters in same zone

2. **Zone Boundaries**
   - Move between zones with different weather
   - Verify density updates correctly
   - Test indoor/outdoor transitions

3. **Time Progression**
   - Observe density changes through day/night cycle
   - Verify twilight peak density
   - Check seasonal variations if applicable

### Player Experience Testing

1. **Visibility Testing**
   - Ensure information is clear and not overwhelming
   - Verify color coding is helpful, not distracting
   - Check text fits in typical client windows

2. **Balance Testing**
   - Compare mage vs physical combat effectiveness
   - Test PvP scenarios with weather control
   - Verify no single strategy dominates

3. **Performance Testing**
   - Ensure density calculation doesn't lag
   - Check impact on server tick rate
   - Monitor memory usage for new parameters

---

## Future Considerations

### Potential Enhancements

1. **Persistent Weather Effects**
   - Store mana density boosts in zone save data
   - Allow long-lasting enchantments on specific rooms
   - Create permanent "ley lines" or magical nodes

2. **Advanced Weather Spells**
   - New spells to directly manipulate mana density
   - Area-specific weather control (single room vs zone)
   - Defensive spells to reduce enemy mana density

3. **Environmental Hazards**
   - Extremely low density could cause spell failure
   - Very high density could have side effects
   - Wild magic effects in unusual conditions

4. **Player-Built Structures**
   - Allow players to build mage towers with high density
   - Create weather-resistant indoor casting areas
   - Add magical focus items that boost local density

5. **Extended Feedback System**
   - Add "sense magic" skill to detect density
   - Show density in room descriptions for sensitives
   - Indicate optimal casting times via divination spells

### Integration with Other Systems

1. **Quest System**
   - Quests requiring specific weather conditions
   - Tasks to stabilize or destabilize mana density
   - Weather-dependent magical phenomena to investigate

2. **Crafting System**
   - Potion brewing affected by mana density
   - Enchanting items requires specific conditions
   - Weather-dependent material gathering

3. **Zone Design**
   - Create naturally high-density magical areas
   - Design zones with weather control puzzles
   - Add lore about ancient weather-controlling artifacts

---

## Conclusion

This enhancement addresses player concerns while maintaining game balance through:

1. **Visibility:** Clear, color-coded display of weather effects
2. **Reward:** Tangible benefits from Control Weather spell
3. **Strategy:** Meaningful tactical choices based on environment
4. **Balance:** Situational advantages without dominance

The Mana Density system creates a rich, interactive environment where weather truly matters to spellcasters, rewarding strategic thinking while maintaining the core balance between magic and physical combat.

---

## Implementation Checklist

- [ ] Design approval
- [ ] Create helper functions for mana density calculation
- [ ] Enhance weather command display
- [ ] Integrate mana density into spell casting
- [ ] Update Control Weather spell
- [ ] Add appropriate messages and feedback
- [ ] Test all weather conditions
- [ ] Balance testing with dev team
- [ ] Player alpha testing
- [ ] Document in help files
- [ ] Update changelog
- [ ] Deploy to production

---

**Document Version History:**
- v1.0 (2025-11-18): Initial design document
