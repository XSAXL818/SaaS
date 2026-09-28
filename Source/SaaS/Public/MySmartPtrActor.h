// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MySmartPtrActor.generated.h"


class TestA
{
public:
	int a;
	float b;
	
	TestA()
	{
		a = 0;
		b = 1.f;
	}
	
	TestA(int a, float b)
		: a(a), b(b)
	{

	}
	
	~TestA()
	{
		UE_LOG(LogTemp,Warning,TEXT("TestA::~TestA"));
	}
};

UCLASS()
class SAAS_API AMySmartPtrActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMySmartPtrActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void TestAFunc();
	
	void TestBFunc();
	
	void TestC();
};
