// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_DefaultFocus.h"
#include "BTService_GameplayFocus.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYABILITYSYSTEM_API UBTService_GameplayFocus : public UBTService_DefaultFocus
{
	GENERATED_BODY()
	
protected:
	UBTService_GameplayFocus(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
