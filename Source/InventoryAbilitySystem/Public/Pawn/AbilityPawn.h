// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilityPawn.generated.h"

UCLASS()
class INVENTORYABILITYSYSTEM_API AAbilityPawn : public APawn, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
		TArray<struct FInventoryDefinition> initialInventoryItems;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
		TObjectPtr<class UInventoryInputConfig> initialInputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
		TObjectPtr<const class UAbilitySet> initialAbilitySet;

	FGenericTeamId teamID;

protected:
	TObjectPtr<class UInventoryAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TObjectPtr<class UAttributeComponent> AttributeComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TObjectPtr<class UInventoryCosmeticComponent> CosmeticComponent;

public:
	// Sets default values for this character's properties
	AAbilityPawn();

	//Begin AActor Interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//End AActor Interface

	//Begin APawn interface
	virtual void PossessedBy(AController* NewController) override;
	//End APawn interface

	//Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//End IAbilitySystemInterface

	void UninitializeAbilitySystem();

	//Begin IGenericTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return teamID; }
	//End IGenericTeamAgentInterface

protected:
	UFUNCTION()
		virtual void OnDeathFinished(AActor* OwningActor);

};
