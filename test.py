#!/usr/bin/env python3
"""
Run tests to verify correctness, memory safety, etc.
"""
import argparse
import difflib
import glob
import os
import re
import subprocess
import sys

ERROR_EXITCODE = 211


def get_expected_code_and_output(script):
    reading_output = False
    output = ''
    output_re = re.compile(r'; OUTPUT\((\d+)\)')
    with open(script, 'r') as f:
        for line in f:
            if reading_output:
                output += line[line.index('; ') + 2:]
            elif output_re.match(line):
                code = int(output_re.match(line).group(1))
                reading_output = True

    return code, output


def run_test_script(script, runner):
    command = [
        'valgrind',
        '-q',
        '--error-exitcode={}'.format(ERROR_EXITCODE),
        runner,
        script,
    ]
    proc = subprocess.Popen(
        command, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    return proc.returncode, stdout, stderr


def test_case(script, runner):
    print('[{}] {}: '.format(runner, script), end='')
    sys.stdout.flush()

    exp_code, exp_stdout = get_expected_code_and_output(script)
    assert exp_code != ERROR_EXITCODE
    act_code, act_stdout, act_stderr = run_test_script(script, runner)
    act_stdout = act_stdout.decode('utf-8')
    act_stderr = act_stderr.decode('utf-8')

    if act_code == ERROR_EXITCODE:
        print('MEM ERROR')
        print('## STDOUT')
        print(act_stdout)
        print('## STDERR')
        print(act_stderr)
        return False
    elif act_code != exp_code:
        print('FAIL (returned {}, expected {})'.format(
            act_code, exp_code))
        print('## STDOUT')
        print(act_stdout)
        print('## STDERR')
        print(act_stderr)
        return False
    elif act_stdout.strip() != exp_stdout.strip():
        print('FAIL (incorrect output)')
        exp_lines = [l + '\n' for l in exp_stdout.split('\n')]
        act_lines = [l + '\n' for l in act_stdout.split('\n')]
        for line in difflib.unified_diff(exp_lines, act_lines):
            sys.stdout.write(line)
        return False
    else:
        print('PASS')
        return True


def run_tests(test_files, runner):
    for script in test_files:
        if not test_case(script, runner):
            sys.exit(1)


if __name__ == '__main__':
    ap = argparse.ArgumentParser('run tests for funlisp')
    ap.add_argument('directory', help='directory of tests to run')
    ap.add_argument('--runner', '-r', help='binary to run with',
        default='bin/funlisp')
    args = ap.parse_args()
    run_tests(
        glob.glob(os.path.join(args.directory, '*.lisp')),
        args.runner)
