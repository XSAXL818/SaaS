// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuraEffectActor.generated.h"

class UGameplayEffect;
class USphereComponent;

UCLASS()
class SAAS_API AAuraEffectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AAuraEffectActor();
	

protected:
	
	virtual void BeginPlay() override;
	
	UFUNCTION(Blueprintable)
	void ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass);
	
	
	// 用户蓝图中输入
	UPROPERTY(EditAnywhere, Category="Effect")
	TSubclassOf<UGameplayEffect> InstantGameplayEffect;
	

private:


};
