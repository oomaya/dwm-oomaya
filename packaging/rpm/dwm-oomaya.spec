Name:           dwm-oomaya
Version:        0.8.0
Release:        1%{?dist}
Summary:        Sovereign, ultra-fast dynamic window manager and desktop ecosystem
License:        MIT AND GPL-3.0-or-later
URL:            https://github.com/oomaya/dwm-oomaya
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkgconf-pkg-config
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xft)
BuildRequires:  pkgconfig(xinerama)
BuildRequires:  pkgconfig(xrender)
BuildRequires:  pkgconfig(imlib2)
BuildRequires:  pkgconfig(x11-xcb)
BuildRequires:  pkgconfig(xcb)
BuildRequires:  pkgconfig(xcb-res)
BuildRequires:  pkgconfig(fontconfig)
BuildRequires:  pkgconfig(freetype2)

Requires:       xorg-x11-server-Xorg
Requires:       xorg-x11-xinit
Requires:       xrandr
Requires:       xset
Requires:       xsetroot
Requires:       xinput
Requires:       setxkbmap
Requires:       xclip
Requires:       xdotool
Requires:       xprop
Requires:       dunst
Requires:       picom
Requires:       feh
Requires:       dex-autostart
Requires:       mate-polkit
Requires:       xsettingsd

Recommends:     quickshell-oomaya
Recommends:     dmenu-oomaya
Recommends:     brightnessctl
Recommends:     playerctl
Recommends:     pavucontrol
Recommends:     bluez
Recommends:     blueman
Recommends:     upower
Recommends:     power-profiles-daemon

%description
dwm-oomaya is a customized, sovereign dynamic window manager built on pure
Suckless C foundations. It provides native status2d bar integration,
instant keybinding dispatch, multi-monitor workspace reconciliation,
and an ultra-responsive native control center and settings ecosystem.
Qt6/QML desktop overlays are supported via the optional quickshell-oomaya
companion package.

%prep
%autosetup -n %{name}-%{version}

%build
%set_build_flags
make all PREFIX=%{_prefix}

%install
make install-system \
    DESTDIR=%{buildroot} \
    PREFIX=%{_prefix} \
    XSESSIONSDIR=%{_datadir}/xsessions \
    SYSTEMDUSERDIR=%{_prefix}/lib/systemd/user \
    DATADIR=%{_datadir} \
    MANPREFIX=%{_mandir}

# Install default config templates
install -dm0755 %{buildroot}%{_datadir}/%{name}/config
install -pm0644 config/hotkeys.toml %{buildroot}%{_datadir}/%{name}/config/
install -pm0644 config/themes.toml %{buildroot}%{_datadir}/%{name}/config/
install -pm0644 config/window-rules.toml %{buildroot}%{_datadir}/%{name}/config/

%files
%license LICENSE assets/cursors/COPYING
%doc README.md CHANGELOG.md ROADMAP.md
%{_bindir}/dwm
%{_bindir}/dwm-oomaya
%{_bindir}/dwm-oomayad
%{_bindir}/oomaya-ctl
%{_bindir}/dwm-msg
%{_bindir}/dwm-msg-compat
%{_bindir}/active-audio
%{_bindir}/agy-update
%{_bindir}/autostart.sh
%{_bindir}/autostop.sh
%{_bindir}/check-deps.sh
%{_bindir}/dmenu-find
%{_bindir}/dwm-accessibility-settings
%{_bindir}/dwm-app-install
%{_bindir}/dwm-clock
%{_bindir}/dwm-controlcenter
%{_bindir}/dwm-cursor-reload
%{_bindir}/dwm-default-apps
%{_bindir}/dwm-diagnostics
%{_bindir}/dwm-display-profile
%{_bindir}/dwm-display-setup
%{_bindir}/dwm-flow
%{_bindir}/dwm-git-helper.sh
%{_bindir}/dwm-keybind-exec
%{_bindir}/dwm-keybinds
%{_bindir}/dwm-lock
%{_bindir}/dwm-lock-watch
%{_bindir}/dwm-oomaya-release
%{_bindir}/dwm-packages.sh
%{_bindir}/dwm-panel-settings
%{_bindir}/dwm-polkit
%{_bindir}/dwm-scratchpad
%{_bindir}/dwm-screenshot
%{_bindir}/dwm-session-launch
%{_bindir}/dwm-settings
%{_bindir}/dwm-settings-appearance
%{_bindir}/dwm-settings-display
%{_bindir}/dwm-settings-display-profiles
%{_bindir}/dwm-settings-font
%{_bindir}/dwm-settings-hub
%{_bindir}/dwm-settings-input
%{_bindir}/dwm-settings-personalization
%{_bindir}/dwm-settings-provider
%{_bindir}/dwm-settings-theme
%{_bindir}/dwm-settings-wallpaper
%{_bindir}/dwm-shell-mode
%{_bindir}/dwm-status
%{_bindir}/dwm-system-health
%{_bindir}/dwm-system-management
%{_bindir}/dwm-terminal
%{_bindir}/dwm-utils.sh
%{_bindir}/dwm-volume
%{_bindir}/dwm-weather
%{_bindir}/dwm-xdg-autostart
%{_bindir}/dwm-xsettings
%{_bindir}/install-herdr
%{_bindir}/migrate-graphical-session.sh
%{_bindir}/nvidia-gpu
%{_bindir}/nvidia-suspend-test.sh
%{_bindir}/nvidia-temp
%{_bindir}/pkg-scan.py
%{_bindir}/power-management.sh
%{_bindir}/protonrestart
%{_bindir}/seed-autostart-overrides.sh
%{_bindir}/seed-default-apps.sh
%{_bindir}/theme-apply.sh
%{_bindir}/validate-desktop-command
%{_bindir}/validate-hotkeys
%{_bindir}/webapp-create
%{_bindir}/webapp-launch
%{_bindir}/xdg-enable-autostart.sh
%{_bindir}/xscreensaver-setup.sh
%{_libexecdir}/%{name}/dwm-settings-display-root
%{_prefix}/lib/systemd/user/dwm-oomayad.service
%{_datadir}/xsessions/dwm.desktop
%{_mandir}/man1/dwm.1*
%{_datadir}/icons/Capitaine-Cursors
%{_datadir}/icons/Capitaine-Cursors-White
%{_datadir}/%{name}

%changelog
* Mon Oct 19 2026 oomaya <jack.gsit@gmail.com> - 0.8.0-1
- Initial pure C sovereign package with Quickshell extracted to companion plugin
- Full suckless status2d bar, native control center, and settings ecosystem
