// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentComponent.h"
#include "Equipment/EquipmentInstance.h"
#include "Inventory/InventoryItemInstance.h"
#include "Inventory/InventoryComponent.h"
#include "Ability/InventoryAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UEquipmentDefinition::UEquipmentDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstanceType = UEquipmentInstance::StaticClass();
}

UEquipmentComponent::UEquipmentComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
}

void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (FAppliedEquipmentEntry& entry : equipmentList) {
		if (UEquipmentInstance* instance = entry.instance) {
			instance->Tick(DeltaTime);
		}
	}
}

void UEquipmentComponent::UninitializeComponent()
{
	UnequipEverything();

	Super::UninitializeComponent();
}

UEquipmentInstance* UEquipmentComponent::EquipItemDefinition(TSubclassOf<UEquipmentDefinition> EquipmentDefinition)
{
	UEquipmentInstance* Result = nullptr;
	if (EquipmentDefinition)
	{
		const UEquipmentDefinition* EquipmentCDO = GetDefault<UEquipmentDefinition>(EquipmentDefinition);

		TSubclassOf<UEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
		if (InstanceType == nullptr)
		{
			InstanceType = UEquipmentInstance::StaticClass();
		}

		FAppliedEquipmentEntry& NewEntry = equipmentList.AddDefaulted_GetRef();
		NewEntry.equipmentDefinition = EquipmentDefinition;
		NewEntry.instance = NewObject<UEquipmentInstance>(GetOwner(), InstanceType);
		Result = NewEntry.instance;

		if (UInventoryAbilitySystemComponent* component = GetAbilitySystemComponent())
		{
			for (TObjectPtr<const UAbilitySet> AbilitySet : EquipmentCDO->AbilitySetsToGrant)
			{
				AbilitySet->GiveToAbilitySystem(component, /*inout*/ &NewEntry.grantedHandles, Result);
			}
		}

		Result->SpawnEquipmentActors(EquipmentCDO->ActorsToSpawn);

		if (Result != nullptr)
		{
			Result->OnEquipped();

			if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
			{
				AddReplicatedSubObject(Result);
			}
		}
	}

	OnEquip.Broadcast(Result);
	return Result;
}

void UEquipmentComponent::EquipItemInstance(UInventoryItemInstance* instance)
{
	if (instance) {

		if (const UInventoryFragment_EquippableItem* EquipInfo = instance->FindFragmentByClass<UInventoryFragment_EquippableItem>())
		{
			TSubclassOf<UEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef != nullptr)
			{
				TObjectPtr<UEquipmentInstance> NewEquipment = EquipItemDefinition(EquipDef);
				if (NewEquipment)
				{
					NewEquipment->SetInstigator(instance);
				}
			}
		}
		return;
	}
}

void UEquipmentComponent::UnequipItem(UEquipmentInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(ItemInstance);
		}

		ItemInstance->OnUnequipped();
		OnUnequip.Broadcast(ItemInstance);

		for (auto EntryIt = equipmentList.CreateIterator(); EntryIt; ++EntryIt)
		{
			FAppliedEquipmentEntry& Entry = *EntryIt;
			if (Entry.instance == ItemInstance)
			{
				if (UInventoryAbilitySystemComponent* component = GetAbilitySystemComponent())
				{
					Entry.grantedHandles.TakeFromAbilitySystem(component);
				}

				ItemInstance->DestroyEquipmentActors();


				EntryIt.RemoveCurrent();
			}
		}
	}
}

void UEquipmentComponent::UnequipEverything()
{
	TArray<UEquipmentInstance*> AllEquipmentInstances;

	// gathering all instances before removal to avoid side effects affecting the equipment list iterator	
	for (const FAppliedEquipmentEntry& Entry : equipmentList)
	{
		AllEquipmentInstances.Add(Entry.instance);
	}

	for (UEquipmentInstance* EquipInstance : AllEquipmentInstances)
	{
		UnequipItem(EquipInstance);
	}
}

UEquipmentInstance* UEquipmentComponent::GetFirstInstanceOfType(TSubclassOf<UEquipmentInstance> InstanceType)
{
	for (FAppliedEquipmentEntry& Entry : equipmentList)
	{
		if (UEquipmentInstance* Instance = Entry.instance)
		{
			if (Instance->IsA(InstanceType))
			{
				return Instance;
			}
		}
	}
	return nullptr;
}

TArray<UEquipmentInstance*> UEquipmentComponent::GetEquipmentInstancesOfType(TSubclassOf<UEquipmentInstance> InstanceType) const
{
	TArray<UEquipmentInstance*> Results;
	for (const FAppliedEquipmentEntry& Entry : equipmentList)
	{
		if (UEquipmentInstance* Instance = Entry.instance)
		{
			if (Instance->IsA(InstanceType))
			{
				Results.Add(Instance);
			}
		}
	}
	return Results;
}

int32 UEquipmentComponent::GetNextFreeSlot()
{
	int32 SlotIndex = 0;
	for (TObjectPtr<UInventoryItemInstance> ItemPtr : slots)
	{
		if (ItemPtr == nullptr)
		{
			return SlotIndex;
		}
		++SlotIndex;
	}

	return INDEX_NONE;
}

void UEquipmentComponent::AddItemToSlot(int32 slotId, UInventoryItemInstance* item)
{
	if (slots.Num() < numSlots)
		slots.AddDefaulted(numSlots - slots.Num());

	if (slots.IsValidIndex(slotId) && item) {
		slots[slotId] = item;
	}
}

UInventoryItemInstance* UEquipmentComponent::RemoveItemFromSlot(int32 slotId)
{
	UInventoryItemInstance* Result = nullptr;

	if (activeSlotIndex == slotId)
	{
		UnequipItemInSlot();
		activeSlotIndex = -1;
	}

	if (slots.IsValidIndex(slotId))
	{
		Result = slots[slotId];

		if (Result != nullptr)
		{
			slots[slotId] = nullptr;
		}
	}

	return Result;
}

void UEquipmentComponent::SetActiveSlotIndex(int32 newId)
{
	if (slots.IsValidIndex(newId) && (activeSlotIndex != newId)) {
		UnequipItemInSlot();

		activeSlotIndex = newId;

		EquipItemInSlot();
	}
}

void UEquipmentComponent::CycleActiveSlotForward()
{
	if (slots.Num() < numSlots)
	{
		return;
	}

	const int32 OldIndex = (activeSlotIndex < 0 ? slots.Num() - 1 : activeSlotIndex);
	int32 NewIndex = activeSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % slots.Num();
		if (slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	} while (NewIndex != OldIndex);
}

void UEquipmentComponent::CycleActiveSlotBackward()
{
	if (slots.Num() < numSlots)
	{
		return;
	}

	const int32 OldIndex = (activeSlotIndex < 0 ? slots.Num() - 1 : activeSlotIndex);
	int32 NewIndex = activeSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + slots.Num()) % slots.Num();
		if (slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	} while (NewIndex != OldIndex);
}

UInventoryAbilitySystemComponent* UEquipmentComponent::GetAbilitySystemComponent() const
{
	return Cast<UInventoryAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
}

void UEquipmentComponent::EquipItemInSlot()
{
	check(slots.IsValidIndex(activeSlotIndex));
	check(equippedItem == nullptr);

	if (UInventoryItemInstance* SlotItem = slots[activeSlotIndex])
	{
		if (const UInventoryFragment_EquippableItem* EquipInfo = SlotItem->FindFragmentByClass<UInventoryFragment_EquippableItem>())
		{
			TSubclassOf<UEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef != nullptr)
			{
				equippedItem = EquipItemDefinition(EquipDef);
				if (equippedItem != nullptr)
				{
					equippedItem->SetInstigator(SlotItem);
				}
			}
		}
	}
}

void UEquipmentComponent::UnequipItemInSlot()
{
	if (equippedItem != nullptr)
	{
		UnequipItem(equippedItem);
		equippedItem = nullptr;
	}
}
