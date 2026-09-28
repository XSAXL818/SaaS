// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"

bool UMyBlueprintFunctionLibrary::LoadStringFromFile(const FString& FilePath, FString &OutString)
{
	if (FFileHelper::LoadFileToString(OutString,*FilePath))
	{
		return true;
	}
	
	return false;
}

bool UMyBlueprintFunctionLibrary::WriteStringToFile(const TArray<FString>& InStringArray, const FString& FilePath)
{
	
	if (FFileHelper::SaveStringArrayToFile(InStringArray,*FilePath))
	{
		return true;
	}
	
	return false;
}

void UMyBlueprintFunctionLibrary::Test1(int Num, int& OutNum)
{
}
