# 시연 준비 인수인계 (2026-10-08 작업 → 2026-10-09 과제)

다른 PC에서 이어서 작업하기 위한 정리. 10/08까지의 코드는 전부 `main`에 푸시됨.

---

## 1. 10/09 과제

시연영상은 **클라이언트 시점**으로 촬영. 맵이 넓어 남은 봇·플레이어를 못 찾고, 빨리 죽어서 보여줄 게 적음 → **플레이어에게 유리한 세팅**으로 간다.

### 1-1. 자기장 (최우선)
- 목표: 원이 **오두막 마을(무기 배치한 호숫가 마을)** 쪽으로 좁혀져서 남은 봇/플레이어가 모이게.
- 마을 대략 좌표(서버 로그 기준): `X ≈ 114,000 ~ 119,000`, `Y ≈ -159,000 ~ -170,000`, `Z ≈ 5,000`
  - 실내 조명 `IL_` 접두사 액터를 깔아둔 그 마을. 정확한 중심은 에디터에서 확인.
- 필요한 것
  - 서버 권한으로 원 중심/반경 수축 (GameState에 복제 → 클라 표시)
  - 원 밖 지속 데미지 (서버에서 `HealthComponent::ApplyDamage`)
  - HUD/미니맵/월드맵에 원 표시, 다음 원 표시(선택)
  - 봇도 원 밖이면 데미지 받게 (봇도 `ADropCharacter`라 같은 경로)
- 붙일 곳: `ADropGameMode`(서버 로직), `ADropGameState`(복제 값, 생존자 수 이미 있음)

### 1-2. 회복템 증량 + 회복량 상향
- 에너지드링크 / 구급상자 / 붕대를 지금보다 많이 배치 (마을 주변 위주).
- 회복량 상향 (아이템 DataTable 값 조정).
- ⚠️ 스크립트로 배치/정리할 때는 **내가 붙인 라벨 접두사로만** 지울 것. 클래스 전체 일괄 삭제 금지 (과거에 총 픽업 17개 날린 사고 있음).

### 1-3. 이미 해둔 시연용 세팅
- 봇 최대 5마리: `ADropGameMode::MaxBots = 5` (`BP_DropGameMode`에서 조정, `-1` = 제한 없음). 시작 시 초과 봇은 무기째 제거, 맵 자체는 수정 안 함.

---

## 2. 10/08에 끝낸 것 (커밋 기준)

| 항목 | 핵심 |
|---|---|
| 비행기 클라 좌석 이탈/지터 | `BoardedPlane` 복제 + 클라 로컬 좌석 부착, 탑승 중 CMC 틱 정지, 비행기 시간 `SmoothedServerTime` |
| 급강하 가속 | `ServerSetFastFalling` RPC (원래 클라 로컬 bool이라 서버가 되돌림) |
| 낙하 중 고도 리셋 | 뛰어내릴 때 클라도 비행기에서 Detach (`ApplyDropState`) |
| 생존자/킬 HUD + 결과 화면 | `ADropGameState`(Alive/Total), `ADropGameMode` 생존자 관리, `ClientShowMatchResult` RPC, `WBP_GameOver` / `WBP_Victory`(30초 자동 복귀) |
| AWP 스코프 레티클-탄도 불일치 | 스코프 켜면 ScopeCapture를 FollowCamera에 부착 |
| **큰 맵에서 클라 총알이 안 맞음** | 서버가 화면 밖 캐릭터 뼈를 갱신 안 해서 총구가 땅속(4.5m 아래)에 생성 → 서버에서 `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones` |
| 맞다 안 맞다 함 | 조준 트레이스(Visibility)가 캐릭터를 통과해 조준점이 봇 뒤 바닥이 됨 → Pawn 오브젝트 트레이스 병행해서 가까운 쪽 사용 |
| 총알 풀 | 사거리 초과 시 `Destroy()` → `DeactivateBullet()` |

### 아직 인게임 확인 안 된 것
- 조준선 수정 후 봇 머리/가장자리 정조준 명중 여부
- 봇 5마리 제한 (2인 기준 생존자 7로 나오는지)
- 결과 화면: 클라 사망 시 게임오버/호스트 승리, 메인 메뉴 버튼, 30초 자동 복귀

---

## 3. 멀티 테스트 메모

- 2PC는 Tailscale. 스프링(매칭) 서버 PC `100.68.226.127:8080`.
- 패키징 빌드는 `Packaged/Windows/Play_Tailscale.bat`로 실행 (BaseUrl·AdvertiseIp를 인자로 넘김). **패키징하면 bat이 지워질 수 있으니 백업.**
- 패키징 커맨드 (에디터 끄고):
  ```
  RunUAT.bat BuildCookRun -project=<경로>\BattlegroundClone.uproject -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory=<경로>\Packaged -utf8output
  ```
  약 3분.
- 판정 로그는 **호스트(서버) PC**에만 남음: `Packaged\Windows\BattlegroundClone\Saved\Logs\BattlegroundClone.log`
- 큰 맵(`BattleRoyale_Map_a_WP`) PIE 멀티는 로딩이 길어 불안정 → 빠른 확인은 `TestMap`, 큰 맵 확인은 패키징 2PC.
- 한 PC에서 창 2개 띄우면 프레임/스트리밍이 망가져서 네트워크 문제처럼 보임 → 동기화 판단은 2PC로.

## 4. 작업 시 주의
- 서버 판정이 뼈/소켓 위치(총구, 헤드샷)에 의존하면 **화면 밖 뼈 미갱신**부터 의심.
- MCP로 BP 기본값(CDO)을 바꾼 뒤에는 **반드시 BP 컴파일 → 저장**. 안 하면 런타임 인스턴스에 반영 안 됨(결과 화면 안 뜨던 원인).
- 스코프/저격 관련 기능은 **AWP 전용**으로만. 다른 총 동작 건드리지 말 것.
