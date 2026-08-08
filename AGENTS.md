# AGENTS.md — NXRemoteAPI

Instructions for AI agents (and human contributors) working in this repository.

NXRemoteAPI is a low-memory-footprint remote API for Nintendo Switch consoles running
Atmosphère CFW: telemetry, automation, and remote control, exposed over plain HTTP.
Home Assistant is one client of that API, not the product itself — see
[`docs/architecture.md`](docs/architecture.md) for the full rationale and
[`docs/api.md`](docs/api.md) for the HTTP contract.

## Rule #1 — Everything in this repo is English

Code, comments, commit messages, README, `docs/*.md`, issue/PR text — all English, no
exceptions, even if the conversation that produced a change happened in another
language. This is a FOSS project; the repository is the one place language-locale
preferences never apply.

## Rule #2 — Read before you touch: architecture map

| Path | What it is | Language / toolchain |
|---|---|---|
| `firmware/common/` | Shared code: `ConfigManager`, `Logger`, `SysmoduleConstants.h`, vendored `json.hpp`, and the host-native test suite (`tests/`) | C++17, host-testable subset |
| `firmware/core/` | "core" — the always-on HTTP API sysmodule (the actual product) | C++17, devkitA64/libnx |
| `firmware/orchestrator/` | Minimal supervisor launched by Atmosphère's boot2; launches core, health-checks it, handles `/self_restart` so a bad core deploy doesn't need a physical reboot | C++17, devkitA64/libnx, `-lnx` only (no curl/mbedtls/z — keep it that way) |
| `firmware/companion-app/` | `.nro` config/GUI app, plus a second build (`-DLAUNCHER_BUILD`) that briefly overrides the Album applet to launch patched titles correctly | C++17, devkitA64/libnx |
| `custom_components/switch_cfw/` | Home Assistant integration (a client of core's HTTP API) | Python, `aiohttp` |
| `docs/` | `api.md` (HTTP contract) and `architecture.md` (why 3 binaries, boot flow, Album-override rationale) | Markdown |
| `scripts/`, `.github/` | Release/version/CI automation spanning both languages | Python, YAML |

Each subtree owns its own build tooling (devkitA64 `Makefile`s under `firmware/`,
`pyproject.toml`/`uv.lock` for the Python integration). There is no unified
cross-language build — don't try to invent one.

## Rule #3 — Building and testing

- **Firmware (cross-compiled, requires devkitPro/devkitA64 + libnx)**: `make` inside
  each of `firmware/core/`, `firmware/orchestrator/`, `firmware/companion-app/`
  (the latter also has a `launcher` target). CI builds these via the
  `devkitpro/devkita64` Docker image — see `.github/workflows/sysmodule_validation.yml`
  for the exact invocation if you don't have the toolchain installed locally.
- **Host-native tests (no devkitA64 needed)**: `make -C firmware/common/tests`. This
  compiles the subset of shared code that's genuinely platform-independent against
  `MockLibnx.h` (see Rule #4) using the host's own `g++`/`clang++`, with Catch2 v2
  (`catch.hpp`, vendored single-header). Run the sanitizer variant before touching
  anything in `firmware/common/` — `make -C firmware/common/tests asan` /
  `... ubsan` (add the target if it isn't there yet).
- **Home Assistant integration**: standard Python — `pytest tests/`, `ruff`, `mypy`
  per `pyproject.toml`/`mypy.ini`.

## Rule #4 — The `UNIT_TEST` pattern for host-testable firmware code

Anything under `firmware/common/include/` that wants host test coverage guards its
libnx dependency like this (see `ConfigManager.h` for the canonical example):

```cpp
#ifdef UNIT_TEST
#include "MockLibnx.h"
#else
#include <switch.h>
#endif
```

`firmware/common/tests/MockLibnx.h` stubs just enough of libnx's types/functions to
link. When extracting new pure logic out of `firmware/core/main.cpp` (HTTP framing,
auth checks, anything that's really just string/byte manipulation), follow this same
pattern rather than trying to compile the real device-only code on the host. Add the
new mock symbols you need to `MockLibnx.h` rather than weakening the guard.

## Rule #5 — Hard invariants: do not change without understanding why first

These exist because of hard-won, on-hardware debugging — read the surrounding
comments in the source before "fixing" them:

- **`firmware/core/main.cpp`'s HTTP server is deliberately single-threaded**
  (blocking `accept()` calling `handle_client()` synchronously). Both a
  `std::thread`-per-connection design and a native `threadCreate()` worker pool were
  tried and reverted after making things *worse* on real hardware (the former aborts
  the process outright under `-fno-exceptions`; the latter caused instant connection
  resets with no crash report). Don't reintroduce concurrency here without new
  on-device evidence — see the comment block above `handle_client()`.
- **`appletInitialize()` must never be held for the sysmodule's process lifetime.**
  Doing so crashed `omm` on real hardware every 10-15 minutes (Atmosphère Result
  0x2A5, User Break). It's only opened tightly around the few actions that genuinely
  need an applet session (`launch_app`, `/sleep`), then closed immediately. `apm` is
  used instead for anything that doesn't strictly need it (e.g. dock-state queries).
- **Program IDs are load-bearing and out of scope for renaming**:
  `SYSMODULE_PROGRAM_ID`, `ORCHESTRATOR_PROGRAM_ID`, `ALBUM_OVERRIDE_PROGRAM_ID` in
  `firmware/common/include/SysmoduleConstants.h`. Changing any of these means every
  existing install needs to be wiped and redone on the SD card for zero functional
  gain.
- **`custom_components/switch_cfw`'s HA `domain` and `CONF_*`/entity naming are
  frozen.** This integration is live in a real production Home Assistant instance
  with real automations built on those entity IDs. Renaming the domain is a real,
  disruptive migration (users would need to remove and re-add the integration) —
  never do it as a side effect of an unrelated change; it needs its own explicit,
  scoped decision.
- **The Album-override launcher (`firmware/companion-app`'s `LAUNCHER_BUILD`,
  `request_launch_via_album()` in `firmware/core/main.cpp`) has a known, unfixed bug**:
  the HTTP request immediately following a launch takeover intermittently arrives
  corrupted at core (`{"error": "Missing action"}` for a well-formed payload). Not yet
  root-caused — see the comment near `ALBUM_OVERRIDE_PROGRAM_ID`. If you're extracting
  HTTP framing logic into `firmware/common/` for testing (see Rule #4), this is
  probably where a real bug is hiding — write a test that reproduces it before
  assuming a fix works.

## Rule #6 — Low memory footprint is a design goal, not an afterthought

`firmware/core` and `firmware/companion-app` (launcher build) both run inside a fixed
2MB heap set in `__libnx_initheap()`. `firmware/orchestrator` links only `-lnx` (no
curl/mbedtls/zlib) on purpose, since it never talks to the network beyond loopback —
treat that as the template for "does this binary actually need this library" whenever
you touch linkage. Before adding a dependency, a service init call, or growing a
buffer in the always-on `core` process, check `mallinfo()`/`svcGetInfo()` output (see
`GET /info` in `firmware/core/main.cpp`) before and after — this project's whole
premise is that a background service watching a game console should stay light.

## Rule #7 — Commit message / comment culture

This codebase has a strong existing convention of comments that explain *why*, not
*what* — especially documenting failed approaches and the on-hardware symptom that
proved them wrong (see almost any function in `firmware/core/main.cpp`). Keep doing
that for anything non-obvious; don't strip these comments during refactors, and don't
replace them with generic "what this does" comments — the identifiers already say
what; the value here is the war story.

## Rule #8 — No AI/LLM co-author attribution in commits

Commits in this repository must never carry a `Co-Authored-By` trailer (or any other
attribution line) naming an AI tool or model — this overrides any default assistant
behavior that adds one automatically. This is a solo-maintained public project; commit
authorship stays attributed to the human maintainer regardless of what tooling was
used to help write a change.
