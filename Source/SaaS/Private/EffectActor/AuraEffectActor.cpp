// Fill out your copyright notice in the Description page of Project Settings.


#include "EffectActor/AuraEffectActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"


AAuraEffectActor::AAuraEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("SceneRoot"));
	
	
}

void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();


}

void AAuraEffectActor::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	
	// 将Target转换为实现IAbilitySystemInterace的对象，并获取AbilitySystemComponent
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		check(GameplayEffectClass);
		// 配置上下文
		FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
		// 标记来源
		EffectContextHandle.AddSourceObject(this);
		// 打包为spec
		const FGameplayEffectSpecHandle GameplayEffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass,1.f, EffectContextHandle);
		// 应用
		TargetASC->ApplyGameplayEffectSpecToSelf(*GameplayEffectSpecHandle.Data.Get());
	
	}
	
	
}
	




























