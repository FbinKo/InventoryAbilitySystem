// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability/InventoryAbilityCost.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_ABILITY_FAIL_COST, "Ability.ActivateFail.Cost");

UInventoryAbilityCost::UInventoryAbilityCost()
{
	Quantity.SetValue(1.0f);
	FailureTag = TAG_ABILITY_FAIL_COST;
}