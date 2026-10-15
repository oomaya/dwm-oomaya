#!/bin/bash

# .xinitrc — used when starting dwm via startx

# 1. Locale & IME Environment Variables (Must precede dwm/dbus)
export LC_CTYPE=en_US.UTF-8
export XMODIFIERS="@im=fcitx"
export GTK_IM_MODULE="fcitx"
export QT_IM_MODULE="fcitx"
export GLFW_IM_MODULE="ibus"

# 2. Start fcitx5 daemon
fcitx5 -d -r

# 3. Launch DWM inside DBus session (Must be at the very bottom)
exec dbus-run-session dwm
