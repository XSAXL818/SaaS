// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraEffectActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Components/SphereComponent.h"


AAuraEffectActor::AAuraEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(MeshComponent);
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>("Sphere");
	SphereComponent->SetupAttachment(GetRootComponent());
	
}

void AAuraEffectActor::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
	// TODO:使用GAS的Effect，现在使用小技巧const_cast
	if (IAbilitySystemInterface* AscInterface = Cast<IAbilitySystemInterface>(OtherActor))
	{
		
		// 先取 ASC 并判空：实现了 IAbilitySystemInterface，不代表 ASC 已经就绪
		// （角色未被 Possess / 尚未 InitAbilityActorInfo 时这里返回空指针 —— 崩溃就发生在这处）
		UAbilitySystemComponent* TargetASC = AscInterface->GetAbilitySystemComponent();
		if (!TargetASC)
		{
			UE_LOG(LogTemp, Warning, TEXT("OnOverlap: %s 的 ASC 为空，跳过"), *GetNameSafe(OtherActor));
			return;
		}

		if (const UAuraAttributeSet* AuraAttributeSet = Cast<UAuraAttributeSet>(TargetASC->GetAttributeSet(UAuraAttributeSet::StaticClass())))
		{
			UAuraAttributeSet* MutableAuraAttributeSet = const_cast<UAuraAttributeSet*>(AuraAttributeSet);
			MutableAuraAttributeSet->SetHealth(AuraAttributeSet->GetHealth()+10.f);
			MutableAuraAttributeSet->SetMana(AuraAttributeSet->GetMana()+10.f);
			Destroy();
		} 
		
		
		
	}
	
}

void AAuraEffectActor::EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
	
	
	
}


void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this,&AAuraEffectActor::OnOverlap);
	
	// 不能直接写函数名，普通/静态函数可以直接写，会自动隐式转换为函数指针，类的成员函数必须通过类名获取
	SphereComponent->OnComponentEndOverlap.AddDynamic(this,&AAuraEffectActor::EndOverlap);
}
	




























