#!/usr/bin/env bash
# Shared package capability map for dwm-titus installers and diagnostics.

dwm_packages() {
	local family=$1
	local profile=$2

	case "$family:$profile" in
	fedora:build)
		printf '%s\n' \
			gcc make pkgconf-pkg-config libX11-devel libXft-devel \
			libXinerama-devel libXrender-devel imlib2-devel libxcb-devel \
			xcb-util-devel freetype-devel fontconfig-devel
		;;
	fedora:image-build)
		printf '%s\n' xorriso rsync squashfs-tools-ng isomd5sum python3-pillow fontconfig google-noto-sans-fonts
		;;
	fedora:x11)
		printf '%s\n' xorg-x11-server-Xorg xorg-x11-xinit xrandr xset xsetroot xinput setxkbmap xkbset
		;;
	fedora:runtime-required)
		printf '%s\n' dbus-x11 curl git procps-ng psmisc unzip util-linux xclip xdotool xprop xdg-utils
		;;
	fedora:desktop)
		# Fedora 44 publishes the compatible Quickshell snapshot in its official
		# fedora/updates repositories. It is required and belongs in the strict
		# desktop transaction; the Fedora package-map check proves availability.
		printf '%s\n' \
			quickshell dunst picom feh dex-autostart mate-polkit xsettingsd \
			alsa-utils brightnessctl dbus-tools inotify-tools jq pulseaudio-utils pipewire pavucontrol \
			pipewire-pulseaudio wireplumber libnotify light-locker xorg-x11-drv-libinput \
			bluez blueman playerctl upower power-profiles-daemon flatpak xdg-desktop-portal-gtk
		;;
	fedora:system-management)
		printf '%s\n' \
			PackageKit PackageKit-glib python3-gobject python3-rpm accountsservice cups \
			system-config-printer
		;;
	fedora:system-management-optional)
		printf '%s\n' lxqt-admin dnfdragora
		;;
	fedora:source-update)
		# Dependencies introduced after the initial installation that the supported
		# source-checkout synchronization path must reconcile for existing systems.
		printf '%s\n' xsettingsd xkbset
		;;
	fedora:desktop-optional)
		printf '%s\n' \
			Thunar gvfs gvfs-smb tumbler thunar-archive-plugin file-roller \
			xdg-user-dirs gnome-keyring gnome-keyring-pam NetworkManager \
			rsync
		;;
	fedora:gaming)
		if [[ ${ARCH:-$(uname -m)} == x86_64 ]]; then
			printf '%s\n' \
				steam gamescope gamemode.x86_64 gamemode.i686 \
				mangohud.x86_64 mangohud.i686
		fi
		;;
	fedora:theme)
		printf '%s\n' dconf
		;;
	fedora:theme-gtk)
		printf '%s\n' \
			arc-theme adw-gtk3-theme numix-gtk-theme \
			yaru-gtk3-theme yaru-gtk4-theme deepin-gtk-theme \
			bluebird-gtk3-theme
		;;
	fedora:theme-optional)
		printf '%s\n' qt6ct qt5ct
		;;
	fedora:fonts)
		printf '%s\n' google-noto-color-emoji-fonts google-noto-sans-mono-fonts
		;;
	fedora:qml-development)
		printf '%s\n' qt6-qtdeclarative-devel
		;;
	fedora:qml-validation)
		printf '%s\n' quickshell
		dwm_packages "$family" qml-development
		;;
	fedora:lightdm)
		printf '%s\n' lightdm slick-greeter
		;;
	fedora:terminal)
		printf '%s\n' alacritty kitty
		;;
	fedora:terminal-primary)
		printf '%s\n' alacritty
		;;
	fedora:screenshot-optional)
		printf '%s\n' maim
		;;
	fedora:required)
		dwm_packages "$family" build
		dwm_packages "$family" x11
		dwm_packages "$family" runtime-required
		;;
	fedora:recommended)
		dwm_packages "$family" desktop
		dwm_packages "$family" system-management
		dwm_packages "$family" screenshot-optional
		dwm_packages "$family" theme
		dwm_packages "$family" theme-gtk
		dwm_packages "$family" fonts
		;;
	fedora:optional)
		dwm_packages "$family" theme-optional
		dwm_packages "$family" desktop-optional
		dwm_packages "$family" system-management-optional
		;;
	fedora:full)
		dwm_packages "$family" required
		dwm_packages "$family" recommended
		dwm_packages "$family" optional
		dwm_packages "$family" gaming
		;;
	arch:build)
		printf '%s\n' \
			gcc make pkgconf libx11 libxft libxinerama libxrender \
			imlib2 libxcb xcb-util freetype2 fontconfig
		;;
	arch:x11)
		printf '%s\n' \
			xorg-server xorg-xinit xorg-xrandr xorg-xset xorg-xsetroot \
			xorg-xinput xorg-setxkbmap
		;;
	arch:runtime-required)
		printf '%s\n' \
			dbus curl git procps-ng psmisc unzip util-linux xclip xdotool xorg-xprop xdg-utils
		;;
	arch:desktop)
		printf '%s\n' \
			dunst picom feh dex inotify-tools jq \
			alsa-utils brightnessctl libnotify playerctl
		;;
	arch:desktop-optional)
		printf '%s\n' \
			pipewire pipewire-pulse wireplumber pavucontrol \
			bluez blueman upower power-profiles-daemon flatpak \
			xdg-desktop-portal-gtk thunar gvfs file-roller
		;;
	arch:system-management)
		printf '%s\n' packagekit python-gobject accountsservice cups system-config-printer
		;;
	arch:terminal)
		printf '%s\n' alacritty kitty ghostty foot
		;;
	arch:terminal-primary)
		printf '%s\n' alacritty
		;;
	arch:screenshot-optional)
		printf '%s\n' maim
		;;
	arch:fonts)
		printf '%s\n' noto-fonts noto-fonts-emoji noto-fonts-cjk
		;;
	arch:theme)
		printf '%s\n' dconf
		;;
	arch:theme-gtk)
		printf '%s\n' arc-gtk-theme
		;;
	arch:theme-optional)
		printf '%s\n' qt6ct qt5ct
		;;
	arch:lightdm)
		printf '%s\n' lightdm slick-greeter
		;;
	arch:qml-development)
		printf '%s\n' qt6-declarative
		;;
	arch:qml-validation)
		printf '%s\n' quickshell qt6-declarative
		;;
	arch:required)
		dwm_packages "$family" build
		dwm_packages "$family" x11
		dwm_packages "$family" runtime-required
		;;
	arch:recommended)
		dwm_packages "$family" desktop
		dwm_packages "$family" screenshot-optional
		dwm_packages "$family" theme
		dwm_packages "$family" theme-gtk
		dwm_packages "$family" fonts
		;;
	arch:optional)
		dwm_packages "$family" theme-optional
		dwm_packages "$family" desktop-optional
		dwm_packages "$family" system-management
		;;
	arch:full)
		dwm_packages "$family" required
		dwm_packages "$family" recommended
		dwm_packages "$family" optional
		;;
	debian:build)
		printf '%s\n' \
			build-essential pkg-config libx11-dev libxft-dev \
			libxinerama-dev libxrender-dev libimlib2-dev libxcb1-dev \
			libxcb-res0-dev libxcb-util-dev libfontconfig1-dev libfreetype-dev \
			libx11-xcb-dev
		;;
	debian:x11)
		printf '%s\n' \
			xorg x11-xserver-utils x11-utils x11-xkb-utils xinput
		;;
	debian:runtime-required)
		printf '%s\n' \
			dbus-x11 curl git procps psmisc unzip util-linux xclip xdotool xdg-utils
		;;
	debian:desktop)
		printf '%s\n' \
			dunst picom feh dex inotify-tools jq \
			alsa-utils brightnessctl libnotify-bin pulseaudio-utils playerctl
		;;
	debian:desktop-optional)
		printf '%s\n' \
			thunar gvfs file-roller bluez blueman upower \
			flatpak xdg-desktop-portal-gtk
		;;
	debian:system-management)
		printf '%s\n' packagekit python3-gi accountsservice cups system-config-printer
		;;
	debian:terminal)
		printf '%s\n' alacritty kitty
		;;
	debian:terminal-primary)
		printf '%s\n' alacritty
		;;
	debian:screenshot-optional)
		printf '%s\n' maim
		;;
	debian:fonts)
		printf '%s\n' fonts-noto-core fonts-noto-color-emoji
		;;
	debian:theme)
		printf '%s\n' dconf-cli
		;;
	debian:theme-gtk)
		printf '%s\n' arc-theme
		;;
	debian:theme-optional)
		printf '%s\n' qt6ct qt5ct
		;;
	debian:lightdm)
		printf '%s\n' lightdm slick-greeter
		;;
	debian:required)
		dwm_packages "$family" build
		dwm_packages "$family" x11
		dwm_packages "$family" runtime-required
		;;
	debian:recommended)
		dwm_packages "$family" desktop
		dwm_packages "$family" screenshot-optional
		dwm_packages "$family" theme
		dwm_packages "$family" theme-gtk
		dwm_packages "$family" fonts
		;;
	debian:optional)
		dwm_packages "$family" theme-optional
		dwm_packages "$family" desktop-optional
		dwm_packages "$family" system-management
		;;
	debian:full)
		dwm_packages "$family" required
		dwm_packages "$family" recommended
		dwm_packages "$family" optional
		;;
	*)
		return 1
		;;
	esac
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then
	[[ $# == 2 ]] || {
		printf 'usage: %s FAMILY PROFILE\n' "$0" >&2
		exit 2
	}
	dwm_packages "$1" "$2"
	exit $?
fi

dwm_install_package_profile() {
	local profile=$1
	local packages=()
	local package

	while IFS= read -r package; do
		[[ -n $package ]] || continue
		if [[ $package == power-profiles-daemon ]] && dwm_power_profiles_provider_installed; then
			printf '%s\n' \
				'Retaining installed Power Profiles provider (ppd-service); skipping power-profiles-daemon.' >&2
			continue
		fi
		packages+=("$package")
	done < <(dwm_packages "$DISTRO_FAMILY" "$profile")

	if ((${#packages[@]} == 0)); then
		return 0
	fi

	install_packages "${packages[@]}"
}

dwm_power_profiles_provider_installed() {
	command -v rpm >/dev/null 2>&1 &&
		rpm -q --whatprovides ppd-service >/dev/null 2>&1
}

dwm_install_available_package_profile() {
	local profile=$1
	local package
	local status=0

	while IFS= read -r package; do
		[[ -n $package ]] || continue
		if ! install_optional_package "$package"; then
			printf 'Skipping unavailable optional package: %s\n' "$package" >&2
			status=1
		fi
	done < <(dwm_packages "$DISTRO_FAMILY" "$profile")

	return "$status"
}

dwm_install_first_available_package() {
	local package

	for package in "$@"; do
		if install_optional_package "$package" 2>/dev/null; then
			return 0
		fi
	done

	return 1
}

dwm_install_first_available_profile() {
	local profile=$1
	local packages=()
	local package

	while IFS= read -r package; do
		[[ -n $package ]] && packages+=("$package")
	done < <(dwm_packages "$DISTRO_FAMILY" "$profile")

	if ((${#packages[@]} == 0)); then
		return 1
	fi

	dwm_install_first_available_package "${packages[@]}"
}
