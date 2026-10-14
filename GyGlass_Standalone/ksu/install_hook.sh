#!/system/bin/sh
D=${0%/*}
PKG=com.gy.wetype.hook
APK="$D/gyhook.apk"
HASHFILE="$D/.installed-apk.sha256"
[ -s "$APK" ] || { echo 'HOOK_ERROR=missing APK'; exit 20; }
HASH=$(sha256sum "$APK" 2>/dev/null | awk '{print $1}')
[ -n "$HASH" ] || { echo 'HOOK_ERROR=sha256 unavailable'; exit 22; }
if pm path "$PKG" >/dev/null 2>&1 && [ "$(cat "$HASHFILE" 2>/dev/null)" = "$HASH" ]; then
  echo 'HOOK_INSTALLED=1 unchanged'
  exit 0
fi
OUT=$(pm install -r --user 0 "$APK" 2>&1)
RC=$?
if [ "$RC" -eq 0 ] && printf '%s' "$OUT" | grep -q Success; then
  printf '%s\n' "$HASH" > "$HASHFILE"
  echo 'HOOK_INSTALLED=1 updated'
  exit 0
fi
echo "HOOK_ERROR=$OUT"
echo 'HOOK_NOTE=previous Hook package was not deleted, user data untouched'
exit 21
