// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AirPlane.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class BATTLEGROUNDCLONE_API AAirPlane : public AActor
{
	GENERATED_BODY()
	
public:	
	AAirPlane();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	
	/*
	 회전/이동의 기준 루트 (진행 방향)
	*/
	UPROPERTY(VisibleAnywhere, Category = "Plane")
	TObjectPtr<USceneComponent> RootScene;
	
	/*
	 동체 및 루트
	*/
	UPROPERTY(VisibleAnywhere, Category = "Plane")
	TObjectPtr<UStaticMeshComponent> BodyMesh;
	
	/*
	 프로펠러 BodyMesh에 부착하여 매틱 회전
	*/
	UPROPERTY(VisibleAnywhere, Category = "Plane")
	TObjectPtr<UStaticMeshComponent> Propeller;
	
	/*
	비행 시작 시점(월드)
	*/
	UPROPERTY(EditAnywhere, Category = "Flight")
	FVector StartPoint = FVector::ZeroVector;

	
	/* 
	 메시 기수 축 보정 (기수가 +X와 다를 때, 기본 Yaw 180) 
	 */
	UPROPERTY(EditAnywhere, Category = "Plane")
	FRotator MeshRotationOffset = FRotator(0.f, 180.f, 0.f);
	
	/*
	비행 종료 시점(월드)
	*/
	UPROPERTY(EditAnywhere, Category = "Flight")
	FVector EndPoint = FVector(50000.f, 0.f, 0.f);
	
	/*
	Start -> End 까지 걸리는 시간(초).
	*/
	UPROPERTY(EditAnywhere, Category = "Flight", meta = (ClampMin = "1.0"))
	float FlightDuration = 40.f;
	
	/*
	 프로팰러 회전속도
	 */
	UPROPERTY(EditAnywhere, Category = "Flight")
	float PropellerDegPerSec = 2000.f;
	
	/*
	도착하면 스스로 제거 
	*/
	UPROPERTY(EditAnywhere, Category = "Flight")
	bool bDestroyOnArrival = true;

private:
	float Elapsed = 0.f;
};
