#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryVolume.h"

#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryLog.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

DEFINE_LOG_CATEGORY(LogFPSAISoftBoundary);

#define LOCTEXT_NAMESPACE "FPSAISoftBoundaryVolume"

namespace
{
	/** 主体框颜色：KeepOut 红、KeepIn 绿。 */
	const FColor kKeepOutColor(230, 60, 60);
	const FColor kKeepInColor(40, 220, 120);

	/** 触发带框统一用琥珀色，和主体框区分开。 */
	const FColor kTriggerBandColor(255, 200, 0);

	/** 编辑器警告字的颜色与尺寸。 */
	const FColor kWarningTextColor(255, 230, 0);

	constexpr float kVolumeLineThickness = 4.f;
	constexpr float kTriggerBandLineThickness = 2.f;
	constexpr float kWarningTextWorldSize = 40.f;

#if WITH_EDITOR
	/** 警告字浮在主体框顶部再往上的高度（厘米）。 */
	constexpr float kWarningTextHeightAboveVolume = 160.f;
#endif

	/**
	 * 找地面射线的起点补偿（厘米）。体积底面正好压在楼板上时，射线起点就落在碰撞表面上，
	 * 某些碰撞实现会漏掉这次命中；抬高这么一点点再打一次，作为"起点退化"的兜底。
	 * 命中点仍是真实表面，补偿不会改变 GroundZ 的数值。
	 */
	constexpr float kGroundTraceProbeLiftCm = 1.f;

	/** 日志/校验/警告字里的形状名。引擎默认字体只有拉丁字形，所以刻意全用 ASCII。 */
	inline const TCHAR* GetShapeLabel(bool bCylinder)
	{
		return bCylinder ? TEXT("CYLINDER") : TEXT("BOX");
	}
}

AFPSAISoftBoundaryVolume::AFPSAISoftBoundaryVolume()
{
	// 纯几何 + 参数容器：运行时不需要逐帧 Tick，求值在 Pawn 的组件里做。
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

#if WITH_EDITOR
	// 编辑器世界需要一次低频重算：地图刚打开时 World Partition 的地板还没流送进来，竖直判定
	// 会先打空；等地板到位后由 Tick 自动纠正，不需要设计师改一次属性。运行时在 BeginPlay 里
	// 关掉这个 tick，两边互不影响。
	PrimaryActorTick.bStartWithTickEnabled = true;
#endif

	VolumeBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumeBounds"));
	SetRootComponent(VolumeBounds);
	VolumeBounds->SetBoxExtent(FVector(1000.f, 1000.f, 400.f));
	VolumeBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VolumeBounds->SetGenerateOverlapEvents(false);
	VolumeBounds->SetHiddenInGame(true);
	VolumeBounds->SetVisibility(true);
	VolumeBounds->SetLineThickness(kVolumeLineThickness);
	VolumeBounds->ShapeColor = kKeepOutColor;
	VolumeBounds->bDrawOnlyIfSelected = false;

	// 触发带框：只做编辑期可视化，永远没有碰撞，也不参与任何查询。
	TriggerBand = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBand"));
	TriggerBand->SetupAttachment(VolumeBounds);
	TriggerBand->SetBoxExtent(FVector(1000.f, 1000.f, 400.f));
	TriggerBand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TriggerBand->SetGenerateOverlapEvents(false);
	TriggerBand->SetHiddenInGame(true);
	TriggerBand->SetVisibility(true);
	TriggerBand->SetLineThickness(kTriggerBandLineThickness);
	TriggerBand->ShapeColor = kTriggerBandColor;
	TriggerBand->bDrawOnlyIfSelected = false;

	// 编辑器专用警告字。不挂到 VolumeBounds 下面，免得继承设计师拉的缩放把字号放大成天一样；
	// 位置在 UpdateEditorVisuals 里按主体框尺寸直接给世界坐标。
	EditorWarningText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("EditorWarningText"));
	EditorWarningText->SetHorizontalAlignment(EHTA_Center);
	EditorWarningText->SetVerticalAlignment(EVRTA_TextCenter);
	EditorWarningText->SetWorldSize(kWarningTextWorldSize);
	EditorWarningText->SetTextRenderColor(kWarningTextColor);
	EditorWarningText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EditorWarningText->SetHiddenInGame(true);
	EditorWarningText->bIsEditorOnly = true;
	EditorWarningText->SetText(FText::GetEmpty());
}

void AFPSAISoftBoundaryVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 这次刷新只是"立刻看到反馈"，加载时序里它可能还没有地板可打（那时竖直判定会得到
	// GroundUnknown）。GroundUnknown 在视口里是静默的，真正的权威结果由 Tick 里的低频重算
	// 在地板流送进来后补上，见 Tick / RefreshEditorVerticalState。
	UpdateEditorVisuals();
}

bool AFPSAISoftBoundaryVolume::ShouldTickIfViewportsOnly() const
{
	// 编辑器世界按 LEVELTICK_ViewportsOnly 节拍 tick，Actor 默认被跳过；编辑器里的低频重算就靠它。
	return true;
}

void AFPSAISoftBoundaryVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 运行时不需要这次重算：那时的竖直判定由 BeginPlay 负责，逐帧打射线只会白烧性能。
	// KeepIn 没有竖直判定，连重算都不用。
	const UWorld* World = GetWorld();
	if (!World || World->IsGameWorld() || !IsKeepOut())
	{
		return;
	}

#if WITH_EDITOR
	const float Interval = FMath::Max(0.05f, EditorRefreshInterval);
	EditorRefreshAccumulator += FMath::Max(0.f, DeltaSeconds);
	if (EditorRefreshAccumulator < Interval)
	{
		return;
	}
	EditorRefreshAccumulator = 0.f;

	RefreshEditorVerticalState();
#endif
}

#if WITH_EDITOR
void AFPSAISoftBoundaryVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 改 FullBlackDistance / Mode / Shape / bAutoFitFullBlack / GroundTraceDistance /
	// StandingCenterAboveGround 之后立刻刷新颜色、触发带框和警告字，不用等重新选中 Actor。
	// bShowBoundsInEditor / bShowBoundsInGame 也从这里立刻生效（UpdateEditorVisuals 里转成可见性）。
	UpdateEditorVisuals();
}

EDataValidationResult AFPSAISoftBoundaryVolume::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (IsFullBlackUnreachable())
	{
		// 只在这种配置下才会出现：用户关了自动适配，又把 FullBlackDistance 设得比最薄轴还大。
		// 结果就是这个体积永远暗不到全黑、永远不传送。这正是要消灭的静默失败。
		const float MaxReachable = GetMaxReachableDepth();
		Context.AddError(FText::Format(
			LOCTEXT("FullBlackUnreachable",
				"{0}: FullBlackDistance ({1} cm) is larger than the thinnest axis ({2} cm), so this volume can never reach full black and will never teleport. Lower FullBlackDistance or enable bAutoFitFullBlack."),
			FText::FromString(GetName()),
			FText::AsNumber(FullBlackDistance),
			FText::AsNumber(MaxReachable)));
		Result = EDataValidationResult::Invalid;
	}

	// 另一种静默失败：玩家身体中心落在体积的 Z 区间外，所以这个 KeepOut 从来不会触发。自动适配救不了它
	//（几何够宽够深，只是竖直区间没盖住玩家）。太矮和"够不到"的修法相反，所以必须分两种报。
	//
	// 判据改成"向体积脚下取地面"，不再用全局假设常数：抬高的体积只要脚下有楼板就是合法的，
	// 不报；只有"下方地面上的玩家够不到"或"盒子比玩家还矮"才报。
	// GroundUnknown（脚下没有地面）不写任何 issue —— 那是"无法判定"，而悬在沟壑上方的体积
	// 完全可能没问题，不能因此把资产校验刷红。信息只留在 Display 级日志里（视口对这种情况静默）。
	float GroundZ = 0.f;
	float StandingCenterZ = 0.f;
	const EFPSAIVerticalPlacement Placement = EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ);
	if (Placement == EFPSAIVerticalPlacement::TooShort || Placement == EFPSAIVerticalPlacement::AboveHead)
	{
		const FString StandingLabel = FString::Printf(TEXT("on ground z=%.0f"), GroundZ);
		Context.AddError(FText::Format(
			LOCTEXT("VerticalPlacement", "{0}: {1} {2}"),
			FText::FromString(GetName()),
			FText::FromString(GetShapeLabel(IsCylinder())),
			FText::FromString(BuildVerticalPlacementText(Placement, StandingCenterZ, StandingLabel))));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

void AFPSAISoftBoundaryVolume::BeginPlay()
{
	Super::BeginPlay();

	// 编辑器世界的那条低频重算 tick 在这里没有意义（运行时第一帧就会 IsGameWorld 早退），
	// 直接把 tick 关掉，PIE / 游戏里等于零开销。编辑器世界里的 Actor 不会跑 BeginPlay，
	// 所以这条只影响运行时实例。
	SetActorTickEnabled(false);

	// 运行时也可能没人跑过构造脚本（例如运行时生成），这里再刷一次只读字段。
	EffectiveFullBlackDistance = GetEffectiveFullBlackDistance();

	// PIE 的 Actor 是从编辑器世界复制出来的，会继承"编辑器里被关掉线框"的那个 bVisible；
	// 进游戏时按 bShowBoundsInGame 重新算一次，否则调试线框会在 PIE 里永远不出现。
	ApplyBoundsVisibility();

	const FVector Extent = GetVolumeExtent();
	UE_LOG(LogFPSAISoftBoundary, Display,
		TEXT("Soft-boundary volume registered: %s mode=%s shape=%s center=%s extent=%s FullBlackDistance=%.1f EffectiveFullBlackDistance=%.1f ReturnDistance=%.1f"),
		*GetName(),
		IsKeepOut() ? TEXT("KeepOut") : TEXT("KeepIn"),
		IsCylinder() ? TEXT("Cylinder") : TEXT("Box"),
		*GetActorLocation().ToCompactString(),
		*Extent.ToCompactString(),
		FullBlackDistance,
		EffectiveFullBlackDistance,
		ReturnDistance);

	if (IsFullBlackUnreachable())
	{
		// 走到这里有活干也没用：这个体积永远不会传送。说清楚原因和两个修法。
		UE_LOG(LogFPSAISoftBoundary, Warning,
			TEXT("Soft-boundary volume %s will NEVER teleport: FullBlackDistance is %.1f cm but the volume can only reach %.1f cm of depth on its thinnest axis, and bAutoFitFullBlack is off. Lower FullBlackDistance or enable bAutoFitFullBlack."),
			*GetName(),
			FullBlackDistance,
			GetMaxReachableDepth());
	}

	// 竖直可达性：有本地玩家 Pawn 就用它的实测中心 Z（最准 —— 玩家可能正站在二层楼板上）；
	// 没有 Pawn（编辑器世界、启动序里还没生成 Pawn）才走"向体积脚下打射线取地面"。
	// 两条路都不再用全局假设常数。
	const APawn* PlayerPawn = nullptr;
	if (const UWorld* World = GetWorld())
	{
		const APlayerController* PlayerController = World->GetFirstPlayerController();
		PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	}

	if (PlayerPawn)
	{
		const float PlayerCenterZ = static_cast<float>(PlayerPawn->GetActorLocation().Z);
		LogVerticalPlacement(EvaluateVerticalPlacement(PlayerCenterZ), PlayerCenterZ, TEXT("from pawn"));
	}
	else
	{
		float GroundZ = 0.f;
		float StandingCenterZ = 0.f;
		const EFPSAIVerticalPlacement Placement = EvaluateVerticalPlacementFromGround(GroundZ, StandingCenterZ);
		const FString StandingLabel = FString::Printf(TEXT("on ground z=%.0f"), GroundZ);
		LogVerticalPlacement(Placement, StandingCenterZ, StandingLabel);
	}
}

float AFPSAISoftBoundaryVolume::EvaluateOutsideDistance(const FVector& WorldLocation) const
{
	return FPSAISoftBoundaryMath::ComputeShapeOutsideDistance(
		WorldLocation, GetVolumeTransform(), GetVolumeExtent(), IsCylinder());
}

float AFPSAISoftBoundaryVolume::EvaluateDepth(const FVector& WorldLocation) const
{
	// KeepIn 靠"走出体积多远"来变暗，往外没有面可贴，3D 有向距离就是答案，保持原样。
	if (!IsKeepOut())
	{
		return EvaluateOutsideDistance(WorldLocation);
	}

	// KeepOut 的深度是"水平方向往里走了多远"。玩家站在地面时，到盒底面的距离天然很小，
	// 一旦把 Z 算进去，任何贴地摆放的路口盒子都会被 Z 锁死在半高上，永远暗不到全黑。
	const FTransform VolumeTransform = GetVolumeTransform();
	const FVector Extent = GetVolumeExtent();
	const FVector Local = VolumeTransform.InverseTransformPosition(WorldLocation);

	// 竖直方向不在体积里（头顶上、脚底下）：这个位置既不是禁区，也谈不上"走出去了"，深度取 0。
	if (FMath::Abs(Local.Z) > Extent.Z)
	{
		return 0.f;
	}

	const float HorizontalDepth = FPSAISoftBoundaryMath::ComputeKeepOutDepth(
		WorldLocation, VolumeTransform, Extent, IsCylinder());
	if (HorizontalDepth > 0.f)
	{
		return HorizontalDepth;
	}

	// 水平方向在体积外：此时 Z 在范围内，3D 有向距离只剩水平分量，取负就是"还差多远才碰到边界"。
	// 落点校验和"安全落点记忆"（深度 <= -Hysteresis）都靠这个负值，不能省。
	return FPSAISoftBoundaryMath::ComputeSignedDepth(EvaluateOutsideDistance(WorldLocation), true);
}

FTransform AFPSAISoftBoundaryVolume::GetVolumeTransform() const
{
	if (!VolumeBounds)
	{
		return GetActorTransform();
	}

	FTransform BoxTransform = VolumeBounds->GetComponentTransform();
	BoxTransform.SetScale3D(FVector::OneVector);
	return BoxTransform;
}

FVector AFPSAISoftBoundaryVolume::GetVolumeExtent() const
{
	return VolumeBounds ? VolumeBounds->GetScaledBoxExtent() : FVector::ZeroVector;
}

float AFPSAISoftBoundaryVolume::GetMaxReachableDepth() const
{
	// KeepIn 靠"走出体积多远"来变暗，往外是没有上限的，几何永远挡不住全黑。
	if (!IsKeepOut())
	{
		return 0.f;
	}

	// KeepOut 的深度是水平嵌入深度，所以盒内最深能走到"水平方向最薄的那个半轴"：
	// 贴地摆放的盒子不再被半高（中心到底面）限制。圆柱的半径就是 X/Y 的较小值，同一个式子。
	const FVector Extent = GetVolumeExtent();
	return FMath::Max(0.f, static_cast<float>(FMath::Min(Extent.X, Extent.Y)));
}

float AFPSAISoftBoundaryVolume::GetEffectiveFullBlackDistance() const
{
	const float Configured = FMath::Max(1.f, FullBlackDistance);

	// 关掉自动适配，或者体积本来就不受几何限制（KeepIn）：原样使用用户设的值。
	if (!bAutoFitFullBlack || !IsKeepOut())
	{
		return Configured;
	}

	// 自动适配：全黑距离不能超过盒子里能走到的最深处，否则永远到不了全黑。
	return FMath::Clamp(FMath::Min(Configured, GetMaxReachableDepth()), 1.f, Configured);
}

bool AFPSAISoftBoundaryVolume::IsFullBlackUnreachable() const
{
	// 自动适配会把有效值压到能走到的深度，所以只有关掉它才可能出现"永远到不了"。
	if (!IsKeepOut() || bAutoFitFullBlack)
	{
		return false;
	}

	return GetMaxReachableDepth() < FMath::Max(1.f, FullBlackDistance);
}

EFPSAIVerticalPlacement AFPSAISoftBoundaryVolume::EvaluateVerticalPlacement(float PlayerCenterZ) const
{
	// 只管 KeepOut：它靠"玩家身体中心落在体积的 Z 区间内"触发。KeepIn 靠走出体积触发，
	// 竖直方向矮一点、高一点都照常工作，所以不在这里报。
	if (!IsKeepOut())
	{
		return EFPSAIVerticalPlacement::Ok;
	}

	// 判据是"区间包不包得住玩家中心"，不是"半高够不够大"。贴地摆放的盒子 Z 区间是
	// [0, 2*Extent.Z]，玩家中心约 100，所以半高 >= 50 就装得下：半高 90 的盒子区间是
	// [0,180]，正常触发；旧判据 Extent.Z < 100 会把它误报成"永远不触发"。
	const float BoxBottom = GetVerticalBottom();
	const float BoxTop = GetVerticalTop();

	if (BoxTop < PlayerCenterZ)
	{
		// 盒顶低于玩家中心：身体整个在盒子上方，中心永远进不来。
		return EFPSAIVerticalPlacement::TooShort;
	}

	if (BoxBottom > PlayerCenterZ)
	{
		// 盒底高于玩家中心：站着够不到这个盒子（但下方有楼板时可能合法，所以文案只描述事实）。
		return EFPSAIVerticalPlacement::AboveHead;
	}

	return EFPSAIVerticalPlacement::Ok;
}

bool AFPSAISoftBoundaryVolume::TraceGroundBelow(float& OutGroundZ) const
{
	bool bFoundAbove = false;
	return TraceGroundBelowWithSource(OutGroundZ, bFoundAbove);
}

bool AFPSAISoftBoundaryVolume::TraceGroundBelowWithSource(float& OutGroundZ, bool& bOutGroundFoundAbove) const
{
	OutGroundZ = 0.f;
	bOutGroundFoundAbove = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// 起点就是体积底面中心（胶囊"脚下"）。向下、向上各最多 GroundTraceDistance 厘米。
	const FVector BottomCenter = GetVolumeTransform().GetLocation() - FVector(0.f, 0.f, GetVolumeExtent().Z);
	const float Reach = FMath::Max(50.f, GroundTraceDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(FPSAISoftBoundaryGround), false);
	Params.AddIgnoredActor(this);

	// 同类体积永远不当地面：它们的线框现在没有碰撞，但规则写死在这里，以后改碰撞也不会让
	// 一个体积把另一个体积的底面当成脚下地面。
	for (TActorIterator<AFPSAISoftBoundaryVolume> It(World); It; ++It)
	{
		AFPSAISoftBoundaryVolume* OtherVolume = *It;
		if (OtherVolume && OtherVolume != this)
		{
			Params.AddIgnoredActor(OtherVolume);
		}
	}

	// 静态网格、BSP、Landscape 默认都在 ECC_WorldStatic 上：编辑器世界（非 PIE）与 PIE 都查得到。
	// 只取第一个 block（LineTraceSingleByChannel），命中面的 Z 就是地面。
	const auto TraceAlong = [World, &Params, Reach](const FVector& Start, const FVector& Direction, float& OutZ) -> bool
	{
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Start, Start + Direction * Reach, ECC_WorldStatic, Params))
		{
			return false;
		}

		if (!Hit.bBlockingHit)
		{
			return false;
		}

		OutZ = static_cast<float>(Hit.ImpactPoint.Z);
		return true;
	};

	const FVector Down(0.f, 0.f, -1.f);
	if (TraceAlong(BottomCenter, Down, OutGroundZ))
	{
		return true;
	}

	// 底面正好压在楼板表面时，起点落在碰撞表面上，有的碰撞实现不报这次命中；抬 1cm 再打一次。
	if (TraceAlong(BottomCenter + FVector(0.f, 0.f, kGroundTraceProbeLiftCm), Down, OutGroundZ))
	{
		return true;
	}

	// 向下彻底打空：体积可能是"沉进楼板里"摆的（底面在地板面以下），此时楼板在底面**上方**
	// 一点。再从同一个起点向上打同样的距离补一次，命中面的 Z 当地面用。
	// 只有上下都打不到才算"无法判定"。
	const FVector Up(0.f, 0.f, 1.f);
	if (TraceAlong(BottomCenter, Up, OutGroundZ))
	{
		bOutGroundFoundAbove = true;
		return true;
	}

	// 与向下的那次抬升对称：底面正好贴在楼板下表面上时，起点落在碰撞表面上可能不报命中，
	// 从再低 1cm 的位置补打一次（命中面还是真实表面，GroundZ 不受影响）。
	if (TraceAlong(BottomCenter - FVector(0.f, 0.f, kGroundTraceProbeLiftCm), Up, OutGroundZ))
	{
		bOutGroundFoundAbove = true;
		return true;
	}

	return false;
}

EFPSAIVerticalPlacement AFPSAISoftBoundaryVolume::EvaluateVerticalPlacementFromGround(float& OutGroundZ, float& OutStandingCenterZ) const
{
	bool bGroundFoundAbove = false;
	return EvaluateVerticalStateFromGround(OutGroundZ, OutStandingCenterZ, bGroundFoundAbove);
}

EFPSAIVerticalPlacement AFPSAISoftBoundaryVolume::EvaluateVerticalStateFromGround(
	float& OutGroundZ, float& OutStandingCenterZ, bool& bOutGroundFoundAbove) const
{
	OutGroundZ = 0.f;
	OutStandingCenterZ = 0.f;
	bOutGroundFoundAbove = false;

	// KeepIn 不需要竖直判定（见 EvaluateVerticalPlacement），别白打射线。
	if (!IsKeepOut())
	{
		return EFPSAIVerticalPlacement::Ok;
	}

	if (!TraceGroundBelowWithSource(OutGroundZ, bOutGroundFoundAbove))
	{
		// 向下、向上都没打到地面：无法判定可达性。这是"不知道"，不是"永不触发"。
		return EFPSAIVerticalPlacement::GroundUnknown;
	}

	OutStandingCenterZ = OutGroundZ + StandingCenterAboveGround;
	return EvaluateVerticalPlacement(OutStandingCenterZ);
}

FString AFPSAISoftBoundaryVolume::BuildVerticalPlacementText(
	EFPSAIVerticalPlacement Placement, float StandingCenterZ, const FString& StandingLabel) const
{
	// 引擎默认字体是 Roboto（只有拉丁字形），所以这里刻意全用 ASCII：中文会渲染成一排豆腐块。
	// 每条都带上实测数字与"下一步该做什么"，绝不再出现 "will never trigger" 这类绝对断言。
	switch (Placement)
	{
	case EFPSAIVerticalPlacement::TooShort:
		return FString::Printf(
			TEXT("TOO SHORT (top=%.0f < standing %.0f %s) : raise Z extent or lower the box"),
			GetVerticalTop(),
			StandingCenterZ,
			*StandingLabel);

	case EFPSAIVerticalPlacement::AboveHead:
		return FString::Printf(
			TEXT("OUT OF REACH FROM GROUND (bottom=%.0f > standing %.0f %s) : a player walking below cannot enter; raise the box only if a floor exists at that height"),
			GetVerticalBottom(),
			StandingCenterZ,
			*StandingLabel);

	case EFPSAIVerticalPlacement::GroundUnknown:
		// 向下、向上都打空了，所以文案把两个方向都写出来（探测量是每边各 GroundTraceDistance）。
		return FString::Printf(
			TEXT("NO GROUND BELOW OR ABOVE (trace %.0fcm each way) : reachability unknown"),
			FMath::Max(50.f, GroundTraceDistance));

	default:
		return FString();
	}
}

FString AFPSAISoftBoundaryVolume::BuildEditorWarningText(
	EFPSAIVerticalPlacement Placement, float StandingCenterZ, const FString& StandingLabel) const
{
	// GroundUnknown 在视口里保持静默：加载时序（地板还没流送进来）和真正悬在虚空上方的体积都会
	// 走到这里，视口里挂一条 "?" 只会让人以为功能坏了。信息只留在 Display 级日志里。
	if (Placement == EFPSAIVerticalPlacement::GroundUnknown)
	{
		return FString();
	}

	const FString PlacementMessage = BuildVerticalPlacementText(Placement, StandingCenterZ, StandingLabel);
	if (!PlacementMessage.IsEmpty())
	{
		// "!" = 这颗体积摆下去不会触发，必须在视口里喊出来（IsDataValid 里同时报 Error）。
		return FString::Printf(TEXT("! %s %s"), GetShapeLabel(IsCylinder()), *PlacementMessage);
	}

	if (!IsFullBlackUnreachable())
	{
		return FString();
	}

	// 措辞把"该做什么"放在最前面：改尺寸，或者改小全黑距离。右边括号是 FullBlackDistance > 能走到的深度。
	return FString::Printf(
		TEXT("! RESIZE BOX OR LOWER FullBlackDistance (%.0f > %.0f cm)"),
		FullBlackDistance,
		GetMaxReachableDepth());
}

void AFPSAISoftBoundaryVolume::LogVerticalPlacement(
	EFPSAIVerticalPlacement Placement, float StandingCenterZ, const FString& StandingLabel) const
{
	const FString Detail = BuildVerticalPlacementText(Placement, StandingCenterZ, StandingLabel);
	if (Detail.IsEmpty())
	{
		return;
	}

	// "脚下没有地面"跟形状无关，就不加 BOX/CYLINDER 前缀了，免得读成 "BOX NO GROUND BELOW"。
	const FString ShapeSegment = (Placement == EFPSAIVerticalPlacement::GroundUnknown)
		? FString()
		: FString::Printf(TEXT("%s "), GetShapeLabel(IsCylinder()));

	if (Placement == EFPSAIVerticalPlacement::GroundUnknown)
	{
		// "无法判定"不是错误：只上屏（Display），不上 Warning 级，免得把没有地面的体积刷成一片黄。
		UE_LOG(LogFPSAISoftBoundary, Display,
			TEXT("Soft-boundary volume %s: %s%s"),
			*GetName(),
			*ShapeSegment,
			*Detail);
		return;
	}

	UE_LOG(LogFPSAISoftBoundary, Warning,
		TEXT("Soft-boundary volume %s: %s%s"),
		*GetName(),
		*ShapeSegment,
		*Detail);
}

float AFPSAISoftBoundaryVolume::GetVerticalBottom() const
{
	return static_cast<float>(GetVolumeTransform().GetLocation().Z - GetVolumeExtent().Z);
}

float AFPSAISoftBoundaryVolume::GetVerticalTop() const
{
	return static_cast<float>(GetVolumeTransform().GetLocation().Z + GetVolumeExtent().Z);
}

FFPSAISoftBoundaryCrawlParams AFPSAISoftBoundaryVolume::GetCrawlPresetParams(EFPSAISoftBoundaryCrawlPreset Preset)
{
	// 唯一的一份预设表：组件写 MID、自动化测试断言、以后加 HUD 都从这里取，不另抄一份数值。
	FFPSAISoftBoundaryCrawlParams Params;
	switch (Preset)
	{
	case EFPSAISoftBoundaryCrawlPreset::Off:
		// 速度/幅度都为 0：edgeNoise 恒等于 0，画面退化成静态径向渐变。
		Params.NoiseSpeed = 0.f;
		Params.NoiseStrength = 0.f;
		Params.EdgeFeather = 0.30f;
		break;

	case EFPSAISoftBoundaryCrawlPreset::Calm:
		Params.NoiseSpeed = 0.45f;
		Params.NoiseStrength = 0.65f;
		Params.EdgeFeather = 0.35f;
		break;

	case EFPSAISoftBoundaryCrawlPreset::Balanced:
		Params.NoiseSpeed = 1.f;
		Params.NoiseStrength = 1.f;
		Params.EdgeFeather = 0.30f;
		break;

	case EFPSAISoftBoundaryCrawlPreset::Swarm:
		Params.NoiseSpeed = 2.8f;
		Params.NoiseStrength = 1.85f;
		Params.EdgeFeather = 0.20f;
		break;

	case EFPSAISoftBoundaryCrawlPreset::Crawl:
	default:
		// 默认档：更密更快的细噪声占主导，读起来像虫群在爬而不是慢呼吸。
		Params.NoiseSpeed = 1.6f;
		Params.NoiseStrength = 1.35f;
		Params.EdgeFeather = 0.26f;
		break;
	}

	return Params;
}

FFPSAISoftBoundarySettings AFPSAISoftBoundaryVolume::MakeSettings() const
{
	FFPSAISoftBoundarySettings Settings;
	// 用有效距离：自动适配打开时，薄体积的全黑距离已经被压到盒内能走到的深度。
	Settings.FullBlackDistanceCm = FMath::Max(1.f, GetEffectiveFullBlackDistance());
	Settings.HysteresisCm = FMath::Max(0.f, Hysteresis);
	Settings.BrightenSeconds = FMath::Max(0.05f, BrightenDuration);
	// 传送后的恢复淡出和"亮回"用同一时长，避免两个参数互相打架。
	Settings.FadeOutSeconds = FMath::Max(0.05f, BrightenDuration);
	Settings.TeleportInsetCm = FMath::Max(0.f, ReturnDistance);
	Settings.TeleportCooldownSeconds = FMath::Max(0.f, TeleportCooldown);
	return Settings;
}

void AFPSAISoftBoundaryVolume::RecordSafeLocation(const FVector& PawnLocation)
{
	LastSafeLocation = PawnLocation;
	bHasSafeLocation = true;
}

bool AFPSAISoftBoundaryVolume::TryGetSafeLocation(const FVector& CurrentLocation, float MaxDistanceCm, FVector& OutLocation) const
{
	if (!bHasSafeLocation)
	{
		return false;
	}

	// 走远之后旧落点已经没有意义（可能已经被别的东西占住），退化成几何解更稳。
	if (FVector::DistSquared(LastSafeLocation, CurrentLocation) > FMath::Square(FMath::Max(0.f, MaxDistanceCm)))
	{
		return false;
	}

	OutLocation = LastSafeLocation;
	return true;
}

void AFPSAISoftBoundaryVolume::ResetSafeLocation()
{
	LastSafeLocation = FVector::ZeroVector;
	bHasSafeLocation = false;
}

void AFPSAISoftBoundaryVolume::ApplyBoundsVisibility()
{
	const UWorld* World = GetWorld();
	const bool bGameWorld = World != nullptr && World->IsGameWorld();

	// 一个开关管一个视角：编辑器世界看 bShowBoundsInEditor，PIE/游戏世界看 bShowBoundsInGame。
	// 两者互不牵连 —— 在编辑器里关掉线框，不该顺手把 PIE 的调试线框也关掉。
	const bool bShowBounds = bGameWorld ? bShowBoundsInGame : bShowBoundsInEditor;

	if (VolumeBounds)
	{
		// SetVisibility 管两个视角，SetHiddenInGame 只管游戏视角；两个都写，免得出现
		// "编辑器里关掉了、进 PIE 又冒出来"这种分裂状态。
		VolumeBounds->SetVisibility(bShowBounds);
		VolumeBounds->SetHiddenInGame(!bShowBoundsInGame);
	}

	if (TriggerBand)
	{
		TriggerBand->SetVisibility(bShowBounds);
		TriggerBand->SetHiddenInGame(!bShowBoundsInGame);
	}

	if (EditorWarningText)
	{
		// 警告字本来就只属于编辑器视口（bIsEditorOnly），PIE/游戏里一律不画。
		EditorWarningText->SetVisibility(!bGameWorld && bShowBoundsInEditor);
		EditorWarningText->SetHiddenInGame(true);
	}
}

void AFPSAISoftBoundaryVolume::UpdateEditorVisuals()
{
	// 关掉显示开关时主体框、触发带框、警告字一起消失；只碰可见性，碰撞与判定一概不动。
	ApplyBoundsVisibility();

	if (VolumeBounds)
	{
		VolumeBounds->ShapeColor = IsKeepOut() ? kKeepOutColor : kKeepInColor;
		VolumeBounds->SetLineThickness(kVolumeLineThickness);
	}

	// 只读的有效距离和"到不了全黑"的警告字都不依赖触发带框，先刷新它们。
	EffectiveFullBlackDistance = GetEffectiveFullBlackDistance();

	const FVector Extent = GetVolumeExtent();

#if WITH_EDITOR
	// 竖直警告字与只读缓存都交给这次刷新：它自己算文字位置，且判定/文案没变就不写组件。
	// 唯一权威的重算入口是 RefreshEditorVerticalState（OnConstruction / PostEditChangeProperty /
	// Tick 三处都走它），所以不会出现"某一条路径忘了更新"的情况。
	RefreshEditorVerticalState();
#endif

	if (!TriggerBand || !VolumeBounds)
	{
		return;
	}

	TriggerBand->ShapeColor = kTriggerBandColor;
	TriggerBand->SetLineThickness(kTriggerBandLineThickness);

	// 触发带画的是"真正到全黑"的那一层，所以用有效距离而不是用户填的原值：
	// 薄盒子开了自动适配时，带子会收窄到盒子里真正能走到的深度。
	const float Band = FMath::Max(0.f, EffectiveFullBlackDistance);

	// KeepIn：触发带在主体框外面一层（走出去才变暗）。KeepOut：在主体框里面一层（走进去才变暗）。
	// 圆柱形状用轴对齐包围盒近似，够设计师判断"大概到哪全黑"。
	FVector BandWorld;
	if (IsKeepOut())
	{
		// KeepOut 的深度只由水平内缩决定，Z 不参与，所以带子只在 X/Y 上收，高度保持原样 ——
		// 否则贴地摆放的盒子会显示成一片没有厚度的薄片，和实际触发位置对不上。
		BandWorld = FVector(
			FMath::Max(1.f, Extent.X - Band),
			FMath::Max(1.f, Extent.Y - Band),
			FMath::Max(1.f, Extent.Z));
	}
	else
	{
		BandWorld = Extent + FVector(Band, Band, Band);
	}

	// TriggerBand 挂在根组件下面，会继承根组件的缩放；这里除回去，免得缩放被叠加两次。
	const FVector Scale = VolumeBounds->GetComponentScale();
	const FVector LocalBand(
		BandWorld.X / FMath::Max(KINDA_SMALL_NUMBER, FMath::Abs(Scale.X)),
		BandWorld.Y / FMath::Max(KINDA_SMALL_NUMBER, FMath::Abs(Scale.Y)),
		BandWorld.Z / FMath::Max(KINDA_SMALL_NUMBER, FMath::Abs(Scale.Z)));
	TriggerBand->SetBoxExtent(LocalBand);
}

#if WITH_EDITOR
bool AFPSAISoftBoundaryVolume::RefreshEditorVerticalState()
{
	if (!EditorWarningText)
	{
		return false;
	}

	// 位置/朝向每次都算，但只有真的变了才写：文字清空后组件仍然在，若留在世界原点会把 Actor 的
	// 包围盒拖到原点去（编辑器里按 F 聚焦时会跑到地图中心）；而每 0.5 秒无条件 SetWorldLocation
	// 又会无谓地把组件标脏。所以两者都按"变了才写"处理。
	const FVector DesiredAnchorLocation =
		GetActorLocation() + FVector(0.f, 0.f, GetVolumeExtent().Z + kWarningTextHeightAboveVolume);
	const FRotator DesiredAnchorRotation = GetActorRotation();
	if (!bHasCachedWarningAnchor
		|| !DesiredAnchorLocation.Equals(CachedWarningAnchorLocation, 0.1f)
		|| !DesiredAnchorRotation.Equals(CachedWarningAnchorRotation, 0.01f))
	{
		EditorWarningText->SetWorldLocation(DesiredAnchorLocation);
		EditorWarningText->SetWorldRotation(DesiredAnchorRotation);
		CachedWarningAnchorLocation = DesiredAnchorLocation;
		CachedWarningAnchorRotation = DesiredAnchorRotation;
		bHasCachedWarningAnchor = true;
	}

	float GroundZ = 0.f;
	float StandingCenterZ = 0.f;
	bool bGroundFoundAbove = false;
	const EFPSAIVerticalPlacement Placement =
		EvaluateVerticalStateFromGround(GroundZ, StandingCenterZ, bGroundFoundAbove);
	const FString PlacementMessage = BuildEditorWarningText(
		Placement, StandingCenterZ, FString::Printf(TEXT("on ground z=%.0f"), GroundZ));

	// 判定、文案、地面来源三者都没变就什么都不写：地板还没流送进来时每 0.5 秒都会走到这里，
	// 必须便宜，也不能让每次刷新都去写只读字段/组件。
	const bool bPlacementChanged = !bHasCachedVerticalState || CachedVerticalPlacement != Placement;
	const bool bChanged = bPlacementChanged
		|| CachedGroundFoundByUpwardTrace != bGroundFoundAbove
		|| !CachedVerticalText.Equals(PlacementMessage, ESearchCase::CaseSensitive);

	if (!bChanged)
	{
		return false;
	}

	// 只在判定真的换了时写一条日志，避免每 0.5 秒刷一行；GroundUnknown 只给 Display，不报 Warning。
	if (bPlacementChanged && Placement != EFPSAIVerticalPlacement::Ok)
	{
		LogVerticalPlacement(Placement, StandingCenterZ, FString::Printf(TEXT("on ground z=%.0f"), GroundZ));
	}

	bHasCachedVerticalState = true;
	CachedVerticalPlacement = Placement;
	CachedVerticalText = PlacementMessage;
	CachedGroundFoundByUpwardTrace = bGroundFoundAbove;
	bGroundFoundByUpwardTrace = bGroundFoundAbove;

	EditorWarningText->SetText(FText::FromString(PlacementMessage));
	return true;
}
#endif

#undef LOCTEXT_NAMESPACE
