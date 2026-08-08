# 0004. Split the sysmodule into orchestrator + core

Date: 2026-08-08

## Status

Accepted

## Context

The original design was a single always-on sysmodule. A bad deploy of it
(one that crashed on boot, or failed to bind its listening socket) left no
way to recover it without a physical reboot of the console, since a
crash-looping sysmodule can't restart itself in Horizon's process model.

## Decision

Split the sysmodule into two binaries: `firmware/orchestrator`, a minimal
supervisor launched directly by Atmosphère's `boot2`, and `firmware/core`,
the actual always-on HTTP API. The orchestrator launches core, health-checks
it periodically, and can terminate and relaunch it (`/self_restart`) without
requiring a console reboot. The orchestrator links only `-lnx` — no
curl/mbedtls/zlib — since it never talks to the network beyond loopback.

## Consequences

Two binaries now have to coordinate (today via a local control port and
`pm:shell`), which is more moving parts than one process. In exchange, a bad
`core` deploy is recoverable by a HTTP call instead of a trip to the console.
The orchestrator's minimal linkage is now the template referenced in
`AGENTS.md` for "does this binary actually need this library" — any future
binary added to `firmware/` should justify its dependencies against this
baseline, not the other way around.
