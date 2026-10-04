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
	
	UFUNCTION(BlueprintCallable)
	void ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass);
	
	
	// 用户蓝图中输入,即使效果
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Allpied Effects")
	TSubclassOf<UGameplayEffect> InstantGameplayEffect;
	
	// 持续效果
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Allpied Effects")
	TSubclassOf<UGameplayEffect> DurationGameplayEffect;
	

private:


};
