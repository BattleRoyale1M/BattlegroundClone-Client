# 배틀그라운드 모작 — 초기 환경 구축 & 진행 현황

> 작성일: 2026-09-01 · 엔진: UE 5.8 · 브랜치: main

## 1. 목적 / 배경

- **목표**: PUBG류 배틀로얄 모작 포트폴리오. 최종적으로 **UE5 C++ / 데디케이티드 서버 멀티플레이** 지향.
  - 레퍼런스: [[1인 프로젝트] UE5 C++/데디서버 배틀그라운드 모작 포트폴리오](https://youtu.be/Z-3YWwwM5Qs)
- **우선순위**: 캐릭터보다 배경(맵)을 먼저 확보 → 이후 시그니처 플로우(대기방 → 매칭 → 비행기 → 낙하산).
- **학습 접근**: 싱글플레이 프로토타입 → 리슨 서버 → 데디케이티드 서버로 **단계적 확장**.
  코드는 처음부터 C++로 작성하되 리플리케이션을 염두에 두고 설계(멀티 전환 시 재작성 최소화).

## 2. 개발 환경 세팅 (전역 / 시스템 공통)

프로젝트 한정이 아니라 **모든 UE 5.8 프로젝트에 적용**되도록 구성.

### 2.1 MCP — AI가 에디터를 직접 인지·편집
- 목적: Claude가 레벨/액터/블루프린트/에셋을 직접 조회·수정할 수 있게 함.
- `unreal-mcp` 서버 등록을 **프로젝트 스코프(`.mcp.json`) → user 스코프(`~/.claude.json`)** 로 이동. 프로젝트 `.mcp.json` 삭제.
- 엔진 플러그인 `"EnabledByDefault": true` 로 전역 활성화 (각 `.uplugin` 옆에 `.bak-<ts>` 백업):
  `ModelContextProtocol`, `ToolsetRegistry`, `EditorToolset`, `UMGToolSet`
  경로: `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\...`
  ⚠️ 엔진 Verify/업데이트 시 초기화됨 → `.bak` 참고해 재적용.
- 전역 config 신규 생성 — `C:\Users\user\AppData\Local\Unreal Engine\Engine\Config\`:
  - `UserEditorPerProjectUserSettings.ini` → MCP HTTP 서버 자동 시작
    (`bAutoStartServer=True`, `ServerPortNumber=8000`, `ServerUrlPath=/mcp`)
  - `UserEditorSettings.ini` → 기본 소스 에디터 = Rider (`PreferredAccessor=Rider`)

### 2.2 기본 IDE = JetBrains Rider (전역)
- `PreferredAccessor=Rider` (Rider 플러그인 v1.7의 sln 통합 accessor 이름은 literal `Rider`).
- 에디터에서 C++ 파일 열기·솔루션 생성이 Rider로 연결됨.

## 3. 프로젝트 구조 결정

- **단일 게임 모듈** `BattlegroundClone` (`Source/BattlegroundClone/`).
- `Public/` `Private/` 분리 안 함 (게임 모듈 1개, YAGNI). **feature 서브폴더**로 정리:
  | 폴더 | 용도 |
  |---|---|
  | `Core/` | GameMode, 공유 타입(enum) |
  | `Character/` | 플레이어 폰 |
  | `Drop/` | 비행기·낙하산 (예정) |
- **C++ = 로직/데이터 계약, BP = 그 클래스의 인스턴스 설정값.** BP는 `Content/Blueprints/` 아래 동일 구조로 미러링.
- `Content/BluePrints` → `Content/Blueprints` 로 폴더명 케이싱 수정 (Linux 데디서버에서 경로 대소문자 구분 대비).

## 4. 현재까지 구현

### C++ (빌드 통과)
| 심볼 | 경로 | 상태 |
|---|---|---|
| `EDropState` (enum) | `Core/Enums/DropTypes.h` | `Ground / InPlane / Freefall / Parachuting`. 상태머신용, 아직 미사용 |
| `ADropGameMode` | `Core/DropGameMode.{h,cpp}` | `DefaultPawnClass = ADropCharacter` |
| `ADropCharacter` | `Character/DropCharacter.{h,cpp}` | `ACharacter` 상속, 위저드 스텁 (컴포넌트/입력/상태 미구현) |
| `ALobbyGameMode` | `Core/LobbyGameMode.{h,cpp}` | `BeginPlay`에서 `CountdownSeconds`(5s) 타이머 → `TravelToMatch()` → `UGameplayStatics::OpenLevel(TargetLevel)`. `TargetLevel`은 `TSoftObjectPtr<UWorld>` |

### 애셋 / 에디터
- `BattleRoyaleIslandPack`(환경 아트팩)이 이미 `Content/BattleRoyaleStarterKit/`에 마이그레이션 완료
  — 배틀로얄 섬 맵 2개(`BattleRoyale_Map_a_WP`, `Map_b`, 둘 다 World Partition) + 건물/프롭 쇼케이스 맵, 약 15,000 uasset, git LFS 커밋·푸시됨.
  이 팩엔 **로비/메뉴 맵은 없음** (순수 아트팩).
- `BP_DropGameMode` (parent `ADropGameMode`) — `Content/Blueprints/Core/`
- `BP_DropCharacter` (parent `ADropCharacter`, `Mesh = SKM_Manny_Simple`, AnimBP 미지정) — `Content/Blueprints/Character/`
- 맵 계획: `Content/Maps/L_TestRange`(3인칭 템플릿 잔재 그레이박스)를 **로비로 재사용**, `BattleRoyale_Map_a_WP`를 **낙하맵**으로.

## 5. 게임플레이 설계 방향

### 플로우
```
로비(L_TestRange) --5초 타이머--> 낙하맵(BattleRoyale_Map_a_WP)
   --> 비행기 탑승 --> Space 탈출 --> 자유낙하 --> 낙하산 전개 --> 착지 --> (게임플레이)
```

### 상태머신 — 자유낙하~낙하산의 척추
`Ground → InPlane → Freefall → Parachuting → Ground`
`SetDropState(New)` 하나로 `CharacterMovement` 파라미터(GravityScale/AirControl/속도 클램프) + AnimBP 상태 변수를 전환.

### 이동 구현 전략
- **A단계(시작용)**: 순정 `UCharacterMovementComponent`. `MOVE_Falling` / `MOVE_Flying` 토글 + 파라미터 스왑 + `TickComponent`에서 종단속도 클램프.
- **B단계(제대로)**: `UCharacterMovementComponent` 상속 → `MOVE_Custom` + `CustomMovementMode` + `PhysCustom()` 오버라이드. saved move 시스템으로 리플리케이션 자동.

### 마일스톤
- **M0 (진행 중)**: 캐릭터 스폰 + 로비 → 낙하맵 트래블
- **M1**: 비행기 고정 경로 비행 (`APlane`, `BP_Aircraft_Full` 메시 재사용)
- **M2**: 비행기 탑승 + 탈출 + 자유낙하
- **M3**: 낙하산 전개 + 활공(WASD) + 착지 트레이스
- **M4**: 카메라/FOV/속도/사운드 감 튜닝
- 이후: 대기방 UI·매칭(세션/PostLogin), 리슨 서버, 데디케이티드 서버(`*Server.Target.cs`), 자기장/루팅/전투

## 6. 남은 작업 (다음 세션)

- [ ] `BP_LobbyGameMode` 생성 (parent `ALobbyGameMode`) → `Default Pawn Class = BP_DropCharacter`, `Target Level = BattleRoyale_Map_a_WP`, `Countdown Seconds = 5`
- [ ] `L_TestRange` → World Settings → GameMode Override = `BP_LobbyGameMode`
- [ ] `BattleRoyale_Map_a_WP` → World Settings → GameMode Override = `BP_DropGameMode`; 지형 위 `PlayerStart` 배치 후 **Save**(WP 외부 액터)
- [ ] Project Settings → Maps & Modes → Editor Startup Map / Game Default Map = `L_TestRange`
- [ ] Play 테스트: 로비에서 5초 후 낙하맵으로 전환되는지 확인
- [ ] `DropCharacter.h` 채우기: `USpringArmComponent` + `UCameraComponent`, Enhanced Input(`IMC_Default`, `IA_Move/Look/Eject/Dive`), `EDropState DropState` 멤버, `SetDropState()`
- [ ] `DropTypes.h`: `UENUM(BlueprintType)` 다음 빈 줄 제거 (선택, 관례)

## 7. 작업 규칙 메모

| 상황 | 방법 |
|---|---|
| 함수 **본문만** 수정 | 에디터 켠 채 `Ctrl+Alt+F11` (Live Coding) |
| 새 `UCLASS` / `UPROPERTY` / `UFUNCTION` / 멤버 추가·삭제 | 에디터 닫고 Rider 풀 빌드 |
| WP 맵에 액터 배치 | 배치 후 반드시 Save (외부 액터로 저장됨) |
| 커밋 메시지 | 한국어, gitmoji 형식 유지 (`:emoji: 타입 : 요약`) |
| 엔진 Verify/업데이트 후 | MCP 플러그인 `.uplugin` 4종 `EnabledByDefault` 재적용 (`.bak` 참고) |
