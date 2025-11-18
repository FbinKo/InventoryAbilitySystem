// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability/InventoryAbilityCost.h"
#include "InventoryAbilityCost_Item.generated.h"

class UInventoryItemDefinition;

UCLASS(meta = (DisplayName = "Item"))
class INVENTORYABILITYSYSTEM_API UInventoryAbilityCost_Item : public UInventoryAbilityCost
{
	GENERATED_BODY()
	
public:
	//~UInventoryAbilityCost interface
	virtual bool CheckCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	//~End of UInventoryAbilityCost interface

protected:
	/** Which item to consume */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AbilityCost)
		TSubclassOf<UInventoryItemDefinition> ItemDefinition;
};
