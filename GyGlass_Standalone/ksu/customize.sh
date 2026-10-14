#!/system/bin/sh
ui_print '- GyGlass v29: independent keyboard hook'
[ -n "${MODPATH:-}" ] || abort 'MODPATH missing'
[ -s "$MODPATH/gyhook.apk" ] || abort 'missing gyhook.apk (build APK before packaging)'
for f in install_hook.sh switch.sh action.sh service.sh; do
  chmod 0755 "$MODPATH/$f" || abort 'chmod failed'
done
ui_print '- Will install Gy Hook through Android PackageManager after reboot'
ui_print '- First activation requires LSPosed scope: com.tencent.wetype'
true
