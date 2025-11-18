// Fill out your copyright notice in the Description page of Project Settings.


#include "BehaviorTree/BTService_GameplayFocus.h"
#include "AIController.h"

UBTService_GameplayFocus::UBTService_GameplayFocus(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	FocusPriority = EAIFocusPriority::Gameplay;
}
