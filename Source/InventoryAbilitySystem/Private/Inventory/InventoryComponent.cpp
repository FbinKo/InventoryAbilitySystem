// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemInstance.h"
#include "Equipment/EquipmentComponent.h"
#include "Equipment/EquipmentInstance.h"
#include "NativeGameplayTags.h"
#include "GameFrameWork/PlayerState.h"

void FInventoryEntry::AddStack(int32& additionalStack)
{
	int32 stackLimit = instance->GetItemDef()->GetDefaultObject<UInventoryItemDefinition>()->stackLimit;
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

bool UInventoryComponent::CanAddItemToInventory_Implementation(TSubclassOf<UInventoryItemDefinition> itemDef, int32& stackCount)
{
	// eg. check for uniqueness, weight or other limitations and update stackCount to how many can be added
	return true;
}

UInventoryItemInstance* UInventoryComponent::AddItemDefinition(TSubclassOf<UInventoryItemDefinition> itemDef, int32& stackCount)
{
	UInventoryItemInstance* Result = nullptr;
	if (stackCount > 0)
	{
		if (itemDef) {
			if (!CanAddItemToInventory(itemDef, stackCount))
				return Result;

			// add to existing stack
			for (auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
			{
				FInventoryEntry& Entry = *EntryIt;
				if (Entry.instance->itemDef == itemDef)
				{
					Entry.AddStack(stackCount);
					Result = Entry.instance;
					if (stackCount == 0)
						return Result;
				}
			}

			// if not found or more to add than space in existing stack, add a new stack
			while (stackCount > 0)
			{
				if (IsInventoryBigEnough())
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
		TSubclassOf<UInventoryItemDefinition> itemDefToAdd = instance->GetItemDef();
		if (CanAddItemToInventory(itemDefToAdd, stackCount))
		{
			if (itemDefToAdd->GetDefaultObject<UInventoryItemDefinition>()->bInstancesAlwaysStack)
			{
				for (auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
				{
					FInventoryEntry& Entry = *EntryIt;
					if (Entry.instance->itemDef == itemDefToAdd)
					{
						Entry.AddStack(stackCount);
						if (stackCount == 0)
							return true;
					}
				}
			}

			while (stackCount > 0)
			{
				if (IsInventoryBigEnough())
				{
					// either dont stack or more than previous stacks could hold need to be added
					FInventoryEntry& NewEntry = inventoryList.AddDefaulted_GetRef();
					NewEntry.AddStack(stackCount);
					if (stackCount == 0)
					{
						NewEntry.instance = instance;
						return true;
					}
					else
					{
						NewEntry.instance = NewObject<UInventoryItemInstance>(GetOwner());
						NewEntry.instance->SetItemDef(itemDefToAdd);
					}
				}
				else
					break;
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
		Result = NewObject<UInventoryItemInstance>(GetOwner());
		Result->SetItemDef(instance->itemDef);
		for (auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
		{
			FInventoryEntry& Entry = *EntryIt;
			if (Entry.instance == instance)
			{
				int32 actualRemoval = FMath::Min(stackCount, Entry.stackCount);
				Entry.RemoveStack(stackCount);
				if (Entry.stackCount <= 0)
					EntryIt.RemoveCurrent();

				stackCount = actualRemoval;
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
		while (0 < stackCount)
		{
			for (auto EntryIt = inventoryList.CreateIterator(); EntryIt; ++EntryIt)
			{
				FInventoryEntry& Entry = *EntryIt;
				if (Entry.instance->itemDef == itemDef)
				{
					Entry.RemoveStack(stackCount);
					if (Entry.stackCount <= 0)
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

TArray<FInventoryEntry> UInventoryComponent::GetItems(TSubclassOf<UInventoryFragment_EquippableItem> type) const
{
	TArray<FInventoryEntry> Result;
	if (type)
		Result = inventoryList.FilterByPredicate([type](const FInventoryEntry& Entry) { return Entry.stackCount > 0 && Entry.instance->FindFragmentByClass<UInventoryFragment_EquippableItem>()->IsA(type); });
	else
		Result = inventoryList.FilterByPredicate([](const FInventoryEntry& Entry) { return Entry.stackCount > 0; });
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
