#!/usr/bin/env bash
set -euo pipefail
task_ue_root="${1:-${UE_ROOT:-}}"
if [[ -z "$task_ue_root" ]]; then
  echo "UE_ROOT is required for actual installation preflight." >&2
  exit 2
fi
task_repo_root="$(cd "$(dirname "$0")/.." && pwd)"
exec python3 "$task_repo_root/tools/unreal_preflight.py" --engine-root "$task_ue_root" --host Linux --android
