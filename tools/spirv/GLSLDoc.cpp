/*
** Copyright 2014-2026 The Khronos Group Inc.
** SPDX-License-Identifier: MIT
*/

//
// Author: John Kessenich, LunarG
//
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>

// We need CMake to provide us the absolute path to the JSON grammar file.
#ifndef GLSL_JSON_GRAMMAR_PATH
  #error "GLSL_JSON_GRAMMAR_PATH needs to be defined for GLSL grammar."
#endif

namespace spv {
    // Include C-based headers that don't have a namespace
    #include "unified1/GLSL.std.450.h"
}

#include "jsoncpp/dist/json/json.h"

#include "doc.h"
#include "GLSLDoc.h"
#include "buildHeaders/jsonToSpirv.h"

#define LINE_BREAK "\n\n"
#define GAP "\n\n"
#define NOTE "*Note:* "
#define FRAGMENT_ONLY GAP " This instruction is only valid in the *Fragment* execution model."
#define INPUT_ONLY(op) GAP " The operand " #op " must be a pointer to the *Input* Storage Class."
#define FLOAT_OP(op) GAP " The operand " #op " must be a scalar or vector whose component type is floating-point."
#define FLOAT_OPS GAP " The operands must all be a scalar or vector whose component type is floating-point."
#define ALL_16_32_FLOAT(op) GAP " The operand " #op " must be a              scalar or vector whose component type is 16-bit or 32-bit floating-point."
#define ALL_32_FLOAT_P(op)  GAP " The operand " #op " must be a pointer to a scalar or vector whose component type is 32-bit floating-point."
#define ALL_MATCHING(op) GAP " _Result Type_ and the type of " #op " must be the same type."
#define ALL_MATCHING_POINTER(op) GAP " _Result Type_ and the type that " #op " points to must be the same type."
#define ALL_MATCHINGS GAP " _Result Type_ and the type of all operands must be the same type."
#define ALL_INT(op) GAP " _Result Type_ and the type of " #op " must both be integer scalar or integer vector types."
#define SAME_NUM_COMPS " _Result Type_ and operand types must have the same number of components with the same component width."
#define COMPONENT_WISE " Results are computed per component."
#define WIDTH_32_ONLY GAP "This instruction is currently limited to 32-bit width components."
#define NO_SIGNED_WRAP GAP "This instruction can be decorated with *NoSignedWrap*."

namespace spv {

std::string Names[GLSLstd450Count];

OperandParameters Parameters[GLSLstd450Count];
const char* Description[GLSLstd450Count];
std::string GLSLCapabilityReqirements[GLSLstd450Count];

void FillParameters()
{
    for (int d = 0; d < GLSLstd450Count; ++d) {
        Description[d] = 0;
        GLSLCapabilityReqirements[d] = "";
        Names[d] = "Unknown";
    }

    // Read the JSON grammar file.
    bool fileReadOk = false;
    std::string content;
    std::tie(fileReadOk, content) = ReadFile(GLSL_JSON_GRAMMAR_PATH);
    if (!fileReadOk) {
        std::cerr << "Failed to read JSON grammar file: "
                  << GLSL_JSON_GRAMMAR_PATH << std::endl;
        exit(1);
    }

    // Decode the JSON grammar file.
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(content, root)) {
        std::cerr << "Failed to parse JSON grammar:\n"
                  << reader.getFormattedErrorMessages();
        exit(1);
    }

    const Json::Value insts = root["instructions"];
    for (int i = 0; i < (int)insts.size(); ++i) {
        const unsigned int opcode = insts[i]["opcode"].asUInt();
        Names[opcode] = insts[i]["opname"].asString();

        // Get information about the operand lists and names.
        const Json::Value operands = insts[i]["operands"];
        for (int j = 0; j < (int)operands.size(); ++j) {
            const std::string kind = operands[j]["kind"].asString();
            const std::string doc = "_" + operands[j].get("name", "").asString() + "_";
            // It happens that all operands are <id> references in GLSL std450 instructions.
            assert(kind == "IdRef" && "non-IdRef operands found");
            Parameters[opcode].push(OperandId, doc);
        }

        // Get information about capabilities.
        const Json::Value caps = insts[i]["capabilities"];
        if (!caps.empty()) {
            assert(caps.isArray() && caps.size() == 1 && "instructions requiring more than one capability found");
            GLSLCapabilityReqirements[opcode] = GAP "Use of this instruction requires declaration of the *" + caps[0].asString() + "* capability.";
        }
    }

    Description[GLSLstd450Round] = "Result is the value equal to the nearest whole number to _x_."
        " The fraction 0.5 rounds in a direction chosen by the implementation, presumably the direction that is fastest."
        " This includes the possibility that *Round* _x_ is the same value as *RoundEven* _x_ for all values of _x_."
        GAP " round({plusmn}0) = {plusmn}0"
        GAP " round({plusmn}Inf) = {plusmn}Inf."
        FLOAT_OP(_x_)
        ALL_MATCHING(_x_)
        COMPONENT_WISE;

    Description[GLSLstd450RoundEven] = "Result is the value equal to the nearest whole number to _x_."
        " A fractional part of 0.5 rounds toward the nearest even whole number."
        " (Both 3.5 and 4.5 for _x_ round to 4.0.)"
        GAP " roundEven({plusmn}0) = {plusmn}0."
        GAP " roundEven({plusmn}Inf) = {plusmn}Inf."
        FLOAT_OP(_x_)
        ALL_MATCHING(_x_)
        COMPONENT_WISE;

    Description[GLSLstd450Trunc] = "Result is the value equal to the nearest whole number to _x_ whose absolute value is not larger than the absolute value of _x_."
                         GAP " trunc({plusmn}0) = {plusmn}0."
                         GAP " trunc({plusmn}Inf) = {plusmn}Inf."
                         FLOAT_OP(_x_)
                         ALL_MATCHING(_x_)
                         COMPONENT_WISE;

    Description[GLSLstd450FAbs] = "Result is _+0.0_ if _x_ is _{plusmn}0.0_, _x_ if _x > 0.0_, and _-x_ if _x < 0.0_."
                       GAP " fabs({plusmn}0) = +0.0."
                       GAP " fabs({plusmn}Inf) = {plusmn}Inf."
                       FLOAT_OP(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450SAbs] = "Result is _x_ if _x {ge} 0_; otherwise result is -_x_, where _x_ is interpreted as a signed integer."
                       ALL_INT(_x_)
                       SAME_NUM_COMPS
                       COMPONENT_WISE
                       NO_SIGNED_WRAP;

    Description[GLSLstd450FSign] = "Result is _1.0_ if _x > 0_, _-1.0_ if _x < 0_, _+0.0_ if _x = +0.0_, and _{plusmn}0.0_ if _x = -0.0_."
        " fsign({plusmn}Inf) = {plusmn}1"
        " If _x_ = _{plusmn}NaN_, the result can be any of _{plusmn}1.0_ or _{plusmn}0.0_, regardless of whether shader_float_controls is in use."
                       FLOAT_OP(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450SSign] = "Result is 1 if _x > 0_, _0_ if _x = 0_, or _-1_ if _x < 0_, where _x_ is interpreted as a signed integer."
                       ALL_INT(_x_)
                       SAME_NUM_COMPS
                       COMPONENT_WISE;

    Description[GLSLstd450Floor] = "Result is the value equal to the nearest whole number that is less than or equal to _x_."
                         GAP " floor({plusmn}0) = {plusmn}0."
                         GAP " floor({plusmn}Inf) = {plusmn}Inf."
                         FLOAT_OP(_x_)
                         ALL_MATCHING(_x_)
                         COMPONENT_WISE;

    Description[GLSLstd450Ceil] = "Result is the value equal to the nearest whole number that is greater than or equal to _x_."
                        GAP " ceil({plusmn}0) = {plusmn}0."
                        GAP " ceil({plusmn}Inf) = {plusmn}Inf."
                        FLOAT_OP(_x_)
                        ALL_MATCHING(_x_)
                        COMPONENT_WISE;

    Description[GLSLstd450Fract] = "Result is _x_ - *floor* _x_."
                         GAP " fract({plusmn}0) = +0."
                         GAP " fract({plusmn}Inf) = NaN."
                         FLOAT_OP(_x_)
                         ALL_MATCHING(_x_)
                         COMPONENT_WISE;

    Description[GLSLstd450Radians] = "Converts _degrees_ to radians, i.e., _degrees_ * {pi} / 180."
                           ALL_16_32_FLOAT(_degrees_)
                           ALL_MATCHING(_degrees_)
                           COMPONENT_WISE;

    Description[GLSLstd450Degrees] = "Converts _radians_ to degrees, i.e., _radians_ * 180 / {pi}."
                           ALL_16_32_FLOAT(_radians_)
                           ALL_MATCHING(_radians_)
                           COMPONENT_WISE;

    Description[GLSLstd450Sin] = "The standard trigonometric sine of _x_ radians."
                       GAP " sin({plusmn}Inf) = NaN."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Cos] = "The standard trigonometric cosine of _x_ radians."
                       GAP " cos({plusmn}Inf) = NaN."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Tan] = "The standard trigonometric tangent of _x_ radians."
                       GAP " tan({plusmn}Inf) = NaN."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Asin] = "Arc sine. Result is an angle, in radians, whose sine is _x_."
                       " The range of result values is [-{pi} / 2, {pi} / 2]. The resulting value is NaN if *abs* _x_ > 1."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Acos] = "Arc cosine. Result is an angle, in radians, whose cosine is _x_."
                       " The range of result values is [0, {pi}]. The resulting value is NaN if *abs* _x_ > 1."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Atan] = "Arc tangent. Result is an angle, in radians, whose tangent is _y_over_x_. The range of result values is [-{pi} / 2, {pi} / 2]."
                       ALL_16_32_FLOAT(_y_over_x_)
                       ALL_MATCHING(_y_over_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Sinh] = "Hyperbolic sine of _x_ radians."
                       GAP " sinh({plusmn}Inf) = {plusmn}Inf."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Cosh] = "Hyperbolic cosine of _x_ radians."
                       GAP " cosh({plusmn}Inf) = Inf."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Tanh] = "Hyperbolic tangent of _x_ radians."
                       GAP " tanh({plusmn}Inf) = {plusmn}1."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Asinh] = "Arc hyperbolic sine; result is the inverse of *sinh*."
                       GAP " asinh({plusmn}Inf) = {plusmn}Inf."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Acosh] = "Arc hyperbolic cosine; Result is the non-negative inverse of *cosh*."
                       " The resulting value is NaN if _x_ < 1."
                       GAP " acosh(Inf) = Inf."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Atanh] = "Arc hyperbolic tangent; result is the inverse of tanh."
                       " The resulting value is NaN if *abs* _x_ {ge} 1."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Atan2] = "Arc tangent. Result is an angle, in radians, whose tangent is _y_ / _x_."
                       " The signs of _x_ and _y_ are used to determine what quadrant the angle is in."
                       " The range of result values is  [-{pi}, {pi}]."
                       " The resulting value is _poison_ if _x_ and _y_ are both 0."
                       ALL_16_32_FLOAT(_x_ and _y_)
                       ALL_MATCHINGS
                       COMPONENT_WISE;

    Description[GLSLstd450Pow] = "Result is _x_ raised to the _y_ power; _x_^_y_^."
                       " The resulting value is _poison_ if _x_ < 0. Result is _poison_ if _x_ = 0 and _y_ {le} 0."
                       ALL_16_32_FLOAT(_x_ and _y_)
                       ALL_MATCHINGS
                       COMPONENT_WISE;

    Description[GLSLstd450Exp] = "Result is the natural exponentiation of _x_; _e_^_x_^."
                       GAP " exp(Inf) = Inf."
                       GAP " exp(-Inf) = +0."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Log] = "Result is the natural logarithm of _x_, i.e.,"
                       " the value _y_ which satisfies the equation _x_ = _e_^_y_^."
                       " The resulting value is NaN if _x_ < 0."
                       GAP " log(Inf) = Inf."
                       GAP " log(1.0) = +0."
                       GAP " log({plusmn}0) = -Inf."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Exp2] = "Result is 2 raised to the _x_ power; 2^_x_^."
                       GAP " exp2(Inf) = Inf."
                       GAP " exp2(-Inf) = +0."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Log2] = "Result is the base-2 logarithm of _x_, i.e.,"
                       " the value _y_ which satisfies the equation  _x_ = 2^_y_^."
                       " The resulting value is NaN if _x_ < 0."
                       GAP " log(Inf) = Inf."
                       GAP " log(1.0) = +0."
                       GAP " log({plusmn}0) = -Inf."
                       ALL_16_32_FLOAT(_x_)
                       ALL_MATCHING(_x_)
                       COMPONENT_WISE;

    Description[GLSLstd450Sqrt] = "Result is the square root of _x_."
                        " The resulting value is NaN if _x_ < 0."
                        GAP " sqrt(Inf) = Inf."
                        GAP " sqrt({plusmn}0) = {plusmn}0."
                        FLOAT_OP(_x_)
                        ALL_MATCHING(_x_)
                        COMPONENT_WISE;

    Description[GLSLstd450InverseSqrt] = "Result is the reciprocal of *sqrt* _x_."
                               " The resulting value is NaN if _x_ < 0."
                               GAP " inversesqrt(Inf) = +0."
                               GAP " inversesqrt({plusmn}0) = {plusmn}Inf."
                               FLOAT_OP(_x_)
                               ALL_MATCHING(_x_)
                               COMPONENT_WISE;

    Description[GLSLstd450Determinant] = "Result is the determinant of _x_."
                               GAP "The operand _x_ must be a square matrix."
                               GAP "_Result Type_ must be the same type as the component type in the columns of _x_.";

    Description[GLSLstd450MatrixInverse] = "Result is a matrix that is the inverse of _x_."
                                 " The resulting values are _poison_ if _x_ is singular or poorly conditioned (nearly singular)."
                                 GAP "The operand _x_ must be a square matrix."
                                 ALL_MATCHING(_x_);

    Description[GLSLstd450Modf] =
                        "*Modf* is deprecated, use *ModfStruct* instead."
                        GAP
                        "Result is the fractional part of _x_, and stores through _i_ the whole-number part as a whole-number floating-point value."
                        " Both the result and the output parameter have the same sign as _x_."
                        FLOAT_OP(_x_)
                        GAP "The operand _i_ must have a pointer type."
                        GAP "_Result Type_, the type of _x_, and the type _i_ points to must all be the same type and have a floating-point component type."
                       COMPONENT_WISE;

    Description[GLSLstd450ModfStruct] =
        "Result is a structure containing both the fractional part of _x_ and the whole number part of _x_."
        GAP
        "_Result Type_ must be an *OpTypeStruct* with two members."
        " Member 0 holds the fractional part."
        " Member 1 holds the whole number part."
        " Both members get the same sign as _x_."
        GAP " modf({plusmn}0) = { {plusmn}0, {plusmn}0 }."
        GAP " modf({plusmn}Inf) = { {plusmn}0, {plusmn}Inf }."
        GAP " The two members of the returned struct and _x_ must all be the same type."
        COMPONENT_WISE
        FLOAT_OP(_x_);

    Description[GLSLstd450FMin] = "Result is _y_ if _y_ < _x_, otherwise _x_."
                                  " -0 compares less than +0."
                                  " Which operand is the result if one of the operands is a NaN is implementation-dependent."
                       FLOAT_OPS
                       ALL_MATCHINGS
                       COMPONENT_WISE;

    Description[GLSLstd450NMin] = "Result is _y_ if _y_ < _x_, otherwise _x_."
                                  " -0 compares less than +0."
                                  " If one operand is a NaN, the other operand is the result."
                                  " If both operands are NaN, the result is a NaN."
                       FLOAT_OPS
                       ALL_MATCHINGS
                       COMPONENT_WISE;

    Description[GLSLstd450SMin] = "Result is _y_ if _y_ < _x_; otherwise result is _x_, where _x_ and _y_ are interpreted as signed integers."
                       ALL_INT(_x_ and _y_)
                       SAME_NUM_COMPS
                       COMPONENT_WISE;

    Description[GLSLstd450UMin] = "Result is _y_ if _y_ < _x_; otherwise result is _x_, where _x_ and _y_ are interpreted as unsigned integers."
                       ALL_INT(_x_ and _y_)
                       SAME_NUM_COMPS
                       COMPONENT_WISE;

    Description[GLSLstd450FMax] = "Result is _y_ if _x_ < _y_. otherwise _x_."
                                  " -0 compares less than +0."
                                  " Which operand is the result if one of the operands is a NaN is implementation-dependent."
                       FLOAT_OPS
                       ALL_MATCHINGS
                       COMPONENT_WISE;

    Description[GLSLstd450NMax] = "Result is _y_ if _x_ < _y_, otherwise _x_."
                                  " -0 compares less than +0."
                                  " If one operand is a NaN, the other operand is the result."
                                  " If both operands are NaN, the result is a NaN."
                       FLOAT_OPS
                       ALL_MATCHINGS
                       COMPONENT_WISE;

    Description[GLSLstd450UMax] = "Result is _y_ if _x_ < _y_; otherwise result is _x_, where _x_ and _y_ are interpreted as unsigned integers."
                       ALL_INT(_x_ and _y_)
                       SAME_NUM_COMPS
                       COMPONENT_WISE;

    Description[GLSLstd450SMax] = "Result is _y_ if _x_ < _y_; otherwise result is _x_, where _x_ and _y_ are interpreted as signed integers."
                       ALL_INT(_x_ and _y_)
                       SAME_NUM_COMPS
                       COMPONENT_WISE;

    Description[GLSLstd450FClamp] = "Result is min(max(_x_, _minVal_), _maxVal_)."
                         " The resulting value is _poison_ if _minVal_ > _maxVal_."
                         " The semantics used by min() and max() are those of FMin and FMax."
                         FLOAT_OPS
                         ALL_MATCHINGS
                         COMPONENT_WISE;

    Description[GLSLstd450NClamp] = "Result is min(max(_x_, _minVal_), _maxVal_)."
                         " The resulting value is _poison_ if _minVal_ > _maxVal_."
                         " The semantics used by min() and max() are those of NMin and NMax."
                         FLOAT_OPS
                         ALL_MATCHINGS
                         COMPONENT_WISE;

    Description[GLSLstd450SClamp] = "Result is min(max(_x_, _minVal_), _maxVal_), where _x_,"
                       " _minVal_ and _maxVal_ are interpreted as signed integers."
                       " The resulting value is _poison_ if _minVal_ > _maxVal_."
                       ALL_INT(the operands)
                       SAME_NUM_COMPS
                       COMPONENT_WISE;

    Description[GLSLstd450UClamp] = "Result is min(max(_x_, _minVal_), _maxVal_),"
                       " where _x_, _minVal_ and _maxVal_ are interpreted as unsigned integers."
                       " The resulting value is _poison_ if _minVal_ > _maxVal_."
                       ALL_INT(the operands)
                       SAME_NUM_COMPS
                       COMPONENT_WISE;

    Description[GLSLstd450FMix] = "Result is the linear blend of _x_ and _y_, i.e., _x_ * (1 - _a_) + _y_ * _a_."
                       FLOAT_OPS
                       ALL_MATCHINGS
                       COMPONENT_WISE;

    Description[GLSLstd450Step] = "Result is 0.0 if _x_ < _edge_; otherwise result is 1.0."
                        FLOAT_OPS
                        ALL_MATCHINGS
                        COMPONENT_WISE;

    Description[GLSLstd450SmoothStep] = "Result is 0.0 if _x_ {le} _edge0_ and 1.0 if _x_ {ge} _edge1_ and performs smooth Hermite interpolation between 0 and 1 if"
                              " _edge0_ < _x_ < _edge1_."
                              " This is equivalent to:"
                              GAP "_t_ * _t_ * (3 - 2 * _t_), where _t_ = clamp ((_x_ - _edge0_) / (_edge1_ - _edge0_), 0, 1)"
                              GAP "The resulting value is _poison_ if _edge0_ {ge} _edge1_."
                              FLOAT_OPS
                              ALL_MATCHINGS
                              COMPONENT_WISE;

    Description[GLSLstd450Fma] = "Computes _a_ * _b_ + _c_."
                                 FLOAT_OPS
                                 ALL_MATCHINGS
                                 COMPONENT_WISE;

    Description[GLSLstd450Frexp] = "*Frexp* is deprecated, use *FrexpStruct* instead."
                         GAP "Splits _x_ into a floating-point significand in the range"
                             " (-1.0, 0.5] or [0.5, 1.0) and an integral exponent of 2, such that:"
                         LINE_BREAK "_x_ = _significand_ * 2^_exponent_^"
                         LINE_BREAK "The _significand_ is the instruction result."
                                    " An _x_ of _-0.0_ results in a significand _-0.0_, while an _x_ of _0.0_ results in _0.0_."
                                    " For a floating-point value that is an infinity or is not a number, the significand is _poison_."
                         FLOAT_OP(_x_)
                         GAP "The exponent is returned through the pointer-parameter _exp_."
                             " The _exp_ operand must be a pointer to a scalar or vector with integer component type,"
                             " with 32-bit component width."
                             " The number of components in _x_ and what _exp_ points to must be the same."
                             " If _x_ is a zero, the exponent is 0.0."
                             " If _x_ is an infinity or a NaN, the exponent is _poison_."
                         GAP "_Result Type_ must be the same type as the type of _x_."
                         COMPONENT_WISE;

    Description[GLSLstd450FrexpStruct] =
                         "Result is a structure containing _x_ split into a floating-point significand in the range"
                         " (-1.0, 0.5] or [0.5, 1.0) and an integral exponent of 2, such that:"
                         LINE_BREAK "_x_ = _significand_ * 2^_exponent_^"
                         LINE_BREAK "If _x_ is a zero, the exponent is 0.0."
                                    " If _x_ is an infinity or a NaN, the exponent is _poison_."
                                    " If _x_ is _0.0_, the significand is _0.0_."
                                    " If _x_ is _-0.0_, the significand is _-0.0_"
                         GAP "_Result Type_ must be an *OpTypeStruct* with two members.  Member 0 must have the same type as the type of _x_."
                             " Member 0 holds the significand."
                             " Member 1 must be a scalar or vector with integer component type, with 32-bit component width."
                             " Member 1 holds the exponent."
                             " These two members and _x_ must have the same number of components."
                         FLOAT_OP(_x_);

    Description[GLSLstd450Ldexp] = "Builds a floating-point number from _x_ and the corresponding integral exponent of two in _exp_:"
                         LINE_BREAK "_x_ * 2^_exp_^"
                         LINE_BREAK "If this product is too large to be finitely represented in the floating-point type,"
                                    " the resulting value is _poison_."
                                    " _exp_ is interpreted as a signed integer."
                                    " If _exp_ is greater than +128 (single precision), +1024 (double precision) or +16 (half precision),"
                                    " the resulting value is _poison_."
                                    " If _exp_ is less than -126 (single precision), -1022 (double precision) or -14 (half precision),"
                                    " the result may be flushed to zero."
                                    " Additionally, splitting the value into a significand and exponent using *frexp* and then reconstructing a"
                                    " floating-point value using *ldexp* should yield the original input for zero and all finite non-denormalized values."
                         FLOAT_OP(_x_)
                         GAP "The _exp_ operand must be a scalar or vector with integer component type.  The number of components in _x_ and _exp_ must be the same."
                         GAP "_Result Type_ must be the same type as the type of _x_."
                         COMPONENT_WISE;

#define RELAXED_PRECISION_AFFECT_CONVERSION "The *RelaxedPrecision* Decoration only affects the conversion step of the instruction."

    Description[GLSLstd450PackSnorm4x8] = "First, converts each component of the normalized floating-point value _v_ into 8-bit integer values."
                                " These are then packed into the result."
                                LINE_BREAK "The conversion for component _c_ of _v_ to fixed point is done as follows:"
                                LINE_BREAK "round(clamp(_c_, -1, +1) * 127.0)"
                                LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                LINE_BREAK "The first component of the vector is written to the least significant bits of the output;"
                                           " the last component is written to the most significant bits."
                                GAP "The _v_ operand must be a vector of 4 components whose type is a 32-bit floating-point."
                                GAP "_Result Type_ must be a 32-bit integer type.";

    Description[GLSLstd450PackUnorm4x8] = "First, converts each component of the normalized floating-point value _v_ into 8-bit integer values."
                                " These are then packed into the result."
                                LINE_BREAK "The conversion for component _c_ of _v_ to fixed point is done as follows:"
                                LINE_BREAK "round(clamp(_c_, 0, +1) * 255.0)"
                                LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                LINE_BREAK "The first component of the vector is written to the least significant bits of the output;"
                                           " the last component is written to the most significant bits."
                                GAP "The _v_ operand must be a vector of 4 components whose type is a 32-bit floating-point."
                                GAP "_Result Type_ must be a 32-bit integer type.";

    Description[GLSLstd450PackSnorm2x16] = "First, converts each component of the normalized floating-point value _v_ into 16-bit integer values."
                                " These are then packed into the result."
                                LINE_BREAK "The conversion for component _c_ of _v_ to fixed point is done as follows:"
                                LINE_BREAK "round(clamp(_c_, -1, +1) * 32767.0)"
                                LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                LINE_BREAK "The first component of the vector is written to the least significant bits of the output;"
                                           " the last component is written to the most significant bits."
                                GAP "The _v_ operand must be a vector of 2 components whose type is a 32-bit floating-point."
                                GAP "_Result Type_ must be a 32-bit integer type.";

    Description[GLSLstd450PackUnorm2x16] = "First, converts each component of the normalized floating-point value _v_ into 16-bit integer values."
                                " These are then packed into the result."
                                LINE_BREAK "The conversion for component _c_ of _v_ to fixed point is done as follows:"
                                LINE_BREAK "round(clamp(_c_, 0, +1) * 65535.0)"
                                LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                LINE_BREAK "The first component of the vector is written to the least significant bits of the output;"
                                           " the last component is written to the most significant bits."
                                GAP "The _v_ operand must be a vector of 2 components whose type is a 32-bit floating-point."
                                GAP "_Result Type_ must be a 32-bit integer type.";

    Description[GLSLstd450PackHalf2x16] = "Result is the unsigned integer obtained by converting the components of a two-component floating-point vector to the 16-bit *OpTypeFloat*,"
                                " and then packing these two 16-bit integers into a 32-bit unsigned integer."
                                " The first vector component specifies the 16 least-significant bits of the result; the second component specifies the 16 most-significant bits."
                                LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                GAP "The _v_ operand must be a vector of 2 components whose type is a 32-bit floating-point."
                                GAP "_Result Type_ must be a 32-bit integer type.";

    Description[GLSLstd450PackDouble2x32] = "Result is the double-precision value obtained by packing the components of _v_ into a 64-bit value."
                                  " If an IEEE 754 Inf or NaN is created, it will not signal, and the resulting floating-point value is unspecified."
                                  " Otherwise, the bit-level representation of _v_ is preserved. The first vector component specifies the 32 least significant bits;"
                                  " the second component specifies the 32 most significant bits."
                                  GAP "The _v_ operand must be a vector of 2 components whose type is a 32-bit integer."
                                  GAP "_Result Type_ must be a 64-bit floating-point scalar.";

    Description[GLSLstd450UnpackSnorm2x16] = "First, unpacks a single 32-bit unsigned integer _p_ into a pair of 16-bit signed integers."
                                   " Then, each component is converted to a normalized floating-point value to generate the result."
                                   " The conversion for unpacked fixed-point value _f_ to floating point is done as follows:"
                                   LINE_BREAK "clamp(_f_ / 32767.0, -1, +1)"
                                   LINE_BREAK "The first component of the result is extracted from the least significant bits of the input;"
                                   " the last component is extracted from the most significant bits."
                                   LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                   GAP "The _p_ operand must be a scalar with 32-bit integer type."
                                   GAP "_Result Type_ must be a vector of 2 components whose type is 32-bit floating point.";

    Description[GLSLstd450UnpackUnorm2x16] = "First, unpacks a single 32-bit unsigned integer _p_ into a pair of 16-bit unsigned integers."
                                   " Then, each component is converted to a normalized floating-point value to generate the result."
                                   " The conversion for unpacked fixed-point value _f_ to floating point is done as follows:"
                                   LINE_BREAK "_f_ / 65535.0"
                                   LINE_BREAK "The first component of the result is extracted from the least significant bits of the input;"
                                   " the last component is extracted from the most significant bits."
                                   LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                   GAP "The _p_ operand must be a scalar with 32-bit integer type."
                                   GAP "_Result Type_ must be a vector of 2 components whose type is 32-bit floating point.";

    Description[GLSLstd450UnpackHalf2x16] = "Result is the two-component floating-point vector with components obtained by unpacking a 32-bit"
                                  " unsigned integer into a pair of 16-bit values, interpreting those values as 16-bit floating-point numbers"
                                  " according to the OpenGL Specification, and converting them to 32-bit floating-point values."
                                  " Subnormal numbers are either preserved or flushed to zero, consistently within an implementation."
                                  LINE_BREAK " The first component of the vector is obtained from the 16 least-significant bits of _v_;"
                                  " the second component is obtained from the 16 most-significant bits of _v_."
                                  LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                  GAP "The _v_ operand must be a scalar with 32-bit integer type."
                                  GAP "_Result Type_ must be a vector of 2 components whose type is 32-bit floating point.";

    Description[GLSLstd450UnpackSnorm4x8] = "First, unpacks a single 32-bit unsigned integer _p_ into four 8-bit signed integers."
                                   " Then, each component is converted to a normalized floating-point value to generate the result."
                                   " The conversion for unpacked fixed-point value _f_ to floating point is done as follows:"
                                   LINE_BREAK "clamp(_f_ / 127.0, -1, +1)"
                                   LINE_BREAK "The first component of the result is extracted from the least significant bits of the input;"
                                   " the last component is extracted from the most significant bits."
                                   LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                   GAP "The _p_ operand must be a scalar with 32-bit integer type."
                                   GAP "_Result Type_ must be a vector of 4 components whose type is 32-bit floating point.";

    Description[GLSLstd450UnpackUnorm4x8] = "First, unpacks a single 32-bit unsigned integer _p_ into four 8-bit unsigned integers."
                                   " Then, each component is converted to a normalized floating-point value to generate the result."
                                   " The conversion for unpacked fixed-point value _f_ to floating point is done as follows:"
                                   LINE_BREAK "_f_ / 255.0"
                                   LINE_BREAK "The first component of the result is extracted from the least significant bits of the input;"
                                   " the last component is extracted from the most significant bits."
                                   LINE_BREAK RELAXED_PRECISION_AFFECT_CONVERSION
                                   GAP "The _p_ operand must be a scalar with 32-bit integer type."
                                   GAP "_Result Type_ must be a vector of 4 components whose type is 32-bit floating point.";

    Description[GLSLstd450UnpackDouble2x32] = "Result is the two-component unsigned integer vector representation of _v_. The bit-level representation of _v_ is preserved."
                                    " The first component of the vector contains the 32 least significant bits of the double; the second component consists of the 32 most significant bits."
                                    GAP "The _v_ operand must be a scalar whose type is 64-bit floating point."
                                    GAP "_Result Type_ must be a vector of 2 components whose type is a 32-bit integer.";

    Description[GLSLstd450Length] = "Result is the length of vector _x_, i.e., sqrt(_x_ [0] ^2^ + _x_ [1] ^2^ + ...)."
                          FLOAT_OP(_x_)
                          GAP "_Result Type_ must be a scalar of the same type as the component type of _x_.";

    Description[GLSLstd450Distance] = "Result is the distance between _p0_ and _p1_, i.e., length(_p0_ - _p1_)."
                            FLOAT_OPS
                            GAP "_Result Type_ must be a scalar of the same type as the component type of the operands.";

    Description[GLSLstd450Cross] = "Result is the cross product of _x_ and _y_, i.e., the resulting components are, in order:"
                         LINE_BREAK "_x_[1] * _y_[2] - _y_[1] * _x_[2]"
                         LINE_BREAK "_x_[2] * _y_[0] - _y_[2] * _x_[0]"
                         LINE_BREAK "_x_[0] * _y_[1] - _y_[0] * _x_[1]"
                         GAP "All the operands must be vectors of 3 components of a floating-point type."
                         ALL_MATCHINGS;

    Description[GLSLstd450Normalize] = "Result is the vector in the same direction as _x_ but with a length of 1."
                             FLOAT_OP(_x_)
                             ALL_MATCHING(_x_);

    Description[GLSLstd450FaceForward] = "If the dot product of _Nref_ and _I_ is negative, the result is _N_, otherwise it is _-N_."
                               FLOAT_OPS
                               ALL_MATCHINGS;

    Description[GLSLstd450Reflect] = "For the incident vector _I_ and surface orientation _N_, returns _I_ - 2 * dot(_N_, _I_) * _N_."
                           GAP "If _N_ is normalized then this corresponds to _I_ reflected from a surface with normal _N_."
                           FLOAT_OPS
                           ALL_MATCHINGS;

    Description[GLSLstd450Refract] = "For the incident vector _I_ and surface normal _N_, and the ratio of indices of refraction _eta_, the result is the refraction vector."
                           " The result is computed by"
                           LINE_BREAK "k = 1.0 - _eta_ * _eta_ * (1.0 - dot(_N_, _I_) * dot(_N_, _I_))"
                           LINE_BREAK "if k < 0.0 the result is 0.0"
                           LINE_BREAK "otherwise, the result is _eta_ * _I_ - (_eta_ * dot(_N_, _I_) + sqrt(k)) * _N_"
                           LINE_BREAK "This computation assumes the input parameters for the incident vector _I_"
                                      " and the surface normal _N_ are already normalized."
                           GAP "The type of _I_ and _N_ must be a scalar or vector with a floating-point component type."
                           GAP "The type of _eta_ must be a floating-point scalar."
                           GAP "_Result Type_, the type of _I_, the type of _N_, and the type of _eta_ must all have the same component type.";

#define RESULT_IS_MINUS_1 " the result has all bits set (e.g., -1 if interpreted as signed)"

    Description[GLSLstd450FindILsb] = "Integer least-significant bit."
                            GAP "Results in the bit number of the least-significant 1-bit in the binary representation of _Value_."
                                " If _Value_ is 0," RESULT_IS_MINUS_1 "."
                            ALL_INT(_Value_)
                            SAME_NUM_COMPS
                            COMPONENT_WISE
                            WIDTH_32_ONLY;

    Description[GLSLstd450FindSMsb] = "Signed-integer most-significant bit, with _Value_ interpreted as a signed integer."
                            GAP "For positive numbers, the result is the bit number of the most significant 1-bit."
                                " For negative numbers, the result is the bit number of the most significant 0-bit."
                                " For a _Value_ of 0 or -1," RESULT_IS_MINUS_1 "."
                            ALL_INT(_Value_)
                            SAME_NUM_COMPS
                            COMPONENT_WISE
                            WIDTH_32_ONLY;

    Description[GLSLstd450FindUMsb] = "Unsigned-integer most-significant bit."
                            GAP "Results in the bit number of the most-significant 1-bit in the binary representation of _Value_."
                                 " If _Value_ is 0," RESULT_IS_MINUS_1 "."
                            ALL_INT(_Value_)
                            SAME_NUM_COMPS
                            COMPONENT_WISE
                            WIDTH_32_ONLY;

    Description[GLSLstd450InterpolateAtCentroid] = "Result is the value of the input _interpolant_ sampled at a location inside both the fragment and the primitive being processed."
                                         " The value obtained would be the same value assigned to the input variable if it were decorated as *Centroid*."
                                         INPUT_ONLY(_interpolant_) ALL_32_FLOAT_P(_interpolant_)
                                         FRAGMENT_ONLY
                                         ALL_MATCHING_POINTER(_interpolant_);

    Description[GLSLstd450InterpolateAtSample] = "Result is the value of the input _interpolant_ variable at the location of sample number _sample_."
                                       " If sample _sample_ does not exist, the position used to interpolate the input variable is _poison_."
                                       INPUT_ONLY(_interpolant_) ALL_32_FLOAT_P(_interpolant_)
                                       FRAGMENT_ONLY
                                       GAP "The _sample_ operand must be a scalar 32-bit integer."
                                       ALL_MATCHING_POINTER(_interpolant_);

    Description[GLSLstd450InterpolateAtOffset] = "Result is the value of the input _interpolant_ variable sampled at an offset from the center of the fragment specified by _offset_."
                                       " The two floating-point components of _offset_, give the offset in pixels in the _x_ and _y_ directions, respectively."
                                       " An _offset_ of (0, 0) identifies the center of the fragment."
                                       " The range and granularity of offsets supported are implementation-dependent."
                                       INPUT_ONLY(_interpolant_) ALL_32_FLOAT_P(_interpolant_)
                                       FRAGMENT_ONLY
                                       GAP "The _offset_ operand must be a vector of 2 components of 32-bit floating-point type."
                                       ALL_MATCHING_POINTER(_interpolant_);
}

void PrintTable(int builtInNum)
{
    if (Description[builtInNum] == 0)
        return;

    int numOperands = Parameters[builtInNum].getNum();

    // Table start
    int width = std::max(numOperands * 12, (int)strlen(Description[builtInNum]) / 3);
    width = std::max(width, 50);
    printf("[cols=\"%d\",width=\"%d%%\"]\n", numOperands + 1, std::min(100, width));
    printf("|=====\n");

    // Name
    printf("%d+|*%s*", numOperands + 1, Names[builtInNum].c_str());

    // Semantics
    printf(" +\n +\n%s%s\n", Description[builtInNum], GLSLCapabilityReqirements[builtInNum].c_str());

    // Number
    printf("| %d\n", builtInNum);

    // Operands
    if (numOperands > 0)
        PrintOperands(Parameters[builtInNum], 0);

    // Table end
    printf("\n|=====\n");
}

void PrintGLSLDoc()
{
    FillParameters();

    for (int i = 0; i < GLSLstd450Count; ++i)
        PrintTable(i);
}

};  // end namespace spv
