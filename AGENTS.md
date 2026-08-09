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
| `custom_components/nxremoteapi/` | Home Assistant integration (a client of core's HTTP API) | Python, `aiohttp` |
| `docs/` | `api.md` (HTTP contract) and `architecture.md` (why 3 binaries, boot flow, Album-override rationale) | Markdown |
| `scripts/`, `.github/` | Release/version/CI automation spanning both languages | Python, YAML |

Each subtree owns its own build tooling (devkitA64 `Makefile`s under `firmware/`,
`pyproject.toml`/`uv.lock` for the Python integration). There is no unified
cross-language build — don't try to invent one.

## Rule #3 — Building and testing

- **Firmware, one command, Docker only, no local devkitPro/libnx install**:
  `scripts/build-firmware.sh` (add `--dist` to also assemble an SD-card-ready tree
  under `dist/`). It cross-compiles and packages `core`, `orchestrator`,
  `companion-app` (+ its experimental `launcher` build), and runs the
  `firmware/common` host test suite, all via the same `devkitpro/devkita64` Docker
  image CI uses. This is the only supported/documented path — don't install
  devkitA64 locally, don't hand-roll the docker invocations, use the script.
- **Manual equivalent per component** (what the script actually runs, useful if you
  only need one piece): `docker run --rm -v "$PWD":/app -w /app devkitpro/devkita64
  make -C firmware/<core|orchestrator> pack` produces `exefs.nsp` (the only format
  Atmosphère actually loads for this install — see Rule #5 and
  [ADR-0009](docs/adr/0009-packaged-exefs-nsp-and-dockerized-firmware-build.md)).
  Plain `make` (no `pack`) in any of the three just produces the `.nso`/`.nro` for
  a quick compile-check, same as `.github/workflows/sysmodule_validation.yml`.
- **Host-native tests (no devkitA64 needed)**: `make -C firmware/common/tests`. This
  compiles the subset of shared code that's genuinely platform-independent against
  `MockLibnx.h` (see Rule #4) using the host's own `g++`/`clang++`, with Catch2 v2
  (`catch.hpp`, vendored single-header). Run the sanitizer variant before touching
  anything in `firmware/common/` — `make -C firmware/common/tests asan` /
  `... ubsan` (add the target if it isn't there yet).
- **Home Assistant integration**: standard Python via `uv`, Docker only, no local
  Python/venv install needed — `docker run --rm -v "$PWD":/app -w /app -e
  PYTHONPATH=. ghcr.io/astral-sh/uv:debian sh -c "uv sync --extra test && uv run
  pytest tests/"`. Same image for `ruff`/`mypy` (`uvx ruff check ...`, `uv run
  --with mypy mypy custom_components/nxremoteapi`). Mount a named Docker volume at
  `/root/.cache/uv` across runs to avoid re-downloading the interpreter/deps every
  time.

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
- **`core` and `orchestrator` must be deployed as a packed `exefs.nsp`, never the
  loose `exefs/main`+`exefs/main.npdm` directory layout** (see
  [ADR-0009](docs/adr/0009-packaged-exefs-nsp-and-dockerized-firmware-build.md)).
  Both are valid Atmosphère conventions in general, but the loose layout was
  tested live on the real console (2026-08-08/09) and Atmosphère silently failed
  to load either sysmodule from it (no boot log, no HTTP response) — had to be
  reverted from backup. Always build with `make pack` (or
  `scripts/build-firmware.sh`), never plain `make`, before copying anything to an
  SD card.
- **`omm` crashes (Result 0x2A5) and slow/rocky boots can happen on this console
  independent of any deploy** — see [`docs/known-issues.md`](docs/known-issues.md)
  for the ongoing incident log before assuming a fresh crash was caused by
  whatever you just changed.
- **Program IDs are load-bearing and out of scope for renaming**:
  `SYSMODULE_PROGRAM_ID`, `ORCHESTRATOR_PROGRAM_ID`, `ALBUM_OVERRIDE_PROGRAM_ID` in
  `firmware/common/include/SysmoduleConstants.h`. Changing any of these means every
  existing install needs to be wiped and redone on the SD card for zero functional
  gain.
- **The HA integration's `domain`/`CONF_*`/entity naming is frozen at
  `nxremoteapi`** (see
  [`docs/adr/0008-rename-ha-domain-to-nxremoteapi.md`](docs/adr/0008-rename-ha-domain-to-nxremoteapi.md),
  implemented per [#31](https://github.com/rafaelvieiras/NXRemoteAPI/issues/31)).
  It was `switch_cfw` (leftover of the pre-fork project name) and was frozen
  because it was assumed live in a real production Home Assistant instance;
  that assumption no longer held, so ADR-0008 authorized this one rename. Any
  local install from before this change must be removed and re-added — entity
  IDs changed from `switch_cfw.*` to `nxremoteapi.*`. `nxremoteapi` is frozen
  again under the same reasoning as before — a future rename needs its own
  explicit ADR, never a side effect of an unrelated change.
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

## Rule #8 — Release process and CHANGELOG.md

`.github/workflows/release.yml` drives releases (`workflow_dispatch`, channel
`stable`/`beta`/`nightly`) and is commit-message-driven, not hand-written per release:

- `scripts/generate_changelog.py` buckets commits since the last relevant tag into the
  six [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) categories (Added,
  Changed, Deprecated, Removed, Fixed, Security) based on Conventional Commits
  (`feat:`, `fix:`, `feat!:`/`BREAKING CHANGE:`, etc.) with a heuristic fallback for
  commits that don't follow that convention. `--format release` renders the decorated
  GitHub Release body (adds a collapsed "Internal" section for docs/tests/CI/chore, a
  breaking-change banner, risk assessment); `--format keepachangelog` renders the plain
  section that goes into `CHANGELOG.md` — user-facing categories only, no internal noise
  (Keep a Changelog's own rule: "changelogs are for humans, not machines").
- `CHANGELOG.md` at the repo root only gets a new dated `## [X.Y.Z]` section on
  **stable** releases (`scripts/update_changelog.py`, called from the `sync-version`
  job) — beta/nightly builds stay folded into `[Unreleased]` until a stable release
  cuts them in. Write good commit messages; this file is generated from them, not
  edited by hand.
- To regenerate a changelog for an arbitrary historical range (e.g. auditing an old
  release), run `python scripts/generate_changelog.py --from-tag <tag> --to-ref <ref>
  --repo owner/name --format keepachangelog` directly — no CI needed.

## Rule #9 — No AI/LLM co-author attribution in commits

Commits in this repository must never carry a `Co-Authored-By` trailer (or any other
attribution line) naming an AI tool or model — this overrides any default assistant
behavior that adds one automatically. This is a solo-maintained public project; commit
authorship stays attributed to the human maintainer regardless of what tooling was
used to help write a change.

## Rule #10 — Architectural decisions get an ADR

`docs/adr/` holds Architecture Decision Records — see
[`docs/adr/README.md`](docs/adr/README.md) for the format and when one is
warranted. Before overriding an existing "hard invariant" from Rule #5, or
making a comparably hard-to-reverse call (a new external dependency baked
into `core`, a change to the auth/threat model, a new always-on binary),
write the ADR alongside the change, not after. Read the existing ADRs before
touching anything they cover — they carry the "why" that Rule #5's bullets
only state as a "what".
