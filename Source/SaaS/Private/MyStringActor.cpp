// Fill out your copyright notice in the Description page of Project Settings.


#include "MyStringActor.h"
#include "Kismet/KismetStringLibrary.h"



// Sets default values
AMyStringActor::AMyStringActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyStringActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyStringActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AMyStringActor::MyStringOperate()
{
	
	FString MyString = TEXT("123456789_AbcdEFG");
	
	// 查找字符串是否包含指定字符
	int32 isFind = MyString.Find("b",ESearchCase::IgnoreCase,ESearchDir::FromStart);
	
	UE_LOG(LogTemp, Warning, TEXT("%s find b is %d"),*MyString,isFind);
	
	isFind = UKismetStringLibrary::Contains(MyString,"b",false, false);
	
	UE_LOG(LogTemp, Warning, TEXT("%s find b is %d"),*MyString,isFind);
	
	// 判断是否相等
	isFind = MyString.Equals("13");
	
	// 拼接字符串
	FString MyString2 = MyString.Append("1111");
	
	// 长度
	int32 Num = MyString.Len();
	
	// 是否为空
	bool isEmpty = MyString.IsEmpty();
	
	// 返回指定位置的字符串
	FString SubString = UKismetStringLibrary::GetSubstring(MyString, 10, 3);
	

}

