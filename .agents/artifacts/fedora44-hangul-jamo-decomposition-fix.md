# Fedora 44 한글 자소 분리(풀림) 문제 진단 및 영구 해결 가이드
(Fedora 44 Hangul Jamo Decomposition Root Cause Diagnosis & Resolution Guide)

---

## 1. 문제 정의 (Problem Definition)

### 1.1 현상 (Symptoms)
- Antigravity IDE, VS Code, Chromium/Electron 기반 웹 환경에서 한글 텍스트 실시간 스트리밍(Token Streaming) 또는 타이핑 시, 완성형 음절(`가상 서버`)이 낱자 자음/모음(`가ㅅ ㅏ ㅇ ㅅ ㅓ ㅂ ㅓ`)으로 쪼개져 렌더링되는 현상 발생.
- 내부 저장 데이터(JSONL, 파일 원본)는 정상 완성형 유니코드(`U+C0C1`, `U+C11C`)로 저장되어 있으나, 화면 렌더링 레이어에서만 자소 분리 형태로 고착됨.

### 1.2 배포판 간 특이점 (Cross-Distro Behavior)
- **발생 환경**: **Fedora 44 (최신 브랜치)**에서만 유독 발생.
- **미발생 환경**: Debian (Peppermint), Ubuntu (Linux Mint), Arch Linux (CachyOS) 등 타 배포판에서는 동일한 조건에서 전혀 발생하지 않고 정상 렌더링됨.

---

## 2. 심층 근본 원인 분석 (Deep Root Cause Analysis)

### 2.1 가변 폰트(Variable Font, `VF.ttc`) vs 정적 폰트(Static Font, `OTF/TTF`)
- **Debian / Ubuntu / Arch 계열**:
  - 전통적인 **정적 단일 글꼴 패키지**(`NotoSansCJK-Regular.otf` 또는 나눔고딕, `fonts-noto-cjk`)를 사용합니다.
  - 고정된 웨이트(Weight)를 가진 단일 글꼴 파일은 OpenType GSUB(Glyph Substitution) 테이블이 단순 명확하여 글리프 조합 시 모호성이 없습니다.
- **Fedora 44 패키징 정책**:
  - Fedora는 시스템 용량 최적화를 위해 CJK 폰트를 다국어 통합 가변 글꼴 컬렉션인 **`google-noto-sans-cjk-vf-fonts` (`NotoSansCJK-VF.ttc`)**로 전격 교체했습니다.
  - 조사 결과 Fedora 44 기본 설치 상태에서는 정적 한글 글꼴이 전무하고, 오직 `*-vf-fonts` 가변 폰트만 탑재되어 있었습니다.

### 2.2 Chromium / Electron 텍스트 셰이퍼(HarfBuzz)의 업스트림 결함
- Antigravity IDE와 VS Code의 기반인 Chromium 렌더러는 폰트 셰이핑을 위해 **HarfBuzz + FreeType**을 사용합니다.
- 복합 가변 글꼴 컬렉션(`.ttc` 파일 내부에 한·중·일 여러 서브폰트와 `wght` 가변 축이 포함된 구조) 환경에서 실시간 텍스트 조각(Token Chunks)이 빠르게 인입될 때:
  1. Chromium의 텍스트 런(Text Run) 캐시가 가변 서브폰트의 완성형 글리프 ID를 즉시 매핑하지 못하는 레이스 컨디션 발생.
  2. 글리프 매핑에 실패한 셰이퍼가 유니코드 분해(Decomposition) 테이블을 참조하여 한글을 낱자 자모(`ㅅ`, `ㅏ`, `ㅇ`)로 폴백하여 개별 렌더링.
  3. 한 번 DOM에 분리된 자모 형태로 그려지면, 이후 완성형 데이터가 들어와도 화면 캐시가 무효화되기 전까지 분리된 상태가 유지됨.

### 2.3 초최신 HarfBuzz / Pango 엔진과의 상호작용
- Fedora 44는 최신 업스트림인 `harfbuzz-14.1.0-2.fc44` 및 `pango-1.57.1-1.fc44`를 탑재하고 있어, 안정화 버전(HarfBuzz 8.x~9.x)을 사용하는 Debian/Ubuntu/Arch와 달리 가변 TTC 서브폰트 인덱싱 변경점이 두드러지게 노출되었습니다.

---

## 3. 영구 해결 방안 (Permanent Resolution)

가변 폰트 컬렉션(`VF.ttc`) 대신, 타 배포판들과 동일하게 안정적인 **정적(Static) 한글 폰트(`Noto Sans KR`)**를 설치하여 fontconfig 우선순위를 전환합니다.

### 3.1 해결 절차 (Execution Steps)

```bash
# 1. 정적(Static) 한글 OTF 글꼴 패키지 설치
sudo dnf install -y google-noto-sans-kr-fonts

# 2. 폰트 캐시 즉각 재구성
fc-cache -fv
```

### 3.2 작동 원리
- `google-noto-sans-kr-fonts`는 한글 단독 정적 글꼴(`NotoSansKR-Regular.otf` 등 7종)을 제공합니다.
- 설치 즉시 fontconfig의 한글(`:lang=ko`) 매칭 룰이 복잡한 다국어 가변 컬렉션(`NotoSansCJK-VF.ttc`) 대신 안정적인 단일 정적 글꼴을 우선 선택합니다.
- 결과적으로 Chromium/Electron의 HarfBuzz 셰이퍼가 지연이나 분해 없이 100% 완성형 음절로 직결 렌더링합니다.

---

## 4. 검증 및 확인 (Verification)

### 4.1 Fontconfig 매칭 확인
터미널에서 아래 명령을 실행하여 정적 폰트가 1순위로 조회되는지 확인합니다:

```bash
fc-match "sans-serif:lang=ko"
```
*출력 예시:*
```text
NotoSansKR-Regular.otf: "Noto Sans KR" "Regular"
```

### 4.2 실시간 렌더링 검증
- Antigravity IDE 또는 VS Code에서 한글 실시간 스트리밍 답변을 수신할 때 자소 분리 없이 미려하게 조합되는지 확인합니다.

---

## 5. VM 롤아웃 및 프로비저닝 자동화 반영 (Fleet Integration)

향후 다른 Fedora 기반 VM 또는 신규 컨테이너 배포 시 한글 환경 프로비저닝 스크립트(`scripts/check-deps.sh` 또는 킥스타트)에 본 패키지를 기본 포함합니다:

```bash
# Fedora Desktop / VM 기본 한글 폰트 보정
if [ -f /etc/fedora-release ]; then
    rpm -q google-noto-sans-kr-fonts >/dev/null 2>&1 || sudo dnf install -y google-noto-sans-kr-fonts
fi
```
