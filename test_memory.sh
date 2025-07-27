#!/bin/bash

echo "Testing memory leaks before and after fixes..."

# Save current zmalloc.log
mv zmalloc.log zmalloc_after_fixes.log 2>/dev/null || true

echo "Running full initialization and shutdown test..."
timeout 15 ./bin/circle -s -c 2>/dev/null || true

echo "Checking for leaks..."
if [ -f zmalloc.log ]; then
    leak_count=$(grep "mallocd at" zmalloc.log | wc -l)
    total_bytes=$(grep "leaks totalling" zmalloc.log | grep -o '[0-9]\+ bytes' | cut -d' ' -f1 || echo "0")
    
    echo "Current leaks: $leak_count allocations"
    echo "Total leaked bytes: $total_bytes"
    
    echo ""
    echo "Leak locations:"
    grep "mallocd at" zmalloc.log | sed 's/.*mallocd at //' | sort | uniq -c | sort -nr
    
    # Compare with original
    if [ -f zmalloc.log.old ]; then
        old_leak_count=$(grep "mallocd at" zmalloc.log.old | wc -l)
        old_total_bytes=$(grep "leaks totalling" zmalloc.log.old | grep -o '[0-9]\+ bytes' | cut -d' ' -f1 || echo "0")
        
        echo ""
        echo "=== COMPARISON ==="
        echo "Original leaks: $old_leak_count allocations"
        echo "Original bytes: $old_total_bytes"
        echo "Fixed leaks: $((old_leak_count - leak_count)) allocations"
        echo "Fixed bytes: $((old_total_bytes - total_bytes))"
        
        if [ $leak_count -lt $old_leak_count ]; then
            echo "✅ Memory leaks significantly reduced!"
        else
            echo "❌ No improvement in memory leaks"
        fi
    fi
else
    echo "No zmalloc.log generated"
fi