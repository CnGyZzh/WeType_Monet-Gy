#!/system/bin/sh
D=${0%/*}
echo 'GyGlass v29 diagnostics'
sh "$D/switch.sh" status
if pm path com.gy.wetype.hook >/dev/null 2>&1; then
  echo 'HOOK_PACKAGE=present'
else
  echo 'HOOK_PACKAGE=missing'
fi
echo 'LSPOSED_SCOPE=first enable Gy Hook for com.tencent.wetype'
echo 'Do not enable older WeTyper / GyGlass duplicate hooks simultaneously'
