# Weather and Climate Effects on Emotions and Moods

## Overview

This document describes how the weather and climate system in Vitalia Reborn is designed to affect mob emotions and player moods. The weather system (defined in `src/weather.c`) tracks multiple environmental factors that create immersive atmospheric conditions and can influence the emotional state of characters in the game world.

## Weather System Components

The weather system tracks several environmental variables per zone:

### Core Weather Variables

1. **Temperature** (`weather->temperature`) - Current temperature in Celsius
   - Range: Varies by climate, typically climate baseline ± 15°C
   - Seasonal variation: Winter (-1.5x), Spring (-0.5x), Summer (+1.5x), Autumn (+0.5x)

2. **Pressure** (`weather->pressure`) - Atmospheric pressure in hPa (hectopascals/millibars)
   - Range: 960-1040 hPa
   - Affects: Sky conditions, precipitation probability

3. **Humidity** (`weather->humidity`) - Relative humidity as a float (0.0-1.0 representing 0%-100%)
   - Range: Climate baseline ± 0.2-0.3
   - Minimum: 0.01 (1%) to prevent zero humidity
   - Influences: Sky transitions, precipitation type

4. **Wind Speed** (`weather->winds`) - Wind velocity in meters per second
   - Range: Climate baseline × 3 to × 25
   - Seasonal variation: Stronger in summer and winter

5. **Sky Condition** (`weather->sky`) - Current weather state
   - SKY_CLOUDLESS: Clear skies
   - SKY_CLOUDY: Overcast
   - SKY_RAINING: Precipitation (rain)
   - SKY_LIGHTNING: Thunderstorm with lightning
   - SKY_SNOWING: Frozen precipitation (snow)

6. **Sunlight** (`weather->sunlight`) - Time of day lighting
   - SUN_DARK: Nighttime
   - SUN_RISE: Dawn
   - SUN_LIGHT: Daytime
   - SUN_SET: Dusk

## Emotional Impact Design

### Weather-Emotion Correlation

Based on real-world psychological research and game design principles, weather conditions should influence the **MOOD layer** (global emotional state) of mobs. The relationship layer (per-entity emotions) remains unaffected by weather, as personal relationships transcend environmental conditions.

### Recommended Emotion Modifiers

#### 1. Sky Condition Effects

**Clear Skies (SKY_CLOUDLESS)**
- **Happiness**: +5 to +10 (bright, pleasant conditions)
- **Energy/Excitement**: +3 to +5 (stimulating environment)
- **Sadness**: -3 to -5 (reduced melancholy)
- **Fear**: -2 to -3 (visible surroundings, fewer unknowns)

**Cloudy (SKY_CLOUDY)**
- **Sadness**: +2 to +4 (gloomy atmosphere)
- **Anxiety**: +1 to +3 (uncertain conditions)
- **Happiness**: -2 to -3 (diminished cheer)
- **Energy**: -2 to -3 (reduced stimulation)

**Raining (SKY_RAINING)**
- **Sadness**: +3 to +6 (melancholic atmosphere)
- **Calm/Peace**: +2 to +4 (soothing sound, except in lightning)
- **Discomfort**: +3 to +5 (wet conditions)
- **Happiness**: -3 to -5 (dampened spirits)
- **Note**: Some cultures/mobs may find rain refreshing and respond positively

**Lightning Storm (SKY_LIGHTNING)**
- **Fear**: +5 to +8 (danger from lightning)
- **Anxiety**: +6 to +10 (intense environmental threat)
- **Excitement**: +3 to +5 (for thrill-seeking personalities)
- **Discomfort**: +5 to +7 (harsh conditions)
- **Calm**: -8 to -10 (highly disruptive)

**Snowing (SKY_SNOWING)**
- **Wonder/Awe**: +3 to +5 (beauty of snowfall)
- **Discomfort**: +4 to +6 (cold, wet conditions)
- **Calm/Peace**: +2 to +4 (quiet, muffled sounds)
- **Fear**: +2 to +3 (in extreme cold, see temperature effects)

#### 2. Temperature Effects

**Very Cold (< 0°C / 32°F)**
- **Discomfort**: +5 to +8 (freezing conditions)
- **Fear**: +3 to +5 (danger of hypothermia)
- **Anger**: +2 to +4 (irritation from cold)
- **Energy**: -3 to -5 (sluggishness)

**Cold (0-10°C / 32-50°F)**
- **Discomfort**: +2 to +4 (chilly)
- **Energy**: -2 to -3 (reduced activity)
- **Alertness**: +1 to +2 (bracing effect)

**Comfortable (10-25°C / 50-77°F)**
- **Happiness**: +3 to +5 (ideal conditions)
- **Calm**: +2 to +4 (pleasant environment)
- **Discomfort**: -5 to -8 (minimal physical stress)

**Hot (25-35°C / 77-95°F)**
- **Discomfort**: +3 to +6 (heat stress)
- **Anger**: +3 to +5 (heat-induced irritability)
- **Energy**: -4 to -6 (lethargy)
- **Thirst**: +5 to +8 (increased dehydration)

**Very Hot (> 35°C / 95°F)**
- **Discomfort**: +7 to +10 (extreme heat)
- **Anger**: +5 to +8 (severe irritability)
- **Fear**: +2 to +4 (danger of heatstroke)
- **Energy**: -7 to -10 (severe lethargy)
- **Pain**: +2 to +3 (heat exhaustion)

#### 3. Humidity Effects

**Low Humidity (< 0.30 / 30%)**
- **Discomfort**: +2 to +4 (dry air, skin/throat irritation)
- **Energy**: +2 to +3 (crisp, invigorating air)

**Comfortable Humidity (0.30-0.60 / 30%-60%)**
- **Calm**: +2 to +3 (optimal conditions)
- **Discomfort**: -3 to -5 (minimal stress)

**High Humidity (0.60-0.80 / 60%-80%)**
- **Discomfort**: +3 to +5 (muggy, oppressive)
- **Energy**: -3 to -5 (draining conditions)
- **Anger**: +2 to +3 (irritability)

**Very High Humidity (> 0.80 / 80%)**
- **Discomfort**: +6 to +8 (extremely oppressive)
- **Energy**: -5 to -7 (severe fatigue)
- **Anxiety**: +2 to +4 (difficulty breathing sensation)

#### 4. Wind Effects

**Calm (< 2 m/s / < 4.5 mph)**
- **Calm**: +3 to +5 (peaceful)
- **Boredom**: +1 to +2 (lack of stimulation)

**Gentle Breeze (2-5 m/s / 4.5-11 mph)**
- **Happiness**: +2 to +4 (pleasant)
- **Calm**: +2 to +3 (refreshing)
- **Energy**: +1 to +2 (invigorating)

**Moderate Wind (5-10 m/s / 11-22 mph)**
- **Energy**: +2 to +3 (stimulating)
- **Discomfort**: +2 to +3 (windy)
- **Anxiety**: +1 to +2 (unsettling)

**Strong Wind (10-15 m/s / 22-34 mph)**
- **Fear**: +3 to +5 (dangerous conditions)
- **Discomfort**: +5 to +7 (difficult movement)
- **Anger**: +2 to +4 (frustration)
- **Anxiety**: +4 to +6 (threatening)

**Very Strong Wind (> 15 m/s / > 34 mph)**
- **Fear**: +6 to +10 (severe danger)
- **Horror**: +3 to +5 (in extreme cases)
- **Discomfort**: +8 to +10 (impossible conditions)
- **Panic**: +5 to +8 (fight-or-flight response)

#### 5. Time of Day Effects (Sunlight)

**Dawn (SUN_RISE)**
- **Hope**: +4 to +6 (new beginning)
- **Energy**: +3 to +5 (awakening)
- **Happiness**: +2 to +4 (beauty of sunrise)

**Daylight (SUN_LIGHT)**
- **Energy**: +5 to +7 (peak activity time)
- **Alertness**: +4 to +6 (visibility)
- **Confidence**: +2 to +4 (safety in light)
- **Fear**: -3 to -5 (reduced danger)

**Dusk (SUN_SET)**
- **Calm**: +3 to +5 (winding down)
- **Melancholy**: +2 to +3 (reflective)
- **Anxiety**: +1 to +2 (approaching darkness)

**Nighttime (SUN_DARK)**
- **Fear**: +3 to +6 (darkness, reduced visibility)
- **Anxiety**: +2 to +4 (unknown threats)
- **Fatigue**: +3 to +5 (natural sleep cycle)
- **Energy**: -5 to -7 (reduced activity)
- **Alertness**: -3 to -5 (drowsiness)
- **Mystery/Intrigue**: +2 to +4 (for curious types)

### Compound Effects

Weather conditions often combine to create stronger emotional impacts:

**Stormy Night (Lightning + Night + High Wind)**
- Fear: +10 to +15 (compounding effects)
- Horror: +5 to +8 (overwhelming threat)
- Anxiety: +8 to +12 (severe stress)

**Pleasant Summer Day (Clear + Comfortable Temp + Light Breeze + Daylight)**
- Happiness: +10 to +15 (ideal conditions)
- Energy: +8 to +10 (peak vitality)
- Calm: +5 to +8 (peaceful)

**Blizzard (Snowing + Very Cold + Strong Wind)**
- Fear: +12 to +18 (life-threatening)
- Discomfort: +15 to +20 (extreme conditions)
- Horror: +5 to +10 (in severe cases)

**Hot Humid Rainstorm (Rain + Hot + High Humidity)**
- Discomfort: +12 to +15 (oppressive)
- Anger: +6 to +10 (high irritability)
- Fatigue: +8 to +12 (draining)

## Implementation Guidelines

### When to Apply Weather Effects

Weather effects on emotions should be updated:
1. **Regularly**: Every game hour when `weather_and_time()` is called
2. **On Zone Entry**: When a character moves between zones with different weather
3. **On Weather Change**: When sky conditions change (clear → cloudy → raining, etc.)

### Applying Weather to Mood Layer

Weather effects should modify the **MOOD layer** (global emotional state) stored in `mob->ai_data->emotion_*` fields:

```c
// Pseudo-code example
void apply_weather_to_mood(struct char_data *ch, int zone) {
    struct weather_data *weather = zone_table[zone].weather;
    
    // Sky condition effects
    if (weather->sky == SKY_LIGHTNING) {
        adjust_emotion(ch, EMOTION_TYPE_FEAR, +7);
        adjust_emotion(ch, EMOTION_TYPE_ANXIETY, +8);
        adjust_emotion(ch, EMOTION_TYPE_CALM, -9);
    }
    
    // Temperature effects
    if (weather->temperature < 0) {
        adjust_emotion(ch, EMOTION_TYPE_DISCOMFORT, +6);
        adjust_emotion(ch, EMOTION_TYPE_FEAR, +4);
    }
    
    // Wind effects
    if (weather->winds > 15.0) {
        adjust_emotion(ch, EMOTION_TYPE_FEAR, +8);
        adjust_emotion(ch, EMOTION_TYPE_DISCOMFORT, +9);
    }
    
    // ... etc for other conditions
}
```

### Personality Trait Modifiers

Different mobs should react differently to weather based on their **emotional_profile** genetics:

- **Stoic/Resilient**: Reduced weather effects (-30% to -50%)
- **Sensitive**: Increased weather effects (+30% to +50%)
- **Nature-loving**: Positive reactions to natural weather (rain, snow, wind)
- **Urban-adapted**: Negative reactions to harsh weather
- **Aquatic races**: Positive reactions to rain, negative to dry conditions
- **Desert races**: Positive reactions to heat, negative to cold/rain
- **Northern races**: Positive reactions to cold/snow, negative to heat

### Player Notifications

Players should receive subtle hints about weather affecting NPCs:

- "The shopkeeper seems irritable in this oppressive heat."
- "The guard appears nervous as lightning flashes overhead."
- "The innkeeper seems cheerful on this pleasant spring day."
- "The old wizard shivers miserably in the cold wind."

### Balance Considerations

1. **Magnitude**: Weather effects should be moderate (±2 to ±10 per condition)
2. **Duration**: Effects should be continuous while conditions persist
3. **Stacking**: Multiple weather factors combine additively
4. **Caps**: Total weather influence should not exceed ±30 per emotion
5. **Decay**: When weather improves, emotion adjustments should decay naturally
6. **Override**: Relationship emotions (per-entity) should not be affected by weather

### Configuration Options

Recommended configuration flags:

- `CONFIG_WEATHER_AFFECTS_EMOTIONS`: Global toggle (default: ON)
- `CONFIG_WEATHER_EFFECT_MULTIPLIER`: Scale factor 0.0-2.0 (default: 1.0)
- `CONFIG_PLAYERS_AFFECTED_BY_WEATHER`: Whether PCs are affected (default: Optional)

## Technical Integration

### Files to Modify

1. **src/weather.c**: Add emotion updates in `weather_change()` function
2. **src/utils.c**: Create weather-to-emotion mapping functions
3. **src/utils.h**: Add function prototypes
4. **src/config.c**: Add configuration options
5. **src/structs.h**: Already has necessary data structures

### Existing Functions to Use

- `get_effective_emotion_toward()`: For checking current emotions
- Emotion adjustment functions: For modifying mood layer
- `zone_table[zone].weather`: For accessing weather data
- Emotion decay functions: For natural recovery

### Performance Considerations

- Weather-emotion updates should be batched during `weather_and_time()` calls
- Only update mobs that are loaded in active zones
- Cache weather-to-emotion mappings to avoid redundant calculations
- Use delta changes rather than recalculating full emotional state

## Future Enhancements

### Advanced Weather Effects

1. **Seasonal Affective Disorder**: Some mobs more affected during winter months
2. **Weather Preferences**: Individual mobs remember favorite weather conditions
3. **Adaptation**: Prolonged exposure to conditions reduces emotional impact
4. **Indoor Shelter**: Mobs indoors are partially protected from weather effects
5. **Magical Weather**: Enchanted weather has unique emotional signatures

### Climate-Based Cultures

Different climates could produce mobs with adapted emotional baselines:

- **Tropical zones**: Higher baseline happiness, lower cold tolerance
- **Arctic zones**: Higher resilience to cold, value warmth more
- **Desert zones**: Adapted to heat, fear of water/rain
- **Temperate zones**: Balanced responses to all weather

### Weather-Emotion Interactions

1. **Comfort Seeking**: Mobs seek shelter during harsh weather
2. **Mood Contagion**: Weather-induced emotions spread in groups
3. **Memory Formation**: Weather during emotional events strengthens memories
4. **Ritual Behavior**: Some mobs celebrate or fear specific weather

## References

This document is based on the emotion system described in:
- `HYBRID_EMOTION_SYSTEM.md`: Core emotion architecture
- `EMOTION_SYSTEM_TODO.md`: Implementation roadmap
- `src/weather.c`: Weather system implementation
- `src/structs.h`: Weather and emotion data structures

## Version History

- v1.0 (2024): Initial documentation of weather-emotion integration design
- Pending: Implementation of weather-emotion updates in code
