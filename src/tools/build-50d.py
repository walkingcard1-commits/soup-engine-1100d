#!/usr/bin/env python3
"""The 50D build, kept under its old name. Same as: build-ml.py --model 50D"""
import os
import sys

here = os.path.dirname(os.path.abspath(__file__))
os.execv(sys.executable, [sys.executable, os.path.join(here, "build-ml.py"),
                          "--model", "50D"] + sys.argv[1:])
