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
