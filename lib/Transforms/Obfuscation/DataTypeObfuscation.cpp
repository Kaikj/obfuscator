//===- DataTypeObfuscation.cpp - Data Type Obfuscation pass----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements operators substitution's pass
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Obfuscation/DataTypeObfuscation.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Intrinsics.h"
#include <llvm/IR/ValueMap.h>
#include <iostream>

#define DEBUG_TYPE "dto"

//#define NUMBER_SPLIT_OBF 1

// Stats
STATISTIC(VarSplitted, "Variables split");

namespace {

  struct DataTypeObfuscation : public FunctionPass {
  public:
    static char ID; // Pass identification, replacement for typeid
//  void (DataTypeObfuscation::*funcSplit[NUMBER_SPLIT_OBF])(BinaryOperator *bo);
    bool flag;

    typedef std::pair<Value*,Value*> typeMapValue;
    typedef ValueMap<Value*,typeMapValue> typeMap;
    typeMap varsRegister;

    DataTypeObfuscation() : FunctionPass(ID) {}

    DataTypeObfuscation(bool flag) : FunctionPass(ID) {
      this->flag = flag;
//    funcSplit[0] = &DataTypeObfuscation::splitVariable;
    }

    bool runOnFunction(Function &F);
    bool dataTypeObfuscate(Function *f);

//void splitVariable(Value* V. Instruction &inst);
    bool variableCanSplit(Value* V);
    bool variableIsSplit(Value* V);
  };
}

char DataTypeObfuscation::ID = 0;
static RegisterPass<DataTypeObfuscation> X("DataTypeObfuscation", "data type obfuscation pass");
Pass *llvm::createDataTypeObfuscation(bool flag) { return new DataTypeObfuscation(flag); }

bool DataTypeObfuscation::runOnFunction(Function &F) {
  Function *tmp = &F;

  // Do we obfuscate
  if (toObfuscate(flag, tmp, "dto")) {
    dataTypeObfuscate(tmp);
  }

  return false;
}

bool DataTypeObfuscation::dataTypeObfuscate(Function *f) {
  Function *tmp = f;
  for (Function::iterator bb = tmp->begin(); bb != tmp->end(); ++bb) {
    for (BasicBlock::iterator instIter = bb->begin(); instIter!= bb->end(); ++instIter) {
      bool isVolatile = false;
      Instruction &inst = *instIter;

      if (inst.isBinaryOp()) {
        switch (inst.getOpcode()) {
          case BinaryOperator::Add: { // can only split on add
            BinaryOperator *bo = cast<BinaryOperator>(&inst);
            // Check if it has been split before
            cout << "no of operands in this inst: " << inst.getNumOperands() << endl;
            for (size_t i = 0; i < inst.getNumOperands(); ++i) {
              cout << "getting operand\n";
              Value *V = inst.getOperand(i);
              cout << "got operand: " << endl;
              if (variableCanSplit(V)) { // if valid operand, check if it's integer
                cout << "is valid\n";
                if (!variableIsSplit(V)) {   // if valid and not split, split
                  cout << "is valid, not split\n";
                  // Split the variable
//                  Type *ty = bo->getType();
//                  Type *ty = V->getType();
                  auto& ctx = getGlobalContext();
                  Type *ty = Type::getInt32Ty(ctx);
//                  Type *ty = IntegerType::get(inst.getParent()->getContext(),sizeof(uint32_t)*8);//32bits
                  ConstantInt *ten = (ConstantInt *)ConstantInt::get(ty, 10);
                  cout << "got ten " << endl;

                  IRBuilder<> Builder(&inst);

                  // Allocate registers for xa and xb
                  Value* registerA = Builder.CreateAlloca(ty, 0, "x_a");
                  Value* registerB = Builder.CreateAlloca(ty, 0, "x_b");
                  cout << "allocated registers for xa xb " << endl;

                  // Store V in those registers
                  Builder.CreateStore(V, registerA, isVolatile);
                  Builder.CreateStore(V, registerB, isVolatile);

                  Value* loadA = Builder.CreateLoad(registerA, isVolatile);
                  Value* loadB = Builder.CreateLoad(registerB, isVolatile);
                  cout << "create load for xa xb " << endl;
                  cout << "load xa: "<< loadA->getValueName() << endl;
                  cout << "load xb: "<< loadB->getValueName() << endl;

                  Value* xaModTen = Builder.CreateURem(loadA, ten); //X_A mod 10
                  Value* xbDivTen = Builder.CreateUDiv(loadB, ten); //X_B div 10

                  cout << "xa % 10 "<< xaModTen->getValueName() << endl;
                  cout << "xb / 10 "<< xbDivTen->getValueName() << endl;
                  Builder.CreateStore(xaModTen, registerA, isVolatile);
                  Builder.CreateStore(xbDivTen, registerB, isVolatile);

                  cout << "create store for xa xb " << endl;

                  Value* mapKey = V;

                  // Register X_A and X_B associated with V
                  varsRegister[mapKey] = std::make_pair(registerA,registerB);
                  // Split the variable end
                }
              }
            }

            IRBuilder<> Builder(&inst);

//            Type *ty = bo->getType();
            auto& ctx = getGlobalContext();
            Type *ty = Type::getInt32Ty(ctx);
//            Type *ty = IntegerType::get(inst.getParent()->getContext(),sizeof(uint32_t)*8);//32bits
            ConstantInt *ten = (ConstantInt *)ConstantInt::get(ty, 10);

            Value *operand0 = bo->getOperand(0);
            Value *operand1 = bo->getOperand(1);

            if(!(variableIsSplit(operand0) && variableIsSplit(operand1))){
              cout << "Error : operands aren't split !\n";
              break;
            }

            typeMap::iterator it = varsRegister.find(operand0);
            Value *xa = Builder.CreateLoad(it->second.first, isVolatile);
            Value *xb = Builder.CreateLoad(it->second.second, isVolatile);

            it = varsRegister.find(operand1);
            Value *ya = Builder.CreateLoad(it->second.first, isVolatile);
            Value *yb = Builder.CreateLoad(it->second.second, isVolatile);

            Value* xaYa = Builder.CreateAdd(xa, ya); // za = xa + ya
            Value* xbYb = Builder.CreateAdd(xb, yb); // zb = xb + yb
            Value* za = Builder.CreateURem(xaYa, ten, "z_a");
            Value* tenXbYb = Builder.CreateMul(xbYb, ten);//10*(X_B+Y_B)
            Value* tenXbYbPlusXaYa = Builder.CreateAdd(tenXbYb, xaYa);//10*(X_B+Y_B)+[X_A+Y_A]
            Value* zb = Builder.CreateUDiv(tenXbYbPlusXaYa, ten, "z_b"); //{ 10*(X_B+Y_B)+[X_A+Y_A] } div 10 => Z_B

            Value* registerZa = Builder.CreateAlloca(ty, nullptr, "z_a");
            Value* registerZb = Builder.CreateAlloca(ty, nullptr, "z_b");

            Builder.CreateStore(za, registerZa, isVolatile);
            Builder.CreateStore(zb, registerZb, isVolatile);

            Value* loadResultA = Builder.CreateLoad(registerZa, isVolatile);
            Value* loadResultB = Builder.CreateLoad(registerZb, isVolatile);

            Value* registerAns = Builder.CreateAlloca(ty, nullptr, "ans");
            Value* tenZb = Builder.CreateMul(loadResultB, ten);
            Value* answer = Builder.CreateAdd(tenZb, za);
            Builder.CreateStore(answer, registerAns, isVolatile);
            Value* loadAnswer = Builder.CreateLoad(registerAns, isVolatile);

            // the key to access to the variables Z_A and Z_B from the ValueMap is Z_A.
            varsRegister[loadResultA] = std::make_pair(registerZa, registerZb);

            // We replace all uses of the add result with the register that contains Z_A
            inst.replaceAllUsesWith(loadAnswer);

            ++VarSplitted;
            break;
          }
          case BinaryOperator::Sub: {
            break;
          }
          default: {
            break;
          }
        }
      }
    }
  }
  return false;
}

// Implementation of Variable Splitting
// An example integer variable x is split into two variables a and b such that
// a = x div 10 and b = x mod 10
//void DataTypeObfuscation::splitVariable(Value* V. Instruction &inst) {
//
//}

// Helper functions
bool DataTypeObfuscation::variableCanSplit(Value *V){
  cout << "check can split\n";
  cout << V->getType()->isIntegerTy() << endl;
  cout << V->getType()->isFloatTy() << endl;
  cout << V->getType()->isFloatingPointTy() << endl;
  return V->getType()->isIntegerTy();
//  if (Constant *a = dyn_cast<Constant>(V)) {
//    cout << "true, it's an int\n";
//    return a->getType()->isIntegerTy();
//  } else {
//    cout << "false\n";
//    return false;
//  }
}

bool DataTypeObfuscation::variableIsSplit(Value *V){
  // Check if it exists in dictionary?
  return varsRegister.count(V) == 1;
}