# 开发日志（DEVLOG）

> 用法：每天收工前在文件末尾追加一条，然后 `git commit` + `git push`。
> 这份是「人看得懂的记录」，commit 历史是「机器记录」，两者配合最省事。
>
> 模板：
>
> ```markdown
> ## YYYY-MM-DD（Day N 可省）
> - 今天做了什么：（跟到哪一节 / 写了哪些代码、资产）
> - 遇到的问题 / 怎么解决的：（症状 → 根因 → 怎么验证 → 修法）
> 
> 	---
> 	- 症状
> 	- 根因
> 	- 怎么验证
> 	- 修法
> 
> 	---
> 	- 症状
> 	- 根因
> 	- 怎么验证
> 	- 修法
> 
> 	--- 
> 
> - 学到的知识点：（结论 + 依据：引擎源码 文件:行号 / 官方链接 / 课程章节）
> - 明天要做：（一句话即可，如"下一节：XXX"；没想法就留空）
> ```
>
> 两条填写规则：
> 1. **没内容的项写"无"，不要为了凑格式硬编** —— 能天天写的模板才是好模板；
> 2. 「遇到的问题」和「学到的知识点」是重点：前者要**能照着复现**，后者要**能照着复核**（写清依据，别只写结论）。
>
> 极简版（忙的时候就用这三行，同样算完成）：
> ```markdown
> ## YYYY-MM-DD
> - 做了什么：
> - 学到：
> - 坑：
> ```

---

## 2026-09-28

- 今天做了什么：给项目建立 Git 版本控制。仓库只跟踪 `Source/`、`Config/`、`SaaS.uproject` 和自己做的 `Content/Aura` 等内容（合计约 6 MB），商店与教程资源全部排除，由「重新下载 + 定期整包快照」另行管理。
- 遇到的问题 / 怎么解决的：
  - GitHub 单文件硬上限 100 MiB、仓库建议 < 1 GB，而本地 `Content/Fab` 有 6.3 GB、`Map_Changan.umap` 单个 155 MB —— 决定大资产不进 Git。
  - 修好了药瓶不加血的问题：`BP_Aura` 的父类由 `AuraCharacterBase` 改回 `AuraCharacter`，`PossessedBy → InitAbilityActorInfo()` 才会把 PlayerState 上的 ASC 赋给角色。
- 明天要做：



## 2026-10-06
- 今天做了什么：

FPS：创建并配置IA和IMC，创建角色并完善手臂、枪身、弹夹、枪托等插槽绑定，实现了移动、视角功能。

- 遇到的问题 / 怎么解决的：

  ---

  - 症状：已有的项目资产目录不能直接复制粘贴到其他项目，除非保持原有的目录结果。A项目有Content/Asset1，只能将Asset1放到B项目的Content下，否则会导致相关资产依赖引用失效。

  - 根因：UE引擎会对每个资产的管理可以理解为以Content开始的绝对路径，即如果Content/Asset1/bp1用到了Content/Asset2/bp2，那直接复制粘贴到新项目后也必须保持Content/Asset1/bp1，Content/Asset2/bp2。不能改变，即原先以为复制到Content/FPS/Asset1/bp1，Content/FPS/Asset2/bp2，这中相对路径也可以，实际不行，必须以Content开始。

  - 修法：UE编辑器选中所有迁移的资产，右键点击迁移功能。

  ---

  - 症状：设置移动的InputAction，默认参数导致同时按住AD或者WS不能静止。

  - 根因：源码
    ```cpp
    switch (AccumulationBehavior)
    {
    // Sometimes you may want to cumulatively merge input. This would allow you to, for example, map WASD to movement and have pressing W and S at the same time
    // completely cancel out input because "W" is a value of +1.0, and "S" is a value of -1.0
    case EInputActionAccumulationBehavior::Cumulative:
    {
      Merged[Component] += Modified[Component];
    }										
    break;

    // By default, we will accept the input with the highest absolute value
    case EInputActionAccumulationBehavior::TakeHighestAbsoluteValue:
    default:
    {
      if (FMath::Abs(Modified[Component]) >= FMath::Abs(Merged[Component]))
      {
        Merged[Component] = Modified[Component];
      }
    }
    break;
    }	
    ```
    IA的累加行为有两种方式：Cumulative累积和TakeHighestAbsoluteValue取最大值

    首先明确设置IA为按W时x=1，按S时X=-1。
  
  一：累积Cumulative，当同时按时，x = 1 + (-1)，即值相加的结果。

  一：取绝对值对最大，算法必定取其中的一个值，具体取值和IMC中设置的W、S按键顺序有关。

  - 修法：在IA处设置累加行为为Cumulative。
  
  --- 

  - 症状：第一人称摄像机离武器太近，导致枪的部分结构看不见，不是因为出现在摄像机之外了，而是被裁切了。

  - 根因：项目设置中的相机近剪切面设置默认为10，可以理解模型在相机10cm以内，都不会被看见。

  - 修法：项目设置搜索 near clip，设置为1即可。