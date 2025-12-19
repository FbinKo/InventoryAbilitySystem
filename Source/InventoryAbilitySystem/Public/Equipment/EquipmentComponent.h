// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ability/AbilitySet.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "EquipmentComponent.generated.h"

class UInventoryItemInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquipEvent, UEquipmentInstance*, NewEquipment);

USTRUCT(BlueprintType)
struct INVENTORYABILITYSYSTEM_API FSlotsChangedMessage
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadOnly)
		TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly)
		TArray<TObjectPtr<UInventoryItemInstance>> Slots;
};

USTRUCT(BlueprintType)
struct INVENTORYABILITYSYSTEM_API FActiveIndexChangedMessage
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadOnly)
		TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly)
		int32 ActiveIndex = 0;
};

USTRUCT(BlueprintType)
struct INVENTORYABILITYSYSTEM_API FAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

		FAppliedEquipmentEntry()
	{}

private:
	friend UEquipmentComponent;

	UPROPERTY()
		TSubclassOf<UEquipmentDefinition> equipmentDefinition;
	UPROPERTY()
		TObjectPtr<UEquipmentInstance> instance = nullptr;
	UPROPERTY(NotReplicated)
		FAbilitySet_GrantedHandles grantedHandles;
};

USTRUCT(BlueprintType)
struct FEquipmentActorToSpawn
{
	GENERATED_BODY()

		FEquipmentActorToSpawn()
	{}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Equipment)
		TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Equipment)
		FName AttachSocket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Equipment)
		FTransform AttachTransform;
};

UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class UEquipmentDefinition : public UObject
{
	GENERATED_BODY()

public:
	UEquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Class to spawn
	UPROPERTY(EditDefaultsOnly, Category = Equipment)
		TSubclassOf<UEquipmentInstance> InstanceType;

	// Gameplay ability sets to grant when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = Equipment)
		TArray<TObjectPtr<const class UAbilitySet>> AbilitySetsToGrant;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = Equipment)
		TArray<FEquipmentActorToSpawn> ActorsToSpawn;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class INVENTORYABILITYSYSTEM_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere)
		TArray<FAppliedEquipmentEntry> equipmentList;
	TObjectPtr<UEquipmentInstance> equippedWeapon;
	TArray<TArray<TObjectPtr<UEquipmentInstance>>> equippedItems;

protected:
	/* number of weapon slots */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		int32 numWeaponSlots = 2;

	TArray<TSubclassOf<UInventoryFragment_EquippableItem>> equipmentCategories;
	TArray<TArray<TObjectPtr<UInventoryItemInstance>>> equipmentSlots;
	TArray<TObjectPtr<UInventoryItemInstance>> weaponSlots;
	int32 activeSlotIndex = -1;

public:
	UEquipmentComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void UninitializeComponent() override;

	UPROPERTY(BlueprintAssignable)
		FEquipEvent OnEquip;

	UPROPERTY(BlueprintAssignable)
		FEquipEvent OnUnequip;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
		UEquipmentInstance* EquipItemDefinition(TSubclassOf<UEquipmentDefinition> EquipmentDefinition);

	void EquipItemInstance(int32 slotId, UInventoryItemInstance* instance);

	UInventoryItemInstance* RemoveItemInstance(int32 slotId, TSubclassOf<UInventoryFragment_EquippableItem> type);

	// only made accessible if user wants to handle unequip themself (this is only unequip, not remove from slot)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
		void UnequipItem(UEquipmentInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
		void UnequipEverything();

	/** Returns the first equipped instance of a given type, or nullptr if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = InstanceType))
		UEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UEquipmentInstance> InstanceType);

	/** Returns all equipped instances of a given type, or an empty array if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DeterminesOutputType = InstanceType))
		TArray<UEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UEquipmentInstance> InstanceType) const;

	UFUNCTION(BlueprintPure)
		TArray<UInventoryItemInstance*> GetSlots(TSubclassOf<UInventoryFragment_EquippableItem> type) const;
	UFUNCTION(BlueprintPure)
		UInventoryItemInstance* GetCurrentWeapon() {
		if (activeSlotIndex >= 0) return weaponSlots[activeSlotIndex];
		else return nullptr;
	}

	UFUNCTION(BlueprintPure)
		int32 GetNextFreeWeaponSlot();

	/* also equips item (exception are weapons, to keep current weapon active
	* for weapons use SetActiveSlotIndex, CycleActiveSlotForward and CycleActiveSlotBackward
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
		void AddItemToSlot(int32 slotId, UInventoryItemInstance* item);

	/* removes and unequips the item in slot */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
		UInventoryItemInstance* RemoveItemFromSlot(int32 slotId, TSubclassOf<UInventoryFragment_EquippableItem> type);

	UFUNCTION(BlueprintPure)
		int32 GetActiveSlotIndex() { return activeSlotIndex; }
	/* sets new active weapon slot and updates equipment */
	UFUNCTION(BlueprintCallable)
		void SetActiveSlotIndex(int32 newId);

	UFUNCTION(BlueprintCallable)
		void CycleActiveSlotForward();

	UFUNCTION(BlueprintCallable)
		void CycleActiveSlotBackward();

private:
	class UInventoryAbilitySystemComponent* GetAbilitySystemComponent() const;
	void EquipWeaponInSlot();
	void UnequipWeaponInSlot();
};
