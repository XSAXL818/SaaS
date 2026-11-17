# 项目笔记（Docs）

> **定位：主题线笔记。** 与 [DEVLOG.md](../DEVLOG.md) 的分工如下：
>
> - `DEVLOG.md`：**时间线**，记「今天我做了什么、踩了什么坑」，只追加、不回头改；
> - `Docs/`：**按知识点组织**，同一主题反复修订，用来查。
>
> 写笔记的触发条件：**同一件事你查了第二次，就该写进 Docs**。第一次记进 DEVLOG 就够。

## 索引

新增笔记必须在下表登记一行，否则等于没写。索引只写「一句话结论」，不写摘要。

| 主题 | 文件 | 一句话结论 | 最后更新 |
|---|---|---|---|
| FPS 线总览 | [fps/README.md](fps/README.md) | 纯蓝图 FPS 练习，25 次提交、50 个资产，按输入/战斗/伤害/丧尸 AI 四条线拆分 | 2026-11-03 |
| FPS 输入与移动 | [fps/fps-01-input-locomotion.md](fps/fps-01-input-locomotion.md) | 枚举驱动的移动状态机，非 GAS；`IA_*` 默认累加行为会导致 AD/WS 同时按不静止 | 2026-11-03 |
| FPS 武器与开火 | [fps/fps-02-weapon-combat.md](fps/fps-02-weapon-combat.md) | 演进顺序是「动作+音效 → 视觉反馈 → 数值与 UI」，换弹 bug 成因未记录 | 2026-11-03 |
| FPS 伤害与死亡 | [fps/fps-03-damage-health-death.md](fps/fps-03-damage-health-death.md) | 伤害走独立 DamageType 资产 + 蓝图接口；与 Aura 线 GAS 回血实现完全不同，不可互相引用 | 2026-11-03 |
| FPS 丧尸 AI | [fps/fps-04-zombie-ai.md](fps/fps-04-zombie-ai.md) | 用状态树而非行为树；部位物理材质 → 命中音效映射实现爆头反馈 | 2026-11-03 |

## 目录规划

目前建了 `inbox/` 和 `fps/`。**其余目录等真有归属时再建**——git 不跟踪空目录，一上来铺一堆空文件夹只会制造噪声。

> **按项目分目录的硬规则**：一条学习线一个子目录（`fps/`、`gas/`、`editor/` …），同一主题内的笔记互相引用，**跨项目不引用结论**。原因：FPS 线是纯蓝图实现，Aura 线是 GAS 实现，同一个词（比如"回血"）在两边的机制完全不同，混写会误导。

| 目录 | 放什么 |
|---|---|
| `inbox/` | 临时、未分类。每周清一次，清空后可删 |
| `ue-core/` | 反射与宏（`UCLASS` / `UPROPERTY` / `GENERATED_BODY`）、UObject 生命周期、GC、模块与构建（`.Build.cs` / `.Target.cs`） |
| `gas/` | ASC、AttributeSet、GameplayEffect、GameplayAbility、GameplayCue、网络复制与预测 |
| `cpp/` | C++ 规范与坑：指针与引用、接口、序列化、前向声明、头文件依赖 |
| `editor/` | 资产迁移与重定向、蓝图、数据资产、插件启用、输入系统 |
| `debug/` | 崩溃排查、日志与 verbosity、性能分析（Insights）、Live Coding |
| `ref/` | 官方文档逐字摘录，必须附原始 URL 与页面内自查关键词 |

**文件命名**：`主题-子主题.md`，英文小写 + 连字符，例如 `gas-asc-init-order.md`。

- 不用中文文件名（`grep`、`Ctrl+Shift+N`、`@` 引用、AI 读取都不会碰上编码问题）；
- 不加日期前缀（改期就得重命名，引用会断）——日期写在文末。

## 笔记模板

新建文件时复制下面骨架：

```markdown
# 一句话结论

## 结论
（能直接照着做/照着判断的话，不要写"一般来说"）

## 依据
（必须可自查，至少一条）
- 引擎源码：`Engine/Source/.../Xxx.cpp:123`
- 官方文档：<URL> ＋ 页面内搜索关键词
- 课程/书籍：章节号

## 反例与坑
（错的做法长什么样，为什么错，症状是什么）

## 怎么自己验证
（可复现步骤，别人照着能得出同样结论）

---
最后更新：YYYY-MM-DD
```

## 三条硬规则

1. **不放 `Content/`**：UE 会把那里当资产目录扫描，而且 `.gitignore` 是按白名单放行 `Content/` 的，笔记混进去会让版本控制范围变模糊。
2. **没有「依据」的笔记不写**：宁可留一句 `TODO: 待查官方文档`，也不要凭记忆下结论。
3. **一层封顶**：`Docs/gas/xxx.md` 到此为止，不再往下分层——层级一深，找笔记的成本会超过重新查资料。

## 与 AI 的分工

| 位置 | 用途 | 谁维护 |
|---|---|---|
| `Docs/` | 给人看的笔记 | 人 |
| `.codebuddy/rules/` | 希望 AI 每次都遵守的强制约定 | 人 |
| `.codebuddy/memory/` | 项目背景上下文 | AI 自动，别手写 |
| `DEVLOG.md` | 每日进度与踩坑（已存在） | 人，追加 |

`Docs/` 里的内容 AI 不会主动读；要让它生效，把约定改写进 `.codebuddy/rules/`。
