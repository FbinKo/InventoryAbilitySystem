// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentInstance.h"
#include "Equipment/EquipmentComponent.h"
#include "Inventory/InventoryCosmeticComponent.h"
#include "Inventory/IInventory.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UEquipmentInstance::UEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EquipmentLayerPrefix = TAG_Cosmetic_AnimationStyle;
}

void UEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Instigator);
	DOREPLIFETIME(ThisClass, SpawnedActors);
}

void UEquipmentInstance::SpawnEquipmentActors(const TArray<FEquipmentActorToSpawn>& ActorsToSpawn)
{
	if (APawn* OwningPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
		if (OwningPawn->Implements<UInventory>()) {
			if (USceneComponent* inventoryComponent = IInventory::Execute_GetEquipmentAttachmentComponent(OwningPawn))
				AttachTarget = inventoryComponent;
		}
		else if (ACharacter* Char = Cast<ACharacter>(OwningPawn))
		{
			AttachTarget = Char->GetMesh();
		}

		for (const FEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
		{
			AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, OwningPawn, OwningPawn);
			NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
			NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);

			SpawnedActors.Add(NewActor);
		}
	}
}

void UEquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
}

void UEquipmentInstance::OnEquipped()
{
	USkeletalMeshComponent* mesh = GetPawn()->GetComponentByClass<USkeletalMeshComponent>();
	if (mesh) {
		if (UInventoryCosmeticComponent* cosmeticComponent = GetPawn()->GetComponentByClass<UInventoryCosmeticComponent>())
			CosmeticAnimationStyleTags = cosmeticComponent->GetCombinedTags(EquipmentLayerPrefix);
		if (TSubclassOf<UAnimInstance> EquipmentLayer = EquipmentLayerSet.SelectBestLayer(CosmeticAnimationStyleTags))
			mesh->LinkAnimClassLayers(EquipmentLayer);
		if (UAnimInstance* animInstance = mesh->GetAnimInstance())
			animInstance->Montage_Play(EquipMontage);
	}

	K2_OnEquipped();

	UWorld* World = GetWorld();
	check(World);
	TimeLastEquipped = World->GetTimeSeconds();
}

void UEquipmentInstance::OnUnequipped()
{
	USkeletalMeshComponent* mesh = GetPawn()->GetComponentByClass<USkeletalMeshComponent>();
	if (mesh) {
		if (TSubclassOf<UAnimInstance> EquipmentLayer = EquipmentLayerSet.SelectBestLayer(CosmeticAnimationStyleTags))
			mesh->UnlinkAnimClassLayers(EquipmentLayer);
		if (UAnimInstance* animInstance = mesh->GetAnimInstance())
			animInstance->Montage_Play(UnequipMontage);
	}

	K2_OnUnequipped();
}

void UEquipmentInstance::UpdateUseTime()
{
	UWorld* World = GetWorld();
	check(World);
	TimeLastUsed = World->GetTimeSeconds();
}

float UEquipmentInstance::GetTimeLastInteracted()
{
	UWorld* World = GetWorld();
	check(World);
	const double WorldTime = World->GetTimeSeconds();

	double Result = WorldTime - TimeLastEquipped;

	if (TimeLastUsed > 0.0)
	{
		const double TimeSinceFired = WorldTime - TimeLastUsed;
		Result = FMath::Min(Result, TimeSinceFired);
	}

	return Result;
}

void UEquipmentInstance::OnRep_Instigator()
{
}

TSubclassOf<UAnimInstance> FInventoryAnimLayerSelectionSet::SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const
{
	for (const FInventoryAnimLayerSelectionEntry& Rule : LayerRules)
	{
		if ((Rule.Layer != nullptr) && CosmeticTags.HasAll(Rule.RequiredTags))
		{
			return Rule.Layer;
		}
	}

	return DefaultLayer;
}
