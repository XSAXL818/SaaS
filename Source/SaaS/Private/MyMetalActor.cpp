// Fill out your copyright notice in the Description page of Project Settings.


#include "MyMetalActor.h"


// Sets default values
AMyMetalActor::AMyMetalActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	MySceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MySceneRoot"));
	MyStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyStaticMeshComponent"));
	
	RootComponent = MySceneRoot;
	MyStaticMeshComponent->SetupAttachment(MySceneRoot);
	
	// 静态加载
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TempStaticMesh(
		// TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple'")
		TEXT("/Script/Engine.StaticMesh'/Game/Fishermans_Cabin/Meshes/Fish_Plaque/SM_Fish_Plaque_01.SM_Fish_Plaque_01'")
		);
	if( TempStaticMesh.Succeeded())
	{
		UE_LOG(LogTemp,Warning,TEXT("TempStaticMesh.Succeeded()"));
	}
	
	MyStaticMeshComponent->SetStaticMesh(TempStaticMesh.Object);
	
	
	
}

// Called when the game starts or when spawned
void AMyMetalActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 材质
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Script/Engine.Material'/Game/Code/Metal/M_TestMetal.M_TestMetal'")
		);
	// 创建动态材质
	MyDynamicMaterial = MyStaticMeshComponent->CreateDynamicMaterialInstance(0, Material);
	// 动态修改材质参数
	MyDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"),FLinearColor::Red);
	MyDynamicMaterial->SetScalarParameterValue(TEXT("Metallic"),1.0f);
	
}

// Called every frame
void AMyMetalActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}



