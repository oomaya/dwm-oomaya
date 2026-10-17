# Task Packet: Fix CI validate failure

## Context
The `validate` CI job on dwm-oomaya is failing. This started around the Fedora 44 work.
Recent commits (2026-10-17) added image-viewer/video-player support to
`scripts/dwm-default-apps` and `scripts/dwm-settings-hub`. Those changes were
tested on Peppermint VM and work correctly. The CI failure is likely pre-existing
or from earlier Fedora 44 changes, not from the 2026-10-17 script changes.

## Task
1. Check the `validate` workflow definition (`.github/workflows/`).
2. Identify what the validate job checks (likely shellcheck, shfmt, or similar).
3. Find the specific failure - get the job logs from the latest run.
4. Fix the root cause. If it's from the 2026-10-17 script changes, fix those.
   If it's pre-existing from Fedora 44 work, fix that instead.
5. Run the repo's own checks locally before pushing (Pre-push Verification):
   - shellcheck, shfmt, and any linters the repo uses
   - See AGENTS.md Testing section for the full list.

## Constraints
- Do NOT rerun or modify the existing red CI runs (38792256394, 38772005494).
- Only fix and push; let CI run fresh on the new commit.
- Keep changes minimal and focused on the validate failure.

## Expected Outcome
- `validate` job passes on the next push.
- No regressions in other jobs (analyze, quickshell-qml).

## Verification
- [ ] Local checks pass (shellcheck, shfmt, etc.)
- [ ] Pushed commit shows validate: success on GitHub Actions
