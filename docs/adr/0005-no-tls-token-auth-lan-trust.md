# 0005. Plain HTTP with a per-install API token, trusted-LAN threat model

Date: 2026-08-08

## Status

Accepted

## Context

The HTTP API needs some form of access control — it exposes remote-control
actions (button input, launch, reboot, shutdown) and, depending on config,
screenshot capture. `core` runs inside a fixed 2MB heap on hardware whose
primary job is running a game, not terminating TLS connections; certificate
provisioning is also impractical on a homebrew device with no stable
identity or trusted CA relationship. The realistic deployment is a console
and a client (Home Assistant, a phone, a script) on the same home LAN.

## Decision

Serve plain HTTP, gate every route except `GET /info` and `GET /health`
behind a per-install, randomly generated `X-API-Token` header, and document
the threat model explicitly as "trusted home LAN, not a public network" —
the token protects against other devices on that LAN, not against a
compromised network path to the internet.

## Consequences

There is no confidentiality against another device already on the LAN
(traffic, including the token itself, travels in the clear), and users are
responsible for not exposing the port to the WAN (e.g. via router port
forwarding). Given that, the token's own randomness *is* the entire security
boundary of the product — a token that's guessable or brute-forceable
defeats this decision entirely regardless of how sound the LAN-trust
reasoning is. (This surfaced in practice: the token generator in use as of
the 2026-08-08 security audit produced only ~6,400 possible values from a
non-cryptographic RNG, which is tracked as a critical bug fix, not a
reason to revisit this ADR — the fix is a stronger token, not a different
model.)
