#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSAISprintComponent.generated.h"

class APawn;

UCLASS(ClassGroup = (FPSGame_AI), meta = (BlueprintSpawnableComponent))
class SAAS_API UFPSAISprintComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFPSAISprintComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|Sprint")
	float SprintSpeed = 900.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|Sprint")
	float WalkSpeed = 600.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|Sprint")
	bool bSprinting = false;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void CacheWalkSpeed();
	void BindSprintInput();
	void SetOwnerMaxWalkSpeed(float NewSpeed);

	UFUNCTION()
	void StartSprint();

	UFUNCTION()
	void StopSprint();

	UPROPERTY()
	TObjectPtr<APawn> CachedPawn;

	bool bInputBound = false;
};
