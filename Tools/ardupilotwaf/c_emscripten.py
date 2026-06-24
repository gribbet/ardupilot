#!/usr/bin/env python
# -*- coding: utf-8 vi:ts=4:noexpandtab
#
# Patched copy of modules/waf/waflib/extras/c_emscripten.py.
# Kept in Tools/ardupilotwaf/ so the submodule is left unmodified.
#
# Changes vs upstream:
#   - get_emscripten_version: accept __EMSCRIPTEN__ as well as EMSCRIPTEN
#     (newer Emscripten toolchains no longer define the bare EMSCRIPTEN macro)
#   - cstlib_PATTERN / cxxstlib_PATTERN: use lib%s.a (POSIX convention)

import subprocess, sys, re

from waflib.Tools import ccroot, gcc, gxx
from waflib.Configure import conf
from waflib.Tools.compiler_c import c_compiler
from waflib.Tools.compiler_cxx import cxx_compiler

for supported_os in ('linux', 'darwin', 'gnu', 'aix'):
	c_compiler[supported_os].append('c_emscripten')
	cxx_compiler[supported_os].append('c_emscripten')


@conf
def get_emscripten_version(conf, cc):
	"""
	Detect Emscripten version via emcc --version.
	Sets DEST_OS/DEST_BINFMT/DEST_CPU/CC_VERSION on the env.
	"""
	cmd = cc + ['--version']
	try:
		p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		out, err = p.communicate()
	except Exception as e:
		conf.fatal('Could not determine emscripten version %r: %s' % (cmd, e))

	output = (out + err).decode(sys.stdout.encoding or 'latin-1', errors='replace')
	if 'emscripten' not in output.lower():
		conf.fatal('Could not determine the emscripten compiler version.')

	conf.env.DEST_OS = 'generic'
	conf.env.DEST_BINFMT = 'elf'
	conf.env.DEST_CPU = 'asm-js'
	m = re.search(r'(\d+)\.(\d+)\.(\d+)', output)
	conf.env.CC_VERSION = (m.group(1), m.group(2), m.group(3)) if m else ('0', '0', '0')

@conf
def find_emscripten(conf):
	cc = conf.find_program(['emcc'], var='CC')
	conf.get_emscripten_version(cc)
	conf.env.CC = cc
	conf.env.CC_NAME = 'emscripten'
	cxx = conf.find_program(['em++'], var='CXX')
	conf.env.CXX = cxx
	conf.env.CXX_NAME = 'emscripten'
	conf.find_program(['emar'], var='AR')

def configure(conf):
	conf.find_emscripten()
	conf.find_ar()
	conf.gcc_common_flags()
	conf.gxx_common_flags()
	conf.cc_load_tools()
	conf.cc_add_flags()
	conf.cxx_load_tools()
	conf.cxx_add_flags()
	conf.link_add_flags()
	conf.env.ARFLAGS = ['rcs']
	conf.env.cshlib_PATTERN = '%s.js'
	conf.env.cxxshlib_PATTERN = '%s.js'
	conf.env.cstlib_PATTERN = 'lib%s.a'
	conf.env.cxxstlib_PATTERN = 'lib%s.a'
	conf.env.cprogram_PATTERN = '%s.html'
	conf.env.cxxprogram_PATTERN = '%s.html'
	conf.env.CXX_TGT_F           = ['-c', '-o', '']
	conf.env.CC_TGT_F            = ['-c', '-o', '']
	conf.env.CXXLNK_TGT_F        = ['-o', '']
	conf.env.CCLNK_TGT_F         = ['-o', '']
