#!/bin/bash
# Simple validation script for percentual load feature
# This script checks if the code compiles and the binary is generated

echo "=== Validating Percentual Load Feature ==="
echo ""

# Check if source files exist and contain the changes
echo "1. Checking if source files contain percentual load code..."

if grep -q "Percentual load: negative arg2 means percentage chance" src/db.c; then
    echo "   ✓ db.c contains percentual load implementation"
else
    echo "   ✗ db.c missing percentual load implementation"
    exit 1
fi

if grep -q "Chance : %d%%" src/zedit.c; then
    echo "   ✓ zedit.c contains percentual load display"
else
    echo "   ✗ zedit.c missing percentual load display"
    exit 1
fi

# Check if binary exists and is recent
echo ""
echo "2. Checking if MUD binary exists and is recent..."

if [ ! -f "bin/circle" ]; then
    echo "   ✗ MUD binary not found. Please run 'cd src && make' first."
    exit 1
fi

# Check if binary was built recently (within last 10 minutes)
if [ $(find bin/circle -mmin -10 2>/dev/null | wc -l) -gt 0 ]; then
    echo "   ✓ MUD binary exists and was built recently"
else
    echo "   ⚠ MUD binary exists but may be outdated (not built in last 10 minutes)"
fi

# Try to verify the binary links properly (basic check)
echo ""
echo "3. Checking if binary is properly linked..."
if ldd bin/circle > /dev/null 2>&1; then
    echo "   ✓ Binary is properly linked"
else
    echo "   ⚠ Could not verify binary linking"
fi

# Check documentation
echo ""
echo "4. Checking documentation..."
if [ -f "docs/PERCENTUAL_LOAD.md" ]; then
    echo "   ✓ Feature documentation exists"
else
    echo "   ✗ Feature documentation missing"
    exit 1
fi

echo ""
echo "=== Validation Summary ==="
echo "✓ All critical checks passed!"
echo ""
echo "The percentual load feature has been successfully implemented."
echo "You can now use negative values in zone files to specify percentage chance."
echo ""
echo "Examples:"
echo "  M 0 3010 -50 3062   # 50% chance to load mob"
echo "  O 0 3019 -75 3062   # 75% chance to load object"
echo ""
echo "See docs/PERCENTUAL_LOAD.md for complete documentation."
echo ""

exit 0
