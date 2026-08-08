# Architecture

NXRemoteAPI is a low-memory remote API for a Nintendo Switch running Atmosphère CFW:
telemetry, automation hooks, and remote control over plain HTTP. It grew out of a
Home-Assistant-specific integration; Home Assistant is now one client among others
(see [`api.md`](api.md) for the stable HTTP contract every client talks to).

## Why three binaries

Horizon (the Switch's OS) and Atmosphère's process model force a split that a
single "server" process on a normal OS wouldn't need:

```
boot2 (Atmosphère)
   │
   ▼
firmware/orchestrator          firmware/core                  firmware/companion-app
(always-on supervisor)   ──▶   (always-on HTTP API)      ◀──   (.nro GUI, run on demand
 launches + health-checks       the actual product:             from the Homebrew Menu)
 core, owns /self_restart        /info, /titles, /button,
 on port 8276                    /command, /screenshot, ...
                                       │
                                       │ (Album-override handoff,
                                       │  only for patched-title launches)
                                       ▼
                                firmware/companion-app
                                (LAUNCHER_BUILD variant,
                                 runs as a real Application
                                 applet briefly)
```

- **`firmware/orchestrator`** is what boot2 actually launches. Its only jobs: start
  `core`, poll its HTTP port every 15s, and restart it (via `pm:shell`
  Terminate→drain the process-event queue→Cleanup→Launch, not a naive
  terminate-then-immediately-relaunch — see the comment above `terminate_core()` in
  `firmware/orchestrator/main.cpp` for why the naive version silently no-ops) either
  on 3 consecutive failed health checks or on a `POST /self_restart` call to its own
  tiny control port (8276). This exists so a bad `core` redeploy — or `core` crashing
  outright — never requires a physical console reboot, and so that recovery logic
  lives somewhere other than the process it's supposed to be able to recover.
- **`firmware/core`** is the actual product: the always-on HTTP server. It
  deliberately does **not** hold an `appletInitialize()` session for its lifetime
  (that crashed `omm` every 10-15 minutes on real hardware — see `AGENTS.md` Rule
  #5) and deliberately serves HTTP **single-threaded** (both `std::thread`-per-
  connection and a native thread-pool were tried and made things worse on real
  hardware — same rule). It runs in a fixed 2MB heap.
- **`firmware/companion-app`** is compiled two ways from the same `main.cpp`:
  - The default `.nro` build: a GUI run from the Homebrew Menu to show the
    console's IP/port/token, tail `core`'s and the orchestrator's boot logs, and
    self-repair/update `core`'s installed binary from GitHub releases.
  - The `-DLAUNCHER_BUILD` variant (`make launcher`): a standalone `exefs.nsp`,
    never run directly — see "the Album-override launch path" below.

## The Album-override launch path

`core`'s normal launch mechanism (`pmshellLaunchProgram`, `NcmStorageId_SdCard` /
`NcmStorageId_BuiltInUser`) works for most titles, but fails with
`NcaBaseStorageOutOfRangeC` for any title that has an installed patch/update: this
low-level path doesn't resolve base+patch NCA layering the way `am`'s own launch path
does.

The workaround: Atmosphère's content-override mechanism
(`/atmosphere/contents/<program_id>/`) works identically for real firmware titles as
it does for custom sysmodules. Album's real program ID
(`ALBUM_OVERRIDE_PROGRAM_ID`) is temporarily overridden with the `LAUNCHER_BUILD`
binary, then `core` asks `pm:shell` to launch *Album* — which briefly runs our code
instead, with a genuine `AppletType_Application` context, from which
`appletRequestLaunchApplication()` can launch the real target title correctly
(patch layering included). The two processes hand off through two files on the SD
card (`LAUNCH_REQUEST_PATH` / `LAUNCH_STATUS_PATH` in `SysmoduleConstants.h`) since
they're independent processes with no other IPC between them. `core` restores the
real Album as soon as the launcher reports a terminal status (or after a timeout).

**Known open bug**: the HTTP request `core` handles immediately after this handoff
intermittently arrives corrupted (a well-formed body gets parsed as if the action
were missing). Not yet root-caused — see the comment near `ALBUM_OVERRIDE_PROGRAM_ID`
in `SysmoduleConstants.h` and `request_launch_via_album()` in `firmware/core/main.cpp`.

## Clients

`core`'s HTTP API (documented in [`api.md`](api.md)) is the only contract clients
need. Today:

- **`custom_components/switch_cfw`** — the Home Assistant integration. Polls
  `/info` on a dynamic interval (backs off while the console appears asleep/
  unreachable, keeping entities "available" on cached data rather than flapping
  unavailable).

Planned, not yet built:

- A Go-based Prometheus exporter, meant to run in Docker on a homelab/PC, polling
  the same JSON `core` already exposes and re-serving it in Prometheus's text
  format for Grafana. When it exists, it lives under `clients/` at the repo root —
  `custom_components/switch_cfw` stays where it is (HACS requires the
  `custom_components/<domain>` directory at the repository root for the
  "integration" category; it cannot be nested).
- A simplified client SDK for third-party homebrew/off-console apps to consume the
  API without hand-rolling HTTP calls.

## Fork history

This project started as a fork of
[FaserF/ha-NintendoSwitchCFW](https://github.com/FaserF/ha-NintendoSwitchCFW). See
the root `README.md` for the credits/rationale on why it became a permanent,
independent fork rather than a set of upstream PRs.
