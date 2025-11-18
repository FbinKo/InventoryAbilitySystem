// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryGameplayAbility_Jump.h"
#include "GameFramework/Character.h"

UInventoryGameplayAbility_Jump::UInventoryGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool UInventoryGameplayAbility_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->CanJump())
	{
		return false;
	}

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UInventoryGameplayAbility_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Stop jumping in case the ability blueprint doesn't call it.
	JumpStop();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UInventoryGameplayAbility_Jump::JumpStart()
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (Character->IsLocallyControlled() && !Character->bPressedJump)
		{
			Character->UnCrouch();
			Character->Jump();
		}
	}
}

void UInventoryGameplayAbility_Jump::JumpStop()
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (Character->IsLocallyControlled() && Character->bPressedJump)
		{
			Character->StopJumping();
		}
	}
}
