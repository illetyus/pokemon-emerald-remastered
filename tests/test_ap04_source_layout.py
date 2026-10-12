import importlib.util
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ap04_source_layout',ROOT/'tools/ap04_source_layout.py')
layout=importlib.util.module_from_spec(spec);sys.modules[spec.name]=layout;spec.loader.exec_module(layout)
class SourceLayoutTests(unittest.TestCase):
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
    def test_source_pin_and_array_covers_new_game_fields(self):
        self.assertTrue(set(layout.ANCHORS)<=set(layout.EXPRESSIONS))
        self.assertEqual(layout.AGBCC_PIN,'da598c1d918402c42c0c0d7128ba14567f3175e9')
        required={'sb1.dewfordTrends','sb1.oldMan','sb1.mail','sb1.lilycoveLady',
            'sb1.berryTrees','sb2.hallRecords1P','sb2.hallRecords2P','sb2.playerApprentice',
            'storage.boxNames','storage.boxWallpapers'}
        self.assertTrue(required<=set(layout.EXPRESSIONS))
if __name__=='__main__':unittest.main()
