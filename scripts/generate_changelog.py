#!/usr/bin/env python3
"""Generates a changelog from git commit history, bucketed into the six
Keep a Changelog (https://keepachangelog.com/en/1.1.0/) categories.

Two renderers share the same bucketing pass:
  --format keepachangelog   plain sections for CHANGELOG.md (user-facing only)
  --format release          decorated GitHub Release body (adds a collapsed
                             "Internal" section for docs/tests/CI/chore, since
                             contributors browsing a release still want that)
"""

import argparse
import re
import subprocess
import sys

# Noise filter — commits matching ANY pattern are silently dropped entirely,
# from both renderers (not even "Internal" wants these).
NOISE_PATTERNS = [
    r"^\s*$",
    r"^(Update|Aktualisier[et]?|Add|Adds|Adde|Delete|Deletes|Remove|Removes|Rename|Renames|Move|Moves|Fix|Edit|Change|Modify)\s+[\w\-\.\/]+\.\w{1,10}\s*$",
    r"^Merge (pull request|branch|remote-tracking branch)\b",
    r"^Merge from\b",
    r"^(chore|build)(\([^)]*\))?:\s*(bump|release|version)\b",
    r"^(bump|release)(\s+version)?\s+v?\d",
    r"^v?\d+\.\d+\.\d+\s*$",
    r"^\[skip[- ]ci\]",
    r"^chore: regenerate (manifest|connections|changelog)\b",
    r"^(auto.?generated?|automated?|bot:)\b",
    r'^Revert "Revert',
    r"^Initial commit\s*$",
    r"^WIP\b",
    r"^wip\b",
    r"^.{1,3}$",
    r"\[skip[- ]ci\]\s*$",
]

# Keep a Changelog's six categories, in the order the spec lists them.
KAC_ORDER = ["added", "changed", "deprecated", "removed", "fixed", "security"]
KAC_LABEL = {
    "added": "Added",
    "changed": "Changed",
    "deprecated": "Deprecated",
    "removed": "Removed",
    "fixed": "Fixed",
    "security": "Security",
}
# Only shown in --format release, as a collapsed section for contributors —
# never written into CHANGELOG.md, per Keep a Changelog's "for humans" rule.
INTERNAL_LABEL = "Internal (CI, docs, tests, chores)"

# Conventional commit type -> bucket. Types not listed here fall through to
# the heuristic classifier below, or into "internal" if nothing matches.
TYPE_MAP = {
    "feat": "added",
    "feature": "added",
    "fix": "fixed",
    "bugfix": "fixed",
    "hotfix": "fixed",
    "revert": "fixed",
    "security": "security",
    "sec": "security",
    "perf": "changed",
    "optim": "changed",
    "ui": "changed",
    "style": "changed",
    "ux": "changed",
    "deprecate": "deprecated",
    "deprecated": "deprecated",
    "remove": "removed",
    "removed": "removed",
    "refactor": "internal",
    "refact": "internal",
    "docs": "internal",
    "doc": "internal",
    "test": "internal",
    "tests": "internal",
    "ci": "internal",
    "cd": "internal",
    "build": "internal",
    "chore": "internal",
    "maint": "internal",
    "deps": "internal",
    "bump": "internal",
}

# Scope overrides — a recognized scope wins over the commit type.
SCOPE_MAP = {
    "docs": "internal",
    "readme": "internal",
    "test": "internal",
    "tests": "internal",
    "ci": "internal",
    "workflow": "internal",
    "actions": "internal",
    "security": "security",
}

MAX_PER_SECTION = 15


def get_norm_key(msg: str) -> str:
    n = msg.lower()
    n = re.sub(
        r"^(feat|fix|docs|style|refactor|perf|test|chore|ci|security|build|ui|ux|revert|remove|deprecate)(\([^)]*\))?(!)?:\s*",
        "",
        n,
    )
    n = re.sub(r"[\.\!\?\,\;\:\"\'`]", "", n)
    n = re.sub(r"\b(the|a|an|for|of|in|to|with|from|on|at|by)\b", "", n)
    n = re.sub(r"\s+", " ", n)
    return n.strip()


def classify_heuristic(msg_lower: str) -> str:
    """Best-effort bucket for commits that aren't `type(scope): desc`."""
    if re.search(r"\bfix(es|ed)?\b", msg_lower) or any(
        w in msg_lower for w in ["bug fix", "bugfix"]
    ):
        return "fixed"
    if any(
        w in msg_lower
        for w in ["security", "vulnerability", "cve", "auth bypass"]
    ):
        return "security"
    if any(w in msg_lower for w in ["deprecate"]):
        return "deprecated"
    if msg_lower.startswith(("remove ", "removes ", "drop ", "drops ")):
        return "removed"
    if any(
        w in msg_lower
        for w in ["add feature", "added feature", "adds feature", "new feature", "add support", "introduce"]
    ) or msg_lower.startswith(("add ", "adds ", "expose ", "exposed ")):
        return "added"
    if any(w in msg_lower for w in ["perf", "speed", "faster", "optim", "lower memory", "reduce memory"]):
        return "changed"
    if any(w in msg_lower for w in ["ui", "ux", "layout", "style", "theme", "translation", "strings", "lang"]):
        return "changed"
    if any(
        w in msg_lower
        for w in ["ci", "linter", "lint fix", "pipeline", "workflow", "github action", "generate_changelog", "changelog"]
    ):
        return "internal"
    if any(w in msg_lower for w in ["update depend", "bump depend", "renovate", "dependency update", "upgrade dep"]):
        return "internal"
    if any(w in msg_lower for w in ["doc", "readme", "wiki", "guide"]):
        return "internal"
    if any(w in msg_lower for w in ["test", "spec", "unit test"]):
        return "internal"
    if "refactor" in msg_lower or "cleanup" in msg_lower or "clean up" in msg_lower:
        return "internal"
    if "improve" in msg_lower or msg_lower.startswith(("use ", "avoid ", "robust ")):
        return "changed"
    return "internal"


def get_formatted_item(display: str, hashes: list, repo: str, commit_authors: dict) -> str:
    if not hashes:
        return display
    links = []
    attributions = []
    for h in hashes:
        links.append(f"[{h}](https://github.com/{repo}/commit/{h})" if repo else f"`{h}`")
        author = commit_authors.get(h, "")
        if author:
            author_lower = author.lower()
            is_ignored = any(
                x in author_lower
                for x in [
                    "faserf",
                    "fabian",
                    "seitz",
                    "rafael",
                    "vieiras",
                    "action",
                    "bot",
                    "dependabot",
                    "renovate",
                ]
            )
            if not is_ignored:
                attributions.append(f"thanks to @{author} for this contribution!")
    hash_str = ", ".join(links)
    attr_str = f" — {', '.join(attributions)}" if attributions else ""
    return f"{display} ({hash_str}){attr_str}"


def collect_commits(from_tag: str, to_ref: str = "HEAD"):
    if from_tag:
        git_args = ["git", "log", f"{from_tag}..{to_ref}", "--pretty=format:%h %an || %s"]
    else:
        git_args = ["git", "log", to_ref, "--pretty=format:%h %an || %s", "--max-count=2000"]
    try:
        raw_output = subprocess.check_output(git_args, stderr=subprocess.DEVNULL).decode(
            "utf-8", errors="ignore"
        )
    except subprocess.CalledProcessError:
        raw_output = ""
    return [line.strip() for line in raw_output.splitlines() if line.strip()]


def bucket_commits(commit_lines):
    commit_authors = {}
    buckets = {k: [] for k in KAC_ORDER + ["internal"]}
    seen_items = {}

    for line in commit_lines:
        author = ""
        if " || " in line:
            meta, msg = line.split(" || ", 1)
            msg = msg.strip()
            meta_parts = meta.split(" ", 1)
            commit_hash = meta_parts[0]
            if len(meta_parts) > 1:
                author = meta_parts[1].strip()
        else:
            match = re.match(r"^([0-9a-fA-F]+)\s+(.*)$", line)
            commit_hash, msg = (match.group(1), match.group(2).strip()) if match else ("", line.strip())

        if commit_hash and author:
            commit_authors[commit_hash] = author

        if not msg or any(re.search(p, msg) for p in NOISE_PATTERNS):
            continue

        bucket = "internal"
        display = msg
        is_break = False

        conv_match = re.match(r"^([A-Za-z][A-Za-z0-9_-]*)(\([^)]*\))?(!)?:\s*(.+)$", msg)
        if conv_match:
            raw_type = conv_match.group(1).lower()
            raw_scope = conv_match.group(2)
            raw_scope = re.sub(r"[()]", "", raw_scope).lower().strip() if raw_scope else ""
            is_break = bool(conv_match.group(3))
            desc = conv_match.group(4).strip()

            if raw_scope and raw_scope in SCOPE_MAP:
                bucket = SCOPE_MAP[raw_scope]
            elif raw_type in TYPE_MAP:
                bucket = TYPE_MAP[raw_type]
            else:
                bucket = classify_heuristic(msg.lower())

            desc_cap = desc[0].upper() + desc[1:] if desc else desc
            display = f"**{raw_scope}:** {desc_cap}" if raw_scope else desc_cap
        else:
            display = msg[0].upper() + msg[1:] if msg else msg
            bucket = classify_heuristic(msg.lower())

        if is_break:
            display = f"**BREAKING:** {display}"
            if bucket == "internal":
                bucket = "changed"

        norm_key = f"{bucket}:{get_norm_key(display)}"
        if norm_key in seen_items:
            existing_item = seen_items[norm_key]
            if commit_hash and commit_hash not in existing_item["hashes"]:
                existing_item["hashes"].append(commit_hash)
            continue

        item = {"display": display, "hashes": [commit_hash] if commit_hash else [], "breaking": is_break}
        seen_items[norm_key] = item
        buckets[bucket].append(item)

    return buckets, commit_authors


def render_section(items, repo, commit_authors, collapse=True):
    out = []
    do_collapse = collapse and len(items) > MAX_PER_SECTION
    visible = items[:MAX_PER_SECTION] if do_collapse else items
    for item in visible:
        out.append(f"- {get_formatted_item(item['display'], item['hashes'], repo, commit_authors)}")
    if do_collapse:
        remaining = len(items) - MAX_PER_SECTION
        out += ["", "<details>", f"<summary>Show {remaining} more changes…</summary>", ""]
        for item in items[MAX_PER_SECTION:]:
            out.append(f"- {get_formatted_item(item['display'], item['hashes'], repo, commit_authors)}")
        out += ["", "</details>"]
    return out


def render_keepachangelog(buckets, repo, commit_authors) -> str:
    """Plain sections for CHANGELOG.md — user-facing categories only, no
    banners, no internal/maintenance noise, per keepachangelog.com."""
    out = []
    has_any = False
    for key in KAC_ORDER:
        bucket = buckets[key]
        if not bucket:
            continue
        has_any = True
        out.append(f"### {KAC_LABEL[key]}")
        out.append("")
        out += render_section(bucket, repo, commit_authors, collapse=False)
        out.append("")
    if not has_any:
        out.append("_No user-facing changes._")
        out.append("")
    return "\n".join(out).strip() + "\n"


def render_release(buckets, repo, commit_authors, from_tag, total_raw) -> str:
    """Decorated GitHub Release body."""
    out = []
    breaking_items = [i for i in sum((buckets[k] for k in KAC_ORDER), []) if i["breaking"]]
    if breaking_items:
        out.append("> [!CAUTION]")
        out.append("> **This release contains breaking changes. Please review before updating.**")
        out.append(">")
        for item in breaking_items:
            out.append(f"> - {get_formatted_item(item['display'], item['hashes'], repo, commit_authors)}")
        out.append("")

    has_any = False
    for key in KAC_ORDER:
        bucket = buckets[key]
        if not bucket:
            continue
        has_any = True
        out.append(f"### {KAC_LABEL[key]}")
        out.append("")
        out += render_section(bucket, repo, commit_authors)
        out.append("")

    if buckets["internal"]:
        has_any = True
        out.append("<details>")
        out.append(f"<summary>🔧 {INTERNAL_LABEL} ({len(buckets['internal'])})</summary>")
        out.append("")
        out += render_section(buckets["internal"], repo, commit_authors)
        out.append("")
        out.append("</details>")
        out.append("")

    if not has_any:
        out.append("> *No categorised changes found in this release.*")
        out.append("> Most commits were maintenance, dependency updates, or automated changes.")
        out.append("")

    filtered_count = sum(len(buckets[k]) for k in KAC_ORDER + ["internal"])
    out.append("---")
    if total_raw > 0:
        out.append(f"*{filtered_count} significant changes from {total_raw} total commits since `{from_tag}`.*")
    else:
        range_str = f"{from_tag}..HEAD" if from_tag else "all history"
        out.append(f"*Changelog generated from `{range_str}`.*")
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description="Generate a Keep a Changelog-bucketed git changelog.")
    parser.add_argument("--from-tag", default="", help="Git ref to diff against")
    parser.add_argument("--to-ref", default="HEAD", help="End of the range (default HEAD) — used to regenerate historical entries")
    parser.add_argument("--total-commits", default="", help="Total commit count input")
    parser.add_argument("--repo", default="", help="Repository identifier (owner/name)")
    parser.add_argument(
        "--format",
        choices=["release", "keepachangelog"],
        default="release",
        help="release = decorated GitHub Release body; keepachangelog = plain CHANGELOG.md section",
    )
    args = parser.parse_args()

    commit_lines = collect_commits(args.from_tag, args.to_ref)
    try:
        total_raw = int(args.total_commits) if args.total_commits else len(commit_lines)
    except ValueError:
        total_raw = len(commit_lines)

    buckets, commit_authors = bucket_commits(commit_lines)

    sys.stdout.reconfigure(encoding="utf-8")
    if args.format == "keepachangelog":
        print(render_keepachangelog(buckets, args.repo, commit_authors))
    else:
        print(render_release(buckets, args.repo, commit_authors, args.from_tag, total_raw))


if __name__ == "__main__":
    main()
