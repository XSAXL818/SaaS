#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryComponent.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryMath.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryVolume.h"
#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"

#if WITH_EDITOR
// 数据校验用例要直接调用 AFPSAISoftBoundaryVolume::IsDataValid。
#include "Misc/DataValidation.h"
#endif

namespace FPSAISoftBoundaryTestPrivate
{
	struct FTestWorld
	{
		UWorld* World = nullptr;

		bool Open()
		{
			World = UWorld::CreateWorld(EWorldType::Game, true, TEXT("FPSAISoftBoundaryTestWorld"));
			return World != nullptr;
		}

		void Close()
		{
			if (!World)
			{
				return;
			}
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
			World->DestroyWorld(false);
			World = nullptr;
		}
	};

	/** 摆一个软边界体积并给它尺寸/模式/触发带宽度。 */
	AFPSAISoftBoundaryVolume* SpawnVolume(
		UWorld* World,
		const FVector& Center,
		const FVector& Extent,
		EFPSAISoftBoundaryMode Mode,
		float FullBlackDistance)
	{
		AFPSAISoftBoundaryVolume* Volume = World->SpawnActor<AFPSAISoftBoundaryVolume>(
			AFPSAISoftBoundaryVolume::StaticClass(), FTransform(FRotator::ZeroRotator, Center));
		if (!Volume)
		{
			return nullptr;
		}

		Volume->Mode = Mode;
		Volume->FullBlackDistance = FullBlackDistance;
		if (UBoxComponent* Bounds = Volume->GetVolumeBounds())
		{
			Bounds->SetBoxExtent(Extent);
		}
		// 直接改属性不会经过 PostEditChangeProperty，手动跑一次构造脚本让触发带框跟上新尺寸。
		Volume->OnConstruction(Volume->GetActorTransform());
		return Volume;
	}

	/**
	 * 自定义厚度的楼板：顶面 TopZ，底面 TopZ - Thickness。"体积沉进楼板"的用例要控制向上
	 * 补打时先碰到哪个面，所以厚度必须能指定。
	 */
	UBoxComponent* SpawnFloorPlate(UWorld* World, float TopZ, float Thickness, float HalfXY)
	{
		const FVector SlabCenter(0.f, 0.f, TopZ - Thickness * 0.5f);

		AActor* Floor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(SlabCenter));
		if (!Floor)
		{
			return nullptr;
		}

		UBoxComponent* Slab = NewObject<UBoxComponent>(Floor, TEXT("TestFloorSlab"));
		Floor->SetRootComponent(Slab);
		Slab->SetBoxExtent(FVector(HalfXY, HalfXY, Thickness * 0.5f));
		// 和下面 blocker 一样：QueryOnly 不挡 sweep，必须 QueryAndPhysics + 全通道 Block。
		Slab->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Slab->SetCollisionObjectType(ECC_WorldStatic);
		Slab->SetCollisionResponseToAllChannels(ECR_Block);
		Slab->RegisterComponent();
		// SetWorldLocation 必须显式调用：NewObject + SetRootComponent + RegisterComponent 不会可靠继承
		// SpawnActor 的变换，历史上这里踩过坑（盒子会留在原点）。
		Slab->SetWorldLocation(SlabCenter);
		return Slab;
	}

	/**
	 * 在测试世界里铺一块"地面/楼板"：用 UBoxComponent 当碰撞体（不依赖任何静态网格资产），
	 * 顶面正好落在 TopZ 上、厚 100cm，XY 各覆盖 ±HalfXY。返回碰撞盒。
	 *
	 * 竖直可达性现在靠"向体积脚下打射线取地面"，所以任何要验证"能不能装下玩家"的用例都必须
	 * 先有真实地面，否则判定只会得到 GroundUnknown。
	 */
	UBoxComponent* SpawnFloorSlab(UWorld* World, float TopZ, float HalfXY)
	{
		constexpr float Thickness = 100.f;
		return SpawnFloorPlate(World, TopZ, Thickness, HalfXY);
	}

	/** 给 Pawn 挂一个软边界组件并跑一帧。 */
	UFPSAISoftBoundaryComponent* AttachComponent(APawn* Pawn)
	{
		UFPSAISoftBoundaryComponent* Component = NewObject<UFPSAISoftBoundaryComponent>(Pawn, TEXT("FPSAISoftBoundaryTest"));
		if (!Component)
		{
			return nullptr;
		}
		Component->RegisterComponent();
		return Component;
	}

	void TickComponentOnce(UFPSAISoftBoundaryComponent* Component, float DeltaTime)
	{
		Component->TickComponent(DeltaTime, LEVELTICK_All, &Component->PrimaryComponentTick);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryDarknessIncreasesTest,
	"SaaS.FPSGame_AI.SoftBoundary.DarknessIncreasesWithDepth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryDarknessIncreasesTest::RunTest(const FString& Parameters)
{
	const float FullBlack = 800.f;
	const float Inside = FPSAISoftBoundaryMath::ComputeTargetDarkness(-100.f, FullBlack);
	const float Edge = FPSAISoftBoundaryMath::ComputeTargetDarkness(0.f, FullBlack);
	const float Mid = FPSAISoftBoundaryMath::ComputeTargetDarkness(400.f, FullBlack);
	const float Far = FPSAISoftBoundaryMath::ComputeTargetDarkness(800.f, FullBlack);
	const float Past = FPSAISoftBoundaryMath::ComputeTargetDarkness(2000.f, FullBlack);

	TestEqual(TEXT("Center darkness is 0"), Inside, 0.f);
	TestEqual(TEXT("Boundary darkness is 0"), Edge, 0.f);
	TestTrue(TEXT("Mid darkness is between 0 and 1"), Mid > 0.f && Mid < 1.f);
	TestTrue(TEXT("Darkness increases with depth"), Mid > Edge && Far > Mid);
	TestEqual(TEXT("Full black at configured distance"), Far, 1.f);
	TestEqual(TEXT("Past the band stays black"), Past, 1.f);

	FFPSAISoftBoundaryState State;
	FFPSAISoftBoundarySettings Settings;
	Settings.FullBlackDistanceCm = FullBlack;
	float Previous = 0.f;
	for (float Distance = 0.f; Distance <= FullBlack; Distance += 100.f)
	{
		const FFPSAISoftBoundaryUpdate Update = FPSAISoftBoundaryMath::UpdateState(State, Distance, 0.05f, Settings);
		TestTrue(TEXT("Display darkness does not drop while walking out"), Update.Darkness + KINDA_SMALL_NUMBER >= Previous);
		Previous = Update.Darkness;
	}
	TestTrue(TEXT("Full black triggers teleport"), State.bRecovering);

	FFPSAISoftBoundaryState ReturnState;
	FPSAISoftBoundaryMath::UpdateState(ReturnState, 300.f, 0.05f, Settings);
	const float Dark = ReturnState.DisplayDarkness;
	bool bTeleported = false;
	for (int32 Step = 0; Step < 40; ++Step)
	{
		const FFPSAISoftBoundaryUpdate Back = FPSAISoftBoundaryMath::UpdateState(ReturnState, -200.f, 0.05f, Settings);
		bTeleported = bTeleported || Back.bTeleport;
	}
	TestFalse(TEXT("Walking back before full black does not teleport"), bTeleported);
	TestTrue(TEXT("Walking back reduces darkness"), ReturnState.DisplayDarkness < Dark);

	// 同样的深度，KeepOut 的符号语义必须反过来：盒内为正（越往里越暗），盒外为负。
	const float KeepOutInsideDepth = FPSAISoftBoundaryMath::ComputeSignedDepth(-400.f, true);
	const float KeepOutOutsideDepth = FPSAISoftBoundaryMath::ComputeSignedDepth(400.f, true);
	const float KeepInOutsideDepth = FPSAISoftBoundaryMath::ComputeSignedDepth(400.f, false);
	TestEqual(TEXT("KeepOut depth is positive inside the volume"), KeepOutInsideDepth, 400.f);
	TestEqual(TEXT("KeepOut depth is negative outside the volume"), KeepOutOutsideDepth, -400.f);
	TestEqual(TEXT("KeepIn depth is positive outside the volume"), KeepInOutsideDepth, 400.f);
	TestTrue(TEXT("KeepOut inside darkens"),
		FPSAISoftBoundaryMath::ComputeTargetDarkness(KeepOutInsideDepth, FullBlack) > 0.f);
	TestEqual(TEXT("KeepOut outside stays clear"),
		FPSAISoftBoundaryMath::ComputeTargetDarkness(KeepOutOutsideDepth, FullBlack), 0.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryCylinderTest,
	"SaaS.FPSGame_AI.SoftBoundary.CylinderDepthMatchesRadiusAndHalfHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryCylinderTest::RunTest(const FString& Parameters)
{
	const FTransform Transform(FRotator::ZeroRotator, FVector::ZeroVector);
	// Radius must come from the smaller horizontal extent (500), half height from Z (400).
	const FVector Extent(500.f, 800.f, 400.f);

	TestEqual(TEXT("Radius is the smaller horizontal extent"), FPSAISoftBoundaryMath::GetCylinderRadius(Extent), 500.f);
	TestEqual(
		TEXT("Radius ignores axis order"),
		FPSAISoftBoundaryMath::GetCylinderRadius(FVector(800.f, 500.f, 400.f)),
		500.f);

	TestEqual(TEXT("Inside the barrel is negative"),
		FPSAISoftBoundaryMath::ComputeCylinderOutsideDistance(FVector(300.f, 0.f, 0.f), Transform, 500.f, 400.f), -200.f);
	TestEqual(TEXT("Beyond the radius is positive"),
		FPSAISoftBoundaryMath::ComputeCylinderOutsideDistance(FVector(700.f, 0.f, 0.f), Transform, 500.f, 400.f), 200.f);
	TestEqual(TEXT("Above the cap is positive"),
		FPSAISoftBoundaryMath::ComputeCylinderOutsideDistance(FVector(0.f, 0.f, 500.f), Transform, 500.f, 400.f), 100.f);
	TestEqual(TEXT("Past the radius but under the cap uses the horizontal delta"),
		FPSAISoftBoundaryMath::ComputeCylinderOutsideDistance(FVector(300.f, 0.f, 500.f), Transform, 500.f, 400.f), 100.f);
	TestEqual(TEXT("Outside on both axes combines in 3D"),
		FPSAISoftBoundaryMath::ComputeCylinderOutsideDistance(FVector(700.f, 0.f, 500.f), Transform, 500.f, 400.f),
		FMath::Sqrt(50000.f), 0.01f);

	// The corner of the bounding box is outside the cylinder: the dispatcher must use the round shape.
	const FVector Corner(400.f, 400.f, 0.f);
	const float BoxOutside = FPSAISoftBoundaryMath::ComputeShapeOutsideDistance(Corner, Transform, Extent, false);
	const float CylinderOutside = FPSAISoftBoundaryMath::ComputeShapeOutsideDistance(Corner, Transform, Extent, true);
	TestEqual(TEXT("Box keeps the corner inside"), BoxOutside, -100.f);
	TestTrue(TEXT("Cylinder has the corner outside"), CylinderOutside > 0.f);
	TestEqual(TEXT("Cylinder corner distance matches the radial gap"),
		CylinderOutside, FMath::Sqrt(400.f * 400.f + 400.f * 400.f) - 500.f, 0.01f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryKeepInTeleportTest,
	"SaaS.FPSGame_AI.SoftBoundary.KeepInFullBlackTeleportsInsideFacingIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryKeepInTeleportTest::RunTest(const FString& Parameters)
{
	const FTransform Box(FRotator::ZeroRotator, FVector(1000.f, 2000.f, 100.f));
	const FVector Extent(1500.f, 1200.f, 600.f);
	const FVector Outside(1000.f + 1500.f + 800.f, 2000.f, 180.f);

	// Pure geometry first: the preserved box teleport still lands inside, facing in.
	const FFPSAISoftBoundaryTeleport Teleport = FPSAISoftBoundaryMath::ComputeTeleport(Outside, Box, Extent, 350.f);
	TestTrue(TEXT("Landing is inside with margin"), FPSAISoftBoundaryMath::IsInsideBox(Teleport.WorldLocation, Box, Extent, 200.f));
	const FVector ToCenter = (Box.GetLocation() - Teleport.WorldLocation).GetSafeNormal2D();
	const FVector Facing = FRotator(0.f, Teleport.YawDegrees, 0.f).Vector().GetSafeNormal2D();
	TestTrue(TEXT("Yaw faces inward"), FVector::DotProduct(Facing, ToCenter) > 0.9f);

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Box.GetLocation(), Extent, EFPSAISoftBoundaryMode::KeepIn, 800.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}
	Volume->ReturnDistance = 350.f;

	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(Outside));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}
	Component->RefreshVolumes();
	TestEqual(TEXT("Component discovered the volume"), Component->GetVolumeCount(), 1);

	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	const FVector After = Character->GetActorLocation();
	TestTrue(
		TEXT("Pawn is inside after full black"),
		FPSAISoftBoundaryMath::ComputeOutsideDistance(After, Volume->GetVolumeTransform(), Volume->GetVolumeExtent()) < -100.f);
	const FVector AfterFacing = Character->GetActorRotation().Vector().GetSafeNormal2D();
	const FVector AfterToCenter = (Volume->GetActorLocation() - After).GetSafeNormal2D();
	TestTrue(TEXT("Pawn yaw faces inward"), FVector::DotProduct(AfterFacing, AfterToCenter) > 0.9f);
	if (const UPawnMovementComponent* Movement = Character->GetMovementComponent())
	{
		TestTrue(TEXT("Velocity cleared"), Movement->Velocity.IsNearlyZero());
	}

	// 落点在安全一侧，下一帧目标暗度必须回到 0。
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestEqual(TEXT("Back inside means no target darkness"), Component->GetTargetDarkness(), 0.f);
	TestTrue(TEXT("Landing was remembered as a safe location"), Volume->HasSafeLocation());

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryKeepOutTest,
	"SaaS.FPSGame_AI.SoftBoundary.KeepOutDarkensInsideAndTeleportsOutsideFacingOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryKeepOutTest::RunTest(const FString& Parameters)
{
	const FVector Center(0.f, 0.f, 100.f);
	const FVector Extent(1200.f, 1200.f, 600.f);
	const float FullBlack = 500.f;

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, FullBlack);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}
	Volume->ReturnDistance = 400.f;

	// KeepOut 的深度是水平嵌入深度：盒内 XY 中心处一定是水平最薄半轴（1200），
	// 不是"到最近面"的 600 —— 这一条就是贴地摆放的盒子能不能触发全部区别所在。
	AddInfo(FString::Printf(TEXT("KeepOut centre depth = %.1f cm, half height = %.1f cm"),
		Volume->EvaluateDepth(Center), Extent.Z));
	TestEqual(TEXT("Depth at the XY centre is the thinnest horizontal half axis"),
		Volume->EvaluateDepth(Center), 1200.f, 1.f);

	// 浅进去 100cm：贴着 +X 面，水平深度 100。
	const FVector Shallow(1100.f, 100.f, 600.f);
	TestEqual(TEXT("Depth near the +X face is the horizontal penetration"),
		Volume->EvaluateDepth(Shallow), 100.f, 1.f);
	TestTrue(TEXT("KeepOut depth is positive inside the box"), Volume->EvaluateDepth(Shallow) > 0.f);
	TestTrue(TEXT("KeepOut depth is negative outside the box"), Volume->EvaluateDepth(FVector(3000.f, 100.f, 600.f)) < 0.f);

	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(Shallow));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	// 浅进去 100cm：目标暗度 > 0 但远没到全黑，不该传送（100 / 500 = 0.2）。
	Component->RefreshVolumes();
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	const float TargetDepth = Component->GetTargetDarkness();
	TestTrue(TEXT("Standing inside a KeepOut volume produces target darkness"), TargetDepth > 0.f);
	TestTrue(TEXT("Shallow penetration is not full black"), TargetDepth < 0.99f);
	TestEqual(TEXT("Signed depth equals the penetration"), Component->GetSignedDepth(), 100.f, 1.f);
	TestTrue(TEXT("Player not moved yet"), Character->GetActorLocation().Equals(Shallow, 1.f));

	// 深入 520cm：超过全黑距离，必须被弹出到体积外面，而不是送到禁区中心。
	const FVector Deep(300.f, 100.f, 180.f);
	Character->SetActorLocation(Deep, false, nullptr, ETeleportType::TeleportPhysics);
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);

	const FVector After = Character->GetActorLocation();
	const float AfterOutside = Volume->EvaluateOutsideDistance(After);
	AddInfo(FString::Printf(TEXT("KeepOut landing = %s, outside distance = %.1f"), *After.ToString(), AfterOutside));

	TestFalse(TEXT("Player left the forbidden volume"), AfterOutside < 0.f);
	TestTrue(TEXT("Landing sits outside the volume"), AfterOutside > 100.f);
	TestTrue(TEXT("Landing is not the volume center"), !After.Equals(Center, 100.f));

	const FVector AfterFacing = Character->GetActorRotation().Vector().GetSafeNormal2D();
	const FVector Outward = (After - Center).GetSafeNormal2D();
	TestTrue(TEXT("Pawn yaw faces away from the volume center"), FVector::DotProduct(AfterFacing, Outward) > 0.9f);
	if (const UPawnMovementComponent* Movement = Character->GetMovementComponent())
	{
		TestTrue(TEXT("Velocity cleared"), Movement->Velocity.IsNearlyZero());
	}

	// 弹出去之后目标暗度归零，并且这个落点被记成安全点。
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestEqual(TEXT("Outside the KeepOut volume means no target darkness"), Component->GetTargetDarkness(), 0.f);
	TestTrue(TEXT("KeepOut remembered the landing as a safe location"), Volume->HasSafeLocation());

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryMultiVolumeTest,
	"SaaS.FPSGame_AI.SoftBoundary.MultiVolumeTakesMaxTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryMultiVolumeTest::RunTest(const FString& Parameters)
{
	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	// A: KeepIn, pawn is 500cm outside it -> 500/800 = 0.625
	AFPSAISoftBoundaryVolume* KeepInVolume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, FVector(0.f, 0.f, 100.f), FVector(500.f, 500.f, 500.f),
		EFPSAISoftBoundaryMode::KeepIn, 800.f);
	// B: KeepOut, pawn sits at its center -> 200cm deep -> 200/800 = 0.25
	AFPSAISoftBoundaryVolume* KeepOutVolume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, FVector(1000.f, 0.f, 100.f), FVector(200.f, 200.f, 200.f),
		EFPSAISoftBoundaryMode::KeepOut, 800.f);

	if (!TestNotNull(TEXT("KeepIn volume spawned"), KeepInVolume) || !TestNotNull(TEXT("KeepOut volume spawned"), KeepOutVolume))
	{
		TestWorld.Close();
		return false;
	}

	const FVector PawnLocation(1000.f, 0.f, 100.f);
	TestEqual(TEXT("KeepIn volume depth"), KeepInVolume->EvaluateDepth(PawnLocation), 500.f, 1.f);
	TestEqual(TEXT("KeepOut volume depth"), KeepOutVolume->EvaluateDepth(PawnLocation), 200.f, 1.f);

	// 这颗 KeepOut 只有 200cm 的最薄轴、却填了 800 的全黑距离：自动适配会把它压到 200，
	// 目标是直接全黑（P2 修的就是这件事）。但本用例验证的是"多体积取最大"这一件事，
	// 需要两个不同的、都不到 1 的目标值，所以这里显式关掉自动适配保留 0.625 / 0.25。
	KeepInVolume->bAutoFitFullBlack = false;
	KeepOutVolume->bAutoFitFullBlack = false;

	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(PawnLocation));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	TestEqual(TEXT("Component discovered both volumes"), Component->GetVolumeCount(), 2);

	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);

	// 0.625 = max(0.625, 0.25)。若实现成取最小会是 0.25，取和会是 0.875。
	TestEqual(TEXT("Global target darkness is the maximum over volumes"), Component->GetTargetDarkness(), 0.625f, 0.01f);
	TestEqual(TEXT("Dominant volume depth drives the signed depth"), Component->GetSignedDepth(), 500.f, 1.f);
	TestTrue(TEXT("No teleport while below full black"), Character->GetActorLocation().Equals(PawnLocation, 1.f));

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundarySweepTest,
	"SaaS.FPSGame_AI.SoftBoundary.SweepThroughBoundaryIsNotBlocking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundarySweepTest::RunTest(const FString& Parameters)
{
	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	const FVector Center(0.f, 0.f, 100.f);
	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, FVector(400.f, 400.f, 200.f), EFPSAISoftBoundaryMode::KeepIn, 600.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}

	TestEqual(TEXT("Volume bounds are not blocking collision"),
		Volume->GetVolumeBounds()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	// KeepIn 的触发带框比主体框大得多，必须同样没有碰撞。
	TestEqual(TEXT("Trigger band is not blocking collision"),
		Volume->GetTriggerBand()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestTrue(TEXT("Trigger band extends past the volume"),
		Volume->GetTriggerBand()->GetScaledBoxExtent().X > Volume->GetVolumeBounds()->GetScaledBoxExtent().X);

	AActor* Blocker = TestWorld.World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FVector(2000.f, 0.f, 100.f)));
	UBoxComponent* BlockingBox = NewObject<UBoxComponent>(Blocker, TEXT("BlockingBox"));
	Blocker->SetRootComponent(BlockingBox);
	BlockingBox->SetBoxExtent(FVector(50.f));
	// QueryOnly produces overlap queries only and never blocks a sweep, so the control
	// needs QueryAndPhysics plus an explicit blocking response on every channel.
	BlockingBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingBox->SetCollisionObjectType(ECC_WorldStatic);
	BlockingBox->SetCollisionResponseToAllChannels(ECR_Block);
	BlockingBox->RegisterComponent();
	// NewObject + SetRootComponent + RegisterComponent does not always inherit the spawn
	// transform, so pin the world location explicitly; otherwise the box sits at origin.
	BlockingBox->SetWorldLocation(FVector(2000.f, 0.f, 100.f));
	AddInfo(FString::Printf(TEXT("Blocker world loc = %s"), *Blocker->GetActorLocation().ToString()));

	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(34.f, 88.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FPSAISoftBoundarySweep), false);
	FHitResult BoundaryHit;
	// Sweep on a plane well above the 400cm-tall boundary box (Z in [-100,300]) and its
	// floor, so this path can never pass through any obstacle; only an invisible wall
	// from the soft boundary itself could register here.
	const bool bBoundaryHit = TestWorld.World->SweepSingleByChannel(
		BoundaryHit,
		FVector(-800.f, 0.f, 400.f),
		FVector(800.f, 0.f, 400.f),
		FQuat::Identity,
		ECC_Pawn,
		Capsule,
		Params);
	AddInfo(FString::Printf(
		TEXT("Soft-boundary sweep: anyHit=%d blocking=%d hitActor=%s"),
		bBoundaryHit ? 1 : 0,
		(bBoundaryHit && BoundaryHit.bBlockingHit) ? 1 : 0,
		BoundaryHit.GetActor() ? *BoundaryHit.GetActor()->GetName() : TEXT("None")));
	TestFalse(TEXT("Sweep through soft boundary has no blocking hit"), bBoundaryHit && BoundaryHit.bBlockingHit);

	FHitResult BlockerHit;
	const bool bBlockerHit = TestWorld.World->SweepSingleByChannel(
		BlockerHit,
		FVector(1600.f, 0.f, 100.f),
		FVector(2400.f, 0.f, 100.f),
		FQuat::Identity,
		ECC_Pawn,
		Capsule,
		Params);
	AddInfo(FString::Printf(
		TEXT("Control sweep: anyHit=%d blocking=%d hitActor=%s"),
		bBlockerHit ? 1 : 0,
		(bBlockerHit && BlockerHit.bBlockingHit) ? 1 : 0,
		BlockerHit.GetActor() ? *BlockerHit.GetActor()->GetName() : TEXT("None")));
	TestTrue(TEXT("Control sweep hits a blocking box"), bBlockerHit && BlockerHit.bBlockingHit);

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryThinAutoFitTest,
	"SaaS.FPSGame_AI.SoftBoundary.ThinVolumeAutoFitReachesFullBlackAndTeleports",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryThinAutoFitTest::RunTest(const FString& Parameters)
{
	// P2 回归：薄盒子 Extent=(120,600,150) + FullBlackDistance=600。盒内最深处只有 120cm，
	// 按原值目标暗度最多 120/600=0.2，永远到不了全黑、永远不传送。开了自动适配必须能全黑并弹出。
	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	const FVector Center(0.f, 0.f, 100.f);
	const FVector ThinExtent(120.f, 600.f, 150.f);
	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, ThinExtent, EFPSAISoftBoundaryMode::KeepOut, 600.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}
	Volume->ReturnDistance = 400.f;

	TestTrue(TEXT("Auto-fit is on by default"), Volume->bAutoFitFullBlack);
	TestEqual(TEXT("Reachable depth is the thinnest horizontal half axis"), Volume->GetMaxReachableDepth(), 120.f, 0.01f);
	TestEqual(TEXT("Effective full black distance is clamped to the thinnest axis"),
		Volume->GetEffectiveFullBlackDistance(), 120.f, 0.01f);
	TestEqual(TEXT("Read-only EffectiveFullBlackDistance mirrors the effective value"),
		Volume->EffectiveFullBlackDistance, 120.f, 0.01f);
	TestFalse(TEXT("Auto-fit means the configuration is not flagged unreachable"), Volume->IsFullBlackUnreachable());
	TestEqual(TEXT("Settings handed to the math layer use the effective distance"),
		Volume->MakeSettings().FullBlackDistanceCm, 120.f, 0.01f);

	// 站在盒内最深处（中心）：有效距离 120 -> 目标暗度正好 1.0 -> 当帧全黑并传送。
	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(Center));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	TestEqual(TEXT("Component discovered the thin volume"), Component->GetVolumeCount(), 1);
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);

	TestEqual(TEXT("Deepest point of a thin volume reaches full black"),
		Component->GetTargetDarkness(), 1.f, 0.01f);
	TestTrue(TEXT("Signed depth equals the thinnest half axis"), FMath::IsNearlyEqual(Component->GetSignedDepth(), 120.f, 1.f));

	const FVector After = Character->GetActorLocation();
	const float AfterOutside = Volume->EvaluateOutsideDistance(After);
	AddInfo(FString::Printf(TEXT("Thin KeepOut landing = %s, outside distance = %.1f"), *After.ToString(), AfterOutside));
	TestTrue(TEXT("Pawn was teleported out of the thin volume"), AfterOutside > 100.f);
	TestFalse(TEXT("Pawn no longer sits at the deepest point"), After.Equals(Center, 1.f));

	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestEqual(TEXT("Back outside means no target darkness"), Component->GetTargetDarkness(), 0.f);

	TestWorld.Close();

	// 形状相关的有效距离单独用一个干净世界验：KeepIn 的越界深度没有上限，
	// 混在上面的世界里会让它成为主导体积，干扰"薄 KeepOut 能全黑"这一条。
	FPSAISoftBoundaryTestPrivate::FTestWorld ShapeWorld;
	if (!ShapeWorld.Open())
	{
		AddError(TEXT("Failed to create the shape test world"));
		return false;
	}

	// 圆柱的最薄轴深度取水平半径 min(Extent.X, Extent.Y)：深度只看水平内缩，半高不再参与。
	AFPSAISoftBoundaryVolume* Cylinder = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		ShapeWorld.World, FVector(0.f, 0.f, 100.f), FVector(300.f, 900.f, 200.f), EFPSAISoftBoundaryMode::KeepOut, 600.f);
	if (!TestNotNull(TEXT("Cylinder volume spawned"), Cylinder))
	{
		ShapeWorld.Close();
		return false;
	}
	Cylinder->Shape = EFPSAISoftBoundaryShape::Cylinder;
	TestEqual(TEXT("Cylinder reachable depth is the horizontal radius"), Cylinder->GetMaxReachableDepth(), 300.f, 0.01f);
	TestEqual(TEXT("Cylinder effective distance is clamped to that depth"),
		Cylinder->GetEffectiveFullBlackDistance(), 300.f, 0.01f);

	// KeepIn 靠越界距离变暗，往外没有上限，几何不该削减它的触发带宽度。
	AFPSAISoftBoundaryVolume* KeepIn = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		ShapeWorld.World, FVector(6000.f, 0.f, 100.f), FVector(120.f, 600.f, 150.f), EFPSAISoftBoundaryMode::KeepIn, 600.f);
	if (!TestNotNull(TEXT("KeepIn volume spawned"), KeepIn))
	{
		ShapeWorld.Close();
		return false;
	}
	TestEqual(TEXT("KeepIn keeps the configured distance"), KeepIn->GetEffectiveFullBlackDistance(), 600.f, 0.01f);
	TestFalse(TEXT("KeepIn is never flagged unreachable"), KeepIn->IsFullBlackUnreachable());

	ShapeWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryThinNoAutoFitTest,
	"SaaS.FPSGame_AI.SoftBoundary.ThinVolumeWithoutAutoFitNeverReachesFullBlack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryThinNoAutoFitTest::RunTest(const FString& Parameters)
{
	// 同一颗薄盒子，关掉自动适配：证明"永远到不了全黑"这个条件是真实存在的，
	// 所以警告（编辑器文本 / 数据校验 / BeginPlay 日志）不是多余的。
	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	const FVector Center(0.f, 0.f, 100.f);
	const FVector ThinExtent(120.f, 600.f, 150.f);
	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, ThinExtent, EFPSAISoftBoundaryMode::KeepOut, 600.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}
	Volume->ReturnDistance = 400.f;
	Volume->bAutoFitFullBlack = false;
	// 直接改属性不经过 PostEditChangeProperty；手动跑一次构造脚本，让只读字段和警告字跟上。
	Volume->OnConstruction(Volume->GetActorTransform());

	TestEqual(TEXT("Without auto-fit the configured distance is kept"),
		Volume->GetEffectiveFullBlackDistance(), 600.f, 0.01f);
	TestEqual(TEXT("Read-only field shows the kept distance"), Volume->EffectiveFullBlackDistance, 600.f, 0.01f);
	TestEqual(TEXT("Reachable depth is still the thinnest horizontal half axis"), Volume->GetMaxReachableDepth(), 120.f, 0.01f);
	TestTrue(TEXT("Unreachable condition holds: thinnest axis < FullBlackDistance"), Volume->IsFullBlackUnreachable());

	// 盒内最深处 -> 120/600 = 0.2，远不到 0.995 的传送阈值。
	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FTransform(Center));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestEqual(TEXT("Deepest point only reaches one fifth of the band"),
		Component->GetTargetDarkness(), 0.2f, 0.001f);
	TestTrue(TEXT("Target darkness stays below the teleport threshold"), Component->GetTargetDarkness() < 0.995f);
	TestTrue(TEXT("Pawn is not teleported"), Character->GetActorLocation().Equals(Center, 1.f));

	// 一直站着也不会传送：暗度停在 0.2，位置一动不动。
	for (int32 Step = 0; Step < 60; ++Step)
	{
		FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	}
	AddInfo(FString::Printf(TEXT("Darkness after 60 ticks = %.4f, location = %s"),
		Component->GetDarkness(), *Character->GetActorLocation().ToString()));
	TestTrue(TEXT("Darkness never reaches full black"), Component->GetDarkness() < 0.995f);
	TestTrue(TEXT("Pawn never teleports while the volume is unreachable"), Character->GetActorLocation().Equals(Center, 1.f));

#if WITH_EDITOR
	// A.3：数据校验必须把这条配置报出来，而不是等人发现"怎么从来不传送"。
	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Volume->IsDataValid(ValidationContext);
	bool bFoundError = false;
	for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
	{
		if (Issue.Severity == EMessageSeverity::Error && Issue.Message.ToString().Contains(TEXT("never reach full black")))
		{
			bFoundError = true;
		}
	}
	AddInfo(FString::Printf(TEXT("Data validation issues = %d, matched full-black error = %d"),
		ValidationContext.GetIssues().Num(), bFoundError ? 1 : 0));
	TestTrue(TEXT("Data validation reports the unreachable configuration"), bFoundError);
	TestTrue(TEXT("Data validation result is Invalid"), ValidationResult == EDataValidationResult::Invalid);
#endif

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryGroundAlignedKeepOutTest,
	"SaaS.FPSGame_AI.SoftBoundary.GroundAlignedKeepOutReachesFullBlack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryGroundAlignedKeepOutTest::RunTest(const FString& Parameters)
{
	// 路口用的盒子必然是贴地摆的：盒底面落在 z=0 上，中心 z 就等于半高。玩家站在地面时
	// 身体中心 z≈100，到盒底面的距离天然很小，深度若仍按"到最近面的距离"算，就会被 Z 锁死
	// 在半高上，无论往前走多远都到不了全黑、也永远不传送，而且没有任何提示。
	// 这颗体积的参数与 L_SoftBoundaryTest 里的 SB_NormalKeepOut 完全一致。
	const FVector Extent(400.f, 400.f, 300.f);
	const FVector Center(0.f, 0.f, Extent.Z);
	const float FullBlack = 250.f;
	const float PlayerZ = 100.f;

	// 纯几何：站在盒的 XY 中心，深度是水平最薄半轴 400，而不是被 Z 锁死的 100。
	const FTransform BoxTransform(FRotator::ZeroRotator, Center);
	TestEqual(TEXT("Math layer reports the horizontal inset at the XY centre"),
		FPSAISoftBoundaryMath::ComputeKeepOutDepthBox(FVector(0.f, 0.f, PlayerZ), BoxTransform, Extent), 400.f, 0.01f);

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, FullBlack);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}
	Volume->ReturnDistance = 400.f;

	// 可达深度是水平最薄半轴 400（旧模型是 min3(400,400,300)=300），240 之外不再被钳制。
	TestEqual(TEXT("Reachable depth is the thinnest horizontal half axis"),
		Volume->GetMaxReachableDepth(), 400.f, 0.01f);
	TestEqual(TEXT("Effective full black distance keeps the configured value"),
		Volume->GetEffectiveFullBlackDistance(), FullBlack, 0.01f);
	TestFalse(TEXT("A floor-resting volume is not flagged as thin"), Volume->IsFullBlackUnreachable());
	TestEqual(TEXT("Volume depth at the XY centre is the horizontal inset"),
		Volume->EvaluateDepth(FVector(0.f, 0.f, PlayerZ)), 400.f, 0.01f);

	// 从盒外沿 +X 走进来：深度随水平位置线性增长，旧模型在这里全程停在 100。
	AddInfo(FString::Printf(TEXT("Depth outside the +X face = %.1f, at X=-300 = %.1f, at X=-150 = %.1f, centre = %.1f"),
		Volume->EvaluateDepth(FVector(500.f, 0.f, PlayerZ)),
		Volume->EvaluateDepth(FVector(-300.f, 0.f, PlayerZ)),
		Volume->EvaluateDepth(FVector(-150.f, 0.f, PlayerZ)),
		Volume->EvaluateDepth(FVector(0.f, 0.f, PlayerZ))));
	TestTrue(TEXT("Outside the +X face is a safe negative depth"), Volume->EvaluateDepth(FVector(500.f, 0.f, PlayerZ)) < 0.f);
	TestEqual(TEXT("100 cm inside the +X face is darkness 0.4"),
		FPSAISoftBoundaryMath::ComputeTargetDarkness(
			Volume->EvaluateDepth(FVector(-300.f, 0.f, PlayerZ)), Volume->GetEffectiveFullBlackDistance()), 0.4f, 0.01f);
	TestEqual(TEXT("250 cm inside the +X face is full black"),
		FPSAISoftBoundaryMath::ComputeTargetDarkness(
			Volume->EvaluateDepth(FVector(-150.f, 0.f, PlayerZ)), Volume->GetEffectiveFullBlackDistance()), 1.f, 0.01f);

	// 站在盒的 XY 中心：深度 400 >= 250，当帧全黑并被弹出体积外，朝向背对禁区。
	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(FVector(0.f, 0.f, PlayerZ)));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	TestEqual(TEXT("Component discovered the volume"), Component->GetVolumeCount(), 1);
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);

	TestEqual(TEXT("Standing at the XY centre reaches full black"),
		Component->GetTargetDarkness(), 1.f, 0.001f);
	TestEqual(TEXT("Signed depth at the XY centre is the horizontal inset"),
		Component->GetSignedDepth(), 400.f, 1.f);

	const FVector After = Character->GetActorLocation();
	const float AfterOutside = Volume->EvaluateOutsideDistance(After);
	AddInfo(FString::Printf(TEXT("Ground-aligned KeepOut landing = %s, outside distance = %.1f"), *After.ToString(), AfterOutside));
	TestTrue(TEXT("Pawn was teleported out of the ground-aligned volume"), AfterOutside > 100.f);
	TestFalse(TEXT("Pawn no longer sits at the deepest point"), After.Equals(FVector(0.f, 0.f, PlayerZ), 1.f));
	TestEqual(TEXT("Landing has no horizontal penetration left"),
		FPSAISoftBoundaryMath::ComputeKeepOutDepthBox(After, Volume->GetVolumeTransform(), Volume->GetVolumeExtent()), 0.f, 0.01f);

	const FVector AfterFacing = Character->GetActorRotation().Vector().GetSafeNormal2D();
	const FVector Outward = (After - Center).GetSafeNormal2D();
	TestTrue(TEXT("Pawn yaw faces away from the volume center"), FVector::DotProduct(AfterFacing, Outward) > 0.9f);

	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestEqual(TEXT("Back outside means no target darkness"), Component->GetTargetDarkness(), 0.f);
	TestTrue(TEXT("The landing was remembered as a safe location"), Volume->HasSafeLocation());

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryKeepOutZMembershipTest,
	"SaaS.FPSGame_AI.SoftBoundary.KeepOutIgnoresVerticalDepthButRespectsZMembership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryKeepOutZMembershipTest::RunTest(const FString& Parameters)
{
	// 竖直位置只做归属判断：Z 范围外深度必须是 0（否则站在体积顶上也会一路变黑），
	// Z 范围内深度只由水平内缩决定，跟站在高处还是低处无关。
	const FVector Extent(400.f, 400.f, 300.f);
	const FTransform Box(FRotator::ZeroRotator, FVector(0.f, 0.f, 300.f));   // 贴地：盒底面 z=0，顶面 z=600

	TestEqual(TEXT("Above the box has no depth"),
		FPSAISoftBoundaryMath::ComputeKeepOutDepthBox(FVector(0.f, 0.f, 700.f), Box, Extent), 0.f, 0.01f);
	TestEqual(TEXT("Below the box has no depth"),
		FPSAISoftBoundaryMath::ComputeKeepOutDepthBox(FVector(0.f, 0.f, -100.f), Box, Extent), 0.f, 0.01f);
	TestEqual(TEXT("Sitting exactly on the top plane is still inside"),
		FPSAISoftBoundaryMath::ComputeKeepOutDepthBox(FVector(0.f, 0.f, 600.f), Box, Extent), 400.f, 0.01f);

	// 同一个 XY、不同 Z：深度必须完全一样。
	const float LowDepth = FPSAISoftBoundaryMath::ComputeKeepOutDepthBox(FVector(100.f, 0.f, 60.f), Box, Extent);
	const float HighDepth = FPSAISoftBoundaryMath::ComputeKeepOutDepthBox(FVector(100.f, 0.f, 540.f), Box, Extent);
	TestEqual(TEXT("Depth near the +X face is the horizontal inset"), LowDepth, 300.f, 0.01f);
	TestEqual(TEXT("Depth does not change with height inside the Z span"), HighDepth, LowDepth, 0.01f);

	// 圆柱同理：深度是半径方向的内缩，Z 只做归属判断。
	const FVector CylinderExtent(300.f, 900.f, 200.f);
	const FTransform Cylinder(FRotator::ZeroRotator, FVector(0.f, 0.f, 200.f));
	TestEqual(TEXT("Cylinder axis depth is the radius"),
		FPSAISoftBoundaryMath::ComputeKeepOutDepthCylinder(FVector(0.f, 0.f, 100.f), Cylinder, CylinderExtent), 300.f, 0.01f);
	TestEqual(TEXT("Cylinder radial depth is the inset toward the axis"),
		FPSAISoftBoundaryMath::ComputeKeepOutDepthCylinder(FVector(100.f, 0.f, 100.f), Cylinder, CylinderExtent), 200.f, 0.01f);
	TestEqual(TEXT("Above the cylinder cap has no depth"),
		FPSAISoftBoundaryMath::ComputeKeepOutDepthCylinder(FVector(0.f, 0.f, 500.f), Cylinder, CylinderExtent), 0.f, 0.01f);

	// 体积层：Z 范围外深度 0、暗度 0；Z 范围内深度是水平内缩；水平在外是负（安全侧）。
	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, FVector(0.f, 0.f, 300.f), Extent, EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}

	TestEqual(TEXT("Volume depth above the roof is zero"), Volume->EvaluateDepth(FVector(0.f, 0.f, 700.f)), 0.f, 0.01f);
	TestEqual(TEXT("Roof position produces no darkness"),
		FPSAISoftBoundaryMath::ComputeTargetDarkness(
			Volume->EvaluateDepth(FVector(0.f, 0.f, 700.f)), Volume->GetEffectiveFullBlackDistance()), 0.f);
	TestEqual(TEXT("Volume depth inside the Z span is the horizontal inset"),
		Volume->EvaluateDepth(FVector(0.f, 0.f, 100.f)), 400.f, 0.01f);
	TestTrue(TEXT("Horizontally outside is a safe negative depth"), Volume->EvaluateDepth(FVector(-900.f, 0.f, 100.f)) < 0.f);

	// 站在体积正上方跑一帧：不产生暗度，也不传送。
	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(FVector(0.f, 0.f, 700.f)));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestEqual(TEXT("Standing on the roof produces no target darkness"), Component->GetTargetDarkness(), 0.f);
	TestEqual(TEXT("Signed depth on the roof is zero"), Component->GetSignedDepth(), 0.f, 0.01f);
	TestTrue(TEXT("Standing on the roof does not teleport"), Character->GetActorLocation().Equals(FVector(0.f, 0.f, 700.f), 1.f));

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryTooShortVerticallyTest,
	"SaaS.FPSGame_AI.SoftBoundary.TooShortKeepOutIsFlaggedUnreachable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryTooShortVerticallyTest::RunTest(const FString& Parameters)
{
	// 第三种静默失败：体积竖直方向装不下玩家。自动适配救不了它 —— 盒子又宽又深，
	// GetMaxReachableDepth 很大、IsFullBlackUnreachable 为假，但玩家中心永远落在 Z 区间外，
	// 所以永远不触发。只能靠单独一条校验报出来。
	// 贴地语义：盒底面落在 z=0 上，中心 z 就等于半高。半高 40 的盒子 Z 区间是 [0,80]，
	// 玩家中心 100 永远在盒顶上方，这就是"太矮"。
	const FVector Extent(400.f, 400.f, 40.f);
	const FVector Center(0.f, 0.f, Extent.Z);      // 贴地：盒顶面只有 80cm 高

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	// 竖直判定现在要向体积脚下取地面，所以世界里得先有地面（顶面 z=0）。
	// 半边长 100 米，够盖住下面 5000cm 外那颗"够高"的对照体积。
	if (!TestNotNull(TEXT("Floor slab created"),
		FPSAISoftBoundaryTestPrivate::SpawnFloorSlab(TestWorld.World, 0.f, 10000.f)))
	{
		TestWorld.Close();
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}

	float GroundZ = -1.f;
	float StandingCenterZ = -1.f;
	TestTrue(TEXT("The floor below the volume is found"), Volume->TraceGroundBelow(GroundZ));
	TestEqual(TEXT("Ground trace lands on the floor top"), GroundZ, 0.f, 1.f);
	TestEqual(TEXT("Standing centre above ground defaults to the standing body centre"),
		Volume->StandingCenterAboveGround, 100.f, 0.01f);
	TestEqual(TEXT("Ground trace distance defaults to 1000 cm"), Volume->GroundTraceDistance, 1000.f, 0.01f);
	TestEqual(TEXT("Box top sits at 80 cm"), Volume->GetVerticalTop(), 80.f, 0.01f);
	TestTrue(TEXT("Box top below the standing centre is flagged as too short"),
		Volume->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::TooShort);
	TestEqual(TEXT("Standing centre is the ground plus the standing height"), StandingCenterZ, 100.f, 1.f);
	TestEqual(TEXT("Horizontal reachable depth is unaffected by the height"), Volume->GetMaxReachableDepth(), 400.f, 0.01f);
	TestFalse(TEXT("This is not the thin-axis failure"), Volume->IsFullBlackUnreachable());
	TestEqual(TEXT("Auto-fit leaves the configured distance alone"),
		Volume->GetEffectiveFullBlackDistance(), 250.f, 0.01f);

	// 阈值之外不能误报：同尺寸、半高 150 的贴地盒子 Z 区间是 [0,300]，正常可用。
	AFPSAISoftBoundaryVolume* TallEnough = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, FVector(5000.f, 0.f, 150.f), FVector(400.f, 400.f, 150.f),
		EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Tall volume spawned"), TallEnough))
	{
		TestWorld.Close();
		return false;
	}
	TestTrue(TEXT("A volume tall enough for the player is not flagged"),
		TallEnough->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::Ok);

#if WITH_EDITOR
	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Volume->IsDataValid(ValidationContext);
	bool bFoundTooShort = false;
	for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
	{
		if (Issue.Severity == EMessageSeverity::Error && Issue.Message.ToString().Contains(TEXT("TOO SHORT")))
		{
			bFoundTooShort = true;
			AddInfo(FString::Printf(TEXT("Validation error: %s"), *Issue.Message.ToString()));
		}
	}
	AddInfo(FString::Printf(TEXT("Data validation issues = %d, matched too-short error = %d"),
		ValidationContext.GetIssues().Num(), bFoundTooShort ? 1 : 0));
	TestTrue(TEXT("Data validation reports the too-short volume"), bFoundTooShort);
	TestTrue(TEXT("Data validation result is Invalid"), ValidationResult == EDataValidationResult::Invalid);

	// 编辑器警告字也要显示这一条（复用"到不了全黑"那套 TextRender 机制）。
	if (const UTextRenderComponent* WarningText = Volume->GetEditorWarningText())
	{
		// UTextRenderComponent 在 5.8 没有 GetText()，直接读公开的 Text 属性。
		const FString WarningString = WarningText->Text.ToString();
		AddInfo(FString::Printf(TEXT("EditorWarningText = %s"), *WarningString));
		TestTrue(TEXT("Editor warning text shows the too-short message"),
			WarningString.Contains(TEXT("TOO SHORT")));
		// 文案必须给出实测数字与地面来源，不能再出现绝对断言。
		// 地面 Z 的数值本身已在上面的 TestEqual 里核过，这里只核文字里带了"地面来源"。
		TestTrue(TEXT("Editor warning text quotes the measured ground"),
			WarningString.Contains(TEXT("on ground z=")));
		TestFalse(TEXT("Editor warning text makes no absolute never-trigger claim"),
			WarningString.Contains(TEXT("never")));
	}
	else
	{
		AddError(TEXT("Volume has no EditorWarningText component"));
	}
#endif

	TestWorld.Close();

	// 运行时后果：更矮的盒子（半高 40，顶面 z=80）对站在地面的玩家（中心 z=100）永远不触发。
	FPSAISoftBoundaryTestPrivate::FTestWorld RuntimeWorld;
	if (!RuntimeWorld.Open())
	{
		AddError(TEXT("Failed to create the runtime test world"));
		return false;
	}

	if (!TestNotNull(TEXT("Runtime floor slab created"),
		FPSAISoftBoundaryTestPrivate::SpawnFloorSlab(RuntimeWorld.World, 0.f, 10000.f)))
	{
		RuntimeWorld.Close();
		return false;
	}

	AFPSAISoftBoundaryVolume* Flat = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		RuntimeWorld.World, FVector(0.f, 0.f, 40.f), FVector(400.f, 400.f, 40.f),
		EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Flat volume spawned"), Flat))
	{
		RuntimeWorld.Close();
		return false;
	}

	TestTrue(TEXT("The flat volume is flagged too"),
		Flat->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::TooShort);
	TestEqual(TEXT("A ground-standing player centre is outside the flat volume vertically"),
		Flat->EvaluateDepth(FVector(0.f, 0.f, 100.f)), 0.f, 0.01f);

	ACharacter* Character = RuntimeWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(FVector(0.f, 0.f, 100.f)));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		RuntimeWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		RuntimeWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	for (int32 Step = 0; Step < 40; ++Step)
	{
		FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	}
	AddInfo(FString::Printf(TEXT("Flat volume after 40 ticks: target darkness = %.4f, location = %s"),
		Component->GetTargetDarkness(), *Character->GetActorLocation().ToString()));
	TestEqual(TEXT("A too-short volume never produces darkness"), Component->GetTargetDarkness(), 0.f);
	TestTrue(TEXT("A too-short volume never teleports"),
		Character->GetActorLocation().Equals(FVector(0.f, 0.f, 100.f), 1.f));

	RuntimeWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryFloorResting90Test,
	"SaaS.FPSGame_AI.SoftBoundary.FloorRestingKeepOutAt90IsNotFlagged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryFloorResting90Test::RunTest(const FString& Parameters)
{
	// D1 回归护栏：贴地摆放、半高 90 的盒子实际工作正常。盒体 Z 区间是 [0,180]，站在地面的
	// 玩家中心 100 就在区间里，实测触发深度 400cm。旧判据 Extent.Z < 100 会把它误报成
	// "TOO SHORT VERTICALLY ... will never trigger"，编辑器警告字、BeginPlay Warning、
	// 数据校验 Error 三处一起假警告。
	const FVector Extent(400.f, 400.f, 90.f);
	const FVector Center(0.f, 0.f, Extent.Z);      // 贴地：盒底面 z=0，顶面 z=180
	const float PlayerCenterZ = 100.f;
	const FVector PlayerLocation(0.f, 0.f, PlayerCenterZ);

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	// 贴地摆放的前提是脚下真有地面：竖直判定要向体积脚下取地面，没地面只会得到 GroundUnknown。
	if (!TestNotNull(TEXT("Floor slab created"),
		FPSAISoftBoundaryTestPrivate::SpawnFloorSlab(TestWorld.World, 0.f, 10000.f)))
	{
		TestWorld.Close();
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}
	Volume->ReturnDistance = 400.f;

	float GroundZ = -1.f;
	float StandingCenterZ = -1.f;
	TestTrue(TEXT("The floor below the volume is found"), Volume->TraceGroundBelow(GroundZ));
	TestEqual(TEXT("Ground trace lands on the floor top"), GroundZ, 0.f, 1.f);
	TestEqual(TEXT("Box bottom sits on the floor"), Volume->GetVerticalBottom(), 0.f, 0.01f);
	TestEqual(TEXT("Box top sits at 180 cm"), Volume->GetVerticalTop(), 180.f, 0.01f);
	TestTrue(TEXT("A floor-resting half height of 90 is vertically OK"),
		Volume->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::Ok);
	TestEqual(TEXT("Standing centre is ground plus the standing height"), StandingCenterZ, 100.f, 1.f);
	TestTrue(TEXT("The player centre is inside the volume Z span"),
		Volume->EvaluateVerticalPlacement(PlayerCenterZ) == EFPSAIVerticalPlacement::Ok);
	TestFalse(TEXT("A half height of 90 is not treated as too short"),
		Volume->GetVerticalTop() < PlayerCenterZ);
	TestFalse(TEXT("This is not the thin-axis failure"), Volume->IsFullBlackUnreachable());
	TestEqual(TEXT("Depth at the XY centre is the horizontal inset"),
		Volume->EvaluateDepth(PlayerLocation), 400.f, 0.01f);

#if WITH_EDITOR
	// 三处提示都不能出声。
	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Volume->IsDataValid(ValidationContext);
	AddInfo(FString::Printf(TEXT("Data validation issues = %d"), ValidationContext.GetIssues().Num()));
	TestEqual(TEXT("A working volume produces no validation issues"), ValidationContext.GetIssues().Num(), 0);
	TestTrue(TEXT("Data validation is not Invalid"), ValidationResult != EDataValidationResult::Invalid);

	if (const UTextRenderComponent* WarningText = Volume->GetEditorWarningText())
	{
		const FString WarningString = WarningText->Text.ToString();
		AddInfo(FString::Printf(TEXT("EditorWarningText = '%s'"), *WarningString));
		TestTrue(TEXT("Editor warning text is empty"), WarningString.IsEmpty());
	}
	else
	{
		AddError(TEXT("Volume has no EditorWarningText component"));
	}
#endif

	// 而且真的能触发：XY 中心深度 400 >= 全黑距离 250，当帧全黑并被弹出体积外。
	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(PlayerLocation));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	TestEqual(TEXT("Component discovered the volume"), Component->GetVolumeCount(), 1);
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);

	TestEqual(TEXT("Standing at the XY centre reaches full black"),
		Component->GetTargetDarkness(), 1.f, 0.001f);
	TestEqual(TEXT("Signed depth at the XY centre is the horizontal inset"),
		Component->GetSignedDepth(), 400.f, 1.f);

	const FVector After = Character->GetActorLocation();
	const float AfterOutside = Volume->EvaluateOutsideDistance(After);
	AddInfo(FString::Printf(TEXT("Floor-resting half height 90 landing = %s, outside distance = %.1f"),
		*After.ToString(), AfterOutside));
	TestTrue(TEXT("Pawn was teleported out of the floor-resting volume"), AfterOutside > 100.f);

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryAboveHeadTest,
	"SaaS.FPSGame_AI.SoftBoundary.KeepOutAboveHeadIsFlagged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryAboveHeadTest::RunTest(const FString& Parameters)
{
	// O1 漏报：体积整个悬在玩家头顶（盒体 Z 区间 [250,350]）时，玩家从下面走过去永远触发不到，
	// 旧实现三处提示全部沉默。这是和"太矮"相反的竖直失败，修法也相反：往下挪或加高。
	const FVector Extent(400.f, 400.f, 50.f);
	const FVector Center(0.f, 0.f, 300.f);
	const float PlayerCenterZ = 100.f;
	const FVector PlayerLocation(0.f, 0.f, PlayerCenterZ);

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	// 世界里的地面顶面在 z=0：判定要把"站在这块地面上的玩家"和盒子比。
	if (!TestNotNull(TEXT("Floor slab created"),
		FPSAISoftBoundaryTestPrivate::SpawnFloorSlab(TestWorld.World, 0.f, 10000.f)))
	{
		TestWorld.Close();
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}
	Volume->ReturnDistance = 400.f;

	float GroundZ = -1.f;
	float StandingCenterZ = -1.f;
	TestTrue(TEXT("The floor below the volume is found"), Volume->TraceGroundBelow(GroundZ));
	TestEqual(TEXT("Ground trace lands on the floor top"), GroundZ, 0.f, 1.f);
	TestEqual(TEXT("Box bottom sits at 250 cm"), Volume->GetVerticalBottom(), 250.f, 0.01f);
	TestEqual(TEXT("Box top sits at 350 cm"), Volume->GetVerticalTop(), 350.f, 0.01f);
	TestTrue(TEXT("A volume out of reach from the ground floor is flagged"),
		Volume->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::AboveHead);
	TestEqual(TEXT("Standing centre is ground plus the standing height"), StandingCenterZ, 100.f, 1.f);
	TestTrue(TEXT("The player centre is below the box"),
		Volume->EvaluateVerticalPlacement(PlayerCenterZ) == EFPSAIVerticalPlacement::AboveHead);
	TestEqual(TEXT("A walking player produces no depth"), Volume->EvaluateDepth(PlayerLocation), 0.f, 0.01f);
	TestFalse(TEXT("This is not the thin-axis failure"), Volume->IsFullBlackUnreachable());

#if WITH_EDITOR
	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Volume->IsDataValid(ValidationContext);
	bool bFoundAboveHead = false;
	for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
	{
		if (Issue.Severity == EMessageSeverity::Error && Issue.Message.ToString().Contains(TEXT("OUT OF REACH FROM GROUND")))
		{
			bFoundAboveHead = true;
			AddInfo(FString::Printf(TEXT("Validation error: %s"), *Issue.Message.ToString()));
		}
	}
	TestTrue(TEXT("Data validation reports the out-of-reach volume"), bFoundAboveHead);
	TestTrue(TEXT("Data validation result is Invalid"), ValidationResult == EDataValidationResult::Invalid);

	if (const UTextRenderComponent* WarningText = Volume->GetEditorWarningText())
	{
		const FString WarningString = WarningText->Text.ToString();
		AddInfo(FString::Printf(TEXT("EditorWarningText = '%s'"), *WarningString));
		TestTrue(TEXT("Editor warning text says the box is out of reach from the ground"),
			WarningString.Contains(TEXT("OUT OF REACH FROM GROUND")));
		TestFalse(TEXT("Editor warning text is not the too-short wording"),
			WarningString.Contains(TEXT("TOO SHORT")));
		// D2 的核心：不能再说"永不触发"，只能说"站在下方地面够不到"，并给出实测数字。
		TestFalse(TEXT("Editor warning text makes no absolute never-trigger claim"),
			WarningString.Contains(TEXT("never")));
		TestTrue(TEXT("Editor warning text quotes the measured ground and standing centre"),
			WarningString.Contains(TEXT("on ground z=")) && WarningString.Contains(TEXT("standing 100")));
	}
	else
	{
		AddError(TEXT("Volume has no EditorWarningText component"));
	}
#endif

	// 站着不动也不触发：跑 20 帧，目标暗度恒为 0，位置一动没动。
	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(PlayerLocation));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	for (int32 Step = 0; Step < 20; ++Step)
	{
		FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	}
	AddInfo(FString::Printf(TEXT("Above-head volume after 20 ticks: target darkness = %.4f, location = %s"),
		Component->GetTargetDarkness(), *Character->GetActorLocation().ToString()));
	TestEqual(TEXT("A volume above the head never produces darkness"), Component->GetTargetDarkness(), 0.f);
	TestTrue(TEXT("A volume above the head never teleports"),
		Character->GetActorLocation().Equals(PlayerLocation, 1.f));

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryRaisedOnFloorTest,
	"SaaS.FPSGame_AI.SoftBoundary.RaisedKeepOutOnFloorIsNotFlagged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryRaisedOnFloorTest::RunTest(const FString& Parameters)
{
	// D2 回归：抬到二层楼板上的 KeepOut 实际能正常工作（acceptance round-6 实测：站在 z=390 上
	// 会被弹回去），旧判据却拿全局常数 100 跟它比，在视口文字和 BeginPlay 日志两处断言
	// "BOX ABOVE HEAD ... will never trigger"。
	// 现在向体积脚下取地面：脚下楼板 z=300 -> 站立中心 400 -> 落在盒体 [300,480] 里 -> Ok，零警告。
	const FVector Extent(400.f, 400.f, 90.f);
	const FVector Center(0.f, 0.f, 390.f);
	const float SlabTopZ = 300.f;
	const float PlayerCenterOnSlab = SlabTopZ + 100.f;

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	// 二层楼板：顶面 z=300，覆盖 ±40 米，够盖住整颗体积和传送落点。
	if (!TestNotNull(TEXT("Raised floor slab created"),
		FPSAISoftBoundaryTestPrivate::SpawnFloorSlab(TestWorld.World, SlabTopZ, 4000.f)))
	{
		TestWorld.Close();
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Raised volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}
	Volume->ReturnDistance = 400.f;

	float GroundZ = -1.f;
	float StandingCenterZ = -1.f;
	TestTrue(TEXT("The ground trace finds the raised floor slab"), Volume->TraceGroundBelow(GroundZ));
	TestEqual(TEXT("Ground Z is the slab the box stands on"), GroundZ, SlabTopZ, 1.f);
	TestTrue(TEXT("A raised volume standing on a floor is not flagged"),
		Volume->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::Ok);
	TestEqual(TEXT("Standing centre is the slab top plus the standing height"),
		StandingCenterZ, SlabTopZ + 100.f, 1.f);
	TestTrue(TEXT("A player standing on that same slab is inside the volume Z span"),
		Volume->EvaluateVerticalPlacement(PlayerCenterOnSlab) == EFPSAIVerticalPlacement::Ok);
	// 这颗体积不是空摆设：同一块楼板上的玩家中心真的落进盒体，横向深度也是正的。
	TestEqual(TEXT("A player on the slab gets a positive KeepOut depth"),
		Volume->EvaluateDepth(FVector(0.f, 0.f, PlayerCenterOnSlab)), 400.f, 1.f);

#if WITH_EDITOR
	// 三处出口都不能出声：数据校验零 issue、警告字为空。
	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Volume->IsDataValid(ValidationContext);
	for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
	{
		AddInfo(FString::Printf(TEXT("Unexpected validation issue: %s"), *Issue.Message.ToString()));
	}
	TestEqual(TEXT("A working raised volume produces no validation issues"),
		ValidationContext.GetIssues().Num(), 0);
	TestTrue(TEXT("Data validation is not Invalid"), ValidationResult != EDataValidationResult::Invalid);

	if (const UTextRenderComponent* WarningText = Volume->GetEditorWarningText())
	{
		const FString WarningString = WarningText->Text.ToString();
		AddInfo(FString::Printf(TEXT("EditorWarningText = '%s'"), *WarningString));
		TestTrue(TEXT("Editor warning text is empty for a working raised volume"), WarningString.IsEmpty());
	}
	else
	{
		AddError(TEXT("Volume has no EditorWarningText component"));
	}
#endif

	// 运行时：把角色放在同一块楼板上跑一帧，深度 400 >= 全黑距离 250 -> 当帧全黑并被弹出。
	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(FVector(0.f, 0.f, PlayerCenterOnSlab)));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	TestEqual(TEXT("Component discovered the raised volume"), Component->GetVolumeCount(), 1);
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);

	TestEqual(TEXT("Standing on the raised slab reaches full black"),
		Component->GetTargetDarkness(), 1.f, 0.001f);

	const FVector After = Character->GetActorLocation();
	const float AfterOutside = Volume->EvaluateOutsideDistance(After);
	AddInfo(FString::Printf(TEXT("Raised KeepOut landing = %s, outside distance = %.1f"), *After.ToString(), AfterOutside));
	TestTrue(TEXT("Pawn was teleported out of the raised volume"), AfterOutside > 100.f);
	TestFalse(TEXT("Pawn no longer sits where it stood"), After.Equals(FVector(0.f, 0.f, PlayerCenterOnSlab), 1.f));

	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestEqual(TEXT("Back outside means no target darkness"), Component->GetTargetDarkness(), 0.f);

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryFloatingOverOpenFloorTest,
	"SaaS.FPSGame_AI.SoftBoundary.FloatingKeepOutOverOpenFloorIsReachableFromBelow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryFloatingOverOpenFloorTest::RunTest(const FString& Parameters)
{
	// 同一颗抬高的盒子，脚下却只有 z=0 的地面：站在那块地面上的玩家（中心 100）确实够不到它，
	// 所以值得报警。但文案必须说清"站在下方的玩家摸不到"，并给出实测地面与站立中心，
	// 不能再出现 "will never trigger" 这种绝对断言（acceptance round-6 D2 的核心）。
	const FVector Extent(400.f, 400.f, 90.f);
	const FVector Center(0.f, 0.f, 390.f);
	const FVector GroundPlayer(0.f, 0.f, 100.f);

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	if (!TestNotNull(TEXT("Ground slab created"),
		FPSAISoftBoundaryTestPrivate::SpawnFloorSlab(TestWorld.World, 0.f, 4000.f)))
	{
		TestWorld.Close();
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Floating volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}

	float GroundZ = -1.f;
	float StandingCenterZ = -1.f;
	TestTrue(TEXT("The ground trace finds the floor far below"), Volume->TraceGroundBelow(GroundZ));
	TestEqual(TEXT("Ground Z is the open floor at the bottom"), GroundZ, 0.f, 1.f);
	TestTrue(TEXT("A volume above the reach of a ground walker is flagged"),
		Volume->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::AboveHead);
	TestEqual(TEXT("Standing centre is the ground plus the standing height"), StandingCenterZ, 100.f, 1.f);
	TestEqual(TEXT("A player walking on the ground produces no KeepOut depth"),
		Volume->EvaluateDepth(GroundPlayer), 0.f, 0.01f);

#if WITH_EDITOR
	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Volume->IsDataValid(ValidationContext);
	bool bFoundOutOfReach = false;
	for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
	{
		const FString Message = Issue.Message.ToString();
		AddInfo(FString::Printf(TEXT("Validation issue: %s"), *Message));
		if (Issue.Severity == EMessageSeverity::Error && Message.Contains(TEXT("OUT OF REACH FROM GROUND")))
		{
			bFoundOutOfReach = true;
			TestFalse(TEXT("Validation text makes no absolute never-trigger claim"),
				Message.Contains(TEXT("never")));
		}
	}
	TestTrue(TEXT("Data validation still reports the unreachable volume"), bFoundOutOfReach);
	TestTrue(TEXT("Data validation result is Invalid"), ValidationResult == EDataValidationResult::Invalid);

	if (const UTextRenderComponent* WarningText = Volume->GetEditorWarningText())
	{
		const FString WarningString = WarningText->Text.ToString();
		AddInfo(FString::Printf(TEXT("EditorWarningText = '%s'"), *WarningString));
		TestTrue(TEXT("Editor warning text says the box is out of reach from the ground"),
			WarningString.Contains(TEXT("OUT OF REACH FROM GROUND")));
		TestFalse(TEXT("Editor warning text makes no absolute never-trigger claim"),
			WarningString.Contains(TEXT("never")));
		TestTrue(TEXT("Editor warning text quotes the measured ground and standing centre"),
			WarningString.Contains(TEXT("on ground z=")) && WarningString.Contains(TEXT("standing 100")));
	}
	else
	{
		AddError(TEXT("Volume has no EditorWarningText component"));
	}
#endif

	// 运行时：站在下方地面的玩家跑 20 帧，既不产生暗度也不会被传送 —— 这就是被报警的那个事实。
	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(GroundPlayer));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	Component->RefreshVolumes();
	for (int32 Step = 0; Step < 20; ++Step)
	{
		FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	}
	AddInfo(FString::Printf(TEXT("Floating volume after 20 ticks: target darkness = %.4f, location = %s"),
		Component->GetTargetDarkness(), *Character->GetActorLocation().ToString()));
	TestEqual(TEXT("A ground walker under the floating volume gets no darkness"),
		Component->GetTargetDarkness(), 0.f);
	TestTrue(TEXT("A ground walker under the floating volume is not teleported"),
		Character->GetActorLocation().Equals(GroundPlayer, 1.f));

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundarySunkKeepOutTest,
	"SaaS.FPSGame_AI.SoftBoundary.SunkKeepOutFindsGroundAbove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundarySunkKeepOutTest::RunTest(const FString& Parameters)
{
	// acceptance round-7 B：体积"沉进楼板"的摆法（底面在地板面以下）向下打射线必然打空，
	// 旧实现于是永远报 GroundUnknown。现在向下打空后会从同一个起点向上补打同样的距离。
	// 几何：楼板从 z=0（底面）到 z=20（顶面），体积 center z=150、Extent.Z=300 -> bottom=-150，
	// 站在楼板上（或按"地面 Z + 站立高"算）的玩家中心都落在体积 Z 区间 [-150,450] 里。
	const FVector Extent(400.f, 400.f, 300.f);
	const FVector Center(0.f, 0.f, 150.f);
	const float PlateBottomZ = 0.f;
	const float PlateTopZ = 20.f;

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	// 楼板必须先于体积存在，否则体积的构造脚本那一次刷新就已经把"没地面"缓存下来了。
	if (!TestNotNull(TEXT("Floor plate created"),
		FPSAISoftBoundaryTestPrivate::SpawnFloorPlate(TestWorld.World, PlateTopZ, PlateTopZ - PlateBottomZ, 4000.f)))
	{
		TestWorld.Close();
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}

	TestEqual(TEXT("Box bottom sits 150 cm below the floor surface"), Volume->GetVerticalBottom(), -150.f, 0.01f);
	TestEqual(TEXT("Box top sits at 450 cm"), Volume->GetVerticalTop(), 450.f, 0.01f);

	float GroundZ = -1.f;
	TestTrue(TEXT("Ground is found even though the bottom is below the floor"), Volume->TraceGroundBelow(GroundZ));
	TestEqual(TEXT("The hit surface is the one right above the box bottom"), GroundZ, PlateBottomZ, 1.f);
	TestTrue(TEXT("The read-only flag says the ground came from the upward fallback"),
		Volume->bGroundFoundByUpwardTrace);

	float StandingCenterZ = -1.f;
	TestTrue(TEXT("A sunken but reachable volume is vertically OK"),
		Volume->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::Ok);
	TestEqual(TEXT("Standing centre is the ground plus the standing height"), StandingCenterZ, 100.f, 1.f);
	TestTrue(TEXT("A player centre of 100 is inside the volume Z span"),
		Volume->EvaluateVerticalPlacement(100.f) == EFPSAIVerticalPlacement::Ok);
	// 这颗体积不是空摆设：站在体积里的玩家真的吃得到水平深度。
	TestEqual(TEXT("A player inside gets the horizontal inset as depth"),
		Volume->EvaluateDepth(FVector(0.f, 0.f, 100.f)), 400.f, 0.01f);

#if WITH_EDITOR
	const UTextRenderComponent* WarningText = Volume->GetEditorWarningText();
	if (!TestNotNull(TEXT("Volume has an editor warning text component"), WarningText))
	{
		TestWorld.Close();
		return false;
	}

	// 视口里一个字都不能有：这颗体积是可用的，不能被"沉进楼板"的摆法挂在视口里报警。
	const FString WarningString = WarningText->Text.ToString();
	AddInfo(FString::Printf(TEXT("EditorWarningText = '%s'"), *WarningString));
	TestTrue(TEXT("A sunken but reachable volume shows no viewport warning"), WarningString.IsEmpty());

	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Volume->IsDataValid(ValidationContext);
	for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
	{
		AddInfo(FString::Printf(TEXT("Unexpected validation issue: %s"), *Issue.Message.ToString()));
	}
	TestEqual(TEXT("A sunken but reachable volume produces no validation issues"),
		ValidationContext.GetIssues().Num(), 0);
	TestTrue(TEXT("Data validation is not Invalid"), ValidationResult != EDataValidationResult::Invalid);
#endif

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryUnresolvableGroundTest,
	"SaaS.FPSGame_AI.SoftBoundary.UnresolvableGroundIsSilentInViewport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryUnresolvableGroundTest::RunTest(const FString& Parameters)
{
	// acceptance round-7 A/3：上下都打不到地面时是"无法判定"，不是"摆错了"。
	// 视口必须静默（加载时序里地板还没流送进来时也是这个状态，一条 "?" 会让人以为功能坏了），
	// Map Check 不能刷红，但 Display 级日志必须还在 —— 下面这条期望消息就是证明日志没被删掉。
	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	// 这个世界里没有任何碰撞体：向下、向上都打不到东西。
	AddExpectedMessagePlain(TEXT("NO GROUND BELOW"), ELogVerbosity::Display,
		EAutomationExpectedMessageFlags::Contains);

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, FVector(0.f, 0.f, 400.f), FVector(400.f, 400.f, 300.f),
		EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}

	float GroundZ = -1.f;
	TestFalse(TEXT("Neither the downward nor the upward trace finds any ground"),
		Volume->TraceGroundBelow(GroundZ));
	TestEqual(TEXT("Ground Z stays untouched when nothing is hit"), GroundZ, 0.f, 0.01f);
	TestFalse(TEXT("The read-only flag stays false when nothing was hit"),
		Volume->bGroundFoundByUpwardTrace);

	float StandingCenterZ = -1.f;
	TestTrue(TEXT("The placement is GroundUnknown, not TooShort or AboveHead"),
		Volume->EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ) == EFPSAIVerticalPlacement::GroundUnknown);

#if WITH_EDITOR
	const UTextRenderComponent* WarningText = Volume->GetEditorWarningText();
	if (!TestNotNull(TEXT("Volume has an editor warning text component"), WarningText))
	{
		TestWorld.Close();
		return false;
	}

	const FString WarningString = WarningText->Text.ToString();
	AddInfo(FString::Printf(TEXT("EditorWarningText = '%s'"), *WarningString));
	TestTrue(TEXT("The viewport says nothing when the ground cannot be determined"), WarningString.IsEmpty());

	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Volume->IsDataValid(ValidationContext);
	for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
	{
		AddInfo(FString::Printf(TEXT("Unexpected validation issue: %s"), *Issue.Message.ToString()));
	}
	TestEqual(TEXT("Ground unknown produces no Map Check issue"), ValidationContext.GetIssues().Num(), 0);
	TestTrue(TEXT("Data validation is not Invalid"), ValidationResult != EDataValidationResult::Invalid);
#endif

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryCrawlPresetTableTest,
	"SaaS.FPSGame_AI.SoftBoundary.CrawlPresetMapsToExpectedParameters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryCrawlPresetTableTest::RunTest(const FString& Parameters)
{
	// 预设表是"风格"的唯一来源：组件写 MID 和这里断言的都是 AFPSAISoftBoundaryVolume::GetCrawlPresetParams。
	// 五档的期望值照着设计表抄一遍，改表就必须改这里，不允许两边各说一套。
	struct FExpected
	{
		EFPSAISoftBoundaryCrawlPreset Preset;
		const TCHAR* Label;
		float NoiseSpeed;
		float NoiseStrength;
		float EdgeFeather;
	};

	const FExpected Expected[] =
	{
		{ EFPSAISoftBoundaryCrawlPreset::Off,      TEXT("Off"),      0.0f, 0.00f, 0.30f },
		{ EFPSAISoftBoundaryCrawlPreset::Calm,     TEXT("Calm"),     0.45f, 0.65f, 0.35f },
		{ EFPSAISoftBoundaryCrawlPreset::Balanced, TEXT("Balanced"), 1.0f, 1.00f, 0.30f },
		{ EFPSAISoftBoundaryCrawlPreset::Crawl,    TEXT("Crawl"),    1.6f, 1.35f, 0.26f },
		{ EFPSAISoftBoundaryCrawlPreset::Swarm,    TEXT("Swarm"),    2.8f, 1.85f, 0.20f },
	};

	for (const FExpected& Row : Expected)
	{
		const FFPSAISoftBoundaryCrawlParams Params =
			AFPSAISoftBoundaryVolume::GetCrawlPresetParams(Row.Preset);
		AddInfo(FString::Printf(TEXT("%s -> NoiseSpeed=%.3f NoiseStrength=%.3f EdgeFeather=%.3f"),
			Row.Label, Params.NoiseSpeed, Params.NoiseStrength, Params.EdgeFeather));

		TestEqual(FString::Printf(TEXT("%s NoiseSpeed"), Row.Label), Params.NoiseSpeed, Row.NoiseSpeed, 0.001f);
		TestEqual(FString::Printf(TEXT("%s NoiseStrength"), Row.Label), Params.NoiseStrength, Row.NoiseStrength, 0.001f);
		TestEqual(FString::Printf(TEXT("%s EdgeFeather"), Row.Label), Params.EdgeFeather, Row.EdgeFeather, 0.001f);
	}

	// Off 必须是"完全不蠕动"：速度与幅度都为 0，画面才等于静态径向渐变。
	const FFPSAISoftBoundaryCrawlParams OffParams =
		AFPSAISoftBoundaryVolume::GetCrawlPresetParams(EFPSAISoftBoundaryCrawlPreset::Off);
	TestEqual(TEXT("Off freezes the noise speed"), OffParams.NoiseSpeed, 0.f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Off freezes the noise strength"), OffParams.NoiseStrength, 0.f, KINDA_SMALL_NUMBER);

	// 档位是从"静止"到"最剧烈"单调加码的，别写成有起伏的乱序表。
	const FFPSAISoftBoundaryCrawlParams CalmParams =
		AFPSAISoftBoundaryVolume::GetCrawlPresetParams(EFPSAISoftBoundaryCrawlPreset::Calm);
	const FFPSAISoftBoundaryCrawlParams SwarmParams =
		AFPSAISoftBoundaryVolume::GetCrawlPresetParams(EFPSAISoftBoundaryCrawlPreset::Swarm);
	TestTrue(TEXT("Calm is weaker than Balanced"), CalmParams.NoiseStrength < Expected[2].NoiseStrength);
	TestTrue(TEXT("Crawl is stronger than Balanced"), Expected[3].NoiseStrength > Expected[2].NoiseStrength);
	TestTrue(TEXT("Swarm is the strongest"), SwarmParams.NoiseStrength > Expected[3].NoiseStrength);
	TestTrue(TEXT("Swarm is the fastest"), SwarmParams.NoiseSpeed > Expected[3].NoiseSpeed);

	// 默认档必须是 Crawl：本版把"虫爬更多一点"定为默认观感。
	const AFPSAISoftBoundaryVolume* Defaults = GetDefault<AFPSAISoftBoundaryVolume>();
	if (TestNotNull(TEXT("Volume CDO is available"), Defaults))
	{
		TestTrue(TEXT("The default crawl preset is Crawl"),
			Defaults->CrawlPreset == EFPSAISoftBoundaryCrawlPreset::Crawl);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryCrawlPresetDominantTest,
	"SaaS.FPSGame_AI.SoftBoundary.CrawlPresetFollowsDominantVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryCrawlPresetDominantTest::RunTest(const FString& Parameters)
{
	// 多颗体积同时在场时，风格由"主导体积"（当前最深的那颗）决定，预设也必须在它们之间跟着换。
	// 深度都刻意留在全黑距离以内（0.4 / 0.6），免得当帧触发传送把玩家挪走。
	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	const FVector NearPawn(600.f, 0.f, 100.f);    // 在 VolumeA 里，深度 400
	const FVector FarPawn(3400.f, 0.f, 100.f);    // 在 VolumeB 里，深度 600

	AFPSAISoftBoundaryVolume* VolumeA = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, FVector(0.f, 0.f, 200.f), FVector(1000.f, 1000.f, 200.f),
		EFPSAISoftBoundaryMode::KeepOut, 3000.f);
	AFPSAISoftBoundaryVolume* VolumeB = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, FVector(3000.f, 0.f, 200.f), FVector(1000.f, 1000.f, 200.f),
		EFPSAISoftBoundaryMode::KeepOut, 3000.f);
	if (!TestNotNull(TEXT("VolumeA spawned"), VolumeA) || !TestNotNull(TEXT("VolumeB spawned"), VolumeB))
	{
		TestWorld.Close();
		return false;
	}

	VolumeA->CrawlPreset = EFPSAISoftBoundaryCrawlPreset::Off;
	VolumeB->CrawlPreset = EFPSAISoftBoundaryCrawlPreset::Swarm;

	ACharacter* Character = TestWorld.World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FTransform(NearPawn));
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		TestWorld.Close();
		return false;
	}

	UFPSAISoftBoundaryComponent* Component = FPSAISoftBoundaryTestPrivate::AttachComponent(Character);
	if (!TestNotNull(TEXT("Component attached"), Component))
	{
		TestWorld.Close();
		return false;
	}

	TestFalse(TEXT("No preset is applied before the first tick"), Component->HasAppliedCrawlPreset());

	Component->RefreshVolumes();
	TestEqual(TEXT("Component discovered both volumes"), Component->GetVolumeCount(), 2);

	// 站在 VolumeA 里：它更深（400 > VolumeB 的负深度），预设应该是 Off。
	AddInfo(FString::Printf(TEXT("At NearPawn: depthA=%.1f depthB=%.1f"),
		VolumeA->EvaluateDepth(NearPawn), VolumeB->EvaluateDepth(NearPawn)));
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestTrue(TEXT("The nearest volume is the dominant one"),
		Component->GetAppliedCrawlPreset() == EFPSAISoftBoundaryCrawlPreset::Off);

	// 换到 VolumeB 里：主导体积换成 B，预设跟着换成 Swarm。
	Character->SetActorLocation(FarPawn, false, nullptr, ETeleportType::TeleportPhysics);
	AddInfo(FString::Printf(TEXT("At FarPawn: depthA=%.1f depthB=%.1f"),
		VolumeA->EvaluateDepth(FarPawn), VolumeB->EvaluateDepth(FarPawn)));
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestTrue(TEXT("The preset follows the new dominant volume"),
		Component->GetAppliedCrawlPreset() == EFPSAISoftBoundaryCrawlPreset::Swarm);

	// 主导体积不变、只改它自己的预设：下一帧必须重新写一遍（这就是"预设有变化才写"的另一半）。
	VolumeB->CrawlPreset = EFPSAISoftBoundaryCrawlPreset::Calm;
	FPSAISoftBoundaryTestPrivate::TickComponentOnce(Component, 0.05f);
	TestTrue(TEXT("Editing the dominant volume's preset is picked up on the next tick"),
		Component->GetAppliedCrawlPreset() == EFPSAISoftBoundaryCrawlPreset::Calm);

	TestTrue(TEXT("No teleport happened during the preset checks"),
		Character->GetActorLocation().Equals(FarPawn, 1.f));

	TestWorld.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPSAISoftBoundaryBoundsVisibilityTest,
	"SaaS.FPSGame_AI.SoftBoundary.BoundsVisibilityTogglesDoNotAffectCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFPSAISoftBoundaryBoundsVisibilityTest::RunTest(const FString& Parameters)
{
	// 两个显示开关只允许影响可视性：关掉之后体积仍然 NoCollision、仍然能选中、深度判定一个数都不能变。
	const FVector Center(0.f, 0.f, 300.f);
	const FVector Extent(400.f, 400.f, 300.f);

	FPSAISoftBoundaryTestPrivate::FTestWorld TestWorld;
	if (!TestWorld.Open())
	{
		AddError(TEXT("Failed to create test world"));
		return false;
	}

	AFPSAISoftBoundaryVolume* Volume = FPSAISoftBoundaryTestPrivate::SpawnVolume(
		TestWorld.World, Center, Extent, EFPSAISoftBoundaryMode::KeepOut, 250.f);
	if (!TestNotNull(TEXT("Volume spawned"), Volume))
	{
		TestWorld.Close();
		return false;
	}

	UBoxComponent* Bounds = Volume->GetVolumeBounds();
	UBoxComponent* Band = Volume->GetTriggerBand();
	if (!TestNotNull(TEXT("Volume has a bounds box"), Bounds) || !TestNotNull(TEXT("Volume has a trigger band"), Band))
	{
		TestWorld.Close();
		return false;
	}

	// 采集一组基准深度：开关前后必须逐点相等。
	constexpr int32 ProbeCount = 4;
	const FVector Probes[ProbeCount] =
	{
		FVector(0.f, 0.f, 100.f),      // 盒内中心
		FVector(-300.f, 0.f, 100.f),   // 盒内贴 +X 面
		FVector(900.f, 0.f, 100.f),    // 盒外
		FVector(0.f, 0.f, 900.f),      // 盒顶上方
	};

	// 先两个开关都打开，作为"开着也能用"的对照。
	Volume->bShowBoundsInEditor = true;
	Volume->bShowBoundsInGame = true;
	Volume->OnConstruction(Volume->GetActorTransform());
	TestTrue(TEXT("Bounds are visible when both toggles are on"), Bounds->IsVisible());
	TestTrue(TEXT("Trigger band is visible when both toggles are on"), Band->IsVisible());

	float Baseline[ProbeCount];
	for (int32 Index = 0; Index < ProbeCount; ++Index)
	{
		Baseline[Index] = Volume->EvaluateDepth(Probes[Index]);
	}

	// 两个开关都关掉：PostEditChangeProperty 走的就是 OnConstruction -> UpdateEditorVisuals 这一条。
	Volume->bShowBoundsInEditor = false;
	Volume->bShowBoundsInGame = false;
	Volume->OnConstruction(Volume->GetActorTransform());

	AddInfo(FString::Printf(TEXT("After hiding: bounds visible=%d band visible=%d bounds collision=%d band collision=%d"),
		Bounds->IsVisible() ? 1 : 0, Band->IsVisible() ? 1 : 0,
		static_cast<int32>(Bounds->GetCollisionEnabled()), static_cast<int32>(Band->GetCollisionEnabled())));

	TestFalse(TEXT("Bounds are not visible with both toggles off"), Bounds->IsVisible());
	TestFalse(TEXT("Trigger band is not visible with both toggles off"), Band->IsVisible());

	// 可视性归零，但要能选中：bSelectable 必须还是 true，Actor 也不能被隐藏。
	TestTrue(TEXT("Bounds stay selectable in the outliner"), Bounds->bSelectable);
	TestTrue(TEXT("Trigger band stays selectable in the outliner"), Band->bSelectable);
	TestFalse(TEXT("The actor itself is not hidden"), Volume->IsHidden());

	TestEqual(TEXT("Bounds collision stays off"), Bounds->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestEqual(TEXT("Trigger band collision stays off"), Band->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Bounds never generate overlap events"), Bounds->GetGenerateOverlapEvents());

	for (int32 Index = 0; Index < ProbeCount; ++Index)
	{
		TestEqual(
			FString::Printf(TEXT("EvaluateDepth is unchanged at probe %d"), Index),
			Volume->EvaluateDepth(Probes[Index]),
			Baseline[Index],
			0.001f);
	}

	// 再打开 PIE 的调试线框：同一帧就得看得见（PostEditChangeProperty 之后立刻生效的要求）。
	Volume->bShowBoundsInGame = true;
	Volume->OnConstruction(Volume->GetActorTransform());
	TestTrue(TEXT("Turning the in-game toggle back on makes the bounds visible again"), Bounds->IsVisible());
	TestTrue(TEXT("Turning the in-game toggle back on makes the band visible again"), Band->IsVisible());
	TestEqual(TEXT("Bounds collision is still off after re-showing"), Bounds->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

	TestWorld.Close();
	return true;
}

#endif
