# Task Packet: Unify dwm-oomaya remnants to dwm-oomaya

## Context
The codebase has mixed references to `dwm-oomaya` and `dwm-oomaya`, causing CI
failures and confusion. This is wasted energy. All references should be unified
to `dwm-oomaya`.

## Task
1. Search the entire codebase for `dwm-oomaya` references (grep).
2. Unify all to `dwm-oomaya` (paths, config names, state directories, etc.).
3. Update test fixtures accordingly.
4. Verify locally with `make check` before pushing (Pre-push Verification).

## Constraints
- Unify names only; do NOT change existing behavior.
- Pre-push Verification is mandatory (see AGENTS.md Testing section).
- Keep changes focused on the rename; no refactoring.

## Expected Outcome
- Zero `dwm-oomaya` references remain in the codebase.
- CI validate passes on the next push.

## Verification
- [ ] `grep -r "dwm-oomaya"` returns no results (excluding git history)
- [ ] Local `make check` passes
- [ ] Pushed commit shows validate: success on GitHub Actions
