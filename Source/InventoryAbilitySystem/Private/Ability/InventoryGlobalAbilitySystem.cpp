// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryGlobalAbilitySystem.h"
#include "Ability/InventoryAbilitySystemComponent.h"

void FGlobalAppliedAbilityList::AddToASC(TSubclassOf<UGameplayAbility> Ability, UInventoryAbilitySystemComponent* ASC)
{
	if (FGameplayAbilitySpecHandle* SpecHandle = Handles.Find(ASC))
	{
		RemoveFromASC(ASC);
	}

	UGameplayAbility* AbilityCDO = Ability->GetDefaultObject<UGameplayAbility>();
	FGameplayAbilitySpec AbilitySpec(AbilityCDO);
	const FGameplayAbilitySpecHandle AbilitySpecHandle = ASC->GiveAbility(AbilitySpec);
	Handles.Add(ASC, AbilitySpecHandle);
}

void FGlobalAppliedAbilityList::RemoveFromASC(UInventoryAbilitySystemComponent* ASC)
{
	if (FGameplayAbilitySpecHandle* SpecHandle = Handles.Find(ASC))
	{
		ASC->ClearAbility(*SpecHandle);
		Handles.Remove(ASC);
	}
}

void FGlobalAppliedAbilityList::RemoveFromAll()
{
	for (auto& KVP : Handles)
	{
		if (KVP.Key != nullptr)
		{
			KVP.Key->ClearAbility(KVP.Value);
		}
	}
	Handles.Empty();
}



void FGlobalAppliedEffectList::AddToASC(TSubclassOf<UGameplayEffect> Effect, UInventoryAbilitySystemComponent* ASC)
{
	if (FActiveGameplayEffectHandle* EffectHandle = Handles.Find(ASC))
	{
		RemoveFromASC(ASC);
	}

	const UGameplayEffect* GameplayEffectCDO = Effect->GetDefaultObject<UGameplayEffect>();
	const FActiveGameplayEffectHandle GameplayEffectHandle = ASC->ApplyGameplayEffectToSelf(GameplayEffectCDO, /*Level=*/ 1, ASC->MakeEffectContext());
	Handles.Add(ASC, GameplayEffectHandle);
}

void FGlobalAppliedEffectList::RemoveFromASC(UInventoryAbilitySystemComponent* ASC)
{
	if (FActiveGameplayEffectHandle* EffectHandle = Handles.Find(ASC))
	{
		ASC->RemoveActiveGameplayEffect(*EffectHandle);
		Handles.Remove(ASC);
	}
}

void FGlobalAppliedEffectList::RemoveFromAll()
{
	for (auto& KVP : Handles)
	{
		if (KVP.Key != nullptr)
		{
			KVP.Key->RemoveActiveGameplayEffect(KVP.Value);
		}
	}
	Handles.Empty();
}

UInventoryGlobalAbilitySystem::UInventoryGlobalAbilitySystem()
{
}

void UInventoryGlobalAbilitySystem::ApplyAbilityToAll(TSubclassOf<UGameplayAbility> Ability)
{
	if ((Ability.Get() != nullptr) && (!AppliedAbilities.Contains(Ability)))
	{
		FGlobalAppliedAbilityList& Entry = AppliedAbilities.Add(Ability);
		for (UInventoryAbilitySystemComponent* ASC : RegisteredASCs)
		{
			Entry.AddToASC(Ability, ASC);
		}
	}
}

void UInventoryGlobalAbilitySystem::ApplyEffectToAll(TSubclassOf<UGameplayEffect> Effect)
{
	if ((Effect.Get() != nullptr) && (!AppliedEffects.Contains(Effect)))
	{
		FGlobalAppliedEffectList& Entry = AppliedEffects.Add(Effect);
		for (UInventoryAbilitySystemComponent* ASC : RegisteredASCs)
		{
			Entry.AddToASC(Effect, ASC);
		}
	}
}

void UInventoryGlobalAbilitySystem::RemoveAbilityFromAll(TSubclassOf<UGameplayAbility> Ability)
{
	if ((Ability.Get() != nullptr) && AppliedAbilities.Contains(Ability))
	{
		FGlobalAppliedAbilityList& Entry = AppliedAbilities[Ability];
		Entry.RemoveFromAll();
		AppliedAbilities.Remove(Ability);
	}
}

void UInventoryGlobalAbilitySystem::RemoveEffectFromAll(TSubclassOf<UGameplayEffect> Effect)
{
	if ((Effect.Get() != nullptr) && AppliedEffects.Contains(Effect))
	{
		FGlobalAppliedEffectList& Entry = AppliedEffects[Effect];
		Entry.RemoveFromAll();
		AppliedEffects.Remove(Effect);
	}
}

void UInventoryGlobalAbilitySystem::RegisterASC(UInventoryAbilitySystemComponent* ASC)
{
	check(ASC);

	for (auto& Entry : AppliedAbilities)
	{
		Entry.Value.AddToASC(Entry.Key, ASC);
	}
	for (auto& Entry : AppliedEffects)
	{
		Entry.Value.AddToASC(Entry.Key, ASC);
	}

	RegisteredASCs.AddUnique(ASC);
}

void UInventoryGlobalAbilitySystem::UnregisterASC(UInventoryAbilitySystemComponent* ASC)
{
	check(ASC);
	for (auto& Entry : AppliedAbilities)
	{
		Entry.Value.RemoveFromASC(ASC);
	}
	for (auto& Entry : AppliedEffects)
	{
		Entry.Value.RemoveFromASC(ASC);
	}

	RegisteredASCs.Remove(ASC);
}