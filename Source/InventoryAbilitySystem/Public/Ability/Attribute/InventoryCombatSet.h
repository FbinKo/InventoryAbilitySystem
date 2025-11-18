// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InventoryAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "InventoryCombatSet.generated.h"

UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryCombatSet : public UInventoryAttributeSet
{
	GENERATED_BODY()
	
public:

	UInventoryCombatSet();

	ATTRIBUTE_ACCESSORS(UInventoryCombatSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UInventoryCombatSet, BaseHeal);
	ATTRIBUTE_ACCESSORS(UInventoryCombatSet, ArmorPenetration);
	ATTRIBUTE_ACCESSORS(UInventoryCombatSet, EnergyPenetration);

protected:

	UFUNCTION()
		void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_BaseHeal(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_EnergyPenetration(const FGameplayAttributeData& OldValue);

private:

	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "Inventory|Combat", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData BaseDamage;

	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseHeal, Category = "Inventory|Combat", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData BaseHeal;

	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorPenetration, Category = "Inventory|Combat", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData ArmorPenetration;

	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EnergyPenetration, Category = "Inventory|Combat", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData EnergyPenetration;
};
