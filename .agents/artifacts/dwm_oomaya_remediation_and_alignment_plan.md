# Remediation & Alignment Plan: `dwm-oomaya` & Host Knowledge Vault

## Executive Summary

Following the audit report authored by the **Omarchy VM Techlead** ([`dwm_oomaya_alignment_audit.md`](file:///home/rand/Vault/antigravity-artifacts/current/dwm_oomaya_alignment_audit.md)), this remediation plan establishes full compliance with **Rule 4.5 (The Pristine Obsidian Vault Law)** and **Rule 7 (Cross-Machine Intelligence Federation)** across the host machine, the `dwm-oomaya` repository, and shared dotfiles.

The C window manager core, desktop scripts, and systemd daemons are confirmed 100% clean. The remaining actions focus on physical directory migration on the host machine, aligning project-local documentation in `dwm-oomaya`, merging `feat/oomaya-dwm-standalone` into `main`, and verifying cross-machine synchronization.

---

## 1. Remediation Matrix & Component Status

| Component | Target Location | Current State | Remediation Action |
| :--- | :--- | :--- | :--- |
| **Host Master Vault** | `~/antigravity-vault` | Cloned at `~/Vault` | Move `~/Vault` $\rightarrow$ `~/antigravity-vault`, preserving git history. |
| **Obsidian Personal Vault** | `~/Vault` | Contains git & AI artifacts | Re-create clean, unversioned `~/Vault` exclusively for Obsidian / `rclone`. |
| **Artifact Symlink** | `~/Documents/artifacts` | Points to `~/Vault/...` | Update symlink $\rightarrow$ `~/antigravity-vault/antigravity-artifacts`. |
| **Sync Daemon** | `antigravity-artifact-sync` | Running | Restow from updated dotfiles & restart daemon. |
| **`dwm-oomaya` Artifacts** | `.agents/artifacts/` | Mentions `~/Vault` | Update `walkthrough.md` & sync plan to `~/antigravity-vault`. |
| **`dwm-oomaya` Branch** | `main` | 8 commits behind `feat/*` | Merge `feat/oomaya-dwm-standalone` $\rightarrow$ `main` & push to GitHub. |
| **Neovim Keymap Labels** | `dotfiles/nvim` | Mentions "All Notes & Artifacts" | Update label to "Find Personal Notes (Obsidian Vault)". |

---

## 2. Itemized Execution Plan

### Phase 1: Host System Migration to `~/antigravity-vault` (Immediate)
1. **Stop Artifact Daemon Temporarily**:
   ```bash
   systemctl --user stop antigravity-artifact-sync.service
   ```
2. **Migrate Master AI Vault**:
   - Move `/home/rand/Vault` to `/home/rand/antigravity-vault`.
   - Re-establish pristine `/home/rand/Vault` directory for personal Obsidian notes.
3. **Re-wire Symlinks**:
   - Re-link `~/Documents/artifacts` $\rightarrow$ `~/antigravity-vault/antigravity-artifacts`.
4. **Re-stow Updated Dotfiles Package**:
   - `cd ~/.dotfiles && stow -R antigravity` to ensure `antigravity-sync-artifacts`, `antigravity-watch-artifacts`, and `art` point to `~/antigravity-vault`.
5. **Restart & Validate Daemon**:
   - `systemctl --user restart antigravity-artifact-sync.service`
   - Run `~/.dotfiles/verify.sh` to confirm zero test failures.

---

### Phase 2: `dwm-oomaya` Project-Local Artifact Alignment
1. **Target Artifacts on `feat/oomaya-dwm-standalone`**:
   - `.agents/artifacts/walkthrough.md`: Update all paths from `~/Vault/antigravity-artifacts` to `~/antigravity-vault/antigravity-artifacts` (or `~/Documents/artifacts`).
   - `.agents/artifacts/artifact_vault_and_github_sync_plan.md`: Add architectural note codifying Rule 4.5 (Pristine Obsidian Vault Law).
2. **Mirror This Plan**:
   - Save `dwm_oomaya_remediation_and_alignment_plan.md` into `~/.local/src/dwm-oomaya/.agents/artifacts/`.
3. **Commit Artifact Updates**:
   - Commit with message: `docs(artifacts): align vault architecture with Pristine Obsidian Vault Law (Rule 4.5)`.

---

### Phase 3: `dwm-oomaya` Compilation, Verification & Upstream Merge
1. **Compile & Lint Verification**:
   ```bash
   cd ~/.local/src/dwm-oomaya
   make clean && make
   ```
2. **Merge Feature Branch to Main**:
   ```bash
   git checkout main
   git merge --ff-only feat/oomaya-dwm-standalone || git merge feat/oomaya-dwm-standalone -m "feat(core): merge tab bar engine, xsettingsd theme sync, and logo refinements"
   git push origin main
   git checkout feat/oomaya-dwm-standalone
   ```
3. **Deploy Binary**:
   - Install verified binary using safe process replacement:
     ```bash
     install -Dm755 dwm ~/.local/bin/dwm
     ```

---

### Phase 4: Upstream Dotfiles Keymap Polish
1. **Refine Description in `~/.dotfiles/nvim/.config/nvim/lua/config/keymaps.lua`**:
   - Clarify `<leader>fv` description to `"Find Personal Notes (Obsidian Vault)"`.
2. **Commit & Push**:
   ```bash
   cd ~/.dotfiles
   git add nvim/.config/nvim/lua/config/keymaps.lua
   git commit -m "feat(nvim): clarify leader-fv description as personal Obsidian vault"
   git push origin master
   ```

---

### Phase 5: Verification Assertions
1. `~/.dotfiles/verify.sh` passes 100% with:
   - `oomaya/vault initialized and git-tracked at ~/antigravity-vault` $\rightarrow$ PASS.
   - `Obsidian ~/Vault remains pristine (clean of git tracking & AI artifacts)` $\rightarrow$ PASS.
   - `Documents/artifacts symlink is healthy` $\rightarrow$ PASS.
2. `systemctl --user status antigravity-artifact-sync.service` is active and watching `~/antigravity-vault`.
3. `art` CLI accurately finds and displays all mirrored artifacts.
4. Git remotes on `oomaya/dwm-oomaya` and `oomaya/dotfiles` are clean and synchronized.
