// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DropCharacter.h"

// Sets default values
ADropCharacter::ADropCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADropCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADropCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ADropCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

