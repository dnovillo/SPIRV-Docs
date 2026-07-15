// Copyright 2014-2026 LunarG, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <string>
#include <cctype>
#include <algorithm>

// Extended instruction sets
namespace spv {
    // Include C-based headers that don't have a namespace
    #include "unified1/GLSL.std.450.h"
}
#include "unified1/OpenCL.std.h"
#include "unified1/spirv.hpp"

// This tool's includes
#include "buildHeaders/jsonToSpirv.h"
#include "doc.h"
#include "printSpec.h"
#include "GLSLDoc.h"
#include "OclDoc.h"

const char* OpenCL20DebugNames[spv::OclExtInstCeiling];

// Command-line options
enum TOptions {
    EOptionNone                       = 0x000,
    EOptionPrintAsciidoc              = 0x001,
    EOptionPrintGLSLBuiltInsAsciidoc  = 0x002,
    EOptionPrintHeader                = 0x008,
    EOptionPrintOclBuiltinsAsciidoc   = 0x010,
};

std::string Filename;
int Options;

void Usage()
{
    printf("Usage: spirv option [file]\n"
           "\n"
           "  -p print documentation\n"
           "  -s [version] prints the SPIR-V extended instructions documentation\n"
           "      'GLSL': GLSL std450 extended instructions documentation\n"
           "      'OpenCL': OpenCL extended instructions documentation\n"
           );
}

std::string tolower_s(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

bool ProcessArguments(int argc, char* argv[])
{
    argc--;
    argv++;
    for (; argc >= 1; argc--, argv++) {
        if (argv[0][0] == '-') {
            switch (argv[0][1]) {
            case 'p':
                Options |= EOptionPrintAsciidoc;
                break;
            case 's': {
                if (argc < 2) {
                    return false;
                }
                argc--; argv++;
                std::string version(argv[0]);
                if (version.compare("OpenCL") == 0) {
                    Options |= EOptionPrintOclBuiltinsAsciidoc;
                }
                else if (version.compare("GLSL") == 0) {
                    Options |= EOptionPrintGLSLBuiltInsAsciidoc;
                }
                else {
                    return false;
                }
                break;
            }
            default:
                return false;
            }
        } else {
            Filename = std::string(argv[0]);
        }
    }

    return true;
}

int main(int argc, char* argv[])
{
    // The generated AsciiDoc is written to stdout, so this warning goes to
    // stderr to avoid corrupting the output. The SPIR Working Group intends
    // to remove this generator after the spec source migrates to GitHub.
    fprintf(stderr,
            "warning: this SPIR-V spec generator is deprecated. The SPIR "
            "Working Group intends to remove it after the spec source migrates "
            "to GitHub (see SPIR-V issue #940).\n");

    if (argc < 2 || ! ProcessArguments(argc, argv)) {
        Usage();
        return 1;
    }

    spv::jsonToSpirv(Filename, (Options & EOptionPrintHeader) != 0);
    spv::Parameterize();

    if (Options & EOptionPrintAsciidoc)
        spv::PrintDoc();

    if (Options & EOptionPrintOclBuiltinsAsciidoc) {
        spv::OclGetNames(OpenCL20DebugNames);
        spv::PrintOclCommonDoc();
    }

    if (Options & EOptionPrintGLSLBuiltInsAsciidoc)
        spv::PrintGLSLDoc();

    return 0;
}
