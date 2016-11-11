//===- DataFlowTransformation.cpp - Data Flow Transformation Obfuscation pass----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------------===//
//
// This file implements the data flow transformation pass
//
//===----------------------------------------------------------------------------===//

#include "llvm/Transforms/Obfuscation/DataFlowTransformation.h"

#define DEBUG_TYPE "dataflowtransformation"

using namespace llvm;
