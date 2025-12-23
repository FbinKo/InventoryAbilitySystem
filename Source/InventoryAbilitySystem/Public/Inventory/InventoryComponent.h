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
struct INVENTORYABILITYSYSTEM_API FLoadout
{
	GENERATED_BODY()

		FLoadout() {}

public:
	UPROPERTY(EditAnywhere)
		int32 slotID = 0;

	UPROPERTY(EditAnywhere)
		FInventoryDefinition item;

	UPROPERTY(EditAnywhere)
		bool bEquipItem = false;
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

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EInventoryCategory : uint8
{
	None = 0 UMETA(Hidden),
	Weapon = 1 << 0,
	Armor = 1 << 1,
	Accessoir = 1 << 2,
	Useable = 1 << 3,
	All = (1 << 4) - 1,
};
ENUM_CLASS_FLAGS(EInventoryCategory);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EInventoryArmorCategory : uint8
{
	None = 0 UMETA(Hidden),
	Head = 1 << 1,
	Upperbody = 1 << 2,
	Lowerbody = 1 << 3,
	Feet = 1 << 4,
	Arms = 1 << 5,
	Hands = 1 << 6,
	All = (1 << 7) - 1,
};
ENUM_CLASS_FLAGS(EInventoryArmorCategory);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EInventoryAccessoirCategory : uint8
{
	None = 0 UMETA(Hidden),
	Ring = 1 << 1,
	Necklace = 1 << 2,
	All = (1 << 3) - 1,
};
ENUM_CLASS_FLAGS(EInventoryAccessoirCategory);

UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class INVENTORYABILITYSYSTEM_API UInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const {}
};

UCLASS(BlueprintType, Blueprintable)
class UInventoryFragment_EquippableItem : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		TSubclassOf<class UEquipmentDefinition> EquipmentDefinition;
};

UCLASS()
class UInventoryFragment_WeaponItem : public UInventoryFragment_EquippableItem
{
	GENERATED_BODY()
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

	// default limit of how many items can be in one stack, can be modified in UInventoryComponent
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		int32 stackLimit = -1;

	// when an instance is added, it checks if another instance below stackLimit already exists and adds it to it or creates a new stack
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		bool bInstancesAlwaysStack = true;

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

	// check for uniqueness, weight or other limitations and update stackCount to how many can be added
	UFUNCTION(BlueprintNativeEvent)
		bool CanAddItemToInventory(TSubclassOf<UInventoryItemDefinition> itemDef, UPARAM(ref) int32& stackCount);

	UFUNCTION(BlueprintCallable)
		UInventoryItemInstance* AddItemDefinition(TSubclassOf<UInventoryItemDefinition> itemDef, UPARAM(ref) int32& stackCount);

	// adds a specific instance of the item (might have differences to what it is in UInventoryItemDefinition, eg. a magazine which is not completely full)
	// stackCount is how many instances should be added and gets updated how many couldn't be added
	UFUNCTION(BlueprintCallable)
		bool AddItemInstance(UInventoryItemInstance* instance, UPARAM(ref) int32& stackCount);

	UFUNCTION(BlueprintCallable)
		void RemoveItemDefinition(TSubclassOf<UInventoryItemDefinition> itemDef, int32 stackCount);

	// removes instances of a specific stack
	// stackCount is how much is to be removed and gets updated to how much were successful removed
	UFUNCTION(BlueprintCallable)
		UInventoryItemInstance* RemoveItemInstance(UInventoryItemInstance* instance, UPARAM(ref) int32& stackCount);

	UFUNCTION(BlueprintCallable)
		void RemoveAllItems();

	// if no category, returns all items
	UFUNCTION(BlueprintCallable)
		TArray<FInventoryEntry> GetItems(TSubclassOf<UInventoryFragment_EquippableItem> type) const;

	class UEquipmentComponent* FindEquipmentManager();

};
