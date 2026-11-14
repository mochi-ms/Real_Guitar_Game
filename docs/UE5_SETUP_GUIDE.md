# Unreal Engine 5 개발 환경 설치 가이드

## 1. 현재 PC 하드웨어 점검 결과
2026-11-14 기준 시스템 점검 결과, **실사급 언리얼 엔진 5 개발 및 실행에 매우 충분한 사양**입니다.

| 항목 | 현재 PC 상태 | UE5 실사 권장 기준 |
|---|---|---|
| **GPU** | `NVIDIA GeForce RTX 4070 SUPER` | RTX 3060 / 4060 이상 (충분함) |
| **RAM** | `31.65 GB` (약 32GB) | 32GB 권장 (충분함) |
| **여유 저장공간** | `C:` 드라이브 `1,705.19 GB` 여유 | 150~300GB 이상 권장 (매우 넉넉함) |
| **설치된 개발 도구** | Python 3.12, Git, GitHub CLI (`gh`), VS Code | — |
| **미설치 (설치 필요)** | **Visual Studio 2022 (C++)**, **Epic Games Launcher**, **Unreal Engine 5** | 아래 순서로 설치 필요 |

---

## 2. 필수 프로그램 설치 순서

### Step 1. Visual Studio 2022 Community (C++ 컴파일러)
언리얼 엔진 5 C++ 소스(`ue5/RealGuitarGame/Source/`)를 빌드하려면 반드시 필요합니다.
PowerShell에서 아래 명령어로 설치할 수 있습니다(또는 Visual Studio Installer 다운로드):
```powershell
winget install Microsoft.VisualStudio.2022.Community
```
설치 화면(Visual Studio Installer)에서 아래 **워크로드**를 반드시 체크하세요:
- **C++를 사용한 데스크톱 개발 (Desktop development with C++)**
- **C++를 사용한 게임 개발 (Game development with C++)**

### Step 2. Epic Games Launcher & Unreal Engine 5 설치
```powershell
winget install EpicGames.EpicGamesLauncher
```
1. **Epic Games Launcher**를 실행하고 에픽게임즈 계정으로 로그인합니다. (무료 가입)
2. 왼쪽 메뉴 **Unreal Engine -> 라이브러리(Library) -> 엔진 버전(+)** 클릭 후 최신 버전(**UE 5.8** 등)을 설치합니다. (대략 50~80GB 다운로드)

### Step 3. 프로젝트 열기 & C++ 컴파일
1. 탐색기에서 `ue5/RealGuitarGame/RealGuitarGame.uproject` 파일을 우클릭 -> **Generate Visual Studio project files** 선택.
2. `RealGuitarGame.uproject` 더블클릭 -> C++ 모듈 빌드 확인 창이 뜨면 **예(Yes)** 클릭.

### Step 4. 무료 실사 에셋 & 기타 음원 가져오기 (Fab)
1. **배경 공간:** 에픽게임즈 **Fab**에서 **Quixel Megascans**(영구 무료) 환경/나무/벤치/인테리어 에셋 추가.
2. **연주자 손/캐릭터:** UE5 내장 **MetaHuman**으로 실사 캐릭터 생성 (무료, 매출 100만 달러 미만).
3. **기타 3D 모델 & 멀티샘플 음원:** Fab 또는 Sketchfab에서 무료 어쿠스틱/일렉 기타 모델(FBX/GLTF)과 무료 기타 멀티샘플(WAV/SFZ)을 프로젝트 `Content/`에 임포트한 뒤, `UGuitarFretboardComponent`의 Nut/Bridge 길이를 맞추고 `Harmonix FusionPatch`에 연결합니다.
