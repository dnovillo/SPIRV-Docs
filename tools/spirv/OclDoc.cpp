// Copyright 2014-2026 The Khronos Group Inc.
// SPDX-License-Identifier: MIT

#include "unified1/spirv.hpp"
#include "unified1/OpenCL.std.h"

#include "doc.h"
#include "OclDoc.h"

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <map>

#define LINE_BREAK "\n\n"
#define GAP "\n\n"
#define NOTE "*Note:* "

#define RES_TYPE "_Result Type_"
#define PRINT_INTRO(OCLVER, SPIRVersion, LIBNAME, MEMMODEL)   \
    printf("\n[[Introduction]]\n== Introduction\n"); \
    printf("\nThis is the specification of *%s* extended instruction set.\n", LIBNAME); \
    printf("\nThe library is imported into a SPIR-V module in the following manner:\n");\
    printf("\n_<ext-inst-id> OpExtInstImport \"%s\"_\n", LIBNAME); \
    printf("\nThe library can only be imported if *Memory Model* is set to *%s*\n", MEMMODEL); \
    printf("\n[[Binary]]\n== Binary Form\n"); \
    printf("This section contains the semantics and exact form of execution of %s extended instructions using the *OpExtInst* instruction." LINE_BREAK, OCLVER); \
    printf("In this section we use the following naming conventions:" LINE_BREAK); \
    printf("- _void_ denote an *OpTypeVoid*." LINE_BREAK); \
    printf("- _half_, _float_ and _double_ denote an *OpTypeFloat* with a width of 16, 32 and 64 bits using the IEEE 754 encoding respectively." LINE_BREAK); \
    printf("- _i8_, _i16_, _i32_ and _i64_ denote an *OpTypeInt* with a width of 8, 16, 32 and 64 bits respectively." LINE_BREAK); \
    printf("- _bool_ denotes an *OpTypeBool*." LINE_BREAK); \
    printf("- _size_t_ denotes an i32 if the *Addressing Model* is *Physical32* and i64 if the *Addressing Model* is *Physical64*." LINE_BREAK); \
    printf("- _vector(n)_ denotes an *OpTypeVector* where _n_ indicates the component count." LINE_BREAK); \
    printf("** _vector(n~1~, n~2~, ..., n~i~)_ abbreviates _vector(n~1~)_, _vector(n~2~)_, ... or _vector(n~i~)_." LINE_BREAK); \
    printf("- _integer_ denotes _i8_, _i16_, _i32_ or _i64_." LINE_BREAK); \
    printf("- _floating-point_ denotes _half_, _float_, _double_." LINE_BREAK); \
    printf("- _pointer_(_storage_) denotes an *OpTypePointer* which points to _storage_ *Storage Class*." LINE_BREAK); \
    printf("** _pointer_(_constant_) denotes an OpTypePointer with *UniformConstant* *Storage Class*." LINE_BREAK); \
    printf("** _pointer_(_generic_) denotes an OpTypePointer with *Generic* *Storage Class*." LINE_BREAK); \
    printf("** _pointer_(_global_) denotes an OpTypePointer with *CrossWorkgroup* *Storage Class*." LINE_BREAK); \
    printf("** _pointer_(_local_) denotes an OpTypePointer with *Workgroup* *Storage Class*." LINE_BREAK); \
    printf("** _pointer_(_private_) denotes an OpTypePointer with *Function* *Storage Class*." LINE_BREAK); \
    printf("** _pointer(s~1~, s~2~, ..., s~i~)_ abbreviates _pointer(s~1~)_, _pointer(s~2~)_, ... or _pointer(s~i~)_." LINE_BREAK); \
    printf("- _image_ defines all types of image memory objects (See https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_Env.html#_image_related_data_types[image related data types] section of the OpenCL environment specification)." LINE_BREAK); \
    printf("- _sampler_ a SPIR-V sampler object (See https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_Env.html#_image_related_data_types[image related data types] section of the OpenCL environment specification)." LINE_BREAK);



#define DEFAULT_IDENTICAL_TYPES_MESSAGE "All of the operands, including the _Result Type_ operand, must be of the same type." LINE_BREAK
#define IDENTICAL_TYPES_AND_POINTERS_MESSAGE "All of the operands, including the _Result Type_ operand, must be of the same type, or must be a pointer to the same type." LINE_BREAK
#define HALF_FUNC_ULP " "
   // "This function is implemented with a minimum of 10 bits of accuracy i.e. an ULP value <= 8192 ulp." LINE_BREAK \
   //  " The support for denormal values is optional and may return any result allowed even when -cl-denormals-are-zero flag is not in force.  " LINE_BREAK
#define NATIVE_FUNC_NOTE "This instruction may map to one or more native device instructions and " \
    "typically has better performance compared to the corresponding non-native instruction. " \
    "Support for denormal values is implementation-defined for native instructions." LINE_BREAK
#define CONTRACTIONS_NOTE "This instruction can be implemented using contractions such as *mad* or *fma*." LINE_BREAK

#define SCALAR_AND_VECTOR   ScalarVecType | VectorVecType
#define ALL_VEC_SIZE        TwoComp | ThreeComp | FourComp | EightComp | SixteenComp
#define NO_SIGNED_WRAP      "This instruction can be decorated with *NoSignedWrap*."

extern const char* OpenCL20DebugNames[spv::OclExtInstCeiling];

namespace spv {
    using namespace OpenCLLIB;
static SPIRVersion version;
BuiltInFunctionParameters BuiltInDesc[OclExtInstCeiling];

static const int POINTER_MASK = PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrConstantDataType | PtrGenericDataType;

std::set<OpenCLLIB::Entrypoints> OclInstrPageBreaks;

void OclSetExtraPageBreaks()
{
    //printf("Adding extra page breaks instructions\n");
    OclInstrPageBreaks.insert(Acospi);
    OclInstrPageBreaks.insert(Asinpi);
    OclInstrPageBreaks.insert(Atanh);
    OclInstrPageBreaks.insert(Cbrt);
    OclInstrPageBreaks.insert(Cosh);
    OclInstrPageBreaks.insert(Exp);
    OclInstrPageBreaks.insert(Fabs);
    OclInstrPageBreaks.insert(Fmax);
    OclInstrPageBreaks.insert(Fract);
    OclInstrPageBreaks.insert(Ilogb);
    OclInstrPageBreaks.insert(Lgamma_r);
    OclInstrPageBreaks.insert(Nextafter);
    OclInstrPageBreaks.insert(Rootn);
    OclInstrPageBreaks.insert(Sqrt);
    OclInstrPageBreaks.insert(Tgamma);
    OclInstrPageBreaks.insert(Half_exp);
    OclInstrPageBreaks.insert(Half_log2);
    OclInstrPageBreaks.insert(Half_rsqrt);
    OclInstrPageBreaks.insert(Native_cos);
    OclInstrPageBreaks.insert(Native_exp2);
    OclInstrPageBreaks.insert(Native_recip);
    OclInstrPageBreaks.insert(UAdd_sat);
    OclInstrPageBreaks.insert(SMax);
    OclInstrPageBreaks.insert(SMul_hi);
    OclInstrPageBreaks.insert(U_Upsample);
    OclInstrPageBreaks.insert(S_Upsample);
    OclInstrPageBreaks.insert(SMad24);
    OclInstrPageBreaks.insert(UMul24);
    OclInstrPageBreaks.insert(FMin_common);
    OclInstrPageBreaks.insert(Normalize);
    OclInstrPageBreaks.insert(Fast_normalize);
    OclInstrPageBreaks.insert(Vload_half);
    OclInstrPageBreaks.insert(Vstore_halfn_r);
    OclInstrPageBreaks.insert(Vstorea_halfn);
    OclInstrPageBreaks.insert(Shuffle2);
}

void OclGetNames(const char** names)
{
    for (int i = 0; i < OclExtInstCeiling; ++i)
        names[i] = "unknown";

    // math functions
    names[Acos] = "acos";
    names[Acosh] = "acosh";
    names[Acospi] = "acospi";
    names[Asin] = "asin";
    names[Asinh] = "asinh";
    names[Asinpi] = "asinpi";
    names[Atan] = "atan";
    names[Atan2] = "atan2";
    names[Atanh] = "atanh";
    names[Atanpi] = "atanpi";
    names[Atan2pi] = "atan2pi";
    names[Cbrt] = "cbrt";
    names[Ceil] = "ceil";
    names[Copysign] = "copysign";
    names[Cos] = "cos";
    names[Cosh] = "cosh";
    names[Cospi] = "cospi";
    names[Erfc] = "erfc";
    names[Erf] = "erf";
    names[Exp] = "exp";
    names[Exp2] = "exp2";
    names[Exp10] = "exp10";
    names[Expm1] = "expm1";
    names[Fabs] = "fabs";
    names[Fdim] = "fdim";
    names[Floor] = "floor";
    names[Fma] = "fma";
    names[Fmax] = "fmax";
    names[Fmin] = "fmin";
    names[Fmod] = "fmod";
    names[Fract] = "fract";
    names[Frexp] = "frexp";
    names[Hypot] = "hypot";
    names[Ilogb] = "ilogb";
    names[Ldexp] = "ldexp";
    names[Lgamma] = "lgamma";
    names[Lgamma_r] = "lgamma_r";
    names[Log] = "log";
    names[Log2] = "log2";
    names[Log10] = "log10";
    names[Log1p] = "log1p";
    names[Logb] = "logb";
    names[Mad] = "mad";
    names[Maxmag] = "maxmag";
    names[Minmag] = "minmag";
    names[Modf] = "modf";
    names[Nan] = "nan";
    names[Nextafter] = "nextafter";
    names[Pow] = "pow";
    names[Pown] = "pown";
    names[Powr] = "powr";
    names[Remainder] = "remainder";
    names[Remquo] = "remquo";
    names[Rint] = "rint";
    names[Rootn] = "rootn";
    names[Round] = "round";
    names[Rsqrt] = "rsqrt";
    names[Sin] = "sin";
    names[Sincos] = "sincos";
    names[Sinh] = "sinh";
    names[Sinpi] = "sinpi";
    names[Sqrt] = "sqrt";
    names[Tan] = "tan";
    names[Tanh] = "tanh";
    names[Tanpi] = "tanpi";
    names[Tgamma] = "tgamma";
    names[Trunc] = "trunc";
    names[Half_cos] = "half_cos";
    names[Half_divide] = "half_divide";
    names[Half_exp] = "half_exp";
    names[Half_exp2] = "half_exp2";
    names[Half_exp10] = "half_exp10";
    names[Half_log] = "half_log";
    names[Half_log2] = "half_log2";
    names[Half_log10] = "half_log10";
    names[Half_powr] = "half_powr";
    names[Half_recip] = "half_recip";
    names[Half_rsqrt] = "half_rsqrt";
    names[Half_sin] = "half_sin";
    names[Half_sqrt] = "half_sqrt";
    names[Half_tan] = "half_tan";
    names[Native_cos] = "native_cos";
    names[Native_divide] = "native_divide";
    names[Native_exp] = "native_exp";
    names[Native_exp2] = "native_exp2";
    names[Native_exp10] = "native_exp10";
    names[Native_log] = "native_log";
    names[Native_log2] = "native_log2";
    names[Native_log10] = "native_log10";
    names[Native_powr] = "native_powr";
    names[Native_recip] = "native_recip";
    names[Native_rsqrt] = "native_rsqrt";
    names[Native_sin] = "native_sin";
    names[Native_sqrt] = "native_sqrt";
    names[Native_tan] = "native_tan";

    // common functions
    names[FClamp] = "fclamp";
    names[Degrees] = "degrees";
    names[Mix] = "mix";
    names[FMax_common] = "fmax_common";
    names[FMin_common] = "fmin_common";
    names[Radians] = "radians";
    names[Step] = "step";
    names[Smoothstep] = "smoothstep";
    names[Sign] = "sign";

    // Geometrics
    names[Cross] = "cross";
    names[Distance] = "distance";
    names[Length] = "length";
    names[Normalize] = "normalize";
    names[Fast_distance] = "fast_distance";
    names[Fast_length] = "fast_length";
    names[Fast_normalize] = "fast_normalize";

    // Integers
    names[SAbs] = "s_abs";
    names[UAbs] = "u_abs";
    names[SAbs_diff] = "s_abs_diff";
    names[UAbs_diff] = "u_abs_diff";
    names[SAdd_sat] = "s_add_sat";
    names[UAdd_sat] = "u_add_sat";
    names[SHadd] = "s_hadd";
    names[UHadd] = "u_hadd";
    names[SRhadd] = "s_rhadd";
    names[URhadd] = "u_rhadd";
    names[SClamp] = "s_clamp";
    names[UClamp] = "u_clamp";
    names[Clz] = "clz";
    names[Ctz] = "ctz";
    names[SMad_hi] = "s_mad_hi";
    names[UMad_hi] = "u_mad_hi";
    names[SMad_sat] = "s_mad_sat";
    names[UMad_sat] = "u_mad_sat";
    names[SMax] = "s_max";
    names[SMin] = "s_min";
    names[UMax] = "u_max";
    names[UMin] = "u_min";
    names[SMul_hi] = "s_mul_hi";
    names[UMul_hi] = "u_mul_hi";
    names[Rotate] = "rotate";
    names[SSub_sat] = "s_sub_sat";
    names[USub_sat] = "u_sub_sat";
    names[U_Upsample] = "u_upsample";
    names[S_Upsample] = "s_upsample";
    names[Popcount] = "popcount";
    names[SMad24] = "s_mad24";
    names[UMad24] = "u_mad24";
    names[SMul24] = "s_mul24";
    names[UMul24] = "u_mul24";

    // Vector Loads/Stores
    names[Vloadn] = "vloadn";
    names[Vstoren] = "vstoren";
    names[Vload_half] = "vload_half";
    names[Vload_halfn] = "vload_halfn";
    names[Vstore_half] = "vstore_half";
    names[Vstore_half_r] = "vstore_half_r";
    names[Vstore_halfn] = "vstore_halfn";
    names[Vstore_halfn_r] = "vstore_halfn_r";
    names[Vloada_halfn] = "vloada_halfn";
    names[Vstorea_halfn] = "vstorea_halfn";
    names[Vstorea_halfn_r] = "vstorea_halfn_r";

    // Others
    names[Shuffle] = "shuffle";
    names[Shuffle2] = "shuffle2";
    names[Printf] = "printf";
    names[Prefetch] = "prefetch";

    // Relationals
    names[Bitselect] = "bitselect";
    names[Select] = "select";

}

std::string GetEnumOperandDesc(OCLEnumOperands op) {
    switch (op)
    {
    case spv::ImageChannelOrderEnumOperand: return "ImageChannelOrder";
    case spv::ImageChannelTypeEnumOperand:  return "ImageChannelType";

    default: return "Unknown OCL Enum Operand";
    }
}

bool isPointer(int Ty) {
    if (Ty & POINTER_MASK) {
        return true;
    }
    return false;
}

const std::string GetPointerTypeDesc(int Ty, std::string dataType, int isPlural) {
    std::string desc("");
    bool notFirstIter = false;
    for (int i = PointerDataTypeStart; i < (PointerDataTypeStart + PointerDataTypeCount); i++) {
        if ((Ty >> i) & 0x1) {
            if (notFirstIter) {
                desc += ", ";
            }
            else {
                if (isPlural == 1) {
                    desc = "a _pointer_(";
                }
                else {
                    desc = "_pointers_(";
                }
            }

            switch (i) {
            case PointerDataTypeStart: desc += "_global_"; break;
            case PointerDataTypeStart+1: desc += "_local_"; break;
            case PointerDataTypeStart+2: desc += "_private_"; break;
            case PointerDataTypeStart+3: desc += "_constant_"; break;
            case PointerDataTypeStart+4: desc += "_generic_"; break;
            }
            notFirstIter = true;
        }
    }
    if (desc != "") {
        desc += ") to ";
    }
    desc += dataType;
    return desc;
}

const std::string GetDataTypeDesc(int Ty) {
    std::string desc("");
    bool notFirstIter = false;
    for (int i = BasicDataTypeStart; i < BasicDataTypeCount + BasicDataTypeStart; i++) {
        if ((Ty >> i) & 0x1) {

            if (notFirstIter) {
                if ((Ty >> (i + 1)) == 0) {
                    desc += " or ";
                }
                else {
                    desc += ", ";
                }
            }

            switch (i) {
            case 0: desc += "_half_"; break;
            case 1: desc += "_float_"; break;
            case 2: desc += "_double_"; break;
            case 3: desc += "_bool_"; break;
            case 4: desc += "_i8_"; break;
            case 5: desc += "_i16_"; break;
            case 6: desc += "_i32_"; break;
            case 7: desc += "_i64_"; break;
            case 8: desc += "_size_t_"; break;
            case 9: desc += "_floating-point_"; break;
            case 10: desc += "_integer_"; break;
            case 11: desc += "_void_"; break;
            }
            notFirstIter = true;
        }
    }

    return desc;
}

const std::string GetVectorComponentsDesc(int numComponents) {
    if (numComponents == 0) return NULL;

    std::string desc("(");
    bool notFirstIter = false;
    for (int i = 0; i < NumVecCompCount; i++) {
        if ((numComponents >> i) & 0x1) {
            if (notFirstIter) {
                desc += ",";
            }

            switch (i) {
            case 0: desc += "2";  break;
            case 1: desc += "3";  break;
            case 2: desc += "4";  break;
            case 3: desc += "8";  break;
            case 4: desc += "16"; break;
            }
            notFirstIter = true;
        }
    }
    desc += ")";
    return desc;
}

const std::string GetImageDataTypeDesc(int Ty, bool italic) {
    std::string desc("");
    bool notFirstIter = false;
    for (int i = 0; i < ImageDataTypeCount; i++) {
        if ((Ty >> i) & 0x1) {

            if (notFirstIter) {
                if ((Ty >> (i + 1)) == 0) {
                    desc += " or ";
                }
                else {
                    desc += ", ";
                }
            }

            if (italic) desc += "_";
            switch (i) {
            case 0: desc += "image1d"; break;
            case 1: desc += "image1dBuffer"; break;
            case 2: desc += "image1dArray"; break;
            case 3: desc += "image2d"; break;
            case 4: desc += "image2dArray"; break;
            case 5: desc += "image2dArrayDepth"; break;
            case 6: desc += "image2dDepth"; break;
            case 7: desc += "image2dMsaa"; break;
            case 8: desc += "image2dArrayMsaa"; break;
            case 9: desc += "image2dMsaaDepth"; break;
            case 10: desc += "image2dArrayMsaaDepth"; break;
            case 11: desc += "image3d"; break;
            case 12: desc += "image"; break;
            case 13: desc += "sampler"; break;
            }
            if (italic) desc += "_";
            notFirstIter = true;
        }
    }

    return desc;
}
const std::string GetOperandScalarVectorTypeDesc(int vecTy, int compWidths, std::string datatype) {
    std::string desc("");
    if (vecTy & ScalarVecType) {
        desc = datatype;
        if (vecTy & VectorVecType) {
            desc += " or";
        }
    }

    if (vecTy & VectorVecType) {
        desc += " _vector" + GetVectorComponentsDesc(compWidths) + "_";
        desc +=  " of " + datatype + " values";
    }


    return desc;
}

// Print the remainder of a row as operands.
void PrintBIOperands(const BuiltInOperandParameters& operands, int reservedOperands)
{
    int numArgs = operands.getNum();
    if (numArgs == 0)
        printf(" %d+|", reservedOperands);

    for (int arg = 0; arg < numArgs; ++arg) {
        if (arg == numArgs - 1 && reservedOperands > arg + 1)
            printf(" %d+| ", reservedOperands - arg);
        else
            printf(" | ");
        printf("_%s_", GetOperandDesc(operands.getClass(arg)));
        printf(" +\n%s", operands.getDesc(arg));
    }
    printf("\n");
}

void PrintBIOperandsTypeInfo(const OperandDataTypeDesc& operand, std::vector<std::string>& names) {

    std::string text;
    bool printSep = false;
    int num = (int)names.size();
    for (int i = 0; i < num; i++) {
        if (printSep) {
            if ((i + 1) == num) {
                text += " and ";
            }
            else {
                text += ", ";
            }
        }

        text += names[i];
        printSep = true;
    }

    std::string dataType = GetDataTypeDesc(operand.dataType);
    dataType = GetOperandScalarVectorTypeDesc(operand.vecType, operand.numVecComponents, dataType);
    if (isPointer(operand.dataType)) {
        dataType = GetPointerTypeDesc(operand.dataType, dataType, (int)names.size());
    }
    printf("%s must be %s. " LINE_BREAK, text.c_str(), dataType.c_str());
}

void PrintBIParametersRules(BuiltInFunctionParameters* desc) {
    // Find the used operand data types.
    std::set<OperandDataTypeDesc> usedOperands;
    for (OperandTypeMap::iterator it = desc->sameType.begin();
        it != desc->sameType.end(); it++) {
        usedOperands.insert((*it).first);
    }

    // Print the operands (including the ones that are shared)
    for (std::set<OperandDataTypeDesc>::iterator sit = usedOperands.begin();
        sit != usedOperands.end(); sit++) {
        std::pair<OperandTypeMap::iterator, OperandTypeMap::iterator> mapIt;
        mapIt = desc->sameType.equal_range(*sit);
        std::vector<std::string> opNames;
        for (OperandTypeMap::iterator it = mapIt.first; it != mapIt.second; ++it) {
            int idx = (*it).second;
            if (idx == 0) {
                opNames.push_back(desc->resultOperand.getDesc(0));
            }
            else {
                opNames.push_back(desc->operands.getDesc(idx-1));
            }
        }
        PrintBIOperandsTypeInfo((*sit), opNames);
    }

    // print the OpenCL unique operands
    for (int i = 0; i < desc->operands.getNum(); i++ ) {
        if (desc->operands.checkIsImage(i)) {
            int imgTy = desc->operands.getImageType(i);
            std::string imgString = desc->operands.getDesc(i);
            imgString += " must be ";
            const std::string tyString = GetImageDataTypeDesc(imgTy, true);
            imgString += tyString + " value";
            if (imgTy != SamplerType){
                imgString += ", with ";
                AccessQualifier qual = desc->operands.getAccessQualifier(i);
                if (qual == AccessQualifierReadWrite) {
                    imgString += OperandClassParams[OperandAccessQualifier].getName(AccessQualifierReadOnly);
                    imgString += ", ";
                    imgString += OperandClassParams[OperandAccessQualifier].getName(AccessQualifierWriteOnly);
                    imgString += " or ";
                    imgString += OperandClassParams[OperandAccessQualifier].getName(AccessQualifierReadWrite);
                }
                else {
                    imgString += OperandClassParams[OperandAccessQualifier].getName(qual);
                    imgString += " or " + std::string(OperandClassParams[OperandAccessQualifier].getName(AccessQualifierReadWrite));
                }
                imgString += " access qualifier." LINE_BREAK;
            }
            else {
                imgString += "." LINE_BREAK;
            }
            printf("%s", imgString.c_str());
        }
    }

    // Print what operands must share the same component types
    if (desc->useDefaultIdenticalTypesMsg()) {
        printf(DEFAULT_IDENTICAL_TYPES_MESSAGE);
    } else {
        printf("%s",desc->getNonDefaultIdenticalTypesMsg().c_str());
    }

    // Print additional notes
    if (desc->hasNotes()){
        std::vector<std::string>& notes = desc->getNotes();
        for (std::vector<std::string>::iterator it = notes.begin(); it != notes.end(); it++) {
            printf("%s", (*it).c_str());
        }
    }
}
void PrintSingleOpcode(BuiltInFunctionParameters* desc, unsigned int op)
{
    if (desc->opDesc == NULL) return;

    // Compute word count and number of operands
    int numUsedOperands = desc->operands.getNum();
    bool variable = false;
    if (numUsedOperands > 0) {
        switch (desc->operands.getClass(numUsedOperands - 1)) {
        case OperandVariableIds:
        case OperandOptionalLiteral:
        case OperandVariableLiterals:
        case OperandAnySizeLiteralNumber:
        case OperandVariableLiteralId:
        case OperandLiteralString:
            variable = true;
            break;
        default:
            break;
        }
    }

    int wordCount = 1+2; // extended instructions set id, opcode
    if (desc->hasType())
        ++wordCount;
    if (desc->hasResult())
        ++wordCount;
    wordCount += numUsedOperands;

    bool capabilities = desc->capabilities.size() > 0;

    // Table start
    int width = std::max((wordCount + 1) * 12, (int)strlen(desc->opDesc) / 3);
    width = std::max(width, 40);
    printf("[cols=\"2*1,%d*3\",width=\"%d%%\"]\n", wordCount - 1, std::min(100, width));
    printf("|=====\n");

    // Name
    printf("%d+|[[%s]]*%s*", capabilities ? wordCount : wordCount + 1, desc->opName, desc->opName);

    // Semantics
    printf(" +\n +\n%s\n" LINE_BREAK, desc->opDesc);
    PrintBIParametersRules(desc);

    // Word Count
    if (variable)
        printf("| %d + variable ", wordCount - 1);
    else
        printf("| %d ", wordCount);

    // Opcode
    printf("| %d ", OpExtInst);

    // Rest of words
    if (desc->hasType())
        printf(" | %s", "_<id>_ +\n_Result Type_");
    if (desc->hasResult())
        printf(" | %s", "_Result <id>_");
    printf("| extended instructions set _<id>_");
    printf("| %d\n", op);
    PrintBIOperands(desc->operands, 0);

    // Table end
    printf("\n|=====\n");
}
void ParameterizeEnums(SPIRVersion) {/*
    ImageChannelOrderParams[OpenCLLIB::R_ChannelOrder].
    ImageChannelOrderParams[OpenCLLIB::A_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::RG_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::RA_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::RGB_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::RGBA_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::BGRA_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::ARGB_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::INTENSITY_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::LUMINANCE_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::Rx_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::RGx_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::RGBx_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::DEPTH_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::DEPTH_STENCIL_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::sRGB_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::sRGBx_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::sRGBA_ChannelOrder]
    ImageChannelOrderParams[OpenCLLIB::sBGRA_ChannelOrder]
    //ImageChannelTypeParams[OpenCLLIB::Count_ChannelType];*/

}

void ParameterizeBuiltins(SPIRVersion /*ver*/) {
    for (unsigned int i = 0; i < OclExtInstCeiling; i++) {
        BuiltInDesc[i].opName = OpenCL20DebugNames[i];
    }

#define R_IN_RADS GAP "_Result_ is an angle in radians."

    BuiltInDesc[OpenCLLIB::Acos].opDesc = "Compute the arc cosine of _x_." R_IN_RADS ;
    BuiltInDesc[OpenCLLIB::Acos].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Acos].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Acos].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Acosh].opDesc = "Compute the inverse hyperbolic cosine of _x_ .  " R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Acosh].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Acosh].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Acosh].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Acospi].opDesc = "Compute *acos*(_x_) / {pi}." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Acospi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Acospi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Acospi].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Asin].opDesc = "Compute the arc sine of _x_." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Asin].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Asin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Asin].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Asinh].opDesc = "Compute the inverse hyperbolic sine of _x_." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Asinh].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Asinh].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Asinh].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Asinpi].opDesc = "Compute *asin*(_x_) / {pi}." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Asinpi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Asinpi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Asinpi].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Atan].opDesc = "Compute the arc tangent of _x_." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Atan].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Atan].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Atan].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Atan2].opDesc = "Compute the arc tangent of _y_ / _x_." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Atan2].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Atan2].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Atan2].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Atan2].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Atanh].opDesc = "Compute the hyperbolic arc tangent of _x_." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Atanh].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Atanh].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Atanh].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Atanpi].opDesc = "Compute *atan*(_x_) / {pi}." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Atanpi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Atanpi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Atanpi].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Atan2pi].opDesc = "Compute *atan*(_y_, _x_) / {pi}." R_IN_RADS;
    BuiltInDesc[OpenCLLIB::Atan2pi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Atan2pi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Atan2pi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Atan2pi].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Cbrt].opDesc = "Compute the cube root of _x_.";
    BuiltInDesc[OpenCLLIB::Cbrt].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Cbrt].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Cbrt].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Ceil].opDesc = "Round _x_ to integral value using the round to positive infinity rounding mode.";
    BuiltInDesc[OpenCLLIB::Ceil].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Ceil].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Ceil].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Copysign].opDesc = "Returns _x_ with its sign changed to match the sign of _y_.";
    BuiltInDesc[OpenCLLIB::Copysign].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Copysign].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Copysign].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Copysign].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Cos].opDesc = "Compute the cosine of _x_ radians.";
    BuiltInDesc[OpenCLLIB::Cos].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Cos].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Cos].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Cosh].opDesc = "Compute the hyperbolic cosine of _x_ radians.";
    BuiltInDesc[OpenCLLIB::Cosh].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Cosh].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Cosh].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Cospi].opDesc = "Compute *cos*(_x_) / {pi} radians.";
    BuiltInDesc[OpenCLLIB::Cospi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Cospi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Cospi].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Erfc].opDesc = "Complementary error function of _x_." ;
    BuiltInDesc[OpenCLLIB::Erfc].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Erfc].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Erfc].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Erf].opDesc = "Error function of _x_ encountered in integrating the normal distribution.";
    BuiltInDesc[OpenCLLIB::Erf].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Erf].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Erf].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Exp].opDesc = "Compute the base-e exponential of _x_. (i.e. _e_^_x_^)";
    BuiltInDesc[OpenCLLIB::Exp].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Exp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Exp].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Exp2].opDesc = "Computes 2 raised to the power of _x_. (i.e. _2_^_x_^)";
    BuiltInDesc[OpenCLLIB::Exp2].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Exp2].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Exp2].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Exp10].opDesc = "Computes 10 raised to the power of _x_. (i.e. _10_^_x_^)";
    BuiltInDesc[OpenCLLIB::Exp10].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Exp10].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Exp10].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Expm1].opDesc = "Computes _e_^_x_^ _- 1.0_ .";
    BuiltInDesc[OpenCLLIB::Expm1].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Expm1].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Expm1].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Fabs].opDesc = "Compute the absolute value of _x_.";
    BuiltInDesc[OpenCLLIB::Fabs].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fabs].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Fabs].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Fdim].opDesc = "_x - y_ if _x > y_, _+0_ if _x_ is less than or equal to _y_.";
    BuiltInDesc[OpenCLLIB::Fdim].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fdim].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Fdim].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Fdim].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Floor].opDesc = "Round _x_ to the integral value using the round to negative infinity rounding mode.";
    BuiltInDesc[OpenCLLIB::Floor].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Floor].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Floor].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Fma].opDesc = "Compute the correctly rounded floating-point representation of the sum of _c_ with the infinitely precise product of _a_ and _b_. "
        "Rounding of intermediate products shall not occur. Edge case results are per the IEEE 754-2008 standard.";
    BuiltInDesc[OpenCLLIB::Fma].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fma].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_a_");
    BuiltInDesc[OpenCLLIB::Fma].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_b_");
    BuiltInDesc[OpenCLLIB::Fma].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_c_");
    BuiltInDesc[OpenCLLIB::Fma].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Fmax].opDesc = "Returns _y_ if _x_ < _y_, otherwise it returns _x_. "
        "If one operand is a NaN, *fmax* returns the other argument.  If both arguments are NaNs, *fmax* returns a NaN.";
    BuiltInDesc[OpenCLLIB::Fmax].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fmax].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Fmax].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Fmax].addNote(NOTE "*fmax* behaves as defined by C99 and may not match the IEEE 754-2008 definition for *maxNum* with regard to signaling NaNs. "
        "Specifically, signaling NaNs may behave as quiet NaNs");
    BuiltInDesc[OpenCLLIB::Fmax].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Fmin].opDesc = "Returns _y_ if _y_ < _x_, otherwise it returns _x_. "
        "If one operand is a NaN, *fmin* returns the other argument.  If both arguments are NaNs, *fmin* returns a NaN.";
    BuiltInDesc[OpenCLLIB::Fmin].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fmin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Fmin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Fmin].addNote(NOTE "*fmin* behaves as defined by C99 and may not match the IEEE 754-2008 definition for *minNum* with regard to signaling NaNs. "
        "Specifically, signaling NaNs may behave as quiet NaNs");
    BuiltInDesc[OpenCLLIB::Fmin].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Fmod].opDesc = "Modulus. Returns _x_ - _y_ * *trunc*(_x_/_y_).";
    BuiltInDesc[OpenCLLIB::Fmod].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fmod].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Fmod].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Fmod].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Fract].opDesc = "Returns *fmin*( _x_ - *floor*(_x_), _0x1.fffffep-1f_ ). *floor*(_x_) is returned in _ptr_.";
    BuiltInDesc[OpenCLLIB::Fract].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fract].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Fract].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_ptr_");
    BuiltInDesc[OpenCLLIB::Fract].overrideIdenticalTypesMsg(std::string(IDENTICAL_TYPES_AND_POINTERS_MESSAGE));
    BuiltInDesc[OpenCLLIB::Fract].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Frexp].opDesc = "Extract the mantissa and exponent from _x_. The _Result Type_ holds the mantissa, and _exp_ points to the exponent. "
        "For each component the mantissa returned is a _floating-point_ with magnitude in the interval [1/2, 1) or 0.  Each component of _x_ equals mantissa returned * 2^exp^.";
    BuiltInDesc[OpenCLLIB::Frexp].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Frexp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Frexp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_exp_");
    BuiltInDesc[OpenCLLIB::Frexp].overrideIdenticalTypesMsg(std::string(RES_TYPE " and _x_ operands must be of the same type. "
        "_exp_ operand must point to an _i32_ with the same component count as " RES_TYPE " and _x_ operands." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::Frexp].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Hypot].opDesc = "Compute the value of the square root of _x_^2^+ _y_^2^ without undue overflow or underflow.";
    BuiltInDesc[OpenCLLIB::Hypot].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Hypot].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Hypot].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Hypot].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Ilogb].opDesc = "Return the exponent of _x_ as an _i32_ value.";
    BuiltInDesc[OpenCLLIB::Ilogb].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Ilogb].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Ilogb].overrideIdenticalTypesMsg(std::string(RES_TYPE " and _x_ operands must have the same component count." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::Ilogb].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Ldexp].opDesc = "Multiply _x_ by 2 to the power _k_.";
    BuiltInDesc[OpenCLLIB::Ldexp].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Ldexp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Ldexp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_k_");
    BuiltInDesc[OpenCLLIB::Ldexp].overrideIdenticalTypesMsg(std::string(RES_TYPE " and _x_ operands must be of the same type. "
        "_k_ operand must have the same component count as " RES_TYPE " and _x_ operands." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::Ldexp].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Lgamma].opDesc = "Log gamma function of _x_. Returns the natural logarithm of the absolute value of the gamma function.";
    BuiltInDesc[OpenCLLIB::Lgamma].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Lgamma].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Lgamma].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Lgamma_r].opDesc = "Log gamma function of _x_. Returns the natural logarithm of the absolute value of the gamma function. "
        "The sign of the gamma function is returned in the _signp_ operand";
    BuiltInDesc[OpenCLLIB::Lgamma_r].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Lgamma_r].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Lgamma_r].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_signp_");
    BuiltInDesc[OpenCLLIB::Lgamma_r].overrideIdenticalTypesMsg(std::string(RES_TYPE " and _x_ operands must be of the same type. "
        "_signp_ operand must point to an _i32_ with the same component count as " RES_TYPE " and _x_ operands." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::Lgamma_r].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Log].opDesc = "Compute the natural logarithm of _x_.";
    BuiltInDesc[OpenCLLIB::Log].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Log].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Log].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Log2].opDesc = "Compute the base 2 logarithm of _x_.";
    BuiltInDesc[OpenCLLIB::Log2].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Log2].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Log2].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Log10].opDesc = "Compute the base 10 logarithm of _x_.";
    BuiltInDesc[OpenCLLIB::Log10].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Log10].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Log10].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Log1p].opDesc = "Compute *log~e~*(1.0 + _x_).";
    BuiltInDesc[OpenCLLIB::Log1p].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Log1p].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Log1p].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Logb].opDesc = "Compute the exponent of _x_, which is the integral part of log~r~  \\| _x_ \\|.";
    BuiltInDesc[OpenCLLIB::Logb].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Logb].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Logb].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Mad].opDesc = "Computes _a_ * _b_ + _c_.  *mad* may compute _a_ * _b_ + _c_ with reduced accuracy in the "
        "embedded profile - see the OpenCL SPIR-V Environment specification for details. On some hardware the *mad* instruction may "
        "provide better performance than the expanded computation of _a_ * _b_ + _c_.";
    BuiltInDesc[OpenCLLIB::Mad].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Mad].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_a_");
    BuiltInDesc[OpenCLLIB::Mad].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_b_");
    BuiltInDesc[OpenCLLIB::Mad].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_c_");
    BuiltInDesc[OpenCLLIB::Mad].addNote(NOTE "For some usages, e.g. *mad*(_a_, _b_, -_a_ * _b_), "
        "the definition of *mad* is loose enough that almost any result is allowed from *mad* for some values of _a_ and _b_.");
    BuiltInDesc[OpenCLLIB::Mad].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Maxmag].opDesc = "Returns _x_ if \\| _x_ \\| > \\| _y_ \\| , _y_ if \\| _y_ \\| > \\| _x_ \\| , otherwise *fmax*(_x_, _y_).";
    BuiltInDesc[OpenCLLIB::Maxmag].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Maxmag].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Maxmag].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Maxmag].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Minmag].opDesc = "Returns _x_ if \\| _x_ \\| < \\| _y_ \\|, _y_ if \\| _y_ \\| < \\| _x_ \\|, otherwise *fmin*(_x_, _y_).";
    BuiltInDesc[OpenCLLIB::Minmag].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Minmag].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Minmag].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Minmag].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Modf].opDesc = "Decompose a _floating-point_ number. The *modf* instruction breaks the operand _x_ into integral and fractional parts, "
        "each of which has the same sign as the operand. It stores the integral part in the object pointed to by _iptr_";
    BuiltInDesc[OpenCLLIB::Modf].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Modf].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Modf].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_iptr_");
    BuiltInDesc[OpenCLLIB::Modf].overrideIdenticalTypesMsg(std::string(IDENTICAL_TYPES_AND_POINTERS_MESSAGE));
    BuiltInDesc[OpenCLLIB::Modf].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Nan].opDesc = "Returns a quiet NaN. The _nancode_ may be placed in the significand of the resulting NaN.";
    BuiltInDesc[OpenCLLIB::Nan].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Nan].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_nancode_");
    BuiltInDesc[OpenCLLIB::Nan].overrideIdenticalTypesMsg(std::string(RES_TYPE " and _nancode_ operands must have the same component count. "
        "The primitive data type size of _nancode_ and _Result Type_ must be equal." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::Nan].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Nextafter].opDesc = "Computes the next representable _floating-point_ value following _x_ in the direction of _y_. "
        "Thus, if _y_ is less than _x_, *nextafter* returns the largest representable floating-point number less than _x_.";
    BuiltInDesc[OpenCLLIB::Nextafter].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Nextafter].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Nextafter].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Nextafter].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Pow].opDesc = "Compute _x_ to the power _y_.";
    BuiltInDesc[OpenCLLIB::Pow].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Pow].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Pow].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Pow].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Pown].opDesc = "Compute _x_ to the power _y_, where _y_ is an _i32_ integer.";
    BuiltInDesc[OpenCLLIB::Pown].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Pown].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Pown].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::Pown].overrideIdenticalTypesMsg(std::string(RES_TYPE " and _x_ operands must be of the same type. "
        "_y_ operand must have the same component count as " RES_TYPE " and _x_ operands." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::Pown].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Powr].opDesc = "Compute _x_ to the power _y_, where _x_ is {ge} 0.";
    BuiltInDesc[OpenCLLIB::Powr].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Powr].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Powr].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Powr].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Remainder].opDesc = "Compute the value r such that r = _x_ - n*_y_, where n is the integer nearest the exact value of _x_/_y_. "
        "If there are two integers closest to _x_/_y_, n shall be the even one. If r is zero, it is given the same sign as _x_.";
    BuiltInDesc[OpenCLLIB::Remainder].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Remainder].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Remainder].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Remainder].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Remquo].opDesc = "The *remquo* instruction computes the value r such that r = _x_ - k*_y_, "
    "where k is the integer nearest the exact value of _x_/_y_.  If there are two integers closest to _x_/_y_, k shall be the even one. "
    "If r is zero, it is given the same sign as _x_.  This is the same value that is returned by the *remainder* instruction. "
    "*remquo* also calculates at least the lower seven bits of the integral quotient _x_/_y_, and gives that value the same sign as _x_/_y_. "
    "It stores this signed value in the object pointed to by _quo_.";
    BuiltInDesc[OpenCLLIB::Remquo].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Remquo].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Remquo].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Remquo].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_quo_");
    BuiltInDesc[OpenCLLIB::Remquo].overrideIdenticalTypesMsg(std::string(RES_TYPE ", _x_ and _y_ operands must be of the same type. "
        "_quo_ operand must point to an _i32_ with the same component count as " RES_TYPE ", _x_ and _y_ operands." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::Remquo].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Rint].opDesc = "Round _x_ to integral value (using round to nearest even rounding mode) in floating-point format.";
    BuiltInDesc[OpenCLLIB::Rint].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Rint].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Rint].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Rootn].opDesc = "Compute _x_ to the power 1/_y_.";
    BuiltInDesc[OpenCLLIB::Rootn].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Rootn].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Rootn].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::Rootn].overrideIdenticalTypesMsg(std::string(RES_TYPE " and _x_ operands must be of the same type. "
        "_y_ operand must have the same component count as " RES_TYPE " and _x_ operands." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::Rootn].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Round].opDesc = "Return the integral value nearest to _x_ rounding halfway cases away from zero, regardless of the current rounding direction.";
    BuiltInDesc[OpenCLLIB::Round].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Round].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Round].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Rsqrt].opDesc = "Compute inverse square root of _x_.";
    BuiltInDesc[OpenCLLIB::Rsqrt].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Rsqrt].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Rsqrt].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Sin].opDesc = "Compute sine of _x_ radians.";
    BuiltInDesc[OpenCLLIB::Sin].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Sin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Sin].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Sincos].opDesc = "Compute sine and cosine of _x_ radians. The computed sine is the return value and computed cosine is returned in _cosval_.";
    BuiltInDesc[OpenCLLIB::Sincos].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Sincos].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Sincos].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_cosval_");
    BuiltInDesc[OpenCLLIB::Sincos].overrideIdenticalTypesMsg(std::string(IDENTICAL_TYPES_AND_POINTERS_MESSAGE));
    BuiltInDesc[OpenCLLIB::Sincos].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Sinh].opDesc = "Compute hyperbolic sine of _x_ radians.";
    BuiltInDesc[OpenCLLIB::Sinh].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Sinh].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Sinh].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Sinpi].opDesc = "Compute _sin_ ({pi} x) radians.";
    BuiltInDesc[OpenCLLIB::Sinpi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Sinpi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Sinpi].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Sqrt].opDesc = "Compute square root of _x_.";
    BuiltInDesc[OpenCLLIB::Sqrt].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Sqrt].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Sqrt].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Tan].opDesc = "Compute tangent of _x_ radians.";
    BuiltInDesc[OpenCLLIB::Tan].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Tan].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Tan].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Tanh].opDesc = "Compute hyperbolic tangent of _x_ radians.";
    BuiltInDesc[OpenCLLIB::Tanh].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Tanh].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Tanh].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Tanpi].opDesc = "Compute _tan_ ({pi} x) radians.";
    BuiltInDesc[OpenCLLIB::Tanpi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Tanpi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Tanpi].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Tgamma].opDesc = "Compute the gamma function of _x_.";
    BuiltInDesc[OpenCLLIB::Tgamma].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Tgamma].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Tgamma].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Trunc].opDesc = "Round _x_ to integral value using the round to zero rounding mode.";
    BuiltInDesc[OpenCLLIB::Trunc].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Trunc].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Trunc].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_cos].opDesc = "Compute cosine of _x_ radians."
        " The resulting value is _poison_ if _x_ is not in the range -2^16^ ... +2^16^.ha";
    BuiltInDesc[OpenCLLIB::Half_cos].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_cos].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_cos].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_divide].opDesc = "Compute _x_ / _y_.";
    BuiltInDesc[OpenCLLIB::Half_divide].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_divide].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_divide].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::Half_divide].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_exp].opDesc = "Compute the base-e exponential of _x_.";
    BuiltInDesc[OpenCLLIB::Half_exp].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_exp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_exp].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_exp2].opDesc = "Compute the base 2 exponential of _x_.";
    BuiltInDesc[OpenCLLIB::Half_exp2].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_exp2].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_exp2].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_exp10].opDesc = "Compute the base 10 exponential of _x_.";
    BuiltInDesc[OpenCLLIB::Half_exp10].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_exp10].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_exp10].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_log].opDesc = "Compute the natural logarithm of _x_.";
    BuiltInDesc[OpenCLLIB::Half_log].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_log].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_log].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_log2].opDesc = "Compute the base 2 logarithm of _x_.";
    BuiltInDesc[OpenCLLIB::Half_log2].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_log2].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_log2].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_log10].opDesc = "Compute the base 10 logarithm of _x_.";
    BuiltInDesc[OpenCLLIB::Half_log10].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_log10].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_log10].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_powr].opDesc = "Compute _x_ to the power _y_, where _x_ is {ge} 0.";
    BuiltInDesc[OpenCLLIB::Half_powr].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_powr].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_powr].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::Half_powr].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_recip].opDesc = "Compute the reciprocal of _x_.";
    BuiltInDesc[OpenCLLIB::Half_recip].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_recip].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_recip].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_rsqrt].opDesc = "Compute the inverse square root of _x_.";
    BuiltInDesc[OpenCLLIB::Half_rsqrt].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_rsqrt].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_rsqrt].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_sin].opDesc = "Compute the sine of _x_ radians."
        " The resulting value is _poison_ if _x_ is not in the range -2^16^ ... +2^16^.";
    BuiltInDesc[OpenCLLIB::Half_sin].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_sin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_sin].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_sqrt].opDesc = "Compute the square root of _x_.";
    BuiltInDesc[OpenCLLIB::Half_sqrt].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_sqrt].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_sqrt].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Half_tan].opDesc = "Compute tangent value of _x_ radians."
        " The resulting values are _poison_ if _x_ is not in the range -2^16^ ... +2^16^.";
    BuiltInDesc[OpenCLLIB::Half_tan].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Half_tan].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Half_tan].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_cos].opDesc = "Compute cosine of _x_ radians over an implementation-defined range. The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_cos].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_cos].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_cos].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_cos].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_divide].opDesc = "Compute _x_ / _y_ over an implementation-defined range. The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_divide].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_divide].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_divide].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::Native_divide].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_divide].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_exp].opDesc = "Compute the base-e exponential of _x_ over an implementation-defined range.  The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_exp].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_exp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_exp].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_exp].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_exp2].opDesc = "Compute the base- 2 exponential of _x_ over an implementation-defined range.  The maximum error is implementation-defined..";
    BuiltInDesc[OpenCLLIB::Native_exp2].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_exp2].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_exp2].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_exp2].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_exp10].opDesc = "Compute the base- 10 exponential of _x_ over an implementation-defined range.  The maximum error is implementation-defined..";
    BuiltInDesc[OpenCLLIB::Native_exp10].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_exp10].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_exp10].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_exp10].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_log].opDesc = "Compute natural logarithm of _x_ over an implementation-defined range.  The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_log].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_log].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_log].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_log].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_log2].opDesc = "Compute a base 2 logarithm of _x_ over an implementation-defined range.  The maximum error is implementation-defined. ";
    BuiltInDesc[OpenCLLIB::Native_log2].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_log2].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_log2].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_log2].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_log10].opDesc = "Compute a base 10 logarithm of _x_ over an implementation-defined range.  The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_log10].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_log10].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_log10].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_log10].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_powr].opDesc = "Compute _x_ to the power _y_, where _x_ is {ge} 0.";
    BuiltInDesc[OpenCLLIB::Native_powr].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_powr].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_powr].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::Native_powr].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_powr].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_recip].opDesc = "Compute reciprocal of _x_ over an implementation-defined range. The range of x and y are implementation-defined. "
        "The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_recip].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_recip].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_recip].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_recip].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_rsqrt].opDesc = "Compute inverse square root of _x_ over an implementation-defined range.  The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_rsqrt].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_rsqrt].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_rsqrt].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_rsqrt].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_sin].opDesc = "Compute sine of _x_ radians over an implementation-defined range.  The maximum error is implementation-defined. ";
    BuiltInDesc[OpenCLLIB::Native_sin].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_sin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_sin].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_sin].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_sqrt].opDesc = "Compute the square root of _x_ over an implementation-defined range.  The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_sqrt].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_sqrt].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_sqrt].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_sqrt].extInstClass = OclExInstClassMath;

    BuiltInDesc[OpenCLLIB::Native_tan].opDesc = "Compute tangent value of _x_ radians over an implementation-defined range.  The maximum error is implementation-defined.";
    BuiltInDesc[OpenCLLIB::Native_tan].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Native_tan].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Float32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::Native_tan].addNote(NOTE NATIVE_FUNC_NOTE);
    BuiltInDesc[OpenCLLIB::Native_tan].extInstClass = OclExInstClassMath;


    // INTEGER FUNCTIONS
    BuiltInDesc[OpenCLLIB::SAbs].opDesc = "Returns \\|_x_\\|, where _x_ is treated as signed integer.";
    BuiltInDesc[OpenCLLIB::SAbs].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SAbs].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SAbs].extInstClass = OclExInstClassIntegers;
    BuiltInDesc[OpenCLLIB::SAbs].addNote(NO_SIGNED_WRAP);

    BuiltInDesc[OpenCLLIB::UAbs].opDesc = "Returns \\|_x_\\|, where _x_ is treated as unsigned integer.";
    BuiltInDesc[OpenCLLIB::UAbs].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UAbs].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UAbs].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SAbs_diff].opDesc = "Returns \\| _x_ - _y_ \\| without modulo overflow, where _x_ and _y_ are treated as signed integers.";
    BuiltInDesc[OpenCLLIB::SAbs_diff].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SAbs_diff].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SAbs_diff].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SAbs_diff].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UAbs_diff].opDesc = "Returns \\| _x_ - _y_ \\| without modulo overflow, where _x_ and _y_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::UAbs_diff].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UAbs_diff].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UAbs_diff].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::UAbs_diff].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SAdd_sat].opDesc = "Returns the saturated value of _x_ + _y_, where _x_ and _y_ are treated as signed integers.";
    BuiltInDesc[OpenCLLIB::SAdd_sat].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SAdd_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SAdd_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SAdd_sat].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UAdd_sat].opDesc = "Returns the saturated value of _x_ + _y_, where _x_ and _y_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::UAdd_sat].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UAdd_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UAdd_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::UAdd_sat].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SHadd].opDesc = "Returns the value of (_x_ + _y_) >> 1, where _x_ and _y_ are treated as signed integers. The intermediate sum does not modulo overflow.";
    BuiltInDesc[OpenCLLIB::SHadd].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SHadd].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SHadd].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SHadd].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UHadd].opDesc = "Returns the value of (_x_ + _y_) >> 1, where _x_ and _y_ are treated as unsigned integers. The intermediate sum does not modulo overflow.";
    BuiltInDesc[OpenCLLIB::UHadd].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UHadd].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UHadd].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::UHadd].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SRhadd].opDesc = "Returns the value of (_x_ + _y_ + 1) >> 1, where _x_ and _y_ are treated as signed integers. The intermediate sum does not modulo overflow.";
    BuiltInDesc[OpenCLLIB::SRhadd].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SRhadd].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SRhadd].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SRhadd].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::URhadd].opDesc = "Returns the value of (_x_ + _y_ + 1) >> 1, where _x_ and _y_ are treated as unsigned integers. The intermediate sum does not modulo overflow.";
    BuiltInDesc[OpenCLLIB::URhadd].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::URhadd].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::URhadd].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::URhadd].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SClamp].opDesc =
        "Returns _s_min_(_s_max_(_x_,_minval_),_maxval_), where _x_, _minval_, and _maxval_ are treated as signed integers."
        " The resulting values are _poison_ if _minval_ > _maxval_.";
    BuiltInDesc[OpenCLLIB::SClamp].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_minval_");
    BuiltInDesc[OpenCLLIB::SClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_maxval_");
    BuiltInDesc[OpenCLLIB::SClamp].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UClamp].opDesc =
        "Returns _u_min_(_u_max_(_x_,_minval_),_maxval_), where _x_, _minval_, and _maxval_ are treated as unsigned integers."
        " The resulting values are _poison_ if _minval_ > _maxval_.";
    BuiltInDesc[OpenCLLIB::UClamp].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_minval_");
    BuiltInDesc[OpenCLLIB::UClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_maxval_");
    BuiltInDesc[OpenCLLIB::UClamp].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::Clz].opDesc = "Returns the number of leading 0 bits in _x_, starting at the most significant bit position. "
        "If _x_ is 0, returns the size in bits of the type of _x_ or component type of _x_, if _x_ is a vector.";
    BuiltInDesc[OpenCLLIB::Clz].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Clz].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Clz].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::Ctz].opDesc = "Returns the count of trailing 0 bits in _x_. "
        "If _x_ is 0, returns the size in bits of the type of _x_ or component type of _x_, if _x_ is a vector.";
    BuiltInDesc[OpenCLLIB::Ctz].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Ctz].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Ctz].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SMad_hi].opDesc = "Returns _mul_hi_(_a_, _b_) + _c_, where _a_,_b_ and _c_ are treated as signed integers.";
    BuiltInDesc[OpenCLLIB::SMad_hi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SMad_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_a_");
    BuiltInDesc[OpenCLLIB::SMad_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_b_");
    BuiltInDesc[OpenCLLIB::SMad_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_c_");
    BuiltInDesc[OpenCLLIB::SMad_hi].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UMad_hi].opDesc = "Returns _mul_hi_(_a_, _b_) + _c_, where _a_,_b_ and _c_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::UMad_hi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UMad_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_a_");
    BuiltInDesc[OpenCLLIB::UMad_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_b_");
    BuiltInDesc[OpenCLLIB::UMad_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_c_");
    BuiltInDesc[OpenCLLIB::UMad_hi].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UMin].opDesc = "Returns _y_ if _y_ < _x_, otherwise it returns _x_, where _x_ and _y_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::UMin].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UMin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UMin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::UMin].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SMin].opDesc = "Returns _y_ if _y_ < _x_, otherwise it returns _x_, where _x_ and _y_ are treated as signed integers.";
    BuiltInDesc[OpenCLLIB::SMin].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SMin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SMin].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SMin].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UMad_sat].opDesc = "Returns _x_ * _y_ + _z_ and saturates the result where _x_, _y_ and _z_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::UMad_sat].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UMad_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UMad_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::UMad_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_z_");
    BuiltInDesc[OpenCLLIB::UMad_sat].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SMad_sat].opDesc = "Returns _x_ * _y_ + _z_ and saturates the result where _x_, _y_ and _z_ are treated as signed integers.";
    BuiltInDesc[OpenCLLIB::SMad_sat].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SMad_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SMad_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SMad_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_z_");
    BuiltInDesc[OpenCLLIB::SMad_sat].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SMax].opDesc = "Returns _y_ if _x_ < _y_, otherwise it returns _x_, where _x_ and _y_ are treated as signed integers.";
    BuiltInDesc[OpenCLLIB::SMax].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SMax].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SMax].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SMax].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UMax].opDesc = "Returns _y_ if _x_ < _y_, otherwise it returns _x_, where _x_ and _y_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::UMax].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UMax].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UMax].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::UMax].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SMul_hi].opDesc = "Computes _x_ * _y_ and returns the high half of the product of _x_ and _y_, where _x_ and _y_ are treated as signed integers.";
    BuiltInDesc[OpenCLLIB::SMul_hi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SMul_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SMul_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SMul_hi].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UMul_hi].opDesc = "Computes _x_ * _y_ and returns the high half of the product of _x_ and _y_, where _x_ and _y_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::UMul_hi].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UMul_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::UMul_hi].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::UMul_hi].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::Rotate].opDesc = "For each element in _v_, the bits are shifted left by the number of bits given by the corresponding element in _i_."
        " Bits shifted off the left side of the element are shifted back in from the right.";
    BuiltInDesc[OpenCLLIB::Rotate].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Rotate].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_v_");
    BuiltInDesc[OpenCLLIB::Rotate].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_i_");
    BuiltInDesc[OpenCLLIB::Rotate].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SSub_sat].opDesc = "Returns the saturated value of _x_ - _y_, where _x_ and _y_ are treated as signed integers.";
    BuiltInDesc[OpenCLLIB::SSub_sat].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SSub_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::SSub_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::SSub_sat].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::USub_sat].opDesc = "Returns the saturated value of _x_ - _y_, where _x_ and _y_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::USub_sat].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::USub_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::USub_sat].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_y_");
    BuiltInDesc[OpenCLLIB::USub_sat].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::U_Upsample].opDesc =
        "If _hi_ and _lo_ component type is i8:" LINE_BREAK
        "Result = ((upcast...to i16)_hi_ << 8) \\| _lo_" GAP
        "If _hi_ and _lo_ component type is i16:" LINE_BREAK
        "Result = ((upcast...to i32)_hi_ << 16) \\| _lo_" GAP
        "If _hi_ and _lo_ component i32:" LINE_BREAK
        "Result = ((upcast...to i64)_hi_ << 32) \\| _lo_" GAP
        "_hi_ and _lo_ are treated as unsigned integers.";
    BuiltInDesc[OpenCLLIB::U_Upsample].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int16DataType | Int32DataType | Int64DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::U_Upsample].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int8DataType | Int16DataType | Int32DataType, "_hi_");
    BuiltInDesc[OpenCLLIB::U_Upsample].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int8DataType | Int16DataType | Int32DataType, "_lo_");
    BuiltInDesc[OpenCLLIB::U_Upsample].overrideIdenticalTypesMsg(std::string("_hi_ and _lo_ operands must be of the same type. "
        "If _hi_ and _lo_ component type is i8, the " RES_TYPE " component type must be i16. "
        "If _hi_ and _lo_ component type is i16, the " RES_TYPE " component type must be i32. "
        "If _hi_ and _lo_ component type is i32, the " RES_TYPE " component type must be i64. "
        RES_TYPE " must have the same component count as _hi_ and _lo_ operands." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::U_Upsample].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::S_Upsample].opDesc =
        "If _hi_ and _lo_ component type is i8:" LINE_BREAK
        "Result = ((upcast...to i16)_hi_ << 8) \\| _lo_" GAP
        "If _hi_ and _lo_ component type is i16:" LINE_BREAK
        "Result = ((upcast...to i32)_hi_ << 16) \\| _lo_" GAP
        "If _hi_ and _lo_ component i32:" LINE_BREAK
        "Result = ((upcast...to i64)_hi_ << 32) \\| _lo_" GAP
        "_hi_ is treated as a signed integer and _lo_ is treated as an unsigned integer.";
    BuiltInDesc[OpenCLLIB::S_Upsample].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int16DataType | Int32DataType | Int64DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::S_Upsample].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int8DataType | Int16DataType | Int32DataType, "_hi_");
    BuiltInDesc[OpenCLLIB::S_Upsample].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int8DataType | Int16DataType | Int32DataType, "_lo_");
    BuiltInDesc[OpenCLLIB::S_Upsample].overrideIdenticalTypesMsg(std::string("_hi_ and _lo_ operands must be of the same type. "
        "If _hi_ and _lo_ component type is i8, the " RES_TYPE " component type must be i16. "
        "If _hi_ and _lo_ component type is i16, the " RES_TYPE " component type must be i32. "
        "If _hi_ and _lo_ component type is i32, the " RES_TYPE " component type must be i64. "
        RES_TYPE " must have the same component count as _hi_ and _lo_ operands." LINE_BREAK));
    BuiltInDesc[OpenCLLIB::S_Upsample].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::Popcount].opDesc = "Returns the number of non-zero bits in _x_.";
    BuiltInDesc[OpenCLLIB::Popcount].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Popcount].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Popcount].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SMad24].opDesc = "Multiply two 24-bit integer values _x_ and _y_ and add the 32-bit integer result to the 32-bit integer _z_. "
        "Refer to definition of s_mul24 to see how the 24-bit integer multiplication is performed.";
    BuiltInDesc[OpenCLLIB::SMad24].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SMad24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::SMad24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::SMad24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_z_");
    BuiltInDesc[OpenCLLIB::SMad24].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UMad24].opDesc = "Multiply two 24-bit integer values _x_ and _y_ and add the 32-bit integer result to the 32-bit integer _z_. "
        "Refer to definition of u_mul24 to see how the 24-bit integer multiplication is performed.";
    BuiltInDesc[OpenCLLIB::UMad24].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UMad24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::UMad24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::UMad24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_z_");
    BuiltInDesc[OpenCLLIB::UMad24].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::SMul24].opDesc = "Multiply two 24-bit integer values _x_ and _y_, where _x_ and _y_ are treated as signed integers. "
        "_x_ and _y_ are 32-bit integers but only the low-order 24 bits are used to perform the multiplication. "
        "s_mul24 should only be used if values in _x_ and _y_ are in the range [-2^23^, 2^23^-1]. "
        "If _x_ and _y_ are not in this range, the multiplication result is implementation-defined.";
    BuiltInDesc[OpenCLLIB::SMul24].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::SMul24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::SMul24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::SMul24].extInstClass = OclExInstClassIntegers;

    BuiltInDesc[OpenCLLIB::UMul24].opDesc = "Multiply two 24-bit integer values _x_ and _y_, where _x_ and _y_ are treated as unsigned integers. "
        "_x_ and _y_ are 32-bit integers but only the low-order 24 bits are used to perform the multiplication. "
        "u_mul24 should only be used if values in _x_ and _y_ are in the range [0, 2^24^-1]. "
        "If _x_ and _y_ are not in this range, the multiplication result is implementation-defined.";
    BuiltInDesc[OpenCLLIB::UMul24].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::UMul24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_x_");
    BuiltInDesc[OpenCLLIB::UMul24].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, Int32DataType, "_y_");
    BuiltInDesc[OpenCLLIB::UMul24].extInstClass = OclExInstClassIntegers;

    // COMMON FUNCTIONS
    BuiltInDesc[OpenCLLIB::FClamp].opDesc = "Returns _fmin_(_fmax_(_x_, _minval_), _maxval_)."
        " The resulting values are _poison_ if _minval_ > _maxval_.";
    BuiltInDesc[OpenCLLIB::FClamp].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::FClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::FClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_minval_");
    BuiltInDesc[OpenCLLIB::FClamp].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_maxval_");
    BuiltInDesc[OpenCLLIB::FClamp].extInstClass = OclExInstClassCommon;

    BuiltInDesc[OpenCLLIB::Degrees].opDesc = "Converts _radians_ to degrees, i.e. (180 / {pi}) * _radians_.";
    BuiltInDesc[OpenCLLIB::Degrees].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Degrees].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_radians_");
    BuiltInDesc[OpenCLLIB::Degrees].extInstClass = OclExInstClassCommon;

    BuiltInDesc[OpenCLLIB::FMin_common].opDesc = "Returns _y_ if _y_ < _x_, otherwise it returns _x_. "
        "If x or y are infinite or NaN, the resulting values are _poison_.";
    BuiltInDesc[OpenCLLIB::FMin_common].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::FMin_common].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::FMin_common].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::FMin_common].extInstClass = OclExInstClassCommon;

    BuiltInDesc[OpenCLLIB::FMax_common].opDesc = "Returns _y_ if _x_ < _y_, otherwise it returns _x_. "
        "If x or y are infinite or NaN, the resulting values are _poison_.";
    BuiltInDesc[OpenCLLIB::FMax_common].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::FMax_common].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::FMax_common].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::FMax_common].extInstClass = OclExInstClassCommon;

    BuiltInDesc[OpenCLLIB::Mix].opDesc = "Returns the linear blend of _x_ & _y_ implemented as:" LINE_BREAK
        "_x_ + (_y_ - _x_) * _a_";
    BuiltInDesc[OpenCLLIB::Mix].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Mix].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Mix].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Mix].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_a_");
    BuiltInDesc[OpenCLLIB::Mix].addNote(NOTE CONTRACTIONS_NOTE);
    BuiltInDesc[OpenCLLIB::Mix].extInstClass = OclExInstClassCommon;

    BuiltInDesc[OpenCLLIB::Radians].opDesc = "Converts _degrees_ to radians, i.e. ({pi} / 180) * _degrees_.";
    BuiltInDesc[OpenCLLIB::Radians].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Radians].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_degrees_");
    BuiltInDesc[OpenCLLIB::Radians].extInstClass = OclExInstClassCommon;

    BuiltInDesc[OpenCLLIB::Step].opDesc = "Returns 0.0 if _x_ < _edge_, otherwise it returns 1.0.";
    BuiltInDesc[OpenCLLIB::Step].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Step].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_edge_");
    BuiltInDesc[OpenCLLIB::Step].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Step].extInstClass = OclExInstClassCommon;

    BuiltInDesc[OpenCLLIB::Smoothstep].opDesc = "Returns 0.0 if _x_ {le} _edge~0~_ and 1.0 if _x_ {ge} _edge~1~_ "
        "and performs smooth Hermite interpolation between 0 and 1, if _edge~0~_ < x < _edge~1~_."
        GAP " This is equivalent to :" LINE_BREAK
        " t = _fclamp_((_x_ - _edge~0~_) / (_edge~1~_ - _edge~0~_), 0, 1);" LINE_BREAK
        " return t * t * (3 - 2 * t);"
        GAP "The resulting values are _poison_ if _edge~0~_ {ge} _edge~1~_ or if _x_, _edge~0~_ or _edge~1~_ is a NaN.";
    BuiltInDesc[OpenCLLIB::Smoothstep].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Smoothstep].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_edge~0~_");
    BuiltInDesc[OpenCLLIB::Smoothstep].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_edge~1~_");
    BuiltInDesc[OpenCLLIB::Smoothstep].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Smoothstep].addNote(NOTE CONTRACTIONS_NOTE);
    BuiltInDesc[OpenCLLIB::Smoothstep].extInstClass = OclExInstClassCommon;

    BuiltInDesc[OpenCLLIB::Sign].opDesc = "Returns 1.0 if _x_ > 0, -0.0 if _x_ = -0.0, +0.0 if _x_ = +0.0, or -1.0 if _x_ < 0. "
        "Returns 0.0 if _x_ is a NaN.";
    BuiltInDesc[OpenCLLIB::Sign].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Sign].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Sign].extInstClass = OclExInstClassCommon;

    // GEOMETRIC FUNCTIONS
    BuiltInDesc[OpenCLLIB::Cross].opDesc = "Returns the cross product of _p~0~_.xyz and _p~1~_.xyz. " LINE_BREAK
        "If the vector component count is 4, the w component returned is 0.0.";
    BuiltInDesc[OpenCLLIB::Cross].pushResOperand(OperandId, VectorVecType, ThreeComp | FourComp, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Cross].pushOperand(OperandId, VectorVecType, ThreeComp | FourComp, FPDataType, "_p~0~_");
    BuiltInDesc[OpenCLLIB::Cross].pushOperand(OperandId, VectorVecType, ThreeComp | FourComp, FPDataType, "_p~1~_");
    BuiltInDesc[OpenCLLIB::Cross].extInstClass = OclExInstClassGeometrics;

    BuiltInDesc[OpenCLLIB::Distance].opDesc = "Returns the distance between _p~0~_ and _p~1~_.  This is calculated as _length_(_p~0~_ - _p~1~_). ";
    BuiltInDesc[OpenCLLIB::Distance].pushResOperand(OperandId, ScalarVecType, 0, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Distance].pushOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, "_p~0~_");
    BuiltInDesc[OpenCLLIB::Distance].pushOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, "_p~1~_");
    BuiltInDesc[OpenCLLIB::Distance].overrideIdenticalTypesMsg("_p~0~_ and _p~1~_ operands must have the same type. "
        RES_TYPE ", _p~0~_ and _p~1~_ operands must have the same component type");
    BuiltInDesc[OpenCLLIB::Distance].extInstClass = OclExInstClassGeometrics;

    BuiltInDesc[OpenCLLIB::Length].opDesc = "Return the length of vector _p_, i.e. _sqrt_( _p_.x^2^ + _p_.y^2^ + ... )";
    BuiltInDesc[OpenCLLIB::Length].pushResOperand(OperandId, ScalarVecType, 0, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Length].pushOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Length].overrideIdenticalTypesMsg(RES_TYPE " and _p_ operands must have the same component type");
    BuiltInDesc[OpenCLLIB::Length].extInstClass = OclExInstClassGeometrics;

    BuiltInDesc[OpenCLLIB::Normalize].opDesc = "Returns a vector in the same direction as _p_ but with a length of 1.";
    BuiltInDesc[OpenCLLIB::Normalize].pushResOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Normalize].pushOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Normalize].extInstClass = OclExInstClassGeometrics;

    BuiltInDesc[OpenCLLIB::Fast_distance].opDesc = "Returns _fast_length_(_p~0~_ - _p~1~_). ";
    BuiltInDesc[OpenCLLIB::Fast_distance].pushResOperand(OperandId, ScalarVecType, 0, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fast_distance].pushOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, "_p~0~_");
    BuiltInDesc[OpenCLLIB::Fast_distance].pushOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, "_p~1~_");
    BuiltInDesc[OpenCLLIB::Fast_distance].overrideIdenticalTypesMsg("_p~0~_ and _p~1~_ operands must have the same type. "
        RES_TYPE ", _p~0~_ and _p~1~_ operands must have the same component type");
    BuiltInDesc[OpenCLLIB::Fast_distance].extInstClass = OclExInstClassGeometrics;

    BuiltInDesc[OpenCLLIB::Fast_length].opDesc = "Return the length of vector _p_ computed as: _half_sqrt_( _p_.x^2^ + _p_.y^2^ + ... )";
    BuiltInDesc[OpenCLLIB::Fast_length].pushResOperand(OperandId, ScalarVecType, 0, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fast_length].pushOperand(OperandId, VectorVecType, TwoComp | ThreeComp | FourComp, FPDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Fast_length].overrideIdenticalTypesMsg(RES_TYPE " and _p_ operands must have the same component type");
    BuiltInDesc[OpenCLLIB::Fast_length].extInstClass = OclExInstClassGeometrics;

    BuiltInDesc[OpenCLLIB::Fast_normalize].opDesc = "Returns a vector in the same direction as _p_ but with a length of 1 computed as:" LINE_BREAK
        "_p_ * _half_rsqrt_( _p_.x^2^ + _p_.y^2^ ... ) "
        GAP "The result shall be within 8192 ulps error from the infinitely precise result of:" LINE_BREAK
        "if (_all_( _p_ == 0.0f )) {" " result = _p_; }" LINE_BREAK
        "else { " " result = _p_ / _sqrt_(_p_.x^2^ + _p_.y^2^ + ...); }"
        GAP "with the following exceptions :" LINE_BREAK
        "1) If the sum of squares is greater than FLT_MAX then the value of the floating-point values in the result vector are _poison_." LINE_BREAK
        "2) If the sum of squares is less than FLT_MIN then the implementation may return back _p_." LINE_BREAK
        "3) If the device is in \"denorms are flushed to zero\" mode, "
        "individual operand elements with magnitude less than _sqrt_(FLT_MIN) may be flushed to zero before proceeding with the calculation.";
    BuiltInDesc[OpenCLLIB::Fast_normalize].pushResOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Fast_normalize].pushOperand(OperandId, SCALAR_AND_VECTOR, TwoComp | ThreeComp | FourComp, FPDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Fast_normalize].extInstClass = OclExInstClassGeometrics;

    // RELATIONAL FUNCTIONS
    BuiltInDesc[OpenCLLIB::Bitselect].opDesc = "Each bit of the result is the corresponding bit of _a_ if the corresponding bit of _c_ is 0.  "
        "Otherwise it is the corresponding bit of _b_.";
    BuiltInDesc[OpenCLLIB::Bitselect].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType | FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Bitselect].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType | FPDataType, "_a_");
    BuiltInDesc[OpenCLLIB::Bitselect].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType | FPDataType, "_b_");
    BuiltInDesc[OpenCLLIB::Bitselect].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType | FPDataType, "_c_");
    BuiltInDesc[OpenCLLIB::Bitselect].extInstClass = OclExInstClassRelationals;

    BuiltInDesc[OpenCLLIB::Select].opDesc = "For each component of a vector type, the result is _a_ if the most significant bit of _c_ is zero, otherwise it is _b_." GAP
        "For a scalar type, the result is _a_ if _c_ is zero, otherwise it is _b_.";
    BuiltInDesc[OpenCLLIB::Select].pushResOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType | FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Select].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType | FPDataType, "_a_");
    BuiltInDesc[OpenCLLIB::Select].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType | FPDataType, "_b_");
    BuiltInDesc[OpenCLLIB::Select].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, IntDataType, "_c_");
    BuiltInDesc[OpenCLLIB::Select].overrideIdenticalTypesMsg(RES_TYPE ", _a_ and _b_ must have the same type. "
        "_c_ operand must have the same component count and component bit width as the rest of the operands.");
    BuiltInDesc[OpenCLLIB::Select].extInstClass = OclExInstClassRelationals;

    // VECTOR STORE LOAD FUNCTIONS

    BuiltInDesc[OpenCLLIB::Vloadn].opDesc = "Reads _n_ components from the address computed as (_p_ + (_offset_ * _n_)) "
        "and creates a vector result value from the _n_ components. " LINE_BREAK
        "Behavior is undefined if the computed address is not 8-bit aligned when _p_ points to an i8 value; "
        "16-bit aligned when _p_ points to an i16 or half value; "
        "32-bit aligned when _p_ points to an i32 or float value; "
        "64-bit aligned when _p_ points to an i64 or double value.";
    BuiltInDesc[OpenCLLIB::Vloadn].pushResOperand(OperandId, VectorVecType, ALL_VEC_SIZE, IntDataType | FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vloadn].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vloadn].pushOperand(OperandId, ScalarVecType, 0, IntDataType | FPDataType | PtrConstantDataType |
                                                                            PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType |
                                                                            PtrGenericDataType, "_p_");

    BuiltInDesc[OpenCLLIB::Vloadn].pushLiteralNumberOperand("_n_");
    BuiltInDesc[OpenCLLIB::Vloadn].overrideIdenticalTypesMsg(RES_TYPE " component count must be equal to _n_ and its component type must be equal to the type pointed by _p_." LINE_BREAK
        "_n_ must be 2, 3, 4, 8 or 16.");
    BuiltInDesc[OpenCLLIB::Vloadn].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vstoren].opDesc = "Writes _n_ components from the _data_ vector value to the address computed as (_p_ + (_offset_ * _n_)), "
        "where _n_ is equal to the component count of the vector _data_." LINE_BREAK
        "Behavior is undefined if the computed address is not 8-bit aligned when _p_ points to an i8 value; "
        "16-bit aligned when _p_ points to an i16 or half value; "
        "32-bit aligned when _p_ points to an i32 or float value; "
        "64-bit aligned when _p_ points to an i64 or double value.";
    BuiltInDesc[OpenCLLIB::Vstoren].pushResOperand(OperandId, ScalarVecType, 0, VoidDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vstoren].pushOperand(OperandId, VectorVecType, ALL_VEC_SIZE, IntDataType | FPDataType, "_data_");
    BuiltInDesc[OpenCLLIB::Vstoren].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vstoren].pushOperand(OperandId, ScalarVecType, 0, IntDataType | FPDataType | PtrGlobalDataType | PtrLocalDataType |
                                                                             PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vstoren].overrideIdenticalTypesMsg("_data_ component type must be equal to the type pointed by _p_.");
    BuiltInDesc[OpenCLLIB::Vstoren].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vload_half].opDesc = "Reads a half value from the address computed as (_p_ + (_offset_)) and converts it to a float result value. " LINE_BREAK
        "Behavior is undefined if the computed address is not 16-bit aligned.";
    BuiltInDesc[OpenCLLIB::Vload_half].pushResOperand(OperandId, ScalarVecType, 0, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vload_half].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vload_half].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrConstantDataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vload_half].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Vload_half].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vload_halfn].opDesc = "Reads _n_ half components from the address (_p_ + (_offset_ * _n_)), "
        "converts to _n_ float components, and creates a float vector result value from the _n_ float components." LINE_BREAK
        "Behavior is undefined if the computed address is not 16-bit aligned.";
    BuiltInDesc[OpenCLLIB::Vload_halfn].pushResOperand(OperandId, VectorVecType, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vload_halfn].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vload_halfn].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrConstantDataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vload_halfn].pushLiteralNumberOperand("_n_");
    BuiltInDesc[OpenCLLIB::Vload_halfn].overrideIdenticalTypesMsg(RES_TYPE " component count must be equal to _n_." LINE_BREAK
        "_n_ must be 2, 3, 4, 8 or 16.");
    BuiltInDesc[OpenCLLIB::Vload_halfn].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vstore_half].opDesc = "Converts the _data_ float or double value to a half value using the default rounding mode "
        "and writes the half value to the address computed as (_p_ + _offset_). " LINE_BREAK
        "Behavior is undefined if the computed address is not 16-bit aligned.";
    BuiltInDesc[OpenCLLIB::Vstore_half].pushResOperand(OperandId, ScalarVecType, 0, VoidDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vstore_half].pushOperand(OperandId, ScalarVecType, 0, Float32DataType | Float64DataType, "_data_");
    BuiltInDesc[OpenCLLIB::Vstore_half].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vstore_half].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vstore_half].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Vstore_half].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vstore_halfn].opDesc = "Converts the _data_ vector of float or vector of double values "
        "to a vector of half values using the default rounding mode and writes the half values to memory." LINE_BREAK
        "Let _n_ be the component count of the vector _data_." LINE_BREAK
        "The _n_ components from the converted vector of half values are written to the address computed as (_p_ + (_offset_ * _n_))." LINE_BREAK
        "Behavior is undefined if the computed address is not 16-bit aligned.";
    BuiltInDesc[OpenCLLIB::Vstore_halfn].pushResOperand(OperandId, ScalarVecType, 0, VoidDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vstore_halfn].pushOperand(OperandId, VectorVecType, ALL_VEC_SIZE, Float32DataType | Float64DataType, "_data_");
    BuiltInDesc[OpenCLLIB::Vstore_halfn].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vstore_halfn].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vstore_halfn].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Vstore_halfn].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vstore_half_r].opDesc = "Converts the _data_ float or double value to a half value using the specified rounding mode _mode_ "
        "and writes the half value to the address computed as (_p_ + _offset_). " LINE_BREAK
        "Behavior is undefined if the computed address is not 16-bit aligned.";
    BuiltInDesc[OpenCLLIB::Vstore_half_r].pushResOperand(OperandId, ScalarVecType, 0, VoidDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vstore_half_r].pushOperand(OperandId, ScalarVecType, 0, Float32DataType | Float64DataType, "_data_");
    BuiltInDesc[OpenCLLIB::Vstore_half_r].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vstore_half_r].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vstore_half_r].pushRoundingModeOperand("_mode_");
    BuiltInDesc[OpenCLLIB::Vstore_half_r].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Vstore_half_r].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vstore_halfn_r].opDesc = "Converts the _data_ vector of float or vector of double values "
        "to a vector of half values using the specified rounding mode _mode_ and writes the half values to memory." LINE_BREAK
        "Let _n_ be the component count of the vector _data_." LINE_BREAK
        "The _n_ components from the converted vector of half values are written to the address computed as (_p_ + (_offset_ * _n_))." LINE_BREAK
        "Behavior is undefined if the computed address is not 16-bit aligned.";
    BuiltInDesc[OpenCLLIB::Vstore_halfn_r].pushResOperand(OperandId, ScalarVecType, 0, VoidDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vstore_halfn_r].pushOperand(OperandId, VectorVecType, ALL_VEC_SIZE, Float32DataType | Float64DataType, "_data_");
    BuiltInDesc[OpenCLLIB::Vstore_halfn_r].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vstore_halfn_r].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vstore_halfn_r].pushRoundingModeOperand("_mode_");
    BuiltInDesc[OpenCLLIB::Vstore_halfn_r].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Vstore_halfn_r].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vloada_halfn].opDesc = "Reads a vector of _n_ half values from aligned memory and converts it to a float vector result value." LINE_BREAK
        "For _n_ equal to 2, 4, 8, and 16, the vector of _n_ half values is read from the address computed"
        " as (_p_ + (_offset_ * _n_))."
        " Behavior is undefined if the computed address is not aligned to (_sizeof(half)_ * _n_) bytes." LINE_BREAK
        "For _n_ equal to 3, the vector of _n_ half values are read from the address computed "
        "as (_p_ + (_offset_ * 4))."
        " Behavior is undefined if the computed address is not aligned to (_sizeof(half)_ * 4) bytes.";
    BuiltInDesc[OpenCLLIB::Vloada_halfn].pushResOperand(OperandId, VectorVecType, ALL_VEC_SIZE, Float32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vloada_halfn].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vloada_halfn].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrConstantDataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vloada_halfn].pushLiteralNumberOperand("_n_");
    BuiltInDesc[OpenCLLIB::Vloada_halfn].overrideIdenticalTypesMsg(RES_TYPE " component count must be equal to _n_." LINE_BREAK
        "_n_ must be 2, 3, 4, 8 or 16.");
    BuiltInDesc[OpenCLLIB::Vloada_halfn].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vstorea_halfn].opDesc = "Converts the _data_ vector of float or vector of double values "
        "to a vector of half values using the default rounding mode, and then writes the converted vector of half values to aligned memory." LINE_BREAK
        "Let _n_ be the component count of the vector _data_." LINE_BREAK
        "For _n_ equal to 2, 4, 8, and 16, the converted vector of half values is written to the address computed "
        "as (_p_ + (_offset_ * _n_))."
        " Behavior is undefined if the computed address is not aligned to (_sizeof(half)_ * _n_) bytes." LINE_BREAK
        "For _n_ equal to 3, the converted vector of half values is written to the address computed "
        "as (_p_ + (_offset_ * 4))."
        " Behavior is undefined if the computed address is not aligned to (_sizeof(half)_ * 4) bytes.";
    BuiltInDesc[OpenCLLIB::Vstorea_halfn].pushResOperand(OperandId, ScalarVecType, 0, VoidDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vstorea_halfn].pushOperand(OperandId, VectorVecType, ALL_VEC_SIZE, Float32DataType | Float64DataType, "_data_");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn].extInstClass = OclExInstClassVectorLoadStore;

    BuiltInDesc[OpenCLLIB::Vstorea_halfn_r].opDesc = "Converts the _data_ vector of float or vector of double values "
        "to a vector of half values using the specified rounding mode _mode_, and then write the converted vector of half values to aligned memory." LINE_BREAK
        "Let _n_ be the component count of the vector _data_." LINE_BREAK
        "For _n_ equal to 2, 4, 8, and 16, the converted vector of half values is written to the address computed "
        "as (_p_ + (_offset_ * _n_))."
        " Behavior is undefined if the computed address is not aligned to (_sizeof(half)_ * _n_) bytes." LINE_BREAK
        "For _n_ equal to 3, the converted vector of half values is written to the address computed "
        "as (_p_ + (_offset_ * 4))."
        " Behavior is undefined if the computed address is not aligned to (_sizeof(half)_ * 4) bytes.";
    BuiltInDesc[OpenCLLIB::Vstorea_halfn_r].pushResOperand(OperandId, ScalarVecType, 0, VoidDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Vstorea_halfn_r].pushOperand(OperandId, VectorVecType, ALL_VEC_SIZE, Float32DataType | Float64DataType, "_data_");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn_r].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_offset_");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn_r].pushOperand(OperandId, ScalarVecType, 0, Float16DataType | PtrGlobalDataType | PtrLocalDataType | PtrPrivateDataType | PtrGenericDataType, "_p_");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn_r].pushRoundingModeOperand("_mode_");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn_r].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Vstorea_halfn_r].extInstClass = OclExInstClassVectorLoadStore;

    // VECTOR MISC FUNCTIONS
    BuiltInDesc[OpenCLLIB::Shuffle].opDesc = "Construct a permutation of components from _x_ vector value, "
        "returning a vector value with the same component type as _x_ and component count that is the same as _shuffle mask_." LINE_BREAK
        "For this instruction, only the _ilogb_(2 _m_ -1) least significant bits of each mask element are considered, where _m_ is equal to the component count of _x_." LINE_BREAK
        "_shuffle mask_ operand specifies, for each component in the result vector, which component of _x_ it gets." LINE_BREAK
        "The size of each component in _shuffle mask_ must match the size of each component in " RES_TYPE "." LINE_BREAK
        RES_TYPE " must have the same component type as _x_ and component count as _shuffle mask_.";
    BuiltInDesc[OpenCLLIB::Shuffle].pushResOperand(OperandId, VectorVecType, TwoComp | FourComp | EightComp | SixteenComp, IntDataType | FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Shuffle].pushOperand(OperandId, VectorVecType, TwoComp | FourComp | EightComp | SixteenComp, IntDataType | FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Shuffle].pushOperand(OperandId, VectorVecType, TwoComp | FourComp | EightComp | SixteenComp, IntDataType, "_shuffle mask_");
    BuiltInDesc[OpenCLLIB::Shuffle].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Shuffle].extInstClass = OclExInstClassVectorMisc;

    BuiltInDesc[OpenCLLIB::Shuffle2].opDesc = "Construct a permutation of components from _x_ and _y_ vector values, "
        "returning a vector value with the same component type as _x_ and _y_ and component count that is the same as _shuffle mask_." LINE_BREAK
        "For this instruction, only the _ilogb_(2 _m_ - 1) + 1 least significant bits of each mask component are considered, where _m_ is equal to the component count of _x_ and _y_." LINE_BREAK
        "_shuffle mask_ operand specifies, for each component in the result vector, which component of _x_ or _y_ it gets. Where component count begins with _x_ and then proceeds to _y_." LINE_BREAK
        "_x_ and _y_ must be of the same type." LINE_BREAK
        "The size of each component in _shuffle mask_ must match the size of each component in " RES_TYPE "." LINE_BREAK
        RES_TYPE " must have the same component type as _x_ and component count as _shuffle mask_.";
    BuiltInDesc[OpenCLLIB::Shuffle2].pushResOperand(OperandId, VectorVecType, TwoComp | FourComp | EightComp | SixteenComp, IntDataType | FPDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Shuffle2].pushOperand(OperandId, VectorVecType, TwoComp | FourComp | EightComp | SixteenComp, IntDataType | FPDataType, "_x_");
    BuiltInDesc[OpenCLLIB::Shuffle2].pushOperand(OperandId, VectorVecType, TwoComp | FourComp | EightComp | SixteenComp, IntDataType | FPDataType, "_y_");
    BuiltInDesc[OpenCLLIB::Shuffle2].pushOperand(OperandId, VectorVecType, TwoComp | FourComp | EightComp | SixteenComp, IntDataType, "_shuffle mask_");
    BuiltInDesc[OpenCLLIB::Shuffle2].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Shuffle2].extInstClass = OclExInstClassVectorMisc;

    // PRINTF
    BuiltInDesc[OpenCLLIB::Printf].opDesc = "The _printf_ extended instruction writes output to an implementation-defined stream such as stdout under control "
        "of the string pointed to by format that specifies how subsequent arguments are converted for output.  "
        "If there are insufficient arguments for the format, the behavior is undefined. "
        "If the format is exhausted while arguments remain, the excess arguments are evaluated (as always) but are otherwise ignored.  "
        "The printf instruction returns when the end of the format string is encountered" LINE_BREAK
        "_printf_ returns 0 if it was executed successfully and -1 otherwise";
    BuiltInDesc[OpenCLLIB::Printf].pushResOperand(OperandId, ScalarVecType, 0, Int32DataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Printf].pushOperand(OperandId, ScalarVecType, 0, Int8DataType | PtrConstantDataType, "_format_");
    BuiltInDesc[OpenCLLIB::Printf].pushVariableIds("_additional arguments_");
    BuiltInDesc[OpenCLLIB::Printf].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Printf].extInstClass = OclExInstClassMisc;

    BuiltInDesc[OpenCLLIB::Prefetch].opDesc = "Prefetch _num_elements_ * size in bytes of the type pointed by _p_,"
        " into the global cache."
        " The prefetch instruction is applied to an invocation in a workgroup and does not affect the"
        " functionality of the kernel.";
    BuiltInDesc[OpenCLLIB::Prefetch].pushResOperand(OperandId, ScalarVecType, 0, VoidDataType, RES_TYPE);
    BuiltInDesc[OpenCLLIB::Prefetch].pushOperand(OperandId, SCALAR_AND_VECTOR, ALL_VEC_SIZE, FPDataType | IntDataType | PtrGlobalDataType, "_ptr_");
    BuiltInDesc[OpenCLLIB::Prefetch].pushOperand(OperandId, ScalarVecType, 0, SizeTDataType, "_num_elements_");
    BuiltInDesc[OpenCLLIB::Prefetch].overrideIdenticalTypesMsg("");
    BuiltInDesc[OpenCLLIB::Prefetch].extInstClass = OclExInstClassMisc;


}

void PrintOclExtInstClass(OclExtInstClass c) {
    for (unsigned int i = 0; i < OclExtInstCeiling; i++) {
        if (BuiltInDesc[i].extInstClass == c) {
            if (OclInstrPageBreaks.find((OpenCLLIB::Entrypoints)(i)) != OclInstrPageBreaks.end()) {
                //printf("adding extra break\n");
                printf("<<<\n");
            }
            PrintSingleOpcode(&BuiltInDesc[i], i);
        }
    }

}

// prints the list of math functions
void PrintMathFunctions(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[Math]]\n=== Math extended instructions\n");
    printf("This section describes the list of external math instructions."
        " The external math instructions are categorized into the following:" LINE_BREAK
        "- A list of instructions that have scalar or vector argument versions, and," LINE_BREAK
        "- A list of instructions that only take scalar float arguments." LINE_BREAK
        "The vector versions of the math instructions operate component-wise.  The description is per-component." LINE_BREAK
        "The math instructions are not affected by the prevailing rounding mode in the calling environment, "
        "and always return the same value as they would if called with the round to nearest even rounding mode." LINE_BREAK
        "For environments that allow use of *FPFastMathMode* decorations on *OpExtInst* instructions, "
        "*FPFastMathMode* decorations may be applied to the math instructions." LINE_BREAK
        );
    PrintOclExtInstClass(OclExInstClassMath);
}

void PrintIntegerFunctions(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[Integer]]\n=== Integer instructions\n");
    printf("This section describes the list of integer instructions that take scalar or vector arguments. "
        "The vector versions of the integer instructions operate component-wise.  The description is per-component." LINE_BREAK);
    PrintOclExtInstClass(OclExInstClassIntegers);
}

void PrintCommonFunctions(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[Common]]\n=== Common instructions\n");
    printf("This section describes the list of common instructions that take scalar or vector arguments. "
        "The vector versions of the integer instructions operate component-wise.  The description is per-component. "
        "The common instructions are implemented using the round to nearest even rounding mode." LINE_BREAK
        "For environments that allow use of *FPFastMathMode* decorations on *OpExtInst* instructions, "
        "*FPFastMathMode* decorations may be applied to the common instructions." LINE_BREAK
        );
    PrintOclExtInstClass(OclExInstClassCommon);
}

void PrintGeometricFunctions(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[Geometric]]\n=== Geometric instructions\n");
    printf("This section describes the list of geometric instructions. "
        "In this section _x_,_y_,_z_ and _w_ denote the first, second, third and fourth component respectively, of vectors with 3 and four components. "
        "The geometric instructions are implemented using the round to nearest even rounding mode." LINE_BREAK
        NOTE "The geometric instructions can be implemented using contractions such as mad or fma" LINE_BREAK
        "For environments that allow use of *FPFastMathMode* decorations on *OpExtInst* instructions, "
        "*FPFastMathMode* decorations may be applied to the geometric instructions." LINE_BREAK
        );
    PrintOclExtInstClass(OclExInstClassGeometrics);
}

void PrintRelationalFunctions(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[Relational]]\n=== Relational instructions\n");
    printf("This section describes the list of relational instructions that take scalar or vector arguments. "
        "The vector versions of the integer instructions operate component-wise.  The description is per-component. " LINE_BREAK);
    PrintOclExtInstClass(OclExInstClassRelationals);
}

void PrintVectorLoadStoreFunctions(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[Vector]]\n=== Vector Data Load and Store instructions\n");
    printf("This section describes the list of instructions that allow reading and writing of vector types from a pointer to memory. " LINE_BREAK
        "For environments that allow use of *FPFastMathMode* decorations on *OpExtInst* instructions, "
        "*FPFastMathMode* decorations may be applied to vector data load and store instructions that convert to or from _half_ values." LINE_BREAK
        );
    PrintOclExtInstClass(OclExInstClassVectorLoadStore);
}

void PrintMiscVectorFunctions(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[MiscVec]]\n=== Miscellaneous Vector instructions\n");
    printf("This section describes additional vector instructions. " LINE_BREAK);
    PrintOclExtInstClass(OclExInstClassVectorMisc);
}

void PrintPrintf(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[Misc]]\n=== Misc instructions\n");
    printf("This section describes additional miscellaneous instructions. " LINE_BREAK);
    PrintOclExtInstClass(OclExInstClassMisc);
}

#if 0
void PrintImageOpcode(ImageDataType Ty, const char* desc)
{
    int wordCount = 9;
    Dim dim = Dim2D;
    int isArray = 0;
    int isDepth = 0;
    int isMultiSample = 0;
    int isSampledImage = 0;
    bool needAccessQualifier = true;
    switch (Ty)
    {
    case spv::Image1DType:
        dim = Dim1D;
        isArray = 0;
        isDepth = 0;
        isMultiSample = 0;
        break;
    case spv::Image1DBufferType:
        dim = DimBuffer;
        isArray = 0;
        isDepth = 0;
        isMultiSample = 0;
        break;
    case spv::Image1DArrayType:
        dim = Dim1D;
        isArray = 1;
        isDepth = 0;
        isMultiSample = 0;
        break;
    case spv::Image2DType:
        dim = Dim2D;
        isArray = 0;
        isDepth = 0;
        isMultiSample = 0;
        break;
    case spv::Image2DArrayType:
        dim = Dim2D;
        isArray = 1;
        isDepth = 0;
        isMultiSample = 0;
        break;
    case spv::Image2DArrayDepthType:
        dim = Dim2D;
        isArray = 1;
        isDepth = 1;
        isMultiSample = 0;
        break;
    case spv::Image2DDepthType:
        dim = Dim2D;
        isArray = 0;
        isDepth = 1;
        isMultiSample = 0;
        break;
    case spv::Image2DMsaaType:
        dim = Dim2D;
        isArray = 0;
        isDepth = 0;
        isMultiSample = 1;
        break;
    case spv::Image2DArrayMsaaType:
        dim = Dim2D;
        isArray = 1;
        isDepth = 0;
        isMultiSample = 1;
        break;
    case spv::Image2DMsaaDepthType:
        dim = Dim2D;
        isArray = 0;
        isDepth = 1;
        isMultiSample = 1;
        break;
    case spv::Image2DArrayMsaaDepthType:
        dim = Dim2D;
        isArray = 1;
        isDepth = 1;
        isMultiSample = 1;
        break;
    case spv::Image3DType:
        isArray = 0;
        dim = Dim3D;
        isDepth = 0;
        isMultiSample = 0;
        break;
    case spv::SamplerType:
        isArray = 0;
        dim = Dim1D;
        isDepth = 0;
        isMultiSample = 0;
        isSampledImage = 1;
        needAccessQualifier = false;
        break;
    }

    // Table start
    if (!needAccessQualifier) wordCount--;
    int width = std::max((wordCount + 1) * 12, (int)strlen(desc) / 3);
    width = std::max(width, 40);
    printf("[cols=\"2*1,%d*3\",width=\"%d%%\"]\n", wordCount - 1, std::min(100, width));
    printf("|=====\n");

    // Name
    printf("%d+|[[%s]]*%s*", wordCount + 1, GetImageDataTypeDesc(Ty,false).c_str(), GetImageDataTypeDesc(Ty, false).c_str());

    // Semantics
    printf(" +\n +\n%s\n" LINE_BREAK, desc);

    // Word Count
    printf("| %d ", wordCount);

    // Opcode
    printf("| %d ", OpTypeImage);

    // result id
    printf(" | %s", "_Result <id>_");

    // sampled type
    printf(" | _Sampled Type_ OpTypeVoid" );

    // dimensionality
    printf(" | _%s_ +\n%s", GetOperandDesc(OperandDimensionality), OperandClassParams[OperandDimensionality].getName(dim));

    // depth
    printf("| %s +\n%d", "_Depth_", isDepth);

    // array
    printf("| %s +\n%d", "_Arrayed_", isArray);

    // ms
    printf("| %s +\n%d", "_MS_", isMultiSample);

    // sampled
    printf("| %s +\n%d", "_Sampled_", isSampledImage);

    // image format
    printf("| %s +\n%s", "_Image Format_", OperandClassParams[OperandSamplerImageFormat].getName(ImageFormat::ImageFormatUnknown));

    // access qualifier
    if (needAccessQualifier) {
        printf("| %s +\n%s ", GetOperandDesc(OperandAccessQualifier), "_qualifier_");
    }

    // Table end
    printf("\n|=====\n");
}
#endif

#if 0
static void PrintSamplerTypeEncoding(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[SamplerEnc]]\n=== Sampler encoding\n");
    printf("A SPIR-V _sampler_ object is encoded via the *OpTypeSampler* instruction via a kernel function argument:" LINE_BREAK);
    printf("In addition, it is possible to define a constant (or inline) _sampler_ using the *OpConstantSampler* instruction." LINE_BREAK);
}
#endif

#if 0
static void PrintImageTypeEncoding(SPIRVersion) {
    printf("<<<\n");
    printf("\n[[ImageEnc]]\n=== Image encoding\n");
    printf("The following list denotes the different valid *OpTypeImage* encodings of image objects." LINE_BREAK);
    PrintImageOpcode(Image1DType, "A 1D image");
    PrintImageOpcode(Image1DBufferType, "A 1D image created from a buffer object.");
    PrintImageOpcode(Image1DArrayType, "A 1D image array.");
    PrintImageOpcode(Image2DType, "A 2D image.");
    PrintImageOpcode(Image2DArrayType, "A 2D image array.");
    printf("<<<\n");
    PrintImageOpcode(Image2DDepthType, "A 2D depth image.");
    PrintImageOpcode(Image2DArrayDepthType, "A 2D depth image array.");
    PrintImageOpcode(Image2DMsaaType, "A 2D multi-sample color image.");
    PrintImageOpcode(Image2DArrayMsaaType, "A 2D multi-sample color image array.");
    PrintImageOpcode(Image2DMsaaDepthType, "A 2D multi-sample depth image.");
    PrintImageOpcode(Image2DArrayMsaaDepthType, "A 2D multi-sample depth image array.");
    PrintImageOpcode(Image3DType, "A 3D image object.");
}
#endif

#if 0
static void PrintEnumHeader(OCLEnumOperands operand)
{
    printf("[cols=\"^1,1*9\",options=\"header\",width = \"60%%\"]\n");

    printf("|====\n");
    printf("2+^.^| [[%s]]%s", GetEnumOperandDesc(operand).c_str(), GetEnumOperandDesc(operand).c_str());
    printf("\n");
}

static void PrintImageFormatEncoding(SPIRVersion) {
    printf("\n[[ImageFormatEnc]]\n==== Image format encoding\n");
    printf("Every image memory object has a format. An image format is a combination of _channel order_ and _channel data type_. "
        "The _channel order_ specifies the number of channels and the channel layout i.e.the memory layout in which channels are stored in the image. "
        " The _channel data type_ describes the size of the channel data type." LINE_BREAK);

    PrintEnumHeader(ImageChannelOrderEnumOperand);
    PrintImmediateRow(R_ChannelOrder, ImageChannelOrderString(R_ChannelOrder));
    PrintImmediateRow(A_ChannelOrder, ImageChannelOrderString(A_ChannelOrder));
    PrintImmediateRow(RG_ChannelOrder, ImageChannelOrderString(RG_ChannelOrder));
    PrintImmediateRow(RA_ChannelOrder, ImageChannelOrderString(RA_ChannelOrder));
    PrintImmediateRow(RGB_ChannelOrder, ImageChannelOrderString(RGB_ChannelOrder));
    PrintImmediateRow(RGBA_ChannelOrder, ImageChannelOrderString(RGBA_ChannelOrder));
    PrintImmediateRow(BGRA_ChannelOrder, ImageChannelOrderString(BGRA_ChannelOrder));
    PrintImmediateRow(ARGB_ChannelOrder, ImageChannelOrderString(ARGB_ChannelOrder));
    PrintImmediateRow(INTENSITY_ChannelOrder, ImageChannelOrderString(INTENSITY_ChannelOrder));
    PrintImmediateRow(LUMINANCE_ChannelOrder, ImageChannelOrderString(LUMINANCE_ChannelOrder));
    PrintImmediateRow(Rx_ChannelOrder, ImageChannelOrderString(Rx_ChannelOrder));
    PrintImmediateRow(RGx_ChannelOrder, ImageChannelOrderString(RGx_ChannelOrder));
    PrintImmediateRow(RGBx_ChannelOrder, ImageChannelOrderString(RGBx_ChannelOrder));
    PrintImmediateRow(DEPTH_ChannelOrder, ImageChannelOrderString(DEPTH_ChannelOrder));
    PrintImmediateRow(DEPTH_STENCIL_ChannelOrder, ImageChannelOrderString(DEPTH_STENCIL_ChannelOrder));
    PrintImmediateRow(sRGB_ChannelOrder, ImageChannelOrderString(sRGB_ChannelOrder));
    PrintImmediateRow(sRGBx_ChannelOrder, ImageChannelOrderString(sRGBx_ChannelOrder));
    PrintImmediateRow(sRGBA_ChannelOrder, ImageChannelOrderString(sRGBA_ChannelOrder));
    PrintImmediateRow(sBGRA_ChannelOrder, ImageChannelOrderString(sBGRA_ChannelOrder));
    printf("|====\n");

    PrintEnumHeader(ImageChannelTypeEnumOperand);
    PrintImmediateRow(SNORM_INT8_ChannelType, ImageChannelTypeString(SNORM_INT8_ChannelType));
    PrintImmediateRow(SNORM_INT16_ChannelType, ImageChannelTypeString(SNORM_INT16_ChannelType));
    PrintImmediateRow(UNORM_INT8_ChannelType, ImageChannelTypeString(UNORM_INT8_ChannelType));
    PrintImmediateRow(UNORM_INT16_ChannelType, ImageChannelTypeString(UNORM_INT16_ChannelType));
    PrintImmediateRow(UNORM_SHORT_565_ChannelType, ImageChannelTypeString(UNORM_SHORT_565_ChannelType));
    PrintImmediateRow(UNORM_SHORT_555_ChannelType, ImageChannelTypeString(UNORM_SHORT_555_ChannelType));
    PrintImmediateRow(UNORM_INT_101010_ChannelType, ImageChannelTypeString(UNORM_INT_101010_ChannelType));
    PrintImmediateRow(SIGNED_INT8_ChannelType, ImageChannelTypeString(SIGNED_INT8_ChannelType));
    PrintImmediateRow(SIGNED_INT16_ChannelType, ImageChannelTypeString(SIGNED_INT16_ChannelType));
    PrintImmediateRow(SIGNED_INT32_ChannelType, ImageChannelTypeString(SIGNED_INT32_ChannelType));
    PrintImmediateRow(UNSIGNED_INT8_ChannelType, ImageChannelTypeString(UNSIGNED_INT8_ChannelType));
    PrintImmediateRow(UNSIGNED_INT16_ChannelType, ImageChannelTypeString(UNSIGNED_INT16_ChannelType));
    PrintImmediateRow(UNSIGNED_INT32_ChannelType, ImageChannelTypeString(UNSIGNED_INT32_ChannelType));
    PrintImmediateRow(HALF_FLOAT_ChannelType, ImageChannelTypeString(HALF_FLOAT_ChannelType));
    PrintImmediateRow(FLOAT_ChannelType, ImageChannelTypeString(FLOAT_ChannelType));
    PrintImmediateRow(UNORM_INT24_ChannelType, ImageChannelTypeString(UNORM_INT24_ChannelType));
    printf("|====\n");
}
#endif

#if 0
void PrintImageFunctions(SPIRVersion ver) {
    PrintImageTypeEncoding(ver);
    PrintSamplerTypeEncoding(ver);
    PrintImageFormatEncoding(ver);
}
#endif

void PrintOclDoc(SPIRVersion ver) {

    OclSetExtraPageBreaks();

    switch (ver)
    {
    case spv::SPIROpenCLCommonVersion:
        PRINT_INTRO("OpenCL", ver, "OpenCL.std", "OpenCL")
    default:
        break;
    }
    ParameterizeBuiltins(ver);
    ParameterizeEnums(ver);
    PrintMathFunctions(ver);
    PrintIntegerFunctions(ver);
    PrintCommonFunctions(ver);
    PrintGeometricFunctions(ver);
    PrintRelationalFunctions(ver);
    PrintVectorLoadStoreFunctions(ver);
    PrintMiscVectorFunctions(ver);
    PrintPrintf(ver);
    //PrintImageFunctions(ver);
}
// Main function for printing out the documentation.
void PrintOclCommonDoc()
{
    version = SPIROpenCLCommonVersion;
    PrintOclDoc(SPIROpenCLCommonVersion);
}

};  // end namespace spv
