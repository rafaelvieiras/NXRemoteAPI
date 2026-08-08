#!/usr/bin/env python3
"""Inserts a new dated version section into CHANGELOG.md from a pre-generated
Keep a Changelog entry body, and refreshes the compare-link footer.

Only meant to be called for stable releases (see .github/workflows/release.yml)
-- beta/nightly channels don't get their own CHANGELOG.md section, they stay
folded into [Unreleased] until a stable release cuts them in.

This fully replaces whatever was under [Unreleased] with the freshly generated
entry for the same commit range, rather than trying to merge hand-edited
Unreleased content -- the source of truth here is git history, not a manually
curated section.
"""

import argparse
import re
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True, help="e.g. 1.2.3")
    parser.add_argument("--tag", required=True, help="e.g. v1.2.3")
    parser.add_argument("--previous-tag", default="", help="Previous stable tag, for the compare link")
    parser.add_argument("--date", required=True, help="YYYY-MM-DD")
    parser.add_argument("--entry-file", required=True, help="Path to the keepachangelog-format section body")
    parser.add_argument("--repo", required=True, help="owner/name")
    parser.add_argument("--changelog", default="CHANGELOG.md")
    args = parser.parse_args()

    with open(args.entry_file, "r", encoding="utf-8") as f:
        entry_body = f.read().strip()

    if not entry_body or entry_body.startswith("_No user-facing changes._"):
        print("No user-facing changes to record in CHANGELOG.md -- skipping.")
        return

    with open(args.changelog, "r", encoding="utf-8") as f:
        content = f.read()

    marker = "## [Unreleased]"
    idx = content.find(marker)
    if idx == -1:
        print("Could not find '## [Unreleased]' in CHANGELOG.md -- aborting.", file=sys.stderr)
        sys.exit(1)

    next_section = content.find("\n## [", idx + len(marker))
    if next_section == -1:
        next_section = len(content)

    new_block = f"{marker}\n\n## [{args.version}] - {args.date}\n\n{entry_body}\n"
    content = content[:idx] + new_block + content[next_section:]

    compare_base = args.previous_tag or args.tag
    new_unreleased_link = f"[Unreleased]: https://github.com/{args.repo}/compare/{args.tag}...HEAD"
    new_version_link = (
        f"[{args.version}]: https://github.com/{args.repo}/compare/{args.previous_tag}...{args.tag}"
        if args.previous_tag
        else f"[{args.version}]: https://github.com/{args.repo}/releases/tag/{args.tag}"
    )

    if re.search(r"^\[Unreleased\]:.*$", content, flags=re.MULTILINE):
        content = re.sub(r"^\[Unreleased\]:.*$", new_unreleased_link, content, count=1, flags=re.MULTILINE)
    else:
        content = content.rstrip("\n") + f"\n\n{new_unreleased_link}\n"
    content = content.replace(new_unreleased_link, f"{new_unreleased_link}\n{new_version_link}", 1)

    with open(args.changelog, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)

    print(f"CHANGELOG.md updated with [{args.version}] - {args.date}")


if __name__ == "__main__":
    main()
