// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UAuraAttributeSet::UAuraAttributeSet()
{
	
	InitHealth(50.f);
	InitMaxHealth(100.f);
	InitMana(5.f);
	InitMaxMana(100.f);
	
}

void UAuraAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	

	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	
}

// 常用来截断数值，防止溢出
void UAuraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	
	
	if (Attribute == GetHealthAttribute())
	{
		// 有BaseAttribute和Attribute之分，Base是整个GAS系统内部记录的
		// 如果当前NewValue血量为5，在此处设置NewValue=100，下次给血量+5，角色的属性会变成10
		// static bool b = true;
		// if (b)
		// {
		// 	b = false;
		// 	NewValue = 100;
		// }
		NewValue = FMath::Clamp(NewValue,0,GetMaxHealth());
		
		UE_LOG(LogTemp, Warning, TEXT("Health PreAttributeChange: %f"), NewValue);
	}

	
	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue,0,GetMaxMana());
		
		UE_LOG(LogTemp, Warning, TEXT("Mana PreAttributeChange: %f"), NewValue);
	}

}

void UAuraAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	
	// if (Attribute == GetHealthAttribute())
	// {
	// 	
	// 	// NewValue = FMath::Clamp(NewValue,0,GetMaxHealth());
	// 	
	// 	UE_LOG(LogTemp, Warning, TEXT("Health PreAttributeBaseChange: %f"), NewValue);
	// }
	//
	// if (Attribute == GetMaxHealthAttribute())
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("MaxHealth PreAttributeBaseChange: %f"), NewValue);
	// }
	//
	// if (Attribute == GetManaAttribute())
	// {
	// 	NewValue = FMath::Clamp(NewValue,0,GetMaxMana());
	// 	
	// 	UE_LOG(LogTemp, Warning, TEXT("Mana PreAttributeBaseChange: %f"), NewValue);
	// }
	//
	// if (Attribute == GetMaxManaAttribute())
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("MaxMana PreAttributeBaseChange: %f"), NewValue);
	// 	
	// }
}

void UAuraAttributeSet::SetEffectProperties(const struct FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const
{
	
	// source是效果发起者，target是效果的目标（target拥有AS）
	
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	
	Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();
	
	// AvatarActor是拥有该ASC的角色
	if ( IsValid(Props.SourceASC) && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid() )
	{
		Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
		if ( Props.SourceController != nullptr && Props.SourceAvatarActor!= nullptr )
		{
			if ( const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor) )
			{
				Props.SourceController = Pawn->GetController();
			}
		}
		if (Props.SourceController)
		{
			ACharacter* SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
			
		}
	}
	
	if ( Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid() )
	{
		// AActor* TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetAvatarActor =  Data.Target.GetAvatarActor();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
		
	}
	
	
	
}

void UAuraAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	FEffectProperties Props;
	SetEffectProperties(Data,Props);
	
	
	
}	



void UAuraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Health, OldHealth);
	
}

void UAuraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxHealth, OldMaxHealth);
}

void UAuraAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Mana, OldMana);
}

void UAuraAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxMana, OldMaxMana);
}



