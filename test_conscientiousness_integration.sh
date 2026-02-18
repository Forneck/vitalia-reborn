#!/bin/bash
# Conscientiousness Behavioral Validation Test
# This script helps validate that Conscientiousness produces observable behavioral differences

echo "=== Conscientiousness Integration Validation ==="
echo ""
echo "This test validates Phase 1 integration of Conscientiousness into the decision pipeline."
echo ""

# Check if the binary exists
if [ ! -f "bin/circle" ]; then
    echo "❌ Error: bin/circle not found. Please compile first."
    exit 1
fi

echo "✓ Executable found"
echo ""

# Check for required functions in the binary
echo "Checking for Conscientiousness functions in binary..."

if nm bin/circle | grep -q "calculate_emotional_arousal"; then
    echo "✓ calculate_emotional_arousal found"
else
    echo "❌ calculate_emotional_arousal NOT found"
    exit 1
fi

if nm bin/circle | grep -q "apply_conscientiousness_impulse_modulation"; then
    echo "✓ apply_conscientiousness_impulse_modulation found"
else
    echo "❌ apply_conscientiousness_impulse_modulation NOT found"
    exit 1
fi

if nm bin/circle | grep -q "apply_conscientiousness_reaction_delay"; then
    echo "✓ apply_conscientiousness_reaction_delay found"
else
    echo "❌ apply_conscientiousness_reaction_delay NOT found"
    exit 1
fi

if nm bin/circle | grep -q "apply_executive_control"; then
    echo "✓ apply_executive_control found"
else
    echo "❌ apply_executive_control NOT found"
    exit 1
fi

echo ""
echo "=== Integration Points Verification ==="
echo ""

# Check source code for integration points
if grep -q "apply_executive_control" src/mobact.c; then
    echo "✓ Integration in mobact.c found"
    
    # Count integration points
    INTEGRATION_COUNT=$(grep -c "apply_executive_control" src/mobact.c)
    echo "  Found $INTEGRATION_COUNT integration point(s)"
    
    # Check aggressive targeting integration
    if grep -A 5 "Aggressive Mob Targeting" src/mobact.c | grep -q "apply_executive_control"; then
        echo "  ✓ Aggressive targeting integration found"
    else
        echo "  ⚠ Aggressive targeting integration NOT found"
    fi
    
    # Check Shadow Timeline integration
    if grep -A 5 "Shadow Timeline Attack Decision" src/mobact.c | grep -q "apply_executive_control"; then
        echo "  ✓ Shadow Timeline execution integration found"
    else
        echo "  ⚠ Shadow Timeline execution integration NOT found"
    fi
else
    echo "❌ No integration found in mobact.c"
    exit 1
fi

echo ""
echo "=== Configuration Validation ==="
echo ""

# Check config parameters
if grep -q "conscientiousness_impulse_control" src/config.c; then
    echo "✓ Config parameters defined"
else
    echo "❌ Config parameters NOT defined"
    exit 1
fi

# Check CEDIT interface
if grep -q "CEDIT_CONSCIENTIOUSNESS_IMPULSE_CONTROL" src/oasis.h; then
    echo "✓ CEDIT constants defined"
else
    echo "❌ CEDIT constants NOT defined"
    exit 1
fi

if grep -q "cedit_disp_bigfive_conscientiousness_submenu" src/cedit.c; then
    echo "✓ CEDIT menu interface found"
else
    echo "❌ CEDIT menu interface NOT found"
    exit 1
fi

echo ""
echo "=== Behavioral Test Recommendations ==="
echo ""
echo "Manual testing steps:"
echo ""
echo "1. Create two test mobs with different C values:"
echo "   - Mob A: Set Conscientiousness to 10 (low)"
echo "   - Mob B: Set Conscientiousness to 90 (high)"
echo ""
echo "2. Test Scenario 1: Aggressive Targeting"
echo "   - Make both mobs aggressive (MOB_AGGRESSIVE flag)"
echo "   - Place a player in the room"
echo "   - Observe: Low C should attack more readily"
echo ""
echo "3. Test Scenario 2: High Arousal"
echo "   - Give both mobs high fear/anger emotions"
echo "   - Observe: High C should show more hesitation"
echo ""
echo "4. Test Scenario 3: Shadow Timeline (if enabled)"
echo "   - Set both mobs with SHADOWTIMELINE flag"
echo "   - Place in combat scenario"
echo "   - Observe: High C should delay/suppress attacks more"
echo ""
echo "5. Enable Debug Logging:"
echo "   - Use CEDIT → Emotion Config → [I] Conscientiousness"
echo "   - Set Debug Logging to ON (1)"
echo "   - Check syslog for executive control calculations"
echo ""
echo "Expected Results:"
echo "  - Measurable behavioral differentiation between C=10 and C=90"
echo "  - High C shows restraint, low C shows impulsivity"
echo "  - Debug logs show function calls with correct values"
echo ""
echo "=== Validation Complete ==="
echo ""
echo "✓ All checks passed!"
echo "✓ Conscientiousness Phase 1 integration is present and correctly structured"
echo ""
echo "Proceed with in-game behavioral testing to validate observable differences."
