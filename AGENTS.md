# SaaS 项目开发约定

## FPSGame_AI 目录规则

- **总规则**：以后所有新建内容（蓝图、地图、UI、动画、数据资产、材质、C++ 类、测试/临时 Actor 等）一律创建在 FPSGame_AI 目录体系内，不得新建到其他目录；只有用户明确要求时才例外。
- **蓝图与资产**：所有 FPSGame_AI 相关的蓝图、地图、UI、动画、数据资产、材质等，统一放在 `Content/FPSGame_AI/` 下。
- **C++ 代码**：所有 FPSGame_AI 相关 C++ 代码，统一放在 `Source/SaaS/FPSGame_AI/` 下，不要散落到 `Source/SaaS/Public` 或 `Source/SaaS/Private` 的其他目录。
  - 头文件：`Source/SaaS/FPSGame_AI/Public/FPSGame_AI/<子目录>/<类名>.h`
  - 源文件：`Source/SaaS/FPSGame_AI/Private/FPSGame_AI/<子目录>/<类名>.cpp`
  - 头文件引用：`#include "FPSGame_AI/<子目录>/<类名>.h"`
- 新增 FPSGame_AI 功能时，蓝图资产路径与 C++ 类名保持语义一致，便于在编辑器里对应查找。
- 已有 `Content/FPS/`、`Content/Aura/` 等目录属于历史内容，不做无关联迁移；只有明确要求时才移动。

## 通用

- 修改保持最小范围，不顺手重构与当前任务无关的代码或资产。
- C++ 使用现有代码风格（UE 命名、Tab 缩进、UCLASS/UPROPERTY 规范）。
