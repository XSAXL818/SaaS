// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/Enemy/AuraEnemy.h"
#include "Interaction/EnemyInterface.h"

AAuraPlayerController::AAuraPlayerController()
{
	// 开启网络复制（Replication）。
	// 复制 = 服务器把 Actor 的状态同步给客户端；设为 true 后，该 PlayerController 才能
	// 在服务器与客户端之间同步状态、收发 RPC（如 ClientRPC / ServerRPC）。
	// 本学习项目目前单机运行，此开关不产生副作用，只是为将来多人联网预留。
	bReplicates = true;
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
	CursorTrace();
	
	
}



void AAuraPlayerController::CursorTrace()
{
	FHitResult CursorHit;
	GetHitResultUnderCursor(ECC_Visibility,false,CursorHit);
	if (!CursorHit.bBlockingHit) return;
	
	// GEngine->AddOnScreenDebugMessage(-1,10,FColor::Red,TEXT("Tick"));
	
	LastActor = CurrentActor;
	CurrentActor = Cast<IEnemyInterface>(CursorHit.GetActor());
	// AAuraEnemy* EnemyActor = Cast<AAuraEnemy>(CurrentActor);
	
	if ( CurrentActor != LastActor )
	{
		/** 三种情况
		 * 1. Current 空，Last 不空，鼠标移出敌人
		 * 2. Current 不空，Last 空，鼠标移入敌人
		 * 3. Current 不空，Last 不空，鼠标在敌人之间切换
		 * 规律：Current只做高亮，Last只做取消高亮，如果为空就不做任何操作
		 */
		if (CurrentActor) CurrentActor->HighlightActor();
		if (LastActor) LastActor->UnHighlightActor();
	} 
	else{
		/** 两种情况
		 * 1. 都为空
		 * 2. 指向同一个对象
		 * 不做任何做操
		 */
	}
	
}



void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 断言，判断输入映射上下文是否存在
	check(AuraInputMappingContext);
	
	// 获取"本地玩家的增强输入子系统"。
	// Enhanced Input 的映射上下文就注册在这个子系统上；每个本地玩家各有一份，
	// 所以要先 GetLocalPlayer() 拿到当前玩家，再从他身上取子系统。
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	
	// 断言：子系统必须存在，为 false 时立即停止执行（相当于"这里不该为空"的强保证）。
	// 注意（官方原文）：check 族系"默认不会在发布版本中运行"——
	// 默认只在 Debug / Development 构建生效，Shipping 中会被移除；
	// 需要时可用 USE_CHECKS_IN_SHIPPING=1 在 Test / Shipping 中开启（发布应设回 0）。
	check(Subsystem);
	
	// 把输入映射上下文注册进子系统 —— 这之后 IMC 里配置的按键才真正生效。
	// 第二个参数是优先级（Priority），决定多个映射上下文共存时的叠加顺序；
	// 本项目目前只有一个上下文，填 0 即可。
	Subsystem->AddMappingContext(AuraInputMappingContext, 0);
	
	// 显示鼠标光标（点地移动类 RPG 需要看得见光标）
	bShowMouseCursor = true;
	// 光标的默认外观：十字准星
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	
	// 设置输入模式：游戏 + UI 同时可用（既能操作角色，也能点击 UI 按钮）
	FInputModeGameAndUI InputModeData;
	// 鼠标不锁定在视口内（可以移出游戏窗口）
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	// 鼠标被捕获时（例如按住左键拖动）不隐藏光标
	InputModeData.SetHideCursorDuringCapture(false);
	// 把上面配置好的输入模式应用到本 PlayerController
	SetInputMode(InputModeData);
	
	
	
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// 将自身的输入组件转换为增强输入组件
	// UE5中如果启用了增强输入后，实际对象类型是UEnhancedInputComponent
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	
	// 绑定不同操作的回调函数
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Look);
}

void AAuraPlayerController::Move(const FInputActionValue& Value)
{
	// 将行为值转换为2D的
	const FVector2D MovementVector = Value.Get<FVector2D>();
	// 获取控制器的旋转，控制器一般和摄像头绑定
	const FRotator Rotation = GetControlRotation();
	// 只要Yaw方向的旋转
	const FRotator YawRotation{0.f,Rotation.Yaw,0.f};
	// 获取X、Y的轴上的单位向量，即向前、向右方向向量
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	
	if ( APawn* ControllerPawn = GetPawn() )
	{
		// XY坐标系通常左右是X轴，所以IMC中故意将AD映射到X轴
		ControllerPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
		ControllerPawn->AddMovementInput(RightDirection, MovementVector.X);
	}
	
}

void AAuraPlayerController::Look(const FInputActionValue& Value)
{
	
	
}
