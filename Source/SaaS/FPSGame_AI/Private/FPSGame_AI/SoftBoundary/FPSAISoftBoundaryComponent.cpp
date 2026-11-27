#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryLog.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryVolume.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

const TCHAR* UFPSAISoftBoundaryComponent::DefaultPostProcessMaterialPath = TEXT("/Game/FPSGame_AI/Materials/MI_FPSAISoftBoundaryPP.MI_FPSAISoftBoundaryPP");

namespace
{
	/** 体积是关卡里摆的，1 秒扫一次就够，玩家重生/流送进来也能被发现。 */
	constexpr float kVolumeRefreshSeconds = 1.f;

	/** 安全落点离当前位置超过这个距离就不用了，退化成几何解。 */
	constexpr float kMaxSafeLocationDistanceCm = 4000.f;

	/** 落点被挡住时沿安全一侧微调的步长与次数。 */
	constexpr float kLandingNudgeStepCm = 50.f;
	constexpr int32 kLandingNudgeAttempts = 8;
}

UFPSAISoftBoundaryComponent::UFPSAISoftBoundaryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PostProcessMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultPostProcessMaterialPath));
}

void UFPSAISoftBoundaryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!IsValid(Pawn))
	{
		return;
	}

	VolumeRefreshCountdown -= DeltaTime;
	if (VolumeRefreshCountdown <= 0.f)
	{
		RefreshVolumes();
	}
	else if (Volumes.ContainsByPredicate([](const TObjectPtr<AFPSAISoftBoundaryVolume>& Volume) { return !IsValid(Volume.Get()); }))
	{
		RefreshVolumes();
	}

	if (Volumes.Num() == 0)
	{
		// 关卡里一个软边界体积都没摆：什么都不驱动，别留下上一次的残留暗度。
		LastTargetDarkness = 0.f;
		LastSignedDepth = 0.f;
		SmoothedDarkness = 0.f;
		const float Pushed = (DebugDarknessOverride >= 0.f) ? FMath::Clamp(DebugDarknessOverride, 0.f, 1.f) : 0.f;
		PushDarkness(Pushed);
		return;
	}

	EnsurePostProcess();

	const FVector PawnLocation = Pawn->GetActorLocation();

	AFPSAISoftBoundaryVolume* Dominant = nullptr;
	float MaxDepth = 0.f;
	float MaxTarget = 0.f;
	bool bFirstVolume = true;

	for (const TObjectPtr<AFPSAISoftBoundaryVolume>& Volume : Volumes)
	{
		if (!IsValid(Volume.Get()))
		{
			continue;
		}

		const float Depth = Volume->EvaluateDepth(PawnLocation);

		// 先把"安全"位置记下来：全黑传送优先回到这里，KeepOut 才不会把人塞进禁区中心。
		if (Depth <= -Volume->Hysteresis)
		{
			Volume->RecordSafeLocation(PawnLocation);
		}

		// 用体积的有效全黑距离：薄 KeepOut 盒开了自动适配之后，值已经被压到盒内能走到的深度，
		// 否则 600cm 的触发带宽对这种盒子永远走不满，永远不传送。
		const float Target = FPSAISoftBoundaryMath::ComputeTargetDarkness(Depth, Volume->GetEffectiveFullBlackDistance());
		MaxTarget = FMath::Max(MaxTarget, Target);

		// 多个体积时取深度最大的那个作为主导：它决定暗度推进速度、传送落点和朝向。
		if (bFirstVolume || Depth > MaxDepth)
		{
			MaxDepth = Depth;
			Dominant = Volume;
			bFirstVolume = false;
		}
	}

	if (!IsValid(Dominant))
	{
		return;
	}

	DominantVolume = Dominant;
	LastSignedDepth = MaxDepth;
	LastTargetDarkness = MaxTarget;

	// 蠕动风格跟着主导体积走：它最深，也就是它决定玩家此刻看到的那层暗影。
	ApplyCrawlPreset(*Dominant);

	const FFPSAISoftBoundarySettings Settings = Dominant->MakeSettings();
	const FFPSAISoftBoundaryUpdate Update = FPSAISoftBoundaryMath::UpdateState(State, MaxDepth, DeltaTime, Settings);

	if (Update.bTeleport)
	{
		// 全黑这一帧直接推到 1 再传送，否则会看到"半黑瞬间被挪走"。
		SmoothedDarkness = FMath::Max(SmoothedDarkness, Update.Darkness);
		TeleportToSafety(Pawn, *Dominant);
	}
	else if (Update.Darkness >= SmoothedDarkness)
	{
		const float DarkenSeconds = FMath::Max(0.f, Dominant->DarkenDuration);
		SmoothedDarkness = (DarkenSeconds <= KINDA_SMALL_NUMBER)
			? Update.Darkness
			: FMath::Min(Update.Darkness, SmoothedDarkness + DeltaTime / DarkenSeconds);
	}
	else
	{
		// 变亮方向已经由数学层平滑过，这里直接跟随。
		SmoothedDarkness = Update.Darkness;
	}

	const float Pushed = (DebugDarknessOverride >= 0.f)
		? FMath::Clamp(DebugDarknessOverride, 0.f, 1.f)
		: SmoothedDarkness;
	PushDarkness(Pushed);
}

void UFPSAISoftBoundaryComponent::RefreshVolumes()
{
	VolumeRefreshCountdown = kVolumeRefreshSeconds;
	Volumes.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, AFPSAISoftBoundaryVolume::StaticClass(), Found);
	Volumes.Reserve(Found.Num());
	for (AActor* Actor : Found)
	{
		if (AFPSAISoftBoundaryVolume* Volume = Cast<AFPSAISoftBoundaryVolume>(Actor))
		{
			Volumes.Add(Volume);
		}
	}

	if (Volumes.Num() != LastLoggedVolumeCount)
	{
		LastLoggedVolumeCount = Volumes.Num();
		UE_LOG(LogFPSAISoftBoundary, Display,
			TEXT("Soft-boundary: %d volume(s) active for owner %s"), Volumes.Num(), *GetNameSafe(GetOwner()));
	}
}

void UFPSAISoftBoundaryComponent::EnsurePostProcess()
{
	if (DarknessMID && BoundCamera)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!BoundCamera)
	{
		// The camera usually lives directly on the Pawn, but some templates keep it
		// under a SpringArm or other sub-component; fall back to any owned camera.
		BoundCamera = Owner->FindComponentByClass<UCameraComponent>();
		if (!BoundCamera)
		{
			TArray<UCameraComponent*> Cameras;
			Owner->GetComponents<UCameraComponent>(Cameras);
			if (Cameras.Num() > 0)
			{
				BoundCamera = Cameras[0];
			}
		}
	}
	if (!BoundCamera || DarknessMID)
	{
		if (!BoundCamera && !bLoggedSetupWarning)
		{
			bLoggedSetupWarning = true;
			UE_LOG(LogFPSAISoftBoundary, Warning,
				TEXT("Soft-boundary: no UCameraComponent found on owner %s, darkness will not render"),
				*GetNameSafe(Owner));
		}
		return;
	}

	UMaterialInterface* Material = PostProcessMaterial.LoadSynchronous();
	if (!Material)
	{
		if (!bLoggedSetupWarning)
		{
			bLoggedSetupWarning = true;
			UE_LOG(LogFPSAISoftBoundary, Warning, TEXT("Soft-boundary post process material missing: %s"), *PostProcessMaterial.ToString());
		}
		return;
	}

	DarknessMID = UMaterialInstanceDynamic::Create(Material, this);
	if (!DarknessMID)
	{
		return;
	}

	DarknessMID->SetScalarParameterValue(TEXT("Darkness"), 0.f);
	BoundCamera->PostProcessBlendWeight = 1.f;
	BoundCamera->PostProcessSettings.AddBlendable(DarknessMID, 1.f);

	// 新的 MID 会从 MI 的默认值起算，所以把"已经写过预设"作废，让下一次 ApplyCrawlPreset 真的写一遍。
	bHasAppliedCrawlPreset = false;
}

void UFPSAISoftBoundaryComponent::ApplyCrawlPreset(const AFPSAISoftBoundaryVolume& Volume)
{
	// 只有预设有变化才算一次写：三个 SetScalarParameterValue 每帧重写既浪费也可能让后处理管线反复重建。
	if (bHasAppliedCrawlPreset && AppliedCrawlPreset == Volume.CrawlPreset)
	{
		return;
	}

	AppliedCrawlPreset = Volume.CrawlPreset;
	bHasAppliedCrawlPreset = true;

	if (!DarknessMID)
	{
		// MID 还没建好（Pawn 上还没相机 / 材质没加载）：预设先记着，EnsurePostProcess 建好 MID 后
		// 下次 Tick 会把 bHasAppliedCrawlPreset 清掉，这一帧之后自然补写。
		return;
	}

	const FFPSAISoftBoundaryCrawlParams Params = AFPSAISoftBoundaryVolume::GetCrawlPresetParams(Volume.CrawlPreset);
	DarknessMID->SetScalarParameterValue(TEXT("NoiseSpeed"), Params.NoiseSpeed);
	DarknessMID->SetScalarParameterValue(TEXT("NoiseStrength"), Params.NoiseStrength);
	DarknessMID->SetScalarParameterValue(TEXT("EdgeFeather"), Params.EdgeFeather);
}

void UFPSAISoftBoundaryComponent::PushDarkness(float Darkness)
{
	if (DarknessMID)
	{
		DarknessMID->SetScalarParameterValue(TEXT("Darkness"), FMath::Clamp(Darkness, 0.f, 1.f));
	}
}

void UFPSAISoftBoundaryComponent::TeleportToSafety(APawn* Pawn, AFPSAISoftBoundaryVolume& Volume)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Pawn))
	{
		return;
	}

	const FVector VolumeCenter = Volume.GetVolumeTransform().GetLocation();
	const FVector PawnLocation = Pawn->GetActorLocation();
	const bool bKeepOut = Volume.IsKeepOut();

	// 首选：本体积记下的安全落点。KeepOut 靠它才不会把玩家送到禁区中心。
	FVector Landing;
	const bool bFromSafeLocation = Volume.TryGetSafeLocation(PawnLocation, kMaxSafeLocationDistanceCm, Landing);
	if (!bFromSafeLocation)
	{
		// 退化：沿"体积中心 -> Pawn"方向找最近出射面，再往外（KeepOut）/往里（KeepIn）退 ReturnDistance。
		const float Offset = bKeepOut ? Volume.ReturnDistance : -Volume.ReturnDistance;
		const FFPSAISoftBoundaryTeleport Geometric = FPSAISoftBoundaryMath::ComputeTeleportAlongRay(
			PawnLocation,
			Volume.GetVolumeTransform(),
			Volume.GetVolumeExtent(),
			Volume.IsCylinder(),
			Offset);
		Landing = Geometric.WorldLocation;
	}

	FVector Resolved;
	ResolveLanding(Pawn, Volume, Landing, Resolved);

	// KeepIn 朝向体积中心（前方是可玩区），KeepOut 朝向体积外（背对禁区）。
	const float Yaw = FPSAISoftBoundaryMath::ComputeFacingYaw(Resolved, VolumeCenter, bKeepOut);
	const FRotator Facing(0.f, Yaw, 0.f);

	Pawn->SetActorLocation(Resolved, false, nullptr, ETeleportType::TeleportPhysics);
	Pawn->SetActorRotation(Facing);

	if (AController* Controller = Pawn->GetController())
	{
		Controller->SetControlRotation(Facing);
	}

	if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
	{
		Movement->StopMovementImmediately();
		Movement->Velocity = FVector::ZeroVector;
	}

	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
		{
			CharacterMovement->StopMovementImmediately();
			CharacterMovement->Velocity = FVector::ZeroVector;
			CharacterMovement->ClearAccumulatedForces();
		}
	}

	UE_LOG(LogFPSAISoftBoundary, Display,
		TEXT("Soft-boundary[%s] teleport: mode=%s source=%s from=%s to=%s depth=%.1fcm yaw=%.1f"),
		*Volume.GetName(),
		bKeepOut ? TEXT("KeepOut") : TEXT("KeepIn"),
		bFromSafeLocation ? TEXT("LastSafeLocation") : TEXT("Geometry"),
		*PawnLocation.ToCompactString(),
		*Resolved.ToCompactString(),
		LastSignedDepth,
		Facing.Yaw);
}

void UFPSAISoftBoundaryComponent::ResolveLanding(APawn* Pawn, const AFPSAISoftBoundaryVolume& Volume, const FVector& Desired, FVector& OutLanding) const
{
	OutLanding = Desired;

	UWorld* World = GetWorld();
	if (!World || !IsValid(Pawn))
	{
		return;
	}

	float HalfHeight = 88.f;
	float Radius = 34.f;
	if (const UCapsuleComponent* Capsule = Pawn->FindComponentByClass<UCapsuleComponent>())
	{
		HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		Radius = Capsule->GetScaledCapsuleRadius();
	}

	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FPSAISoftBoundaryLanding), false, Pawn);

	// 落点被几何卡住时继续朝安全一侧挪：KeepOut 往体积外、KeepIn 往体积里。
	FVector SafeDirection = (Desired - Volume.GetVolumeTransform().GetLocation()).GetSafeNormal2D();
	if (!Volume.IsKeepOut())
	{
		SafeDirection = -SafeDirection;
	}
	if (SafeDirection.IsNearlyZero())
	{
		SafeDirection = FVector::ForwardVector;
	}

	const float Hysteresis = FMath::Max(0.f, Volume.Hysteresis);
	FVector Candidate = Desired;

	for (int32 Attempt = 0; Attempt < kLandingNudgeAttempts; ++Attempt)
	{
		// 向下贴地：从落点上方打一条线，落到 ImpactPoint + 半高 + 2。
		FVector Grounded = Candidate;
		FHitResult GroundHit;
		const FVector TraceStart = Candidate + FVector(0.f, 0.f, HalfHeight + 200.f);
		const FVector TraceEnd = Candidate - FVector(0.f, 0.f, 1500.f);
		if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, Params) && GroundHit.bBlockingHit)
		{
			Grounded = GroundHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 2.f);
		}

		// 落点必须站在体积认为安全的一侧，且胶囊没被几何卡住。
		const bool bSafeSide = Volume.EvaluateDepth(Grounded) <= -Hysteresis;
		const bool bBlocked = World->OverlapBlockingTestByChannel(Grounded, FQuat::Identity, ECC_Pawn, Shape, Params);

		OutLanding = Grounded;
		if (bSafeSide && !bBlocked)
		{
			return;
		}

		Candidate = Grounded + SafeDirection * kLandingNudgeStepCm;
	}
}
