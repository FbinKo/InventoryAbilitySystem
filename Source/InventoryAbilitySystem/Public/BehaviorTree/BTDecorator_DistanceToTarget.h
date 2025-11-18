// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyEnums.h"
#include "BTDecorator_DistanceToTarget.generated.h"

/**
 * 
 */
UCLASS()
class UBTDecorator_DistanceToTarget : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = DistanceCheck)
		struct FBlackboardKeySelector ActorToCheck;

	UPROPERTY(EditAnywhere, Category = DistanceCheck)
		struct FBlackboardKeySelector Target;

	UPROPERTY(EditAnywhere, Category = Condition)
		float Distance;

	UPROPERTY(EditAnywhere, Category = Condition)
		TEnumAsByte<EArithmeticKeyOperation::Type> comparisonType;

	UPROPERTY(EditAnywhere, Category = Condition)
		FAIDistanceType geometricDistanceType;

	UPROPERTY()
		FString CachedDescription;

public:
	INVENTORYABILITYSYSTEM_API virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	INVENTORYABILITYSYSTEM_API virtual FString GetStaticDescription() const override;
	
protected:
	INVENTORYABILITYSYSTEM_API FVector::FReal GetGeometricDistanceSquared(const FVector& A, const FVector& B) const;
	INVENTORYABILITYSYSTEM_API bool GetLocation(const class UBlackboardComponent* blackboard, FBlackboardKeySelector key, FVector& location) const;
#if WITH_EDITOR
	/** describe decorator and cache it */
	INVENTORYABILITYSYSTEM_API virtual void BuildDescription();

	INVENTORYABILITYSYSTEM_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	INVENTORYABILITYSYSTEM_API virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
