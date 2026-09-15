# Systems Craftsmanship: Antigravity Suite Updates & Dual-Model Synergy Guide

A comprehensive architectural manual and operational runbook on automating **Antigravity Suite (CLI & IDE)** lifecycle updates with zero token bloat, executing the **Artifact-Handoff Protocol** for pristine context sharing, and leveraging **Claude Sonnet/Opus** alongside **Gemini** in an optimal pair-programming symbiosis.

---

## 1. The Dual-Model Synergy Matrix

Google Antigravity natively integrates industry-leading frontier models. Rather than defaulting to a single model for all tasks, peak development velocity is achieved by routing tasks according to model specialization:

```mermaid
flowchart TD
    Task[Incoming Task] --> Nature{Task Characteristics}
    Nature -->|Broad scans / 50+ files / Architectural mapping| GFlash["🧠 Gemini 3.8 Flash (High)<br>Speed, 1M+ context, structural grep"]
    Nature -->|Deep algorithmic proof / Long-chain deduction / Synthesis| GPro["🔬 Gemini 3.1 Pro (High)<br>Massive synthesis across months of records"]
    Nature -->|Suckless C / Wayland IPC / Pointer math / Adversarial review| CSonnet["⚡ Claude Sonnet 4.6 (Thinking)<br>Surgical systems coding & zero-regression editing"]
    Nature -->|High-stakes architectural breakthroughs| COpus["👑 Claude Opus 4.6 (Thinking)<br>Maximum depth reasoning & formal verification"]
```

### Strategic Workload Routing

| Domain / Workload | Optimal Model | Why This Model Excels | Concrete Trigger / CLI Alias |
| :--- | :--- | :--- | :--- |
| **Suckless C Core & Layouts** | **`claude-sonnet-4-6`** | Flawless pointer arithmetic, bounds checking, X11 event loop logic without off-by-one errors in `dwm.c`/`drw.c`. | `agy-code "Fix layout cycle in dwm.c"` |
| **Wayland & Linux IPC Plumbing** | **`claude-sonnet-4-6`** | Precision Lua/C IPC dispatching (`hyprctl`, `niri`), avoiding subprocess injection and string escaping traps. | `agy-code "Fix atomic focus in hyprland.lua"` |
| **Adversarial Code Review** | **`claude-sonnet-4-6`** | Deeply skeptical auditor. Catches unquoted bash expansions, race conditions, and signal leaks that Gemini glides over. | `agy-code "Audit scripts/dwm-app-install"` |
| **Multi-File Codemods** | **`claude-sonnet-4-6`** | Surgical editing precision. Never drops comments or alters whitespace unnecessarily across 10+ files. | `agy-code "Refactor notification calls"` |
| **High-Context Repository Audits** | **`gemini-3.8-flash-high`** | 1M+ token context window. Ingests dozens of files across 3 repositories in sub-seconds with zero latency friction. | `agy-plan "Audit all scripts invoking dmenu"` |
| **Fast-Path Architectural Planning** | **`gemini-3.8-flash-high`** | Low-latency drafting of structured implementation plans and technical specifications. | `agy-plan "Design a multi-machine sync daemon"` |
| **Deep Historical Synthesis** | **`gemini-3.1-pro-high`** | Reasoning across massive document troves in `~/Vault` to distill multi-month architectural patterns. | `agy-pro "Synthesize post-mortems into a rule"` |
| **Active Quota Failover** | **`claude-sonnet-4-6`** | Instant, zero-downtime failover during hourly Gemini API spikes or 503 capacity limits. | `agy -m claude-sonnet-4-6 -c` |

---

## 2. The Artifact-Handoff Protocol: Context Sharing Without Token Bloat

> [!CAUTION]
> **The Chat Transcript Accumulation Trap**:
> Continuing a single conversation across dozens of turns accumulates massive transcript history (50,000+ tokens). This inflates prompt costs, hits hourly provider quotas, and triggers attention degradation ("lost in the middle").

### The Protocol Flow

Instead of carrying bloated chat transcripts between models, models communicate asynchronously through **Structured Markdown Artifacts**:

```mermaid
sequenceDiagram
    autonumber
    actor User as User / Developer
    participant Architect as 🧠 Gemini 3.8 Flash (Tech Lead)
    participant File as 📄 ~/Documents/artifacts/current_plan.md
    participant Implementer as ⚡ Claude Sonnet 4.6 (Implementer)

    User->>Architect: Step 1: Scope problem & explore codebase
    Architect->>File: Step 2: Write bounded implementation plan & target file pointers
    Architect-->>User: "Plan finalized at current_plan.md. Ready for implementation."
    
    User->>Implementer: Step 3: Launch clean session: agy-code "Implement file://.../current_plan.md"
    Note over Implementer: Claude starts with a 100% clean context window!<br>Zero transcript baggage, laser focus on target lines.
    Implementer->>Implementer: Surgical implementation & empirical verification
    Implementer-->>User: Step 4: Delivers walkthrough.md artifact with diffs and test results
    
    User->>Architect: Step 5: Final review and handoff logging
```

---

### 2.1 How the Protocol Works in Antigravity IDE (GUI)

The Antigravity IDE sidebar panel provides a clean, visual interface to execute the Artifact-Handoff Protocol without leaving your code editor:

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│ Antigravity IDE Workspace                                                   │
├────────────────────────────────────────┬────────────────────────────────────┤
│ Editor Canvas (dwm.c, hyprland.lua)    │ Sidebar Chat Panel                 │
│                                        ├────────────────────────────────────┤
│                                        │ [Thread A: Gemini 3.8 Flash]       │
│                                        │  User: "Help me scope and plan X"  │
│                                        │  Gemini: Generates current_plan.md │
│                                        │  --------------------------------  │
│                                        │  (Click '+' to Open Clean Thread)  │
│                                        ├────────────────────────────────────┤
│                                        │ [Thread B: Claude Sonnet 4.6]      │
│                                        │  Model Dropdown: [Claude 4.6 ▼]    │
│                                        │  User: "Implement @current_plan.md"│
│                                        │  Claude: Applies surgical edits -> │
│                                        │          Visual Diff Overlay       │
└────────────────────────────────────────┴────────────────────────────────────┘
```

#### Step-by-Step IDE Workflow:
1. **Thread 1 (Scoping with Gemini 3.8 Flash)**:
   - In your active sidebar chat thread, set model to **Gemini 3.8 Flash (High)**.
   - Describe your requirement or bug (e.g. *"Investigate why layout cycling fails in dwm-oomaya and propose a fix"*).
   - Gemini rapidly scans workspace files using its 1M+ token context and writes the plan to:
     `~/Documents/artifacts/current_plan.md` *(or `.agents/artifacts/plan.md`)*.
2. **The Clean Context Transition**:
   - Instead of continuing in the same bloated thread, click the **`+` (New Thread)** button at the top of the chat panel (or press `Ctrl + Shift + L`).
   - *Why?* This leaves behind the 20+ turns of exploration and gives you a **pristine 0-token canvas**.
3. **Thread 2 (Surgical Implementation with Claude Sonnet 4.6)**:
   - In the new thread, change the model dropdown at the bottom to **Claude Sonnet 4.6 (Thinking)**.
   - Send:
     ```text
     Implement the plan in @current_plan.md
     ```
     *(You can also use a file URI: `Implement file://${HOME}/Documents/artifacts/current_plan.md`)*.
   - Claude reads only the plan and the targeted source files. It executes with 100% focused attention, avoiding hallucinations and zero token waste.
4. **Visual In-Editor Review**:
   - Claude’s edits appear directly in your editor canvas as inline visual red/green diff overlays.
   - Review and accept/reject diffs per-hunk or run your terminal build test.

---

### 2.2 How the Protocol Works in Antigravity CLI (`agy`)

For terminal-first workflows (e.g. running inside Alacritty, Ghostty, or `dwm-flow`):

1. **Step 1: Scoping with Gemini**:
   ```bash
   cd ~/dwm-oomaya
   agy-plan "Scope and draft an implementation plan for vanity gaps in config.def.h"
   ```
   Gemini writes the approved plan to `~/Documents/artifacts/current_plan.md`.
2. **Step 2: Clean Implementation with Claude**:
   ```bash
   agy-code "Implement the changes specified in file://${HOME}/Documents/artifacts/current_plan.md"
   ```
   Claude boots in a fresh session with a 100% clean context window, mounts the workspace, applies surgical code edits, and verifies the build (`make dwm`).
3. **Step 3: Verification & Walkthrough**:
   Claude outputs a concise `walkthrough.md` with git diffs and test results.

---

### 2.3 Concrete Sample Scenarios

#### Scenario 1: Refactoring C Window Manager Pointer Logic (`dwm.c`)
- **Challenge**: Modifying client window focus handling or layout cycling in `dwm.c` risks subtle pointer corruption, off-by-one index arithmetic, and window manager segfaults.
- **Workflow**:
  1. *Gemini Flash*: You run `agy-plan "Analyze layouts[] indices in dwm.c and config.def.h; find why Super+M switches to spiral instead of monocle"`. Gemini flags that index `2` is spiral while monocle is at index `1`, and drafts `current_plan.md`.
  2. *Claude Sonnet 4.6*: You invoke `agy-code "Implement file://${HOME}/Documents/artifacts/current_plan.md"`. Claude makes the precise C edit, ensures `setlayout(&layouts[1])` is used, runs `make -j$(nproc)`, and tests compiler warnings.
  3. *Result*: Fixed in 1 shot, 0 compiler warnings, zero bloated transcript.

#### Scenario 2: Debugging a Wayland Lua IPC Dispatch Bug (`hyprland.lua`)
- **Challenge**: `omarchy-menu-windows` fails to focus windows on off-screen scrolling ribbons due to legacy `hyprctl dispatch` shell collisions.
- **Workflow**:
  1. *Gemini Flash*: Explores the dotfiles ecosystem, discovers all references to `omarchy-menu-windows`, and writes the architectural blueprint into `current_plan.md`.
  2. *Claude Sonnet 4.6*: Opens `hyprland.lua`, replaces the string interpolation with the atomic Lua IPC dispatch (`hl.dispatch(hl.dsp.focus({ window = 'address:0x...' }))`), and verifies Lua syntax using `luac -p`.
  3. *Result*: Flawless Wayland camera panning with crisp window centering.

#### Scenario 3: Investigating an Application Crash via Systemd Coredump
- **Challenge**: An application segfaults, generating a coredump in `systemd-coredump`.
- **Workflow**:
  1. *Gemini Flash*: Ingests the 2,000-line `coredumpctl info` stack trace and maps the faulting instruction pointer to the binary's symbol table.
  2. *Claude Sonnet 4.6*: Reviews the exact lines of C source code where the SIGSEGV occurred, spots an unchecked `NULL` pointer returned by a malloc/lookup call, and applies defensive bounds checking.
  3. *Result*: Crash permanently resolved with zero regression risk.

---

### Why the Artifact-Handoff Protocol is 10x More Efficient:
1. **Clean Attention Window**: The implementer model (Claude) starts fresh, focusing 100% of its reasoning budget on the code rather than parsing 40 turns of conversational chat.
2. **Model Independence**: Seamlessly switch models between phases (Gemini scopes $\rightarrow$ Claude codes $\rightarrow$ Claude audits $\rightarrow$ Gemini documents).
3. **Auditability**: Every handoff is preserved as a version-controlled markdown document in `~/antigravity-vault` (`oomaya/vault`).

---

## 3. Shell Ergonomics: Terminal Aliases

Configured in `~/.bashrc` and `~/dotfiles/bash/.bashrc`:

```bash
# Antigravity Model Synergy Aliases
alias agy-code="agy --model claude-sonnet-4-6"          # Deep systems coder (Claude Sonnet 4.6 Thinking)
alias agy-opus="agy --model claude-opus-4-6-thinking"  # High-depth reasoning (Claude Opus 4.6 Thinking)
alias agy-plan="agy --model gemini-3.8-flash-high --mode plan" # Rapid architect (Gemini 3.8 Flash)
alias agy-pro="agy --model gemini-3.1-pro-high"         # Heavy synthesis (Gemini 3.1 Pro)
```

### Real-Life Terminal Workflows

```bash
# 1. Surgical Systems Bug Fix (Claude mounts cwd, git state, and active skills):
cd ~/dwm-oomaya
agy-code "We are seeing layout collapse when pressing Super+D. Inspect keys[] in config.def.h and dwm.c."

# 2. Planning a New Feature (Gemini Flash creates blueprint):
cd ~/.dotfiles
agy-plan "Design a new wayland notification daemon module for omarchy"

# 3. Interactive Prompt with Claude:
cd ~/.config/hypr
agy-code -i "Refactor window focus logic to use atomic Lua dispatch"

# 4. Resuming Latest Conversation with Claude Failover:
agy -m claude-sonnet-4-6 -c
```

---

## 4. Agent-Triad Guardrails: The 1-Revision Law (`MAX_ROUND_COUNT=2`)

When orchestrating a 3-agent triad (Designer, Reviewer, Mediator), unconstrained debate can result in a "token-fire." The updated `agent-triad` skill (`~/.gemini/config/skills/agent-triad/SKILL.md`) enforces two strict architectural controls:

1. **`MAX_ROUND_COUNT=2` (The 1-Revision Law)**:
   - **Round 1**: Designer drafts architecture/code $\rightarrow$ Reviewer executes empirical tests and grades 5 vectors.
   - **Round 2**: Designer applies surgical fixes addressing feedback $\rightarrow$ Reviewer audits the fixes.
   - **Immediate Escalation**: If consensus is not reached after Round 2, debate **immediately halts**. The Mediator summarizes remaining trade-offs and escalates to the Human Partner (User) for a binding tie-breaker decision.
2. **Mandatory Consensus File Offload**:
   - Final deliverables are **never dumped into chat**.
   - Output is automatically written to `~/Documents/artifacts/current_plan.md` *(convenience mirror: `current/triad_consensus.md`)*.

---

## 5. Zero-Token Automated Suite Updater (`agy-update`)

The `agy-update` orchestrator solves the friction of keeping both the CLI and IDE in sync across your machine fleet with zero token expenditure.

### Architecture & Capabilities

```text
agy-update
├── [1/2] CLI Engine: Runs native 'agy update' silently
└── [2/2] IDE Engine:
    ├── Auto-detects 'Antigravity IDE*.tar.gz' in ~/Downloads (from antigravity.google)
    ├── Compares archive timestamp against installed instance in ~/.local/share/antigravity
    ├── Invokes 'app-install --user' in < 1 second
    ├── Applies atomic symlink swap (~/.local/bin/antigravity) to prevent ETXTBSY crashes
    ├── Defensively updates XDG desktop database
    └── Sends optional desktop notification via notify-send
```

### Usage & CLI Flags

```bash
# 1. Full automatic update (CLI + newest IDE archive in ~/Downloads):
agy-update

# 2. Inspect versions without making changes:
agy-update --check

# 3. Ingest explicit IDE archive from custom path:
agy-update --ide ~/Downloads/Antigravity\ IDE.tar.gz

# 4. Force re-installation of IDE even if timestamps match:
agy-update --force

# 5. Quiet mode (ideal for cron or non-blocking startup hooks):
agy-update --quiet
```

### Defensive Error Boundaries
- **`ETXTBSY` Protection**: Uses atomic symlink pointer swapping (`ln -sfn`) rather than copying files over an actively running IDE process.
- **Rollback Staging**: Backs up the previous version to `~/.local/share/antigravity.bak` until the new installation verifies cleanly.
- **Defensive Database Updates**: Guards `update-desktop-database` with `command -v` to prevent unexpected script crashes on minimal or headless systems.

---

## 6. Summary Checklist

- [ ] Model chosen according to the **Model Specialization Matrix** (Gemini for breadth/planning; Claude for surgical C/systems).
- [ ] Context shared cleanly via the **Artifact-Handoff Protocol** (`current_plan.md`) without transcript bloat.
- [ ] Triad loops bounded by the **1-Revision Law** (`MAX_ROUND_COUNT=2`).
- [ ] Antigravity CLI and IDE maintained seamlessly via `agy-update` and `app-install`.
