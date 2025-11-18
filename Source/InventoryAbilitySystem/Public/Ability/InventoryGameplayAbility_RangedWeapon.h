// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "InventoryGameplayAbility_Weapon.h"
#include "InventoryGameplayAbility_RangedWeapon.generated.h"

/** Defines where an ability starts its trace from and where it should face */
UENUM(BlueprintType)
enum class ETargetingSource : uint8
{
	// From the player's camera towards camera focus
	CameraTowardsFocus,
	// From the pawn's center, in the pawn's orientation
	PawnForward,
	// From the pawn's center, oriented towards camera focus
	PawnTowardsFocus,
	// From the weapon's muzzle or location, in the pawn's orientation
	WeaponForward,
	// From the weapon's muzzle or location, towards camera focus
	WeaponTowardsFocus,
	// Custom blueprint-specified source location
	Custom
};

UCLASS()
class INVENTORYABILITYSYSTEM_API UInventoryGameplayAbility_RangedWeapon : public UInventoryGameplayAbility_Weapon
{
	GENERATED_BODY()

protected:
	//UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void PostCommitAbility() override;
	//end UGameplayAbility interface

	virtual void AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const;

	// Determine the trace channel to use for the weapon trace(s)
	virtual ECollisionChannel DetermineTraceChannel(FCollisionQueryParams& TraceParams, bool bIsSimulated) const;

	// Does a single weapon trace, either sweeping or ray depending on if SweepRadius is above zero
	UFUNCTION(BlueprintCallable)
		FHitResult WeaponTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated, TArray<FHitResult>& OutHitResults) const;

	UFUNCTION(BlueprintCallable)
		FTransform GetTargetingTransform(ETargetingSource source);
	
};
