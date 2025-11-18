// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EquipmentInstance.h"
#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "WeaponInstance.generated.h"

UCLASS()
class UPhysicalMaterialWithTags : public UPhysicalMaterial
{
	GENERATED_BODY()

public:
	UPhysicalMaterialWithTags(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// A container of gameplay tags that game code can use to reason about this physical material
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalProperties)
		FGameplayTagContainer Tags;
};
/**
 * 
 */
UCLASS()
class INVENTORYABILITYSYSTEM_API UWeaponInstance : public UEquipmentInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
		float GetMaxDamageRange();

	UFUNCTION(BlueprintPure)
		float GetDamageAtDistance(float Distance);

	UFUNCTION(BlueprintPure)
		float GetDamageMultiplier(const UPhysicalMaterial* PhysicalMaterial);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
		UAnimMontage* MeleeAttackMontage;

protected:
	// A curve that maps the distance (in cm) to a multiplier on the base damage from the associated gameplay effect
	// If there is no data in this curve, then the weapon is assumed to have no falloff with distance
	UPROPERTY(EditAnywhere, Category = "Weapon Config")
		FRuntimeFloatCurve DistanceDamageFalloff;

	// List of special tags that affect how damage is dealt
	// These tags will be compared to tags in the physical material of the thing being hit
	// If more than one tag is present, the multipliers will be combined multiplicatively
	UPROPERTY(EditAnywhere, Category = "Weapon Config")
		TMap<FGameplayTag, float> MaterialDamageMultiplier;	
};

UCLASS()
class INVENTORYABILITYSYSTEM_API URangedWeaponInstance : public UWeaponInstance
{
	GENERATED_BODY()
	friend class UInventoryGameplayAbility_RangedWeapon;

private:
	bool bWasOverheated = false;

	// The current heat
	float CurrentHeat = 0.0f;

	// The current spread angle (in degrees, diametrical)
	float CurrentSpreadAngle = 0.0f;

	// The current *combined* spread angle multiplier
	float CurrentSpreadAngleMultiplier = 1.0f;

	// The current standing still multiplier
	float StandingStillMultiplier = 1.0f;

	// The current jumping/falling multiplier
	float JumpFallMultiplier = 1.0f;

	// The current crouching multiplier
	float CrouchingMultiplier = 1.0f;

protected:
	// Multiplier when standing still or moving very slowly
	// (starts to fade out at StandingStillSpeedThreshold, and is gone completely by StandingStillSpeedThreshold + StandingStillToMovingSpeedRange)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
		float SpreadAngleMultiplier_StandingStill = 1.0f;

	// Rate at which we transition to/from the standing still accuracy (higher values are faster, though zero is instant; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
		float TransitionRate_StandingStill = 5.0f;

	// Speeds at or below this are considered standing still
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = "cm/s"))
		float StandingStillSpeedThreshold = 80.0f;

	// Speeds no more than this above StandingStillSpeedThreshold are used to feather down the standing still bonus until it's back to 1.0
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = "cm/s"))
		float StandingStillToMovingSpeedRange = 20.0f;

	// Multiplier when crouching, smoothly blended to based on TransitionRate_Crouching
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
		float SpreadAngleMultiplier_Crouching = 1.0f;

	// Rate at which we transition to/from the crouching accuracy (higher values are faster, though zero is instant; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
		float TransitionRate_Crouching = 5.0f;

	// Spread multiplier while jumping/falling, smoothly blended to based on TransitionRate_JumpingOrFalling
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params", meta = (ForceUnits = x))
		float SpreadAngleMultiplier_JumpingOrFalling = 1.0f;

	// Rate at which we transition to/from the jumping/falling accuracy (higher values are faster, though zero is instant; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Player Params")
		float TransitionRate_JumpingOrFalling = 5.0f;

	// Multiplier when in an aiming camera mode
	UPROPERTY(EditAnywhere, Category = "Spread|Player Params", meta = (ForceUnits = x))
		float SpreadAngleMultiplier_Aiming = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params", meta = (Units = "Percent", ClampMin = "0.0", ClampMax = "100", UIMin = "0.0", UIMax = "100"))
		float EquipHeatAlpha = 50.f;

	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
		bool bCanOverheat;

	// A curve that maps the heat to the spread angle
	// The X range of this curve typically sets the min/max heat range of the weapon
	// The Y range of this curve is used to define the min and maximum spread angle
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
		FRuntimeFloatCurve HeatToSpreadCurve;

	// A curve that maps the current heat to the amount a single shot will further 'heat up'
	// This is typically a flat curve with a single data point indicating how much heat a shot adds,
	// but can be other shapes to do things like punish overheating by adding progressively more heat.
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
		FRuntimeFloatCurve HeatToHeatPerShotCurve;

	// A curve that maps the current heat to the heat cooldown rate per second
	// This is typically a flat curve with a single data point indicating how fast the heat
	// wears off, but can be other shapes to do things like punish overheating by slowing down
	// recovery at high heat.
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params")
		FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

	// Time since firing before spread cooldown recovery begins (in seconds)
	UPROPERTY(EditAnywhere, Category = "Spread|Fire Params", meta = (ForceUnits = s))
		float SpreadRecoveryCooldownDelay = 0.0f;

	// Number of attacks to fire in a attack (typically 1, but may be more for eg. shotguns)
	UPROPERTY(EditAnywhere, Category = "Hit Detection")
		int32 HitsPerAttack = 1;

	UPROPERTY(EditAnywhere, Category = "Hit Detection")
		TEnumAsByte<ECollisionChannel> WeaponTraceChannel;

	UPROPERTY(EditAnywhere, Category = "Hit Detection")
		float HitTraceRadius = 0.f;

public:
	URangedWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaSeconds) override;

	//UEquipmentInstance
	virtual void OnEquipped() override;
	//End UEquipmentInstance
	void AddSpread();

	UFUNCTION(BlueprintPure)
		float GetSpreadAngle(bool bIgnoreMultiplier);

	UFUNCTION(BlueprintPure)
		float GetHeat() const;

	UFUNCTION(BlueprintPure)
		FTransform GetMuzzleTransform() const;
	UFUNCTION(BlueprintPure)
		FVector GetMuzzleLocation() const;

	UFUNCTION(BlueprintPure)
		int32 GetHitsPerAttack() const;
	UFUNCTION(BlueprintPure)
		ETraceTypeQuery GetTraceType() const;
	ECollisionChannel GetTraceChannel() const;
	UFUNCTION(BlueprintPure)
		float GetHitTraceRadius() const;

private:
	void ComputeHeatRange(float& MinHeat, float& MaxHeat);
	inline float ClampHeat(float NewHeat)
	{
		float MinHeat;
		float MaxHeat;
		ComputeHeatRange(/*out*/ MinHeat, /*out*/ MaxHeat);

		if (bCanOverheat) {
			if (bWasOverheated) {
				bWasOverheated = NewHeat <= MinHeat;
			}
			else {
				bWasOverheated = NewHeat >= MaxHeat;
			}
		}

		return FMath::Clamp(NewHeat, MinHeat, MaxHeat);
	}

	// Updates the spread and returns true if the spread is at minimum
	bool UpdateSpread(float DeltaSeconds);

	//// Updates the multipliers and returns true if they are at minimum
	bool UpdateMultipliers(float DeltaSeconds);
};
