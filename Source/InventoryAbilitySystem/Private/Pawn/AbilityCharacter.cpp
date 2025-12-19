// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/AbilityCharacter.h"
#include "Player/AbilityPlayerState.h"
#include "Ability/AttributeComponent.h"
#include "Equipment/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemInstance.h"
#include "Inventory/InventoryCosmeticComponent.h"
#include "Ability/InventoryAbilitySystemComponent.h"
#include "InventoryInputConfig.h"
#include "InventoryAbilitySystem/InventoryGameplayTags.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

// Sets default values
AAbilityCharacter::AAbilityCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttributeComponent"));
	AttributeComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);
	CosmeticComponent = CreateDefaultSubobject<UInventoryCosmeticComponent>(TEXT("CosmeticComponent"));

	teamID = FGenericTeamId::NoTeam;
}

void AAbilityCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();

	Super::EndPlay(EndPlayReason);
}

void AAbilityCharacter::PossessedBy(AController* NewController)
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

UAbilitySystemComponent* AAbilityCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAbilityCharacter::UninitializeAbilitySystem()
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

void AAbilityCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (teamID != NewTeamID)
	{
		teamID = NewTeamID;
		// @todo notify perception system that a controller changed team ID
	}
}

void AAbilityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (initialInputConfig) {
		for (const FInputMappingContextAndPriority& Mapping : initialInputConfig->InputMappings)
		{
			if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
			{
				if (Mapping.bRegisterWithSettings)
				{
					FModifyContextOptions Options = {};
					Options.bIgnoreAllPressedKeysUntilRelease = false;
					Options.bNotifyUserSettings = true;
					// Actually add the config to the local player							
					Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
				}
			}
		}

		UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		for (FInventoryInputAction action : initialInputConfig->AbilityInputActions) {
			Input->BindAction(action.InputAction, ETriggerEvent::Triggered, this, &ThisClass::Input_AbilityInputTagPressed, action.InputTag);
			Input->BindAction(action.InputAction, ETriggerEvent::Completed, this, &ThisClass::Input_AbilityInputTagReleased, action.InputTag);
		}
	}
}

void AAbilityCharacter::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputTagPressed(InputTag);
	}
}

void AAbilityCharacter::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputTagReleased(InputTag);
	}
}

void AAbilityCharacter::OnDeathFinished(AActor* OwningActor)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::UninitializeAbilitySystem);
}
