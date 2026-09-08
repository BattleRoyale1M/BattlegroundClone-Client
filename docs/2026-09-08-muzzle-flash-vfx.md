# 머즐플래시 VFX (Niagara) — 2026-09-08

UE 5.8 / `AWeaponBase`. 총구 소켓에 원샷 Niagara 시스템을 붙여 재생.

## 1. 소스 에셋

StarterContent 파티클 묶음(`Downloads/UE4_StarterContent_Particles`)을
`Content/StarterContent/{Particles,Textures}` 로 복사 완료. 내부 참조가
`/Game/StarterContent/...` 라서 이 경로에 있어야 해석됨.

사용 텍스처:

| 텍스처 | 격자 | 용도 |
|---|---|---|
| `T_Explosion_SubUV` | 6x6 (36) | 섬광 플룸 |
| `T_Spark_Core` | 단일 | 스파크 점 |
| `T_Smoke_SubUV` | 8x8 (64) | 잔연 |

## 2. 머티리얼 (스크립트 자동 생성)

`Tools/VFX/build_muzzleflash_vfx.py` 를 **Tools > Execute Python Script** 로 실행하면
`/Game/VFX/MuzzleFlash/` 아래에 생성:

- `M_MuzzleFlash_Core` — Additive / Unlit / TwoSided, SubUV 샘플 × ParticleColor × `EmissiveBoost`(기본 45)
- `M_MuzzleFlash_Spark` — Additive / Unlit, `T_Spark_Core` × ParticleColor × `EmissiveBoost`(기본 60)
- `M_MuzzleFlash_Smoke` — Translucent / Unlit, SubUV × `SmokeTint`(어두운 회색) × ParticleColor
- `NS_MuzzleFlash` — 빈 Niagara System (에미터는 아래에서 구성)

> 스크립트가 노드 이름/enum 에서 에러 내면 알려줘 — 에디터 켜고 RiderLink 연결되면
> `ue_execute_python` 로 직접 돌리면서 고칠 수 있음.

## 3. NS_MuzzleFlash 에미터 구성

`NS_MuzzleFlash` 더블클릭 → 우측 Timeline 에서 **+ Emitter** → **Empty** 3개 추가.
전부 **버스트 1회, 루프 없음** 이 핵심 (머즐플래시는 순간).

System 설정: **System State > Loop Behavior = Once**, Age Mode = None.
모든 에미터: **Emitter State > Loop Behavior = Once**, **Life Cycle Mode = Self**.

### 에미터 A — Flash (섬광)
- **Emitter Update > Spawn Burst Instantaneous**: Spawn Count `1`, Spawn Time `0`
- **Particle Spawn**
  - Initialize Particle: Lifetime `0.04 ~ 0.06`, Color `(R 20, G 9, B 3, A 1)` (HDR),
    Sprite Size Mode = Uniform, Uniform Sprite Size `18 ~ 35` (cm; 무기 크기 맞게)
  - Sprite Rotation Rate / Init: Rotation `Random -180..180` (매 발 회전 다르게)
- **Particle Update**
  - Scale Sprite Size: 곡선으로 1.0 → 1.6 (짧게 팽창)
  - Color: Alpha 곡선 1 → 0 (수명 끝 페이드)
- **Render**: Sprite Renderer
  - Material = `M_MuzzleFlash_Core`
  - **Sub UV**: Sub Image Size `(6, 6)`
  - Sub UV 모드: Particle Spawn 에 **Sub UV Animation** 모듈 추가 → Playback = `Random`
    (또는 Infinite Loop, Start Frame Range 0..35) — 매 발 다른 프레임
  - Alignment = Unaligned, Facing = Face Camera

### 에미터 B — Sparks (튀는 불똥)
- **Emitter Update > Spawn Burst Instantaneous**: Spawn Count `Random 8..16`, Time `0`
- **Particle Spawn**
  - Initialize Particle: Lifetime `Random 0.05..0.18`, Color `(R 30, G 12, B 3, A 1)`,
    Sprite Size `Random 1.5..4`
  - Add Velocity: Mode = **Cone**, Cone Angle `18`, Velocity `Random 350..900`,
    Cone Axis = +X (총구 전방; 소켓 축 확인)
  - Add Velocity (Random) 약간: `50` 정도로 흩뿌림
- **Particle Update**
  - Gravity Force: `(0, 0, -600)`
  - Drag: `2 ~ 4`
  - Scale Color: Alpha 1 → 0
  - (선택) Sprite Size 곡선으로 꼬리 줄이기
- **Render**: Sprite Renderer, Material = `M_MuzzleFlash_Spark`
  - Alignment = **Velocity Aligned**, Sprite Size 비율로 세로 길쭉하게 (스파크 스트릭 느낌)

### 에미터 C — Smoke (옅은 잔연, 선택)
- **Spawn Burst Instantaneous**: Spawn Count `Random 1..2`, Time `0.01`
- **Particle Spawn**
  - Lifetime `Random 0.35..0.6`, Color `(0.05, 0.05, 0.06, 0.35)`,
    Sprite Size `Random 12..20`
  - Add Velocity: Cone Angle `25`, Velocity `Random 60..140` (전방 약하게)
- **Particle Update**
  - Scale Sprite Size: 1.0 → 2.2 (퍼짐)
  - Curl Noise Force: Strength `120` (연기 뒤틀림)
  - Scale Color: Alpha 곡선 0.35 → 0
- **Render**: Sprite Renderer, Material = `M_MuzzleFlash_Smoke`
  - Sub Image Size `(8, 8)`, Sub UV Animation = Random
  - Face Camera

저장. 미리보기에서 한 번 "펑" 하고 0.2초 내에 끝나면 정상.

## 4. C++ / BP 연동

`WeaponBase` 에 이미 반영됨:

```
Weapon|FX
  MuzzleFlashFX        (UNiagaraSystem*)   <- NS_MuzzleFlash 지정
  FireSound            (USoundBase*)       <- 사격 사운드(선택)
  MuzzleLightIntensity (float, 기본 1500)  <- 총구 섬광 라이트, 0이면 끔
  MuzzleLightColor / MuzzleLightRadius / MuzzleLightFadeTime
```

- `AWeaponBase::Fire()` 가 탄약 차감 후 `PlayFireFX()` 호출.
- `PlayFireFX()` 는 `WeaponMesh` 의 `MuzzleSocketName`("Muzzle") 소켓에
  `SpawnSystemAttached` 로 원샷 재생 + 짧은 PointLight.
- 소켓이 없으면 메시 원점에 붙음 (`GetMuzzleLocation` 폴백과 동일).

### 필수: 무기 메시에 "Muzzle" 소켓
`SM_AR4`, `SM_KA47` 등 각 웨폰 스태틱메시 에디터에서 총열 끝에
소켓 이름 **`Muzzle`** 추가, +X 가 발사 방향을 향하게 회전.
(에미터 B/C 의 Cone Axis 가 +X 기준)

### BP 설정
`BP_WeaponBase`(또는 자식 `BP_AR4` 등) 열어서 **Class Defaults**:
- `Muzzle Flash FX` = `NS_MuzzleFlash`
- (선택) `Fire Sound` 지정
- 라이트 세기/색은 취향껏

## 5. 빌드

`BattlegroundClone.Build.cs` 에 `"Niagara"` 모듈 추가됨 → **에디터 닫고 솔루션 리빌드**
(또는 Live Coding 은 모듈 추가라 불가, 풀 빌드 필요).

## 남은 개선 여지
- 멀티플레이 시 `PlayFireFX` 는 코스메틱이므로 `Multicast` RPC 또는
  `bReplicated` 발사 이벤트에서 호출하도록 분리 필요 (지금은 로컬 전용).
- Niagara SubUV 를 `M_*` 머티리얼의 SubUV 노드 대신 렌더러 Sub UV + Sub Image Index
  방식으로 통일하면 프레임 제어가 쉬움.
- 1인칭/3인칭 분리 시 총구 소켓을 캐릭터 무기 메시(1P) 와 월드 무기(3P) 각각에.
