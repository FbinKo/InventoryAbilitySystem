// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryGameplayAbility_Death.h"
#include "Ability/AttributeComponent.h"
#include "Ability/InventoryAbilitySystemComponent.h"
#include "InventoryAbilitySystem/InventoryGameplayTags.h"

UInventoryGameplayAbility_Death::UInventoryGameplayAbility_Death(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	bAutoStartDeath = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// Add the ability trigger tag as default to the CDO.
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = InventoryGameplayTags::GameplayEvent_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UInventoryGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	UInventoryAbilitySystemComponent* ASC = CastChecked<UInventoryAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(InventoryGameplayTags::Ability_Behavior_SurvivesDeath);

	// Cancel all abilities and block others from starting.
	ASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	SetCanBeCanceled(false);

	if (bAutoStartDeath)
	{
		StartDeath();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UInventoryGameplayAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	check(ActorInfo);

	// Always try to finish the death when the ability ends in case the ability doesn't.
	// This won't do anything if the death hasn't been started.
	FinishDeath();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UInventoryGameplayAbility_Death::StartDeath()
{
	if (UAttributeComponent* AttributeComponent = GetAvatarActorFromActorInfo()->FindComponentByClass<UAttributeComponent>())
	{
		if (AttributeComponent->GetDeathState() == EDeathState::NotDead)
		{
			AttributeComponent->StartDeath();
		}
	}
}

void UInventoryGameplayAbility_Death::FinishDeath()
{
	if (UAttributeComponent* AttributeComponent = GetAvatarActorFromActorInfo()->FindComponentByClass<UAttributeComponent>())
	{
		if (AttributeComponent->GetDeathState() == EDeathState::DeathStarted)
		{
			AttributeComponent->FinishDeath();
		}
	}
}
