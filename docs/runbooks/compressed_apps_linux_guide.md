# Systems Craftsmanship: Compressed App & Suite Installation on Linux

A comprehensive engineering guide on installing, structuring, and integrating unpackaged archive downloads—including **JetBrains suites**, **VMware Workstation**, **AppImages**, and standalone developer tools—into the Linux filesystem hierarchy following POSIX and XDG standards.

---

## 1. The Unix Filesystem Standard: `/opt` vs `~/.local`

When downloading `.tar.gz`, `.tar.xz`, `.bundle`, or AppImage packages, never extract them haphazardly into `$HOME/Downloads` or dump entire application trees into `~/.local/bin`.

| Destination | Scope | Permissions | Best Suited For |
| :--- | :--- | :--- | :--- |
| **`/opt/<app-name>`** | System-Wide (All Users) | `root:root` (`755`) | Multi-user developer suites (JetBrains IntelliJ/PyCharm/GoLand, VMware Workstation, Android Studio, Google Chrome). |
| **`~/.local/share/<app-name>`** | User-Local (Single User) | `$USER:$USER` (`755`) | Unprivileged personal installations, tools without `sudo` access, or roaming developer profiles. |
| **`/usr/local/bin`** | Executable Path (System) | `root:root` (`755`) | Single symlinks or trampoline scripts pointing to `/opt/<app-name>/bin/<app>`. |
| **`~/.local/bin`** | Executable Path (User) | `$USER:$USER` (`755`) | Single symlinks or trampoline scripts pointing to `~/.local/share/<app-name>/...`. |

> [!IMPORTANT]
> **Rule of Clean Paths**:
> `PATH` directories (`/usr/local/bin`, `~/.local/bin`) must contain **only executable binaries, symlinks, or trampoline scripts**. Never extract a multi-file suite (with `lib/`, `plugins/`, `assets/`) directly inside `bin/`.

---

## 2. JetBrains Developer Suites (Tarball `.tar.gz`)

### Step-by-Step Installation
```bash
# 1. Choose target product and extract to /opt
APP_NAME="idea-IU" # or pycharm, clion, goland
ARCHIVE="ideaIU-*.tar.gz"

# 2. Extract into /opt
sudo tar -xzf "$ARCHIVE" -C /opt/

# 3. Standardize folder name (symlink or rename versionless for seamless upgrades)
TARGET_DIR=$(find /opt -maxdepth 1 -type d -name "idea-*" | head -n 1)
sudo ln -sfn "$TARGET_DIR" /opt/intellij-idea

# 4. Create executable symlink in /usr/local/bin
sudo ln -sfn /opt/intellij-idea/bin/idea.sh /usr/local/bin/idea
```

### XDG Desktop Entry Integration
Create `/usr/share/applications/intellij-idea.desktop`:
```ini
[Desktop Entry]
Version=1.0
Type=Application
Name=IntelliJ IDEA Ultimate
GenericName=Java / Kotlin IDE
Comment=Capable and Ergonomic IDE for JVM
Exec=/usr/local/bin/idea %u
Icon=/opt/intellij-idea/bin/idea.svg
Terminal=false
Categories=Development;IDE;
StartupNotify=true
StartupWMClass=jetbrains-idea
```
Update application caches:
```bash
sudo update-desktop-database /usr/share/applications
```

---

## 3. VMware Workstation (`.bundle` / `.tar.gz`)

VMware Workstation uses an interactive shell bundle requiring kernel module compilation (`vmmon`, `vmnet`) and systemd service management.

### Installation & Module Compilation
```bash
# 1. Make installer bundle executable
chmod +x VMware-Workstation-*.bundle

# 2. Execute installation with systemd service configuration
sudo ./VMware-Workstation-*.bundle --console --eulas-agreed --required

# 3. Compile kernel modules for current kernel (Fedora / CachyOS / Arch)
sudo vmware-modconfig --console --install-all

# 4. Enable and start core background services
sudo systemctl enable --now vmware.service
sudo systemctl enable --now vmware-USBArbitrator.service
```

### dwm / Tiling Window Manager Protection
VMware spawns internal helper windows (`vmware-user`, `vmtoolsd`) that lack `override_redirect`. Ensure your window manager rules flag them as `unmanaged = 1` to prevent ghost window hijacking:
```c
/* config.def.h */
{ "vmware-user", NULL, NULL, 0, 0, 0, 0, 0, -1, 1 },
{ "vmtoolsd",    NULL, NULL, 0, 0, 0, 0, 0, -1, 1 },
```

---

## 4. AppImage Self-Contained Packages

AppImages package runtimes and dependencies in a mountable squashfs image.

### Recommended Deployment
```bash
# 1. Create dedicated AppImage repository
mkdir -p ~/.local/share/appimages/
mkdir -p ~/.local/bin/

# 2. Move binary and make executable
mv MyApp.AppImage ~/.local/share/appimages/myapp.AppImage
chmod +x ~/.local/share/appimages/myapp.AppImage

# 3. Create trampoline script in ~/.local/bin
cat > ~/.local/bin/myapp <<'EOF'
#!/bin/sh
exec "$HOME/.local/share/appimages/myapp.AppImage" "$@"
EOF
chmod +x ~/.local/bin/myapp
```

### Gear Lever Integration
For automatic desktop integration, icon extraction, and update tracking, use **Gear Lever**:
```bash
# Gear Lever monitors and integrates AppImages into XDG menus automatically
gearlever ~/.local/share/appimages/myapp.AppImage
```

---

## 5. Antigravity IDE Packaging Standard

To install Antigravity IDE cleanly following this runbook:

```bash
# 1. Create target directory
sudo mkdir -p /opt/antigravity-ide

# 2. Move extracted suite
sudo cp -r "$HOME/Downloads/Antigravity IDE/"* /opt/antigravity-ide/
sudo chmod -R 755 /opt/antigravity-ide/

# 3. Create global binary trampoline
sudo ln -sfn /opt/antigravity-ide/antigravity-ide /usr/local/bin/antigravity

# 4. Create desktop entry
sudo cat > /usr/share/applications/antigravity.desktop <<'EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=Antigravity IDE
GenericName=AI Pair Programming IDE
Comment=Next-generation advanced agentic coding environment
Exec=/usr/local/bin/antigravity %F
Icon=/opt/antigravity-ide/resources/app/resources/linux/code.png
Terminal=false
Categories=Development;IDE;
StartupNotify=true
StartupWMClass=Antigravity IDE
EOF

sudo update-desktop-database /usr/share/applications
```

---

## 6. The Automated 1-Command Solution: `app-install`

To eliminate the manual 5-step checklist friction, we created **`dwm-app-install`** (symlinked as **`app-install`** in `~/.local/bin/app-install`). It automates the entire lifecycle for `.tar.gz`, `.tar.xz`, `.zip`, and uncompressed app folders.

### Usage
```bash
# 1. User-local installation (no root / sudo required):
app-install --user ~/Downloads/ideaIU-2024.1.tar.gz

# 2. Installing an extracted folder (e.g. Antigravity IDE):
app-install --user "$HOME/Downloads/Antigravity IDE" --name antigravity --title "Antigravity IDE"

# 3. System-wide installation (installs to /opt and /usr/local/bin):
sudo app-install ~/Downloads/postman-linux-x64.tar.gz
```

### What `app-install` Automates in Under 1 Second:
1. **Archive Extraction**: Unpacks `.tar.gz`, `.tar.xz`, `.zip`, or pre-extracted folders into `/opt/<name>` (system) or `~/.local/share/<name>` (user).
2. **Binary Auto-Detection**: Scans `bin/` or root for the main executable or startup script.
3. **Icon Auto-Discovery**: Finds the highest-resolution `.svg` or `.png` application logo inside the package.
4. **PATH Trampoline**: Creates an immediate symlink in `/usr/local/bin/<name>` or `~/.local/bin/<name>`.
5. **XDG Desktop Entry Generation**: Creates a compliant `.desktop` file with `Name`, `Exec`, `Icon`, and `StartupWMClass`.
6. **Desktop Index Refresh**: Runs `update-desktop-database` so `dmenu-desktop` and desktop launchers detect the app instantly without restarting the window manager.

---

## 7. Summary Checklist

- [ ] Extracted to `/opt/<app>` (system-wide) or `~/.local/share/<app>` (user-local).
- [ ] Symlink or trampoline script created in `/usr/local/bin` or `~/.local/bin`.
- [ ] `.desktop` file created with valid `Exec`, `Icon`, `Categories`, and `StartupWMClass`.
- [ ] Tested execution via terminal and dynamic desktop launcher (`dmenu-desktop`).
- [ ] `update-desktop-database` executed to refresh application index.
*(All 5 steps above are automated via `app-install`)*
