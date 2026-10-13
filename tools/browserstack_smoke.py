#!/usr/bin/env python3
import argparse
import base64
import json
import os
import pathlib
import subprocess
import sys
import time
import urllib.error
import urllib.request

API_BASE = "https://api-cloud.browserstack.com"
HUB_SESSION_URL = "https://hub-cloud.browserstack.com/wd/hub/session"

DEFAULT_MARKERS = [
    "REM_SMOKE: BOOT_OK",
    "REM_SMOKE: RENDER_PACKAGE_OK",
    "REM_SMOKE: HOUSE_RENDER_OK",
    "REM_SMOKE: HOUSE_WARP_OK",
    "REM_SMOKE: LITTLEROOT_RENDER_OK",
    "REM_SMOKE: ROUTE101_RENDER_OK",
    "REM_SMOKE: PASS",
]


def basic_auth_header(username: str, access_key: str) -> str:
    raw = f"{username}:{access_key}".encode("utf-8")
    return "Basic " + base64.b64encode(raw).decode("ascii")


def http_json(url: str, method: str, auth: str, payload=None, timeout=120):
    data = None
    headers = {"Authorization": auth, "Accept": "application/json"}
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            body = resp.read()
            return json.loads(body.decode("utf-8")) if body else {}
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(
            f"{method} {url} failed: HTTP {exc.code}: {body}"
        ) from exc


def http_text(url: str, auth: str, timeout=120) -> str:
    req = urllib.request.Request(
        url,
        headers={"Authorization": auth, "Accept": "text/plain"},
        method="GET",
    )
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(
            f"GET {url} failed: HTTP {exc.code}: {body}"
        ) from exc


def upload_app(
    apk: pathlib.Path,
    username: str,
    access_key: str,
    custom_id: str,
) -> str:
    cmd = [
        "curl",
        "--silent",
        "--show-error",
        "--fail-with-body",
        "--max-time",
        "300",
        "--retry",
        "3",
        "--retry-all-errors",
        "-u",
        f"{username}:{access_key}",
        "-X",
        "POST",
        f"{API_BASE}/app-automate/upload",
        "-F",
        f"file=@{apk}",
        "-F",
        f"custom_id={custom_id}",
    ]
    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"BrowserStack app upload failed (curl {result.returncode}): "
            f"{result.stderr.strip()} {result.stdout.strip()}"
        )

    payload = json.loads(result.stdout)
    app_url = payload.get("app_url")
    if not app_url:
        raise RuntimeError(
            f"BrowserStack upload response has no app_url: {payload}"
        )
    return app_url


def create_session(
    auth: str,
    app_url: str,
    device: str,
    os_version: str,
    build_name: str,
):
    payload = {
        "capabilities": {
            "alwaysMatch": {
                "platformName": "Android",
                "appium:automationName": "UIAutomator2",
                "appium:app": app_url,
                "bstack:options": {
                    "deviceName": device,
                    "osVersion": os_version,
                    "projectName": "Pokemon Emerald Remastered",
                    "buildName": build_name,
                    "sessionName": "Android real-device smoke",
                    "deviceLogs": True,
                    "appiumLogs": True,
                    "video": True,
                },
            }
        }
    }

    response = http_json(
        HUB_SESSION_URL,
        "POST",
        auth,
        payload,
        timeout=180,
    )
    value = response.get("value", response)
    session_id = value.get("sessionId") or response.get("sessionId")
    if not session_id:
        raise RuntimeError(
            f"BrowserStack did not return a session id: {response}"
        )
    return session_id


def end_session(auth: str, session_id: str):
    url = f"{HUB_SESSION_URL}/{session_id}"
    try:
        http_json(url, "DELETE", auth, timeout=60)
    except Exception as exc:
        print(
            f"warning: failed to end session cleanly: {exc}",
            file=sys.stderr,
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--apk", required=True, type=pathlib.Path)
    parser.add_argument(
        "--device",
        default=os.getenv("BROWSERSTACK_DEVICE", "Google Pixel 8"),
    )
    parser.add_argument(
        "--os-version",
        default=os.getenv("BROWSERSTACK_OS_VERSION", "14.0"),
    )
    parser.add_argument("--wait-seconds", type=int, default=45)
    parser.add_argument(
        "--output-dir",
        type=pathlib.Path,
        default=pathlib.Path("artifacts/browserstack"),
    )
    parser.add_argument(
        "--custom-id",
        default="pokemon-emerald-remastered-ci",
    )
    parser.add_argument(
        "--build-name",
        default=os.getenv(
            "GITHUB_RUN_ID",
            "local-browserstack-smoke",
        ),
    )
    parser.add_argument("--skip-marker-check", action="store_true")
    args = parser.parse_args()

    username = os.getenv("BROWSERSTACK_USERNAME")
    access_key = os.getenv("BROWSERSTACK_ACCESS_KEY")
    if not username or not access_key:
        raise SystemExit(
            "BROWSERSTACK_USERNAME and BROWSERSTACK_ACCESS_KEY are required"
        )

    apk = args.apk.resolve()
    if not apk.is_file():
        raise SystemExit(f"APK not found: {apk}")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    auth = basic_auth_header(username, access_key)

    print(f"Uploading APK: {apk}")
    app_url = upload_app(
        apk,
        username,
        access_key,
        args.custom_id,
    )
    print(f"BrowserStack app: {app_url}")

    print(
        f"Starting real-device session: "
        f"{args.device} / Android {args.os_version}"
    )
    session_id = create_session(
        auth,
        app_url,
        args.device,
        args.os_version,
        args.build_name,
    )
    print(f"Session id: {session_id}")

    try:
        time.sleep(max(1, args.wait_seconds))
    finally:
        end_session(auth, session_id)

    time.sleep(5)

    details = http_json(
        f"{API_BASE}/app-automate/sessions/{session_id}.json",
        "GET",
        auth,
    )
    session = details.get("automation_session", {})

    safe_details = {
        "session_id": session_id,
        "status": session.get("status"),
        "reason": session.get("reason"),
        "device": session.get("device"),
        "os_version": session.get("os_version"),
        "browser_url": session.get("browser_url"),
        "app_url": app_url,
    }
    (args.output_dir / "session.json").write_text(
        json.dumps(safe_details, indent=2),
        encoding="utf-8",
    )

    device_logs_url = session.get("device_logs_url")
    if not device_logs_url:
        raise RuntimeError(
            f"No device_logs_url in session details: {details}"
        )

    device_logs = http_text(device_logs_url, auth)
    (args.output_dir / "device.log").write_text(
        device_logs,
        encoding="utf-8",
    )

    if not args.skip_marker_check:
        missing = [
            marker
            for marker in DEFAULT_MARKERS
            if marker not in device_logs
        ]
        if missing:
            print("Missing smoke markers:", file=sys.stderr)
            for marker in missing:
                print(f"  - {marker}", file=sys.stderr)
            return 2

    print("BrowserStack smoke completed.")
    if session.get("browser_url"):
        print(f"Session: {session['browser_url']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
