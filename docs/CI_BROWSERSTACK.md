# BrowserStack Android smoke

This repository includes a real-device Android smoke harness in
`tools/browserstack_smoke.py` and a manual workflow in
`.github/workflows/browserstack-smoke.yml`.

## Secret source

The workflow supports either of these existing secret delivery models:

1. Doppler GitHub sync provides `BROWSERSTACK_USERNAME` and
   `BROWSERSTACK_ACCESS_KEY` as GitHub Actions secrets.
2. GitHub provides only `DOPPLER_TOKEN`; the self-hosted runner has the
   Doppler CLI installed, and the test runs through `doppler run`.

No BrowserStack credential is committed to the repository.

## Runtime flow

1. Resolve an Android APK on the self-hosted Windows runner.
2. Upload the APK to BrowserStack App Automate.
3. Start a real Android Appium session.
4. Keep the game running for the requested smoke interval.
5. Stop the session.
6. Download BrowserStack device logs.
7. Optionally require all `REM_SMOKE` markers.
8. Publish sanitized session metadata and device logs as a GitHub artifact.

## Expected markers

When marker enforcement is enabled, the device log must contain:

- `REM_SMOKE: BOOT_OK`
- `REM_SMOKE: RENDER_PACKAGE_OK`
- `REM_SMOKE: HOUSE_RENDER_OK`
- `REM_SMOKE: HOUSE_WARP_OK`
- `REM_SMOKE: LITTLEROOT_RENDER_OK`
- `REM_SMOKE: ROUTE101_RENDER_OK`
- `REM_SMOKE: PASS`

Marker enforcement is disabled by default until the Unreal runtime emits the
full acceptance sequence.

## Runner labels

The workflow currently expects:

`self-hosted`, `windows`, `unreal-5.8`, `android`

The Windows runner setup is intentionally deferred until the local UE 5.8.3
installation is ready.
