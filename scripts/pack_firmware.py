#!/usr/bin/env python

import subprocess
import sys
import os
import re
from pathlib import Path

PYTHON = sys.executable
IDF_PATH = os.environ.get('IDF_PATH')
IDF_PYTHON_ENV_PATH = os.environ.get('IDF_PYTHON_ENV_PATH')
IDF_COMPONENTS = Path(IDF_PATH, 'components')
ESPTOOL_PY = Path(IDF_COMPONENTS, 'esptool_py', 'esptool', 'esptool.py')
PROJECT_DIR = Path(__file__, '../..').resolve()


def get_project_version():
    with open(Path(PROJECT_DIR, 'version.txt')) as f:
        return f.read().strip()


def get_project_name():
    with open(Path(PROJECT_DIR, 'CMakeLists.txt')) as f:
        pattern = re.compile(r'^project\((.+)\)$', re.MULTILINE)
        contents = f.read()
        match = pattern.search(contents)

        if match is None:
            print('failed to find project name in CMakeLists.txt, using project basename', file=sys.stderr)
            return os.path.basename(PROJECT_DIR)
        else:
            return match.group(1)


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

    build_dir = Path(PROJECT_DIR, 'build')
    images_dir = Path(PROJECT_DIR, 'images')
    image_path = Path(images_dir, '{}-pack-{}.bin'.format(get_project_name(), get_project_version()))

    if not build_dir.exists():
        print('./build directory was not found, please run "idf.py build"', file=sys.stderr)
        sys.exit(1)

    try:
        subprocess.check_call([PYTHON, ESPTOOL_PY, '--chip', 'esp32', 'merge_bin', '-o', image_path, '@flash_args'], cwd=build_dir)
        print('firmware image created at {}'.format(image_path))
    except subprocess.CalledProcessError as e:
        print('failed to merge bin files', file=sys.stderr)
        sys.exit(e.returncode)


if __name__ == '__main__':
    main()
