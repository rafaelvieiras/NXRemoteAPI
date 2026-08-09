# Known issues (hardware-observed, unresolved)

Unlike [ADRs](adr/), this is a living log — appended to as new evidence comes in,
not frozen once "accepted". It exists because some issues here manifest only on
real hardware, intermittently, and take multiple incidents spread over time to
even characterize, let alone root-cause. Read this before treating a fresh `omm`
crash or a slow boot as caused by whatever you just deployed — it might not be.

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
