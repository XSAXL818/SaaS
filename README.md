# SaaS

基于 **Unreal Engine 5.8** 的 GAS（Gameplay Ability System）学习项目：照 Aura 系列教程做的第三人称角色 + 属性 / 技能系统 + HUD（血球 / 蓝球），另附早期 C++ 练习代码（Actor 生命周期、Soft Reference、Smart Pointer、Timeline、接口、蓝图函数库等）。

---

## ⚠️ 先读这一段：仓库里**故意不放**美术资源

商店 / 教程素材体积太大（本地约 17 GB），既进不了 GitHub 的单文件限制，也不需要进 Git。

| | 内容 | 体积 |
|---|---|---|
| ✅ 在仓库里 | `Source/`（C++）、`Config/`、`SaaS.uproject`、自制蓝图 `Content/Aura`、`Content/Code`、`Content/Cursor`、`Content/Starfield`、`Content/TopDown` | 约 **6.4 MB** |
| ❌ 不在仓库里 | `Content/Fab`、`Content/AuraAssets`、`Content/Maps`、`Content/Characters`、`Content/StarterContent`、`Plugins/UnrealAgentLink` | 约 **17 GB** |

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
| Changan（长安场景） | `Content/Maps/Changan/`、`Content/Maps/Map_Changan.umap` | 703 MB + 155 MB | 商店（待补链接） |
| StonePineForest | `Content/Maps/StonePineForest/` | 676 MB | 商店（待补链接） |
| StarterContent / Characters | `Content/StarterContent/`、`Content/Characters/` | 194 / 125 MB | 引擎模板 / 教程 |
| UnrealAgentLink（AI/MCP 调试插件） | `Plugins/UnrealAgentLink/` | 114 MB | 插件发布页（待补链接） |

> `Map_Changan.umap` 单文件 155 MB，**超过 GitHub 单文件 100 MiB 的硬上限**，永远不会出现在仓库里 —— 如需保留只能靠本地快照（移动硬盘 / 网盘）。

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

## 代码导览（GAS 主线）

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
git commit -m "Day 6: 修好 XXX"
git push
```

- 同时在 `DEVLOG.md` 末尾追加一条：做了什么 / 遇到的问题 / 明天做什么。
- **新增自制内容目录**（例如 `Content/MyStuff`）时，要在 `.gitignore` 里加一行 `!Content/MyStuff/`，否则不会被跟踪。

## 常见问题

- **打开项目一片 Missing / 红色引用**：正常，说明对应资源没装。按上面的清单装回原路径即可。
- **药瓶碰到不加血**（历史问题，已修）：根因是 `BP_Aura` 的父类被错误设为 `AuraCharacterBase`，导致 `AAuraCharacterBase::AbilitySystemComponent` 永远为 `nullptr`；改回 `AuraCharacter` 后 `InitAbilityActorInfo()` 才会赋值。
- **`showdebug abilitysystem` 看不到属性面板**：UE 5.8 中 `AAbilitySystemDebugHUD::bEnableBasicHUD` 默认为 `false`，可用控制台命令 `AbilitySystem.Debug.ToggleBasicHUD` 打开。
- **改了 C++ 但行为没变**：确认编译产物是最新的（重新编译 `SaaSEditor`），PIE 中运行的是 DLL 而非源码。
