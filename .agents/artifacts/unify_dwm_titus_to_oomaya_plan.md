# Unify dwm-titus to dwm-oomaya Architectural Plan

## Executive Summary
Mixed references to `dwm-titus` and `dwm-oomaya` across paths, environment variables, configuration directories, runtime state paths, and test fixtures create cognitive overhead, test fragility, and repeated CI churn. This plan establishes a clean, systematic unification to `dwm-oomaya` across the entire codebase per `.agents/tasks/unify-dwm-titus-to-oomaya.md`.

---

## Quota & Token Budget Analysis (Go/No-Go Decision Gate)
- **Current Live Quota Status:**
  - Gemini Weekly: 5.19% consumed (~94.8% remaining, resets 2026-10-24)
  - Claude Weekly: 0.0% consumed (100% remaining)
  - Gemini 5-hour: 16.66% consumed
- **Projected Token Burn:** ~40,000–60,000 tokens across planning, multi-file replacement, and test verification.
- **Model Allocation Strategy:** Gemini 3.8 Flash (High Context) for file scanning and surgical replacement; local linters and bash runners for verification.
- **Decision:** **GO** (Ample quota available; well above the 15% triage threshold).

---

## 1. Scope & Categorization of `dwm-titus` References

### Category A: Core C Sources & Headers
- `dwm.c`: default autostart path (`~/.config/dwm-oomaya/autostart.sh`), config comments, and class/title matchers.
- `tomlparser.c` & `tomlparser.h`: default config lookup paths (`~/.config/dwm-oomaya/`).

### Category B: Runtime Scripts
- `scripts/dwm-settings-*`: appearance, font, personalization, theme, wallpaper, display, input, provider helpers. Remove legacy dual-resolution heuristics that checked `dwm-oomaya` vs `dwm-titus` and standardize directly on `dwm-oomaya`.
- `scripts/dwm-quickshell-*`: state paths, socket and instance checks.
- `scripts/theme-apply.sh`: update theme destination paths and comments.
- `scripts/run-tests`: default workspace directory prefix (`dwm-oomaya-tests.XXXXXX`).
- `scripts/dwm-titus-release`: rename to `scripts/dwm-oomaya-release` (and update references).
- `scripts/dev-sync-install.sh`: synchronize paths to `dwm-oomaya`.

### Category C: Quickshell & Desktop Configurations
- `config/quickshell/appearance/AppearanceModel.qml`
- `config/quickshell/accessibility/AccessibilityModel.qml`
- `config/quickshell/notifications/NotificationModel.qml`
- `config/quickshell/panel/PanelSettingsModel.qml`
- `config/quickshell/settings/DisplaySettingsPane.qml`
- `config/alacritty/` & `config/kitty/` & `config/Thunar/uca.xml`

### Category D: Test Suites & Fixtures
- `tests/test-dwm-settings-*.sh`: update fixture paths, directory creation, and assertions from `dwm-titus` to `dwm-oomaya`.
- `tests/test-quickshell-*.sh`: standardize mock data and state homes.
- `tests/test-dev-sync-install.sh`, `tests/test-autostop.sh`, `tests/test-autostart.sh`, etc.

### Category E: Build, Packaging & CI
- `Makefile`: clean targets, release targets, scripts list.
- `install.sh`, `dwm-fedora.ks`, `dwm-fedora-nvidia.ks`.
- `.github/workflows/c-cpp.yml`: `DWM_TEST_TMP_ROOT: /var/tmp/dwm-oomaya-tests`.

### Category F: Documentation & Meta
- `AGENTS.md`, `README.md`, `ROADMAP.md`, `CHANGELOG.md`, `docs/*.md`.
- `docs/evidence/`: update active evidence manifests or scrub legacy logs where specified.

---

## 2. Itemized Execution Phases

1. **Phase 1: Core C & Scripts Unification**
   - Update `dwm.c`, `tomlparser.c`, `tomlparser.h`.
   - Update scripts: strip dual-directory fallbacks (`[[ -d $dir/dwm-oomaya || ! -d $dir/dwm-titus ]]`), make `dwm-oomaya` canonical.
   - Rename `scripts/dwm-titus-release` to `scripts/dwm-oomaya-release`.
2. **Phase 2: Desktop & Quickshell Configuration Unification**
   - Update QML models in `config/quickshell/`.
   - Update terminal configs in `config/alacritty/` and `config/kitty/`.
3. **Phase 3: Test Suites & Test Fixtures Unification**
   - Update all test files in `tests/` to use `dwm-oomaya` exclusively in fixture setup and assertions.
   - Rename temporary workspace templates in `scripts/run-tests` and test helpers.
4. **Phase 4: Packaging, Installer & CI Workflow Unification**
   - Update `Makefile`, `install.sh`, `dwm-fedora*.ks`.
   - Update `.github/workflows/c-cpp.yml`.
5. **Phase 5: Documentation Harmonization**
   - Update `AGENTS.md`, `README.md`, `ROADMAP.md`, and markdown guides in `docs/`.
6. **Phase 6: Pre-Push Local Gate Verification**
   - Run `grep -rn "dwm-titus" .` (excluding `.git/`) to confirm 0 remaining references.
   - Run `make check-shell`.
   - Run `shfmt -d ...`.
   - Run `bash scripts/run-tests make check`.
7. **Phase 7: Git Commit, Push & CI Verification**
   - Commit: `refactor: unify all dwm-titus remnants to dwm-oomaya`.
   - Push to `origin main`.
   - Monitor GitHub Actions `validate` run to green completion.

---

## 3. Acceptance Tollgates
- [ ] `git grep "dwm-titus"` returns 0 results across active codebase.
- [ ] Local `make clean all` builds cleanly.
- [ ] Local `make check-shell` passes without warnings.
- [ ] Local `make check` (or key test suites) passes cleanly.
- [ ] GitHub Actions CI `validate` workflow passes on `main`.
