// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"   // 一般不用单独加，Kismet 已带


#include "DrawDebugHelpers.h"

#include "MyHealthWidget.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"
#include "EnhancedInputSubsystems.h"
#include "MyInterface.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "GameFramework/Character.h"
#include "MyCharacter.generated.h"


class UInputAction;

UCLASS()
class SAAS_API AMyCharacter : public ACharacter, public IMyInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter();
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 弹簧臂
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,category="MySceneComponent")
	USpringArmComponent* MySpringArm;

	// 摄像机
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, category = "MySceneComponent")
	UCameraComponent* MyCamera;

	// 输入映射上下文/情景 IMC
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Input")
	class UInputMappingContext* DefaultInputMappingContext;

	// Move 移动输入操作IA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Input")
	UInputAction* MoveAction;
	
	// Look 视角输入操作IA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Input")
	UInputAction* LookAction;
	
	// Debug1 输入操作IA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Input")
	UInputAction* Debug1Action;
	
	// Debug2 输入操作IA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Input")
	UInputAction* Debug2Action;

	// Move 移动输入回调函数
	void Move(const FInputActionValue& Value);
	// Look 视角输入回调函数
	void Look(const FInputActionValue& Value);
	
	// Debug_1 射线检测
	void Debug1(const FInputActionValue& Value);
	
	// Debug_2 射线检测
	void Debug2(const FInputActionValue& Value);
	
	// Debug_2 将要操作的Actor对象
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Debug")
	AActor* Debug2ActorObject;
	
	
	// 测试旋转变量：Yaw++会一直相加，不会到了360后从0开始
	FRotator TestRotation;

	// 重写接口函数
	virtual void Attack() override;
	virtual void CalculateHealth() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "MySceneComponent")
	UWidgetComponent* MyHealthWidgetComponent;
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	
	UFUNCTION()
	void MyDelayFunctionFinished();
	
	
private:
	// 定时器
	FTimerHandle TimerHandle;
	// 打印函数
	void PrintInfo();
	// 定时器执行次数
	int32 TimerCount = 0;
	
	// 射线检测
	FVector StartLocation;
	FVector ForwardVector;
	FVector EndLocation;
	FHitResult HitResult;
	bool bHit;
	// 多射线检测结果
	TArray<FHitResult> HitResults;
};
