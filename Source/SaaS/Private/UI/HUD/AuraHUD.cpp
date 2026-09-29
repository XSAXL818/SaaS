// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/AuraHUD.h"


#include "UI/Widget/AuraUserWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"

UOverlayWidgetController* AAuraHUD::GetOverlayController(const FWidgetControllerParams& WCParams)
{
	if ( OverlayWidgetController == nullptr )
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
		
	}
	
	return OverlayWidgetController;
}

void AAuraHUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC,
	UAttributeSet* AS)
{
	
	checkf(OverlayWidgetClass, TEXT("Overlay Widget Class 未初始化, 在BP_AuraHUD中填写"));
	checkf(OverlayWidgetControllerClass, TEXT("Overlay Widget Controller Class 未初始化, 在BP_AuraHUD中填写"));
	
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
	OverlayWidget = Cast<UAuraUserWidget>(Widget);
	
	const  FWidgetControllerParams WidgetControllerParams{PC, PS, ASC, AS};
	
	UOverlayWidgetController* WidgetController = GetOverlayController(WidgetControllerParams);
	
	OverlayWidget->SetWidgetController(WidgetController);
	
	
	Widget->AddToViewport();
	
	
	
}
