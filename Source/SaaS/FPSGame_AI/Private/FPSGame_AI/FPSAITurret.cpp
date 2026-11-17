#include "FPSGame_AI/FPSAITurret.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AFPSAITurret::AFPSAITurret()
{
	PrimaryActorTick.bCanEverTick = false;

	// 底座：根组件，固定不动
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	SetRootComponent(BaseMesh);
	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 炮塔：附着在底座上，抬高后球形旋转轴位于底座孔洞内
	TopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopMesh"));
	TopMesh->SetupAttachment(BaseMesh);
	TopMesh->SetMobility(EComponentMobility::Movable);
	TopMesh->SetCollisionProfileName(TEXT("BlockAll"));
	TopMesh->SetRelativeLocation(FVector(0.f, 0.f, TopHeightOffset));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BaseMeshAsset(TEXT("/Game/FPSGame_AI/Meshes/Turret_Base.Turret_Base"));
	if (BaseMeshAsset.Succeeded())
	{
		BaseMesh->SetStaticMesh(BaseMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TopMeshAsset(TEXT("/Game/FPSGame_AI/Meshes/Turret_Top.Turret_Top"));
	if (TopMeshAsset.Succeeded())
	{
		TopMesh->SetStaticMesh(TopMeshAsset.Object);
	}
}

void AFPSAITurret::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 在编辑器中调整 TopHeightOffset 后即时生效
	TopMesh->SetRelativeLocation(FVector(0.f, 0.f, TopHeightOffset));
}

void AFPSAITurret::SetTopRotation(float Yaw, float Pitch)
{
	TopMesh->SetRelativeRotation(FRotator(FMath::Clamp(Pitch, MinPitch, MaxPitch), Yaw, 0.f));
}
