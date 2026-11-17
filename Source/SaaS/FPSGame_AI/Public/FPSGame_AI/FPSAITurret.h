#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSAITurret.generated.h"

class UStaticMeshComponent;

/**
 * 炮台：由底座 (Turret_Base) 和炮塔 (Turret_Top) 两个静态网格体组成。
 * 炮塔底部的球形旋转轴位于底座孔洞内部，炮塔绕该球轴旋转。
 */
UCLASS()
class SAAS_API AFPSAITurret : public AActor
{
	GENERATED_BODY()

public:

	AFPSAITurret();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** 设置炮塔相对底座的朝向（Pitch 会被限制在 MinPitch ~ MaxPitch 之间） */
	UFUNCTION(BlueprintCallable, Category = "FPSGame_AI|Turret")
	void SetTopRotation(float Yaw, float Pitch);

	/** 底座，带孔洞，作为根组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|Turret")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** 炮塔，底部带球形旋转轴，附着在底座上 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|Turret")
	TObjectPtr<UStaticMeshComponent> TopMesh;

	/** 炮塔相对底座抬高的高度（厘米），用于让球形旋转轴落在底座孔洞内部 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|Turret")
	float TopHeightOffset = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|Turret")
	float MinPitch = -20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|Turret")
	float MaxPitch = 45.f;
};
