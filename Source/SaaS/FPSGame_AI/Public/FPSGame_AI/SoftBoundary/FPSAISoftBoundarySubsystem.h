#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Subsystems/WorldSubsystem.h"
#include "FPSAISoftBoundarySubsystem.generated.h"

/**
 * 极轻量的世界子系统，唯一职责：保证本地玩家 Pawn 上有 UFPSAISoftBoundaryComponent。
 *
 * 它不创建任何体积、不硬编码任何坐标——软边界体积由设计师在关卡里摆放。
 * 之所以留着它而不是让体积自己去挂组件：体积可能在 Pawn 之前就开始播放（也可能一个都没摆、
 * 或者中途被流送进来），由单一所有者轮询"Pawn 有没有组件"是确定性的，不会出现
 * "没有 Actor 就没有组件"的死结，也不需要在每个体积里各写一份重试逻辑。
 */
UCLASS()
class SAAS_API UFPSAISoftBoundarySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UFPSAISoftBoundarySubsystem, STATGROUP_Tickables);
	}

private:

	/** 玩家 Pawn 存在但还没有软边界组件时，把组件挂上去。 */
	void EnsurePawnComponent();

	float ScanCountdown = 0.f;
};
