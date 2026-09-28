// Fill out your copyright notice in the Description page of Project Settings.


#include "MyFunctionActor.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AMyFunctionActor::AMyFunctionActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyFunctionActor::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<AActor*> OutActors;
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), OutActors);
	
	GEngine->AddOnScreenDebugMessage(-1,5.0f, 
			FColor::Emerald, 
			FString::Printf(TEXT("Actor Num:%d"), OutActors.Num())
			);
	
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		[OutActors]()
		{
			for (auto Actor : OutActors)
			{
		
				GEngine->AddOnScreenDebugMessage(-1,5.0f, 
					FColor::Emerald, 
					FString::Printf(TEXT("Actor Name:%s"), *Actor->GetName())
					);
		
			}
		},
		2.0f,
		false
	);
	
	
	
}

// Called every frame
void AMyFunctionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyFunctionActor::OpenLevel()
{
	UGameplayStatics::OpenLevel(GetWorld(), TEXT("SciFiCreaturesResearchRoomA"));
}

void AMyFunctionActor::GetLevelName()
{
	FString LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("关卡名:%s"), *LevelName));
	
}

void AMyFunctionActor::QuitGame()
{
	UGameplayStatics::GetPlayerController(GetWorld(),0)->ConsoleCommand(TEXT("quit"));
}

