#!/bin/bash

# Test script for multiline alias functionality in Vitalia Reborn
# This script verifies that the semicolon-separated alias feature works correctly

echo "Testing multiline alias functionality..."

# Check if the MUD binary exists
if [ ! -f "bin/circle" ]; then
    echo "Error: MUD binary not found. Please build the project first."
    exit 1
fi

# Start the MUD server in the background
echo "Starting MUD server..."
./bin/circle 4001 > /dev/null 2>&1 &
MUD_PID=$!

# Wait for server to start
sleep 3

# Check if server is running
if ! kill -0 $MUD_PID 2>/dev/null; then
    echo "Error: Failed to start MUD server"
    exit 1
fi

echo "MUD server started (PID: $MUD_PID)"

# Test multiline alias functionality by connecting and sending commands
{
    echo "testuser"
    echo "s"
    echo "password"
    echo "password"
    echo "M"
    echo "G"
    echo "1"
    echo ""
    echo "1"
    # Create a multiline alias
    echo "alias test look;score;time"
    # List aliases to verify it was created
    echo "alias"
    # Execute the alias
    echo "test"
    # Quit
    echo "quit"
    echo "0"
    sleep 2
} | telnet localhost 4001 > test_output.txt 2>&1

# Stop the MUD server
echo "Stopping MUD server..."
kill $MUD_PID 2>/dev/null
wait $MUD_PID 2>/dev/null

# Check test results
if grep -q "look;score;time" test_output.txt; then
    echo "✓ Multiline alias creation: PASSED"
else
    echo "✗ Multiline alias creation: FAILED"
    FAILED=1
fi

if grep -q "Atalhos definidos:" test_output.txt && grep -q "test.*look;score;time" test_output.txt; then
    echo "✓ Multiline alias listing: PASSED"
else
    echo "✗ Multiline alias listing: FAILED"
    FAILED=1
fi

# Count how many times we see character stats (should be multiple from the alias execution)
SCORE_COUNT=$(grep -c "Guerreiro.*nivel" test_output.txt)
if [ "$SCORE_COUNT" -gt 1 ]; then
    echo "✓ Multiline alias execution: PASSED (score shown $SCORE_COUNT times)"
else
    echo "✗ Multiline alias execution: FAILED (score shown $SCORE_COUNT times)"
    FAILED=1
fi

# Clean up
rm -f test_output.txt

if [ "$FAILED" = "1" ]; then
    echo ""
    echo "Some tests failed. Multiline alias functionality may have issues."
    exit 1
else
    echo ""
    echo "All tests passed! Multiline alias functionality is working correctly."
    exit 0
fi