// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AbilityPlayerController.h"
#include "Player/AbilityPlayerState.h"
#include "Ability/InventoryAbilitySystemComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Equipment/EquipmentComponent.h"

AAbilityPlayerController::AAbilityPlayerController(const FObjectInitializer& ObjectInitializer)
{
	teamID = 0;// EAffiliation::Affiliation_Player;

	inventoryComponent = CreateOptionalDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void AAbilityPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (inventoryComponent)
		if(UEquipmentComponent* equipment = inventoryComponent->FindEquipmentManager())
			equipment->SetActiveSlotIndex(0);
}

void AAbilityPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UInventoryAbilitySystemComponent* InventoryASC = GetAbilitySystemComponent())
	{
		InventoryASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AAbilityPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	//TODO in case we need to change the id, would need to update all ai how they perceive player
}

FGenericTeamId AAbilityPlayerController::GetGenericTeamId() const
{
	return uint8(teamID);
}

AAbilityPlayerState* AAbilityPlayerController::GetAbilityPlayerState() const
{
	if (AAbilityPlayerState* state = Cast<AAbilityPlayerState>(PlayerState))
		return state;
	return nullptr;
}

UInventoryAbilitySystemComponent* AAbilityPlayerController::GetAbilitySystemComponent() const
{
	const AAbilityPlayerState* AbilityPS = GetAbilityPlayerState();
	return (AbilityPS ? Cast<UInventoryAbilitySystemComponent>(AbilityPS->GetAbilitySystemComponent()) : nullptr);
}
