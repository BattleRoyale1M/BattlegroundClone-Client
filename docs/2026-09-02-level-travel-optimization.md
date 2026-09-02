# 로비 → 낙하맵 전환 "20초 락" 분석 & 최적화 방향

> 작성일: 2026-09-02 · 엔진: UE 5.8 · 브랜치: main

## 1. 증상

- 로비(`L_TestRange`)에서 `CountdownSeconds`(5s) 타이머가 끝나는 순간부터
  약 20초 동안 게임이 완전히 멈춘 것처럼 보임("lock").
- 다음 맵(`BattleRoyale_Map_a_WP`)이 World Partition + 약 15,000 uasset 규모라
  세팅에 시간이 오래 걸림.

## 2. 원인 (코드 기준)

`ALobbyGameMode::TravelToMatch()` → `UGameplayStatics::OpenLevel()`.

- **`OpenLevel`은 동기(blocking) flush 로드.** 타이머가 끝나는 순간
  게임 스레드가 통째로 정지 → 다음 맵이 다 뜨고 시작 지점 주변
  WP 셀 스트리밍이 끝날 때까지 프레임이 안 넘어감. 이게 "20초 락"의 정체.
- 상태머신(`SetDropState`)·입력 잠금과는 **무관**. `EnterPlane()`의
  `DisableMovement()`나 별도 입력 락이 20초를 만드는 게 아님.
- **에디터/PIE 첫 로드 특유의 비용**이 크게 얹힘:
  - 셰이더 컴파일
  - Nanite / distance field / 텍스처 등 에셋 온디맨드 컴파일
  - → 패키징된 Development 빌드나 Standalone에서는 이 부분이 대부분 사라짐.
- WP 런타임 그리드의 Loading Range가 넓으면 시작하자마자 많은 셀을 물고 들어옴.

## 3. 최적화 방향 (권장 순서)

### 3.1 먼저: 진짜 병목 가르기
- **Standalone Game 또는 패키지(Development) 빌드로 실제 전환 시간 측정.**
- 에디터 수치는 과장돼 있음. 여기서 이미 견딜 만하면 나머지는 연출로 덮으면 끝.
- 판단: 병목이 "맵 로드 자체"인지 "에디터 에셋/셰이더 컴파일"인지 확정.

### 3.2 본질적 해결: Seamless Travel
- **GameMode `bUseSeamlessTravel = true`** + `DefaultEngine.ini`에 작은 TransitionMap 지정.
  ```
  [/Script/EngineSettings.GameMapsSettings]
  TransitionMap=/Game/Maps/L_Transition
  ```
- 목적지 맵을 백그라운드 스레드에서 로드 → 하드 프리즈 제거.
- 데디케이티드 서버 지향 프로젝트라 어차피 필요한 방식(클라 연결 유지). 장기적으로 맞는 선택.
- 주의: seamless travel 시 유지할 액터는 `GetSeamlessTravelActorList` / `bCanBeInCluster` 등 고려.
  현재는 폰/PC 정도라 부담 적음.

### 3.3 연출로 덮기: 로딩 스크린
- `MoviePlayer`의 `SetupLoadingScreen` 또는 `FCoreUObjectDelegates::PreLoadMap` 델리게이트로
  로딩 위젯/무비 표시.
- 속도는 그대로지만 "멈춤"이 "로딩 중"으로 바뀌어 체감 개선.
- seamless travel과 병행하면 전환 구간이 매끄러워짐.

### 3.4 World Partition 튜닝
- 런타임 그리드 **셀 크기 / Loading Range 축소** → 시작 시 로드하는 셀 수 감소.
- **HLOD 활성화** → 원경은 프록시 메시로 표시, 풀 에셋 로드 회피.
- 필수 아닌 콘텐츠는 시작 시 **Unloaded 상태인 Data Layer**로 분리.
- **`Is Spatially Loaded = false`** 인 액터 점검 → 이게 많으면 전부 시작 시 로드됨.
- 콘솔: `wp.Runtime.MaxLoadingLevelStreamingCells` 로 프레임당 로딩 셀 스로틀.

### 3.5 캐시 워밍
- DDC 공유/워밍, 셰이더 프리컴파일 → 반복 테스트 시 2회차부터 빨라짐.

## 4. 결론

- 가장 효과 큰 조합: **3.2 (seamless travel) + 3.3 (로딩 스크린)**.
- 단, 그 전에 **3.1로 진짜 병목이 로드인지 에디터 컴파일인지부터 확정**할 것.
- 3.4는 맵 자체를 다이어트하는 작업이라 병렬로 천천히 진행.

## 5. 관련 파일

| 파일 | 관련 내용 |
|---|---|
| `Source/BattlegroundClone/Core/LobbyGameMode.cpp` | `TravelToMatch()` → `OpenLevel` (교체 대상) |
| `Source/BattlegroundClone/Core/LobbyGameMode.h` | `CountdownSeconds`, `TargetLevel` |
| `Config/DefaultEngine.ini` | TransitionMap, seamless travel 설정 위치 |
| `Content/Maps/` | TransitionMap용 경량 맵 신규 필요 |
