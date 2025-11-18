// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryAbilityCost.h"
#include "InventoryAbilityCost_TagStack.generated.h"

/**
 * 
 */
UCLASS(meta=(DisplayName="Tag Stack"))
class INVENTORYABILITYSYSTEM_API UInventoryAbilityCost_TagStack : public UInventoryAbilityCost
{
	GENERATED_BODY()
public:
	//~UInventoryAbilityCost interface
	virtual bool CheckCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	//~End of UInventoryAbilityCost interface

protected:
	/** Which tag to spend some of */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
		FGameplayTag Tag;
};
