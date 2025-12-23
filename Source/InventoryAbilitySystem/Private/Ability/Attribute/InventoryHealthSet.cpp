// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability/Attribute/InventoryHealthSet.h"
#include "InventoryAbilitySystem/InventoryGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_Damage, "Gameplay.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageImmunity, "Gameplay.DamageImmunity");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageSelfDestruct, "Gameplay.Damage.SelfDestruct");

UInventoryHealthSet::UInventoryHealthSet()
{
	bOutOfShield = true;
	ShieldBeforeAttributeChange = 0.f;
	MaxShieldBeforeAttributeChange = 0.f;
	bOutOfHealth = false;
	HealthBeforeAttributeChange = 0.0f;
	MaxHealthBeforeAttributeChange = 0.0f;

	KineticArmorBeforeAttributeChange = 0.f;
	EnergyArmorBeforeAttributeChange = 0.f;
	KineticResistanceBeforeAttributeChange = 0.f;
	EnergyResistanceBeforeAttributeChange = 0.f;
}

void UInventoryHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryHealthSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryHealthSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryHealthSet, KineticArmor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryHealthSet, EnergyArmor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryHealthSet, KineticResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryHealthSet, EnergyResistance, COND_None, REPNOTIFY_Always);
}

void UInventoryHealthSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryHealthSet, Shield, OldValue);

	const float CurrentShield = GetShield();
	const float EstimatedMagnitude = CurrentShield - OldValue.GetCurrentValue();

	OnShieldChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentShield);

	if (!bOutOfShield && CurrentShield <= 0.0f)
	{
		OnOutOfShield.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentShield);
	}

	bOutOfShield = (CurrentShield <= 0.0f);
}

void UInventoryHealthSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryHealthSet, MaxShield, OldValue);

	OnMaxShieldChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxShield() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxShield());
}

void UInventoryHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryHealthSet, Health, OldValue);

	// Call the change callback, but without an instigator
	// This could be changed to an explicit RPC in the future
	// These events on the client should not be changing attributes

	const float CurrentHealth = GetHealth();
	const float EstimatedMagnitude = CurrentHealth - OldValue.GetCurrentValue();

	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentHealth);

	if (!bOutOfHealth && CurrentHealth <= 0.0f)
	{
		OnOutOfHealth.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentHealth);
	}

	bOutOfHealth = (CurrentHealth <= 0.0f);
}

void UInventoryHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryHealthSet, MaxHealth, OldValue);

	// Call the change callback, but without an instigator
	// This could be changed to an explicit RPC in the future
	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxHealth());
}

void UInventoryHealthSet::OnRep_KineticArmor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryHealthSet, KineticArmor, OldValue);
}

void UInventoryHealthSet::OnRep_EnergyArmor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryHealthSet, EnergyArmor, OldValue);
}

void UInventoryHealthSet::OnRep_KinecticResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryHealthSet, KineticResistance, OldValue);
}

void UInventoryHealthSet::OnRep_EnergyResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryHealthSet, EnergyResistance, OldValue);
}

bool UInventoryHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// Handle modifying incoming normal damage
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		if (Data.EvaluatedData.Magnitude > 0.0f)
		{
			const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(TAG_Gameplay_DamageSelfDestruct);

			if (Data.Target.HasMatchingGameplayTag(TAG_Gameplay_DamageImmunity) && !bIsDamageFromSelfDestruct)
			{
				// Do not take away any health.
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}

#if !UE_BUILD_SHIPPING
			// Check GodMode cheat, unlimited health is checked below
			if (Data.Target.HasMatchingGameplayTag(InventoryGameplayTags::Cheat_GodMode) && !bIsDamageFromSelfDestruct)
			{
				// Do not take away any health.
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}
#endif // #if !UE_BUILD_SHIPPING
		}
	}

	// Save the current health
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();

	ShieldBeforeAttributeChange = GetShield();
	MaxShieldBeforeAttributeChange = GetMaxShield();

	KineticArmorBeforeAttributeChange = GetKineticArmor();
	EnergyArmorBeforeAttributeChange = GetEnergyArmor();
	KineticResistanceBeforeAttributeChange = GetKineticResistance();
	EnergyResistanceBeforeAttributeChange = GetEnergyResistance();

	return true;
}

void UInventoryHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(TAG_Gameplay_DamageSelfDestruct);
	float MinimumHealth = 0.0f;

#if !UE_BUILD_SHIPPING
	// Godmode and unlimited health stop death unless it's a self destruct
	if (!bIsDamageFromSelfDestruct &&
		(Data.Target.HasMatchingGameplayTag(InventoryGameplayTags::Cheat_GodMode) || Data.Target.HasMatchingGameplayTag(InventoryGameplayTags::Cheat_UnlimitedHealth)))
	{
		MinimumHealth = 1.0f;
	}
#endif // #if !UE_BUILD_SHIPPING

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// Convert into -Health and then clamp
		float damage = GetDamage();
		SetDamage(0.0f);
		if (GetShield() > 0.f) {
			float newShield = GetShield();
			const float shieldDiff = FMath::Min(newShield, damage);
			damage -= shieldDiff;
			newShield -= shieldDiff;
			SetShield(newShield);
		}

		if (damage > 0.f) {
			SetHealth(FMath::Clamp(GetHealth() - damage, MinimumHealth, GetMaxHealth()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		// Convert into +Health and then clamo
		SetHealth(FMath::Clamp(GetHealth() + GetHealing(), MinimumHealth, GetMaxHealth()));
		SetHealing(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.f, GetMaxShield()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxShieldAttribute())
	{
		OnMaxShieldChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxShieldBeforeAttributeChange, GetMaxShield());
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Clamp and fall into out of health handling below
		SetHealth(FMath::Clamp(GetHealth(), MinimumHealth, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		// Notify on any requested max health changes
		OnMaxHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxHealthBeforeAttributeChange, GetMaxHealth());
	}
	else if (Data.EvaluatedData.Attribute == GetKineticArmorAttribute())
	{
		SetKineticArmor(GetKineticArmor());
	}
	else if (Data.EvaluatedData.Attribute == GetEnergyArmorAttribute())
	{
		SetEnergyArmor(GetEnergyArmor());
	}
	else if (Data.EvaluatedData.Attribute == GetKineticResistanceAttribute())
	{
		SetKineticResistance(GetKineticResistance());
	}
	else if (Data.EvaluatedData.Attribute == GetEnergyResistanceAttribute())
	{
		SetEnergyResistance(GetEnergyResistance());
	}

	if (GetShield() != ShieldBeforeAttributeChange)
	{
		OnShieldChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, ShieldBeforeAttributeChange, GetShield());
	}

	if ((GetShield() <= 0.0f) && !bOutOfShield)
	{
		OnOutOfShield.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, ShieldBeforeAttributeChange, GetShield());
	}

	bOutOfShield = (GetShield() <= 0.f);

	// If health has actually changed activate callbacks
	if (GetHealth() != HealthBeforeAttributeChange)
	{
		OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
	}

	if ((GetHealth() <= 0.0f) && !bOutOfHealth)
	{
		OnOutOfHealth.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
	}

	// Check health again in case an event above changed it.
	bOutOfHealth = (GetHealth() <= 0.0f);
}

void UInventoryHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UInventoryHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UInventoryHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxShieldAttribute())
	{
		if (GetShield() > NewValue)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponentChecked();
			ASC->ApplyModToAttribute(GetShieldAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// Make sure current health is not greater than the new max health.
		if (GetHealth() > NewValue)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponentChecked();
			ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
		}
	}

	if (bOutOfShield && (GetShield() > 0.0f))
	{
		bOutOfShield = false;
	}
	if (bOutOfHealth && (GetHealth() > 0.0f))
	{
		bOutOfHealth = false;
	}
}

void UInventoryHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetHealthAttribute())
	{
		// Do not allow health to go negative or above max health.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// Do not allow max health to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
