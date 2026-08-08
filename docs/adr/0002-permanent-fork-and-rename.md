# 0002. Permanently fork and rename to NXRemoteAPI

Date: 2026-08-08

## Status

Accepted

## Context

This project started as a fork of `FaserF/ha-NintendoSwitchCFW`: a
Home-Assistant-first integration for a Nintendo Switch running Atmosphère
CFW. Substantial structural work happened on top of upstream — splitting the
sysmodule into an orchestrator+core pair, pulling shared logic into a
host-testable layer, generalizing the HTTP API into something meant to
outlive any single client, reorganizing everything into a monorepo. Two
paths existed: keep rebasing/merging against upstream (tracking a project
whose direction and scope diverged further with every change), or cut the
tie formally.

## Decision

Create an independent GitHub repository (`rafaelvieiras/NXRemoteAPI`),
pushing the full existing commit history rather than starting from an empty
repo — the history includes real, hard-won hardware debugging that stays
valuable. Rebrand across code, docs, and CI. Credit FaserF's original work in
the README rather than maintaining a fork relationship that no longer
reflects reality.

## Consequences

No more upstream merges or `git fetch upstream` — any future feature FaserF's
project ships has to be re-implemented here deliberately, not inherited for
free. The `upstream` git remote stays configured locally as a reference, not
as an active sync target. The project's own release/versioning, issue
tracker, and CI now run independently. The Home Assistant integration keeps
working as one client of the API, unaffected by the rename at the protocol
level.
