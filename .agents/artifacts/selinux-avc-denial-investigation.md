# Forensic Investigation: SELinux AVC Denial Alert (`nnp_transition`)

**Date**: 2026-11-03 KST  
**Investigator**: dwm-oomaya Tech Lead Pair  
**Host Node**: Fedora Linux 44 (Rawhide/Branched)  
**Target Alert**: `(A) New SELinux security alert: AVC denial, click icon to view`  
**Uploaded Evidence**: `media_1793682536291_fdc0247a.png`

---

## 1. Executive Summary & Root Cause

The desktop alert shown in the screenshot was triggered by **SELinux audit logs** during our recent test runs (13:42 ~ 13:51 KST).

When running test suites (such as `make -C quickshell-oomaya check` and isolated Xvfb verification) inside Antigravity's standard sandbox:
1. **The Sandbox Constraint**: Antigravity's sandbox runner applies the Linux kernel feature `PR_SET_NO_NEW_PRIVS` (NNP) to prevent child processes from escalating privileges or jumping domains.
2. **The SELinux Conflict**: The test scripts executed `dbus-run-session` / `dbus-launch` to spawn temporary D-Bus buses. In Fedora 44's default policy, executing `/usr/bin/dbus-daemon` from `unconfined_t` attempts a domain transition to `unconfined_dbusd_t`.
3. **The Denial**: Because NNP is set on the process tree, the kernel checks whether `unconfined_t` is explicitly permitted to perform an `nnp_transition` into `unconfined_dbusd_t`. Fedora 44's default targeted policy lacks this permission rule, triggering an **AVC Denial**:
   ```text
   AVC avc: denied { nnp_transition } for pid=3023551 comm="dbus-run-sessio"
   scontext=unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023
   tcontext=unconfined_u:unconfined_r:unconfined_dbusd_t:s0-s0:c0.c1023
   tclass=process2 permissive=0
   ```
4. **The Notification**: Fedora's user-space daemon `setroubleshootd` detected these audit entries and surfaced the desktop notification.

> [!NOTE]
> This is **harmless** and strictly confined to automated background test runs inside sandboxes with `NO_NEW_PRIVS`. Your host system, window manager (`dwm`), dotfiles, and daily desktop sessions run natively outside this sandbox constraint and will **not** trigger this alert during normal daily operations.

---

## 2. Forensic Audit Log Evidence

From `journalctl` and `/var/log/audit/audit.log`:

```text
Nov 03 13:42:33 fedora44 audit[2998709]: AVC avc: denied { nnp_transition } for pid=2998709 comm="dbus-run-sessio"
Nov 03 13:43:06 fedora44 audit[3011285]: AVC avc: denied { nnp_transition } for pid=3011285 comm="dbus-run-sessio"
Nov 03 13:43:13 fedora44 audit[3011837]: AVC avc: denied { nnp_transition } for pid=3011837 comm="dbus-run-sessio"
Nov 03 13:50:36 fedora44 audit[3019868]: AVC avc: denied { nnp_transition } for pid=3019868 comm="dbus-run-sessio"
Nov 03 13:50:46 fedora44 audit[3020239]: AVC avc: denied { nnp_transition } for pid=3020239 comm="dbus-run-sessio"
Nov 03 13:51:04 fedora44 audit[3022383]: AVC avc: denied { nnp_transition } for pid=3022383 comm="dbus-run-sessio"
Nov 03 13:51:15 fedora44 audit[3023551]: AVC avc: denied { nnp_transition } for pid=3023551 comm="dbus-run-sessio"
```

The audit timestamps directly correspond to our test executions:
- `13:42:33`: `test-quickshell-health-xvfb.sh`
- `13:43:06`: `test-quickshell-settings-xvfb.sh`
- `13:50:36`: live quickshell launch inspection under Xvfb
- `13:51:04`: `dwm-shell-mode` dual-shell switching test

---

## 3. Policy Analysis (`audit2allow`)

Running `audit2allow` against the exact denial vector yields:

```text
#============= unconfined_t ==============
allow unconfined_t unconfined_dbusd_t:process2 nnp_transition;
```

This confirms the issue is strictly a missing Type Enforcement (TE) allow rule in Fedora's policy for process domain transitions when `PR_SET_NO_NEW_PRIVS` is set.

---

## 4. Remediation Options

### Option 1: Do Nothing (Recommended)
Because this denial only occurs when test suites invoke `dbus-run-session` inside the Antigravity developer sandbox, no action is needed for daily window management, coding, or fleet deployment. Normal desktop operations (launching apps, dwm, quickshell, Dunst) do not set `NO_NEW_PRIVS` and will not trigger this alert.

### Option 2: Permanently Authorize the Transition on This Host
If you prefer to permanently silence the alert during future sandboxed test runs on this Fedora machine, you can install a local SELinux policy module:

```bash
# 1. Extract the recent dbus-run-session denials into a policy module
sudo ausearch -c 'dbus-run-sessio' -ts recent --raw | audit2allow -M my-dbus-nnp

# 2. Compile and install the policy module into the SELinux store
sudo semodule -X 300 -i my-dbus-nnp.pp
```

Once installed, SELinux will allow `dbus-run-session` to transition cleanly under NNP, and no alert will appear.
