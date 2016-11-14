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

    bool variableCanSplit(Value* V);
    bool variableIsSplit(Value* V);
    Value* parseOperand(Value* V);
    bool instValidForSplit(Instruction &inst);
    bool instValidForMerge(Instruction &inst);
    void splitVariable(bool isVolatile, Instruction &inst);
    void mergeVariable(bool isVolatile, Value* V, Instruction &inst);
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
    varsRegister.clear();
    for (BasicBlock::iterator instIter = bb->begin(); instIter!= bb->end(); ++instIter) {
      bool isVolatile = false;
      Instruction &inst = *instIter;

      if (instValidForSplit(inst)) {
        BinaryOperator *bo = cast<BinaryOperator>(&inst);
        splitVariable(isVolatile, inst);
        IRBuilder<> Builder(&inst);
        Type *ty = bo->getType();

        Value *operand0 = parseOperand(bo->getOperand(0));
        Value *operand1 = parseOperand(bo->getOperand(1));

        if(!(variableIsSplit(operand0) && variableIsSplit(operand1))){
          break;
        }

        switch (inst.getOpcode()) {
          case BinaryOperator::Add: {
//            auto& ctx = getGlobalContext();
//            Type *ty = Type::getInt32Ty(ctx);
//            Type *ty = IntegerType::get(inst.getParent()->getContext(),sizeof(uint32_t)*8);//32bits
            ConstantInt *ten = (ConstantInt *)ConstantInt::get(ty, 10);

            /*
               From https://www.cs.ox.ac.uk/files/2936/RR-10-02.pdf:
               Given variable Z = X + Y, we split A and B as follows:
               X_A = X % 10, X_B = X / 10, Y_A = Y % 10, Y_B = Y / 10
               We then form Z as follows:
               Z_A = (X_A + Y_A) % 10, Z_B = { 10 * (X_B + Y_B) + (X_A + Y_A) } / 10
               Z = 10 * Z_B + Z_A
            */
            typeMap::iterator it = varsRegister.find(operand0);
            Value *xa = Builder.CreateLoad(it->second.first, isVolatile);
            Value *xb = Builder.CreateLoad(it->second.second, isVolatile);

            it = varsRegister.find(operand1);
            Value *ya = Builder.CreateLoad(it->second.first, isVolatile);
            Value *yb = Builder.CreateLoad(it->second.second, isVolatile);

            Value* xaYa = Builder.CreateAdd(xa, ya); // za = xa + ya
            Value* xbYb = Builder.CreateAdd(xb, yb); // zb = xb + yb
            Value* za = Builder.CreateURem(xaYa, ten, "z_a");
            Value* tenXbYb = Builder.CreateMul(xbYb, ten); // 10 * (xb + yb)
            Value* tenXbYbPlusXaYa = Builder.CreateAdd(tenXbYb, xaYa); // 10 * (xb + yb) + (xa + ya)
            Value* zb = Builder.CreateUDiv(tenXbYbPlusXaYa, ten, "z_b"); // {10 * (xb + yb) + (xa + ya)} / 10 => Z_B

            Value* registerZa = Builder.CreateAlloca(ty, nullptr, "z_a");
            Value* registerZb = Builder.CreateAlloca(ty, nullptr, "z_b");

            Builder.CreateStore(za, registerZa, isVolatile);
            Builder.CreateStore(zb, registerZb, isVolatile);

            Value* loadResultA = Builder.CreateLoad(registerZa, isVolatile);

            varsRegister[parseOperand(loadResultA)] = std::make_pair(registerZa, registerZb);

            // We replace all uses of the add result with the register that contains Z_A
            inst.replaceAllUsesWith(loadResultA);

//
//            Value* loadResultB = Builder.CreateLoad(registerZb, isVolatile);
//
//            Value* registerAns = Builder.CreateAlloca(ty, nullptr, "ans");
//            Value* tenZb = Builder.CreateMul(loadResultB, ten);
//            Value* answer = Builder.CreateAdd(tenZb, za);
//            Builder.CreateStore(answer, registerAns, isVolatile);
//            Value* loadAnswer = Builder.CreateLoad(registerAns, isVolatile);
//
//            // the key to access to the variables Z_A and Z_B from the ValueMap is Z_A.
//            varsRegister[loadResultA] = std::make_pair(registerZa, registerZb);
//
//            // We replace all uses of the add result with the register that contains Z_A
//            inst.replaceAllUsesWith(loadAnswer);

            ++VarSplitted;
            break;
          }
          case BinaryOperator::Sub: {
//            auto& ctx = getGlobalContext();
//            Type *ty = Type::getInt32Ty(ctx);
//            Type *ty = IntegerType::get(inst.getParent()->getContext(),sizeof(uint32_t)*8);//32bits
            ConstantInt *ten = (ConstantInt *)ConstantInt::get(ty, 10);

            /*
               Given variable Z = X - Y, we split A and B as follows:
               X_A = X % 10, X_B = X / 10, Y_A = Y % 10, Y_B = Y / 10
               We then form Z as follows:
               Z_A = (X_A - Y_A) % 10, Z_B = { 10 * (X_B - Y_B) + (X_A - Y_A) } / 10
               Z = 10 * Z_B + Z_A

               e.g. X = 25, Y = 15
               xa = 5, xb = 2, ya = 5, yb = 1
               za = 0 mod 10 = 0, zb = 10 * 1 + 0 / 10 = 1
               x = 10 * 1 + 0 = 10, correct

               e.g. X = 15, Y = 25
               xa = 5, xb = 1, ya = 5, yb = 2
               za = 0 mod 10 = 0, zb = 10 * -1 + 0 / 10 = -1
               x = 10 * -1 + 0 = -10

              int max = 2147483647; X
              int min = -2147483648; Y
              xa = 7, xb = 214748364, ya = 8, yb = -214748364
              za = -1 mod 10 = -1, zb = 10 * 429496728 + -1 / 10 = 0 / 10 = -214748364
              z = 10 * -214748364 + -1 = -2147483641

              4294967280
              2147483647
            */
            typeMap::iterator it = varsRegister.find(operand0);
            Value *xa = Builder.CreateLoad(it->second.first, isVolatile);
            Value *xb = Builder.CreateLoad(it->second.second, isVolatile);

            it = varsRegister.find(operand1);
            Value *ya = Builder.CreateLoad(it->second.first, isVolatile);
            Value *yb = Builder.CreateLoad(it->second.second, isVolatile);

            Value* xaYa = Builder.CreateSub(xa, ya); // za = xa - ya
            Value* xbYb = Builder.CreateSub(xb, yb); // zb = xb - yb
            Value* za = Builder.CreateSRem(xaYa, ten, "z_a");
            Value* tenXbYb = Builder.CreateMul(xbYb, ten); // 10 * (xb - yb)
            Value* tenXbYbPlusXaYa = Builder.CreateAdd(tenXbYb, xaYa); // 10 * (xb - yb) + (xa - ya)
            Value* zb = Builder.CreateSDiv(tenXbYbPlusXaYa, ten, "z_b"); // {10 * (xb - yb) + (xa - ya)} / 10 => Z_B

            Value* registerZa = Builder.CreateAlloca(ty, nullptr, "z_a");
            Value* registerZb = Builder.CreateAlloca(ty, nullptr, "z_b");

            Builder.CreateStore(za, registerZa, isVolatile);
            Builder.CreateStore(zb, registerZb, isVolatile);

            Value* loadResultA = Builder.CreateLoad(registerZa, isVolatile);

            varsRegister[parseOperand(loadResultA)] = std::make_pair(registerZa, registerZb);

            // We replace all uses of the add result with the register that contains Z_A
            inst.replaceAllUsesWith(loadResultA);

//            Value* loadResultB = Builder.CreateLoad(registerZb, isVolatile);
//
//            Value* registerAns = Builder.CreateAlloca(ty, nullptr, "ans");
//            Value* tenZb = Builder.CreateMul(loadResultB, ten);
//            Value* answer = Builder.CreateAdd(tenZb, za);
//            Builder.CreateStore(answer, registerAns, isVolatile);
//            Value* loadAnswer = Builder.CreateLoad(registerAns, isVolatile);
//
//            // the key to access to the variables Z_A and Z_B from the ValueMap is Z_A.
//            varsRegister[loadResultA] = std::make_pair(registerZa, registerZb);
//
//            // We replace all uses of the add result with the register that contains Z_A
//            inst.replaceAllUsesWith(loadAnswer);
//
//            ++VarSplitted;
            break;
          }
          default: {
            break;
          }
        }
      } else if (instValidForMerge(inst)) {
        if (isa<StoreInst>(&inst)) {
          Value* operand = parseOperand(inst.getOperand(0));

          if (varsRegister.find(operand) != varsRegister.end()) {
            mergeVariable(false, operand, inst);
          }
        }
      }
    }
  }
  return false;
}

// Implementation of Variable Splitting
// An example integer variable x is split into two variables a and b such that
// xa = x div 10 and xb = x mod 10
void DataTypeObfuscation::splitVariable(bool isVolatile, Instruction &inst) {// Check if it has been split before
  // cout << "no of operands in this inst: " << inst.getNumOperands() << endl;
  for (size_t i = 0; i < inst.getNumOperands(); ++i) {
    // cout << "getting operand\n";
    Value* V = inst.getOperand(i);
    // cout << "got operand: " << endl;
    if (variableCanSplit(V)) { // if valid operand, check if it's integer
      // cout << "is valid\n";
      if (!variableIsSplit(V)) {   // if valid and not split, split
        // cout << "is valid, not split\n";
//                  Type *ty = bo->getType();
//                  Type *ty = V->getType();
        auto& ctx = getGlobalContext();
        Type *ty = Type::getInt32Ty(ctx);
//                  Type *ty = IntegerType::get(inst.getParent()->getContext(),sizeof(uint32_t)*8);//32bits
        ConstantInt *ten = (ConstantInt *)ConstantInt::get(ty, 10);
        // cout << "got ten " << endl;

        IRBuilder<> Builder(&inst);

        // Allocate registers for xa and xb
        Value* registerA = Builder.CreateAlloca(ty, 0, "x_a");
        Value* registerB = Builder.CreateAlloca(ty, 0, "x_b");
        // cout << "allocated registers for xa xb " << endl;

        // Store V in those registers
        Builder.CreateStore(V, registerA, isVolatile);
        Builder.CreateStore(V, registerB, isVolatile);

        Value* loadA = Builder.CreateLoad(registerA, isVolatile);
        Value* loadB = Builder.CreateLoad(registerB, isVolatile);
        // cout << "create load for xa xb " << endl;
        // cout << "load xa: "<< loadA->getValueName() << endl;
        // cout << "load xb: "<< loadB->getValueName() << endl;

        Value* xaModTen = Builder.CreateSRem(loadA, ten); //X_A mod 10
        Value* xbDivTen = Builder.CreateSDiv(loadB, ten); //X_B div 10

        // cout << "xa % 10 "<< xaModTen->getValueName() << endl;
        // cout << "xb / 10 "<< xbDivTen->getValueName() << endl;
        Builder.CreateStore(xaModTen, registerA, isVolatile);
        Builder.CreateStore(xbDivTen, registerB, isVolatile);

        // cout << "create store for xa xb " << endl;

        Value* mapKey = V;

        // Register X_A and X_B associated with V
        this->varsRegister[mapKey] = std::make_pair(registerA, registerB);
      }
    }
  }
}

// Implementation of Variable Merging
// Two variables a and b such that a = x div 10 and b = x mod 10 will be merged
// x = 10 * xb + xa
void DataTypeObfuscation::mergeVariable(bool isVolatile, Value* V, Instruction &inst) {
  typeMap::iterator it;
  BinaryOperator *bo = cast<BinaryOperator>(&inst);
  Type *ty = bo->getType();
  ConstantInt *ten = (ConstantInt *)ConstantInt::get(ty, 10);

  if ((it = varsRegister.find(V)) != varsRegister.end()) {
    IRBuilder<> Builder(&inst);

    Value *xa = Builder.CreateLoad(it->second.first, isVolatile);
    Value *xb = Builder.CreateLoad(it->second.second, isVolatile);
    Value* tenXb = Builder.CreateMul(xb, ten);
    Value* answer = Builder.CreateAdd(tenXb, xa);

    inst.setOperand(0, answer);
  }
}

// Helper functions
bool DataTypeObfuscation::variableCanSplit(Value *V){
  // cout << "check can split\n";
  // cout << V->getType()->isIntegerTy() << endl;
  // cout << V->getType()->isFloatTy() << endl;
  // cout << V->getType()->isFloatingPointTy() << endl;
  return V->getType()->isIntegerTy();
}

bool DataTypeObfuscation::variableIsSplit(Value *V){
  // Check if it exists in dictionary?
  return varsRegister.count(V) == 1;
}

/*
 * ParseOperand is used to make a "clean" key for the ValueMap
 * For instance if the value V is "load i32* %x, align 4" it will return %x and
 * we will use the pointer to %x and not the pointer to load as key for the ValueMap
 */
Value* DataTypeObfuscation::parseOperand(Value* V) {
  if(LoadInst *loadInst = dyn_cast<LoadInst>(V)){
    return loadInst->getPointerOperand();
  } else {
    return V;
  }
}

bool DataTypeObfuscation::instValidForSplit(Instruction &inst) {
  switch(inst.getOpcode()){
    case Instruction::Add:
    case Instruction::Sub:
      return true;
    default:
      return false;
  }
}

bool DataTypeObfuscation::instValidForMerge(Instruction &inst) {
  return isa<TerminatorInst>(&inst) || isa<StoreInst>(&inst) || isa<ReturnInst>(&inst);
}
