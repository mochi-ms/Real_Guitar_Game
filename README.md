# RealGuitarGame

언리얼 엔진 5(UE5) 실사 공간에서 1인칭으로 걸어 다니다가 벤치나 의자에 앉아, **타브 악보(Guitar Pro `.gp5`/`.gp` · MusicXML) · 코드 진행+주법(`.rgchart`) · MIDI**를 넣으면 자동으로 기타를 연주하는 모습을 감상할 수 있는 1인용 기타 시뮬레이션 게임 프로젝트입니다.

## 핵심 특징
- **스트로크 & 핑거스타일 통합 인식 아키텍처:**
  - **타브 악보 (`Guitar Pro .gp3/.gp4/.gp5/.gp/.gpif`, `MusicXML .musicxml/.mxl`)**: 줄 번호(1~6), 프렛(0~24), 해머링/풀링/슬라이드/브러시 스트로크 방향을 손실 없이 그대로 파싱
  - **코드+주법 차트 (`.rgchart`)**: 코드 진행과 스트로크(`8beat_folk`, `16beat_ballad`, `pop_rock` 등) · 핑거스타일(`pima_ballad`, `travis_pick` 등) 패턴을 섹션별로 자유롭게 조합
  - **일반 MIDI (`.mid`)**: Dynamic Programming(Viterbi) 기반 6현 24프렛 자동 운지 최적화
  - 위 모든 입력이 하나의 표준 **`GuitarPerformance.json`** 타임라인(`schemas/guitar_performance.schema.json`)으로 컴파일되어 UE5와 브라우저 시뮬레이터에서 동일하게 구동됩니다.

## 폴더 구조
- `docs/`: 전체 아키텍처([ARCHITECTURE.md](docs/ARCHITECTURE.md)), 데이터 규격([DATA_SCHEMA.md](docs/DATA_SCHEMA.md)), UE5 설치 가이드([UE5_SETUP_GUIDE.md](docs/UE5_SETUP_GUIDE.md))
- `engine/`: 악보 파서(`.gp5`/`.gp`/MusicXML/MIDI/차트) · 운지 솔버 · 휴머나이저 · Karplus-Strong 어쿠스틱 기타 WAV 합성기 · 멀티샘플 은행 생성기 · 미리보기 서버
- `tools/`: UE5 스케일(`64.8cm`) 정밀 매칭 어쿠스틱 기타 3D 모델 생성기(`generate_guitar_glb.py`) 및 에셋 일괄 준비 스크립트(`prepare_ue5_assets.py`)
- `preview/`: 드래그 앤 드롭 업로드, 3가지 카메라 뷰 전환, 미니 코드 다이어그램, 실시간 지판·운지·사운드 스튜디오 시뮬레이터
- `samples/`: 샘플 타브 악보(`fingerstyle_canon_intro.musicxml`) 및 코드+스트로크/핑거스타일 차트(`acoustic_ballad_strum.rgchart`, `pop_strum_groove.rgchart`)
- `ue5/RealGuitarGame/`: Unreal Engine 5 C++ 프로젝트
  - 코어/오디오/IK: `UGuitarScoreSubsystem`, `UGuitarFretboardComponent`, `UGuitarStringVisualComponent`, `UGuitarAudioMetaSoundBridge`
  - 게임플레이/카메라/UI: `AGuitarPerformerActor`, `AInteractableSeatActor`, `AAcousticChillCharacter`, `UGuitarSongLibrarySubsystem`, `UGuitarPerformanceHUDWidget`, `ARealGuitarGameMode`
  - 에셋/자동화: `Content/Models/AcousticGuitar_Reference.glb`, `Content/Scores/*.performance.json`, `Tools/ue5_editor_bootstrap.py`
- `tests/`: 파서(`.gp5` 포함), 운지 솔버, 휴머나이저, GLB 생성기, WAV 합성기, 미리보기 서버 단위 테스트

## 지금 바로 실행해보기 (UE5 설치 전에도 즉시 동작)

### 1. 테스트 실행
```powershell
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
```

### 2. UE5용 악보·3D 모델·멀티샘플 음원 일괄 생성
```powershell
.\.venv\Scripts\python.exe -m tools.prepare_ue5_assets
```

### 3. 브라우저 스튜디오 시뮬레이터 실행
```powershell
.\.venv\Scripts\python.exe -m engine.preview_server
```
실행 후 브라우저에서 `http://127.0.0.1:8765` 에 접속하면 `.gp5`/`.gp`/`.musicxml`/`.mid` 파일을 드래그 앤 드롭하거나 코드+주법을 직접 조합해 지판·코드 다이어그램·카메라 뷰·어쿠스틱 기타 소리를 바로 테스트할 수 있습니다.
