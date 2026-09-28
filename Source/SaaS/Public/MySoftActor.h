// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "MySoftActor.generated.h"

UCLASS()
class SAAS_API AMySoftActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMySoftActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	int TickCount = 0;
	
	// 软对象引用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SoftPath")
	FSoftClassPath AssertSoftClassPath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SoftPath")
	FSoftObjectPath AssertSoftObjectPath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SoftPath")
	TSoftObjectPtr<AActor> AssertSoftObjectPtr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SoftPath")
	TSoftClassPtr<AActor> AssertSoftClassPtr;
	
	
	
};
