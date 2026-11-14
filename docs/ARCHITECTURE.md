# RealGuitarGame — 시스템 설계 문서 (Architecture)

## 1. 프로젝트 비전
실사 수준의 3D 공간(언리얼 엔진 5 + Lumen/Nanite + Quixel Megascans)을 1인칭 시점으로 자유롭게 거닐다가 벤치나 의자에 앉으면 카메라가 기타 연주 감상 구도로 부드럽게 전환되고, 사용자가 넣은 타브 악보·MIDI·코드 진행에 맞춰 **3D 캐릭터의 왼손 운지·오른손 스트로크/핑거스타일·기타 줄 진동·실제 어쿠스틱/일렉 기타 사운드가 실시간 동기화**되는 PC 감상형 오토 기타 시뮬레이터입니다.

---

## 2. 악보 포맷 선정: 스트로크 & 핑거스타일 통합 전략

기타 연주는 피아노와 달리 **같은 음(예: E4)도 어느 줄·몇 번 프렛에서 잡느냐에 따라 손 위치와 음색이 완전히 달라집니다.** 따라서 게임 내 인식 정확도를 극대화하기 위해 **입력 계층**과 **엔진 내부 표준 계층**을 분리했습니다.

### A. 외부 입력 포맷 우선순위
1. **1순위 — 타브 악보 (`MusicXML .musicxml/.mxl`, `Guitar Pro 7/8 .gp/.gpif`)**
   - **강점:** 줄 번호(`1~6`), 프렛(`0~24`), 주법(`Hammer-On`, `Pull-Off`, `Slide`, `Palm Mute`, `Let Ring`, `Harmonic`), 브러시 방향(`Down`/`Up` 스트로크)이 원본에 그대로 들어 있습니다.
   - 핑거스타일 곡과 복잡한 아르페지오를 가장 정확하게 재현합니다.
2. **2순위 — 코드 진행 + 주법 DSL (`.rgchart`)**
   - **강점:** `"C, G/B, Am7, Fmaj7"` 같은 코드 진행과 BPM, 그리고 **스트로크 패턴(`16beat_ballad`, `8beat_folk`, `pop_rock`)** 또는 **핑거스타일 패턴(`pima_ballad`, `travis_pick`, `romanza_triplet`)**만 지정하면 곡 전체의 연주 데이터를 즉시 생성합니다.
   - 섹션별로 `Verse = 핑거스타일` -> `Chorus = 16비트 스트로크` 전환이 자유롭습니다.
3. **3순위 — 일반 MIDI (`.mid`)**
   - **한계 보완:** 일반 MIDI는 줄/프렛 정보가 없으므로 `engine/fingering_solver.py`의 **Dynamic Programming(Viterbi) 운지 최적화 솔버**를 거쳐 6현 24프렛 위에서 손 이동 거리가 가장 적고 자연스러운 포지션으로 자동 변환합니다.

### B. 엔진 내부 단일 표준 규격 (`GuitarPerformance.json`)
어떤 입력 포맷을 넣든 `engine/compiler.py`를 거치면 동일한 **`schemas/guitar_performance.schema.json` (v1.0)** 규격으로 컴파일됩니다.

```
[MusicXML / .gp / .mid / .rgchart]
               │
               ▼  (engine/compiler.py)
   [GuitarPerformance.json]  ──▶  브라우저 2D/음원 미리보기 (preview/index.html)
               │
               ▼  (UGuitarScoreSubsystem)
   ┌───────────┴───────────┐
   ▼                       ▼
[오디오 브릿지]         [3D 손/기타 제어]
UGuitarAudioMetaSound   UGuitarFretboardComponent
(MetaSounds/Harmonix)   (6현×24프렛 물리 좌표 -> Control Rig IK)
```

---

## 3. 언리얼 엔진 5 (UE5) 런타임 모듈 구조 (`ue5/RealGuitarGame`)

| C++ 클래스 | 역할 |
|---|---|
| `GuitarPerformanceTypes.h` | JSON 스키마와 1:1 대응하는 `FGuitarEvent`, `FGuitarChordSnapshot`, `FGuitarHandIKTargets` 구조체 정의 |
| `UGuitarScoreSubsystem` | `UTickableWorldSubsystem`. `GuitarPerformance.json`을 로드하고 재생 시계에 맞춰 `OnGuitarEventTriggered` / `OnChordChanged` 이벤트를 브로드캐스트 |
| `UGuitarFretboardComponent` | 너트(Nut)에서 브릿지까지의 현 길이 공식 $d(n) = L \cdot (1 - 2^{-n/12})$ 을 기반으로 1~6번 줄·0~24프렛의 정확한 3D 월드 좌표와 오른손 사운드홀 타겟을 계산해 보간(Interpolation) |
| `UGuitarAudioMetaSoundBridge` | `UGuitarScoreSubsystem` 이벤트를 받아 `UAudioComponent`의 MetaSound 파라미터(`MidiNote`, `StringIndex`, `Fret`, `Velocity`, `Articulation`, `IsDownStrum`, `TriggerPluck`)로 전달 |
| `AInteractableSeatActor` | 벤치/의자 액터. 플레이어 상호작용 시 `PerformanceCamera`로 부드럽게 블렌드 전환(`SetViewTargetWithBlend`)하고 악보 자동 재생 |
| `AAcousticChillCharacter` | 1인칭 이동(`EnhancedInput`), 시선/오버랩으로 의자 감지, 착석/기립 토글(`TryToggleSeat`) 및 재생 일시정지 제어 |

---

## 4. 실사 비주얼 & 사운드 통합 전략
* **공간(배경):** `Quixel Megascans` (Fab 무료 라이브러리) + UE5 Lumen 실시간 전역 조명 + Nanite 메쉬
* **연주자 손/캐릭터:** `MetaHuman` 무료 실사 캐릭터의 상체·양손 스켈레탈 메쉬 + `Control Rig`로 `UGuitarFretboardComponent::GetSmoothedHandIKTargets()` 좌표를 매 프레임 연결
* **기타 음원:** UE5 `Harmonix` 플러그인의 `FusionPatch` + `FusionSampler`에 멀티샘플 기타 음원(WAV)을 매핑하거나, MetaSounds DSP로 스트링 물리 모델링/앰프 이펙트 체인 구성
