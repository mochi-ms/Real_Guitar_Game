# RealGuitarGame — 시스템 설계 문서 (Architecture)

## 1. 프로젝트 비전
실사 수준의 3D 공간(언리얼 엔진 5 + Lumen/Nanite + Quixel Megascans)을 1인칭 시점으로 자유롭게 거닐다가 벤치나 의자에 앉으면 카메라가 기타 연주 감상 구도로 부드럽게 전환되고, 사용자가 넣은 타브 악보·MIDI·코드 진행에 맞춰 **3D 캐릭터의 왼손 운지·오른손 스트로크/핑거스타일·기타 줄 진동·실제 어쿠스틱/일렉 기타 사운드가 실시간 동기화**되는 PC 감상형 오토 기타 시뮬레이터입니다.

---

## 2. 악보 포맷 선정: 스트로크 & 핑거스타일 통합 전략

기타 연주는 피아노와 달리 **같은 음(예: E4)도 어느 줄·몇 번 프렛에서 잡느냐에 따라 손 위치와 음색이 완전히 달라집니다.** 따라서 게임 내 인식 정확도를 극대화하기 위해 **입력 계층**과 **엔진 내부 표준 계층**을 분리했습니다.

### A. 외부 입력 포맷 우선순위
1. **1순위 — 타브 악보 (`Guitar Pro .gp3/.gp4/.gp5/.gp/.gpif`, `MusicXML .musicxml/.mxl`)**
   - **강점:** 줄 번호(`1~6`), 프렛(`0~24`), 주법(`Hammer-On`, `Pull-Off`, `Slide`, `Palm Mute`, `Let Ring`, `Harmonic`), 브러시 방향(`Down`/`Up` 스트로크)이 원본에 그대로 들어 있습니다.
   - 핑거스타일 곡과 복잡한 아르페지오를 가장 정확하게 재현합니다.
2. **2순위 — 코드 진행 + 주법 DSL (`.rgchart`)**
   - **강점:** `"C, G/B, Am7, Fmaj7"` 같은 코드 진행과 BPM, 그리고 **스트로크 패턴(`16beat_ballad`, `8beat_folk`, `pop_rock`)** 또는 **핑거스타일 패턴(`pima_ballad`, `travis_pick`, `romanza_triplet`)**만 지정하면 곡 전체의 연주 데이터를 즉시 생성합니다.
   - 섹션별로 `Verse = 핑거스타일` -> `Chorus = 16비트 스트로크` 전환이 자유롭습니다.
3. **3순위 — 일반 MIDI (`.mid`)**
   - **한계 보완:** 일반 MIDI는 줄/프렛 정보가 없으므로 `engine/fingering_solver.py`의 **Dynamic Programming(Viterbi) 운지 최적화 솔버**를 거쳐 6현 24프렛 위에서 손 이동 거리가 가장 적고 자연스러운 포지션으로 자동 변환합니다.

### B. 엔진 내부 단일 표준 규격 (`GuitarPerformance.json`)
어떤 입력 포맷을 넣든 `engine/compiler.py` (+ `engine/humanizer.py`)를 거치면 동일한 **`schemas/guitar_performance.schema.json` (v1.0)** 규격으로 컴파일됩니다.

```
[GP5 / GP / MusicXML / .mid / .rgchart]
               │
               ▼  (engine/compiler.py + humanizer.py)
   [GuitarPerformance.json]  ──▶  브라우저 스튜디오 (preview/index.html)
               │
               ▼  (UGuitarScoreSubsystem)
   ┌───────────┼───────────────────────┐
   ▼           ▼                       ▼
[오디오]     [3D 손/기타 제어]        [HUD & 카메라]
MetaSound   Fretboard + StringVisual  PerformanceHUD + SongLibrary
```

---

## 3. 언리얼 엔진 5 (UE5) 런타임 모듈 구조 (`ue5/RealGuitarGame`)

| C++ 클래스 | 역할 |
|---|---|
| `GuitarPerformanceTypes.h` | JSON 스키마와 1:1 대응하는 `FGuitarEvent`, `FGuitarChordSnapshot`, `FGuitarHandIKTargets` 구조체 정의 |
| `UGuitarScoreSubsystem` | `UTickableWorldSubsystem`. `GuitarPerformance.json`을 로드하고 재생 시계에 맞춰 `OnGuitarEventTriggered` / `OnChordChanged` 이벤트를 브로드캐스트 |
| `UGuitarSongLibrarySubsystem` | `UGameInstanceSubsystem`. `Content/Scores/`·`Saved/UserScores/` 곡 목록 스캔 및 인게임 실시간 코드+스트로크/핑거스타일 생성기 |
| `UGuitarFretboardComponent` | 너트(Nut)에서 브릿지까지의 현 길이 공식 $d(n) = L \cdot (1 - 2^{-n/12})$ 을 기반으로 1~6번 줄·0~24프렛의 정확한 3D 월드 좌표와 오른손 사운드홀 타겟을 계산해 보간(Interpolation) |
| `UGuitarStringVisualComponent` | 줄별 진동 진폭(`StringVibration_1..6`)과 활성 프렛(`ActiveFret_1..6`)을 다이내믹 머티리얼 파라미터로 전달 |
| `UGuitarAudioMetaSoundBridge` | `UGuitarScoreSubsystem` 이벤트를 받아 `UAudioComponent`의 MetaSound 파라미터(`MidiNote`, `StringIndex`, `Fret`, `Velocity`, `Articulation`, `IsDownStrum`, `TriggerPluck`)로 전달 |
| `AGuitarPerformerActor` | 기타 메쉬, 프렛보드 IK, 줄 진동, 오디오 브릿지, 3종 시네마틱 카메라 뷰(`전체 / 왼손 프렛 / 오른손 주법`) 통합 액터 |
| `AInteractableSeatActor` | 벤치/의자 액터. 플레이어 상호작용 시 `AGuitarPerformerActor` 또는 `PerformanceCamera`로 부드럽게 블렌드 전환(`SetViewTargetWithBlend`)하고 악보 자동 재생 |
| `AAcousticChillCharacter` | 1인칭 이동(`EnhancedInput`), 시선/오버랩으로 의자 감지, 착석/기립 토글(`TryToggleSeat`), 카메라 뷰 전환(`C`), 다음 곡(`N`), 재생 토글(`Space`) |
| `UGuitarPerformanceHUDWidget` | 미니멀리스트 UMG HUD 베이스 클래스 (상호작용 프롬프트, 현재/다음 코드, 속도 슬라이더, 카메라 전환, 라이브 코드 패턴 스튜디오) |
| `ARealGuitarGameMode` | `AAcousticChillCharacter`를 기본 폰으로 지정하는 게임 모드 |

---

## 4. 실사 비주얼 & 사운드 통합 전략
* **공간(배경):** `Quixel Megascans` (Fab 무료 라이브러리) + UE5 Lumen 실시간 전역 조명 + Nanite 메쉬
* **기타 3D 모델 & 스케일 기준:** `tools/generate_guitar_glb.py`로 생성한 `Content/Models/AcousticGuitar_Reference.glb` (너트 기준 64.8cm 정밀 매칭) + UE5 설치 후 Fab 무료 실사 기타 모델 교체/보강 가능
* **연주자 손/캐릭터:** `MetaHuman` 무료 실사 캐릭터의 상체·양손 스켈레탈 메쉬 + `Control Rig`로 `UGuitarFretboardComponent::GetSmoothedHandIKTargets()` 좌표를 매 프레임 연결
* **기타 음원:** `engine/soundbank.py`가 생성하는 44.1kHz 어쿠스틱 멀티샘플(`Content/Audio/Samples/*.wav` + `multisample_map.json`)을 UE5 `Harmonix FusionPatch` / `MetaSounds`에 연결해 즉시 고품질 기타 사운드 구현
* **에디터 자동 세팅 스크립트:** `ue5/RealGuitarGame/Tools/ue5_editor_bootstrap.py`를 UE5 에디터에서 1회 실행하면 Enhanced Input, GLB, WAV 샘플 및 기본 액터를 자동 배치합니다.
