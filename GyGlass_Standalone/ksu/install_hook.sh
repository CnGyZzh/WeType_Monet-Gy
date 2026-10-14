#!/system/bin/sh
D=${0%/*}
PKG=com.gy.wetype.hook
APK="$D/gyhook.apk"
[ -s "$APK" ] || { echo 'HOOK_ERROR=missing Gy hook APK'; exit 20; }
if pm path "$PKG" >/dev/null 2>&1; then
  echo 'HOOK_INSTALLED=1'
  exit 0
fi
OUT=$(pm install -r --user 0 "$APK" 2>&1)
RC=$?
if [ "$RC" -eq 0 ] && printf '%s' "$OUT" | grep -q Success; then
  echo 'HOOK_INSTALLED=1'
  exit 0
fi
echo "HOOK_ERROR=$OUT"
exit 21
