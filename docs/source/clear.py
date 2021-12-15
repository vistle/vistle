# -*- coding: utf-8 -*-
"""
Module for deleting files or directories
"""

import os
from shutil import rmtree
from pathlib import Path

ERR_STR = "Error: {path} : {err_mss}"


def deleteDir(path):
    """Delete complete directory.

    Keyword arguments:
    path -- string to directory
    """
    try:
        rmtree(path)
        print("Cleared " + path + "!")
    except OSError as e:
        print(ERR_STR.format(path=path, err_mss=e.strerror))


def deleteFilesInDir(path, exclude=[], pattern='*'):
    """
    Delete all files which matching the pattern in path and exclude given filenames.

    Keyword arguments:
    path    -- string to directory
    exclude -- list of files
    pattern -- regex pattern
    """
    for file in Path(path).glob(pattern):
        if not any(Path(file).name == eF for eF in exclude):
            try:
                file.unlink()
            except OSError as e:
                print(ERR_STR.format(path=path, err_mss=e.strerror))


if __name__ == "__main__":
    # some tests
    # deleteDir("/home/mdjur/program/vistle/docs/build")
    # deleteFilesInDir("/home/mdjur/program/vistle/docs/source/lib", "message_link.md libsim_link.md".split(), "*.md")
    # deleteFilesInDir("/home/mdjur/program/vistle/docs/source/lib", ["index.rst"])
    deleteFilesInDir("/home/mdjur/program/vistle/docs/source/lib")
