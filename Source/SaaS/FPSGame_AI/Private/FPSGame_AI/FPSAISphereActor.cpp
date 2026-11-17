#include "FPSGame_AI/FPSAISphereActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AFPSAISphereActor::AFPSAISphereActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
	SetRootComponent(SphereMesh);

	SphereMesh->SetMobility(EComponentMobility::Movable);
	SphereMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SphereMesh->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		SphereMesh->SetStaticMesh(SphereMeshAsset.Object);
	}
}
