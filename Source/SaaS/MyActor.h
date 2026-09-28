// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/GameplayStatics.h"
#include "MyCharacter.h"

#include "GameFramework/Actor.h"
#include "MyActor.generated.h"


class UBoxComponent;
class URotatingMovementComponent;

UCLASS()
class SAAS_API AMyActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
public:
	
	UPROPERTY(EditAnywhere,Category="Category1")
	int MyCategory1;
	
	UPROPERTY(EditAnywhere,Category="Category1|SubCategory1")
	int MySubCategory1;
	
	UPROPERTY(EditAnyWhere, meta=(DisplayName="是否开启TestValue1编辑",ToolTip="为true则开启TestValue1的编辑"))
	bool isOpenTestValue1;
	
	UPROPERTY(EditAnywhere, meta=(EditCondition="isOpenTestValue1"))
	int TestValue1;
	// 前向声明，告诉编译器有URotatingMovementComponent这么一个类，因此不需要加入头文件来告诉编译器这个类的定义
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MySceneComponent" )
	URotatingMovementComponent* MyRotatingMovementComponent;
	
	UPROPERTY(EditAnywhere, meta=(DisplayName="Chinese"))
	int MyInt;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MySceneComponent")	
	USkeletalMeshComponent* MySkeletalMeshComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MySceneComponent")	
	UStaticMeshComponent* MyStaticMeshComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MySceneComponent")
	USceneComponent *MySceneRoot;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MySceneComponent")
	UBoxComponent* MyBoxComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MyClass")
	TSubclassOf<AActor> MyActor;
	
	UFUNCTION()
	void BeginOverLapFunction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void EndOverlapFunction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	


};
