# AI Context & Instructions for NXRemoteAPI

See [`AGENTS.md`](../../AGENTS.md) at the repo root first — it's the canonical
source for architecture, build/test commands, and hard invariants. This file
only adds Copilot/Antigravity-specific hints for the Home Assistant client.

## 🧬 Project DNA
**Repository:** `NXRemoteAPI`
**Type:** Home Assistant Custom Integration (one client of the console's HTTP API)
**Domain:** `nxremoteapi`
**Description:** A modern, high-quality Home Assistant integration for Nintendo Switch consoles running Atmosphere Custom Firmware. Monitor your console's health, track current games, and execute system commands directly from your dashboard.

## 🛠 Tech Stack & Standards
- **Core Languages:** Python (Async/Await), Home Assistant Core API
- **Toolchain:** Make
- **Dependencies:** None specified

## 📐 Coding Guidelines (Home Assistant Context)
- **Architecture:** Must adhere to modern Home Assistant architecture guidelines (`config_flow`, `coordinator`, `entity`).
- **Typing:** Strict typing is enforced. Use `mypy` annotations and avoid `Any`.
- **Asynchronous Patterns:** Operations must be non-blocking. Use `asyncio` and `aiohttp` for I/O operations. Do not use `requests` or blocking time sleeps.
- **Naming Conventions:** Follow PEP 8. Prefix internal variables appropriately. Use English for all logging and documentation.
- **Error Handling:** Use `UpdateFailed` for coordinator errors, and cleanly handle connection timeouts without logging sensitive credentials.

## 🤖 Tool-Specific Optimization

### 🐙 GitHub Copilot
- **Code Generation:** When generating entity definitions, ensure they inherit from standard HA classes (e.g., `CoordinatorEntity`). 
- **Boilerplate:** Match the existing structure of the file. Do not invent new configuration schemas if `cv.schema` or `voluptuous` are already imported.

### 🧠 Claude Code
- **Refactoring & Complex Tasks:** Before executing changes, review `__init__.py` and `manifest.json`. Map out the data flow from `config_flow` -> `Coordinator` -> `Entities`.
- **Validation:** Always verify imports and type safety. If proposing a fix for a state error, trace the variable back to the API response.

### 🚀 Google Antigravity
- **Codebase Navigation:** Begin by analyzing `custom_components/nxremoteapi/`. Search for established patterns in `api.py` or `coordinator.py` before modifying entities.
- **Testing Requirements:** When modifying logic, run or propose tests matching the existing `tests/` directory structure. 

## 🧪 Test Procedures
- **Execution:** Use standard `pytest` framework.
- **Coverage:** Mock network requests using `aioresponses` or `pytest-httpx`. Never make live API calls in tests.

## 🚫 Exclusion Rules
See `AGENTS.md`'s Rule #5 ("hard invariants") for what's actually frozen in
this repo. There is no blanket ban on `translations/`, `hacs.json`, or
`.github/workflows/` — all three have legitimately changed as part of
sanctioned work (e.g. the domain rename in ADR-0008, the release pipeline in
ADR-0007). Don't assume a path is off-limits just because it's CI/release
config; check `AGENTS.md` and the relevant ADR instead.
