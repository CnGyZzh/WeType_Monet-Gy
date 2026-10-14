#!/system/bin/sh
D=\${0%/*}
I=0
while [ "$(getprop sys.boot_completed)" != 1 ] && [ "$I" -lt 100 ]; do
 sleep 2
 I=$((I+1))
done
[ "$(getprop sys.boot_completed)" = 1 ] || exit 0
I=0
until sh "$D/install_hook.sh" >> "$D/gyglass.log" 2>&1; do
 I=$((I+1))
 [ "$I" -lt 3 ] || break
 sleep 12
done
[ -s "$D/key_background.state" ] || printf 'off\n' > "$D/key_background.state"
P=/data/user/0/com.tencent.wetype/shared_prefs
if [ -d "$P" ] && [ ! -f "$P/gy_glass.xml" ]; then
 GY_NO_RESTART=1 sh "$D/switch.sh" "$(cat "$D/key_background.state")" >> "$D/gyglass.log" 2>&1 || :
fi
# No periodic pollers or services after initialization.
