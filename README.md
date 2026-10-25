# 🎮 BattlegroundClone

![Uploading image.png…]()

[![BattlegroundClone Gameplay](https://img.youtube.com/vi/a0FPOMAMtws/maxresdefault.jpg)](https://www.youtube.com/watch?v=a0FPOMAMtws)

[![Watch on YouTube](https://img.shields.io/badge/YouTube-Watch%20Gameplay-FF0000?logo=youtube&logoColor=white)](https://youtu.be/a0FPOMAMtws)

---

<p>
  <img src="https://img.shields.io/badge/Unreal%20Engine-5.8-0E1128?logo=unrealengine&logoColor=white" alt="Unreal Engine 5.8" />
  <img src="https://img.shields.io/badge/Genre-Battle%20Royale-F9A825" alt="Battle Royale" />
  <img src="https://img.shields.io/badge/Multiplayer-Online-2E8B57" alt="Multiplayer" />
  <img src="https://img.shields.io/badge/Status-In%20Development-0366D6" alt="Status: In Development" />
</p>

배틀그라운드 고유의 분위기를 담아낸 멀티플레이 슈팅게임 프로젝트입니다.

---

## System Architecture

<img width="5140" height="3302" alt="Game_Network_Architecture (1)" src="https://github.com/user-attachments/assets/53deff77-4fec-42a2-8cc0-e7291717ce03" />

1. 모든 PC는 **Tailscale VPN** 망 안에서 연결됩니다.
2. 방을 만든 호스트가 **리슨 서버**가 되어, 자기 IP를 **세션 서버(Spring Boot)** 에 HTTP로 등록합니다.
3. 클라이언트는 세션 서버에서 방 목록을 받아온 뒤, 선택한 호스트 IP로 직접 접속합니다.
4. 이후 게임 동기화는 **언리얼 리플리케이션(UDP 7777)** 으로 처리됩니다.

| 구분 | 구조 | 접속 범위 |
|---|---|---|
| Tailscale 이전 | LAN + 리슨 서버 | 같은 공유기 안에서만 |
| Tailscale 이후 | 가상 LAN(Tailscale) + 리슨 서버 | 서로 다른 공유기 간에도 가능 |

> Tailscale로 같은 공유기에서만 접속되던 LAN의 한계를 극복했고, 호스트 IP 자동 조회는 Spring Boot 세션 서버가 담당합니다.
> 다만 이것도 가상 LAN이므로 참가자 전원이 Tailscale에 가입해 있어야 하며, 일반 인터넷 사용자가 바로 접속하는 구조는 아닙니다.

### 확장 계획

- **데디케이티드 서버 전환**: 리슨 서버를 언리얼 데디케이티드 서버로 분리해 AWS EC2에 배포하면, 호스트 PC 없이 공인 IP로 접속할 수 있습니다.
- **세션 서버 클라우드 이전**: Spring Boot 세션 서버도 EC2에 배포해 특정 PC에 의존하지 않고 방 목록을 상시 제공합니다.
- **보류 사유**: 두 구성 모두 Tailscale 없이 일반 인터넷 접속이 가능해지지만, 현재는 운영 비용을 고려해 보류했습니다.

---


## 주요 게임 특징

- **비행기 탑승 및 낙하 시퀀스**
  - 상공을 가르는 비행기에서 자유낙하와 낙하산 개방을 거쳐 전장에 착지하는 배틀로얄의 상징적인 시작을 구현했습니다.
- **파밍 및 무기 인벤토리**
  - 필드 곳곳의 상자와 아이템을 뒤져 주무기와 보조무기를 획득하고 슬롯에 장착하여 실시간으로 전황에 맞는 장비를 갖출 수 있습니다.
- **멀티플레이 생존**
  - 멀티플레이어 환경 속에서 유저와 항로를 공유하며 전략적인 FPS 생존플레이가 가능합니다.

---

## 🛠️ 핵심 기술 요약

> * **네트워크 동기화**: 서버 권위(Server Authority) 기반의 멀티플레이어 리플리케이션 적용
> * **모듈식 구조**: 낙하, 전투, 인벤토리 도메인을 컴포넌트 단위로 분리하여 독립적인 관리 및 확장 구현
> * **유연한 프레임워크**: 컴포넌트와 인터페이스 중심의 설계로 높은 재사용성과 확장성 확보


---
