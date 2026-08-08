<p align="center">
  <img src="logo.png" alt="NXRemoteAPI Logo" width="150">
</p>

<h1 align="center">NXRemoteAPI</h1>
<p align="center"><b>A low-memory HTTP API and telemetry sysmodule for a Nintendo Switch running Atmosphère CFW.</b></p>

<p align="center">
  <a href="https://github.com/rafaelvieiras/NXRemoteAPI/releases"><img src="https://img.shields.io/github/release/rafaelvieiras/NXRemoteAPI.svg?style=flat-square" alt="GitHub Release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/rafaelvieiras/NXRemoteAPI.svg?style=flat-square" alt="License"></a>
  <a href="https://github.com/rafaelvieiras/NXRemoteAPI/actions/workflows/sysmodule_validation.yml"><img src="https://github.com/rafaelvieiras/NXRemoteAPI/actions/workflows/sysmodule_validation.yml/badge.svg" alt="Build & Quality"></a>
  <a href="https://hacs.xyz"><img src="https://img.shields.io/badge/HACS-custom-orange.svg?style=flat-square" alt="hacs"></a>
  <a href="CHANGELOG.md"><img src="https://img.shields.io/badge/changelog-Keep%20a%20Changelog-orange.svg?style=flat-square" alt="Changelog"></a>
</p>

---

Most Switch homebrew that talks to the outside world is built around one consumer —
usually Home Assistant. NXRemoteAPI inverts that: the console runs a small, always-on
HTTP service that exposes its state (battery, dock/sleep status, current title, storage,
heap usage) and a handful of remote-control actions (buttons, launch, reboot, shutdown)
as a plain JSON API. What reads or drives that API is not this project's concern —
today that's a Home Assistant integration bundled in this repo; tomorrow it might be a
Prometheus exporter, a shell script, or a phone app. **The API is the product. Home
Assistant is one client of it.**

That reframing drives most of the engineering decisions here:

- **Memory is the scarce resource.** The core service runs inside a fixed heap
  measured in kilobytes, permanently, in the background, on hardware whose primary
  job is running a game — not this. Every dependency, buffer, and always-on syscall
  gets weighed against that. `GET /info` exposes live heap stats (`mallinfo()`) for
  exactly this reason: you should be able to *watch* the footprint, not just trust it.
- **The HTTP contract is the real interface, not the Home Assistant integration.**
  [`docs/api.md`](docs/api.md) is written and versioned as something a client that
  isn't Python/aiohttp can implement against.
- **Correctness on real hardware over correctness in theory.** Horizon's process
  model and Atmosphère's launch pipeline produce failure modes you won't find in a
  general-purpose HTTP server tutorial (a bounded applet lifetime that crashes a
  system service if held too long, a launch path that silently mis-resolves patched
  titles). See [`docs/architecture.md`](docs/architecture.md) for the specifics and
  the dead ends that got ruled out along the way.
- **Testable where the platform allows it.** Logic that's really just string/byte
  manipulation (HTTP framing, auth checks, config parsing) is split out and covered
  by a host-native Catch2 suite, run under ASan/UBSan in CI — a small, honest slice
  of test coverage on a platform that mostly can't be unit-tested at all.

## Repository layout

```
firmware/
├── common/          shared C++ (ConfigManager, Logger, HTTP framing) + host test suite
├── core/             the always-on HTTP API sysmodule — the actual product
├── orchestrator/     boot2 supervisor: launches core, health-checks it, self-restart
└── companion-app/    .nro config/GUI app + Album-override launcher build
custom_components/switch_cfw/   Home Assistant integration (one client of core's API)
docs/                            api.md (HTTP contract) + architecture.md (why 3 binaries)
CHANGELOG.md                     every release, in Keep a Changelog format
```

Each part keeps its own native tooling (devkitA64 `Makefile`s under `firmware/`,
`pyproject.toml` for the Python integration) — see [`AGENTS.md`](AGENTS.md) for the
full map, build/test commands, and the invariants that exist because of hard-won,
on-hardware debugging.

## Installation

### 1. On the console

1. Download `main`, `main.npdm`, `boot2.flag`, and `nxremoteapi.nro` from the
   [Releases page](https://github.com/rafaelvieiras/NXRemoteAPI/releases).
2. On the SD card, create `/atmosphere/contents/010000000000CAFE/exefs/` and
   `/atmosphere/contents/010000000000CAFE/flags/`.
3. Copy `main` and `main.npdm` into `exefs/`, and `boot2.flag` into `flags/` (without
   it, the service won't be launched at boot).
4. Copy `nxremoteapi.nro` to `/switch/nxremoteapi.nro`.
5. Reboot. Launch the `.nro` from the Homebrew Menu once to see the console's IP,
   port, and generated API token.

The service (`core`) has no UI of its own by design — it's a background process, not
an app; the `.nro` exists only to surface the connection details and let you configure
it without hand-editing JSON on the SD card.

### 2. A client

The API itself needs nothing beyond an HTTP client and the token from step 1 — see
[`docs/api.md`](docs/api.md) for the full contract. If your client is Home Assistant,
this repo already includes an integration for it:

<details>
<summary><b>Home Assistant via HACS</b></summary>

[![Open your Home Assistant instance and open a repository inside the Home Assistant Community Store.](https://my.home-assistant.io/badges/hacs_repository.svg)](https://my.home-assistant.io/redirect/hacs_repository?owner=rafaelvieiras&repository=NXRemoteAPI&category=integration)

1. In HACS, open **Custom repositories** and add `rafaelvieiras/NXRemoteAPI` as an
   **Integration**.
2. Install "Nintendo Switch CFW", restart Home Assistant.
3. It should auto-discover the console on the same network; otherwise add it
   manually with the console's IP and API token.

</details>

## Security

There's no TLS, no user accounts — this is a service meant to live on a trusted home
LAN. The only gate is a per-install random **API token**, generated on first launch,
stored in `sdmc:/config/NXRemoteAPI/settings.json`, required on every route except
`GET /info` and `GET /health`. Regenerate it from the `.nro` at any time.

## Roadmap

Not built yet, but the repo layout already leaves room for these as siblings of
`custom_components/switch_cfw`:

- A Go-based Prometheus exporter (`clients/prometheus-bridge`), polling the same
  JSON `core` already serves, for anyone who'd rather graph this in Grafana than in
  Home Assistant.
- A minimal client SDK for third-party homebrew or off-console apps that want to
  talk to the API without hand-rolling HTTP calls.

## Credits & project history

This project started as a fork of
[FaserF/ha-NintendoSwitchCFW](https://github.com/FaserF/ha-NintendoSwitchCFW) — the
original sysmodule, companion app, and Home Assistant integration are FaserF's work,
and this repo wouldn't exist without it.

It's now a **permanent, independent fork**, no longer tracked against upstream. That's
not a comment on the original project's direction — it's that the scope of what changed
(splitting the sysmodule into an orchestrator+core pair, pulling shared logic into a
host-testable layer, generalizing the API into something meant to outlive any one
client, and reorganizing everything into a monorepo built for that) made merging back
upstream impractical. The Home Assistant integration keeps working as one client of the
result, same as it always did.

## License

MIT — see [LICENSE](LICENSE).
