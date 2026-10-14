#!/system/bin/sh
# Install GyGlass Hook only; never uninstall or clear WeChat Input Method data.
D=${0%/*}
PKG=com.gy.wetype.hook
APK="$D/GyGlassHook.apk"
HASHFILE="$D/.installed-apk.sha256"
PREVIOUS="$D/.previous-gyglass-hook.apk"
[ -s "$APK" ] || { echo 'HOOK_ERROR=missing internal Hook'; exit 20; }
HASH=$(sha256sum "$APK" 2>/dev/null | awk '{print $1}')
[ -n "$HASH" ] || { echo 'HOOK_ERROR=checksum unavailable'; exit 22; }
if pm path "$PKG" >/dev/null 2>&1 && [ "$(cat "$HASHFILE" 2>/dev/null)" = "$HASH" ]; then
  echo 'HOOK_INSTALL=unchanged'; exit 0
fi
OUT=$(pm install -r --user 0 "$APK" 2>&1)
RC=$?
if [ "$RC" -ne 0 ] || ! printf '%s' "$OUT" | grep -q 'Success'; then
  case "$OUT" in
    *INSTALL_FAILED_UPDATE_INCOMPATIBLE*|*signatures\ do\ not\ match*)
      OLD=$(pm path "$PKG" 2>/dev/null | sed -n 's/^package://p' | head -n 1)
      if [ -z "$OLD" ] || [ ! -r "$OLD" ]; then
        echo 'HOOK_ERROR=previous GyGlass Hook APK is unavailable'; exit 23
      fi
      cp "$OLD" "$PREVIOUS" || { echo 'HOOK_ERROR=failed to back up previous Hook';exit 24; }
      chmod 0600 "$PREVIOUS" 2>/dev/null || :
      echo 'HOOK_INSTALL=GyGlass signing certificate migration'
      pm uninstall --user 0 "$PKG" >/dev/null 2>&1 || { echo 'HOOK_ERROR=GyGlass replacement denied'; exit 25; }
      OUT=$(pm install -r --user 0 "$APK" 2>&1)
      RC=$?
      if [ "$RC" -ne 0 ] || ! printf '%s' "$OUT" | grep -q 'Success'; then
        echo 'HOOK_INSTALL=new Hook failed; attempting rollback'
        pm install -r --user 0 "$PREVIOUS" >/dev/null 2>&1 || :
      fi
      echo 'HOOK_SCOPE=verify LSPosed scope after update'
      ;;
  esac
fi
if [ "$RC" -eq 0 ] && printf '%s' "$OUT" | grep -q 'Success'; then
  printf '%s\n' "$HASH" > "$HASHFILE"
  rm -f /data/user/0/com.tencent.wetype/files/gyglass_runtime.txt 2>/dev/null || :
  echo 'HOOK_INSTALL=success'
  exit 0
fi
echo "HOOK_ERROR=$OUT"
exit 21
