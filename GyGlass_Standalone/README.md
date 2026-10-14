# GyGlass v30

微信输入法玻璃键盘主题。提供系统 Monet 动态配色、低透明度底板和两套 WebUI 预设：一体式与独立半透明按键，默认一体式。

## 安装和切换

直接在 KernelSU 刷入 GyGlass_v30_KSU.zip；该 ZIP 自动注册内部 Hook 组件，无需额外下载 APK。内部组件无桌面图标和激活页面。重启后在 LSPosed 中启用 GyGlass（包名 com.gy.glass.engine），作用域仅选择微信输入法 com.tencent.wetype。首次启用后可进入 KernelSU WebUI 切换预设，切换时只停止一次输入法进程。

## 性能与兼容

不启用会模糊整个微信聊天窗口的 Window Blur。输入法启动或显示时更新系统 Monet 颜色，无后台定时轮询，不在按键事件中启动 Root 子进程。窗口与键盘内部自绘资源是否透明，仍取决于当前微信输入法与 HyperOS 的绘制实现。

本项目目前为开发测试版。已编译不代表已通过真机适配测试。WebUI 的“配置已保存”不代表 Hook 一定已加载，也不代表玻璃质感达到设计参考图。

## 构建

JDK 17、Gradle 8.7、AGP 8.5.2、Android SDK 35。运行 `gradle :app:assembleDebug`，将 APK 放置到 `ksu/gyhook.apk`，然后运行 `python3 package.py`。

本模块由 Gy 开发。