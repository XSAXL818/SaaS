#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSAISphereActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class SAAS_API AFPSAISphereActor : public AActor
{
	GENERATED_BODY()

public:

	AFPSAISphereActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSGame_AI")
	TObjectPtr<UStaticMeshComponent> SphereMesh;
};
