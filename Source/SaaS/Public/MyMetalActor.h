// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyMetalActor.generated.h"

UCLASS()
class SAAS_API AMyMetalActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyMetalActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MySceneComponent")
	USceneComponent *MySceneRoot;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MySceneComponent")	
	UStaticMeshComponent* MyStaticMeshComponent;
	
	UMaterialInstanceDynamic* MyDynamicMaterial;
	
};
