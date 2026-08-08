# 0006. Hijack the Album applet to launch patched titles

Date: 2026-08-08

## Status

Accepted

## Context

Launching a title that has an active patch through the normal
`pm`/`ns`-driven path doesn't resolve correctly for all patched titles — the
launch either fails or launches the unpatched base. The System Album applet,
when invoked in a specific way, does resolve the patch correctly, but only
because it's the applet the system itself uses for that flow.

## Decision

Ship a second build target of `firmware/companion-app`
(`-DLAUNCHER_BUILD`) that briefly takes over the Album applet to perform the
launch on `core`'s behalf, communicating with `core` through file-based
request/status markers (`LAUNCH_REQUEST_PATH`/`LAUNCH_STATUS_PATH`) rather
than sockets, since the launcher process never initializes networking.

## Consequences

This is the most fragile part of the system: it depends on undocumented
behavior of a system applet rather than a public, stable API, and it's
isolated into its own build target specifically so a bug in it can't take
down `core`. It currently has a known, unfixed bug — the HTTP request
immediately following a launch takeover sometimes arrives corrupted at
`core` — tracked as a bug fix, not a reason to abandon the approach, since no
alternative launch path that handles patched titles correctly has been
found. The feature is marked experimental/WIP in the changelog for this
reason.
