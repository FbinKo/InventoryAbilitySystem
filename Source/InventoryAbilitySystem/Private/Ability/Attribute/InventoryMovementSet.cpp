// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability/Attribute/InventoryMovementSet.h"
#include "InventoryAbilitySystem/InventoryGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"

UInventoryMovementSet::UInventoryMovementSet()
{
	bOutOfEnergy = false;
	EnergyBeforeAttributeChange = 0.f;
	MaxEnergyBeforeAttributeChange = 0.f;
}

void UInventoryMovementSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryMovementSet, SpeedModifier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryMovementSet, Energy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryMovementSet, MaxEnergy, COND_None, REPNOTIFY_Always);
}

void UInventoryMovementSet::OnRep_SpeedModifier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryMovementSet, SpeedModifier, OldValue);
}

void UInventoryMovementSet::OnRep_Energy(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryMovementSet, Energy, OldValue);

	const float CurrentEnergy = GetEnergy();
	const float EstimatedMagnitude = CurrentEnergy - OldValue.GetCurrentValue();

	OnEnergyChanged.Broadcast(OldValue.GetCurrentValue(), CurrentEnergy);

	if (!bOutOfEnergy && CurrentEnergy <= 0.0f)
	{
		OnOutOfEnergy.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentEnergy);
	}

	bOutOfEnergy = (CurrentEnergy <= 0.0f);
}

void UInventoryMovementSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryMovementSet, MaxEnergy, OldValue);

	OnMaxEnergyChanged.Broadcast(OldValue.GetCurrentValue(), GetMaxEnergy());
}

bool UInventoryMovementSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	MaxEnergyBeforeAttributeChange = GetMaxEnergy();

	return true;
}

void UInventoryMovementSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	float MinimumEnergy = 0.f;

#if !UE_BUILD_SHIPPING
	if (Data.Target.HasMatchingGameplayTag(InventoryGameplayTags::Cheat_GodMode) || Data.Target.HasMatchingGameplayTag(InventoryGameplayTags::Cheat_UnlimitedEnergy))
	{
		MinimumEnergy = 1.f;
	}
#endif // #if !UE_BUILD_SHIPPING

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetEnergyDamageAttribute())
	{
		float damage = GetEnergyDamage();
		SetEnergyDamage(0.0f);
		if (damage > 0.f) {
			SetEnergy(FMath::Clamp(GetEnergy() - damage, MinimumEnergy, GetMaxEnergy()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetEnergyHealingAttribute())
	{
		SetEnergy(FMath::Clamp(GetEnergy() + GetEnergyHealing(), MinimumEnergy, GetMaxEnergy()));
		SetEnergyHealing(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetEnergyAttribute())
	{
		SetEnergy(FMath::Clamp(GetEnergy(), MinimumEnergy, GetMaxEnergy()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxEnergyAttribute())
	{
		OnMaxEnergyChanged.Broadcast(MaxEnergyBeforeAttributeChange, GetMaxEnergy());
	}

	if (GetEnergy() != EnergyBeforeAttributeChange)
	{
		OnEnergyChanged.Broadcast(EnergyBeforeAttributeChange, GetEnergy());
	}

	if ((GetEnergy() <= 0.f) && !bOutOfEnergy)
	{
		OnOutOfEnergy.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, EnergyBeforeAttributeChange, GetEnergy());
	}

	bOutOfEnergy = GetEnergy() <= 0.f;
}

void UInventoryMovementSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UInventoryMovementSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UInventoryMovementSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetSpeedModifierAttribute())
	{
		OnSpeedModifierChanged.Broadcast(OldValue, NewValue);
	}
	else if (Attribute == GetMaxEnergyAttribute())
	{
		if (GetEnergy() > NewValue)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponentChecked();
			ASC->ApplyModToAttribute(GetEnergyAttribute(), EGameplayModOp::Override, NewValue);
		}
	}

	if (bOutOfEnergy && (GetEnergy() > 0.f))
	{
		bOutOfEnergy = false;
	}
}

void UInventoryMovementSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetEnergyAttribute())
	{
		// Do not allow Energy to go negative or above max Energy.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEnergy());
	}
	else if (Attribute == GetMaxEnergyAttribute())
	{
		// Do not allow max Energy to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
