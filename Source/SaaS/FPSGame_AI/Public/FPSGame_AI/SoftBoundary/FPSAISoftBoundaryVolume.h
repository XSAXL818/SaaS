#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSGame_AI/SoftBoundary/FPSAISoftBoundaryMath.h"
#include "FPSAISoftBoundaryVolume.generated.h"

class UBoxComponent;
class UTextRenderComponent;

/** 体积语义：禁区往里触发（封路口），可玩区往外触发（整张图外圈）。 */
UENUM(BlueprintType)
enum class EFPSAISoftBoundaryMode : uint8
{
	/** 禁区：走进去触发变暗并退回，用来封死路口或划出不能进的区域。 */
	KeepOut UMETA(DisplayName = "KeepOut (禁区)"),

	/** 可玩区：走出去触发变暗并拉回，用来圈住整张图的合法活动范围。 */
	KeepIn UMETA(DisplayName = "KeepIn (可玩区)"),
};

/** 体积形状。圆柱用 BoxExtent 的 X/Y 较小值当半径、Z 当半高。 */
UENUM(BlueprintType)
enum class EFPSAISoftBoundaryShape : uint8
{
	Box UMETA(DisplayName = "Box (盒体)"),
	Cylinder UMETA(DisplayName = "Cylinder (圆柱)"),
};

/**
 * 暗影蠕动风格预设。切换档位等于换一组 (NoiseSpeed, NoiseStrength, EdgeFeather) 材质标量，
 * 具体数值只在 AFPSAISoftBoundaryVolume::GetCrawlPresetParams 里有一份，不散落。
 *
 * 风格由"主导体积"决定：多颗体积同时在场时，取当前最深的那颗，它的预设写进后处理材质。
 */
UENUM(BlueprintType)
enum class EFPSAISoftBoundaryCrawlPreset : uint8
{
	/** 完全不蠕动：静态径向渐变。 */
	Off UMETA(DisplayName = "Off (静止)"),

	/** 轻微起伏。 */
	Calm UMETA(DisplayName = "Calm (轻微)"),

	/** 上一版的默认观感。 */
	Balanced UMETA(DisplayName = "Balanced (平衡)"),

	/** 明显的爬行，本次新默认。 */
	Crawl UMETA(DisplayName = "Crawl (爬行)"),

	/** 最剧烈。 */
	Swarm UMETA(DisplayName = "Swarm (最剧烈)"),
};

/** 一个蠕动预设翻译出来的三个后处理材质标量。 */
struct FFPSAISoftBoundaryCrawlParams
{
	float NoiseSpeed = 1.f;
	float NoiseStrength = 1.f;
	float EdgeFeather = 0.30f;
};

/**
 * KeepOut 体积在竖直方向能不能包容玩家身体中心。
 *
 * 判据不是"拿一个全局常数跟盒子比"，而是先从体积底面往下打射线取到脚下的地面（向下打空时再
 * 向上补打一次，覆盖"体积沉进楼板里"的摆法），再用
 * "地面 Z + StandingCenterAboveGround" 当成玩家站立时的身体中心：
 *   打不到地面          -> GroundUnknown（无法判定：视口静默，只留 Display 级日志，不报警）
 *   BoxTop    < 站立中心 -> TooShort（盒顶低于玩家身体，走不到里面）
 *   BoxBottom > 站立中心 -> AboveHead（站在下方地面的玩家够不到，盒子离地面太高）
 * 抬高的体积只要脚下有楼板就落在 Ok，不再被误报成"永不触发"。
 */
UENUM(BlueprintType)
enum class EFPSAIVerticalPlacement : uint8
{
	/** 玩家中心落在体积的 Z 区间内，能正常触发。 */
	Ok UMETA(DisplayName = "Ok (能触发)"),

	/** 盒顶低于站立中心：玩家身体整个在体积上方，中心永远进不来。 */
	TooShort UMETA(DisplayName = "TooShort (太矮)"),

	/** 盒底高于站立中心：站在下方地面的玩家够不到这个盒子，但更高处有楼板时可能是合法的。 */
	AboveHead UMETA(DisplayName = "AboveHead (地面够不到)"),

	/**
	 * 上下 GroundTraceDistance 内都没有任何地面：无法判定可达性。这是"不知道"，不是"永不触发"，
	 * 所以视口里一个字都不显示（加载时序也会走到这里），只在 Display 级日志里留一条。
	 */
	GroundUnknown UMETA(DisplayName = "GroundUnknown (脚下无地面)"),
};

/**
 * 可摆放的软边界体积。设计师把它拖进关卡、缩放、调参数即可，不再有硬编码坐标。
 *
 * 本类只负责"几何 + 参数 + 每个体积自己的安全落点记忆"，真正的逐帧求值、后处理
 * 推参和传送在 UFPSAISoftBoundaryComponent（挂在玩家 Pawn 上）里完成。
 *
 * 根组件是一个 NoCollision 的 UBoxComponent：它永远不会挡住角色，只作为编辑期可见的
 * 线框，同时提供 BoxExtent。第二个线框 TriggerBand 画出"从这里开始就全黑"的带子。
 */
UCLASS(Blueprintable, ClassGroup = (FPSGame_AI), meta = (DisplayName = "软边界体积 (FPS AI Soft Boundary)"))
class SAAS_API AFPSAISoftBoundaryVolume : public AActor
{
	GENERATED_BODY()

public:

	AFPSAISoftBoundaryVolume();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/**
	 * 编辑器世界里的低频重算节拍：世界分区的地板还没流送进来时，竖直判定会先打空（"脚下没地面"），
	 * 靠这个 tick 在地板到位后自动重新判定，不需要设计师去改一次属性才看到正确结果。
	 *
	 * 构造时打开了 bCanEverTick，但真正干活的只有编辑器世界：运行时的竖直判定由 BeginPlay 负责，
	 * 所以 Tick 第一件事就是 IsGameWorld() 早退，PIE / 游戏里绝不逐帧打射线。频率由
	 * EditorRefreshInterval 限制（默认 0.5 秒一次），且结果没变就不写组件。
	 */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * 编辑器世界用的是 LEVELTICK_ViewportsOnly 节拍，Actor 默认会被跳过 —— Actor 的 tick 函数
	 * 上并没有 bTickInEditor（那个开关只存在于 UActorComponent）。要拿到编辑器 tick 只能覆盖这个。
	 */
	virtual bool ShouldTickIfViewportsOnly() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * 立刻按当前几何重算一次竖直判定，更新视口警告字与只读缓存，判定变了才写日志。
	 * 编辑器世界里由 Tick 按 EditorRefreshInterval 调用；测试和排查也可以直接调用。
	 * 返回 true 表示这次刷新真的改了内容（文案或判定）。
	 */
	bool RefreshEditorVerticalState();

	/**
	 * Map Check / 资产校验：两种"摆下去也不会触发"的配置直接报错，不再静默 ——
	 * ①体积太薄又关了自动适配；②按体积脚下的实测地面算，玩家身体中心进不了体积的 Z 区间
	 *（盒子比玩家还矮，或站在下方地面的玩家根本够不到）。脚下打不到地面的体积只算"无法判定"，
	 * 不在这里报，免得把合法的抬高/悬空体积刷红。
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/**
	 * 有符号深度（厘米）。正数 = 已经越过边界多深，也就是"已经该变暗多少"：
	 * KeepIn 在盒外为正，KeepOut 在盒内为正。0 或负数表示安全。
	 *
	 * KeepOut 的深度只由**水平方向**往里走了多远决定；Z 只用来判断玩家在不在体积的竖直范围内。
	 * 贴地摆放的路口盒子因此不会被"到地面的距离"锁死。KeepIn 仍然是原来的 3D 有向距离。
	 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float EvaluateDepth(const FVector& WorldLocation) const;

	/** 形状有向距离：盒内/柱内为负，表面为 0，外面为正。不考虑 Mode。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float EvaluateOutsideDistance(const FVector& WorldLocation) const;

	/** 体积自身的变换，缩放单位化（BoxExtent 已经包含缩放）。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	FTransform GetVolumeTransform() const;

	/** 世界空间半尺寸（已含缩放）。圆柱只用 X/Y 的较小值作半径、Z 作半高。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	FVector GetVolumeExtent() const;

	/**
	 * 体积几何上最多能"走多深"（厘米）。KeepOut 的深度是水平嵌入深度，所以取水平方向最薄的
	 * 半轴 min(Extent.X, Extent.Y)（圆柱即半径）；贴地摆放的盒子不再被半高限制。
	 * KeepIn 靠越界多远来变暗，往外没有上限，所以固定返回 0，表示几何不构成限制。
	 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float GetMaxReachableDepth() const;

	/**
	 * 运行时真正参与暗度计算的全黑距离。bAutoFitFullBlack 打开时是
	 * min(FullBlackDistance, GetMaxReachableDepth())，薄体积因此也能走到全黑。
	 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float GetEffectiveFullBlackDistance() const;

	/** 关了自动适配、又因为体积太薄而永远到不了全黑。编辑器警告、数据校验、BeginPlay 日志共用这个判断。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	bool IsFullBlackUnreachable() const;

	/**
	 * 竖直方向能不能装下玩家（只对 KeepOut 有意义）。判据拿体积的 Z 区间跟"玩家身体中心"比：
	 *   BoxTop    < PlayerCenterZ -> TooShort（玩家身体整个在体积上方）
	 *   BoxBottom > PlayerCenterZ -> AboveHead（体积整个悬在玩家头顶）
	 * 两种情况 KeepOut 都不会触发，所以各自的修法相反，必须分开报。
	 *
	 * PlayerCenterZ 从哪来由调用方决定：运行时优先用实测 Pawn 中心 Z；编辑器里和"还没有 Pawn"的
	 * 启动序里用 EvaluateVerticalPlacementFromGround 按实测地面算出来。没有任何全局假设常数。
	 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	EFPSAIVerticalPlacement EvaluateVerticalPlacement(float PlayerCenterZ) const;

	/**
	 * 从体积底面中心找地面。起点是 (Center.X, Center.Y, Center.Z - Extent.Z)；
	 * 先往下打最多 GroundTraceDistance 厘米，打不到再往上打同样的距离（体积嵌进楼板里时，
	 * 楼板就在底面上面一点）。找到返回 true 并把命中面 Z 写进 OutGroundZ。
	 *
	 * 通道用 ECC_WorldStatic（静态网格、BSP、Landscape 默认都在上面），编辑器世界（非 PIE）与
	 * PIE 里都可用。每条射线都只取第一个 block；查询参数排除自己，并忽略所有
	 * AFPSAISoftBoundaryVolume —— 体积之间绝不能把对方的底面当成"地面"。
	 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	bool TraceGroundBelow(float& OutGroundZ) const;

	/**
	 * 同 TraceGroundBelow，并报告这次地面是"向下打到"还是"向上补打打到"的。
	 * 只读字段 bGroundFoundByUpwardTrace 与日志用它区分来源，便于排查"体积嵌进地板"的摆放。
	 */
	bool TraceGroundBelowWithSource(float& OutGroundZ, bool& bOutGroundFoundAbove) const;

	/**
	 * 竖直可达性：先向体积脚下取地面（含"向上补打"兜底），再用 GroundZ + StandingCenterAboveGround
	 * 当站立中心判定。向下向上都打不到地面时返回 GroundUnknown（"无法判定"），调用方不得据此
	 * 断言"永不触发"。编辑器警告字、数据校验、BeginPlay（没有本地玩家 Pawn 时）都走这一条。
	 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	EFPSAIVerticalPlacement EvaluateVerticalPlacementFromGround(float& OutGroundZ, float& OutStandingCenterZ) const;

	/** 体积 Z 区间的下沿（世界 Z）。玩家中心低于它才可能触发 KeepOut。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float GetVerticalBottom() const;

	/** 体积 Z 区间的上沿（世界 Z）。玩家中心高于它就不可能触发 KeepOut。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	float GetVerticalTop() const;

	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	bool IsKeepOut() const { return Mode == EFPSAISoftBoundaryMode::KeepOut; }

	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	bool IsCylinder() const { return Shape == EFPSAISoftBoundaryShape::Cylinder; }

	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	UBoxComponent* GetVolumeBounds() const { return VolumeBounds; }

	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	UBoxComponent* GetTriggerBand() const { return TriggerBand; }

	/** 编辑器警告字组件：测试和调试用它确认"装不下玩家 / 到不了全黑"真的显示出来了。 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	UTextRenderComponent* GetEditorWarningText() const { return EditorWarningText; }

	/** 把本体积的参数翻译成数学层用的设置结构。 */
	FFPSAISoftBoundarySettings MakeSettings() const;

	/**
	 * 预设 -> 材质标量的唯一映射表。组件写 MID 与自动化测试都用这一份，避免两套数值漂移。
	 * 标量名与 M_FPSAISoftBoundaryPP 上的参数一致：NoiseSpeed / NoiseStrength / EdgeFeather。
	 */
	static FFPSAISoftBoundaryCrawlParams GetCrawlPresetParams(EFPSAISoftBoundaryCrawlPreset Preset);

	/** 记录一个"安全"的 Pawn 位置，供全黑传送时优先使用。 */
	UFUNCTION(BlueprintCallable, Category = "FPSGame_AI|SoftBoundary")
	void RecordSafeLocation(const FVector& PawnLocation);

	/**
	 * 取本体积记录的安全落点。从未记录、或离当前位置超过 MaxDistanceCm 时返回 false，
	 * 调用方应退化成几何解。
	 */
	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	bool TryGetSafeLocation(const FVector& CurrentLocation, float MaxDistanceCm, FVector& OutLocation) const;

	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	bool HasSafeLocation() const { return bHasSafeLocation; }

	UFUNCTION(BlueprintPure, Category = "FPSGame_AI|SoftBoundary")
	FVector GetLastSafeLocation() const { return LastSafeLocation; }

	UFUNCTION(BlueprintCallable, Category = "FPSGame_AI|SoftBoundary")
	void ResetSafeLocation();

	/** KeepOut：禁区，越往里越暗；KeepIn：可玩区，越往外越暗。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ToolTip = "KeepOut=禁区：走进去触发变暗并退回，用来封路口。KeepIn=可玩区：走出去触发变暗并拉回，用来圈住整张图。"))
	EFPSAISoftBoundaryMode Mode = EFPSAISoftBoundaryMode::KeepOut;

	/** 盒体或圆柱。圆柱用 BoxExtent 的 X/Y 较小值作半径、Z 作半高。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ToolTip = "体积形状。Cylinder 用 BoxExtent 的 X/Y 较小值作半径、Z 作半高。"))
	EFPSAISoftBoundaryShape Shape = EFPSAISoftBoundaryShape::Box;

	/**
	 * 暗影蠕动风格。切换档位会把 (NoiseSpeed, NoiseStrength, EdgeFeather) 写进后处理材质；
	 * 多颗体积同时在场时只有"主导体积"（当前最深的那颗）的预设生效。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ToolTip = "暗影蠕动风格预设。暗度由当前最深的主导体积驱动，预设也跟随主导体积。"))
	EFPSAISoftBoundaryCrawlPreset CrawlPreset = EFPSAISoftBoundaryCrawlPreset::Crawl;

	/** 触发带宽度：从边界再往里/往外多深才到全黑。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ClampMin = "10.0", UIMin = "50.0", Units = "cm",
			ToolTip = "触发带宽度（厘米）：从边界再往里（KeepOut）/往外（KeepIn）多深才全黑并传送。"))
	float FullBlackDistance = 600.f;

	/**
	 * 自动适配（默认开）：KeepOut 盒内最深处只有"水平最薄半轴"这么深，薄盒子按原值永远黑不掉。
	 * 打开后有效全黑距离会被压到能走到的深度，保证薄体积也能全黑并传送。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ToolTip = "自动适配（默认开）：体积水平太薄、按 FullBlackDistance 永远走不到全黑时，运行时改用 min(FullBlackDistance, 水平最薄半轴深度)。关掉会保留老行为，但编辑器会警告、数据校验会报错，且该体积永远不会传送。"))
	bool bAutoFitFullBlack = true;

	/**
	 * 从体积底面往上/往下找地面的最大距离（厘米）。超出这个距离还没打到地面，就只提示"无法判定
	 * 可达性"，不报警 —— 悬在沟壑上方的体积可能完全正常。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ClampMin = "50.0", UIMin = "200.0", Units = "cm",
			ToolTip = "找地面的最大距离（厘米）：先向下打，打不到再向上打同样的距离。两条都没打到就只提示无法判定可达性，不报警。"))
	float GroundTraceDistance = 1000.f;

	/**
	 * 编辑器内的重算间隔（秒）。地板流送进来之后，竖直警告最多等这么久就会自动纠正，
	 * 不需要设计师改一次属性。只影响编辑器世界；运行时由 BeginPlay 负责。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary", AdvancedDisplay,
		meta = (ClampMin = "0.05", UIMin = "0.2", Units = "s",
			ToolTip = "编辑器内重算竖直判定的间隔（秒）。默认 0.5；世界分区的地板流送进来后，警告最多等这么久就会自动更新。"))
	float EditorRefreshInterval = 0.5f;

	/** 站在地面上时，玩家身体中心离地的高度（厘米）。默认 100，和 Character 默认胶囊一致。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ClampMin = "10.0", UIMin = "50.0", Units = "cm",
			ToolTip = "站在地面上时玩家身体中心离地的高度（厘米）。竖直判定用 地面Z + 这个值。"))
	float StandingCenterAboveGround = 100.f;

	/**
	 * 编辑器视口里的线框开关。关掉只影响可视性（主体框、触发带框、警告文字都不画），
	 * Actor 仍然能在大纲里选中、Details 里编辑，暗度/传送照常工作。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|Debug",
		meta = (ToolTip = "在编辑器视口里显示本体积的范围线框与触发带。"))
	bool bShowBoundsInEditor = true;

	/**
	 * PIE / 游戏里的线框开关。默认关（线框不该进正式画面）；打开后能在 PIE 里看到真实触发范围。
	 * 只影响可视性，不影响碰撞（体积本身是 NoCollision）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|Debug",
		meta = (ToolTip = "在 PIE/游戏运行时也显示范围线框（调试用，会影响画面）。"))
	bool bShowBoundsInGame = false;

	/** 只读：运行时实际使用的全黑距离（自动适配打开时是 min(FullBlackDistance, 水平最薄半轴深度)）。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FPSGame_AI|SoftBoundary",
		meta = (DisplayName = "EffectiveFullBlackDistance",
			ToolTip = "只读，运行时实际参与暗度计算的全黑距离。自动适配打开时等于 min(FullBlackDistance, 水平最薄半轴深度)。"))
	float EffectiveFullBlackDistance = 600.f;

	/**
	 * 只读：上一次竖直判定里的地面是"向上补打"打到的（体积底面沉进了楼板/地板里）。
	 * 排查"为什么这颗体积的 GroundZ 比底面还高"时看这个字段。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FPSGame_AI|SoftBoundary",
		meta = (DisplayName = "bGroundFoundByUpwardTrace",
			ToolTip = "只读，上一次竖直判定的地面是向上补打打到的（体积沉进楼板里）。"))
	bool bGroundFoundByUpwardTrace = false;

	/** 传送落点离边界多远。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ClampMin = "0.0", UIMin = "100.0", Units = "cm",
			ToolTip = "退回距离（厘米）：传送落点离边界多远。KeepOut 往体积外退，KeepIn 往体积里退。"))
	float ReturnDistance = 400.f;

	/** 变暗用时（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ClampMin = "0.0", UIMin = "0.2", Units = "s",
			ToolTip = "变暗用时（秒）：从透明到全黑至少需要这么久，避免一步跳黑。"))
	float DarkenDuration = 0.6f;

	/** 亮回用时（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ClampMin = "0.05", UIMin = "0.2", Units = "s",
			ToolTip = "亮回用时（秒）：退回安全区后画面恢复清晰需要的时间。"))
	float BrightenDuration = 0.75f;

	/** 滞回：深度回退到 -Hysteresis 才算真正安全，防止边界抖动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm",
			ToolTip = "滞回（厘米）：要退回边界内/外这么深才算安全，防止贴着边界抖动反复触发。"))
	float Hysteresis = 50.f;

	/** 传送冷却（秒），冷却期间不再触发传送。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPSGame_AI|SoftBoundary",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s",
			ToolTip = "传送冷却（秒）：传送后这段时间内不再触发第二次传送。"))
	float TeleportCooldown = 2.f;

protected:

	/** 主体框：可玩区外框 / 禁区本体，NoCollision 的编辑期线框。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|SoftBoundary",
		meta = (DisplayName = "VolumeBounds"))
	TObjectPtr<UBoxComponent> VolumeBounds;

	/** 触发带框：从主体框边界再往里/往外 FullBlackDistance 的那一层。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|SoftBoundary",
		meta = (DisplayName = "TriggerBand"))
	TObjectPtr<UBoxComponent> TriggerBand;

	/**
	 * 编辑器专用警告字：竖直摆放不可达（太矮 / 地面够不到）、或体积太薄到不了全黑时浮在 Actor 上方。
	 * 用 TextRender 而不是 DrawDebugString —— 后者只往 PlayerController 的 HUD 上画，
	 * 编辑器世界（非 PIE）没有 PlayerController，画出来是空的。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSGame_AI|SoftBoundary",
		meta = (DisplayName = "EditorWarningText"))
	TObjectPtr<UTextRenderComponent> EditorWarningText;

	/** 本体积记住的最后一个"安全"位置（调试可见）。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FPSGame_AI|SoftBoundary",
		meta = (DisplayName = "LastSafeLocation"))
	FVector LastSafeLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FPSGame_AI|SoftBoundary",
		meta = (DisplayName = "HasSafeLocation"))
	bool bHasSafeLocation = false;

private:

	/** 按 Mode 上色、按有效全黑距离更新触发带框尺寸、刷新只读字段和编辑器警告字。 */
	void UpdateEditorVisuals();

	/**
	 * 把两个显示开关翻译成组件可见性：
	 *   编辑器世界（含 PIE 之外的编辑器视口）—— bShowBoundsInEditor 决定主体框/触发带/警告字画不画；
	 *   PIE / 游戏世界 —— bShowBoundsInGame 决定主体框/触发带画不画（警告字是编辑器专用，永远不画）。
	 * 只碰可见性，不碰碰撞：体积依旧是 NoCollision，EvaluateDepth 不受影响。
	 */
	void ApplyBoundsVisibility();

	/**
	 * 竖直判定的完整实现：取地面（先向下，再向上补打）后再判定，并报告地面是从哪一侧打到的。
	 * EvaluateVerticalPlacementFromGround 与编辑器周期性刷新都走这一条，避免两套口径。
	 */
	EFPSAIVerticalPlacement EvaluateVerticalStateFromGround(
		float& OutGroundZ, float& OutStandingCenterZ, bool& bOutGroundFoundAbove) const;

	/**
	 * 竖直判定的措辞。视口文字、BeginPlay 日志、数据校验三处共用同一套语义，避免措辞各说一套。
	 * 只描述实测到的东西（含具体数字与可执行动作），不再出现 "will never trigger" 这种绝对断言。
	 * StandingLabel 说明 standing 这个数字的来源，例如 "on ground z=0" 或 "from pawn"。
	 * 返回空字符串表示这种摆放没有问题、不需要提示。
	 */
	FString BuildVerticalPlacementText(EFPSAIVerticalPlacement Placement, float StandingCenterZ, const FString& StandingLabel) const;

	/**
	 * 视口里该显示的字（带 "!" 前缀），没有要说的就返回空串。
	 * GroundUnknown 静默：无法判定时视口一个字都不留，信息只进 Display 级日志 —— 否则
	 * 加载时序（地板还没流送进来）和真正悬空的体积都会在视口里挂一条 "?"，让人以为功能坏了。
	 */
	FString BuildEditorWarningText(EFPSAIVerticalPlacement Placement, float StandingCenterZ, const FString& StandingLabel) const;

	/** 把一次竖直判定写进日志：TooShort / AboveHead 用 Warning，GroundUnknown 只给 Display。 */
	void LogVerticalPlacement(EFPSAIVerticalPlacement Placement, float StandingCenterZ, const FString& StandingLabel) const;

#if WITH_EDITOR
	/** 编辑器重算的累计时间（秒）。累到 EditorRefreshInterval 才真的重算一次。 */
	float EditorRefreshAccumulator = 0.f;

	/** 上一次刷新的判定、文案与地面来源。结果没变就不写组件、不刷只读字段。 */
	EFPSAIVerticalPlacement CachedVerticalPlacement = EFPSAIVerticalPlacement::Ok;
	FString CachedVerticalText;
	bool CachedGroundFoundByUpwardTrace = false;
	bool bHasCachedVerticalState = false;

	/** 警告字上一次摆放的位置/朝向。只有真的变了才写，避免每 0.5 秒把组件标脏一次。 */
	FVector CachedWarningAnchorLocation = FVector::ZeroVector;
	FRotator CachedWarningAnchorRotation = FRotator::ZeroRotator;
	bool bHasCachedWarningAnchor = false;
#endif
};
