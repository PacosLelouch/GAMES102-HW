// Fill out your copyright notice in the Description page of Project Settings.


#include "CGDemoPlayerController3D.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"


ACGDemoPlayerController3D::ACGDemoPlayerController3D(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ACGDemoPlayerController3D::BeginPlay()
{
	Super::BeginPlay();
	APawn* PossessedPawn = GetPawn();
	FVector NewSpawnLocation = FVector(-500, 0, 0);
	if (PossessedPawn) {
		PossessedPawn->SetActorLocation(NewSpawnLocation, false, nullptr, ETeleportType::None);
	}
	else {
		SetSpawnLocation(NewSpawnLocation);
	}
	//UWorld* World = GetWorld();
	//if (!World) {
	//	return;
	//}
}