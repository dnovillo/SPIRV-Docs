Copyright 2015-2026 The Khronos Group Inc.
SPDX-License-Identifier: CC-BY-4.0

This file documents the JSON format describing SPIR-V data, and the
per-language output created from the JSON data.

0. Version History

   1.0 Initial version

   2.0 Per suggestions:
       - Added overloaded operator| to C++ and C++11 headers for mask types.
       - Removed name redundancy in C++11, Lua, and Python.  E.g,
          RelaxedShift -> Relaxed ("Shift" is already given in the enum class name).
       - "meta" node is printed first
       - comments are stored a line at a time in the JSON
       - JSON enum values are printed in numerical order

1. JSON Specifics

1.1 High Level Structure

    The JSON for SPIR-V data lies entirely under a root collection containing
    a single association named "spv", all lower case, itself containing several
    other containers:

    {
        "spv":
        {
            "meta":
            {
                ...
            },
            "enum":
            [
                ...
            ]
        }
    }

    There are currently two containers in "spv", though others will be added
    in the future to describe, for example, operand details.  The currently
    defined containers are:

    "enum" - This is an ordered list of SPIR-V enumerant descriptions, the
             format of which is described below.

    "meta" - This is an unordered (associative) container of metadata,
             containing information such as magic numbers, versions, and
             copyrights.

1.2 "enum" container

    The enum container is an ordered list of entries describing SPIR-V
    enumerant values.  Each entry is itself an associative container with the
    following fields:

            {
                "Name": "AddressingModel",
                "Type": "Value",
                "Values":
                {
                    "Logical": 0,
                    "Physical32": 1,
                    "Physical64": 2
                }
            },

    The fields are as follows:

    "Name" - A text string containing the enumerant name: "AddressingModel" in
             the example above.

    "Type" - Must be either "Value", for a collection of values, or "Bit", for
             bitmasks and shifts.

    "Values" - an associative container providing the value of each possible
             enumerant value.  For the "Bit" type, this is a bit number,
             which will be shifted appropriately to form a bitmask.

1.3 "meta" container

    "meta" is an associative container with the following entries:


     "Comment" - this is an array of arrays of comment lines, including copyrights.
            There is one entry in the inner array per line.

     "MagicNumber" - the SPIR-V header magic number.

     "OpCodeMask" - mask value for extracting opcodes from a SPIR-V word.

     "WordCountShift" - shift value for extracting the instruction word count
             from a SPIR-V word.

     "Version" - SPIR-V specification version.

     "Revision" - SPIR-V specification revision.


1.4 TODO: operand values and semantic descriptions


2.0 Python specifics

    The python module produced from the JSON data is meant to be used as
    follows, given "spirv.py" in the python search path:

        >>> from spirv import spv

    This will provide a "spv" dictionary which can be used as follows:

        spv['SourceLanguage']['GLSL']


3.0 Lua Specifics

    There are multiple methods available to load a Lua module, any of which
    can be used:

        dofile("spirv.lua")

    This will produce a Lua table named "spv", which can be used as follows:

        spv.SourceLanguage.GLSL

4.0 C99 Specifics

    The C header can be #included:

        #include "spirv.h"

    In C there are no namespaces, so "Spv" is prepended to symbols.  For
    example:

        SpvSourceLanguageGLSL

5.0 C++03 Specifics

    The C++03 header can be #included:

        #include "spirv.hpp"

    In C++ the data is included in the "spv" namespace, and each enumerant
    value resides in that namespace, so is disambiguated with the enumeration
    name:

        spv::SourceLanguageGLSL

6.0 C++11 Specifics

    The C++11 header can be #included:

        #include "spirv.hpp11"

    The C++11 header uses enum classes to separate the namespaces of each
    enumeration.  It can be used as follows:

        spv::SourceLanguage::GLSL

