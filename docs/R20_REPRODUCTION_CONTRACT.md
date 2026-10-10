# R20-T1/V1 — Actual clean reproduction and source acceptance

From a complete clean source checkout with full Git history and BUILD.md tools:

```sh
python tools/r20_full_suite.py --build-dir build/r20-full --receipt build/r20-full-suite-receipt.json
```

Use fresh ignored build/receipt paths. The entry point executes the entire R19
native/Python/asset/package/source suite, checks all seven actual components,
then executes the mandatory five R20 regression modules with zero skips.
It creates two independent detached worktrees of the actual HEAD, each containing
complete tracked inputs, and invokes the production CLI in a separate process.
Each package is verified against its external generation receipt. Complete index
and package SHA-256, file/input counts must match between the clean checkouts.
No owner/generator is replaced with a mock in the default pipeline.

Nine real-package negative cases must return the specific validation rejection:
corrupt World, Render, Characters and Environment manifests; missing owner file;
extra file; changed payload with fully rebuilt adjacent index; symlink file; and
tracked generator drift. Every temporary mutation is restored in finally, and
original verification is repeated before owned worktrees are removed. No active
source branch or private existing output is overwritten. Failed work blocks the
acceptance receipt and prints its first failing component.

The entry point then executes the complete tracked/history security scan and
source release-preparation check. Coverage failures, skips, mismatched evidence,
unsafe/unsupported packages, security findings and dirty/shallow inputs fail.
The final SOURCE_PRODUCTION_ACCEPTANCE_PASS receipt contains actual counts,
hashes, component evidence and false actual_unreal_or_device_verified. CI runs
this same entry point and uploads only this metadata receipt. Owner payloads,
ROMs/private saves/imports/audio/APKs and credentials are never uploaded.

Eight new receipt-gate tests were RED before the entry point existed and pass
with explicit synthetic evidence after implementation. Those tests cover
acceptance refusal; actual reproduction/security/full-suite counts come only
from the hosted default execution. No local full-suite claim is made from this
partial source cache. Cross-toolchain reproducibility, engine compile/cook/APK,
imports, listening/quality and devices remain R18/runtime obligations.
