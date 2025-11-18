#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "InventoryInputConfig.generated.h"

class UInputAction;
class UObject;
class UInputMappingContext;
struct FFrame;

/*
* Struct used for adding input mappings with priority
*/
USTRUCT(BlueprintType)
struct INVENTORYABILITYSYSTEM_API FInputMappingContextAndPriority
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input", meta = (AssetBundles = "Client,Server"))
		TSoftObjectPtr<UInputMappingContext> InputMapping;

	// Higher priority input mappings will be prioritized over mappings with a lower priority.
	UPROPERTY(EditAnywhere, Category = "Input")
		int32 Priority = 0;

	/** If true, then this mapping context will be registered with the settings when this game feature action is registered. */
	UPROPERTY(EditAnywhere, Category = "Input")
		bool bRegisterWithSettings = true;
};

/*
 *	Struct used to map a input action to a gameplay input tag.
 */
USTRUCT(BlueprintType)
struct INVENTORYABILITYSYSTEM_API FInventoryInputAction
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
		FGameplayTag InputTag;
};

/*
 *	Non-mutable data asset that contains input configuration properties.
 */
UCLASS(BlueprintType, Const)
class UInventoryInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	UInventoryInputConfig(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Pawn")
		const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag) const;

public:
	// List of input actions used by the owner.  These input actions are mapped to a gameplay tag and are automatically bound to abilities with matching input tags.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
		TArray<FInventoryInputAction> AbilityInputActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputMapping"))
		TArray<FInputMappingContextAndPriority> InputMappings;
};