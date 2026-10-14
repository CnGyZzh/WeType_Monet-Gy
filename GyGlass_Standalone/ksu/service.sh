#!/system/bin/sh
D=${0%/*}
I=0
while [ "$(getprop sys.boot_completed)" != 1 ] && [ "$I" -lt 100 ]; do
  sleep 2
  I=$((I+1))
done
[ "$(getprop sys.boot_completed)" = 1 ] || exit 0
sh "$D/install_hook.sh" >> "$D/gyglass.log" 2>&1
[ -f "$D/key_background.state" ] || printf "off\n" > "$D/key_background.state"
sh "$D/switch.sh" status >> "$D/gy.log" 2>&1
