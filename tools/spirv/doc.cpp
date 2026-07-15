// Copyright 2014-2015 LunarG, Inc.
// SPDX-License-Identifier: Apache-2.0

//
// Author: John Kessenich, LunarG
//

//
// 1) Programmatically fill in instruction/operand information.
//    This can be used for disassembly, printing documentation, etc.
//
// 2) Print documentation from this parameterization.
//

#include <assert.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <utility>

#include "jsoncpp/dist/json/json.h"

#include "doc.h"
#include "unified1/spirv.hpp"

// We need CMake to provide us the absolute path to the JSON grammar file.
#ifndef SPIRV_JSON_GRAMMAR_PATH
  #error "SPIRV_JSON_GRAMMAR_PATH needs to be defined for SPIR-V grammar."
#endif

namespace spv {

// Set up all the parameterizing descriptions of the opcodes, operands, etc.
void Parameterize()
{
    // only do this once.
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    for (int operand_source = OperandSource; operand_source < OperandOpcode; ++operand_source) {
        for (auto& enumValue : OperandClassParams[operand_source]) {
            for (int DescId = 0; DescId < enumValue.operands.getNum(); DescId++) {
                enumValue.operands.setDesc(DescId, EmphText(enumValue.operands.getDesc(DescId)));
            }
        }
    }

    for (auto& inst : InstructionDesc) {
        const unsigned int i = inst.value; // The opcode
        auto& operands = inst.operands;
        for (int DescId = 0; DescId < operands.getNum(); DescId++) {
            operands.setDesc(DescId, EmphText(operands.getDesc(DescId)));
        }
        if (i == OpSpecConstantOp) {
            // For OpSpecConstantOp, the grammar uses "LiteralSpecConstantOpInteger" as the last
            // operand to govern the expansion of following operands. In the spec, we list out
            // those additional operands.
            operands.push(OperandVariableIds, "_Operands_", false);
        } else if (i == OpDecorate || i == OpMemberDecorate) {
            // Similarly for Op*Decorate, the grammar uses "Decoration" as the last operand
            // to govern the expansion of following operands. In the spec, we list out
            // those additional operands.
            operands.push(OperandVariableLiterals, "See <<Decoration,_Decoration_>>.", false);
        } else if (i == OpDecorateString || i == OpMemberDecorateString) {
            operands.push(OperandLiteralString, "See <<Decoration,_Decoration_>>.", false);
            operands.push(OperandOptionalLiteralStrings, "See <<Decoration,_Decoration_>>.", false);
        } else if (i == OpDecorateId || i == OpMemberDecorateIdEXT) {
            // this time with <id> instead of literals
            operands.push(OperandVariableIds, "See <<Decoration,_Decoration_>>.", false);
        } else if (i == OpExecutionMode) {
            // Similarly for OpExecutionMode, the grammar uses "ExecutionMode" with literals.
            operands.push(OperandVariableLiterals, "See <<Execution_Mode,_Execution Mode_>>", false);
        } else if (i == OpExecutionModeId) {
            // Similarly for OpExecutionModeId, the grammar uses "ExecutionMode" with <id>s.
            operands.push(OperandVariableIds, "See <<Execution_Mode,_Execution Mode_>>", false);
        } else if (i == OpLoopMerge) {
            // Similarly for OpExecutionMode, the grammar uses "LoopControl".
            operands.push(OperandVariableLiterals, "_Loop Control Parameters_", false);
        } else {
            const int num_operands = operands.getNum();
            if (num_operands == 0) continue;
            if (operands.getClass(num_operands - 1) == OperandImageOperands) {
                // Similarly for image related instructions. The last operands for them in the
                // grammar is "ImageOperands", which governs the expansion of following operands.
                if (operands.isOptional(num_operands - 1)) {
                    operands.push(OperandVariableIds, "", true);
                } else {
                    operands.push(OperandId, "", false);
                    operands.push(OperandVariableIds, "", true);
                }
            }
        }
    }
}

// Add emphasis to text
std::string EmphText(const char* desc) {
    if (!*desc)
        return "";
    return "_" + std::string{desc} + "_";
}

}; // end spv namespace
