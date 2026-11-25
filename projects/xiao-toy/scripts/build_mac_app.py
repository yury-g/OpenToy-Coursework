#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build a native macOS launcher using the installed pinned Python runtime.
Alias packaging is intentionally local: keep the project and runtime paths.
"""
import sys,tempfile,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='xiao-app-') as scratch:
 p=Path(scratch);setup=p/'setup.py'
 setup.write_text('from setuptools import setup\nsetup(app='+repr([str(ROOT/'scripts/mac_app.py')])+',options={"py2app":{"argv_emulation":False,"plist":{"CFBundleName":"OpenToy Control","CFBundleIdentifier":"org.opentoy.maccontroller","NSBluetoothAlwaysUsageDescription":"Connect to the XIAO toy for live controls and firmware updates."}}},setup_requires=["py2app"])\n')
 subprocess.run([sys.executable,str(setup),'py2app','--alias','--dist-dir',str(ROOT.parent),'--bdist-base',str(p/'build')],cwd=p,check=True)
print(ROOT.parent/'OpenToy Control.app')
