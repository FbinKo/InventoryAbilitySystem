// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryGameplayAbility_Weapon.h"
#include "Equipment/WeaponInstance.h"
#include "Ability/InventoryAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace InventoryConsoleVariables
{
	static float DrawBulletTracesDuration = 0.0f;
	static FAutoConsoleVariableRef CVarDrawBulletTraceDuraton(
		TEXT("Inventory.Weapon.DrawBulletTraceDuration"),
		DrawBulletTracesDuration,
		TEXT("Should we do debug drawing for bullet traces (if above zero, sets how long (in seconds))"),
		ECVF_Default);

	static float DrawBulletHitDuration = 0.0f;
	static FAutoConsoleVariableRef CVarDrawBulletHits(
		TEXT("Inventory.Weapon.DrawBulletHitDuration"),
		DrawBulletHitDuration,
		TEXT("Should we do debug drawing for bullet impacts (if above zero, sets how long (in seconds))"),
		ECVF_Default);

	static float DrawBulletHitRadius = 3.0f;
	static FAutoConsoleVariableRef CVarDrawBulletHitRadius(
		TEXT("Inventory.Weapon.DrawBulletHitRadius"),
		DrawBulletHitRadius,
		TEXT("When bullet hit debug drawing is enabled (see DrawBulletHitDuration), how big should the hit radius be? (in uu)"),
		ECVF_Default);
}

// Weapon fire will be blocked/canceled if the player has this tag
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_WeaponFireBlocked, "Ability.Weapon.NoFiring");

UInventoryGameplayAbility_Weapon::UInventoryGameplayAbility_Weapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationBlockedTags.AddTag(TAG_WeaponFireBlocked);
}

bool UInventoryGameplayAbility_Weapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	if (bResult)
	{
		if (IsInstantiated() && GetWeaponInstance<UWeaponInstance>() == nullptr)
		{
			UE_LOG(LogInventoryAbilitySystem, Error, TEXT("Weapon ability %s cannot be activated because there is no associated ranged weapon (equipment instance=%s but needs to be derived from %s)"),
				*GetPathName(),
				*GetPathNameSafe(GetAssociatedEquipment()),
				*UWeaponInstance::StaticClass()->GetName());
			bResult = false;
		}
	}

	return bResult;
}

UWeaponInstance* UInventoryGameplayAbility_Weapon::GetWeaponInstance(TSubclassOf<UWeaponInstance> WeaponInstanceClass) const
{
	return GetWeaponInstance<UWeaponInstance>();
}

void UInventoryGameplayAbility_Weapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Bind target data callback
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	OnTargetDataReadyCallbackDelegateHandle = MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	// Update the last use time
	UWeaponInstance* WeaponData = GetWeaponInstance<UWeaponInstance>();
	check(WeaponData);
	WeaponData->UpdateUseTime();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UInventoryGameplayAbility_Weapon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		if (ScopeLockCount > 0)
		{
			WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility, Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
			return;
		}

		UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
		check(MyAbilityComponent);

		// When ability ends, consume target data and remove delegate
		MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(OnTargetDataReadyCallbackDelegateHandle);
		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
}

void UInventoryGameplayAbility_Weapon::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	if (const FGameplayAbilitySpec* AbilitySpec = MyAbilityComponent->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		FScopedPredictionWindow	ScopedPrediction(MyAbilityComponent);

		// Take ownership of the target data to make sure no callbacks into game code invalidate it out from under us
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

		const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority();
		if (bShouldNotifyServer)
		{
			MyAbilityComponent->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), LocalTargetDataHandle, ApplicationTag, MyAbilityComponent->ScopedPredictionKey);
		}

		const bool bIsTargetDataValid = true;

		// See if we still have required ressources
		if (bIsTargetDataValid && CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{
			PostCommitAbility();

			// Let the blueprint do stuff like apply effects to the targets
			OnWeaponTargetDataReady(LocalTargetDataHandle);
		}
		else
		{
			UE_LOG(LogInventoryAbilitySystem, Warning, TEXT("Weapon ability %s failed to commit (bIsTargetDataValid=%d)"), *GetPathName(), bIsTargetDataValid ? 1 : 0);
			K2_EndAbility();
		}
	}

	// We've processed the data
	MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

void UInventoryGameplayAbility_Weapon::StartWeaponTargeting()
{
	check(CurrentActorInfo);

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	check(AvatarActor);

	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	FScopedPredictionWindow ScopedPrediction(MyAbilityComponent, CurrentActivationInfo.GetActivationPredictionKey());

	TArray<FHitResult> FoundHits;
	PerformLocalTargeting(/*out*/ FoundHits);

	// Fill out the target data from the hit results
	FGameplayAbilityTargetDataHandle TargetData;
	TargetData.UniqueId = 0;

	if (FoundHits.Num() > 0)
	{
		for (const FHitResult& FoundHit : FoundHits)
		{
			FGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FGameplayAbilityTargetData_SingleTargetHit();
			NewTargetData->HitResult = FoundHit;

			TargetData.Add(NewTargetData);
		}
	}

	// Process the target data immediately
	OnTargetDataReadyCallback(TargetData, FGameplayTag());
}
