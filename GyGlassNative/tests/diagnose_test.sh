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
printf '  versionName=%s\n' "$WETYPE_TEST_VERSION"
MOCK
cat > "$tmp/bin/getprop" <<'MOCK'
#!/bin/sh
printf '36\n'
MOCK
chmod +x "$tmp/bin/"*
for version in 4.0.0.58111 4.0.0.58112 5.0.0.1; do
  out=$(PATH="$tmp/bin:$PATH" WETYPE_TEST_VERSION="$version" sh "$tmp/module/diagnose.sh")
  printf '%s\n' "$out" | grep -q "INPUT_VERSION=$version"
  printf '%s\n' "$out" | grep -q 'RENDERER=NOT_IMPLEMENTED'
  case "$version" in
    4.0.0.58111) printf '%s\n' "$out" | grep -q 'COMPAT=KNOWN_BUILD_NOT_RENDER_TESTED';;
    4.0.0.58112) printf '%s\n' "$out" | grep -q 'COMPAT=NEW_BUILD_NEEDS_SIGNATURE_CHECK';;
    *) printf '%s\n' "$out" | grep -q 'COMPAT=UNRECOGNIZED_HOST_VERSION';;
  esac
done
echo 'GyGlass compatibility diagnostics: PASS'
