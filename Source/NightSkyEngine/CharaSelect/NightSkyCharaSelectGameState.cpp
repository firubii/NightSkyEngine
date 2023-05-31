﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "NightSkyCharaSelectGameState.h"
#include "NightSkyEngine/Battle/Actors/NightSkyGameState.h"
#include "NightSkyEngine/Miscellaneous/NightSkyGameInstance.h"


// Sets default values
ANightSkyCharaSelectGameState::ANightSkyCharaSelectGameState()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	P1Positions.Add(FVector(-150, -100, 0));
	P1Positions.Add(FVector(-220, -50, 0));
	P1Positions.Add(FVector(-300, 0, 0));
	
	P2Positions.Add(FVector(150, -100, 0));
	P2Positions.Add(FVector(220, -50, 0));
	P2Positions.Add(FVector(300, 0, 0));
}

// Called when the game starts or when spawned
void ANightSkyCharaSelectGameState::BeginPlay()
{
	Super::BeginPlay();
	GameInstance = Cast<UNightSkyGameInstance>(GetGameInstance());
}

// Called every frame
void ANightSkyCharaSelectGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (const auto Chara : P1Charas)
		Chara->UpdateNotBattle();
	for (const auto Chara : P2Charas)
		Chara->UpdateNotBattle();
}

void ANightSkyCharaSelectGameState::AddPlayerObject(TSubclassOf<APlayerObject> InClass, bool IsP1)
{
	if (IsP1)
	{
		if (P1Charas.Num() >= MaxPlayerObjects / 2)
			return;
		P1Charas.Add(GetWorld()->SpawnActor<APlayerObject>(InClass));
		P1Charas.Last()->InitPlayer();
		P1Charas.Last()->SetActorLocation(P1Positions[P1Charas.Num() - 1]);
		GameInstance->PlayerList[P1Charas.Num() - 1] = InClass;
	}
	else
	{
		if (P2Charas.Num() >= MaxPlayerObjects / 2)
			return;
		P2Charas.Add(GetWorld()->SpawnActor<APlayerObject>(InClass));
		P2Charas.Last()->InitPlayer();
		P2Charas.Last()->SetActorLocation(P2Positions[P2Charas.Num() - 1]);
		P2Charas.Last()->SetActorScale3D(FVector(-1, 1, 1));
		GameInstance->PlayerList[P2Charas.Num() - 1 + MaxPlayerObjects / 2] = InClass;
	}
}

void ANightSkyCharaSelectGameState::AddColorIndex(int InColor, bool IsP1)
{
	if (IsP1)
	{
		if (P1Charas.Num() >= MaxPlayerObjects / 2)
			return;
		GameInstance->ColorIndices[P1Charas.Num() - 1] = InColor;
	}
	else
	{
		if (P2Charas.Num() >= MaxPlayerObjects / 2)
			return;
		GameInstance->ColorIndices[P2Charas.Num() - 1 + MaxPlayerObjects / 2] = InColor;
	}
}