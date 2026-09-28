// Fill out your copyright notice in the Description page of Project Settings.


#include "MySmartPtrActor.h"


// Sets default values
AMySmartPtrActor::AMySmartPtrActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMySmartPtrActor::BeginPlay()
{
	Super::BeginPlay();
	
	
	TestAFunc();
	TestBFunc();
	TestC();
}

// Called every frame
void AMySmartPtrActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMySmartPtrActor::TestAFunc()
{
	// C++中创建指针
	TestA* Ptr1 = new TestA(1,3.f);
	// 共享指针左值初始化,不推荐使用这种方式，容易混淆共享指针和原生指针
	TSharedPtr<TestA> SharedPtr1{Ptr1};
	Ptr1 = nullptr; // Ptr1原先指向的堆内存由共享指针管理了，置空防止错误使用
	
	// 共享指针右值初始化
	TSharedPtr<TestA> SharedPtr2{new TestA{1,2.f}};
	
	// 拷贝构造函数初始化
	TSharedPtr<TestA> SharedPtr3{ SharedPtr2 };
	
	// UE 推荐方法
	TSharedPtr<TestA> SharedPtr4{};
	SharedPtr4 = MakeShared<TestA>(1,65.f);
	
	// 线程安全的共享
	TSharedPtr<TestA, ESPMode::ThreadSafe> SharedPtr5{new TestA{1,2.f}};
	
	// 共享指针常用的接口
	// 判断是否指向有效的对象
	if (SharedPtr5.IsValid())
	{
		TSharedRef<TestA> SharedRef{new TestA{1,4.f}};
		// 原有对象被引用计数减1，指向新的对象
		SharedRef = SharedPtr5.ToSharedRef();

		int32 SharedCount = SharedPtr5.GetSharedReferenceCount();
		UE_LOG(LogTemp, Warning, TEXT("SharedCount: %d"), SharedCount);
		
		if (!SharedPtr5.IsUnique())
		{
			UE_LOG(LogTemp, Warning, TEXT("SharedPtr5 is Not Unique!") );
		}
		
		UE_LOG(LogTemp, Warning, TEXT("SharedPtr5 解引用得a=%d,b=%f"),SharedPtr5.Get()->a, SharedPtr5.Get()->b );
		
		// 共享指针变为Null
		SharedPtr5.Reset();
		
		UE_LOG(LogTemp, Warning, TEXT("SharedPtr5.Reset()后的引用计数：%d "),SharedPtr5.GetSharedReferenceCount() );
		
		
	}
	
}

void AMySmartPtrActor::TestBFunc()
{
	// 共享引用初始必须指向有效的对象
	TSharedRef<TestA> SharedRef{new TestA{1,4.f}};
	
	// 判断是否是唯一的共享引用
	if (SharedRef.IsUnique())
	{
		UE_LOG(LogTemp,Warning,TEXT("共享引用通过->获取a=%d b=%f"),SharedRef->a,SharedRef->b);
		
		// 共享引用转换为共享指针
		TSharedPtr<TestA> SharedPtr{SharedRef.ToSharedPtr()};
		// 推荐使用->a直接获取，->重载操作符内部进行了check(IsValid())
		UE_LOG(LogTemp,Warning,TEXT("共享引用转换为共享指针后，获取a=%d b=%f"),SharedPtr->a,SharedPtr.Get()->b);

	}
	
	TSharedRef<TestA> SharedRef2{SharedRef};
	
	TSharedRef<TestA> SharedRef3{MakeShared<TestA>(1,65.f)};
	
	TSharedRef<TestA, ESPMode::ThreadSafe> SharedRef4{new TestA{1,2.f}};
}

void AMySmartPtrActor::TestC()
{
	// 弱指针解决循环引用问题，只对弱指针保留引用权，不参与引用计数
	// 不阻止对象被销毁，如果弱指针指向对象被销毁，弱指针会自动清空

	

	
	TSharedPtr<TestA> SharedPtr1 = MakeShared<TestA>(1,65.f);
	
	
	TWeakPtr<TestA> WeakPtr1{SharedPtr1};

	{
		
		
		SharedPtr1.Reset();
	}
	
	if (WeakPtr1.IsValid())
	{
		UE_LOG(LogTemp,Warning,TEXT("WeakPtr1 is Valid!"));
	}else
	{
		UE_LOG(LogTemp,Warning,TEXT("WeakPtr1 is not Valid!"));
	}
}

