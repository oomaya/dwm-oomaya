# Task 3: `dotfiles/install.sh` (Stage 3.7) Fleet Rollout Pipeline Audit Report

- **Date**: 2026-11-04 KST
- **Author**: dwm-oomaya Tech Lead Pair
- **Base Commits**:
  - `dotfiles`: `1252842` (`master`)
  - `dwm-oomaya-pr1`: `12040b8` (`main`)
  - `quickshell-oomaya`: `d1751dc` (`main`)
- **Target Repositories & Execution Bounds**:
  - `dotfiles`: `/home/rand/dotfiles`
  - Core: `/home/rand/.local/src/dwm-oomaya-pr1`
  - Quickshell: `/home/rand/.local/src/quickshell-oomaya`
  - Off-Limits: `~/dwm-oomaya`, `~/.local/src/dwm-oomaya` (Protected)

---

## 1. Executive Summary

In Task 3, we audited and empirically validated Stage 3.7 (`sync_workstation_desktops`) in `~/dotfiles/install.sh` for multi-node fleet rollout readiness. 

### Key Accomplishments:
1. **Critical Architectural Bug Discovery & Fix**:
   - `install.sh` Stage 3.7 previously checked for git repository presence using `[[ -d "$dwm_dir/.git" ]]`.
   - In git worktrees (such as `dwm-oomaya-pr1`), `.git` is a **regular file** containing `gitdir: <path>`, not a directory. Consequently, worktrees were silently skipped during Stage 3.7 synchronization.
   - Fixed by upgrading test assertions from `-d` to `-e` across `dwm_dir` and `qs_dir`. Committed and pushed to `dotfiles` (`1252842`).
2. **Empirical Stage 3.7 Execution**:
   - Invoked `sync_workstation_desktops` targeting `/home/rand/.local/src/dwm-oomaya-pr1` and `quickshell-oomaya`.
   - Verified clean core compilation (`dwm-oomayad`), full binary deployment (all 66 utility scripts via `install-user`), Quickshell companion deployment, and user configuration preservation. Exit Code: **0**.
3. **Fleet Dry-Run Topology & Readiness**:
   - Executed `./install.sh --fleet --dry-run` over Tailscale:
     - Local Canary: `fedora44-vm` (`100.101.243.54`, SELF)
     - Online Peers: `cachyos-niri` (1ms), `omarchy-gram` (2ms), `omarchy-vm` (1ms), `peppermint-vm` (1ms)
     - Gatekeeper validation: Non-dwm nodes cleanly skip Stage 3.7 without error.

---

## 2. Technical Audit & Code Diff

### 2.1 The Worktree Discovery Bug (`.git` File vs Directory)
In `install.sh` prior to commit `1252842`:
```bash
sync_workstation_desktops() {
  local dwm_dir="${DWM_DIR:-$HOME/.local/src/dwm-oomaya}"
  [[ -d "$dwm_dir/.git" ]] || dwm_dir="$HOME/.local/src/dwm-oomaya-pr1"
  local qs_dir="${QUICKSHELL_DIR:-$HOME/.local/src/quickshell-oomaya}"

  # Gatekeeper: skip nodes that do not carry the dwm/quickshell stack
  if [[ ! -d "$dwm_dir/.git" && ! -d "$qs_dir/.git" ]]; then
    return 0
  fi
...
```
When `DWM_DIR` pointed to a git worktree (`.git` is a file):
1. `[[ -d "$dwm_dir/.git" ]]` evaluated to `false`.
2. The gatekeeper evaluated to `true` (skipping execution) or bypassed the `dwm-oomaya` block entirely.

### 2.2 Applied Patch (`dotfiles@1252842`)
```diff
--- a/install.sh
+++ b/install.sh
@@ -920,18 +920,18 @@ sync_workstation_desktops() {
   local dwm_dir="${DWM_DIR:-$HOME/.local/src/dwm-oomaya}"
-  [[ -d "$dwm_dir/.git" ]] || dwm_dir="$HOME/.local/src/dwm-oomaya-pr1"
+  [[ -e "$dwm_dir/.git" ]] || dwm_dir="$HOME/.local/src/dwm-oomaya-pr1"
   local qs_dir="${QUICKSHELL_DIR:-$HOME/.local/src/quickshell-oomaya}"
 
   # Gatekeeper: skip nodes that do not carry the dwm/quickshell stack
-  if [[ ! -d "$dwm_dir/.git" && ! -d "$qs_dir/.git" ]]; then
+  if [[ ! -e "$dwm_dir/.git" && ! -e "$qs_dir/.git" ]]; then
     return 0
   fi
 
   print_header "Stage 3.7: Synchronizing DWM & Quickshell Desktop Stack"
 
   # 1. dwm-oomaya
-  if [[ -d "$dwm_dir/.git" ]]; then
+  if [[ -e "$dwm_dir/.git" ]]; then
     print_step "Synchronizing dwm-oomaya repository ($dwm_dir)..."
     git -C "$dwm_dir" pull --rebase --autostash -q origin main 2>/dev/null \
       || print_warn "dwm-oomaya pull skipped (offline or local conflicts)"
@@ -952,7 +952,7 @@ sync_workstation_desktops() {
   fi
 
   # 2. quickshell-oomaya
-  if [[ -d "$qs_dir/.git" ]]; then
+  if [[ -e "$qs_dir/.git" ]]; then
     print_step "Synchronizing quickshell-oomaya repository..."
```

---

## 3. Fleet Dry-Run Matrix

```
Node             Tailscale IP     State          RTT      Planned Action
----------------------------------------------------------------------------------------
fedora44-vm      100.101.243.54   SELF           -        CANARY (local): install.sh --sync, gate: 0 Failed
cachyos-niri     100.115.225.96   ONLINE         1ms      after canary: ssh -> fleet-sync (sequential)
gtab-s7          100.88.177.97    OFFLINE        -        [SKIPPED: offline]
mint-vm          100.118.69.27    OFFLINE        -        [SKIPPED: offline]
muse-1           100.69.212.28    SKIP-NONLINUX  -        [SKIPPED: non-Linux (unknown)]
muse             100.106.200.118  OFFLINE        -        [SKIPPED: offline]
omarchy-gram     100.82.99.66     ONLINE         2ms      after canary: ssh -> fleet-sync (sequential)
omarchy-vm       100.124.231.48   ONLINE         1ms      after canary: ssh -> fleet-sync (sequential)
peppermint-vm    100.126.37.73    ONLINE         1ms      after canary: ssh -> fleet-sync (sequential)
win11-pc         100.126.57.34    SKIP-NONLINUX  -        [SKIPPED: non-Linux (windows)]
```
