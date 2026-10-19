# Task Packet #008: Comprehensive User Manual, Verification & Docs Modernization

## Status: IN_PROGRESS
- **Owner**: Antigravity Pair
- **Branch**: `packet/008-user-manual-and-docs`
- **Related PRs**: TBD

## Description
Provide end-to-end user manual (`USER-GUIDE.md`), smoke-test matrix (`SMOKE-TEST.md`), synchronize all shortcuts across C/TOML/docs, modernize outdated GitHub docs (`README.md`, `SPEC.md`, `ARCHITECTURE.md`), and prepare `v0.8.0` sovereign release.

## Checklist
- [x] 1. Run `scripts/validate-hotkeys` and generate `docs/SMOKE-TEST.md` for user host verification.
- [x] 2. Create comprehensive `docs/USER-GUIDE.md` (installation, updates, keybindings, settings, troubleshooting).
- [x] 3. Modernize `README.md`, `SPEC.md`, `docs/ARCHITECTURE.md`, `ROADMAP.md` (reflect pure C + companion decoupling).
- [x] 4. Validate `make release` archive and run packaging checks.
- [ ] 5. Commit, push branch, open PR, achieve 100% green CI, and merge to `main`.
- [ ] 6. Tag `v0.8.0` and prepare GitHub Release notes.
