#!/system/bin/sh
D=${0%/*}
P=/data/user/0/com.tencent.wetype/shared_prefs
F="$P/gy_glass.xml"
S="$D/key_background.state"
mode=${1:-status}
read_state(){ [ -s "$S" ] && cat "$S" || printf 'off'; }
case "$mode" in
 status) echo "STATE=$(read_state)"; [ -f "$F" ] && echo 'CONFIG=written' || echo 'CONFIG=pending'; exit 0;;
 on|off) :;;
 *) echo 'RESULT=ERROR invalid mode'; exit 2;;
esac
[ "$(id -u)" = 0 ] || { echo 'RESULT=ERROR root required'; exit 3; }
[ -d "$P" ] || { echo 'RESULT=PENDING WeType not initialized'; exit 11; }
U=$(stat -c %u "/data/user/0/com.tencent.wetype" 2>/dev/null)
G=$(stat -c %g "/data/user/0/com.tencent.wetype" 2>/dev/null)
case "$U:$G" in *[!0-9:]*|:|*::*) echo 'RESULT=ERROR bad owner'; exit 12;; esac
T="$P/.gy_glass.$$"
VALUE=false
[ "$mode" = on ] && VALUE=true
printf '<?xml version="1.0" encoding="utf-8" standalone="yes" ?>\n<map>\n  <boolean name="key_background" value="%s" />\n</map>\n' "$VALUE" > "$T" || exit 13
if ! chown "$U:$G" "$T" || ! chmod 0600 "$T"; then
  rm -f "$T"; echo 'RESULT=ERROR ownership failed'; exit 14
fi
restorecon "$T" >/dev/null 2>&1 || :
mv -f "$T" "$F" || { rm -f "$T"; echo 'RESULT=ERROR commit failed'; exit 15; }
restorecon "$F" >/dev/null 2>&1 || :
printf '%s\n' "$mode" > "$S"
echo "STATE=$mode"
if [ "${GY_NO_RESTART:-0}" != 1 ]; then
  am force-stop com.tencent.wetype >/dev/null 2>&1 && echo 'INPUT_METHOD_RESTARTED=1' || echo 'INPUT_METHOD_RESTARTED=0'
fi
echo 'RESULT=APPLIED; reopen keyboard for effect'
