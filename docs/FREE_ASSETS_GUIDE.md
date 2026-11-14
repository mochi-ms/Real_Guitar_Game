# 무료 고퀄리티 UE 5.8.4 에셋 가이드 (Free Asset Blueprint)

스팀 출시 목적이 아닌 **개인용 실사 1인칭 기타 감상 게임** 기준으로, 라이선스 걱정 없이 고퀄리티를 낼 수 있는 검증된 무료 에셋 구성표입니다.  
Unreal Engine **5.8.4** 설치가 완료되면 아래 순서대로 프로젝트(`ue5/RealGuitarGame`)에 적용할 수 있습니다.

---

## 1. 배경 / 환경 (Photorealistic Environment)
- **Quixel Megascans (`Fab.com` / UE5 내장 Fab 플러그인)**
  - **비용/라이선스:** 무료 (Epic Games가 `Fab.com` 을 통해 Quixel Megascans 약 23,000여 개 스캔 에셋을 무료 제공)
  - **UE 5.8 호환:** Nanite 고폴리곤 지오메트리 및 Lumen 전역 조명 기본 지원
  - **추천 조합 키워드 (Fab 검색어):**
    - 숲/호숫가 산책로 테마: `Megascans Nordic Forest`, `Mossy Rocks`, `Wooden Dock`, `Wild Grass`
    - 아늑한 테라스/방 테마: `Megascans Wooden Flooring`, `Brick Wall`, `Warm Carpet`, `Indoor Plants`
- **UE5 기본 포함 에셋:** `Engine/Content` + `Volumetric Clouds`, `Sky Atmosphere`, `Exponential Height Fog` (본 프로젝트 `ASanctuaryAtmosphereActor`가 이미 낮/노을/밤/비 프리셋으로 자동 제어)

---

## 2. 벤치 / 의자 3D 프롭 (Interactable Seating)
- **Poly Haven ([polyhaven.com/models](https://polyhaven.com/models)) — 100% CC0 완전 무료**
  - 상업/비상업 제한 없는 CC0 라이선스 + 4K/8K PBR 텍스처 기본 포함:
    1. `Wooden Chair 01`
    2. `Painted Wooden Chair 01` / `Painted Wooden Chair 02`
    3. `Painted Wooden Bench`
    4. `Modular Street Seating`
  - **적용 방법:** GLTF 또는 FBX로 다운로드해 UE5 `Content/Models/Props/` 로 임포트한 뒤, 레벨에 배치한 액터 위에 `AInteractableSeatActor` 컴포넌트를 배치하고 `LinkedPerformer` 에 기타 액터를 지정하면 `[E] 앉기/일어나기`와 카메라 블렌딩이 자동 연결됩니다.

---

## 3. 캐릭터 & 연주 손 모델링 (Characters & Hands)
- **MetaHuman (Unreal Engine 공식)**
  - **비용/라이선스:** 연 매출 100만 달러 미만 개인/인디 무료
  - **활용:** MetaHuman Creator에서 실사 캐릭터(혹은 1인칭 팔·손 전용 리그)를 생성해 UE5로 임포트한 후, 본 프로젝트 `UGuitarFretboardComponent::ComputeHandIKTargets()` 가 매 프레임 산출하는 왼손 4손가락(`Index/Middle/Ring/Pinky`) 및 오른손 피킹/스트로크 좌표를 MetaHuman `Control Rig` IK 타깃에 바인딩합니다.

---

## 4. 어쿠스틱 / 일렉트릭 기타 3D 모델
- **즉시 사용 가능 내장 모델:** `ue5/RealGuitarGame/Content/Models/AcousticGuitar_Reference.glb`
  - 648mm 실제 스케일, 20프렛 간격, 6개 독립 줄 머티리얼 슬롯(`String_1..6`)이 사전 보정되어 있어 곧바로 줄 진동 셰이더(`UGuitarStringVisualComponent`)가 작동합니다.
- **실사 하이폴리 교체:** `Fab.com` 무료 필터(`Price: Free`) 또는 Sketchfab CC0에서 `Acoustic Guitar` 검색 후 임포트 → `AGuitarPerformerActor::GuitarMesh` 스태틱 메시를 교체하고 `Fretboard` 컴포넌트의 `Nut` 기준 위치만 맞추면 됩니다.

---

## 5. 게임 UI / UX (CommonUI + 내장 다크 글래스 UI 팩)
- **즉시 작동하는 내장 UI (`C++` 구현 완료):**
  - 타이틀 화면 & 일시정지 메뉴(`URealGuitarMenuWidget`, `[ESC]`): 곡 선택, 즉석 코드/아르페지오/스트로크 패턴 생성, 해상도/창모드/FPS/그래픽 프리셋/Lumen 설정, 마스터·기타·환경음·리버브 볼륨, 시간대 분위기 전환, 마우스 감도/FOV/Y축 반전 및 JSON 영구 저장(`UGuitarGameSettingsSubsystem`).
  - 인게임 감상 HUD(`UGuitarPerformanceHUDWidget`): 탐색 시 `[E]` 프롬프트, 착석 시 하단 플레이어 바 + 실시간 코드/진행도 카드 + `[H]` 시네마틱 HUD 숨김.
- **UI 아이콘 & 다크 글래스 패널 텍스처 팩 (생성 포함):**
  - 경로: `ue5/RealGuitarGame/Content/UI/Textures/` (`tools/generate_ui_textures.py`)
  - 포함: `T_UI_Play`, `T_UI_Pause`, `T_UI_Next`, `T_UI_Prev`, `T_UI_Loop`, `T_UI_Camera`, `T_UI_Settings`, `T_UI_MoodSunset`, `T_UI_MoodNight`, `T_UI_SitBench`, `T_UI_GlassPanel_9Slice`
  - 에디터에서 `Tools -> Execute Python Script` 로 `ue5/RealGuitarGame/Tools/ue5_editor_bootstrap.py` 실행 시 `/Game/UI/Textures` 로 자동 임포트됩니다.
- **권장 무료 한글 폰트 (OFL 라이선스):**
  - `Pretendard` 또는 `나눔스퀘어라운드 (NanumSquareRound)` `.ttf`/`.otf`를 UE5 `Content/UI/Fonts/` 로 임포트해 프로젝트 기본 UI Font에 지정하면 더욱 선명한 한글 UI를 볼 수 있습니다.

---

## 6. 무료 고음질 기타 사운드 뱅크
- **즉시 사용 가능 내장 뱅크:** `tools/prepare_ue5_assets.py` 가 E2~C6 멀티샘플 WAV(`ue5/RealGuitarGame/Content/Audio/Samples/`)와 `multisample_map.json`을 생성해 `ue5_editor_bootstrap.py` 가 `/Game/Audio/Samples` 로 자동 임포트합니다.
- **실사 생기타 음원 확장 (무료):** `Harmonix` + `MetaSounds` 파이프라인(`UGuitarAudioMetaSoundBridge`)에 무료 CC0 어쿠스틱 단음 녹음 샘플(예: `Freesound.org` CC0 `Acoustic Guitar Single Notes`) 또는 무료 어쿠스틱 사운드폰트 WAV를 연결해 더욱 풍성한 생기타 울림으로 업그레이드할 수 있습니다.
