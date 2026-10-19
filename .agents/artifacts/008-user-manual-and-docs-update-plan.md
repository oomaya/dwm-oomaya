# Implementation Plan: Comprehensive User Manual, Verification & Docs Modernization

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish an end-to-end user manual (`USER-GUIDE.md`), modernize all outdated GitHub docs (`README.md`, `SPEC.md`, `ARCHITECTURE.md`), provide a structured smoke-test verification checklist for user host testing, and prepare the `v0.8.0` sovereign release.

**Architecture:** Pure C sovereign dwm ecosystem with optional `quickshell-oomaya` companion. Documentation is strictly synchronized against live code (`config.def.h`, `config/hotkeys.toml`, `scripts/dwm-utils.sh`) and verified using existing validators (`validate-hotkeys`, `check-packaging`, `check-core`).

**Tech Stack:** POSIX Shell, Markdown, C99/C11, GitHub Flavored Markdown, TOML.

---

## Quota & Token Budget Analysis (Go/No-Go Decision Gate)

Inspected via `agy-usage --json` at 2026-10-19 19:29 KST:

| Provider / Model Window | Remaining Quota | Status | Triage Safe Floor | Decision |
| :--- | :---: | :---: | :---: | :---: |
| **Gemini Weekly Cap** | **55.5%** | 🟢 Healthy | > 15.0% | **GO (Green)** |
| **Gemini 5-Hour Session** | **93.0%** | 🟢 Abundant | > 20.0% | **GO (Green)** |
| **Claude & GPT 3P Models (Weekly)** | **100.0%** | 🟢 Pristine | > 15.0% | **GO (Green)** |
| **Claude & GPT 3P Models (5-Hour)** | **100.0%** | 🟢 Pristine | > 20.0% | **GO (Green)** |

> [!NOTE]
> All token allowances are in prime condition. Documentation generation and structural consistency checks will be executed directly with Gemini without touching Claude quota.

---

## Gap Analysis: 추가로 고려해야 할 핵심 작업 (놓치기 쉬운 포인트)

사용자께서 제안하신 3가지 핵심 축(기능 확인, 매뉴얼 작성, GitHub 문서 업데이트)은 프로젝트 마무리와 공개 릴리스를 위해 정확한 방향입니다. 여기에 더해 **누락 시 불일치나 배포 장애를 유발할 수 있는 3가지 필수 연계 작업**을 보완 항목으로 추가합니다:

1. **사용자 기능 점검용 '대화형 스모크 테스트 체크리스트 (Smoke Test Matrix)'**:
   - 사용자가 호스트 환경에서 무엇을 어떤 키로 테스트해야 하는지 일목요연한 10단계 시나리오(터미널 실행, 태그 이동, 제어판/설정허브 호출, 볼륨/밝기 OSD, 다중 모니터 워크스페이스, 셸 모드 전환)를 제공하여 검증 누락 방지.
2. **단축키 원천 데이터 삼자 동기화 검증 (`Triple-Sync Verification`)**:
   - 매뉴얼에 기재될 단축키가 실제 C 소스코드([`config.def.h`](file:///home/rand/.local/src/dwm-oomaya/config.def.h)), 런타임 TOML([`config/hotkeys.toml`](file:///home/rand/.local/src/dwm-oomaya/config/hotkeys.toml)), 그리고 화면 안내 팝업([`scripts/dwm-keybinds`](file:///home/rand/.local/src/dwm-oomaya/scripts/dwm-keybinds))과 100% 일치하는지 `validate-hotkeys`로 사전 검증.
3. **버전 `v0.8.0` 공식 Git 태그 및 릴리스 아카이브 생성 (`Release Engineering`)**:
   - `config.mk`, RPM spec, PKGBUILD가 모두 `0.8.0`으로 맞춰져 있으나 Git 태그 `v0.8.0`이 아직 부재함.
   - 패키징의 `Source0` URL(`archive/v0.8.0/...`)이 유효하게 작동하도록 `make release` 검증 및 GitHub Release / Git 태그 생성 준비.
4. **컴패니언 저장소([`oomaya/quickshell-oomaya`](https://github.com/oomaya/quickshell-oomaya)) 상호 링크(Cross-link) 점검**:
   - 사용자가 Quickshell 기능을 원할 때 원클릭으로 찾아갈 수 있도록 두 레포의 README 상호 참조 정리.

---

## Task Decomposition

### Task 1: Single-Source Hotkey Audit & Smoke Test Checklist
- **Goal**: Validate shortcuts and generate a structured user verification checklist.
- **Files**:
  - Check: `config.def.h`, `config/hotkeys.toml`
  - Run: `scripts/validate-hotkeys`
  - Create: `docs/SMOKE-TEST.md` (호스트 라이브 검증용 단계별 점검표)
- **Steps**:
  - [ ] 1. Run `scripts/validate-hotkeys` to ensure zero drift between C header and TOML.
  - [ ] 2. Create `docs/SMOKE-TEST.md` listing the 10 core verification scenarios with exact key combinations and expected behaviors.

### Task 2: Comprehensive User Manual (`USER-GUIDE.md`)
- **Goal**: Provide an end-to-end, distribution-agnostic user manual.
- **Files**:
  - Create: `docs/USER-GUIDE.md`
- **Contents**:
  - [ ] 1. **시스템 아키텍처 개요**: Pure C 주권적 코어 vs 선택형 Quickshell 컴패니언.
  - [ ] 2. **설치 가이드**:
    - 배포판별 원클릭 설치 (`./install.sh --desktop`, Fedora / Arch / Debian / Ubuntu / Mint / Peppermint).
    - 소스 빌드 및 수동 설치 (`make && sudo make install-system` / `make install-user`).
    - 바이너리 패키지 설치 (RPM / PKGBUILD).
  - [ ] 3. **업데이트 & 유지보수 가이드**:
    - `agy-update`를 통한 무중단 업데이트 워크플로우.
    - 소스 재빌드 시 유저 설정 보존 메커니즘.
  - [ ] 4. **단축키 및 조작법 (Full Shortcuts Matrix)**:
    - 윈도우 관리 (이동, 크기 조절, 타일링 레이아웃 순환, 갭 조절).
    - 멀티태그 & 멀티모니터 워크스페이스 조작.
    - 런처 & 제어판: `Super+Space` (dmenu), `Super+C` (제어판), `Super+I` (설정 허브).
    - 미디어 & 하드웨어: 볼륨, 밝기, 스크린샷(`Print`, `Super+Shift+S`).
  - [ ] 5. **설정 커스터마이징 (TOML & C)**:
    - `~/.config/dwm-oomaya/config/` (hotkeys, themes, window-rules).
    - 색상 테마 및 폰트 변경 (`dwm-settings-appearance`).
  - [ ] 6. **셸 모드 전환 (Suckless vs Quickshell)**:
    - `dwm-shell-mode suckless` (기본 초경량 모드).
    - `dwm-shell-mode quickshell` (동적 오버레이 모드).
  - [ ] 7. **트러블슈팅**:
    - 디스플레이 세션 잠금 해제, 시스템 진단(`dwm-diagnostics`), 로그 위치.

### Task 3: Outdated GitHub Docs Modernization
- **Goal**: Update `README.md`, `SPEC.md`, `ARCHITECTURE.md`, `ROADMAP.md` to reflect pure C sovereign architecture.
- **Files**:
  - Modify: `README.md`
  - Modify: `SPEC.md`
  - Modify: `docs/ARCHITECTURE.md`
  - Modify: `ROADMAP.md`
- **Steps**:
  - [ ] 1. `README.md`:
    - 배지 및 헤더 업데이트: Pure C ChadDWM core + Optional Quickshell Companion.
    - 배포판 지원 명시: Fedora, Arch Linux/CachyOS, Debian, Ubuntu, Linux Mint, Peppermint OS.
    - 패키지 설치 섹션(RPM, PKGBUILD) 및 최신 `USER-GUIDE.md` 링크 추가.
    - 레거시 ChrisTitusTech 오타 및 내장 Quickshell 구버전 설명 교체.
  - [ ] 2. `SPEC.md`: Phase 2 및 Phase 3 완료 상태 및 저장소 분리 반영.
  - [ ] 3. `docs/ARCHITECTURE.md`: 패키징 및 데스크톱 모듈 구조도 업데이트.
  - [ ] 4. `ROADMAP.md`: Packet #006, #007 완료 기록 및 차기 계획 업데이트.

### Task 4: Release v0.8.0 Engineering & Final CI Gate
- **Goal**: Validate release archive generation, verify all test suites, and prepare tag `v0.8.0`.
- **Files**:
  - Run: `make release`
  - Run: `make check-core`
- **Steps**:
  - [ ] 1. Run `make release` to generate and validate `release/dwm-oomaya-0.8.0-oomaya.tar.gz`.
  - [ ] 2. Run `make check-core` to guarantee 100% green test passes locally.
  - [ ] 3. Commit docs updates to a new PR or `main`.
  - [ ] 4. Draft GitHub Release v0.8.0 notes with links to the new `USER-GUIDE.md` and RPM/PKGBUILD.
