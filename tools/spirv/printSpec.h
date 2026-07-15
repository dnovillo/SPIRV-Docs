// Copyright 2014-2026 LunarG, Inc.
// SPDX-License-Identifier: Apache-2.0

//
// Author: John Kessenich, LunarG
//

//
// 1) Return English versions of instruction/operand information.
//    This can be used for disassembly, printing documentation, etc.
//
// 2) Print documentation.
//

#include <vector>

namespace spv {

// Fill in all the parameters of the instruction set
void ParameterizeSpec();

// Print out the documentation.
void PrintDoc();

};  // end namespace spv
