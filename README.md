# BattlegroundClone

Unreal Engine 5 기반 배틀로얄(PUBG류) 클론 프로젝트. 낙하(Drop) 시퀀스, 전투(Combat), 루팅·인벤토리의 멀티플레이 리플리케이션에 집중해서 만든 C++ 게임플레이 프레임워크.

## 폴더 구조

```
Source/BattlegroundClone/
├── Core/          # 게임모드(로비/매치)·플레이어 컨트롤러/스테이트, 맵 경계 유틸, 공용 enum
│   ├── GameModes/ #   ALobbyGameMode, ADropGameMode
│   └── Enums/     #   EDropState, EFireMode
├── Drop/          # 낙하 시퀀스 전용 액터 (비행기)
├── Character/     # 플레이어 폰 — 이동/카메라/낙하산/조준/상호작용 입력
├── Combat/        # 체력·데미지·투사체 (전투 판정 도메인)
├── Weapon/        # 무기 액터 + 무기 슬롯 인벤토리 컴포넌트
├── Interaction/   # 상호작용 인터페이스 + 월드 아이템(픽업, 루팅 컨테이너)
│   ├── Interactable/
│   └── Item/
└── UI/            # HUD·미니맵·월드맵 라벨·메인메뉴 위젯의 C++ 백엔드
```

각 폴더는 하나의 책임 영역(도메인)을 나타낸다. Unreal은 폴더 구조를 강제하지 않지만, 나중에 플러그인/모듈로 쪼갤 때 폴더 경계가 그대로 모듈 경계가 되도록 처음부터 도메인 단위로 나눠둔 것.

## 게임 흐름

```
메인메뉴(Host/Join) → 로비(카운트다운) → ServerTravel → 매치
  → 비행기 탑승 → 자유낙하 → 낙하산 → 착지 → 루팅/전투 → 사망
```

- `UMainMenuWidget` — Host(리슨 서버로 로비 오픈) / Join(IP 입력 후 접속).
- `ALobbyGameMode` — `CountdownSeconds` 후 `TargetLevel`로 ServerTravel. 쿡된(패키지드) 빌드에서도 동작하도록 맵 경로를 SoftObjectPtr로 관리.

## "Drop"이 의미하는 것

배틀로얄 장르 특유의 **비행기 탑승 → 자유낙하 → 낙하산 → 착지** 흐름을 가리키는 이름. `EDropState`(Ground/InPlane/Freefall/Parachuting) enum으로 상태를 표현하고, `ADropCharacter::DropState`가 이 상태를 Replicated 프로퍼티로 들고 있다. GameMode/Character/PlayerController 이름 전부에 Drop이 붙은 이유는, 이 낙하 시스템이 이 프로젝트의 핵심 게임플레이 루프임을 명시하기 위함이다.

- `ADropGameMode` — 매치 시작 시 비행기 런타임 스폰, 항로(낙하 경로) 주입, 맵 경계값 소유
- `ADropCharacter` — 상태별(Ground/InPlane/Freefall/Parachuting) 이동 로직 + 카메라 전환
- `AAirPlane` — 항로 이동, 탑승 좌석 관리, `FlightAlpha` 서버 권위 계산 + 클라 보간
- `ADropPlayerController` — 미니맵/월드맵 좌표 변환, 낙하 마커, 비행 경로 라인, 지역 라벨(위젯에 데이터만 공급) + HUD/인벤토리/사망 UI 전환
- `ADropPlayerState` — 맵 마커 색상(`MarkerColor`), 킬 수(`KillCount`) 복제

월드맵 지역 이름은 `FMapLocationRow`(DataTable Row) 기반이라, 지역 추가/수정은 C++ 재빌드 없이 DataTable 편집만으로 끝난다.

## Combat을 별도 도메인으로 분리한 이유

캐릭터의 "이동/상태" 로직(Drop, 카메라, 입력)과 "전투" 로직(데미지, 체력, 사망, 히트리액션)은 변경 빈도와 관심사가 다르다. 낙하산 연출을 손보는 작업과 히트리액션 판정을 고치는 작업이 서로 다른 파일에서만 일어나게 하고 싶었다.

그래서 `UHealthComponent`(Combat)는 소유자(`ADropCharacter`)의 구체 타입을 전혀 모른다 — `ApplyDamage()`를 호출받아 판정만 하고, 결과는 델리게이트(`OnHealthChanged`, `OnDeath`, `OnHit`)로만 알린다. `ADropCharacter`는 이 델리게이트를 구독해 몽타주 재생·래그돌 전환·넉백 같은 "연출"만 담당하고, 사망 시 조준 강제 해제·입력 차단·페이드아웃·HUD 숨김은 컨트롤러가 처리한다.

- **Combat = 판정 로직** — 데미지 계산, HP 복제, 사망 여부. 소유자 비의존, 재사용 가능.
- **Character = 연출/입력** — 몽타주, 넉백(`LaunchCharacter`), 카메라. Combat이 broadcast한 이벤트를 소비.

이 컴포지션 구조 덕분에 나중에 다른 폰(AI, 차량, 파괴 가능 오브젝트)에도 `UHealthComponent`를 그대로 붙일 수 있다.

## Weapon — 무기 액터와 슬롯 인벤토리

- `AWeaponBase` — 총기/근접/투척(`EWeaponType`) 공용 베이스. 스탯(탄창, RPM, 사거리, 데미지)·FX·아이콘은 `EditDefaultsOnly`로 노출해 `BP_AR4`, `BP_KA47` 같은 자식 BP에서 값만 바꿔 무기를 만든다. 사격 판정은 서버에서 카메라 기준 트레이스, 총구 섬광/트레이서는 `NetMulticast`로 연출만. 단발/연사(`EFireMode`) 전환도 서버 권위.
- `UWeaponInventoryComponent` — 5슬롯(주무기1·2 / 보조 / 근접 / 투척) 관리. 슬롯 클래스·인스턴스·장착 무기를 복제하고, 변경 시 `OnInventoryChanged`로 UI에 알린다. 드래그&드롭 슬롯 교체(`SwapWeaponSlots`)도 Server RPC로 처리.

## Interaction — 인터페이스 기반 상호작용

`IInteractableInterface`(`Interact`, `GetInteractionPromptText`)만 구현하면 무엇이든 상호작용 대상이 된다. 캐릭터는 오버랩으로 주변 후보를 모아 가장 적합한 대상을 고르고, 프롬프트 텍스트를 델리게이트로 HUD에 넘긴 뒤, 입력 시 `ServerInteract` RPC로 서버에서 실행한다. 캐릭터는 대상의 구체 타입을 모른다.

- `AItemPickupActor` — 단일 무기 픽업. 허용 슬롯(`AllowedSlots`) 중 빈 곳에 장착.
- `ALootContainer` — 다중 아이템 루팅 상자. `LootItems`를 복제하고, 다 털리면 스스로 정리. 컨트롤러의 `OpenLootScreen`으로 인벤토리 UI와 연결.

## UI

위젯 로직은 가능한 한 C++(컨트롤러/컴포넌트)에서 데이터로 만들어 넘기고, UMG는 표시만 담당한다.

- `UDropHUDWidget` — 체력·탄약·아이템 슬롯·나침반·미니맵 HUD. 게임플레이/내비게이션 그룹 단위로 표시 토글(비행기 탑승·인벤토리·사망 시), 중앙 알림
- `UMapGridWidget` — 월드맵 격자 그리기(`UWidget` 직접 상속)
- `UDropMapLabelWidget` — 월드맵 지역 이름 라벨
- 인벤토리(BP) — 배틀그라운드식 3단 레이아웃(루팅 / 장비·캐릭터 프리뷰 / 무기·부착물 슬롯), 슬롯 드래그&드롭, SceneCapture2D 기반 캐릭터 3D 실시간 프리뷰

## 설계 원칙 (포트폴리오 관점 요약)

1. **도메인별 폴더 분리** — 향후 플러그인화의 전제조건. `Combat/`을 통째로 별도 UE 플러그인 모듈로 옮겨도, `HealthComponent`가 소유자 타입을 모르는 구조라 이식 비용이 낮다.
2. **서버 권위 + Replicated/델리게이트 기반 이벤트** — `HealthComponent`의 `Health`/`bDead`, `AirPlane`의 `FlightAlpha`, `WeaponInventoryComponent`의 슬롯, `LootContainer`의 `LootItems`처럼 서버가 진실을 갖고 `ReplicatedUsing`으로 각 클라가 로컬 보간·연출·UI 갱신만 담당하는 패턴을 일관되게 사용.
3. **컴포넌트 + 인터페이스로 결합도 낮추기** — 체력(`UHealthComponent`)·무기(`UWeaponInventoryComponent`)는 컴포넌트로, 상호작용은 `IInteractableInterface`로 분리해 캐릭터가 구체 타입에 의존하지 않게.
4. **Subsystem 활용** — `UProjectilePoolSubsystem`(WorldSubsystem)으로 투사체 풀링을 전역 서비스처럼 제공해 액터 간 직접 참조를 줄임.
5. **BlueprintFunctionLibrary로 순수 로직 분리** — `UDropMapLibrary`처럼 상태 없는 계산은 라이브러리로 빼서 C++/블루프린트 양쪽에서 검증 가능하게.
6. **데이터 주도** — 무기 스탯은 자식 BP 기본값, 지역 라벨은 DataTable로 관리해 재컴파일 없이 콘텐츠 추가.

## 개발 로그

작업 중 겪은 이슈와 해결 과정은 [`docs/`](docs/)에 날짜별로 정리.

- 레벨 트래블 최적화 / 심리스 트래블, 맵 다이어트
- Bundled PSO 캐시 설정
- 총구 섬광 VFX
- 멀티플레이 리플리케이션 교훈

## 향후 확장 방향

- `Combat/`, `Drop/`, `Interaction/`을 별도 Runtime 플러그인 모듈로 승격 → 다른 프로젝트에서도 재사용 가능한 게임플레이 킷으로.
- `DropTypes.h`의 enum들을 GameplayTag로 옮겨 데이터 기반 확장 (새 상태 추가 시 재컴파일 불필요).
- 무기/투사체 스탯을 자식 BP 기본값에서 DataAsset/DataTable로 이전해 한 곳에서 밸런싱.
- 루팅 아이템을 무기 외(탄약·회복·방어구·부착물)로 확장 — 현재 `FLootItemEntry`는 무기 클래스 전용.
- 자기장(블루존)·순위/승리 판정 등 매치 진행 로직.
