// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/AttributeComponent.h"
#include "Ability/Attribute/InventoryHealthSet.h"
#include "Ability/Attribute/InventoryMovementSet.h"
#include "Ability/InventoryAbilitySystemComponent.h"
#include "InventoryAbilitySystem/InventoryGameplayTags.h"

#include "Net/UnrealNetwork.h"
#include "GameplayAbilitiesModule.h"
#include "AbilitySystemGlobals.h"

// Sets default values for this component's properties
UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
	DeathState = EDeathState::NotDead;
}

void UAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAttributeComponent, DeathState);
}

void UAttributeComponent::InitializeAfterAbilitySystemComponent(UInventoryAbilitySystemComponent* asc, int32 levelToInitialize)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("AttributeComponent: Attribute component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = asc;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("AttributeComponent: Cannot initialize Attribute component for owner [%s] with NULL ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet = AbilitySystemComponent->GetSet<UInventoryHealthSet>();
	if (!HealthSet)
	{
		UE_LOG(LogTemp, Error, TEXT("AttributeComponent: Cannot initialize Attribute component for owner [%s] with NULL health set on the ability system."), *GetNameSafe(Owner));
		return;
	}

	MovementSet = AbilitySystemComponent->GetSet<UInventoryMovementSet>();
	if (!MovementSet)
	{
		UE_LOG(LogTemp, Error, TEXT("AttributeComponent: Cannot initialize Attribute component for owner [%s] with NULL movement set on the ability system."), *GetNameSafe(Owner));
		return;
	}

	// Register to listen for attribute changes.
	HealthSet->OnShieldChanged.AddUObject(this, &ThisClass::HandleShieldChanged);
	HealthSet->OnMaxShieldChanged.AddUObject(this, &ThisClass::HandleMaxShieldChanged);
	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);
	MovementSet->OnSpeedModifierChanged.AddUObject(this, &ThisClass::HandleSpeedChanged);
	MovementSet->OnEnergyChanged.AddUObject(this, &ThisClass::HandleEnergyChanged);
	MovementSet->OnMaxEnergyChanged.AddUObject(this, &ThisClass::HandleMaxEnergyChanged);
	MovementSet->OnOutOfEnergy.AddUObject(this, &ThisClass::HandleOutOfEnergy);

	IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->InitAttributeSetDefaults(AbilitySystemComponent, AttributeGroupName, levelToInitialize, /*IsInitialLoad*/true);

	HandleShieldChanged(nullptr, nullptr, nullptr, 1.f, HealthSet->GetShield(), HealthSet->GetShield());
	HandleMaxShieldChanged(nullptr, nullptr, nullptr, 1.f, HealthSet->GetMaxShield(), HealthSet->GetMaxShield());
	HandleHealthChanged(nullptr, nullptr, nullptr, 1.f, HealthSet->GetHealth(), HealthSet->GetHealth());
	HandleMaxHealthChanged(nullptr, nullptr, nullptr, 1.f, HealthSet->GetHealth(), HealthSet->GetHealth());
	HandleSpeedChanged(MovementSet->GetSpeedModifier(), MovementSet->GetSpeedModifier());
	HandleMaxEnergyChanged(MovementSet->GetMaxEnergy(), MovementSet->GetMaxEnergy());
	HandleEnergyChanged(MovementSet->GetEnergy(), MovementSet->GetEnergy());
}

void UAttributeComponent::StartDeath()
{
	if (DeathState != EDeathState::NotDead)
	{
		return;
	}

	DeathState = EDeathState::DeathStarted;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(InventoryGameplayTags::Status_Death_Dying, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathStarted.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UAttributeComponent::FinishDeath()
{
	if (DeathState != EDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EDeathState::DeathFinished;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(InventoryGameplayTags::Status_Death_Dead, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathFinished.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UAttributeComponent::HandleShieldChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnShieldChanged.Broadcast(OldValue, NewValue, DamageInstigator);
}

void UAttributeComponent::HandleMaxShieldChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxShieldChanged.Broadcast(OldValue, NewValue, DamageInstigator);
}

void UAttributeComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnHealthChanged.Broadcast(OldValue, NewValue, DamageInstigator);
}

void UAttributeComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(OldValue, NewValue, DamageInstigator);
}

void UAttributeComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	if (AbilitySystemComponent && DamageEffectSpec)
		// Send the "GameplayEvent.Death" gameplay event through the owner's ability system.  This can be used to trigger a death gameplay ability.
	{
		FGameplayEventData Payload;
		Payload.EventTag = InventoryGameplayTags::GameplayEvent_Death;
		Payload.Instigator = DamageInstigator;
		Payload.Target = AbilitySystemComponent->GetAvatarActor();
		Payload.OptionalObject = DamageEffectSpec->Def;
		Payload.ContextHandle = DamageEffectSpec->GetEffectContext();
		Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
		Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
		Payload.EventMagnitude = DamageMagnitude;

		FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
		AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
}

void UAttributeComponent::HandleSpeedChanged(float OldValue, float NewValue)
{
	OnSpeedChanged.Broadcast(OldValue, NewValue, nullptr);
}

void UAttributeComponent::HandleEnergyChanged(float OldValue, float NewValue)
{
	OnEnergyChanged.Broadcast(OldValue, NewValue, nullptr);
}

void UAttributeComponent::HandleMaxEnergyChanged(float OldValue, float NewValue)
{
	OnMaxEnergyChanged.Broadcast(OldValue, NewValue, nullptr);
}

void UAttributeComponent::HandleOutOfEnergy(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	if (AbilitySystemComponent && DamageEffectSpec)
	{
		FGameplayEventData Payload;
		Payload.EventTag = InventoryGameplayTags::GameplayEvent_OutOfEnergy;
		Payload.Instigator = DamageInstigator;
		Payload.Target = AbilitySystemComponent->GetAvatarActor();
		Payload.OptionalObject = DamageEffectSpec->Def;
		Payload.ContextHandle = DamageEffectSpec->GetEffectContext();
		Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
		Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
		Payload.EventMagnitude = DamageMagnitude;

		FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
		AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
}

void UAttributeComponent::OnRep_DeathState(EDeathState OldDeathState)
{
	const EDeathState NewDeathState = DeathState;

	// Revert the death state for now since we rely on StartDeath and FinishDeath to change it.
	DeathState = OldDeathState;

	if (OldDeathState > NewDeathState)
	{
		// The server is trying to set us back but we've already predicted past the server state.
		UE_LOG(LogTemp, Warning, TEXT("UAttributeComponent: Predicted past server death state [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		return;
	}

	if (OldDeathState == EDeathState::NotDead)
	{
		if (NewDeathState == EDeathState::DeathStarted)
		{
			StartDeath();
		}
		else if (NewDeathState == EDeathState::DeathFinished)
		{
			StartDeath();
			FinishDeath();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAttributeComponent: Invalid death transition [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}
	else if (OldDeathState == EDeathState::DeathStarted)
	{
		if (NewDeathState == EDeathState::DeathFinished)
		{
			FinishDeath();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UAttributeComponent: Invalid death transition [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}

	ensureMsgf((DeathState == NewDeathState), TEXT("UAttributeComponent: Death transition failed [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
}
