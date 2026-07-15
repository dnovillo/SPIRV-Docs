// Copyright 2014-2015 LunarG, Inc.
// SPDX-License-Identifier: Apache-2.0

//
// Author: John Kessenich, LunarG
//

//
// Parameterize the SPIR-V enumerants.
//

#include <string.h>

#include "buildHeaders/jsonToSpirv.h"

namespace spv {

// Fill in all the parameters
void Parameterize();

// Return the English names of all the enums.
const char* SourceString(int);
const char* AddressingString(int);
const char* MemoryString(int);
const char* ExecutionModelString(int);
const char* ExecutionModeString(int);
const char* StorageClassString(int);
const char* DecorationString(int);
const char* BuiltInString(int);
const char* DimensionString(int);
const char* SelectControlString(int);
const char* LoopControlString(int);
const char* FunctionControlString(int);
const char* SamplerAddressingModeString(int);
const char* SamplerFilterModeString(int);
const char* ImageFormatString(int);
const char* ImageChannelOrderString(int);
const char* ImageChannelTypeString(int);
const char* ImageOperands(int);
const char* FPFastMathString(int);
const char* FPRoundingModeString(int);
const char* LinkageTypeString(int);
const char* FuncParamAttrString(int);
const char* AccessQualifierString(int);
const char* MemorySemanticsString(int);
const char* MemoryAccessString(int);
const char* ExecutionScopeString(int);
const char* GroupOperationString(int);
const char* KernelEnqueueFlagsString(int);
const char* KernelProfilingInfoString(int);
const char* CapabilityString(int);
const char* OpcodeString(int);

const char* GetOperandDesc(OperandClass operand);

// Print an immediate row, using explicit value and name, where the overall
// table may have a capability column, an extenions column, and where the enum
// value might have to be printed in hex.
void PrintImmediateRow(int imm, const char* name, bool caps_column = false, bool hex = false);
// Print an immediate row for the given enum, where the overall table may have a
// capability column, an extenions column, and where the enum value might have to be
// printed in hex.
void PrintImmediateRow(const EnumValue& e, bool caps_column, bool hex = false);
const char* AccessQualifierString(int attr);

// Print the remainder of a row as operands, including trailing newline.
void PrintOperands(const OperandParameters& operands, int reservedOperands);

std::string EmphText(const char* desc);

};  // end namespace spv
