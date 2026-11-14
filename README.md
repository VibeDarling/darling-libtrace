# Libtrace
The library implementing os_log and friends.

## Debug switches

| Variable | Effect |
|---|---|
| `DARLING_OS_LOG_STDERR=1` | Every `os_log` message is also printed to the process's stderr as `os_log[subsystem/category] type=N: text`, where `N` is the numeric `os_log_type_t` (0 default, 1 info, 2 debug, 16 error, 17 fault). |

The variable is read once per process, on the first message. Only the exact value `1` turns it on; unset, empty or any other value leaves behaviour unchanged. Set it on the guest command, not on the launcher.

With the switch on, writes go to the process's stderr, so a closed stderr pipe can deliver SIGPIPE to the app.

Darling's logging daemon is disabled, so without this switch `os_log` output goes nowhere. The switch exists to make a silent app failure diagnosable.

The text is printed exactly as the app logged it, with no redaction, the way a terminal would show it. It can contain user data (names, paths, addresses, tokens). Redact it before pasting it into an issue, a pull request or a chat.
