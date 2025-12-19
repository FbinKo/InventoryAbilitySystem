// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AbilityPlayerState.h"
#include "Ability/InventoryAbilitySystemComponent.h"
#include "Ability/Attribute/InventoryHealthSet.h"
#include "Ability/Attribute/InventoryCombatSet.h"
#include "Ability/Attribute/InventoryMovementSet.h"

AAbilityPlayerState::AAbilityPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UInventoryAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// attribute set will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
	HealthSet = CreateDefaultSubobject<UInventoryHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UInventoryCombatSet>(TEXT("CombatSet"));
	MovementSet = CreateDefaultSubobject<UInventoryMovementSet>(TEXT("MovementSet"));
}

UAbilitySystemComponent* AAbilityPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
