// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryCosmeticComponent.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Cosmetic_AnimationStyle, "Cosmetic.AnimationStyle");
UE_DEFINE_GAMEPLAY_TAG(TAG_Cosmetic_BodyStyle, "Cosmetic.BodyStyle");

FGameplayTagContainer UInventoryCosmeticComponent::GetCombinedTags(FGameplayTag RequiredPrefix) const
{
	return AnimationStyleTags;
}
