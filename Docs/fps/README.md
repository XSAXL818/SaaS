# FPS 学习线（Content/FPS）

> **边界声明**：本目录只写 **FPS 线**（纯蓝图实现的 FPS 练习）。GAS/Aura 线的内容一律不放这里，见 `../gas/`；引擎通用机制见 `../ue-core/`。
>
> 这条线**全部是蓝图资产，没有 C++**，因此依据以 **commit + 资产路径** 为主，引擎行为类结论一律标「待核实」并写明验证方式。

## 一、范围与资产地图

资产根目录：`Content/FPS/`（共 50 个 `.uasset/.umap`，19 个子目录）

| 目录 | 数量 | 内容 |
|---|---|---|
| `Code/Player/` | 2 | 玩家角色蓝图、玩家控制器 |
| `Code/Player/Animation/Player` | 2 | 玩家动画蓝图 |
| `Code/Player/Animation/Weapon` | 2 | 武器动画 |
| `Code/Player/System/` | 1 | 玩家系统蓝图 |
| `Code/Player/System/Input` | 7 | `IA_*` 输入动作 + `IMC_*` 映射上下文 |
| `Code/Player/System/Enum` | 3 | 移动/瞄准等状态枚举 |
| `Code/Weapon/` | 2 | 武器主蓝图 |
| `Code/Weapon/Base` | 1 | 武器基类 |
| `Code/Weapon/MetaSound` | 1 | 开火音效（MetaSound） |
| `Code/Zombie` | 6 | 丧尸角色、AI 相关 |
| `Code/Physics` | 10 | 物理材质（头/手/脚等部位） |
| `Code/DamgaeTpye` | 5 | **伤害类型**（注意目录名拼写错误：`DamgaeTpye`，正确应为 `DamageType`） |
| `Code/AnimNotify` | 1 | 动画通知 |
| `Code/BlueprintInterface` | 1 | 蓝图接口 |
| `Code/UI` | 1 | 游戏 UI |
| `Maps` | 2 | 测试关卡（含 `Default.umap`） |

**已知历史包袱**：`Code/DamgaeTpye` 是拼错的目录名。目录名不影响运行（资产引用走完整包路径），改名需要走「重定向器」流程，成本大于收益——**记录在此，不轻易动**。

## 二、功能时间线（按 commit 倒序，共 25 次提交）

| commit | 内容 | 归属笔记 |
|---|---|---|
| `5b94402` | 角色死亡界面和复活，核心功能已完成 | [fps-03-damage-health-death](fps-03-damage-health-death.md) |
| `7e7b9cd` | 呼吸回血机制 | [fps-03-damage-health-death](fps-03-damage-health-death.md) |
| `8ab7758` | 玩家受击和死亡 | [fps-03-damage-health-death](fps-03-damage-health-death.md) |
| `dded900` | 完成丧尸受击事件 | [fps-04-zombie-ai](fps-04-zombie-ai.md) |
| `b57cfe8` | 丧尸设置头/手/脚的物理材质，实现击中头部的特殊音效 | [fps-04-zombie-ai](fps-04-zombie-ai.md) |
| `3e29c97` | 状态树实现 AI 丧尸攻击 | [fps-04-zombie-ai](fps-04-zombie-ai.md) |
| `6b25510` | 添加枪械 UI 元素 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `912564b` | 修复换弹 bug | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `8b1bb41` | 优化开火，角色后退行走时上移弹道 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `98cf9e1` | 完成换弹系统 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `b23505e` | 添加弹孔痕迹 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `89a60c3` | 添加击中特效和音效 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `879cb34` | 添加后坐力效果 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `3deb846` | 优化开火逻辑 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `074fcfe` | 修复 run 切 walk 时立刻开火的动画过渡太慢 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `310a97a` | 添加开火时生成弹壳 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `8f649b5` | 添加开火时枪口火焰特效 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `b85549f` | 优化开火逻辑，使用 MetaSound 实现音效 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `70a1572` | 完成开火蒙太奇动画 | [fps-02-weapon-combat](fps-02-weapon-combat.md) |
| `1be23a5` | 完善灵敏度系统 | [fps-01-input-locomotion](fps-01-input-locomotion.md) |
| `580b7ef` | 完善瞄准系统 | [fps-01-input-locomotion](fps-01-input-locomotion.md) |
| `a09b57c` | 转为三角洲的角色移动逻辑 | [fps-01-input-locomotion](fps-01-input-locomotion.md) |
| `6259352` | 添加角色、武器的 Walk/Run/Aim 动画 | [fps-01-input-locomotion](fps-01-input-locomotion.md) |
| `07c7f7d` | 完成角色移动状态切换 | [fps-01-input-locomotion](fps-01-input-locomotion.md) |
| `b3e3d67` | 添加 FPS 学习，制作角色基本能力 | [fps-01-input-locomotion](fps-01-input-locomotion.md) |

## 三、笔记目录

| 文件 | 覆盖内容 |
|---|---|
| [fps-01-input-locomotion.md](fps-01-input-locomotion.md) | 输入系统（IA/IMC）、角色与插槽、移动状态机、瞄准与灵敏度 |
| [fps-02-weapon-combat.md](fps-02-weapon-combat.md) | 武器、开火、蒙太奇、枪口火焰、弹壳、后坐力、弹道、弹孔、音效、换弹、枪械 UI |
| [fps-03-damage-health-death.md](fps-03-damage-health-death.md) | 伤害类型、玩家受击、血量、呼吸回血、死亡界面与复活 |
| [fps-04-zombie-ai.md](fps-04-zombie-ai.md) | 丧尸 AI（状态树）、部位物理材质、爆头音效、丧尸受击事件 |

## 四、待补清单

以下内容目前只有 commit 标题，缺少实现细节，需要用蓝图 MCP 读图补齐（`BlueprintTools.read_graph_dsl`）：

- [ ] 开火的完整判定链：射线由谁发射、命中判定用什么通道、后坐力如何影响弹道
- [ ] 换弹 bug 的具体成因（`912564b`）
- [ ] 呼吸回血的实现方式（Timeline / Timer / 状态树）与数值（`7e7b9cd`）
- [ ] 丧尸状态树的节点构成与切换条件（`3e29c97`）
- [ ] 物理材质 → 爆头音效的映射链路（`b57cfe8`）
