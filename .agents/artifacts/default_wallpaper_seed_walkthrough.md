# Walkthrough: Replace Logo-as-Default-Wallpaper with Real Wallpaper

## 1. Executive Summary

- **Repository**: [dwm-oomaya](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya)
- **Commit SHA**: `79aa42fc17f978279d47b1ccc1dec6cd89eea966`
- **Scope**:
  - Replaced project logo (`assets/dwm-oomaya.jpg`) seeding with real 16:9 wallpaper (`assets/dwm-oomaya-wallpaper.jpg`) in [install.sh](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/install.sh#L824-L844).
  - Updated wallpaper directory emptiness check in [install.sh](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/install.sh#L833) to exclude `dwm-oomaya-wallpaper.jpg`.
  - Added default wallpaper seeding to `make install-user` in [Makefile](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/Makefile#L328-L332) (per user confirmation on Item #4).
  - Extended [tests/test-install-preservation.sh](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/tests/test-install-preservation.sh#L404-L411) with wallpaper presence, file integrity, and user ownership assertions.

---

## 2. Changes Applied

### A. [install.sh](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/install.sh) (Lines 824–836)
```diff
@@ -824,13 +824,13 @@ fi
 mkdir -p "$HOME/Pictures"
 mkdir -p "$HOME/Pictures/Screenshots"
 mkdir -p "$BG_DIR"
-if [ -f "$REPO_DIR/assets/dwm-oomaya.jpg" ]; then
-	cp -n "$REPO_DIR/assets/dwm-oomaya.jpg" "$BG_DIR/" 2>/dev/null || true
+if [ -f "$REPO_DIR/assets/dwm-oomaya-wallpaper.jpg" ]; then
+	cp -n "$REPO_DIR/assets/dwm-oomaya-wallpaper.jpg" "$BG_DIR/" 2>/dev/null || true
 	ok "Default dwm-oomaya wallpaper seeded in $BG_DIR."
 fi
 
 if install_recommended_profile; then
-	if [ ! -d "$BG_DIR/.git" ] && [ -z "$(find "$BG_DIR" -mindepth 1 -maxdepth 1 ! -name 'dwm-oomaya.jpg' -print -quit 2>/dev/null)" ]; then
+	if [ ! -d "$BG_DIR/.git" ] && [ -z "$(find "$BG_DIR" -mindepth 1 -maxdepth 1 ! -name 'dwm-oomaya-wallpaper.jpg' -print -quit 2>/dev/null)" ]; then
 		info "Downloading Nord wallpapers pack..."
 		if dwm_git_safe_clone https://github.com/ChrisTitusTech/nord-background.git "$BG_DIR" 2>/dev/null; then
 			ok "Wallpapers downloaded to $BG_DIR"
```

### B. [Makefile](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/Makefile) (Lines 14 & 328–332)
```diff
@@ -11,6 +11,7 @@ XDG_CONFIG_HOME ?= ${USER_HOME}/.config
 XDG_DATA_HOME ?= ${USER_HOME}/.local/share
 DATA_DIR  := ${XDG_DATA_HOME}/dwm-oomaya
 CFG_DIR   := ${XDG_CONFIG_HOME}
+BG_DIR    ?= ${USER_HOME}/Pictures/backgrounds
 DATADIR   ?= ${PREFIX}/share
 SYSTEMDUSERDIR ?= ${PREFIX}/lib/systemd/user
 CAPITAINE_DARK_THEME = Capitaine-Cursors
@@ -324,6 +325,11 @@ install-user:
 	test -f ${CFG_DIR}/dwm-oomaya/hotkeys.toml || install -Dm644 config/hotkeys.toml ${CFG_DIR}/dwm-oomaya/hotkeys.toml
 	test -f ${CFG_DIR}/dwm-oomaya/themes.toml  || install -Dm644 config/themes.toml  ${CFG_DIR}/dwm-oomaya/themes.toml
 	test -f ${CFG_DIR}/dwm-oomaya/window-rules.toml || install -Dm644 config/window-rules.toml ${CFG_DIR}/dwm-oomaya/window-rules.toml
+	@echo "==> Seeding default wallpaper (skipping existing file)..."
+	mkdir -p "${BG_DIR}"
+	if [ -f assets/dwm-oomaya-wallpaper.jpg ] && [ ! -f "${BG_DIR}/dwm-oomaya-wallpaper.jpg" ]; then \
+		install -Dm644 assets/dwm-oomaya-wallpaper.jpg "${BG_DIR}/dwm-oomaya-wallpaper.jpg"; \
+	fi
 	@echo "==> Migrating legacy graphical-session startup..."
 	HOME="${USER_HOME}" XDG_CONFIG_HOME="${XDG_CONFIG_HOME}" scripts/migrate-graphical-session.sh
 	@echo "==> Installing Meslo font aliases..."
```

### C. [tests/test-install-preservation.sh](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/tests/test-install-preservation.sh) (Lines 404–411)
```diff
@@ -401,6 +401,14 @@ for user_path in \
 	test "$(stat -c %U "$user_path")" = "$OWNER"
 	test "$(stat -c %G "$user_path")" = "$OWNER_GROUP"
 done
+test -f "$TEST_HOME/Pictures/backgrounds/dwm-oomaya-wallpaper.jpg"
+cmp "$TEST_REPO/assets/dwm-oomaya-wallpaper.jpg" \
+	"$TEST_HOME/Pictures/backgrounds/dwm-oomaya-wallpaper.jpg"
+test -f "$FRESH_HOME/Pictures/backgrounds/dwm-oomaya-wallpaper.jpg"
+cmp "$TEST_REPO/assets/dwm-oomaya-wallpaper.jpg" \
+	"$FRESH_HOME/Pictures/backgrounds/dwm-oomaya-wallpaper.jpg"
+test "$(stat -c %U "$FRESH_HOME/Pictures/backgrounds/dwm-oomaya-wallpaper.jpg")" = "$OWNER"
+test "$(stat -c %G "$FRESH_HOME/Pictures/backgrounds/dwm-oomaya-wallpaper.jpg")" = "$OWNER_GROUP"
 
 EMPTY_CONFIG_HOME="$WORK_DIR/empty-config"
 EMPTY_CONFIG_DIRS="$WORK_DIR/empty-etc-xdg"
```

---

## 3. Untouched Boundaries Preserved

As explicitly specified in the task packet:
- [lightdm/Makefile](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/lightdm/Makefile) was untouched (`wallpaper.jpg` -> `/usr/share/pixmaps/dwm-oomaya.jpg` login screen asset preserved).
- [install.sh:665](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/install.sh#L665) was untouched (LightDM restorecon pixmap reference preserved).
- `assets/dwm-oomaya.jpg` was untouched (retained as project logo asset).

---

## 4. Verification Results

| Check / Test | Command | Result |
| :--- | :--- | :--- |
| **Linting** | `shellcheck install.sh tests/test-install-preservation.sh` | PASS (0 errors, 0 warnings) |
| **Formatting** | `shfmt -d install.sh tests/test-install-preservation.sh` | PASS (clean, matches shfmt 3.7.0) |
| **Simulation 1 (Recommended Profile)** | Empty `$BG_DIR`, `INSTALL_PROFILE=recommended` | PASS (`dwm-oomaya-wallpaper.jpg` landed, Nord pack clone attempted) |
| **Simulation 2 (Minimal Profile)** | `$BG_DIR` with seeded wallpaper, `INSTALL_PROFILE=minimal` | PASS (no download attempted, no logo file copied) |
| **Simulation 3 (Makefile User Install)** | `make install-user` in fresh user tree | PASS (`dwm-oomaya-wallpaper.jpg` seeded, idempotent on rerun) |
| **Preservation Test Suite** | `make check-install-preservation` | PASS (verified repeated installs, permissions, and file integrity) |
| **Shell & Format Suite** | `make check-shell check-format` | PASS (all repository shell scripts and styles verified) |
