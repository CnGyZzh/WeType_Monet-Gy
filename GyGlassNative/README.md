# GyGlass v31 — no-APK native development branch

This is a native C++ engineering checkpoint, NOT a ready-to-flash glass renderer.

- No APK, no desktop app, no activation.
- 2 presets only: seamless (0% key fill) and independent (about 16% key fill).
- Theme color is a Monet input to the C++ palette rather than hardcoded purple/blue.
- Dedicated Zygisk entry activates only in com.tencent.wetype processes.
- Real ART hooks and properly bounded IME blur are NOT implemented yet.
- In particular, never call Window.setBackgroundBlurRadius on the entire IME window:
  on the tested HyperOS build it blurs chat content above the keyboard.
- Avoid background polling; update on keyboard show/configuration change only.

CI compiles the arm64 library and verifies there is no APK; successful CI is
not proof of actual keyboard rendering. Do not flash an incomplete binary.

## Compatibility strategy (31.1)
- Current host reference: WeType 4.0.0.58111. Previous host reference: 4.0.0.58107.
- LSPosed v2.2.1 (7925), Android 9–17, is a reference environment, NOT a dependency bundled with GyGlass.
- `module/diagnose.sh` queries the installed WeType version ON DEMAND; never polls.
- Recognized builds mean identified version only, NOT working glass hooks.
- New 4.0.0.x builds are classified `NEW_BUILD_NEEDS_SIGNATURE_CHECK`. Different host
  families are `UNRECOGNIZED_HOST_VERSION`; no blind offsets are applied.
- A future rendering hook must resolve actual classes/resources at runtime and fail
  closed when signatures differ. Unknown builds cannot be guaranteed compatible.
- WebUI reports `RENDERER=NOT_IMPLEMENTED` until an actual native ART hook and
  keyboard-bounded glass renderer is independently tested.
- Privacy: diagnostic output is version, Android API, profile, and hook readiness only;
  no text, clipboard, chats, or keyboard contents are collected.

## Host APK reverse mapping (v31.3)
- The user supplied the official WeType APK as a *local reference only*; its binary is not stored in this repository or module.
- AndroidManifest.xml: `com.tencent.wetype`, `versionName=4.0.0`, `versionCode=57206`.
- The app's About page independently displays `4.0.0.58111`. Do not assume it equals Android's `versionName`.
- `hostmaps/wetype_4_0_0_57206.json` records layout/resource attributes and
  DEX method signatures with a SHA-256 identifying the analyzed binary.
- Regular QWERTY letters: `ime_color_btn_white_bg`, `ime_color_btn_white_border`.
- Shift, delete, number, emoji and enter: `ime_color_btn_grey_bg`,
  `ime_color_btn_grey_border`.
- The spacebar also uses `ime_color_btn_white_bg`. A global override of all
  "white" buttons could make the spacebar unexpectedly disappear, so rendering
  must classify key role before applying style.
- Verified self-drawn `KeyboardView.kt`: `...keyboard.selfdraw.o`,
  `onDraw(Canvas)`, `getColorMap()`, `getButtonDrawer()`.
- Verified `ButtonDrawer.kt`: `...keyboard.selfdraw.e`, method `d(Canvas)`.
- The previous speculative `...hld.s/o` field signatures were **removed**.
  `host_signature.hpp` now represents actual DEX method contracts.
- Runtime matching and hooking those methods remains unfinished; class names may
  change in subsequent obfuscated builds, and matching metadata is not proof of
  functional rendering.
- The new diagnostics identify the real manifest buildCode and explicitly state
  `RENDERER=NOT_IMPLEMENTED` until hooks are verified.
- Per-chat wallpaper appearance cannot be derived from Monet alone; it requires
  a correctly composited transparent IME window and *keyboard-bounded* blur.
- No clipboard/text/wallpaper images are read during these compatibility checks.

Do not package as a user-ready KernelSU ZIP until the scoped renderer is functional.
