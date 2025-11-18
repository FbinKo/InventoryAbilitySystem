// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/WeaponInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

UPhysicalMaterialWithTags::UPhysicalMaterialWithTags(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float UWeaponInstance::GetMaxDamageRange()
{
	if (const FRichCurve* curve = DistanceDamageFalloff.GetRichCurveConst()) {
		float min, max;
		curve->GetTimeRange(min, max);
		return max;
	}
	return 0.0f;
}

float UWeaponInstance::GetDamageAtDistance(float Distance)
{
	if (const FRichCurve* curve = DistanceDamageFalloff.GetRichCurveConst()) {
		return curve->Eval(Distance);
	}
	return 0.0f;
}

float UWeaponInstance::GetDamageMultiplier(const UPhysicalMaterial* PhysicalMaterial)
{
	float CombinedMultiplier = 1.0f;
	if (const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(PhysicalMaterial))
	{
		for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
		{
			if (const float* pTagMultiplier = MaterialDamageMultiplier.Find(MaterialTag))
			{
				CombinedMultiplier *= *pTagMultiplier;
			}
		}
	}
	return CombinedMultiplier;
}

URangedWeaponInstance::URangedWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeatToHeatPerShotCurve.EditorCurveData.AddKey(0.0f, 1.0f);
	HeatToCoolDownPerSecondCurve.EditorCurveData.AddKey(0.0f, 2.0f);
}

void URangedWeaponInstance::Tick(float DeltaSeconds)
{
	APawn* Pawn = GetPawn();
	check(Pawn != nullptr);

	const bool bMinSpread = UpdateSpread(DeltaSeconds);
	const bool bMinMultipliers = UpdateMultipliers(DeltaSeconds);
}

void URangedWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	// Start heat in the middle
	float MinHeatRange;
	float MaxHeatRange;
	ComputeHeatRange(/*out*/ MinHeatRange, /*out*/ MaxHeatRange);
	CurrentHeat = (MinHeatRange + MaxHeatRange) * (EquipHeatAlpha / 100.f);

	// Derive spread
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);

	// Default the multipliers to 1x
	CurrentSpreadAngleMultiplier = 1.0f;
	StandingStillMultiplier = 1.0f;
	JumpFallMultiplier = 1.0f;
	CrouchingMultiplier = 1.0f;
}

void URangedWeaponInstance::AddSpread()
{
	// Sample the heat up curve
	const float HeatPerShot = HeatToHeatPerShotCurve.GetRichCurveConst()->Eval(CurrentHeat);
	CurrentHeat = ClampHeat(CurrentHeat + HeatPerShot);

	// Map the heat to the spread angle
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);
}

float URangedWeaponInstance::GetSpreadAngle(bool bIgnoreMultiplier)
{
	return bIgnoreMultiplier ? CurrentSpreadAngle : CurrentSpreadAngle * CurrentSpreadAngleMultiplier;
}

float URangedWeaponInstance::GetHeat() const
{
	return CurrentHeat;
}

FTransform URangedWeaponInstance::GetMuzzleTransform() const
{
	if (GetSpawnedActors().Num() > 0) {
		return GetSpawnedActors()[0]->GetRootComponent()->GetSocketTransform("Muzzle");
	}
	return FTransform();
}

FVector URangedWeaponInstance::GetMuzzleLocation() const
{
	if (GetSpawnedActors().Num() > 0) {
		return GetSpawnedActors()[0]->GetRootComponent()->GetSocketLocation("Muzzle");
	}
	return FVector();
}

int32 URangedWeaponInstance::GetHitsPerAttack() const
{
	return HitsPerAttack;
}

ETraceTypeQuery URangedWeaponInstance::GetTraceType() const
{
	return UEngineTypes::ConvertToTraceType(WeaponTraceChannel);
}

ECollisionChannel URangedWeaponInstance::GetTraceChannel() const
{
	return WeaponTraceChannel;
}

float URangedWeaponInstance::GetHitTraceRadius() const
{
	return HitTraceRadius;
}

void URangedWeaponInstance::ComputeHeatRange(float& MinHeat, float& MaxHeat)
{
	float Min1;
	float Max1;
	HeatToHeatPerShotCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min1, /*out*/ Max1);

	float Min2;
	float Max2;
	HeatToCoolDownPerSecondCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min2, /*out*/ Max2);

	float Min3;
	float Max3;
	HeatToSpreadCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min3, /*out*/ Max3);

	MinHeat = FMath::Min(FMath::Min(Min1, Min2), Min3);
	MaxHeat = FMath::Max(FMath::Max(Max1, Max2), Max3);
}

bool URangedWeaponInstance::UpdateSpread(float DeltaSeconds)
{
	const float TimeSinceFired = GetWorld()->TimeSince(GetTimeLastUsed());

	if (TimeSinceFired > SpreadRecoveryCooldownDelay)
	{
		const float CooldownRate = HeatToCoolDownPerSecondCurve.GetRichCurveConst()->Eval(CurrentHeat);
		CurrentHeat = ClampHeat(CurrentHeat - (CooldownRate * DeltaSeconds));
		CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat);
	}

	float MinSpread;
	float MaxSpread;
	HeatToSpreadCurve.GetRichCurveConst()->GetValueRange(/*out*/ MinSpread, /*out*/ MaxSpread);

	return FMath::IsNearlyEqual(CurrentSpreadAngle, MinSpread, KINDA_SMALL_NUMBER);
}

bool URangedWeaponInstance::UpdateMultipliers(float DeltaSeconds)
{
	const float MultiplierNearlyEqualThreshold = 0.05f;

	APawn* Pawn = GetPawn();
	check(Pawn != nullptr);
	UCharacterMovementComponent* CharMovementComp = Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent());

	// See if we are standing still, and if so, smoothly apply the bonus
	const float PawnSpeed = Pawn->GetVelocity().Size();
	const float MovementTargetValue = FMath::GetMappedRangeValueClamped(
		/*InputRange=*/ FVector2D(StandingStillSpeedThreshold, StandingStillSpeedThreshold + StandingStillToMovingSpeedRange),
		/*OutputRange=*/ FVector2D(SpreadAngleMultiplier_StandingStill, 1.0f),
		/*Alpha=*/ PawnSpeed);
	StandingStillMultiplier = FMath::FInterpTo(StandingStillMultiplier, MovementTargetValue, DeltaSeconds, TransitionRate_StandingStill);
	const bool bStandingStillMultiplierAtMin = FMath::IsNearlyEqual(StandingStillMultiplier, SpreadAngleMultiplier_StandingStill, SpreadAngleMultiplier_StandingStill * 0.1f);

	// See if we are crouching, and if so, smoothly apply the bonus
	const bool bIsCrouching = (CharMovementComp != nullptr) && CharMovementComp->IsCrouching();
	const float CrouchingTargetValue = bIsCrouching ? SpreadAngleMultiplier_Crouching : 1.0f;
	CrouchingMultiplier = FMath::FInterpTo(CrouchingMultiplier, CrouchingTargetValue, DeltaSeconds, TransitionRate_Crouching);
	const bool bCrouchingMultiplierAtTarget = FMath::IsNearlyEqual(CrouchingMultiplier, CrouchingTargetValue, MultiplierNearlyEqualThreshold);

	// See if we are in the air (jumping/falling), and if so, smoothly apply the penalty
	const bool bIsJumpingOrFalling = (CharMovementComp != nullptr) && CharMovementComp->IsFalling();
	const float JumpFallTargetValue = bIsJumpingOrFalling ? SpreadAngleMultiplier_JumpingOrFalling : 1.0f;
	JumpFallMultiplier = FMath::FInterpTo(JumpFallMultiplier, JumpFallTargetValue, DeltaSeconds, TransitionRate_JumpingOrFalling);
	const bool bJumpFallMultiplerIs1 = FMath::IsNearlyEqual(JumpFallMultiplier, 1.0f, MultiplierNearlyEqualThreshold);

	// Determine if we are aiming down sights, and apply the bonus based on how far into the camera transition we are
	float AimingAlpha = 0.0f;
	//TODO
	//if (const ULyraCameraComponent* CameraComponent = ULyraCameraComponent::FindCameraComponent(Pawn))
	//{
	//	float TopCameraWeight;
	//	FGameplayTag TopCameraTag;
	//	CameraComponent->GetBlendInfo(/*out*/ TopCameraWeight, /*out*/ TopCameraTag);

	//	AimingAlpha = (TopCameraTag == TAG_Lyra_Weapon_SteadyAimingCamera) ? TopCameraWeight : 0.0f;
	//}
	const float AimingMultiplier = FMath::GetMappedRangeValueClamped(
		/*InputRange=*/ FVector2D(0.0f, 1.0f),
		/*OutputRange=*/ FVector2D(1.0f, SpreadAngleMultiplier_Aiming),
		/*Alpha=*/ AimingAlpha);
	const bool bAimingMultiplierAtTarget = FMath::IsNearlyEqual(AimingMultiplier, SpreadAngleMultiplier_Aiming, KINDA_SMALL_NUMBER);

	// Combine all the multipliers
	const float CombinedMultiplier = AimingMultiplier * StandingStillMultiplier * CrouchingMultiplier * JumpFallMultiplier;
	CurrentSpreadAngleMultiplier = CombinedMultiplier;

	// need to handle these spread multipliers indicating we are not at min spread
	return bStandingStillMultiplierAtMin && bCrouchingMultiplierAtTarget && bJumpFallMultiplerIs1 && bAimingMultiplierAtTarget;
}
