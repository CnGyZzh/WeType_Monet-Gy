#!/system/bin/sh
D=\${0%/*}
echo 'GyGlass v30 | Diagnostics'
sh "$D/switch.sh" status
printf '\nRecent log:\n'
tail -n 12 "$D/gyglass.log" 2>/dev/null || :
echo 'A stored preset does not prove the rendering Hook is active.'
