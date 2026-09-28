// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SAAS_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	// 转化为蓝图节点后：const & 会被当做入参，&会被当做出参
	
	UFUNCTION(BlueprintCallable, Category = "FileOperate")
	static bool LoadStringFromFile(const FString& FilePath, FString& OutString);
	
	UFUNCTION(BlueprintCallable, Category="FileOperate")
	static bool WriteStringToFile(const TArray<FString>& InStringArray, const FString& FilePath);
	
	UFUNCTION(BlueprintCallable, Category="FileOperate")
	static void Test1(int Num, int& OutNum);
	
};
