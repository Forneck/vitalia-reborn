# Vitalia Reborn Exit Codes

This document explains the exit codes returned by the Vitalia Reborn MUD server and their meanings.

## Exit Code Reference

### Exit Code 0 - Normal Shutdown
**Meaning:** Clean, intentional shutdown

**Common Causes:**
- Admin issued the `shutdown` command from within the MUD
- Syntax check mode completed (`-c` flag)
- Normal completion of operations

**What to do:** This is expected behavior. No action needed.

---

### Exit Code 1 - Signal-Induced Shutdown
**Meaning:** Server received a termination signal from the operating system

**Common Causes:**
- **SIGHUP (Signal 1):** Hangup signal - typically sent when controlling terminal is closed or SSH connection is lost
- **SIGINT (Signal 2):** Interrupt signal - typically sent when user presses Ctrl+C
- **SIGTERM (Signal 15):** Termination signal - sent by system shutdown, service managers (systemd), or `kill` command

**What to do:**
1. Check the syslog for the specific signal received and uptime at shutdown
2. Look for patterns:
   - Regular SIGHUP after 20+ days → Likely SSH timeout or session management issue
   - SIGTERM → May be system maintenance, service restart, or resource limits
   - SIGINT → Manual interruption (should not happen on production servers)
3. Review system logs (`journalctl`, `/var/log/syslog`) around the time of shutdown
4. Check for:
   - Scheduled system maintenance windows
   - SSH timeout configurations
   - Terminal multiplexer (screen/tmux) session issues
   - Systemd service timeouts or restarts

**Prevention:**
- Run the MUD under a process supervisor (systemd, supervisor, or autorun script)
- Use a terminal multiplexer (screen/tmux) for manual starts
- Configure appropriate SSH timeout settings if running via SSH
- Set up monitoring to alert on unexpected shutdowns

---

### Exit Code 52 - Reboot
**Meaning:** Server is rebooting (intentional restart)

**Common Causes:**
- Admin issued `shutdown reboot` or `shutdown now` command
- Automatic reboot after updates or maintenance

**What to do:** This is expected behavior. The autorun script will restart the server automatically.

---

## Diagnostic Information in Logs

When a signal causes shutdown (exit code 1), the server logs include:

```
SYSERR: Received signal 15 (SIGTERM - Termination). Shutting down gracefully...
SYSERR: Uptime at shutdown: 20 days, 5:42
```

This information helps identify:
- **Which signal** caused the termination
- **How long** the server was running before shutdown
- **When** the shutdown occurred (timestamp in logs)

## Historical Context

Prior to this improvement, the server would log only:
```
SYSERR: Received SIGHUP, SIGINT, or SIGTERM. Shutting down...
```

This made it difficult to diagnose the cause of unexpected shutdowns, especially when they occurred after long uptimes (e.g., 20 days as reported in issue #XX).

## Signal Handling Details

The server now:
1. **Identifies** the specific signal received
2. **Logs** detailed information including uptime
3. **Performs graceful shutdown** instead of immediate exit
4. **Allows proper cleanup** of resources and player data
5. **Returns exit code 1** to indicate signal-induced termination

This ensures data integrity while providing better diagnostics for troubleshooting unexpected shutdowns.

## For System Administrators

If you're experiencing frequent exit code 1 shutdowns:

1. **Check your process supervisor configuration:**
   ```bash
   # If using systemd
   systemctl status vitalia
   journalctl -u vitalia -n 100
   ```

2. **Review terminal session management:**
   - Ensure the MUD is not running in a bare SSH session
   - Use screen, tmux, or systemd for production deployments

3. **Monitor system events:**
   ```bash
   # Check for system signals or OOM killer
   dmesg | grep -i "killed process"
   grep -i "Out of memory" /var/log/syslog
   ```

4. **Verify SSH settings** (if applicable):
   ```
   # In /etc/ssh/sshd_config
   ClientAliveInterval 60
   ClientAliveCountMax 120
   ```

## References

- Signal handling code: `src/comm.c` (hupsig function)
- Exit code logic: `src/comm.c` (main function)
- Autorun script: `autorun` (handles automatic restarts)
