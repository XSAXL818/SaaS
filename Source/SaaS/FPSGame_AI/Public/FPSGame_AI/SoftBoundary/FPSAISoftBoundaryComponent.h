#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryMath.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryVolume.h"
#include "FPSAISoftBoundaryComponent.generated.h"

class APawn;
class UCameraComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * 挂在玩家 Pawn 上的软边界求值器。
 *
 * 每帧遍历关卡里所有 AFPSAISoftBoundaryVolume，各自算一个有符号深度（正数=已经越过边界），
 * 取最大值作为全局目标暗度，再推给后处理动态材质实例的 Darkness 标量。全黑时把 Pawn 传回
 * 触发它的那个体积记下的安全落点（没有就按几何解算），并保证 KeepOut 的落点一定在体积外面。
 *
 * 组件由 UFPSAISoftBoundarySubsystem 挂到玩家 Pawn 上，因此"关卡里没摆体积"也不会卡死，
 * 摆几个、什么时候摆都行。
 */
UCLASS(ClassGroup = (FPSGame_AI), meta = (BlueprintSpawnableComponent))
class SAAS_API UFPSAISoftBoundaryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFPSAISoftBoundaryComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 已经推进后处理材质的暗度（0..1）：上升受体积的 DarkenDuration 限制。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float GetDarkness() const { return SmoothedDarkness; }

	/** 当前目标暗度（0..1）：所有体积里最大的那个目标值，不做平滑。0 表示安全。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float GetTargetDarkness() const { return LastTargetDarkness; }

	/** 主导体积的有符号深度（厘米），正数表示已经越过边界。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float GetSignedDepth() const { return LastSignedDepth; }

	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	bool IsRecovering() const { return State.bRecovering; }

	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	int32 GetVolumeCount() const { return Volumes.Num(); }

	/**
	 * 最近一次选中并写进后处理材质的蠕动预设。选中的是"主导体积"（当前最深的那颗）的预设；
	 * 没有主导体积时保持上一次的值，所以传送后的淡出阶段风格不会突然跳回别的档位。
	 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	EFPSAISoftBoundaryCrawlPreset GetAppliedCrawlPreset() const { return AppliedCrawlPreset; }

	/** GetAppliedCrawlPreset 是否已经有值（还没遇到过任何体积时为 false）。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	bool HasAppliedCrawlPreset() const { return bHasAppliedCrawlPreset; }

	/** 重新扫描关卡里的软边界体积。体积是运行时动态生成时手动调一次。 */
	UFUNCTION(BlueprintCallable, Category = "FPSGame_AI|SoftBoundary")
	void RefreshVolumes();

	const FFPSAISoftBoundaryState& GetState() const { return State; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary")
	TSoftObjectPtr<UMaterialInterface> PostProcessMaterial;

	/** Debug/test only: >=0 forces this Darkness (clamped 0..1) into the post process material, ignoring the computed value. <0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary")
	float DebugDarknessOverride = -1.f;

	static const TCHAR* DefaultPostProcessMaterialPath;

protected:

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DarknessMID;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> BoundCamera;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFPSAISoftBoundaryVolume>> Volumes;

	/** 本帧产生最大深度的体积，全黑传送以它为准。 */
	UPROPERTY(Transient)
	TObjectPtr<AFPSAISoftBoundaryVolume> DominantVolume;

private:

	void EnsurePostProcess();
	void PushDarkness(float Darkness);

	/**
	 * 把主导体积的蠕动预设写进后处理 MID。只有预设有变化才写 —— 每帧三次
	 * SetScalarParameterValue 是白烧的（每次都要重建 MID 的参数缓冲并让后处理重编译管线）。
	 * MID 还没建好（没相机 / 材质没加载）时只记下预设，等 MID 建好那一帧再补写。
	 */
	void ApplyCrawlPreset(const AFPSAISoftBoundaryVolume& Volume);

	void TeleportToSafety(APawn* Pawn, AFPSAISoftBoundaryVolume& Volume);
	void ResolveLanding(APawn* Pawn, const AFPSAISoftBoundaryVolume& Volume, const FVector& Desired, FVector& OutLanding) const;

	FFPSAISoftBoundaryState State;

	/** 推到材质上的暗度（上升受 DarkenDuration 限制）。 */
	float SmoothedDarkness = 0.f;

	/** 本帧全局目标暗度（多体积取最大）。 */
	float LastTargetDarkness = 0.f;

	/** 本帧主导体积的有符号深度。 */
	float LastSignedDepth = 0.f;

	/** 最近一次生效（并写进 MID）的蠕动预设；没有主导体积时保持不动，避免风格跳变。 */
	EFPSAISoftBoundaryCrawlPreset AppliedCrawlPreset = EFPSAISoftBoundaryCrawlPreset::Crawl;
	bool bHasAppliedCrawlPreset = false;

	float VolumeRefreshCountdown = 0.f;
	int32 LastLoggedVolumeCount = -1;
	bool bLoggedSetupWarning = false;
};
