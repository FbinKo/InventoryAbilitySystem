// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "InventoryGameplayAbility.generated.h"

/**
 * EInventoryAbilityActivationPolicy
 *
 *	Defines how an ability is meant to activate.
 */
UENUM(BlueprintType)
enum class EInventoryAbilityActivationPolicy : uint8
{
	// Try to activate the ability when the input is triggered.
	OnInputTriggered,

	// Continually try to activate the ability while the input is active.
	WhileInputActive,

	// Try to activate the ability when an avatar is assigned.
	OnSpawn
};

/**
 * EInventoryAbilityActivationGroup
 *
 *	Defines how an ability activates in relation to other abilities.
 */
UENUM(BlueprintType)
enum class EInventoryAbilityActivationGroup : uint8
{
	// Ability runs independently of all other abilities.
	Independent,

	// Ability is canceled and replaced by other exclusive abilities.
	Exclusive_Replaceable,

	// Ability blocks all other exclusive abilities from activating.
	Exclusive_Blocking,

	MAX	UMETA(Hidden)
};

UCLASS(Abstract, HideCategories = Input, Meta = (ShortTooltip = "The base gameplay ability class used by this project."))
class INVENTORYABILITYSYSTEM_API UInventoryGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	friend class UInventoryAbilitySystemComponent;

protected:
	// Defines how this ability is meant to activate.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Activation")
		EInventoryAbilityActivationPolicy ActivationPolicy;

	// Defines the relationship between this ability activating and other abilities activating.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Activation")
		EInventoryAbilityActivationGroup ActivationGroup;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = Costs)
		TArray<TObjectPtr<class UInventoryAbilityCost>> AdditionalCosts;
	
public:
	UInventoryGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	EInventoryAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	EInventoryAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }

	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	void OnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const
	{
		NativeOnAbilityFailedToActivate(FailedReason);
		ScriptOnAbilityFailedToActivate(FailedReason);
	}

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// Called when the ability fails to activate
	virtual void NativeOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const;

	// Called when the ability fails to activate
	UFUNCTION(BlueprintImplementableEvent)
		void ScriptOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const;

	virtual void OnPawnAvatarSet();

	/** Called when the ability system is initialized with a pawn avatar. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnPawnAvatarSet")
		void K2_OnPawnAvatarSet();

	UFUNCTION(BlueprintCallable, DisplayName = "Can Activate")
		bool K2_CanActivate();

};
