# Architecture Decision Records

This directory records the architecturally significant decisions made in this
project: the ones that are expensive to reverse, that trade off against a
real alternative, or that future contributors (human or AI) would otherwise
have to reconstruct from commit archaeology.

It complements, rather than replaces, the existing documentation:

- [`../architecture.md`](../architecture.md) describes **what the system looks
  like today** — the 3 binaries, the boot flow, the HTTP contract.
- [`../../AGENTS.md`](../../AGENTS.md)'s "hard invariants" section lists
  **rules to follow** because of decisions already made.
- ADRs record **why** those rules exist: the context at the time, the
  alternatives that were tried or considered, and the trade-off that was
  accepted. When a decision changes, the old ADR is marked `Superseded` and a
  new one is added — ADRs are never edited to pretend the original reasoning
  never happened.

## When to write one

Write an ADR when a decision is hard to reverse, closes off a real
alternative, or is likely to be second-guessed later by someone who wasn't in
the conversation where it was made. Small implementation details, bug fixes,
and anything reversible in a single PR don't need one.

## Format

Copy [`0000-template.md`](0000-template.md). Number sequentially
(`NNNN-kebab-case-title.md`), never reuse or renumber. Status is one of
`Proposed`, `Accepted`, or `Superseded by ADR-NNNN`.
