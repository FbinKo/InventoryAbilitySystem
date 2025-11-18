// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/Execution/HealExecution.h"
#include "Ability/Attribute/InventoryCombatSet.h"
#include "Ability/Attribute/InventoryHealthSet.h"

UHealExecution::UHealExecution()
{
	RelevantAttributesToCapture.Add(FGameplayEffectAttributeCaptureDefinition(UInventoryCombatSet::GetBaseHealAttribute(), EGameplayEffectAttributeCaptureSource::Source, true));
}

void UHealExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	float BaseHeal = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(FGameplayEffectAttributeCaptureDefinition(UInventoryCombatSet::GetBaseHealAttribute(), EGameplayEffectAttributeCaptureSource::Source, true), EvaluateParameters, BaseHeal);

	const float HealingDone = FMath::Max(0.0f, BaseHeal);

	if (HealingDone > 0.0f)
	{
		// Apply a healing modifier, this gets turned into + health on the target
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UInventoryHealthSet::GetHealingAttribute(), EGameplayModOp::Additive, HealingDone));
	}
#endif // #if WITH_SERVER_CODE
}
