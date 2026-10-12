#!/usr/bin/env python3
"""Prepare the authoritative R11/R12/R13 C includes used by Unreal core embeds."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CATALOGS = (
    ('build_r11_domain_catalog.py', 'emerald_domain_catalog.inc', 'domain_catalog_report.json'),
    ('build_r12_encounter_catalog.py', 'emerald_encounter_catalog.inc', 'encounter_catalog_report.json'),
    ('build_r13_trainer_catalog.py', 'emerald_trainer_catalog.inc', 'trainer_catalog_report.json'),
)


def main():
    output = ROOT / 'unreal/Intermediate/CoreCatalogs'
    output.mkdir(parents=True, exist_ok=True)
    for script, include, report in CATALOGS:
        subprocess.run([sys.executable, str(ROOT / 'tools' / script),
                        str(ROOT / 'vendor/vanillaplus'), str(output / include),
                        str(output / report)], cwd=ROOT, check=True, timeout=120)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
