/* See LICENSE file for copyright and license details. */

#include <X11/XF86keysym.h>

/* appearance */
static const unsigned int refresh_rate  = 60;    /* matches dwm's mouse event processing to your monitor's refresh rate */
static const unsigned int enable_noborder = 1;  /* toggles noborder feature (0=disabled, 1=enabled) */
static const int cursorwarp             = 1;    /* 1 means warp cursor to center of focused window/monitor */
static const unsigned int borderpx      = 1;    /* border pixel of windows */
static const unsigned int default_border= 0;    /* to switch back to default border after dynamic border resizing */
static const unsigned int snap          = 32;   /* snap pixel */
static const unsigned int gappih        = 10;   /* horiz inner gap between windows */
static const unsigned int gappiv        = 10;   /* vert inner gap between windows */
static const unsigned int gappoh        = 10;   /* horiz outer gap between windows and screen edge */
static const unsigned int gappov        = 10;   /* vert outer gap between windows and screen edge */
static const int smartgaps              = 0;    /* 1 means no outer gap when there is only one window */
static const int showbar                = 1;    /* 0 means no bar */
static const int showtab                = showtab_auto;
static const int toptab                 = 1;    /* 0 means bottom tab */
static const int topbar                 = 1;    /* 0 means bottom bar */
static const int swallowfloating        = 0;    /* 1 means swallow floating windows by default */
#define ICONSIZE                        17      /* icon size */
#define ICONSPACING                     5       /* space between icon and title */
#define SHOWWINICON                     1       /* 0 means no winicon */

/* Fonts */
static const char dmenufont[]           = "MesloLGS Nerd Font Mono:size=16";
static const char *fonts[]              = { "MesloLGS Nerd Font Mono:size=14:antialias=true:autohint=true", "NotoColorEmoji:pixelsize=14:antialias=true:autohint=true" };

/* Theme (TokyoNight) */
#include "themes/tokyonight.h"

static const char *colors[][3]          = {
    /*                     fg       bg      border */
    [SchemeNorm]       = { gray3,   black,  gray2 },
    [SchemeSel]        = { gray4,   blue,   blue  },
    [SchemeTitle]      = { white,   black,  black  }, // active window title
    [TabSel]           = { blue,    gray2,  black },
    [TabNorm]          = { gray3,   black,  black },
    [SchemeTag]        = { gray3,   black,  black },
    [SchemeTag1]       = { blue,    black,  black },
    [SchemeTag2]       = { red,     black,  black },
    [SchemeTag3]       = { orange,  black,  black },
    [SchemeTag4]       = { green,   black,  black },
    [SchemeTag5]       = { pink,    black,  black },
    [SchemeLayout]     = { green,   black,  black },
    [SchemeBtnPrev]    = { green,   black,  black },
    [SchemeBtnNext]    = { yellow,  black,  black },
    [SchemeBtnClose]   = { red,     black,  black },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const char ptagf[] = "[%s %s]";	/* format of a tag label */
static const char etagf[] = "[%s]";	/* format of an empty tag */
static const int lcaselbl = 0;		/* 1 means make tag label lowercase */

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   alwaysontop  isterminal  noswallow  monitor  unmanaged */
	{ "Gimp",     NULL,       NULL,       0,            1,           0,           0,          0,         -1,      0 },
	{ "Firefox",  NULL,       NULL,       1 << 8,       0,           0,           0,         -1,         -1,      0 },
	{ "ghostty",  NULL,       NULL,       0,            0,           0,           1,          0,         -1,      0 },
	{ "kitty",    NULL,       NULL,       0,            0,           0,           1,          0,         -1,      0 },
	{ "dwm-scratchpad", NULL, NULL,       0,            1,           1,           1,          0,         -1,      0 },
	{ "vmware-user", NULL,    NULL,       0,            0,           0,           0,          0,         -1,      1 },
	{ "vmtoolsd",    NULL,    NULL,       0,            0,           0,           0,          0,         -1,      1 },
	{ "VBoxClient",  NULL,    NULL,       0,            0,           0,           0,          0,         -1,      1 },
};

/* layout(s) */
static const float mfact     = 0.50; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

#define FORCE_VSPLIT 1  /* nrowgrid layout: force two clients to always split vertically */
#include "functions.h"

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=",      tile },    /* first entry is default */
    { "[M]",      monocle },
    { "[@]",      spiral },
    { "[\\]",     dwindle },
    { "H[]",      deck },
    { "TTT",      bstack },
    { "===",      bstackhoriz },
    { "HHH",      grid },
    { "###",      nrowgrid },
    { "---",      horizgrid },
    { ":::",      gaplessgrid },
    { "|M|",      centeredmaster },
    { ">M>",      centeredfloatingmaster },
    { "><>",      NULL },    /* no layout function means floating behavior */
    { NULL,       NULL },
};

/* key definitions */
#define MODKEY Mod4Mask
#define PREFER_HEADER_KEYS 0
#define STATUSBAR "dwmblocks"

#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands (Zero Hardcoded Literals compliant via PATH lookup) */
static const char *upvol[]      = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%",    NULL };
static const char *downvol[]    = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%",    NULL };
static const char *mutevol[]    = { "pactl", "set-sink-mute",   "@DEFAULT_SINK@", "toggle", NULL };
static const char *light_up[]   = { "light", "-A", "5", NULL };
static const char *light_down[] = { "light", "-U", "5", NULL };

static const Key keys[] = {
    /* modifier                         key         function        argument */

    // brightness and audio (native XF86 keysyms, no sxhkd required)
    { 0,                                XF86XK_AudioLowerVolume, spawn, {.v = downvol} },
    { 0,                                XF86XK_AudioMute,        spawn, {.v = mutevol } },
    { 0,                                XF86XK_AudioRaiseVolume, spawn, {.v = upvol} },
    { 0,                                XF86XK_MonBrightnessUp,   spawn, {.v = light_up} },
    { 0,                                XF86XK_MonBrightnessDown, spawn, {.v = light_down} },

    // screenshot fullscreen and cropped
    { MODKEY|ControlMask,               XK_u,       spawn,          SHCMD("maim | xclip -selection clipboard -t image/png") },
    { MODKEY,                           XK_u,       spawn,          SHCMD("maim --select | xclip -selection clipboard -t image/png") },

    // application launchers & terminals (Ghostty/Kitty support)
    { MODKEY,                           XK_c,       spawn,          SHCMD("dwm-flow") },
    { MODKEY|ShiftMask,                 XK_c,       spawn,          SHCMD("dwm-flow") },
    { MODKEY,                           XK_d,       spawn,          SHCMD("dmenu-desktop") },
    { MODKEY|ShiftMask,                 XK_Return,  spawn,          SHCMD("ghostty || kitty") },
    { MODKEY,                           XK_a,       spawn,          SHCMD("antigravity-ide") },
    { MODKEY|ShiftMask,                 XK_a,       spawn,          SHCMD("ghostty -e antigravity || kitty -e antigravity") },
    { MODKEY,                           XK_s,       spawn,          SHCMD("dwm-scratchpad") },
    { MODKEY,                           XK_slash,   spawn,          SHCMD("quickshell ipc --path \"${XDG_CONFIG_HOME:-$HOME/.config}/quickshell/shell.qml\" call controlcenter openKeybinds || dwm-keybinds") },
    { Mod1Mask,                         XK_p,       spawn,          SHCMD("dmenu-run") },
    { Mod1Mask,                         XK_x,       spawn,          SHCMD("dmenu-power") },
    { Mod1Mask,                         XK_Tab,     spawn,          SHCMD("dmenu-windows") },
    { MODKEY|ShiftMask,                 XK_h,       spawn,          SHCMD("dmenu-hub") },
    { MODKEY,                           XK_Print,   spawn,          SHCMD("dmenu-scrot") },

    // toggle stuff
    { MODKEY,                           XK_b,       togglebar,      {0} },
    { MODKEY|ControlMask,               XK_t,       togglegaps,     {0} },
    { MODKEY|ShiftMask,                 XK_space,   togglefloating, {0} },
    { MODKEY,                           XK_f,       togglefullscr,  {0} },
    { MODKEY|ControlMask,               XK_w,       tabmode,        { -1 } },

    // focus & master count
    { MODKEY,                           XK_j,       focusstack,     {.i = +1 } },
    { MODKEY,                           XK_k,       focusstack,     {.i = -1 } },
    { MODKEY,                           XK_i,       incnmaster,     {.i = +1 } },
    { MODKEY|ShiftMask,                 XK_i,       incnmaster,     {.i = -1 } },

    // shift view
    { MODKEY,                           XK_Left,    shiftview,      {.i = -1 } },
    { MODKEY,                           XK_Right,   shiftview,      {.i = +1 } },

    // change mfact & cfact sizes
    { MODKEY,                           XK_h,       setmfact,       {.f = -0.05} },
    { MODKEY,                           XK_l,       setmfact,       {.f = +0.05} },
    { MODKEY|ControlMask,               XK_h,       setcfact,       {.f = +0.25} },
    { MODKEY|ControlMask,               XK_l,       setcfact,       {.f = -0.25} },
    { MODKEY|ShiftMask,                 XK_o,       setcfact,       {.f =  0.00} },

    // stack movement & zoom
    { MODKEY|ShiftMask,                 XK_j,       movestack,      {.i = +1 } },
    { MODKEY|ShiftMask,                 XK_k,       movestack,      {.i = -1 } },
    { MODKEY,                           XK_Return,  zoom,           {0} },
    { MODKEY,                           XK_Tab,     cyclelayout,    {.i = +1 } },
    { MODKEY|ShiftMask,                 XK_Tab,     cyclelayout,    {.i = -1 } },

    // overall gaps (suckless vanitygaps keybinds + aliases)
    { MODKEY|Mod1Mask,                  XK_0,       togglegaps,     {0} },
    { MODKEY|Mod1Mask,                  XK_equal,   incrgaps,       {.i = +5 } },
    { MODKEY|Mod1Mask,                  XK_minus,   incrgaps,       {.i = -5 } },
    { MODKEY|Mod1Mask|ShiftMask,        XK_equal,   defaultgaps,    {0} },
    { MODKEY|ControlMask,               XK_i,       incrgaps,       {.i = +5 } },
    { MODKEY|ControlMask,               XK_d,       incrgaps,       {.i = -5 } },

    // inner gaps
    { MODKEY|ShiftMask,                 XK_i,       incrigaps,      {.i = +5 } },
    { MODKEY|ControlMask|ShiftMask,     XK_i,       incrigaps,      {.i = -5 } },

    // outer gaps
    { MODKEY|ControlMask,               XK_o,       incrogaps,      {.i = +5 } },
    { MODKEY|ControlMask|ShiftMask,     XK_o,       incrogaps,      {.i = -5 } },

    // inner+outer hori, vert gaps
    { MODKEY|ControlMask,               XK_6,       incrihgaps,     {.i = +5 } },
    { MODKEY|ControlMask|ShiftMask,     XK_6,       incrihgaps,     {.i = -5 } },
    { MODKEY|ControlMask,               XK_7,       incrivgaps,     {.i = +5 } },
    { MODKEY|ControlMask|ShiftMask,     XK_7,       incrivgaps,     {.i = -5 } },
    { MODKEY|ControlMask,               XK_8,       incrohgaps,     {.i = +5 } },
    { MODKEY|ControlMask|ShiftMask,     XK_8,       incrohgaps,     {.i = -5 } },
    { MODKEY|ControlMask,               XK_9,       incrovgaps,     {.i = +5 } },
    { MODKEY|ControlMask|ShiftMask,     XK_9,       incrovgaps,     {.i = -5 } },
    { MODKEY|ControlMask|ShiftMask,     XK_d,       defaultgaps,    {0} },

    // layout selection & cycling
    { MODKEY,                           XK_t,       setlayout,      {.v = &layouts[0]} },
    { MODKEY|ShiftMask,                 XK_f,       setlayout,      {.v = &layouts[1]} },
    { MODKEY,                           XK_m,       setlayout,      {.v = &layouts[1]} },
    { MODKEY|ControlMask,               XK_g,       setlayout,      {.v = &layouts[10]} },
    { MODKEY|ControlMask|ShiftMask,     XK_t,       setlayout,      {.v = &layouts[13]} },
    { MODKEY,                           XK_space,   setlayout,      {0} },
    { MODKEY|ControlMask,               XK_comma,   cyclelayout,    {.i = -1 } },
    { MODKEY|ControlMask,               XK_period,  cyclelayout,    {.i = +1 } },
    { MODKEY,                           XK_bracketleft, cyclelayout, {.i = -1 } },
    { MODKEY,                           XK_bracketright, cyclelayout, {.i = +1 } },

    // tag & monitor view
    { MODKEY,                           XK_0,       view,           {.ui = ~0 } },
    { MODKEY|ShiftMask,                 XK_0,       tag,            {.ui = ~0 } },
    { MODKEY,                           XK_comma,   focusmon,       {.i = -1 } },
    { MODKEY,                           XK_period,  focusmon,       {.i = +1 } },
    { MODKEY|ShiftMask,                 XK_comma,   tagmon,         {.i = -1 } },
    { MODKEY|ShiftMask,                 XK_period,  tagmon,         {.i = +1 } },

    // change border size
    { MODKEY|ShiftMask,                 XK_minus,   setborderpx,    {.i = -1 } },
    { MODKEY|ShiftMask,                 XK_p,       setborderpx,    {.i = +1 } },
    { MODKEY|ShiftMask,                 XK_w,       setborderpx,    {.i = default_border } },

    // lifecycle
    { MODKEY|ControlMask,               XK_q,       spawn,          SHCMD("killall bar.sh dwm") },
    { MODKEY,                           XK_q,       killclient,     {0} },
    { MODKEY|ShiftMask,                 XK_r,       restart,        {0} },

    // hide & restore windows
    { MODKEY,                           XK_e,       hidewin,        {0} },
    { MODKEY|ShiftMask,                 XK_e,       restorewin,     {0} },

    TAGKEYS(                            XK_1,                       0)
    TAGKEYS(                            XK_2,                       1)
    TAGKEYS(                            XK_3,                       2)
    TAGKEYS(                            XK_4,                       3)
    TAGKEYS(                            XK_5,                       4)
    TAGKEYS(                            XK_6,                       5)
    TAGKEYS(                            XK_7,                       6)
    TAGKEYS(                            XK_8,                       7)
    TAGKEYS(                            XK_9,                       8)
};

/* button definitions */
static const Button buttons[] = {
    /* click                event mask      button          function        argument */
    { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
    { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[1]} },
    { ClkWinTitle,          0,              Button2,        zoom,           {0} },
    { ClkStatusText,        0,              Button2,        spawn,          SHCMD("ghostty || kitty") },
    { ClkClientWin,         MODKEY,         Button1,        moveorplace,    {.i = 0} },
    { ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
    { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
    { ClkClientWin,         ControlMask,    Button1,        dragmfact,      {0} },
    { ClkClientWin,         ControlMask,    Button3,        dragcfact,      {0} },
    { ClkTagBar,            0,              Button1,        view,           {0} },
    { ClkTagBar,            0,              Button3,        toggleview,     {0} },
    { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
    { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
    { ClkTabBar,            0,              Button1,        focuswin,       {0} },
    { ClkTabPrev,           0,              Button1,        movestack,      { .i = -1 } },
    { ClkTabNext,           0,              Button1,        movestack,      { .i = +1 } },
    { ClkTabClose,          0,              Button1,        killclient,     {0} },
};
