//===- DataFlowTransformation.h - Data Flow Transformation Obfuscation pass------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------------------------===//
//
// This file contains includes and defines for the data flow transformation pass
//
//===---------------------------------------------------------------------------------------===//

#ifndef _DATAFLOWTRANSFORMATION_INCLUDES_
#define _DATAFLOWTRANSFORMATION_INCLUDES_

// LLVM include
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/Utils/Local.h" // For DemoteRegToStack and DemotePHIToStack
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/CryptoUtils.h"
#include "llvm/Transforms/Obfuscation/Utils.h"

// Namespace
using namespace std;

namespace llvm {
  Pass *createDataFlowTransformation();
  Pass *createDataFlowTransformation(bool flag);
}

#endif