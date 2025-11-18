// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryAbilityCost_TagStack.h"
#include "Ability/InventoryGameplayAbility_FromEquipment.h"
#include "Inventory/InventoryItemInstance.h"

bool UInventoryAbilityCost_TagStack::CheckCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (const UInventoryGameplayAbility_FromEquipment* EquipmentAbility = Cast<const UInventoryGameplayAbility_FromEquipment>(Ability))
	{
		if (UInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem())
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumStacks = FMath::TruncToInt(NumStacksReal);
			const int32 NumStacksAvailable = ItemInstance->GetStatTagStackCount(Tag);
			const bool bCanApplyCost = NumStacksAvailable >= NumStacks;

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

void UInventoryAbilityCost_TagStack::ApplyCost(const UInventoryGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo->IsNetAuthority())
	{
		if (const UInventoryGameplayAbility_FromEquipment* EquipmentAbility = Cast<const UInventoryGameplayAbility_FromEquipment>(Ability))
		{
			if (UInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem())
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

				ItemInstance->RemoveStatTagStack(Tag, NumStacks);
			}
		}
	}
}
