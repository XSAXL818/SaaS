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
		const FGameplayEffectSpecHandle GameplayEffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass,ActorLevel, EffectContextHandle);
		// 应用并存储返回的活动句柄
		FActiveGameplayEffectHandle ActiveGameplayEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*GameplayEffectSpecHandle.Data.Get());
		// 判断是否是无限效果
		const bool bIsInfinite = GameplayEffectSpecHandle.Data.Get()->Def->DurationPolicy == EGameplayEffectDurationType::Infinite;
		// 保留无线效果的句柄
		if (bIsInfinite && InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
		{
			// 保存handle
			ActiveEffectHandles.FindOrAdd(TargetASC).Add(ActiveGameplayEffectHandle);
		}
		
		
	}
	
	
}

void AAuraEffectActor::OnBeginOverlap(AActor* TargetActor)
{
	
	// 即时
	if ( InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnBeginOverlap )
	{
		ApplyEffectToTarget(TargetActor,InstantGameplayEffectClass);
	}
	// 持续
	if ( DurationEffectApplicationPolicy== EEffectApplicationPolicy::ApplyOnBeginOverlap )
	{
		ApplyEffectToTarget(TargetActor,DurationGameplayEffectClass);
	}
	// 无限
	if ( InfiniteEffectApplicationPolicy== EEffectApplicationPolicy::ApplyOnBeginOverlap )
	{
		ApplyEffectToTarget(TargetActor,InfiniteGameplayEffectClass);
	}
	
	
}

void AAuraEffectActor::OnEndOverlap(AActor* TargetActor)
{
	// 即时
	if ( InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap )
	{
		ApplyEffectToTarget(TargetActor,InstantGameplayEffectClass);
	}
	// 持续
	if ( DurationEffectApplicationPolicy== EEffectApplicationPolicy::ApplyOnEndOverlap )
	{
		ApplyEffectToTarget(TargetActor,DurationGameplayEffectClass);
	}
	// 无限
	if ( InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap )
	{
		ApplyEffectToTarget(TargetActor,InfiniteGameplayEffectClass);
	}
	// 移除无限效果
	if ( InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap )
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (TargetASC)
		{
			TArray<FActiveGameplayEffectHandle>	HandlesToRemove;
			if (TSet<FActiveGameplayEffectHandle>* Handles = ActiveEffectHandles.Find(TargetASC))
			{
				for (const FActiveGameplayEffectHandle& Handle : *Handles)
				{
					
					TargetASC->RemoveActiveGameplayEffect(Handle,1);
					
				}
				Handles->Empty();
				
			}
		}
		
	}
	
	
	
}
	




























