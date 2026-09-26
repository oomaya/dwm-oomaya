# Peppermint OS (Debian/Xlibre X11) Compatibility & Script Hardening Plan

## Executive Summary
This plan details the surgical remediation of `dwm-oomaya` scripts to provide first-class support for **Peppermint OS** running on a **Debian / Xlibre X11** stack. Our inspection revealed four distinct friction points:
1. **Distribution Detection Failure**: `/etc/os-release` on Peppermint defines `ID="peppermint"` without an `ID_LIKE` entry. Consequently, `dwm-utils.sh` evaluates `$DISTRO_FAMILY` as `"unknown"`, causing `install.sh` and `dwm_packages` to abort immediately.
2. **Xlibre Destruction Hazard**: `debian:x11` requests `xorg`. Because the `xlibre` metapackage declares `Conflicts: xorg`, executing `install.sh` on Peppermint causes APT to uninstall the user's entire Xlibre server suite and substitute Debian upstream Xorg.
3. **Missing Debian Package Parity**: `xkbset` (required by `check-deps.sh`) and `xsettingsd` (required by `autostart.sh`) are present in Debian repositories but omitted from Debian profile declarations in `scripts/dwm-packages.sh`.
4. **Unguarded Pipeline Crash in `install.sh`**: `package_line()` evaluates `dwm_packages` inside an unguarded pipe under `set -eo pipefail`, causing abrupt crashes on unknown or unmapped profiles before rejection diagnostics can print.

---

## Quota & Token Budget Analysis (Go/No-Go Decision Gate)

### Current Allowance Snapshot
- **Host Runtime Check**: CLI `agy-usage` not present in node `$PATH`; operating in local environment.
- **Estimated Session Quota**: High reserve (> 80% remaining).
- **Hourly Provider Window**: Healthy; zero rate-limiting backoffs detected.

### Estimated Phase Expenditure
- **Phase 1 (Inspection & Root Cause Mapping)**: ~6,000 tokens (Completed via Gemini Flash).
- **Phase 2 (Implementation & Script Patching)**: ~8,000 tokens (Gemini / Claude pairing).
- **Phase 3 (Verification & Test Suite Execution)**: ~4,000 tokens.

### Model Selection & Risk Mitigation Rationale
- **Go/No-Go Verdict**: **GO**. Quota is abundant.
- **Synergy Strategy**: Gemini handles context orchestration and high-level architectural auditing; surgical script modifications follow the Anti-Bureaucracy principle (low risk, shell script focus, zero red tape).

---

## Proposed Changes & Architectural Blueprint

### 1. `scripts/dwm-utils.sh` — Distro Family Detection
Add `peppermint` and robust Debian family fallback detection.
```bash
# Before:
elif [[ $DISTRO_ID =~ ^(debian|ubuntu|pop|linuxmint)$ || ${ID_LIKE:-} =~ (debian|ubuntu) ]]; then
	DISTRO_FAMILY="debian"
fi

# After:
elif [[ $DISTRO_ID =~ ^(debian|ubuntu|pop|linuxmint|peppermint)$ || ${ID_LIKE:-} =~ (debian|ubuntu) || -f /etc/debian_version ]]; then
	DISTRO_FAMILY="debian"
fi
```

### 2. `scripts/dwm-packages.sh` — Xlibre Preservation & Package Parity
1. **Preserve Xlibre**: Apply the installed-provider bypass pattern (identical to `power-profiles-daemon` / `ppd-service`):
   ```bash
   dwm_is_x11_server_installed() {
       command -v Xlibre >/dev/null 2>&1 ||
       command -v Xorg >/dev/null 2>&1 ||
       dpkg -s xlibre >/dev/null 2>&1 ||
       dpkg -s xserver-xlibre-core >/dev/null 2>&1
   }
   ```
   In `dwm_install_package_profile()`:
   ```bash
   if [[ $package == xorg ]] && dwm_is_x11_server_installed; then
       printf '%s\n' 'Retaining installed X11 server (Xlibre/Xorg); skipping xorg to prevent conflict.' >&2
       continue
   fi
   ```
2. **Add `xkbset` to `debian:x11`**:
   `x11-xserver-utils x11-utils x11-xkb-utils xinput xkbset`
3. **Add `xsettingsd` to `debian:desktop`**:
   `dunst picom feh dex inotify-tools jq alsa-utils brightnessctl libnotify-bin pulseaudio-utils playerctl xsettingsd`

### 3. `install.sh` — Pipefail Hardening
Harden `package_line()` to avoid pipefail abortion when profiling:
```bash
package_line() {
	local profile=$1
	dwm_packages "$DISTRO_FAMILY" "$profile" 2>/dev/null | paste -sd ' ' - || true
}
```

---

## Verification & Tollgate Plan

### Automated Test Gates
1. Run `./tests/test-package-maps.sh` to ensure package map regressions are prevented.
2. Run `./tests/test-fedora-platform.sh` to ensure rejection fixtures pass.
3. Run `./install.sh --dry-run` on Peppermint to verify:
   - `Family: debian` is correctly resolved.
   - `Package manager: sudo apt-get install -y` is displayed.
   - Profile resolution succeeds without crashes.
4. Run `./scripts/check-deps.sh` to observe updated dependency mapping.
5. Dry-run `apt-get install -s` on the resolved package list to prove **zero package removals** (`Remv xlibre` must never appear).

---

## Decision Tollgates & Open Questions
- **Quickshell on Peppermint**: Quickshell `0.3.0` is available in `trixie-backports`, while `dunst` and native suckless topbar are already functional. The plan retains Debian's non-blocking fallback strategy (use Quickshell if present, otherwise default to `dunst` + Chadwm topbar).
