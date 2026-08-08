# 0007. Adopt Keep a Changelog + Conventional Commits for releases

Date: 2026-08-08

## Status

Accepted

## Context

The pre-rename release pipeline generated a decorated GitHub Release body
from an ad-hoc, 12-category commit classifier, with no persistent
`CHANGELOG.md` at all. Switch homebrew's aggregator landscape (hb-appstore/
libget and similar) turned out to have no formal changelog schema of its
own — the surface that actually matters to installers/updaters is the GitHub
Release itself. The choice was between adopting third-party release tooling
(e.g. release-please) wholesale, or retrofitting the existing bespoke
pipeline to a recognized human-facing standard.

## Decision

Adopt [Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/) for
`CHANGELOG.md`'s structure (the six categories, dated sections, compare-link
footer), sourced from Conventional Commits (`feat:`, `fix:`, `feat!:`/
`BREAKING CHANGE:`) with a heuristic fallback for commits that don't follow
that convention. Keep the existing bespoke Python pipeline rather than
switching to third-party tooling, but have it render two outputs from one
shared commit-bucketing pass: a plain `--format keepachangelog` section for
`CHANGELOG.md` (user-facing categories only), and a decorated
`--format release` body for the GitHub Release (adds a collapsed "Internal"
section, a breaking-change banner). `CHANGELOG.md` only gets a new dated
section on stable releases — beta/nightly stay folded into `[Unreleased]`.

## Consequences

Changelog quality now depends directly on commit message quality — a commit
that doesn't follow Conventional Commits still gets bucketed (via the
heuristic fallback), but less precisely, and the heuristic needs upkeep as
new patterns show up. `CHANGELOG.md` and the GitHub Release body are
guaranteed to agree on categorization, since they come from the same
bucketing pass, but that also means a categorization bug affects both
outputs at once rather than just one. Historical entries below the rename
were reconstructed from commit history rather than written by hand at
release time, and are marked as such in `CHANGELOG.md`.
