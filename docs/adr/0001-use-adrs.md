# 0001. Record architectural decisions with ADRs

Date: 2026-08-08

## Status

Accepted

## Context

By the time this repository was renamed to NXRemoteAPI, most of its
hard-won decisions already lived somewhere: a comment above `handle_client()`
explaining why the HTTP server is single-threaded, a paragraph in
`AGENTS.md` about `appletInitialize()` never being held for the process
lifetime, a line in the README about not using TLS. That's real information,
but it's scattered, and it only covers decisions that happened to get a
comment. As the project grows into a monorepo with more languages (a future
Go Prometheus bridge, a client SDK) and more contributors — including AI
agents working from a single conversation's context — decisions made once
and never written down get re-litigated, or worse, silently reversed by
someone who didn't know why they existed.

## Decision

Record architecturally significant decisions as ADRs in `docs/adr/`, using
the lightweight format in `0000-template.md`. A decision is significant if
it's expensive to reverse, forecloses a real alternative, or is likely to be
second-guessed later. Numbered sequentially, never edited after acceptance —
a changed decision gets a new ADR that supersedes the old one.

## Consequences

Some decisions will now be documented twice — once as a rule in `AGENTS.md`
(the "follow this" form) and once as an ADR (the "here's why" form). That
duplication is intentional: `AGENTS.md` stays a short operational checklist,
ADRs carry the context. Writing one costs a few minutes per decision; the
alternative is the cost already paid several times in this project's history,
where a fix or a rewrite had to rediscover a constraint on real hardware
before rules like the ones in `AGENTS.md`'s "hard invariants" section
existed.
