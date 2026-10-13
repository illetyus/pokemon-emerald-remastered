#!/usr/bin/env python3
"""Prepare the authoritative R11/R12/R13 C includes used by Unreal core embeds."""
from pathlib import Path
import hashlib
import json
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
    head=subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip()
    scopes=['core','unreal/Source','unreal/Config','tools/build_unreal_core_catalogs.py']
    names=subprocess.check_output(['git','-C',str(ROOT),'ls-files','--cached','--others','--exclude-standard','--',*scopes],text=True).splitlines()
    hashes={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sorted(set(names))}
    digest=hashlib.sha256(json.dumps(hashes,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    dirty=bool(subprocess.check_output(['git','-C',str(ROOT),'status','--porcelain','--',*scopes],text=True).strip())
    tag='#define REMASTER_AP04_HEAD "'+head+'"\n#define REMASTER_AP04_INPUT_SHA "'+digest+'"\n#define REMASTER_AP04_DIRTY '+str(int(dirty))+'\n'
    (output/'remaster_ap04_build_tag.inc').write_bytes(tag.encode())
    (output/'ap04_build_tag_receipt.json').write_bytes((json.dumps({'head':head,'inputs_sha256':digest,'dirty':dirty,'files':hashes},indent=2)+'\n').encode())
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
