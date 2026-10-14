#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/bin" "$tmp/module"
cp "$root/module/diagnose.sh" "$tmp/module/diagnose.sh"
cat > "$tmp/bin/pm" <<'MOCK'
#!/bin/sh
[ "$1" = path ] && [ "$2" = com.tencent.wetype ]
MOCK
cat > "$tmp/bin/dumpsys" <<'MOCK'
#!/bin/sh
printf '  versionCode=%s minSdk=24 targetSdk=35\n  versionName=%s\n' "$WETYPE_TEST_CODE" "$WETYPE_TEST_VERSION"
MOCK
cat > "$tmp/bin/getprop" <<'MOCK'
#!/bin/sh
printf '36\n'
MOCK
chmod +x "$tmp/bin/"*
for pair in '4.0.0:57206' '4.0.0:57207' '5.0.0:90001'; do
 version=${pair%:*}
 code=${pair#*:}
 out=$(PATH="$tmp/bin:$PATH" WETYPE_TEST_VERSION="$version" WETYPE_TEST_CODE="$code" sh "$tmp/module/diagnose.sh")
 printf '%s\n' "$out" | grep -q "INPUT_VERSION=$version"
 printf '%s\n' "$out" | grep -q "BUILD_CODE=$code"
 printf '%s\n' "$out" | grep -q 'RENDERER=NOT_IMPLEMENTED'
 case "$code" in
 57206) printf '%s\n' "$out" | grep -q 'COMPAT=KNOWN_PACKAGE_NOT_RENDER_TESTED';;
 57207) printf '%s\n' "$out" | grep -q 'COMPAT=NEW_BUILD_NEEDS_SIGNATURE_CHECK';;
 *) printf '%s\n' "$out" | grep -q 'COMPAT=UNRECOGNIZED_HOST_VERSION';;
 esac
done
echo 'GyGlass manifest build-code diagnostics: PASS'
