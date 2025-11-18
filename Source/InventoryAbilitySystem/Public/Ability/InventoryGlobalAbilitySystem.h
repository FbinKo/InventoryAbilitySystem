// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "InventoryGlobalAbilitySystem.generated.h"

class UInventoryAbilitySystemComponent;

USTRUCT()
struct FGlobalAppliedAbilityList
{
	GENERATED_BODY()

	UPROPERTY()
		TMap<TObjectPtr<UInventoryAbilitySystemComponent>, FGameplayAbilitySpecHandle> Handles;

	void AddToASC(TSubclassOf<UGameplayAbility> Ability, UInventoryAbilitySystemComponent* ASC);
	void RemoveFromASC(UInventoryAbilitySystemComponent* ASC);
	void RemoveFromAll();
};

USTRUCT()
struct FGlobalAppliedEffectList
{
	GENERATED_BODY()

	UPROPERTY()
		TMap<TObjectPtr<UInventoryAbilitySystemComponent>, FActiveGameplayEffectHandle> Handles;

	void AddToASC(TSubclassOf<UGameplayEffect> Effect, UInventoryAbilitySystemComponent* ASC);
	void RemoveFromASC(UInventoryAbilitySystemComponent* ASC);
	void RemoveFromAll();
};

UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryGlobalAbilitySystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UInventoryGlobalAbilitySystem();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
		void ApplyAbilityToAll(TSubclassOf<UGameplayAbility> Ability);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
		void ApplyEffectToAll(TSubclassOf<UGameplayEffect> Effect);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
		void RemoveAbilityFromAll(TSubclassOf<UGameplayAbility> Ability);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
		void RemoveEffectFromAll(TSubclassOf<UGameplayEffect> Effect);

	/** Register an ASC with global system and apply any active global effects/abilities. */
	void RegisterASC(UInventoryAbilitySystemComponent* ASC);

	/** Removes an ASC from the global system, along with any active global effects/abilities. */
	void UnregisterASC(UInventoryAbilitySystemComponent* ASC);

private:
	UPROPERTY()
		TMap<TSubclassOf<UGameplayAbility>, FGlobalAppliedAbilityList> AppliedAbilities;

	UPROPERTY()
		TMap<TSubclassOf<UGameplayEffect>, FGlobalAppliedEffectList> AppliedEffects;

	UPROPERTY()
		TArray<TObjectPtr<UInventoryAbilitySystemComponent>> RegisteredASCs;
};
