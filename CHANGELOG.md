# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Entries below `v0.2.4-beta.0` predate the rename to NXRemoteAPI (see
[README's Credits & project history](README.md#credits--project-history)) and were
reconstructed from commit history rather than written by hand at release time.

## [Unreleased]

### Added

- Album-override launcher for patched titles (experimental, WIP) ([ae21bf1](https://github.com/rafaelvieiras/NXRemoteAPI/commit/ae21bf1))
- Split the sysmodule into a minimal orchestrator + core so a bad redeploy no longer needs a physical reboot ([1b36376](https://github.com/rafaelvieiras/NXRemoteAPI/commit/1b36376))
- Sleep wired up in Home Assistant via `spsm EnterSleep`, with socket recovery after wake ([fb8d4d5](https://github.com/rafaelvieiras/NXRemoteAPI/commit/fb8d4d5))
- `GET /info` now reports live heap stats (`mallinfo()`) alongside process memory
- `enable_screenshot` / `enable_input` config flags to skip `capssc`/`hiddbg` init entirely for consumers that don't need them

### Changed

- **Project permanently forked and renamed to NXRemoteAPI** — no longer tracked against upstream; see the README for the full rationale
- Repository restructured into a monorepo (`firmware/{common,core,orchestrator,companion-app}`, `custom_components/`, `docs/`)
- Shared firmware code (`ConfigManager`, `Logger`, HTTP framing) pulled out into `firmware/common/` and made host-testable (Catch2 suite, ASan/UBSan)
- `SocketInitConfig` TCP buffers doubled, using previously unused headroom in the fixed heap (not yet validated on real hardware)
- Auto-fix and formatting pass with Ruff ([41bc71f](https://github.com/rafaelvieiras/NXRemoteAPI/commit/41bc71f))

### Fixed

- `self_restart` now actually reloads core instead of silently no-oping ([7542f5f](https://github.com/rafaelvieiras/NXRemoteAPI/commit/7542f5f))
- Launch storage resolution, actually-running title reporting, and split-TCP-segment POST bodies ([ade4e04](https://github.com/rafaelvieiras/NXRemoteAPI/commit/ade4e04))
- Installed titles are now listed via `ncm` instead of the unreliable `ns` `ApplicationRecord` API ([a002d6b](https://github.com/rafaelvieiras/NXRemoteAPI/commit/a002d6b))
- Reboot/shutdown now use `spsm` instead of `bpc` ([92ca259](https://github.com/rafaelvieiras/NXRemoteAPI/commit/92ca259))
- Home Assistant version attributes held a version string instead of the JSON field name ([80c3655](https://github.com/rafaelvieiras/NXRemoteAPI/commit/80c3655))
- Stack corruption in firmware version reporting ([af89500](https://github.com/rafaelvieiras/NXRemoteAPI/commit/af89500))
- HTTP hangs traced to undersized TCP buffers, not the threading model ([d336b18](https://github.com/rafaelvieiras/NXRemoteAPI/commit/d336b18))
- `num_bsd_sessions` raised — 3 left no headroom for HTTP clients ([16e0ed2](https://github.com/rafaelvieiras/NXRemoteAPI/commit/16e0ed2))
- Native worker-thread pool dropped in favor of a single-threaded server with `Connection: close` ([b51c95b](https://github.com/rafaelvieiras/NXRemoteAPI/commit/b51c95b), [391c014](https://github.com/rafaelvieiras/NXRemoteAPI/commit/391c014))
- `SO_RCVTIMEO`/`SO_SNDTIMEO` replaced with an explicit `select()` ([86a7b18](https://github.com/rafaelvieiras/NXRemoteAPI/commit/86a7b18))
- `write()` now bounded by a socket timeout, not just the first `recv()` ([1fb68b3](https://github.com/rafaelvieiras/NXRemoteAPI/commit/1fb68b3))
- The long-lived applet/`am` session was dropped entirely — it crashed `omm` every 10-15 minutes on real hardware ([1cd389f](https://github.com/rafaelvieiras/NXRemoteAPI/commit/1cd389f), [4bc5d8d](https://github.com/rafaelvieiras/NXRemoteAPI/commit/4bc5d8d))
- Temperature readings no longer truncate to zero ([7a460b0](https://github.com/rafaelvieiras/NXRemoteAPI/commit/7a460b0))
- Crash on boot that prevented the HTTP server from ever starting ([2269c44](https://github.com/rafaelvieiras/NXRemoteAPI/commit/2269c44))
- `async_timeout` replaced with `asyncio.timeout` (Python 3.11+ builtin) ([c40c2ac](https://github.com/rafaelvieiras/NXRemoteAPI/commit/c40c2ac))
- Multiple exception types parenthesized for Python 3.12+ compatibility ([35d933d](https://github.com/rafaelvieiras/NXRemoteAPI/commit/35d933d))

## [0.2.4-beta.0] - 2026-06-21

### Changed

- Auto-fix and formatting pass with Ruff ([41bc71f](https://github.com/rafaelvieiras/NXRemoteAPI/commit/41bc71f))

### Fixed

- HACS release zip structure and download count reporting ([19e8b13](https://github.com/rafaelvieiras/NXRemoteAPI/commit/19e8b13), [9762bea](https://github.com/rafaelvieiras/NXRemoteAPI/commit/9762bea))
- Linter fixes ([74f6cf6](https://github.com/rafaelvieiras/NXRemoteAPI/commit/74f6cf6))

## [0.2.3] - 2026-03-22

### Changed

- Power optimization and advanced system commands (`/command`) ([0362da5](https://github.com/rafaelvieiras/NXRemoteAPI/commit/0362da5))
- Applet UI modernized with a box-drawing layout ([5c361f6](https://github.com/rafaelvieiras/NXRemoteAPI/commit/5c361f6))

### Fixed

- Sysmodule buffer overflows, missing auth on GET routes ([d42bebc](https://github.com/rafaelvieiras/NXRemoteAPI/commit/d42bebc))
- Release workflow and linter fixes ([d8387df](https://github.com/rafaelvieiras/NXRemoteAPI/commit/d8387df), [ea40099](https://github.com/rafaelvieiras/NXRemoteAPI/commit/ea40099))
- Sysmodule connection reliability ([2f0091a](https://github.com/rafaelvieiras/NXRemoteAPI/commit/2f0091a))

## [0.2.1] - 2026-03-20

### Changed

- General improvements to the sysmodule and companion app

### Fixed

- Companion app and sysmodule interaction fixes ([371f77c](https://github.com/rafaelvieiras/NXRemoteAPI/commit/371f77c), [cdf3637](https://github.com/rafaelvieiras/NXRemoteAPI/commit/cdf3637))

## [0.2.0] - 2026-03-20

Initial tagged release of the project (as `ha-NintendoSwitchCFW`): Home Assistant
integration, sysmodule, and companion app for a Nintendo Switch running Atmosphère CFW.

[Unreleased]: https://github.com/rafaelvieiras/NXRemoteAPI/compare/v0.2.4b0...HEAD
[0.2.4-beta.0]: https://github.com/rafaelvieiras/NXRemoteAPI/compare/v0.2.3...v0.2.4b0
[0.2.3]: https://github.com/rafaelvieiras/NXRemoteAPI/compare/v0.2.1...v0.2.3
[0.2.1]: https://github.com/rafaelvieiras/NXRemoteAPI/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/rafaelvieiras/NXRemoteAPI/releases/tag/v0.2.0
