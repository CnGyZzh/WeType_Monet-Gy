<div align="center">

# 微信输入法 Monet

为**安卓版微信输入法**提供 Material You / Monet 动态配色的
Magisk / KernelSU Overlay 模块，以及保持原包名的独立 Monet 安装包。

[![GitHub release](https://img.shields.io/github/v/release/CnGyZzh/WeType_Monet-Gy?style=flat-square&label=Release&color=34C759)](https://github.com/CnGyZzh/WeType_Monet-Gy/releases)
[![License](https://img.shields.io/badge/License-GPL--3.0-blue.svg?style=flat-square)](LICENSE)
[![Android](https://img.shields.io/badge/Android-14%2B-3DDC84?style=flat-square&logo=android&logoColor=white)](https://developer.android.com/about/versions/14)
[![Magisk](https://img.shields.io/badge/Magisk-%E2%9C%94-00B4D8?style=flat-square)](https://github.com/topjohnwu/Magisk)
[![KernelSU](https://img.shields.io/badge/KernelSU-%E2%9C%94-7C4DFF?style=flat-square)](https://github.com/tiann/KernelSU)
[![CI](https://img.shields.io/github/actions/workflow/status/CnGyZzh/WeType_Monet-Gy/auto-update.yml?style=flat-square&label=Build)](https://github.com/CnGyZzh/WeType_Monet-Gy/actions)

</div>

> 让微信输入法跟随系统壁纸自动取色，呈现原生 Android 14+ 的 Material You 动态配色。

> ⚠️ **个人自用分支**：本仓库为 [@0x1e93d/WeType_Monet](https://github.com/0x1e93d/WeType_Monet) 的 fork，**仅适配微信输入法测试版**，随缘更新、不作长期维护。感谢原作者 [@0x1e93d](https://github.com/0x1e93d) 的开源工作！

## ✨ 特性

- 🎨 **Monet 动态色彩** — 键盘、候选栏、工具栏、菜单等界面全部跟随系统 Material You 配色，深浅色自动切换
- 🧩 **两种安装方式** — Magisk / KernelSU Overlay 模块（免改包名、可回滚），或独立 Monet APK（免 Root）
- ⚡ **KernelSU 免重启热更新** — 首次安装后，后续更新热安装即可生效，无需重启设备
- 🧪 **适配微信输入法测试版** — 针对测试版 APK 适配，随缘更新
- 👥 **多用户支持** — KernelSU 安装器可检测多用户环境，按需为其他用户安装
- 🛡️ **可随时回滚** — Overlay 模块卸载即恢复官方原样

## 📱 安装

### Magisk / KernelSU Overlay 模块

> 需要 Android 14+，以及已启用的 Magisk 或 KernelSU。模块依赖已安装的官方微信输入法。

1. 在 [Releases](https://github.com/CnGyZzh/WeType_Monet-Gy/releases) 下载最新的 `Wetype_Monet_vN.zip`
2. 用 Magisk 或 KernelSU 安装该 ZIP
3. 重启设备后使用微信输入法

**KernelSU 动态功能：**

- **首次安装需重启** — 首次以静态方式挂载 Overlay；完成一次启动后模块会记录状态
- **后续更新免重启** — 新版本会热安装 Overlay APK 并停止微信输入法进程，重新打开输入法即生效
- **多用户安装** — 检测到其他 Android 用户时可按音量加选择为其安装，并自动启用 Overlay

> 多用户环境中，每个需要使用主题的用户都应已安装官方微信输入法；选择「不安装」时其他用户不会自动获得 Overlay。

### 独立 Monet APK

> 保留官方包名 `com.tencent.wetype`，但使用本项目公开发布签名，**不能与官方微信输入法共存**。

1. 首次安装前，先备份需要保留的输入法数据
2. 卸载当前官方微信输入法
3. 安装 Release 中对应版本的 Monet APK
4. 后续更新直接安装新版本；必须持续使用本项目发布的同一签名版本

> 如需回到官方版本：卸载 Monet APK → 安装同一 Release 提供的官方原始 APK，或从[微信输入法官网](https://z.weixin.qq.com/)下载。

## 📦 发布产物

| 产物 | 说明 |
| --- | --- |
| `Wetype_Monet_vN.zip` | 用于 Magisk / KernelSU 的 Overlay 模块 |
| `Wetype_Monet_<版本>_vN.apk` | 写入 Monet 资源、保持原包名的独立安装包 |
| `Wetype_<版本>.apk` | 构建时下载归档的官方微信输入法原始安装包 |
| `wetype_monet.json` | KernelSU / Magisk 在线更新清单 |
| `CHANGELOG.md` | KernelSU 在线更新界面展示的更新日志 |

> 本分支 Release 目前仅提供 Overlay 模块 ZIP；独立 Monet APK 与官方 APK 归档见上游仓库。

## 🔧 构建与更新

本分支**随缘更新**：仅在微信输入法测试版需要适配时手动触发构建（GitHub Actions `Manual force update` 工作流），不提供定时自动构建。构建流程沿用上游 [@0x1e93d](https://github.com/0x1e93d) 的自动化脚本。

## ❓ 常见问题

<details>
<summary><b>为什么需要 Android 14+？</b></summary>

Monet 动态取色依赖 Android 12+ 的 Material You，而本项目的 Overlay 目标与系统资源引用基于 Android 14 的接口设计，因此要求 Android 14 及以上。
</details>

<details>
<summary><b>微信输入法更新后主题失效了怎么办？</b></summary>

等待本分支 Release 适配新版本后更新即可；如测试版变化较大，作者会随缘更新适配。
</details>

<details>
<summary><b>独立 APK 和 Overlay 模块该选哪个？</b></summary>

- 已 Root（Magisk / KernelSU）→ 优先用 **Overlay 模块**，免改包名、易回滚
- 未 Root → 用 **独立 Monet APK**，但需卸载官方版且不能共存
</details>

## 📄 许可证

本项目基于上游 [@0x1e93d/WeType_Monet](https://github.com/0x1e93d/WeType_Monet) 修改。

[GPL-3.0](LICENSE) © [0x1e93d](https://github.com/0x1e93d)
