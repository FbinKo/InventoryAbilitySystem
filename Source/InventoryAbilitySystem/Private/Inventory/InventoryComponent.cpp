// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemInstance.h"
#include "Equipment/EquipmentComponent.h"
#include "Equipment/EquipmentInstance.h"
#include "NativeGameplayTags.h"
#include "GameFrameWork/PlayerState.h"

void FInventoryEntry::AddStack(int32& additionalStack)
{
	int32 stackLimit = Cast<UInventoryItemDefinition>(instance->GetItemDef()->GetDefaultObject())->stackLimit;
	if (stackLimit < 0) {
		stackCount += additionalStack;
		additionalStack = 0;
	}
	else {
		int32 diff = FMath::Max(stackCount + additionalStack - stackLimit, 0);
		stackCount = FMath::Clamp(stackCount + additionalStack, 0, stackLimit);
		additionalStack = diff;
	}
}

void FInventoryEntry::RemoveStack(int32& removeStack)
{
	int32 diff = FMath::Max(removeStack - stackCount, 0);
	stackCount = stackCount - removeStack;
	removeStack = diff;
}

void UInventoryFragment_SetStats::OnInstanceCreated(UInventoryItemInstance* Instance) const
{
	for (const auto& InitialStat : InitialItemStats)
	{
		Instance->AddStatTagStack(InitialStat.Key, InitialStat.Value);
	}
}

int32 UInventoryFragment_SetStats::GetItemStatByTag(FGameplayTag Tag) const
{
	if (const int32* StatPtr = InitialItemStats.Find(Tag))
	{
		return *StatPtr;
	}

	return 0;
}

UInventoryItemDefinition::UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
{
	bAddToSlots = false;
	bEquip = true;
}

const UInventoryItemFragment* UInventoryItemDefinition::FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

int32 UInventoryComponent::GetStackCountDefinition(TSubclassOf<UInventoryItemDefinition> itemDef)
{
	if (itemDef) {
		for (auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
		{
			FInventoryEntry& Entry = *EntryIt;
			if (Entry.instance->itemDef == itemDef)
			{
				return Entry.stackCount;
			}
		}
	}

	return 0;
}

int32 UInventoryComponent::GetStackCount(UInventoryItemInstance* item)
{
	if (item) {
		for (auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
		{
			FInventoryEntry& Entry = *EntryIt;
			if (Entry.instance == item)
			{
				return Entry.stackCount;
			}
		}
	}

	return 0;
}

bool UInventoryComponent::CanAddItemToStack_Implementation(FInventoryEntry targetStack, TSubclassOf<UInventoryItemDefinition> itemDef, int32 stackCount)
{
	// eg. check for tag stack limitations like a magazine with different amount of ammo in
	return true;
}

bool UInventoryComponent::CanAddItemToInventory_Implementation(TSubclassOf<UInventoryItemDefinition> itemDef, int32 stackCount)
{
	// eg. check for uniqueness, weight or other limitations
	return true;
}

UInventoryItemInstance* UInventoryComponent::AddItemDefinition(TSubclassOf<UInventoryItemDefinition> itemDef, int32& stackCount)
{
	UInventoryItemInstance* Result = nullptr;
	if (stackCount > 0)
	{
		if (itemDef) {
			// add to existing stack
			for (auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
			{
				FInventoryEntry& Entry = *EntryIt;
				if (Entry.instance->itemDef == itemDef)
				{
					if (CanAddItemToStack(Entry, itemDef, stackCount))
					{
						Entry.AddStack(stackCount);
						Result = Entry.instance;
						if (stackCount == 0)
							return Result;
					}
				}
			}

			// if not found or more to add than space in existing stack, add a new stack
			while (stackCount > 0)
			{
				if (IsInventoryBigEnough())
				{
					if (CanAddItemToInventory(itemDef, stackCount))
					{
						const UInventoryItemDefinition* defaultItem = GetDefault<UInventoryItemDefinition>(itemDef);
						FInventoryEntry& NewEntry = inventoryList.AddDefaulted_GetRef();
						NewEntry.instance = NewObject<UInventoryItemInstance>(GetOwner());
						NewEntry.instance->SetItemDef(itemDef);
						NewEntry.AddStack(stackCount);
						for (UInventoryItemFragment* Fragment : defaultItem->Fragments) {
							if (Fragment)
								Fragment->OnInstanceCreated(NewEntry.instance);
						}

						Result = NewEntry.instance;
					}
				}
				else
					break;
			}
		}
	}
	return Result;
}

bool UInventoryComponent::AddItemInstance(UInventoryItemInstance* instance, int32& stackCount)
{
	if (instance) {
		if (IsInventoryBigEnough())
		{
			if (CanAddItemToInventory(instance->itemDef, stackCount))
			{
				FInventoryEntry& NewEntry = inventoryList.AddDefaulted_GetRef();
				NewEntry.instance = instance;
				NewEntry.AddStack(stackCount);
				if (stackCount == 0)
					return true;
			}
		}
	}

	return false;
}

UInventoryItemInstance* UInventoryComponent::RemoveItemInstance(UInventoryItemInstance* instance, int32& stackCount)
{
	UInventoryItemInstance* Result = nullptr;
	if (instance)
	{
		UInventoryItemInstance* newInstance = NewObject<UInventoryItemInstance>(GetOwner());
		newInstance->SetItemDef(instance->itemDef);
		for (auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
		{
			FInventoryEntry& Entry = *EntryIt;
			if (Entry.instance == instance)
			{
				stackCount = FMath::Min(stackCount, Entry.stackCount);
				Entry.RemoveStack(stackCount);
				if (Entry.stackCount <= 0)
					EntryIt.RemoveCurrent();

				break;
			}
		}
	}
	return Result;
}

void UInventoryComponent::RemoveItemDefinition(TSubclassOf<UInventoryItemDefinition> itemDef, int32 stackCount)
{
	if (itemDef)
	{
		int consumedCount = 0;
		while (consumedCount < stackCount)
		{
			for(auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
			{
				FInventoryEntry& Entry = *EntryIt;
				if (Entry.instance->itemDef == itemDef)
				{
					consumedCount += Entry.stackCount;
					EntryIt.RemoveCurrent();
					break;
				}
			}
		}
	}
}

void UInventoryComponent::RemoveAllItems()
{
	inventoryList.Empty();
}

TArray<FInventoryEntry> UInventoryComponent::GetItems() const
{
	TArray<FInventoryEntry> Result = inventoryList.FilterByPredicate([](const FInventoryEntry& Entry) { return Entry.stackCount > 0; });
	return Result;
}

UEquipmentComponent* UInventoryComponent::FindEquipmentManager()
{
	if (AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (APawn* Pawn = OwnerController->GetPawn())
		{
			return Pawn->FindComponentByClass<UEquipmentComponent>();
		}
		else if (OwnerController->GetPlayerState<APlayerState>()) {
			if (APawn* StatePawn = OwnerController->GetPlayerState<APlayerState>()->GetPawn()) {
				//pawn in aicontroller is set after possess, which is atm too late for the setup
				//so this is a fallback
				return StatePawn->FindComponentByClass<UEquipmentComponent>();
			}
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("Trying to find equipmentmanager on %s, but either no pawn and no playerstate"), *OwnerController->GetName());
		}
	}
	return nullptr;
}
