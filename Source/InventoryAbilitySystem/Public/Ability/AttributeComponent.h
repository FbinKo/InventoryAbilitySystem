// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

class UInventoryAbilitySystemComponent;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDeathEvent, AActor*, OwningActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAttributeChanged, float, OldValue, float, NewValue, AActor*, Instigator);

UENUM(BlueprintType)
enum class EDeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class INVENTORYABILITYSYSTEM_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly)
		FName AttributeGroupName;

protected:
	UPROPERTY()
		TObjectPtr<UInventoryAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY()
		TObjectPtr<const class UInventoryHealthSet> HealthSet;
	UPROPERTY()
		TObjectPtr<const class UInventoryMovementSet> MovementSet;

	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
		EDeathState DeathState;

public:	
	// Sets default values for this component's properties
	UAttributeComponent();

	void InitializeAfterAbilitySystemComponent(UInventoryAbilitySystemComponent* asc, int32 levelToInitialize = 1);
	
	UPROPERTY(BlueprintAssignable)
		FAttributeChanged OnShieldChanged;
	UPROPERTY(BlueprintAssignable)
		FAttributeChanged OnMaxShieldChanged;

	// Delegate fired when the health value has changed. This is called on the client but the instigator may not be valid
	UPROPERTY(BlueprintAssignable)
		FAttributeChanged OnHealthChanged;

	// Delegate fired when the max health value has changed. This is called on the client but the instigator may not be valid
	UPROPERTY(BlueprintAssignable)
		FAttributeChanged OnMaxHealthChanged;

	// Delegate fired when the death sequence has started.
	UPROPERTY(BlueprintAssignable)
		FDeathEvent OnDeathStarted;

	// Delegate fired when the death sequence has finished.
	UPROPERTY(BlueprintAssignable)
		FDeathEvent OnDeathFinished;

	// Delegate fired when speed modifier value has changed
	UPROPERTY(BlueprintAssignable)
		FAttributeChanged OnSpeedChanged;

	UPROPERTY(BlueprintAssignable)
		FAttributeChanged OnEnergyChanged;
	UPROPERTY(BlueprintAssignable)
		FAttributeChanged OnMaxEnergyChanged;

	UFUNCTION(BlueprintCallable)
		EDeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintPure)
		bool IsDeadOrDying() const { return DeathState > EDeathState::NotDead; }

	// Begins the death sequence for the owner.
	virtual void StartDeath();

	// Ends the death sequence for the owner.
	virtual void FinishDeath();

protected:
	virtual void HandleShieldChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxShieldChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleSpeedChanged(float OldValue, float NewValue);
	virtual void HandleEnergyChanged(float OldValue, float NewValue);
	virtual void HandleMaxEnergyChanged(float OldValue, float NewValue);
	virtual void HandleOutOfEnergy(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	UFUNCTION()
		virtual void OnRep_DeathState(EDeathState OldDeathState);
		
};
