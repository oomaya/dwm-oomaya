# Diagnosis & Verification Walkthrough: `check-quickshell-settings-xvfb`

## 1. Executive Summary

CI failed at gate `check-quickshell-settings-xvfb` (Makefile:589) with:
```
Settings Xvfb failed while validating defaults and autostart settings (status 1)
```

Through full headless Xvfb reproduction, root cause analysis revealed that this was **not** a missing helper or missing `mimeapps.list` issue. Instead, the failure stemmed from test fixture drift following repository default updates (`alacritty` → `ghostty` and `nord` → `tokyonight`), along with a strict case branch in text-scale baseline verification.

All issues have been resolved cleanly in [`tests/test-quickshell-settings-xvfb.sh`](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/tests/test-quickshell-settings-xvfb.sh) without modifying any production code or user-facing defaults in `config/`.

---

## 2. Root Cause Analysis

### A. Primary Failure: Defaults & Autostart Settings (Line 1440)
- **Mechanism**:
  - The fixture copies repository TOMLs into the test environment: `cp "$repo/config/"*.toml "$config_home/dwm-titus/"`.
  - In `config/hotkeys.toml`, `[vars]` defines `terminal = "ghostty"`.
  - At line 1431, the test executed:
    ```bash
    sed -i 's/terminal = "alacritty"/terminal = "kitty"/' "$config_home/dwm-titus/hotkeys.toml"
    ```
  - Because `terminal = "alacritty"` was absent, `sed -i` silently made 0 replacements.
  - The test then polled for `quickshell ipc ... call settings defaultsRoleDesktopId terminal` to transition to `kitty.desktop`. Because `hotkeys.toml` still had `terminal = "ghostty"` (and `ghostty` is neither candidate nor mapped in `dwm-default-apps`), the call returned `""`.
  - Line 1440 `[ "$terminal_id" = kitty.desktop ]` failed and exited with status 1.

### B. Secondary Masked Failure: Appearance Startup Readiness (Line 1744)
- **Mechanism**:
  - `config/themes.toml` sets `theme = "tokyonight"`.
  - The test copied `config/themes.toml` as-is, but lines 1744, 3428, 3432, and 3452 specifically assert `[ "$appearance_theme" = nord ]`.
  - Without normalizing the fixture's theme baseline to `nord`, line 1744 immediately failed.

### C. Tertiary Masked Failure: Text-Scale Baseline Case Branch (Line 2830)
- **Mechanism**:
  - In CI and minimal test containers, `xsettingsd` and `dump_xsettings` are not installed (`xsettingsd` belongs to `fedora:desktop`, which CI does not install during test validation).
  - When verification tools are absent, [`scripts/dwm-settings-personalization`](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/scripts/dwm-settings-personalization#L508-L511) explicitly sets `apply_state=restricted` (*"Apply requires XSETTINGS verification tools; reset remains available"*) and `reset_state=available`.
  - However, line 2830 in `tests/test-quickshell-settings-xvfb.sh` only matched:
    ```bash
    available/available | restricted/restricted) text_size_baseline_valid=true ;;
    ```
    omitting the valid `restricted/available` case, which caused `capturing healthy appearance baseline` to fail.

---

## 3. Surgical Changes Applied

In [`tests/test-quickshell-settings-xvfb.sh`](file:///home/rand/Deliverable/antigravity-artifacts/dwm-oomaya/tests/test-quickshell-settings-xvfb.sh):

```diff
diff --git i/tests/test-quickshell-settings-xvfb.sh w/tests/test-quickshell-settings-xvfb.sh
index 2f957e0..72b1d65 100755
--- i/tests/test-quickshell-settings-xvfb.sh
+++ w/tests/test-quickshell-settings-xvfb.sh
@@ -341,7 +341,12 @@ cp "$repo/config/quickshell/assets/ctt_logo.png" "$home/Pictures/backgrounds/tes
 sed -i 's/readonly property var nativeBattery: UPower.displayDevice/readonly property var nativeBattery: null/' \
 	"$config_home/quickshell/power/PowerModel.qml"
 cp "$repo/config/"*.toml "$config_home/dwm-titus/"
+sed -i 's/^[[:space:]]*terminal[[:space:]]*=.*/terminal = "alacritty"/' \
+	"$config_home/dwm-titus/hotkeys.toml"
 cp "$repo/config/themes.toml" "$data_home/dwm-titus/config/themes.toml"
+sed -i '0,/^[[:space:]]*theme[[:space:]]*=.*/s//theme = "nord"/' \
+	"$config_home/dwm-titus/themes.toml" \
+	"$data_home/dwm-titus/config/themes.toml"
 printf '# inactive integration watch fixture\n' >"$config_home/dwm-titus/theme-env.sh"
 cat >"$data_home/applications/kitty.desktop" <<'EOF'
 [Desktop Entry]
@@ -2822,7 +2827,7 @@ while [ "$i" -lt 200 ]; do
 	case $baseline_text_size_state in
 	available | partial)
 		case $baseline_text_size_apply_state/$baseline_text_size_reset_state in
-		available/available | restricted/restricted) text_size_baseline_valid=true ;;
+		available/available | restricted/restricted | restricted/available) text_size_baseline_valid=true ;;
 		esac
 		;;
 	esac
```

---

## 4. Empirical Verification

1. **Full Xvfb Run**:
   ```
   Quickshell Settings Xvfb and closed-idle sample: PASS (0.000% CPU)
   Exit Code: 0
   ```
2. **Syntax Validation**:
   - `sh -n tests/test-quickshell-settings-xvfb.sh`: PASS (exit code 0)
3. **Format Compliance**:
   - `shfmt v3.7.0 -d tests/test-quickshell-settings-xvfb.sh`: PASS (0 differences)
4. **Static Analysis**:
   - `shellcheck tests/test-quickshell-settings-xvfb.sh`: PASS (exit code 0)
   - `make check-shell`: PASS (exit code 0)
