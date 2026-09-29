#!/bin/bash
# seed-default-apps.sh — Seed sane default applications on first run.
#
# Never replaces existing preferences. If any mimeapps.list already exists,
# this exits quietly, preserving the user's (or another tool's) choices.
#
# Supports ubuntu / fedora / debian / arch: for each category we try a
# fallback chain and use the first installed candidate. This is the
# "It Just Works" bootstrap — a fresh account gets working defaults
# without touching anything.
#
# Categories: browser, file-manager, editor, image viewer, video player.
# (Terminal is configured via dwm-default-apps -> hotkeys.toml, not MIME.)
set -euo pipefail
[[ $(id -u) != 0 ]]

config_home=${XDG_CONFIG_HOME:-$HOME/.config}
data_home=${XDG_DATA_HOME:-$HOME/.local/share}

# --- Safety: never overwrite existing preferences ---
shopt -s nullglob
for file in "$config_home/mimeapps.list" "$config_home/"*-mimeapps.list \
	"$data_home/applications/mimeapps.list" "$data_home/applications/"*-mimeapps.list \
	"$data_home/applications/defaults.list"; do
	if [[ -e $file || -L $file ]]; then
		printf 'Preserving existing application defaults: %s\n' "$file"
		exit 0
	fi
done

# --- Resolve installed candidates, first match wins per category ---
associations=$(
	python3 <<'PY'
import configparser
import os
from pathlib import Path
import shutil

roots = [Path(os.environ.get('XDG_DATA_HOME', str(Path.home() / '.local/share')))]
roots += [Path(p) for p in os.environ.get('XDG_DATA_DIRS', '/usr/local/share:/usr/share').split(':') if p]

def find_entry(desktop_ids, command):
    """Return (desktop_id, mimes) for first usable candidate, or None."""
    if shutil.which(command) is None:
        return None
    for desktop in desktop_ids:
        path = next((root / 'applications' / desktop for root in roots
                     if (root / 'applications' / desktop).is_file()), None)
        if path is None:
            continue
        parser = configparser.ConfigParser(interpolation=None, strict=False)
        try:
            parser.read(path)
            app = parser['Desktop Entry']
        except Exception:
            continue
        if app.get('Hidden', 'false').lower() == 'true':
            continue
        if app.get('Type') != 'Application':
            continue
        if app.get('NoDisplay', 'false').lower() == 'true':
            continue
        mimes = [m for m in app.get('MimeType', '').split(';') if m]
        return desktop, mimes
    return None

def seed(category, candidates, want_prefixes):
    """candidates: [(desktop_ids, command)], want_prefixes: mime prefixes to claim."""
    for desktop_ids, command in candidates:
        hit = find_entry(desktop_ids, command)
        if not hit:
            continue
        desktop, mimes = hit
        claimed = [m for m in mimes if m.startswith(want_prefixes)]
        if not claimed:
            continue
        for mime in claimed:
            print(f'{mime}={desktop};')
        print(f'# {category}: {desktop} ({command})', file=__import__('sys').stderr)
        return True
    return False

BROWSER_PREFIXES = ('text/html', 'x-scheme-handler/http', 'application/xhtml', 'application/pdf')

# Browser: brave -> firefox -> chromium
seed('browser', [
    (['brave-browser.desktop', 'brave.desktop'], 'brave'),
    (['firefox.desktop', 'org.mozilla.firefox.desktop'], 'firefox'),
    (['chromium-browser.desktop', 'chromium.desktop', 'org.chromium.Chromium.desktop'], 'chromium'),
], BROWSER_PREFIXES)

# File manager: thunar -> nautilus -> dolphin -> pcmanfm
seed('file-manager', [
    (['thunar.desktop', 'org.xfce.Thunar.desktop'], 'thunar'),
    (['org.gnome.Nautilus.desktop', 'nautilus.desktop'], 'nautilus'),
    (['org.kde.dolphin.desktop', 'dolphin.desktop'], 'dolphin'),
    (['pcmanfm.desktop'], 'pcmanfm'),
], ('inode/directory',))

# Editor (GUI): gedit -> kate -> mousepad ; terminal editors handled via $EDITOR
seed('editor', [
    (['org.gnome.gedit.desktop', 'gedit.desktop', 'org.gnome.TextEditor.desktop'], 'gedit'),
    (['org.kde.kate.desktop', 'kate.desktop'], 'kate'),
    (['mousepad.desktop'], 'mousepad'),
    (['code.desktop', 'visual-studio-code.desktop'], 'code'),
], ('text/plain',))

# Image viewer: sxiv -> eog -> ristretto -> eom
seed('image', [
    (['sxiv.desktop'], 'sxiv'),
    (['org.gnome.eog.desktop', 'eog.desktop'], 'eog'),
    (['ristretto.desktop'], 'ristretto'),
    (['org.mate.eom.desktop', 'eom.desktop'], 'eom'),
], ('image/',))

# Video player: celluloid -> mpv -> vlc
seed('video', [
    (['io.github.celluloid_player.Celluloid.desktop', 'celluloid.desktop'], 'celluloid'),
    (['mpv.desktop'], 'mpv'),
    (['vlc.desktop', 'org.videolan.VLC.desktop'], 'vlc'),
], ('video/', 'audio/'))
PY
)

# --- Atomic publication ---
mkdir -p "$config_home"
target=$(mktemp "$config_home/.dwm-mimeapps.XXXXXX")
trap 'rm -f "$target"' EXIT
printf '[Default Applications]\n%s\n' "$associations" >"$target"
# Hard-link: fails if a preference appeared since our initial check,
# preserving the concurrent choice instead of overwriting it.
if ! ln -T -- "$target" "$config_home/mimeapps.list"; then
	if [[ -e $config_home/mimeapps.list || -L $config_home/mimeapps.list ]]; then
		printf 'Preserving existing application defaults: %s\n' "$config_home/mimeapps.list"
		exit 0
	fi
	printf 'Could not publish application defaults: %s\n' "$config_home/mimeapps.list" >&2
	exit 1
fi
printf 'Seeded default applications (browser, file-manager, editor, image, video).\n'
