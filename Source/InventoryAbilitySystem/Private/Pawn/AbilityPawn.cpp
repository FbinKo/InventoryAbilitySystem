// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/AbilityPawn.h"
#include "Player/AbilityPlayerState.h"
#include "Equipment/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemInstance.h"
#include "Ability/AttributeComponent.h"
#include "Ability/InventoryAbilitySystemComponent.h"
#include "InventoryAbilitySystem/InventoryGameplayTags.h"

// Sets default values
AAbilityPawn::AAbilityPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttributeComponent"));
	AttributeComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);

	teamID = FGenericTeamId::NoTeam;
}

void AAbilityPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();

	Super::EndPlay(EndPlayReason);
}

void AAbilityPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (IGenericTeamAgentInterface* teamAgent = Cast<IGenericTeamAgentInterface>(NewController))
		SetGenericTeamId(teamAgent->GetGenericTeamId());
	else
		SetGenericTeamId(FGenericTeamId::NoTeam);

	//Init player
	if (AbilitySystemComponent)
		return;

	AAbilityPlayerState* playerState = GetPlayerState<AAbilityPlayerState>();
	if (playerState) {
		AbilitySystemComponent = Cast<UInventoryAbilitySystemComponent>(playerState->GetAbilitySystemComponent());
		if (AbilitySystemComponent) {
			AbilitySystemComponent->InitAbilityActorInfo(playerState, this);
			AttributeComponent->InitializeAfterAbilitySystemComponent(AbilitySystemComponent);
			if (initialAbilitySet) {
				initialAbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
			}
		}
	}

	if (UInventoryComponent* inventory = NewController->GetComponentByClass<UInventoryComponent>()) {
		int slot = 0;
		for (int i = 0; i < initialLoadout.Num(); i++) {
			FLoadout info = initialLoadout[i];
			if (UInventoryItemInstance* item = inventory->AddItemDefinition(info.item.Definition, info.item.Amount)) {
				if(info.bEquipItem)
				{
					if (UEquipmentComponent* equipment = inventory->FindEquipmentManager()) {
						equipment->AddItemToSlot(info.slotID, item);
					}
				}
			}
		}
	}
}

UAbilitySystemComponent* AAbilityPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAbilityPawn::UninitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		FGameplayTagContainer AbilityTypesToIgnore;

		AbilitySystemComponent->CancelAbilities(nullptr, &AbilityTypesToIgnore);
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->RemoveAllGameplayCues();

		if (AbilitySystemComponent->GetOwnerActor() != nullptr)
		{
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			// If the ASC doesn't have a valid owner, we need to clear *all* actor info, not just the avatar pairing
			AbilitySystemComponent->ClearActorInfo();
		}
	}

	AbilitySystemComponent = nullptr;
}

void AAbilityPawn::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (teamID != NewTeamID)
	{
		teamID = NewTeamID;
		// @todo notify perception system that a controller changed team ID
	}
}

void AAbilityPawn::OnDeathFinished(AActor* OwningActor)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::UninitializeAbilitySystem);
}