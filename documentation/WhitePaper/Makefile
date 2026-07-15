# Copyright 2015-2026 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

AD_OPTIONS = -a asciimath --attribute stylesdir=./spirvStyle
DB_OPTIONS = -p dblatex/asciidoc-dblatex.xsl -s dblatex/asciidoc-dblatex.sty

all: WhitePaper.html WhitePaper.pdf

WhitePaper.html: WhitePaper.asciidoc *.png add.dis
	asciidoc $(AD_OPTIONS) --backend=html5 --out-file=WhitePaper.html WhitePaper.asciidoc

WhitePaper.pdf: WhitePaper.asciidoc *.png add.dis
	asciidoc $(AD_OPTIONS) --backend=docbook45 --out-file=WhitePaper.xml WhitePaper.asciidoc
	dblatex $(DB_OPTIONS) WhitePaper.xml
