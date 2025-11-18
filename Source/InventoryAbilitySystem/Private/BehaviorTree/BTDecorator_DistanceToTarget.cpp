// Fill out your copyright notice in the Description page of Project Settings.


#include "BehaviorTree/BTDecorator_DistanceToTarget.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_DistanceToTarget::UBTDecorator_DistanceToTarget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "Distance Check";
	Distance = 500.f;
	comparisonType = EArithmeticKeyOperation::LessOrEqual;
	geometricDistanceType = FAIDistanceType::Distance3D;

	// Accept only actors and vectors
	ActorToCheck.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_DistanceToTarget, ActorToCheck), AActor::StaticClass());
	ActorToCheck.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_DistanceToTarget, ActorToCheck));
	Target.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_DistanceToTarget, Target), AActor::StaticClass());
	Target.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_DistanceToTarget, Target));

	// Default to using Self Actor
	ActorToCheck.SelectedKeyName = FBlackboard::KeySelf;

	// For now, don't allow users to select any "Abort Observers", because it's currently not supported.
	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

bool UBTDecorator_DistanceToTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == NULL)
	{
		return false;
	}

	FVector A, B;
	if (GetLocation(BlackboardComp, ActorToCheck, A) && GetLocation(BlackboardComp, Target, B)) {
		float actualDistance = GetGeometricDistanceSquared(A, B);
		float compareDistance = FMath::Square(Distance);
		switch (comparisonType)
		{
		case EArithmeticKeyOperation::Equal:
			return actualDistance == compareDistance;
		case EArithmeticKeyOperation::Greater:
			return actualDistance > compareDistance;
		case EArithmeticKeyOperation::GreaterOrEqual:
			return actualDistance >= compareDistance;
		case EArithmeticKeyOperation::Less:
			return actualDistance < compareDistance;
		case EArithmeticKeyOperation::LessOrEqual:
			return actualDistance <= compareDistance;
		case EArithmeticKeyOperation::NotEqual:
			return actualDistance =! compareDistance;
		default:
			break;
		}
	}
	return false;
}

FString UBTDecorator_DistanceToTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *CachedDescription);
}

FVector::FReal UBTDecorator_DistanceToTarget::GetGeometricDistanceSquared(const FVector& A, const FVector& B) const
{
	FVector::FReal Result = TNumericLimits<FVector::FReal>::Max();

	switch (geometricDistanceType)
	{
	case FAIDistanceType::Distance3D:
		Result = FVector::DistSquared(A, B);
		break;
	case FAIDistanceType::Distance2D:
		Result = FVector::DistSquaredXY(A, B);
		break;
	case FAIDistanceType::DistanceZ:
		Result = FMath::Square(A.Z - B.Z);
		break;
	default:
		checkNoEntry();
		break;
	}
	return Result;
}

bool UBTDecorator_DistanceToTarget::GetLocation(const UBlackboardComponent* blackboard, FBlackboardKeySelector key, FVector& location) const
{
	if (key.SelectedKeyType == UBlackboardKeyType_Object::StaticClass()) {
		UObject* KeyValue = blackboard->GetValue<UBlackboardKeyType_Object>(key.GetSelectedKeyID());
		if (AActor* actor = Cast<AActor>(KeyValue)) {
			location = actor->GetActorLocation();
			return true;
		}
	}
	else if (key.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass()) {
		const FVector TargetLocation = blackboard->GetValue<UBlackboardKeyType_Vector>(key.GetSelectedKeyID());
		if (FAISystem::IsValidLocation(TargetLocation)) {
			location = TargetLocation;
			return true;
		}
	}
	return false;
}

#if WITH_EDITOR
void UBTDecorator_DistanceToTarget::BuildDescription()
{
	FString KeyDesc("invalid");
	if (ActorToCheck.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
		ActorToCheck.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		KeyDesc = ActorToCheck.SelectedKeyName.ToString();
	}

	FString KeyDescTarget("invalid");
	if (Target.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
		Target.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		KeyDescTarget = Target.SelectedKeyName.ToString();
	}

	CachedDescription = FString::Printf(TEXT("%s to %s"), *KeyDesc, *KeyDescTarget);

}

void UBTDecorator_DistanceToTarget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property == NULL)
	{
		return;
	}

	BuildDescription();
}
#endif //WITH_EDITOR

void UBTDecorator_DistanceToTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	UBlackboardData* BBAsset = GetBlackboardAsset();
	if (ensure(BBAsset))
	{
		ActorToCheck.ResolveSelectedKey(*BBAsset);
		Target.ResolveSelectedKey(*BBAsset);
	}

#if WITH_EDITOR
	BuildDescription();
#endif	// WITH_EDITOR

}
