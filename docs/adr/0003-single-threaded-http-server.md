# 0003. Keep core's HTTP server single-threaded

Date: 2026-08-08

## Status

Accepted

## Context

`firmware/core`'s HTTP server needed to handle more than one client
reliably (Home Assistant polling plus a manual request arriving at the same
time). Two concurrency models were tried on real hardware: a
`std::thread`-per-connection design, and a native `threadCreate()` worker
pool. The first aborted the process outright under `-fno-exceptions`
(libnx's/libstdc++'s threading path relies on exceptions in a way that isn't
available with that build flag). The second caused instant connection resets
with no crash report — worse than the problem it was meant to solve, and
harder to debug since nothing was logged before the reset.

## Decision

Keep `main.cpp`'s HTTP server deliberately single-threaded: a blocking
`accept()` calling `handle_client()` synchronously, one connection fully
handled before the next is accepted.

## Consequences

The server cannot serve two clients at once — a slow or misbehaving client
occupies the only request-handling path until it disconnects or times out
(see the tracked issue on the body-read loop's missing total deadline, which
is a direct consequence of this constraint, not a separate design flaw).
Any future attempt to add concurrency here needs new on-device evidence, not
just a theoretical argument — this ADR exists specifically so nobody
re-attempts either of the two approaches already ruled out without knowing
they were already tried and reverted.
