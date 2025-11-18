// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InventoryAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "InventoryMovementSet.generated.h"

UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryMovementSet : public UInventoryAttributeSet
{
	GENERATED_BODY()
	
public:
	UInventoryMovementSet();

	ATTRIBUTE_ACCESSORS(UInventoryMovementSet, SpeedModifier);
	ATTRIBUTE_ACCESSORS(UInventoryMovementSet, Energy);
	ATTRIBUTE_ACCESSORS(UInventoryMovementSet, MaxEnergy);
	ATTRIBUTE_ACCESSORS(UInventoryMovementSet, EnergyHealing);
	ATTRIBUTE_ACCESSORS(UInventoryMovementSet, EnergyDamage);

	mutable FInventoryAttributeUpdate OnSpeedModifierChanged;
	mutable FInventoryAttributeUpdate OnEnergyChanged;
	mutable FInventoryAttributeUpdate OnMaxEnergyChanged;
	mutable FInventoryAttributeEvent OnOutOfEnergy;

protected:
	UFUNCTION()
		void OnRep_SpeedModifier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_Energy(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_MaxEnergy(const FGameplayAttributeData& OldValue);

	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	//to modify the acceleration and max speed
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SpeedModifier, Category = "Inventory|Movement", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData SpeedModifier;
	//resource used to check if speedmofier can be applied
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Energy, Category = "Inventory|Movement", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData Energy;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxEnergy, Category = "Inventory|Movement", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData MaxEnergy;

	bool bOutOfEnergy;
	float EnergyBeforeAttributeChange;
	float MaxEnergyBeforeAttributeChange;

	// -------------------------------------------------------------------
	//	Meta Attribute (please keep attributes that aren't 'stateful' below 
	// -------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Movement", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData EnergyHealing;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Movement", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData EnergyDamage;
};
