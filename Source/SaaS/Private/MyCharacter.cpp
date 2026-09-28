// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// 创建组件
	MySpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("MySpringArm"));
	MyCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MyCamera"));
	MySpringArm->TargetArmLength = 400.0f;
	
	MyHealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MyWidgetComponent"));
	MyHealthWidgetComponent->SetupAttachment(RootComponent);
	
	static ConstructorHelpers::FClassFinder<UUserWidget> MyHealthWidgetClass(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/Code/BP_MyHealthWidget.BP_MyHealthWidget_C'"));
	MyHealthWidgetComponent->SetWidgetClass(MyHealthWidgetClass.Class);
	MyHealthWidgetComponent->SetRelativeLocation(FVector(0,0,100));
	MyHealthWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	MyHealthWidgetComponent->SetDrawSize(FVector2D(400.0f, 20.0f));
	
	
	MyCamera->SetupAttachment(MySpringArm);
	MySpringArm->SetupAttachment(RootComponent);


	
	
	// 角色不接受控制器的旋转，即旋转摄像机
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	// 角色面朝加速度方向
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// 弹簧臂接受控制器进行旋转
	MySpringArm->bUsePawnControlRotation = true;
	
	

}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if ( APlayerController* PlayerController = Cast<APlayerController>( GetController() ) ){
		if ( UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) )
		{
			Subsystem->AddMappingContext(DefaultInputMappingContext,0);
		}
	}
	
	// Attack();
	// CalculateHealth();
	// GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AMyCharacter::PrintInfo, 1.0f, true);

	// Delay 不建议使用，是给蓝图提供的
	// const FLatentActionInfo LatentInfo(0,FMath::Rand(), TEXT("MyDelayFunctionFinished"),this);
	// UKismetSystemLibrary::Delay(this,3.0f,LatentInfo);
	

}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// TestRotation += FRotator{1,1,1};
	// GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("TestRotation:\n %s "), *TestRotation.ToString()));
	//
	// FVector Forward = TestRotation.Vector();
	// GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Forward:\n %s "), *Forward.ToString()));
	//
	
	
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if ( UEnhancedInputComponent* EnhancedInputComponent= CastChecked<UEnhancedInputComponent>(PlayerInputComponent) )
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered,this, &AMyCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered,this, &AMyCharacter::Look);
		EnhancedInputComponent->BindAction(Debug1Action, ETriggerEvent::Triggered,this, &AMyCharacter::Debug1);
		EnhancedInputComponent->BindAction(Debug2Action, ETriggerEvent::Triggered,this, &AMyCharacter::Debug2);
	}

}

void AMyCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if ( IsValid(Controller) ){
		
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation{0,Rotation.Yaw, 0};
		
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		const FVector MyForwardDirection = YawRotation.Vector();
		const FVector MyRightDirection = ( FRotator{0.f, 90.f ,0.f} + YawRotation ).Vector();
		
		
		// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Forward:\n %s \n %s "), *ForwardDirection.ToString(), *MyForwardDirection.ToString()));
		// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Right:\n %s \n %s "), *RightDirection.ToString(), *MyRightDirection.ToString()));
		
		
		AddMovementInput(ForwardDirection, MovementVector.X);
		AddMovementInput(RightDirection, MovementVector.Y);
		
	}
}

void AMyCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	if ( IsValid(Controller ) )
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
		
	}

}

void AMyCharacter::Debug1(const FInputActionValue& Value)
{

	
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("射线检测开启")));
	StartLocation = GetActorLocation();
	ForwardVector = GetActorForwardVector();
	EndLocation = StartLocation + ForwardVector*1000;
	
	// 根据通道查询
	// bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility);
	
	
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	
	// 根据对象查询检测
	// bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, StartLocation, EndLocation, ObjectParams);
	
	// 多射线通道检测
	// bHit = GetWorld()->LineTraceMultiByChannel(HitResults, StartLocation, EndLocation, ECC_Visibility);
	
	// 多射线对象检测
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	bHit = GetWorld()->LineTraceMultiByObjectType(HitResults, StartLocation, EndLocation, ObjectParams);
	
	// UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartLocation, EndLocation, 100.0f, 100.0f, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, TArray<AActor*>(), EDrawDebugTrace::ForOneFrame, HitResult, true);
	// 用 Object Type 查询，只扫"Pawn/PhysicsBody"等，不含 WorldStatic(地形)
	
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	
	FCollisionQueryParams IgnoreParams;
	IgnoreParams.AddIgnoredActor(this);
	
	
	// bHit = GetWorld()->SweepSingleByObjectType(HitResult, StartLocation, EndLocation, 
	// 	FQuat::Identity, 
	// 	ObjectParams,
	// 	FCollisionShape::MakeSphere(100.0f),
	// 	IgnoreParams
	// 	); 
	
	
	
	if ( bHit )
	{
		// 多对象射线检测Debug
		GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red,FString::Printf(TEXT("射线检测命中数量: %d"),HitResults.Num()));
		// for (auto& Hit : HitResults)
		// {
		// 	GEngine->AddOnScreenDebugMessage( -1, 5.0f, FColor::Red, 
		// 		FString::Printf(TEXT("射线检测命中: %s"),*Hit.GetActor()->GetName() ) 
		// 		);
		// }
		
		for (int i = 0; i < HitResults.Num(); i++)
		{
			FHitResult& HR = HitResults[i];
			AActor* HitActor = HR.GetActor();
			if (HitActor)
			{
				FString IsBlocking = TEXT("重叠");
				if ( HR.bBlockingHit )
				{
					IsBlocking = TEXT("阻挡");
				}
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
					FString::Printf(TEXT("%s命中[%d]: %s @ %s"),
						*IsBlocking,
						i,
						*HitActor->GetName(),
						*HR.Location.ToString()
					));
				
				// UE_LOG(LogTemp,Warning,TEXT("命中[%d]: %s @ %s"),i,*HitActor->GetName(),*HR.Location.ToString());
			}
				// DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 12.f, FColor::Green, false, 5.0f);
			if ( i == HitResults.Num()-1 )
			{
				DrawDebugSphere(GetWorld(), HR.Location, 20.f, 12, FColor::Green, false, 5.0f);
			} else 
			{
				DrawDebugSphere(GetWorld(), HR.Location, 20.f, 12, FColor::Red, false, 5.0f);
			}
			
				
		}
		 
		
		// 射线检测Debug
		// {
			// AActor* HitActor = HitResult.GetActor();
			// DrawDebugLine(GetWorld(),StartLocation,
		// 		bHit ? HitResult.ImpactPoint : EndLocation,
		// 		FColor::Red, true, 1.0f, 0, 1.0f
		// 	);
		// 	
		// 	DrawDebugPoint(GetWorld(),HitResult.ImpactPoint,10.0f,FColor::Green,true,1.0f);
		//
		// 	DrawDebugLine(GetWorld(),HitResult.ImpactPoint,
		// 		bHit ? EndLocation: HitResult.ImpactPoint ,
		// 		FColor::Green, true, 1.0f, 0, 1.0f
		// 	);
		// 	
		// 	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("射线检测命中: %s"), *HitActor->GetName()));
		// }
		
		
		// 球体检测
		// {
		// 	
		// 	// ===== 完整绘制扫掠检测体积（起点球 → 终点球的胶囊）=====
		// 	
		// 	FVector  SweepDir = EndLocation - StartLocation;
		// 	float    HalfLen  = SweepDir.Size() * 0.5f;          // 胶囊圆柱半长
		// 	FVector  CapsuleCenter = (StartLocation + EndLocation) * 0.5f;
		// 	// FQuat CapsuleRot = FQuat::FindBetweenVectors(FVector::UpVector, SweepDir.GetSafeNormal());
		// 	FQuat CapsuleRot = FRotationMatrix::MakeFromZ(SweepDir).Rotator().Quaternion();
		// 	// FRotationMatrix::MakeFrom
		// 	UE_LOG(LogTemp, Warning, TEXT("(EndLocation-StartLocation).Rotation().Quaternion(): %s"), *(EndLocation-StartLocation).Rotation().Quaternion().ToString());;
		// 	
		// 	UE_LOG(LogTemp, Warning, TEXT("FRotationMatrix::MakeFromZ(SweepDir): %s"), *FRotationMatrix::MakeFromZ(SweepDir).ToString());
		// 	UE_LOG(LogTemp, Warning, TEXT("FRotationMatrix::MakeFromZ(SweepDir).Rotator(): %s"), *FRotationMatrix::MakeFromZ(SweepDir).Rotator().ToString());
		// 	UE_LOG(LogTemp, Warning, TEXT("FRotationMatrix::MakeFromZ(SweepDir).Rotator().Quaternion(): %s"), *FRotationMatrix::MakeFromZ(SweepDir).Rotator().Quaternion().ToString());
		//
		// 	// 半径 100 与 MakeSphere(100.f) 一致
		// 	DrawDebugCapsule(
		// 		GetWorld(),
		// 		CapsuleCenter,        // 胶囊中心
		// 		HalfLen,              // 半长（不含两端半球）
		// 		100.f,                // 半径（= 球半径）
		// 		CapsuleRot,
		// 		// (EndLocation-StartLocation).Rotation().Quaternion(),
		// 		FColor::Yellow,       // 体积颜色，可改
		// 		false,                // 不持久
		// 		20.0f                  // 存活 1 秒
		// 	);
		//
		// 	// 可选：标出起点球和终点球轮廓，便于确认两端
		// 	// DrawDebugSphere(GetWorld(), StartLocation, 100.f, 16, FColor::Cyan,   false, 1.0f);
		// 	// DrawDebugSphere(GetWorld(), EndLocation,   100.f, 16, FColor::Cyan,   false, 1.0f);
		// 	
		//
		// 	
		// 	// // 起点球
		// 	// DrawDebugSphere(GetWorld(), StartLocation, 100.f, 16, FColor::Yellow, false, 5.0f);
		// 	// // 终点球
		// 	// DrawDebugSphere(GetWorld(), EndLocation, 100.f, 16, FColor::Cyan, false, 5.0f);
		// 	// 命中点
		// 	DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 12.f, FColor::Green, false, 5.0f);
		// 	DrawDebugSphere(GetWorld(), HitResult.Location, 20.f, 12, FColor::Green, false, 5.0f);
		// 	//
		// 	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("球体检测命中: %s"), *HitActor->GetName()));
		// }
		
		
	}
	
}

void AMyCharacter::Debug2(const FInputActionValue& Value)
{
	
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Debug2")));
	
	if ( Debug2ActorObject )
	{
		ACharacter* Character = Cast<ACharacter>(Debug2ActorObject);
		// 新的正向，即局部坐标系下Z轴的正向 
		FVector TargetDirection = Character->GetActorForwardVector();
		FRotator TargetRotation = FRotationMatrix::MakeFromZ(TargetDirection).Rotator();
		FRotator MeshTargetRotation = FRotationMatrix::MakeFromZX(TargetDirection,Character->GetActorUpVector()).Rotator();
		
		TArray<UCapsuleComponent*> Capsules;
		Character->GetComponents<UCapsuleComponent>(Capsules);
		// Capsules[0] 通常是根胶囊，Capsules[1] 是你加的子胶囊
		if (Capsules.Num() > 1)
		{
			Capsules[1]->SetWorldRotation(TargetRotation);
		}

		// UActorComponent* MeshComponent = Character->GetComponentByClass(USkeletalMeshComponent::StaticClass());
		USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(
		Character->GetComponentByClass(USkeletalMeshComponent::StaticClass())
		);
		MeshComponent->SetWorldRotation(MeshTargetRotation);
		
	}
}

void AMyCharacter::Attack()
{
	GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red, TEXT("进攻"));
	
}

void AMyCharacter::CalculateHealth()
{
	GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red, TEXT("计算生命值"));
	
}

float AMyCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	
	UMyHealthWidget* MyHealthWidget = Cast<UMyHealthWidget>(MyHealthWidgetComponent->GetUserWidgetObject());
	if ( MyHealthWidget )
	{
		if ( MyHealthWidget->CurrentHealth<= 0.0f )
		{
			return 0.f;
		}
		MyHealthWidget->CurrentHealth -= DamageAmount;
	}
	
	GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red, FString::Printf(TEXT("TakeDamage: %f"), DamageAmount));
	
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AMyCharacter::MyDelayFunctionFinished()
{
	GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red, TEXT("MyDelayFunctionFinished"));
	// Destroy();
}


void AMyCharacter::PrintInfo()
{
	if (++TimerCount > 5 && TimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
	GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red, TEXT("Timer 触发"));
}

