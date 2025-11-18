// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanActivateAbility.generated.h"

/**
 * 
 */
UCLASS()
class UBTDecorator_CanActivateAbility : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = GameplayTagCheck)
		struct FBlackboardKeySelector ActorToCheck;

	UPROPERTY(EditAnywhere, Category = GameplayTagCheck)
		EGameplayContainerMatchType TagsToMatch;

	UPROPERTY(EditAnywhere, Category = GameplayTagCheck)
		FGameplayTagContainer GameplayTags;

	UPROPERTY()
		FString CachedDescription;

public:
	INVENTORYABILITYSYSTEM_API virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	INVENTORYABILITYSYSTEM_API virtual FString GetStaticDescription() const override;
	
protected:
#if WITH_EDITOR
	/** describe decorator and cache it */
	INVENTORYABILITYSYSTEM_API virtual void BuildDescription();

	INVENTORYABILITYSYSTEM_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	INVENTORYABILITYSYSTEM_API virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
