#include "InventoryInputConfig.h"

UInventoryInputConfig::UInventoryInputConfig(const FObjectInitializer& ObjectInitializer)
{
}

const UInputAction* UInventoryInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FInventoryInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && (Action.InputTag == InputTag))
		{
			return Action.InputAction;
		}
	}

	return nullptr;
}