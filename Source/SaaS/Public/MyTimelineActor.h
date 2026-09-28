// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Components/TimelineComponent.h"
#include "Components/BoxComponent.h"
#include "MyCharacter.h"

#include "GameFramework/Actor.h"
#include "MyTimelineActor.generated.h"

UCLASS()
class SAAS_API AMyTimelineActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyTimelineActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyCurve")
	UCurveFloat* MyCurveFloat;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MySceneComponent")
	UTimelineComponent* MyTimelineComponent;
	
	FOnTimelineFloat TimelineDelegate;
	FOnTimelineEvent TimelineStartDelegate;
	FOnTimelineEvent TimelineFinishedDelegate;
	
	UFUNCTION()
	void TimelineUpdate(float value);
	
	UFUNCTION()
	void TimelineFinished();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MySceneComponent")
	USceneComponent* MySceneComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MySceneComponent")
	UStaticMeshComponent* MyStaticMeshComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MySceneComponent")
	UBoxComponent* MyBoxComponent;
	
	UFUNCTION()
	void BeginOverLapFunction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void EndOverLapFunction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
private:
	// 开门初始Yaw
	float InitialYaw;
	
};
