#!/usr/bin/env bash
set -euo pipefail

UE_ROOT="${1:-${UE_ROOT:-}}"

if [[ -z "$UE_ROOT" ]]; then
  echo "UE_ROOT is required." >&2
  exit 1
fi

BUILD_SH="$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh"
UAT_SH="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"

if [[ ! -x "$BUILD_SH" ]]; then
  echo "Missing Unreal Linux Build.sh: $BUILD_SH" >&2
  exit 1
fi

if [[ ! -x "$UAT_SH" ]]; then
  echo "Missing Unreal RunUAT.sh: $UAT_SH" >&2
  exit 1
fi

PROJECT="$(cd "$(dirname "$0")/.." && pwd)/unreal/PokemonEmeraldRemastered.uproject"
if [[ ! -f "$PROJECT" ]]; then
  echo "Missing Unreal project: $PROJECT" >&2
  exit 1
fi

echo "Unreal root: $UE_ROOT"
echo "Project: $PROJECT"

if command -v clang >/dev/null 2>&1; then
  clang --version | head -n 1
else
  echo "WARNING: clang is not on PATH; Unreal may use its bundled toolchain." >&2
fi

if command -v java >/dev/null 2>&1; then
  java -version 2>&1 | head -n 1
else
  echo "WARNING: Java not found on PATH. Android packaging may fail." >&2
fi

android_sdk="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -n "$android_sdk" ]]; then
  echo "Android SDK: $android_sdk"
else
  echo "WARNING: ANDROID_SDK_ROOT/ANDROID_HOME is not set." >&2
fi

ndk_root="${ANDROID_NDK_ROOT:-${NDKROOT:-}}"
if [[ -n "$ndk_root" ]]; then
  echo "Android NDK: $ndk_root"
else
  echo "WARNING: ANDROID_NDK_ROOT/NDKROOT is not set." >&2
fi

if [[ -x "$UE_ROOT/Engine/Extras/Android/SetupAndroid.sh" ]]; then
  echo "SetupAndroid.sh is available."
else
  echo "WARNING: Unreal SetupAndroid.sh not found." >&2
fi

echo "Linux Unreal preflight passed."
