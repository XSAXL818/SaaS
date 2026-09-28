// Fill out your copyright notice in the Description page of Project Settings.


#include "MyActor.h"

#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AMyActor::AMyActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	MySkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("我的骨骼网格体"));
	MyStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("我的静态网格体"));
	MyBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("我的碰撞盒子"));
	MySceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("我的根组件"));
	
	// 静态加载资源
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> TempStaticMesh(
		TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple'")
		);
	
	MySkeletalMeshComponent->SetSkeletalMeshAsset(TempStaticMesh.Object);
	
	RootComponent = MySceneRoot;
	MyStaticMeshComponent->SetupAttachment(RootComponent);
	MyBoxComponent->SetupAttachment(RootComponent);
	MySkeletalMeshComponent->SetupAttachment(RootComponent);
	
	
	// 静态加载类
	static ConstructorHelpers::FClassFinder<AActor> TempMyActor(
		TEXT("/Script/Engine.Code'/Game/TopDown/Code/BP_TopDownCharacter.BP_TopDownCharacter_C'")
		);

	MyActor = TempMyActor.Class;
	
	MyBoxComponent->SetGenerateOverlapEvents(true);
	MyBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AMyActor::BeginOverLapFunction);
	MyBoxComponent->OnComponentEndOverlap.AddDynamic(this, &AMyActor::EndOverlapFunction);

}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();
	
	// GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red, TEXT("Hello, Unreal Engine!"));
	
	// if (MyActor) {
	// 	GEngine->AddOnScreenDebugMessage(-1, 
	// 		5.0f, 
	// 		FColor::Green, 
	// 		FString::Format(TEXT("操作成功, {0}"),{ MyActor->GetName() })
	// 		);
	// }
	// else {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("加载类失败"));
	// }

	// UE_LOG(LogTemp, Error, TEXT("My Color Is Red!"));
	// UE_LOG(LogTemp, Warning, TEXT("My Color Is Yellow!"));
	// UE_LOG(LogTemp, Display, TEXT("My Color Is White!"));

	// FString MyString = TEXT("Hello, Unreal Engine!");
	// UE_LOG(LogTemp, Display, TEXT("My String Is: %s"), *MyString);
	// MyString = "123";
	// UE_LOG(LogTemp, Display, TEXT("My String Is: %s"), *MyString);

	// FName Name1 = "123";
	// FName Name2 = "123";
	
	// UE_LOG(LogTemp,Display,TEXT("FName对象地址不同：%p %p；\n但其内部保存字符串的NameEntry的地址相同：%p %p"), &Name1, &Name2, Name1.GetDisplayNameEntry(), Name2.GetDisplayNameEntry());

	// GEngine->AddOnScreenDebugMessage(-1,5.f,FColor::Red,TEXT("打印到屏幕"));
	
	// TArray<int> MyArray;
	// MyArray.Add(1);
	// MyArray.Add(2);
	// MyArray.Add(3);
	// MyArray.AddUnique(1);

	// for (auto& Element : MyArray)
	// {	
		// UE_LOG(LogTemp, Display, TEXT("My String Is: %d"), Element);
	// }
	
	// TMap<FString, int> MyMap;
	// MyString = TEXT("Hello, Unreal Engine!");
	// MyMap.Add("Hello", 1);
	// MyMap.Add("World", 2);
	// MyMap.Add("Hello", 3); // 重写key为"Hello"的value
	// MyMap.Emplace("n", 4);
	// MyMap.Emplace(MyString, 5);
	
	// TTuple<int,int> MyTuple;
	// MyTuple.Key = 1;
	// MyTuple.Value = 2;

	// for (auto& Pair : MyMap)
	// {
		// UE_LOG(LogTemp, Display, TEXT("My Map Is: %s => %d"), *(Pair.Key), Pair.Value );
	// }
	
	// 获取蓝图中添加的旋转运动组件（需包含头文件）
	URotatingMovementComponent* RotatingMovementComp = GetComponentByClass<URotatingMovementComponent>();
	if (IsValid(RotatingMovementComp))
	{
		MyRotatingMovementComponent = RotatingMovementComp;
		// 修改旋转速度等属性
		RotatingMovementComp->RotationRate = FRotator(90.0f, 0.0f, 0.0f); // 每秒 90 度
	}

	
	
	
}

// Called every frame
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMyActor::BeginOverLapFunction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	if (AMyCharacter* MyCharacter = Cast<AMyCharacter>(OtherActor); IsValid(MyCharacter))
	{
		UGameplayStatics::ApplyDamage(MyCharacter, 10.0f, nullptr, this, UDamageType::StaticClass());
	}
	
}

void AMyActor::EndOverlapFunction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("End"));
	
}

