// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryGameplayAbility_FromEquipment.h"
#include "Equipment/EquipmentInstance.h"
#include "Inventory/InventoryItemInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UInventoryGameplayAbility_FromEquipment::UInventoryGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UEquipmentInstance* UInventoryGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<UEquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

UInventoryItemInstance* UInventoryGameplayAbility_FromEquipment::GetAssociatedItem() const
{
	if (UEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<UInventoryItemInstance>(Equipment->GetInstigator());
	}
	return nullptr;
}


#if WITH_EDITOR
EDataValidationResult UInventoryGameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
	{
		Context.AddError(NSLOCTEXT("Inventory", "EquipmentAbilityMustBeInstanced", "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif