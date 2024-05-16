﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LinkActorData.generated.h"

USTRUCT(BlueprintType)
struct FLinkedActorStruct
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FString Name;
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ActorClass;
	UPROPERTY(EditAnywhere)
	int32 MaxInstances = 1;
};

/**
 * 
 */
UCLASS()
class NIGHTSKYENGINE_API ULinkedActorData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FLinkedActorStruct> LinkedActorStructs;
};
