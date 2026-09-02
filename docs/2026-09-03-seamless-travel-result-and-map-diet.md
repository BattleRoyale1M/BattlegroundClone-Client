# Seamless Travel 적용 결과 & 맵 다이어트 계획

> 작성일: 2026-09-03 · 엔진: UE 5.8 · 브랜치: main
> 이어지는 문서: [2026-09-02-level-travel-optimization.md](2026-09-02-level-travel-optimization.md)

## 1. 한 일 — `TravelToMatch` → Seamless Travel

`Source/BattlegroundClone/Core/LobbyGameMode.{h,cpp}`:

- 생성자 추가 → `bUseSeamlessTravel = true;`
- `TravelToMatch()`: `UGameplayStatics::OpenLevel(...)` → `GetWorld()->ServerTravel(PackageName)`
- `TargetLevel` 미설정 시 `UE_LOG(Warning)` (기존엔 조용히 return)
- include 정리: `Kismet/GameplayStatics.h` 제거, `Engine/World.h` 추가

부수: 임시로 만들었다 지운 `UDropGameInstance` 흔적으로 `Config/DefaultEngine.ini`의
`GameInstanceClass=/Script/BattlegroundClone.DropGameInstance` 줄 제거 필요(로컬 정리 항목).

## 2. 검증 — Standalone Game (`-game`) 로그, 2026-09-03 00:26

| 시각 | 이벤트 |
|---|---|
| 15:26:06.6 | 로비 `L_TestRange` 로드 완료 (1.2s) |
| 15:26:12.9 | `LogGameMode: ProcessServerTravel: .../BattleRoyale_Map_a_WP` |
| 15:26:12.9 | `LogWorld: SeamlessTravel to: .../BattleRoyale_Map_a_WP` ← seamless 경로 진입 |
| 15:26:13.0 | `Bringing World /Temp/Untitled_1` ← 빈 transition 맵 (TransitionMap 미지정) |
| 15:26:16.8 | `Bringing World .../BattleRoyale_Map_a_WP up for play` |
| 15:26:16.8 | `----SeamlessTravel finished in 3.87 seconds ------` |

**로비 → 낙하맵 월드 활성화 = 3.87초, 하드 프리즈 없음.** seamless travel 코드는 정상 동작 확정.

- PIE "Selected Viewport"에선 ServerTravel/seamless가 반쪽만 굴러감 → 검증은 Standalone(`-game`)이 정확.
- 일상 확인은 "New Editor Window (PIE)"로 충분(에디터+standalone 동시보다 RAM 유리).

## 3. 남은 문제 — seamless와 무관

`15:26:16` ~ `15:27:47` 약 90초 동안:

- `LogStaticMesh: Waiting on static mesh ... being ready before playing` × 수백 (StaticMesh / Nanite / distance field 빌드)
- `LogAsyncCompilation: [TextureDerivedData] ... MemoryLimit = 88 MiB` ← 텍스처 DDC 컴파일, RAM 고갈
- `UWorld::AddToWorld: updating components ... took 613 ms` ← WP 셀 스트리밍 히칭

이 실행 시작 시점 **여유 RAM 291 MB** (에디터 켜둔 채 standalone 실행 → 16GB 바닥).
→ 이 90초 = **언쿡 빌드의 에셋 컴파일 + RAM 부족**. 쿡(패키지)하면 에셋 컴파일분은 사라짐.
RAM 증설은 현재 불가(비용). 다음 작업은 32GB 머신에서 진행 예정.

## 4. 다음 작업 (32GB 머신)

### Step 0 — 쿡 빌드 1회로 "맵 무게" vs "컴파일" 분리
- Platforms ▸ Windows ▸ Development 로 Package Project.
- 첫 쿡만 오래 걸림(쿡 캐시 이후 재사용).
- `<출력>\...\Saved\Logs\BattlegroundClone.log`에서 `Bringing World .../BattleRoyale_Map_a_WP up for play`
  및 `SeamlessTravel finished in N seconds`, `AddToWorld: updating components` 확인.
- 쿡 후에도 남는 시간 = HLOD 미빌드 + WP 셀 스트리밍. 이게 맵 다이어트 타겟.

### 맵 다이어트 (효과 큰 순)
1. **HLOD 빌드** — `Build ▸ Build HLODs`. 원경 셀을 프록시 메시로 → 시작 시 스트리밍 에셋 급감.
   스타터킷 맵은 HLOD 미빌드 상태로 추정. **효과 최대.**
2. **런타임 그리드 Loading Range 축소** — World Settings ▸ World Partition Setup.
   크면 스폰 즉시 넓은 반경 셀을 물음. 절반으로 줄여 테스트. **효과 큼 / 노력 작음.**
3. **`Is Spatially Loaded = false` 액터 점검** — false면 위치 무관 항상 시작 시 로드.
   로그의 `Waiting on static mesh ... before playing` 수백 줄이 그 신호. WP 아웃라이너에서 큰 액터 감사.
4. **Data Layer로 잡동사니 분리** — `SM_*_Debris`, 호스, 양동이, 소품류를 시작 시 Unloaded Data Layer로.

### 덤 (정리성)
- `SM_Tractor_SingleMesh` — 로그 끝에서 이거 하나 때문에 대기(`Waiting for static meshes 0/1`).
  단일 거대 메시 의심 → 트라이 수 확인.
- `LogLevel: Warning: Failed to find streaming level object 'Battle_Royale_Map_a_x1_y1'` — 죽은
  스트리밍 레벨 참조. 찾아서 제거.
- `Invalid material [MI_Glass] used on Nanite static mesh [SM_Warehouse_1/2]` — Nanite 메시에 반투명
  머티리얼. 시각 경고, 성능 무관. "Disallow Nanite"로 억제 가능.

### 선택 — 로딩 연출
- `Config/DefaultEngine.ini`의 `[/Script/EngineSettings.GameMapsSettings]`에 `TransitionMap=` 지정
  + 경량 `L_Transition` 레벨 신규.
- 현재는 seamless가 빈 임시맵(`/Temp/Untitled_*`)을 씀 → 그 3.87초 구간에 로딩 위젯 띄울 무대가 없음.
- 로딩 위젯: `UGameInstance` 파생 + `FCoreUObjectDelegates::PreLoadMap` / `PostLoadMapWithWorld`
  (또는 `MoviePlayer` 모듈).

## 5. 관련 파일

| 파일 | 내용 |
|---|---|
| `Source/BattlegroundClone/Core/LobbyGameMode.{h,cpp}` | seamless travel 적용 (커밋 대기) |
| `Config/DefaultEngine.ini` | `GameInstanceClass` 줄 제거 필요 / `TransitionMap` 후속 |
| `Content/BattleRoyaleStarterKit/Maps/BattleRoyale_Map_a/BattleRoyale_Map_a_WP` | 맵 다이어트 대상 (HLOD / Loading Range / Data Layer) |
