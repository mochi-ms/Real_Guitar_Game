# GuitarPerformance JSON 규격 (`schemas/guitar_performance.schema.json`)

## 1. 좌표 및 현 번호 규약
- **`string`**: `1` (가장 가는 1번 줄 High E4) ~ `6` (가장 굵은 6번 줄 Low E2)
- **`tuning_midi`**: 기본 스탠다드 튜닝 `[64, 59, 55, 50, 45, 40]` (1번 줄 → 6번 줄 순서)
- **`fret`**: `0` (개방현) ~ `24` (코드 스냅샷 배열에서는 `-1`이 뮤트 `X`)
- **`left_finger`**: `0` (개방현/누르지 않음), `1` (검지), `2` (중지), `3` (약지), `4` (소지), `5` (엄지)

## 2. 오른손 주법 (`right_action`)
| 값 | 설명 |
|---|---|
| `pluck_thumb` | 엄지(`P`) 핑거스타일 피킹 (주로 6, 5, 4번 줄) |
| `pluck_index` | 검지(`i`) 핑거스타일 피킹 (주로 3번 줄) |
| `pluck_middle` | 중지(`m`) 핑거스타일 피킹 (주로 2번 줄) |
| `pluck_ring` | 약지(`a`) 핑거스타일 피킹 (주로 1번 줄) |
| `pick_down` / `pick_up` | 피크 단음 다운/업 피킹 |
| `strum_down` | 굵은 줄에서 가는 줄 방향으로 쓸어내리는 스트로크 (줄별 3~4ms 시차 포함) |
| `strum_up` | 가는 줄에서 굵은 줄 방향으로 쓸어올리는 업 스트로크 |
| `percussive_slap` | 오른손/왼손 뮤트 퍼커시브 커팅 (`X`) |

## 3. 연주 아티큘레이션 (`articulation`)
- `normal`, `let_ring`, `palm_mute`, `dead_note`, `hammer_on`, `pull_off`, `slide`, `harmonic`, `vibrato`

## 4. `.rgchart` 작성 문법 예시
```json
{
  "title": "나만의 어쿠스틱 곡",
  "bpm": 84,
  "sections": [
    {
      "name": "Verse",
      "style": "fingerstyle",
      "pattern": "pima_ballad",
      "measures": ["C", "G/B", "Am7", "Fmaj7"]
    },
    {
      "name": "Chorus",
      "style": "strum",
      "pattern": "16beat_ballad",
      "measures": ["C", "G", "Am", "Fmaj7"]
    }
  ]
}
```
내장 스트로크 프리셋: `8beat_folk`, `16beat_ballad`, `pop_rock`, `island_strum`, `waltz_3_4`  
내장 핑거스타일 프리셋: `pima_ballad`, `folk_arp_8`, `travis_pick`, `waltz_arp_3_4`, `romanza_triplet`
