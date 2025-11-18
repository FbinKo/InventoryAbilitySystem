// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "InventoryCosmeticComponent.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cosmetic_AnimationStyle);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cosmetic_BodyStyle);

UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryCosmeticComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Animation")
		FGameplayTagContainer AnimationStyleTags;

	FGameplayTagContainer GetCombinedTags(FGameplayTag RequiredPrefix) const;
	
};
