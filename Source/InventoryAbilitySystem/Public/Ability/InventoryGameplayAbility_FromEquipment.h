// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InventoryGameplayAbility.h"
#include "InventoryGameplayAbility_FromEquipment.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryGameplayAbility_FromEquipment : public UInventoryGameplayAbility
{
	GENERATED_BODY()
	
public:
	UInventoryGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Ability")
		class UEquipmentInstance* GetAssociatedEquipment() const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
		class UInventoryItemInstance* GetAssociatedItem() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
