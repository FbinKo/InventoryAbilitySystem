// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryAbilitySystemComponent.h"
#include "Ability/InventoryGlobalAbilitySystem.h"

DEFINE_LOG_CATEGORY(LogInventoryAbilitySystem);

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

UInventoryAbilitySystemComponent::UInventoryAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ClearAbilityInput();

	FMemory::Memset(ActivationGroupCounts, 0, sizeof(ActivationGroupCounts));
}

void UInventoryAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInventoryGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UInventoryGlobalAbilitySystem>(GetWorld()))
	{
		GlobalAbilitySystem->UnregisterASC(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UInventoryAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bHasNewPawnAvatar)
	{
		// Notify all abilities that a new pawn avatar has been set
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			for (UGameplayAbility* AbilityInstance : Instances)
			{
				UInventoryGameplayAbility* InventoryAbilityInstance = Cast<UInventoryGameplayAbility>(AbilityInstance);
				if (InventoryAbilityInstance)
				{
					// Ability instances may be missing for replays
					InventoryAbilityInstance->OnPawnAvatarSet();
				}
			}
		}

		// Register with the global system once we actually have a pawn avatar. We wait until this time since some globally-applied effects may require an avatar.
		if (UInventoryGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UInventoryGlobalAbilitySystem>(GetWorld()))
		{
			GlobalAbilitySystem->RegisterASC(this);
		}

		TryActivateAbilitiesOnSpawn();
	}
}

void UInventoryAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility)
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		UInventoryGameplayAbility* AbilityCDO = Cast<UInventoryGameplayAbility>(AbilitySpec.Ability);
		if (!AbilityCDO)
		{
			UE_LOG(LogInventoryAbilitySystem, Error, TEXT("CancelAbilitiesByFunc: Ability %s was granted but is not an InventoryGameplayAbility."), *AbilitySpec.Ability.GetName());
			continue;
		}

		// Cancel all the spawned instances, not the CDO.
		TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
		for (UGameplayAbility* AbilityInstance : Instances)
		{
			UInventoryGameplayAbility* InventoryAbilityInstance = CastChecked<UInventoryGameplayAbility>(AbilityInstance);

			if (ShouldCancelFunc(InventoryAbilityInstance, AbilitySpec.Handle))
			{
				if (InventoryAbilityInstance->CanBeCanceled())
				{
					InventoryAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), InventoryAbilityInstance->GetCurrentActivationInfo(), bReplicateCancelAbility);
				}
				else
				{
					UE_LOG(LogInventoryAbilitySystem, Error, TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."), *AbilityInstance->GetName());
				}
			}
		}
	}
}

void UInventoryAbilitySystemComponent::CancelInputActivatedAbilities(bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [this](const UInventoryGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle)
		{
			const EInventoryAbilityActivationPolicy ActivationPolicy = Ability->GetActivationPolicy();
			return ((ActivationPolicy == EInventoryAbilityActivationPolicy::OnInputTriggered) || (ActivationPolicy == EInventoryAbilityActivationPolicy::WhileInputActive));
		};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UInventoryAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void UInventoryAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void UInventoryAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked))
	{
		ClearAbilityInput();
		return;
	}

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	//
	// Process all abilities that activate when the input is held.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			// GAS doesnt allow tick for instanced per actor -> if tick desired, use delay wait node
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const UInventoryGameplayAbility* AbilityCDO = CastChecked<UInventoryGameplayAbility>(AbilitySpec->Ability);

				if (AbilityCDO->GetActivationPolicy() == EInventoryAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	//
	// Process all abilities that had their input pressed this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
					const UInventoryGameplayAbility* AbilityCDO = CastChecked<UInventoryGameplayAbility>(AbilitySpec->Ability);

					if (AbilityCDO->GetActivationPolicy() == EInventoryAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	//
	// Try to activate all the abilities that are from presses and holds.
	// We do it all at once so that held inputs don't activate the ability
	// and then also send a input event to the ability because of the press.
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	//
	// Process all abilities that had their input released this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	//
	// Clear the cached ability handles.
	//
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UInventoryAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

bool UInventoryAbilitySystemComponent::IsActivationGroupBlocked(EInventoryAbilityActivationGroup Group) const
{
	bool bBlocked = false;

	switch (Group)
	{
	case EInventoryAbilityActivationGroup::Independent:
		// Independent abilities are never blocked.
		bBlocked = false;
		break;

	case EInventoryAbilityActivationGroup::Exclusive_Replaceable:
	case EInventoryAbilityActivationGroup::Exclusive_Blocking:
		// Exclusive abilities can activate if nothing is blocking.
		bBlocked = (ActivationGroupCounts[(uint8)EInventoryAbilityActivationGroup::Exclusive_Blocking] > 0);
		break;

	default:
		checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), (uint8)Group);
		break;
	}

	return bBlocked;
}

void UInventoryAbilitySystemComponent::AddAbilityToActivationGroup(EInventoryAbilityActivationGroup Group, UInventoryGameplayAbility* InventoryAbility)
{
	check(InventoryAbility);
	check(ActivationGroupCounts[(uint8)Group] < INT32_MAX);

	ActivationGroupCounts[(uint8)Group]++;

	const bool bReplicateCancelAbility = false;

	switch (Group)
	{
	case EInventoryAbilityActivationGroup::Independent:
		// Independent abilities do not cancel any other abilities.
		break;

	case EInventoryAbilityActivationGroup::Exclusive_Replaceable:
	case EInventoryAbilityActivationGroup::Exclusive_Blocking:
		CancelActivationGroupAbilities(EInventoryAbilityActivationGroup::Exclusive_Replaceable, InventoryAbility, bReplicateCancelAbility);
		break;

	default:
		checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), (uint8)Group);
		break;
	}

	const int32 ExclusiveCount = ActivationGroupCounts[(uint8)EInventoryAbilityActivationGroup::Exclusive_Replaceable] + ActivationGroupCounts[(uint8)EInventoryAbilityActivationGroup::Exclusive_Blocking];
	if (!ensure(ExclusiveCount <= 1))
	{
		UE_LOG(LogInventoryAbilitySystem, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
	}
}

void UInventoryAbilitySystemComponent::RemoveAbilityFromActivationGroup(EInventoryAbilityActivationGroup Group, UInventoryGameplayAbility* InventoryAbility)
{
	check(InventoryAbility);
	check(ActivationGroupCounts[(uint8)Group] > 0);

	ActivationGroupCounts[(uint8)Group]--;
}

void UInventoryAbilitySystemComponent::CancelActivationGroupAbilities(EInventoryAbilityActivationGroup Group, UInventoryGameplayAbility* IgnoreInventoryAbility, bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [this, Group, IgnoreInventoryAbility](const UInventoryGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle)
		{
			return ((Ability->GetActivationGroup() == Group) && (Ability != IgnoreInventoryAbility));
		};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UInventoryAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
	TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData = AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
	if (ReplicatedData.IsValid())
	{
		OutTargetDataHandle = ReplicatedData->TargetData;
	}
}

void UInventoryAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		const UInventoryGameplayAbility* AbilityCDO = CastChecked<UInventoryGameplayAbility>(AbilitySpec.Ability);
		AbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
	}
}

void UInventoryAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputPress ability task works.
	if (Spec.IsActive())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
			// Fixing this up to use the instance activation, but this function should be deprecated as it cannot work with InstancedPerExecution
			UE_CLOG(Spec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution, LogInventoryAbilitySystem, Warning, TEXT("%hs: %s is InstancedPerExecution. This is unreliable for Input as you may only interact with the latest spawned Instance"), __func__, *GetNameSafe(Spec.Ability));
		TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
		const FGameplayAbilityActivationInfo& ActivationInfo = Instances.IsEmpty() ? Spec.ActivationInfo : Instances.Last()->GetCurrentActivationInfoRef();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

			// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, ActivationInfo.GetActivationPredictionKey());
	}
}

void UInventoryAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputRelease ability task works.
	if (Spec.IsActive())
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
			// Fixing this up to use the instance activation, but this function should be deprecated as it cannot work with InstancedPerExecution
			UE_CLOG(Spec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution, LogInventoryAbilitySystem, Warning, TEXT("%hs: %s is InstancedPerExecution. This is unreliable for Input as you may only interact with the latest spawned Instance"), __func__, *GetNameSafe(Spec.Ability));
		TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
		const FGameplayAbilityActivationInfo& ActivationInfo = Instances.IsEmpty() ? Spec.ActivationInfo : Instances.Last()->GetCurrentActivationInfoRef();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

			// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputReleased event to the server.
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, ActivationInfo.GetActivationPredictionKey());
	}
}

void UInventoryAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	UInventoryGameplayAbility* InventoryAbility = CastChecked<UInventoryGameplayAbility>(Ability);

	AddAbilityToActivationGroup(InventoryAbility->GetActivationGroup(), InventoryAbility);
}

void UInventoryAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

	if (APawn* Avatar = Cast<APawn>(GetAvatarActor()))
	{
		if (!Avatar->IsLocallyControlled() && Ability->IsSupportedForNetworking())
		{
			ClientNotifyAbilityFailed(Ability, FailureReason);
			return;
		}
	}

	HandleAbilityFailed(Ability, FailureReason);
}

void UInventoryAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	UInventoryGameplayAbility* InventoryAbility = CastChecked<UInventoryGameplayAbility>(Ability);

	RemoveAbilityFromActivationGroup(InventoryAbility->GetActivationGroup(), InventoryAbility);
}

void UInventoryAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, BlockTags, bExecuteCancelTags, CancelTags);

	//TODO: Apply any special logic like blocking input or movement
}

void UInventoryAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
{
	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);

	//TODO: Apply any special logic like blocking input or movement
}

void UInventoryAbilitySystemComponent::ClientNotifyAbilityFailed_Implementation(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	HandleAbilityFailed(Ability, FailureReason);
}

void UInventoryAbilitySystemComponent::HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	if (const UInventoryGameplayAbility* InventoryAbility = Cast<const UInventoryGameplayAbility>(Ability))
	{
		InventoryAbility->OnAbilityFailedToActivate(FailureReason);
	}
}
