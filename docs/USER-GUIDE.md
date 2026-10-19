# DWM-Oomaya Comprehensive User Guide & Manual

**DWM-Oomaya**는 순수 C 언어(Suckless C99/C11) 기반의 초고속, 무결점 다이내믹 타일링 윈도우 매니저이자 주권적(Sovereign) 데스크톱 에코시스템입니다.

이 가이드는 최초 설치부터 단축키 숙달, 커스터마이징, 유지보수 및 무중단 업데이트까지 필요한 모든 사용자 지침을 다룹니다.

---

## 목차
1. [시스템 아키텍처 개요](#1-시스템-아키텍처-개요)
2. [지원 배포판 및 사전 요구사항](#2-지원-배포판-및-사전-요구사항)
3. [설치 가이드 (Installation)](#3-설치-가이드-installation)
4. [세션 시작 및 진입 (Session Launch)](#4-세션-시작-및-진입-session-launch)
5. [단축키 전체 일람 (Complete Keybindings Matrix)](#5-단축키-전체-일람-complete-keybindings-matrix)
6. [제어판 & 설정 허브 (Control Center & Settings)](#6-제어판--설정-허브-control-center--settings)
7. [상태 바 및 위젯 (Status Bar & Widgets)](#7-상태-바-및-위젯-status-bar--widgets)
8. [셸 모드 전환 (Suckless vs Quickshell)](#8-셸-모드-전환-suckless-vs-quickshell)
9. [설정 커스터마이징 (TOML Configuration)](#9-설정-커스터마이징-toml-configuration)
10. [업데이트 및 유지보수 (Update & Maintenance)](#10-업데이트-및-유지보수-update--maintenance)
11. [시스템 진단 및 트러블슈팅 (Troubleshooting)](#11-시스템-진단-및-트러블슈팅-troubleshooting)

---

## 1. 시스템 아키텍처 개요

DWM-Oomaya는 무거운 런타임에 종속되지 않는 **주권적 데스크톱(Sovereign Desktop)**을 지향합니다:
- **Pure C Core**: Window Manager(`dwm`), 상태 캐시 및 이벤트 데몬(`dwm-oomayad`), 네이티브 메뉴(`dmenu-oomaya`)는 모두 순수 C로 컴파일되어 1MB 이하의 극소 메모리만을 소비합니다.
- **Zero-Dependency by Default**: Qt6, QML, Python 등의 무거운 런타임 없이도 상단 바(Status2d), 런처, 제어판, 설정 허브, 볼륨/밝기 OSD가 100% 네이티브로 완벽 작동합니다.
- **Optional Companion Plugin**: 화려한 QML 오버레이를 원하는 사용자는 선택형 컴패니언 레포지토리([`quickshell-oomaya`](https://github.com/oomaya/quickshell-oomaya))를 자유롭게 추가 장착할 수 있습니다.

---

## 2. 지원 배포판 및 사전 요구사항

DWM-Oomaya는 다음 배포판에서 100% 네이티브 호환 및 패키지 맵이 검증되었습니다:
- **Fedora**: Fedora 41, 42, 43, 44+ (Workstation, Silverblue/Kinoite, Minimal)
- **Arch Linux & CachyOS**: Arch Linux, CachyOS, EndeavourOS, Manjaro
- **Debian Family**: Debian 12/13 (Bookworm/Trixie), Ubuntu LTS (22.04/24.04/26.04), Linux Mint, Peppermint OS, Pop!_OS

---

## 3. 설치 가이드 (Installation)

### 방법 A: 통합 자동 설치 스크립트 (권장)
최소 설치(Netinst/Minimal) 환경에서도 모든 필수 의존성 패키지와 폰트, 테마, 기본 설정을 자동으로 배포합니다:

```bash
# 저장소 클론
git clone https://github.com/oomaya/dwm-oomaya.git ~/.local/src/dwm-oomaya
cd ~/.local/src/dwm-oomaya

# 대화형 설치 (또는 --desktop 플래그 사용)
./install.sh --desktop
```

### 방법 B: 네이티브 패키지 매니저 설치
배포판별 빌드 스펙이 `packaging/` 디렉토리에 완비되어 있습니다:

- **Fedora (RPM)**:
  ```bash
  # RPM 패키지 빌드
  rpmbuild -ba packaging/rpm/dwm-oomaya.spec
  sudo dnf install ~/rpmbuild/RPMS/x86_64/dwm-oomaya-0.8.0-*.rpm
  ```
- **Arch Linux / CachyOS (PKGBUILD)**:
  ```bash
  cd packaging/arch
  makepkg -si
  ```

### 방법 C: 소스 직접 빌드 및 시스템 설치
```bash
make clean all
# 시스템 전체 설치 (/usr/local/bin, /usr/share/xsessions)
sudo make install-system

# 또는 현재 사용자 로컬 디렉토리에만 설치 (~/.local/bin)
make install-user
```

---

## 4. 세션 시작 및 진입 (Session Launch)

### 디스플레이 매니저 (GDM, LightDM, SDDM)
설치 완료 후 로그아웃하면 디스플레이 매니저의 세션 목록에 **DWM** 또는 **dwm-oomaya** 항목이 자동으로 등록됩니다. 세션을 선택하고 로그인하십시오.

### startx / xinit 사용 시
`~/.xinitrc` 파일 끝에 다음 라인을 추가합니다:
```bash
exec dwm-session-launch
```

---

## 5. 단축키 전체 일람 (Complete Keybindings Matrix)

> **기본 모디파이어 키**: `Super` = Windows/Command 키, `Alt` = Option 키

### 1) 핵심 애플리케이션 및 런처
| 기능 | 단축키 | 실행 명령 |
| :--- | :--- | :--- |
| **기본 터미널** | `Super + Shift + Return` | Ghostty / Kitty / Alacritty |
| **플로팅 스크래치패드** | `Super + s` | `dwm-scratchpad` (중앙 팝업 터미널) |
| **파일 관리자** | `Super + e` | Thunar / xdg-open |
| **dmenu 앱 런처** | `Super + d` | `dmenu-desktop` |
| **명령어 직접 실행** | `Alt + p` | `dmenu-run` |
| **실행 중인 창 전환** | `Alt + Tab` | `dmenu-windows` |
| **단축키 안내 팔레트** | `Super + /` | `dwm-keybinds` (카테고리별 검색) |

### 2) 제어판, 설정, 시스템 관리
| 기능 | 단축키 | 설명 |
| :--- | :--- | :--- |
| **네이티브 제어판** | `Super + F1` 또는 `Super + c` | `dwm-controlcenter` (Wi-Fi, BT, 오디오, 전원) |
| **시스템 설정 허브** | `Super + Shift + s` | `dwm-settings-hub` (외형, 폰트, 디스플레이 등) |
| **dmenu 도구 모음** | `Super + Shift + h` | `dmenu-hub` |
| **전원 관리 메뉴** | `Alt + x` 또는 `Super + Ctrl + q` | `dmenu-power` (잠금, 로그아웃, 재부팅, 종료) |
| **셸 모드 토글** | `Super + Ctrl + r` | `dwm-shell-mode toggle` (Suckless ↔ Quickshell) |
| **dwm 핫 리로드** | `Super + Shift + r` | 창 상태 유지하며 dwm 바이너리 즉시 재시작 |

### 3) 윈도우 조작 및 타일링
| 기능 | 단축키 | 설명 |
| :--- | :--- | :--- |
| **창 닫기** | `Super + q` | 활성 윈도우 정상 종료 |
| **포커스 이동** | `Super + j` / `Super + k` | 다음/이전 창으로 포커스 이동 |
| **창 위치 교환** | `Super + Shift + j` / `k` | 스택 내에서 활성 창 위치 이동 |
| **마스터 크기 조절** | `Super + h` / `Super + l` | 마스터 영역 너비 확대/축소 |
| **마스터 창으로 승격** | `Super + Return` | 현재 선택된 창을 메인 마스터로 줌 |
| **마스터 창 개수 조절** | `Super + i` / `Super + Shift + i` | 마스터 슬롯 개수 늘리기/줄이기 |
| **전체화면 토글** | `Super + f` | F11 스타일 무테두리 전체화면 |
| **플로팅 모드 토글** | `Super + Shift + Space` | 타일링 ↔ 자유 부유 창 전환 |
| **레이아웃 순환** | `Super + Tab` | Tile ↔ Monocle ↔ Floating ↔ Grid 등 |
| **상태 바 숨김/표시** | `Super + b` | 상단 바 토글 |

### 4) 여백(Vanitygaps) 조절
| 기능 | 단축키 | 설명 |
| :--- | :--- | :--- |
| **갭 On/Off** | `Super + Ctrl + t` | 창 사이 간격 켜기/끄기 |
| **갭 크기 조절** | `Super + Ctrl + i` / `d` | 간격 +5px 확대 / -5px 축소 |
| **기본 갭 복원** | `Super + Alt + Shift + =` | 초기 설정된 표준 간격으로 리셋 |

### 5) 워크스페이스(태그) 및 멀티모니터
| 기능 | 단축키 | 설명 |
| :--- | :--- | :--- |
| **태그 이동** | `Super + 1 ~ 9` | 1번부터 9번 데스크톱으로 즉시 전환 |
| **창 태그 이동** | `Super + Shift + 1 ~ 9` | 활성 창을 지정한 번호의 태그로 전송 |
| **태그 순환 이동** | `Super + Left` / `Right` | 이전/다음 활성 워크스페이스로 이동 |
| **모니터 포커스** | `Super + ,` / `Super + .` | 이전/다음 물리 모니터로 포커스 전환 |
| **모니터 창 전송** | `Super + Shift + ,` / `.` | 현재 창을 다른 물리 모니터로 전송 |

### 6) 미디어 및 스크린샷
| 기능 | 단축키 | 설명 |
| :--- | :--- | :--- |
| **볼륨 조절** | `XF86AudioRaiseVolume` / `Lower` | 음량 5% 증가/감소 (Dunst OSD 팝업) |
| **음소거** | `XF86AudioMute` | 음소거 토글 |
| **밝기 조절** | `XF86MonBrightnessUp` / `Down` | 화면 밝기 10% 증가/감소 |
| **스크린샷 메뉴** | `Print` 또는 `Super + Print` | `dmenu-scrot` 전체/선택/지연 캡처 메뉴 |
| **영역 캡처 후 복사** | `Super + u` 또는 `Shift + Print` | 마우스 드래그 영역 캡처 후 클립보드 저장 |

---

## 6. 제어판 & 설정 허브 (Control Center & Settings)

DWM-Oomaya는 마우스가 필요 없는 키보드 중심의 dmenu 기반 제어 환경을 제공합니다:

- **초경량 제어판 (`dwm-controlcenter`)**:
  - `Super + F1` 단축키로 호출.
  - 네트워크 연결 상태 확인, 블루투스 기기 페어링, 오디오 입출력 선택, 전원 프로필(Balanced/Power-saver) 변경을 단일 메뉴에서 수행합니다.
  - 모든 하위 호출에는 서브세컨드(0.8s) 타임아웃 가드가 내장되어 네트워크 지연 시에도 시스템이 멈추지 않습니다.
- **통합 설정 허브 (`dwm-settings-hub`)**:
  - `Super + Shift + s` 단축키로 호출.
  - 디스플레이 해상도 및 다중 모니터 프로필, 외형(Theme), 폰트(Fonts), 키보드 레이아웃, 기본 브라우저/터미널 설정을 즉시 변경할 수 있습니다.

---

## 7. 상태 바 및 위젯 (Status Bar & Widgets)

화면 상단의 ChadDWM 상태 바는 다음과 같은 마우스 인터랙션을 지원합니다:
- **워크스페이스 번호 (1~9)**: 좌클릭 시 해당 워크스페이스로 전환.
- **날씨 위젯 (Weather)**: 좌클릭 시 상세 날씨 요약 팝업, 우클릭 시 전체 예보 확인.
- **볼륨 위젯 (Volume)**:
  - 휠 업/다운: 음량 조절
  - 좌클릭: `pavucontrol` 오디오 믹서 실행
  - 우클릭: 음소거 토글
- **시계 위젯 (Clock)**: 좌클릭 시 12시간제와 24시간제 표시 모드 즉시 전환.

---

## 8. 셸 모드 전환 (Suckless vs Quickshell)

DWM-Oomaya는 두 가지 셸 모드를 제공하며 언제든 자유롭게 전환할 수 있습니다:

1. **Suckless Mode (기본 권장, 초경량 주권 모드)**:
   - 순수 C 상태 바(`status2d`)와 `dunst` 알림 데몬, `dmenu` 제어판 조합.
   - 메모리 점유율 최소화, 즉각적인 반응 속도, 제로 종속성.
   - 활성화: `dwm-shell-mode suckless`

2. **Quickshell Mode (선택형 컴패니언 오버레이)**:
   - 현대적인 QML 기반 플로팅 바, 제어판 위젯, 미디어 플레이어 오버레이 제공.
   - 활성화: `dwm-shell-mode quickshell` (컴패니언 레포 설치 필요)
   - 빠른 전환 단축키: `Super + Ctrl + r`

컴패니언 레포 설치 방법:
```bash
git clone https://github.com/oomaya/quickshell-oomaya.git ~/.config/quickshell
```

---

## 9. 설정 커스터마이징 (TOML Configuration)

모든 사용자 맞춤 설정은 재컴파일 없이 즉시 수정 가능한 TOML 파일로 관리됩니다:
- **단축키 설정**: `~/.config/dwm-oomaya/config/hotkeys.toml`
  - 원하는 키와 실행할 명령어 셸 스크립트를 추가/수정합니다.
  - 변경 후 `Super + Shift + r`을 누르면 즉각 반영됩니다.
- **테마 및 색상**: `~/.config/dwm-oomaya/config/themes.toml`
  - Tokyo Night, Catppuccin, Gruvbox, Nordic 테마 선택 지원.
- **윈도우 자동 규칙**: `~/.config/dwm-oomaya/config/window-rules.toml`
  - 특정 프로그램이 열릴 때 자동으로 플로팅 모드로 띄우거나 지정된 태그 번호로 전송하도록 규칙을 정의합니다.

---

## 10. 업데이트 및 유지보수 (Update & Maintenance)

DWM-Oomaya는 사용자 설정 파일(`~/.config/dwm-oomaya/`)을 절대 덮어쓰지 않는 무손실 업데이트를 보장합니다:

### 빠른 업데이트
```bash
cd ~/.local/src/dwm-oomaya
git pull origin main
make install-user
```
설치 완료 후 `Super + Shift + r`을 누르면 실행 중인 프로그램이 그대로 유지된 채 새로운 dwm 버전으로 즉시 핫 리로드됩니다.

---

## 11. 시스템 진단 및 트러블슈팅 (Troubleshooting)

### 시스템 상태 원클릭 진단
데스크톱 실행 중 문제가 발생하면 진단 도구를 실행하십시오:
```bash
dwm-diagnostics
```
X11 서버 상태, 컴포지터(`picom`), 오디오 서버(`pipewire`/`pulseaudio`), 알림 데몬(`dunst`)의 동작 상태를 일괄 점검하여 리포트를 출력합니다.

### 자주 발생하는 문제 해결
1. **창 테두리나 그림자가 깨질 때**:
   - 컴포지터 재시작: `killall picom && picom -b &`
2. **사운드가 안 들리거나 볼륨 키가 안 먹힐 때**:
   - `dwm-volume status` 실행 후 활성 싱크 확인.
3. **VM 환경(VMware/VirtualBox) 3D 가속 문제**:
   - 가상머신 환경에서 검은 화면이 발생할 경우 `dwm-display-setup --vm-safe`를 실행합니다.
