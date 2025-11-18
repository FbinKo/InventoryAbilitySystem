// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryGameplayAbility.h"
#include "InventoryGameplayAbility_Death.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryGameplayAbility_Death : public UInventoryGameplayAbility
{
	GENERATED_BODY()
	
public:
	UInventoryGameplayAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// Starts the death sequence.
	UFUNCTION(BlueprintCallable)
		void StartDeath();

	// Finishes the death sequence.
	UFUNCTION(BlueprintCallable)
		void FinishDeath();

protected:

	// If enabled, the ability will automatically call StartDeath.  FinishDeath is always called when the ability ends if the death was started.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		bool bAutoStartDeath;
};
