// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

#include "AuraAbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SAAS_API UAuraAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
	
public:
	
	void AbilityActorInfoSet();
	

protected:
	
	
	void EffectApplied(UAbilitySystemComponent* AbilitySystemComponent,  const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActivateEffectHandle);
	
	
};
