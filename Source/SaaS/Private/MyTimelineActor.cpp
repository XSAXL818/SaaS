// Fill out your copyright notice in the Description page of Project Settings.


#include "MyTimelineActor.h"


// Sets default values
AMyTimelineActor::AMyTimelineActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	MyTimelineComponent = CreateDefaultSubobject<UTimelineComponent>(TEXT("MyTimelineComponent"));
	
	MySceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MySceneComponent"));
	MyStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyStaticMeshComponent"));
	MyBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("MyBoxComponent"));
	
	// static ConstructorHelpers::FObjectFinder<UStaticMesh>StaticMesh(TEXT("/Script/Engine.StaticMesh'/ControlRig/Controls/ControlRig_RoundedSquare_solid.ControlRig_RoundedSquare_solid'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMesh(
		TEXT("/Script/Engine.StaticMesh'/Game/Maps/_GENERATED/75271/Door.Door'")
		);
	
	if ( StaticMesh.Succeeded() )
	{
		MyStaticMeshComponent->SetStaticMesh(StaticMesh.Object);
		MyStaticMeshComponent->SetRelativeRotation(FRotator(0.0f,-90.0f,0.0f));
		MyStaticMeshComponent->SetRelativeLocation(FVector(0.0f,0.0f,60.0f));
	}
	
	
	RootComponent = MySceneComponent;
	MyStaticMeshComponent->SetupAttachment(RootComponent);
	MyBoxComponent->SetupAttachment(RootComponent);
	// MyStaticMeshComponent->AddRelativeRotation(FRotator(90,0,0));
	MyBoxComponent->SetupAttachment(RootComponent);
	MyBoxComponent->SetBoxExtent(FVector(200.0f,100.0f,100.0f));
	MyBoxComponent->SetRelativeLocation(FVector(200.0f,0.0f,100.0f));
	MyBoxComponent->SetRelativeRotation(FRotator(0.0f,0.0f,90.0f));
	
	
	
	
	
}

// Called when the game starts or when spawned
void AMyTimelineActor::BeginPlay()
{
	Super::BeginPlay();
	
	TimelineDelegate.BindUFunction(this,TEXT("TimelineUpdate"));
	TimelineFinishedDelegate.BindUFunction(this,TEXT("TimelineFinished"));
	
	MyTimelineComponent->AddInterpFloat(MyCurveFloat,TimelineDelegate);
	MyTimelineComponent->SetLooping(false);
	// MyTimelineComponent->PlayFromStart();
	MyTimelineComponent->SetTimelineFinishedFunc(TimelineFinishedDelegate);
	
	MyBoxComponent->OnComponentBeginOverlap.AddDynamic(this,&AMyTimelineActor::BeginOverLapFunction);
	MyBoxComponent->OnComponentEndOverlap.AddDynamic(this,&AMyTimelineActor::EndOverLapFunction);
	
	InitialYaw = MyStaticMeshComponent->GetRelativeRotation().Yaw;
}

// Called every frame
void AMyTimelineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyTimelineActor::TimelineUpdate(float value)
{
	// GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red,FString::Printf(TEXT("TimelineUpdate: %f"),value));
	
	FRotator CurrentRotation = MyStaticMeshComponent->GetRelativeRotation();
	CurrentRotation.Yaw = FMath::Lerp(0.0f,90.0f,value) + InitialYaw;
	MyStaticMeshComponent->SetRelativeRotation(CurrentRotation);
	// MyStaticMeshComponent->SetRelativeRotation(FRotator(0.0f,FMath::Lerp(0.0f,90.0f,value),0.0f));
	
	
}

void AMyTimelineActor::TimelineFinished()
{
	GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red,FString::Printf(TEXT("TimelineFinished")));
}

void AMyTimelineActor::BeginOverLapFunction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ( AMyCharacter* TmpMyCharacter = Cast<AMyCharacter>(OtherActor) )
	{
		// GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red,FString::Printf(TEXT("BeginOverLapFunction")));
		MyTimelineComponent->PlayFromStart();
	}
}

void AMyTimelineActor::EndOverLapFunction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if ( AMyCharacter* TmpMyCharacter = Cast<AMyCharacter>(OtherActor) )
	{
		MyTimelineComponent->ReverseFromEnd();
	}
}

