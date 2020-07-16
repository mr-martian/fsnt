#!/usr/bin/env python3
import subprocess
import unittest
import tempfile
import shutil

class TestBase:
    src_path = '../src/tools/'
    def run_cmd(self, cmd, input_text=None):
        cmd2 = [TestBase.src_path + cmd[0]] + cmd[1:]
        return subprocess.check_output(cmd2, input=input_text, universal_newlines=True)
    def failed_command(self, cmd, input_text=None):
        with self.assertRaises(subprocess.CalledProcessError):
            self.run_cmd(cmd, input_text)
    def match_output(self, cmd, input_text=None, output_text=''):
        self.maxDiff = None
        actual = self.run_cmd(cmd, input_text)
        self.assertEqual(output_text, actual)
    def match_sorted_output(self, cmd, input_text=None, output_text=''):
        self.maxDiff = None
        actual = self.run_cmd(cmd, input_text)
        check = '\n'.join(sorted(actual.splitlines()))
        ref = '\n'.join(sorted(output_text.splitlines()))
        self.assertEqual(ref, check)
    def match_sorted_output_file(self, cmd, input_text='', output_text=''):
        it = None
        if input_text:
            with open(input_text) as txt:
                it = txt.read()
        ot = output_text
        if output_text:
            with open(output_text) as txt:
                ot = txt.read()
        self.match_sorted_output(cmd, it, ot)

class TestCompose(TestBase, unittest.TestCase):
    def compose(self, f1, f2, tapes, result_att=None, result_text=None):
        tmp = tempfile.mkdtemp()
        try:
            self.run_cmd(['fsnt-txt2fst', f1, tmp + '/f1.bin'])
            self.run_cmd(['fsnt-txt2fst', f2, tmp + '/f2.bin'])
            cmd = ['fsnt-compose']
            for t1, t2 in tapes:
                cmd += ['-g', t1, t2]
            self.run_cmd(cmd + [tmp + '/f1.bin', tmp + '/f2.bin', tmp + '/out.bin'])
            if result_att:
                self.match_sorted_output_file(['fsnt-fst2txt', tmp + '/out.bin'], output_text=result_att)
            if result_text:
                self.match_sorted_output_file(['fsnt-expand', tmp + '/out.bin'], output_text=result_text)
        finally:
            shutil.rmtree(tmp)
    def test_deterministic_identity(self):
        self.compose('compose/lex_simple_identity.att', 'compose/rule_simple_identity.att',
                     [('lex_out', 'rule_in')], result_att='compose/result_simple_identity.att',
                     result_text='compose/result_simple3.txt')
    def test_deterministic_identity_multitape(self):
        self.compose('compose/lex_simple_identity.att', 'compose/rule_simple_identity.att',
                     [('lex_in', 'rule_in'), ('lex_out', 'rule_out')],
                     result_att='compose/result_simple_identity_multi.att',
                     result_text='compose/result_simple2.txt')

if __name__ == '__main__':
    unittest.main(buffer=True, verbosity=2)
