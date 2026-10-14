#!/system/bin/sh
D=${0%/*}
STATE="$D/preset.conf"
MODE=seamless
KEY_OPACITY=16
if [ -f "$STATE" ];then
 old=$(sed -n 's/^MODE=\(seamless\|independent\)$/\1/p' "$STATE" | head -1)
 [ -n "$old" ] && MODE="$old"
 old=$(sed -n 's/^KEY_OPACITY=\([0-9][0-9]*\)$/\1/p' "$STATE" | head -1)
 case "$old" in ''|*[!0-9]*) ;; *) if [ "$old" -ge 1 ] && [ "$old" -le 100 ];then KEY_OPACITY="$old";fi;;esac
fi
case "${1:-status}" in
 status) printf 'MODE=%s\nKEY_OPACITY=%s\n' "$MODE" "$KEY_OPACITY";exit 0;;
 seamless|independent) MODE="$1";;
 opacity)
  v="${2:-}"
  case "$v" in ''|*[!0-9]*) echo 'RESULT=ERROR invalid opacity';exit 2;;esac
  [ "$v" -ge 1 ] && [ "$v" -le 100 ] || { echo 'RESULT=ERROR out of range';exit 2; }
  KEY_OPACITY="$v";;
 *) echo 'RESULT=ERROR unsupported action';exit 2;;
esac
[ "$(id -u)" = 0 ] || { echo 'RESULT=ERROR root required';exit 3; }
umask 077
TEMP="$D/.preset.$$"
printf 'MODE=%s\nKEY_OPACITY=%s\n' "$MODE" "$KEY_OPACITY" > "$TEMP" || exit 4
mv -f "$TEMP" "$STATE" || exit 5
echo 'RESULT=SAVED'
printf 'MODE=%s\nKEY_OPACITY=%s\n' "$MODE" "$KEY_OPACITY"
echo 'RENDERER=NOT_VERIFIED'
