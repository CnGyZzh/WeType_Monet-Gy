#!/system/bin/sh
D=${0%/*}
S="$D/preset.conf"
case "${1:-status}" in
 status) if [ -s "$S" ]; then cat "$S"; else echo 'MODE=seamless'; fi ;;
 seamless|independent)
  [ "$(id -u)" = 0 ] || { echo 'RESULT=ERROR root required'; exit 3; }
  T="$D/.preset.$$"
  umask 077
  printf 'MODE=%s\n' "$1" > "$T" || exit 4
  mv -f "$T" "$S" || exit 5
  echo "MODE=$1"
  echo 'RESULT=SAVED'
  echo 'RENDERER=NOT_READY'
  ;;
 *) echo 'RESULT=ERROR bad mode'; exit 2 ;;
esac
