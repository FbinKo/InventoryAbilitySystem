// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "AbilityPlayerController.generated.h"

UCLASS()
class INVENTORYABILITYSYSTEM_API AAbilityPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly)
		FGenericTeamId teamID;

	UPROPERTY(VisibleDefaultsOnly)
		TObjectPtr<class UInventoryComponent> inventoryComponent;

public:
	AAbilityPlayerController(const FObjectInitializer& ObjectInitializer);

	virtual void OnPossess(APawn* InPawn) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	UFUNCTION(BlueprintCallable)
		class AAbilityPlayerState* GetAbilityPlayerState() const;

	UFUNCTION(BlueprintCallable)
		class UInventoryAbilitySystemComponent* GetAbilitySystemComponent() const;
};
