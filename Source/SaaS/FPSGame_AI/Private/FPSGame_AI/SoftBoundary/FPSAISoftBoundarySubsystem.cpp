#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundarySubsystem.h"

#include "Engine/World.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryComponent.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryLog.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryVolume.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** 玩家 Pawn 通常在世界开始播放后不久出现，半秒扫一次足够，也不会有感开销。 */
	constexpr float kScanIntervalSeconds = 0.5f;
}

bool UFPSAISoftBoundarySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// Only game/preview worlds; never editor-preview, thumbnail or editor worlds.
	const UWorld* World = Cast<UWorld>(Outer);
	return World != nullptr && World->IsGameWorld();
}

void UFPSAISoftBoundarySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	EnsurePawnComponent();
}

void UFPSAISoftBoundarySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ScanCountdown -= DeltaTime;
	if (ScanCountdown > 0.f)
	{
		return;
	}
	ScanCountdown = kScanIntervalSeconds;
	EnsurePawnComponent();
}

void UFPSAISoftBoundarySubsystem::EnsurePawnComponent()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(Pawn))
	{
		// 还在等 Pawn 生成，下次再扫。
		return;
	}

	if (Pawn->FindComponentByClass<UFPSAISoftBoundaryComponent>())
	{
		return;
	}

	// 世界里一个软边界体积都没有就别挂组件：不去动和软边界无关的地图（例如 Aura 的 StartupMap），
	// 也不会白建后处理 MID。等设计师摆上体积之后，下一次扫描自然就把组件补上。
	TArray<AActor*> Volumes;
	UGameplayStatics::GetAllActorsOfClass(World, AFPSAISoftBoundaryVolume::StaticClass(), Volumes);
	if (Volumes.Num() == 0)
	{
		return;
	}

	UFPSAISoftBoundaryComponent* Component = NewObject<UFPSAISoftBoundaryComponent>(Pawn, TEXT("FPSAISoftBoundary"));
	if (!Component)
	{
		UE_LOG(LogFPSAISoftBoundary, Warning, TEXT("Soft-boundary: failed to create the boundary component on pawn %s"), *Pawn->GetName());
		return;
	}

	Pawn->AddInstanceComponent(Component);
	Component->RegisterComponent();

	UE_LOG(LogFPSAISoftBoundary, Display,
		TEXT("Soft-boundary: attached UFPSAISoftBoundaryComponent to pawn %s in world %s"),
		*Pawn->GetName(), *World->GetMapName());
}
