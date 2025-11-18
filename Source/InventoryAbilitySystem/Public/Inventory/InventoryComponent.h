// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "InventoryComponent.generated.h"

class UInventoryItemInstance;

USTRUCT(BlueprintType)
struct INVENTORYABILITYSYSTEM_API FInventoryDefinition
{
	GENERATED_BODY()

		FInventoryDefinition() {}

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1))
		int32 Amount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSubclassOf<class UInventoryItemDefinition> Definition;
};

USTRUCT(BlueprintType)
struct INVENTORYABILITYSYSTEM_API FInventoryEntry
{
	GENERATED_BODY()

		FInventoryEntry() {}

private:
	friend UInventoryComponent;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess))
		TObjectPtr<UInventoryItemInstance> instance = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess))
		int32 stackCount = 0;

	// return value of how many could not be added
	void AddStack(int32& additionalStack);
	void RemoveStack(int32& removeStack);
};

UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class INVENTORYABILITYSYSTEM_API UInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const {}
};

UCLASS()
class UInventoryFragment_EquippableItem : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UEquipmentDefinition> EquipmentDefinition;
};

UCLASS()
class UInventoryFragment_SetStats : public UInventoryItemFragment
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = Equipment)
		TMap<FGameplayTag, int32> InitialItemStats;

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const override;

	int32 GetItemStatByTag(FGameplayTag Tag) const;
};

UCLASS()
class UInventoryFragment_UI : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FSlateBrush Brush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FSlateBrush AmmoBrush;
};

UCLASS()
class UInventoryFragment_Ammo : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Equipment)
		TSubclassOf<class UInventoryItemDefinition> AmmoType;
};

UCLASS(Blueprintable, Const, Abstract)
class UInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		FText Category;

	// default limit of how many items can be in one stack, can be modified in UInventoryComponent
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		int32 stackLimit = -1;

	// if the item should get added to the weapons slots (this allows to cycle through)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		bool bAddToSlots;

	// if item is not an weapon, and it should not only be part of inventory
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "!bAddToSlots", EditConditionHides))
		bool bEquip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced)
		TArray<TObjectPtr<UInventoryItemFragment>> Fragments;

	const UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;

};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class INVENTORYABILITYSYSTEM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UPROPERTY()
		TArray<FInventoryEntry> inventoryList;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 inventorySize = -1;

public:
	// Sets default values for this component's properties
	UInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	FORCEINLINE bool IsInventoryBigEnough() { return inventorySize < 0 || inventoryList.Num() < inventorySize; }

public:
	UFUNCTION(BlueprintPure)
		int32 GetStackCountDefinition(TSubclassOf<UInventoryItemDefinition> itemDef);
	UFUNCTION(BlueprintPure)
		int32 GetStackCount(UInventoryItemInstance* item);

	UFUNCTION(BlueprintNativeEvent)
		bool CanAddItemToStack(FInventoryEntry targetStack, TSubclassOf<UInventoryItemDefinition> itemDef, int32 stackCount);
	UFUNCTION(BlueprintNativeEvent)
		bool CanAddItemToInventory(TSubclassOf<UInventoryItemDefinition> itemDef, int32 stackCount);

	UFUNCTION(BlueprintCallable)
		UInventoryItemInstance* AddItemDefinition(TSubclassOf<UInventoryItemDefinition> itemDef, UPARAM(ref) int32& stackCount);

	// adds a specific instance of the item (might have differences to what it is in UInventoryItemDefinition, eg. a magazine which is not completely full)
	UFUNCTION(BlueprintCallable)
		bool AddItemInstance(UInventoryItemInstance* instance, UPARAM(ref) int32& stackCount);

	UFUNCTION(BlueprintCallable)
		void RemoveItemDefinition(TSubclassOf<UInventoryItemDefinition> itemDef, int32 stackCount);

	// removes instances of a specific stack
	UFUNCTION(BlueprintCallable)
		UInventoryItemInstance* RemoveItemInstance(UInventoryItemInstance* instance, UPARAM(ref) int32& stackCount);

	UFUNCTION(BlueprintCallable)
		void RemoveAllItems();

	UFUNCTION(BlueprintCallable)
		TArray<FInventoryEntry> GetItems() const;

	class UEquipmentComponent* FindEquipmentManager();

};
