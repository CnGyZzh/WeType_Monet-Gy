# Gy 玻璃键盘 · v29 独立 Hook 开发测试

这是独立实现的微信输入法玻璃键盘实验项目。**不包含 WeTyper APK、原始 DEX、授权校验或激活界面。** Android 应用仅提供 LSPosed Hook 入口，没有桌面 Activity；设置均通过 KernelSU WebUI 操作。

## 构建

GitHub Actions 使用 JDK 17、Gradle 8.7、AGP 8.5.2、Android SDK 35。执行 `gradle :app:assembleDebug`，再将生成的 `app-debug.apk` 放入 `ksu/gyhook.apk`，最后运行 `python3 package.py`。

## 使用与限制

KernelSU 安装 ZIP 后，模块在开机时通过 `pm install` 注册第一方 Hook APK。首次在 LSPosed 中启用 `com.gy.wetype.hook` 并将作用域限制为 `com.tencent.wetype`。旧的 WeTyper / GyGlass Hook 应停用，防止重复 Hook。

默认一体式，WebUI 可切换独立玻璃键帽。采用键盘显示时按需更新 Monet 颜色的方案，没有常驻颜色轮询、按键触发 Root 或逐帧软件模糊。

目前仅实现键帽填充、功能键、描边、阴影及基础窗口背景；工具栏和剪贴板白色卡片尚未独立适配。Android Window Blur 是否生效由 ROM 及微信输入法绘制方式决定，**不保证与 v20.1 完全相同的液态玻璃效果**。

这是开发测试版本。请先备份 KernelSU 模块配置，避免在无法恢复的环境中直接使用。
