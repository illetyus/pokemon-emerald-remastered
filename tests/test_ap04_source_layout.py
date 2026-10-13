import importlib.util
from pathlib import Path
import sys
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ap04_source_layout',ROOT/'tools/ap04_source_layout.py')
layout=importlib.util.module_from_spec(spec);sys.modules[spec.name]=layout;spec.loader.exec_module(layout)
class SourceLayoutTests(unittest.TestCase):
    def test_named_data_blocks_decode_exact_little_endian_bytes(self):
        asm='other:\n.byte 9\nap04_bits:\n.byte 127\n.short 258\n.space 2\n.word 4294967295\n.size ap04_bits, .-ap04_bits\n'
        self.assertEqual(layout.parse_data(asm,'ap04_bits',9),b'\x7f\x02\x01\0\0\xff\xff\xff\xff')
    def test_bad_named_data_blocks_fail_closed(self):
        for asm,size in [('bad:\n.byte 1\n',1),('ap04_bits:\n.word external\n',4),
                ('ap04_bits:\n.byte 1\n',2),('ap04_bits:\n.space 65537\n',1),
                ('ap04_bits:\n.byte 1\nap04_bits:\n.byte 2\n',1)]:
            with self.subTest(asm=asm),self.assertRaises(ValueError):
                layout.parse_data(asm,'ap04_bits',size)
    def test_only_named_probe_array_is_read(self):
        text='unrelated:\n.word 99\nap04_layout:\n.word 15816\n.word 3908\n.size ap04_layout, .-ap04_layout\nother:\n.word 77\n'
        self.assertEqual(layout.parse_words(text,2),[15816,3908])
    def test_missing_truncated_duplicate_or_nonconstant_array_rejected(self):
        for text in ['other:\n.word 1\n','ap04_layout:\n.word 1\n',
            'ap04_layout:\n.word 1\nap04_layout:\n.word 2\n',
            'ap04_layout:\n.word external\n.word 2\n']:
            with self.subTest(text=text),self.assertRaises(ValueError):layout.parse_words(text,2)
    def test_known_agbcc_source_anchors_validate(self):
        anchors={'sb1.bytes':0x3DC8,'sb2.bytes':0xF44,'storage.bytes':0x83D0,
            'object.bytes':0x28,'object_template.bytes':0x18,'sb1.flags':0x12B0,
            'sb1.vars':0x13DC,'sb1.unused_3598':0x35D8,'sb2.encryptionKey':0xAC,
            'sb2.follower':0xF2C,'storage.boxes':4,'sb1.roamer':0x321C,
            'sb1.playerPartyCount':0x234,'sb1.objectEventTemplates':0xCB0}
        layout.validate_anchors(anchors)
        for key in anchors:
            bad=dict(anchors);bad[key]+=1
            with self.subTest(key=key),self.assertRaises(ValueError):layout.validate_anchors(bad)
        bad=dict(anchors);del bad['sb1.flags']
        with self.assertRaises(ValueError):layout.validate_anchors(bad)
    def test_actual_clang_candidate_cannot_pass_as_production_layout(self):
        candidate={'sb1.bytes':15488,'sb2.bytes':3872,'storage.bytes':33744,
            'object.bytes':40,'object_template.bytes':24,'sb1.flags':4744,
            'sb1.vars':5044,'sb1.unused_3598':0,'sb2.encryptionKey':172,
            'sb2.follower':3848,'storage.boxes':4,'sb1.roamer':0,
            'sb1.playerPartyCount':564,'sb1.objectEventTemplates':0}
        with self.assertRaises(ValueError):layout.validate_anchors(candidate)
    def test_snapshot_ignores_equal_width_dirty_header_and_untracked_inputs(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp) / 'repo'; repo.mkdir()
            def git(*args):
                return subprocess.check_output(['git', '-C', str(repo), *args])
            git('init', '-q')
            header = repo / 'include/global.h'; header.parent.mkdir()
            original = b'struct Mail { unsigned short species; unsigned short itemId; };\n'
            header.write_bytes(original)
            git('add', 'include/global.h')
            git('-c', 'user.name=fixture', '-c', 'user.email=fixture@example.invalid',
                'commit', '-qm', 'pinned layout input')
            tree = git('rev-parse', 'HEAD^{tree}').decode().strip()
            header.write_bytes(original.replace(b'species; unsigned short itemId',
                b'itemId; unsigned short species'))
            (repo / 'include/untracked.h').write_text('wrong input', encoding='utf-8')
            destination = Path(temp) / 'snapshot'
            hashes = layout.export_tree(repo, tree, destination)
            self.assertEqual((destination / 'include/global.h').read_bytes(), original)
            self.assertFalse((destination / 'include/untracked.h').exists())
            self.assertEqual(set(hashes), {'include/global.h'})
            with self.assertRaises(ValueError):
                layout.export_tree(repo, tree, destination)

    def test_compiler_snapshot_ignores_dirty_tracked_source_and_stale_binary(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp) / 'compiler'; repo.mkdir()
            def git(*args):
                return subprocess.check_output(['git', '-C', str(repo), *args])
            git('init', '-q')
            gcc = repo / 'gcc'; gcc.mkdir()
            (gcc / 'Makefile').write_bytes(b'all:; echo pinned-source\n')
            git('add', 'gcc/Makefile')
            git('-c', 'user.name=fixture', '-c', 'user.email=fixture@example.invalid',
                'commit', '-qm', 'compiler source')
            tree = git('rev-parse', 'HEAD^{tree}').decode().strip()
            (gcc / 'Makefile').write_bytes(b'all:; echo dirty-source\n')
            (gcc / 'agbcc').write_bytes(b'stale executable')
            snapshot = Path(temp) / 'fresh-compiler'
            layout.export_tree(repo, tree, snapshot)
            self.assertEqual((snapshot / 'gcc/Makefile').read_bytes(), b'all:; echo pinned-source\n')
            self.assertFalse((snapshot / 'gcc/agbcc').exists())

    def test_source_pin_and_array_covers_new_game_fields(self):
        self.assertTrue(set(layout.ANCHORS)<=set(layout.EXPRESSIONS))
        self.assertEqual(layout.AGBCC_PIN,'da598c1d918402c42c0c0d7128ba14567f3175e9')
        required={'sb1.dewfordTrends','sb1.oldMan','sb1.mail','sb1.lilycoveLady',
            'sb1.berryTrees','sb2.hallRecords1P','sb2.hallRecords2P','sb2.playerApprentice',
            'storage.boxNames','storage.boxWallpapers'}
        self.assertTrue(required<=set(layout.EXPRESSIONS))
if __name__=='__main__':unittest.main()
