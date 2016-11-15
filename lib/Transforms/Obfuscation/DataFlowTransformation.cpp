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
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/APInt.h"

#define DEBUG_TYPE "dataflowtransformation"

using namespace llvm;

STATISTIC(Transformed, "Data flow transformed");

// static cl::opt<string> FunctionName();

namespace {
struct DataFlowTransformation : public ModulePass {
  static char ID;
  bool flag;

  // Declaration of global array name
  Twine globalArrayBaseName = "dataFlowTransArray";

  DataFlowTransformation() : ModulePass(ID) {}

  bool runOnModule(Module &M);
  void runOnFunction(Function &F);

  Twine getArrayName(Module &M);
};
}

char DataFlowTransformation::ID = 0;
static RegisterPass<DataFlowTransformation> X("dataflowtransformation", "Data flow transformation");
APSInt currentIndex = APSInt(APInt(32, 0));
std::map<APSInt, ConstantInt *> intToIndex;

// Public exposed function
Pass *llvm::createDataFlowTransformation() {
  return new DataFlowTransformation();
}

bool DataFlowTransformation::runOnModule(Module &M) {
  // Type definitions
  ArrayType *IntArrayType_0 = ArrayType::get(IntegerType::get(M.getContext(), 32), 0);

  // Global variable definitions
  StringRef arrayName = getArrayName(M).str();
  M.getOrInsertGlobal(arrayName, IntArrayType_0);
  GlobalVariable *globalArray = M.getNamedGlobal(arrayName);
  globalArray->setConstant(false);
  globalArray->setLinkage(GlobalValue::ExternalLinkage);

  // Set initial values
  ConstantInt *zero = ConstantInt::get(M.getContext(), APInt(32, 0));
  globalArray->setInitializer(zero);

  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F) {
    runOnFunction(*F);
  }

  std::vector<Constant *> v(intToIndex.size(), 0);
  for (std::map<APSInt, ConstantInt *>::iterator it = intToIndex.begin(), ite = intToIndex.end(); it != ite; ++it) {
    // v[*(it->second->getValue().getRawData())] = ConstantInt::get(M.getContext(), it->first);
    v[*(it->second->getValue().getRawData())] = ConstantInt::get(M.getContext(), APSInt(32, 0));
  }

  // Type definitions
  ArrayType *IntArrayType_size = ArrayType::get(IntegerType::get(M.getContext(), 32), intToIndex.size());

  // Global variable definitions
  StringRef newArrayName = (getArrayName(M) + "new").str();
  M.getOrInsertGlobal(newArrayName, IntArrayType_size);
  GlobalVariable *newGlobalArray = M.getNamedGlobal(newArrayName);
  newGlobalArray->setConstant(false);
  newGlobalArray->setLinkage(GlobalValue::ExternalLinkage);

  // Set initial values
  Constant *values = ConstantArray::get(IntArrayType_size, ArrayRef<Constant *>(v));
  newGlobalArray->setInitializer(values);

  globalArray->replaceAllUsesWith(newGlobalArray);
  globalArray->eraseFromParent();

  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F) {
    Function::iterator fIter = F->begin();
    BasicBlock *firstBlock = fIter;

    BasicBlock *newBlock = BasicBlock::Create(F->getContext(), "one", F, firstBlock);
    for (std::map<APSInt, ConstantInt *>::iterator it = intToIndex.begin(), ite = intToIndex.end(); it != ite; ++it) {
      GetElementPtrInst *gepInst = GetElementPtrInst::CreateInBounds(newGlobalArray, ArrayRef<Value *>(std::vector<Value *> {ConstantInt::get(M.getContext(), APInt(32, 0)), it->second}), "pointer" + it->second->getValue().toString(7, false), newBlock);
      new StoreInst(ConstantInt::get(F->getContext(), it->first), gepInst, newBlock);
    }
  }
}

void DataFlowTransformation::runOnFunction(Function &F) {
  ++Transformed;

  for (Function::iterator bb = F.begin(), bbE = F.end(); bb != bbE; ++bb) {
    for (BasicBlock::iterator inst = bb->begin(), instE = bb->end(); inst != instE; ++inst) {
      // There are some instructions which we don't want to mess with
      if (dyn_cast<AllocaInst>(inst)) {
        continue; 
      } else if (dyn_cast<StoreInst>(inst)) {
        continue;
      }

      for (Use *operand = inst->op_begin(), *operandE = inst->op_end(); operand != operandE; ++operand) {
        if (ConstantInt *CI = dyn_cast<ConstantInt>(operand->get())) {
          Module *M = F.getParent();

          // Get the array and current index
          GlobalVariable *globalArray = M->getNamedGlobal(getArrayName(*M).str());
          ConstantInt *arrayIndex = ConstantInt::get(F.getContext(), currentIndex);

          // Manage the actual index that is used
          std::map<APSInt, ConstantInt *>::iterator it;
          bool result;
          std::tie(it, result) = intToIndex.emplace(APSInt(CI->getValue()), arrayIndex);
          if (result) {
            currentIndex++;
          }

          GetElementPtrInst *gepInst = GetElementPtrInst::Create(globalArray, ArrayRef<Value *>(std::vector<Value *> { ConstantInt::get(F.getContext(), APInt(32, 0)), it->second }), "array" + it->second->getValue().toString(10, false), inst);
          LoadInst *loadI = new LoadInst(gepInst, "load" + it->second->getValue().toString(10, false), inst);
          *operand = loadI;

          break;
        } else {
          continue;
        }
      }
    }
  }
}

Twine DataFlowTransformation::getArrayName(Module &M) {
  return globalArrayBaseName + "_" + M.getModuleIdentifier();
}