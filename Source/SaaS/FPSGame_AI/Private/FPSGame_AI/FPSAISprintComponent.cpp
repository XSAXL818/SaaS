#include "FPSGame_AI/FPSAISprintComponent.h"

#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

UFPSAISprintComponent::UFPSAISprintComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UFPSAISprintComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheWalkSpeed();

	CachedPawn = Cast<APawn>(GetOwner());
	BindSprintInput();
}

void UFPSAISprintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInputBound)
	{
		BindSprintInput();
	}
}

void UFPSAISprintComponent::CacheWalkSpeed()
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			WalkSpeed = Movement->MaxWalkSpeed;
		}
	}
}

void UFPSAISprintComponent::BindSprintInput()
{
	if (bInputBound || !CachedPawn || !CachedPawn->InputComponent)
	{
		return;
	}

	CachedPawn->InputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &UFPSAISprintComponent::StartSprint);
	CachedPawn->InputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &UFPSAISprintComponent::StopSprint);

	bInputBound = true;
	SetComponentTickEnabled(false);
}

void UFPSAISprintComponent::StartSprint()
{
	bSprinting = true;
	SetOwnerMaxWalkSpeed(SprintSpeed);
}

void UFPSAISprintComponent::StopSprint()
{
	bSprinting = false;
	SetOwnerMaxWalkSpeed(WalkSpeed);
}

void UFPSAISprintComponent::SetOwnerMaxWalkSpeed(float NewSpeed)
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = NewSpeed;
		}
	}
}
