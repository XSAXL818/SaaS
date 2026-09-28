# 开发日志（DEVLOG）

> 用法：每天收工前在文件末尾追加一条，然后 `git commit` + `git push`。
> 这份是「人看得懂的记录」，commit 历史是「机器记录」，两者配合最省事。
>
> 模板：
> ```markdown
> ## YYYY-MM-DD
> - 今天做了什么：
> - 遇到的问题 / 怎么解决的：
> - 明天要做：
> ```

---

## 2026-09-28

- 今天做了什么：给项目建立 Git 版本控制。仓库只跟踪 `Source/`、`Config/`、`SaaS.uproject` 和自己做的 `Content/Aura` 等内容（合计约 6 MB），商店与教程资源全部排除，由「重新下载 + 定期整包快照」另行管理。
- 遇到的问题 / 怎么解决的：
  - GitHub 单文件硬上限 100 MiB、仓库建议 < 1 GB，而本地 `Content/Fab` 有 6.3 GB、`Map_Changan.umap` 单个 155 MB —— 决定大资产不进 Git。
  - 修好了药瓶不加血的问题：`BP_Aura` 的父类由 `AuraCharacterBase` 改回 `AuraCharacter`，`PossessedBy → InitAbilityActorInfo()` 才会把 PlayerState 上的 ASC 赋给角色。
- 明天要做：
