// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraCharacterBase.h"

#include "AbilitySystemComponent.h"


AAuraCharacterBase::AAuraCharacterBase()
{
	// 基类不tick
	PrimaryActorTick.bCanEverTick = false;
	
	// 用来显示武器的网格体
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	// 绑定到网格体的指定插槽socket
	Weapon->SetupAttachment(GetMesh(),FName("WeaponHandSocket"));
	// 取消碰撞
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
}

UAbilitySystemComponent* AAuraCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAuraCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	
}


