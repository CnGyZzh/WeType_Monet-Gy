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
