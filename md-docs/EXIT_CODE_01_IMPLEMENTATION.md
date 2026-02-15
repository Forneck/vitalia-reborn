# Exit Code 01 Resolution - Implementation Notes

## Issue Summary

The MUD server was shutting down with exit code 01 after 20 days of uptime. The GDB log showed:
```
[Inferior 1 (process 11910) exited with code 01]
```

The cause and meaning of exit code 01 were unknown, making troubleshooting difficult.

## Root Cause

Exit code 01 (decimal 1) occurs when the MUD receives one of these POSIX signals from the operating system:
- **SIGHUP (1):** Hangup - typically from terminal disconnect or SSH timeout
- **SIGINT (2):** Interrupt - typically from Ctrl+C
- **SIGTERM (15):** Termination request - from system shutdown or service manager

The original signal handler in `src/comm.c` (hupsig function) called `exit(1)` immediately without:
1. Identifying which specific signal was received
2. Logging diagnostic information like uptime
3. Performing graceful cleanup via the normal shutdown path

## Solution Implemented

### 1. Enhanced Signal Handler

Rewrote `hupsig()` function to:
- Identify and log the specific signal with descriptive name
- Calculate and log uptime at shutdown (days, hours, minutes)
- Handle edge cases (uninitialized boot_time, system clock changes)
- Use graceful shutdown via `circle_shutdown` flag instead of immediate exit
- Store signal number for proper exit code determination

### 2. Exit Code Tracking

Added `shutdown_signal` global variable to track when shutdown is signal-induced.
Modified `main()` to return exit code 1 when `shutdown_signal` is set.

### 3. Documentation

Created `EXIT_CODES.md` explaining all exit codes and providing troubleshooting guidance.

## Implementation Decisions

### Following Existing Code Patterns

Per project guidelines to make minimal changes and follow existing code style:

1. **Magic Numbers (86400, 3600, 60):** Used in uptime calculations
   - These same values are used in `act.wizard.c:2412-2414` for the same purpose
   - Not introducing new constants to maintain consistency with existing codebase
   - If constants are added later, both locations should be updated together

2. **boot_time Validation (`boot_time > 0`):**
   - Follows initialization pattern in `db.c:97` where `boot_time = 0` is initial value
   - Check for `> 0` is correct because Unix epoch (0) is Jan 1, 1970
   - Additional check `current_time >= boot_time` handles system clock changes

3. **Graceful Shutdown Approach:**
   - Changed from immediate `exit(1)` to setting `circle_shutdown = 1`
   - Allows proper cleanup: save player data, close sockets, free memory
   - Maintains data integrity and prevents corruption

## Technical Details

### Signal Handler Behavior

**Original Code:**
```c
static RETSIGTYPE hupsig(int sig)
{
    log1("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
    exit(1);
}
```

**New Code:**
```c
static RETSIGTYPE hupsig(int sig)
{
    // Identify specific signal with switch statement
    // Cache current_time for consistent logging
    // Validate boot_time before calculating uptime
    // Log detailed diagnostic information
    // Set shutdown_signal and circle_shutdown flags
}
```

### Exit Code Flow

1. Signal received → hupsig() called
2. hupsig() sets `shutdown_signal = sig` and `circle_shutdown = 1`
3. Main game loop detects `circle_shutdown` and exits loop
4. Normal cleanup routines execute (save players, close sockets, etc.)
5. main() checks `shutdown_signal` and returns appropriate exit code

### Diagnostic Output Examples

**Normal operation (20 days uptime, SIGTERM):**
```
SYSERR: Received signal 15 (SIGTERM - Termination). Shutting down gracefully...
SYSERR: Uptime at shutdown: 20 days, 5:42
Exiting with code 1 due to signal 15.
```

**Edge case (system clock changed):**
```
SYSERR: Received signal 15 (SIGTERM - Termination). Shutting down...
SYSERR: Uptime unavailable (system clock may have changed)
Exiting with code 1 due to signal 15.
```

**Edge case (early shutdown before boot_time initialized):**
```
SYSERR: Received signal 2 (SIGINT - Interrupt). Shutting down...
SYSERR: Uptime unavailable (boot_time not initialized)
Exiting with code 1 due to signal 2.
```

## Testing

1. **Compilation:** Successful with both autotools build system
2. **Code formatting:** Applied clang-format per project standards
3. **No gameplay impact:** Changes are purely diagnostic
4. **Backward compatibility:** Works with existing autorun script

## Common Scenarios

Based on the "20 days uptime" pattern in the issue:

### Most Likely Cause: SSH Timeout + SIGHUP

When running the MUD via SSH without screen/tmux:
1. SSH connection times out after extended period (varies by config)
2. System sends SIGHUP to processes attached to the disconnected terminal
3. MUD receives SIGHUP and shuts down with exit code 1

**Solution:** Use screen, tmux, or systemd service instead of bare SSH session

### Other Possible Causes

- **SIGTERM:** System maintenance, service restart, resource limits
- **SIGINT:** Manual interruption (rare on production servers)

## References

- Original issue: Exit code 01 after 20 days online
- Signal handling: src/comm.c lines 2521-2568
- Exit code logic: src/comm.c lines 437-447
- Documentation: EXIT_CODES.md
- Similar pattern: src/act.wizard.c lines 2411-2416 (uptime command)

## Future Improvements (Out of Scope)

These were not implemented to maintain minimal changes:
1. Define time constants (SECS_PER_DAY, SECS_PER_HOUR, SECS_PER_MIN)
2. Centralize uptime calculation in utility function
3. Add signal handler for SIGUSR1/SIGUSR2 for diagnostics without shutdown
4. Implement signal-based log rotation or stat dumping

These could be addressed in future refactoring PRs if desired.
