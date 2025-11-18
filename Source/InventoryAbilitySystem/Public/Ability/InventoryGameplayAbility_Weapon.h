// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InventoryGameplayAbility_FromEquipment.h"
#include "InventoryGameplayAbility_Weapon.generated.h"

/**
 *
 */
UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryGameplayAbility_Weapon : public UInventoryGameplayAbility_FromEquipment
{
	GENERATED_BODY()

private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;

public:
	UInventoryGameplayAbility_Weapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	template<typename T>
	T* GetWeaponInstance() const
	{
		return Cast<T>(GetAssociatedEquipment());
	}

	UFUNCTION(BlueprintCallable, meta = (ComponentClass = "/Script/InventoryAbilitySystem.WeaponInstance"), meta = (DeterminesOutputType = "WeaponInstanceClass"))
		UWeaponInstance* GetWeaponInstance(TSubclassOf<UWeaponInstance> WeaponInstanceClass) const;

	//~UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

protected:
	virtual void PostCommitAbility() {};

	// Called when attacking and using no projectile weapons
	UFUNCTION(BlueprintImplementableEvent)
		void PerformLocalTargeting(TArray<FHitResult>& OutHits);

	virtual void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	// Called when target data is ready
	UFUNCTION(BlueprintImplementableEvent)
		void OnWeaponTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);

	UFUNCTION(BlueprintCallable)
		void StartWeaponTargeting();
};
