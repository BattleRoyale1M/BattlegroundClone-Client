# Bundled PSO 캐시 - 인프라 세팅 (레코딩은 마일스톤에서)

> 작성일: 2026-09-03 · 엔진: UE 5.8 · 브랜치: main
> 선행 문서: [2026-09-03-seamless-travel-result-and-map-diet.md](2026-09-03-seamless-travel-result-and-map-diet.md)

## 배경 - 왜 하나

쿡 빌드에서 로비→낙하맵 seamless travel은 0.25s까지 내려갔지만, 도착 직후
`LogRHI: Encountered a new graphics PSO ... PSOPrecacheState: Missed` 가 수십 건 찍힘.
= 그 머티리얼/버텍스팩토리/렌더스테이트 조합의 GPU 파이프라인을 **처음 그릴 때 만든다**는 뜻.

두 단계로 잡는다:

1. **런타임 PSO 프리캐싱** (`r.PSOPrecaching` 등, 이미 적용) - 로드 시 예측해서 async 컴파일,
   `ProxyCreationStrategy=1`로 준비 전엔 드로우를 미룸. → 하드 프리즈는 사라지고 pop-in만 남음.
   단 `Missed`가 여전히 뜸 = 예측이 다 커버하진 못함. 드라이버 셰이더 캐시가 식은 머신
   (인천 노트북, 드라이버 업데이트 후, 배포 대상 PC 첫 실행)에선 다시 히칭 가능.
2. **Bundled PSO 캐시** (이 문서) - 실제 플레이로 필요한 PSO 목록을 수집해 빌드에 넣고,
   실행 시 로딩스크린 뒤에서 전부 미리 컴파일. → 어느 머신에서도 게임플레이 중 0 히칭.

## 지금 상태 (인프라만 세팅 완료)

| 항목 | 상태 |
|---|---|
| `bShareMaterialShaderCode` | 엔진 기본값 True (BaseGame.ini) - 별도 설정 불필요 |
| `r.ShaderPipelineCache.Enabled=1` | `Config/DefaultEngine.ini [SystemSettings]`에 추가 |
| `r.ShaderPipelineCache.StartupMode=2` | 2=Background(저우선, 로딩스크린 없을 때 안전). 로딩위젯 생기면 1=Fast로 |
| `Build/Windows/PipelineCaches/` | 폴더 생성 + README. 여기에 `.stable.upipelinecache` 커밋 |
| `Tools/PSOCache/Record-PSOs.ps1` | 패키지 빌드를 `-logPSO`로 실행 |
| `Tools/PSOCache/Build-PSOCache.ps1` | 레코딩 + stable keys → `.stable.upipelinecache` Expand |
| `.gitignore` | `/Package/` 추가 (레코딩은 `Saved/`라 이미 무시됨) |

**레코딩 패스는 아직 안 함.** 콘텐츠(스타터킷 + 비행기/캐릭터)가 유동적이라, 지금 수집해도
머티리얼/메시 바뀌면 stale 됨(깨지진 않고 런타임 프리캐시로 폴백). 첫 플레이테스트나
배포 빌드 직전, 콘텐츠가 한 번 얼었을 때 아래 루프를 돌린다.

## 레코딩 → 빌드 → 번들 루프 (마일스톤에서)

### 1. 쿡 + 패키지 (stable keys 생성)
```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
  -project="<repo>\BattlegroundClone.uproject" `
  -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -iostore -compressed -prereqs `
  -archive -archivedirectory="<repo>\Package"
```
`bShareMaterialShaderCode`가 켜져 있으므로 쿡이 `Saved/Cooked/Windows/.../Metadata/`에
`*.scl.csv` (shader stable keys)를 남긴다.

### 2. 레코딩
```powershell
pwsh Tools/PSOCache/Record-PSOs.ps1
```
- **에디터 아님. 쿡된 실행파일.**
- 모든 맵 / 무기·스코프 / 차량 / VFX·데칼 / 메뉴·HUD / 낮·밤을 한 번씩 그린다. 빠뜨린 콘텐츠 = 배포 빌드에 남는 히칭.
- 인게임 메뉴로 정상 종료 (Alt-F4 금지 - 레코딩 유실).
- 결과: `Saved/CollectedPSOs/PipelineCaches/Windows/*.rec.upipelinecache`
- 커버리지를 넓히려면 여러 세션 반복 - `.rec` 파일이 쌓이고 Expand가 다 합친다.

### 3. stable 캐시 빌드
```powershell
pwsh Tools/PSOCache/Build-PSOCache.ps1
```
`ShaderPipelineCacheTools Expand`로 `.rec` + `.scl.csv` → `Build/Windows/PipelineCaches/BattlegroundClone_SF_D3D_SM6.stable.upipelinecache`.
(엔진 빌드별로 인자 순서가 다르면 `UnrealEditor-Cmd <proj> -run=ShaderPipelineCacheTools help`로 확인.)

### 4. 재패키지 + 검증
1번을 다시 실행. UAT가 `Build/Windows/PipelineCaches/*.stable.upipelinecache`를 감지 →
바이너리 `*.upipelinecache`로 변환 → 팩에 `Content/PipelineCaches/Windows/`로 스테이징.

검증 (패키지 게임 실행 후 로그):
- `LogShaderPipelineCache: Opened pipeline cache ... N tasks` - 번들 캐시 로드됨
- `LogShaderPipelineCache: Completed ... 0 tasks remaining` - 게임플레이 진입 전 완료
- `LogRHI: Encountered a new graphics PSO ... Missed` 개수가 이전 대비 급감 (이상적으로 0)
- `LogPSOHitching` 요약줄 없음

### 5. stable 캐시 커밋
`Build/Windows/PipelineCaches/*.stable.upipelinecache`는 **커밋한다** (배포 산출물).
`Saved/CollectedPSOs/`의 `.rec`만 버린다.

## StartupMode 전환 (로딩위젯 생긴 뒤)

`r.ShaderPipelineCache.StartupMode` 값 (엔진 소스 기준):
`0` = 일시정지(게임 코드가 `ResumeBatching()` 호출해야 시작),
`1` = Fast(공격적 컴파일 - 로딩스크린 뒤에서 씀),
`2` = Background(저우선 - 로딩스크린 없을 때 기본). `3`은 없음.

`Config/DefaultEngine.ini [SystemSettings]`:
```ini
r.ShaderPipelineCache.StartupMode=1   ; 2 -> 1
```
Fast로 두고, drop 로딩 위젯 종료 조건에
`FShaderPipelineCache::NumPrecompilesRemaining() == 0` 을 AND로 건다. 컴파일 버스트가
위젯 뒤에서 끝나도록. (또는 StartupMode=0 + 위젯 뜰 때 `SetBatchMode(Fast)`.)
참고: [2026-09-03-seamless-travel-result-and-map-diet.md](2026-09-03-seamless-travel-result-and-map-diet.md) 5절 - 로딩 연출

## 데디케이트 서버 / 매치메이킹 방향과의 관계

- **데디케이트 서버 빌드는 렌더링 안 함 → PSO 캐시 무관.** 클라이언트 빌드에만 적용된다.
- 웹 매치메이킹 서버도 PSO와 무관.
- 정작 "지금 해두면 나중에 편한" 서버 관련 항목은 **빌드 타겟 분리**
  (`BattlegroundCloneClient.Target.cs` / `BattlegroundCloneServer.Target.cs`).
  현재는 단일 `Game` 타겟 추정. 매치메이킹 붙이기 전에 분리하는 게 리트로핏보다 훨씬 싸다.
  → 별도 작업으로 트래킹.

## 관련 파일

| 파일 | 내용 |
|---|---|
| `Config/DefaultEngine.ini` | `[SystemSettings]` - PSO 프리캐시 + `r.ShaderPipelineCache.*` |
| `Build/Windows/PipelineCaches/` | `.stable.upipelinecache` 배치 (커밋) |
| `Tools/PSOCache/Record-PSOs.ps1` | 레코딩 실행 |
| `Tools/PSOCache/Build-PSOCache.ps1` | Expand → stable 캐시 |
