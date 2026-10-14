#!/system/bin/sh
D=\${0%/*}
P=/data/user/0/com.tencent.wetype/shared_prefs
F="$P/gy_glass.xml"
S="$D/key_background.state"
R=/data/user/0/com.tencent.wetype/files/gyglass_runtime.txt
mode=\${1:-status}
read_state() {
 case "$(cat "$S" 2>/dev/null)" in on|off) cat "$S";; *) echo off;; esac
}
report() {
 echo "STATE=$(read_state)"
 [ -f "$F" ] && echo "CONFIG=written" || echo "CONFIG=pending"
 pm path com.gy.glass.engine >/dev/null 2>&1 && echo "HOOK_PACKAGE=installed" || echo "HOOK_PACKAGE=missing"
 if [ -f "$R" ]; then
  sed -n '/^HOOK=/p;/^MODE=/p;/^EVENT=/p;/^AT=/p' "$R" |
   sed 's/^HOOK=/HOOK_RUNTIME=/;s/^MODE=/HOOK_MODE=/;s/^EVENT=/HOOK_EVENT=/;s/^AT=/HOOK_TIMESTAMP=/'
 else echo "HOOK_RUNTIME=unverified"; fi
}
case "$mode" in
 status|read) report;exit 0;;
 on|off) :;;
 *) echo "RESULT=ERROR invalid preset";exit 2;;
esac
[ "$(id -u)" = 0 ] || { echo "RESULT=ERROR root required"; exit 3; }
[ -d /data/user/0/com.tencent.wetype ] || { echo "RESULT=PENDING keyboard not initialized"; exit 11; }
mkdir -p "$P" || exit 8
U=$(stat -c %u /data/user/0/com.tencent.wetype 2>/dev/null)
G=$(stat -c %g /data/user/0/com.tencent.wetype 2>/dev/null)
case "$U:$G" in *[!0-9:]*|:|*::*) echo "RESULT=ERROR owner";exit 9;; esac
T="$P/.gy_glass.$$.tmp"
VALUE=false
[ "$mode" = on ] && VALUE=true
printf '<?xml version="1.0" encoding="utf-8" standalone="yes" ?>\n<map>\n <boolean name="key_background" value="%s" />\n</map>\n' "$VALUE" > "$T" || exit 10
if ! chown "$U:$G" "$T" || ! chmod 0600 "$T"; then
 rm -f "$T";echo "RESULT=ERROR permissions";exit 12
fi
restorecon "$T" >/dev/null 2>&1 || :
mv -f "$T" "$F" || { rm -f "$T";echo "RESULT=ERROR commit";exit 13; }
restorecon "$F" >/dev/null 2>&1 || :
printf '%s\n' "$mode" > "$S"
# Restart once on user action only. No background polling or per-key subprocesses.
am force-stop com.tencent.wetype >/dev/null 2>&1 || :
echo "RESULT=APPLIED"
report
