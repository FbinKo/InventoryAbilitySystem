// Fill out your copyright notice in the Description page of Project Settings.


#include "BehaviorTree/BTDecorator_CanActivateAbility.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AIController.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_CanActivateAbility::UBTDecorator_CanActivateAbility(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "Can Activate Ability Condition";

	// Accept only actors
	ActorToCheck.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_CanActivateAbility, ActorToCheck), AActor::StaticClass());

	// Default to using Self Actor
	ActorToCheck.SelectedKeyName = FBlackboard::KeySelf;

	// For now, don't allow users to select any "Abort Observers", because it's currently not supported.
	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

bool UBTDecorator_CanActivateAbility::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == NULL)
	{
		return false;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(ActorToCheck.GetSelectedKeyID()));
	if (AbilitySystemInterface == NULL)
	{
		return false;
	}

	if (UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent()) {
		TArray<FGameplayAbilitySpecHandle> specs;
		ASC->FindAllAbilitiesWithTags(specs, GameplayTags, (bool)TagsToMatch);

		for (FGameplayAbilitySpecHandle handle : specs) {
			if (handle.IsValid()) {
				FGameplayAbilitySpec* spec = ASC->FindAbilitySpecFromHandle(handle);
				// try to get the ability instance
				if (UGameplayAbility* AbilityInstance = spec->GetPrimaryInstance()) {
					return AbilityInstance->CanActivateAbility(handle, ASC->AbilityActorInfo.Get());
				}
				else {
					// fallback to CDO
					return spec->Ability->CanActivateAbility(handle, ASC->AbilityActorInfo.Get());
				}
			}
		}
	}

	return false;
}

FString UBTDecorator_CanActivateAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *CachedDescription);
}

#if WITH_EDITOR
void UBTDecorator_CanActivateAbility::BuildDescription()
{
	CachedDescription = GameplayTags.ToMatchingText(TagsToMatch, IsInversed()).ToString();

}

void UBTDecorator_CanActivateAbility::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property == NULL)
	{
		return;
	}

	BuildDescription();
}
#endif //WITH_EDITOR

void UBTDecorator_CanActivateAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	UBlackboardData* BBAsset = GetBlackboardAsset();
	if (ensure(BBAsset))
	{
		ActorToCheck.ResolveSelectedKey(*BBAsset);
	}

#if WITH_EDITOR
	BuildDescription();
#endif	// WITH_EDITOR

}
