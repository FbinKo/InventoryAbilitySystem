// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryGameplayAbility.h"
#include "InventoryGameplayAbility_Jump.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryGameplayAbility_Jump : public UInventoryGameplayAbility
{
	GENERATED_BODY()
	
public:
	UInventoryGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable)
		void JumpStart();

	UFUNCTION(BlueprintCallable)
		void JumpStop();
};
