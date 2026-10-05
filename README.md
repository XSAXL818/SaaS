# SaaS（UE 5.8 学习项目）

> **名字说明**：`SaaS` 是当初建工程时随手起的，**没有任何具体含义**，纯属历史遗留。这是一个**个人学习 / 练习项目**，不是产品。

用 Unreal Engine 5.8 搭的一块练习田，内容分四部分：

1. **GAS 主线** —— 照 Aura 系列教程学习 GAS（属性集 → 效果 Actor → UI WidgetController），对应 `Content/Aura` 与 `Source/SaaS/{AbilitySystem, Character, Player, UI, ...}`。**具体进度与踩坑见 [DEVLOG.md](DEVLOG.md)**；
2. **长安大学建模地图** —— `Content/Maps/Changan`（约 950 个资产：静态网格 689 MB + 贴图 14 MB + 材质 0.4 MB）与关卡 `Content/Maps/Map_Changan.umap`（155 MB）；
3. **早期 C++ 练习** —— Actor 生命周期、Soft / Weak Reference、Smart Pointer、Timeline、接口、蓝图函数库等，对应 `Source/SaaS/My*.cpp` 与 `Content/Code`；
4. **FPS 学习线** —— 自建的纯蓝图 FPS 玩家与 GameMode（`Content/FPS/Code/Player`）与测试关卡 `Content/FPS/Maps/Default.umap`。

## 当前状态

- 性质：**个人学习项目**（不是产品）；学习内容见上方列表，**进度与踩坑见 [DEVLOG.md](DEVLOG.md)**
- 进度 / 踩坑记录：见 [DEVLOG.md](DEVLOG.md)；提交历史本身就是进度快照（`git log --oneline`）
- 本 README 的更新原则：**只在结构 / 配置 / 流程变化时更新**；进度一律写进 DEVLOG

---

## ⚠️ 先读这一段：仓库里**故意不放**美术资源

商店 / 教程素材体积太大（本地约 17 GB），既进不了 GitHub 的单文件限制，也不需要进 Git。

| | 内容 | 体积 |
|---|---|---|
| ✅ 在仓库里 | `Source/`、`Config/`、`SaaS.uproject`、`DEVLOG.md`、`README.md`，以及 `Content/` 下**自制目录**（放行规则见 `.gitignore` 白名单） | 约 **6.5 MB** |
| ❌ 不在仓库里 | `Content/` 下的**商店 / 教程素材**（`Fab`、`AuraAssets`、`Maps`、`Characters`、`StarterContent`、`FPSAssets` …）与 `Plugins/UnrealAgentLink` | 约 **17 GB** |

**引用不会断**：蓝图与 C++ 里对外部资产的引用只是**路径字符串**（`FSoftObjectPath`、`LoadObject` / `FObjectFinder`）。把资源按下面的清单装回**原来的路径**，所有引用会自动恢复，不需要手工重连。

## 环境要求

- Unreal Engine **5.8**（`SaaS.uproject` 的 `EngineAssociation` 为 `5.8`）
- Windows 10/11 + Visual Studio 2022（工作负载「使用 C++ 的游戏开发」）或 Rider
- 依赖引擎插件：`GameplayAbilities`、`ModelContextProtocol`、`AllToolsets`（已写进 `.uproject`）

## 克隆后怎么跑起来

1. `git clone https://github.com/XSAXL818/SaaS.git`
2. **补齐资源**：Epic Games Launcher → 库 → 找到对应资源包 → *添加到工程* → 选 `SaaS.uproject`；`AuraAssets` 直接拷贝到 `Content/AuraAssets/`
3. 生成工程文件：右键 `SaaS.uproject` → *Generate Visual Studio project files*
4. 编译 C++（VS / Rider 里直接 Build，或命令行）：
   ```
   "<UE安装目录>\Engine\Build\BatchFiles\Build.bat" SaaSEditor Win64 Development -project="<路径>\SaaS.uproject" -waitmutex
   ```
5. 打开 `SaaS.uproject`（默认关卡 `Content/Aura/Maps/StartupMap`）

> 没装 `Plugins/UnrealAgentLink`（AI/MCP 调试插件，114 MB）时 UE 可能提示找不到该插件 —— 它在 `.uproject` 里本来就是 `Enabled: false`，忽略提示或在 `.uproject` 里删掉该条目即可。

## 需要重新安装的资源清单

| 资源 | 安装路径 | 体积 | 来源 |
|---|---|---|---|
| AuraAssets（角色 / 敌人 / 特效 / 拾取物 / UI / 音效 / 字体） | `Content/AuraAssets/` | 781 MB | 教程配套素材（待补链接） |
| SD_Art Industrial_Infrastructure | `Content/Fab/SD_Art/` | 3.4 GB | Fab（待补链接） |
| Fishermans_Cabin | `Content/Fab/Fishermans_Cabin/` | 1.9 GB | Fab（待补链接） |
| SciFiWorld | `Content/Fab/SciFiWorld/` | 770 MB | Fab（待补链接） |
| 企鹅 / 机器猫 / 钢铁侠 | `Content/Fab/企鹅`、`Content/Fab/机器猫`、`Content/Fab/钢铁侠` | 97 / 57 / 48 MB | Fab（待补链接） |
| Wooden_Door / RPGEnvironmentVFX | `Content/Fab/Wooden_Door`、`Content/Fab/RPGEnvironmentVFX` | 62 / 52 MB | Fab（待补链接） |
| RealCitySF | `Content/Maps/RealCitySF/` | 1.7 GB | 商店（待补链接） |
| 长安大学建模地图（`MESH` 689 MB / `Textures` 14 MB / `Materials` 0.4 MB，共 950 个资产） | `Content/Maps/Changan/` + `Content/Maps/Map_Changan.umap` | 703 MB + 155 MB | 待补（若为自制，则需单独备份） |
| StonePineForest | `Content/Maps/StonePineForest/` | 676 MB | 商店（待补链接） |
| StarterContent / Characters | `Content/StarterContent/`、`Content/Characters/` | 194 / 125 MB | 引擎模板 / 教程 |
| UnrealAgentLink（AI/MCP 调试插件） | `Plugins/UnrealAgentLink/` | 114 MB | 插件发布页（待补链接） |

> ⚠️ **长安大学建模地图是这里最需要单独保管的一份东西**：`Map_Changan.umap` 单文件 155 MB，**超过 GitHub 单文件 100 MiB 的硬上限**，永远不会出现在仓库里；`Changan/MESH` 又有 689 MB。**如果这张图是你自己建模 / 采集的（不可再生），一定要用移动硬盘或网盘单独留快照** —— 这个仓库救不了它。反过来，若是从商店 / 他人处下载的，按原链接重新获取即可。

## 目录结构（仓库实际跟踪的内容）

```
SaaS.uproject                 # 项目描述（UE 5.8、模块 SaaS、插件列表）
.gitignore                    # 白名单式放行：Content/* 只跟踪自制目录
DEVLOG.md                     # 开发日志（每天一条）
Config/
  DefaultEngine.ini           # 默认关卡、渲染设置、类重定向
  DefaultGame.ini / DefaultEditor.ini / DefaultInput.ini
Source/
  SaaS/                       # 运行时模块（C++ 主线）
  SaaS.Target.cs / SaaSEditor.Target.cs
Content/
  Aura/                       # ★ 自制内容：Blueprint / Map / UI / Input
    Blueprints/Character/{Aura,Enemy}/     # BP_Aura、BP_EnemyBase、Goblin 系列 + 动画
    Blueprints/Effect/                     # BP_AuraEffectActor（药瓶）
    Blueprints/Game/                       # BP_AuraGameMode
    Blueprints/Player/                     # BP_AuraPlayerController / BP_AuraPlayerState
    Blueprints/UI/                         # BP_AuraHUD、WBP_Overlay、血球/蓝球进度条
    Maps/StartupMap.umap                   # 默认关卡
  Code/                       # 早期 C++ 练习的蓝图（MyActor / MyAutoDoor / …）
  Cursor/ Starfield/ TopDown/ # 光标、星空材质、TopDown 模板残留
```

## 代码导览（GAS 主线，截至 2026-10）

- `AAuraCharacterBase` → `AAuraCharacter`：玩家角色。`PossessedBy` / `OnRep_PlayerState` 时调用 `InitAbilityActorInfo()`，从 PlayerState 取 ASC 与 AttributeSet 赋给自身成员。
- `AAuraPlayerState`：持有 `UAuraAbilitySystemComponent` + `UAuraAttributeSet`（**玩家的 ASC 挂在 PlayerState 上，不在角色 Pawn 上**）。
- `AAuraEnemy`：敌人基类，实现 `IEnemyInterface`。
- `AAuraEffectActor`：药瓶 / 效果 Actor，`OnOverlap` 中取 ASC 并加属性。
- `AAuraGameModeBase` + `BP_AuraGameMode`：GameMode。
- `AAuraHUD`、`UAuraUserWidget`、`UAuraWidgetController`：HUD 与 UI 控制器（血 / 蓝球）。
- 早期练习：`MyActor`、`MyCharacter`、`MyTimelineActor`、`MySoftActor`、`MySmartPtrActor`、`MyStringActor`、`AutoDoor`、`MyBlueprintFunctionLibrary`。

## 关键配置在哪

| 项 | 位置 |
|---|---|
| 默认关卡 / 编辑器启动关卡 | `Config/DefaultEngine.ini` → `/Game/Aura/Maps/StartupMap` |
| GameMode | **关卡级 Override**：`StartupMap` 的 World Settings → `BP_AuraGameMode`（`Config` 中无 `GlobalDefaultGameMode`） |
| 输入映射 | `Content/Aura/Blueprints/Character/Aura/Input/`（`IMC_Aura`、`IA_Aura_Move`、`IA_Aura_Look`）+ `Config/DefaultInput.ini` |
| 项目 / 模块定义 | `SaaS.uproject`、`Source/SaaS/SaaS.Build.cs` |

## 日常开发流程

```powershell
git add -A
git commit -m "Aura：xxx"
git push
```

- 同时在 `DEVLOG.md` 末尾追加一条（字段：今天做了什么 / 遇到的问题·怎么解决的 / 学到的知识点 / 明天要做）。
- **新增自制内容目录**（例如 `Content/MyStuff`）时，要在 `.gitignore` 里加一行 `!Content/MyStuff/`，否则不会被跟踪。

## 常见问题

- **打开项目一片 Missing / 红色引用**：正常，说明对应资源没装。按上面的清单装回原路径即可。
- **药瓶碰到不加血**（历史问题，已修）：根因是 `BP_Aura` 的父类被错误设为 `AuraCharacterBase`，导致 `AAuraCharacterBase::AbilitySystemComponent` 永远为 `nullptr`；改回 `AuraCharacter` 后 `InitAbilityActorInfo()` 才会赋值。
- **`showdebug abilitysystem` 看不到属性面板**：UE 5.8 中 `AAbilitySystemDebugHUD::bEnableBasicHUD` 默认为 `false`，可用控制台命令 `AbilitySystem.Debug.ToggleBasicHUD` 打开。
- **改了 C++ 但行为没变**：确认编译产物是最新的（重新编译 `SaaSEditor`），PIE 中运行的是 DLL 而非源码。
