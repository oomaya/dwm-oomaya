---
trigger: always_on
description: Mandatory development protocol, branching discipline, and quality gates for dwm-oomaya.
---

# DWM-Oomaya Development Protocol & Operating Laws

These rules are strictly binding for all modifications within the `dwm-oomaya` project.

---

## 1. The Three Operating Laws

1. **Pre-Modification Plan & Human Tollgate**:
   - Prior to modifying any source files, thoroughly inspect current layouts, definitions, functions, and state.
   - Present a concise, structured implementation plan with clear rationale and diff outlines.
   - **Wait for human review and explicit approval before applying any edits.**

2. **Zero-Warning Compilation Gate (Self-Healing Loop)**:
   - After applying updates, execute a clean compilation (`make clean dwm`).
   - If any compiler warnings or errors occur, resolve them immediately and re-run the build until the output is 100% clean (0 errors, 0 warnings).

3. **Strict Branching Discipline**:
   - Always verify or create a dedicated local feature branch (`git checkout -b <feature-branch>`) before modifying code.
   - Never develop directly on dirty branches; ensure clean revert capability at every step.

---

## 2. The North Star Vision

> **Make `dwm-oomaya` an epitome of innovation borrowed from Omarchy-oomaya and DWM-titus for continuous flow and least meaningless frictions.**

- **From Omarchy-oomaya**:
  - Sovereign, frictionless desktop ergonomics.
  - Zero Hardcoded Literals Law (no user paths, dynamic `$PATH` and `$HOME` lookups).
  - Sharp-Corner Aesthetic Law (`radius = 0`, crisp minimal borders).
  - Unified theme harmony (TokyoNight palette).
- **From DWM-Titus**:
  - Smart borders (`noborder`), window swallowing (`swallow`), cursor warping, and clean process lifecycle.
  - Robust X11 state and external bar/IPC integration.
