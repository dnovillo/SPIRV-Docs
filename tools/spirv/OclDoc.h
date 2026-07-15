// Copyright 2014-2026 The Khronos Group Inc.
// SPDX-License-Identifier: MIT

//
// Author: Raun Krisch, Intel (October 2015 - current)
// Author: Boaz Ouriel, Intel (origin - October 2015)
//

//
//
// Print OpenCL extended instruction sets documentation.
//

#include <set>
#include <string>
#include <map>
#include "unified1/spirv.hpp"

namespace spv {

void OclGetNames(const char** names);

enum SPIRVersion {
    SPIROpenCLCommonVersion,
    SPIRVersionCount
};

enum GenericVecNumComp {
    TwoComp     = 0x1,
    ThreeComp   = 0x2,
    FourComp    = 0x4,
    EightComp   = 0x8,
    SixteenComp = 0x10,
    NumVecCompCount = 5
};

enum GenericVecType {
    ScalarVecType = 0x1,  // scalar type
    VectorVecType = 0x2,  // vector type
    NumVecType = 2
};

enum GenericDataType{
    Float16DataType      = 0x1,
    Float32DataType      = 0x2,
    Float64DataType      = 0x4,
    BoolDataType         = 0x8,
    Int8DataType         = 0x10,
    Int16DataType        = 0x20,
    Int32DataType        = 0x40,
    Int64DataType        = 0x80,
    SizeTDataType        = 0x100,
    FPDataType           = 0x200,
    IntDataType          = 0x400,
    VoidDataType         = 0x800,
    PtrGlobalDataType    = 0x1000,
    PtrLocalDataType     = 0x2000,
    PtrPrivateDataType   = 0x4000,
    PtrConstantDataType  = 0x8000,
    PtrGenericDataType   = 0x10000,

    // helper values
    BasicDataTypeStart   = 0,
    BasicDataTypeCount   = 12,
    PointerDataTypeStart = 12,
    PointerDataTypeCount = 5,
    NumDataType = 17,
};

enum ImageDataType{
    Image1DType               = 0x1,
    Image1DBufferType         = 0x2,
    Image1DArrayType          = 0x4,
    Image2DType               = 0x8,
    Image2DArrayType          = 0x10,
    Image2DArrayDepthType     = 0x20,
    Image2DDepthType          = 0x40,
    Image2DMsaaType           = 0x80,
    Image2DArrayMsaaType      = 0x100,
    Image2DMsaaDepthType      = 0x200,
    Image2DArrayMsaaDepthType = 0x400,
    Image3DType               = 0x800,
    ImageType                 = 0x1000,
    SamplerType               = 0x2000,
    // helper values
    ImageDataTypeCount = 14,
};

enum OCLEnumOperands {
    ImageChannelOrderEnumOperand,
    ImageChannelTypeEnumOperand,
};


// For grouping extended instruction opcodes into subsections
enum OclExtInstClass {
    OclExInstClassMisc,            // default, until opcode is classified
    OclExInstClassMath,
    OclExInstClassCommon,
    OclExInstClassGeometrics,
    OclExInstClassImageRead,
    OclExInstClassImageWrite,
    OclExInstClassImageQuery,
    OclExInstClassIntegers,
    OclExInstClassVectorLoadStore,
    OclExInstClassVectorMisc,
    OclExInstClassRelationals,

    OclExInstClassCount
};

const int OclExtInstCeiling = 205;

struct OperandDataTypeDesc {
    bool operator <(const OperandDataTypeDesc& other) const {
        // treat a an int3 (vecType, numVecComponents, dataType)
        if (vecType != other.vecType) {
            if (vecType < other.vecType) return true;
            else return false;
        }

        if (numVecComponents != other.numVecComponents) {
            if (numVecComponents < other.numVecComponents) return true;
            else return false;
        }

        if (dataType < other.dataType) return true;

        return false;
    }

    int vecType;
    int numVecComponents;
    int dataType;
};


class BuiltInOperandParameters {
public:
    BuiltInOperandParameters()  {}

    void pushImage(OperandClass oc, int Ty, AccessQualifier qual, const char* d)
    {
        opClass.push_back(oc);
        desc.push_back(d);
        genericDataType.push_back(0);
        genericVecType.push_back(0);
        genericNumVecComp.push_back(0);
        isImage.push_back(true);
        accessQualifier.push_back(qual);
        Type.push_back(Ty);
    }

    void push(OperandClass oc, int vecType, int numVecComp, int dataType, const char* d)
    {
        opClass.push_back(oc);
        desc.push_back(d);
        genericDataType.push_back(dataType);
        genericVecType.push_back(vecType);
        genericNumVecComp.push_back(numVecComp);
        isImage.push_back(false);
    }
    OperandClass getClass(int op) const { return opClass[op]; }
    const char* getDesc(int op) const { return desc[op]; }
    int getNumVecComp(int op) const { return genericNumVecComp[op]; }
    int getVecType(int op) const { return genericVecType[op]; }
    int getDataType(int op) const { return genericDataType[op]; }
    int getNum() const { return (int)opClass.size(); }
    bool checkIsImage(int op) { return isImage[op]; }
    int getImageType(int op) { return Type[op]; }
    AccessQualifier getAccessQualifier(int op) { return accessQualifier[op]; }

protected:
    std::vector<OperandClass> opClass;
    std::vector<const char*> desc;
    std::vector<int> genericVecType;
    std::vector<int> genericNumVecComp;
    std::vector<int> genericDataType;
    std::vector<bool> isImage;
    std::vector<int> Type;
    std::vector<AccessQualifier> accessQualifier;
};

typedef std::multimap<OperandDataTypeDesc, int> OperandTypeMap;
class BuiltInFunctionParameters {
public:
    BuiltInFunctionParameters() :
        typePresent(true),         // most normal, only exceptions have to be spelled out
        resultPresent(true),       // most normal, only exceptions have to be spelled out
        opName(0),
        opDesc(0),
        printDefaultIdenticalTypesMsg(true),
        extInstClass(OclExInstClassMisc) // the default class
    {}

    void setResultAndType(bool r, bool t)
    {
        resultPresent = r;
        typePresent = t;
    }

    bool hasResult() const { return resultPresent != 0; }
    bool hasType()   const { return typePresent != 0; }

    void pushRoundingModeOperand(const char* d) {
        operands.push(OperandFPRoundingMode, 0, 0, 0, d);
    }
    void pushLiteralNumberOperand(const char* d) {
        operands.push(OperandLiteralNumber, 0, 0, 0, d);
    }

    void pushStringOperand(const char* d) {
        operands.push(OperandId, 0, 0, 0, d);
    }

    void pushVariableIds(const char* d) {
        operands.push(OperandVariableIds, 0, 0, 0, d);
    }

    void pushImageId(int Ty, AccessQualifier qual,  const char* d) {
        operands.pushImage(OperandId, Ty, qual, d);
    }

    void pushOperand(OperandClass oc, int vecType, int numVecComp, int dataType, const char* d, bool printOnlyOpName = false) {
        operands.push(oc, vecType, numVecComp, dataType, d);
        if (!printOnlyOpName)
        {
            updateSameTypeMap(vecType, numVecComp, dataType, operands.getNum()); // operands count start from 1 and not from 0. 0 is reserved for the return value.
        }
    }

    void pushResOperand(OperandClass oc, int vecType, int numVecComp, int dataType, const char* d) {
        resultOperand.push(oc, vecType, numVecComp, dataType, d);
        updateSameTypeMap(vecType, numVecComp, dataType, 0);
    }

    void overrideIdenticalTypesMsg(std::string msg) {
        printDefaultIdenticalTypesMsg = false;
        identicalTypesMsg = msg;
    }

    bool useDefaultIdenticalTypesMsg() {
        return printDefaultIdenticalTypesMsg;
    }

    std::string getNonDefaultIdenticalTypesMsg() {
        return identicalTypesMsg;
    }

    const char* opName;
    const char* opDesc;
    OclExtInstClass extInstClass;
    EnumCaps capabilities;
    BuiltInOperandParameters operands;
    BuiltInOperandParameters resultOperand;
    OperandTypeMap sameType; // bucket together operands which share the same data type (including result operand)

    void addNote(std::string note) {
        notes.push_back(note);
    }

    bool hasNotes() {
        return (bool)(notes.size()>0);
    }

    std::vector<std::string>& getNotes() {
        return notes;
    }

protected:
    void updateSameTypeMap(int vecType, int numVecComp, int dataType, int opIdx) {
        OperandDataTypeDesc dTy;
        dTy.vecType = vecType;
        dTy.numVecComponents = numVecComp;
        dTy.dataType = dataType;

        sameType.insert(std::pair<OperandDataTypeDesc, int>(dTy, opIdx));
    }

protected:
    int typePresent : 1;
    int resultPresent : 1;
    bool printDefaultIdenticalTypesMsg; // by default, all components are considered to have the same type. When false, this message is not printed and instead the message is taken from
    std::string identicalTypesMsg; // this is the message which is printed when printIdentcalTypesMsg is false.
    std::vector<std::string> notes;
};

// Print out the OpenCL common (all spec revisions) documentation.
void PrintOclCommonDoc();

};  // end namespace spv
