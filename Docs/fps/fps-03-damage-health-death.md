# FPS 伤害、血量与死亡复活

> 范围：伤害类型资产、玩家受击、血量、呼吸回血、死亡界面与复活。丧尸侧的受击事件见 `fps-04`。

## 结论

这条线把「伤害」做成了**独立的伤害类型资产**（`Content/FPS/Code/DamgaeTpye`，5 个），而不是在武器里写死数值；受伤入口统一走**蓝图接口**（`Content/FPS/Code/BlueprintInterface`）。血量逻辑经历了三步：

1. **受击 → 死亡**（`8ab7758`）：先有血量和死亡判定；
2. **呼吸回血**（`7e7b9cd`）：从「不回血」到「脱战后自动恢复」；
3. **死亡界面 + 复活**（`5b94402`）：补齐死亡后的完整闭环。

**注意**：`DamgaeTpye` 目录名是拼写错误（应为 `DamageType`）。资产引用走完整包路径，改名需处理重定向器，**不建议动**。

## 已核实：呼吸回血用的是 Timer，不是 Timeline

**核实方式**：编辑器连着 RiderLink，用 UE MCP 的 `BlueprintTools.list_variables` 读取 `BP_FPS_Player` 的成员变量（2026-11-03）。

**核实结果**：变量表里存在 **`HealHeathHandle`**——这是一个 **Timer Handle**（定时器句柄），说明「呼吸回血」由**定时器周期触发**，而非 Timeline 曲线，也不是状态树。

同一张变量表还解释了这条线的其他三个现象：

| 变量 | 说明 |
|---|---|
| `Health` / `MaxHealth` | 血量与上限（**不是** GAS 的 AttributeSet） |
| `HealHeathHandle` | 回血定时器句柄（拼写错误：应为 `HealHealthHandle`） |
| `IsDead` | 死亡状态开关 |
| `受击画面渲染不透明度` | 受击时的屏幕视觉反馈（变量名直接用了中文） |
| `RunToWalkTimerHandle` | Run→Walk 过渡的定时器——这正是 `074fcfe` 修的那个「过渡太慢」问题的落点 |
| `UpdateFOVTimer` + `AimFOV`/`DefaultFOV`/`TargetFOV` | 瞄准时的 FOV 过渡由定时器插值 |
| `MouseSensitivity` | 灵敏度（`1be23a5`） |
| `E_PlayerState` / `E_AimState` | 两个状态枚举，对应 `Code/Player/System/Enum` 下的 3 个枚举资产 |

**未核实**：定时器的**间隔**与**每次回复量**藏在 `Set Timer by Event/Function` 节点的引脚默认值里，需要进一步读节点（见文末待补）。

## 依据

**提交记录**

| commit | 内容 |
|---|---|
| `8ab7758` | 玩家受击和死亡 |
| `7e7b9cd` | 呼吸回血机制 |
| `5b94402` | 角色死亡界面和复活，核心功能已完成 |

**资产路径**

| 用途 | 说明 |
|---|---|
| `Content/FPS/Code/DamgaeTpye`（5 个） | 伤害类型资产 |
| `Content/FPS/Code/BlueprintInterface`（1 个） | 蓝图接口，推测为「受击 / 造成伤害」统一入口（**待核实**） |
| `Content/FPS/Code/UI`（1 个） | 游戏 UI（含死亡界面，`5b94402` 提交里改的是 `UI_Main.uasset`） |
| `Content/FPS/Code/Player` | 玩家角色（血量、回血逻辑所在） |

**`5b94402` 的实际改动**（`git show --stat` 原文）

```
Content/FPS/Code/Player/BP_FPS_Player.uasset | Bin 789990 -> 866420 bytes
Content/FPS/Code/UI/UI_Main.uasset           | Bin 81989 -> 122842 bytes
```

即：复活逻辑落在**玩家蓝图**，死亡界面落在 **`UI_Main`**，两者没有新增资产——说明是在既有蓝图里扩功能，而非新建一套系统。

## 反例与坑

**坑 1：`.uasset` 改动在 git 里不可读**

上面那两行 `Bin 789990 -> 866420 bytes` 就是全部信息。体积涨了 76 KB 但**没有任何逻辑可见**——这意味着：

- 回看历史时无法用 `git diff` 复盘「当时改了什么逻辑」；
- 只能靠 commit message 和 DEVLOG。**所以提交信息必须写到知识点级别**，否则这段历史等于丢失。

**坑 2：呼吸回血容易与「治疗道具」混淆**

Aura 线（GAS）里也有回血，但那是 `GameplayEffect` + `AttributeSet` 驱动的（见 `../gas/`）。FPS 线是纯蓝图按固定间隔加血。**两条线的回血实现完全不同，笔记不要互相引用结论。**

## 待核实

- [x] ~~呼吸回血的驱动方式~~ → **已核实：Timer（`HealHeathHandle`）**
- [ ] 定时器的间隔与每次回复量（需读 `Set Timer` 节点的引脚默认值）
- [ ] 「呼吸」的触发与中断条件：脱战多久开始、受击是否立即中断
- [ ] 死亡流程：是 `DisableInput` + 播动画，还是切 UI 独占了输入
- [ ] 复活方式：重载关卡（`Open Level`）还是原地重置状态
- [ ] 蓝图接口的签名（几个函数、参数是什么）

---
最后更新：2026-11-03
