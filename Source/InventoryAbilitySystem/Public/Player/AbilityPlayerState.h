// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "AbilityPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYABILITYSYSTEM_API AAbilityPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
private:
	UPROPERTY(VisibleAnywhere, Category = "PlayerState")
		TObjectPtr<class UInventoryAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
		TObjectPtr<const class UInventoryHealthSet> HealthSet;
	UPROPERTY()
		TObjectPtr<const class UInventoryCombatSet> CombatSet;

public:
	AAbilityPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//End IAbilitySystemInterface
};
