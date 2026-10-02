#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
  echo "Usage: $0 <apk-path> <package-name> [report-path]"
  echo "Example: $0 app-debug.apk com.illetyus.emeraldremaster.r0sdl"
  exit 2
fi

APK="$1"
PACKAGE="$2"
REPORT="${3:-r0-android-${PACKAGE}.log}"
EXPECTED_HASH="7218695048241891488"

if ! command -v adb >/dev/null 2>&1; then
  echo "adb is required."
  exit 1
fi

if [ ! -f "$APK" ]; then
  echo "APK not found: $APK"
  exit 1
fi

adb get-state >/dev/null

echo "[R0] Installing $APK"
adb install -r "$APK" >/dev/null

echo "[R0] Clearing previous app state"
adb shell pm clear "$PACKAGE" >/dev/null || true
adb logcat -c

launch_app() {
  adb shell monkey -p "$PACKAGE" -c android.intent.category.LAUNCHER 1 >/dev/null
}

send_key() {
  adb shell input keyevent "$1"
  sleep 0.15
}

echo "[R0] Cold launch"
launch_app
sleep 3

echo "[R0] Running canonical gameplay input sequence"
send_key KEYCODE_DPAD_UP
send_key KEYCODE_DPAD_RIGHT
send_key KEYCODE_DPAD_RIGHT
send_key KEYCODE_ENTER
send_key KEYCODE_DPAD_DOWN
send_key KEYCODE_DPAD_DOWN
send_key KEYCODE_DPAD_DOWN
sleep 1

echo "[R0] Backgrounding app to exercise lifecycle save"
send_key KEYCODE_HOME
sleep 2

echo "[R0] Resuming app"
launch_app
sleep 2

echo "[R0] Force-stopping process after persisted save"
adb shell am force-stop "$PACKAGE"
sleep 1

echo "[R0] Relaunching to exercise persistent load"
launch_app
sleep 7

adb logcat -d -v time > "$REPORT"

echo
echo "[R0] Relevant log lines:"
grep -E "R0 (persistent|lifecycle|PERF)|R0 persistent|R0 lifecycle|R0 PERF" "$REPORT" || true

if grep -q "$EXPECTED_HASH" "$REPORT"; then
  echo
  echo "[R0] PASS: canonical state hash $EXPECTED_HASH observed."
else
  echo
  echo "[R0] WARNING: canonical state hash $EXPECTED_HASH was not observed."
  echo "Inspect $REPORT before accepting lifecycle/save equivalence."
fi

if grep -q "persistent load complete" "$REPORT"; then
  echo "[R0] PASS: persistent load observed after process restart."
else
  echo "[R0] WARNING: persistent load marker not observed."
fi

if grep -q "R0 PERF" "$REPORT"; then
  echo "[R0] PASS: frame-pacing telemetry observed."
else
  echo "[R0] WARNING: frame-pacing telemetry marker not observed."
fi

echo "[R0] Full log saved to: $REPORT"
