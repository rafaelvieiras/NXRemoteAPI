# 0008. Rename the Home Assistant integration domain to `nxremoteapi`

Date: 2026-08-08

## Status

Accepted (implementation pending — see
[#31](https://github.com/rafaelvieiras/NXRemoteAPI/issues/31))

## Context

`custom_components/switch_cfw`'s domain, `CONF_*` constants, and entity
naming were previously treated as frozen (see the earlier "hard invariants"
in `AGENTS.md`), on the basis that the integration was live in a real
production Home Assistant instance with real automations depending on those
entity IDs — renaming would have required removing and re-adding the
integration, breaking those automations. `switch_cfw` itself is a leftover
of the pre-rename project name (`ha-NintendoSwitchCFW`), inconsistent with
the NXRemoteAPI rebrand (ADR-0002).

On 2026-08-08 the maintainer confirmed that premise no longer holds: the
integration is not currently in production use anywhere, and he will be its
first real user. The constraint that blocked this rename was based on a
production dependency that does not exist.

## Decision

Rename the integration's domain from `switch_cfw` to `nxremoteapi`: the
folder (`custom_components/nxremoteapi/`), `manifest.json`'s `domain` field,
`const.py`'s `DOMAIN` constant, and by consequence every `entity_id` the
integration generates. This supersedes the frozen-domain invariant in
`AGENTS.md`.

## Consequences

This is a one-time breaking change for any existing install of the
integration (the maintainer's own future instance included) — the
integration must be removed and re-added, and any automation written
against the old `switch_cfw.*` entity IDs would need updating. Since there
is currently no production instance, that cost is paid once, now, at zero
real disruption, instead of being paid later after real automations exist.
Going forward, the domain is `nxremoteapi` and is frozen again under the
same reasoning as before: a future rename would need its own explicit ADR,
not a side effect of an unrelated change.
