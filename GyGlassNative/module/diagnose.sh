#!/system/bin/sh
# On-demand diagnostics only. Does not modify the target keyboard or enable render hooks.
D=${0%/*}
PKG=com.tencent.wetype
MODE=seamless
if [ -s "$D/preset.conf" ]; then
  saved=$(sed -n 's/^MODE=\(seamless\|independent\)$/\1/p' "$D/preset.conf" 2>/dev/null | head -n 1)
  [ -n "$saved" ] && MODE="$saved"
fi
KEY_OPACITY=16
if [ -s "$D/preset.conf" ]; then
  saved=$(sed -n 's/^KEY_OPACITY=\([0-9][0-9]*\)$/\1/p' "$D/preset.conf" | head -n 1)
  case "$saved" in ''|*[!0-9]*) ;; *) if [ "$saved" -ge 1 ] && [ "$saved" -le 100 ];then KEY_OPACITY="$saved";fi;;esac
fi
printf 'MODE=%s\nKEY_OPACITY=%s\n' "$MODE" "$KEY_OPACITY"
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
echo 'RENDERER=EXPERIMENTAL_JNI_NOT_DEVICE_VERIFIED'
# Only read lines emitted by GyGlass; do not log keyboard text or other applications.
EVENT=$(logcat -d -s GyGlass:I 2>/dev/null | sed -n 's/.*keyboard_map_status=\(-\{0,1\}[0-9][0-9]*\).*/\1/p' | tail -n 1)
case "$EVENT" in
  4) echo 'ACTIVE_HOOK=COLOR_MAP_CHANGED_IN_LOG';;
  5) echo 'ACTIVE_HOOK=COLORS_ALREADY_MATCH_IN_LOG';;
  -1) echo 'ACTIVE_HOOK=KEYBOARD_VIEW_NOT_FOUND_IN_LOG';;
  1|2|3) echo "ACTIVE_HOOK=MAP_NOT_CHANGED_STATUS_$EVENT";;
  *) echo 'ACTIVE_HOOK=NO_RENDER_EVENT_LOG';;
esac
