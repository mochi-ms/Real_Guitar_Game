# RealGuitarGame

언리얼 엔진 5(UE5) 실사 공간에서 1인칭으로 걸어 다니다가 벤치나 의자에 앉아, **타브 악보(MusicXML / Guitar Pro) · 코드 진행+주법(.rgchart) · MIDI**를 넣으면 자동으로 기타를 연주하는 모습을 감상할 수 있는 프로젝트입니다.

## 핵심 특징
- **스트로크 & 핑거스타일 통합 인식 아키텍처:**
  - **타브 악보 (`MusicXML .musicxml/.mxl`, `Guitar Pro .gp/.gpif`)**: 줄 번호(1~6), 프렛(0~24), 해머링/풀링/슬라이드/브러시 스트로크 방향을 손실 없이 그대로 파싱
  - **코드+주법 차트 (`.rgchart`)**: 코드 진행과 스트로크(`8beat_folk`, `16beat_ballad`, `pop_rock` 등) · 핑거스타일(`pima_ballad`, `travis_pick` 등) 패턴을 섹션별로 자유롭게 조합
  - **일반 MIDI (`.mid`)**: Dynamic Programming(Viterbi) 기반 6현 24프렛 자동 운지 최적화
  - 위 모든 입력이 하나의 표준 **`GuitarPerformance.json`** 타임라인(`schemas/guitar_performance.schema.json`)으로 컴파일되어 UE5와 미리보기에서 동일하게 구동됩니다.

## 폴더 구조
- `docs/`: 전체 아키텍처([ARCHITECTURE.md](docs/ARCHITECTURE.md)), 데이터 규격([DATA_SCHEMA.md](docs/DATA_SCHEMA.md)), UE5 설치 가이드([UE5_SETUP_GUIDE.md](docs/UE5_SETUP_GUIDE.md))
- `engine/`: Python 3.12 무의존(Zero-dependency) 악보 파서 · 운지 솔버 · Karplus-Strong 어쿠스틱 기타 WAV 합성기 · 미리보기 서버
- `preview/`: 브라우저에서 기타 지판·운지·스트로크/핑거스타일·소리를 즉시 테스트할 수 있는 웹 시뮬레이터
- `samples/`: 샘플 타브 악보(`fingerstyle_canon_intro.musicxml`) 및 코드+스트로크/핑거스타일 차트(`acoustic_ballad_strum.rgchart`, `pop_strum_groove.rgchart`)
- `ue5/RealGuitarGame/`: Unreal Engine 5.8 C++ 프로젝트 스켈레톤 (`UGuitarScoreSubsystem`, `UGuitarFretboardComponent`, `UGuitarAudioMetaSoundBridge`, `AInteractableSeatActor`, `AAcousticChillCharacter`)
- `tests/`: 파서, 운지 솔버, WAV 합성기, 미리보기 서버 단위 테스트

## 지금 바로 실행해보기 (UE5 설치 전에도 즉시 동작)

### 1. 테스트 실행
```powershell
python -m unittest discover -s tests -v
```

### 2. 악보 컴파일 & 어쿠스틱 기타 WAV 생성
```powershell
python -m engine.compiler samples/fingerstyle_canon_intro.musicxml --wav
python -m engine.compiler samples/acoustic_ballad_strum.rgchart --wav
```

### 3. 브라우저 지판·운지·사운드 시뮬레이터 실행
```powershell
python -m engine.preview_server
```
실행 후 브라우저에서 `http://127.0.0.1:8765` 에 접속하면 샘플 악보를 들으면서 지판 위 왼손 운지와 오른손 스트로크/핑거스타일 움직임을 실시간으로 확인하고, 코드 진행과 주법을 바꿔가며 바로 소리를 생성할 수 있습니다.
