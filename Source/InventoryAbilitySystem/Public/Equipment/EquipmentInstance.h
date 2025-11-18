// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "EquipmentInstance.generated.h"

USTRUCT(BlueprintType)
struct FInventoryAnimLayerSelectionEntry
{
	GENERATED_BODY()

	// Layer to apply if the tag matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<UAnimInstance> Layer;

	// Cosmetic tags required (all of these must be present to be considered a match)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Cosmetic"))
		FGameplayTagContainer RequiredTags;
};

USTRUCT(BlueprintType)
struct FInventoryAnimLayerSelectionSet
{
	GENERATED_BODY()

	// List of layer rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = Layer))
		TArray<FInventoryAnimLayerSelectionEntry> LayerRules;

	// The layer to use if none of the LayerRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<UAnimInstance> DefaultLayer;

	// Choose the best layer given the rules
	TSubclassOf<UAnimInstance> SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const;
};

UCLASS(BlueprintType, Blueprintable)
class INVENTORYABILITYSYSTEM_API UEquipmentInstance : public UObject
{
	GENERATED_BODY()

private:
	UPROPERTY(ReplicatedUsing = OnRep_Instigator)
		TObjectPtr<UObject> Instigator;
	UPROPERTY(Replicated)
		TArray<TObjectPtr<AActor>> SpawnedActors;

	double TimeLastEquipped = 0.0;
	double TimeLastUsed = 0.0;

protected:
	UPROPERTY(EditAnywhere, Category = "Animation")
		UAnimMontage* EquipMontage;

	UPROPERTY(EditAnywhere, Category = "Animation")
		UAnimMontage* UnequipMontage;

	/* in case we have different layers in UInventoryCosmeticComponent, use this to filter */
	UPROPERTY(EditAnywhere, Category = "Animation")
		FGameplayTag EquipmentLayerPrefix;
	FGameplayTagContainer CosmeticAnimationStyleTags;

	UPROPERTY(EditAnywhere, Category = "Animation")
		FInventoryAnimLayerSelectionSet EquipmentLayerSet;

public:
	UEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void Tick(float DeltaSecondsd) {};

	UFUNCTION(BlueprintPure, Category = Equipment)
		UObject* GetInstigator() const { return Instigator; }
	void SetInstigator(UObject* inInstigator) { Instigator = inInstigator; }

	UFUNCTION(BlueprintPure)
		APawn* GetPawn() { return Cast<APawn>(GetOuter()); }

	UFUNCTION(BlueprintPure)
		TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	virtual void SpawnEquipmentActors(const TArray<struct FEquipmentActorToSpawn>& ActorsToSpawn);
	virtual void DestroyEquipmentActors();

	virtual void OnEquipped();
	virtual void OnUnequipped();

	UFUNCTION(BlueprintCallable)
		void UpdateUseTime();

	double GetTimeLastUsed() { return TimeLastUsed; }
	UFUNCTION(BlueprintPure)
		float GetTimeLastInteracted();


protected:
	UFUNCTION(BlueprintImplementableEvent, Category = Equipment, meta = (DisplayName = "OnEquipped"))
		void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category = Equipment, meta = (DisplayName = "OnUnequipped"))
		void K2_OnUnequipped();

private:
	UFUNCTION()
		void OnRep_Instigator();
};
