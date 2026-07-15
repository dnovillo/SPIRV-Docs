# Copyright 2011-2026 The Khronos Group, Inc.
# SPDX-License-Identifier: Apache-2.0

# Main SPIR-V Specification makefile
# it contain rules to build from scratch the specs:
#   - build SPIR-V headers generator and generate new headers
#   - build the spec generator
#   - generate specs: html or pdf
#
# there is a generic rule to run other rules inside
# the docker container containing all required tools.
#
# make html
# make docker-html

.ONESHELL:

all: all-specs

#
QUIET    ?= @
PYTHON   ?= python3
ASCIIDOC ?= asciidoctor
# Using such variables allows running local version with:
# BUNDLE_GEMFILE=.../asciidoctor-pdf/Gemfile make QUIET= ASCIIDOCPDF="bundle exec asciidoctor-pdf" pdf
ASCIIDOCPDF ?= asciidoctor-pdf
RUBY      = ruby
NODEJS    = node
PATCH     = patch
RM        = rm -f
RMRF      = rm -rf
MKDIR     = mkdir -p
CP        = cp
ECHO      = echo

# Path to Python scripts used in generation
SCRIPTS  = scripts

# Compute the absolute directory name from the location of this Makefile
# so that we can compile from anywhere even if we use make -f
# <this_makefile>:
SPIR_DIR := $(abspath $(dir $(firstword $(MAKEFILE_LIST))))

# SPIR-V header paths

SPIRVHeader_DIR = $(SPIR_DIR)/SPIRV-Headers
SPIRVHeaderTool_DIR = $(SPIRVHeader_DIR)/tools/buildHeaders
SPIRVHeaderInclude_DIR = $(SPIRVHeader_DIR)/include/spirv/unified1
SPIRVHeaderBUILD_DIR = $(SPIRVHeaderTool_DIR)/build
BuildSpvHeaders = $(SPIRVHeaderBUILD_DIR)/install/bin/buildSpvHeaders

# SPIR-V specs paths

SPIRVSpecTool_DIR = $(SPIR_DIR)/tools/spirv
SPIRVSpecToolBUILD_DIR = $(SPIRVSpecTool_DIR)/build
BuildSPIRV = $(SPIRVSpecToolBUILD_DIR)/install/bin/spirv

SPIRVSpec_DIR = $(SPIR_DIR)/specs

header: $(SPIRVHeaderInclude_DIR)/spirv.hpp

# build / run SPIR-V headers

$(SPIRVHeaderInclude_DIR)/spirv.hpp: $(SPIRVHeaderInclude_DIR)/spirv.core.grammar.json $(BuildSpvHeaders)
	cd $(SPIRVHeaderInclude_DIR)
	$(BuildSpvHeaders) -H $(SPIRVHeaderInclude_DIR)/spirv.core.grammar.json

$(SPIRVHeaderBUILD_DIR):
	mkdir -p $(SPIRVHeaderBUILD_DIR)
	cd $(SPIRVHeaderBUILD_DIR)
	cmake ../ -DCMAKE_INSTALL_PREFIX=install

$(BuildSpvHeaders): $(SPIRVHeaderBUILD_DIR)
	cd $(SPIRVHeaderBUILD_DIR)
	make
	make install

build-SpvHeaders: $(BuildSpvHeaders)

# build spec printer

$(SPIRVSpecToolBUILD_DIR): header
	mkdir -p $(SPIRVSpecToolBUILD_DIR)
	cd $(SPIRVSpecToolBUILD_DIR)
	cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=install

$(BuildSPIRV): $(SPIRVSpecToolBUILD_DIR)
	cd $(SPIRVSpecToolBUILD_DIR)
	make
	make install

build-printspec: $(BuildSPIRV)

# build spec

html: build-printspec
	cd $(SPIRVSpec_DIR)
	make html

check:
	cd $(SPIRVSpec_DIR)
	make check

all-specs: build-printspec
	cd $(SPIRVSpec_DIR)
	make all

clean:
	rm -Rf $(SPIRVSpecToolBUILD_DIR)
	rm -Rf $(SPIRVHeaderBUILD_DIR)
	rm -Rf diff
	rm -Rf rel-diff
	cd $(SPIRVSpec_DIR) && make clean


# Expose docker-TARGET to forward a TARGET inside a Docker container.
# For example:
#   make docker-clean docker-html docker-pdf
# Also useful to have a shell inside docker:
#   make docker-bash
docker-%:
	# Run with current user and group id the published AsciiDoctor
	# capable Khronos docker image with current SYCL specification
	# directory mounted in /sycl
	# Re-set MAKEFLAGS to pass variables to the inner make since
	# variables are dropped by docker.
	docker run --user `id --user`:`id --group` \
	  --interactive --tty --rm \
	  --volume $(SPIR_DIR):$(SPIR_DIR) -w $(SPIR_DIR) \
	  khronosgroup/docker-images:asciidoctor-spec \
	  $(MAKE) MAKEFLAGS="$(MAKEFLAGS)" $*

SPECS_HTML = $(wildcard specs/*.html)
DIFF_HTML = $(patsubst specs/%.html,diff/%.html,$(SPECS_HTML))
DIFF_RELEASE_HTML = $(patsubst specs/%.html,rel-diff/%.html,$(SPECS_HTML))

diff/%.html:
	mkdir -p diff
	tools/htmldiff/htmldiff latest/$*.html specs/$*.html > diff/$*-diff.html
rel-diff/%.html:
	mkdir -p rel-diff
	tools/htmldiff/htmldiff ../Registry-Root-SPIR-V/specs/unified1/$*.html specs/$*.html > rel-diff/$*-diff.html

# OpenCL.std.html is published to Registry-Root-SPIR-V under a different name.
rel-diff/OpenCL.std.html:
	mkdir -p rel-diff
	tools/htmldiff/htmldiff ../Registry-Root-SPIR-V/specs/unified1/OpenCL.ExtendedInstructionSet.100.html specs/OpenCL.std.html > rel-diff/OpenCL.std-diff.html

diff-dev: $(DIFF_HTML)

diff-rel: $(DIFF_RELEASE_HTML)


# Use a default rule to just execute by the shell the given rule.
# For example "make bash" will run bash, "make env" will display the
# environment to debug the configuration, etc.
# Mainly to be used in docker context.
.DEFAULT:
	$@
