#!/system/bin/sh
# On-demand diagnostics only. Does not modify the target keyboard or enable render hooks.
D=${0%/*}
PKG=com.tencent.wetype
MODE=seamless
if [ -s "$D/preset.conf" ]; then
  saved=$(sed -n 's/^MODE=\(seamless\|independent\)$/\1/p' "$D/preset.conf" 2>/dev/null | head -n 1)
  [ -n "$saved" ] && MODE="$saved"
fi
printf 'MODE=%s\n' "$MODE"
SDK=$(getprop ro.build.version.sdk 2>/dev/null)
case "$SDK" in ''|*[!0-9]*) SDK=unknown;; esac
printf 'ANDROID_SDK=%s\n' "$SDK"
if pm path "$PKG" >/dev/null 2>&1; then
  PACKAGE_INFO=$(dumpsys package "$PKG" 2>/dev/null)
  VERSION=$(printf '%s\n' "$PACKAGE_INFO" | sed -n 's/^[[:space:]]*versionName=\([^[:space:]]*\).*$/\1/p' | head -n 1)
  BUILD_CODE=$(printf '%s\n' "$PACKAGE_INFO" | sed -n 's/^[[:space:]]*versionCode=\([0-9][0-9]*\).*$/\1/p' | head -n 1)
  case "$VERSION:$BUILD_CODE" in
    4.0.0:57206) PROFILE=4.0.0; COMPAT=KNOWN_PACKAGE_NOT_RENDER_TESTED;;
    4.0.0:*) PROFILE=4.0.0; COMPAT=NEW_BUILD_NEEDS_SIGNATURE_CHECK;;
    :*) PROFILE=unknown; COMPAT=VERSION_UNAVAILABLE;;
    *) PROFILE=unknown; COMPAT=UNRECOGNIZED_HOST_VERSION;;
  esac
  [ -n "$VERSION" ] || VERSION=unknown
  [ -n "$BUILD_CODE" ] || BUILD_CODE=unknown
  printf 'INPUT_VERSION=%s\nBUILD_CODE=%s\nPROFILE=%s\nCOMPAT=%s\n' "$VERSION" "$BUILD_CODE" "$PROFILE" "$COMPAT"
else
  printf 'INPUT_VERSION=not_installed\nPROFILE=none\nCOMPAT=INPUT_METHOD_NOT_INSTALLED\n'
fi
if [ -s "$D/zygisk/arm64-v8a.so" ]; then
  echo 'NATIVE_BINARY=present'
else
  echo 'NATIVE_BINARY=missing'
fi
echo 'RENDERER=NOT_IMPLEMENTED'
echo 'ACTIVE_HOOK=NOT_VERIFIED'
