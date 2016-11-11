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

// Public exposed function
Pass *llvm::createDataFlowTransformation() {
  return new DataFlowTransformation();
}

Pass *llvm::createDataFlowTransformation(bool flag) {
  return new DataFlowTransformation(flag);
}

// Declaration of global array name
Twine globalArrayName = "dataFlowTransArray";

bool DataFlowTransformation::doInitialization(Module &M) {
  // Type definitions
  ArrayType *IntArrayType = ArrayType::get(IntegerType::get(M.getContext(), 32), 10);

  // Global variable definitions
  GlobalVariable *globalArray = new GlobalVariable(
    M,
    IntArrayType,
    false,
    GlobalValue::ExternalLinkage,
    0,
    globalArrayName + "_" + M.getModuleIdentifier()
  );
}
}
