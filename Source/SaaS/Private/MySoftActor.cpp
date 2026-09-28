// Fill out your copyright notice in the Description page of Project Settings.


#include "MySoftActor.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"


// Sets default values
AMySoftActor::AMySoftActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMySoftActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 同步加载
	// 新版不能直接用 = TEXT()
	FSoftObjectPath Path1{
		// 不是运行关卡中的纹理
		TEXT("/Script/Engine.Texture2D'/Game/Maps/Changan/MESH/JIAFANG/JianZhu_30/CA_WALL_06_ncl1_1.CA_WALL_06_ncl1_1'")
	};
	
	// 同步加载，这个语句制定完后，资源就已经加载完了，可以直接使用
	TSharedPtr<FStreamableHandle> SyncStreamHandle = UAssetManager::GetStreamableManager().RequestSyncLoad(Path1);
	
	if (SyncStreamHandle)
	{
		UTexture2D* Image1 = Cast<UTexture2D>(SyncStreamHandle->GetLoadedAsset());
		if (Image1)
		{
			GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red,FString::Printf(TEXT("Image1: %s"), *Image1->GetName()));
		}
	}
	
	
	// 异步加载
	// PIE和编辑器在同一进程，所以编辑器中加载的资源，在PIE中也可用，运行一次后，除非关闭编辑器，不然资源一直在内存内加载
	FSoftObjectPath Path2{
		TEXT("/Script/Engine.Texture2D'/Game/Fishermans_Cabin/Textures/Tiling_Textures/Rock/T_Rocks_N.T_Rocks_N'")
	};

	TSharedPtr<FStreamableHandle> AsyncStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(Path2);
	
	// 添加回调函数
	if ( AsyncStreamHandle.IsValid() )
	{
		TWeakObjectPtr<AMySoftActor> WeakThis{this};
		AsyncStreamHandle->BindCompleteDelegate(
			FStreamableDelegate::CreateLambda([WeakThis,AsyncStreamHandle]() {
				if ( !WeakThis.IsValid() )
				{
					// 说明当前对象已经被销毁，不应该再执行回调函数了
					return;
				}
				UTexture2D* Image3 = Cast<UTexture2D>(AsyncStreamHandle->GetLoadedAsset());
				if (Image3)
				{
					GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red,FString::Printf(TEXT("回调函数-Image2: %s"), *Image3->GetName()));
				}
			})	
		);
	}
	
	// if (AsyncStreamHandle)
	// {
	// 	UTexture2D* Image2 = Cast<UTexture2D>(AsyncStreamHandle->GetLoadedAsset());
	// 	if (Image2)
	// 	{
	// 		UE_LOG(LogTemp, Warning, TEXT("Image2: %s"), *Image2->GetName());
	// 		GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red,FString::Printf(TEXT("Image2: %s"), *Image2->GetName()));
	// 	} else
	// 	{
	// 		GEngine->AddOnScreenDebugMessage(-1,5.0f,FColor::Red,FString::Printf(TEXT("异步加载还未完成")));
	// 	}
	// }
	

	
}

// Called every frame
void AMySoftActor::Tick(float DeltaTime)
{
	
	Super::Tick(DeltaTime);
	
	
}

