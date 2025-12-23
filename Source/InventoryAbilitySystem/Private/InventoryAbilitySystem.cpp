// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryAbilitySystem.h"
#include "GameplayTagsManager.h"

#define LOCTEXT_NAMESPACE "FInventoryAbilitySystemModule"

void FInventoryAbilitySystemModule::StartupModule()
{
	UGameplayTagsManager::Get().AddTagIniSearchPath(FPaths::ProjectPluginsDir() / TEXT("InventoryAbilitySystem/Config/Tags"));
}

void FInventoryAbilitySystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInventoryAbilitySystemModule, InventoryAbilitySystem)