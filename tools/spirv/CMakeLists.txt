# Copyright 2015-2026 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.5)

set(CMAKE_INSTALL_PREFIX "install" CACHE STRING "prefix" FORCE)

project(spirv)

set(SPV_HEADERS ../../SPIRV-Headers)
set(SPV_INCLUDE ${SPV_HEADERS}/include/spirv)
set(SPV_BUILD   ${SPV_HEADERS}/tools)

include_directories(${SPV_INCLUDE}
                    ${SPV_BUILD})

set(SOURCES
    ${SPV_BUILD}/buildHeaders/jsonToSpirv.cpp
    main.cpp
    doc.cpp
    printSpec.cpp
    GLSLDoc.cpp
    OclDoc.cpp
    ${SPV_BUILD}/buildHeaders/jsoncpp/dist/jsoncpp.cpp)

set(HEADERS
    ${SPV_INCLUDE}/unified1/spirv.core.grammar.json
    ${SPV_INCLUDE}/unified1/GLSL.std.450.h
    ${SPV_INCLUDE}/unified1/OpenCL.std.h
    ${SPV_BUILD}/buildHeaders/jsonToSpirv.h
    ${SPV_BUILD}/buildHeaders/jsoncpp/dist/json/json.h
    doc.h
    printSpec.h
    GLSLDoc.h
    OclDoc.h)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=c++11)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_definitions(-std=c++11)
endif()

add_executable(spirv ${SOURCES} ${HEADERS})
target_compile_definitions(spirv
  PRIVATE SPIRV_JSON_GRAMMAR_PATH="${CMAKE_CURRENT_SOURCE_DIR}/${SPV_INCLUDE}/unified1/spirv.core.grammar.json"
  PRIVATE  GLSL_JSON_GRAMMAR_PATH="${CMAKE_CURRENT_SOURCE_DIR}/${SPV_INCLUDE}/unified1/extinst.glsl.std.450.grammar.json"
)

install(TARGETS spirv
        RUNTIME DESTINATION bin)
