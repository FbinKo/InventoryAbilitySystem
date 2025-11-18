// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InventoryAttributeSet.h"
#include "NativeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "InventoryHealthSet.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Damage);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageImmunity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageSelfDestruct);

UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryHealthSet : public UInventoryAttributeSet
{
	GENERATED_BODY()
	
public:
	UInventoryHealthSet();

	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, Shield);
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, MaxShield);
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, Health);
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, Healing);
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, Damage);
	//flat damage block
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, KineticArmor);
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, EnergyArmor);
	//percetile damage block
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, KineticResistance);
	ATTRIBUTE_ACCESSORS(UInventoryHealthSet, EnergyResistance);

	mutable FInventoryAttributeEvent OnShieldChanged;
	mutable FInventoryAttributeEvent OnMaxShieldChanged;
	mutable FInventoryAttributeEvent OnOutOfShield;

	// Delegate when health changes due to damage/healing, some information may be missing on the client
	mutable FInventoryAttributeEvent OnHealthChanged;

	// Delegate when max health changes
	mutable FInventoryAttributeEvent OnMaxHealthChanged;

	// Delegate to broadcast when the health attribute reaches zero
	mutable FInventoryAttributeEvent OnOutOfHealth;

protected:
	UFUNCTION()
		void OnRep_Shield(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_MaxShield(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_KineticArmor(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_EnergyArmor(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_KinecticResistance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
		void OnRep_EnergyResistance(const FGameplayAttributeData& OldValue);

	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	// The current shield attribute.  The shield will be capped by the max shield attribute.  Shield is hidden from modifiers so only executions can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "Inventory|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
		FGameplayAttributeData Shield;

	// The current max shield attribute.  Max shield is an attribute since gameplay effects can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxShield, Category = "Inventory|Health", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData MaxShield;
	bool bOutOfShield;
	float ShieldBeforeAttributeChange;
	float MaxShieldBeforeAttributeChange;

	// The current health attribute.  The health will be capped by the max health attribute.  Health is hidden from modifiers so only executions can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Inventory|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
		FGameplayAttributeData Health;

	// The current max health attribute.  Max health is an attribute since gameplay effects can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Inventory|Health", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData MaxHealth;

	// Used to track when the health reaches 0.
	bool bOutOfHealth;

	// Store the health before any changes 
	float HealthBeforeAttributeChange;
	float MaxHealthBeforeAttributeChange;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_KineticArmor, Category = "Inventory|Armor", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData KineticArmor;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EnergyArmor, Category = "Inventory|Armor", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData EnergyArmor;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_KinecticResistance, Category = "Inventory|Armor", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData KineticResistance;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EnergyResistance, Category = "Inventory|Armor", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData EnergyResistance;

	float KineticArmorBeforeAttributeChange;
	float EnergyArmorBeforeAttributeChange;
	float KineticResistanceBeforeAttributeChange;
	float EnergyResistanceBeforeAttributeChange;

	// -------------------------------------------------------------------
	//	Meta Attribute (please keep attributes that aren't 'stateful' below 
	// -------------------------------------------------------------------

	// Incoming healing. This is mapped directly to +Health
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Health", Meta = (AllowPrivateAccess = true))
		FGameplayAttributeData Healing;

	// Incoming damage. This is mapped directly to -Health
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
		FGameplayAttributeData Damage;
};
