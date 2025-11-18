// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryAbilityCost_Item.h"
#include "Ability/InventoryGameplayAbility.h"
#include "Inventory/InventoryComponent.h"

bool UInventoryAbilityCost_Item::CheckCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (AController* PC = Ability->GetActorInfo().PlayerController.Get())
	{
		if (UInventoryComponent* InventoryComponent = PC->GetComponentByClass<UInventoryComponent>())
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumItemsReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumItems = FMath::TruncToInt(NumItemsReal);
			const int32 NumItemsAvailable = InventoryComponent->GetStackCountDefinition(ItemDefinition);
			const bool bCanApplyCost = NumItemsAvailable >= NumItems;

			// Inform other abilities why this cost cannot be applied
			if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid())
			{
				OptionalRelevantTags->AddTag(FailureTag);
			}
			return bCanApplyCost;
		}
	}
	return false;
}

void UInventoryAbilityCost_Item::ApplyCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo->IsNetAuthority())
	{
		if (AController* PC = Ability->GetActorInfo().PlayerController.Get())
		{
			if (UInventoryComponent* InventoryComponent = PC->GetComponentByClass<UInventoryComponent>())
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumItemsReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumItems = FMath::TruncToInt(NumItemsReal);
				InventoryComponent->RemoveItemDefinition(ItemDefinition, NumItems);
			}
		}
	}
}
