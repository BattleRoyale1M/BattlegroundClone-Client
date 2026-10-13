#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DropMovementComponent.generated.h"

class ADropCharacter;

UCLASS()
class BATTLEGROUNDCLONE_API UDropMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	enum ECustomDropMode : uint8
	{
		CMOVE_Freefall = 0,
		CMOVE_Parachute = 1,
	};

	bool bWantsFastFall = false;
	bool bWantsDeploy = false;

	bool IsInCustomMode(uint8 Mode) const
	{
		return MovementMode == MOVE_Custom && CustomMovementMode == Mode;
	}

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

private:
	ADropCharacter* GetDropOwner() const;
};

/*
네트워크 예측을 위해 프레임별 캐릭터의 이동 상태와 입력값을 저장
*/
class FSavedMove_Drop : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	uint8 bSavedWantsFastFall : 1;
	uint8 bSavedWantsDeploy : 1;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;
};

/*
클라이언트 측에서 커스텀 SavedMove객체들을 생성(Allocate)하고 관리 
*/
class FNetworkPredictionData_Client_Drop : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	explicit FNetworkPredictionData_Client_Drop(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement) {}

	virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FSavedMove_Drop()); }
};
