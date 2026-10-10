import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import r19_full_suite as suite


class FullSuiteEvidenceTests(unittest.TestCase):
    def test_empty_or_missing_native_tests_cannot_pass(self):
        for discovery in [{'tests': []}, {'tests': [{'name': 'other', 'command': ['native']}]}]:
            with self.assertRaisesRegex(ValueError, 'native coverage'):
                suite.validate_discovery(discovery, ['required'])

    def test_native_command_and_unique_name_are_required(self):
        for tests in [[{'name': 'required', 'command': []}],
                      [{'name': 'required', 'command': ['a']}, {'name': 'required', 'command': ['b']}]]:
            with self.assertRaises(ValueError): suite.validate_discovery({'tests': tests}, ['required'])

    def test_native_failure_skip_or_truncated_evidence_fails(self):
        for xml in ['<testsuite/>', '<testsuite><testcase name="required"><skipped/></testcase></testsuite>',
                    '<testsuite><testcase name="required"><failure/></testcase></testsuite>']:
            with self.assertRaises(ValueError): suite.validate_junit(xml, ['required'])
        self.assertEqual(suite.validate_junit('<testsuite><testcase name="required"/></testsuite>',
                                             ['required']), 1)

    def test_first_component_failure_is_named_and_propagated(self):
        def broken(): raise ValueError('first divergent snapshot 2/save_block1')
        with self.assertRaisesRegex(ValueError, 'replay.*snapshot 2/save_block1'):
            suite.component('replay', broken)

    def test_python_failure_or_skip_cannot_be_full_success(self):
        result = unittest.TestResult(); result.testsRun = 2
        case = unittest.FunctionTestCase(lambda: None)
        result.skipped.append((case, 'dependency unavailable'))
        with self.assertRaisesRegex(ValueError, 'skip'): suite.validate_python(result, 2)
        result.skipped.clear(); result.failures.append((case, 'changed state'))
        with self.assertRaisesRegex(ValueError, 'first failure'): suite.validate_python(result, 2)

    def test_versioned_roster_requires_every_component_and_original_test(self):
        config = json.loads((ROOT / 'data/r19/full_suite.json').read_text())
        suite.validate(config)
        config['components'].pop()
        with self.assertRaisesRegex(ValueError, 'component coverage'): suite.validate(config)


if __name__ == '__main__': unittest.main()
