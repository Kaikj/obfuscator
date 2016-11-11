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

STATISTIC(Transformed, "Data flow transformed");

namespace {
struct DataFlowTransformation : public FunctionPass {
  static char ID;
  bool flag;

  DataFlowTransformation() : FunctionPass(ID) {}
  DataFlowTransformation(bool flag) : FunctionPass(ID) { this->flag = flag; }

  bool doInitialization(Module &M);
  bool runOnFunction(Function &F);
};
}

char DataFlowTransformation::ID = 0;
static RegisterPass<DataFlowTransformation> X("dataflowtransformation", "Data flow transformation");
