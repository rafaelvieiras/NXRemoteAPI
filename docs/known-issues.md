# Known issues (hardware-observed, unresolved)

Unlike [ADRs](adr/), this is a living log — appended to as new evidence comes in,
not frozen once "accepted". It exists because some issues here manifest only on
real hardware, intermittently, and take multiple incidents spread over time to
even characterize, let alone root-cause. Read this before treating a fresh `omm`
crash or a slow boot as caused by whatever you just deployed — it might not be.
Tracked on the issue tracker as [#34](https://github.com/rafaelvieiras/NXRemoteAPI/issues/34) — update both when new evidence comes in.

## System-level fatal errors (`nn.am.WindowSystem`, `nn.olsc.EventHandler`), likely unrelated to this project

**Status: open, evidence points away from this project entirely - stock Horizon
services crashing, not anything under `firmware/`. Tracked as
[#35](https://github.com/rafaelvieiras/NXRemoteAPI/issues/35).**

Separate from every other section in this file: `atmosphere/erpt_reports/`
(official Nintendo error-report telemetry, distinct from `crash_reports/`,
which only captures per-process aborts like `omm`'s) recorded a cluster of
**genuine system fatal errors** - the actual black-screen/QR-code error screen,
not anything this project's code path touches - during the same 2026-08-08/09
session covered elsewhere in this file:

- `2521-0521` at 22:13, `ThreadName: nn.am.WindowSystem` - **Applet Manager's
  window/UI system** crashing. This is exactly the kind of crash that presents
  as a full system freeze/black screen to a user, since `am` underlies the
  entire Home Menu/app-switching UI - a far better explanation for a
  user-reported "freeze requiring a reboot" than anything in this project.
- `2123-0011` at 21:54, 22:18, 00:02, and again on 2026-08-09 at 01:04 -
  `ThreadName: nn.olsc.EventHandler`, the **cloud save-sync service** crashing
  repeatedly. The 01:04 occurrence caused a spontaneous reboot right after the
  [#1](https://github.com/rafaelvieiras/NXRemoteAPI/issues/1) fix was uploaded
  (a live `core` exefs.nsp swap, same as the incidents in the next section) -
  another data point for that section's "does a live swap trigger anything"
  question, this time with a **different** crash class than `omm`: no
  `omm` crash report appeared at all around this reboot (`crash_reports/`
  unchanged since 2026-08-08 21:33), only this same pre-existing
  `nn.olsc.EventHandler` fatal - consistent with "unrelated to this project"
  reading below, and further separating the two crash classes.
- `2162-0004` (generic system fatal, no `ThreadName` field in this report
  category) at 21:34, 22:17, 22:32, 00:15, and 01:17 on 2026-08-09 (the second
  of the pair alongside the 01:04 `nn.olsc.EventHandler` fatal above,
  presumably the same underlying stall's tail end) - the earlier 00:15 one lines up almost
  exactly with the freeze-and-manual-reboot the user reported.
- Cross-referencing timestamps with `orchestrator_boot.log`'s own timestamped
  entries (see previous sections) - the orchestrator's `terminate_core()` wait
  stalled for **~79 real minutes** (`[TERM] TerminateProgram` at an elapsed-tick
  matching ~22:43, `[TERM] timed out waiting for exit event` only appearing
  ~79 minutes later, matching ~00:02) almost exactly bracketed by the 22:32 and
  00:02 fatals above. That's `eventWait()` - a kernel primitive with an
  explicit 200ms-per-slice timeout in our code - not returning for ~79 real
  minutes, which only makes sense if something well below our code (kernel
  scheduling, IPC, or the whole console) was genuinely stuck system-wide during
  that window, not just `core` or `orchestrator`.

None of the crashing threads (`nn.am.WindowSystem`, `nn.olsc.EventHandler`)
belong to this project - `atmosphere/fatal_errors/` (where Atmosphère logs
fatals it specifically intercepts) is empty, suggesting these are stock
Horizon-side fatals Atmosphère isn't specially handling, not something our
sysmodules triggered directly. Best-effort reading: this looks like real,
pre-existing instability in this console's OS/hardware, independent of
`core`/`orchestrator` - though running two extra always-on background
sysmodules for months can't be fully ruled out as a contributing factor
(resource/handle pressure) without more data. Not confirmed either way -
record any further `erpt_reports` entries here (`ErrorCode`, `ThreadName`,
timestamp) alongside whatever else was going on at the time.

## `omm` crashes (Result 0x2A5, User Break) recurring independent of any deploy

**Status: open, not root-caused. Evidence so far points away from "caused by our
code/deploys" and toward a pre-existing, intermittent characteristic of this
console's install.**

One specific cause of this crash class is fixed and understood — see AGENTS.md
Rule #5: holding `appletInitialize()` for `core`'s process lifetime crashed `omm`
every 10-15 minutes, fixed by scoping applet sessions tightly. That fix is in
production and not in question here.

What's still open is a *different* recurrence of the same crash signature
(`Process Name: omm`, `Program ID: 0100000000000045`, `Result: 0x2A5`), showing up
around console reboots, unrelated to the applet-session bug above:

- **2026-08-08, ~20:36** — occurred during/after a failed firmware deploy test (see
  [ADR-0009](adr/0009-packaged-exefs-nsp-and-dockerized-firmware-build.md)): a
  *running* sysmodule's on-disk `exefs.nsp`/`exefs/` files were overwritten right
  before a reboot, and the reboot+shutdown were also slower than usual (~1 min).
  Working hypothesis at the time: overwriting a running sysmodule's files right
  before reboot triggers this.
- **2026-08-08, ~21:14** — occurred again, on a reboot where **zero files had been
  touched beforehand** (only read-only checks: `curl` GET requests, FTP `LIST`/
  download). The same boot session's `ha_orchestrator_boot.log` also showed a rocky
  start (see next section). This is evidence *against* the "overwriting live files
  causes it" hypothesis from the first incident — the crash recurred with no file
  changes at all.
- **2026-08-08, ~21:33** — occurred a third time, immediately preceding the reboot
  that loaded the corrected, properly-packed `exefs.nsp` build (the fix for the
  incident in [ADR-0009](adr/0009-packaged-exefs-nsp-and-dockerized-firmware-build.md)).
  This reboot was again reported as unusually slow (~2 minutes, vs. a normal
  reboot and vs. the fast reboot observed after incident #1's revert). File
  changes (the new `exefs.nsp` for both `core` and `orchestrator`) had been
  uploaded beforehand — this incident matches incident #1's pattern (file swap +
  slow reboot + crash) and re-weakens the "unrelated to file changes" reading from
  incident #2. Despite the crash and slow boot, the console came back up clean:
  `core` answered `/info` immediately, `app_version` reflected the new build, and
  the orchestrator's boot log showed only a single non-escalating health-check
  miss (`(1/3)`, no full restart cycle) — much better than incidents #1/#2's
  multi-restart boot sequences.

Net read after three data points: inconclusive, leaning slightly back toward "a
file swap of a running sysmodule right before reboot increases the odds of this,
but it's not the only trigger" (2 of 3 incidents had a file change beforehand and
a slow reboot; 1 of 3 had neither). Could also simply be that slow boots and this
crash share a common cause neither is entirely triggering. The user has since
enabled additional boot logging on the console specifically to capture more
detail on future occurrences — update this entry when that data comes in, and
keep recording every occurrence here (date, what preceded the reboot, boot
duration, cold vs. warm) rather than reading too much into any single incident.

## `core` going unreachable mid-runtime (not just at boot), no crash report at all

**Status: open, but with a concrete working hypothesis this time — likely
explained by the already-filed [#9](https://github.com/rafaelvieiras/NXRemoteAPI/issues/9)
("HTTP body-read loop has no total deadline, letting a slow client starve the
single-threaded server").**

Distinct from both sections above: this is `core` going unreachable *after*
having booted cleanly and served traffic successfully for a while, not during
startup, and with **no crash report generated at all** (neither `omm` nor
anything under `core`'s own program ID) — consistent with a hang, not a crash.

- **2026-08-08/09, boot session `[27]`** (the boot right after the corrected
  `exefs.nsp` redeploy — see previous section, incident #3): `core` answered
  `/info` normally for the first ~18 minutes (manually confirmed working at
  `uptime: 75s`, and HA's coordinator kept polling successfully). Then, per
  `orchestrator_boot.log`: at `[1110]`s (~18.5 min) `core` went unreachable, hit
  3 consecutive health-check failures by `[1141]`s (~19 min), got terminated
  (`pm:shell` exit-event wait timed out, cleaned up anyway) and relaunched as a
  new pid. Two more isolated, non-escalating health-check misses followed
  around the `[1216]`-`[1231]`s and `[2401]`-`[2506]`s marks (~20 min and ~40-42
  min) before a physical reboot ended that session. No crash report exists for
  any of this — `core` simply stopped responding to the orchestrator's loopback
  health check.

A hang with no crash report is exactly what a single-threaded, blocking
`accept()`/`handle_client()` loop (see `AGENTS.md` Rule #5 and #9's own
description) would produce if some client's request never completed a full
`Content-Length` body — the whole server blocks forever on that one connection,
answering nothing else, including the orchestrator's own health-check
connection. This doesn't yet identify *which* client/request triggered it (HA's
coordinator, the companion app, or something else polling), but the timing
(spontaneous, mid-session, no crash) fits #9 far better than it fits the `omm`
crash pattern above. Treat this as supporting real-world evidence for #9 rather
than a new, separate mystery - if #9 gets fixed (a total read deadline), check
whether this specific symptom (long unexplained `core` hangs) stops recurring.

## Orchestrator sometimes needs multiple relaunch cycles to stabilize `core` on boot

**Status: open, not root-caused. Possibly related to the `omm` instability above,
possibly a separate network/timing issue.**

`firmware/orchestrator` launches `core` and kills+relaunches it after 3
consecutive failed health checks (`docs/architecture.md`, "Why three binaries").
On at least two separate 2026-08-08 boot sessions, `ha_orchestrator_boot.log`
showed `core` needing more than one such cycle before staying up — e.g. one
session logged `core` launched as pid `8A`, hit 3 consecutive health-check
failures (~640s/655s/673s marks), got terminated and relaunched as pid `8B`,
failed the same way again, relaunched as pid `8C`, and only then settled into
isolated non-escalating "(1/3)" blips (no further full restarts) before
stabilizing.

Not yet isolated whether this is: the console's Wi-Fi/network genuinely taking
longer than the orchestrator's health-check grace period to come up on some cold
boots (a false-positive kill of an actually-fine `core`), or symptomatic of
whatever is causing the `omm` crashes above. Record further occurrences here with
timestamps and whether the boot was cold (power off) vs. warm (Hekate reboot).

- **2026-08-08, ~21:34** (boot after the corrected-`exefs.nsp` redeploy, ~2 minute
  boot time per the user, preceded by an `omm` crash — see previous section) —
  only a **single** non-escalating health-check miss (`(1/3)`), no full
  terminate+relaunch cycle at all, despite this being the slowest boot of the day
  and the one immediately following a crash. Doesn't fit a simple "slow boot ⇒
  rocky orchestrator startup" story either — the two roughest boot sequences
  (pid `8A`→`8B`→`8C`) happened on the *other* two incidents, not this one.
- **A later reboot the same day, boot session `[28]`** — rocky again: `core`
  (first pid) hit 3 consecutive health-check failures by `[79]`s, got
  terminated and relaunched (second pid), then took two more isolated misses
  (`[92]`s, `[107]`s: `(1/3)`, `(2/3)`) before finally answering `/info`
  successfully at `uptime: 152s`. No `omm` crash this time at all — this
  session's instability is boot-time-only and resolved on its own via the
  orchestrator's relaunch, same mechanism [architecture.md](../architecture.md)
  describes it existing for. Reinforces that boot-time instability and the
  `omm` crash are likely two separate phenomena that don't always co-occur.
