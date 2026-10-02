# 🎮 BattlegroundClone

<img width="1916" height="1025" alt="ReadMe" src="https://github.com/user-attachments/assets/089046f1-45ed-4d80-8c43-cecf686312ce" />

---

<p>
  <img src="https://img.shields.io/badge/Unreal%20Engine-5.8-0E1128?logo=unrealengine&logoColor=white" alt="Unreal Engine 5.8" />
  <img src="https://img.shields.io/badge/Genre-Battle%20Royale-F9A825" alt="Battle Royale" />
  <img src="https://img.shields.io/badge/Multiplayer-Online-2E8B57" alt="Multiplayer" />
  <img src="https://img.shields.io/badge/Status-In%20Development-0366D6" alt="Status: In Development" />
</p>

배틀그라운드 고유의 분위기를 담아낸 멀티플레이 슈팅게임 프로젝트입니다.

---

## 주요 게임 특징

- **비행기 탑승 및 낙하 시퀀스**
  - 상공을 가르는 비행기에서 자유낙하와 낙하산 개방을 거쳐 전장에 착지하는 배틀로얄의 상징적인 시작을 구현했습니다.
- **파밍 및 무기 인벤토리**
  - 필드 곳곳의 상자와 아이템을 뒤져 주무기와 보조무기를 획득하고 슬롯에 장착하여 실시간으로 전황에 맞는 장비를 갖출 수 있습니다.
- **멀티플레이 생존**
  - 멀티플레이어 환경 속에서 동료들과 항로를 공유하고, 긴장감 넘치는 사격과 전략적인 생존 플레이를 함께 즐길 수 있습니다.

---

## 🗺️ Architecture

```mermaid
flowchart TD
    %% 스타일 정의
    classDef ui fill:#2A2E43,stroke:#6C7A9C,stroke-width:2px,color:#fff;
    classDef core fill:#1E2235,stroke:#4A90E2,stroke-width:2px,color:#fff;
    classDef domain fill:#16192B,stroke:#555,stroke-width:1px,color:#ccc;

    %% 노드 배치
    UI["UI<br/>HUD / MiniMap / Widgets"]:::ui
    Core["⚙️Core<br/>GameMode / Controller / State"]:::core
    
    Drop["Drop<br/>AAirPlane"]:::domain
    Character["👤Character<br/>ADropCharacter"]:::core
    
    Combat["Combat<br/>UHealthComponent"]:::domain
    Weapon["Weapon<br/>WeaponBase / Inventory"]:::domain
    Interaction["Interaction<br/>Interface / Pickup"]:::domain

    %% 연결 관계
    UI -->|데이터 공급 / 입력| Core
    Core -->|GameMode가 스폰/관리| Drop
    Core -->|Enum / Possess| Character
    
    Character -->|IInteractableInterface 호환| Interaction
    Character -->|owns HealthComponent| Combat
    Character -->|owns InventoryComponent| Weapon

---

## 🛠️ 핵심 기술 요약

> * **네트워크 동기화**: 서버 권위(Server Authority) 기반의 멀티플레이어 리플리케이션 적용
> * **모듈식 구조**: 낙하, 전투, 인벤토리 도메인을 컴포넌트 단위로 분리하여 독립적인 관리 및 확장 구현
> * **유연한 프레임워크**: 컴포넌트와 인터페이스 중심의 설계로 높은 재사용성과 확장성 확보
