#!/usr/bin/env python

import subprocess
import sys
import os
import tempfile
from pathlib import Path
from argparse import ArgumentParser

PYTHON = sys.executable
IDF_PATH = os.environ.get('IDF_PATH')
IDF_PYTHON_ENV_PATH = os.environ.get('IDF_PYTHON_ENV_PATH')
IDF_COMPONENTS = Path(IDF_PATH, 'components')
ESPTOOL_PY = Path(IDF_COMPONENTS, 'esptool_py', 'esptool', 'esptool.py')
PARTTOOL_PY = Path(IDF_COMPONENTS, 'partition_table', 'parttool.py')
NVS_PARTITION_GEN_PY = Path(IDF_COMPONENTS, 'nvs_flash', 'nvs_partition_generator', 'nvs_partition_gen.py')

PROJECT_DIR = Path(__file__, '../..').resolve()


def get_project_version():
    with open(Path(PROJECT_DIR, 'version.txt')) as f:
        return f.read().strip()


def get_project_name():
    return os.path.basename(PROJECT_DIR)


def main():
    if IDF_PATH is None or not Path(IDF_PATH).exists():
        print('IDF_PATH is not set', file=sys.stderr)
        sys.exit(1)

    if IDF_PYTHON_ENV_PATH is None or not Path(IDF_PYTHON_ENV_PATH).exists():
        print('IDF_PYTHON_ENV_PATH is not set', file=sys.stderr)
        sys.exit(1)

    if not PYTHON.startswith(IDF_PYTHON_ENV_PATH):
        print('python interpreter is not being run from esp-idf\'s python env', file=sys.stderr)
        print('please, make sure you have idf loaded in your shell', file=sys.stderr)
        sys.exit(1)

    args_parser = ArgumentParser()
    args_parser.add_argument('nvs_config', help='path to nvs csv file')

    mode_args_group = args_parser.add_mutually_exclusive_group(required=True)
    mode_args_group.add_argument('-o', '--output', help='path to output file')
    mode_args_group.add_argument('--flash', help='flash the config', action='store_true')

    args = args_parser.parse_args()

    nvs_partition_file = tempfile.NamedTemporaryFile(suffix='.bin') if args.flash else Path(args.output)

    try:
        subprocess.check_call([PYTHON, NVS_PARTITION_GEN_PY, 'generate', args.nvs_config, nvs_partition_file.name, "0x4000"])
        print('successfully generated nvs partition at {}'.format(nvs_partition_file.name))
    except subprocess.CalledProcessError as e:
        print('failed to generate nvs partition', file=sys.stderr)
        sys.exit(e.returncode)

    if args.flash:
        try:
            subprocess.check_call([PYTHON, PARTTOOL_PY, 'write_partition', '--partition-name=nvs', '--input', nvs_partition_file.name])
            print('successfully written nvs data to the connected panel')
        except subprocess.CalledProcessError as e:
            print('failed to write nvs partition')
            sys.exit(e.returncode)


if __name__ == '__main__':
    main()
