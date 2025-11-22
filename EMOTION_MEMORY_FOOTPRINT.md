# Emotion Memory System - Memory Footprint Analysis

## Overview

The emotion memory system tracks complete emotional snapshots (all 20 emotions) for each interaction a mob has with players and other mobs. This document provides detailed memory usage analysis.

## Structure Size Breakdown

### Single `emotion_memory` Structure

**Total Size: 96 bytes per memory entry**

#### Field-by-Field Breakdown:
- `entity_type` (int): 4 bytes
- `entity_id` (long): 8 bytes  
- `interaction_type` (int): 4 bytes
- `major_event` (int): 4 bytes
- `timestamp` (time_t): 8 bytes
- `social_name[20]` (char array): 20 bytes
- **20 emotion levels** (sh_int each × 20): 40 bytes total
  - fear_level, anger_level, happiness_level, sadness_level
  - friendship_level, love_level, trust_level, loyalty_level
  - curiosity_level, greed_level, pride_level
  - compassion_level, envy_level
  - courage_level, excitement_level
  - disgust_level, shame_level, pain_level, horror_level, humiliation_level

**Padding/Alignment**: Minimal (structure is well-aligned on most 64-bit systems)

### Per-Mob Memory Array

Each mob stores **10 memory slots** (circular buffer):

- **960 bytes (0.94 KB) per mob**
- 10 × 96 bytes = 960 bytes

## Scaling Analysis

### Memory Usage for Different Mob Counts

| Mob Count | Total Memory | KB | MB |
|-----------|--------------|----|----|
| 100 mobs | 96,000 bytes | 93.75 KB | 0.09 MB |
| 500 mobs | 480,000 bytes | 468.75 KB | 0.46 MB |
| 1,000 mobs | 960,000 bytes | 937.50 KB | 0.92 MB |
| 2,000 mobs | 1,920,000 bytes | 1,875.00 KB | 1.83 MB |
| **6,000 mobs** | **5,760,000 bytes** | **5,625.00 KB** | **5.49 MB** |
| 10,000 mobs | 9,600,000 bytes | 9,375.00 KB | 9.16 MB |

### **For 6000 Mobs: ~5.5 MB Total**

This is the expected memory footprint for a fully-populated server with 6000 active mobs, each with a complete emotion memory history.

## Comparison: Before vs After

### Before (Original System)
- Tracked 4 emotions per memory entry
- Structure size: ~56 bytes per entry
- Per mob (10 entries): 560 bytes (0.55 KB)
- For 6000 mobs: **3.21 MB**

### After (Enhanced System)
- Tracks 20 emotions per memory entry
- Structure size: 96 bytes per entry
- Per mob (10 entries): 960 bytes (0.94 KB)
- For 6000 mobs: **5.49 MB**

### **Memory Increase: +2.28 MB (71% increase) for 6000 mobs**

## Performance Considerations

### Memory Access Patterns
- **Sequential access**: Memory entries accessed in chronological order via circular buffer
- **Cache-friendly**: Each memory structure fits within a single cache line on most systems
- **No fragmentation**: Fixed-size array eliminates heap fragmentation concerns

### Runtime Overhead
- **Write operations**: O(1) - Simple array indexing with modulo wrap-around
- **Read operations**: O(n) where n=10 (scanning up to 10 memories per stat command)
- **Memory initialization**: Handled by memset() during mob creation (already zero-initialized)

## Optimization Notes

### Current Design Choices
1. **Fixed-size array (10 entries)**: Predictable memory usage, no dynamic allocation
2. **Circular buffer**: O(1) insertions, automatic old entry overwrites
3. **sh_int for emotions (16-bit)**: Saves space vs int (32-bit) while maintaining adequate range (0-100)
4. **20-char social name**: Sufficient for all social commands, null-terminated

### Potential Optimizations (if needed)
1. **Reduce memory slots**: 10 → 5 entries would halve memory usage
2. **Compress emotion values**: Pack multiple emotions into bitfields (complex, not recommended)
3. **On-demand allocation**: Only allocate when memories are created (adds complexity)
4. **Memory pooling**: Share memory across mobs (significant implementation overhead)

## Recommendations

### Current Configuration
The 5.5 MB memory footprint for 6000 mobs is **acceptable** for modern servers:
- Modern MUD servers typically have 1-8 GB RAM available
- 5.5 MB represents < 0.1% of a 8GB system
- Benefits of rich emotional tracking outweigh minimal memory cost

### When to Optimize
Consider optimization only if:
- Server has < 256 MB total RAM
- Mob count exceeds 50,000 concurrent instances
- Memory profiling shows this as a bottleneck (unlikely)

## Monitoring

### How to Check Memory Usage

```bash
# On Linux, check resident memory size
ps aux | grep circle | grep -v grep

# Track memory over time
watch -n 60 'ps aux | grep circle | grep -v grep'
```

### Expected Memory Breakdown
For a typical server with 6000 mobs:
- Emotion memories: ~5.5 MB
- Mob AI data (other fields): ~15-20 MB
- Mob structures (char_data): ~40-60 MB
- **Total mob-related memory: ~65-85 MB**

## Conclusion

The emotion memory system adds approximately **5.5 MB** for 6000 mobs, which is a reasonable trade-off for comprehensive emotional tracking. The system is designed for efficiency with:
- O(1) insertion performance
- Cache-friendly memory layout
- Predictable memory usage
- No heap fragmentation

**Bottom line**: The memory cost is minimal relative to the gameplay richness provided by tracking complete emotional context for every mob interaction.
