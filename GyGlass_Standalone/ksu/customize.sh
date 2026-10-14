#!/system/bin/sh
ui_print '- GyGlass v30'
[ -n "\${MODPATH:-}" ] || abort 'MODPATH missing'
[ -s "$MODPATH/gyhook.apk" ] || abort 'Missing keyboard Hook component'
for f in install_hook.sh switch.sh action.sh service.sh; do
 chmod 0755 "$MODPATH/$f" || abort 'chmod failed'
done
ui_print '- KernelSU WebUI: Seamless / Glass Keys'
ui_print '- First use: enable GyGlass in LSPosed for WeChat Input Method'
true
