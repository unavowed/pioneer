## Process this file with automake to produce Makefile.in
SUBDIRS = contrib src data osx win32

include $(top_srcdir)/Makefile.common

.PHONY: codedoc
codedoc:
	@naturaldocs -i src/ -o HTML codedoc/ -p nd/ -do -s Default Local

EXTRA_DIST = \
	AUTHORS.txt \
	COMPILING.OSX.txt \
	COPYING.txt \
	Changelog.txt \
	Modelviewer.txt \
	Quickstart.txt \
	README.txt \
	bootstrap \
	generic-exec \
	Makefile.common \
	modelviewer \
	pioneer
