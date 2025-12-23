// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "InventoryAbilityCost.generated.h"

class UInventoryGameplayAbility;

UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class INVENTORYABILITYSYSTEM_API UInventoryAbilityCost : public UObject
{
	GENERATED_BODY()

protected:
	/** How much of the item to spend (keyed on ability level) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
		FScalableFloat Quantity;

	/** Which tag to send back as a response if this cost cannot be applied */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
		FGameplayTag FailureTag;
	
public:
	UInventoryAbilityCost();

	virtual bool CheckCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
	{
		return true;
	}

	virtual void ApplyCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
	{
	}
};
