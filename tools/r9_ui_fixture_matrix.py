#!/usr/bin/env python3
"""Independent expectations over frames emitted by the same model used by UE."""
import argparse
import json
import subprocess
from pathlib import Path

def verify(probe: str) -> None:
    frames = json.loads(subprocess.check_output([probe, '--fixtures'], text=True))
    by_id = {frame['screen']: frame for frame in frames}
    recipe = json.loads((Path(__file__).resolve().parents[1] / 'tests/fixtures/r9/ui_expectations.json').read_text())
    for expected in recipe['frames']:
        actual = by_id[expected['screen']]
        assert actual['title'] == expected['title'], (expected, actual)
        assert actual['modal'] == expected['modal'], (expected, actual)
        for text in expected.get('body_contains', []):
            assert text in actual['body'], (expected, actual)
        for text in expected.get('row_contains', []):
            assert any(text in row['label'] for row in actual['rows']), (expected, actual)
    assert len(frames) == len(recipe['frames']) == 10
    print('R9 fixture matrix: 10 required screens verified')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--probe', required=True)
    verify(parser.parse_args().probe)
