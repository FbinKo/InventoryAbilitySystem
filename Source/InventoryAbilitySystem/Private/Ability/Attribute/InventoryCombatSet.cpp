// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/Attribute/InventoryCombatSet.h"
#include "Net/UnrealNetwork.h"

UInventoryCombatSet::UInventoryCombatSet()
{
}

void UInventoryCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryCombatSet, ArmorPenetration, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryCombatSet, EnergyPenetration, COND_OwnerOnly, REPNOTIFY_Always);
}

void UInventoryCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryCombatSet, BaseDamage, OldValue);
}

void UInventoryCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryCombatSet, BaseHeal, OldValue);
}

void UInventoryCombatSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryCombatSet, ArmorPenetration, OldValue);
}

void UInventoryCombatSet::OnRep_EnergyPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryCombatSet, EnergyPenetration, OldValue);
}
