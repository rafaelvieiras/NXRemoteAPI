# 0009. Package sysmodule content as a PFS0 `exefs.nsp`, build it via Docker

Date: 2026-08-09

## Status

Accepted

## Context

`firmware/core` and `firmware/orchestrator` are boot2 sysmodules loaded by
Atmosphère from `atmosphere/contents/<program_id>/`. Atmosphère accepts two
layouts there: a single packed `exefs.nsp` (PFS0 archive containing `main` +
`main.npdm`), or a loose `exefs/` directory holding the same two files
separately - both are documented, valid Atmosphère conventions, and this
repo's own README described the loose layout (inherited from upstream
FaserF).

The console's actual installed content had always been the packed
`exefs.nsp` form, produced by an ad-hoc local Python PFS0 packer that was
never committed to this repository - a gap only discovered while trying to
redeploy the 2026-08-08 rename/quality-fixes session's build. Lacking that
packer, the loose `exefs/main` + `exefs/main.npdm` layout was tried instead
on 2026-08-08/09 as a same-format substitute. It was tested live on the real
console: neither `core` nor `orchestrator` produced a boot log or answered
HTTP after reboot - Atmosphère silently failed to load either sysmodule from
that layout on this install. The attempt was reverted (restored from a
pre-change backup, verified byte-identical via MD5) after a fast follow-up
reboot confirmed the revert restored normal operation. A `omm` crash and a
slower-than-usual shutdown were observed around the failed attempt, most
likely from overwriting a *running* sysmodule's on-disk files right before
reboot (the established, previously-safe workflow for every prior deploy in
this project's history) rather than from the loose layout itself - but this
was not conclusively isolated.

Separately, `main.npdm` for both `core` and `orchestrator` was a committed
*binary* with no build rule to regenerate it - `main.json` (the actual
human-editable source) was committed too, and a one-off, core-only,
never-wired-into-any-build `scripts/gen_npdm_json.py` was the only trace of
how it had once been produced. `firmware/companion-app`'s `launcher` build
target already had the correct pattern (`launcher_main.npdm: launcher_main.json`
via `npdmtool`) - core and orchestrator just never got the same treatment.

## Decision

1. Always package `core` and `orchestrator` as `exefs.nsp` for deployment -
   never the loose `exefs/` layout, regardless of it being valid Atmosphère
   syntax in general; it's confirmed not to work on this install.
2. Use devkitPro's own `build_pfs0` tool (already bundled in the
   `devkitpro/devkita64` Docker image at `/opt/devkitpro/tools/bin/build_pfs0`)
   to produce it - not a custom/reimplemented packer. `firmware/core/Makefile`
   and `firmware/orchestrator/Makefile` each gained a `main.npdm: main.json`
   rule (matching the companion-app launcher pattern) and an `exefs.nsp` /
   `pack` target that stages `main` + `main.npdm` into a throwaway
   `pfs0_staging/` directory and runs `build_pfs0` on it.
3. Add `scripts/build-firmware.sh`, a single Docker-only entry point that
   cross-compiles and packages all of `firmware/` (`core`, `orchestrator`,
   `companion-app` + its experimental `launcher` build) and runs the
   `firmware/common` host test suite, with an optional `--dist` flag that
   assembles an SD-card-ready tree. No local devkitPro/libnx install is
   required or supported as a documented path - see AGENTS.md Rule #3.
4. `firmware/orchestrator/boot2.flag` (a presence-only marker file, content
   irrelevant to Atmosphère) is now committed as a template instead of being
   something an installer has to create by hand.

## Consequences

The full firmware build+package pipeline is reproducible from a clean clone
with one command and zero manually-maintained artifacts, closing the gap
that caused the 2026-08-08/09 incident in the first place. `main.npdm` stays
committed (host-only npdmtool output, not worth requiring Docker just to
read it), but is now regenerable - if `main.json` changes (a new
`program_id`, permission, etc.), `make main.npdm` picks it up instead of
silently going stale. The loose `exefs/` layout finding is specific to this
console's Atmosphère/Hekate setup as tested; it is not a general claim that
the layout is broken everywhere, and should be re-verified before relying on
it again elsewhere. README's install instructions and AGENTS.md Rule #5
gained this as a hard invariant - see the entries there instead of
duplicating the reasoning.
