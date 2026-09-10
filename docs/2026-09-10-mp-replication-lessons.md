# 멀티플레이 리플리케이션 삽질 복기 (2026-09-09 ~ 09-10)

낙하(drop)·비행기·지도 시스템을 싱글 → 리슨서버 멀티로 옮기며 밟은 함정 모음.
증상 → 원인 → 왜 → 해법 → 교훈.

---

## 1. 무기가 2개 스폰됨

- **증상**: 캐릭터 손에 총 2정. 1인칭 스코프 때 얼굴 옆에 유령 총.
- **원인**: `EquipDefaultWeapon()` 이 `BeginPlay` 에서 모든 머신에 실행. 서버가 스폰→복제하는데, 클라도 `BeginPlay` 시점엔 `EquippedWeapon` 이 아직 null → 클라가 자기 총을 또 스폰.
- **해법**: `if (!HasAuthority()) return;` — 스폰은 서버만. 클라는 `OnRep_EquippedWeapon` 으로 받은 총을 손에 붙이기만.
- **교훈**: "생성"은 서버만. 클라는 복제로 받는다. `BeginPlay` 시점엔 복제가 아직 안 왔을 수 있음.

## 2. 낙하 상태(DropState) / 조준(AimMode) 가 다른 클라에 안 보임

- **증상**: 상태가 입력 핸들러에서만 바뀜 → 서버·타 클라는 모름.
- **원인**: 입력은 소유 클라에서만 실행됨. 그냥 `UPROPERTY` 는 복제 안 됨.
- **해법**: `UPROPERTY(ReplicatedUsing = OnRep_X)` + `GetLifetimeReplicatedProps` 등록. 입력 → `Server*` RPC → 서버가 값 변경 → `OnRep_X` 가 클라에서 자동 호출. 연출 코드를 `ApplyX()` 로 분리 → 서버 경로와 OnRep 경로가 같은 함수를 탐.
- **교훈**:
  - `UPROPERTY` 는 명시 안 하면 복제 안 됨 (`Replicated` 지정자 + `DOREPLIFETIME`).
  - 입력→서버는 `UFUNCTION(Server, ...)`. 이름이 아니라 지정자가 서버로 보냄.
  - "상태 변경"과 "그 연출"을 분리하면 서버/클라가 같은 코드로 일치.

## 3. 클라가 비행기에 안 탐

- **증상**: 호스트는 타는데 클라만 지상 PlayerStart에 서 있음.
- **원인**: 비행기가 `BeginPlay` 다음 틱 1회만 탑승 시도. 그 시점에 클라 폰이 서버에서 아직 possess 안 됨 → `PC->GetPawn()` null → 스킵. 그리고 `GetPlayerCharacter(this, 0)` 은 로컬 플레이어 0 = 항상 호스트만 반환.
- **해법**: 0.25초 반복 타이머 + 순회를 `It->Get()->GetPawn()` 로 (그 PC의 폰).
- **교훈**: 접속·possess·스폰은 즉시가 아님. 1회성 대신 재시도. `GetPlayerController(0)` / `GetPlayerCharacter(0)` 은 로컬 인덱스 → 서버에선 호스트.

## 4. F로 뛰어내려도 비행기로 순간이동 (재탑승 루프)

- **증상**: F 여러 번/길게 눌러야 겨우 떨어짐. 클라 비행기 깜빡임.
- **원인**: `TryBoardAll` 이 `DropState != InPlane` 인 사람을 태움. F 누르면 `Freefall` → "InPlane 아님" → 다음 틱에 또 태움.
- **해법**: `DropState == Ground` (갓 스폰 = 아직 한 번도 안 탄 사람) 만 태움.
- **교훈**: 상태 체크할 때 모든 상태를 생각. "관심 있는 하나가 아닌 것"이 아니라 "정확히 대기 상태인 것". `!=` 한 개 vs `==` 한 개는 의미가 완전히 다름.

## 5. 큰 WP 맵에서 클라가 비행기를 아예 못 봄

- **증상**: TestMap(작음)은 됨. BR맵(큰 WP)은 클라에 비행기 없음. 클라 폰은 공중에 떠 있음.
- **원인**: 비행기가 레벨에 배치된 액터 + World Partition spatially loaded. 클라의 WP 스트리밍이 비행기 있는 셀을 안 불러오면 클라 월드에 비행기 인스턴스 자체가 없음. `bAlwaysRelevant` 는 넷 릴러번시지 WP 스트리밍이 아님.
- **해법**: 배치 액터 삭제 → `ADropGameMode::BeginPlay` 에서 서버가 `SpawnActorDeferred`. 런타임 스폰 액터는 WP 셀 관리 대상이 아님 → `bReplicates` + `bAlwaysRelevant` → 전 클라에 무조건 복제.
- **교훈**:
  - WP에서 배치 액터 = 셀 스트리밍 종속 (클라가 못 받을 수 있음).
  - 서버 런타임 스폰 + bAlwaysRelevant = 확실. 매치 시작 오브젝트(비행기 등)의 정석.
  - `bAlwaysRelevant`(넷) ≠ `bIsSpatiallyLoaded`(WP 스트리밍) — 다른 레이어.

## 6. 비행기 이동이 클라에서 버벅/역행/딴 위치 (제일 오래 걸림, 4단계)

### 6a. `SetReplicateMovement` 이 불안정
`SetActorLocation` 으로 매 틱 옮기는 액터는 `AActor` 의 `FRepMovement` 복제가 부실 → 클라 비행기가 얼거나 튐.
→ 결정론적 lerp: 서버가 진행률만 관리, 각 머신이 `Lerp(Start, End, alpha)` 계산.

### 6b. `GetServerWorldTimeSeconds()` 가 요동침
프레임 히칭(VRAM 터짐) 심하면 서버시계 추정이 `a=0.41 → 0.19` 로 역행.
→ 시계 계산 버리고 `FlightAlpha`(진행률) 자체를 복제 + 클라는 매 프레임 전진 후 `FInterpTo` 로 수렴.

### 6c. 비행 타임라인이 레벨 로드부터 시작
BR맵 로딩 30~55초에 비행 시간이 다 잡아먹혀서, 탑승할 땐 이미 비행기가 맵 끝(바다).
→ 첫 탑승 시점에 `FlightStartTime` 기록 (이륙).

### 6d. 경로(`StartPoint`/`EndPoint`)가 복제 안 됨 ★핵심★
`SetRoute` 는 서버 비행기에만 경로를 넣음. 클라 비행기는 BP 기본 경로로 날아감 → 서버/클라 비행기가 다른 항로. "앞에서 다가와 지나간 유령 비행기" = 클라의 로컬 비행기.
→ `UPROPERTY(EditAnywhere, Replicated)` + `DOREPLIFETIME_CONDITION(..., COND_InitialOnly)` (스폰 시 1회만).

- **교훈**:
  - 연속 이동은 최소 권위값(스칼라 하나, 시작 시각)만 복제하고 각자 계산, 또는 값 복제 + 보간. `AActor` FRepMovement 에 의존 X.
  - `GetServerWorldTimeSeconds` 는 접속 직후/히칭 시 못 믿음.
  - `SetX()` 를 서버에서 불렀다고 클라에 안 감. 그 프로퍼티가 `Replicated` 여야 함 (이거 놓쳐서 하루 날림).
  - 한 번만 세팅되는 값은 `COND_InitialOnly`.

## 7. BR맵 PIE = 서버 넷 스레드 58초 정지

- **증상**: PIE 로비→BR 하면 클라 60초간 접속 불가, 패킷 다 드롭, 서버시간 58초 점프.
- **원인**: 쿡 안 된 거대 WP 맵 로딩 55초가 게임 스레드(=넷 스레드)를 통째로 블록.
- **해법**: 없음 (에디터 전용 오버헤드). 쿡 빌드는 1초 → BR맵 멀티 테스트는 패키지로만.
- **교훈**: 에디터 PIE ≠ 출시 게임. 무거운 맵의 네트워킹 디버깅은 패키지 빌드로. 로그도 화면에 찍어서(`AddOnScreenDebugMessage`, 안정 키, 서버/클라 나란히) 봐야 추측 안 하고 잡음.

## 8. 클라 미니맵이 딴 곳을 가리킴

- **증상**: 로그상 `selfLoc` 은 같은데 지도 렌더가 호스트랑 다름.
- **원인**: `WorldMin/Max`(맵 경계)를 `ADropPlayerController::BeginPlay` 가 `GetAuthGameMode()->GetMapBounds()` 로 받는데, 클라는 `GetAuthGameMode` = null → 경계가 기본값 `±100000` 그대로. 서버는 튜닝된 `(7655~407655, ...)` → 투영이 완전히 다름.
- **해법**: `EnsureBounds()`: 클라는 복제된 `GameState->GameModeClass` 의 CDO 에서 경계값 읽기 (클래스 디폴트라 CDO에 있음). 지연 로드(1회) + `mutable` lazy 캐시 + 자가 재시도.
- **교훈**:
  - GameMode 는 클라에 존재하지 않음. 클라가 필요한 매치 설정은 → 복제(Actor/PlayerState/GameState/PC) 하거나, 클래스 CDO 에서 읽기.
  - `GameState->GameModeClass` 는 클라에 복제됨 → CDO 접근 경로.
  - `mutable` + 플래그 = const 접근자에서 지연 캐시하는 표준 패턴.

---

## 관통하는 원칙

1. 서버가 진실, 클라는 복제받은 것만 안다. "호스트만 되고 클라만 안 됨" = 대부분 "서버에서만 하고 전파 안 함".
2. 복제는 opt-in. `UPROPERTY` 에 `Replicated` + `GetLifetimeReplicatedProps` 등록 안 하면 안 감.
3. GameMode 는 서버 전용. `GetAuthGameMode` 는 클라에서 null.
4. WP 배치 액터 ≠ 클라에 존재 보장. 확실히 하려면 서버 런타임 스폰 + `bAlwaysRelevant`.
5. 타이밍 가정 금지. 접속/possess/GameState/스폰 다 지연됨 → 1회성 대신 재시도/지연 로드.
6. 연속 이동은 결정론 or 값복제+보간. `AActor` FRepMovement 나 서버시계에 기대지 마.
7. 에디터 PIE ≠ 게임. 무거운 맵은 패키지로 네트워크 테스트.
8. 추측 대신 로그. 서버/클라 값을 화면에 나란히 찍으면 desync 가 즉시 보임.

---

## 아직 남은 것

- 클라 비행기 살짝 버벅임 (보간 `FInterpTo` 속도 8 → 더 올리거나 제대로 된 스무딩).
- 다른 플레이어 색깔 마커: `GetPlayerMarkers` 가 `PS->GetPawn()->GetActorLocation()` 을 읽는데 BR맵에서 멀리 있는 폰은 relevant 안 해서 null → `ADropPlayerState` 에 `FVector MapLocation` 복제 추가 필요.
