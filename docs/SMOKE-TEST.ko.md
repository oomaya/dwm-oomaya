# DWM-Oomaya 라이브 스모크 테스트 점검표 (Smoke Test Checklist)

[English](SMOKE-TEST.md) | [한국어](SMOKE-TEST.ko.md)

호스트 데스크톱 환경(X11 세션)에서 `dwm-oomaya`의 주요 기능이 정상 작동하는지 점검하는 10단계 시나리오입니다.

---

## 1. 세션 진입 및 상태 바 (Status Bar)
- [ ] **상태 바 표시 확인**: 화면 상단에 ChadDWM/Status2d 기반의 상단 바가 정상 표시되는지 확인합니다.
- [ ] **태그 번호 (1~9)**: 현재 워크스페이스 번호와 활성 윈도우 인디케이터가 정상 렌더링되는지 확인합니다.
- [ ] **시스템 모니터 위젯**: 날씨(Weather), CPU/RAM 점유율, 오디오 볼륨, 시계(Clock)가 상단 바 우측에 표시되는지 확인합니다.
- [ ] **상태 바 마우스 인터랙션**:
  - 시계 좌클릭: 12/24시간 형식 전환
  - 볼륨 휠 스크롤: 음량 조절 (`dwm-volume up/down`)
  - 볼륨 우클릭: 음소거 토글 (`dwm-volume mute`)

---

## 2. 터미널 및 기본 앱 실행
- [ ] `Super + Shift + Return`: 주 터미널(Ghostty 또는 Kitty/Alacritty)이 즉시 기동되는지 확인합니다.
- [ ] `Super + s`: 플로팅 스크래치패드(Scratchpad) 터미널이 중앙에 토글되는지 확인합니다.
- [ ] `Super + e`: 기본 파일 관리자(Thunar 또는 xdg-open)가 열리는지 확인합니다.

---

## 3. 프로그램 검색 & 런처 (dmenu)
- [ ] `Super + d`: dmenu 기반 앱 런처가 화면 상단에 열리고, 검색어 입력 시 즉시 필터링되는지 확인합니다.
- [ ] `Alt + p`: 바이너리 실행 런처(`dmenu-run`)가 정상 실행되는지 확인합니다.
- [ ] `Alt + Tab`: 현재 실행 중인 윈도우 목록이 dmenu로 열리고 창 전환이 가능한지 확인합니다.

---

## 4. 제어판 & 설정 허브 (Control Center & Settings)
- [ ] `Super + F1` 또는 `Super + c`: 초경량 네이티브 제어판(`dwm-controlcenter` / `dwm-flow`)이 호출되는지 확인합니다.
  - Wi-Fi/네트워크, 블루투스, 볼륨, 다크모드/테마 전환 메뉴 항목들이 정상 작동하는지 확인합니다.
- [ ] `Super + Shift + s`: 전체 시스템 설정 허브(`dwm-settings-hub`)가 실행되고 디스플레이, 외형, 폰트 등의 하위 메뉴가 선택 가능한지 확인합니다.

---

## 5. 단축키 안내 팝업 (Keybinding Palette)
- [ ] `Super + /`: 화면 중앙에 카테고리별 전체 단축키 치트시트(`dwm-keybinds`)가 팝업되는지 확인합니다.
  - 검색창에서 키 검색 시 즉시 필터링되는지 확인합니다.

---

## 6. 윈도우 타일링 및 레이아웃 조작
- [ ] `Super + j` / `Super + k`: 창 간 포커스 이동 (Focus Stack Down/Up).
- [ ] `Super + Shift + j` / `Super + Shift + k`: 창 위치 교환 (Move Stack).
- [ ] `Super + h` / `Super + l`: 마스터 영역 너비 조절 (Master Fact).
- [ ] `Super + Tab`: 타일링 레이아웃 순환 (Master-Stack -> Monocle -> Grid 등).
- [ ] `Super + f`: 전체화면 모드 토글 (Fullscreen toggle).
- [ ] `Super + Shift + Space`: 플로팅(Floating) 창 모드 토글.
- [ ] `Super + q`: 현재 포커스된 창 닫기 (Kill Client).

---

## 7. 갭(Gaps) 및 여백 조작
- [ ] `Super + Ctrl + t`: 창 간 여백(Vanitygaps) 켜기/끄기 토글.
- [ ] `Super + Ctrl + i` / `Super + Ctrl + d`: 갭 크기 증가/감소.
- [ ] `Super + Alt + Shift + =`: 기본 갭 크기로 복구.

---

## 8. 멀티태그 & 멀티모니터 워크스페이스
- [ ] `Super + 1~9`: 해당 번호의 태그(가상 데스크톱)로 이동.
- [ ] `Super + Shift + 1~9`: 포커스된 창을 해당 태그로 이동.
- [ ] `Super + Left` / `Super + Right`: 이전/다음 활성 태그로 순환 이동 (Shiftview).
- [ ] `Super + ,` / `Super + .`: 다중 모니터 환경에서 다른 모니터로 포커스 전환.
- [ ] `Super + Shift + ,` / `Super + Shift + .`: 현재 창을 다른 모니터로 이동.

---

## 9. 하드웨어 키 & 미디어/스크린샷
- [ ] 볼륨 키 (`XF86AudioRaiseVolume`, `XF86AudioLowerVolume`, `XF86AudioMute`): 볼륨 조절 및 Dunst OSD 알림 확인.
- [ ] 밝기 키 (`XF86MonBrightnessUp`, `XF86MonBrightnessDown`): 모니터/노트북 화면 밝기 조절 확인.
- [ ] `Print`: 스크린샷 메뉴(`dmenu-scrot`) 호출.
- [ ] `Super + u` 또는 `Shift + Print`: 영역 선택 스크린샷 후 클립보드 복사.

---

## 10. 셸 모드 전환 & 전원 메뉴
- [ ] `Super + Ctrl + r`: 셸 모드 토글 (`dwm-shell-mode toggle`).
  - Quickshell 컴패니언이 미설치된 경우 알림과 함께 순수 Suckless 모드 유지 확인.
- [ ] `Alt + x` 또는 `Super + Ctrl + q`: 전원 관리 메뉴(`dmenu-power`) 호출 (잠금, 로그아웃, 재부팅, 종료).
- [ ] `Super + Shift + r`: dwm 무중단 핫 리로드 (재부팅 없이 창 상태 유지하며 dwm 재시작).
