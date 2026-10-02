// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/AuraHUD.h"


#include "UI/Widget/AuraUserWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"

// 获取Overlay控制器，首次获取会初始化控制器
UOverlayWidgetController* AAuraHUD::GetOverlayController(const FWidgetControllerParams& WCParams)
{
	if ( OverlayWidgetController == nullptr )
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
		// 绑定AttributeSet中属性的值变化的广播
		OverlayWidgetController->BindCallbacksToDependencies();
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
	
	// 广播初始化
	WidgetController->BroadcastInitialValue();
	
	
	Widget->AddToViewport();
	
	
	
}
