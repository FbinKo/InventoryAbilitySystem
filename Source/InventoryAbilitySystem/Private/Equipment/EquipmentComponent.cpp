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

void UEquipmentComponent::EquipItemInstance(int32 slotId, UInventoryItemInstance* instance)
{
	if (instance) {
		if (const UInventoryFragment_EquippableItem* EquipInfo = instance->FindFragmentByClass<UInventoryFragment_EquippableItem>())
		{
			TObjectPtr<UEquipmentInstance> NewEquipment;
			TSubclassOf<UEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef != nullptr)
			{
				NewEquipment = EquipItemDefinition(EquipDef);
				if (NewEquipment)
				{
					NewEquipment->SetInstigator(instance);
				}
			}

			int32 categoryId = INDEX_NONE;
			if (!equipmentCategories.Find(EquipInfo->GetClass(), categoryId))
			{
				categoryId = equipmentCategories.Emplace(EquipInfo->GetClass());
			}

			TArray<TObjectPtr<UInventoryItemInstance>> categoryItems;
			TArray<TObjectPtr<UEquipmentInstance>> categoryEquipment;
			if (equipmentSlots.IsValidIndex(categoryId))
			{
				categoryItems = equipmentSlots[categoryId];
				categoryEquipment = equippedItems[categoryId];

				if (categoryItems.IsValidIndex(slotId))
				{
					categoryItems[slotId] = instance;
					categoryEquipment[slotId] = NewEquipment;
				}
				else
				{
					categoryItems.EmplaceAt(slotId, instance);
					categoryEquipment.EmplaceAt(slotId, NewEquipment);
				}

				equipmentSlots[categoryId] = categoryItems;
				equippedItems[categoryId] = categoryEquipment;
			}
			else
			{
				categoryItems.EmplaceAt(slotId, instance);
				equipmentSlots.Emplace(categoryItems);
				categoryEquipment.EmplaceAt(slotId, NewEquipment);
				equippedItems.Emplace(categoryEquipment);
			}
		}
		return;
	}
}

UInventoryItemInstance* UEquipmentComponent::RemoveItemInstance(int32 slotId, TSubclassOf<UInventoryFragment_EquippableItem> type)
{
	UInventoryItemInstance* Result = nullptr;
	int32 categoryId = INDEX_NONE;
	if (equipmentCategories.Find(type, categoryId))
	{
		if (TArray<TObjectPtr<UInventoryItemInstance>>* slots = &equipmentSlots[categoryId])
		{
			if (slots->IsValidIndex(slotId))
			{
				Result = (*slots)[slotId];
				if (Result)
				{
					UnequipItem(equippedItems[categoryId][slotId]);
					(*slots)[slotId] = nullptr;
				}
			}
		}
	}
	return Result;
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

TArray<UInventoryItemInstance*> UEquipmentComponent::GetSlots(TSubclassOf<UInventoryFragment_EquippableItem> type) const
{
	if (type->IsChildOf(UInventoryFragment_WeaponItem::StaticClass()))
		return weaponSlots;
	else
	{
		TArray<UInventoryItemInstance*> Results;
		int32 categoryId = INDEX_NONE;
		if (equipmentCategories.Find(type, categoryId))
		{
			if (const TArray<TObjectPtr<UInventoryItemInstance>>* slots = &equipmentSlots[categoryId])
			{
				Results = *slots;
			}
		}
		return Results;
	}
}

int32 UEquipmentComponent::GetNextFreeWeaponSlot()
{
	int32 SlotIndex = 0;
	for (TObjectPtr<UInventoryItemInstance> ItemPtr : weaponSlots)
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
	if (const UInventoryFragment_EquippableItem* EquipInfo = item->FindFragmentByClass<UInventoryFragment_EquippableItem>())
	{
		if (EquipInfo->IsA(UInventoryFragment_WeaponItem::StaticClass()))
		{
			if (weaponSlots.Num() < numWeaponSlots)
				weaponSlots.AddDefaulted(numWeaponSlots - weaponSlots.Num());

			if (weaponSlots.IsValidIndex(slotId) && item)
				weaponSlots[slotId] = item;
		}
		else
			EquipItemInstance(slotId, item);
	}
}

UInventoryItemInstance* UEquipmentComponent::RemoveItemFromSlot(int32 slotId, TSubclassOf<UInventoryFragment_EquippableItem> type)
{
	UInventoryItemInstance* Result = nullptr;

	if (type->IsChildOf(UInventoryFragment_WeaponItem::StaticClass()))
	{
		if (activeSlotIndex == slotId)
		{
			UnequipWeaponInSlot();
			activeSlotIndex = -1;
		}

		if (weaponSlots.IsValidIndex(slotId))
		{
			Result = weaponSlots[slotId];

			if (Result != nullptr)
			{
				weaponSlots[slotId] = nullptr;
			}
		}
	}
	else
	{
		Result = RemoveItemInstance(slotId, type);
	}

	return Result;
}

void UEquipmentComponent::SetActiveSlotIndex(int32 newId)
{
	if (weaponSlots.IsValidIndex(newId) && (activeSlotIndex != newId)) {
		UnequipWeaponInSlot();

		activeSlotIndex = newId;

		EquipWeaponInSlot();

		OnWeaponSlotChanged.Broadcast(activeSlotIndex);
	}
}

void UEquipmentComponent::CycleActiveSlotForward()
{
	if (weaponSlots.Num() < numWeaponSlots)
	{
		return;
	}

	const int32 OldIndex = (activeSlotIndex < 0 ? weaponSlots.Num() - 1 : activeSlotIndex);
	int32 NewIndex = activeSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % weaponSlots.Num();
		if (weaponSlots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	} while (NewIndex != OldIndex);
}

void UEquipmentComponent::CycleActiveSlotBackward()
{
	if (weaponSlots.Num() < numWeaponSlots)
	{
		return;
	}

	const int32 OldIndex = (activeSlotIndex < 0 ? weaponSlots.Num() - 1 : activeSlotIndex);
	int32 NewIndex = activeSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + weaponSlots.Num()) % weaponSlots.Num();
		if (weaponSlots[NewIndex] != nullptr)
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

void UEquipmentComponent::EquipWeaponInSlot()
{
	check(weaponSlots.IsValidIndex(activeSlotIndex));
	check(equippedWeapon == nullptr);

	if (UInventoryItemInstance* SlotItem = weaponSlots[activeSlotIndex])
	{
		if (const UInventoryFragment_EquippableItem* EquipInfo = SlotItem->FindFragmentByClass<UInventoryFragment_EquippableItem>())
		{
			TSubclassOf<UEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef != nullptr)
			{
				equippedWeapon = EquipItemDefinition(EquipDef);
				if (equippedWeapon != nullptr)
				{
					equippedWeapon->SetInstigator(SlotItem);
				}
			}
		}
	}
}

void UEquipmentComponent::UnequipWeaponInSlot()
{
	if (equippedWeapon != nullptr)
	{
		UnequipItem(equippedWeapon);
		equippedWeapon = nullptr;
	}
}
