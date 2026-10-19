# Task Packet #006: Quickshell Standalone Extraction & Codebase Diet

## Status: IN_PROGRESS
- **Owner**: Antigravity Pair
- **Branch**: `packet/006-quickshell-standalone-extraction`
- **Related PRs**: TBD

## Description
Extract Quickshell QML configs (99 files), 8 helper scripts, and 26 test scripts from `dwm-oomaya` into a standalone companion repository `oomaya/quickshell-oomaya`. Re-architect `dwm-oomaya` into a 100% pure ChadDWM C desktop with sub-2-minute CI.

## Checklist
- [ ] 1. Extract `config/quickshell`, `scripts/dwm-quickshell-*`, `tools/dwm-quickshell-state.c`, `tests/test-quickshell*.sh` into `$HOME/.local/src/quickshell-oomaya`.
- [ ] 2. Initialize git repository in `quickshell-oomaya` with standalone `Makefile` and `README.md`.
- [ ] 3. Create GitHub repository `oomaya/quickshell-oomaya` and push initial release.
- [ ] 4. In `dwm-oomaya`: delete extracted files.
- [ ] 5. Update `dwm-oomaya/Makefile` (remove Quickshell targets from install, clean, check).
- [ ] 6. Update `scripts/dwm-shell-mode` to handle graceful installation prompt for Quickshell companion repo.
- [ ] 7. Update `.github/workflows/c-cpp.yml` to remove Quickshell triggers and jobs.
- [ ] 8. Verify locally with `make clean all`, `make check`, `make check-shell`, `make check-format`.
- [ ] 9. Open PR, achieve 100% green CI, and merge to `main`.
